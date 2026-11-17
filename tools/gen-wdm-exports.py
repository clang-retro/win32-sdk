#!/usr/bin/env python3
"""Make wdm9x-exports.txt from the WDM import libraries of the Windows 98
and ME DDKs.

    gen-wdm-exports.py VERSION=WDM.LIB ... > wdm9x-exports.txt

    gen-wdm-exports.py "4.10=4.10 DDK (unpacked)/lib/i386/free/wdm.lib" \\
        "4.90=4.90 DDK (unpacked)/lib/i386/free/wdm.lib" \\
        > tools/wdm9x-exports.txt

A WDM driver (a .sys that Windows 98 and ME load as well as Windows 2000
and later) links wdm.lib, which names the functions of the kernel that
NTKERN.VXD gives it on 98 and ME. This writes one line per function: the
module, the name and the versions whose wdm.lib has it, for
check-imports.py --wdm. Only the names are taken from the libraries.
"""
import argparse
import importlib.util
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location(
    "gen_kernel_versions", os.path.join(HERE, "gen-kernel-versions.py"))
gkv = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gkv)
sys.path.insert(0, HERE)
import ms_oldlib  # noqa: E402


def imports(lib):
    """(module, name) for each import in a WDM library. The 98 and ME
    libraries are long-format, whose descriptors say which of NTOSKRNL.EXE
    and HAL.DLL each import comes from; a short-format one names it."""
    with open(lib, "rb") as f:
        r = ms_oldlib.analyse_import_lib(f.read())
    if not isinstance(r, tuple):
        return gkv.exports(lib, "ntoskrnl.exe")
    _, descs, imps, _, _ = r
    out = set()
    for sym, (name, ordinal, desc) in imps.items():
        if ordinal is not None:
            continue
        dll = (descs.get(desc) or "ntoskrnl.exe").lower()
        out.add((dll, name or gkv.undecorate(sym[len("__imp_"):])))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("libs", nargs="+", metavar="VERSION=WDM.LIB")
    a = ap.parse_args()
    have = {}
    for arg in a.libs:
        version, _, lib = arg.partition("=")
        for key in imports(lib):
            have.setdefault(key, []).append(version)
    if not have:
        print("no imports found", file=sys.stderr)
        return 1
    print("# WDM functions of Windows 98 and ME: module, name, the versions")
    print("# whose wdm.lib has it. Made by gen-wdm-exports.py.")
    for (module, name), versions in sorted(have.items()):
        print("%s %s %s" % (module, name, " ".join(versions)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
