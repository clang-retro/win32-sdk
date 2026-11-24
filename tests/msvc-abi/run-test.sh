#!/bin/sh
# Tests clang's Microsoft C++ ABI for old Visual C++ against code that a real
# Visual C++ compiled, in Wine (or on Windows, with WINE empty).
#
#   PACKAGE=~/clang-dos DDK=/path/to/WINDDK sh tests/msvc-abi/run-test.sh
#   PACKAGE=~/clang-dos KIT=vc60 VCDIR=/path/to/VS sh tests/msvc-abi/run-test.sh
#
# PACKAGE    the toolchain package, with a C++ kit imported by
#            tools/import-ms-sdk.py (KIT, default xp-vc6)
# DDK        the Windows Server 2003 SP1 DDK's WINDDK folder, for the DDK
#            kits: its cl.exe (Visual C++ 2003's compiler, 13.10) compiles
#            the other side with the DDK's headers and libraries.
# VCDIR      the folder of an installed Visual C++, for the vc* kits (the
#            folder that the importer took for KIT): its own cl.exe compiles
#            the other side with its own headers and libraries.
#            Without DDK or VCDIR, only the clang-only tests (stl, mfc) run.
# RUNTIME    more folders (separated by ':') that hold the runtime DLLs of
#            the kit (msvcp*.dll, mfc*.dll, msvcr71.dll). The script also
#            looks in DDK, VCDIR and the package's bin folder.
# MFC42_DLL  a copy of the MFC DLL of the kit (mfc42.dll, from Windows 2000
#            or later or the redistributables), for the MFC test. Without an
#            MFC DLL, the MFC program is built but not run.
# WINE       the command that runs a program (default wine)
# OUT        the folder for the programs (default /tmp/msvc-abi-test)
#
# The tests:
#   cl-cl      abi.cpp and client.cpp, both with cl: the reference
#   clang-cl   client.cpp with clang, against msabi.dll from cl
#   clang7-cl  the same, with -fms-compatibility-version=13.10 (cl's own;
#              only with DDK: the vc* kits already use the version of cl)
#   cl-clang   client.cpp with cl, against abi.cpp built by clang
#   stl        stl.cpp with clang and the C++ library DLL (msvcp60.dll;
#              not for vc40 and vc42, which have no standard C++ library)
#   mfc        mfc.cpp with clang and mfc42.dll
# Each program prints one line for each check and returns the number that
# failed.

set -u
here=$(cd "$(dirname "$0")" && pwd)
: "${PACKAGE:?give PACKAGE, the toolchain package}"
KIT=${KIT:-xp-vc6}
WINE=${WINE-wine}
OUT=${OUT:-/tmp/msvc-abi-test}
DDK=${DDK:-}
VCDIR=${VCDIR:-}
RUNTIME=${RUNTIME:-}
MFC42_DLL=${MFC42_DLL:-}
cfg=$PACKAGE/bin/ms-$KIT.cfg
sysroot=$PACKAGE/sysroots/ms-$KIT
clang="$PACKAGE/bin/clang++ --config=$cfg -O2 ${CLANG_FLAGS:-}"
[ -f "$cfg" ] || { echo "no $cfg: import the $KIT kit first" >&2; exit 2; }
export WINEDEBUG=${WINEDEBUG:--all}
# Wine has its own C and C++ runtime DLLs and no MFC: use Microsoft's.
export WINEDLLOVERRIDES="msvcrt40,msvcp50,msvcp60,msvcr71,msvcp71,mfc40,mfc42,mfc71=n"

failed=0
passed=0
mkdir -p "$OUT"

# Converts a path to the form that a Windows program takes.
winpath() {
  if [ -n "$WINE" ]; then
    winepath -w "$1" 2>/dev/null
  else
    printf '%s\n' "$1"
  fi
}

# Prints the folders of the kit $1 under the folder $2 that the importer
# uses for "include" or "lib" ($3), one for each line. Names match without
# case, as in the importer.
kit_dirs() {
  python3 - "$here/../../tools/import-ms-sdk.py" "$1" "$2" "$3" <<'EOF'
import importlib.util, sys
spec = importlib.util.spec_from_file_location("imp", sys.argv[1])
imp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(imp)
for d in imp.find(sys.argv[3], imp.KITS[sys.argv[2]][sys.argv[4]]):
    print(d)
EOF
}

