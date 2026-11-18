#!/usr/bin/env python3
"""Make the VxD subsystem headers in vxd/ (configmg.h, ifs.h, ios.h, ...)
from the facts in Microsoft's 95, 98 and ME DDKs: the names and values of
their constants and the names, types and layouts of their structures,
unions, enumerations and typedefs.

    gen-vxd-headers.py [--kits DIR] CFGDIR [HEADER ...]

CFGDIR is a toolchain package's bin/ with the ms-w95-vxd, ms-w98-vxd and
ms-me-vxd kits that import-ms-sdk.py makes from the DDKs. DIR holds the
DDKs as import-ms-sdk.py found them ("4.0 W95 DDK", "4.10 DDK
(unpacked)", "4.90 DDK (unpacked)"), for the block device headers in
their source trees (dcb.h, iop.h ...), which the kits don't have. For each
header in SPECS (all of them by default) this compiles the DDK's header
with each kit, reads the declarations from clang's AST and the constants
from the preprocessor (each evaluated by compiling it), and writes:

  vxd/include/NAME.h  the declarations, each under
                      #if VXD_WINVER >= the first DDK that has it, with the
                      introduction from SPECS
  vxd/layout/NAME.c   static asserts of each structure's size and its
                      fields' offsets as the DDK's compiler laid them out
  vxd/include/vxdtypes.h  the DDK base types (from basedef.h, vmm.h ...) the
                      headers use

Only names, numbers and types are taken: no comments, no code (inline
functions, function-like macros and the service wrappers, which
<vxdsvc.h> already has, are left out), and each type is written the way
clang prints it. A constant whose value differs between DDKs gets the
latest one, and the script says so on stderr.
"""
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
VXD = os.path.join(HERE, "..", "vxd")
INCLUDE = os.path.join(VXD, "include")
LAYOUT = os.path.join(VXD, "layout")
GATE = "VXD_WINVER"     # what each declaration's #if tests
PRELUDE = ["vxd.h", "vxdtypes.h"]  # what each header includes first
PACKED = True           # the whole header is #pragma pack(1), as -Zp1
EXTRA = []              # more flags for the DDK's compiler

KITS = [("0x0400", "ms-w95-vxd"), ("0x040A", "ms-w98-vxd"),
        ("0x045A", "ms-me-vxd")]
# Where each DDK keeps the block device headers IOS's port drivers use
# (dcb.h and the rest are not in its inc), under --kits.
BLOCK = {"ms-w95-vxd": "4.0 W95 DDK/ddk/BLOCK/INC",
         "ms-w98-vxd": "4.10 DDK (unpacked)/src/block/inc",
         "ms-me-vxd": "4.90 DDK (unpacked)/src/win_me/block/inc"}
BASE = ["basedef.h", "vmm.h", "vxdwraps.h"]

# name: (DDK headers to read, DDK headers to include first, introduction)
SPECS = {}


def spec(name, pre, intro, files=None):
    SPECS[name] = (files or [name + ".h"], pre, intro)


spec("vmmreg", [], """The VMM's registry: REG_* value types and errors, the
   HKEY_* roots and the structures _RegQueryMultipleValues and the
   registry services take.""")
spec("configmg", ["vmmreg.h"], """The Configuration Manager (CONFIGMG), Plug and
   Play for VxDs: CONFIGRET and the CR_* results, the CM_* flags and
   function codes (CONFIG_START ...), the resource descriptors (MEM_DES,
   IO_DES, DMA_DES, IRQ_DES and the ResType_*) and CMCONFIG, the
   configuration a device was given, and the types of the handlers a
   driver registers (CMCONFIGHANDLER, CMENUMHANDLER, CMARBHANDLER). The
   services are _CONFIGMG_* in <vxdsvc.h>.""")
spec("vpicd", [], """The virtualized interrupt controller: the VPICD_* flags
   and the structures for virtualizing an IRQ.""")
spec("vdmad", [], """The virtual DMA device: DMA_* modes and flags and the
   structures of a DMA region and buffer.""")
spec("vtd", [], """The virtual timer device: its flags and structures.""")
spec("vwin32", [], """VWIN32, the Win32 side of the VMM: the structures for
   events, APCs, threads and the Win32 services (DIOCParams is in
   <vxd.h>).""")
spec("vxdldr", [], """VXDLDR, the loader of dynamic VxDs: its errors and
   the structures describing a loaded device.""")
spec("shell", [], """The SHELL device: message box flags and the
   structures of its services and its broadcast and Windows event
   hooks.""")
spec("vpowerd", [], """The power management device (APM): power states,
   events and the structures of its services.""")
spec("pci", ["configmg.h"], """The PCI bus: the configuration space layout
   and the PCI enumerator's constants.""")
spec("pccard", ["configmg.h"], """PC Card (PCMCIA) Socket Services and Card
   Services: their function codes, return codes and structures.""")
spec("vcache", [], """VCACHE, the block cache file systems use: its buffer
   handles, flags and structures.""")
spec("ifs", ["vmmreg.h"], """The installable file system manager (IFSMgr):
   ioreq, the request a file system driver or a file system API hook
   gets, the IFSFN_* function numbers, the file and volume handle
   function tables, the error codes and the structures of the IFSMgr
   services.""")
spec("ior", ["configmg.h"], """IOS's I/O request (IOR) and its IOR_* functions
   and flags.""")
