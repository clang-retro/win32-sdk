// tlbimport.cpp - the type library half of Visual C++'s #import: reads a
// type library (a .tlb file, or the TYPELIB resource of a .dll, .ocx,
// .exe or .olb) and writes the primary header (.tlh) and the wrapper
// implementations (.tli) that #import includes. clang runs it for an
// #import directive; it can also be run by hand.
//
//   tlbimport [options] [attributes] file[\N]
//
//   -o DIR         where to write NAME.tlh and NAME.tli (default: .)
//   -L DIR         where to look for the type libraries this one imports
//   -n NAME        the base name of the output files (default: the input's)
//   --print-tlh    print the .tlh's path on standard output when done
//
// The attributes are #import's, one argument each, as they appear in the
// directive: no_namespace, rename_namespace("N"), named_guids,
// raw_interfaces_only, raw_native_types, raw_dispinterfaces,
// no_smart_pointers, no_implementation, implementation_only,
// rename("Old","New"), exclude("A","B"), high_method_prefix("P"),
// high_property_prefixes("Get","Put","PutRef"), raw_method_prefix("P"),
// raw_property_prefixes("get_","put_","putref_"), inject_statement("S"),
// no_auto_exclude, tlbid(N). Others (auto_rename, auto_search,
// embedded_idl, include, no_dual_interfaces, no_search_namespace,
// rename_search_namespace) are accepted.
//
// Type libraries in the MSFT format (what MIDL and ICreateTypeLib2 write)
// are read; the older SLTG format is not.
//
// Part of clang-retro win32-sdk.
// SPDX-License-Identifier: AGPL-3.0-only
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------
// OLE constants (the values are those of the Windows headers)

enum {
  VT_EMPTY = 0, VT_NULL = 1, VT_I2 = 2, VT_I4 = 3, VT_R4 = 4, VT_R8 = 5, VT_CY = 6, VT_DATE = 7,
  VT_BSTR = 8, VT_DISPATCH = 9, VT_ERROR = 10, VT_BOOL = 11, VT_VARIANT = 12, VT_UNKNOWN = 13,
  VT_DECIMAL = 14, VT_I1 = 16, VT_UI1 = 17, VT_UI2 = 18, VT_UI4 = 19, VT_I8 = 20, VT_UI8 = 21,
  VT_INT = 22, VT_UINT = 23, VT_VOID = 24, VT_HRESULT = 25, VT_PTR = 26, VT_SAFEARRAY = 27,
  VT_CARRAY = 28, VT_USERDEFINED = 29, VT_LPSTR = 30, VT_LPWSTR = 31, VT_RECORD = 36,
  VT_INT_PTR = 37, VT_UINT_PTR = 38, VT_ARRAY = 0x2000, VT_BYREF = 0x4000
};
enum {
  TKIND_ENUM, TKIND_RECORD, TKIND_MODULE, TKIND_INTERFACE, TKIND_DISPATCH, TKIND_COCLASS,
  TKIND_ALIAS, TKIND_UNION
};
enum { FUNC_VIRTUAL, FUNC_PUREVIRTUAL, FUNC_NONVIRTUAL, FUNC_STATIC, FUNC_DISPATCH };
enum { INVOKE_FUNC = 1, INVOKE_PROPERTYGET = 2, INVOKE_PROPERTYPUT = 4, INVOKE_PROPERTYPUTREF = 8 };
enum { VAR_PERINSTANCE, VAR_STATIC, VAR_CONST, VAR_DISPATCH };
enum {
  PARAMFLAG_FIN = 1, PARAMFLAG_FOUT = 2, PARAMFLAG_FLCID = 4, PARAMFLAG_FRETVAL = 8,
  PARAMFLAG_FOPT = 0x10, PARAMFLAG_FHASDEFAULT = 0x20
};
enum { TYPEFLAG_FDUAL = 0x40, TYPEFLAG_FHIDDEN = 0x10, TYPEFLAG_FRESTRICTED = 0x200 };
enum { IMPLTYPEFLAG_FDEFAULT = 1, IMPLTYPEFLAG_FSOURCE = 2, IMPLTYPEFLAG_FRESTRICTED = 4 };
enum { VARFLAG_FREADONLY = 1 };
enum { FUNCFLAG_FRESTRICTED = 1 };
enum { CC_CDECL = 1, CC_STDCALL = 4 };

[[noreturn]] void Fail(const char* fmt, const std::string& a = "", const std::string& b = "") {
  fprintf(stderr, "tlbimport: ");
  fprintf(stderr, fmt, a.c_str(), b.c_str());
  fprintf(stderr, "\n");
  exit(1);
}

struct Guid {
  uint8_t b[16] = {};
  bool operator<(const Guid& o) const { return memcmp(b, o.b, 16) < 0; }
  bool operator==(const Guid& o) const { return memcmp(b, o.b, 16) == 0; }
  uint32_t Data1() const { return b[0] | b[1] << 8 | b[2] << 16 | (uint32_t)b[3] << 24; }
  uint16_t Data2() const { return (uint16_t)(b[4] | b[5] << 8); }
  uint16_t Data3() const { return (uint16_t)(b[6] | b[7] << 8); }
  std::string Text() const {
    char s[40];
    snprintf(s, sizeof s, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", Data1(), Data2(),
             Data3(), b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    return s;
  }
  // The initializer of a GUID: {0x..., 0x..., 0x..., {0x.., ...}}
  std::string Init() const {
    char s[120];
    snprintf(s, sizeof s,
             "0x%08x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
             Data1(), Data2(), Data3(), b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    return s;
  }
};

// ---------------------------------------------------------------------
// The model of a type library

struct Lib;

struct TypeDesc {
  int vt = VT_EMPTY;
  std::shared_ptr<TypeDesc> inner;  // VT_PTR, VT_SAFEARRAY, VT_CARRAY
  std::vector<uint32_t> dims;       // VT_CARRAY
  int href = -1;                    // VT_USERDEFINED
};
using TD = std::shared_ptr<TypeDesc>;

struct Value {
  bool valid = false;
  int vt = VT_EMPTY;
  int64_t i = 0;
  double d = 0;
  std::string s;
};

struct Param {
  std::string name;
  TD type;
  int flags = 0;
  Value def;
};

struct Func {
  std::string name, doc;
  int32_t memid = 0;
  int funckind = 0, invkind = INVOKE_FUNC, callconv = CC_STDCALL, flags = 0;
  int vtoff = 0;
  TD ret;
  std::vector<Param> params;
  int cParamsOpt = 0;
};

struct Var {
  std::string name, doc;
  int32_t memid = 0;
  TD type;
  int varkind = 0, flags = 0;
  Value value;
};

struct Impl {
  int href = -1;
  int flags = 0;
};

struct TypeInfo {
  Lib* lib = nullptr;
  int index = 0;
  int kind = 0;
  std::string name, doc, dll;
  Guid guid;
  bool hasGuid = false;
  int flags = 0, align = 4, size = 0;
  std::vector<Func> funcs;
  std::vector<Var> vars;
  std::vector<Impl> impls;
  TD alias;
};

// A type another library holds.
struct ImportRef {
  std::string file;
  Guid libGuid;
  bool byGuid = false;
  Guid typeGuid;
  int typeIndex = 0;
  int kind = 0;
};

struct Lib {
  std::string path, name, doc;
  Guid guid;
  int major = 0, minor = 0, syskind = 1;
  std::vector<TypeInfo> types;
  std::map<int, ImportRef> imports;  // by import info offset
};

// ---------------------------------------------------------------------
// Reading files

std::vector<uint8_t> ReadFile(const std::string& path) {
  std::vector<uint8_t> data;
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) return data;
  uint8_t buf[65536];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) data.insert(data.end(), buf, buf + n);
  fclose(f);
  return data;
}

uint32_t U32(const std::vector<uint8_t>& d, size_t o) {
  if (o + 4 > d.size()) Fail("the type library is cut short");
  return d[o] | d[o + 1] << 8 | d[o + 2] << 16 | (uint32_t)d[o + 3] << 24;
}
int32_t I32(const std::vector<uint8_t>& d, size_t o) { return (int32_t)U32(d, o); }
uint16_t U16(const std::vector<uint8_t>& d, size_t o) {
  if (o + 2 > d.size()) Fail("the type library is cut short");
  return (uint16_t)(d[o] | d[o + 1] << 8);
}

