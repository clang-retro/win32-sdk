#!/usr/bin/env python3
"""Make VxD headers for subsystems whose DDK headers are only MASM .inc files.

    gen-vxd-inc.py CFGDIR [HEADER ...]

VKD (the keyboard) and VMOUSE (the mouse) have no C headers in the 95, 98
or ME DDK, only vkd.inc, vkdpaste.inc and vmd.inc. This reads them from the
kits that import-ms-sdk.py imported beside CFGDIR (sysroots/ms-*-vxd/
include), evaluates their EQUs and lays out their STRUCs and UNIONs as MASM
does (byte-packed), and writes vxd/include/HEADER.h with the names and
values, and C structures in their place, and vxd/layout/HEADER.c with the
sizes and offsets MASM gives them. It copies no text from the DDK: comments,
macros and the service tables (which <vxdsvc.h> has) are left out, and each
value is written as the number it comes to.

As gen-vxd-headers.py does, a name is under the first VXD_WINVER whose DDK
has it, with #if/#elif when its value changes, and names <vxd.h> already
defines are left out. NEC_98 (the PC-98) and WIN31COMPAT are not defined;
Not_VxD is not either.
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
VXD = os.path.join(HERE, "..", "vxd")
INCLUDE = os.path.join(VXD, "include")
LAYOUT = os.path.join(VXD, "layout")
KITS = [("0x0400", "ms-w95-vxd"), ("0x040A", "ms-w98-vxd"),
        ("0x045A", "ms-me-vxd")]

# name: (the .inc to read, which includes the rest, introduction)
SPECS = {
    "vkd": ("vkd.inc", """The virtual keyboard device (VKD): the shift state
   bits (SS_*), the hot key flags and notifications VKD_Define_Hot_Key
   takes and calls back with (CallOn*, Hot_Key_*, HKSS_*), the paste
   buffer's Paste_Rec and completion codes, and the VKD_* protected mode
   API's functions and FKSS_* shift states. The services are VKD_* in
   <vxdsvc.h>."""),
    "vmd": ("vmd.inc", """The virtual mouse device (VMOUSE): the mouse types
   (VMD_Type_*), the VMDAPI_* functions, the Mouse_Instance a mini mouse
   driver fills in and its MIF_* flags, and the cursor drawing VxD
   interface (the cursor procedure's functions, DISPLAYMODEINFO, the text
   and graphics cursors and Int10Notify). The services are VMD_* in
   <vxdsvc.h>."""),
}

SIZES = {"db": 1, "byte": 1, "sbyte": 1, "dw": 2, "word": 2, "sword": 2,
         "dd": 4, "dword": 4, "sdword": 4, "df": 6, "fword": 6,
         "dq": 8, "qword": 8, "dt": 10, "tbyte": 10}
CTYPE = {1: "BYTE", 2: "WORD", 4: "DWORD"}
C_KEYWORDS = {"auto", "break", "case", "char", "const", "continue",
              "default", "do", "double", "else", "enum", "extern", "float",
              "for", "goto", "if", "inline", "int", "long", "register",
              "return", "short", "signed", "sizeof", "static", "struct",
              "switch", "typedef", "union", "unsigned", "void", "volatile",
              "while", "class", "new", "delete", "this", "template"}


class Unknown(Exception):
    pass


# --- MASM expressions -------------------------------------------------------

TOKEN = re.compile(r"\s*(?:([0-9][0-9A-Fa-f]*[HhBbOoQqDd]?)|([A-Za-z_?@$][\w?@$]*)"
                   r"|(<=|>=|==|!=|[-+*/()<>]))")


def number(t):
    s = t.lower()
    if s.endswith("h"):
        return int(s[:-1], 16)
    if s.endswith("b") and re.fullmatch(r"[01]+b", s):
        return int(s[:-1], 2)
    if s[-1] in "oq":
        return int(s[:-1], 8)
    if s.endswith("d") and s[:-1].isdigit():
        return int(s[:-1])
    if s.isdigit():
        return int(s)
    raise Unknown(t)


def tokens(text):
    out, i = [], 0
    text = text.rstrip()
    while i < len(text):
        m = TOKEN.match(text, i)
        if not m or m.end() == i:
            raise Unknown(text[i:])
        i = m.end()
        if m.group(1):
            out.append(("n", number(m.group(1))))
        elif m.group(2):
            out.append(("w", m.group(2)))
        else:
            out.append(("o", m.group(3)))
    return out


def evaluate(text, syms, sizes):
    """A MASM constant expression's value, as a 32-bit number."""
    toks = tokens(text)
    pos = [0]

    def peek():
        return toks[pos[0]] if pos[0] < len(toks) else (None, None)

    def take():
        pos[0] += 1
        return toks[pos[0] - 1]

    def word(*ws):
        k, v = peek()
        return k == "w" and v.lower() in ws

    def op(*os_):
        k, v = peek()
        return k == "o" and v in os_

    # MASM's precedence, lowest first: OR XOR, AND, NOT, EQ NE LT LE GT GE,
    # binary + -, * / MOD SHL SHR, unary + -, SIZE, ( ).
    def p_or():
        v = p_and()
        while word("or", "xor"):
            w = take()[1].lower()
            r = p_and()
            v = v | r if w == "or" else v ^ r
        return v

    def p_and():
        v = p_not()
        while word("and"):
            take()
            v &= p_not()
        return v

    def p_not():
        if word("not"):
            take()
            return ~p_not() & 0xFFFFFFFF
        return p_rel()

    def p_rel():
        v = p_add()
        while word("eq", "ne", "lt", "le", "gt", "ge"):
            w = take()[1].lower()
            r = p_add()
            v = {"eq": v == r, "ne": v != r, "lt": v < r, "le": v <= r,
                 "gt": v > r, "ge": v >= r}[w]
            v = 0xFFFFFFFF if v else 0
        return v

    def p_add():
        v = p_mul()
        while op("+", "-"):
            o = take()[1]
            r = p_mul()
            v = v + r if o == "+" else v - r
        return v

    def p_mul():
        v = p_unary()
        while op("*", "/") or word("mod", "shl", "shr"):
            o = take()[1].lower()
            r = p_unary()
            if o == "*":
                v *= r
            elif o == "/":
                v //= r
            elif o == "mod":
                v %= r
            elif o == "shl":
                v <<= r
            else:
                v >>= r
        return v

    def p_unary():
        if op("-"):
            take()
            return -p_unary()
        if op("+"):
            take()
            return p_unary()
        return p_atom()

    def p_atom():
        k, v = take() if pos[0] < len(toks) else (None, None)
        if k == "n":
            return v
        if k == "o" and v == "(":
            r = p_or()
            if not op(")"):
                raise Unknown(text)
            take()
            return r
        if k == "w" and v.lower() in ("size", "sizeof", "type"):
            k2, name = take()
            if k2 == "o" and name == "(":
                k2, name = take()
                take()
            if name.lower() in sizes:
                return sizes[name.lower()]
            raise Unknown(name)
        if k == "w" and v.lower() in syms:
            return syms[v.lower()][1]
        raise Unknown(text)

    v = p_or()
    if pos[0] != len(toks):
        raise Unknown(text)
    return v & 0xFFFFFFFF


