#!/usr/bin/env python3
"""Write a .SYM file (MAPSYM's format) for a VxD or a driver, for the
Windows 9x debuggers: WDEB386 (/S:FILE.SYM, or .SYM files loaded with
the debug VMM) and SoftICE (its loader takes .SYM files too).

    mapsym.py FILE.sys|FILE.exe|FILE.dll [-o FILE.SYM] [--name NAME]
    mapsym.py FILE.vxd --map FILE.map [--obj FILE.o ...] [-o FILE.SYM]
              [--name NAME]
    mapsym.py --dump [--lines] FILE.SYM

A PE image (a WDM or NT driver, a DLL) has its symbols in its COFF symbol
table, which clang's linker keeps unless -s strips it: every function and
variable, static ones too, by section. A VxD (an LE file) has none, so
its symbols come from the map file that vxd-link.py --map (JWlink) wrote,
by object: the public ones and the static ones (vxd-link.py --sym does
this). Each section or object is a segment of the .SYM, numbered as in
the image, with the symbols' offsets in it; its name is the section's, or
for a VxD object its group's (DGROUP) or first segment's (.text). NAME,
the module name (by default the file's name without its extension, in
capitals), is the one the debugger shows; for a VxD it is the VxD's name.

Line numbers come from DWARF line tables (compile with -g): a PE
image's own .debug_line, or for a VxD, whose linker drops the debug
sections, each object's (--obj, the objects the VxD was linked from;
vxd-link.py --sym passes them), placed with the map's symbols. They go in
the .SYM as MAPSYM writes them, a table of code offsets and lines for
each source file in each segment.

--dump prints a .SYM file's segments and symbols, MAPSYM's or ours, and
with --lines its line numbers.
"""
import argparse
import os
import re
import struct
import sys

SEG32 = 0x01  # a segment's symbols have 32-bit offsets
LINES16, LINES32 = 1, 2  # a line table's kind: 16- or 32-bit offsets
MAX_LINES = 8000  # entries in one line table (its offsets are words)


def coff_sections(data, off, nsect, strtab):
    """The section headers of a COFF object or PE image at OFF: dicts with
    name, va, size, raw (file offset), reloc and nreloc."""
    out = []
    for i in range(nsect):
        o = off + 40 * i
        (name, vsize, va, rsize, raw, reloc, _, nreloc, _, _) = \
            struct.unpack_from("<8sIIIIIIHHI", data, o)
        name = name.rstrip(b"\0")
        if name[:1] == b"/" and name[1:].isdigit() and strtab is not None:
            n = strtab + int(name[1:])
            name = data[n:data.index(b"\0", n)]
        out.append({"name": name.decode("latin-1"), "va": va,
                    "size": max(vsize, rsize), "raw": raw,
                    # A PE section's data is padded to the file alignment.
                    "rsize": min(vsize, rsize) if vsize else rsize,
                    "reloc": reloc, "nreloc": nreloc})
    return out


def uleb(d, i):
    v = shift = 0
    while True:
        b = d[i]
        i += 1
        v |= (b & 0x7F) << shift
        shift += 7
        if b < 0x80:
            return v, i


def sleb(d, i):
    v = shift = 0
    while True:
        b = d[i]
        i += 1
        v |= (b & 0x7F) << shift
        shift += 7
        if b < 0x80:
            if b & 0x40:
                v -= 1 << shift
            return v, i


def cstr(d, i):
    j = d.index(b"\0", i)
    return d[i:j].decode("latin-1"), j + 1


