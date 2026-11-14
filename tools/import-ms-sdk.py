#!/usr/bin/env python3
"""Make clang sysroots and --config files from Microsoft Windows SDKs and DDKs.

Microsoft's kits can't be redistributed, so this repository and the
toolchain package hold no Microsoft files. Run this on your own copies of
the kits, once, to add them to an unpacked clang-dos toolchain package:

    import-ms-sdk.py --sdks DIR PACKAGE [KIT...]

DIR holds the kits in the folders KITS below names (--list shows them),
PACKAGE is the toolchain package's folder, and without KITs every kit found
in DIR is imported. --kit-dir KIT=FOLDER gives one kit's folder instead.
For each KIT it writes

    PACKAGE/sysroots/ms-KIT/include/   the kit's headers, names in lower case
    PACKAGE/sysroots/ms-KIT/lib/       its import and C runtime libraries
    PACKAGE/sysroots/ms-KIT/vfs.yaml   a case-insensitive view of both
    PACKAGE/bin/ms-KIT.cfg             the config file

and programs are then built with `clang --config=ms-KIT.cfg`. A kit that has
no C runtime of its own takes one from the kit in its "crt" entry. The
kernel-mode kits (the KIT-ddk ones) build drivers instead: their config
files link the kernel and its C library and make a driver image.

The headers and libraries are copied unchanged except for the names (and
the few mistakes in the DDKs' headers in HEADER_FIXES). On a
case-sensitive file system the overlay in vfs.yaml lets `#include
<Windows.h>` and a library's `/DEFAULTLIB:LIBC` find `windows.h` and
`libc.lib`, as on Windows.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Each kit: what it is, the Windows it targets, its include and library
# folders relative to the kit's folder for each architecture (first match
# wins, so the C runtime headers usually come before the Windows ones),
# the _MSC_VER of the compiler it came with and extra predefined macros.
# "old" marks the kits from before 1997, whose libraries need ms_oldlib's
# fixes, "gs" the ones whose C runtime is built with /GS (the security
# cookie is in bufferoverflowu.lib), "intrin-shim" the ones whose winnt.h
# declares intrinsics differently from clang (see ms-intrin-shim.h).
KITS = {
    "nt31": {
        "name": "Win32 SDK for Windows NT 3.1 (MSDN, October 1994)",
        "folder": "3.1 SDK DDK",
        "triple": "i386-pc-windows3.10-msvc",
        "msc": "8.0",
        "include": ["sdk/MSTOOLS/H"],
        "lib": ["sdk/MSTOOLS/LIB/I386"],
        "old": True,
    },
    "nt351": {
        "name": "Win32 SDK for Windows NT 3.51 (June 1995)",
        "folder": "3.51 SDK",
        "triple": "i386-pc-windows3.51-msvc",
        "msc": "9.0",
        "include": ["sdk/MSTOOLS/INCLUDE", "sdk/MSTOOLS/INCLUDE/CRT"],
        "lib": ["sdk/MSTOOLS/LIB/I386", "sdk/MSTOOLS/LIB/WINNT/I386"],
        # It has C runtime headers but no i386 C runtime library.
        "crt": "nt31",
        "old": True,
    },
    "nt40": {
        "name": "Win32 SDK for Windows NT 4.0 and Windows 95 (August 1996)",
        "folder": "4.0 W95 SDK (expanded)",
        "triple": "i486-pc-windows4.0-msvc",
        "msc": "10.20",
        "include": ["sdk/MSTOOLS/INCLUDE"],
        "lib": ["sdk/MSTOOLS/LIB/I386", "sdk/MSTOOLS/LIB/WINNT/I386"],
        # No C runtime at all: NT 3.1's runs on 95 and NT 4.0 too.
        "crt": "nt31",
        "old": True,
    },
    # The DDKs' inc/w2k and inc/wxp lack a few headers windows.h includes
    # (cderr.h, lzexpand.h, ole.h, winperf.h): inc/wnet comes after them to
    # fill the gaps.
    "w2k": {
        "name": "Windows 2000 from the Windows Server 2003 SP1 DDK (3790.1830)",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.0-msvc",
        "msc": "14.0",
        "include": ["WINDDK/inc/crt", "WINDDK/inc/w2k", "WINDDK/inc/wnet"],
        "lib": ["WINDDK/lib/w2k/i386", "WINDDK/lib/crt/i386"],
        "gs": True,
    },
    "xp": {
        "name": "Windows XP from the Windows Server 2003 SP1 DDK (3790.1830)",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.1-msvc",
        "msc": "14.0",
        "include": ["WINDDK/inc/crt", "WINDDK/inc/wxp", "WINDDK/inc/wnet"],
        "lib": ["WINDDK/lib/wxp/i386", "WINDDK/lib/crt/i386"],
        "gs": True,
    },
    "ws03": {
        "name": "Windows Server 2003 SP1 DDK (3790.1830), x86",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.2-msvc",
        "msc": "14.0",
        "include": ["WINDDK/inc/crt", "WINDDK/inc/wnet"],
        "lib": ["WINDDK/lib/wnet/i386", "WINDDK/lib/crt/i386"],
        "gs": True,
        "intrin-shim": True,
    },
    "ws03-x64": {
        "name": "Windows Server 2003 SP1 DDK (3790.1830), x64",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "x86_64-pc-windows5.2-msvc",
        "msc": "14.0",
        "include": ["WINDDK/inc/crt", "WINDDK/inc/wnet"],
        "lib": ["WINDDK/lib/wnet/amd64", "WINDDK/lib/crt/amd64"],
        "gs": True,
        "intrin-shim": True,
    },
    # Kernel-mode kits: the DDKs' headers and libraries for drivers (.sys),
    # built with `clang --config=ms-KIT.cfg` too. "kcrt" is the kernel's C
    # library (string functions, 64-bit arithmetic, SEH) in the kit.
    "nt31-ddk": {
        "name": "Windows NT 3.1 DDK (MSDN, October 1994), drivers",
        "folder": "3.1 SDK DDK",
        "triple": "i386-pc-windows3.10-msvc",
        "msc": "8.0",
        # The DDK's ntddk.h, then the SDK's C headers (string.h, ...).
        "include": ["sdk/DDK/INC", "sdk/MSTOOLS/H"],
        "lib": ["sdk/DDK/LIB/I386/FREE"],
        "kernel": True,
        "kcrt": ["libcnt"],
        "old": True,
    },
    "nt40-ddk": {
        "name": "Windows NT 4.0 DDK (4.0.1381), drivers",
        "folder": "4.0 DDK",
        "triple": "i386-pc-windows4.0-msvc",
        "msc": "10.20",
        "include": ["ddk/ddk/inc"],
        "lib": ["ddk/ddk/lib/i386/free"],
        # The DDK has neither the SDK's pshpack*.h nor C headers: the Win32
        # SDK's of the same release and NT 3.1's C headers.
        "headers-from": ["nt40", "nt31"],
        "kernel": True,
        "kcrt": ["libcntpr"],
        "old": True,
    },
    "w2k-ddk": {
        "name": "Windows 2000 drivers, from the Windows Server 2003 SP1 DDK "
                "(3790.1830)",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.0-msvc",
        "msc": "14.0",
        "include": ["WINDDK/inc/ddk/w2k", "WINDDK/inc/ddk/wdm/w2k",
                    "WINDDK/inc/w2k", "WINDDK/inc/crt", "WINDDK/inc/wnet"],
        "lib": ["WINDDK/lib/w2k/i386"],
        "kernel": True,
        "kcrt": ["libcntpr"],
        "gs": True,
    },
    "xp-ddk": {
        "name": "Windows XP drivers, from the Windows Server 2003 SP1 DDK "
                "(3790.1830)",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.1-msvc",
        "msc": "14.0",
        "include": ["WINDDK/inc/ddk/wxp", "WINDDK/inc/ddk/wdm/wxp",
                    "WINDDK/inc/wxp", "WINDDK/inc/crt", "WINDDK/inc/wnet"],
        "lib": ["WINDDK/lib/wxp/i386"],
        "kernel": True,
        "kcrt": ["libcntpr"],
        "gs": True,
    },
    "ws03-ddk": {
        "name": "Windows Server 2003 SP1 DDK (3790.1830), x86 drivers",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.2-msvc",
        "msc": "14.0",
        "include": ["WINDDK/inc/ddk/wnet", "WINDDK/inc/ddk/wdm/wnet",
                    "WINDDK/inc/wnet", "WINDDK/inc/crt"],
        "lib": ["WINDDK/lib/wnet/i386"],
        "kernel": True,
        "kcrt": ["libcntpr"],
        "gs": True,
        "intrin-shim": True,
    },
    "ws03-x64-ddk": {
        "name": "Windows Server 2003 SP1 DDK (3790.1830), x64 drivers",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "x86_64-pc-windows5.2-msvc",
        "msc": "14.0",
        "include": ["WINDDK/inc/ddk/wnet", "WINDDK/inc/ddk/wdm/wnet",
                    "WINDDK/inc/wnet", "WINDDK/inc/crt"],
        "lib": ["WINDDK/lib/wnet/amd64"],
        "kernel": True,
        "kcrt": ["libcntpr"],
        "gs": True,
        "intrin-shim": True,
    },
}


def lower_tree(srcs, dst, what):
    """Copy the files of each folder in srcs into dst with lower-case names.
    The first folder that has a file wins."""
    seen = {}
    for src in srcs:
        for root, dirs, files in os.walk(src):
            dirs.sort()
            rel = os.path.relpath(root, src)
            reldst = "" if rel == "." else rel.lower()
            for f in sorted(files):
                if what == "lib" and not f.lower().endswith((".lib", ".obj")):
                    continue
                key = os.path.join(reldst, f.lower())
                if key in seen:
                    continue
                seen[key] = os.path.join(root, f)
            if what == "lib":
                # Only the folder itself: subfolders of a lib folder hold
                # other architectures or checked builds.
                dirs[:] = []
    for key, path in seen.items():
        out = os.path.join(dst, key)
        os.makedirs(os.path.dirname(out), exist_ok=True)
        shutil.copyfile(path, out)
    return sorted(seen)


def vfs_tree(names):
    """The directory entries of a VFS overlay for a list of relative paths."""
    root = {}
    for name in names:
        node = root
        parts = name.split(os.sep)
        for p in parts[:-1]:
            node = node.setdefault(p, {})
        node[parts[-1]] = None

    def entries(node, path):
        out = []
        for k, v in sorted(node.items()):
            p = k if not path else path + "/" + k
            if v is None:
                out.append({"type": "file", "name": k, "external-contents": p})
            else:
                out.append({"type": "directory", "name": k,
                            "contents": entries(v, p)})
        return out
    return entries(root, "")


def write_vfs(sysroot, includes, libs):
    # Names and contents are relative to the overlay's folder, so the
    # package can be moved; matching ignores case.
    roots = [{"type": "directory", "name": "include",
              "contents": vfs_tree(includes)},
             {"type": "directory", "name": "lib",
              "contents": vfs_tree(libs)}]

    def prefix(node, base):
        if node["type"] == "file":
            node["external-contents"] = base + "/" + node["external-contents"]
        else:
            for c in node["contents"]:
                prefix(c, base)
    for r in roots:
        prefix(r, r["name"])
    vfs = {"version": 0, "case-sensitive": "false",
           "overlay-relative": "true", "root-relative": "overlay-dir",
           "roots": roots}
    with open(os.path.join(sysroot, "vfs.yaml"), "w") as f:
        json.dump(vfs, f, indent=1)  # JSON is YAML
        f.write("\n")


def kernel_defines(triple):
    """The macros the DDKs' build utility defines for a driver (from their
    makefile.def and i386mk.inc), for the Windows version in the triple."""
    v = re.search(r"windows(\d+)\.(\d+)", triple)
    ver = "0x%02X%02X" % (int(v.group(1)), int(v.group(2)))
    out = ["_NT1X_=100", "WINNT=1", "_WIN32_WINNT=" + ver, "WINVER=" + ver,
           "WIN32=100", "STD_CALL", "CONDITION_HANDLING=1", "NT_INST=0",
           "NT_UP=1", "DEVL=1"]
    if triple.startswith("x86_64"):
        out += ["_WIN64", "_AMD64_", "AMD64"]
    else:
        out += ["i386=1", "FPO=1"]
    return out


def kernel_flags(triple):
    """Compiler and linker flags for a kernel-mode driver."""
    v = re.search(r"windows(\d+)\.(\d+)", triple)
    ver = "%d.%02d" % (int(v.group(1)), int(v.group(2)))
    out = [
        # Drivers are not C++ programs with exceptions and RTTI (cl's
        # /kernel), and 32-bit ones must not touch the FPU or SSE registers
        # without KeSaveFloatingPointState.
        "-fno-exceptions -fno-rtti",
        # The image: native subsystem, entry point DriverEntry, checksum
        # (the kernel checks it), at the DDK's base address.
        "-Wl,-driver -Xlinker -subsystem:native,%s -Xlinker -osversion:%s"
        % (ver, ver),
        "-Wl,-entry:DriverEntry",
        "-Wl,-base:0x10000 -Wl,-release",
        # The DDK's alloc_text sections: INIT is thrown away after
        # DriverEntry, PAGE* can be paged out. (-Xlinker: -Wl would split
        # INIT,d at the comma.)
        "-Wl,-merge:_PAGE=PAGE -Wl,-merge:_TEXT=.text -Xlinker -section:INIT,d",
    ]
    if not triple.startswith("x86_64"):
        # The DDKs build 32-bit drivers with cl /Gz, and their headers rely
        # on it: dispatch routines and the like are declared without a
        # calling convention, and are __stdcall.
        out.insert(1, "-mno-sse -mno-mmx -mrtd")
    return out


def write_cfg(path, kit, sysname, triple, msc, defines, crtlibs, vfs, extra,
              kernel=False):
    lines = [
        "# %s (imported with import-ms-sdk.py)" % kit,
        "--target=%s" % triple,
        "-fuse-ld=lld",
        "-nostdlibinc",
        "-fms-compatibility-version=%s" % msc,
        "-isystem <CFGDIR>/../sysroots/%s/include" % sysname,
        "-L<CFGDIR>/../sysroots/%s/lib" % sysname,
        # Old headers predate clang's warnings about them, and old programs
        # include them in any case.
        "-Wno-microsoft -Wno-pragma-pack -Wno-ignored-pragmas",
        "-Wno-nonportable-include-path",
        # Link the kit's C runtime, not clang's default libcmt and oldnames,
        # which an old kit may not have. -fms-omit-default-lib keeps them out
        # of the objects and -nostartfiles off the link.
        "-fms-omit-default-lib -nostartfiles" + (" -nostdlib" if kernel
                                                  else ""),
    ]
    if vfs:
        lines.append("-vfsoverlay <CFGDIR>/../sysroots/%s/vfs.yaml" % sysname)
    lines += ["-Xclang --dependent-lib=%s" % l for l in crtlibs]
    lines += ["-D%s" % d for d in defines]
    lines += extra
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")


def find(base, rels):
    """The folders rels under base that exist, matching names without case:
    kits come from CDs with upper-case names and installs with mixed case."""
    out = []
    for rel in rels:
        cur = base
        for part in rel.split("/"):
            try:
                match = [e for e in os.listdir(cur)
                         if e.lower() == part.lower()]
            except OSError:
                match = []
            if not match:
                cur = None
                break
            cur = os.path.join(cur, match[0])
        if cur and os.path.isdir(cur):
            out.append(cur)
    return out


def import_kit(name, kitdirs, package, llvm_lib):
    kit = KITS[name]
    base = kitdirs[name]
    incdirs = find(base, kit["include"])
    libdirs = find(base, kit["lib"])
    if len(incdirs) != len(kit["include"]) or len(libdirs) != len(kit["lib"]):
        print("%s: %s is missing some of %s" %
              (name, base, ", ".join(kit["include"] + kit["lib"])),
              file=sys.stderr)
        return False
    crtlibdirs = []
    if "crt" in kit:
        crt = KITS[kit["crt"]]
        if kit["crt"] not in kitdirs:
            print("%s: needs the C runtime of kit %s" % (name, kit["crt"]),
                  file=sys.stderr)
            return False
        # Its headers come after the kit's own, so they only add the C
        # runtime's, and its libraries come after the kit's.
        incdirs += find(kitdirs[kit["crt"]], crt["include"])
        # A driver takes only the C headers: its C library is the kernel's.
        if not kit.get("kernel"):
            crtlibdirs = find(kitdirs[kit["crt"]], crt["lib"])

    for k in kit.get("headers-from", []):
        # Headers only, after the kit's own.
        if k not in kitdirs:
            print("%s: needs the headers of kit %s" % (name, k),
                  file=sys.stderr)
            return False
        incdirs += find(kitdirs[k], KITS[k]["include"])

    sysname = "ms-" + name
    sysroot = os.path.join(package, "sysroots", sysname)
    if os.path.exists(sysroot):
        shutil.rmtree(sysroot)
    includes = lower_tree(incdirs, os.path.join(sysroot, "include"), "include")
    for n in fix_headers(name, os.path.join(sysroot, "include")):
        print("%s: %s" % (name, n))
    libs = lower_tree(libdirs, os.path.join(sysroot, "lib"), "lib")
    if crtlibdirs:
        # Only the C runtime from the other kit, not its Windows libraries.
        crtonly = set(CRTLIBS) - set(libs)
        tmp = lower_tree(crtlibdirs, os.path.join(sysroot, "crt.tmp"), "lib")
        for f in tmp:
            if f in crtonly:
                os.replace(os.path.join(sysroot, "crt.tmp", f),
                           os.path.join(sysroot, "lib", f))
                libs.append(f)
        shutil.rmtree(os.path.join(sysroot, "crt.tmp"))
        libs.sort()
    kernel = kit.get("kernel")
    for need in ("ntddk.h", "string.h") if kernel else ("windows.h",
                                                         "stdio.h"):
        if need not in includes:
            print("%s: warning: no %s" % (name, need), file=sys.stderr)
    for need in (("ntoskrnl.lib", "hal.lib") if kernel else
                 ("kernel32.lib", "user32.lib")):
        if need not in libs:
            print("%s: warning: no %s" % (name, need), file=sys.stderr)
    extra = []
    if kit.get("old"):
        libdir = os.path.join(sysroot, "lib")
        notes = fix_old_libs(libdir, libs, llvm_lib)
        for n in notes:
            print("%s: %s" % (name, n))
        # The 1993 C runtime predates __int64: take the helpers clang calls
        # for 64-bit division and the like from a newer kit, if there is one.
        ll64 = make_ll64(kitdirs, libdir, llvm_lib)
        if ll64:
            libs.append("ll64.lib")
            extra.append("-Xclang --dependent-lib=ll64")
        else:
            print("%s: warning: no ll64.lib (needs the ws03 kit): 64-bit "
                  "division won't link" % name, file=sys.stderr)
        # Its libcmt asks for no kernel32 and has no SafeSEH tables.
        if not kernel:
            extra.append("-Xclang --dependent-lib=kernel32")
        extra.append("-Wl,-safeseh:no")
    if kit.get("gs"):
        # The security cookie for the C library's /GS checks: the kernel's
        # own for drivers.
        extra.append("-Xclang --dependent-lib=bufferoverflow%s"
                     % ("k" if kernel else "u"))
    if kit.get("intrin-shim"):
        shutil.copyfile(os.path.join(HERE, "ms-intrin-shim.h"),
                        os.path.join(sysroot, "include", SHIM))
        includes.append(SHIM)
        extra.append("-include <CFGDIR>/../sysroots/%s/include/%s"
                     % (sysname, SHIM))
    libs.sort()
    if os.name != "nt":
        write_vfs(sysroot, includes, libs)

    # The C runtime: the multithreaded static one if the kit has it (clang
    # defines _MT as for it), else the single-threaded one. A driver links
    # the kernel and its C library instead.
    if kernel:
        crtlibs = [l for l in ["ntoskrnl", "hal"] + kit["kcrt"]
                   if l + ".lib" in libs]
    else:
        crtlibs = [l[:-4] for l in ("libcmt.lib", "libc.lib")
                   if l in libs][:1]
    if not crtlibs:
        print("%s: warning: no libcmt.lib or libc.lib: no C runtime" % name,
              file=sys.stderr)
    if "oldnames.lib" in libs and not kernel:
        crtlibs.append("oldnames")

    defines = []
    if kit["triple"].startswith(("i386", "i486", "i586", "i686")):
        # Early Win32 headers want the build to say the CPU, as the SDK's
        # makefiles did.
        defines.append("_X86_=1")
    elif kit["triple"].startswith("x86_64"):
        defines.append("_AMD64_=1")
    os.makedirs(os.path.join(package, "bin"), exist_ok=True)
    cfg = os.path.join(package, "bin", sysname + ".cfg")
    if kernel:
        defines += kernel_defines(kit["triple"])
        extra = kernel_flags(kit["triple"]) + extra
    write_cfg(cfg, kit["name"], sysname, kit["triple"], kit["msc"], defines,
              crtlibs, os.name != "nt", extra, kernel)
    print("%s: %d headers, %d libraries -> bin/%s.cfg" %
          (sysname, len(includes), len(libs), sysname))
    return True


# Mistakes in the DDKs' headers that cl accepts and clang does not: the
# header, the text to replace and with what. They are fixed in the copies of
# every kernel-mode kit that has them, never in the kits.
HEADER_FIXES = {
    "ntddk.h": [
        # The 2000 and XP DDKs declare _InterlockedXor with a LONG for the
        # pointer (cl ignores declarations of its intrinsics).
        ("_InterlockedXor (\n    IN OUT LONG volatile Target,",
         "_InterlockedXor (\n    IN OUT LONG volatile *Target,"),
        # Inline assembly that reads a UCHAR field of the KPCR without
        # saying its size (NT 4.0 to XP).
        ("movzx eax, _PCR KPCR.Number", "movzx eax, byte ptr _PCR KPCR.Number"),
    ],
}


def fix_headers(name, incdir):
    notes = []
    if not KITS[name].get("kernel"):
        return notes
    for hdr, fixes in HEADER_FIXES.items():
        p = os.path.join(incdir, hdr)
        if not os.path.exists(p):
            continue
        with open(p, "rb") as f:
            data = f.read()
        crlf = b"\r\n" in data
        text = data.decode("latin-1").replace("\r\n", "\n")
        done = 0
        for old, new in fixes:
            if old in text:
                text = text.replace(old, new)
                done += 1
        if done:
            if crlf:
                text = text.replace("\n", "\r\n")
            with open(p, "wb") as f:
                f.write(text.encode("latin-1"))
            notes.append("fixed %d mistakes in %s" % (done, hdr))
    return notes


HERE = os.path.dirname(os.path.abspath(__file__))
SHIM = "clang-dos-ms-intrin-shim.h"


def fix_old_libs(libdir, libs, llvm_lib):
    """Apply ms_oldlib's fixes to the copied libraries of an old kit."""
    import ms_oldlib
    notes = []
    nhdr = nobj = nimp = 0
    for f in libs:
        if not f.endswith(".lib"):
            continue
        p = os.path.join(libdir, f)
        nhdr += bool(ms_oldlib.fix_archive(p))
        nobj += bool(ms_oldlib.fix_objects(p))
        r = ms_oldlib.convert_import_lib(p, llvm_lib)
        if r and r.startswith("converted"):
            nimp += 1
        elif r:
            notes.append("%s: %s" % (f, r))
    notes.insert(0, "fixed %d archives and %d libraries' objects, converted "
                 "%d import libraries" % (nhdr, nobj, nimp))
    return notes


