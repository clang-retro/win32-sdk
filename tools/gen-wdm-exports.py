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
        "4.90=$ME/hidparse.sys" "4.10=$L98/stream.lib" \\
        "4.10.2222=$SE/STREAM.SYS" "4.90=$ME/stream.sys" \\
        "4.10=$L98/usbcamd.lib" "4.10=$L98/battc.lib" \\
        "4.10.2222=$SE/BATTC.SYS" "4.90=$ME/battc.sys" \\
        "4.10.2222=$SE/NDIS.VXD" > tools/wdm9x-exports.txt

A WDM driver (a .sys that Windows 98 and ME load as well as Windows 2000
and later) links wdm.lib, which names the functions of the kernel that
NTKERN.VXD gives it on 98 and ME. A WDM audio driver also imports
PORTCLS.SYS (port class) and KS.SYS (kernel streaming), a USB driver
USBD.SYS, and a HID driver HIDCLASS.SYS and HIDPARSE.SYS, whose functions
changed in 98 Second Edition (4.10.2222) and ME: the 98 DDK has their
import libraries, and the 98 SE and ME DDKs the drivers. So do STREAM.SYS
(stream class minidrivers), USBCAMD.SYS (USB cameras; only 98's library)
and BATTC.SYS (batteries). NDIS miniports (.sys) import NDIS.SYS, which on
98 and ME is NDIS.VXD: it loads them and resolves their imports from the
names in its _NdisNames table, which this reads from 98 SE's debug
NDIS.VXD, found with the NDIS.SYM next to it (no other version's is in
the DDKs). This writes one line
per function: the module, the name and the versions whose library or
driver has it, for check-imports.py --wdm. Only the names are taken from
the files.
"""
import argparse
import importlib.util
import os
import struct
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


def le_fixups(d):
    """{(object, offset): (object, offset)} of an LE file's internal 32-bit
    offset fixups: where each pointer in its objects points."""
    le = struct.unpack_from("<I", d, 0x3C)[0]
    psize, = struct.unpack_from("<I", d, le + 0x28)
    npages, = struct.unpack_from("<I", d, le + 0x14)
    fpt = le + struct.unpack_from("<I", d, le + 0x68)[0]
    frt = le + struct.unpack_from("<I", d, le + 0x6C)[0]
    objtab = le + struct.unpack_from("<I", d, le + 0x40)[0]
    nobj, = struct.unpack_from("<I", d, le + 0x44)
    objs = [struct.unpack_from("<IIIIII", d, objtab + 24 * i)
            for i in range(nobj)]
    out = {}
    for p in range(1, npages + 1):
        a, b = struct.unpack_from("<II", d, fpt + 4 * (p - 1))
        obj = pbase = None
        for n, (_, _, _, first, count, _) in enumerate(objs, 1):
            if first <= p < first + count:
                obj, pbase = n, (p - first) * psize
        i, end = frt + a, frt + b
        while i < end:
            src, flags = d[i], d[i + 1]
            i += 2
            if src & 0x20:
                count = d[i]
                i += 1
            else:
                srcs = [struct.unpack_from("<h", d, i)[0]]
                i += 2
            if flags & 3:
                raise ValueError("an imported or entry fixup")
            if flags & 0x40:
                target = struct.unpack_from("<H", d, i)[0]
                i += 2
            else:
                target = d[i]
                i += 1
            toff = 0
            if src & 0xF != 2:  # not a selector
                if flags & 0x10:
                    toff = struct.unpack_from("<I", d, i)[0]
                    i += 4
                else:
                    toff = struct.unpack_from("<H", d, i)[0]
                    i += 2
            if src & 0x20:
                srcs = list(struct.unpack_from("<%dh" % count, d, i))
                i += 2 * count
            for so in srcs:
                if obj is not None:
                    out[(obj, pbase + so)] = (target, toff)
    dpages, = struct.unpack_from("<I", d, le + 0x80)

    def where(obj, off):
        return dpages + (objs[obj - 1][3] - 1) * psize + off
    return out, where


def ndis_vxd_exports(vxd):
    """(ndis.sys, name) for each function 98's NDIS.VXD gives the NDIS
    miniports (.sys) it loads: the names in its _NdisNames table, found
    with the NDIS.SYM next to it."""
    spec = importlib.util.spec_from_file_location(
        "mapsym", os.path.join(HERE, "mapsym.py"))
    ms = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(ms)
    base = os.path.splitext(vxd)[0]
    symfile = next((base + e for e in (".SYM", ".sym")
                    if os.path.exists(base + e)), None)
    if not symfile:
        raise SystemExit("no .SYM next to %s" % vxd)
    with open(symfile, "rb") as f:
        _, segs = ms.read_sym(f.read())
    start = stop = None
    for number, _, syms, _ in segs:
        offs = sorted(syms)
        for k, (off, name) in enumerate(offs):
            if name == "_NdisNames":
                start = number, off
                stop = offs[k + 1][0]
    if start is None:
        raise SystemExit("no _NdisNames in %s" % symfile)
    with open(vxd, "rb") as f:
        d = f.read()
    fixups, where = le_fixups(d)
    out = set()
    obj, off = start
    while off < stop:
        t = fixups.get((obj, off))
        if t:
            o = where(*t)
            out.add(("ndis.sys", d[o:d.index(b"\0", o)].decode("latin-1")))
        off += 4
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("libs", nargs="+", metavar="VERSION=FILE")
    a = ap.parse_args()
    have = {}
    for arg in a.libs:
        version, _, lib = arg.partition("=")
        low = lib.lower()
        for key in (exports(lib) if low.endswith(".sys")
                    else ndis_vxd_exports(lib) if low.endswith(".vxd")
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
