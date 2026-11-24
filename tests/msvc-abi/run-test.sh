#!/bin/sh
# Tests clang's Microsoft C++ ABI for old Visual C++ against code that a real
# Visual C++ compiled, in Wine (or on Windows, with WINE empty).
#
#   PACKAGE=~/clang-dos DDK=/path/to/WINDDK sh tests/msvc-abi/run-test.sh
#
# PACKAGE    the toolchain package, with a C++ kit imported by
#            tools/import-ms-sdk.py (KIT, default xp-vc6)
# DDK        the Windows Server 2003 SP1 DDK's WINDDK folder: its cl.exe
#            (Visual C++ 2003's compiler, 13.10) compiles the other side.
#            Without it, only the clang-only tests (stl, mfc) run.
# MFC42_DLL  a copy of mfc42.dll (from Windows 2000 or later, or the Visual
#            C++ 6.0 redistributables), for the MFC test. Without it, the
#            MFC program is built but not run.
# WINE       the command that runs a program (default wine)
# OUT        the folder for the programs (default /tmp/msvc-abi-test)
#
# The tests:
#   cl-cl      abi.cpp and client.cpp, both with cl: the reference
#   clang-cl   client.cpp with clang, against msabi.dll from cl
#   clang7-cl  the same, with -fms-compatibility-version=13.10 (cl's own)
#   cl-clang   client.cpp with cl, against abi.cpp built by clang
#   stl        stl.cpp with clang and msvcp60.dll
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
MFC42_DLL=${MFC42_DLL:-}
cfg=$PACKAGE/bin/ms-$KIT.cfg
sysroot=$PACKAGE/sysroots/ms-$KIT
clang="$PACKAGE/bin/clang++ --config=$cfg -O2 ${CLANG_FLAGS:-}"
[ -f "$cfg" ] || { echo "no $cfg: import the $KIT kit first" >&2; exit 2; }
export WINEDEBUG=${WINEDEBUG:--all}
# Wine has its own msvcp60.dll and no mfc42.dll: use Microsoft's.
export WINEDLLOVERRIDES="msvcp60=n;mfc42=n"

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

# Runs Visual C++'s cl.exe from the DDK with its headers and libraries.
cl() {
  INCLUDE="$(winpath "$DDK/inc/crt");$(winpath "$DDK/inc/wxp");$(winpath "$DDK/inc/wnet")" \
  LIB="$(winpath "$DDK/lib/crt/i386");$(winpath "$DDK/lib/wxp/i386")" \
    $WINE "$DDK/bin/x86/cl.exe" /nologo /GX /GR /MD /O2 "$@"
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

# Copies the C++ library DLL next to a program in the folder $1.
runtime() {
  for d in "$DDK/bin/x86/msvcp60.dll" "$sysroot/../../bin/msvcp60.dll"; do
    [ -f "$d" ] && { cp "$d" "$1/"; return; }
  done
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
cl_lib() {
  cl /LD abi.cpp /Femsabi.dll /link bufferoverflowu.lib
}

# Builds the ABI test library with clang, named as cl's.
clang_lib() {
  $clang -shared abi.cpp -o msabi.dll -Wl,-implib:msabi.lib
}

# Builds the client with cl.
cl_client() {
  cl client.cpp msabi.lib bufferoverflowu.lib /Feclient.exe
}

# Builds the client with clang, with any extra flags given.
clang_client() {
  $clang "$@" client.cpp msabi.lib -o client.exe
}

if [ -n "$DDK" ] && [ -f "$DDK/bin/x86/cl.exe" ]; then
  setup cl-cl
  if (cd "$OUT/cl-cl" && cl_lib > build.log 2>&1); then
    test_one cl-cl client.exe cl_client
    for t in clang-cl clang7-cl; do
      setup $t
      cp "$OUT/cl-cl/msabi.dll" "$OUT/cl-cl/msabi.lib" "$OUT/$t/"
    done
    test_one clang-cl client.exe clang_client
    test_one clang7-cl client.exe clang_client -fms-compatibility-version=13.10
  else
    nobuild cl-cl
  fi
  setup cl-clang
  if (cd "$OUT/cl-clang" && clang_lib > build.log 2>&1); then
    test_one cl-clang client.exe cl_client
  else
    nobuild cl-clang
  fi
else
  echo "skip cl-cl, clang-cl, clang7-cl, cl-clang (no DDK with cl.exe)"
fi

setup stl
test_one stl stl.exe $clang stl.cpp -o stl.exe

setup mfc
if (cd "$OUT/mfc" && $clang -D_AFXDLL mfc.cpp -o mfc.exe > build.log 2>&1); then
  if [ -n "$MFC42_DLL" ]; then
    cp "$MFC42_DLL" "$OUT/mfc/mfc42.dll"
    run "$OUT/mfc" mfc.exe mfc
  else
    echo "skip running mfc (no MFC42_DLL)"
  fi
else
  nobuild mfc
fi

echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
