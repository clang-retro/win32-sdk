#!/bin/sh
# Add Visual C++'s compiler COM support to a Windows sysroot, for comdef.h
# and #import:
#   $SYSROOT/lib/libcomsupp.a   the functions comdef.h declares but does not
#                               define (comsupp/comsupp.cpp): the dispatch
#                               calls of #import's dispinterface wrappers
#                               and _com_issue_errorex
#   $SYSROOT/lib/libcomsuppw.a  the same library under its other name
#                               (comsupp.lib and comsuppw.lib differ only in
#                               wchar_t in Visual C++)
# and two changes to mingw-w64's headers:
#   comdefsp.h  defines IUnknownPtr, IDispatchPtr and the other smart
#               pointers of the system interfaces for every compiler, not
#               only for Visual C++ (#import's headers use them)
#   comdef.h    links libcomsupp, as Visual C++'s does, with #pragma
#               comment(lib) (which clang honours with -fms-extensions;
#               otherwise link with -lcomsupp)
#
#   BIN, SYSROOT as for build-sysroot.sh; WORK (default: ./build-comsupp)
#   TARGET   the sysroot's target (default: i686-w64-windows-gnu)
#   CPU      -march for the library (default: i686)
#
# Part of clang-retro win32-sdk.
# SPDX-License-Identifier: AGPL-3.0-only
set -e
: "${BIN:?set BIN}" "${SYSROOT:?set SYSROOT}"
here=$(cd "$(dirname "$0")/.." && pwd)
WORK=${WORK:-$PWD/build-comsupp}
TARGET=${TARGET:-i686-w64-windows-gnu}
mkdir -p "$WORK" "$SYSROOT/lib"

inc=$SYSROOT/include
grep -q '^#if !defined(RC_INVOKED) && USE___UUIDOF != 0$' "$inc/comdefsp.h" &&
  sed -i 's/^#if !defined(RC_INVOKED) && USE___UUIDOF != 0$/#if !defined(RC_INVOKED)/' \
    "$inc/comdefsp.h"
if ! grep -q 'comment(lib, "comsupp")' "$inc/comdef.h"; then
  sed -i 's|^#include <comutil.h>$|&\
\
#if !defined(_MSC_VER) \&\& !defined(_COM_NO_COMSUPP_LIB)\
/* clang-retro: the COM support library, as Visual C++ links comsuppw.lib. */\
#pragma comment(lib, "comsupp")\
#pragma comment(lib, "oleaut32")\
#pragma comment(lib, "ole32")\
#pragma comment(lib, "uuid")\
#endif|' "$inc/comdef.h"
  grep -q 'comment(lib, "comsupp")' "$inc/comdef.h" ||
    { echo "could not change $inc/comdef.h" >&2; exit 1; }
fi

case $TARGET in
x86_64*) march= ;;
*) march=-march=${CPU:-i686} ;;
esac
"$BIN/clang++" --target="$TARGET" --sysroot="$SYSROOT" -stdlib=libc++ $march \
  -O2 -Wall -c "$here/comsupp/comsupp.cpp" -o "$WORK/comsupp.o"
rm -f "$SYSROOT/lib/libcomsupp.a" "$SYSROOT/lib/libcomsuppw.a"
"$BIN/llvm-ar" rcs "$SYSROOT/lib/libcomsupp.a" "$WORK/comsupp.o"
cp "$SYSROOT/lib/libcomsupp.a" "$SYSROOT/lib/libcomsuppw.a"
echo "COM support library installed in $SYSROOT"