// The type library inside a PE file: its TYPELIB resource with the given
// number (or the first one), in any language.
bool ExtractTypelibResource(std::vector<uint8_t>& data, int id) {
  if (data.size() < 0x40 || data[0] != 'M' || data[1] != 'Z') return false;
  uint32_t pe = U32(data, 0x3c);
  if (pe + 24 > data.size() || memcmp(&data[pe], "PE\0\0", 4) != 0) return false;
  uint16_t nsec = U16(data, pe + 6), optsize = U16(data, pe + 20);
  size_t opt = pe + 24;
  uint16_t magic = U16(data, opt);
  size_t dirs = opt + (magic == 0x20b ? 112 : 96);
  uint32_t rsrcRva = U32(data, dirs + 2 * 8);
  if (rsrcRva == 0) return false;
  size_t sec = opt + optsize;
  auto rvaToOff = [&](uint32_t rva) -> size_t {
    for (int i = 0; i < nsec; i++) {
      size_t s = sec + i * 40;
      uint32_t va = U32(data, s + 12), vsize = U32(data, s + 8), raw = U32(data, s + 20),
               rawsize = U32(data, s + 16);
      uint32_t span = std::max(vsize, rawsize);
      if (rva >= va && rva < va + span) return raw + (rva - va);
    }
    return (size_t)-1;
  };
  size_t root = rvaToOff(rsrcRva);
  if (root == (size_t)-1) return false;
  // Directory entries: named ones first, then numbered ones.
  auto entryName = [&](uint32_t nameField) {
    std::string s;
    if (!(nameField & 0x80000000)) return s;
    size_t o = root + (nameField & 0x7fffffff);
    uint16_t len = U16(data, o);
    for (int i = 0; i < len; i++) s += (char)U16(data, o + 2 + 2 * i);
    return s;
  };
  auto findEntry = [&](size_t dir, const std::string& name, int num) -> int64_t {
    uint16_t nnamed = U16(data, dir + 12), nids = U16(data, dir + 14);
    for (int i = 0; i < nnamed + nids; i++) {
      size_t e = dir + 16 + i * 8;
      uint32_t nameField = U32(data, e), off = U32(data, e + 4);
      bool match = false;
      if (num < 0 && name.empty()) match = true;
      else if (!name.empty()) {
        std::string n = entryName(nameField);
        match = n.size() == name.size() &&
                std::equal(n.begin(), n.end(), name.begin(),
                           [](char a, char b) { return toupper(a) == toupper(b); });
      } else
        match = !(nameField & 0x80000000) && (int)nameField == num;
      if (match) return off;
    }
    return -1;
  };
  int64_t t = findEntry(root, "TYPELIB", -1);
  if (t < 0 || !(t & 0x80000000)) return false;
  size_t typeDir = root + (t & 0x7fffffff);
  int64_t n = findEntry(typeDir, "", id > 0 ? id : -1);
  if (n < 0 || !(n & 0x80000000)) return false;
  size_t langDir = root + (n & 0x7fffffff);
  int64_t l = findEntry(langDir, "", -1);
  if (l < 0 || (l & 0x80000000)) return false;
  size_t de = root + l;
  uint32_t dataRva = U32(data, de), dataSize = U32(data, de + 4);
  size_t off = rvaToOff(dataRva);
  if (off == (size_t)-1 || off + dataSize > data.size()) return false;
  data = std::vector<uint8_t>(data.begin() + off, data.begin() + off + dataSize);
  return true;
}

// ---------------------------------------------------------------------
// The MSFT format

class MsftReader {
public:
  MsftReader(const std::vector<uint8_t>& data, Lib& lib) : d(data), lib(lib) {}

  void Read() {
    if (d.size() < 0x54 || U32(d, 0) != 0x5446534d) {
      if (d.size() >= 4 && memcmp(d.data(), "SLTG", 4) == 0)
        Fail("%s: type libraries in the old SLTG format are not supported", lib.path);
      Fail("%s is not a type library", lib.path);
    }
    int32_t posguid = I32(d, 8);
    uint32_t varflags = U32(d, 0x14);
    lib.syskind = varflags & 0xf;
    uint32_t version = U32(d, 0x18);
    lib.major = version & 0xffff;
    lib.minor = version >> 16;
    int ntypes = I32(d, 0x20);
    int32_t helpstring = I32(d, 0x24);
    int32_t nameOffset = I32(d, 0x38);
    size_t p = 0x54;
    if (varflags & 0x100) p += 4;
    typeOffsetsPos = p;
    p += 4 * ntypes;
    for (int i = 0; i < 15; i++) {
      seg[i].offset = I32(d, p + i * 16);
      seg[i].length = I32(d, p + i * 16 + 4);
    }
    lib.name = Name(nameOffset);
    lib.doc = String(helpstring);
    if (posguid >= 0) lib.guid = GuidAt(posguid);
    lib.types.resize(ntypes);
    for (int i = 0; i < ntypes; i++) ReadType(i);
  }

private:
  enum { TYPEINFO, IMPINFO, IMPFILES, REFTAB, GUIDHASH, GUIDTAB, NAMEHASH, NAMETAB, STRINGTAB,
         TYPEDESC, ARRAYDESC, CUSTDATA, CDGUIDS };
  struct Seg {
    int32_t offset = -1, length = 0;
  } seg[15];
  const std::vector<uint8_t>& d;
  Lib& lib;
  size_t typeOffsetsPos = 0;

  std::string Name(int32_t o) {
    if (o < 0 || seg[NAMETAB].offset < 0) return "";
    size_t at = seg[NAMETAB].offset + o;
    uint32_t len = U32(d, at + 8) & 0xff;
    if (at + 12 + len > d.size()) Fail("%s: a name is outside the file", lib.path);
    return std::string((const char*)&d[at + 12], len);
  }
  std::string String(int32_t o) {
    if (o < 0 || seg[STRINGTAB].offset < 0) return "";
    size_t at = seg[STRINGTAB].offset + o;
    uint16_t len = U16(d, at);
    if (at + 2 + len > d.size()) Fail("%s: a string is outside the file", lib.path);
    return std::string((const char*)&d[at + 2], len);
  }
  Guid GuidAt(int32_t o) {
    Guid g;
    size_t at = seg[GUIDTAB].offset + o;
    if (o < 0 || at + 16 > d.size()) return g;
    memcpy(g.b, &d[at], 16);
    return g;
  }

  TD Type(int32_t t) {
    auto td = std::make_shared<TypeDesc>();
    if (t < 0) {
      td->vt = t & 0xffff;
      return td;
    }
    size_t at = seg[TYPEDESC].offset + t;
    int32_t w0 = I32(d, at), w1 = I32(d, at + 4);
    td->vt = w0 & 0xffff;
    switch (td->vt) {
    case VT_PTR:
    case VT_SAFEARRAY: td->inner = Type(w1); break;
    case VT_CARRAY: {
      size_t a = seg[ARRAYDESC].offset + (w1 & 0x7fffffff);
      td->inner = Type(I32(d, a));
      uint16_t cdims = U16(d, a + 4);
      for (int i = 0; i < cdims; i++) td->dims.push_back(U32(d, a + 8 + i * 8));
      break;
    }
    case VT_USERDEFINED: td->href = w1; break;
    default: break;
    }
    return td;
  }

  // A constant or default value: inline when it fits, else in the custom
  // data segment.
  Value ValueAt(int32_t v) {
    Value val;
    if (v == -1) return val;
    val.valid = true;
    if (v < 0) {
      val.vt = (v >> 26) & 0x1f;
      uint32_t x = (uint32_t)v & 0x3ffffff;
      switch (val.vt) {
      case VT_I1: val.i = (int8_t)x; break;
      case VT_I2: case VT_BOOL: val.i = (int16_t)x; break;
      default: val.i = x; break;
      }
      return val;
    }
    size_t at = seg[CUSTDATA].offset + v;
    val.vt = U16(d, at);
    at += 2;
    switch (val.vt) {
    case VT_I1: val.i = (int8_t)d[at]; break;
    case VT_UI1: val.i = d[at]; break;
    case VT_I2: case VT_BOOL: val.i = (int16_t)U16(d, at); break;
    case VT_UI2: val.i = U16(d, at); break;
    case VT_I4: case VT_INT: case VT_ERROR: case VT_HRESULT: val.i = I32(d, at); break;
    case VT_UI4: case VT_UINT: val.i = U32(d, at); break;
    case VT_I8: case VT_CY: val.i = (int64_t)((uint64_t)U32(d, at) | (uint64_t)U32(d, at + 4) << 32); break;
    case VT_UI8: val.i = (int64_t)((uint64_t)U32(d, at) | (uint64_t)U32(d, at + 4) << 32); break;
    case VT_R4: { uint32_t x = U32(d, at); float f; memcpy(&f, &x, 4); val.d = f; break; }
    case VT_R8: case VT_DATE: {
      uint64_t x = (uint64_t)U32(d, at) | (uint64_t)U32(d, at + 4) << 32;
      memcpy(&val.d, &x, 8);
      break;
    }
    case VT_BSTR: {
      uint32_t len = U32(d, at);
      if (len != 0xffffffff && at + 4 + len <= d.size())
        val.s.assign((const char*)&d[at + 4], len);
      break;
    }
    default: val.valid = false; break;
    }
    return val;
  }

