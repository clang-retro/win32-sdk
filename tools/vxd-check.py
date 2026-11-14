#!/usr/bin/env python3
"""Check a VxD (vxd-link.py's output) the way the Windows VMM loads it.

    vxd-check.py [--dynamic | --static | --win31] FILE.VXD ...

It checks that the file is an LE module for Windows 386 marked as a
virtual device driver (dynamic or static for 95 and later, or as Windows
3.x marked them), that its one export is NAME_DDB at ordinal 1, and that
the DDB it points at has that NAME, the DDK version (4.00, or 3.10 for
Windows 3.1) in it and in the LE header, its "Prev" and
"Rsv1".."Rsv3" signatures and the right size, and a control procedure
with a fixup in a code object. It prints what it found. The exit status
is 1 when something is wrong.
"""
import argparse
import struct
import sys

OS_WIN386 = 4
VXD_31 = 0x8020  # module flags: a Windows 3.x VxD
VXD_STATIC = 0x28000  # a Windows 95 virtual device driver
VXD_DYNAMIC = 0x38000  # ... that can be loaded and unloaded
KINDS = {VXD_31: "Windows 3.x", VXD_STATIC: "static", VXD_DYNAMIC: "dynamic"}


class LE:
    def __init__(self, path):
        d = self.d = open(path, "rb").read()
        if d[:2] != b"MZ":
            raise ValueError("not an MZ file")
        le = self.le = struct.unpack_from("<I", d, 0x3C)[0]
        if d[le:le + 2] != b"LE":
            raise ValueError("no LE header")
        h = lambda o, f="<I": struct.unpack_from(f, d, le + o)[0]
        self.os = h(0x0A, "<H")
        self.flags = h(0x10)
        self.pagesize = h(0x28)
        self.lastpage = h(0x2C)
        self.objtab, self.nobj, self.pagemap = h(0x40), h(0x44), h(0x48)
        self.resname, self.entry = h(0x58), h(0x5C)
        self.fixpage, self.fixrec = h(0x68), h(0x6C)
        self.datapages = h(0x80)
        self.nonres, self.nonreslen = h(0x88), h(0x8C)
        self.npages = h(0x14)
        # The VxD fields after the OS/2 ones: the device ID and DDK
        # version, which the linker copies from the DDB.
        self.devid, self.ddkver = h(0xC0, "<H"), h(0xC2, "<H")
        self.objects = []
        for i in range(self.nobj):
            vsize, base, fl, pidx, np, _ = struct.unpack_from(
                "<IIIIII", d, le + self.objtab + 24 * i)
            self.objects.append((vsize, base, fl, pidx, np))

    def page_data(self, page):
        """The bytes of logical page `page` (1-based) in the file."""
        off = self.datapages + (page - 1) * self.pagesize
        size = self.lastpage if page == self.npages else self.pagesize
        return self.d[off:off + size].ljust(self.pagesize, b"\0")

    def object_data(self, n):
        vsize, base, fl, pidx, np = self.objects[n - 1]
        data = b"".join(self.page_data(p) for p in range(pidx, pidx + np))
        return data[:vsize].ljust(vsize, b"\0")

    def names(self):
        """{ordinal: name} of the resident and non-resident name tables
        (ordinal 0 is the module's name)."""
        out = self.name_table(self.le + self.resname)
        if self.nonres:
            out.update(self.name_table(self.nonres))
        return out

    def name_table(self, o):
        out = {}
        while o < len(self.d) and self.d[o]:
            n = self.d[o]
            name = self.d[o + 1:o + 1 + n].decode("latin-1")
            out[struct.unpack_from("<H", self.d, o + 1 + n)[0]] = name
            o += n + 3
        return out

    def entries(self):
        """{ordinal: (object, offset)} of the 32-bit entries."""
        out = {}
        o = self.le + self.entry
        ordinal = 1
        while self.d[o]:
            count, kind = self.d[o], self.d[o + 1]
            if kind == 0:
                ordinal += count
                o += 2
                continue
            obj = struct.unpack_from("<H", self.d, o + 2)[0]
            o += 4
            for _ in range(count):
                if kind == 3:
                    out[ordinal] = (obj, struct.unpack_from(
                        "<I", self.d, o + 1)[0])
                    o += 5
                else:
                    raise ValueError("entry bundle of type %d" % kind)
                ordinal += 1
        return out

    def fixups(self):
        """[(page, offset in page, target object, target offset)] for the
        internal 32-bit offset fixups."""
        out = []
        base = self.le + self.fixrec
        for p in range(1, self.npages + 1):
            a, b = struct.unpack_from("<II", self.d,
                                      self.le + self.fixpage + 4 * (p - 1))
            o = base + a
            while o < base + b:
                src, flags = self.d[o], self.d[o + 1]
                o += 2
                if src & 0x20:
                    raise ValueError("fixup source lists are not handled")
                srcoff = struct.unpack_from("<h", self.d, o)[0]
                o += 2
                if flags & 3 != 0:
                    raise ValueError("fixup to an import in a VxD")
                obj = self.d[o] if not flags & 0x40 else \
                    struct.unpack_from("<H", self.d, o)[0]
                o += 2 if flags & 0x40 else 1
                if (src & 0xF) == 2:  # selector only
                    tgt = 0
                elif flags & 0x10:
                    tgt = struct.unpack_from("<I", self.d, o)[0]
                    o += 4
                else:
                    tgt = struct.unpack_from("<H", self.d, o)[0]
                    o += 2
                out.append((p, srcoff, obj, tgt))
        return out