def make_ll64(kitdirs, libdir, llvm_lib):
    import ms_oldlib
    import tempfile
    for k in ("ws03", "xp"):
        if k not in kitdirs:
            continue
        libcmt = find(kitdirs[k], ["WINDDK/lib/crt/i386"])
        libcmt = [os.path.join(d, f) for d in libcmt for f in os.listdir(d)
                  if f.lower() == "libcmt.lib"]
        if not libcmt:
            continue
        with tempfile.TemporaryDirectory() as tmp:
            objs = ms_oldlib.extract_ll64(libcmt[0], tmp)
            if not objs:
                continue
            out = os.path.join(libdir, "ll64.lib")
            r = subprocess.run([llvm_lib, "/out:" + out] + objs,
                               capture_output=True, text=True)
            if r.returncode == 0:
                return out
    return None


# The C runtime libraries a kit can take from another.
CRTLIBS = ["libc.lib", "libcmt.lib", "oldnames.lib", "crtdll.lib",
           "msvcrt.lib", "setargv.obj", "binmode.obj", "commode.obj",
           "chkstk.obj"]


def complete(name, kitdirs):
    kit = KITS[name]
    want = kit["include"] + kit["lib"]
    return len(find(kitdirs[name], want)) == len(want)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("package", nargs="?", help="toolchain package folder")
    ap.add_argument("kits", nargs="*", metavar="KIT",
                    help="kits to import (default: all found)")
    ap.add_argument("--sdks", help="folder holding the kits")
    ap.add_argument("--kit-dir", action="append", default=[],
                    metavar="KIT=FOLDER", help="one kit's folder")
    ap.add_argument("--llvm-lib", help="llvm-lib to rebuild old import "
                    "libraries with (default: the package's)")
    ap.add_argument("--list", action="store_true", help="list the kits")
    a = ap.parse_args()

    if a.list:
        for k, v in KITS.items():
            print("%-9s %-27s %s\n%9s %s" % (k, v["triple"], v["name"], "",
                                              v["folder"]))
        return 0
    if not a.package:
        ap.error("give PACKAGE, or --list")
    for k in a.kits:
        if k not in KITS:
            ap.error("unknown kit %s (--list shows them)" % k)

    kitdirs = {}
    if a.sdks:
        for k, v in KITS.items():
            d = find(a.sdks, [v["folder"]])
            if d:
                kitdirs[k] = d[0]
    for kd in a.kit_dir:
        k, _, d = kd.partition("=")
        if k not in KITS or not d:
            ap.error("bad --kit-dir %s" % kd)
        kitdirs[k] = d
    # Without KITs, every kit whose folders are all there: some kits share
    # a folder (an SDK and its DDK, a DDK's versions), and a copy of the
    # folder may hold only one of them.
    names = a.kits or [k for k in KITS if k in kitdirs and complete(k, kitdirs)]
    llvm_lib = a.llvm_lib or os.path.join(
        a.package, "bin", "llvm-lib" + (".exe" if os.name == "nt" else ""))
    if any(KITS[k].get("old") for k in names) and not os.path.exists(llvm_lib):
        ap.error("no %s: give --llvm-lib" % llvm_lib)
    if not names:
        ap.error("no kits found: give --sdks or --kit-dir")
    ok = True
    for k in names:
        if k not in kitdirs:
            print("%s: no folder for it" % k, file=sys.stderr)
            ok = False
        elif not import_kit(k, kitdirs, a.package, llvm_lib):
            ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