# --- Reading a kit's .inc files ---------------------------------------------

def lines_of(path):
    with open(path, encoding="latin-1") as f:
        for line in f:
            yield line.split(";", 1)[0].rstrip()


def find(incdir, name):
    for n in os.listdir(incdir):
        if n.lower() == name.lower():
            return os.path.join(incdir, n)
    return None


def read_kit(incdir, top):
    """The constants and records of TOP and the .inc files it includes, in
    order: [("const", name, value) | ("rec", name, kind, fields, size)],
    where a field is (name, ctype or record, count, offset)."""
    items = []
    syms = {}          # lower name: (name, value)
    sizes = dict(SIZES)
    recs = {}          # lower name: (name, size)
    defined = set()    # IFDEF symbols: none

    def walk(path):
        stack = []     # IF states: [taking, any branch taken]
        skip_until = None
        rec = None
        for raw in lines_of(path):
            line = raw.strip()
            if not line:
                continue
            ws = line.split()
            w0 = ws[0].lower()
            w1 = ws[1].lower() if len(ws) > 1 else ""
            if skip_until:
                if w0 in skip_until or w1 in skip_until:
                    skip_until = None
                continue
            # conditionals
            if w0 in ("if", "ifdef", "ifndef", "ife", "ifb", "ifnb"):
                taking = all(s[0] for s in stack)
                if w0 == "ifdef":
                    c = w1 in defined
                elif w0 == "ifndef":
                    c = w1 not in defined
                else:
                    try:
                        c = evaluate(line.split(None, 1)[1], syms, sizes) != 0
                    except Unknown:
                        c = False
                    if w0 == "ife":
                        c = not c
                stack.append([taking and c, c])
                continue
            if w0 in ("else", "elseif", "elseifdef", "elseifndef"):
                outer = all(s[0] for s in stack[:-1])
                if w0 == "else":
                    c = not stack[-1][1]
                elif w0 == "elseifdef":
                    c = not stack[-1][1] and w1 in defined
                elif w0 == "elseifndef":
                    c = not stack[-1][1] and w1 not in defined
                else:
                    try:
                        c = not stack[-1][1] and evaluate(
                            line.split(None, 1)[1], syms, sizes) != 0
                    except Unknown:
                        c = False
                stack[-1] = [outer and c, stack[-1][1] or c]
                continue
            if w0 == "endif":
                stack.pop()
                continue
            if not all(s[0] for s in stack):
                continue
            # what is left out
            if w0 == "begindoc":
                skip_until = ("enddoc",)
                continue
            if w1 == "macro":
                skip_until = ("endm",)
                continue
            if w0 == "begin_service_table":
                skip_until = ("end_service_table",)
                continue
            if w0.startswith("."):
                continue
            if w0 == "include":
                p = find(os.path.dirname(path), ws[1])
                if p:
                    walk(p)
                continue
            # records
            if w1 in ("struc", "struct", "union") and rec is None:
                rec = [ws[0], "union" if w1 == "union" else "struct", [], 0]
                continue
            if rec is not None:
                if w1 == "ends" or w0 == "ends":
                    name, kind, fields, size = rec
                    items.append(("rec", name, kind, fields, size))
                    sizes[name.lower()] = size
                    recs[name.lower()] = (name, size)
                    rec = None
                    continue
                field(rec, line, syms, sizes, recs)
                continue
            # constants
            m = re.match(r"([A-Za-z_?@$][\w?@$]*)\s+(equ\b|=)\s*(.*)$", line,
                         re.I)
            if m:
                name, val = m.group(1), m.group(3)
                try:
                    v = evaluate(val, syms, sizes)
                except Unknown:
                    continue
                syms[name.lower()] = (name, v)
                items.append(("const", name, v))
        assert not stack, path

    walk(find(incdir, top))
    return items