  void ReadType(int index) {
    TypeInfo& ti = lib.types[index];
    ti.lib = &lib;
    ti.index = index;
    size_t b = seg[TYPEINFO].offset + index * 0x64;
    int32_t typekind = I32(d, b);
    ti.kind = typekind & 0xf;
    ti.align = (typekind >> 11) & 0x1f;
    int32_t memoffset = I32(d, b + 4);
    uint32_t cElement = U32(d, b + 0x18);
    int32_t posguid = I32(d, b + 0x2c);
    ti.flags = I32(d, b + 0x30);
    ti.name = Name(I32(d, b + 0x34));
    ti.doc = String(I32(d, b + 0x3c));
    int16_t cImplTypes = (int16_t)U16(d, b + 0x4c);
    ti.size = I32(d, b + 0x50);
    int32_t datatype1 = I32(d, b + 0x54);
    if (posguid >= 0) {
      ti.guid = GuidAt(posguid);
      ti.hasGuid = true;
    }
    switch (ti.kind) {
    case TKIND_ALIAS: ti.alias = Type(datatype1); break;
    case TKIND_MODULE: ti.dll = String(datatype1); break;
    case TKIND_COCLASS: {
      int32_t r = datatype1;
      for (int i = 0; i < cImplTypes && r >= 0; i++) {
        size_t at = seg[REFTAB].offset + r;
        Impl impl;
        impl.href = I32(d, at);
        impl.flags = I32(d, at + 4);
        ti.impls.push_back(impl);
        r = I32(d, at + 12);
      }
      break;
    }
    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
      if (cImplTypes > 0 && datatype1 != -1) {
        Impl impl;
        impl.href = datatype1;
        ti.impls.push_back(impl);
      }
      break;
    default: break;
    }
    int nfuncs = cElement & 0xffff, nvars = cElement >> 16;
    if (nfuncs + nvars == 0 || memoffset < 0 || (size_t)memoffset + 4 > d.size()) return;
    int32_t recSize = I32(d, memoffset);
    size_t recs = memoffset + 4;
    size_t tables = recs + recSize;
    int n = nfuncs + nvars;
    auto memid = [&](int i) { return I32(d, tables + 4 * i); };
    auto nameOf = [&](int i) { return Name(I32(d, tables + 4 * (n + i))); };
    auto recOf = [&](int i) { return recs + I32(d, tables + 4 * (2 * n + i)); };
    for (int i = 0; i < nfuncs; i++) {
      size_t r = recOf(i);
      Func f;
      f.memid = memid(i);
      f.name = nameOf(i);
      uint16_t size = U16(d, r);
      f.ret = Type(I32(d, r + 4));
      f.flags = I32(d, r + 8) & 0xffff;
      f.vtoff = (int16_t)U16(d, r + 12);
      uint32_t fkccic = U32(d, r + 16);
      f.funckind = fkccic & 7;
      f.invkind = (fkccic >> 3) & 0xf;
      f.callconv = (fkccic >> 8) & 0xf;
      bool hasDefaults = (fkccic & 0x1000) != 0;
      int nargs = (int16_t)U16(d, r + 20);
      f.cParamsOpt = (int16_t)U16(d, r + 22);
      size_t params = r + size - nargs * 12;
      size_t defaults = params - (hasDefaults ? nargs * 4 : 0);
      int nopt = ((int)(defaults - (r + 24))) / 4;
      if (nopt >= 2) f.doc = String(I32(d, r + 28));
      for (int a = 0; a < nargs; a++) {
        Param p;
        size_t pa = params + a * 12;
        p.type = Type(I32(d, pa));
        int32_t oName = I32(d, pa + 4);
        p.name = oName >= 0 ? Name(oName) : "";
        p.flags = I32(d, pa + 8);
        if (hasDefaults && (p.flags & PARAMFLAG_FHASDEFAULT)) p.def = ValueAt(I32(d, defaults + a * 4));
        f.params.push_back(p);
      }
      ti.funcs.push_back(f);
    }
    for (int i = 0; i < nvars; i++) {
      size_t r = recOf(nfuncs + i);
      Var v;
      v.memid = memid(nfuncs + i);
      v.name = nameOf(nfuncs + i);
      uint16_t size = U16(d, r);
      v.type = Type(I32(d, r + 4));
      v.flags = I32(d, r + 8) & 0xffff;
      v.varkind = (int16_t)U16(d, r + 12);
      int32_t offsValue = I32(d, r + 16);
      if (v.varkind == VAR_CONST) v.value = ValueAt(offsValue);
      if (size >= 28) v.doc = String(I32(d, r + 24));
      ti.vars.push_back(v);
    }
  }

public:
  // An imported type: the import information at an href.
  void ReadImports() {
    if (seg[IMPINFO].offset < 0) return;
    for (int32_t o = 0; o + 12 <= seg[IMPINFO].length; o += 12) {
      size_t at = seg[IMPINFO].offset + o;
      uint32_t flags = U32(d, at);
      int32_t oImpFile = I32(d, at + 4), oGuid = I32(d, at + 8);
      ImportRef ref;
      ref.kind = flags >> 24;
      ref.byGuid = (flags & 0x10000) != 0;
      if (ref.byGuid)
        ref.typeGuid = GuidAt(oGuid);
      else
        ref.typeIndex = oGuid;
      size_t f = seg[IMPFILES].offset + oImpFile;
      ref.libGuid = GuidAt(I32(d, f));
      uint16_t lenField = U16(d, f + 12);
      uint16_t len = lenField >> 2;
      if (f + 14 + len <= d.size()) ref.file.assign((const char*)&d[f + 14], len);
      lib.imports[o] = ref;
    }
  }
};

// ---------------------------------------------------------------------
// Loading type libraries and resolving references between them

std::vector<std::string> g_searchDirs;
std::vector<std::unique_ptr<Lib>> g_libs;

std::string DirName(const std::string& p) {
  size_t s = p.find_last_of("/\\");
  return s == std::string::npos ? "." : p.substr(0, s);
}
std::string BaseName(const std::string& p) {
  size_t s = p.find_last_of("/\\");
  return s == std::string::npos ? p : p.substr(s + 1);
}

Lib* LoadLib(const std::string& path, int resId, bool required) {
  for (auto& l : g_libs)
    if (l->path == path) return l.get();
  std::vector<uint8_t> data = ReadFile(path);
  if (data.empty()) {
    if (required) Fail("cannot read %s", path);
    return nullptr;
  }
  if (data[0] == 'M' && data[1] == 'Z' && !ExtractTypelibResource(data, resId)) {
    if (required) Fail("%s has no type library resource", path);
    return nullptr;
  }
  auto lib = std::make_unique<Lib>();
  lib->path = path;
  MsftReader reader(data, *lib);
  reader.Read();
  reader.ReadImports();
  g_libs.push_back(std::move(lib));
  return g_libs.back().get();
}

// The library an import names, found by its file name next to the
// importing library or in the search directories.
Lib* FindImportedLib(const Lib& from, const ImportRef& ref) {
  for (auto& l : g_libs)
    if (l->guid == ref.libGuid) return l.get();
  std::string base = BaseName(ref.file);
  std::vector<std::string> dirs;
  dirs.push_back(DirName(from.path));
  dirs.insert(dirs.end(), g_searchDirs.begin(), g_searchDirs.end());
  for (const auto& dir : dirs) {
    std::string p = dir + "/" + base;
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) continue;
    fclose(f);
    Lib* l = LoadLib(p, 0, false);
    if (l) return l;
  }
  return nullptr;
}

// The well-known types of stdole2.tlb, for when it cannot be found.
const char* KnownName(const Guid& g) {
  static const struct {
    const char* guid;
    const char* name;
  } known[] = {
      {"00000000-0000-0000-c000-000000000046", "IUnknown"},
      {"00020400-0000-0000-c000-000000000046", "IDispatch"},
      {"00020404-0000-0000-c000-000000000046", "IEnumVARIANT"},
      {"bef6e002-a874-101a-8bba-00aa00300cab", "IFont"},
      {"bef6e003-a874-101a-8bba-00aa00300cab", "Font"},
      {"7bf80980-bf32-101a-8bbb-00aa00300cab", "IPicture"},
      {"7bf80981-bf32-101a-8bbb-00aa00300cab", "Picture"},
      {"4ef6100a-af88-11d0-9846-00c04fc29993", "FontEvents"},
      {"0be35203-8f91-11ce-9de3-00aa004bb851", "StdFont"},
      {"0be35204-8f91-11ce-9de3-00aa004bb851", "StdPicture"},
  };
  std::string t = g.Text();
  for (const auto& k : known)
    if (t == k.guid) return k.name;
  return nullptr;
}

struct Resolved {
  const TypeInfo* ti = nullptr;  // null when only the name is known
  std::string name;
  int kind = -1;
  bool local = false;  // in the library being imported
};

Resolved Resolve(const Lib& lib, int href, const Lib* main) {
  Resolved r;
  if ((href & 3) == 0) {
    int index = href / 0x64;
    if (index >= 0 && index < (int)lib.types.size()) {
      r.ti = &lib.types[index];
      r.name = r.ti->name;
      r.kind = r.ti->kind;
      r.local = &lib == main;
    }
    return r;
  }
  auto it = lib.imports.find(href & ~3);
  if (it == lib.imports.end()) {
    r.name = "__missing_type__";
    return r;
  }
  const ImportRef& ref = it->second;
  r.kind = ref.kind;
  if (Lib* other = FindImportedLib(lib, ref)) {
    if (ref.byGuid) {
      for (const auto& t : other->types)
        if (t.hasGuid && t.guid == ref.typeGuid) {
          r.ti = &t;
          break;
        }
    } else if (ref.typeIndex >= 0 && ref.typeIndex < (int)other->types.size()) {
      r.ti = &other->types[ref.typeIndex];
    }
    if (r.ti) {
      r.name = r.ti->name;
      r.kind = r.ti->kind;
      return r;
    }
  }
  if (ref.byGuid) {
    if (const char* n = KnownName(ref.typeGuid)) {
      r.name = n;
      return r;
    }
  }
  fprintf(stderr, "tlbimport: warning: a type from %s was not found; it is written as __missing_type__\n",
          ref.file.c_str());
  r.name = "__missing_type__";
  return r;
}

// ---------------------------------------------------------------------
// Options

struct Options {
  bool noNamespace = false, namedGuids = false, rawInterfacesOnly = false, rawNativeTypes = false,
       rawDispinterfaces = false, noSmartPointers = false, noImplementation = false,
       implementationOnly = false, noAutoExclude = false;
  std::string ns;
  bool nsSet = false;
  std::map<std::string, std::string> renames;
  std::set<std::string> excludes;
  std::string highMethodPrefix;
  std::string highGet = "Get", highPut = "Put", highPutRef = "PutRef";
  std::string rawMethodPrefix = "raw_";
  std::string rawGet = "get_", rawPut = "put_", rawPutRef = "putref_";
  std::vector<std::string> injects;  // "Type\x01statement"
};
Options g_opt;

// The strings in an attribute's parentheses: name("a", "b") or name(3).
std::vector<std::string> AttrArgs(const std::string& attr) {
  std::vector<std::string> args;
  size_t open = attr.find('(');
  if (open == std::string::npos) return args;
  size_t i = open + 1;
  while (i < attr.size()) {
    while (i < attr.size() && (isspace((unsigned char)attr[i]) || attr[i] == ',')) i++;
    if (i >= attr.size() || attr[i] == ')') break;
    std::string s;
    if (attr[i] == '"') {
      i++;
      while (i < attr.size() && attr[i] != '"') {
        if (attr[i] == '\\' && i + 1 < attr.size()) {
          i++;
          switch (attr[i]) {
          case 'n': s += '\n'; break;
          case 't': s += '\t'; break;
          default: s += attr[i]; break;
          }
        } else
          s += attr[i];
        i++;
      }
      i++;
    } else {
      while (i < attr.size() && attr[i] != ',' && attr[i] != ')') s += attr[i++];
      while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
    }
    args.push_back(s);
  }
  return args;
}