def dwarf_lines(d, resolve, line_str=b""):
    """[(segment, offset, file, line)] of the statements (is_stmt rows) in
    the DWARF line tables D (.debug_line). RESOLVE(pos, value) turns the
    address of a DW_LNE_set_address at POS in D into (segment, offset), or
    None."""
    rows = []
    u = 0
    while u + 4 <= len(d):
        length = struct.unpack_from("<I", d, u)[0]
        if not length or length >= 0xFFFFFFF0:
            break  # padding, or 64-bit DWARF (not a 32-bit target's)
        end = u + 4 + length
        version = struct.unpack_from("<H", d, u + 4)[0]
        i = u + 6
        if version >= 5:
            i += 2  # address and segment selector sizes
        hlen = struct.unpack_from("<I", d, i)[0]
        prog = i + 4 + hlen
        i += 4
        min_inst = d[i]
        i += 1
        if version >= 4:
            i += 1  # maximum operations per instruction
        default_stmt = d[i]
        i += 1
        line_base = struct.unpack_from("<b", d, i)[0]
        line_range, opbase = d[i + 1], d[i + 2]
        lengths = d[i + 3:i + 2 + opbase]
        i += 2 + opbase
        dirs, files = [], []
        if version < 5:
            while d[i]:
                name, i = cstr(d, i)
                dirs.append(name)
            i += 1
            while d[i]:
                name, i = cstr(d, i)
                di, i = uleb(d, i)
                _, i = uleb(d, i)
                _, i = uleb(d, i)
                files.append((name, di))
            # File 1 is the first; directory 1 the first (0: the
            # compilation's).
            files = [None] + files
            dirs = [""] + dirs
        else:
            def entries(i):
                n = d[i]
                i += 1
                fmt = []
                for _ in range(n):
                    t, i = uleb(d, i)
                    f, i = uleb(d, i)
                    fmt.append((t, f))
                count, i = uleb(d, i)
                out = []
                for _ in range(count):
                    e = {}
                    for t, f in fmt:
                        if f == 0x08:
                            v, i = cstr(d, i)
                        elif f in (0x1F, 0x0E):
                            o = struct.unpack_from("<I", d, i)[0]
                            i += 4
                            v = cstr(line_str, o)[0] if f == 0x1F and \
                                o < len(line_str) else ""
                        elif f == 0x0F:
                            v, i = uleb(d, i)
                        elif f in (0x0B, 0x05, 0x06, 0x07, 0x1E):
                            n = {0x0B: 1, 0x05: 2, 0x06: 4, 0x07: 8,
                                 0x1E: 16}[f]
                            v = int.from_bytes(d[i:i + n], "little")
                            i += n
                        elif f == 0x09:
                            n, i = uleb(d, i)
                            v, i = None, i + n
                        else:
                            raise ValueError("DWARF form %#x in a line "
                                             "table" % f)
                        e[t] = v
                    out.append(e)
                return out, i
            ds, i = entries(i)
            fs, i = entries(i)
            dirs = [e.get(1, "") for e in ds]
            files = [(e.get(1, ""), e.get(2, 0)) for e in fs]

        def path(n):
            if n >= len(files) or files[n] is None:
                return "?"
            name, di = files[n]
            if name.startswith(("/", "\\")) or name[1:2] == ":" or \
                    not di or di >= len(dirs) or not dirs[di]:
                return name
            return dirs[di].rstrip("/\\") + "/" + name
        i = prog
        where, off, fileno, line, stmt = None, 0, 1, 1, default_stmt
        while i < end:
            op = d[i]
            i += 1
            if op >= opbase:
                adj = op - opbase
                off += adj // line_range * min_inst
                line += line_base + adj % line_range
                if where is not None and line > 0 and stmt:
                    rows.append((where, off, path(fileno), line))
            elif op == 0:
                n, i = uleb(d, i)
                sub = d[i]
                if sub == 1:
                    where, off, fileno, line = None, 0, 1, 1
                    stmt = default_stmt
                elif sub == 2:
                    value = int.from_bytes(d[i + 1:i + n], "little")
                    r = resolve(i + 1, value)
                    where, off = (r if r else (None, 0))
                i += n
            elif op == 1:
                if where is not None and line > 0 and stmt:
                    rows.append((where, off, path(fileno), line))
            elif op == 2:
                n, i = uleb(d, i)
                off += n * min_inst
            elif op == 3:
                n, i = sleb(d, i)
                line += n
            elif op == 4:
                fileno, i = uleb(d, i)
            elif op == 6:
                stmt = not stmt
            elif op == 8:
                off += (255 - opbase) // line_range * min_inst
            elif op == 9:
                off += struct.unpack_from("<H", d, i)[0]
                i += 2
            else:
                for _ in range(lengths[op - 1]):
                    _, i = uleb(d, i)
        u = end
    return rows


