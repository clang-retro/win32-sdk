# win32-sdk

The 32 and 64-bit Windows part of the clang-dos SDK: scripts that build
mingw-w64 sysroots for clang from [clang-dos/llvm](https://github.com/clang-dos/llvm)
that run on Win32s, Windows NT 3.1 to 4.0, Windows 95, 98 and ME, 2000 and
XP, and Vista to 11, the `--config` files for each, an import checker and
the tests. It was split out of [clang-dos/dos-sdk](https://github.com/clang-dos/dos-sdk)
(its `win9x/` directory and the Windows parts of `packaging/`), which still
builds the toolchain packages and takes these sysroots from here.

```
build-sysroot.sh                 builds one of the package's sysroots (below)
scripts/build-win9x.sh           mingw-w64 sysroot for 9x, NT 3.x/4.0, 2000/XP
scripts/build-unicows.sh         adds libunicows to it
scripts/build-cxx.sh             adds libc++, libc++abi and libunwind (9x to XP)
scripts/build-modern-sysroot.sh  sysroot for Vista and later, with libc++
scripts/fix-ddk-headers.sh       fixes mingw-w64's DDK headers in a sysroot
scripts/build-idl.sh             adds the .idl files and stdole2.tlb (below)
idl/stdole2.idl                  the OLE Automation type library's source
tools/midl.c                     midl, a MIDL-compatible front end for widl
cfg/                             --config files for the toolchain package
                                 (*-ddk.cfg for kernel-mode drivers)
tools/check-imports.py           checks a program's or driver's imports for a
                                 Windows version
tools/kernel-versions.txt        which NT kernel has each function, for it
tools/gen-kernel-versions.py     makes kernel-versions.txt from the DDKs
tools/import-ms-sdk.py           sysroots from your own copies of Microsoft's kits
tools/ms_oldlib.py               its fixes for the libraries of 1993-96 kits
tools/ms-intrin-shim.h           its fix for the Server 2003 DDK's winnt.h
tools/msvcrt-versions.txt        which msvcrt.dll has each function
tools/gen-msvcrt-versions.py     makes msvcrt-versions.txt
tests/                           the tests, run in Wine
tests/ddk/                       the test driver and its tests
```

## The package's sysroots

```sh
BIN=~/llvm/build/bin LLVM_SRC=~/llvm MINGW_SRC=~/mingw-w64 \
LIBUNICOWS_SRC=~/libunicows-1.1.2 PICOLIBC_SRC=~/picolibc \
sh build-sysroot.sh NAME OUTPUT
```

| NAME | For | Config files |
| --- | --- | --- |
| `win9x-msvcrt` | 95/98/ME and NT 4.0 (msvcrt.dll), with libunicows and libc++ | `nt4`, `win95`, `win98`, `winme` |
| `win9x-crtdll` | NT 3.x, Win32s and 95 without msvcrt.dll (crtdll.dll), with libc++ | `win32s`, `nt31`, `nt35`, `nt351`, `win95-crtdll` |
| `win2k-xp` | 2000 and XP, with libc++, libc++abi and libunwind | `win2000`, `winxp` |
| `win32-picolibc` | 95/98/ME, NT 3.1 and later, with picolibc as the C library (no C runtime DLL) and libc++ | `win32-picolibc`, `nt31-picolibc` |
| `win-vista` | Vista to 11, 32-bit, with libc++ | `vista`, `win7`, `win8`, `win81`, `win10` |
| `win-vista-x64` | the same, 64-bit | `vista-x64`, `win7-x64`, ... |

dos-sdk's toolchain workflow checks out this repository at `main` (or the
`win32_sdk_ref` it is given), builds these sysroots with
`build-sysroot.sh` and puts them and `cfg/` in the package, whose smoke
test builds a program for each Windows version, checks it with
`check-imports.py` and runs it in Wine.

## Windows 95, 98, ME, NT and 2000/XP

clang builds 32-bit programs for Windows 95, 98 and ME with a Windows
version in the target triple and a sysroot made from mingw-w64.

| Windows | Triple | Default CPU |
| --- | --- | --- |
| 95 (and NT 4.0) | `i386-pc-windows4.0-gnu` | 80386 |
| 98 | `i486-pc-windows4.10-gnu` | 80486 |
| ME | `i586-pc-windows4.90-gnu` | Pentium |

The version in the triple is the oldest Windows the program runs on. clang
then:

* targets the CPU of the arch name (`i386`, `i486`, `i586` for a Pentium,
  `i686` for a Pentium Pro) instead of a Pentium 4, so no SSE. Windows 95 and
  NT 4.0 do not save SSE registers. `-march` still overrides it.
* tells the linker to write that subsystem and OS version. Windows 9x refuses
  programs that ask for Windows 5.0 or later, and LLD's default is 6.0.
  `-Wl,--subsystem,console:4.0` and the like still override it.

### Building the sysroot

```sh
LLVM_SRC=~/llvm MINGW_SRC=~/mingw-w64 BIN=~/llvm/build/bin \
  SYSROOT=~/win9x-sysroot sh scripts/build-win9x.sh
```

Use `llvm-dlltool`, `lld` and `clang` from the same new build. LLVM 18's
`llvm-dlltool` turns renamed imports in mingw-w64's `.def` files into weak
aliases, so `atexit` calls itself and programs hang at startup; a new
`llvm-dlltool` with LLVM 18's `lld` imports functions by ordinal 0.

It needs a recent mingw-w64 (current master works): recent mingw-w64 calls
newer Windows functions only through `GetProcAddress` with fallbacks, and
its own test suite runs on Windows 9x. The sysroot holds:

* mingw-w64 headers with `_WIN32_WINNT` defaulting to `0x0400`, so APIs
  newer than Windows 95 and NT 4.0 are hidden unless a program asks for them;
* the mingw-w64 CRT and import libraries, linked against the `msvcrt.dll`
  that ships with Windows (`libmsvcrt.a` is `libmsvcrt-os.a`);
* compiler-rt builtins, as `libgcc.a` so clang links them by default, and
  empty `libgcc_eh.a`, `crtbegin.o` and `crtend.o`, which C does not need.

Everything is compiled for an 80386 (`CPU=i386`), which Windows 95 and NT
run on. An 80386 has no `BSWAP`, `XADD` or `CMPXCHG`. For `-march=i386`
clang-dos/llvm swaps bytes with shifts and makes atomic writes calls to
`__sync_*` functions in compiler-rt's builtins (`libgcc.a`). On an 80486
or later those use `LOCK XADD` and `LOCK CMPXCHG`; on an 80386 they take a
lock, which costs a `Sleep` only when two threads write atomics at once.
Atomic loads stay plain moves. `CPU=i486` builds libraries that need a
486.

### Building programs

```sh
clang --target=i386-pc-windows4.0-gnu --sysroot=$SYSROOT -fuse-ld=lld \
  hello.c -o hello.exe
clang --target=i386-pc-windows4.0-gnu --sysroot=$SYSROOT -fuse-ld=lld \
  -mwindows gui.c -o gui.exe
```

### IDL, ODL and type libraries

Every Windows sysroot has mingw-w64's `.idl` and `.acf` files in
`include/` and the OLE Automation type libraries `stdole2.tlb` and
`stdole32.tlb` in `lib/` (built from `idl/stdole2.idl`). The toolchain
package's `midl` compiles a project's `.idl` or `.odl` file with Wine's
`widl`, taking Microsoft MIDL's command line:

```sh
midl /win32 /h calc_h.h /iid calc_i.c /tlb calc.tlb calc.idl
midl /mktyplib203 /tlb calc.tlb /h calc_h.h calc.odl   # an MFC ODL file
midl /proxy calc_p.c /dlldata dlldata.c /Oicf calc.idl
```

`midl` runs the `widl` beside it (or `$WIDL`) with the include and
library paths of the sysroot `$MIDL_SYSROOT` (default `win2k-xp`) next to
its `bin/` directory, so `import "oaidl.idl";` and
`importlib("stdole2.tlb");` work. A `.odl` file (or `/mktyplib203`) is
compiled with `oaidl.idl` and `ocidl.idl` imported first, as MKTYPLIB
did. Options widl has no use for, such as `/nologo`, `/robust` and
`/no_robust`, are accepted and ignored. A proxy DLL built from the
`/proxy` and `/dlldata` files is compiled with `-DREGISTER_PROXY_DLL`
(and `-DPROXY_DELEGATION` for proxies of derived interfaces) and linked
with `-lrpcrt4 -loleaut32 -luuid`.

### #import and the COM support library

`tlbimport` (`tools/tlbimport.cpp`, in the toolchain package next to
`midl`) is the type library half of Visual C++'s `#import`: it reads a
type library, either a `.tlb` file or the `TYPELIB` resource of a `.dll`,
`.ocx`, `.exe` or `.olb`, and writes the `.tlh` and `.tli` headers that
`#import` includes, with the same namespace, smart pointers (`IFooPtr`),
`__declspec(property)` members, error-checking wrapper methods and
`raw_` methods. `#import`'s attributes are passed as they are written in
the directive:

```sh
tlbimport -L "$SYSROOT/lib" 'rename("FreeSpace","FreeSpaceX")' scrrun.dll
tlbimport no_namespace named_guids calc.tlb
```

`-o DIR` picks where the headers go and `-L DIR` where the type libraries
it imports (`stdole2.tlb`, ...) are found. Type libraries in the MSFT
format, what MIDL, widl and `ICreateTypeLib2` write, are read; the older
SLTG format is not.

The headers compile with or without `-fms-extensions`. Without it, the
GUIDs behind `__uuidof` come from mingw-w64's `__CRT_UUID_DECL`, and the
`__declspec(property)` members are left out, so properties are reached
through their `GetX` and `PutX` wrappers. The names of Windows API macros
clash as they do with Visual C++, and the same `rename` attributes fix
them (`GetFreeSpace` in `scrrun.dll`, `RGB` in `vbscript.dll`).

The wrappers of dispinterfaces call `_com_dispatch_method` and the other
functions `comdef.h` declares, and failed calls throw `_com_error`
through `_com_issue_errorex`. Every Windows sysroot but `win32-picolibc`
(where `comdef.h` needs C library functions picolibc lacks) has them in
`libcomsupp.a` (also as `libcomsuppw.a`), built from
`comsupp/comsupp.cpp`. `comdef.h` links it, with `oleaut32`, `ole32` and
`uuid`, as Visual C++'s links `comsuppw.lib`: clang honours that with
`-fms-extensions`; otherwise link with `-lcomsupp -loleaut32 -lole32
-luuid`. The sysroots' `comdefsp.h` also defines the smart pointers of
the system interfaces (`IUnknownPtr`, `IDispatchPtr`, ...) for clang, not
only for Visual C++.

### The C runtime DLL

Programs import `msvcrt.dll`. Windows 98 and ME ship it; Windows 95 has it
only when something installed it (Internet Explorer 4, Office, many other
programs). For a bare Windows 95, ship Microsoft's redistributable
`msvcrt.dll` 6.0 with the program, or build a second sysroot with
`CRT=crtdll`, whose programs import `crtdll.dll`, which every Windows 95 has.

### Native ANSI builds

Windows 9x programs are native ANSI builds: they call the `A` functions
directly, with no unicows layer. Windows 95, 98 and ME implement only a
few `W` functions (`MessageBoxW`, `TextOutW`, `lstrlenW`,
`MultiByteToWideChar` and a handful more); the others exist only as stubs
that fail with `ERROR_CALL_NOT_IMPLEMENTED`.

* Generic-text code (`TCHAR`, `_T()`, `CreateFile`, `_tmain`) compiles to
  the `A` functions, because nothing defines `UNICODE` or `_UNICODE`. Don't
  pass `-municode` or `-DUNICODE` for a 9x program, unless it is a Unicode
  build with unicows (below).
* The sysroot's own startup code and C library import no `W` functions;
  `check-imports.py` confirms it for every test program.
* `check-imports.py` fails a build that imports a `W` stub. A program that
  checks the Windows version and picks `W` functions only on NT can pass
  `--allow-unicode` to see those as warnings instead.
* Text is in the system's ANSI code page (`GetACP()`), as in any ANSI
  program.

### Unicode builds with unicows

A Unicode program (`-municode`, `wmain`, `CreateFileW`) runs on 9x through
Microsoft's Layer for Unicode, `unicows.dll`, which turns W calls into A
calls. Native ANSI builds remain the default; this is for code that is
Unicode already.

```sh
LIBUNICOWS_SRC=~/libunicows-1.1.2 BIN=~/llvm/build/bin \
  SYSROOT=~/win9x-sysroot sh scripts/build-unicows.sh
clang --target=i386-pc-windows4.0-gnu --sysroot=$SYSROOT -fuse-ld=lld \
  -municode prog.c -lunicows -o prog.exe
```

`build-unicows.sh` builds [libunicows](https://libunicows.sourceforge.net)
(MIT license; it needs `nasm`) into the sysroot. The program then imports no
W functions. At its first W call, libunicows loads `unicows.dll` on 9x or the
system DLLs on NT, so one exe runs on both.

* Ship `unicows.dll` with the program. Microsoft's MSLU redistributable
  (`unicows.exe`) has it; it is not in this repository. Without it, the
  program stops at its first W call with a message box naming the missing
  DLL.
* [clang-retro/unicows](https://github.com/clang-retro/unicows) (AGPL) is
  an open-source `unicows.dll` with all 506 of MSLU's functions, built with
  this toolchain. It also builds as `opencow.dll`, which libunicows loads
  when `unicows.dll` is absent. A program that ships it has to offer its
  source under the AGPL.
* unicows covers the program's own Win32 calls, not the C library's. The
  9x `msvcrt.dll` calls W stubs directly in about 50 wide functions:
  `_wfopen`, `_wopen`, `_wstat`, `_wfindfirst`, `_waccess`, `_wremove`,
  `_wrename`, `_wmkdir`, `_wgetcwd`, `_wfullpath`, `_wsystem`, `_wspawn*`,
  `_wexec*` and the like. They fail on 9x with or without unicows, and
  `check-imports.py` reports them. Use `CreateFileW` and the other Win32
  functions for files instead. `wmain`'s arguments, `_wgetenv` and the wide
  string, character and formatting functions work, because msvcrt falls
  back to the A functions there.
* A `crtdll.dll` sysroot has no wide C library, so it can't build
  `-municode` programs.

### Windows NT 3.1, 3.5 and 3.51

The same sysroot, built with `CRT=crtdll`, makes programs for Windows NT
3.x with an NT version in the triple. Windows NT 3.x has no `msvcrt.dll`,
but every version has `crtdll.dll`.

| Windows | Triple | Subsystem version |
| --- | --- | --- |
| NT 3.1 | `i386-pc-windows3.10-gnu` | 3.10 |
| NT 3.5 | `i386-pc-windows3.50-gnu` | 3.50 |
| NT 3.51 | `i386-pc-windows3.51-gnu` | 3.51 |

```sh
clang --target=i386-pc-windows3.10-gnu --sysroot=$CRTDLL_SYSROOT \
  -fuse-ld=lld hello.c -o hello.exe
```

* NT 3.x refuses programs with a newer subsystem version than its own
  ("not a valid Windows NT application"); the triple sets it, as for 9x.
  A GUI program with subsystem 3.x looks like a 3.x program on later
  Windows too (no 3D frames from Windows 95 on).
* Write the minor version with two digits: `windows3.1` asks for 3.01,
  which loads too, but `windows3.5` asks for 3.05.
* An `i386` triple targets the 80386, which NT 3.x runs on, like the
  sysroot's libraries.
* LLD puts the import address table in `.rdata`, which is read-only. The
  NT 3.x loader predates the IAT directory that NT 4.0 and later use to make
  the table writable while they fill it in, so for a version before 4.0
  clang tells LLD to make `.rdata` writable.
* `check-imports.py --nt 3.10` (or `3.50`, `3.51`) checks a program for
  NT 3.x: its subsystem version, a writable import address table, no
  `msvcrt.dll`, and no kernel32 function from its list of ones added in
  later versions (`GetSystemTimeAsFileTime` is from 3.5, fibers from 3.51,
  `InterlockedCompareExchange`, `IsDebuggerPresent` and
  `TryEnterCriticalSection` from 4.0, and so on). The list is not complete:
  `--dlls` with DLLs copied from a real NT 3.x checks every import.
* Unicode functions work on NT, so `-municode` programs need no unicows,
  but a crtdll sysroot has no wide C startup (`wmain`).
* Win32s also wants subsystem 3.10, but it is a different system (Win32 on
  Windows 3.1, with no threads and a shared address space) and is not
  tested.

Wine has no NT 3.1 or 3.5 mode; `run-tests.sh` runs NT 3.x programs with
Wine reporting NT 3.51. Wine does not limit programs to the NT 3.x API, and
no NT 3.x loader has run these programs yet.

### Windows 2000 and XP

A sysroot built with `WINNT=0x0500` (or `0x0501`) makes C and C++ programs
for Windows 2000 and XP with an NT 5 version in the triple.

| Windows | Triple | Subsystem version |
| --- | --- | --- |
| 2000 | `i586-pc-windows5.0-gnu` | 5.00 |
| XP | `i586-pc-windows5.1-gnu` | 5.01 |
| Server 2003 | `i586-pc-windows5.2-gnu` | 5.02 |

```sh
LLVM_SRC=~/llvm MINGW_SRC=~/mingw-w64 BIN=~/llvm/build/bin \
  SYSROOT=~/xp-sysroot WINNT=0x0501 CPU=i586 sh scripts/build-win9x.sh
LLVM_SRC=~/llvm BIN=~/llvm/build/bin SYSROOT=~/xp-sysroot CPU=i586 \
  sh scripts/build-cxx.sh
clang --target=i586-pc-windows5.1-gnu --sysroot=$SYSROOT -fuse-ld=lld \
  hello.c -o hello.exe
clang++ --target=i586-pc-windows5.1-gnu --sysroot=$SYSROOT -fuse-ld=lld \
  -stdlib=libc++ hello.cpp -o hello.exe
```

* Programs import the system `msvcrt.dll`, which both versions ship. It
  is older than Vista's: it has no `_l` functions (`_strtod_l`,
  `_snprintf_l`, ...), no `_create_locale` and no `_s` functions such as
  `wcrtomb_s`, and the 2000 one lacks a few XP additions.
  `check-imports.py --nt 5.0` (or `5.1`) checks each `msvcrt.dll` import
  against the version that first shipped it (`msvcrt-versions.txt`, made
  from mingw-w64's annotated list with `gen-msvcrt-versions.py`), and
  each kernel32 import against a list of functions added in XP, Server
  2003, Vista and 7 (SRW locks, condition variables, `InitOnce`, `Fls*`,
  `GetThreadId`, `GetTickCount64`, `GetActiveProcessorCount`, ...).
* Use `i586` (Pentium) triples: 2000 and XP run on a Pentium, and `i686`
  code has CMOV, which a Pentium MMX or K6 lacks. As for 9x, there is no
  SSE unless `-march` asks for it.
* `__thread` and `thread_local` use emulated TLS, as with GCC: the clang
  in clang-dos/llvm uses it by default for MinGW triples below 6.0, since
  before Vista Windows sets up native TLS only for the program and the
  DLLs loaded with it, not for one loaded with `LoadLibrary`.
  `-fno-emulated-tls` turns it off.
* `build-cxx.sh` adds a static libc++, libc++abi and libunwind (DWARF
  unwinding). With the sysroot's `_WIN32_WINNT` below 0x0600, the libc++
  in clang-dos/llvm uses no Vista functions:
  * threads: `std::mutex` is a `CRITICAL_SECTION` and
    `std::condition_variable` a queue of waiting threads with an event
    each; `call_once` is an interlocked flag; thread-specific data uses
    `TlsAlloc` with destructors run from the mingw-w64 runtime's thread
    exit callback; `std::thread::get_id` uses `NtQueryInformationThread`
    when there is no `GetThreadId`; `atomic::wait` polls.
  * locales: a locale is its name, set with `setlocale` for each call that
    takes one. The "C" locale costs a `setlocale(LC_ALL, NULL)` per call.
    A named locale changes the process locale for the call, so
    formatting with a non-"C" locale in two threads at once can race.
  * `<filesystem>`: file information comes from `NtQueryInformationFile`.
    Windows before Vista has no symbolic links to files, so
    `create_symlink` fails with an error and `canonical` does not resolve
    junctions.
* Windows 2000 needs Service Pack 4 for `msvcrt.dll`'s
  `_get_heap_handle`, which XP has only from SP2; nothing in the sysroot
  imports it.
* Wine runs the tests reporting Windows 2000 or XP, but Wine does not limit
  programs to that API, and no real 2000 or XP machine has run them yet.
  `--dlls` with DLLs copied from a real system checks every import.

### C++ on 9x, NT 3.x, NT 4.0 and Win32s

The `win9x-msvcrt` and `win9x-crtdll` sysroots have libc++, libc++abi and
libunwind too, built by `build-cxx.sh` for a 386, and their config files
pass `-stdlib=libc++`:

```sh
clang++ --config=win98.cfg -O2 hello.cpp -o hello.exe
clang++ --config=nt351.cfg -O2 hello.cpp -o hello.exe
```

Their headers are for Windows before 2000 (`_WIN32_WINNT` 0x0400), for
which the libc++ in clang-dos/llvm also does without what 9x, NT 3.x and
NT 4.0 lack. On top of the 2000/XP code (below):

* threads: a mutex is a count of waiting threads and a semaphore, since 9x
  has no `TryEnterCriticalSection`; `this_thread::yield` is `Sleep(0)`. On
  9x, which can't get a thread's ID from its handle, `std::thread` keeps
  the IDs of the threads it makes.
* files: `<filesystem>` and `fstream`s opened with a wide name call the
  Unicode functions on NT, found at run time, so no program imports a 9x
  stub. On 9x and Win32s they convert the name to the ANSI code page (a
  name that doesn't fit is refused) and call the ANSI function. Status
  comes from `GetFileAttributes` and `FindFirstFile`, as 9x can't open a
  directory; `rename` over a file deletes it first; there are no hard
  links. `SetFilePointerEx` and `CreateHardLinkW` (2000) and
  `GetSystemTimeAsFileTime` (NT 3.5) are looked up at run time.
* crtdll.dll (NT 3.x, Win32s, bare 95): no per-thread locales, so a named
  locale changes the process locale for each call, and files seek with
  32-bit positions.
* atomics: a 486 has no `CMPXCHG8B`, so 64-bit ones are calls to the
  compiler-rt atomic functions in `libgcc.a`, which take a lock. The
  others are calls to its `__sync_*` functions, which work on a 386 too
  (see above).

The C runtime is still the system one, `msvcrt.dll` or `crtdll.dll`, with
what it lacks (no `_l` functions, an old `printf`). The `win32-picolibc`
sysroot (below) replaces it.

### picolibc instead of the system C runtime

The `win32-picolibc` sysroot, built by `scripts/build-picolibc.sh`, has
[picolibc](https://github.com/clang-dos/picolibc) (its `libos/win32`
layer) as the C library in place of `msvcrt.dll` and `crtdll.dll`, and
libc++ built on it. Programs link it statically and import only
`kernel32.dll` and the system DLLs they call themselves, so one program
runs on Windows 95, 98 and ME, Windows NT 3.1 and every later version
with nothing to install:

```sh
clang --config=win32-picolibc.cfg -O2 hello.c -o hello.exe
clang++ --config=win32-picolibc.cfg -O2 hello.cpp -o hello.exe
```

`win32-picolibc.cfg` is for Windows 95 and NT 4.0 and later.
`nt31-picolibc.cfg` marks programs for Windows NT 3.1 (subsystem version
3.10), which NT 3.x needs; they run on the later versions too, though 9x
and NT 4.0 then draw their windows in the Windows 3.x style.

What it adds over the system C runtime:

* C99 and C11 `printf` and `scanf` (`%zu`, `%lld`, `%a`, `long double`,
  positional arguments), `long double` math, `<uchar.h>`, a per-thread
  `errno`, and POSIX functions (`getline`, `asprintf`,
  `strndup`, `<dirent.h>`, `clock_gettime`, `nanosleep`, ...).
* For C++: the locale, `wchar_t` and `long double` functions libc++
  expects, so `std::format`, `std::print`, `<charconv>`, iostreams and
  `<filesystem>` take the same paths as on other systems, with 64-bit file
  positions everywhere (crtdll.dll has 32-bit ones).
* `time_t` and `off_t` are 64 bits.
* Files open in text mode (CR LF) by default, as with the Microsoft C
  library; `O_BINARY`, `"b"`, `_fmode` and `setmode` work as there.
  `_wfopen` and `_wopen` use the Unicode functions on NT and the ANSI
  code page on 9x.

The headers are mingw-w64's Windows API headers (`windows.h` and the
rest) with picolibc's C library headers in place of mingw-w64's.
Microsoft-only C runtime headers and functions (`<tchar.h>`, `<conio.h>`,
`<direct.h>`, `_stricmp` and the like) are not there; `<io.h>` and
`<process.h>` have the descriptor and thread functions (`_get_osfhandle`,
`_open_osfhandle`, `_beginthreadex`).

The config file passes `-fno-emulated-tls`: picolibc's `errno` and C++
`thread_local` variables are native TLS. DLLs that a program loads at
startup work on every version; a DLL loaded with `LoadLibrary` gets
native TLS only on Vista and later, so on earlier versions such a DLL
should use another sysroot. As elsewhere, TLS callbacks never run on 9x,
so there `thread_local` destructors do not run when a thread ends.

### What does not work

* PE TLS callbacks never run on 9x. `__thread` variables work in an EXE;
  with clang-dos/llvm's default of emulated TLS for versions before 6.0
  they work in DLLs too. C++ `thread_local` destructors do not run on 9x.

### Checking imports

`check-imports.py` lists the functions a program imports that Windows 9x
lacks or has only as Unicode stubs. Without DLLs it uses a built-in list of
NT-only and newer functions. With `--dlls DIR`, where `DIR` holds one
directory of system DLLs per Windows version (for example `95osr2/` and
`98se/` with `KERNEL32.DLL`, `USER32.DLL`, `GDI32.DLL`, `ADVAPI32.DLL` and
`MSVCRT.DLL` copied from those systems), it checks every import against
the real export tables.

```sh
python3 tools/check-imports.py --dlls ~/win9x-dlls hello.exe
```

### Tests

```sh
BIN=~/llvm/build/bin SYSROOT=~/win9x-sysroot DLLS=~/win9x-dlls \
  sh tests/run-tests.sh
```

(`TARGETS="i586-pc-windows5.0-gnu i586-pc-windows5.1-gnu"` with a 2000/XP
sysroot, `TARGETS="i386-pc-windows3.10-gnu i386-pc-windows3.51-gnu
i386-pc-windows4.0-gnu"` with the crtdll one) builds `tests/hello.c`,
`tests/libc.c`, `tests/tchar.c` (a native ANSI build of generic-text code),
`tests/gui.c` and, when the sysroot has libunicows, `tests/unicode.c` (a
Unicode build with unicows) for 95, 98 and ME at `-O0`, `-O2` and `-Os`,
checks their imports and runs the console tests under Wine when it is
installed, with Wine reporting each target's Windows version. When the
sysroot has libc++, it also builds `tests/cxx.cpp` (iostreams, numbers through the C locale,
exceptions, RTTI), `tests/threads.cpp` (mutexes, condition variables,
`call_once`, thread IDs, `thread_local` destructors, futures,
`atomic::wait`), `tests/fs.cpp` (`<filesystem>`) and `tests/tlsmain.cpp`,
which loads `tests/tlsdll.cpp` with `LoadLibrary` and uses its
`thread_local` variables from several threads, and checks that the import
checker rejects a Vista kernel32 function and a Vista `msvcrt.dll` one. Wine does not limit programs to the
9x API, so a real Windows 9x machine is still the final test.

## Kernel-mode drivers

The `*-ddk.cfg` config files build NT kernel-mode drivers (`.sys` files)
with mingw-w64's DDK headers (`<ntddk.h>`, `<wdm.h>`, `<ntifs.h>`,
`<ndis.h>`, ...), which every sysroot has in `include/ddk`, and its
kernel import libraries (`-lntoskrnl`, `-lhal`, `-lndis`, `-lusbd`,
`-lks`, ...). Like the rest of the sysroots, they hold no Microsoft files.

```sh
clang --config=winxp-ddk.cfg -O2 mydriver.c -o mydriver.sys
clang --config=win10-x64-ddk.cfg -O2 mydriver.c -o mydriver.sys
```

| Config | For | Sysroot |
| --- | --- | --- |
| `nt4-ddk` | NT 4.0 (with 2000's kernel API, see below) | `win9x-msvcrt` |
| `win2000-ddk`, `winxp-ddk` | 2000, XP | `win2k-xp` |
| `vista-ddk`, `win7-ddk`, `win10-ddk` | Vista, 7, 10 (x86) | `win-vista` |
| `vista-x64-ddk`, `win7-x64-ddk`, `win10-x64-ddk` | Vista, 7, 10 and 11 (x64) | `win-vista-x64` |

Each one takes the program config of its Windows version (`@winxp.cfg`)
and adds:

* the DDK headers, with `_WIN32_WINNT` (and so `NTDDI_VERSION`) set to the
  version;
* `-nostdlib`: no C runtime, no start-up code. The kernel exports the C
  library a driver may use (`memcpy`, `strlen`, `_snprintf`, ...), and
  compiler-rt's builtins (`libgcc.a`) supply 64-bit division and
  `__chkstk`;
* the image of a driver: native subsystem, entry point `DriverEntry`,
  base address `0x10000`, the checksum the kernel checks, not Terminal
  Server aware;
* for x86, `-mno-sse -mno-mmx`: a 32-bit driver must not touch the FPU,
  MMX or SSE registers without `KeSaveFloatingPointState`. x64 drivers may
  use SSE, as Microsoft's compiler does.

Structured exception handling works as with Microsoft's compiler:
`__try`, `__except`, `__finally`, `__leave`, `GetExceptionCode()` and
`GetExceptionInformation()`, with `-fms-extensions -fasync-exceptions`
(`cl /EHa`'s behaviour: a bad pointer or a division by zero in any
instruction of a `__try` reaches its `__except`, as drivers expect when
they probe user buffers with `ProbeForRead`). x64 drivers use the
kernel's `__C_specific_handler`. 32-bit ones use `_except_handler3`,
which the 2000 and later kernels export but NT 4.0's and the 98 and ME
NTKERN's do not, so the x86 configs link `libntseh.a`, this SDK's own
`_except_handler3` (`kernel/except3.S`, which needs only `RtlUnwind`
from the kernel); `build-win9x.sh` and `build-modern-sysroot.sh` build
it. The 32-bit configs also pass `-fseh-exceptions`, the exception model
whose tables `__try` needs on MinGW. This needs the clang of clang-retro's
LLVM from November 2026 on: older ones leave 32-bit `__try`'s scope
table undefined and ignore `-fasync-exceptions` for MinGW.

Declare the entry point and dispatch routines `NTAPI` (stdcall), as
mingw-w64's headers do: `NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT,
PUNICODE_STRING)`. The tests are in C; C++ drivers need
`-fno-exceptions -fno-rtti` and get no global constructors, as with
Microsoft's kits.

mingw-w64's headers start at Windows 2000 (`NTDDI_VERSION` 0x05000000),
so `nt4-ddk` declares 2000's kernel API, and some functions it declares
are not in NT 4.0's kernel. `check-imports.py --nt VERSION` checks a
32-bit driver's imports from `ntoskrnl.exe`, `hal.dll`, `ndis.sys`,
`scsiport.sys`, `videoprt.sys`, `tdi.sys`, `win32k.sys` (display
drivers) and the class drivers (`stream.sys`, `ks.sys`, `portcls.sys`,
`usbd.sys`, `hidclass.sys`, `hidparse.sys`, `battc.sys`, `1394bus.sys`,
from the kits that have them) against the exports of each
NT version from 3.1 to Server 2003 (`tools/kernel-versions.txt`, made by
`tools/gen-kernel-versions.py` from the import libraries of the NT 3.1,
NT 4.0, 2000, XP and Server 2003 DDKs; NT 3.5 and 3.51 count as 4.0):

```sh
python3 tools/check-imports.py --nt 4.0 mydriver.sys
```

There is no config file for NT 3.x: mingw-w64's `IoCompleteRequest` and
`IoCallDriver` are the fastcall `IofCompleteRequest` and `IofCallDriver`
of later kernels, which NT 3.1 does not have. Microsoft's NT 3.1 DDK,
imported with `import-ms-sdk.py` (`ms-nt31-ddk.cfg`, below), builds
drivers for it.

`scripts/fix-ddk-headers.sh` fixes a mistake in mingw-w64's `wdm.h`
(it tests `NTDDI_WINVISTA`, which nothing defines, so drivers for 2000
and XP got Vista's IRP layout), adds `STREAMAPI` to four `<strmini.h>`
functions that lack it, makes `<ks.h>`'s callbacks `NTAPI` (KS.SYS calls
them stdcall, as the DDK builds them with `cl -Gz`; mingw-w64 left them
cdecl), makes `<ndis.h>` compile next to `<ntddndis.h>`
and adds a comma its `NdisMWanIndicateReceiveComplete` lacks. It also
copies in the DDK headers mingw-w64 lacks, from `kernel/include`
(`share/clang-dos/ddk-include` in the package): `<hidport.h>`, `<1394.h>`,
`<ddrawint.h>`, `<d3dnthal.h>` and `<dvp.h>`. `build-win9x.sh` and
`build-modern-sysroot.sh` run it on each sysroot.

Drivers for Windows 95, 98 and ME are mostly VxDs (LE files, below), not
PE ones. WDM drivers for 98 and ME are PE `.sys` files that NTKERN.VXD
loads. It gives them only part of the kernel API, the functions of the
98 and ME DDKs' `wdm.lib`, and runs only drivers with a native subsystem
version of 4.x or older. `nt4-ddk.cfg` builds such drivers, and
`check-imports.py --wdm 98` (or `--wdm 98se`, `--wdm me`) checks both things. The
function list is `tools/wdm9x-exports.txt`, which
`tools/gen-wdm-exports.py` makes from those two libraries. 98 has 336
functions and ME 346. ME has 12 that 98 lacks, such as `ExCreateCallback`
and `RtlZeroMemory`, and lacks two of 98's. The `ms-w98-wdm` kit (below)
builds with the 98 DDK's own `wdm.h`.

WDM audio drivers (port class miniports) build the same way, with
mingw-w64's `<portcls.h>` and `-lportcls`, the import library
`build-vxd.sh` makes of the PORTCLS.SYS functions
(`tools/portcls9x-exports.txt`) that mingw-w64 lacks:

```sh
clang --config=nt4-ddk.cfg -O2 mydriver.c -lportcls -o mydriver.sys
python3 share/clang-dos/check-imports.py --wdm 98se mydriver.sys
```

Port class and kernel streaming (KS.SYS) grew in each version: 98's
PORTCLS.SYS has 39 functions, 98 Second Edition's 47 (the event
functions, `PcAddToEventTable` and the rest) and ME's 55 (the DRM ones,
`PcAddContentHandlers` and the rest, and `PcRegisterIoTimeout`), and
`wdm9x-exports.txt` has each version's, from the 98 DDK's `portcls.lib`
and `ks.lib` and the drivers of 98 SE and ME. `--wdm 98se` checks for
98 SE, which has 98's kernel functions. `tests/ddk/wdmaudio.c` is an
adapter driver that makes a topology port when its device starts, in C.
`scripts/fix-ddk-headers.sh` makes `ksmedia.h` (which `<portcls.h>`
includes) compile in kernel mode, where nothing defines the `TCHAR` of
its `TRANSPORTBASICPARMS`.

USB drivers import USBD.SYS. Link them with `-lusbd9x`, which
`build-vxd.sh` makes from `tools/usbd9x-exports.txt`, not mingw-w64's
`-lusbd`. 98's and 98 SE's USBD.SYS export
`USBD_CreateConfigurationRequestEx`, `USBD_ParseConfigurationDescriptorEx`
and `USBD_ParseDescriptors` only by their decorated names
(`_USBD_CreateConfigurationRequestEx@8`). The 98 DDK's `usbd.lib` imports
them by those names, and so does `-lusbd9x`; ME and Windows 2000 and
later export them too. mingw-w64's library imports them undecorated, so
the driver loads only on ME and later:

```sh
clang --config=nt4-ddk.cfg -O2 myusb.c -lusbd9x -o myusb.sys
python3 share/clang-dos/check-imports.py --wdm 98 myusb.sys
```

HID drivers link mingw-w64's `-lhidparse` and `-lhidclass` (include
`<hidusage.h>` before `<hidpi.h>`). A HID minidriver includes
`<hidport.h>` (after `<hidusage.h>` and `<hidclass.h>`, which it
includes itself), this SDK's own, which mingw-w64 lacks:
`HID_MINIDRIVER_REGISTRATION` for `HidRegisterMinidriver`,
`HID_DEVICE_EXTENSION`, the HID descriptor and the internal `IOCTL_HID_*`
requests. `tools/gen-vxd-headers.py --wdm` made it, and `<1394.h>`, from
the names, values and layouts of the 2000 and XP DDKs; what only XP has
is under `_WIN32_WINNT >= 0x0501`. `tests/ddk/wdm-layout/` checks their
layouts. `wdm9x-exports.txt` also has the functions of USBD.SYS, HIDCLASS.SYS and
HIDPARSE.SYS, from the 98 DDK's libraries and the drivers of 98 SE and
ME. 98 SE added `HidP_InitializeReportForID`, `HidP_GetExtendedAttributes`
and two more. For these drivers and port class, `--wdm 98se` goes by
98 SE's own driver: it dropped some of 98's functions, such as KS.SYS's
`KsQuerySoftwareBusInterface`. `tests/ddk/usbdrv.c` is a USB client
driver, `tests/ddk/hidparse.c` parses HID reports and
`tests/ddk/hidmini.c` is a HID minidriver.

1394 (FireWire) client drivers send IRBs to the bus driver with
`IOCTL_1394_CLASS`, declared in `<1394.h>`; 1394BUS.SYS exports nothing
a client calls, so they need no import library. `tests/ddk/fw1394.c`
asks for the bus's generation count.

Stream class minidrivers (video capture, TV tuners, `<strmini.h>`) link
`-lstream`, the import library `build-vxd.sh` makes from
`tools/stream9x-exports.txt`, as mingw-w64 has none. STREAM.SYS has 19
functions in 98 and 98 SE, and ME's adds
`StreamClassFilterReenumerateStreams`.
`wdm9x-exports.txt` also has USBCAMD.SYS's (for USB cameras) from the
98 DDK; 98 SE and ME have no list for it, so `--wdm 98se` and `--wdm me`
warn that they do not check it rather than fail. `tests/ddk/streammini.c`
registers a minidriver with `StreamClassRegisterAdapter`.

AVStream minidrivers (`<ks.h>`, `-lks`, `KsInitializeDriver` with a
device descriptor) load on 98 Second Edition and ME, whose KS.SYS
brought AVStream; 98's does not have it. ME's KS.SYS added the stream
pointer functions (`KsPinGetLeadingEdgeStreamPointer`,
`KsStreamPointerAdvanceOffsetsAndUnlock` and the rest), so a pin-centric
minidriver that uses them loads only on ME; `--wdm 98se` says which.
mingw-w64's `<ks.h>` has the same layouts as the XP DDK's for the
AVStream structures (descriptors, dispatch tables, `KSPIN`, `KSFILTER`,
`KSDEVICE`, stream pointers). `tests/ddk/avstream.c` is a capture
filter with one pin-centric output pin.

NDIS 5 miniports (network adapters, `<ndis.h>` with `NDIS50_MINIPORT`,
`-lndis`) are `.sys` files that 98 SE and ME load. 98 SE's NDIS.VXD
resolves their imports from a table of 277 names in NDIS.VXD itself, not
from an export table; `gen-wdm-exports.py` reads it from 98 SE's
NDIS.VXD (found with NDIS.SYM) into `wdm9x-exports.txt`, and
`--wdm 98se` checks against it. There is no such list for 98 or ME, so
those two only warn. `tests/ddk/ndismini.c` registers a miniport with
`NdisMRegisterMiniport`.

Display drivers for NT 4.0 and later are DLLs that import WIN32K.SYS
(GDI's engine, `<winddi.h>`, `-lwin32k`), with `DrvEnableDriver` as
their entry point. `check-imports.py --nt` checks their imports against
NT 4.0's, 2000's and XP's WIN32K.SYS (`EngAlphaBlend` and the rest of
2000's are not in 4.0's). mingw-w64's `<winddi.h>` includes
`<ddrawint.h>` and `<d3dnthal.h>`, the DirectDraw and Direct3D HAL that
`DrvGetDirectDrawInfo` and `DrvEnableDirectDraw` fill in (`DD_HALINFO`,
`DD_CALLBACKS`, `D3DNTHAL_GLOBALDRIVERDATA`, the `DD_*DATA` each callback
gets, the DirectX 6 and later command buffer), and `<dvp.h>`, video
ports, which mingw-w64 lacks. `gen-vxd-headers.py --wdm` made the three
from the 2000 and XP DDKs, and `tests/ddk/wdm-layout/` checks their
layouts; what only XP's has (DirectX 8 and 9) is under
`_WIN32_WINNT >= 0x0501`:

```sh
clang --config=nt4-ddk.cfg -O2 -shared -Wl,--entry,_DrvEnableDriver@12 \
  mydisp.c -lwin32k -o mydisp.dll
python3 share/clang-dos/check-imports.py --nt 4.0 mydisp.dll
```

`tests/ddk/display.c` implements `DrvBitBlt` with `EngBitBlt` and has a
DirectDraw and Direct3D HAL with one callback, `WaitForVerticalBlank`.

`tools/mapsym.py` writes a `.SYM` file (MAPSYM's format), which WDEB386
and SoftICE load. For a driver, it reads the symbols from the image's
COFF symbol table, which the linker keeps unless `-s` strips it. That
includes the static functions and variables, listed by section. Built
with `-g`, the driver also gets line numbers from its DWARF line table, in
MAPSYM's format: one table of code offsets and source lines for each
source file:

```sh
clang --config=nt4-ddk.cfg -O2 -g myusb.c -lusbd9x -o myusb.sys
python3 share/clang-dos/mapsym.py myusb.sys      # writes myusb.sym
python3 share/clang-dos/mapsym.py --dump --lines myusb.sym
```

For WinDbg (and KD) on NT 4.0 and later, build with `-g -gcodeview` and
have the linker write a PDB with `-Wl,--pdb=`. Add
`-Wl,--Xlink=-pdbaltpath:%_PDB%` so that the image's debug directory
names only the PDB, not the folder it was built in, and WinDbg finds it
on its symbol path (`.sympath`) next to the driver. The PDB has the
functions, their types, line numbers and the frame data that WinDbg
needs to walk the stack through code built without frame pointers.
`llvm-symbolizer --obj=mydrv.sys --pdb=mydrv.pdb ADDRESS` reads it the
same way:

```sh
clang --config=winxp-ddk.cfg -O2 -g -gcodeview mydrv.c -o mydrv.sys \
  -Wl,--pdb=mydrv.pdb -Wl,--Xlink=-pdbaltpath:%_PDB%
```

The PDB is version 7.0, which WinDbg 6 and later read; NT 4.0's own
4.0 debuggers read only `.DBG` files and older PDBs, but WinDbg 6 debugs
NT 4.0 targets. With `--pdb`, the linker leaves out the COFF symbol table
and the DWARF that `mapsym.py` reads; to have both a PDB and a `.SYM`
file from one build, add `-gdwarf` and
`-Wl,--Xlink=-debug:symtab -Wl,--Xlink=-debug:dwarf`.

`tests/ddk/hello.c` is a small driver that makes `\Device\ClangHello` and
answers an IOCTL with a 64-bit computation and whether its `__except`
caught a division by zero (with a `__finally` inside);
`tests/ddk/client.c` talks to it. Wine runs drivers as services, so

```sh
CFGDIR=~/clang-dos/bin sh tests/ddk/run-tests.sh
```

builds it with each `*-ddk.cfg` at `-O0` and `-O2`, checks it with
`check-imports.py` (the NT 4.0 build also as a WDM driver for 98 and ME,
and `tests/ddk/notwdm.c`, which calls `PsGetVersion`, must fail that
check), loads it in Wine (32-bit drivers in a 32-bit prefix,
64-bit ones in a 64-bit one) and runs the client. Wine's kernel is not
Windows', so a real Windows is still the final test.

### Signing drivers

64-bit Vista and later load only signed drivers. `tools/drvsign.py`
(`share/clang-dos/drvsign.py` in the package; Python only, no signtool,
makecert, makecat or OpenSSL) signs them for test-signing:

```sh
python3 drvsign.py newcert test.pem          # once: test.pem and test.cer
python3 drvsign.py sign --cert test.pem mydriver.sys
python3 drvsign.py catalog --cert test.pem -o mydriver.cat \
    mydriver.inf mydriver.sys
python3 drvsign.py verify --cert test.pem mydriver.sys mydriver.cat
```

* `newcert` makes an RSA key and a self-signed code-signing certificate
  (`test.pem`, which stays private, and `test.cer`, the certificate alone).
* `sign` embeds an Authenticode signature (PKCS #7, the hash of the whole
  image but its checksum and certificate table) and fixes the checksum;
  signing again replaces the signature.
* `catalog` makes the signed catalog an INF's `CatalogFile` names, with the
  Authenticode hash of each PE file and the plain hash of the rest.
* `verify` checks the hashes and the signatures.

Windows loads such drivers in test mode: `bcdedit /set testsigning on`
(with Secure Boot off) and a restart. To install a package without the
unsigned-driver prompt, add the certificate to the Trusted Root and
Trusted Publishers stores (`certutil -addstore root test.cer`,
`certutil -addstore trustedpublisher test.cer`). Windows Vista, 7 and 8
without updates check only SHA-1: give `--hash sha1` to `newcert` and
`sign` for them (catalogs are SHA-1 unless `--hash sha256`). Release
signing needs a certificate from a certificate authority and, for
Windows 10 and later, Microsoft's attestation signing, which this does
not do. 32-bit Windows and NT 4.0, 2000 and XP do not check drivers'
signatures, but XP asks before installing a package without a signed
catalog.

`tests/ddk/clanghello.inf` installs the test driver as a demand-start
service (Wine installs, runs and removes it from the INF). `osslsigncode verify` accepts these signatures and catalogs, and
Wine's `WinVerifyTrust` accepts the signatures and rejects ones whose
file or signature was changed. The test driver itself is signed in the
tests, and still loads in Wine; real Windows in test mode is still to be
tried.

### SCSI miniports for Windows 95, 98 and ME

Windows 9x drives SCSI host adapters (and many IDE and RAID ones) with the
same kind of miniport as Windows NT. A 9x miniport is a `.mpd` file:

* a PE image with the native subsystem, version 4.0;
* with the entry point `DriverEntry`;
* importing only `SCSIPORT.SYS`, which 9x's `SCSIPORT.PDR` provides.

`SCSIPORT.PDR` has the structures of NT 3.51. Its `HW_INITIALIZATION_DATA`
is 76 bytes and its `PORT_CONFIGURATION_INFORMATION` is 124. mingw-w64's
DDK headers have Windows 2000's (80 and 140 bytes), which it refuses.
`win95-miniport.cfg` uses 9x's headers instead:

```sh
clang --config=win95-miniport.cfg -O2 myadapter.c -o myadapter.mpd
python3 share/clang-dos/check-imports.py --miniport myadapter.mpd
```

* `<miniport.h>`, `<srb.h>` and `<scsi.h>` (in
  `sysroots/win9x-msvcrt/include/miniport9x`) have the 95, 98 and ME DDKs'
  types, constants and structures, and the `ScsiPort*` prototypes. The
  three DDKs agree on them. `tools/gen-vxd-headers.py --miniport` made
  them from the DDKs' `block/inc`, as for the VxD headers, and
  `tests/ddk/miniport-layout/*.c` checks their layouts.
* Functions and callbacks are stdcall (`-mrtd`), as the DDK builds them
  with `cl -Gz`. `ScsiPortNotification` and `ScsiDebugPrint` take variable
  arguments, so they are cdecl.
* `libscsiport9x.a` imports the 45 functions that `SCSIPORT.PDR` has
  (`tools/scsiport9x-exports.txt`, from the 95 and 98 DDKs' import
  libraries). `libvxd.a` has `memcpy`, `memset` and the like.
* `check-imports.py --miniport` checks the subsystem and that each import
  is one of those 45.

`tests/ddk/miniport.c` is a virtual adapter with no hardware: its target
0 answers INQUIRY as a processor device. `run-tests.sh` builds it at
`-O0`, `-O2` and `-Os` and checks it. A miniport is installed with an INF
for the SCSI adapter class, as the DDK's samples are. Nothing loads one
in Wine, so real Windows 9x is the test.

### VxDs for Windows 3.1, 95, 98 and ME

A VxD is a 32-bit virtual device driver in the LE format, run by the VMM
in ring 0. `win95-vxd.cfg` compiles one (freestanding, no FPU or SSE, no
stack protector or unwind tables) and `tools/vxd-link.py` links it with
JWlink:

```sh
clang --config=win95-vxd.cfg -O2 -c hello.c -o hello.o
python3 share/clang-dos/vxd-link.py -o HELLO.VXD hello.o
python3 share/clang-dos/vxd-check.py --dynamic HELLO.VXD
```

`<vxd.h>` has what a VxD written in C needs, with no Microsoft headers:

- `VXD_DEVICE(NAME, ID, INIT_ORDER, MAJOR, MINOR)` defines the device
  descriptor block `NAME_DDB` (exported as ordinal 1, as the VMM wants)
  and an assembly control procedure. That procedure saves the registers
  and calls `int vxd_control(DWORD msg, struct vxd_regs *r)`. Return 0 for
  success (carry clear) and nonzero for failure; change `r->eax` and the
  other fields to return values in registers.
- The control messages (`SYS_DYNAMIC_DEVICE_INIT`, `W32_DEVICEIOCONTROL`,
  ...), `struct DIOCParams`, and `DIOC_OPEN` and `DIOC_CLOSEHANDLE`.
- The services of the VMM and of 40 VxDs of Windows 3.1 to ME (1,391 of
  them, in `<vxdsvc.h>`, made from `tools/vxd-services.txt`).
  `VXDSVC_name` is the dword that follows `int 20h` for each. A service
  is defined when `VXD_WINVER` is at least the Windows version that has
  it: 3.10 (`0x030A`) for `win31-vxd.cfg`, 4.00 by default, and
  `-DVXD_WINVER=0x040A` for 98 or `0x045A` for ME. Each service's version
  is the first of the 3.1, 95, 98 and ME DDKs that has it (95 OSR2's, 4.03,
  where the 95 DDK's table says so).
- `VxDCall(name, &regs)` calls a register service: it loads EAX to EBP
  from a `struct vxd_regs`, calls the service, stores them back and
  returns 1 if the service set the carry flag. The services called like C
  functions (`_HeapAllocate`, `_RegOpenKey`, `_VWIN32_SetWin32Event`, the
  StdCall and FastCall ones) are functions: `libvxd.a` has a stub for
  each that jumps to the service. `<vxd.h>` declares the common ones with
  their types (the heap, pages, the registry, VWIN32's events and APCs).
  `<vxdsvc.h>` declares 383 of the other 496 C services with their types,
  from `tools/vxd-prototypes.txt`. `tools/gen-vxd-protos.py` made that
  list from the 95, 98 and ME DDKs' C headers and, for the services no
  header declares, the DDKs' help and the 95 DDK's documents. It keeps
  only the types and reduces each to `BYTE`, `WORD`, `DWORD` or `void *`. The other 113 are
  in neither: Microsoft's internal and obsolete services (`_ACPI_Obsolete1`
  and the like), or ones the help names without a syntax. It declares
  those with no parameter list (in C before C23), and the StdCall and
  FastCall ones with their dword parameters.
- `VXD_CALLBACK(name, fn)` makes a procedure for the VMM to call with
  registers (a time-out, an event, an I/O trap, an interrupt): it calls
  `int fn(struct vxd_regs *r)` and sets the carry flag if `fn` returned
  nonzero. `VMMCALL_Set_Global_Time_Out`, `VMMCALL_Schedule_Global_Event`,
  `SHELL_SYSMODAL_Message`, `VMMCALL_Get_System_Time` and a few others
  wrap common register services, and `struct VPICD_IRQ_Descriptor` and
  the `HEAP*`, `PAGE*`, `PG_*`, `MB_*`, `HKEY_*` and I/O trap constants
  go with them.
- Code and data are locked (always in memory) unless marked:
  `VXD_INIT_CODE` and `VXD_INIT_DATA` put them in `_ITEXT` and `_IDATA`,
  thrown away once the VxD has initialized, and `VXD_PAGEABLE_CODE` and
  `VXD_PAGEABLE_DATA` in `_PTEXT` and `_PDATA`, which can be paged out
  (not for interrupt handlers). JWlink makes an object of each kind, with
  the flags the VMM's loader tells them by.
- `libvxd.a` also provides `memcpy`, `memmove`, `memset`, `memcmp` and
  `strlen`, and `libgcc.a` the 64-bit arithmetic.

A VxD can provide services of its own, and needs a device ID to do so.
Pick one Microsoft didn't assign; the tests use 0x3E5A.

- `VXD_DEVICE_EX(NAME, ID, INIT_ORDER, MAJOR, MINOR, V86, PM, SERVICES)`
  is `VXD_DEVICE` with more fields filled in. `V86` and `PM` are the
  entry points a DOS or Windows program finds with int 2Fh AX=1684h
  BX=ID (two `VXD_CALLBACK`s, or 0). They get the caller's registers
  through `r->ebp`, a `struct vxd_client_regs`; set or clear
  `VXD_CLIENT_CARRY` in its `eflags` for the caller's carry flag.
  `SERVICES` is the service table, or 0.
- `VXD_SERVICES(NAME, a, b, ...)` defines `NAME_Services`, numbered from
  0. A service is a `VXD_CALLBACK` (register) or a C function.
- Another VxD calls those services with `VXD_CALL_ID(&regs,
  VXD_SERVICE(ID, N))` for a register service. For a C one, it declares
  `VXD_C_SERVICE(Call_Name, ID, N)` and a prototype, then calls
  `Call_Name(...)`.

`<vcomm.h>` (which includes `<vxd.h>`) is for VCOMM port drivers, the
VxDs that drive a serial or parallel port for VCOMM on Windows 95, 98 and
ME. It has `PortFunctions`, the table of the driver's 23 port functions
(`PortSetCommState` to `PortDeviceIOCtl`, as typed function pointers),
`PortData`, the state of an open port that `PortOpen` returns, and the
`_DCB`, `_COMMPROP`, `_COMSTAT`, `COMMTIMEOUTS` and `IORequest` structures
they take, with their constants (`IE_*`, `EV_*`, `CE_*`, `MS_*`, the DCB's
`f*` masks, `BAUD_*`, the escape functions and so on). Their layouts and
values are the 95, 98 and ME DDKs' (packed, as the DDK builds them). These
are VCOMM's versions of the DCB and COMMPROP, not `<winbase.h>`'s, so a
file can't include both. A driver calls `_VCOMM_Register_Port_Driver` from
`Sys_Dynamic_Device_Init`, and `_VCOMM_Add_Port` from the function it
registered. `vxd/portdrv.c` is a loopback port driver built that way.

The VMM's other subsystems have headers of their own, in
`sysroots/win9x-msvcrt/include/vxd`, which `win95-vxd.cfg` searches after
any kit's headers (a VxD built with an `ms-*-vxd` kit still gets the
DDK's own). Each includes `<vxd.h>`, `<vxdtypes.h>` (the DDK base types
they use, `ULONG`, `PVOID` and so on) and the headers it needs, and has
that subsystem's constants, structures, unions, enumerations and
typedefs, under the same names as the DDK's:

| Header | For |
| --- | --- |
| `configmg.h`, `vmmreg.h` | the Configuration Manager (`CONFIGRET`, `CR_*`, `CM_*`, `CMCONFIG`, the resource descriptors, the handler types) and the registry |
| `vpicd.h`, `vdmad.h`, `vtd.h` | interrupts, DMA and the timer |
| `vwin32.h`, `vxdldr.h`, `shell.h` | VWIN32, the dynamic VxD loader and the SHELL device |
| `vpowerd.h`, `pci.h`, `pccard.h`, `vcache.h` | power management, PCI, PC Card and the block cache |
| `ifs.h` | the installable file system manager (`ioreq`, `IFSFN_*`, the handle function tables) |
| `ior.h`, `blockdev.h`, `drp.h`, `ilb.h`, `sgd.h`, `med.h`, `vrp.h`, `aep.h`, `isp.h`, `irs.h`, `ios.h`, `dcb.h`, `iop.h`, `ida.h`, `ddb.h`, `dvt.h`, `ivt.h` | IOS and the layered block device drivers (`.PDR` port drivers and the rest) |
| `ndis.h`, `netvxd.h` | NDIS 3.1 network drivers (from the 95 and 98 DDKs; ME's `ndis.h` is NDIS 5's, for WDM) |
| `minivdd.h` | display mini-VDDs |
| `mmdevldr.h` | MMDEVLDR, which loads sound cards' VxDs and 16-bit drivers (its services, `MMDEVLDR_Register_Device_Driver` and `_MMDEVLDR_SetDevicePresence` and the rest, are in `<vxdsvc.h>`) |
| `vkd.h`, `vmd.h` | the keyboard (`SS_*` shift states, the hot key flags, `Paste_Rec`) and the mouse (`VMD_Type_*`, `Mouse_Instance`, the cursor drawing VxD structures) |

Each declaration is under `#if VXD_WINVER >=` the first DDK that has
it, and one that changed between DDKs (a count of functions that grew, a
structure with a new field) has each DDK's version. `dcb.h`'s `_DCB` is
IOS's device control block, not `<vcomm.h>`'s, so a file can't include
both, as with the DDKs. `tools/gen-vxd-headers.py` made these headers
from the 95, 98 and ME DDKs' (with the block device ones from the DDKs'
`block/inc`): it reads the names, values and types from clang's AST and
preprocessor, leaving out comments, code and the service wrappers
(`<vxdsvc.h>` has the services), and writes `vxd/layout/*.c`, which
`vxd/run-tests.sh` compiles to check that each structure's size and
offsets are the ones the DDK's compiler gave it. To make them again:

```sh
python3 tools/gen-vxd-headers.py --kits ~/kits PACKAGE/bin
```

with the `ms-w95-vxd`, `ms-w98-vxd` and `ms-me-vxd` kits imported into
PACKAGE and `~/kits` the folder `import-ms-sdk.py` read them from.

The DDKs have VKD's, VMOUSE's and MMDEVLDR's only as MASM include files
(`vkd.inc`, `vkdpaste.inc`, `vmd.inc`, `mmdevldr.inc`), so
`tools/gen-vxd-inc.py PACKAGE/bin` makes `vkd.h`, `vmd.h` and `mmdevldr.h`: it evaluates their `EQU`s, writing each as the number
it comes to, and lays out their `STRUC`s and `UNION`s byte-packed as MASM
does, for `vxd/layout/vkd.c` and `vmd.c` to check.

An IOS port driver (`.PDR`, which IOS loads from `SYSTEM\IOSUBSYS` at
boot) is a dynamic VxD whose `Sys_Dynamic_Device_Init` calls
`IOS_Register` with its `DRP` (`<drp.h>`): the DRP names its load group
(`DRP_LGN`), its async event routine and the `ILB` (`<ilb.h>`) IOS fills in.
IOS then calls the routine with each `AEP` (`<aep.h>`), and the driver asks
IOS for things with ISPs (`<isp.h>`) through `ILB_service_rtn`. For
example, `vxd/iospdr.c` is a port driver with no devices that does this
and records the events it got in the registry:

```sh
clang --config=win95-vxd.cfg -O2 -c iospdr.c -o iospdr.o
python3 share/clang-dos/vxd-link.py --name IOSPDR -o IOSPDR.PDR iospdr.o
```

`vxd-link.py --version 1.2.3.4` adds a VERSIONINFO resource the way
Microsoft's VxDs carry one, with `--product-version`, `--description`,
`--company`, `--product` and `--copyright` for its strings. The resource
goes at the end of the file, with LE header fields 0xB8 and 0xBC pointing
to it. `vxd-check.py`
validates it and prints the version.

Unloading a VxD that provides services fails with VXDLDR error 9 ("device
unloadable") on Windows 98. That happened once another VxD had called its
services through int 20h, which the VMM patches into direct calls. A VxD
that provides services should expect to stay loaded.

Win32 services are the ones a Win32 program calls with KERNEL32's
`VxDCall0` to `VxDCall8` (exports 1 to 9, by ordinal) and `ID << 16 | N`.
Microsoft never documented how Windows calls them, and `vxd/w32svc.c`
found out on Windows 98. VWIN32 calls a service stdcall with two
dwords before the program's parameters: the caller's registers (a
`struct vxd_client_regs`) and the VM handle. `VxDCall` then returns the
caller's EAX from that structure.
`VXD_WIN32_SERVICE(Name, Fn, N)` makes a service from a C function
`DWORD Fn(struct vxd_client_regs *c, DWORD vm, DWORD a1, ... aN)` and
stores its return value there. List the services in
`VXD_WIN32_SERVICES(NAME, VXD_WIN32(Name, N), ...)` and call
`VXD_REGISTER_WIN32_SERVICES(NAME)` from `Device_Init` or
`Sys_Dynamic_Device_Init`. That calls the VMM's `_Register_Win32_Services`,
which fills in the DDB's Win32 service table.

More of what `<vxd.h>` has, each with a sample in `vxd/`:

- **DeviceIoControl that finishes later** (`vxd/async.c`, called by
  `vxd/asyncc.c`). When a program passes an OVERLAPPED, the VxD can lock
  the output buffer and the OVERLAPPED with `vxd_lock_global` (global
  addresses, valid in any context). It then returns `VXD_DIOC_PENDING`, so
  `DeviceIoControl` fails with `ERROR_IO_PENDING`. Later, from a time-out,
  an event or an interrupt, the VxD writes the answer, calls
  `VWIN32_DIOCCompletion(overlapped, bytes)` (VWIN32's
  `VWIN32_DIOCCompletionRoutine`) and unlocks both with
  `vxd_unlock_global`. The program's `GetOverlappedResult` then returns
  with the byte count.
- **Hooking a service** (`vxd/hook.c`). `vxd_hook_service(VXDSVC_Name,
  Hook, &prev)` makes every call to a VMM or VxD service go to `Hook`,
  and `vxd_unhook_service` undoes that (95 and later; a dynamic VxD must
  unhook before it unloads). `VXD_HOOK_REG(Hook, Fn, prev)` makes a hook
  for a register service. It calls `int Fn(struct vxd_regs *)`, which
  returns `VXD_HOOK_PASS` to go on to the previous handler, or
  `VXD_HOOK_DONE` or `VXD_HOOK_FAIL` to return to the caller.
  `VXD_HOOK_C(Hook, Fn, prev)` makes a hook for a C service. Both give the
  hook the prologue the VMM uses to chain hooks (`jmp short`, then
  `jmp [prev]`).
- **Services that take their parameters on the stack**, such as
  `IOS_Register`. `VXD_STACK_SERVICE(fn, Name)` defines `fn` as a stub
  that jumps to the service (`int 20h` with the VxDJmp bit), so a
  prototype and a call are all it needs. `vxd/iospdr.c` uses it.
- **Calling real-mode code in the current VM** (`vxd/bios.c`).
  `vxd_exec_int(n, &regs)` saves the VM's registers, runs software
  interrupt `n` there with nested execution, with `struct vxd_int_regs`
  in and out, and restores the registers. `bios.c` reads the base memory
  size with INT 12h and the tick count with INT 1Ah.
- **Debug output.** With `-DVXD_DEBUG`, `Trace_Out("text")` and
  `Debug_Printf("fmt", ...)` print to the debugger (WDEB386, SoftICE or
  the debug VMM's terminal). `VXD_ASSERT(e)` prints the file and line and
  breaks into the debugger, if one is installed, when `e` is false.
  Without `-DVXD_DEBUG` they compile to nothing.

`vxd-link.py --sym HELLO.SYM` also writes a `.SYM` file for WDEB386 and
SoftICE. `tools/mapsym.py` makes it from JWlink's map file, with the public
and static symbols of each object. For objects compiled with `-g`, the
file also gets their line numbers. JWlink leaves the DWARF out of the
VxD, so `mapsym.py` reads each object's line table and places it using
the map's symbols.

`vxd-link.py` writes a dynamic VxD by default, the kind Win32 programs
load with `CreateFile("\\\\.\\C:\\PATH\\HELLO.VXD", ...)` and call
with `DeviceIoControl`. `--static` writes one that SYSTEM.INI loads with
`device=`. `--win31` writes a Windows 3.1 VxD; compile it with
`win31-vxd.cfg`, which sets `VXD_DDK_VERSION` to 3.10. The VxD's name
defaults to the output's base name in upper case (at most 8 letters), and
it must match the `NAME` given to `VXD_DEVICE`. `vxd-check.py` checks the
LE header, each object's kind, the DDB and the control procedure, which
must be locked.

`vxd/hello.c` is a dynamic VxD that answers an IOCTL with a 64-bit
computation and the VMM's version, and `vxd/client.c` is its Win32
client. `vxd/services.c` allocates from the heap, writes and reads the
registry, sets a time-out and an event from init code and answers from
pageable code. `vxd/provider.c` provides services, V86 and PM entry
points and two Win32 services. `client.c`, `vxd/apitest.c` (a DOS
program) and `vxd/w32svc.c` call it, and all three ran on Windows 98 SE.
`CFGDIR=~/clang-dos/bin sh vxd/run-tests.sh` builds `hello.c` as a
dynamic, static and Windows 3.1 VxD, and `services.c`, `provider.c`,
`async.c`, `hook.c` and `bios.c` as dynamic ones, at `-O0`, `-O2` and
`-Os`, and checks each build. No emulator runs VxDs, so with `WIN9X_RUN`
set to a command that runs `CLIENT.EXE` on a real Windows 9x, the dynamic
ones are loaded too. `hello.c` and `services.c` have run on Windows 98
SE; `async.c`, `hook.c` and `bios.c` have not yet.

## Redistributing the SDK

The sysroots hold no Microsoft files and can be passed on freely: see
`NOTICE.md` for what each part is and its licence. `build-sysroot.sh`
puts the licences in each sysroot's `share/licenses/`. Programs built
with them should come with mingw-w64's runtime notice
(`share/licenses/mingw-w64/COPYING.MinGW-w64-runtime.txt`), as with any
mingw-w64 build.

```sh
sh tools/make-dist.sh ~/clang-dos-linux-x86_64 win32-sdk.tar.xz
```

makes the SDK on its own from an unpacked clang-dos package: the five
sysroots, their config files, the checkers and the licences, laid out to
unpack over a clang-dos package. dos-sdk's toolchain workflow makes it
with every package. Sysroots imported from Microsoft's own kits
(`tools/import-ms-sdk.py`, below) are never put in it.

## Microsoft's SDKs and DDKs

The sysroots above are made from mingw-w64. `tools/import-ms-sdk.py` makes
sysroots from Microsoft's own Win32 SDKs and DDKs instead, with their
headers, import libraries and C runtime, for code written against them.
Microsoft's kits can't be redistributed, so neither this repository nor
the toolchain package has any of their files: run the script once on your
own copies to add them to an unpacked toolchain package.

```sh
python3 tools/import-ms-sdk.py --sdks /path/to/WindowsSDKs ~/clang-dos
clang --config=ms-nt351.cfg -O2 hello.c -o hello.exe
```

`--sdks` is a folder holding the kits under the folder names below (an
installed tree or the unpacked CD; `--kit-dir KIT=FOLDER` gives one kit's
folder instead), and every kit found there is imported. Each one becomes
`sysroots/ms-KIT` and `bin/ms-KIT.cfg` in the package.

| Kit | Folder | Triple | From |
| --- | --- | --- | --- |
| `nt31` | `3.1 SDK DDK` | `i386-pc-windows3.10-msvc` | Win32 SDK for NT 3.1 (MSDN, October 1994) |
| `nt351` | `3.51 SDK` | `i386-pc-windows3.51-msvc` | Win32 SDK, June 1995, with NT 3.1's C runtime |
| `nt40` | `4.0 W95 SDK (expanded)` | `i486-pc-windows4.0-msvc` | Win32 SDK for NT 4.0 and 95, August 1996, with NT 3.1's C runtime |
| `w2k` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.0-msvc` | Server 2003 SP1 DDK, Windows 2000 headers and libraries |
| `xp` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.1-msvc` | Server 2003 SP1 DDK, XP headers and libraries |
| `ws03` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.2-msvc` | Server 2003 SP1 DDK |
| `ws03-x64` | `5.2.3970.1830 DDK (unpacked)` | `x86_64-pc-windows5.2-msvc` | Server 2003 SP1 DDK, x64 |
| `nt31-ddk` | `3.1 SDK DDK` | `i386-pc-windows3.10-msvc` | NT 3.1 DDK (MSDN, October 1994), kernel-mode drivers |
| `nt40-ddk` | `4.0 DDK` | `i386-pc-windows4.0-msvc` | NT 4.0 DDK (4.0.1381), drivers, with the headers of `nt40` and `nt31` |
| `w2k-ddk` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.0-msvc` | Server 2003 SP1 DDK, Windows 2000 drivers |
| `xp-ddk` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.1-msvc` | Server 2003 SP1 DDK, XP drivers |
| `ws03-ddk` | `5.2.3970.1830 DDK (unpacked)` | `i586-pc-windows5.2-msvc` | Server 2003 SP1 DDK, drivers |
| `ws03-x64-ddk` | `5.2.3970.1830 DDK (unpacked)` | `x86_64-pc-windows5.2-msvc` | Server 2003 SP1 DDK, x64 drivers |
| `w95-vxd` | `4.0 W95 DDK` | `i386-pc-windows4.0-msvc` | Windows 95 DDK, VxDs |
| `w98-vxd` | `4.10 DDK (unpacked)` | `i386-pc-windows4.10-msvc` | Windows 98 DDK, VxDs |
| `me-vxd` | `4.90 DDK (unpacked)` | `i386-pc-windows4.90-msvc` | Windows ME DDK, VxDs |
| `w31-vxd` | `3.1 DDK` | (`win31-vxd.cfg`'s) | Windows 3.1 DDK, VxDs: C headers made from its MASM ones |
| `w98-wdm` | `4.10 DDK (unpacked)` | `i386-pc-windows4.0-msvc` | Windows 98 DDK, WDM drivers for 98, ME and 2000 |

* The kits are used as Microsoft shipped them, so the 1996 SDK's
  compressed `.LI_` files must be expanded first (`expand -r`) and the
  DDKs' CABs unpacked into the tree their setup installs (`WINDDK\`).
  For the 98 and ME DDKs, `tools/unpack-ms-ddk.py 98DDK.RAR "4.10 DDK
  (unpacked)"` (or `WMEDDK.ZIP` and `"4.90 DDK (unpacked)"`) does that;
  it needs `7z`.
  The Aug 1996 SDK and the June 1995 one have no i386 C runtime library,
  so they take the NT 3.1 SDK's, which also runs on 95 and NT 4.0.
* The triples are MSVC ones, so programs use Microsoft's ABI and C
  runtime (statically, `libcmt.lib` or `libc.lib`, whichever the kit has),
  not mingw-w64's. As with the MinGW triples, the Windows version in the
  triple sets the default CPU and, with LLD, the subsystem and OS version
  the linker writes, and for NT 3.x a writable `.rdata`.
* The config file sets `-fms-compatibility-version` to the compiler the
  kit came with, defines `_X86_` (or `_AMD64_`), which early headers want
  from the makefile, and links the kit's C runtime instead of clang's
  default `libcmt` and `oldnames`.
* On a case-sensitive file system the headers and libraries are copied
  with lower-case names, and `vfs.yaml`, a case-insensitive overlay the
  config file passes with `-vfsoverlay`, lets `#include <Windows.h>` and a
  library's `/DEFAULTLIB:LIBC` find them, as on Windows.
* The libraries of the 1993 to 1996 kits need fixing for LLD, which
  `ms_oldlib.py` does to the sysroot's copies (with the package's
  `llvm-lib`, or `--llvm-lib`): their archive headers are padded with
  NULs; their objects give sections an address, with relocations relative
  to it, and leave out the sections' access bits; and their long-format
  import libraries (a whole object per import) make LLD build an import
  table with a DLL's terminator in the middle, so they are rebuilt as
  short-format ones and checked against the originals. NT 3.1's
  `crtdll.lib` stays as it is, since its data imports (`_iob` and the
  like) have no short-format equivalent: use `libcmt`, the default.
* Those kits' C runtime predates `__int64` and asks for neither kernel32
  nor SafeSEH: their config files link `kernel32`, pass `-safeseh:no` and
  link `ll64.lib`, the 64-bit integer helpers (`__alldiv` and the like)
  taken from the Server 2003 DDK's `libcmt.lib` when that kit is imported
  too. Without it, 64-bit division doesn't link.
* The XP and Server 2003 kits' C runtime is built with `/GS`, so their
  config files link `bufferoverflowu`. The Server 2003 `winnt.h` declares
  `_interlockedbittestandset` and the like without `volatile`, which clang
  rejects, so `ms-ws03` and `ms-ws03-x64` include `ms-intrin-shim.h`
  first. The DDK's `inc/w2k` and `inc/wxp` lack four headers `windows.h`
  includes, which come from `inc/wnet`.
* C only: the kits' C++ libraries are for Microsoft's compilers of the
  time.
* The `-ddk` kits build kernel-mode drivers (`clang --config=ms-xp-ddk.cfg
  -O2 mydriver.c -o mydriver.sys`), with the DDK's `ntddk.h`, `wdm.h` and
  the rest before its Windows headers, and its `ntoskrnl.lib`, `hal.lib`
  and kernel C library (`libcntpr.lib`, NT 3.1's `libcnt.lib`; with
  `bufferoverflowk` for the `/GS` ones) instead of a C runtime. Their
  config files define what the DDKs' `build` did for a driver (`_X86_`,
  `i386`, `STD_CALL`, `CONDITION_HANDLING`, `_NT1X_`, `WINNT`,
  `_WIN32_WINNT` and `WINVER` for the triple's version, ...) and pass
  `-mrtd`, since the DDKs built drivers with `cl /Gz` and declare dispatch
  routines and the like without a calling convention. They pass
  `-fasync-exceptions` for `__try`, as `cl`'s is asynchronous. The NT
  3.1 DDK's `libcnt.lib` has only `cl` 8.0's `_except_handler2`, so
  `ms-nt31-ddk.cfg` links the package's `libntseh.a` for clang's
  `_except_handler3`. The 386 kits (NT 3.1, NT 4.0, 98 WDM, the VxD
  ones) pass `-fno-ms-volatile`: with `cl`'s volatile semantics, clang
  makes volatile accesses atomic ones, which for a 386 are calls to
  `__sync_lock_test_and_set_4`, a function no kit has. The linker makes a
  driver image: `-driver`, native subsystem of the triple's version, entry
  point `DriverEntry`, base `0x10000`, checksum, the `PAGE` and `INIT`
  sections of `alloc_text`, with `INIT` discardable. A few mistakes in the
  DDKs' `ntddk.h` that `cl` accepted are fixed in the sysroot's copy (the
  2000 and XP DDKs declare `_InterlockedXor` with a `LONG` for its
  pointer; NT 4.0 to XP read a byte of the KPCR in inline assembly
  without saying its size). The Windows Vista WDK and later are not
  supported yet.

* The `-vxd` kits build VxDs the DDKs' way, with `vmm.h`, `vxdwraps.h`
  and the other headers of `INC32` (`inc/win98`, `inc/win_me`) and
  `VXDWRAPS.CLB`: compile with `clang --config=ms-w95-vxd.cfg -c` and link
  with `vxd-link.py --kit ms-w95-vxd`, which links the kit's library
  before `libvxd.a`. The config does what the DDKs' makefiles did (`cl
  -Zp -DIS_32`: structures packed, `-fpack-struct=1`), defines the
  service sets the headers declare (`WIN40SERVICES`, and
  `WIN403SERVICES` and `WIN41SERVICES` for 98, `WIN49SERVICES` for ME),
  and allows Microsoft's inline assembly (`-fasm-blocks`). `<vxd.h>` is
  copied into the sysroot: with these configs it includes `vmm.h` and
  adds `VXD_DEVICE` for the DDB (the DDKs did it in assembly),
  `VXD_CALLBACK`, `VXD_CALL` (`VxDCall` is the DDK's) and the code and
  data kinds. The DDK's `#pragma VxD_LOCKED_CODE_SEG` and the like are
  macros that name the pragma, which clang expands with
  `-fms-extensions` as `cl` did. The sysroot's `vmm.h` and `vxdwraps.h`
  are changed for clang: the macros of the service tables take fewer
  arguments than `cl` allowed, `VxDCall` emits the service's number from
  `<vxdsvc.h>` (clang's `_emit` takes numbers, not the enum's
  constants), and `PREPEND` expands `CURSEG()` before pasting.
* The Windows 3.1 DDK's VxD headers are MASM ones only. For `w31-vxd`,
  `tools/inc2h.py` makes a C header from each of them in the sysroot
  (`vmm.h`, `vpicd.h`, `shell.h`, ...). Each header has the file's numeric
  equates as `#define`s and its structures as packed `struct`s
  (`struct Client_Reg_Struc`, `struct VxD_Desc_Block`, ...). It leaves
  out macros, text equates and offsets into structures. Compile with
  `ms-w31-vxd.cfg`, which is `win31-vxd.cfg` plus those headers, then
  include `<vxd.h>` and the headers you need, and link with `vxd-link.py
  --win31`. The `vmda*.h` headers are alternatives, one per display
  adapter, so include only one.
* `w98-wdm` builds WDM drivers with the 98 DDK's `wdm.h` and `wdm.lib`
  (`-DHELLO_WDM` in the test), checked with `check-imports.py --wdm 98`.

`tests/ms-sdk/run-test.sh` tests the script with three small fake kits.
`tests/ms-sdk/kits-test.py PACKAGE --run` builds and runs a program with
each imported kit, for testing with the real ones: all seven user-mode ones
built and ran on Windows 11 at `-O0` and `-O2`. With `--wine` it runs them
in Wine and also loads `tests/ddk/hello.c` built with each `-ddk` kit as a
Wine service: the six of them work in Wine 9.0. `vxd/run-tests.sh` builds
`vxd/msddk.c`, a VxD written with the DDKs' headers, with each `-vxd`
kit it finds (`vxd/msddk31.c` with `w31-vxd`).

The 1993 to 1996 kits' import libraries, after `llvm-lib` rebuilds them,
import stdcall functions by an EXPORTAS name. LLD before version 19 (a
distribution's, as `kits-test.py --stock-clang` uses) reads that as an
import by ordinal #0, so `ms_oldlib.py` makes them plain undecorated
imports, which mean the same thing to every linker. `kits-test.py`
checks each driver's imports with `check-imports.py`, which catches
that.

## License

Copyright (C) clang-retro contributors.

The clang-retro Win32 SDK is free software under the GNU Affero General Public
License, version 3 only (`AGPL-3.0-only`); the full text is in
[LICENSE](LICENSE). If you distribute a program built with it, or let
people use a modified version over a network, you must offer the
program's complete source under the same license.

The sysroots it builds also hold mingw-w64, LLVM, picolibc and
libunicows, each under its own license; [NOTICE.md](NOTICE.md) lists
them.

Each contributor keeps the copyright in their own work and contributes
it under the same license; see [CONTRIBUTING.md](CONTRIBUTING.md).