void ParseAttribute(const std::string& attr, int& tlbid) {
  std::string name = attr.substr(0, attr.find('('));
  while (!name.empty() && isspace((unsigned char)name.back())) name.pop_back();
  std::vector<std::string> args = AttrArgs(attr);
  auto arg = [&](size_t i) { return i < args.size() ? args[i] : std::string(); };
  if (name == "no_namespace") g_opt.noNamespace = true;
  else if (name == "rename_namespace") { g_opt.ns = arg(0); g_opt.nsSet = true; }
  else if (name == "named_guids") g_opt.namedGuids = true;
  else if (name == "raw_interfaces_only") g_opt.rawInterfacesOnly = true;
  else if (name == "raw_native_types") g_opt.rawNativeTypes = true;
  else if (name == "raw_dispinterfaces") g_opt.rawDispinterfaces = true;
  else if (name == "no_smart_pointers") g_opt.noSmartPointers = true;
  else if (name == "no_implementation") g_opt.noImplementation = true;
  else if (name == "implementation_only") g_opt.implementationOnly = true;
  else if (name == "no_auto_exclude") g_opt.noAutoExclude = true;
  else if (name == "rename") { if (args.size() >= 2) g_opt.renames[args[0]] = args[1]; }
  else if (name == "exclude") { for (auto& a : args) g_opt.excludes.insert(a); }
  else if (name == "high_method_prefix") g_opt.highMethodPrefix = arg(0);
  else if (name == "high_property_prefixes") {
    if (args.size() >= 3) { g_opt.highGet = args[0]; g_opt.highPut = args[1]; g_opt.highPutRef = args[2]; }
  } else if (name == "raw_method_prefix") g_opt.rawMethodPrefix = arg(0);
  else if (name == "raw_property_prefixes") {
    if (args.size() >= 3) { g_opt.rawGet = args[0]; g_opt.rawPut = args[1]; g_opt.rawPutRef = args[2]; }
  } else if (name == "inject_statement") {
    if (args.size() >= 2) g_opt.injects.push_back(args[0] + '\x01' + args[1]);
  } else if (name == "tlbid") tlbid = atoi(arg(0).c_str());
  else if (name == "auto_rename" || name == "auto_search" || name == "embedded_idl" ||
           name == "include" || name == "no_dual_interfaces" || name == "no_search_namespace" ||
           name == "rename_search_namespace" || name == "no_registry") {
  } else
    fprintf(stderr, "tlbimport: warning: unknown #import attribute %s ignored\n", name.c_str());
}

// ---------------------------------------------------------------------
// Writing the headers

bool IsKeyword(const std::string& s) {
  static const std::set<std::string> kw = {
      "alignas", "alignof", "and", "asm", "auto", "bool", "break", "case", "catch", "char",
      "class", "const", "constexpr", "continue", "default", "delete", "do", "double",
      "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend",
      "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not",
      "nullptr", "operator", "or", "private", "protected", "public", "register", "return",
      "short", "signed", "sizeof", "static", "struct", "switch", "template", "this", "throw",
      "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual",
      "void", "volatile", "while", "xor"};
  return kw.count(s) != 0;
}

std::string Id(const std::string& name) {
  auto it = g_opt.renames.find(name);
  std::string s = it != g_opt.renames.end() ? it->second : name;
  if (IsKeyword(s)) s += "_";
  return s;
}

class Writer {
public:
  Writer(Lib& lib, const std::string& base) : lib(lib), base(base) {}

  std::string tlh, tli;
  std::set<std::string> typeNames;  // for ParamName

  void Run() {
    ns = g_opt.noNamespace ? "" : (g_opt.nsSet ? g_opt.ns : Id(lib.name));
    for (const auto& t : lib.types)
      if (Included(t)) order.push_back(&t);
    WriteTlh();
    WriteTli();
  }

private:
  Lib& lib;
  std::string base, ns;
  std::vector<const TypeInfo*> order;

  // System types a library may define again; the Windows headers have them.
  bool AutoExcluded(const TypeInfo& t) {
    if (g_opt.noAutoExclude) return false;
    if (t.hasGuid && KnownName(t.guid)) return true;
    static const std::set<std::string> names = {
        "GUID", "IUnknown", "IDispatch", "IEnumVARIANT", "IFont", "IPicture", "IFontDisp",
        "IPictureDisp", "Font", "Picture", "FontEvents", "StdFont", "StdPicture",
        "DISPPARAMS", "EXCEPINFO", "OLE_COLOR", "OLE_XPOS_PIXELS", "OLE_YPOS_PIXELS",
        "OLE_XSIZE_PIXELS", "OLE_YSIZE_PIXELS", "OLE_XPOS_HIMETRIC", "OLE_YPOS_HIMETRIC",
        "OLE_XSIZE_HIMETRIC", "OLE_YSIZE_HIMETRIC", "OLE_XPOS_CONTAINER", "OLE_YPOS_CONTAINER",
        "OLE_XSIZE_CONTAINER", "OLE_YSIZE_CONTAINER", "OLE_HANDLE", "OLE_OPTEXCLUSIVE",
        "OLE_CANCELBOOL", "OLE_ENABLEDEFAULTBOOL", "OLE_TRISTATE", "FONTNAME", "FONTSIZE",
        "FONTBOLD", "FONTITALIC", "FONTUNDERSCORE", "FONTSTRIKETHROUGH", "LoadPictureConstants"};
    return names.count(t.name) != 0;
  }
  bool Included(const TypeInfo& t) {
    if (g_opt.excludes.count(t.name)) return false;
    return !AutoExcluded(t);
  }

  bool IsDual(const TypeInfo& t) { return t.kind == TKIND_DISPATCH && (t.flags & TYPEFLAG_FDUAL); }
  bool IsVtbl(const TypeInfo& t) { return t.kind == TKIND_INTERFACE || IsDual(t); }
  bool IsDispOnly(const TypeInfo& t) { return t.kind == TKIND_DISPATCH && !IsDual(t); }

  Resolved Res(const TypeInfo& from, int href) { return Resolve(*from.lib, href, &lib); }

  // The name a type is written with: qualified when it is another
  // library's... which #import cannot know, so names stay as they are.
  std::string RefName(const Resolved& r) { return Id(r.name); }

  // Follows aliases to the type underneath.
  TD Underlying(const TypeInfo& from, TD t, const TypeInfo** owner = nullptr) {
    const TypeInfo* cur = &from;
    for (int guard = 0; guard < 32 && t && t->vt == VT_USERDEFINED; guard++) {
      Resolved r = Resolve(*cur->lib, t->href, &lib);
      if (!r.ti || r.ti->kind != TKIND_ALIAS) {
        if (owner) *owner = cur;
        return t;
      }
      cur = r.ti;
      t = r.ti->alias;
    }
    if (owner) *owner = cur;
    return t;
  }

  int UserKind(const TypeInfo& from, TD t) {
    const TypeInfo* owner = nullptr;
    TD u = Underlying(from, t, &owner);
    if (!u || u->vt != VT_USERDEFINED) return -1;
    Resolved r = Resolve(*owner->lib, u->href, &lib);
    return r.kind;
  }

  // The C type of a type description, as in the raw methods.
  std::string CType(const TypeInfo& from, const TD& t) {
    switch (t->vt) {
    case VT_I2: return "short";
    case VT_I4: return "long";
    case VT_R4: return "float";
    case VT_R8: return "double";
    case VT_CY: return "CURRENCY";
    case VT_DATE: return "DATE";
    case VT_BSTR: return "BSTR";
    case VT_DISPATCH: return "IDispatch *";
    case VT_ERROR: return "SCODE";
    case VT_BOOL: return "VARIANT_BOOL";
    case VT_VARIANT: return "VARIANT";
    case VT_UNKNOWN: return "IUnknown *";
    case VT_DECIMAL: return "DECIMAL";
    case VT_I1: return "char";
    case VT_UI1: return "unsigned char";
    case VT_UI2: return "unsigned short";
    case VT_UI4: return "unsigned long";
    case VT_I8: return "__int64";
    case VT_UI8: return "unsigned __int64";
    case VT_INT: return "int";
    case VT_UINT: return "unsigned int";
    case VT_VOID: return "void";
    case VT_HRESULT: return "HRESULT";
    case VT_LPSTR: return "LPSTR";
    case VT_LPWSTR: return "LPWSTR";
    case VT_INT_PTR: return "INT_PTR";
    case VT_UINT_PTR: return "UINT_PTR";
    case VT_PTR: return CType(from, t->inner) + " *";
    case VT_SAFEARRAY: return "SAFEARRAY *";
    case VT_CARRAY: return CType(from, t->inner);  // the dimensions follow the name
    case VT_USERDEFINED: {
      Resolved r = Res(from, t->href);
      std::string n = RefName(r);
      if (r.kind == TKIND_ENUM) return "enum " + n;
      if (r.kind == TKIND_INTERFACE || r.kind == TKIND_DISPATCH || r.kind == TKIND_RECORD ||
          r.kind == TKIND_COCLASS)
        return "struct " + n;
      if (r.kind == TKIND_UNION) return "union " + n;
      return n;
    }
    default: return "__missing_type__";
    }
  }
  std::string Dims(const TD& t) {
    std::string s;
    if (t->vt == VT_CARRAY)
      for (uint32_t n : t->dims) s += "[" + std::to_string(n) + "]";
    return s;
  }
  std::string Decl(const TypeInfo& from, const TD& t, const std::string& name) {
    std::string c = CType(from, t);
    if (!c.empty() && c.back() == '*') return c + " " + name + Dims(t);
    return c + " " + name + Dims(t);
  }

