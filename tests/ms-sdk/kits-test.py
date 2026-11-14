#!/usr/bin/env python3
"""Build kit-hello.c with every ms-*.cfg in a toolchain package, check the
programs' PE headers and, on Windows or with --wine, run them.

    kits-test.py PACKAGE [--clang CLANG] [--run] [EXTRA CLANG FLAGS]

The kernel-mode kits (ms-*-ddk.cfg) build the test driver ../ddk/hello.c
instead; --wine loads it as a service in Wine (WINEPREFIX32 for 32-bit
drivers, which needs a prefix made with WINEARCH=win32, WINEPREFIX for
64-bit ones) and runs ../ddk/client.c, built with the package's winxp.cfg
or win7-x64.cfg, which talks to it.

--stock-clang tests the kits with a clang from llvm.org.
"""
import argparse
import glob
import os
import re
import struct
import subprocess
import sys
import time

here = os.path.dirname(os.path.abspath(__file__))


def pe_info(path):
    d = open(path, "rb").read()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    machine = struct.unpack_from("<H", d, pe + 4)[0]
    opt = pe + 24
    osv = struct.unpack_from("<HH", d, opt + 40)
    subv = struct.unpack_from("<HH", d, opt + 48)
    subsystem = struct.unpack_from("<H", d, opt + 68)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    optsize = struct.unpack_from("<H", d, pe + 20)[0]
    rdata_rw = None
    for i in range(nsec):
        s = pe + 24 + optsize + 40 * i
        if d[s:s + 8].rstrip(b"\0") == b".rdata":
            rdata_rw = bool(struct.unpack_from("<I", d, s + 36)[0] & 0x80000000)
    return machine, osv, subv, rdata_rw, subsystem


def find_ddk():
    for d in (os.path.join(here, "..", "ddk"), os.path.join(here, "ddk")):
        if os.path.exists(os.path.join(d, "hello.c")):
            return d
    return None


def run_driver(sys_path, client, prefix):
    """Load the driver in Wine as a demand-start service and run client."""
    env = dict(os.environ)
    if prefix:
        env["WINEPREFIX"] = prefix
    root = env.get("WINEPREFIX", os.path.expanduser("~/.wine"))
    dst = os.path.join(root, "drive_c", "windows", "system32", "drivers",
                       "clanghello.sys")
    with open(sys_path, "rb") as f, open(dst, "wb") as g:
        g.write(f.read())

    def sc(*args):
        # Not into pipes: the service host that sc starts inherits them and
        # dies of SIGPIPE when it writes after sc has exited.
        return subprocess.run(["wine", "sc"] + list(args), env=env,
                              stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL).returncode
    sc("delete", "clanghello")
    sc("create", "clanghello", "type=", "kernel", "start=", "demand",
       "binpath=", "C:\\windows\\system32\\drivers\\clanghello.sys")
    for _ in range(5):
        if sc("start", "clanghello") == 0:
            break
        time.sleep(1)
    p = subprocess.run(["wine", client], env=env, capture_output=True,
                       text=True)
    sc("stop", "clanghello")
    return p.returncode, p.stdout.replace("\r", "").strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("package")
    ap.add_argument("--clang", default="clang")
    ap.add_argument("--run", action="store_true", help="run the programs")
    ap.add_argument("--wine", action="store_true", help="run them in Wine")
    ap.add_argument("--out", default=".")
    ap.add_argument("--stock-clang", action="store_true",
                    help="a clang without clang-dos's MSVC changes: pass the"
                    " subsystem version to the linker by hand")
    a, extra = ap.parse_known_args()
    a.extra = [x for x in extra if x != "--"]
    cfgs = sorted(glob.glob(os.path.join(a.package, "bin", "ms-*.cfg")))
    if not cfgs:
        print("no ms-*.cfg in %s/bin" % a.package)
        return 1
    failed = []
    ddk = find_ddk()
    clients = {}
    for cfg in cfgs:
        kit = os.path.basename(cfg)[:-4]
        triple = re.search(r"--target=(\S+)", open(cfg).read()).group(1)
        want = re.search(r"windows(\d+)\.(\d+)", triple)
        want = (int(want.group(1)), int(want.group(2)))
        driver = kit.endswith("-ddk")
        if driver and not ddk:
            print("%s: skipped (no ddk/hello.c)" % kit)
            continue
        exe = os.path.join(a.out, kit + ("-hello.sys" if driver
                                         else "-hello.exe"))
        src = (os.path.join(ddk, "hello.c") if driver
               else os.path.join(here, "kit-hello.c"))
        cmd = [a.clang, "--config=" + cfg, "-O2", src, "-o", exe] + a.extra
        if a.stock_clang:
            v = "%d.%d" % want
            # -Xlinker: -Wl would split the version at the comma.
            cmd += ["-Xlinker", "-subsystem:console," + v,
                    "-Xlinker", "-osversion:" + v]
            if want < (4, 0):
                cmd += ["-Xlinker", "-section:.rdata,RW"]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            print("%s: BUILD FAILED\n%s%s" % (kit, r.stdout, r.stderr))
            failed.append(kit)
            continue
        machine, osv, subv, rw, subsystem = pe_info(exe)
        note = "subsystem %d.%d, OS %d.%d" % (subv + osv)
        ok = subv == want and osv == want
        if driver:
            ok = ok and subsystem == 1
            note += ", %s" % ("native" if subsystem == 1 else
                              "NOT NATIVE (%d)" % subsystem)
            if a.wine:
                x64 = triple.startswith("x86_64")
                if x64 not in clients:
                    c = os.path.join(a.out, "client%d.exe" % (64 if x64
                                                              else 32))
                    ccfg = os.path.join(a.package, "bin", "win7-x64.cfg"
                                        if x64 else "winxp.cfg")
                    subprocess.run([a.clang, "--config=" + ccfg, "-O2",
                                    os.path.join(ddk, "client.c"), "-o", c],
                                   check=True)
                    clients[x64] = c
                prefix = None if x64 else os.environ.get("WINEPREFIX32")
                rc, out = run_driver(exe, clients[x64], prefix)
                note += "; loaded in Wine: %r (exit %d)" % (out, rc)
                ok = ok and rc == 0 and out == "hello from clang 428571431 1"
            print("%s: %s %s" % (kit, "ok" if ok else "FAILED", note))
            if not ok:
                failed.append(kit)
            continue
        if want < (4, 0):
            ok = ok and rw
            note += ", .rdata %s" % ("writable" if rw else "READ-ONLY")
        if a.run or a.wine:
            run = (["wine"] if a.wine else []) + [exe, "x"]
            p = subprocess.run(run, capture_output=True, text=True)
            out = p.stdout.strip()
            note += "; ran: %r (exit %d)" % (out, p.returncode)
            ok = ok and p.returncode == 0 and out.startswith("hello from kit")
            ok = ok and "3.25 1234567" in out and "argc=2" in out
        print("%s: %s %s" % (kit, "ok" if ok else "FAILED", note))
        if not ok:
            failed.append(kit)
    if failed:
        print("failed: " + " ".join(failed))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
