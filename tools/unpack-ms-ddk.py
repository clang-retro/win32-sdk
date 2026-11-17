#!/usr/bin/env python3
"""Unpack a Windows 98 or ME DDK the way its setup program installs it.

    unpack-ms-ddk.py ARCHIVE OUTDIR

ARCHIVE is the kit as Microsoft shipped it: 98DDK.RAR or a folder of the
Windows 98 DDK's CD (CABS/I386/*.INF and *.CAB), or WMEDDK.ZIP or
WME-DDK.EXE (the Windows ME DDK). Each component's .INF names the files
in its .CAB and the folder each one goes to (inc\\win98, lib\\win_me, ...);
this puts them there, under OUTDIR, as setup would under C:\\98DDK or
C:\\WINMEDDK. import-ms-sdk.py then takes OUTDIR as the kit's folder
("4.10 DDK (unpacked)" or "4.90 DDK (unpacked)" in --sdks DIR).

It needs 7z (p7zip) to open the archives and cabinets, or unar for a
.RAR.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile


def extract(archive, dest):
    os.makedirs(dest, exist_ok=True)
    if archive.lower().endswith(".rar") and shutil.which("unar"):
        cmd = ["unar", "-q", "-f", "-o", dest, archive]
    else:
        seven = shutil.which("7z") or shutil.which("7za")
        if not seven:
            sys.exit("unpack-ms-ddk.py: needs 7z (p7zip) to open %s"
                     % archive)
        cmd = [seven, "x", "-y", "-o" + dest, archive]
    if subprocess.run(cmd, stdout=subprocess.DEVNULL).returncode:
        sys.exit("unpack-ms-ddk.py: could not open %s" % archive)


def find_infs(top):
    """The component .INFs that have a [DestinationDirs] section and a
    .CAB beside them, and, if there are none, the self-extracting .EXEs
    and archives to open next."""
    infs, nested = [], []
    for root, _, files in os.walk(top):
        lower = {f.lower(): f for f in files}
        for f in files:
            l = f.lower()
            if l.endswith(".inf") and l[:-4] + ".cab" in lower:
                infs.append((os.path.join(root, f),
                             os.path.join(root, lower[l[:-4] + ".cab"])))
            elif l.endswith((".zip", ".rar")) or re.match(r".*ddk.*\.exe$",
                                                           l):
                nested.append(os.path.join(root, f))
    return infs, nested


def parse_inf(path):
    """Return {section: [(name, name in the cabinet)]} for the sections
    [DestinationDirs] names, and {section: folder}."""
    sections, dests, sec = {}, {}, None
    with open(path, encoding="latin-1") as f:
        for line in f:
            line = line.split(";", 1)[0].strip()
            m = re.match(r"^\[(.+)\]$", line)
            if m:
                sec = m.group(1).strip().lower()
                continue
            if not line or sec is None:
                continue
            if sec == "destinationdirs":
                if "=" in line:
                    k, v = line.split("=", 1)
                    parts = [p.strip().strip('"') for p in v.split(",")]
                    if parts[0] == "49000":
                        dests[k.strip().lower()] = (
                            parts[1] if len(parts) > 1 else "")
                continue
            if "," in line and "=" not in line:
                name, src = [p.strip() for p in line.split(",")[:2]]
                sections.setdefault(sec, []).append((name, src))
    return {s: sections.get(s, []) for s in dests}, dests


def unpack(inf, cab, outdir, tmp):
    files, dests = parse_inf(inf)
    if not any(files.values()):
        return 0
    where = os.path.join(tmp, os.path.basename(cab))
    extract(cab, where)
    inside = {f.lower(): os.path.join(where, f) for f in os.listdir(where)}
    n = 0
    for sec, entries in files.items():
        folder = os.path.join(outdir, *dests[sec].split("\\"))
        for name, src in entries:
            got = inside.get(src.lower())
            if got is None:
                continue
            os.makedirs(folder, exist_ok=True)
            shutil.copyfile(got, os.path.join(folder, name.lower()))
            n += 1
    return n


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("archive")
    ap.add_argument("outdir")
    a = ap.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        top = a.archive
        if not os.path.isdir(top):
            top = os.path.join(tmp, "kit")
            extract(a.archive, top)
        for _ in range(3):
            infs, nested = find_infs(top)
            if infs or not nested:
                break
            for i, arc in enumerate(nested):
                extract(arc, os.path.join(top, "nested%d" % i))
        if not infs:
            sys.exit("unpack-ms-ddk.py: no setup .INF and .CAB in %s"
                     % a.archive)
        n = sum(unpack(inf, cab, a.outdir, os.path.join(tmp, "cabs"))
                for inf, cab in infs)
    print("%d files in %s" % (n, a.outdir), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
