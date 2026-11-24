/* The library side of the ABI test. Visual C++ compiles it into
 * msabi.dll. clang compiles it into a DLL of the same name, for the
 * reverse test. */
#define ABI_BUILD_DLL
#include "abi.h"

#include <string.h>
#include <stdexcept>
#include <typeinfo>

/* Makes an exception with a code and a message. */
AbiError::AbiError(int code, const char *what) : code_(code) {
  strncpy(what_, what, sizeof what_ - 1);
  what_[sizeof what_ - 1] = 0;
}

/* Copies an exception. */
AbiError::AbiError(const AbiError &other) : code_(other.code_) {
  memcpy(what_, other.what_, sizeof what_);
}

/* Destroys the exception. */
AbiError::~AbiError() {}

/* Makes a range error with a code. */
AbiRangeError::AbiRangeError(int code) : AbiError(code, "range") {}

int Shape::count_ = 0;

/* Counts one more shape. */
Shape::Shape() { ++count_; }

/* Counts one less shape. */
Shape::~Shape() { --count_; }

/* Returns the default name. */
const char *Shape::name() const { return "shape"; }

/* Returns how many shapes exist. */
int Shape::count() { return count_; }

/* Destroys the object. */
Named::~Named() {}

/* Returns the default label. */
const char *Named::label() const { return "named"; }

/* Makes a rectangle. */
Rect::Rect(int w, int h) : w_(w), h_(h) {}

/* Returns the area. */
int Rect::area() const { return w_ * h_; }

/* Returns the label. */
const char *Rect::label() const { return "rect"; }

/* Scales the rectangle. */
void Rect::scale(int by) {
  w_ *= by;
  h_ *= by;
}

/* Makes the virtual base. */
VBase::VBase() : vb(1) {}
/* Destroys the virtual base. */
VBase::~VBase() {}
/* Makes the left side. */
VLeft::VLeft() : vl(10) {}
/* Makes the right side. */
VRight::VRight() : vr(100) {}
/* Makes the diamond. */
VDiamond::VDiamond() : vd(1000) {}
/* Returns the sum of all members. */
int VDiamond::sum() const { return vb + vl + vr + vd; }

/* Adds all arguments, as integers. */
int abi_mangle(signed char a, unsigned char b, short c, unsigned short d,
               long e, unsigned long f, __int64 g, unsigned __int64 h,
               float i, double j, bool k) {
  return (int)(a + b + c + d + e + f + g + h + i + j + k);
}

/* Adds the lengths and the integers. */
int abi_mangle(const char *a, char *const b, const int &c, int *volatile d) {
  return (int)(strlen(a) + strlen(b)) + c + *d;
}

/* Calls fn with arg. */
int abi_call(int (*fn)(int), int arg) { return fn(arg); }

/* Returns a - b, with the callee removing the arguments. */
int __stdcall abi_stdcall(int a, int b) { return a - b; }

/* Returns a * 100 + b, with the arguments in ECX and EDX. */
int __fastcall abi_fastcall(int a, int b) { return a * 100 + b; }

/* Adds the members of both pairs. */
int abi_pair(AbiPair<int> p, AbiPair<double> *q) {
  return p.first + p.second + (int)(q->first + q->second);
}

/* Returns the value of the enum, times 2. */
int abi_color(AbiColor c) { return (int)c * 2 + (int)sizeof(c); }

/* Returns a pointer to Rect::width or Rect::height. */
RectGetter abi_getter(bool width) {
  return width ? &Rect::width : &Rect::height;
}

/* Calls a member function pointer on a Rect. */
int abi_call_getter(const Rect &r, RectGetter g) { return (r.*g)(); }

/* Returns a pointer to Rect::w_ or Rect::h_. */
int Rect::*abi_field(bool width) { return width ? &Rect::w_ : &Rect::h_; }

/* Throws an AbiError, or an AbiRangeError when code is negative. */
void abi_throw(int code) {
  if (code < 0)
    throw AbiRangeError(code);
  throw AbiError(code, "abi_throw");
}

/* Calls s->area() in a try block; -1 if it threw an AbiError. */
int abi_catch_area(const Shape *s) {
  try {
    return s->area();
  } catch (const AbiError &) {
    return -1;
  }
}

/* Calls fn and catches what it throws by base class. */
int abi_catch_call(void (*fn)(int), int code) {
  try {
    fn(code);
  } catch (AbiRangeError &e) {
    return e.code() - 1;
  } catch (AbiError &e) {
    return e.code();
  }
  return 0;
}

/* Calls fn and catches an int. */
int abi_catch_int(void (*fn)()) {
  try {
    fn();
  } catch (int i) {
    return i + 1000;
  }
  return 0;
}

/* Deletes a shape through its virtual destructor. */
void abi_delete(Shape *s) { delete s; }

/* Makes a Rect with new. */
Shape *abi_new_rect(int w, int h) { return new Rect(w, h); }

/* Returns the RTTI name of a shape. */
const char *abi_type_name(const Shape *s) { return typeid(*s).name(); }

/* Casts s to Rect. */
Rect *abi_as_rect(Shape *s) { return dynamic_cast<Rect *>(s); }

/* Casts s to Named, across the hierarchy. */
Named *abi_as_named(Shape *s) { return dynamic_cast<Named *>(s); }

/* Reads the virtual base through both sides. */
int abi_diamond(VDiamond *d) {
  VLeft *l = d;
  VRight *r = d;
  return d->sum() + l->vb * 10000 + r->vb * 100000;
}

/* Makes an array with new[]: each Rect is i by 2. */
Rect *abi_new_rects(int n) {
  Rect *r = new Rect[n];
  for (int i = 0; i < n; i++)
    r[i] = Rect(i, 2);
  return r;
}

/* Deletes an array that the other side made. */
void abi_delete_rects(Rect *r) { delete[] r; }

/* Joins the strings with a separator. */
std::string abi_join(const std::vector<std::string> &parts,
                     const std::string &sep) {
  std::string out;
  for (std::vector<std::string>::size_type i = 0; i < parts.size(); i++) {
    if (i)
      out += sep;
    out += parts[i];
  }
  return out;
}

/* Adds the numbers in v. */
int abi_sum(const std::vector<int> &v) {
  int n = 0;
  for (std::vector<int>::const_iterator i = v.begin(); i != v.end(); ++i)
    n += *i;
  return n;
}

/* Returns a wide string with a prefix. */
std::wstring abi_wide(const wchar_t *s) { return std::wstring(L"w:") + s; }

/* Returns s.at(i), which throws std::out_of_range past the end. */
char abi_string_at(const std::string &s, int i) { return s.at(i); }