spec("blockdev", ["configmg.h", "ior.h"], """The block device descriptor
   (BLOCKDEV and BDD) and its commands.""")
spec("drp", ["configmg.h", "ior.h"], """The driver registration packet (DRP)
   a layered block device driver gives IOS.""")
spec("ilb", ["configmg.h", "ior.h"], """The IOS linkage block (ILB) IOS gives
   each driver.""")
spec("sgd", ["configmg.h", "ior.h"], """Scatter-gather descriptors.""")
spec("med", ["configmg.h", "ior.h"], """IOS memory element descriptors.""")
spec("vrp", ["configmg.h", "ior.h"], """The volume request parameters (VRP)
   IOS keeps for each volume.""")
spec("aep", ["configmg.h", "ior.h", "drp.h", "ilb.h"], """The asynchronous
   event packets (AEP) IOS sends each driver's async event routine, and
   the AEP_* functions.""")
spec("isp", ["configmg.h", "ior.h", "ilb.h"], """The IOS service packets (ISP)
   drivers send to IOS.""")
spec("irs", ["configmg.h", "ior.h", "ilb.h"], """The IOS requestor services
   (IRS) packets that VxDs above IOS send it.""")
spec("ios", ["configmg.h", "ior.h", "blockdev.h", "drp.h", "ilb.h",
             "aep.h", "isp.h"], """The I/O supervisor (IOS): the layers of
   its calldown chain, the DCB a port driver makes for each device and
   the flags of both.""")
spec("dcb", ["configmg.h", "ior.h", "blockdev.h"], """The device control
   block (DCB) a port driver makes for each device it finds, and the
   DCB_* types and flags. (In the DDKs' block/inc.)""")
spec("iop", ["configmg.h", "ior.h", "blockdev.h", "dcb.h"], """The I/O packet
   (IOP) around each IOR in IOS's calldown chain. (In the DDKs'
   block/inc.)""")
spec("ida", ["configmg.h", "ior.h", "blockdev.h", "dcb.h"], """The IOS data
   area (IDA) of IOS's global data. (In the DDKs' block/inc.)""")
spec("ddb", ["configmg.h", "ior.h"], """IOS's driver data block. (In the DDKs'
   block/inc.)""")
spec("dvt", ["configmg.h", "ior.h", "blockdev.h", "dcb.h"], """The driver
   vector table (DVT) IOS keeps for each driver. (In the DDKs'
   block/inc.)""")
spec("ivt", ["configmg.h", "ior.h"], """IOS's vector table. (In the DDKs'
   block/inc.)""")
spec("ndis", [], """NDIS 3.1 on Windows 95 to ME: the NDIS_* types, OIDs,
   statuses and the structures a miniport or protocol driver passes
   NDIS.""")
spec("netvxd", ["ndis.h"], """Network VxDs: their initialization order and
   the structures shared by network transports and drivers.""")
spec("minivdd", [], """Display mini-VDDs: the VDD's function numbers
   (REGISTER_DISPLAY_DRIVER ...) and the structures of its
   callbacks.""")

# --miniport: SCSI miniports (.MPD files), which are NT-style PE drivers
# that import only SCSIPORT.SYS (SCSIPORT.PDR on Windows 9x). Their headers
# are the DDKs' block/inc miniport.h, srb.h and scsi.h, whose structures
# are those of Windows NT 3.51 (mingw-w64's are 2000's, with larger
# HW_INITIALIZATION_DATA and PORT_CONFIGURATION_INFORMATION, which
# SCSIPORT.PDR rejects). They are written to tests/../miniport/include.
MINIPORT_SPECS = {
    "miniport": (["miniport.h"], [], """The types of a SCSI miniport for
   Windows 95, 98 and ME: the NT base types it uses (ULONG, BOOLEAN,
   PHYSICAL_ADDRESS ...), INTERFACE_TYPE, the bus data structures (PCI,
   EISA, MCA) ScsiPortGetBusData returns, and the access ranges."""),
    "srb": (["srb.h"], ["miniport.h"], """The SCSI request block (SRB) and
   what a miniport hands SCSIPORT: HW_INITIALIZATION_DATA, the
   PORT_CONFIGURATION_INFORMATION it fills in, the SRB_* functions,
   statuses and flags, and the ScsiPort* functions' notification types.
   The functions are in SCSIPORT.SYS (-lscsiport); Windows 9x's
   SCSIPORT.PDR has the 45 that tools/scsiport9x-exports.txt lists."""),
    "scsi": (["scsi.h"], ["miniport.h", "srb.h"], """SCSI itself: the CDB
   layouts, the SCSI operation codes (SCSIOP_*), sense data and inquiry
   data."""),
}

# --wdm: headers mingw-w64 lacks for WDM and NT drivers, from the 2000 and
# XP DDKs (which 98's and ME's WDM share); the NT types they use come from
# mingw-w64's <ntddk.h>.
WDM_SPECS = {
    "hidport": (["hidport.h"], ["ntddk.h", "hidusage.h", "hidclass.h"],
                """What a HID
   minidriver (HIDUSB.SYS's kind, under HIDCLASS.SYS) needs:
   HID_MINIDRIVER_REGISTRATION for HidRegisterMinidriver (-lhidclass),
   HID_DEVICE_EXTENSION, the HID descriptor, and the internal IOCTL_HID_*
   requests HIDCLASS sends the minidriver."""),
    "1394": (["1394.h"], ["ntddk.h"], """The IEEE 1394 (FireWire) bus:
   the I/O request block (IRB) a 1394 client driver sends the bus driver
   in IOCTL_1394_CLASS requests, the REQUEST_* functions, address ranges,
   isochronous resources, and the bus and node structures. Windows 98,
   98 SE and ME (1394BUS.SYS) and 2000 and later take the same IRBs."""),
}

