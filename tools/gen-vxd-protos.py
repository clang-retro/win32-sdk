#!/usr/bin/env python3
"""Make vxd-prototypes.txt from the C include files of Microsoft's VxD
DDKs: the parameter types of the services called like C functions.

    gen-vxd-protos.py [--services vxd-services.txt] INCDIR ... \\
        > vxd-prototypes.txt

    gen-vxd-protos.py "4.0 W95 DDK/ddk/INC32" \\
        "4.10 DDK (unpacked)/inc/win98" \\
        "4.90 DDK (unpacked)/inc/win_me" > tools/vxd-prototypes.txt

The include files, oldest DDK first, declare most of those services with
MAKE_HEADER(return type, convention, name, (parameters)) (vxdwraps.h and
the headers of the VxDs), MAKE_CM_HEADER(name, (parameters)) (CONFIGMG's)
or as inline functions. This writes one line for each service that has
one:

    NAME RETURN PARAMETER,PARAMETER,...

with each type reduced to one of void, BYTE, WORD, DWORD, void* (any
pointer) and ... (more arguments), the DDK's types (HVM, CONFIGRET,
PDEVNODE, a function pointer...) followed through their typedefs. A
service whose types can't be reduced (a structure passed by value, a
64-bit or floating-point one) or whose number of parameters changed
between DDKs is left out, so gen-vxd-wraps.py declares it as before,
with no parameter list. Only the types are taken from the include files,
not the names of the parameters or anything else.
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# Words that are not part of a type.
NOISE = {"VXDINLINE", "static", "__inline", "inline", "_inline", "CDECL",
         "_cdecl", "__cdecl", "cdecl", "_stdcall", "__stdcall", "WINAPI",
         "NTAPI", "POWERFAR", "extern", "const", "volatile", "far", "near",
         "_far", "_near", "FAR", "NEAR", "register", "IN", "OUT", "OPTIONAL",
         "CONST", "__based", "EXTERN_C"}
BUILTIN = {
    "void": "void", "char": "BYTE", "short": "WORD", "int": "DWORD",
    "long": "DWORD", "unsigned": "DWORD", "signed": "DWORD",
    "VOID": "void", "BYTE": "BYTE", "WORD": "WORD", "DWORD": "DWORD",
}
FAIL = {"float", "double", "__int64", "_int64", "LONGLONG", "ULONGLONG",
        "QWORD", "struct", "union"}


def clean(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r"\\\n", " ", text)
    return text


def read_typedefs(text, out):
    """Each typedef: name -> 'void*', 'struct' or the words of its base."""
    # #define HPORT DWORD and the like, for a type a header names that way.
    for m in re.finditer(r"^\s*#\s*define\s+(\w+)[ \t]+(\w+(?:[ \t]*\*)*)"
                         r"[ \t]*$", text, re.M):
        out.setdefault(m.group(1), "void*" if "*" in m.group(2)
                       else m.group(2))
    text = re.sub(r"^\s*#[^\n]*", "", text, flags=re.M)
    for m in re.finditer(r"\btypedef\b", text):
        i, depth = m.end(), 0
        while i < len(text):
            c = text[i]
            if c in "{(":
                depth += 1
            elif c in "})":
                depth -= 1
            elif c == ";" and depth <= 0:
                break
            i += 1
        body = text[m.end():i]
        fp = re.search(r"\(\s*[\w\s]*\*\s*(\w+)\s*\)\s*\(", body)
        if fp:
            out.setdefault(fp.group(1), "void*")
            continue
        if "{" in body:
            head = body[:body.index("{")].split()
            decls = body[body.rindex("}") + 1:]
            base = "DWORD" if head[:1] == ["enum"] else "struct"
        else:
            # The base type is the words before the first declarator: its
            # name and the stars before it.
            parts = body.split(",")
            first = parts[0].replace("*", " * ").split()
            if len(first) < 2:
                continue
            k = len(first) - 1
            while k > 0 and first[k - 1] == "*":
                k -= 1
            base = " ".join(first[:k])
            decls = " ".join(first[k:]) + "," + ",".join(parts[1:])
        for d in decls.split(","):
            d = re.sub(r"\[.*?\]", "", d)
            name = re.findall(r"\w+", d)
            if not name:
                continue
            name = [n for n in name if not noise(n)]
            if not name:
                continue
            out.setdefault(name[-1], "void*" if "*" in d else base)


def noise(w):
    # CMFAR, POWERFAR and the like: far in 16-bit code, nothing here.
    return w in NOISE or w.endswith("FAR") and w.isupper()


def reduce(words, typedefs, depth=0):
    """The type in our vocabulary, or None."""
    if depth > 20:
        return None
    if "*" in words:
        return "void*"
    words = [w for w in words if not noise(w)]
    if not words:
        return None
    if any(w in FAIL for w in words):
        return None
    if len(words) > 1 and all(w in BUILTIN for w in words):
        if "char" in words:
            return "BYTE"
        if "short" in words:
            return "WORD"
        if words.count("long") > 1:
            return None
        return "DWORD"
    if len(words) != 1:
        return None
    w = words[0]
    if w in BUILTIN:
        return BUILTIN[w]
    t = typedefs.get(w)
    if t is None or t == "struct":
        return None
    if t == "void*":
        return t
    return reduce(t.replace("*", " * ").split(), typedefs, depth + 1)


def param_type(p, typedefs):
    p = p.strip()
    if p == "...":
        return "..."
    p = re.sub(r"\[.*?\]", " * ", p).replace("*", " * ")
    words = [w for w in p.split() if not noise(w)]
    if not words:
        return None
    # Drop the parameter's name: the last word, after a type word.
    if len(words) > 1 and words[-1] != "*" and re.match(r"\w+$", words[-1]):
        known = (words[-1] in BUILTIN or words[-1] in typedefs)
        if not known or len(words) > 1 and words[-2] in typedefs:
            words = words[:-1]
    return reduce(words, typedefs)


def signature(ret, params, typedefs):
    r = reduce(ret.replace("*", " * ").split(), typedefs)
    if r is None:
        return None
    ps = [p for p in params.split(",")]
    if len(ps) == 1 and ps[0].strip() in ("", "VOID", "void"):
        return r, []
    out = []
    for p in ps:
        t = param_type(p, typedefs)
        if t is None or t == "void":
            return None
        out.append(t)
    return r, out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--services",
                    default=os.path.join(HERE, "vxd-services.txt"))
    ap.add_argument("incdirs", nargs="+")
    a = ap.parse_args()
    names = set()
    with open(a.services) as f:
        for line in f:
            if line.strip() and not line.startswith("#"):
                w = line.split()
                if w[4] == "c":
                    names.add(w[3])
    # found[name] = [(kit, priority, ret, params)], newest kit last.
    found = {}
    for kit, d in enumerate(a.incdirs):
        texts = {}
        for f in sorted(os.listdir(d)):
            if f.lower().endswith(".h"):
                with open(os.path.join(d, f), encoding="latin-1") as fh:
                    texts[f] = clean(fh.read())
        typedefs = {}
        for t in texts.values():
            read_typedefs(t, typedefs)
        for t in texts.values():
            for m in re.finditer(r"MAKE_HEADER\s*\(\s*([^,]+?)\s*,\s*\w+\s*,"
                                 r"\s*(\w+)\s*,\s*\(([^()]*)\)\s*\)", t):
                if m.group(2) in names:
                    found.setdefault(m.group(2), []).append(
                        (kit, 1, m.group(1), m.group(3), typedefs))
            for m in re.finditer(r"MAKE_CM_HEADER\s*\(\s*(\w+)\s*,"
                                 r"\s*\(([^()]*)\)\s*\)", t):
                n = "_CONFIGMG_" + m.group(1)
                if n in names:
                    found.setdefault(n, []).append(
                        (kit, 1, "CONFIGRET", m.group(2), typedefs))
            for m in re.finditer(r"(?:^|[;}\n])\s*((?:[A-Za-z_]\w*[\s*]+)+?)"
                                 r"\b([A-Za-z_]\w*)\s*\(([^()]*)\)\s*[;{]",
                                 t, re.M):
                if m.group(2) in names:
                    found.setdefault(m.group(2), []).append(
                        (kit, 0, m.group(1), m.group(3), typedefs))
    print("# The parameter types of the VxD services called like C functions:")
    print("# name, return type, parameter types. Made by gen-vxd-protos.py.")
    n = 0
    for name in sorted(found):
        sigs = []
        for kit, prio, ret, params, typedefs in found[name]:
            s = signature(ret, params, typedefs)
            sigs.append((kit, prio, s))
        counts = {len(s[1]) if s else None for _, _, s in sigs}
        if None in counts or len(counts) != 1:
            continue
        # The newest kit's MAKE_HEADER, else its inline function.
        _, _, (ret, params) = max(sigs, key=lambda x: (x[0], x[1]))
        print("%s %s %s" % (name, ret, ",".join(params) or "void"))
        n += 1
    print("%d of %d services typed" % (n, len(names)), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