  // ------------------------------------------------------------------
  // The wrapper ("high-level") side

  enum class Kind { Plain, Bstr, Variant, Iface, Void };
  struct HighType {
    std::string type;   // the wrapper's type
    Kind kind = Kind::Plain;
    std::string iface;  // the interface, for Iface
  };

  bool IsInterfacePtr(const TypeInfo& from, const TD& t, std::string* name) {
    if (t->vt == VT_DISPATCH) { *name = "IDispatch"; return true; }
    if (t->vt == VT_UNKNOWN) { *name = "IUnknown"; return true; }
    if (t->vt == VT_PTR && t->inner && t->inner->vt == VT_USERDEFINED) {
      Resolved r = Res(from, t->inner->href);
      if (r.kind == TKIND_INTERFACE || r.kind == TKIND_DISPATCH) {
        *name = RefName(r);
        return true;
      }
      if (r.kind == TKIND_COCLASS && r.ti) {
        // A pointer to a coclass is a pointer to its default interface.
        for (const auto& impl : r.ti->impls)
          if (impl.flags & IMPLTYPEFLAG_FDEFAULT && !(impl.flags & IMPLTYPEFLAG_FSOURCE)) {
            *name = RefName(Resolve(*r.ti->lib, impl.href, &lib));
            return true;
          }
      }
    }
    return false;
  }

  // The type a value of t has in a wrapper.
  HighType High(const TypeInfo& from, const TD& t) {
    HighType h;
    TD u = Underlying(from, t);
    std::string iface;
    if (u->vt == VT_VOID || u->vt == VT_EMPTY) { h.type = "void"; h.kind = Kind::Void; return h; }
    if (!g_opt.rawNativeTypes && u->vt == VT_BSTR && t->vt == VT_BSTR) {
      h.type = "_bstr_t"; h.kind = Kind::Bstr; return h;
    }
    if (!g_opt.rawNativeTypes && u->vt == VT_VARIANT && t->vt == VT_VARIANT) {
      h.type = "_variant_t"; h.kind = Kind::Variant; return h;
    }
    if (IsInterfacePtr(from, u, &iface)) {
      h.iface = iface;
      h.kind = Kind::Iface;
      h.type = g_opt.noSmartPointers ? CType(from, t) : iface + "Ptr";
      return h;
    }
    h.type = CType(from, t);
    return h;
  }

  // A parameter's name. One that names a type of a loaded library gets a
  // trailing underscore, since it would hide the type from the parameters
  // and default arguments after it (IOMode IOMode = ForReading).
  std::string ParamName(const Param& p, size_t i) {
    if (p.name.empty()) return "_arg" + std::to_string(i + 1);
    std::string n = Id(p.name);
    if (typeNames.empty())
      for (auto& l : g_libs)
        for (const auto& t : l->types) typeNames.insert(Id(t.name));
    if (typeNames.count(n)) n += "_";
    return n;
  }

  bool IsOptVariantPtr(const TypeInfo& from, const Param& p) {
    if (!(p.flags & PARAMFLAG_FOPT) || (p.flags & PARAMFLAG_FOUT)) return false;
    TD u = Underlying(from, p.type);
    return u->vt == VT_PTR && Underlying(from, u->inner)->vt == VT_VARIANT;
  }

  bool HasDefault(const TypeInfo& from, const Param& p) {
    if (p.def.valid) return true;
    if (!(p.flags & PARAMFLAG_FOPT)) return false;
    return p.type->vt == VT_VARIANT || IsOptVariantPtr(from, p);
  }

  // Which parameters get default arguments: as in C++, only a trailing run
  // of them can (skip is the retval parameter, which the wrapper returns).
  std::vector<bool> TrailingDefaults(const TypeInfo& from, const Func& f, const Param* skip) {
    std::vector<bool> r(f.params.size(), false);
    for (size_t i = f.params.size(); i-- > 0;) {
      if (&f.params[i] == skip) continue;
      if (!HasDefault(from, f.params[i])) break;
      r[i] = true;
    }
    return r;
  }

  // A wrapper parameter's declaration.
  std::string HighParam(const TypeInfo& from, const Param& p, const std::string& name,
                        bool withDefault) {
    TD t = p.type;
    std::string decl;
    bool isVariant = t->vt == VT_VARIANT && !g_opt.rawNativeTypes;
    bool isBstr = t->vt == VT_BSTR && !g_opt.rawNativeTypes;
    if (isVariant)
      decl = "const _variant_t & " + name;
    else if (isBstr)
      decl = "_bstr_t " + name;
    else
      decl = Decl(from, t, name);
    if (withDefault) {
      if (p.def.valid)
        decl += " = " + DefaultText(from, p);
      else if ((p.flags & PARAMFLAG_FOPT) && t->vt == VT_VARIANT)
        decl += " = vtMissing";
      else if (IsOptVariantPtr(from, p))
        decl += " = &vtMissing";
    }
    return decl;
  }

  std::string DefaultText(const TypeInfo& from, const Param& p) {
    const Value& v = p.def;
    TD u = Underlying(from, p.type);
    char buf[64];
    switch (v.vt) {
    case VT_BSTR: {
      std::string s = "L\"";
      for (char c : v.s) {
        if (c == '"' || c == '\\') s += '\\';
        s += c;
      }
      s += "\"";
      if (u->vt == VT_VARIANT && !g_opt.rawNativeTypes) return "_variant_t(" + s + ")";
      return s;
    }
    case VT_R4: case VT_R8: case VT_DATE:
      snprintf(buf, sizeof buf, "%.17g", v.d);
      if (u->vt == VT_VARIANT && !g_opt.rawNativeTypes) return std::string("_variant_t(") + buf + ")";
      return buf;
    default: {
      std::string s;
      if (u->vt == VT_PTR || u->vt == VT_DISPATCH || u->vt == VT_UNKNOWN || u->vt == VT_BSTR)
        return v.i == 0 ? "NULL" : std::to_string(v.i);
      if (u->vt == VT_VARIANT && !g_opt.rawNativeTypes)
        return "_variant_t(" + std::to_string(v.i) + "L)";
      if (u->vt == VT_USERDEFINED) {
        int k = UserKind(from, p.type);
        if (k == TKIND_ENUM) return "(" + CType(from, p.type) + ")" + std::to_string(v.i);
      }
      if (u->vt == VT_BOOL) return v.i ? "VARIANT_TRUE" : "VARIANT_FALSE";
      return std::to_string(v.i) + (u->vt == VT_I4 || u->vt == VT_UI4 ? "L" : "");
    }
    }
  }

  // ------------------------------------------------------------------
  // The VARTYPE a dispatch call passes a value of type t as.
  int DispVt(const TypeInfo& from, const TD& t) {
    TD u = Underlying(from, t);
    switch (u->vt) {
    case VT_PTR: {
      std::string iface;
      if (IsInterfacePtr(from, u, &iface)) {
        TD in = Underlying(from, u->inner);
        Resolved r = in->vt == VT_USERDEFINED ? Res(from, in->href) : Resolved();
        if (r.ti && r.ti->kind == TKIND_INTERFACE && !(r.ti->flags & 0x1000)) return VT_UNKNOWN;
        return VT_DISPATCH;
      }
      return DispVt(from, u->inner) | VT_BYREF;
    }
    case VT_SAFEARRAY: return VT_ARRAY | (DispVt(from, u->inner) & 0xfff);
    case VT_USERDEFINED: {
      int k = UserKind(from, t);
      if (k == TKIND_ENUM) return VT_I4;
      if (k == TKIND_RECORD) return VT_RECORD;
      return VT_I4;
    }
    case VT_HRESULT: return VT_ERROR;
    case VT_INT_PTR: return VT_I4;
    case VT_UINT_PTR: return VT_UI4;
    case VT_VOID: return VT_EMPTY;
    default: return u->vt;
    }
  }

  static std::string VtName(int vt) {
    static const char* names[] = {"VT_EMPTY", "VT_NULL", "VT_I2", "VT_I4", "VT_R4", "VT_R8",
                                  "VT_CY", "VT_DATE", "VT_BSTR", "VT_DISPATCH", "VT_ERROR",
                                  "VT_BOOL", "VT_VARIANT", "VT_UNKNOWN", "VT_DECIMAL", "15",
                                  "VT_I1", "VT_UI1", "VT_UI2", "VT_UI4", "VT_I8", "VT_UI8",
                                  "VT_INT", "VT_UINT"};
    int base = vt & 0xfff;
    std::string s = base < 24 ? names[base] : (base == VT_RECORD ? "VT_RECORD" : std::to_string(base));
    if (vt & VT_BYREF) s += "|VT_BYREF";
    if (vt & VT_ARRAY) s += "|VT_ARRAY";
    return s;
  }

  static std::string Hex(uint32_t v) {
    char b[16];
    snprintf(b, sizeof b, "0x%x", v);
    return b;
  }

  // ------------------------------------------------------------------

  std::string Comment(const std::string& doc) {
    std::string s;
    for (char c : doc)
      if (c != '\n' && c != '\r') s += c;
    size_t p;
    while ((p = s.find("*/")) != std::string::npos) s.replace(p, 2, "* /");
    return s;
  }