KEYWORDS = set("""auto break case char const continue default do double else
enum extern float for goto if inline int long register restrict return short
signed sizeof static struct switch typedef union unsigned void volatile while
_Bool __int64 __int32 __int16 __int8 __attribute__ __stdcall __cdecl
__fastcall cdecl stdcall fastcall __ptr32 __unaligned __w64 _Atomic
__restrict __inline""".split())


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


class Kit:
    def __init__(self, cfgdir, cfg, kits=None):
        self.cfgdir = cfgdir
        self.clang = os.path.join(cfgdir, "clang")
        self.cfg = os.path.join(cfgdir, cfg + ".cfg")
        self.tmp = tempfile.mkdtemp()
        # poppack.h and pshpack*.h are missing from some DDKs' inc.
        with open(os.path.join(self.tmp, "poppack.h"), "w") as f:
            f.write("#pragma pack(pop)\n")
        for n in (1, 2, 4, 8):
            with open(os.path.join(self.tmp, "pshpack%d.h" % n), "w") as f:
                f.write("#pragma pack(push, %d)\n" % n)
        # The block headers, by lower-case name (the 95 DDK's are upper
        # case).
        self.block = os.path.join(self.tmp, "block")
        os.mkdir(self.block)
        src = os.path.join(kits, BLOCK[cfg]) if kits and cfg in BLOCK \
            else None
        if src and os.path.isdir(src):
            for f in os.listdir(src):
                os.symlink(os.path.join(src, f),
                           os.path.join(self.block, f.lower()))

    def cmd(self, *args):
        return [self.clang, "--config=" + self.cfg, "-idirafter", self.tmp,
                "-idirafter", self.block, "-ferror-limit=0", "-w"] + EXTRA + \
            list(args)

    def source(self, includes, extra=""):
        p = os.path.join(self.tmp, "t.c")
        with open(p, "w") as f:
            if PACKED:
                f.write("#define WANTVXDWRAPS\n")
            else:
                f.write("\n")
            for h in includes:
                f.write("#include <%s>\n" % h)
            f.write(extra)
        return p


# ---- the AST ----

class Walker:
    """Follows clang's JSON AST, which names a location's file only when
    it differs from the last one written."""

    def __init__(self):
        self.file = None

    def bare(self, loc):
        if loc and "file" in loc:
            self.file = loc["file"]

    def loc(self, loc):
        if not loc:
            return self.file
        if "spellingLoc" in loc:
            self.bare(loc["spellingLoc"])
            self.bare(loc["expansionLoc"])
        else:
            self.bare(loc)
        return self.file

    def node(self, n):
        f = self.loc(n.get("loc"))
        r = n.get("range") or {}
        self.loc(r.get("begin"))
        self.loc(r.get("end"))
        n["_file"] = f
        for c in n.get("inner", ()):
            self.node(c)


ANON = re.compile(r"(struct|union|enum) [\w: ]*?"
                  r"(?:\((?:anonymous|unnamed)[^)]*\)::)*"
                  r"\((?:unnamed|anonymous)[^)]* at [^)]*\)")


def ident_tokens(t):
    return [w for w in re.findall(r"[A-Za-z_]\w*", t)
            if w not in KEYWORDS]


def clean_type(t):
    t = t.replace(" __attribute__((cdecl))", "")
    t = t.replace("__attribute__((cdecl))", "")
    return t


def owns(typedef, rec_id):
    """Whether the typedef's type is the record (or enum) rec_id itself."""
    for c in typedef.get("inner", []):
        if c.get("kind") in ("RecordType", "EnumType", "ElaboratedType"):
            if (c.get("decl") or {}).get("id") == rec_id:
                return True
            if c.get("kind") == "ElaboratedType" and owns(c, rec_id):
                return True
    return False


def declarator(t, name):
    t = clean_type(t)
    m = re.search(r"\(\*+\)", t)
    if m:
        return t[:m.start()] + m.group(0)[:-1] + name + ")" + t[m.end():]
    m = re.match(r"([^()]*?) ?(\(.*\))( __attribute__.*)?$", t)
    if m:  # a function type
        return "%s %s%s%s" % (m.group(1), name, m.group(2), m.group(3) or "")
    if t.endswith("]"):
        i = t.index("[")
        return t[:i].rstrip() + " " + name + t[i:]
    sep = "" if t.endswith("*") else " "
    return t + sep + name


