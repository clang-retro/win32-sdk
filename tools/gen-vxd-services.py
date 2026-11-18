#!/usr/bin/env python3
"""Make vxd-services.txt from the include files of Microsoft's VxD DDKs.

    gen-vxd-services.py VERSION=INCDIR ... > vxd-services.txt

Each INCDIR holds the MASM include files (*.inc) of one Windows DDK,
oldest version first:

    gen-vxd-services.py "3.10=3.1 DDK/ddk/WIN31/DDK/386/INCLUDE" \\
        "4.00=4.0 W95 DDK/ddk/INC32" \\
        "4.10=4.10 DDK (unpacked)/inc/win98" \\
        "4.90=4.90 DDK (unpacked)/inc/win_me" > tools/vxd-services.txt

A VxD's services are numbered in the order of its Begin_Service_Table
block. This writes one line for each service a VxD can call:

    DEVICE DEVICE_ID NUMBER NAME CONVENTION VERSION

CONVENTION is "reg" for a service that takes and returns registers, "c"
for one called like a C function (its name starts with an underscore),
and "stdcall/N" or "fastcall/N" for one that takes N dword arguments that
way; VERSION is the first version of Windows that has the service: the
first of the DDKs that lists it, or the version its table tests for
(WIN41SERVICES is 4.10, Windows 98). Only the names
and numbers are taken from the include files. gen-vxd-wraps.py makes the
headers and stubs of vxd/ from the list.
"""
import argparse
import os
import re
import sys

BEGIN = re.compile(r"^\s*Begin_Service_Table\s+(\w+)", re.I)
END = re.compile(r"^\s*End_Service_Table\b", re.I)
SERVICE = re.compile(r"^\s*(\w+?)_(Service|StdCall_Service|FastCall_Service)"
                     r"\s+(\w+)\s*(?:,(.*))?$", re.I)
DEVICE_ID = re.compile(r"^\s*(\w+)_Device_ID\s+EQU\s+<?\s*([^>]+)", re.I)
NUMBER = re.compile(r"^\s*(\w+)\s+EQU\s+([0-9][0-9A-F]*H|[0-9]+)\s*$", re.I)
COND = re.compile(r"^\s*(IFDEF|IFNDEF|IF|ELSE|ENDIF)\b\s*(\S*)", re.I)


def strip_comment(line):
    return line.split(";", 1)[0].rstrip()


def read_lines(path):
    with open(path, "rb") as f:
        return f.read().decode("latin-1").splitlines()


# The symbols the tables test that hold services of later versions of
# Windows: 4.03 is Windows 95 OSR2. Everything else is taken as not
# defined (NEC_98, MULTI_PROC_SUPPORT, WIN31COMPAT and the like).
DEFINED = {"WIN40SERVICES": "4.00", "WIN403SERVICES": "4.03",
           "WIN41SERVICES": "4.10", "WIN49SERVICES": "4.90",
           # Windows 95's VDD is a mini-VDD.
           "MINIVDD": None,
           # VMM.INC's, which MMDEVLDR.INC's table tests for.
           "DDB_SYS_CRIT_INIT_DONE": None}


def number(t):
    """A MASM number (0002AH, 10), or None."""
    if re.match(r"^[0-9][0-9A-F]*H$", t):
        return int(t[:-1], 16)
    if re.match(r"^[0-9]+$", t):
        return int(t)
    return None


