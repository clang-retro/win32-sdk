#!/usr/bin/env python3
"""Make wdm9x-exports.txt from the WDM import libraries of the Windows 98
and ME DDKs, and the port class, kernel streaming, USB and HID drivers
they ship.

    gen-wdm-exports.py VERSION=FILE ... > wdm9x-exports.txt

    L98="4.10 DDK (unpacked)/lib/i386/free"
    LME="4.90 DDK (unpacked)/lib/i386/free"
    SE=new98/98DDK/DEBUG_WINDOWS98SE/DEBUG
    ME="4.90 DDK (unpacked)/bin/win_me/chk"
    gen-wdm-exports.py "4.10=$L98/wdm.lib" "4.90=$LME/wdm.lib" \\
        "4.10=$L98/portcls.lib" "4.10=$L98/ks.lib" \\
        "4.10.2222=$SE/PORTCLS.SYS" "4.10.2222=$SE/KS.SYS" \\
        "4.90=$ME/portcls.sys" "4.90=$ME/ks.sys" \\
        "4.10=$L98/usbd.lib" "4.10=$L98/hidclass.lib" \\
        "4.10=$L98/hidparse.lib" "4.10.2222=$SE/USBD.SYS" \\
        "4.10.2222=$SE/HIDCLASS.SYS" "4.10.2222=$SE/HIDPARSE.SYS" \\
        "4.90=$ME/usbd.sys" "4.90=$ME/hidclass.sys" \\
        "4.90=$ME/hidparse.sys" > tools/wdm9x-exports.txt

A WDM driver (a .sys that Windows 98 and ME load as well as Windows 2000
and later) links wdm.lib, which names the functions of the kernel that
NTKERN.VXD gives it on 98 and ME. A WDM audio driver also imports
PORTCLS.SYS (port class) and KS.SYS (kernel streaming), a USB driver
USBD.SYS, and a HID driver HIDCLASS.SYS and HIDPARSE.SYS, whose functions
changed in 98 Second Edition (4.10.2222) and ME: the 98 DDK has their
import libraries, and the 98 SE and ME DDKs the drivers. This writes one line
per function: the module, the name and the versions whose library or
driver has it, for check-imports.py --wdm. Only the names are taken from
the files.
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


def exports(sys_file):
    """(module, name) for each function a driver (.sys) exports by name."""
    spec = importlib.util.spec_from_file_location(
        "check_imports", os.path.join(HERE, "check-imports.py"))
    ci = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(ci)
    module = os.path.basename(sys_file).lower()
    return {(module, n) for n in ci.PE(sys_file).exports()
            if not n.startswith("#") and n not in ("DllInitialize",
                                                   "DllUnload")}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("libs", nargs="+", metavar="VERSION=FILE")
    a = ap.parse_args()
    have = {}
    for arg in a.libs:
        version, _, lib = arg.partition("=")
        for key in (exports(lib) if lib.lower().endswith(".sys")
                    else imports(lib)):
            have.setdefault(key, []).append(version)
    if not have:
        print("no imports found", file=sys.stderr)
        return 1
    print("# WDM functions of Windows 98, 98 SE and ME: module, name, the")
    print("# versions whose library or driver has it. Made by")
    print("# gen-wdm-exports.py.")
    for (module, name), versions in sorted(have.items()):
        print("%s %s %s" % (module, name, " ".join(versions)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