  // The functions of a pure dispinterface, with the properties first.
  void WriteDispMembers(std::string& o, const TypeInfo& t, std::vector<std::string>& impl) {
    const TypeInfo* src = &t;
    // dispinterface D { interface I; }: I's members, through IDispatch.
    if (t.funcs.empty() && t.vars.empty() && !t.impls.empty()) {
      Resolved r = Res(t, t.impls[0].href);
      if (r.ti && r.name != "IDispatch") src = r.ti;
    }
    std::string cls = Id(t.name);
    if (!src->vars.empty()) o += "    //\n    // Property data\n    //\n\n";
    for (const auto& v : src->vars) {
      std::string n = Id(v.name);
      HighType h = High(*src, v.type);
      bool ro = (v.flags & VARFLAG_FREADONLY) != 0;
      o += "#if __has_declspec_attribute(property)\n";
      o += "    __declspec(property(get=" + g_opt.highGet + n + (ro ? "" : ",put=" + g_opt.highPut + n) +
           "))\n    " + h.type + " " + n + ";\n#endif\n";
    }
    bool any = !src->vars.empty();
    for (const auto& f : src->funcs)
      if (f.invkind != INVOKE_FUNC) any = true;
    if (!src->funcs.empty() || !src->vars.empty()) o += "\n    //\n    // Wrapper methods for error-handling\n    //\n\n";
    (void)any;
    // Properties as variables.
    for (const auto& v : src->vars) {
      std::string n = Id(v.name);
      HighType h = High(*src, v.type);
      int vt = DispVt(*src, v.type);
      o += "    // Properties:\n" + std::string("    ") + h.type + " " + g_opt.highGet + n + " ( );\n";
      impl.push_back(DispGet(*src, cls, n, h, vt, v.memid));
      if (!(v.flags & VARFLAG_FREADONLY)) {
        o += "    void " + g_opt.highPut + n + " ( " + HighValueParam(*src, v.type) + " );\n";
        impl.push_back(DispPut(*src, cls, n, v.type, vt, v.memid));
      }
    }
    // Methods and property functions.
    for (const auto& f : src->funcs) {
      if (f.funckind != FUNC_DISPATCH && !IsDispOnly(*src)) continue;
      WriteDispFunc(o, *src, cls, f, impl);
    }
  }

  std::string HighValueParam(const TypeInfo& from, const TD& t) {
    Param p;
    p.type = t;
    return HighParam(from, p, "_val", false);
  }

  std::string ResultDecl(const HighType& h, const TypeInfo& from, const TD& t, int vt) {
    if (vt == VT_VARIANT) return "    VARIANT _result;\n    VariantInit(&_result);\n";
    std::string c = CType(from, t);
    if (h.kind == Kind::Iface) c = h.iface + " *";
    // A cast, since an enum can't be initialised from 0 in C++.
    return "    " + c + " _result" + (vt == VT_DECIMAL || vt == VT_CY ? "" : " = (" + c + ")0") + ";\n";
  }
  std::string ResultReturn(const HighType& h) {
    switch (h.kind) {
    case Kind::Bstr: return "    return _bstr_t(_result, false);\n";
    case Kind::Variant: return "    return _variant_t(_result, false);\n";
    case Kind::Iface:
      if (!g_opt.noSmartPointers) return "    return " + h.type + "(_result, false);\n";
      return "    return _result;\n";
    case Kind::Void: return "";
    default: return "    return _result;\n";
    }
  }

  std::string DispGet(const TypeInfo& from, const std::string& cls, const std::string& n,
                      const HighType& h, int vt, int32_t memid) {
    TD t;
    for (const auto& v : from.vars)
      if (v.memid == memid) t = v.type;
    std::string s = "inline " + h.type + " " + cls + "::" + g_opt.highGet + n + " ( ) {\n";
    s += ResultDecl(h, from, t, vt);
    s += "    _com_dispatch_propget(this, " + Hex((uint32_t)memid) + ", " + VtName(vt) +
         ", (void*)&_result);\n";
    s += ResultReturn(h) + "}\n\n";
    return s;
  }

  std::string ArgExpr(const TypeInfo& from, const TD& t, const std::string& name) {
    TD u = Underlying(from, t);
    if (u->vt == VT_VARIANT) return "&" + name;
    if (u->vt == VT_BSTR && !g_opt.rawNativeTypes) return "(BSTR)" + name;
    if (u->vt == VT_DECIMAL) return "&" + name;
    return name;
  }

  std::string DispPut(const TypeInfo& from, const std::string& cls, const std::string& n,
                      const TD& t, int vt, int32_t memid) {
    std::string s = "inline void " + cls + "::" + g_opt.highPut + n + " ( " +
                    HighValueParam(from, t) + " ) {\n";
    s += "    _com_dispatch_propput(this, " + Hex((uint32_t)memid) + ", " + VtName(vt) + ", " +
         ArgExpr(from, t, "_val") + ");\n}\n\n";
    return s;
  }

  void WriteDispFunc(std::string& o, const TypeInfo& from, const std::string& cls, const Func& f,
                     std::vector<std::string>& impl) {
    std::string n = Id(f.name);
    std::string wname;
    std::string flags;
    switch (f.invkind) {
    case INVOKE_PROPERTYGET: wname = g_opt.highGet + n; flags = "DISPATCH_PROPERTYGET"; break;
    case INVOKE_PROPERTYPUT: wname = g_opt.highPut + n; flags = "DISPATCH_PROPERTYPUT"; break;
    case INVOKE_PROPERTYPUTREF: wname = g_opt.highPutRef + n; flags = "DISPATCH_PROPERTYPUTREF"; break;
    default: wname = g_opt.highMethodPrefix + n; flags = "DISPATCH_METHOD"; break;
    }
    HighType h = High(from, f.ret);
    int rvt = DispVt(from, f.ret);
    std::vector<std::string> decls, defs;
    std::string info, args;
    std::vector<bool> withDefault = TrailingDefaults(from, f, nullptr);
    for (size_t i = 0; i < f.params.size(); i++) {
      const Param& p = f.params[i];
      std::string pn = ParamName(p, i);
      decls.push_back(HighParam(from, p, pn, withDefault[i]));
      defs.push_back(HighParam(from, p, pn, false));
      int vt = DispVt(from, p.type);
      char b[16];
      snprintf(b, sizeof b, "\\x%04x", vt);
      info += b;
      args += ", " + ArgExpr(from, p.type, pn);
    }
    o += "    " + h.type + " " + wname + " ( " + Join(decls) + " );\n";
    std::string s = "inline " + h.type + " " + cls + "::" + wname + " ( " + Join(defs) + " ) {\n";
    if (h.kind != Kind::Void) s += ResultDecl(h, from, f.ret, rvt);
    s += "    _com_dispatch_method(this, " + Hex((uint32_t)f.memid) + ", " + flags + ", " +
         VtName(h.kind == Kind::Void ? VT_EMPTY : rvt) + ", " +
         (h.kind == Kind::Void ? "NULL" : "(void*)&_result") + ", \n        L\"" + info + "\"" +
         args + ");\n";
    s += ResultReturn(h) + "}\n\n";
    impl.push_back(s);
  }

