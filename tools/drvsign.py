#!/usr/bin/env python3
"""Sign drivers for test-signing: make a test certificate, sign .sys files
(Authenticode, embedded in the file) and make signed catalogs (.cat) for
driver packages, with nothing but Python (no signtool, makecert, makecat
or OpenSSL).

    drvsign.py newcert [--subject NAME] [--years N] [--hash sha256|sha1]
                       [--bits N] CERT.pem
    drvsign.py sign --cert CERT.pem [--hash sha256|sha1] FILE.sys ...
    drvsign.py catalog --cert CERT.pem [--hash sha1|sha256] [--os LIST]
                       -o PACKAGE.cat FILE ...
    drvsign.py verify [--cert CERT.pem] FILE ...

newcert writes CERT.pem (the RSA key and a self-signed code-signing
certificate) and CERT.cer (the certificate alone, for Windows'
certificate stores). Keep CERT.pem private: anyone with it can sign as
you.

sign hashes each PE file as Authenticode does (all of it but its
checksum, its certificate table entry and the table) and appends a
PKCS #7 signature with the certificate, replacing one it had, then fixes
the checksum.

catalog makes a catalog listing each FILE's hash (its Authenticode hash
for PE files, of all its bytes for the rest, such as the INF) and signs
it; an INF's CatalogFile names it. --os sets the catalog's OS attribute
(Inf2Cat's, such as XPX86,7X64,10X64), which Windows does not need for
test-signing.

verify checks the hashes and signatures (with --cert, that the
certificate signed them).

Windows loads drivers signed this way only in test mode
(`bcdedit /set testsigning on`, and Secure Boot off): 64-bit Vista and
later check a driver's signature, and in test mode accept any certificate.
To install a package without the "unsigned driver" warning, add CERT.cer
to the Trusted Root and Trusted Publishers stores
(`certutil -addstore root CERT.cer`, `certutil -addstore trustedpublisher
CERT.cer`). Windows 7 and earlier without updates check SHA-1 signatures
only: give --hash sha1 to newcert, sign and catalog for them.
"""
import argparse
import datetime
import hashlib
import os
import re
import secrets
import struct
import sys

# ---------------------------------------------------------------- DER

