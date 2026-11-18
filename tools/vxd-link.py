#!/usr/bin/env python3
"""Link a VxD (an LE virtual device driver for Windows 3.1, 95, 98 and ME)
from objects built with --config=win95-vxd.cfg (or win31-vxd.cfg, or a
config import-ms-sdk.py made from a Microsoft VxD DDK), with JWlink.

    vxd-link.py [--static | --win31] [--name NAME] [--kit ms-KIT]
                [--version 4.10.0.1998 [--description TEXT] ...]
                [--map OUT.MAP] [--sym OUT.SYM]
                -o OUT.VXD FILE.o ... [LIB.a ...]

The VxD exports its Device Descriptor Block, NAME_DDB (VXD_DEVICE(NAME,
...) in <vxd.h>), as ordinal 1; NAME defaults to the output's file name in
upper case. Without --static it is a dynamic VxD, which Windows 95 and
later load at run time (CreateFile("\\\\.\\PATH\\NAME.VXD"), VXDLDR); with
--static, one that Windows 95 and later load at startup from a device=
line in SYSTEM.INI; with --win31, one marked as Windows 3.x marked them,
which Windows 3.1 loads from SYSTEM.INI (build it with win31-vxd.cfg).
--kit links an imported kit's libraries (its VXDWRAPS.CLB). The sysroot's
libvxd.a (memcpy and the like, and the services' stubs) and libgcc.a
(compiler-rt's builtins: 64-bit division) are linked after the inputs.
Code and data in the sections _ITEXT and _IDATA are init code and data,
in _PTEXT and _PDATA pageable (the VXD_INIT_CODE and like attributes of
vxd.h, or the DDK's #pragma VxD_INIT_CODE_SEG and the like).

--version gives the VxD a version resource (VERSIONINFO), which Windows 95
and later show in its Properties and which setup programs compare, with
the strings --description, --company, --product, --copyright and
--product-version: Windows keeps a VxD's resources after its LE image, in
the 16-bit resource format, where the LE header's two Windows resource
fields point.

--sym writes a .SYM file of the VxD's public and static symbols, for
WDEB386 and SoftICE (mapsym.py, from JWlink's map file), with line numbers
for the objects compiled with -g.
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def default(path, *candidates):
    for c in candidates:
        if os.path.exists(c):
            return c
    return path


def pad4(b):
    return b + b"\0" * (-len(b) % 4)


def ver_block(key, value=b"", children=(), text=False):
    """A block of a 16-bit VS_VERSIONINFO: its size, the size of its value
    (for a string, in bytes with its NUL), the key, and the value and the
    children, each aligned to a dword. As Microsoft's resource compiler
    does, a block's size leaves out the padding after its last part."""
    head = pad4(struct.pack("<HH", 0, len(value)) + key.encode("latin-1")
                + b"\0")
    if children:
        body = head + pad4(value) + b"".join(pad4(c) for c in children[:-1])
        body += children[-1]
    else:
        body = head + value
    return struct.pack("<H", len(body)) + body[2:]


def version_resource(version, strings, device_id):
    """A VxD's version resource, as Microsoft's tools write it: a 16-bit
    resource file entry (RT_VERSION, 1) holding a 16-bit VS_VERSIONINFO."""
    parts = [int(x) for x in version.split(".")]
    parts += [0] * (4 - len(parts))
    ms, ls = parts[0] << 16 | parts[1], parts[2] << 16 | parts[3]
    fixed = struct.pack("<13I", 0xFEEF04BD, 0x00010000, ms, ls, ms, ls,
                        0x3F, 0, 0x00010001,  # VOS_DOS_WINDOWS16
                        5,                    # VFT_VXD
                        device_id, 0, 0)
    table = [ver_block(k, v.encode("latin-1") + b"\0")
             for k, v in strings if v]
    info = ver_block("VS_VERSION_INFO", fixed, [
        ver_block("StringFileInfo", b"", [ver_block("040904E4", b"", table)]),
        ver_block("VarFileInfo", b"", [
            ver_block("Translation", struct.pack("<HH", 0x0409, 0x04E4))])])
    return (b"\xff\x10\x00\xff\x01\x00" + struct.pack("<HI", 0x30, len(info))
            + info)


