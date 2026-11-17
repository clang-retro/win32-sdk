#!/bin/sh
# The IDL side of a Windows sysroot: mingw-w64's .idl and .acf files in
# include/, so that a program's own IDL or ODL can import them, and the OLE
# Automation type libraries stdole2.tlb and stdole32.tlb in lib/, for
# importlib(). The type libraries are compiled from idl/stdole2.idl with
# widl.
#
#   SYSROOT    the sysroot being built
#   MINGW_SRC  mingw-w64 checkout
#   WORK       build directory
#   IDL_ENV    win32 (the default) or win64, the type libraries' platform
#   WIDL       a widl that runs here (optional: without one, widl is built
#              from MINGW_SRC with the build machine's compiler, HOST_CC or
#              cc)
#
# Part of clang-retro win32-sdk.
# SPDX-License-Identifier: AGPL-3.0-only
set -e
: "${SYSROOT:?set SYSROOT}" "${MINGW_SRC:?set MINGW_SRC}" "${WORK:?set WORK}"
here=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$WORK" "$SYSROOT/include" "$SYSROOT/lib"

for f in "$MINGW_SRC"/mingw-w64-headers/include/*.idl \
         "$MINGW_SRC"/mingw-w64-headers/include/*.acf; do
  [ -f "$f" ] && cp "$f" "$SYSROOT/include/"
done

if [ -z "$WIDL" ]; then
  # widl for the build machine only; the packages build their own.
  b=$WORK/widl-build
  mkdir -p "$b"
  (
    cd "$b"
    # widl is ordinary C: the clang-dos tools on PATH are for the targets.
    env CC="${HOST_CC:-cc}" CFLAGS="-O2" \
      "$MINGW_SRC/mingw-w64-tools/widl/configure" --prefix="$WORK/widl" \
      --target=i686-w64-mingw32 >configure.log 2>&1 || { cat configure.log; exit 1; }
    make -j"$(nproc 2>/dev/null || echo 2)" >make.log 2>&1 || { tail -50 make.log; exit 1; }
    make install >install.log 2>&1
  )
  WIDL=$WORK/widl/bin/i686-w64-mingw32-widl
fi

env=--${IDL_ENV:-win32}
"$WIDL" $env -I"$SYSROOT/include" -DSTDOLE_VERSION=2.0 -t \
  -o "$SYSROOT/lib/stdole2.tlb" "$here/idl/stdole2.idl"
"$WIDL" $env -I"$SYSROOT/include" -DSTDOLE_VERSION=1.0 -t \
  -o "$SYSROOT/lib/stdole32.tlb" "$here/idl/stdole2.idl"
echo "IDL files and stdole type libraries installed in $SYSROOT"
