#!/usr/bin/env python3
"""Link a VxD (an LE virtual device driver for Windows 3.1, 95, 98 and ME)
from objects built with --config=win95-vxd.cfg (or win31-vxd.cfg), with
JWlink.

    vxd-link.py [--static | --win31] [--name NAME] -o OUT.VXD FILE.o ...
                [LIB.a ...]

The VxD exports its Device Descriptor Block, NAME_DDB (VXD_DEVICE(NAME,
...) in <vxd.h>), as ordinal 1; NAME defaults to the output's file name in
upper case. Without --static it is a dynamic VxD, which Windows 95 and
later load at run time (CreateFile("\\\\.\\PATH\\NAME.VXD"), VXDLDR); with
--static, one that Windows 95 and later load at startup from a device=
line in SYSTEM.INI; with --win31, one marked as Windows 3.x marked them,
which Windows 3.1 loads from SYSTEM.INI (build it with win31-vxd.cfg). The sysroot's libvxd.a
(memcpy and the like) and libgcc.a (compiler-rt's builtins: 64-bit
division) are linked after the inputs.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def default(path, *candidates):
    for c in candidates:
        if os.path.exists(c):
            return c
    return path


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
    ap.add_argument("--map", help="write a map file")
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
    if a.map:
        lines.append("option map='%s'" % os.path.abspath(a.map))
    objs = [f for f in a.inputs if not f.endswith((".a", ".lib"))]
    libs = [f for f in a.inputs if f.endswith((".a", ".lib"))]
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
    with tempfile.NamedTemporaryFile("w", suffix=".lnk", delete=False) as f:
        f.write("\n".join(lines) + "\n")
        directives = f.name
    if a.v:
        print("\n".join(lines))
    try:
        r = subprocess.run([jwlink, "@" + directives])
    finally:
        os.unlink(directives)
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