def add_version(path, res):
    """Append RES to the VxD and point the LE header's Windows resource
    fields (offset and size, at 0xB8 of the LE header) at it."""
    with open(path, "r+b") as f:
        data = f.read()
        le, = struct.unpack_from("<I", data, 0x3C)
        if data[le:le + 2] != b"LE":
            sys.exit("%s: not an LE file" % path)
        f.seek(len(data))
        f.write(res)
        f.seek(le + 0xB8)
        f.write(struct.pack("<II", len(data), len(res)))
        return struct.unpack_from("<H", data, le + 0xC0)[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("-o", dest="out", required=True)
    ap.add_argument("--name", help="the VxD's name (NAME_DDB)")
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--static", action="store_true",
                   help="a static VxD (SYSTEM.INI device=) for 95 and later")
    g.add_argument("--win31", action="store_true",
                   help="a VxD for Windows 3.1 (SYSTEM.INI device=)")
    ap.add_argument("--jwlink", help="JWlink (default: the package's)")
    ap.add_argument("--sysroot", help="sysroot with libvxd.a and libgcc.a "
                    "(default: the package's win9x-msvcrt)")
    ap.add_argument("--kit", help="a VxD kit that import-ms-sdk.py made "
                    "(ms-w95-vxd, ...): link its libraries (VXDWRAPS.CLB) "
                    "before the package's")
    ap.add_argument("--map", help="write a map file")
    ap.add_argument("--sym", help="write a .SYM file (the VxD's public "
                    "and static symbols, for WDEB386 and SoftICE: "
                    "mapsym.py)")
    ap.add_argument("--version", help="add a version resource with this "
                    "file version (4.10.0.1998)")
    ap.add_argument("--product-version", help="its product version "
                    "(default: --version)")
    ap.add_argument("--description", help="its FileDescription")
    ap.add_argument("--company", help="its CompanyName")
    ap.add_argument("--product", help="its ProductName")
    ap.add_argument("--copyright", help="its LegalCopyright")
    ap.add_argument("-v", action="store_true", help="show the directives")
    ap.add_argument("inputs", nargs="+")
    a = ap.parse_args()
    # In the package this script is in share/clang-dos/; in the repository,
    # in tools/.
    pkg = os.path.normpath(os.path.join(HERE, "..", ".."))
    jwlink = a.jwlink or default(shutil.which("jwlink") or "jwlink",
                                 os.path.join(pkg, "bin", "jwlink"),
                                 os.path.join(pkg, "bin", "jwlink.exe"))
    sysroot = a.sysroot or os.path.join(pkg, "sysroots", "win9x-msvcrt")
    name = a.name or os.path.splitext(os.path.basename(a.out))[0].upper()
    if len(name) > 8:
        ap.error("a VxD's name has at most 8 characters: %s" % name)
    kind = " static" if a.static else "" if a.win31 else " dynamic"
    lines = ["format windows vxd" + kind,
             "name '%s'" % os.path.abspath(a.out),
             "option modname=%s" % name,
             "option quiet"]
    mapfile = a.map
    if a.sym and not mapfile:
        fd, mapfile = tempfile.mkstemp(suffix=".map")
        os.close(fd)
    if mapfile:
        # With the static symbols, which the .SYM file has too.
        lines += ["option map='%s'" % os.path.abspath(mapfile),
                  "option statics"]
    LIBS = (".a", ".lib", ".clb")
    objs = [f for f in a.inputs if not f.lower().endswith(LIBS)]
    libs = [f for f in a.inputs if f.lower().endswith(LIBS)]
    if a.kit:
        kitlib = os.path.join(os.path.dirname(os.path.abspath(sysroot)),
                              a.kit, "lib")
        if not os.path.isdir(kitlib):
            ap.error("no %s: import the kit with import-ms-sdk.py" % kitlib)
        libs += [os.path.join(kitlib, f) for f in sorted(os.listdir(kitlib))
                 if f.lower().endswith(LIBS)]
    if not objs:
        ap.error("no objects")
    lines += ["file '%s'" % os.path.abspath(f) for f in objs]
    for lib in libs:
        if not os.path.exists(lib):
            ap.error("no %s" % lib)
        lines.append("library '%s'" % os.path.abspath(lib))
    for lib in ("libvxd.a", "libgcc.a"):
        p = os.path.join(sysroot, "lib", lib)
        if not os.path.exists(p):
            ap.error("no %s: give --sysroot" % p)
        lines.append("library '%s'" % os.path.abspath(p))
    # The DDB is the VxD's one export, ordinal 1, with the C name's
    # underscore.
    lines.append("export %s_DDB.1=_%s_DDB" % (name, name))
    # The loader tells the kind of each object from its flags: code that
    # is preloaded is locked code; data must be preloaded and shared to be
    # locked data (preloaded only, VXDLDR refuses it as a bad device file),
    # and the DDB has to be locked.
    lines += ["segment type code preload nondiscardable",
              "segment type data preload shared nondiscardable"]
    with tempfile.NamedTemporaryFile("w", suffix=".lnk", delete=False) as f:
        f.write("\n".join(lines) + "\n")
        directives = f.name
    if a.v:
        print("\n".join(lines))
    try:
        r = subprocess.run([jwlink, "@" + directives])
        if r.returncode == 0 and a.sym:
            # The objects' line numbers too, if they were built with -g.
            r = subprocess.run([sys.executable,
                                os.path.join(HERE, "mapsym.py"), a.out,
                                "--map", mapfile, "--name", name,
                                "-o", a.sym] +
                               ["--obj=" + os.path.abspath(f) for f in objs])
    finally:
        os.unlink(directives)
        if mapfile != a.map:
            os.unlink(mapfile)
    if r.returncode == 0 and a.version:
        with open(a.out, "rb") as f:
            head = f.read(0x400)
        le, = struct.unpack_from("<I", head, 0x3C)
        devid, = struct.unpack_from("<H", head, le + 0xC0)
        add_version(a.out, version_resource(a.version, [
            ("CompanyName", a.company),
            ("FileDescription", a.description),
            ("FileVersion", a.version),
            ("InternalName", name),
            ("LegalCopyright", a.copyright),
            ("OriginalFilename", os.path.basename(a.out).upper()),
            ("ProductName", a.product),
            ("ProductVersion", a.product_version or a.version)], devid))
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