  static std::string Join(const std::vector<std::string>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); i++) s += (i ? ", " : "") + v[i];
    return s;
  }

  // The raw name of a vtable function.
  std::string RawName(const Func& f, bool wrapped) {
    std::string n = Id(f.name);
    switch (f.invkind) {
    case INVOKE_PROPERTYGET: return g_opt.rawGet + n;
    case INVOKE_PROPERTYPUT: return g_opt.rawPut + n;
    case INVOKE_PROPERTYPUTREF: return g_opt.rawPutRef + n;
    default: return wrapped ? g_opt.rawMethodPrefix + n : n;
    }
  }

  bool ReturnsHresult(const Func& f) {
    return f.ret->vt == VT_HRESULT || (f.ret->vt == VT_ERROR && false);
  }

  void WriteVtblMembers(std::string& o, const TypeInfo& t, std::vector<std::string>& impl) {
    std::string cls = Id(t.name);
    bool wrappers = !g_opt.rawInterfacesOnly;
    // Properties.
    if (wrappers) {
      std::map<std::string, std::pair<std::string, int>> props;  // name -> type, accessors
      std::vector<std::string> propOrder;
      for (const auto& f : t.funcs) {
        if (f.invkind == INVOKE_FUNC || !ReturnsHresult(f)) continue;
        std::string n = Id(f.name);
        if (!props.count(n)) propOrder.push_back(n);
        auto& pr = props[n];
        pr.second |= f.invkind;
        int nidx = 0;
        for (const auto& p : f.params)
          if (!(p.flags & (PARAMFLAG_FRETVAL | PARAMFLAG_FLCID))) nidx++;
        if (f.invkind == INVOKE_PROPERTYGET) {
          const Param* rv = RetvalParam(f);
          if (rv) pr.first = High(t, rv->type->inner ? rv->type->inner : rv->type).type;
          pr.second |= nidx << 8;
        } else if (pr.first.empty() && !f.params.empty()) {
          pr.first = High(t, f.params.back().type).type;
          pr.second |= (nidx - 1) << 8;
        }
      }
      if (!propOrder.empty()) o += "    //\n    // Property data\n    //\n\n";
      for (const auto& n : propOrder) {
        auto& pr = props[n];
        std::string acc;
        if (pr.second & INVOKE_PROPERTYGET) acc += "get=" + g_opt.highGet + n;
        if (pr.second & INVOKE_PROPERTYPUT) acc += std::string(acc.empty() ? "" : ",") + "put=" + g_opt.highPut + n;
        else if (pr.second & INVOKE_PROPERTYPUTREF)
          acc += std::string(acc.empty() ? "" : ",") + "put=" + g_opt.highPutRef + n;
        std::string idx;
        for (int i = 0; i < (pr.second >> 8); i++) idx += "[]";
        o += "#if __has_declspec_attribute(property)\n";
        o += "    __declspec(property(" + acc + "))\n    " + (pr.first.empty() ? "_variant_t" : pr.first) +
             " " + n + idx + ";\n#endif\n";
      }
      bool anyWrapper = false;
      for (const auto& f : t.funcs)
        if (ReturnsHresult(f)) anyWrapper = true;
      if (anyWrapper) o += "\n    //\n    // Wrapper methods for error-handling\n    //\n\n";
      for (const auto& f : t.funcs)
        if (ReturnsHresult(f)) WriteWrapper(o, t, cls, f, impl);
    }
    o += "\n    //\n    // Raw methods provided by interface\n    //\n\n";
    for (const auto& f : t.funcs) {
      bool wrapped = wrappers && ReturnsHresult(f);
      std::vector<std::string> ps;
      for (size_t i = 0; i < f.params.size(); i++) {
        const Param& p = f.params[i];
        std::string pn = ParamName(p, i);
        std::string attrs;
        if (p.flags & PARAMFLAG_FIN) attrs += "in";
        if (p.flags & PARAMFLAG_FOUT) attrs += std::string(attrs.empty() ? "" : ",") + "out";
        if (p.flags & PARAMFLAG_FRETVAL) attrs += std::string(attrs.empty() ? "" : ",") + "retval";
        if (p.flags & PARAMFLAG_FLCID) attrs += std::string(attrs.empty() ? "" : ",") + "lcid";
        ps.push_back((attrs.empty() ? "" : "/*[" + attrs + "]*/ ") + Decl(t, p.type, pn));
      }
      std::string cc = f.callconv == CC_CDECL ? "__cdecl" : "__stdcall";
      o += "      virtual " + CType(t, f.ret) + " " + cc + " " + RawName(f, wrapped) + " (" +
           (ps.empty() ? " " : "\n        " + Join(ps) + " ") + ") = 0;\n";
    }
  }

  const Param* RetvalParam(const Func& f) {
    for (const auto& p : f.params)
      if (p.flags & PARAMFLAG_FRETVAL) return &p;
    return nullptr;
  }

  std::string WrapperName(const Func& f) {
    std::string n = Id(f.name);
    switch (f.invkind) {
    case INVOKE_PROPERTYGET: return g_opt.highGet + n;
    case INVOKE_PROPERTYPUT: return g_opt.highPut + n;
    case INVOKE_PROPERTYPUTREF: return g_opt.highPutRef + n;
    default: return g_opt.highMethodPrefix + n;
    }
  }

  void WriteWrapper(std::string& o, const TypeInfo& t, const std::string& cls, const Func& f,
                    std::vector<std::string>& impl) {
    const Param* rv = RetvalParam(f);
    HighType h;
    TD rvType;
    if (rv && rv->type->vt == VT_PTR) {
      rvType = rv->type->inner;
      h = High(t, rvType);
    } else {
      h.type = "HRESULT";
      h.kind = Kind::Plain;
    }
    std::vector<std::string> decls, defs, call;
    std::vector<bool> withDefault = TrailingDefaults(t, f, rv);
    for (size_t i = 0; i < f.params.size(); i++) {
      const Param& p = f.params[i];
      if (&p == rv) {
        call.push_back("&_result");
        continue;
      }
      std::string pn = ParamName(p, i);
      decls.push_back(HighParam(t, p, pn, withDefault[i]));
      defs.push_back(HighParam(t, p, pn, false));
      call.push_back(pn);
    }
    std::string wname = WrapperName(f);
    o += "    " + h.type + " " + wname + " ( " + Join(decls) + " );\n";
    std::string s = "inline " + h.type + " " + cls + "::" + wname + " ( " + Join(defs) + " ) {\n";
    if (rv && rvType) {
      int vt = rvType->vt == VT_VARIANT ? VT_VARIANT : VT_EMPTY;
      if (vt == VT_VARIANT)
        s += "    VARIANT _result;\n    VariantInit(&_result);\n";
      else if (h.kind == Kind::Iface)
        s += "    " + CType(t, rvType) + " _result = 0;\n";
      else {
        TD u = Underlying(t, rvType);
        bool aggregate = u->vt == VT_DECIMAL || u->vt == VT_CY || u->vt == VT_CARRAY ||
                         (u->vt == VT_USERDEFINED && UserKind(t, rvType) != TKIND_ENUM);
        std::string c = CType(t, rvType);
        s += "    " + c + " _result" + (aggregate ? "" : " = (" + c + ")0") + ";\n";
        if (aggregate) s += "    memset(&_result, 0, sizeof(_result));\n";
      }
    }
    s += "    HRESULT _hr = " + RawName(f, true) + "(" + Join(call) + ");\n";
    s += "    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));\n";
    if (rv && rvType)
      s += ResultReturn(h);
    else
      s += "    return _hr;\n";
    s += "}\n\n";
    impl.push_back(s);
  }

  // ------------------------------------------------------------------

  std::string UuidSpec(const TypeInfo& t) {
    return t.hasGuid ? "_TLH_UUID(\"" + t.guid.Text() + "\") " : "";
  }

  // The order the types are defined in: what a type uses by value, and
  // its base interface, first.
  std::vector<const TypeInfo*> DefinitionOrder() {
    std::vector<const TypeInfo*> out;
    std::set<const TypeInfo*> done, busy;
    std::function<void(const TypeInfo*)> visit = [&](const TypeInfo* t) {
      if (done.count(t) || busy.count(t)) return;
      busy.insert(t);
      auto dep = [&](const TD& td) {
        TD x = td;
        while (x && (x->vt == VT_CARRAY)) x = x->inner;
        if (x && x->vt == VT_USERDEFINED) {
          Resolved r = Res(*t, x->href);
          if (r.local && r.ti && std::find(order.begin(), order.end(), r.ti) != order.end())
            visit(r.ti);
        }
      };
      switch (t->kind) {
      case TKIND_RECORD:
      case TKIND_UNION:
        for (const auto& v : t->vars) dep(v.type);
        break;
      case TKIND_ALIAS: dep(t->alias); break;
      case TKIND_INTERFACE:
      case TKIND_DISPATCH: {
        for (const auto& i : t->impls) {
          Resolved r = Res(*t, i.href);
          if (r.local && r.ti && std::find(order.begin(), order.end(), r.ti) != order.end())
            visit(r.ti);
        }
        // The aliases the methods use, which must be declared before them
        // (or a type of the same name in the Windows headers is used).
        auto aliasDep = [&](const TD& td) {
          TD x = td;
          while (x && (x->vt == VT_PTR || x->vt == VT_CARRAY || x->vt == VT_SAFEARRAY)) x = x->inner;
          if (x && x->vt == VT_USERDEFINED) {
            Resolved r = Res(*t, x->href);
            if (r.local && r.ti && r.ti->kind == TKIND_ALIAS &&
                std::find(order.begin(), order.end(), r.ti) != order.end())
              visit(r.ti);
          }
        };
        for (const auto& f : t->funcs) {
          aliasDep(f.ret);
          for (const auto& p : f.params) aliasDep(p.type);
        }
        for (const auto& v : t->vars) aliasDep(v.type);
        break;
      }
      default: break;
      }
      busy.erase(t);
      done.insert(t);
      out.push_back(t);
    };
    // Enumerations first: anything may use them.
    for (auto* t : order)
      if (t->kind == TKIND_ENUM) visit(t);
    for (auto* t : order) visit(t);
    return out;
  }

  void WriteTlh() {
    std::string& o = tlh;
    std::string guard;
    o += "// " + base + ".tlh - written by tlbimport from " + BaseName(lib.path) +
         ", the type library \"" + lib.name + "\" version " + std::to_string(lib.major) + "." +
         std::to_string(lib.minor) + " (" + lib.guid.Text() + ").\n";
    o += "// It is #import's primary header: do not edit.\n\n";
    o += "#pragma once\n#pragma pack(push, 8)\n\n#include <comdef.h>\n\n";
    o += "#ifndef __has_declspec_attribute\n#define __has_declspec_attribute(x) 0\n#endif\n";
    o += "#ifndef _TLH_UUID\n#if __has_declspec_attribute(uuid)\n#define _TLH_UUID(u) __declspec(uuid(u))\n"
         "#else\n#define _TLH_UUID(u)\n#endif\n#endif\n\n";
    if (g_opt.implementationOnly) {
      // Only the implementations: the declarations come from a header
      // included before.
      o += OpenNs() + "\n";
      o += "#include \"" + base + ".tli\"\n\n" + CloseNs() + "\n#pragma pack(pop)\n";
      return;
    }
    // Forward references and typedefs.
    o += OpenNs();
    o += "//\n// Forward references and typedefs\n//\n\n";
    o += "struct " + UuidSpec2(lib) + "/* LIBID */ __" + SafeIdent(lib.name) + ";\n";
    for (auto* t : order) {
      std::string n = Id(t->name);
      switch (t->kind) {
      case TKIND_INTERFACE:
        o += "struct " + UuidSpec(*t) + "/* interface */ " + n + ";\n";
        break;
      case TKIND_DISPATCH:
        o += "struct " + UuidSpec(*t) + (IsDual(*t) ? "/* dual interface */ " : "/* dispinterface */ ") +
             n + ";\n";
        break;
      case TKIND_COCLASS: o += "struct /* coclass */ " + n + ";\n"; break;
      case TKIND_RECORD: o += "struct " + UuidSpec(*t) + n + ";\n"; break;
      case TKIND_UNION: o += "union " + UuidSpec(*t) + n + ";\n"; break;
      case TKIND_ENUM: break;
      default: break;
      }
    }
    for (auto* t : order)
      if (t->kind == TKIND_ALIAS) {
        // Aliases of enums, records and basic types; written here when
        // what they name is known already.
      }
    o += CloseNs() + "\n";
    // The GUIDs behind mingw-w64's __uuidof. Under MSVC the macro is empty
    // and __declspec(uuid) carries them.
    for (auto* t : order)
      if (t->hasGuid && t->kind != TKIND_ENUM && t->kind != TKIND_ALIAS && t->kind != TKIND_MODULE)
        o += "__CRT_UUID_DECL(" + Qual(Id(t->name)) + ", " + t->guid.Init() + ")\n";
    o += "__CRT_UUID_DECL(" + Qual("__" + SafeIdent(lib.name)) + ", " + lib.guid.Init() + ")\n";
    o += "\n";
    o += OpenNs();
    // Smart pointers.
    if (!g_opt.noSmartPointers) {
      o += "//\n// Smart pointer typedef declarations\n//\n\n";
      for (auto* t : order)
        if (t->kind == TKIND_INTERFACE || t->kind == TKIND_DISPATCH)
          o += "_COM_SMARTPTR_TYPEDEF(" + Id(t->name) + ", __uuidof(" + Id(t->name) + "));\n";
      o += "\n";
    }
    // Injected statements for types.
    // Type library items.
    o += "//\n// Type library items\n//\n\n";
    std::vector<std::string> impls;
    for (auto* t : DefinitionOrder()) WriteType(o, *t, impls);
    // Named GUIDs.
    if (g_opt.namedGuids) {
      o += "//\n// Named GUID constants initializations\n//\n\n";
      o += "extern \"C\" const GUID __declspec(selectany) LIBID_" + SafeIdent(lib.name) + " =\n    {" +
           lib.guid.Init() + "};\n";
      for (auto* t : order) {
        if (!t->hasGuid) continue;
        const char* prefix = t->kind == TKIND_COCLASS ? "CLSID_"
                             : IsDispOnly(*t)         ? "DIID_"
                             : (t->kind == TKIND_INTERFACE || IsDual(*t)) ? "IID_"
                                                                          : nullptr;
        if (!prefix) continue;
        o += "extern \"C\" const GUID __declspec(selectany) " + std::string(prefix) + Id(t->name) +
             " =\n    {" + t->guid.Init() + "};\n";
      }
      o += "\n";
    }
    if (!g_opt.noImplementation) {
      o += "//\n// Wrapper method implementations\n//\n\n";
      o += "#include \"" + base + ".tli\"\n\n";
    }
    o += CloseNs() + "\n#pragma pack(pop)\n";
    implementations = impls;
  }
  std::vector<std::string> implementations;

  std::string UuidSpec2(const Lib& l) { return "_TLH_UUID(\"" + l.guid.Text() + "\") "; }
  static std::string SafeIdent(const std::string& s) {
    std::string r;
    for (char c : s) r += isalnum((unsigned char)c) ? c : '_';
    return r;
  }
  std::string OpenNs() { return ns.empty() ? "" : "namespace " + ns + " {\n\n"; }
  std::string CloseNs() { return ns.empty() ? "" : "} // namespace " + ns + "\n"; }
  std::string Qual(const std::string& n) { return ns.empty() ? "::" + n : ns + "::" + n; }

  void Injects(std::string& o, const std::string& type) {
    for (const auto& s : g_opt.injects) {
      size_t sep = s.find('\x01');
      if (s.substr(0, sep) == type) o += "    " + s.substr(sep + 1) + "\n";
    }
  }

  void WriteType(std::string& o, const TypeInfo& t, std::vector<std::string>& impls) {
    std::string n = Id(t.name);
    if (!t.doc.empty()) o += "// " + Comment(t.doc) + "\n";
    switch (t.kind) {
    case TKIND_ENUM: {
      o += "enum " + n + "\n{\n";
      for (size_t i = 0; i < t.vars.size(); i++) {
        const Var& v = t.vars[i];
        std::string val = v.value.valid ? std::to_string(v.value.i) : "0";
        o += "    " + Id(v.name) + " = " + val + (i + 1 < t.vars.size() ? "," : "") + "\n";
      }
      o += "};\n\n";
      break;
    }
    case TKIND_RECORD:
    case TKIND_UNION: {
      bool rec = t.kind == TKIND_RECORD;
      o += "#pragma pack(push, " + std::to_string(t.align > 0 ? t.align : 4) + ")\n\n";
      o += std::string(rec ? "struct " : "union ") + UuidSpec(t) + n + "\n{\n";
      for (const auto& v : t.vars) o += "    " + Decl(t, v.type, Id(v.name)) + ";\n";
      Injects(o, t.name);
      o += "};\n\n#pragma pack(pop)\n\n";
      break;
    }
    case TKIND_ALIAS: {
      o += "typedef " + Decl(t, t.alias, n) + ";\n\n";
      break;
    }
    case TKIND_MODULE: {
      // A module's constants; its functions live in a DLL.
      bool any = false;
      for (const auto& v : t.vars)
        if (v.varkind == VAR_CONST && v.value.valid) {
          if (!any) o += "// module " + n + " (" + t.dll + ")\n";
          any = true;
          if (v.value.vt == VT_BSTR)
            o += "const LPCWSTR " + Id(v.name) + " = L\"" + v.value.s + "\";\n";
          else if (v.value.vt == VT_R4 || v.value.vt == VT_R8)
            o += "const double " + Id(v.name) + " = " + std::to_string(v.value.d) + ";\n";
          else
            o += "const long " + Id(v.name) + " = " + std::to_string(v.value.i) + ";\n";
        }
      if (any) o += "\n";
      break;
    }
    case TKIND_COCLASS: {
      o += "struct " + UuidSpec(t) + n + ";\n";
      for (const auto& i : t.impls) {
        Resolved r = Res(t, i.href);
        std::string attrs;
        if (i.flags & IMPLTYPEFLAG_FDEFAULT) attrs += "default";
        if (i.flags & IMPLTYPEFLAG_FSOURCE) attrs += std::string(attrs.empty() ? "" : ", ") + "source";
        if (i.flags & IMPLTYPEFLAG_FRESTRICTED) attrs += std::string(attrs.empty() ? "" : ", ") + "restricted";
        o += "    // " + (attrs.empty() ? "" : "[ " + attrs + " ] ") +
             (r.kind == TKIND_DISPATCH && !(r.ti && IsDual(*r.ti)) ? "dispinterface " : "interface ") +
             RefName(r) + "\n";
      }
      o += "\n";
      break;
    }
    case TKIND_INTERFACE:
    case TKIND_DISPATCH: {
      std::string baseName = "IUnknown";
      if (!t.impls.empty()) baseName = RefName(Res(t, t.impls[0].href));
      if (IsDispOnly(t)) baseName = "IDispatch";
      o += "struct " + UuidSpec(t) + n + " : " + baseName + "\n{\n";
      std::vector<std::string> mine;
      if (IsDispOnly(t)) {
        if (!g_opt.rawDispinterfaces) WriteDispMembers(o, t, mine);
      } else {
        WriteVtblMembers(o, t, mine);
      }
      Injects(o, t.name);
      o += "};\n\n";
      if (!mine.empty()) {
        std::string head = "//\n// " + std::string(IsDispOnly(t) ? "dispinterface " : "interface ") +
                           n + " wrapper method implementations\n//\n\n";
        impls.push_back(head);
        impls.insert(impls.end(), mine.begin(), mine.end());
      }
      break;
    }
    default: break;
    }
  }

  void WriteTli() {
    tli = "// " + base + ".tli - written by tlbimport from " + BaseName(lib.path) +
          ": the implementations of the wrapper methods " + base + ".tlh declares.\n"
          "// It is #import's secondary header: do not edit.\n\n#pragma once\n\n";
    for (const auto& s : implementations) tli += s;
  }
};