# Prints the folders under $1 that hold a file named $2 (without case).
dirs_of() {
  find "$1" -iname "$2" -exec dirname {} \; 2>/dev/null | sort -u
}

# Joins the Windows forms of the folders on standard input with ';'.
winlist() {
  out=
  while IFS= read -r d; do
    out="$out${out:+;}$(winpath "$d")"
  done
  printf '%s\n' "$out"
}

# Finds the cl.exe for the other side and sets cl_exe, cl_inc, cl_lib,
# cl_path and cl_libs: the DDK's for the DDK kits, the kit's own for the
# vc* kits. Fails if there is none.
find_cl() {
  cl_libs=
  if [ -n "$VCDIR" ]; then
    cl_exe=$(find "$VCDIR" -ipath '*/bin/cl.exe' | head -n 1)
    [ -n "$cl_exe" ] || return 1
    cl_inc=$(kit_dirs "$KIT" "$VCDIR" include | winlist)
    cl_lib=$(kit_dirs "$KIT" "$VCDIR" lib | winlist)
    # cl.exe needs the mspdb DLL of its release. Setup puts it in the
    # folder of the IDE.
    cl_path=$( (dirname "$cl_exe"; dirs_of "$VCDIR" 'mspdb*.dll') | winlist)
  elif [ -n "$DDK" ] && [ -f "$DDK/bin/x86/cl.exe" ]; then
    cl_exe=$DDK/bin/x86/cl.exe
    cl_inc="$(winpath "$DDK/inc/crt");$(winpath "$DDK/inc/wxp");$(winpath "$DDK/inc/wnet")"
    cl_lib="$(winpath "$DDK/lib/crt/i386");$(winpath "$DDK/lib/wxp/i386")"
    cl_path=$(winpath "$DDK/bin/x86")
    # The DDK's C runtime library is built with /GS.
    cl_libs=bufferoverflowu.lib
  else
    return 1
  fi
}

# Runs the cl.exe that find_cl found, with its headers and libraries. Wine's
# own runtime DLLs serve cl.exe: the native msvcr71.dll does not load for it.
cl() {
  INCLUDE="$cl_inc" LIB="$cl_lib" WINEPATH="$cl_path" WINEDLLOVERRIDES= \
    $WINE "$cl_exe" /nologo /GX /GR /MD /O2 "$@"
}

# Runs the program $2 in the folder $1 and counts the test $3 as passed if
# all its checks pass.
run() {
  ( cd "$1" && $WINE "./$2" ) > "$OUT/$3.log" 2>&1
  rc=$?
  if [ $rc -eq 0 ]; then
    passed=$((passed + 1))
    echo "PASS $3 ($(grep -c '^ok' "$OUT/$3.log") checks)"
  else
    failed=$((failed + 1))
    echo "FAIL $3 (exit $rc):"
    grep -v '^ok' "$OUT/$3.log" | sed 's/^/    /'
  fi
}

# Copies the runtime DLLs of the kit (the C++ library, MFC, and the C
# runtime of vc71) next to a program in the folder $1. It takes the first
# copy of each DLL in RUNTIME, DDK, VCDIR and the package's bin folder.
runtime() {
  for n in msvcp50.dll msvcp60.dll msvcp71.dll msvcr71.dll mfc40.dll \
      mfc42.dll mfc71.dll; do
    for top in $(printf '%s' "$RUNTIME" | tr ':' ' ') "$DDK" "$VCDIR" \
        "$PACKAGE/bin"; do
      [ -n "$top" ] && [ -d "$top" ] || continue
      d=$(find "$top" -iname "$n" 2>/dev/null | head -n 1)
      if [ -n "$d" ]; then
        cp "$d" "$1/$n"
        break
      fi
    done
  done
  if [ -n "$MFC42_DLL" ]; then
    cp "$MFC42_DLL" "$1/mfc42.dll"
  fi
}

# Counts the test $1 as failed because it did not build.
nobuild() {
  failed=$((failed + 1))
  echo "FAIL $1 (did not build, see $OUT/$1/build.log)"
}

# Runs the build command (the rest of the arguments) in the folder of the
# test $1, then runs $2 there. A test that does not build fails.
test_one() {
  t=$1
  exe=$2
  shift 2
  if (cd "$OUT/$t" && "$@" >> build.log 2>&1); then
    run "$OUT/$t" "$exe" "$t"
  else
    nobuild "$t"
  fi
}

