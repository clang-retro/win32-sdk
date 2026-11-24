/* The interface between a library that Visual C++ compiles and a program
 * that clang compiles, and the reverse. The header uses only what
 * Visual C++ 6.0 and 2003 accept. Each item tests one part of the
 * Microsoft C++ ABI: names, class layout, virtual functions, RTTI,
 * exceptions, member pointers, operator new and the C++ library. */
#ifndef ABI_H
#define ABI_H

/* Visual C++ 5.0 (_MSC_VER 1100) brought bool and the standard C++
 * library. Before it, bool is an int and the test has no C++ library
 * checks. */
#if !defined(_MSC_VER) || _MSC_VER >= 1100
#define ABI_HAS_STD 1
#include <string>
#include <vector>
typedef bool abi_bool;
#else
typedef int abi_bool;
#endif

#ifdef ABI_BUILD_DLL
#define ABI_API __declspec(dllexport)
#elif defined(ABI_STATIC)
#define ABI_API
#else
#define ABI_API __declspec(dllimport)
#endif

/* The exception that both sides throw and catch. */
class ABI_API AbiError {
public:
  /* Makes an exception with a code and a message. */
  AbiError(int code, const char *what);
  /* Copies an exception (the thrower and the catcher must agree). */
  AbiError(const AbiError &other);
  /* Destroys the exception. */
  virtual ~AbiError();
  /* Returns the code. */
  int code() const { return code_; }
  /* Returns the message. */
  const char *what() const { return what_; }

private:
  int code_;
  char what_[32];
};

/* A derived exception, for catch by base class. */
class ABI_API AbiRangeError : public AbiError {
public:
  /* Makes a range error with a code. */
  AbiRangeError(int code);
};

/* An abstract interface that the other side implements. */
class ABI_API Shape {
public:
  /* Destroys the shape. */
  virtual ~Shape();
  /* Returns the area. */
  virtual int area() const = 0;
  /* Returns the name. */
  virtual const char *name() const;
  /* Returns how many shapes exist. */
  static int count();

protected:
  /* Counts one more shape. */
  Shape();

private:
  static int count_;
};

/* A shape that the library has, with multiple inheritance. */
class ABI_API Named {
public:
  /* Destroys the object. */
  virtual ~Named();
  /* Returns the label. */
  virtual const char *label() const;
};

/* A rectangle: a Shape and a Named, so its this pointers differ. */
class ABI_API Rect : public Shape, public Named {
public:
  /* Makes a rectangle (1 by 1 for new[]). */
  Rect(int w = 1, int h = 1);
  /* Returns the area. */
  int area() const;
  /* Returns the label. */
  const char *label() const;
  /* Returns the width (for member pointers). */
  int width() const { return w_; }
  /* Returns the height (for member pointers). */
  int height() const { return h_; }
  /* Scales the rectangle. */
  virtual void scale(int by);

  int w_, h_;
};

/* Virtual inheritance: one shared base. */
class ABI_API VBase {
public:
  /* Makes the base with a value. */
  VBase();
  /* Destroys the base. */
  virtual ~VBase();
  int vb;
};

/* The first side of the diamond. */
class ABI_API VLeft : public virtual VBase {
public:
  /* Makes the left side. */
  VLeft();
  int vl;
};

/* The second side of the diamond. */
class ABI_API VRight : public virtual VBase {
public:
  /* Makes the right side. */
  VRight();
  int vr;
};

/* The bottom of the diamond. */
class ABI_API VDiamond : public VLeft, public VRight {
public:
  /* Makes the diamond. */
  VDiamond();
  /* Returns the sum of all members. */
  virtual int sum() const;
  int vd;
};

/* Names with many kinds of type, to test name mangling. */
ABI_API int abi_mangle(signed char, unsigned char, short, unsigned short,
                       long, unsigned long, __int64, unsigned __int64,
                       float, double, abi_bool);
/* A function with pointers, references and const. */
ABI_API int abi_mangle(const char *, char *const, const int &, int *volatile);
/* A function that takes a function pointer. */
ABI_API int abi_call(int (*fn)(int), int arg);
/* A __stdcall function. */
ABI_API int __stdcall abi_stdcall(int a, int b);
/* A __fastcall function. */
ABI_API int __fastcall abi_fastcall(int a, int b);

/* A template that both sides instantiate. */
template <class T> struct AbiPair {
  T first, second;
};
/* A function that takes a template instance by value. */
ABI_API int abi_pair(AbiPair<int> p, AbiPair<double> *q);

/* An enum, for its name and size. */
enum AbiColor { Red, Green = 10, Blue };
/* A function that takes an enum. */
ABI_API int abi_color(AbiColor c);

/* Member pointers across the boundary. */
typedef int (Rect::*RectGetter)() const;
/* Returns a pointer to the member function Rect::width or Rect::height. */
ABI_API RectGetter abi_getter(abi_bool width);
/* Calls a member function pointer on a Rect. */
ABI_API int abi_call_getter(const Rect &r, RectGetter g);
/* Returns a pointer to the data member Rect::w_ or Rect::h_. */
ABI_API int Rect::*abi_field(abi_bool width);

/* Throws an AbiError (or an AbiRangeError when code is negative). */
ABI_API void abi_throw(int code);
/* Calls s->area() in a try block. Returns -1 if the call threw an
 * AbiError, else the area. */
ABI_API int abi_catch_area(const Shape *s);
/* Calls fn, which throws, and catches what it throws by base class.
 * Returns the code. */
ABI_API int abi_catch_call(void (*fn)(int), int code);
/* Calls fn and catches an int. Returns the int + 1000. */
ABI_API int abi_catch_int(void (*fn)());
/* Destroys a shape that the other side made (virtual destructor). */
ABI_API void abi_delete(Shape *s);
/* Makes a Rect with new. The other side deletes it. */
ABI_API Shape *abi_new_rect(int w, int h);
/* Returns the RTTI name of a shape. */
ABI_API const char *abi_type_name(const Shape *s);
/* Returns s cast to Rect with dynamic_cast, or 0. */
ABI_API Rect *abi_as_rect(Shape *s);
/* Returns s cast to Named with a cross cast, or 0. */
ABI_API Named *abi_as_named(Shape *s);
/* Returns d->sum() and reads the virtual base through VLeft and VRight. */
ABI_API int abi_diamond(VDiamond *d);
/* Makes an array with new[]. The other side deletes it with delete[]. */
ABI_API Rect *abi_new_rects(int n);
/* Deletes an array that the other side made with new[]. */
ABI_API void abi_delete_rects(Rect *r);

#ifdef ABI_HAS_STD
/* The C++ library: std::string and std::vector cross the boundary. */
ABI_API std::string abi_join(const std::vector<std::string> &parts,
                             const std::string &sep);
/* Adds the numbers in v. */
ABI_API int abi_sum(const std::vector<int> &v);
/* Returns a wide string. */
ABI_API std::wstring abi_wide(const wchar_t *s);
/* Throws a std::out_of_range from msvcp60.dll's std::string::at. */
ABI_API char abi_string_at(const std::string &s, int i);
#endif

#endif