class Decls:
    """The declarations of one kit's TU, by name, in order."""

    def __init__(self, ast, files):
        self.items = []    # (key, text, deps, file)
        self.records = {}  # key -> list of (field paths) for layouts
        self.anon = {}
        self.files = files
        self.byid = {}
        top = ast.get("inner", [])
        for n in top:
            self.byid[n.get("id")] = n
        self.pending_anon = None
        for i, n in enumerate(top):
            self.top(n, top, i)

    def mine(self, n):
        f = n.get("_file") or ""
        return os.path.basename(f).lower() in self.files

    def fix(self, t):
        def sub(m):
            return self.anon.get(m.group(0), m.group(0))
        return ANON.sub(sub, t)

    def record_body(self, n, indent, deps, hoist):
        out = []
        inner = n.get("inner", [])
        pend = None
        named = {}
        for c in inner:
            k = c.get("kind")
            if k == "RecordDecl" and c.get("completeDefinition"):
                if c.get("name"):
                    hoist.append(c)
                    named["%s %s" % (c.get("tagUsed"), c["name"])] = c
                    continue
                pend = c
                continue
            if k != "FieldDecl":
                continue
            t = c["type"]["qualType"]
            name = c.get("name", "")
            if not name and t in named:
                # An anonymous member of a tagged type (Microsoft's
                # struct TAG { ... }; in a union): its fields, anonymous.
                r = named[t]
                body = self.record_body(r, indent + "  ", deps, [])
                out.append("%s%s {\n%s%s};" % (indent, r.get("tagUsed"),
                                                body, indent))
                continue
            if pend is not None and ANON.search(t):
                tag = pend.get("tagUsed", "struct")
                body = self.record_body(pend, indent + "  ", deps, hoist)
                rest = ANON.sub("", t, count=1).strip()
                out.append("%s%s {\n%s%s}%s%s;" % (
                    indent, tag, body, indent,
                    (" " + rest) if rest else "",
                    (" " + name) if name and not c.get("isImplicit") else ""))
                pend = None
                continue
            t = self.fix(t)
            deps.update(ident_tokens(t))
            d = declarator(t, name)
            if c.get("isBitfield"):
                w = self.bitwidth(c)
                d += " : %d" % w
            out.append(indent + d + ";")
        return "".join(l + "\n" for l in out)

    def bitwidth(self, c):
        for x in c.get("inner", []):
            if "value" in x:
                return int(x["value"])
            for y in x.get("inner", []):
                if "value" in y:
                    return int(y["value"])
        raise ValueError("bit-field width of " + c.get("name", "?"))

    def emit_record(self, n, name_for_anon=None):
        deps = set()
        hoist = []
        body = self.record_body(n, "  ", deps, hoist)
        for h in hoist:
            hh, hb, hd, _ = self.emit_record(h)
            self.add_record(h, hh, hb, hd)
        texts = []
        tag = n.get("tagUsed", "struct")
        nm = n.get("name")
        head = "%s %s" % (tag, nm) if nm else tag
        return (head, body, deps, texts)

    def add(self, key, text, deps, n):
        self.items.append((key, text, set(deps), n.get("_file")))

    def top(self, n, top, i):
        k = n.get("kind")
        if k == "RecordDecl":
            if not n.get("completeDefinition"):
                if self.mine(n) and n.get("name"):
                    key = "tag " + n["name"]
                    self.add(key + " fwd", "%s %s;" % (n["tagUsed"],
                                                       n["name"]), (), n)
                return
            if not n.get("name"):
                self.pending_anon = n
                return
            head, body, deps, _ = self.emit_record(n)
            self.add_record(n, head, body, deps)
        elif k == "EnumDecl":
            if re.search(r"_SERVICES$|_Services$", n.get("name") or ""):
                return  # a service table: <vxdsvc.h> numbers them
            vals = []
            v = -1
            for c in n.get("inner", []):
                if c.get("kind") != "EnumConstantDecl":
                    continue
                val = self.enumval(c)
                v = val if val is not None else v + 1
                vals.append((c["name"], v))
            if not n.get("name"):
                self.pending_anon = n
                n["_vals"] = vals
                if not any(t.get("kind") == "TypedefDecl" and
                           (ANON.search(t["type"]["qualType"]) or
                            owns(t, n["id"])) for t in top[i + 1:i + 2]):
                    # A bare enum {...}: its constants only.
                    self.add("enum@" + vals[0][0] if vals else "enum@",
                             "enum {\n%s};" % self.enumbody(vals), (), n)
                    self.pending_anon = None
                return
            self.add("tag " + n["name"], "enum %s {\n%s};" % (
                n["name"], self.enumbody(vals)), (), n)
        elif k == "FunctionDecl" and not PACKED:
            # A prototype (types only, no parameter names); not an inline
            # function.
            if n.get("isImplicit") or n.get("storageClass") == "static" or \
                    any(c.get("kind") == "CompoundStmt"
                        for c in n.get("inner", [])):
                return
            t = self.fix(n["type"]["qualType"])
            self.add("func " + n["name"], declarator(t, n["name"]) + ";",
                     ident_tokens(t), n)
        elif k == "TypedefDecl":
            if n.get("isImplicit"):
                return
            t = n["type"]["qualType"]
            name = n["name"]
            a = self.pending_anon
            if a is not None and owns(n, a["id"]):
                # typedef struct { ... } NAME: written as struct NAME
                # { ... } and typedef struct NAME NAME, the way clang
                # names it (struct NAME) and the same as a DDK that
                # tags it.
                self.pending_anon = None
                if a.get("kind") == "EnumDecl":
                    self.add("tag " + name, "enum %s {\n%s};" % (
                        name, self.enumbody(a["_vals"])), (), n)
                    self.add("type " + name, "typedef enum %s %s;" % (
                        name, name), (), n)
                    return
                a = dict(a, name=name)
                head, body, deps, _ = self.emit_record(a)
                a["_file"] = n.get("_file")
                self.add_record(a, head, body, deps)
                self.add("type " + name, "typedef %s %s;" % (head, name),
                         (), n)
                return
            m = ANON.search(t)
            if m and m.group(0) not in self.anon and self.pending_anon:
                a = self.pending_anon
                self.pending_anon = None
                self.anon[m.group(0)] = name
                rest = ANON.sub("", t, count=1).strip()
                if a.get("kind") == "EnumDecl":
                    text = "typedef enum {\n%s} %s;" % (
                        self.enumbody(a["_vals"]), declarator(rest, name)
                        if rest else name)
                    self.add("type " + name, text, (), n)
                    return
                head, body, deps, hoisted = self.emit_record(a)
                text = "typedef %s {\n%s} %s;" % (
                    head, body, declarator(rest, name) if rest else name)
                self.add("type " + name, text, deps, n)
                if not rest:
                    self.layout_of(a, name, n)
                return
            t = self.fix(t)
            deps = ident_tokens(t)
            self.add("type " + name, "typedef %s;" % declarator(t, name),
                     deps, n)

    def enumval(self, c):
        for x in c.get("inner", []):
            if "value" in x:
                return int(x["value"])
            for y in x.get("inner", []):
                if "value" in y:
                    return int(y["value"])
        return None

    def enumbody(self, vals):
        return "".join("  %s = %d,\n" % (a, b) for a, b in vals)

    def add_record(self, n, head, body, deps):
        self.add("tag " + n["name"], "%s {\n%s};" % (head, body), deps, n)
        self.layout_of(n, head, n)

    def layout_of(self, rec, name, n):
        if not self.mine(n):
            return
        fields = []
        self.fields(rec, "", fields)
        self.records[name] = fields

    def fields(self, rec, prefix, out):
        pend = None
        named = {}
        for c in rec.get("inner", []):
            k = c.get("kind")
            if k == "RecordDecl" and c.get("completeDefinition"):
                if c.get("name"):
                    named["%s %s" % (c.get("tagUsed"), c["name"])] = c
                else:
                    pend = c
                continue
            if k != "FieldDecl" or c.get("isBitfield"):
                continue
            name = c.get("name", "")
            t = c["type"]["qualType"]
            if not name and t in named:
                self.fields(named[t], prefix, out)
                continue
            if pend is not None and ANON.search(t):
                if name and not c.get("isImplicit"):
                    out.append(prefix + name)
                    self.fields(pend, prefix + name + ".", out)
                else:
                    self.fields(pend, prefix, out)
                pend = None
                continue
            if name:
                out.append(prefix + name)


