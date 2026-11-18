#!/usr/bin/env python3
"""Write a .SYM file (MAPSYM's format) for a VxD or a driver, for the
Windows 9x debuggers: WDEB386 (/S:FILE.SYM, or .SYM files loaded with
the debug VMM) and SoftICE (its loader takes .SYM files too).

    mapsym.py FILE.sys|FILE.exe|FILE.dll [-o FILE.SYM] [--name NAME]
    mapsym.py FILE.vxd --map FILE.map [-o FILE.SYM] [--name NAME]
    mapsym.py --dump FILE.SYM

A PE image (a WDM or NT driver, a DLL) has its symbols in its COFF symbol
table, which clang's linker keeps unless -s strips it: every function and
variable, static ones too, by section. A VxD (an LE file) has none, so
its symbols come from the map file that vxd-link.py --map (JWlink) wrote,
by object: the public ones and the static ones (vxd-link.py --sym does
this). Each section or
object is a segment of the .SYM, numbered as in the image, with the
symbols' offsets in it; its name is the section's, or for a VxD object
its group's (DGROUP) or first segment's (.text). NAME, the module name
(by default the file's name without its extension, in capitals), is the
one the debugger shows; for a VxD it is the VxD's name.

--dump prints a .SYM file's segments and symbols, MAPSYM's or ours.
Line numbers are not written.
"""
import argparse
import os
import re
import struct
import sys

SEG32 = 0x01  # a segment's symbols have 32-bit offsets


def pe_symbols(data):
    """[(number, name)] of the sections and [(section, offset, name)] of
    the symbols of a PE image, from its COFF symbol table."""
    if data[:2] != b"MZ":
        raise ValueError("not an MZ file")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("not a PE file")
    (_, nsect, _, symptr, nsyms, optsize, _) = struct.unpack_from(
        "<HHIIIHH", data, pe + 4)
    sect = pe + 24 + optsize
    sections = []
    for i in range(nsect):
        name = data[sect + 40 * i:sect + 40 * i + 8].rstrip(b"\0")
        sections.append((i + 1, name.decode("latin-1")))
    if not symptr or not nsyms:
        raise ValueError("no COFF symbol table (linked with -s?)")
    strtab = symptr + 18 * nsyms
    syms = []
    i = 0
    while i < nsyms:
        o = symptr + 18 * i
        raw, value, secnum, _, cls, naux = struct.unpack_from(
            "<8sIhHBB", data, o)
        i += 1 + naux
        if raw[:4] == b"\0\0\0\0":
            off = strtab + struct.unpack_from("<I", raw, 4)[0]
            name = data[off:data.index(b"\0", off)]
        else:
            name = raw.rstrip(b"\0")
        name = name.decode("latin-1")
        # External and static symbols in a section, but not the
        # sections' own (static, with an auxiliary record) or labels.
        if secnum < 1 or cls not in (2, 3) or (cls == 3 and naux):
            continue
        if name.startswith(".") or not name:
            continue
        syms.append((secnum, value, name))
    return sections, syms


def le_map_symbols(path):
    """[(number, name)] of the objects and [(object, offset, name)] of the
    symbols in a JWlink map file (static ones too, with option statics)."""
    objects = {}
    syms = []
    part = None
    with open(path, encoding="latin-1") as f:
        for line in f:
            if "|   Groups   |" in line:
                part = "groups"
            elif "|   Segments   |" in line:
                part = "segments"
            elif "|   Memory Map   |" in line:
                part = "memory"
            elif "|   Libraries Used" in line:
                part = None
            m = re.match(r"(\S+)\s+(?:(\S+)\s+(\S+)\s+)?([0-9a-fA-F]{4}):"
                         r"([0-9a-fA-F]{8})[*+ ]?\s*(\S*)", line)
            if not m or part is None:
                continue
            obj = int(m.group(4), 16)
            if part == "groups":
                objects.setdefault(obj, m.group(1))
            elif part == "segments":
                if m.group(3) and m.group(3) != "AUTO":
                    objects.setdefault(obj, m.group(3))
                else:
                    objects.setdefault(obj, m.group(1))
        f.seek(0)
        part = None
        for line in f:
            if "|   Memory Map   |" in line:
                part = "memory"
            elif "|   Libraries Used" in line:
                part = None
            if part != "memory":
                continue
            m = re.match(r"([0-9a-fA-F]{4}):([0-9a-fA-F]{8})[*+s ]\s*(\S+)",
                         line)
            if m:
                syms.append((int(m.group(1), 16), int(m.group(2), 16),
                             m.group(3)))
    return sorted(objects.items()), syms