def field(rec, line, syms, sizes, recs):
    """Add a STRUC's field line to REC."""
    m = re.match(r"([A-Za-z_?@$][\w?@$]*)?\s*([A-Za-z_]\w*)\s*(.*)$", line)
    name, typ, init = m.group(1), m.group(2).lower(), m.group(3)
    if typ not in SIZES and typ not in recs:
        # "name dd ?" splits as name=None? no: the first word is the name
        raise SystemExit("unknown field type in: " + line)
    count = 1
    d = re.match(r"(.*?)\s+dup\s*\(", init, re.I)
    if d:
        count = evaluate(d.group(1), syms, sizes)
    elif typ in SIZES and "," in init:
        count = len(init.split(","))
    if typ in recs:
        ctype, size = recs[typ][0], recs[typ][1]
    else:
        size = SIZES[typ]
        ctype = CTYPE.get(size)
        if ctype is None:
            ctype, count = "BYTE", count * size
            size = 1
    off = 0 if rec[1] == "union" else rec[3]
    rec[2].append((name, ctype, count, off))
    end = off + size * count
    rec[3] = max(rec[3], end) if rec[1] == "union" else end


# --- Writing ------------------------------------------------------------------

def vxd_names(cfgdir):
    """The macro and type names <vxd.h> (and <vxdsvc.h>) define."""
    clang = os.path.join(cfgdir, "clang")
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "o.c")
        with open(src, "w") as f:
            f.write("#include <vxd.h>\n#include <vxdtypes.h>\n")
        out = subprocess.run(
            [clang, "--config=" + os.path.join(cfgdir, "win95-vxd.cfg"),
             "-I", VXD, "-I", INCLUDE, "-E", "-dD", src],
            capture_output=True, text=True, check=True).stdout
    names = set(re.findall(r"^#define\s+(\w+)", out, re.M))
    names |= set(re.findall(r"\b(\w+)\s*;\s*$", out, re.M))
    names |= set(re.findall(r"\b(?:struct|union)\s+(\w+)", out))
    return names


def merge(per_kit):
    """Each name's text in each kit, in an order that keeps each kit's."""
    order, text = [], {}
    for k, items in enumerate(per_kit):
        prev = None
        for key, t in items:
            if key in text:
                if k not in text[key]:
                    text[key][k] = t
                prev = key
                continue
            text[key] = {k: t}
            at = order.index(prev) + 1 if prev in order else (
                0 if prev is None else len(order))
            order.insert(at, key)
            prev = key
    return order, text


