# libstdc++ (GCC's C++ library)

The Windows sysroots have two C++ standard libraries:

* libc++ (LLVM). This is the default. The config files select it.
* libstdc++ (GCC 15.2.0). Add `-stdlib=libstdc++` to select it.

Use libstdc++ when a program or a library needs GCC's C++ library. For
example, use it for code that uses GCC extensions (`<ext/...>`,
`__gnu_cxx`), or for code that you link with objects made by a MinGW GCC.

```sh
clang++ --config=winxp.cfg -stdlib=libstdc++ -O2 hello.cpp -o hello.exe
clang++ --config=win95.cfg -stdlib=libstdc++ -O2 hello.cpp -o hello.exe
clang++ --config=win10-x64.cfg -stdlib=libstdc++ -O2 hello.cpp -o hello.exe
```

## Where it is

| Sysroot | Windows | Threads |
| --- | --- | --- |
| `win9x-msvcrt` | 95, 98, ME, NT 4.0 | `gthr-win32-compat.c` |
| `win9x-crtdll` | Win32s, NT 3.1, 3.5, 3.51, 95 without msvcrt.dll | `gthr-win32-compat.c` |
| `win2k-xp` | 2000, XP | `gthr-win32-compat.c` |
| `win-vista` | Vista to 11, 32-bit | GCC's win32 thread model |
| `win-vista-x64` | Vista to 11, 64-bit | GCC's win32 thread model |

The `win32-picolibc` sysroot does not have libstdc++.

The files are in the sysroot as a GCC installation, so clang finds them
without more options:

```
include/c++/15.2.0/              the headers
include/c++/15.2.0/mingw32/      the headers for the target
lib/gcc/mingw32/15.2.0/          libstdc++.a, libsupc++.a, libstdc++exp.a,
                                 libstdc++fs.a
share/licenses/libstdc++/        COPYING3, COPYING.RUNTIME
```

## How it is built

`scripts/build-libstdcxx.sh` builds the library. `build-sysroot.sh` runs
it when you set `GCC_SRC` to an unpacked GCC release:

```sh
BIN=~/llvm/build/bin LLVM_SRC=~/llvm MINGW_SRC=~/mingw-w64 \
GCC_SRC=~/gcc-15.2.0 sh build-sysroot.sh win2k-xp out/win2k-xp
```

You can also add the library to a sysroot that you built before:

```sh
BIN=~/llvm/build/bin GCC_SRC=~/gcc-15.2.0 SYSROOT=out/win2k-xp \
ARCH=i686 CPU=i586 sh scripts/build-libstdcxx.sh
```

The script does these steps:

1. It applies `scripts/libstdcxx/gcc-15.2.0.patch` to `GCC_SRC` one time.
   The patch has small changes for clang and for Windows before Vista.
   The top of the patch file gives each change.
2. It runs the `configure` script of `libstdc++-v3` with clang as the
   compiler. GCC does not compile anything. Thus the library has the same
   ABI as the programs that clang compiles.
3. It compiles the thread functions (see below) and puts them in
   `libstdc++.a`.
4. It installs the headers and the libraries in the sysroot.

The library is static. Exceptions use the DWARF unwinder of the sysroot
(LLVM libunwind, which is also `libgcc_eh.a`) on 32-bit Windows, and SEH
on 64-bit Windows.

## Threads

GCC's thread functions for Windows (`libgcc/config/i386/gthr-win32*.c`)
need Windows Vista. They use condition variables, `GetThreadId` and
`TryEnterCriticalSection`.

For the sysroots below Vista, `scripts/libstdcxx/gthr-win32-compat.c`
replaces them. It works on every 32-bit Windows from 95 and NT 3.1. With
it, these work before Vista:

* `std::thread`, `std::jthread`, `std::this_thread`
* `std::mutex`, `std::recursive_mutex`, `std::timed_mutex`,
  `std::shared_mutex`, `std::call_once`
* `std::condition_variable`, `std::future`, `std::async`

The replacement uses the same design as the libc++ threads of clang-retro:

* A condition variable is a queue of waiting threads. Each thread has an
  event. A signal wakes one thread that was waiting when the signal came.
* For 95, 98, ME and NT 4.0, a mutex is a counter and a semaphore. These
  versions do not have `TryEnterCriticalSection`.
* A thread is the handle that `CreateThread` gives.

Note: A `std::thread::id` is a thread handle. After a thread is joined,
Windows can give the same handle value to a different thread. Thus, a
thread that gets its ID for the first time after a join can get the ID
of the joined thread. Get the ID of a thread before you join other
threads if you compare the IDs later. GCC's own thread model has the same
behaviour.

## Changes for Windows before XP

For the sysroots below XP (`_WIN32_WINNT` below 0x0501), the patch removes
calls that these versions do not have:

* `std::random_device` does not use `rand_s`. mingw-w64's `rand_s` calls
  `LoadLibraryW` and `RtlGenRandom`, which are in XP and later.
  `std::random_device` uses the `rdrand` instruction if the processor
  has it. If not, it uses `std::mt19937`.
* `thread_local` destructors do not use `GetModuleHandleExW` to keep a
  DLL loaded until they run.
* `std::chrono::system_clock` finds `GetSystemTimeAsFileTime` at run time.
  NT 3.1 and Win32s do not have it.
* `std::ofstream` and `std::ifstream` with a wide file name find `_wfopen`
  at run time on NT. On 95, 98 and ME, they change the name to the ANSI
  code page and call `fopen`. The `_wfopen` of 95, 98 and ME is a stub.

Before Vista, programs use emulated TLS (`-femulated-tls`, from the
triples). libstdc++ itself does not use `thread_local` there: it keeps its
exception data with `TlsAlloc`. This data must stay available after
compiler-rt frees the emulated TLS at exit, when the standard streams are
flushed.

## Limits

* `std::filesystem` calls the wide C library and the W file functions.
  Use it only on Windows NT. The `win9x-crtdll` sysroot has no wide C
  library, so `std::filesystem` and the stream constructors that take a
  `std::filesystem::path` are not available there.
* The library is static. There is no `libstdc++-6.dll`.
* A program can use libc++ or libstdc++, not the two together.
* libstdc++ uses the GNU (Itanium) C++ ABI. It cannot link with C++ code
  that Visual C++ compiled. See the next section.

## Visual C++ (MSVC ABI) interoperation

libstdc++ and libc++ in these sysroots use the GNU C++ ABI of MinGW.
Visual C++ uses the Microsoft C++ ABI. The two ABIs have different name
mangling, different exception handling, different RTTI and different
layouts of the standard library types. Thus, C++ objects from the two
compilers cannot call each other directly.

These are the ways to connect GNU ABI code and Visual C++ code:

1. **A C interface.** Declare the functions at the boundary with
   `extern "C"` and use only C types. Both ABIs use the same C calling
   conventions (`__cdecl`, `__stdcall`, `__fastcall`). This works with
   libstdc++ and with libc++.
2. **COM-style interfaces.** A class that has only pure virtual
   functions, single inheritance and `__stdcall` methods has the same
   vtable layout in the two ABIs. This is how COM works with MinGW. Do not
   let exceptions or standard library types (`std::string`,
   `std::vector`) go across the boundary.
3. **Compile everything for the MSVC ABI.** clang can compile for the
   Microsoft C++ ABI with the `-msvc` triples (for example
   `i686-pc-windows-msvc`, or the triples of `tools/import-ms-sdk.py`).
   Use the C and C++ libraries of Visual C++ (or libc++ built for the MSVC
   ABI) with it. libstdc++ cannot be built for the MSVC ABI, because it
   needs the GNU ABI's exception and RTTI support (`libsupc++`).

## Licence

libstdc++ is under the GPL 3 with the GCC Runtime Library Exception
(`share/licenses/libstdc++`). A program that clang compiles and links with
libstdc++ can have any licence: clang is not a work based on GCC, so the
compilation is an "Eligible Compilation Process". If you pass on the
sysroot, also make available the source of the library: the GCC 15.2.0
release and `scripts/libstdcxx/` of this repository.