def pe_lines(data):
    """The line rows of a PE image's own DWARF (.debug_line)."""
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    (_, nsect, _, symptr, nsyms, optsize, _) = struct.unpack_from(
        "<HHIIIHH", data, pe + 4)
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    strtab = symptr + 18 * nsyms if symptr else None
    secs = coff_sections(data, pe + 24 + optsize, nsect, strtab)
    found = {s["name"]: s for s in secs}
    if ".debug_line" not in found:
        return []
    line = found[".debug_line"]
    d = data[line["raw"]:line["raw"] + line["rsize"]]
    ls = found.get(".debug_line_str")
    lstr = data[ls["raw"]:ls["raw"] + ls["rsize"]] if ls else b""

    def resolve(pos, va):
        for n, s in enumerate(secs, 1):
            if s["va"] <= va - base < s["va"] + s["size"]:
                return n, va - base - s["va"]
        return None
    return dwarf_lines(d, resolve, lstr)


def obj_lines(path, placed):
    """The line rows of a COFF object's DWARF, placed in the VxD with
    PLACED, the map's {symbol: (object, offset)} for this object."""
    with open(path, "rb") as f:
        data = f.read()
    (_, nsect, _, symptr, nsyms, optsize, _) = struct.unpack_from(
        "<HHIIIHH", data, 0)
    strtab = symptr + 18 * nsyms
    secs = coff_sections(data, 20 + optsize, nsect, strtab)
    syms = {}
    base = {}
    i = 0
    while i < nsyms:
        raw, value, secnum, _, _, naux = struct.unpack_from(
            "<8sIhHBB", data, symptr + 18 * i)
        if raw[:4] == b"\0\0\0\0":
            n = strtab + struct.unpack_from("<I", raw, 4)[0]
            name = data[n:data.index(b"\0", n)].decode("latin-1")
        else:
            name = raw.rstrip(b"\0").decode("latin-1")
        syms[i] = (secnum, value)
        # Where the linker put each section, from a symbol in it that the
        # map lists.
        if secnum > 0 and name in placed and secnum not in base:
            obj, off = placed[name]
            base[secnum] = (obj, off - value)
        i += 1 + naux
    found = {s["name"]: s for s in secs}
    if ".debug_line" not in found:
        return []
    line = found[".debug_line"]
    d = data[line["raw"]:line["raw"] + line["rsize"]]
    relocs = {}
    for k in range(line["nreloc"]):
        va, sym, typ = struct.unpack_from("<IIH", data, line["reloc"] + 10 * k)
        relocs[va] = sym
    ls = found.get(".debug_line_str")
    lstr = data[ls["raw"]:ls["raw"] + ls["rsize"]] if ls else b""

    def resolve(pos, value):
        if pos not in relocs or relocs[pos] not in syms:
            return None
        secnum, symval = syms[relocs[pos]]
        if secnum not in base:
            return None
        obj, off = base[secnum]
        return obj, off + symval + value
    return dwarf_lines(d, resolve, lstr)


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
    """[(number, name)] of the objects, [(object, offset, name)] of the
    symbols in a JWlink map file (static ones too, with option statics),
    and {module: {name: (object, offset)}}, each input file's."""
    objects = {}
    syms = []
    modules = {}
    module = {}
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
            if line.startswith("Module: "):
                name = line[8:].rstrip("\r\n")
                if name.endswith(")") and "(" in name:
                    name = name[:name.rindex("(")]
                module = modules.setdefault(os.path.normpath(name), {})
                continue
            m = re.match(r"([0-9a-fA-F]{4}):([0-9a-fA-F]{8})[*+s ]\s*(\S+)",
                         line)
            if m:
                at = int(m.group(1), 16), int(m.group(2), 16)
                syms.append(at + (m.group(3),))
                module[m.group(3)] = at
    return sorted(objects.items()), syms, modules


