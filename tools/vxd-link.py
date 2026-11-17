#!/usr/bin/env python3
"""Link a VxD (an LE virtual device driver for Windows 3.1, 95, 98 and ME)
from objects built with --config=win95-vxd.cfg (or win31-vxd.cfg, or a
config import-ms-sdk.py made from a Microsoft VxD DDK), with JWlink.

    vxd-link.py [--static | --win31] [--name NAME] [--kit ms-KIT]
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
    ap.add_argument("--kit", help="a VxD kit that import-ms-sdk.py made "
                    "(ms-w95-vxd, ...): link its libraries (VXDWRAPS.CLB) "
                    "before the package's")
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
    finally:
        os.unlink(directives)
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