def der_len(n):
    if n < 0x80:
        return bytes([n])
    b = n.to_bytes((n.bit_length() + 7) // 8, "big")
    return bytes([0x80 | len(b)]) + b


def tlv(tag, content):
    return bytes([tag]) + der_len(len(content)) + content


def seq(*items):
    return tlv(0x30, b"".join(items))


def set_of(*items):
    return tlv(0x31, b"".join(sorted(items)))


def integer(v):
    return tlv(0x02, v.to_bytes(v.bit_length() // 8 + 1, "big", signed=True))


def oid(dotted):
    parts = [int(x) for x in dotted.split(".")]
    out = bytearray([40 * parts[0] + parts[1]])
    for p in parts[2:]:
        b = [p & 0x7F]
        p >>= 7
        while p:
            b.append(0x80 | (p & 0x7F))
            p >>= 7
        out += bytes(reversed(b))
    return tlv(0x06, bytes(out))


NULL = b"\x05\x00"


def octets(b):
    return tlv(0x04, b)


def bits(b, unused=0):
    return tlv(0x03, bytes([unused]) + b)


def bmp(s):
    return tlv(0x1E, s.encode("utf-16-be"))


def explicit(n, content):
    return tlv(0xA0 | n, content)


def der_time(t):
    if t.year < 2050:
        return tlv(0x17, t.strftime("%y%m%d%H%M%SZ").encode())
    return tlv(0x18, t.strftime("%Y%m%d%H%M%SZ").encode())


def parse(data, pos=0):
    """(tag, content, end) of the DER item at pos."""
    tag = data[pos]
    n = data[pos + 1]
    pos += 2
    if n & 0x80:
        k = n & 0x7F
        n = int.from_bytes(data[pos:pos + k], "big")
        pos += k
    return tag, data[pos:pos + n], pos + n


def items(content):
    """The DER items in a constructed item's content, as (tag, content,
    whole encoding)."""
    out, pos = [], 0
    while pos < len(content):
        tag, c, end = parse(content, pos)
        out.append((tag, c, content[pos:end]))
        pos = end
    return out


def to_int(content):
    return int.from_bytes(content, "big", signed=True)

# ---------------------------------------------------------------- OIDs

RSA = "1.2.840.113549.1.1.1"
SIG_ALG = {"sha1": "1.2.840.113549.1.1.5", "sha256": "1.2.840.113549.1.1.11"}
HASH_ALG = {"sha1": "1.3.14.3.2.26", "sha256": "2.16.840.1.101.3.4.2.1"}
HASH_BY_OID = {v: k for k, v in HASH_ALG.items()}
SIGNED_DATA = "1.2.840.113549.1.7.2"
CONTENT_TYPE = "1.2.840.113549.1.9.3"
MESSAGE_DIGEST = "1.2.840.113549.1.9.4"
SIGNING_TIME = "1.2.840.113549.1.9.5"
SPC_INDIRECT_DATA = "1.3.6.1.4.1.311.2.1.4"
SPC_STATEMENT_TYPE = "1.3.6.1.4.1.311.2.1.11"
SPC_SP_OPUS_INFO = "1.3.6.1.4.1.311.2.1.12"
SPC_PE_IMAGE_DATA = "1.3.6.1.4.1.311.2.1.15"
SPC_INDIVIDUAL = "1.3.6.1.4.1.311.2.1.21"
SPC_CAB_DATA = "1.3.6.1.4.1.311.2.1.25"
CTL = "1.3.6.1.4.1.311.10.1"
CATALOG_LIST = "1.3.6.1.4.1.311.12.1.1"
CATALOG_MEMBER = "1.3.6.1.4.1.311.12.1.2"
CATALOG_MEMBER2 = "1.3.6.1.4.1.311.12.1.3"
CAT_NAMEVALUE = "1.3.6.1.4.1.311.12.2.1"
CAT_MEMBERINFO = "1.3.6.1.4.1.311.12.2.2"
CODE_SIGNING = "1.3.6.1.5.5.7.3.3"
# The subject interface packages of PE files and of flat files.
SIP_PE = "{C689AAB8-8E78-11D0-8C47-00C04FC295EE}"
SIP_FLAT = "{DE351A42-8E59-11D0-8C47-00C04FC295EE}"


def alg(dotted):
    return seq(oid(dotted), NULL)

# ---------------------------------------------------------------- RSA

def is_prime(n):
    if n < 2:
        return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % p == 0:
            return n == p
    d, r = n - 1, 0
    while d % 2 == 0:
        d //= 2
        r += 1
    for _ in range(40):
        x = pow(secrets.randbelow(n - 3) + 2, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(r - 1):
            x = pow(x, 2, n)
            if x == n - 1:
                break
        else:
            return False
    return True


def prime(nbits, e):
    while True:
        p = secrets.randbits(nbits) | (3 << (nbits - 2)) | 1
        if p % e != 1 and is_prime(p):
            return p


def new_key(nbits):
    e = 65537
    while True:
        p, q = prime(nbits // 2, e), prime(nbits // 2, e)
        if p == q:
            continue
        n = p * q
        if n.bit_length() != nbits:
            continue
        d = pow(e, -1, (p - 1) * (q - 1))
        return {"n": n, "e": e, "d": d, "p": p, "q": q}


def key_der(k):
    p, q, d = k["p"], k["q"], k["d"]
    return seq(integer(0), integer(k["n"]), integer(k["e"]), integer(d),
               integer(p), integer(q), integer(d % (p - 1)),
               integer(d % (q - 1)), integer(pow(q, -1, p)))


def read_key(der):
    _, c, _ = parse(der)
    f = items(c)
    if f[1][0] == 0x30:     # PKCS #8: the PKCS #1 key in an OCTET STRING
        _, c, _ = parse(f[2][1])
        f = items(c)
    n, e, d = (to_int(f[i][1]) for i in (1, 2, 3))
    return {"n": n, "e": e, "d": d}


def digest_info(h, digest):
    return seq(alg(HASH_ALG[h]), octets(digest))


def rsa_sign(key, h, data):
    k = (key["n"].bit_length() + 7) // 8
    t = digest_info(h, hashlib.new(h, data).digest())
    em = b"\x00\x01" + b"\xff" * (k - len(t) - 3) + b"\x00" + t
    return pow(int.from_bytes(em, "big"), key["d"],
               key["n"]).to_bytes(k, "big")


def rsa_verify(n, e, h, data, sig):
    k = (n.bit_length() + 7) // 8
    em = pow(int.from_bytes(sig, "big"), e, n).to_bytes(k, "big")
    t = digest_info(h, hashlib.new(h, data).digest())
    return em == b"\x00\x01" + b"\xff" * (k - len(t) - 3) + b"\x00" + t

# ---------------------------------------------------------------- certificates

def name(cn):
    v = (tlv(0x13, cn.encode()) if re.fullmatch(r"[A-Za-z0-9 '()+,./:=?-]*",
                                                 cn)
         else tlv(0x0C, cn.encode("utf-8")))
    return seq(set_of(seq(oid("2.5.4.3"), v)))


def new_cert(key, cn, years, h):
    now = datetime.datetime.now(datetime.timezone.utc).replace(microsecond=0)
    nb = now - datetime.timedelta(days=1)
    na = nb.replace(year=nb.year + years)
    spki = seq(alg(RSA), bits(seq(integer(key["n"]), integer(key["e"]))))
    keyid = hashlib.sha1(seq(integer(key["n"]), integer(key["e"]))).digest()
    exts = seq(
        seq(oid("2.5.29.19"), octets(seq())),               # not a CA
        seq(oid("2.5.29.15"), tlv(0x01, b"\xff"),           # critical:
            octets(bits(b"\x80", 7))),                      # digitalSignature
        seq(oid("2.5.29.37"), octets(seq(oid(CODE_SIGNING)))),
        seq(oid("2.5.29.14"), octets(octets(keyid))))
    tbs = seq(explicit(0, integer(2)),
              integer(secrets.randbits(63) | 1 << 62),
              alg(SIG_ALG[h]), name(cn), seq(der_time(nb), der_time(na)),
              name(cn), spki, explicit(3, exts))
    return seq(tbs, alg(SIG_ALG[h]), bits(rsa_sign(key, h, tbs)))


def cert_info(der):
    """The fields of a certificate signing needs."""
    _, c, _ = parse(der)
    tbs = items(c)[0]
    f = items(tbs[1])
    if f[0][0] == 0xA0:
        f = f[1:]
    serial, issuer, subject, spki = f[0], f[2], f[4], f[5]
    _, bs, _ = parse(items(spki[1])[1][2])
    _, pk, _ = parse(bs[1:])
    n, e = (to_int(x[1]) for x in items(pk))
    cn = ""
    for _, rdn, _ in items(subject[1]):
        for _, atv, _ in items(rdn):
            a = items(atv)
            if a[0][2] == oid("2.5.4.3"):
                cn = a[1][1].decode("utf-8", "replace")
    return {"der": der, "serial": serial[2], "issuer": issuer[2], "n": n,
            "e": e, "cn": cn}


def pem_blocks(text):
    import base64
    return [(m.group(1), base64.b64decode(m.group(2)))
            for m in re.finditer(r"-----BEGIN ([A-Z0-9 ]+)-----(.*?)-----END",
                                 text, re.S)]


def pem(label, der):
    import base64
    b = base64.b64encode(der).decode()
    return "-----BEGIN %s-----\n%s\n-----END %s-----\n" % (
        label, "\n".join(b[i:i + 64] for i in range(0, len(b), 64)), label)


def load_signer(path):
    key = cert = None
    for label, der in pem_blocks(open(path).read()):
        if label in ("RSA PRIVATE KEY", "PRIVATE KEY"):
            key = read_key(der)
        elif label == "CERTIFICATE":
            cert = cert_info(der)
    if not key or not cert:
        sys.exit("%s: needs an RSA private key and a certificate" % path)
    if (key["n"], key["e"]) != (cert["n"], cert["e"]):
        sys.exit("%s: the key is not the certificate's" % path)
    return key, cert

# ---------------------------------------------------------------- PKCS #7

def signed_data(key, cert, h, ctype, content):
    """PKCS #7 SignedData of content (DER) of type ctype. As in PKCS #7
    1.5, which Authenticode follows, the message digest is of the
    content's value, without its tag and length."""
    _, value, _ = parse(content)
    attrs = [seq(oid(CONTENT_TYPE), set_of(oid(ctype))),
             seq(oid(MESSAGE_DIGEST),
                 set_of(octets(hashlib.new(h, value).digest())))]
    if ctype == SPC_INDIRECT_DATA:
        attrs += [seq(oid(SPC_SP_OPUS_INFO), set_of(seq())),
                  seq(oid(SPC_STATEMENT_TYPE),
                      set_of(seq(oid(SPC_INDIVIDUAL))))]
    else:
        now = datetime.datetime.now(datetime.timezone.utc)
        attrs.append(seq(oid(SIGNING_TIME), set_of(der_time(now))))
    auth = set_of(*attrs)
    signer = seq(integer(1), seq(cert["issuer"], cert["serial"]),
                 alg(HASH_ALG[h]), b"\xa0" + auth[1:], alg(RSA),
                 octets(rsa_sign(key, h, auth)))
    sd = seq(integer(1), set_of(alg(HASH_ALG[h])),
             seq(oid(ctype), explicit(0, content)),
             tlv(0xA0, cert["der"]), set_of(signer))
    return seq(oid(SIGNED_DATA), explicit(0, sd))


def check_signed_data(blob, certs):
    """(content type, content DER, problems) of a PKCS #7 SignedData."""
    problems = []
    _, ci, _ = parse(blob)
    _, sd, _ = parse(items(ci)[1][1])
    f = items(sd)
    inner = items(f[2][1])
    ctype = inner[0][2]
    content = items(inner[1][1])[0][2]
    _, value, _ = parse(content)
    certs = list(certs)
    rest = f[3:]
    if rest and rest[0][0] == 0xA0:
        certs += [cert_info(c[2]) for c in items(rest[0][1])]
        rest = rest[1:]
    if rest and rest[0][0] == 0xA1:
        rest = rest[1:]
    signers = items(rest[0][1])
    if not signers:
        problems.append("no signer")
    for _, si, _ in signers:
        s = items(si)
        ias = items(s[1][1])
        h = HASH_BY_OID.get(dotted(items(s[2][1])[0][1]))
        if not h:
            problems.append("unknown hash algorithm")
            continue
        sig = s[-1][1]
        auth = [x for x in s if x[0] == 0xA0]
        signed = b"\x31" + auth[0][2][1:] if auth else value
        if auth:
            md = [a for a in items(auth[0][1])
                  if items(a[1])[0][2] == oid(MESSAGE_DIGEST)]
            got = items(items(md[0][1])[1][1])[0][1] if md else None
            if got != hashlib.new(h, value).digest():
                problems.append("the content's digest does not match")
        who = [c for c in certs
               if (c["issuer"], c["serial"]) == (ias[0][2], ias[1][2])]
        if not who:
            problems.append("the signer's certificate is not there")
        elif not rsa_verify(who[0]["n"], who[0]["e"], h, signed, sig):
            problems.append("the signature does not verify")
        else:
            problems.append(None)
            problems.append("signer: %s" % who[0]["cn"])
    return ctype, content, h, problems


def dotted(content):
    out, v = [], 0
    for i, b in enumerate(content):
        v = (v << 7) | (b & 0x7F)
        if not b & 0x80:
            out.append(v)
            v = 0
    first = out.pop(0)
    return ".".join(str(x) for x in [min(first // 40, 2),
                                     first - 40 * min(first // 40, 2)] + out)

# ---------------------------------------------------------------- PE files

def pe_layout(data):
    """(checksum offset, security directory offset) of a PE file, or
    None."""
    if data[:2] != b"MZ" or len(data) < 0x40:
        return None
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        return None
    opt = pe + 24
    magic = struct.unpack_from("<H", data, opt)[0]
    dirs = opt + {0x10B: 96, 0x20B: 112}[magic]
    ndirs = struct.unpack_from("<I", data, dirs - 4)[0]
    if ndirs < 5:
        return None
    return opt + 64, dirs + 4 * 8


def pe_hash(data, h, layout):
    cks, sec = layout
    m = hashlib.new(h)
    m.update(data[:cks])
    m.update(data[cks + 4:sec])
    off, size = struct.unpack_from("<II", data, sec)
    end = off if off and size else len(data)
    m.update(data[sec + 8:end])
    return m.digest()


def pe_checksum(data, cks):
    s = 0
    padded = data + b"\0" * (len(data) & 1)
    for i, (w,) in enumerate(struct.iter_unpack("<H", padded)):
        if i * 2 in (cks, cks + 2):
            continue
        s += w
        s = (s & 0xFFFF) + (s >> 16)
    s = (s & 0xFFFF) + (s >> 16)
    return (s + len(data)) & 0xFFFFFFFF


def spc_indirect(h, digest, pe=True):
    if pe:
        # SpcPeImageData: no flags, file [0] { file [2] { unicode [0]
        # "<<<Obsolete>>>" } }, as signtool writes it.
        data = seq(oid(SPC_PE_IMAGE_DATA),
                   seq(bits(b""), explicit(0, explicit(2, tlv(
                       0x80, "<<<Obsolete>>>".encode("utf-16-be"))))))
    else:
        # A flat file (an INF): SpcLink file [2] { "<<<Obsolete>>>" }, as
        # makecat writes it.
        data = seq(oid(SPC_CAB_DATA), explicit(2, tlv(
            0x80, "<<<Obsolete>>>".encode("utf-16-be"))))
    return seq(data, digest_info(h, digest))


def sign_pe(path, key, cert, h):
    data = bytearray(open(path, "rb").read())
    layout = pe_layout(data)
    if not layout:
        sys.exit("%s: not a PE file" % path)
    cks, sec = layout
    off, size = struct.unpack_from("<II", data, sec)
    if off and size:
        if off + size != len(data):
            sys.exit("%s: its signature is not at its end" % path)
        del data[off:]
        struct.pack_into("<II", data, sec, 0, 0)
    data += b"\0" * (-len(data) % 8)
    content = spc_indirect(h, pe_hash(bytes(data), h, layout))
    blob = signed_data(key, cert, h, SPC_INDIRECT_DATA, content)
    blob += b"\0" * (-len(blob) % 8)
    win_cert = struct.pack("<IHH", 8 + len(blob), 0x200, 2) + blob
    struct.pack_into("<II", data, sec, len(data), len(win_cert))
    data += win_cert
    struct.pack_into("<I", data, cks, pe_checksum(bytes(data), cks))
    with open(path, "wb") as f:
        f.write(data)

# ---------------------------------------------------------------- catalogs

def member(path, h):
    data = open(path, "rb").read()
    layout = pe_layout(data)
    digest = (pe_hash(data, h, layout) if layout
              else hashlib.new(h, data).digest())
    tag = digest.hex().upper()
    fname = os.path.basename(path)
    attrs = [
        seq(oid(CAT_NAMEVALUE), set_of(seq(
            bmp("File"), integer(0x10010001),
            octets((fname + "\0").encode("utf-16-le"))))),
        seq(oid(CAT_MEMBERINFO), set_of(seq(
            bmp(SIP_PE if layout else SIP_FLAT), integer(512)))),
        seq(oid(SPC_INDIRECT_DATA), set_of(spc_indirect(h, digest,
                                                        bool(layout))))]
    return seq(octets((tag + "\0").encode("utf-16-le")), set_of(*attrs))


def make_catalog(out, files, key, cert, h, os_list):
    now = datetime.datetime.now(datetime.timezone.utc)
    exts = []
    if os_list:
        exts.append(seq(oid(CAT_NAMEVALUE), octets(seq(
            bmp("OS"), integer(0x10010001),
            octets((os_list + "\0").encode("utf-16-le"))))))
    ctl = seq(seq(oid(CATALOG_LIST)), octets(secrets.token_bytes(16)),
              der_time(now),
              alg(CATALOG_MEMBER if h == "sha1" else CATALOG_MEMBER2),
              seq(*sorted(member(f, h) for f in files)),
              *([explicit(0, seq(*exts))] if exts else []))
    with open(out, "wb") as f:
        f.write(signed_data(key, cert, h, CTL, ctl))


def check_catalog(path, certs):
    blob = open(path, "rb").read()
    ctype, content, h, problems = check_signed_data(blob, certs)
    if ctype != oid(CTL):
        return ["not a catalog"]
    f = items(parse(content)[1])
    if f[0][0] == 0x02:
        f = f[1:]
    subjects = items(f[4][1]) if len(f) > 4 else []
    here = os.path.dirname(path)
    for _, ts, _ in subjects:
        t = items(ts)
        tag = t[0][1].decode("utf-16-le").rstrip("\0")
        fname = None
        for _, a, _ in items(t[1][1]):
            aa = items(a)
            if aa[0][2] == oid(CAT_NAMEVALUE):
                nv = items(items(aa[1][1])[0][1])
                if nv[0][1].decode("utf-16-be") == "File":
                    fname = nv[2][1].decode("utf-16-le").rstrip("\0")
        if not fname or not os.path.exists(os.path.join(here, fname)):
            problems.append("%s: %s is not next to the catalog" % (tag,
                                                                    fname))
            continue
        data = open(os.path.join(here, fname), "rb").read()
        layout = pe_layout(data)
        hh = "sha1" if len(tag) == 40 else "sha256"
        d = pe_hash(data, hh, layout) if layout else hashlib.new(
            hh, data).digest()
        problems.append(None if d.hex().upper() == tag else
                        "%s: its hash does not match" % fname)
        problems.append("member: %s" % fname)
    return problems


def check_pe(path, certs):
    data = open(path, "rb").read()
    layout = pe_layout(data)
    if not layout:
        return ["not a PE file"]
    off, size = struct.unpack_from("<II", data, layout[1])
    if not (off and size):
        return ["not signed"]
    length, rev, typ = struct.unpack_from("<IHH", data, off)
    if typ != 2:
        return ["not a PKCS #7 signature"]
    ctype, content, h, problems = check_signed_data(
        data[off + 8:off + length], certs)
    if ctype != oid(SPC_INDIRECT_DATA):
        return ["not an Authenticode signature"]
    _, digest, _ = parse(items(items(parse(content)[1])[1][1])[1][2])
    if digest != pe_hash(data, h, layout):
        problems.append("the file's hash does not match")
    if struct.unpack_from("<I", data, layout[0])[0] != pe_checksum(
            data, layout[0]):
        problems.append("the checksum is wrong")
    return problems

# ---------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(
        description="Test-sign drivers and make catalogs.")
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("newcert", help="make a test certificate")
    p.add_argument("--subject", default="clang-dos test driver signing")
    p.add_argument("--years", type=int, default=10)
    p.add_argument("--hash", choices=("sha256", "sha1"), default="sha256")
    p.add_argument("--bits", type=int, default=2048)
    p.add_argument("out")
    p = sub.add_parser("sign", help="sign PE files")
    p.add_argument("--cert", required=True)
    p.add_argument("--hash", choices=("sha256", "sha1"), default="sha256")
    p.add_argument("files", nargs="+")
    p = sub.add_parser("catalog", help="make a signed catalog")
    p.add_argument("--cert", required=True)
    p.add_argument("--hash", choices=("sha256", "sha1"), default="sha1")
    p.add_argument("--os", default="")
    p.add_argument("-o", "--output", required=True)
    p.add_argument("files", nargs="+")
    p = sub.add_parser("verify", help="check signatures and catalogs")
    p.add_argument("--cert")
    p.add_argument("files", nargs="+")
    a = ap.parse_args()

    if a.cmd == "newcert":
        key = new_key(a.bits)
        cert = new_cert(key, a.subject, a.years, a.hash)
        old = os.umask(0o077)
        try:
            with open(a.out, "w") as f:
                f.write(pem("RSA PRIVATE KEY", key_der(key)))
                f.write(pem("CERTIFICATE", cert))
        finally:
            os.umask(old)
        cer = os.path.splitext(a.out)[0] + ".cer"
        with open(cer, "wb") as f:
            f.write(cert)
        print("%s: key and certificate; %s: certificate" % (a.out, cer))
        return 0
    if a.cmd in ("sign", "catalog"):
        key, cert = load_signer(a.cert)
        if a.cmd == "sign":
            for f in a.files:
                sign_pe(f, key, cert, a.hash)
                print("%s: signed (%s)" % (f, a.hash))
        else:
            make_catalog(a.output, a.files, key, cert, a.hash, a.os)
            print("%s: %d files (%s)" % (a.output, len(a.files), a.hash))
        return 0
    certs = []
    trusted = None
    if a.cert:
        raw = open(a.cert, "rb").read()
        for label, der in pem_blocks(raw.decode("latin-1")):
            if label == "CERTIFICATE":
                trusted = cert_info(der)
        if not trusted:     # a .cer: the certificate in DER
            trusted = cert_info(raw)
        certs.append(trusted)
    bad = 0
    for f in a.files:
        is_cat = open(f, "rb").read(2) != b"MZ"
        res = check_catalog(f, certs) if is_cat else check_pe(f, certs)
        notes = [r for r in res if r and r.startswith(("signer:",
                                                       "member:"))]
        errs = [r for r in res if r and r not in notes]
        if trusted and not any(n == "signer: %s" % trusted["cn"]
                               for n in notes):
            errs.append("not signed by %s" % trusted["cn"])
        if not any(r is None for r in res) and not errs:
            errs.append("nothing verified")
        bad += bool(errs)
        print("%s: %s%s" % (f, "; ".join(errs) if errs else "ok",
                            " (%s)" % ", ".join(notes) if notes else ""))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
