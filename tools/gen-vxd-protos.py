#!/usr/bin/env python3
"""Make vxd-prototypes.txt from the C include files of Microsoft's VxD
DDKs: the parameter types of the services called like C functions.

    gen-vxd-protos.py [--services vxd-services.txt] INCDIR ... \\
        > vxd-prototypes.txt

    7z x -ohelp "4.90 DDK (unpacked)/help/win98ddk.chm"  (and the rest)
    gen-vxd-protos.py --docs help --docs "4.0 W95 DDK/ddk/DOCS" \\
        "4.0 W95 DDK/ddk/INC32" \\
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
PDEVNODE, a function pointer...) followed through their typedefs. A type
no header defines is a pointer if its name says so (PNSOBJ, a ...HANDLER)
and a dword otherwise. A service whose parameters grew between DDKs gets
the ones every version has and "...". The services no header declares
come from the DDKs' help (--docs: the .chm files unpacked, and the 95
DDK's Word documents, DOCS/KERNEL/KERNEL.DOC and the rest): a C
prototype, or the assembler syntax, VMMCall _Name, <a, b> or the pushes
before a VMMcall _Name, whose parameters are dwords and pointers
(OFFSET32); a service whose example sets registers instead is left out,
and one that says "No return value" returns void. A service whose types
can't be reduced (a structure passed by value, a 64-bit or
floating-point one) or that neither has is left out, so gen-vxd-wraps.py
declares it as before, with no parameter list. Only the types are taken,
not the names of the parameters or anything else.
"""
import argparse
import html
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# Words that are not part of a type.
NOISE = {"VXDINLINE", "static", "__inline", "inline", "_inline", "CDECL",
         "_cdecl", "__cdecl", "cdecl", "_stdcall", "__stdcall", "WINAPI",
         "NTAPI", "POWERFAR", "extern", "const", "volatile", "far", "near",
         "_far", "_near", "FAR", "NEAR", "register", "IN", "OUT", "OPTIONAL",
         "CONST", "__based", "EXTERN_C", "EXTERNAL", "SERVICE", "VXDAPI",
         "__export", "_export"}
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
    # DECLARE_HANDLE(HPEMODULE): a pointer to a structure of its own.
    for m in re.finditer(r"\bDECLARE_HANDLE\s*\(\s*(\w+)\s*\)", text):
        out.setdefault(m.group(1), "void*")
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


def guess(w):
    """A type no header defines (PNSOBJ, OPREGIONHANDLER, ...): a pointer if
    its name says so (P or LP and a capital, or a callback's name), else a
    dword (a handle, a flag set or an enum)."""
    if re.match(r"(?:L?P|PP)[A-Z_]", w) or re.search(
            r"(?:HANDLER|CALLBACK|PROC|ROUTINE|FUNC|FN)$", w):
        return "void*"
    return "DWORD"


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
    if t is None:
        return guess(w) if re.match(r"[A-Z][A-Z0-9_]*$", w) else None
    if t == "struct":
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


def read_doc(path, names, docs):
    with open(path, "rb") as f:
        raw = f.read()
    if path.lower().endswith((".htm", ".html")):
        t = raw.decode("latin-1")
        t = html.unescape(re.sub(r"<[^>]*>", "", t)).replace("\xa0", " ")
    else:
        # A Word document (the 95 DDK's KERNEL.DOC): its text runs.
        t = "\n".join(m.decode("latin-1") for m in
                      re.findall(rb"[\t\r\x20-\x7e]{3,}", raw))
        t = t.replace("\r", "\n")
    t = re.sub(r"\\\s*\n", " ", t)
    for m in re.finditer(r"^[ \t]*((?:[A-Za-z_]\w*[ \t*]+)+?)\b(_\w+)\s*"
                         r"\(([^()]*)\)\s*;", t, re.M):
        if m.group(2) in names:
            docs.setdefault(m.group(2), (m.group(1), m.group(3)))
    for m in re.finditer(r"\b(?:VMMCall|VMMcall|VxDCall|VxDcall)\s+(_\w+)"
                         r"\s*,\s*<(.*)$", t, re.M):
        if m.group(1) not in names or m.group(1) in docs:
            continue
        args, depth, cur = [], 0, ""
        for c in m.group(2):
            if c == "<":
                depth += 1
            elif c == ">":
                if depth == 0:
                    break
                depth -= 1
            elif c == "," and depth == 0:
                args.append(cur)
                cur = ""
                continue
            cur += c
        if cur.strip():
            args.append(cur)
        docs[m.group(1)] = ("DWORD", ", ".join(
            "PVOID" if "OFFSET" in x.upper() else "DWORD" for x in args)
            or "VOID")
    # VMMcall _Name with nothing after it: the parameters are the pushes
    # before it (the last one first), or none; a service that takes
    # registers (mov eax, ... before it) is left out. "No return value"
    # in its description: void.
    for m in re.finditer(r"\b(?:VMMCall|VMMcall|VxDCall|VxDcall)\s+(_\w+)"
                         r"[ \t]*$", t, re.M):
        if m.group(1) not in names or m.group(1) in docs:
            continue
        before = t[:m.start()].split("\n")
        setup = []
        for line in reversed(before[-12:]):
            if re.match(r"\s*include\b", line, re.I):
                break
            setup.append(line.strip())
        else:
            continue
        if any(re.match(r"(mov|lea|xor|movzx)\b", x, re.I) for x in setup):
            continue
        pushes = [x for x in setup if re.match(r"push\b", x, re.I)]
        after = t[m.end():m.end() + 600]
        docs[m.group(1)] = (
            "VOID" if "No return value" in after else "DWORD",
            ", ".join("PVOID" if "OFFSET" in x.upper() else "DWORD"
                      for x in pushes) or "VOID")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--services",
                    default=os.path.join(HERE, "vxd-services.txt"))
    ap.add_argument("--docs", action="append", default=[],
                    help="a folder of the DDK's help, unpacked (7z x "
                    "win98ddk.chm): for services no header declares")
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
    # The help's pages, for the services the headers leave out: a C
    # prototype, or the assembler syntax (VMMCall _Name, <a, b>), whose
    # parameters are dwords (OFFSET32 ones pointers).
    docs = {}
    for d in a.docs:
        for root, _, files in os.walk(d):
            for f in files:
                if f.lower().endswith((".htm", ".html", ".doc")):
                    read_doc(os.path.join(root, f), names, docs)
    print("# The parameter types of the VxD services called like C functions:")
    print("# name, return type, parameter types. Made by gen-vxd-protos.py.")
    n = 0
    last = typedefs
    for name in sorted(names):
        sigs = []
        for kit, prio, ret, params, typedefs in found.get(name, []):
            s = signature(ret, params, typedefs)
            if s:
                sigs.append((kit, prio, s))
        if not sigs and name in docs:
            s = signature(docs[name][0], docs[name][1], last)
            if s:
                sigs.append((0, 0, s))
        if not sigs:
            continue
        # The newest kit's MAKE_HEADER, else its inline function.
        _, _, (ret, params) = max(sigs, key=lambda x: (x[0], x[1]))
        counts = {len(s[1]) for _, _, s in sigs}
        prefix = [p for p in params[:min(counts)] if p != "..."]
        if len(counts) > 1 and prefix:
            # Parameters added in a later version (_VCOMM_SetupComm's
            # pqsb): the ones every version has, then any more.
            params = prefix + ["..."]
        print("%s %s %s" % (name, ret, ",".join(params) or "void"))
        n += 1
    print("%d of %d services typed" % (n, len(names)), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