std::string g_outDir = ".";

bool WriteText(const std::string& path, const std::string& text) {
  // Unchanged files keep their dates, so builds do not redo work.
  std::vector<uint8_t> old = ReadFile(path);
  if (old.size() == text.size() && memcmp(old.data(), text.data(), text.size()) == 0) return true;
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) return false;
  fwrite(text.data(), 1, text.size(), f);
  return fclose(f) == 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string input, baseName;
  bool printTlh = false;
  int tlbid = 0;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "-o" && i + 1 < argc) g_outDir = argv[++i];
    else if (a == "-L" && i + 1 < argc) g_searchDirs.push_back(argv[++i]);
    else if (a.compare(0, 2, "-L") == 0 && a.size() > 2) g_searchDirs.push_back(a.substr(2));
    else if (a == "-n" && i + 1 < argc) baseName = argv[++i];
    else if (a == "--print-tlh") printTlh = true;
    else if (a == "-h" || a == "--help") {
      printf("usage: tlbimport [-o DIR] [-L DIR] [-n NAME] [--print-tlh] [attributes] file[\\N]\n");
      return 0;
    } else if (input.empty() && (a.find('(') == std::string::npos) &&
               (a.find('.') != std::string::npos || a.find('/') != std::string::npos ||
                a.find('\\') != std::string::npos))
      input = a;
    else
      ParseAttribute(a, tlbid);
  }
  if (input.empty()) Fail("no type library given");
  // file.dll\2: the second TYPELIB resource.
  int resId = tlbid;
  size_t bs = input.find_last_of('\\');
  if (bs != std::string::npos && bs + 1 < input.size() &&
      input.find_first_not_of("0123456789", bs + 1) == std::string::npos) {
    resId = atoi(input.c_str() + bs + 1);
    input = input.substr(0, bs);
  }
  Lib* lib = LoadLib(input, resId, true);
  if (baseName.empty()) {
    baseName = BaseName(input);
    size_t dot = baseName.find_last_of('.');
    if (dot != std::string::npos) baseName = baseName.substr(0, dot);
  }
  Writer w(*lib, baseName);
  w.Run();
  std::string tlhPath = g_outDir + "/" + baseName + ".tlh";
  std::string tliPath = g_outDir + "/" + baseName + ".tli";
  if (!WriteText(tlhPath, w.tlh)) Fail("cannot write %s", tlhPath);
  if (!g_opt.noImplementation && !WriteText(tliPath, w.tli)) Fail("cannot write %s", tliPath);
  if (printTlh) printf("%s\n", tlhPath.c_str());
  return 0;
}