def gated(texts, nkits):
    """#if VXD_WINVER chains for a name's text in each kit."""
    runs, last = [], None
    for k in range(nkits):
        # a name a later DDK drops keeps its value, as gen-vxd-headers.py
        # keeps a declaration
        t = texts.get(k, last)
        if t != last:
            runs.append((k, t))
            last = t
    out = []
    if len(runs) == 1 and runs[0][0] == 0:
        return [runs[0][1]]
    first = True
    for i, (k, t) in enumerate(runs):
        cond = "VXD_WINVER >= %s" % KITS[k][0]
        if i + 1 < len(runs):
            cond += " && VXD_WINVER < %s" % KITS[runs[i + 1][0]][0]
        if t is None:
            continue
        if k == 0:
            cond = "VXD_WINVER < %s" % KITS[runs[1][0]][0]
        if not first and i + 1 == len(runs):
            out.append("#else")
        else:
            out.append(("#if " if first else "#elif ") + cond)
        out.append(t)
        first = False
    out.append("#endif")
    return out


def c_name(n):
    return n + "_" if n.lower() in C_KEYWORDS else n


def render(item, rename):
    if item[0] == "const":
        return "#define %s %s" % (item[1], hex(item[2]) if item[2] > 9
                                  else str(item[2]))
    _, name, kind, fields, size = item
    body = ["%s %s {" % (kind, name)]
    pad = 0
    for i, (fname, ctype, count, off) in enumerate(fields):
        t = ("%s %s" % (rename.get(ctype, "struct"), ctype)
             if ctype not in CTYPE.values() else ctype)
        fname = c_name(fname) if fname else "_reserved%d" % pad
        if not fields[i][0]:
            pad += 1
        body.append("  %s %s%s;" % (t, fname, "[%d]" % count
                                     if count != 1 else ""))
    body.append("};")
    body.append("typedef %s %s %s;" % (kind, name, name))
    return "\n".join(body)


def generate(cfgdir, name, skip):
    top, intro = SPECS[name]
    per_kit, layouts = [], []
    for k, (_, kit) in enumerate(KITS):
        incdir = os.path.join(cfgdir, "..", "sysroots", kit, "include")
        if not find(incdir, top):
            print("%s: %s has no %s" % (name, kit, top), file=sys.stderr)
            per_kit.append([])
            continue
        items = read_kit(incdir, top)
        kinds = {i[1]: i[2] for i in items if i[0] == "rec"}
        rendered = []
        for i in items:
            if i[1] in skip:
                continue
            rendered.append(((i[0], i[1]), render(i, kinds)))
        # a name EQUed twice keeps its last value, as MASM's = does
        seen = {}
        for key, t in rendered:
            seen[key] = t
        out, done = [], set()
        for key, t in rendered:
            if key not in done:
                out.append((key, seen[key]))
                done.add(key)
        per_kit.append(out)
        if kit == "ms-w98-vxd":
            layouts = [i for i in items if i[0] == "rec" and i[1] not in skip]
    order, text = merge(per_kit)
    h = ["/* %s.h: %s\n\n   Made by tools/gen-vxd-inc.py from the names and "
         "values in the 95, 98\n   and ME DDKs' %s; each is under the first "
         "VXD_WINVER whose DDK\n   has it. */\n" % (name, intro, top),
         "#ifndef VXD_%s_H\n#define VXD_%s_H\n\n#include <vxd.h>\n"
         "#include <vxdtypes.h>\n\n#pragma pack(push, 1)\n"
         % (name.upper(), name.upper())]
    for key in order:
        h.extend(t + "\n" for t in gated(text[key], len(KITS)))
    h.append("\n#pragma pack(pop)\n\n#endif\n")
    with open(os.path.join(INCLUDE, name + ".h"), "w") as f:
        f.write("".join(h))
    t = ["/* Made by tools/gen-vxd-inc.py: the layouts of <%s.h> as MASM "
         "lays out\n   the 98 DDK's %s. */\n" % (name, top),
         "#define VXD_WINVER 0x040A\n#include <%s.h>\n#include <stddef.h>\n"
         "#ifdef __cplusplus\n#define A(e) static_assert(e, #e);\n#else\n"
         "#define A(e) _Static_assert(e, #e);\n#endif\n" % name]
    for _, rec, kind, fields, size in layouts:
        t.append("A(sizeof(%s) == %d)\n" % (rec, size))
        for fname, _, _, off in fields:
            if fname:
                t.append("A(offsetof(%s, %s) == %d)\n"
                         % (rec, c_name(fname), off))
    with open(os.path.join(LAYOUT, name + ".c"), "w") as f:
        f.write("".join(t))
    print("%s.h: %d names, %d layout checks" % (
        name, len(order), sum(1 + len(r[3]) for r in layouts)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("cfgdir")
    ap.add_argument("headers", nargs="*")
    a = ap.parse_args()
    skip = vxd_names(a.cfgdir)
    for name in a.headers or sorted(SPECS):
        generate(a.cfgdir, name, skip)


if __name__ == "__main__":
    main()
