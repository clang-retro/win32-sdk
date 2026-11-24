#!/usr/bin/env python3
"""Make the folder of an installed Visual C++ from its CDs, for the vc* kits
of import-ms-sdk.py and for tests/msvc-abi/run-test.sh.

    python3 tools/vc-from-cd.py KIT OUT CD [CD...]

KIT is vc40, vc42, vc50, vc60 or vc71. OUT is the new folder. Each CD is a
folder that holds the files of one CD: extract the CD image or the
self-extracting archive first (7-Zip opens ISO images and the WinZip
archives; the RAR archives of Visual C++ 4.2 need unrar).

The tool copies only the compiler, headers, libraries and runtime DLLs, in
the layout that setup made:

  vc40, vc42  MSDEV
  vc50        VC and SharedIDE/BIN (from DEVSTUDIO)
  vc60        VC98 and Common/MSDev98/Bin. The CD has short names for the
              C++ headers ("algrithm" for "algorithm"). The tool gives the
              long names from the setup file SETUP/VS98*.INF.
  vc71        Vc7, Common7/IDE and redist. Some files (the Platform SDK)
              are in the cabinets of vs_setup.msi. The tool extracts them
              with msiextract (from msitools). redist gets msvcr71.dll,
              msvcp71.dll and the MFC 7.1 DLLs from Win/System.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

VS2003 = "Program Files/Microsoft Visual Studio .NET 2003"


def find_ci(base, rel):
    """The path rel under base, matching each name without case, or None."""
    cur = base
    for part in rel.split("/"):
        try:
            match = [e for e in sorted(os.listdir(cur))
                     if e.lower() == part.lower()]
        except OSError:
            return None
        if not match:
            return None
        cur = os.path.join(cur, match[0])
    return cur


def copy_tree(src, dst):
    """Copy the folder src into dst. A file that dst has already stays, and
    names that differ only in case go into one folder."""
    for root, dirs, files in os.walk(src):
        rel = os.path.relpath(root, src)
        out = dst if rel == "." else os.path.join(dst, rel)
        out = merge_case(dst, os.path.relpath(out, dst))
        os.makedirs(out, exist_ok=True)
        have = {e.lower() for e in os.listdir(out)}
        for f in files:
            if f.lower() not in have:
                shutil.copy2(os.path.join(root, f), os.path.join(out, f))


def merge_case(base, rel):
    """The folder rel under base, with the case of a folder that is there
    already: "Include" and "include" become one folder."""
    if rel == ".":
        return base
    cur = base
    for part in rel.split(os.sep):
        have = find_ci(cur, part) if os.path.isdir(cur) else None
        cur = have if have else os.path.join(cur, part)
    return cur


def copy_from_cds(cds, rel, dst):
    """Copy the folder rel of each CD that has it into dst. Fails if no CD
    has it."""
    found = False
    for cd in cds:
        src = find_ci(cd, rel)
        if src and os.path.isdir(src):
            copy_tree(src, dst)
            found = True
    if not found:
        sys.exit("no %s on the CDs" % rel)


def vc6_long_names(cds, out):
    """Give the files in out the long names that the setup file of Visual
    C++ 6.0 has, for example "vc98\\include\\algrithm<algorithm>"."""
    entry = re.compile(r'^"[^"]*"\s*=\s*\d+,([^,<]+)<([^>,]+)>', re.M)
    n = 0
    for cd in cds:
        setup = find_ci(cd, "SETUP")
        if not setup:
            continue
        for inf in sorted(os.listdir(setup)):
            if not re.match(r"vs98.*\.inf$", inf, re.I):
                continue
            text = open(os.path.join(setup, inf), encoding="latin-1").read()
            for short, long in entry.findall(text):
                parts = short.replace("\\", "/").split("/")
                if parts[0].lower() != "vc98":
                    continue
                path = find_ci(out, "/".join(["VC98"] + parts[1:]))
                if path and os.path.basename(path) != long:
                    os.rename(path, os.path.join(os.path.dirname(path), long))
                    n += 1
    if n == 0:
        sys.exit("no long names in SETUP/VS98*.INF")
    print("vc60: gave %d files their long names" % n)


def msi_names(rel):
    """The installed path of a file that msiextract wrote: it keeps the
    names of the MSI's Directory table ("Vc7:Vc7|Vc7", "PSDK|PlatformSDK",
    ".:common"). The installed name is the long name of the target part."""
    out = []
    for p in rel.split(os.sep):
        p = p.split(":")[0].split("|")[-1]
        if p != ".":
            out.append(p)
    return os.path.join(*out) if out else ""


def vc71_msi(cds, out):
    """Copy the files of Vc7 and Common7/IDE from the cabinets of
    vs_setup.msi into out."""
    msi = None
    for cd in cds:
        msi = msi or find_ci(cd, "vs_setup.msi")
    if not msi:
        sys.exit("no vs_setup.msi on the CDs")
    if not shutil.which("msiextract"):
        sys.exit("no msiextract: install msitools")
    tmp = tempfile.mkdtemp(prefix="vc71-msi.")
    try:
        subprocess.run(["msiextract", "-C", tmp, msi], check=True,
                       stdout=subprocess.DEVNULL)
        for root, _, files in os.walk(tmp):
            for f in files:
                rel = msi_names(os.path.relpath(os.path.join(root, f), tmp))
                rel = rel.split(VS2003.replace("/", os.sep) + os.sep, 1)
                if len(rel) != 2:
                    continue
                rel = rel[1]
                if not (rel.startswith("Vc7" + os.sep) or
                        rel.startswith(os.path.join("Common7", "IDE") + os.sep)):
                    continue
                dst = os.path.join(merge_case(out, os.path.dirname(rel)),
                                   os.path.basename(rel))
                if os.path.exists(dst):
                    continue
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.move(os.path.join(root, f), dst)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def vc71_redist(cds, out):
    """Copy the runtime DLLs of Visual C++ .NET 2003 from Win/System of the
    CDs into out/redist."""
    dst = os.path.join(out, "redist")
    os.makedirs(dst, exist_ok=True)
    want = re.compile(r"(msvcr71|msvcp71|mfc71[a-z]*)\.dll$", re.I)
    for cd in cds:
        system = find_ci(cd, "Win/System")
        if not system:
            continue
        for f in sorted(os.listdir(system)):
            if want.match(f) and not os.path.exists(os.path.join(dst, f.lower())):
                shutil.copy2(os.path.join(system, f), os.path.join(dst, f.lower()))


def main():
    """Parse the arguments and make the folder for the kit."""
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("kit", choices=["vc40", "vc42", "vc50", "vc60", "vc71"])
    ap.add_argument("out")
    ap.add_argument("cds", nargs="+")
    a = ap.parse_args()
    if os.path.exists(a.out) and os.listdir(a.out):
        ap.error("%s is not empty" % a.out)
    os.makedirs(a.out, exist_ok=True)
    if a.kit in ("vc40", "vc42"):
        copy_from_cds(a.cds, "MSDEV", os.path.join(a.out, "MSDEV"))
    elif a.kit == "vc50":
        copy_from_cds(a.cds, "DEVSTUDIO/VC", os.path.join(a.out, "VC"))
        copy_from_cds(a.cds, "DEVSTUDIO/SHAREDIDE/BIN",
                      os.path.join(a.out, "SharedIDE", "BIN"))
    elif a.kit == "vc60":
        copy_from_cds(a.cds, "VC98", os.path.join(a.out, "VC98"))
        copy_from_cds(a.cds, "COMMON/MSDEV98/BIN",
                      os.path.join(a.out, "Common", "MSDev98", "Bin"))
        vc6_long_names(a.cds, a.out)
    else:
        copy_from_cds(a.cds, VS2003 + "/Vc7", os.path.join(a.out, "Vc7"))
        copy_from_cds(a.cds, VS2003 + "/Common7/IDE",
                      os.path.join(a.out, "Common7", "IDE"))
        vc71_msi(a.cds, a.out)
        vc71_redist(a.cds, a.out)
    print("%s: %s" % (a.kit, a.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