def ast_of(kit, includes):
    src = kit.source(includes)
    r = run(kit.cmd("-fsyntax-only", "-Xclang", "-ast-dump=json", src))
    if not r.stdout:
        sys.exit("clang failed:\n" + r.stderr[:3000])
    errors = len(re.findall(r"error:", r.stderr))
    ast = json.loads(r.stdout)
    Walker().node(ast)
    return ast, errors, r.stderr


# ---- the constants ----

DEFINE = re.compile(r"#define ([A-Za-z_]\w*)(\(?)\s*(.*)$")
STRING = re.compile(r'^(?:L?"(?:[^"\\]|\\.)*"\s*)+$')
CHAR = re.compile(r"^L?'(?:[^'\\]|\\.)+'$")


def macros_of(kit, includes, files):
    src = kit.source(includes)
    r = run(kit.cmd("-E", "-dD", src))
    out = {}
    order = []
    cur = None
    for line in r.stdout.splitlines():
        m = re.match(r'# \d+ "([^"]*)"', line)
        if m:
            cur = os.path.basename(m.group(1)).lower()
            continue
        if line.startswith("#undef "):
            out.pop(line.split()[1], None)
            continue
        m = DEFINE.match(line)
        if not m or m.group(2):
            continue
        name, body = m.group(1), m.group(3).strip()
        if cur in files:
            if name not in out:
                order.append(name)
            out[name] = body
        else:
            out.pop(name, None)
    return [(n, out[n]) for n in order if n in out]


def enum_values(kit, includes, exprs):
    """Evaluates each integer constant expression by compiling it into a
    data object; None for one that isn't one."""
    live = list(range(len(exprs)))
    base = len(includes) + 2
    for _ in range(10):
        src = kit.source(includes, "".join(
            "long long __x%d = (%s) + 0LL;\n" % (i, exprs[i]) for i in live))
        r = run(kit.cmd("-c", "-fno-zero-initialized-in-bss", "-o",
                        src + ".o", src))
        bad = {int(m.group(1)) - base for m in
               re.finditer(r"t\.c:(\d+):\d+: error", r.stderr)}
        if r.returncode == 0:
            break
        if not bad:
            sys.exit("the DDK headers do not compile:\n" + r.stderr[:3000])
        live = [i for j, i in enumerate(live) if j not in bad]
    else:
        sys.exit("cannot evaluate constants:\n" + r.stderr[:3000])
    if not live:
        return [None] * len(exprs)
    od = os.path.join(kit.cfgdir, "llvm-objdump")
    syms = {}
    for line in run([od, "-t", src + ".o"]).stdout.splitlines():
        m = re.search(r"0x([0-9a-f]+) _*__x(\d+)$", line)
        if m:
            syms[int(m.group(2))] = int(m.group(1), 16)
    data = bytearray()
    for line in run([od, "-s", "-j", ".data", src + ".o"]).stdout \
            .splitlines():
        m = re.match(r"\s*([0-9a-f]+) ((?:[0-9a-f]{2,8} ?)+)", line)
        if m and int(m.group(1), 16) == len(data):
            data += bytes.fromhex(m.group(2).replace(" ", ""))
    out = [None] * len(exprs)
    for i in live:
        if i in syms:
            out[i] = int.from_bytes(data[syms[i]:syms[i] + 8], "little",
                                    signed=True)
    return out


