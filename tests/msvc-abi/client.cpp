/* The program side of the ABI test. clang compiles it against msabi.dll
 * from Visual C++, and Visual C++ compiles it against the msabi.dll that
 * clang makes. It prints one line for each check and returns the number of
 * checks that failed. */
#include "abi.h"

#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <typeinfo>

static int failures = 0;

/* Prints a check and counts it if it failed. */
static void check(const char *what, bool ok) {
  printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok)
    failures++;
}

/* Prints a check of two integers. */
static void check_int(const char *what, long got, long want) {
  if (got != want)
    printf("     %s: got %ld, want %ld\n", what, got, want);
  check(what, got == want);
}

/* A Shape from this side of the boundary. */
class Square : public Shape {
public:
  /* Makes a square. */
  Square(int side) : side_(side) {}
  /* Returns the area, or throws for a negative side. */
  int area() const {
    if (side_ < 0)
      throw AbiError(side_, "square");
    return side_ * side_;
  }
  /* Returns the name. */
  const char *name() const { return "square"; }

private:
  int side_;
};

/* Doubles an integer, for abi_call. */
static int twice(int x) { return 2 * x; }

/* Throws an AbiError from this side, for abi_catch_call. */
static void throw_here(int code) {
  if (code < 0)
    throw AbiRangeError(code);
  throw AbiError(code, "client");
}

/* Throws an int from this side, for abi_catch_int. */
static void throw_int() { throw 42; }

/* Runs every check. */
int main() {
  // Names and calling conventions.
  check_int("mangle: integer and floating types",
            abi_mangle((signed char)-1, (unsigned char)2, (short)-3,
                       (unsigned short)4, -5L, 6UL, (__int64)7,
                       (unsigned __int64)8, 1.5f, 2.5, true),
            23);
  char buf[] = "abcd";
  int k = 5, l = 6;
  check_int("mangle: pointers, references, cv", abi_mangle("xy", buf, k, &l),
            17);
  check_int("function pointer", abi_call(twice, 21), 42);
  check_int("__stdcall", abi_stdcall(50, 8), 42);
  check_int("__fastcall", abi_fastcall(4, 2), 402);
  AbiPair<int> p = {1, 2};
  AbiPair<double> q = {3.0, 4.0};
  check_int("template by value", abi_pair(p, &q), 10);
  check_int("enum", abi_color(Blue), 26);

  // Classes, virtual functions and multiple inheritance.
  Rect r(6, 7);
  check_int("virtual call into the library", r.area(), 42);
  r.scale(2);
  check_int("non-inline virtual", r.area(), 168);
  Named *n = &r;
  check("second base's virtual", strcmp(n->label(), "rect") == 0);
  check_int("member function pointer", abi_call_getter(r, abi_getter(false)),
            14);
  RectGetter g = &Rect::width;
  check_int("member function pointer, other way", abi_call_getter(r, g), 12);
  check_int("data member pointer", r.*abi_field(false), 14);

  Square sq(9);
  check_int("library calls our virtual", abi_catch_area(&sq), 81);
  check("library calls our name()", strcmp(sq.name(), "square") == 0);
  int before = Shape::count();
  Shape *made = abi_new_rect(3, 4);
  check_int("static data member", Shape::count(), before + 1);
  check_int("object made by the library", made->area(), 12);
  delete made;
  check_int("our delete of the library's object", Shape::count(), before);
  abi_delete(new Square(2));
  check_int("library's delete of our object", Shape::count(), before);
  Rect *arr = new Rect[3];
  abi_delete_rects(arr);
  check_int("library's delete[] of our new[]", Shape::count(), before);
  arr = abi_new_rects(4);
  check_int("library's new[]", arr[3].area(), 6);
  delete[] arr;
  check_int("our delete[] of the library's new[]", Shape::count(), before);

  VDiamond d;
  check_int("virtual base", abi_diamond(&d), 1111 + 10000 + 100000);
  VRight *vr = &d;
  check_int("virtual base from our side", vr->vb + vr->vr, 101);

  // RTTI.
  check("typeid of our class in the library",
        strcmp(abi_type_name(&sq), "class Square") == 0);
  check("typeid of the library's class",
        strcmp(typeid(r).name(), "class Rect") == 0);
  check("dynamic_cast in the library", abi_as_rect(&r) == &r);
  check("dynamic_cast of our class fails", abi_as_rect(&sq) == 0);
  check("cross cast in the library", abi_as_named(&r) == n);
  Shape *sp = &r;
  check("our dynamic_cast of the library's class",
        dynamic_cast<Rect *>(sp) == &r);
  check("our cross cast", dynamic_cast<Named *>(sp) == n);

  // Exceptions in both directions.
  int got = 0;
  try {
    abi_throw(7);
  } catch (const AbiError &e) {
    got = e.code();
  }
  check_int("catch the library's exception", got, 7);
  got = 0;
  try {
    abi_throw(-3);
  } catch (AbiRangeError &e) {
    got = e.code();
  } catch (...) {
    got = 99;
  }
  check_int("catch the library's derived exception", got, -3);
  got = 0;
  try {
    abi_throw(-4);
  } catch (AbiError &e) {
    got = e.code();
  }
  check_int("catch derived by base", got, -4);
  Square bad(-5);
  check_int("library catches our throw through its call", abi_catch_area(&bad),
            -1);
  check_int("library catches our exception", abi_catch_call(throw_here, 11),
            11);
  check_int("library catches our derived exception",
            abi_catch_call(throw_here, -11), -12);
  check_int("library catches our int", abi_catch_int(throw_int), 1042);
  got = 0;
  try {
    try {
      abi_throw(5);
    } catch (AbiError &) {
      got = 1;
      throw;
    }
  } catch (AbiError &e) {
    got += e.code();
  }
  check_int("rethrow", got, 6);

  // The C++ library.
  std::vector<std::string> parts;
  parts.push_back("one");
  parts.push_back("two");
  parts.push_back("three");
  std::string joined = abi_join(parts, ", ");
  check("std::string and std::vector<std::string>",
        joined == "one, two, three");
  std::vector<int> v;
  for (int i = 1; i <= 10; i++)
    v.push_back(i);
  check_int("std::vector<int>", abi_sum(v), 55);
  std::wstring w = abi_wide(L"hi");
  check("std::wstring", w == L"w:hi");
  got = 0;
  try {
    abi_string_at(joined, 100);
  } catch (std::out_of_range &) {
    got = 1;
  } catch (std::exception &) {
    got = 2;
  }
  check_int("std::out_of_range from the C++ library", got, 1);

  printf("%d failed\n", failures);
  return failures;
}
