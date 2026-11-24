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
# fixes ("old-libs" asks for these fixes only: Visual C++ 5.0), "gs" the ones whose C runtime is built with /GS (the security
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
    # C++ kits: "cxx" names the Visual C++ whose C++ ABI the config file
    # pins with -fms-compatibility-version, so that names, class layout,
    # RTTI and exception tables match what that compiler made. They link the
    # C runtime DLL (msvcrt.lib) and, with the C++ library headers, its C++
    # library DLL (msvcprt.lib), as cl /MD does. The DDK ones are Visual
    # C++ 6.0's ABI: their msvcp60.dll and mfc42.dll come from it.
    "w2k-vc6": {
        "name": "Windows 2000 from the Windows Server 2003 SP1 DDK (3790.1830), "
                "C++ with the Visual C++ 6.0 ABI, msvcrt.dll, msvcp60.dll and "
                "mfc42.dll",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.0-msvc",
        "msc": "12.00",
        "include": ["WINDDK/inc/crt", "WINDDK/inc/w2k", "WINDDK/inc/wnet",
                    "WINDDK/inc/mfc42", "WINDDK/inc/atl30"],
        "lib": ["WINDDK/lib/w2k/i386", "WINDDK/lib/crt/i386",
                "WINDDK/lib/mfc/i386", "WINDDK/lib/atl/i386"],
        "gs": True,
        "cxx": "6.0",
    },
    "xp-vc6": {
        "name": "Windows XP from the Windows Server 2003 SP1 DDK (3790.1830), "
                "C++ with the Visual C++ 6.0 ABI, msvcrt.dll, msvcp60.dll and "
                "mfc42.dll",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.1-msvc",
        "msc": "12.00",
        "include": ["WINDDK/inc/crt", "WINDDK/inc/wxp", "WINDDK/inc/wnet",
                    "WINDDK/inc/mfc42", "WINDDK/inc/atl30"],
        "lib": ["WINDDK/lib/wxp/i386", "WINDDK/lib/crt/i386",
                "WINDDK/lib/mfc/i386", "WINDDK/lib/atl/i386"],
        "gs": True,
        "cxx": "6.0",
    },
    "ws03-vc6": {
        "name": "Windows Server 2003 SP1 DDK (3790.1830), x86, C++ with the "
                "Visual C++ 6.0 ABI, msvcrt.dll, msvcp60.dll and mfc42.dll",
        "folder": "5.2.3970.1830 DDK (unpacked)",
        "triple": "i586-pc-windows5.2-msvc",
        "msc": "12.00",
        "include": ["WINDDK/inc/crt", "WINDDK/inc/wnet", "WINDDK/inc/mfc42",
                    "WINDDK/inc/atl30"],
        "lib": ["WINDDK/lib/wnet/i386", "WINDDK/lib/crt/i386",
                "WINDDK/lib/mfc/i386", "WINDDK/lib/atl/i386"],
        "gs": True,
        "intrin-shim": True,
        "cxx": "6.0",
    },
    # Visual C++ itself, installed (the folder holds what setup put in
    # "Microsoft Visual Studio" or "MSDEV"): its headers, which include the
    # Platform SDK of the time, and its libraries, MFC's too. Programs use
    # the C runtime DLL of that release (msvcrt40.dll for 4.0, msvcrt.dll
    # for 4.2 to 6.0, msvcr71.dll for 2003) and its C++ library and MFC
    # DLLs. These kits are not tested with real copies yet.
    "vc40": {
        "name": "Visual C++ 4.0 (msvcrt40.dll, mfc40.dll)",
        "folder": "Visual C++ 4.0",
        "triple": "i486-pc-windows4.0-msvc",
        "msc": "10.00",
        "include": ["MSDEV/INCLUDE", "MSDEV/MFC/INCLUDE"],
        "lib": ["MSDEV/LIB", "MSDEV/MFC/LIB"],
        "old": True,
        "cxx": "4.0",
    },
    "vc42": {
        "name": "Visual C++ 4.2 (msvcrt.dll, mfc42.dll)",
        "folder": "Visual C++ 4.2",
        "triple": "i486-pc-windows4.0-msvc",
        "msc": "10.20",
        "include": ["MSDEV/INCLUDE", "MSDEV/MFC/INCLUDE"],
        "lib": ["MSDEV/LIB", "MSDEV/MFC/LIB"],
        "old": True,
        "cxx": "4.2",
    },
    "vc50": {
        "name": "Visual C++ 5.0 (msvcrt.dll, msvcp50.dll, mfc42.dll)",
        "folder": "Visual C++ 5.0",
        "triple": "i486-pc-windows4.0-msvc",
        "msc": "11.00",
        "include": ["VC/INCLUDE", "VC/MFC/INCLUDE", "VC/ATL/INCLUDE"],
        "lib": ["VC/LIB", "VC/MFC/LIB"],
        "old-libs": True,
        "no-safeseh": True,
        "cxx": "5.0",
    },
    "vc60": {
        "name": "Visual C++ 6.0 (msvcrt.dll, msvcp60.dll, mfc42.dll)",
        "folder": "Visual C++ 6.0",
        "triple": "i586-pc-windows4.0-msvc",
        "msc": "12.00",
        "include": ["VC98/INCLUDE", "VC98/MFC/INCLUDE", "VC98/ATL/INCLUDE"],
        "lib": ["VC98/LIB", "VC98/MFC/LIB"],
        "no-safeseh": True,
        "cxx": "6.0",
    },
    "vc71": {
        "name": "Visual C++ .NET 2003 (msvcr71.dll, msvcp71.dll, mfc71.dll)",
        "folder": "Visual C++ .NET 2003",
        "triple": "i586-pc-windows4.0-msvc",
        "msc": "13.10",
        "include": ["Vc7/include", "Vc7/atlmfc/include",
                    "Vc7/PlatformSDK/Include"],
        "lib": ["Vc7/lib", "Vc7/atlmfc/lib", "Vc7/PlatformSDK/Lib"],
        "cxx": "2003",
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
    # WDM drivers for Windows 98 (and ME, 2000 and later): the 98 DDK's
    # wdm.h and wdm.lib, the import library of the kernel functions NTKERN
    # gives WDM drivers. Check them with check-imports.py --wdm 98.
    "w98-wdm": {
        "name": "Windows 98 DDK, WDM drivers",
        "folder": "4.10 DDK (unpacked)",
        "triple": "i386-pc-windows4.0-msvc",
        "msc": "11.0",
        "include": ["inc/win98", "inc"],
        "lib": ["lib/i386/free"],
        "headers-from": ["nt40", "nt31"],
        "kernel": True,
        "wdm": True,
        "kcrt": ["libcntpr"],
        "old": True,
    },
    # VxD kits: the Windows 9x DDKs' C headers (vmm.h, vxdwraps.h, ...) and
    # VXDWRAPS.CLB, for VxDs built with `clang --config=ms-KIT.cfg -c` and
    # linked with vxd-link.py --kit ms-KIT. "vxd" lists the service sets
    # the headers declare (WIN41SERVICES: Windows 98's). The 98 and ME DDKs
    # come as cabinets: unpack-ms-ddk.py installs them as their setup does.
    "w95-vxd": {
        "name": "Windows 95 DDK, VxDs",
        "folder": "4.0 W95 DDK",
        "triple": "i386-pc-windows4.0-msvc",
        "msc": "10.0",
        "include": ["ddk/INC32"],
        "lib": ["ddk/LIB"],
        "vxd": ["WIN40SERVICES"],
    },
    "w98-vxd": {
        "name": "Windows 98 DDK, VxDs",
        "folder": "4.10 DDK (unpacked)",
        "triple": "i386-pc-windows4.10-msvc",
        "msc": "11.0",
        "include": ["inc/win98"],
        "lib": ["lib/i386/free"],
        "vxd": ["WIN40SERVICES", "WIN403SERVICES", "WIN41SERVICES"],
    },
    "me-vxd": {
        "name": "Windows ME DDK, VxDs",
        "folder": "4.90 DDK (unpacked)",
        "triple": "i386-pc-windows4.90-msvc",
        "msc": "12.0",
        "include": ["inc/win_me"],
        "lib": ["lib/win_me"],
        "vxd": ["WIN40SERVICES", "WIN403SERVICES", "WIN41SERVICES",
                "WIN49SERVICES"],
    },
    # The Windows 3.1 DDK has MASM include files only: inc2h.py makes C
    # headers of their equates and structures (vmm.h, vpicd.h, ...), used
    # with vxd.h and win31-vxd.cfg. There is no VXDWRAPS.CLB: link with
    # vxd-link.py --win31.
    "w31-vxd": {
        "name": "Windows 3.1 DDK, VxDs",
        "folder": "3.1 DDK",
        "include": ["ddk/WIN31/DDK/386/INCLUDE"],
        "lib": [],
        "masm": True,
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
        # cl's __try catches the processor's exceptions (a bad pointer, a
        # division by zero) in any instruction, not just in calls.
        "-fasync-exceptions",
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


def cxx_flags(kit):
    """The flags that make clang use the C++ ABI of the Visual C++ named in
    a C++ kit's "cxx" entry (its _MSC_VER is the kit's "msc")."""
    return [
        # The C runtime DLL: defines _DLL and _MT, as cl /MD did.
        "-fms-runtime-lib=dll",
        # wchar_t is a typedef of unsigned short, as with cl /Zc:wchar_t-
        # (the only kind before Visual C++ 2002, and the default up to
        # 2003): it is mangled as unsigned short (G), as the C++ library
        # and MFC DLLs export it. (C has no wchar_t keyword.)
        "-Xclang -fno-wchar",
    ] + ([
        # Visual C++ 6.0 and before had no operator new[] and delete[]: new
        # T[n] called operator new. The msvcrt.dll of Windows 95, 98 and NT
        # 4.0 does not have them (2000 and ME add them). When the C runtime
        # library has no operator new[], these names take operator new.
        "-Wl,-alternatename:??_U@YAPAXI@Z=??2@YAPAXI@Z",
        "-Wl,-alternatename:??_V@YAXPAX@Z=??3@YAXPAX@Z",
    ] if float(kit["msc"]) < 13 else []) + ([
        # The libraries of Visual C++ 6.0 and before have no SafeSEH tables:
        # /SAFESEH came with Visual C++ .NET 2003 (13.10). The kits of
        # Visual C++ 5.0 and 6.0 say "no-safeseh" ("old" kits have it too).
        "-Wl,-safeseh:no",
    ] if kit.get("no-safeseh") else [])


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
    # cl's volatile has acquire and release semantics on x86 (/volatile:ms,
    # clang's -fms-volatile, the default for the msvc triples), which
    # clang makes atomic accesses; for a 386 those become calls to
    # __sync_lock_test_and_set_4, which no kit has. Without it volatile
    # accesses are plain moves, which x86 processors keep in order; only
    # the compiler may move other memory accesses across them.
    if triple.startswith("i386"):
        lines.append("-fno-ms-volatile")
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
    if kit.get("masm"):
        return import_masm_vxd_kit(name, kitdirs, package)
    if kit.get("vxd"):
        return import_vxd_kit(name, kitdirs, package)
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
    if fix_stdarg(os.path.join(sysroot, "include")):
        print("%s: stdarg.h uses clang's va_start" % name)
    if kit.get("cxx"):
        for n in fix_cxx_headers(os.path.join(sysroot, "include")):
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
    for need in ((("wdm.h" if kit.get("wdm") else "ntddk.h"), "string.h")
                 if kernel else ("windows.h", "stdio.h")):
        if need not in includes:
            print("%s: warning: no %s" % (name, need), file=sys.stderr)
    for need in ((("wdm.lib",) if kit.get("wdm") else
                  ("ntoskrnl.lib", "hal.lib")) if kernel else
                 ("kernel32.lib", "user32.lib")):
        if need not in libs:
            print("%s: warning: no %s" % (name, need), file=sys.stderr)
    extra = []
    if kit.get("old") or kit.get("old-libs"):
        libdir = os.path.join(sysroot, "lib")
        notes = fix_old_libs(libdir, libs, llvm_lib)
        for n in notes:
            print("%s: %s" % (name, n))
    if kit.get("old"):
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
        kern = ["wdm"] if kit.get("wdm") else ["ntoskrnl", "hal"]
        crtlibs = [l for l in kern + kit["kcrt"] if l + ".lib" in libs]
    elif kit.get("cxx"):
        # The C runtime DLL, as cl /MD: its import library, and the C++
        # library's (msvcprt.lib) when a C++ header asks for it.
        crtlibs = ["msvcrt"] if "msvcrt.lib" in libs else []
        extra += cxx_flags(kit)
    else:
        crtlibs = [l[:-4] for l in ("libcmt.lib", "libc.lib")
                   if l in libs][:1]
    if not crtlibs:
        print("%s: warning: no %s: no C runtime" %
              (name, "msvcrt.lib" if kit.get("cxx") else
               "libcmt.lib or libc.lib"), file=sys.stderr)
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
        # clang's 32-bit __try takes _except_handler3, which the NT 3.1
        # DDK's C library does not have (cl 8.0 called _except_handler2):
        # link the package's (libntseh.a, which build-win9x.sh makes).
        kcrt = [os.path.join(sysroot, "lib", l + ".lib")
                for l in kit["kcrt"] if l + ".lib" in libs]
        ntseh = os.path.join(package, "sysroots", "win9x-msvcrt", "lib",
                             "libntseh.a")
        if not kit["triple"].startswith("x86_64") and not any(
                b"__except_handler3" in open(f, "rb").read() for f in kcrt):
            if os.path.exists(ntseh):
                extra.append("-Wl,<CFGDIR>/../sysroots/win9x-msvcrt/lib/"
                             "libntseh.a")
            else:
                print("%s: warning: no _except_handler3 and no "
                      "sysroots/win9x-msvcrt/lib/libntseh.a: __try won't "
                      "link" % name, file=sys.stderr)
    write_cfg(cfg, kit["name"], sysname, kit["triple"], kit["msc"], defines,
              crtlibs, os.name != "nt", extra, kernel)
    print("%s: %d headers, %d libraries -> bin/%s.cfg" %
          (sysname, len(includes), len(libs), sysname))
    return True


# What clang needs changed in the VxD DDKs' headers: the header, and
# (regular expression, replacement) pairs, applied to the copies.
VXD_EMIT = (r"    _asm int 20h \\\n"
            r"    _asm _emit (VXDSVC_##service & 0xff) \\\n"
            r"    _asm _emit (((VXDSVC_##service >> 8) & 0xff){hi}) \\\n"
            r"    _asm _emit ((VXDSVC_##service >> 16) & 0xff) \\\n"
            r"    _asm _emit ((VXDSVC_##service >> 24) & 0xff)\n")
VXD_HEADER_FIXES = {
    "vmm.h": [
        # The service tables are enums made by macros that the headers call
        # with fewer arguments than they take, which cl allows; and the
        # macro that names a service must paste the name before it is
        # expanded (vxdwraps.h defines _HeapAllocate as LCODE__HeapAllocate
        # and the like).
        (r"#define Begin_Service_Table\(device, seg\)",
         r"#define Begin_Service_Table(device, ...)"),
        (r"#define End_Service_Table\(device, seg\)",
         r"#define End_Service_Table(device, ...)"),
        (r"#define Declare_Service\(service, local\) \\\n"
         r"\s*GetVxDServiceOrdinal\(service\),",
         r"#define Declare_Service(service, ...) \\\n    __##service,"),
        # (Windows 95's has no StdCall services.)
        (r"#define Declare_SCService\(service, args, local\) \\\n"
         r"\s*GetVxDServiceOrdinal\(service\),",
         r"#define Declare_SCService(service, ...) \\\n    __##service,",
         "optional"),
        # VxDCall emits the service's number with _emit, which in clang
        # takes numbers, not an enum's constants: take them from vxdsvc.h
        # (of tools/vxd-services.txt), whose VXDSVC_ names are numbers.
        (r"#define VxDCall\(service\)[^\n]*\n(?:[ \t]*_asm[^\n]*\n)*",
         r"#define VXD_NO_SERVICE_DECLS\n#ifndef VXD_WINVER\n"
         r"#define VXD_WINVER 0x7FFF\n#endif\n#include <vxdsvc.h>\n"
         r"#define VxDCall(service) \\\n" + VXD_EMIT.format(hi="") + r"\n"),
        (r"#define VxDJmp\(service\)[^\n]*\n(?:[ \t]*_asm[^\n]*\n)*",
         r"#define VxDJmp(service) \\\n" + VXD_EMIT.format(hi=" | 0x80")
         + r"\n"),
    ],
    "vxdwraps.h": [
        # CURSEG()##_##Name pastes a ")": expand CURSEG() first.
        (r"#define PREPEND\(Name\)\s+CURSEG\(\)##_##Name",
         r"#define PREPEND(Name)       VXD_PREPEND_(CURSEG(), Name)\n"
         r"#define VXD_PREPEND_(s, n)  VXD_PREPEND2_(s, n)\n"
         r"#define VXD_PREPEND2_(s, n) s##_##n"),
    ],
}


def fix_vxd_headers(incdir):
    notes = []
    for hdr, fixes in VXD_HEADER_FIXES.items():
        p = os.path.join(incdir, hdr)
        if not os.path.exists(p):
            notes.append("warning: no %s" % hdr)
            continue
        with open(p, "rb") as f:
            text = f.read().decode("latin-1")
        crlf = "\r\n" in text
        text = text.replace("\r\n", "\n")
        done = need = 0
        for fix in fixes:
            text, n = re.subn(fix[0], fix[1], text)
            if len(fix) < 3:
                need += 1
                done += bool(n)
        if done != need:
            notes.append("warning: %s: %d of %d fixes applied"
                         % (hdr, done, need))
        if crlf:
            text = text.replace("\n", "\r\n")
        with open(p, "wb") as f:
            f.write(text.encode("latin-1"))
    return notes


def import_vxd_kit(name, kitdirs, package):
    """A VxD kit: its headers, fixed for clang, with vxd.h and vxdsvc.h
    from the package's win9x-msvcrt sysroot, its VXDWRAPS.CLB, and a
    config that compiles VxDs as the DDK's makefiles do (cl -Zp -DIS_32);
    vxd-link.py links them."""
    kit = KITS[name]
    base = kitdirs[name]
    incdirs = find(base, kit["include"])
    libdirs = find(base, kit["lib"])
    if len(incdirs) != len(kit["include"]) or len(libdirs) != len(kit["lib"]):
        print("%s: %s is missing some of %s" %
              (name, base, ", ".join(kit["include"] + kit["lib"])),
              file=sys.stderr)
        return False
    ours = os.path.join(package, "sysroots", "win9x-msvcrt", "include")
    for f in ("vxd.h", "vxdsvc.h"):
        if not os.path.exists(os.path.join(ours, f)):
            print("%s: no %s in %s (the package's VxD support)"
                  % (name, f, ours), file=sys.stderr)
            return False
    sysname = "ms-" + name
    sysroot = os.path.join(package, "sysroots", sysname)
    if os.path.exists(sysroot):
        shutil.rmtree(sysroot)
    includes = lower_tree(incdirs, os.path.join(sysroot, "include"),
                          "include")
    for n in fix_vxd_headers(os.path.join(sysroot, "include")):
        print("%s: %s" % (name, n))
    for f in ("vxd.h", "vxdsvc.h"):
        shutil.copyfile(os.path.join(ours, f),
                        os.path.join(sysroot, "include", f))
        if f not in includes:
            includes.append(f)
    libs = []
    os.makedirs(os.path.join(sysroot, "lib"))
    for d in libdirs:
        for f in sorted(os.listdir(d)):
            if f.lower().endswith(".clb") and f.lower() not in libs:
                shutil.copyfile(os.path.join(d, f),
                                os.path.join(sysroot, "lib", f.lower()))
                libs.append(f.lower())
    for need in ("vmm.h", "vxdwraps.h", "basedef.h"):
        if need not in includes:
            print("%s: warning: no %s" % (name, need), file=sys.stderr)
    if "vxdwraps.clb" not in libs:
        print("%s: warning: no vxdwraps.clb" % name, file=sys.stderr)
    includes.sort()
    if os.name != "nt":
        write_vfs(sysroot, includes, libs)
    lines = [
        "# %s (imported with import-ms-sdk.py): compile with -c and link"
        % kit["name"],
        "# with share/clang-dos/vxd-link.py --kit %s" % sysname,
        "--target=%s" % kit["triple"],
        "-nostdlibinc",
        "-fms-compatibility-version=%s" % kit["msc"],
        "-fasm-blocks",
        "-isystem <CFGDIR>/../sysroots/%s/include" % sysname,
        # The DDK's makefiles: cl -Zp (structures packed) -DIS_32.
        "-fpack-struct=1",
        "-DIS_32",
        "-DVXD_MS_DDK",
        # Ring 0: no FPU or SSE registers, no unwind tables.
        "-ffreestanding -fno-builtin",
        "-mno-sse -mno-mmx",
        "-fno-stack-protector",
        "-fno-asynchronous-unwind-tables",
        "-fno-exceptions -fno-rtti",
        "-fms-omit-default-lib",
        "-Wno-microsoft -Wno-pragma-pack -Wno-ignored-pragmas",
        "-Wno-nonportable-include-path",
        "-Wno-gnu-zero-variadic-macro-arguments",
    ]
    # As in write_cfg: a 386 has no atomic exchange clang can use.
    if kit["triple"].startswith("i386"):
        lines.append("-fno-ms-volatile")
    if os.name != "nt":
        lines.append("-vfsoverlay <CFGDIR>/../sysroots/%s/vfs.yaml" % sysname)
    lines += ["-D%s" % d for d in kit["vxd"]]
    os.makedirs(os.path.join(package, "bin"), exist_ok=True)
    with open(os.path.join(package, "bin", sysname + ".cfg"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print("%s: %d headers, %d libraries -> bin/%s.cfg" %
          (sysname, len(includes), len(libs), sysname))
    return True


def import_masm_vxd_kit(name, kitdirs, package):
    """The Windows 3.1 DDK: C headers made from its MASM include files by
    inc2h.py, and a config that adds them to win31-vxd.cfg's."""
    kit = KITS[name]
    incdirs = find(kitdirs[name], kit["include"])
    if len(incdirs) != len(kit["include"]):
        print("%s: %s is missing %s" % (name, kitdirs[name],
                                        ", ".join(kit["include"])),
              file=sys.stderr)
        return False
    if not os.path.exists(os.path.join(package, "bin", "win31-vxd.cfg")):
        print("%s: no bin/win31-vxd.cfg in %s" % (name, package),
              file=sys.stderr)
        return False
    sysname = "ms-" + name
    sysroot = os.path.join(package, "sysroots", sysname)
    if os.path.exists(sysroot):
        shutil.rmtree(sysroot)
    os.makedirs(os.path.join(sysroot, "lib"))
    incs = sorted(os.path.join(d, f) for d in incdirs
                  for f in os.listdir(d) if f.lower().endswith(".inc"))
    here = os.path.dirname(os.path.abspath(__file__))
    r = subprocess.run([sys.executable, os.path.join(here, "inc2h.py"),
                        os.path.join(sysroot, "include")] + incs,
                       capture_output=True, text=True)
    if r.returncode:
        print("%s: inc2h.py failed: %s" % (name, r.stderr), file=sys.stderr)
        return False
    lines = [
        "# %s (imported with import-ms-sdk.py): C headers made from its"
        % kit["name"],
        "# include files. Compile with -c and link with",
        "# share/clang-dos/vxd-link.py --win31",
        "@win31-vxd.cfg",
        "-isystem <CFGDIR>/../sysroots/%s/include" % sysname,
    ]
    with open(os.path.join(package, "bin", sysname + ".cfg"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print("%s: %s -> bin/%s.cfg" % (sysname, r.stderr.strip(), sysname))
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


# The end of stdarg.h. Microsoft's va_start takes the address of the last
# named parameter. That is wrong when clang inlines a variadic function, for
# example CString::Format of MFC 7.1. clang does not inline a function that
# uses its builtins.
STDARG_BUILTINS = """
/* clang-dos: clang's va_start, va_arg and va_end (import-ms-sdk.py). */
#if defined(__clang__) && defined(_M_IX86)
#undef va_start
#undef va_arg
#undef va_end
#define va_start(ap, v) __builtin_va_start(ap, v)
#define va_arg(ap, t) __builtin_va_arg(ap, t)
#define va_end(ap) __builtin_va_end(ap)
#endif
"""


def fix_stdarg(incdir):
    """Add STDARG_BUILTINS to the end of stdarg.h in incdir. Returns True if
    it changed the file."""
    p = os.path.join(incdir, "stdarg.h")
    if not os.path.exists(p):
        return False
    with open(p, "rb") as f:
        data = f.read()
    end = STDARG_BUILTINS
    if b"\r\n" in data:
        end = end.replace("\n", "\r\n")
    with open(p, "wb") as f:
        f.write(data + end.encode("latin-1"))
    return True


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


# What clang needs changed in the C++ library and MFC headers of Visual
# C++ 4.0 to 6.0 (and the DDKs, which have 6.0's): the header, and (regular
# expression, replacement) pairs, applied to the copies. cl had no two-phase
# name lookup, so the stream templates use ios_base's members (in, out,
# unitbuf, oct, ...) without saying ios_base. In a template, clang does not
# look in a base that depends on the template's arguments, and some of the
# names (unitbuf, oct, hex) are also manipulator functions in std, which it
# finds instead.
OPENMODE = r"\b(in|out|trunc|app|ate|binary)\b"
CXX_HEADER_FIXES = {
    "istream": [(r"flags\(\) & skipws", "flags() & ios_base::skipws")],
    "ostream": [
        (r"flags\(\) & unitbuf", "flags() & ios_base::unitbuf"),
        (r"_Bfl == oct \|\| _Bfl == hex",
         "_Bfl == ios_base::oct || _Bfl == ios_base::hex"),
    ],
    # A default argument of an openmode (in fstream and sstream).
    "fstream": [("openmode-defaults", None)],
    "sstream": [("openmode-defaults", None)],
    # "typename const T::type&" is "const typename T::type&", and
    # bind1st and bind2nd make a T::type without "typename".
    "functional": [
        (r"typename const (\w+)::", r"const typename \1::"),
        (r"(?<!typename )\b(_Bfn::\w+_argument_type)\(", r"typename \1("),
    ],
    # "class _CRTIMP _Complex_base<float>;" after the template asks for the
    # DLL's instantiation (Visual C++ 5.0). clang takes it for an explicit
    # specialization that has no definition. The copies drop these lines
    # (Visual C++ 6.0 has none).
    "complex": [
        (r"\nclass _CRTIMP _Complex_base<[\w ]+>;(?=\r?\n)", ""),
    ],
    # In vector<bool>, const_iterator takes an iterator of the vector. There,
    # "iterator" names the std::iterator base of const_iterator. Visual C++
    # 6.0 and before took the vector's class (Visual C++ 4.2 to 6.0).
    "vector": [
        (r"const_iterator\(const iterator& _X\)",
         "const_iterator(const vector::iterator& _X)"),
    ],
    # npos takes a dependent type without "typename" (Visual C++ .NET
    # 2003). The error stops the explicit instantiations of basic_string.
    "xstring": [
        (r"\(basic_string<_Elem, _Traits, _Alloc>::size_type\)\(-1\)",
         "(typename basic_string<_Elem, _Traits, _Alloc>::size_type)(-1)"),
    ],
    # The const members of CPtrList and CObList return a "const void*&"
    # or "const CObject*&" that refers to a "void*" or "CObject*" (MFC 7.1).
    # Visual C++ binds the reference; the copies cast the value.
    "afxcoll.inl": [("const-pointer-refs", None)],
    # Visual C++ 4.2 had no bool: its yvals.h makes one of int, and true
    # and false. In C++, clang has these keywords. The copies keep them for C.
    "yvals.h": [
        (r"(typedef int bool;\s*#if !defined\(false\)\s*#define false\s+0\s*"
         r"#endif\s*#if !defined\(true\)\s*#define true\s+1\s*#endif)",
         r"#ifndef __cplusplus\n\1\n#endif"),
    ],
    # type_info is not in std (Visual C++ 4.2). clang's typeid looks for
    # std::type_info.
    "typeinfo": [
        (r"(?s)\A(?!.*using ::type_info)(.*)(\r?\n)#endif // _INC_TYPEINFO",
         r"\1\2namespace std { using ::type_info; }\2#endif // _INC_TYPEINFO"),
    ],
    # The CTypedPtrList specializations for CObList and CPtrList use TYPE
    # and BASE_CLASS, the parameters of the primary template (MFC of Visual
    # C++ 4.2 to 6.0).
    "afxtempl.h": [("specialization-names", None)],
    # Two operators that MFC 4.2 declares with an implicit int.
    "afxwin1.inl": [
        (r"_AFXWIN_INLINE CMenu::operator(==|!=)",
         r"_AFXWIN_INLINE BOOL CMenu::operator\1"),
    ],
}


def fix_openmode_defaults(text):
    """Qualify the openmode flags in default arguments ("openmode _M = in |
    out") with ios_base::."""
    def qualify(m):
        return m.group(1) + re.sub(r"(?<!::)" + OPENMODE, r"ios_base::\1",
                                   m.group(2))
    return re.sub(r"(openmode\s+\w+\s*=)([^,)]*)", qualify, text)


def fix_specialization_names(text):
    """In each explicit specialization "template<> class CTypedPtrList<B, T>",
    replace TYPE with T and BASE_CLASS with B. Visual C++ 6.0 and before
    found the names of the primary template's parameters there."""
    spec = re.compile(r"(?:template\s*<\s*>\s*)?class\s+CTypedPtrList\s*"
                      r"<\s*(\w+)\s*,\s*(\w+\s*\*)\s*>(.*?)\n\};",
                      re.S)

    def one(m):
        """The specialization m with the names replaced."""
        body = re.sub(r"\bTYPE\b", m.group(2), m.group(3))
        body = re.sub(r"\bBASE_CLASS\b", m.group(1), body)
        return m.group(0).replace(m.group(3), body)
    return spec.sub(one, text)


def fix_const_pointer_refs(text):
    """In each inline function that returns "const T*&" and is const, cast
    the value of each return statement to "const T*&"."""
    fn = re.compile(r"(_AFXCOLL_INLINE const (\w+)\s*\*\s*&[^\n]*\)\s*const"
                    r"[^\n]*\n)(.*?\})", re.S)

    def one(m):
        """The function m with its return values cast."""
        body = re.sub(r"return\s+(?!\(const)([^;]+);",
                      r"return (const %s*&)(\1);" % m.group(2), m.group(3))
        return m.group(1) + body
    return fn.sub(one, text)


# Where the C++ library of Visual C++ 5.0 defines the templates that its
# iosfwd names in "extern template" statements: the class templates by name,
# the function templates by the comment above them in iosfwd.
EXTERN_TEMPLATE_HOMES = {
    "basic_ios": "ios", "basic_streambuf": "streambuf",
    "basic_istream": "istream", "basic_iostream": "istream",
    "basic_ostream": "ostream",
    "basic_stringbuf": "sstream", "basic_istringstream": "sstream",
    "basic_ostringstream": "sstream", "basic_stringstream": "sstream",
    "basic_filebuf": "fstream", "basic_ifstream": "fstream",
    "basic_ofstream": "fstream", "basic_fstream": "fstream",
    "num_get": "xlocnum", "num_put": "xlocnum", "collate": "locale",
    # The comments.
    "istream": "istream", "ostream": "ostream", "string": "string",
    "complex": "complex", "xcomplex": "complex",
}


def move_extern_templates(incdir):
    """Move the "extern template" statements of iosfwd to the ends of the
    headers that define their templates. The iosfwd of Visual C++ 5.0 names
    the instantiations that msvcp50.dll exports before the templates have a
    definition, and names function templates that are not declared yet.
    cl took these as hints. clang needs the definition first. Returns True
    if it moved statements."""
    p = os.path.join(incdir, "iosfwd")
    if not os.path.exists(p):
        return False
    with open(p, "rb") as f:
        text = f.read().decode("latin-1")
    stmt = re.compile(r"^extern template\b[^;]*;[ \t]*\r?\n",
                      re.M)
    section = re.compile(r"^//\s+(\w+)\s*$", re.M)
    moved = {}
    for m in stmt.finditer(text):
        cls = re.match(r"extern template class _CRTIMP\s+(\w+)\s*<[^;&(]*;",
                       m.group(0))
        if cls:
            key = cls.group(1)
        else:
            secs = [s.group(1) for s in section.finditer(text, 0, m.start())]
            key = secs[-1] if secs else ""
        home = EXTERN_TEMPLATE_HOMES.get(key)
        if not home:
            print("warning: no home for %r in iosfwd" % m.group(0)[:60],
                  file=sys.stderr)
            continue
        moved.setdefault(home, []).append(m)
    if not moved:
        return False
    drop = {m.start() for ms in moved.values() for m in ms}
    text = stmt.sub(lambda m: "" if m.start() in drop else m.group(0), text)
    with open(p, "wb") as f:
        f.write(text.encode("latin-1"))
    for home, ms in moved.items():
        guard = "_CLANG_DOS_EXTERN_" + home.upper()
        end = ("\n/* clang-dos: these come from iosfwd (import-ms-sdk.py). */\n"
               "#if defined(_DLL) && !defined(CRTDLL2) && !defined(%s)\n"
               "#define %s\n_STD_BEGIN\n" % (guard, guard) +
               "".join(m.group(0).replace("\r\n", "\n") for m in ms) +
               "_STD_END\n#endif\n")
        hp = os.path.join(incdir, home)
        with open(hp, "ab") as f:
            f.write(end.encode("latin-1"))
    return True


def fix_cxx_headers(incdir):
    """Apply CXX_HEADER_FIXES to the C++ library and MFC headers in incdir.
    Returns notes for the user."""
    notes = []
    for hdr, fixes in CXX_HEADER_FIXES.items():
        p = os.path.join(incdir, hdr)
        if not os.path.exists(p):
            continue
        with open(p, "rb") as f:
            data = f.read()
        text = data.decode("latin-1")
        old = text
        for pat, rep in fixes:
            if pat == "openmode-defaults":
                text = fix_openmode_defaults(text)
            elif pat == "const-pointer-refs":
                text = fix_const_pointer_refs(text)
            elif pat == "specialization-names":
                text = fix_specialization_names(text)
            else:
                text = re.sub(pat, rep, text)
        if text != old:
            with open(p, "wb") as f:
                f.write(text.encode("latin-1"))
            notes.append("fixed %s for clang" % hdr)
    if move_extern_templates(incdir):
        notes.append("moved the extern templates of iosfwd")
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
            print("%-9s %-27s %s\n%9s %s" % (k, v.get("triple", ""), v["name"], "",
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
    if any(KITS[k].get("old") or KITS[k].get("old-libs") for k in names) and not os.path.exists(llvm_lib):
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
