#!/usr/bin/env python3
"""Make kernel-versions.txt from the import libraries of Microsoft's DDKs.

    gen-kernel-versions.py VERSION=LIBDIR ... > kernel-versions.txt

Each LIBDIR holds the x86 import libraries of one Windows NT version's DDK
(ntoskrnl.lib, hal.lib, ...), oldest version first; the sysroots that
import-ms-sdk.py makes from the DDKs have them in lib/, with the libraries
of the 1994 kit already converted to the short import format:

    gen-kernel-versions.py 3.10=PACKAGE/sysroots/ms-nt31-ddk/lib \\
        4.00=PACKAGE/sysroots/ms-nt40-ddk/lib \\
        5.00=PACKAGE/sysroots/ms-w2k-ddk/lib \\
        5.01=PACKAGE/sysroots/ms-xp-ddk/lib \\
        5.02=PACKAGE/sysroots/ms-ws03-ddk/lib > tools/kernel-versions.txt

It writes one line per function or variable that a driver can import from
the kernel and its main helper modules: the module, the name and the first
of those versions that exports it, for check-imports.py. Only the names are
taken from the libraries.
"""
import argparse
import os
import re
import struct
import sys

# Import library -> the module drivers import from.
MODULES = {
    "ntoskrnl.lib": "ntoskrnl.exe",
    "hal.lib": "hal.dll",
    "ndis.lib": "ndis.sys",
    "scsiport.lib": "scsiport.sys",
    "videoprt.lib": "videoprt.sys",
    "tdi.lib": "tdi.sys",
}


def members(path):
    """Yield the members of an archive (an import library)."""
    with open(path, "rb") as f:
        d = f.read()
    if d[:8] != b"!<arch>\n":
        return
    o = 8
    while o + 60 <= len(d):
        name = d[o:o + 16].rstrip()
        size = int(d[o + 48:o + 58].split()[0])
        body = d[o + 60:o + 60 + size]
        if name not in (b"/", b"//"):
            yield body
        o += 60 + size + (size & 1)


def coff_imp_symbols(obj):
    """The __imp_ symbols that an old-style import object defines."""
    if len(obj) < 20:
        return
    nsyms_ptr, nsyms = struct.unpack_from("<II", obj, 8)
    strtab = nsyms_ptr + 18 * nsyms
    i = 0
    while i < nsyms:
        e = nsyms_ptr + 18 * i
        if e + 18 > len(obj):
            return
        raw = obj[e:e + 8]
        sect, = struct.unpack_from("<h", obj, e + 12)
        cls, naux = obj[e + 16], obj[e + 17]
        if raw[:4] == b"\0\0\0\0":
            off, = struct.unpack_from("<I", raw, 4)
            nm = obj[strtab + off:obj.index(b"\0", strtab + off)]
        else:
            nm = raw.rstrip(b"\0")
        if cls == 2 and sect > 0 and nm.startswith(b"__imp_"):
            yield nm[6:].decode("latin-1")
        i += 1 + naux


def undecorate(name):
    # x86 decoration: _name for cdecl and stdcall, @name for fastcall, @N
    # after stdcall and fastcall names. The modules export the plain names.
    if name[:1] in ("_", "@"):
        name = name[1:]
    return re.sub(r"@\d+$", "", name)


def exports(lib, module):
    """(module, name) for each import in the library. A short import names
    its module; an old-style one (the 1994 kit's DATA imports) is taken as
    the library's own module."""
    out = set()
    for m in members(lib):
        if m[:4] == b"\0\0\xff\xff":
            t, = struct.unpack_from("<H", m, 18)
            sym, dll, exportas = m[20:].split(b"\0")[:3]
            sym, dll = sym.decode("latin-1"), dll.decode("latin-1").lower()
            ntype = (t >> 2) & 7
            if ntype == 0:
                continue  # by ordinal
            if ntype == 4:
                sym = exportas.decode("latin-1")  # the name it exports
            elif ntype == 2:
                sym = sym[1:] if sym[:1] in ("_", "@", "?") else sym
            elif ntype == 3:
                sym = undecorate(sym)
            out.add((dll, sym))
        else:
            for sym in coff_imp_symbols(m):
                out.add((module, undecorate(sym)))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("kits", nargs="+", metavar="VERSION=LIBDIR")
    a = ap.parse_args()
    first = {}
    for kit in a.kits:
        version, _, libdir = kit.partition("=")
        files = {f.lower(): os.path.join(libdir, f) for f in os.listdir(libdir)}
        for lib, module in MODULES.items():
            if lib not in files:
                continue
            for key in exports(files[lib], module):
                if key[0] in MODULES.values():
                    first.setdefault(key, version)
    if not first:
        print("no import libraries found", file=sys.stderr)
        return 1
    for (module, name), version in sorted(first.items()):
        print("%s %s %s" % (module, name, version))
    return 0


if __name__ == "__main__":
    sys.exit(main())