def evaluate(kit, includes, macros):
    """Returns {name: (value, unsigned)} for the integer constants."""
    cand = [n for n, b in macros if b and not STRING.match(b)]
    exprs = []
    for n in cand:
        exprs.append("(%s)" % n)
        exprs.append("((%s) - (%s) - 1) > 0" % (n, n))
    v = enum_values(kit, includes, exprs)
    return {n: (v[2 * i], bool(v[2 * i + 1])) for i, n in enumerate(cand)
            if v[2 * i] is not None}


def literal(body, v, unsigned):
    if re.fullmatch(r"\d+[uUlL]*", body) and v >= 0:
        return str(v) + ("u" if unsigned and v > 0x7FFFFFFF else "")
    if v < 0:
        return "(%d)" % v
    s = "0x%0*X" % (8 if v > 0xFFFF else 4 if v > 0xFF else 2, v)
    if v > 0xFFFFFFFF:
        s += "ULL" if unsigned else "LL"
    return s


def layout_values(kit, includes, records):
    """Sizes and offsets of each record, as the DDK lays it out."""
    keys = []
    exprs = []
    for name, fields in records.items():
        keys.append((name, None))
        exprs.append("sizeof(%s)" % name)
        keys.append((name, "__alignof"))
        exprs.append("__alignof(%s)" % name)
        for f in fields:
            keys.append((name, f))
            exprs.append("__builtin_offsetof(%s, %s)" % (name, f))
    v = enum_values(kit, includes, exprs)
    return [(k, x) for k, x in zip(keys, v) if x is not None]


# ---- putting it together ----

def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    global SPECS, BASE, INCLUDE, LAYOUT, GATE, KITS, PRELUDE, PACKED, EXTRA
    args = sys.argv[1:]
    roots = None
    miniport = False
    wdm = False
    while args[:1] in (["--kits"], ["--miniport"], ["--wdm"]):
        if args[0] == "--kits":
            roots, args = args[1], args[2:]
        elif args[0] == "--wdm":
            wdm, args = True, args[1:]
        else:
            miniport, args = True, args[1:]
    if wdm:
        SPECS = WDM_SPECS
        BASE = []
        INCLUDE = os.path.join(HERE, "..", "kernel", "include")
        LAYOUT = os.path.join(HERE, "..", "tests", "ddk", "wdm-layout")
        GATE = "_WIN32_WINNT"
        KITS = [("0x0500", "ms-w2k-ddk"), ("0x0501", "ms-xp-ddk")]
        PRELUDE = []
        PACKED = False
        EXTRA = ["-fpack-struct=8", "-mrtd"]
    if miniport:
        SPECS = MINIPORT_SPECS
        BASE = []
        top = os.path.join(HERE, "..", "miniport")
        INCLUDE = os.path.join(top, "include")
        LAYOUT = os.path.join(HERE, "..", "tests", "ddk", "miniport-layout")
        GATE = "_WIN32_WINDOWS"
        KITS = [("0x0400", "ms-w95-vxd"), ("0x0410", "ms-w98-vxd"),
                ("0x0490", "ms-me-vxd")]
        PRELUDE = []
        PACKED = False
        # Natural alignment, as cl's default, and scsi.h without
        # basedef.h's types (miniport.h has them).
        # -mrtd: stdcall, as the DDK builds them (cl -Gz); DBG, as its
        # samples do, for ScsiDebugPrint.
        EXTRA = ["-fpack-struct=8", "-D_NTDEF_", "-mrtd", "-DDBG=1"]
    cfgdir = args[0]
    want = args[1:] or list(SPECS)
    kits = [(v, Kit(cfgdir, c, roots)) for v, c in KITS]
    owner = {}      # name -> header that declares it
    base = {}       # name -> (gate, text, deps)
    base_order = []
    # mingw-w64's own headers that the WDM ones include first.
    pres = []
    for _, pre, _ in SPECS.values():
        pres += [p for p in pre if p not in pres]
    for n in ours(cfgdir) if PACKED else \
            ours(cfgdir, "nt4-ddk", pres) if wdm else ():
        owner[n] = "vxd"
    last = max(list(SPECS).index(w) for w in want)
    for name in list(SPECS)[:last + 1]:
        files, pre, intro = SPECS[name]
        incl = BASE + pre + files
        files = {f.lower() for f in files}
        per = []
        for gate, kit in kits:
            ast, errs, msg = ast_of(kit, incl)
            m = re.search(r"'([^']*)' file not found", msg)
            if m:
                # A DDK without the header (or what it includes).
                print("%s: %s has no %s" % (name, kit.cfg, m.group(1)),
                      file=sys.stderr)
                continue
            d = Decls(ast, files)
            macros = macros_of(kit, incl, files)
            vals = evaluate(kit, incl, macros)
            lay = layout_values(kit, incl, d.records) \
                if gate == KITS[1][0] else None
            per.append((gate, d, macros, vals, lay, errs, msg))
        if name not in want:
            for gate, d, macros, vals, lay, errs, msg in per:
                for key, text, deps, f in d.items:
                    if os.path.basename(f or "").lower() in files:
                        owner.setdefault(key.split()[-1], name)
                for n, b in macros:
                    owner.setdefault(n, name)
            continue
        write(name, intro, per, files, owner, base, base_order)
    if PACKED:
        write_base(base, base_order)
    elif base:
        sys.exit("declarations from outside the headers: " +
                 ", ".join(base_order))


