# C++ with the Visual C++ ABI

This page tells how to build C++ programs and DLLs with clang that link
with C++ code from old Microsoft Visual C++ compilers. The code can be
`.lib` and `.obj` files, DLLs, MFC 4.2 (`mfc42.dll`) and the C++ library of
Visual C++ 6.0 (`msvcp60.dll`).

The mingw-w64 sysroots use the GNU C++ ABI. Code from Visual C++ uses the
Microsoft C++ ABI. The two ABIs do not link together. For Visual C++ code,
use a C++ kit that `tools/import-ms-sdk.py` makes from your own copy of
Microsoft's kits.

## The C++ kits

| Kit | Folder | Triple | ABI of | Runtime DLLs |
| --- | --- | --- | --- | --- |
| `w2k-vc6` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.0-msvc` | Visual C++ 6.0 | `msvcrt.dll`, `msvcp60.dll`, `mfc42.dll` |
| `xp-vc6` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.1-msvc` | Visual C++ 6.0 | `msvcrt.dll`, `msvcp60.dll`, `mfc42.dll` |
| `ws03-vc6` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.2-msvc` | Visual C++ 6.0 | `msvcrt.dll`, `msvcp60.dll`, `mfc42.dll` |
| `vc40` | `Visual C++ 4.0` | `i486-pc-windows4.0-msvc` | Visual C++ 4.0 | `msvcrt40.dll`, `mfc40.dll` |
| `vc42` | `Visual C++ 4.2` | `i486-pc-windows4.0-msvc` | Visual C++ 4.2 | `msvcrt.dll`, `mfc42.dll` |
| `vc50` | `Visual C++ 5.0` | `i486-pc-windows4.0-msvc` | Visual C++ 5.0 | `msvcrt.dll`, `msvcp50.dll`, `mfc42.dll` |
| `vc60` | `Visual C++ 6.0` | `i586-pc-windows4.0-msvc` | Visual C++ 6.0 | `msvcrt.dll`, `msvcp60.dll`, `mfc42.dll` |
| `vc71` | `Visual C++ .NET 2003` | `i586-pc-windows4.0-msvc` | Visual C++ .NET 2003 | `msvcr71.dll`, `msvcp71.dll`, `mfc71.dll` |

The three DDK kits use the Windows Server 2003 SP1 DDK. This DDK has the
C++ library headers of Visual C++ 6.0, the import libraries of
`msvcp60.dll` and `mfc42.dll`, the MFC 4.2 and ATL 3.0 headers, and
`cl.exe` 13.10, the compiler of Visual C++ .NET 2003.

The `vc*` kits use an installed Visual C++. The folder holds what setup
installed: `MSDEV` for 4.0 and 4.2, `VC` for 5.0, `VC98` for 6.0, and `Vc7`
for 2003. These kits are not tested with real copies yet.

To make the kits:

```sh
python3 tools/import-ms-sdk.py --sdks /path/to/WindowsSDKs ~/clang-dos xp-vc6
python3 tools/import-ms-sdk.py --kit-dir "vc60=/path/to/Microsoft Visual Studio" ~/clang-dos vc60
```

To build programs and DLLs:

```sh
clang++ --config=ms-xp-vc6.cfg -O2 app.cpp vendor.lib -o app.exe
clang++ --config=ms-xp-vc6.cfg -O2 -shared lib.cpp -o lib.dll -Wl,-implib:lib.lib
clang++ --config=ms-xp-vc6.cfg -O2 -D_AFXDLL mfcapp.cpp -o mfcapp.exe
```

Programs that use MFC define `_AFXDLL` to use `mfc42.dll`, as `cl /MD`
does. `afx.h` names the MFC libraries to link.

## What the config file does

* `-fms-compatibility-version` names the compiler of the kit (`12.00` for
  Visual C++ 6.0). This sets `_MSC_VER` and the ABI that clang uses. See
  "The ABI of each Visual C++" below.
* `-fms-runtime-lib=dll` links the C runtime DLL and defines `_DLL` and
  `_MT`, as `cl /MD` does. The headers then ask for the C++ library DLL
  (`msvcprt.lib`) when a program includes them.
* `-Xclang -fno-wchar` makes `wchar_t` a typedef of `unsigned short`, as
  `cl /Zc:wchar_t-` does. Visual C++ 6.0 and before had only this kind.
  Visual C++ 2002 and 2003 used it by default. The C++ library DLL and MFC
  export their wide-character functions with this type.
* The config files of the DDK kits link `bufferoverflowu.lib`, because the
  DDK's C runtime library is built with `/GS`.

## The ABI of each Visual C++

clang changes these parts of the ABI for the version that
`-fms-compatibility-version` gives. All other parts (names of non-template
functions, class layout, virtual functions, virtual bases, member pointers,
RTTI, `throw` and `catch`) are the same from Visual C++ 4.0 to today.

| Part | Up to 6.0 (12.00) | 2002 and 2003 (13.00, 13.10) | 2005 and later (14.00) |
| --- | --- | --- | --- |
| Names of function template specializations | as plain functions: `??6std@@YA...` | with the template arguments: `??$?6D...`, and a link to the plain name | with the template arguments |
| C++ exception handler (32-bit x86) | `__CxxFrameHandler` | `__CxxFrameHandler` | `__CxxFrameHandler3` |
| `FuncInfo` of the exception tables | version `0x19930520` | version `0x19930520` | version `0x19930522` |
| An `extern template` of an inline function | is ignored | is ignored | stops its instantiation |

* Visual C++ 6.0 and before did not put the template arguments of a
  function template in its name. `msvcp60.dll` exports `operator<<` and
  the other function templates of the C++ library by these names.
  `mfc42.dll` exports `ConstructElements<CString>` and the like by them.
  Two specializations with the same parameter and return types get the
  same name, as with those compilers. clang then gives an error.
* Visual C++ 2002 and 2003 put the template arguments in the name. For each
  function template specialization that an object uses but does not
  define, they also write `/alternatename:NEW=OLD` in the object. The
  linker then uses the Visual C++ 6.0 name if no library has the new one.
  Thus these compilers can call `msvcp60.dll` and `mfc42.dll`. clang does
  the same for 13.00 and 13.10.
* `msvcrt.dll`, `msvcr70.dll` and `msvcr71.dll` do not have
  `__CxxFrameHandler3`. Their `__CxxFrameHandler` reads the first version
  of the exception tables. This version has no `ESTypeList` and no
  `EHFlags`. Thus `catch (...)` can also catch a Windows (SEH) exception,
  as with those compilers.
* The C++ library headers of Visual C++ 6.0 declare some inline operators
  of `std::basic_string` with `extern template`. `msvcp60.dll` does not
  export them. Visual C++ ignored these declarations and compiled the
  operators. clang does the same for these versions.

## Fixes to Microsoft's headers

The importer copies the headers. It changes these copies for clang:

* `istream`, `ostream`, `fstream` and `sstream`: the stream templates use
  members of `ios_base` (`in`, `out`, `trunc`, `skipws`, `unitbuf`, `oct`,
  `hex`) without `ios_base::`. Visual C++ found them because it did not
  look up names in two phases. clang does not look in a base class that
  depends on a template argument. Some of these names are also
  manipulator functions in `std`, which clang finds. The copies say
  `ios_base::`.
* `functional`: `typename const T::type&` becomes
  `const typename T::type&`. `bind1st` and `bind2nd` get the `typename`
  that clang needs.
* `afxwin1.inl` (MFC 4.2): two `CMenu` operators have no return type (an
  implicit `int`). The copies say `BOOL`.

## Tests

`tests/msvc-abi/run-test.sh` tests the ABI in Wine against the DDK's
`cl.exe`, which runs in Wine:

```sh
PACKAGE=~/clang-dos DDK="/path/to/5.2.3970.1830 DDK (unpacked)/WINDDK" \
MFC42_DLL=/path/to/mfc42.dll sh tests/msvc-abi/run-test.sh
```

| Test | Library | Program |
| --- | --- | --- |
| `cl-cl` | `abi.cpp`, built by `cl` | `client.cpp`, built by `cl` (the reference) |
| `clang-cl` | `abi.cpp`, built by `cl` | `client.cpp`, built by clang |
| `clang7-cl` | `abi.cpp`, built by `cl` | `client.cpp`, built by clang with `-fms-compatibility-version=13.10` |
| `cl-clang` | `abi.cpp`, built by clang | `client.cpp`, built by `cl` |
| `stl` | `msvcp60.dll` | `stl.cpp`, built by clang |
| `mfc` | `mfc42.dll` | `mfc.cpp`, built by clang |

With the three DDK kits, all six tests pass in Wine 9.0, at `-O2` and
`-O0`: 43 checks in each ABI test, 23 in `stl` and 12 in `mfc`. Set
`CLANG_FLAGS` to give clang more flags, and `KIT` for another kit.

`abi.h` is the interface between the two sides. It has exceptions that
each side throws and the other catches, by type and by base class;
abstract classes that each side implements; multiple and virtual
inheritance; RTTI (`typeid` and `dynamic_cast` across the DLL); member
pointers; `new` and `delete`, and `new[]` and `delete[]`, on different
sides; many types in function names; `__stdcall` and `__fastcall`; and
`std::string`, `std::wstring` and `std::vector` across the DLL. Each
program prints one line for each check and returns the number of checks
that failed.

## Limits

* Visual C++ 6.0 and before had no `operator new[]` and `operator
  delete[]`. The `msvcrt.dll` of Windows 95, 98 and NT 4.0 does not have
  them. The config files of the `vc40` to `vc60` kits link `new[]` and
  `delete[]` to `operator new` and `operator delete` when the C runtime
  library has no array versions. The DDK kits are for Windows 2000 and
  later, whose `msvcrt.dll` has them.

* The `-fno-wchar` flag of the config files makes `wchar_t` a typedef.
  Code that overloads a function on `wchar_t` and `unsigned short` does
  not compile, as with `cl /Zc:wchar_t-`.
* The `vc40` to `vc71` kits are not tested with real copies of Visual
  C++. Some of their headers can need more fixes for clang.
* Only 32-bit x86. The 64-bit DDK kit (`ws03-x64`) is C only.

## 16-bit Windows (Visual C++ 1.5x)

Visual C++ 1.5x for Windows 3.x uses a different C++ ABI. clang does not
support it. Support needs these changes to clang:

* Names: Visual C++ 1.5x puts `__near`, `__far` and `__huge` in the
  names of pointer and reference types, of `this` and of functions. It
  also has `__pascal` and `__far __cdecl` calling conventions in names.
  The memory model sets the default for each.
* Class layout: the size of the virtual function table pointer and of
  `this` depends on the memory model and on `__near` and `__far` classes.
* No RTTI and no C++ exceptions. Visual C++ 1.5x had neither. MFC 2.5
  (`mfc250.dll`) used `setjmp` and `longjmp` for `TRY` and `CATCH`.
* MFC 2.5 and the C++ libraries of Visual C++ 1.5x are OMF libraries for
  each memory model.