def write_sym(module, segments, symbols):
    """The bytes of a .SYM file: module name, [(number, name)] segments,
    [(segment, offset, name)] symbols. Segments without symbols are left
    out."""
    by_seg = {}
    for seg, value, name in symbols:
        by_seg.setdefault(seg, set()).add((value, name[:255]))
    segs = [(n, name) for n, name in segments if n in by_seg]
    module = module.encode("latin-1")[:255]
    maxsym = max([len(n) for s in by_seg.values() for _, n in s] + [0])
    out = bytearray()

    def para():
        while len(out) % 16:
            out.append(0)

    # MAPDEF; its next-map and first-segment pointers are filled in below.
    out += struct.pack("<HBBHHHHHBB", 0, 0, 0, 0, 0, 16 + len(module),
                       len(segs), 0, min(maxsym, 255), len(module))
    out += module
    para()
    starts = []
    for i, (number, name) in enumerate(segs):
        start = len(out)
        starts.append(start)
        if start >= 0x100000:
            raise ValueError("the .SYM file would be over 1 MB")
        syms = sorted(by_seg[number])
        big = any(v > 0xFFFF for v, _ in syms)
        bname = name.encode("latin-1")[:255]
        out += struct.pack("<HHHHHHHBBHBBB", 0, len(syms), 0, number, 0, 0,
                           0, SEG32 if big else 0, 0, 0, 0, 0xFF, len(bname))
        out += bname
        ptrs = []
        for value, sname in syms:
            ptrs.append(len(out) - start)
            b = sname.encode("latin-1")
            out += struct.pack("<I" if big else "<H", value)
            out += bytes([len(b)]) + b
        table = len(out) - start
        if table + 2 * len(ptrs) > 0xFFFF:
            raise ValueError("segment %s: too many symbols for one .SYM "
                             "segment (64 KB)" % name)
        struct.pack_into("<H", out, start + 4, table)
        out += struct.pack("<%dH" % len(ptrs), *ptrs)
        para()
    for i, start in enumerate(starts):
        nxt = starts[i + 1] // 16 if i + 1 < len(starts) else 0
        struct.pack_into("<H", out, start, nxt)
    end = len(out)
    if starts:
        struct.pack_into("<H", out, 12, starts[0] // 16)
    struct.pack_into("<H", out, 0, end // 16)
    # MAPEND: end of the chain, and the format's version, 6.02.
    out += struct.pack("<HBB", 0, 2, 6)
    return bytes(out)


def read_sym(data):
    """(module, [(number, name, [(offset, symbol)])]) of a .SYM file."""
    (_, _, _, _, _, _, nsegs, ppseg, _, mlen) = struct.unpack_from(
        "<HBBHHHHHBB", data, 0)
    module = data[16:16 + mlen].decode("latin-1")
    segs = []
    o = ppseg * 16
    for _ in range(nsegs):
        h = struct.unpack_from("<HHHHHHHBBHBBB", data, o)
        name = data[o + 21:o + 21 + h[12]].decode("latin-1")
        ptrs = struct.unpack_from("<%dH" % h[1], data, o + h[2])
        syms = []
        for p in ptrs:
            q = o + p
            if h[7] & SEG32:
                v = struct.unpack_from("<I", data, q)[0]
                q += 4
            else:
                v = struct.unpack_from("<H", data, q)[0]
                q += 2
            syms.append((v, data[q + 1:q + 1 + data[q]].decode("latin-1")))
        segs.append((h[3], name, syms))
        if not h[0]:
            break
        o = h[0] * 16
    return module, segs


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("image", help="the PE image or VxD, or with --dump the "
                    ".SYM file")
    ap.add_argument("-o", dest="out", help="the .SYM file (default: the "
                    "image's name with .sym)")
    ap.add_argument("--map", help="for a VxD: JWlink's map file")
    ap.add_argument("--name", help="the module name")
    ap.add_argument("--dump", action="store_true",
                    help="print a .SYM file's contents")
    a = ap.parse_args()
    with open(a.image, "rb") as f:
        data = f.read()
    if a.dump:
        module, segs = read_sym(data)
        print("module %s" % module)
        for number, name, syms in segs:
            print("segment %d %s: %d symbols" % (number, name, len(syms)))
            for v, s in syms:
                print("  %04x:%08x %s" % (number, v, s))
        return 0
    try:
        if a.map:
            segments, symbols = le_map_symbols(a.map)
        else:
            segments, symbols = pe_symbols(data)
        name = a.name or os.path.splitext(os.path.basename(a.image))[0]
        sym = write_sym(name.upper(), segments, symbols)
    except (ValueError, struct.error) as e:
        print("mapsym.py: %s: %s" % (a.image, e), file=sys.stderr)
        return 1
    out = a.out or os.path.splitext(a.image)[0] + ".sym"
    with open(out, "wb") as f:
        f.write(sym)
    return 0


if __name__ == "__main__":
    sys.exit(main())