def ours(cfgdir, cfg="win95-vxd", headers=("vxd.h",)):
    """The names <vxd.h> (or HEADERS, with CFG) defines: macros, typedefs
    and tags."""
    kit = Kit(cfgdir, cfg)
    src = os.path.join(kit.tmp, "o.c")
    with open(src, "w") as f:
        for h in headers:
            f.write("#include <%s>\n" % h)
    inc = ["-I", VXD]
    names = set()
    r = run(kit.cmd(*inc, "-E", "-dM", src))
    names |= set(re.findall(r"^#define ([A-Za-z_]\w*)", r.stdout, re.M))
    r = run(kit.cmd(*inc, "-fsyntax-only", "-Xclang", "-ast-dump=json", src))
    for n in json.loads(r.stdout).get("inner", []):
        if n.get("kind") in ("TypedefDecl", "RecordDecl", "EnumDecl") and \
                n.get("name") and not n.get("isImplicit"):
            names.add(n["name"])
        for c in n.get("inner", []) if n.get("kind") == "EnumDecl" else ():
            names.add(c.get("name"))
    names.discard("VXD_WINVER")
    return names


def gate_of(present, kits_order):
    for g in kits_order:
        if g in present:
            return g
    return None


def write(name, intro, per, files, owner, base, base_order):
    gates = [p[0] for p in per]
    # Declarations: in the order of the latest DDK, then any only older
    # ones have.
    seen = {}
    order = []
    for gate, d, *_ in reversed(per):
        prev = None
        here = set()
        for key, text, deps, f in d.items:
            mine = os.path.basename(f or "").lower() in files
            if key in here:
                continue  # declared again
            here.add(key)
            if key not in seen:
                seen[key] = {"gates": set(), "text": text, "deps": set(),
                             "mine": mine, "f": f, "by": {}}
                # After the declaration before it in this DDK.
                order.insert(order.index(prev) + 1 if prev else 0, key)
            prev = key
            seen[key]["gates"].add(gate)
            seen[key]["by"][gate] = text
            seen[key]["deps"] |= deps
            if mine:
                seen[key]["mine"] = True
    # Foreign declarations the header uses go in vxdtypes.h or come from
    # the header that declares them.
    foreign = {k: v for k, v in seen.items() if not v["mine"]}
    fnames = {k.split()[-1]: k for k in foreign}
    includes = set()
    emitted = set()

    def need(dep_names, stack=()):
        for dn in sorted(dep_names):
            if dn in emitted:
                continue
            if dn in owner and owner[dn] != name:
                if owner[dn] != "vxd":
                    includes.add(owner[dn])
                continue
            if dn in fnames and dn not in stack:
                fk = fnames[dn]
                fv = foreign[fk]
                need(fv["deps"], stack + (dn,))
                if dn not in base:
                    base[dn] = (gate_of(fv["gates"], gates), fv["text"])
                    base_order.append(dn)
                emitted.add(dn)

    lay = ([p[4] for p in per if p[0] == KITS[1][0]] or [None])[0]
    aligns = {r: v for (r, f), v in lay or [] if f == "__alignof"}

    def align(tag):
        for k in ("struct ", "union "):
            if k + tag in aligns:
                return aligns[k + tag]
        return 8

    out_decls = []
    for key in order:
        v = seen[key]
        if not v["mine"]:
            continue
        nm = key.split()[-1]
        if key.endswith(" fwd"):
            # struct NAME; claims nothing (another header may define it).
            out_decls.append(versioned(v["by"], gates, lambda t: t))
            continue
        if nm in base or owner.get(nm, name) != name:
            continue  # vxdtypes.h or another header has it
        need(v["deps"])
        emitted.add(nm)
        owner.setdefault(nm, name)
        out_decls.append(versioned(v["by"], gates, lambda t: t))
        if not PACKED and key.startswith("tag ") and align(key[4:]) < 4:
            # Under #pragma pack in the DDK.
            g, text = out_decls[-1]
            out_decls[-1] = (g, "#pragma pack(push, %d)\n%s\n#pragma pack(pop)"
                             % (align(key[4:]), text))
    # Constants.
    consts = {}
    corder = []
    for gate, d, macros, vals, *_ in reversed(per):
        prev = None
        for n, b in dict(macros).items():
            if n in owner and owner[n] != name:
                continue
            if re.fullmatch(r"_+%s_H_*|_+%s_?INC_*" % (name.upper(),
                                                     name.upper()), n):
                continue
            if STRING.match(b) or CHAR.match(b):
                text = b
            elif n == "NULL" and re.fullmatch(r"\(\s*\(\s*void\s*\*\s*\)\s*0\s*\)|0", b):
                text = "((void *)0)"
            elif n in vals:
                text = literal(b, *vals[n])
            else:
                continue
            if n not in consts:
                consts[n] = {}
                corder.insert(corder.index(prev) + 1 if prev else 0, n)
            consts[n][gate] = text
            prev = n
    for n in corder:
        owner.setdefault(n, name)
    # Header.
    wdm = SPECS is WDM_SPECS
    guard = "%s_%s_H" % ("VXD" if PACKED else "WDM" if wdm else "MINIPORT9X",
                         name.upper())
    h = ["/* %s.h: %s\n\n   Made by tools/gen-vxd-headers.py from the names, "
         "values and layouts\n   in the %s DDKs; each "
         "declaration is under the first\n   %s whose DDK has it. "
         "*/\n" % (name, intro, "2000 and XP" if wdm else "95, 98 and ME",
                    GATE),
         "#ifndef %s\n#define %s\n\n" % (guard, guard)]
    for i in PRELUDE:
        h.append("#include <%s>\n" % i)
    incs = {i.replace(".h", "") for i in includes}
    first = []
    if not PACKED:
        # In the order the DDK's header needs them.
        first = [i.replace(".h", "") for i in SPECS[name][1]]
    for i in first + sorted(incs - set(first)):
        h.append("#include <%s.h>\n" % i)
    if PACKED:
        h.append("\n#pragma pack(push, 1)\n")
    body = []
    for n in corder:
        body.append(versioned(consts[n], gates,
                              lambda t, n=n: ("#ifndef NULL\n#define NULL %s\n#endif"
                                              % t) if n == "NULL" else
                              "#define %s %s" % (n, t)))
    body += out_decls
    h.append(gated(body, gates[0]))
    h.append("\n#pragma pack(pop)\n" if PACKED else "")
    h.append("\n#endif\n")
    os.makedirs(INCLUDE, exist_ok=True)
    with open(os.path.join(INCLUDE, name + ".h"), "w") as f:
        f.write("".join(h))
    # Layout test, from the 98 DDK.
    t = ["/* Made by tools/gen-vxd-headers.py: the layouts of <%s.h> as the "
         "%s DDK's\n   compiler laid them out. */\n"
         % (name, "XP" if wdm else "98"),
         "#define %s %s\n#include <%s.h>\n#include <stddef.h>\n"
         "#ifdef __cplusplus\n#define A(e) static_assert(e, #e);\n#else\n"
         "#define A(e) _Static_assert(e, #e);\n#endif\n"
         % (GATE, KITS[1][0], name)]
    for (rec, field), v in lay or []:
        if field is None:
            t.append("A(sizeof(%s) == %d)\n" % (rec, v))
        elif field == "__alignof":
            # Under -Zp1 every VxD record's is 1, and vxd.h's own (DIOCParams,
            # VPICD_IRQ_Descriptor) are natural, as their offsets allow.
            if not PACKED:
                t.append("A(__alignof__(%s) == %d)\n" % (rec, v))
        else:
            t.append("A(offsetof(%s, %s) == %d)\n" % (rec, field, v))
    os.makedirs(LAYOUT, exist_ok=True)
    with open(os.path.join(LAYOUT, name + ".c"), "w") as f:
        f.write("".join(t))
    print("%s.h: %d constants, %d declarations, %d layout checks; "
          "errors per DDK %s" % (name, len(corder), len(out_decls),
                                 len(lay or []),
                                 [p[5] for p in per]), file=sys.stderr)


