#!/usr/bin/env python3
"""Make C headers from MASM include files: the Windows 3.1 DDK's, which
has no C headers, for the ms-w31-vxd kit of import-ms-sdk.py.

    inc2h.py OUTDIR FILE.INC ...

For each FILE.INC, OUTDIR gets file.h with:

  - each equate (NAME EQU 0FFh, NAME = 1 SHL 4, NAME EQU OTHER + 1) whose
    value is a number or an expression of numbers and equates as a
    #define, each in #ifndef NAME so that vxd.h's own names are kept;
  - each structure (NAME STRUC ... NAME ENDS) as a packed struct NAME with
    BYTE, WORD and DWORD fields, arrays of them (DUP and strings) and
    fields of the structures before it;
  - IFDEF, IFNDEF, IF, IFE, ELSE and ENDIF as #ifdef and the like (an IF
    whose test is not an expression of numbers and equates as #if 0);
  - INCLUDE OTHER.INC as #include "other.h".

Everything else (macros, text equates, RECORDs, offsets into structures
such as Client_AX EQU WORD PTR Client_EAX, the service tables, which
vxdsvc.h has) is left out. The output is for the person's own use, made
from their copy of the DDK: it holds the DDK's names and values, so it is
not something to give away.
"""
import os
import re
import sys

TYPES = {"db": "BYTE", "dw": "WORD", "dd": "DWORD", "df": None, "dq": None,
         "dt": None}
OPS = {"shl": "<<", "shr": ">>", "and": "&", "or": "|", "xor": "^",
       "not": "~", "mod": "%"}
# The structures vxd.h declares itself.
OURS = {"VPICD_IRQ_Descriptor"}
TOKEN = re.compile(r"\s*(?:([0-9][0-9a-fA-F]*[hHbBqQoOdD]?)|"
                   r"([A-Za-z_?@$][\w?@$]*)|(<<|>>|[-+*/()~&|^%]))")


def number(t):
    s = t.lower()
    try:
        if s.endswith("h"):
            return int(s[:-1], 16)
        if s.endswith(("q", "o")):
            return int(s[:-1], 8)
        if s.endswith("b") and set(s[:-1]) <= set("01"):
            return int(s[:-1], 2)
        if s.endswith("d") and s[:-1].isdigit():
            return int(s[:-1])
        if s.isdigit():
            return int(s)
    except ValueError:
        pass
    return None


def expr(text, known):
    """MASM expression -> C, or None. known: {lowercase name: name}."""
    text = text.strip()
    if not text:
        return None
    out, i = [], 0
    while i < len(text):
        m = TOKEN.match(text, i)
        if not m or m.end() == i:
            return None
        i = m.end()
        num, word, op = m.groups()
        if num is not None:
            v = number(num)
            if v is None:
                return None
            out.append("0x%X" % v if v > 9 else str(v))
        elif word is not None:
            w = word.lower()
            if w in OPS:
                out.append(OPS[w])
            elif w in known:
                out.append(known[w])
            else:
                return None
        else:
            out.append(op)
        if text[i:].strip() == "":
            break
    c = " ".join(out)
    # A value that does not fit in a signed int is unsigned: 0FFFFFFFFh.
    try:
        v = eval(c.replace("/", "//"), {}, {}) if not any(
            n in c for n in known.values()) else None
    except Exception:
        v = None
    if v is not None and v > 0x7FFFFFFF:
        return "0x%XUL" % v if c.startswith("0x") and " " not in c else \
            "((DWORD)(%s))" % c
    return c if len(out) == 1 else "(%s)" % c


def strip(line):
    """The line without its comment (outside quotes)."""
    q = None
    for i, c in enumerate(line):
        if q:
            if c == q:
                q = None
        elif c in "'\"":
            q = c
        elif c == ";":
            return line[:i].rstrip()
    return line.rstrip()


def field(name, rest, structs):
    """(C type, name, array length) of a structure's field, or None."""
    m = re.match(r"(\w+)\s*(.*)$", rest)
    if not m:
        return None
    kind, init = m.group(1).lower(), m.group(2).strip()
    if kind in TYPES:
        t = TYPES[kind]
        if t is None:
            return None
        d = re.match(r"(\w+)\s+dup\s*\(", init, re.I)
        if d:
            n = number(d.group(1))
            return (t, name, n) if n else None
        s = re.match(r"""(['"])(.*)\1$""", init)
        if s and kind == "db":
            return (t, name, len(s.group(2)))
        if "," in init:
            return (t, name, len(init.split(",")))
        return (t, name, None)
    if kind in structs:
        return ("struct " + structs[kind], name, None)
    return None