def parse_dir(incdir, kit_version):
    """Return ({device: [(number, name, convention, version)]},
    {device: id})."""
    tables, ids, nums = {}, {}, {}
    for fn in sorted(os.listdir(incdir)):
        if not fn.lower().endswith(".inc"):
            continue
        lines = read_lines(os.path.join(incdir, fn))
        for line in lines:
            m = DEVICE_ID.match(strip_comment(line))
            if m:
                ids.setdefault(m.group(1).upper(), m.group(2).upper().strip())
            m = NUMBER.match(strip_comment(line))
            if m:
                nums.setdefault(m.group(1).upper(), number(m.group(2).upper()))
        dev, n = None, 0
        stack = []      # (active, version) for each IF block
        for line in lines:
            line = strip_comment(line)
            m = COND.match(line)
            if m:
                kw, sym = m.group(1).upper(), m.group(2).upper()
                if kw == "IFDEF":
                    stack.append((sym in DEFINED, DEFINED.get(sym)))
                elif kw == "IFNDEF":
                    stack.append((sym not in DEFINED, None))
                elif kw == "IF":
                    stack.append((True, None))
                elif kw == "ELSE" and stack:
                    stack[-1] = (not stack[-1][0], None)
                elif kw == "ENDIF" and stack:
                    stack.pop()
                continue
            if not all(a for a, _ in stack):
                continue
            if dev is None:
                m = BEGIN.match(line)
                if m and m.group(1).upper() != "MACRO":
                    dev, n, services = m.group(1).upper(), 0, []
                continue
            if END.match(line):
                # The first table of a device wins: some kits have two
                # include files for one device (VDD.INC, VDD2.INC).
                tables.setdefault(dev, services)
                dev = None
                continue
            m = SERVICE.match(line)
            if not m or m.group(1).upper() != dev:
                continue
            kind, name = m.group(2).lower(), m.group(3)
            args = [a.strip() for a in (m.group(4) or "").split(",")]
            if kind == "service":
                if args[0].upper() == "RESERVED":
                    name = None
                conv = "c" if name and name.startswith("_") else "reg"
            else:
                conv = "%s/%d" % (kind.split("_")[0], int(args[0], 0))
                if len(args) >= 2 and args[1].upper() == "RESERVED":
                    name = None
            if name is not None and (name.upper().startswith("RESERVED")
                                     or "_INVALID_SERVICE" in name.upper()):
                name = None
            version = max([v for _, v in stack if v] or [kit_version])
            services.append((n, name, conv, min(version, kit_version)))
            n += 1
    # An ID is a number (0002Ah), another device's (APM_DEVICE_ID) or
    # a sum of those and other numbers (Multimedia_OEM_ID + 10).
    def term(t, depth):
        if number(t) is not None:
            return number(t)
        if t.endswith("_DEVICE_ID"):
            return resolve(t[:-len("_DEVICE_ID")], depth + 1)
        return nums.get(t)

    def resolve(dev, depth=0):
        v = ids.get(dev)
        if v is None or depth > 8:
            return None
        total = 0
        for t in re.split(r"\s*\+\s*", v):
            n = term(t, depth)
            if n is None:
                return None
            total += n
        return total
    return tables, {dev: resolve(dev) for dev in tables}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("kits", nargs="+", metavar="VERSION=INCDIR")
    a = ap.parse_args()
    first = {}      # (device, number, name) -> (id, conv, version)
    for kit in a.kits:
        version, incdir = kit.split("=", 1)
        tables, ids = parse_dir(incdir, version)
        for dev, services in tables.items():
            if ids[dev] is None:
                # COMMDRVR: VCOMM's port drivers are called through
                # tables, not as services.
                print("%s: no %s_Device_ID, left out" % (incdir, dev),
                      file=sys.stderr)
                continue
            for n, name, conv, since in services:
                if name is None:
                    continue
                key = (dev, n, name)
                if key not in first or since < first[key][2]:
                    first[key] = (ids[dev], conv, since)
    print("# VxD services: device, device ID, service number, name,")
    print("# convention, first DDK. Made by gen-vxd-services.py.")
    for (dev, n, name), (did, conv, version) in sorted(
            first.items(), key=lambda kv: (kv[1][0], kv[0][0], kv[0][1],
                                           kv[1][2])):
        print("%s 0x%04X 0x%04X %s %s %s" % (dev, did, n, name, conv, version))
    return 0


if __name__ == "__main__":
    sys.exit(main())