def versioned(by_gate, gates, fmt):
    """(gate, text) for a declaration that has by_gate[g] in each DDK g
    that has it: one text under the first gate, or an #if chain."""
    present = [g for g in gates if g in by_gate]
    texts = []
    for g in present:
        if not texts or texts[-1][1] != by_gate[g]:
            texts.append((g, by_gate[g]))
    if len(texts) == 1:
        return present[0], fmt(texts[0][1])
    out = []
    for i, (g, t) in enumerate(reversed(texts)):
        last = i == len(texts) - 1
        if i == 0:
            out.append("#if %s >= %s" % (GATE, g))
        elif not last or g != gates[0]:
            out.append("#elif %s >= %s" % (GATE, g))
        else:
            out.append("#else")
        out.append(fmt(t))
    out.append("#endif")
    return present[0], "\n".join(out)


def gated(body, baseline=KITS[0][0]):
    """Puts each (gate, text) under #if VXD_WINVER >= gate, except those
    of the oldest DDK that has the header, which every version gets."""
    out = []
    cur = None
    for g, text in body:
        g = None if g == baseline else g
        if g != cur:
            if cur:
                out.append("#endif\n")
            if g:
                out.append("#if %s >= %s\n" % (GATE, g))
            cur = g
        out.append(text + "\n")
    if cur:
        out.append("#endif\n")
    return "".join(out)


def write_base(base, order):
    h = ["/* vxdtypes.h: the base types of the DDKs (basedef.h, vmm.h ...) "
         "that the\n   VxD subsystem headers use. Made by "
         "tools/gen-vxd-headers.py. */\n",
         "#ifndef VXD_VXDTYPES_H\n#define VXD_VXDTYPES_H\n\n#include "
         "<vxd.h>\n\n#pragma pack(push, 1)\n"]
    h.append(gated([base[n] for n in order]))
    h.append("\n#pragma pack(pop)\n\n#endif\n")
    with open(os.path.join(INCLUDE, "vxdtypes.h"), "w") as f:
        f.write("".join(h))


if __name__ == "__main__":
    main()