# Makes an empty folder for the test $1, with the sources and the C++
# library DLL.
setup() {
  rm -rf "${OUT:?}/$1"
  mkdir -p "$OUT/$1"
  cp "$here/abi.h" "$here/abi.cpp" "$here/client.cpp" "$here/stl.cpp" \
    "$here/mfc.cpp" "$OUT/$1/"
  runtime "$OUT/$1"
}

# Builds the ABI test library with cl (msabi.dll and msabi.lib).
cl_dll() {
  cl /LD abi.cpp /Femsabi.dll /link $cl_libs
}

# Builds the ABI test library with clang, named as cl's.
clang_lib() {
  $clang -shared abi.cpp -o msabi.dll -Wl,-implib:msabi.lib
}

# Remakes msabi.lib with the lib.exe of the kit, from the exports of
# msabi.dll. The linkers of Visual C++ 5.0 and before cannot read the short
# import libraries that lld writes.
cl_implib() {
  "$PACKAGE/bin/llvm-readobj" --sections --coff-exports msabi.dll |
    python3 -c '
import re, sys
text = sys.stdin.read()
code = []
for m in re.finditer(r"Name: (\S+)[^\n]*\s+VirtualSize: (\w+)\s+VirtualAddress: (\w+)", text):
    if m.group(1) == ".text":
        code.append((int(m.group(3), 16), int(m.group(3), 16) + int(m.group(2), 16)))
print("LIBRARY msabi.dll")
print("EXPORTS")
for m in re.finditer(r"Name: (\S+)\s+RVA: (\w+)", text):
    rva = int(m.group(2), 16)
    data = not any(lo <= rva < hi for lo, hi in code)
    print("  " + m.group(1) + (" DATA" if data else ""))
' > msabi.def
  lib_exe=$(dirname "$cl_exe")/$(ls "$(dirname "$cl_exe")" | grep -ix lib.exe)
  WINEPATH="$cl_path" WINEDLLOVERRIDES= $WINE "$lib_exe" /nologo \
    /def:msabi.def /machine:ix86 /out:msabi.lib
}

# Builds the client with cl.
cl_client() {
  cl client.cpp msabi.lib $cl_libs /Feclient.exe
}

# Builds the client with clang, with any extra flags given.
clang_client() {
  $clang "$@" client.cpp msabi.lib -o client.exe
}

if find_cl; then
  setup cl-cl
  if (cd "$OUT/cl-cl" && cl_dll > build.log 2>&1); then
    test_one cl-cl client.exe cl_client
    clients=clang-cl
    [ -z "$VCDIR" ] && clients="clang-cl clang7-cl"
    for t in $clients; do
      setup $t
      cp "$OUT/cl-cl/msabi.dll" "$OUT/cl-cl/msabi.lib" "$OUT/$t/"
    done
    test_one clang-cl client.exe clang_client
    if [ -z "$VCDIR" ]; then
      test_one clang7-cl client.exe clang_client -fms-compatibility-version=13.10
    fi
  else
    nobuild cl-cl
  fi
  setup cl-clang
  case $KIT in
    vc40 | vc42 | vc50) old_link=1 ;;
    *) old_link= ;;
  esac
  if (cd "$OUT/cl-clang" && clang_lib > build.log 2>&1 &&
      { [ -z "$old_link" ] || cl_implib >> build.log 2>&1; }); then
    test_one cl-clang client.exe cl_client
  else
    nobuild cl-clang
  fi
else
  echo "skip cl-cl, clang-cl, clang7-cl, cl-clang (no cl.exe in DDK or VCDIR)"
fi

# Visual C++ 4.2 and before have no standard C++ library.
case $KIT in
  vc40 | vc42) echo "skip stl (no standard C++ library in $KIT)" ;;
  *)
    setup stl
    test_one stl stl.exe $clang stl.cpp -o stl.exe
    ;;
esac

setup mfc
if (cd "$OUT/mfc" && $clang -D_AFXDLL mfc.cpp -o mfc.exe > build.log 2>&1); then
  if ls "$OUT/mfc" | grep -iq '^mfc[0-9]*\.dll$'; then
    run "$OUT/mfc" mfc.exe mfc
  else
    echo "skip running mfc (no MFC DLL: give MFC42_DLL or RUNTIME)"
  fi
else
  nobuild mfc
fi

echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
