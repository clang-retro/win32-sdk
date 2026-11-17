#!/usr/bin/env python3
"""Make vxdsvc.h and the service stubs of libvxd.a from vxd-services.txt.

    gen-vxd-wraps.py [--services FILE] --header vxdsvc.h --stubs DIR

vxdsvc.h defines VXDSVC_<name> for each service, the dword that follows
int 20h (the device ID and the service number), between #if VXD_WINVER
>= <version> and #endif, and declares the services called like C
functions: with their types where vxd-prototypes.txt has them
(gen-vxd-protos.py), else with no parameter list. DIR gets one assembly file for each of those: a stub that
jumps to the service (int 20h with the VxDJmp bit, which the VMM patches
into a jump), so that a call to the stub is a call to the service, with
the caller's arguments on the stack. scripts/build-vxd.sh assembles the
stubs into libvxd.a.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# Services vxd.h declares with its own types; the others called like C
# functions are declared here, with the types of vxd-prototypes.txt or
# with no parameter list (C17 and earlier).
TYPED = {
    "_HeapAllocate", "_HeapFree", "_HeapReAllocate", "_HeapGetSize",
    "_PageAllocate", "_PageFree", "_LinPageLock", "_LinPageUnLock",
    "_MapPhysToLinear", "_RegOpenKey", "_RegCreateKey", "_RegCloseKey",
    "_RegDeleteKey", "_RegQueryValueEx", "_RegSetValueEx",
    "_RegDeleteValue", "_RegFlushKey", "_VWIN32_QueueUserApc",
    "_VWIN32_SetWin32Event", "_VWIN32_ResetWin32Event",
    "_VWIN32_CloseVxDHandle", "_Register_Win32_Services",
}


def version_code(v):
    major, minor = v.split(".")
    return (int(major) << 8) | int(minor)


def read(path):
    services, seen = [], set()
    with open(path) as f:
        for line in f:
            if not line.strip() or line.startswith("#"):
                continue
            dev, did, num, name, conv, since = line.split()
            # One name can be in two tables (VMD and VMOUSE): the first
            # is kept.
            if name in seen:
                continue
            seen.add(name)
            services.append((dev, int(did, 16), int(num, 16), name, conv,
                             version_code(since)))
    return services


def read_prototypes(path):
    """{name: (return type, [parameter types])} from vxd-prototypes.txt."""
    out = {}
    if not os.path.exists(path):
        return out
    with open(path) as f:
        for line in f:
            if not line.strip() or line.startswith("#"):
                continue
            name, ret, params = line.split()
            c = lambda t: "void *" if t == "void*" else t
            out[name] = (c(ret), [] if params == "void" else
                         [c(t) for t in params.split(",")])
    return out


def symbol(name, conv):
    """The stub's symbol: the C name as clang decorates it."""
    if conv == "c":
        return "_" + name
    kind, n = conv.split("/")
    if kind == "stdcall":
        return "_%s@%d" % (name, int(n) * 4)
    return "@%s@%d" % (name, int(n) * 4)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--services",
                    default=os.path.join(HERE, "vxd-services.txt"))
    ap.add_argument("--prototypes",
                    default=os.path.join(HERE, "vxd-prototypes.txt"))
    ap.add_argument("--header", required=True)
    ap.add_argument("--stubs", required=True)
    a = ap.parse_args()
    services = read(a.services)
    protos = read_prototypes(a.prototypes)
    os.makedirs(a.stubs, exist_ok=True)
    out = ["/* vxdsvc.h: the services of the VMM and the VxDs of Windows 3.1,",
           "   95, 98 and ME. Made by gen-vxd-wraps.py from vxd-services.txt;",
           "   include vxd.h, which includes this. */",
           "#ifndef CLANG_RETRO_VXDSVC_H",
           "#define CLANG_RETRO_VXDSVC_H", ""]
    by_version = {}
    for s in services:
        by_version.setdefault(s[5], []).append(s)
    for since in sorted(by_version):
        out.append("#if VXD_WINVER >= 0x%04X" % since)
        for dev, did, num, name, conv, _ in by_version[since]:
            out.append("#define VXDSVC_%s 0x%08X /* %s */"
                       % (name, (did << 16) | num, dev))
        decls = []
        for dev, did, num, name, conv, _ in by_version[since]:
            if conv == "reg" or name in TYPED:
                continue
            if conv == "c" and name in protos:
                ret, params = protos[name]
                decls.append("%s %s(%s);" % (ret, name,
                                             ", ".join(params) or "void"))
            elif conv == "c":
                decls.append("DWORD %s();" % name)
            else:
                kind, n = conv.split("/")
                decls.append("DWORD __%s %s(%s);" % (
                    kind, name, ", ".join(["DWORD"] * int(n)) or "void"))
        if decls:
            out.append("#ifndef VXD_NO_SERVICE_DECLS")
            # C23 and C++ read () as no parameters: leave those out.
            old = [d for d in decls if d.endswith("();")]
            new = [d for d in decls if not d.endswith("();")]
            if old:
                out.append("#if !defined(__cplusplus) && "
                           "(!defined(__STDC_VERSION__) || "
                           "__STDC_VERSION__ < 202311L)")
                out += old
                out.append("#endif")
            out += new
            out.append("#endif")
        out.append("#endif")
        out.append("")
    out.append("#endif")
    with open(a.header, "w") as f:
        f.write("\n".join(out) + "\n")
    n = 0
    for dev, did, num, name, conv, _ in services:
        if conv == "reg":
            continue
        sym = symbol(name, conv)
        path = os.path.join(a.stubs, "svc_%s.s" % name)
        with open(path, "w") as f:
            f.write(".text\n.globl \"%s\"\n\"%s\":\n"
                    "\tint $0x20\n\t.long 0x%08X\t# %s, VxDJmp\n"
                    % (sym, sym, (did << 16) | num | 0x8000, dev))
        n += 1
    print("%d services, %d stubs" % (len(services), n), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
