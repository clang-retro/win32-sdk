#!/bin/sh
# Correct declarations in mingw-w64's Windows API headers that disagree
# with the Windows SDK's, where code written against the SDK (WTL, for
# one) does not compile:
#   uxtheme.h   GetThemePartSize, GetThemeMargins and
#               DrawThemeParentBackground take a const RECT, and
#               GetThemeTextMetrics a TEXTMETRICW, as the SDK has them
#   commctrl.h  LM_GETIDEALSIZE, the SysLink message's name since Windows
#               Vista (LM_GETIDEALHEIGHT, its Windows XP name, stays)
# Each change is made once; running the script again changes nothing.
#
#   SYSROOT  the sysroot whose include directory to correct
#
# Part of clang-retro win32-sdk.
# SPDX-License-Identifier: AGPL-3.0-only
set -e
: "${SYSROOT:?set SYSROOT}"
inc=$SYSROOT/include

f=$inc/uxtheme.h
if [ -f "$f" ]; then
  sed -i \
    -e 's/^\(THEMEAPI GetThemePartSize(HTHEME hTheme,HDC hdc,int iPartId,int iStateId,\)RECT \*prc,/\1LPCRECT prc,/' \
    -e 's/^\(THEMEAPI GetThemeMargins(HTHEME hTheme,HDC hdc,int iPartId,int iStateId,int iPropId,\)RECT \*prc,/\1LPCRECT prc,/' \
    -e 's/^\(THEMEAPI DrawThemeParentBackground(HWND hwnd,HDC hdc,\)RECT \*prc);/\1const RECT *prc);/' \
    -e 's/^\(THEMEAPI GetThemeTextMetrics(HTHEME hTheme,HDC hdc,int iPartId,int iStateId,\)TEXTMETRIC \*ptm);/\1TEXTMETRICW *ptm);/' \
    "$f"
fi

f=$inc/commctrl.h
if [ -f "$f" ] && ! grep -q 'define LM_GETIDEALSIZE' "$f"; then
  sed -i 's/^#define LM_GETIDEALHEIGHT (WM_USER+0x301)$/&\
#define LM_GETIDEALSIZE LM_GETIDEALHEIGHT/' "$f"
fi