def check(path, want_kind):
    bad = []
    m = LE(path)
    print("%s: LE, %d objects, module flags 0x%x, DDK version %d.%02d"
          % (path, m.nobj, m.flags, m.ddkver >> 8, m.ddkver & 0xFF))
    if m.os != OS_WIN386:
        bad.append("target OS %d, not Windows 386 (4)" % m.os)
    kind = m.flags & 0x38020
    if kind not in KINDS:
        bad.append("not marked as a virtual device driver")
    elif want_kind is not None and kind != want_kind:
        bad.append("a %s VxD, not a %s one" % (KINDS[kind], KINDS[want_kind]))
    want_ver = 0x030A if kind == VXD_31 else 0x0400
    names, entries = m.names(), m.entries()
    exports = {o: n for o, n in names.items() if o}
    print("  module %s" % names.get(0, "?"))
    if list(exports) != [1] or 1 not in entries:
        bad.append("exports %s; a VxD exports only its DDB, ordinal 1"
                   % exports)
        return bad
    name = exports[1]
    if not name.endswith("_DDB"):
        bad.append("its export %s is not a NAME_DDB" % name)
    obj, off = entries[1]
    data = m.object_data(obj)
    ddb = data[off:off + 0x50]
    ver, = struct.unpack_from("<H", ddb, 4)
    dname = ddb[0x0C:0x14].decode("latin-1")
    sigs = struct.unpack_from("<IIIII", ddb, 0x3C)
    print("  %s: name %r, SDK version %d.%02d, size %d" % (
        name, dname, ver >> 8, ver & 0xFF, sigs[1]))
    if dname.rstrip() != name[:-4]:
        bad.append("the DDB's name %r is not %s" % (dname, name[:-4]))
    if ver != want_ver:
        bad.append("DDB version %d.%02d, not %d.%02d" % (
            ver >> 8, ver & 0xFF, want_ver >> 8, want_ver & 0xFF))
    if sigs[0] != 0x50726576 or sigs[2:] != (0x52737631, 0x52737632,
                                             0x52737633):
        bad.append("the DDB's Prev and Rsv1-3 signatures are wrong")
    if m.ddkver != ver:
        bad.append("the LE header's DDK version %d.%02d is not the DDB's"
                   % (m.ddkver >> 8, m.ddkver & 0xFF))
    if sigs[1] != 0x50:
        bad.append("DDB_Size %d, not 80" % sigs[1])
    # The control procedure pointer (DDB + 0x18) needs a fixup to code.
    vsize, base, fl, pidx, np = m.objects[obj - 1]
    page = pidx + (off + 0x18) // m.pagesize
    poff = (off + 0x18) % m.pagesize
    ctl = [f for f in m.fixups() if f[0] == page and f[1] == poff]
    if not ctl:
        bad.append("no fixup for DDB_Control_Proc")
    else:
        tobj = ctl[0][2]
        if not m.objects[tobj - 1][2] & 4:
            bad.append("DDB_Control_Proc points into object %d, which is "
                       "not executable" % tobj)
        else:
            print("  control procedure at %d:%08x" % (tobj, ctl[0][3]))
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--dynamic", action="store_true")
    g.add_argument("--static", action="store_true")
    g.add_argument("--win31", action="store_true")
    ap.add_argument("files", nargs="+")
    a = ap.parse_args()
    want = (VXD_DYNAMIC if a.dynamic else VXD_STATIC if a.static else
            VXD_31 if a.win31 else None)
    failed = False
    for f in a.files:
        try:
            bad = check(f, want)
        except (ValueError, struct.error, IndexError) as e:
            bad = [str(e)]
        for b in bad:
            print("  error: " + b)
        failed = failed or bool(bad)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