def write_sym(module, segments, symbols, lines=()):
    """The bytes of a .SYM file: module name, [(number, name)] segments,
    [(segment, offset, name)] symbols and [(segment, offset, file, line)]
    line numbers. Segments with neither are left out."""
    by_seg = {}
    for seg, value, name in symbols:
        by_seg.setdefault(seg, set()).add((value, name[:255]))
    by_file = {}
    for seg, off, path, line in lines:
        # The last row for an offset is the one that holds.
        by_file.setdefault(seg, {}).setdefault(path, {})[off] = line
    segs = [(n, name) for n, name in segments
            if n in by_seg or n in by_file]
    for n, _ in segs:
        by_seg.setdefault(n, set())
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
        # A line table for each source file (more than one for a long
        # one), chained by paragraph.
        prev = None
        for path in sorted(by_file.get(number, {})):
            rows = sorted(by_file[number][path].items())
            bpath = path.encode("latin-1", "replace")[:255]
            for k in range(0, len(rows), MAX_LINES):
                part = rows[k:k + MAX_LINES]
                here = len(out)
                if here >= 0x100000:
                    raise ValueError("the .SYM file would be over 1 MB")
                if prev is None:
                    struct.pack_into("<H", out, start + 16, here // 16)
                else:
                    struct.pack_into("<H", out, prev, here // 16)
                prev = here
                kind = LINES32 if part[-1][0] > 0xFFFF else LINES16
                out += struct.pack("<HHHHHB", 0, 0, 12 + len(bpath), kind,
                                   len(part), len(bpath))
                out += bpath + b"\0"
                for off, line in part:
                    out += struct.pack("<IH" if kind == LINES32 else "<HH",
                                       off, min(line, 0xFFFF))
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


def read_lines(data, linedef):
    """[(file, [(offset, line)])] of a segment's line tables."""
    out = []
    o = linedef * 16
    while o:
        nxt, _, plines, kind, count, flen = struct.unpack_from(
            "<HHHHHB", data, o)
        name = data[o + 11:o + 11 + flen].decode("latin-1")
        fmt, size = ("<IH", 6) if kind == LINES32 else ("<HH", 4)
        out.append((name, [struct.unpack_from(fmt, data, o + plines + size * k)
                           for k in range(count)]))
        o = nxt * 16
    return out


def read_sym(data):
    """(module, [(number, name, [(offset, symbol)], [(file, [(offset,
    line)])])]) of a .SYM file."""
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
        segs.append((h[3], name, syms, read_lines(data, h[9])))
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
    ap.add_argument("--obj", action="append", default=[],
                    help="for a VxD: an object it was linked from, for its "
                    "line numbers")
    ap.add_argument("--name", help="the module name")
    ap.add_argument("--dump", action="store_true",
                    help="print a .SYM file's contents")
    ap.add_argument("--lines", action="store_true",
                    help="with --dump, print the line numbers too")
    a = ap.parse_args()
    with open(a.image, "rb") as f:
        data = f.read()
    if a.dump:
        module, segs = read_sym(data)
        print("module %s" % module)
        for number, name, syms, lines in segs:
            print("segment %d %s: %d symbols" % (number, name, len(syms)))
            for v, s in syms:
                print("  %04x:%08x %s" % (number, v, s))
            for path, rows in lines:
                print("  lines %s: %d" % (path, len(rows)))
                if a.lines:
                    for off, line in rows:
                        print("    %04x:%08x %d" % (number, off, line))
        return 0
    try:
        if a.map:
            segments, symbols, modules = le_map_symbols(a.map)
            lines = []
            for obj in a.obj:
                placed = modules.get(os.path.normpath(os.path.abspath(obj)),
                                     {})
                lines += obj_lines(obj, placed)
        else:
            segments, symbols = pe_symbols(data)
            lines = pe_lines(data)
        name = a.name or os.path.splitext(os.path.basename(a.image))[0]
        sym = write_sym(name.upper(), segments, symbols, lines)
    except (ValueError, struct.error, IndexError, OSError) as e:
        print("mapsym.py: %s: %s" % (a.image, e), file=sys.stderr)
        return 1
    out = a.out or os.path.splitext(a.image)[0] + ".sym"
    with open(out, "wb") as f:
        f.write(sym)
    return 0


if __name__ == "__main__":
    sys.exit(main())