def convert(path, known, structs, fields):
    """The lines of the C header of one include file."""
    out = []
    lines = open(path, "rb").read().decode("latin-1").splitlines()
    macro = 0
    struc = None
    depth = 0
    pad = 0
    for raw in lines:
        line = strip(raw)
        s = line.strip()
        if not s:
            continue
        w = s.split()
        lw = [x.lower() for x in w]
        if len(lw) > 1 and lw[1] == "macro":
            macro += 1
            continue
        if macro:
            if lw[0] == "endm":
                macro -= 1
            continue
        if struc is not None:
            if len(lw) > 1 and lw[1] == "ends":
                if struc[0] in OURS:
                    out.append("/* struct %s: vxd.h's */" % struc[0])
                elif struc[1]:
                    out.append("struct %s {" % struc[0])
                    for t, n, k in struc[1]:
                        out.append("  %s %s%s;" % (t, n,
                                                   "[%d]" % k if k else ""))
                    out.append("};")
                    structs[struc[0].lower()] = struc[0]
                struc = None
                continue
            if struc[2] is False:
                continue
            if lw[0] in TYPES:
                pad += 1
                f = field("pad%d" % pad, s, structs)
            elif len(w) > 1:
                f = field(w[0], s[len(w[0]):].strip(), structs)
            else:
                f = None
            if f is None:
                # A field we can't type: leave the structure out.
                struc[2] = False
                out.append("/* %s: not converted (%s) */" % (struc[0], s))
                struc[1] = []
                continue
            struc[1].append(f)
            fields.add(f[1])
            continue
        if len(lw) > 1 and lw[1] == "struc":
            struc = [w[0], [], True]
            pad = 0
            continue
        if lw[0] in ("ifdef", "ifndef") and len(w) > 1:
            if re.match(r"[A-Za-z_]\w*$", w[1]):
                out.append("#%s %s" % (lw[0], w[1]))
            else:   # ?NOPARMR and the like: never defined in C
                out.append("#if %d /* %s */" % (lw[0] == "ifndef", s))
            depth += 1
            continue
        if lw[0] in ("if", "ife"):
            e = expr(s[len(w[0]):], known)
            if e is None:
                out.append("#if 0 /* %s */" % s[len(w[0]):].strip())
            elif lw[0] == "ife":
                out.append("#if !%s" % e)
            else:
                out.append("#if %s" % e)
            depth += 1
            continue
        if lw[0].startswith("if") and lw[0] in (
                "ifb", "ifnb", "ifidn", "ifidni", "ifdif", "ifdifi", "if1",
                "if2"):
            out.append("#if 0 /* %s */" % s)
            depth += 1
            continue
        if lw[0] == "else" and depth:
            out.append("#else")
            continue
        if lw[0] == "endif" and depth:
            out.append("#endif")
            depth -= 1
            continue
        if lw[0] == "include" and len(w) > 1:
            inc = os.path.basename(w[1].replace("\\", "/")).lower()
            out.append('#include "%s"' % re.sub(r"\.inc$", ".h", inc))
            continue
        m = re.match(r"([A-Za-z_?@$][\w?@$]*)\s*(?:\bequ\b|=)\s*(.*)$", s,
                     re.I)
        if m:
            name = m.group(1)
            if "?" in name or "@" in name or "$" in name:
                continue
            e = expr(m.group(2), known)
            if e is None:
                continue
            out.append(("define", name, e))
            known[name.lower()] = name
    while depth:
        out.append("#endif")
        depth -= 1
    return out


def main():
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    outdir = sys.argv[1]
    os.makedirs(outdir, exist_ok=True)
    known, structs, fields = {}, {}, set()
    done = []
    for path in sys.argv[2:]:
        name = os.path.basename(path).lower()
        if not name.endswith(".inc"):
            continue
        done.append((name[:-4] + ".h", convert(path, known, structs,
                                                fields)))
    nd = ns = 0
    for h, lines in done:
        guard = "MS_W31_%s_H" % re.sub(r"\W", "_", h[:-2].upper())
        text = ["/* %s: made by inc2h.py from the Windows 3.1 DDK's %s. */"
                % (h, h[:-2].upper() + ".INC"),
                "#ifndef %s" % guard, "#define %s" % guard,
                "#include <vxd.h>", "#pragma pack(push, 1)"]
        for x in lines:
            if isinstance(x, tuple):
                _, n, e = x
                # A name that is also a field would rename the field.
                if n in fields:
                    continue
                text += ["#ifndef %s" % n, "#define %s %s" % (n, e),
                         "#endif"]
                nd += 1
            else:
                if x.startswith("struct "):
                    ns += 1
                text.append(x)
        text += ["#pragma pack(pop)", "#endif", ""]
        with open(os.path.join(outdir, h), "w") as f:
            f.write("\n".join(text))
    print("%d headers, %d equates, %d structures" % (len(done), nd, ns),
          file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
