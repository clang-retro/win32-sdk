#!/bin/sh
# Fix mistakes in the DDK headers of mingw-w64 that build-win9x.sh and
# build-modern-sysroot.sh installed in SYSROOT/include/ddk, for drivers built
# with the *-ddk.cfg config files:
#
#   wdm.h tests NTDDI_WINVISTA, which no header defines (sdkddkver.h has
#   NTDDI_VISTA), so drivers for Windows 2000 and XP get Vista's IRP layout
#   with SystemPowerStateContext and fail to compile.
#
#   ksmedia.h (in include, which portcls.h includes for WDM audio drivers)
#   has a TCHAR in TRANSPORTBASICPARMS, and nothing defines TCHAR in kernel
#   mode (_NTDDK_), so it is a CHAR there.
#
#   strmini.h declares four of STREAM.SYS's stdcall functions
#   (StreamClassAbortOutstandingRequests, StreamClassGetNextEvent,
#   StreamClassQueryMasterClock, StreamClassRegisterFilterWithNoKSPins)
#   without STREAMAPI, so a minidriver would import _Name, which no
#   import library has, instead of _Name@N.
#
#   ks.h (in include) declares its callbacks (AVStream's dispatch
#   tables, intersect handlers, allocators ...) without a calling
#   convention, so they are cdecl; KS.SYS calls them stdcall, as the DDK
#   builds them (cl -Gz). They get NTAPI.
#
#   ndis.h (NDIS miniports) defines NDIS_REQUEST_TYPE again after the
#   ntddndis.h it includes, and misses a comma in
#   NdisMWanIndicateReceiveComplete's parameters, so it does not compile.
#
# It also adds the DDK headers mingw-w64 lacks, from kernel/include (in a
# package, ddk-include next to this script): hidport.h (HID minidrivers)
# and 1394.h (IEEE 1394 client drivers), and the DirectDraw and Direct3D
# HAL of display drivers that mingw-w64's winddi.h includes, ddrawint.h,
# d3dnthal.h and dvp.h, and dxmini.h (DirectDraw in video miniports),
# all of which gen-vxd-headers.py --wdm made from
# the 2000 and XP DDKs; and hdaudio.h (HD Audio function drivers on XP's
# and later's HDAUDBUS.SYS), written from Microsoft's documentation.
#
#   fix-ddk-headers.sh SYSROOT
set -e
inc=${1:?usage: fix-ddk-headers.sh SYSROOT}/include/ddk
[ -f "$inc/wdm.h" ] || exit 0
here=$(dirname "$0")
for d in "$here/../kernel/include" "$here/ddk-include"; do
  if [ -d "$d" ]; then cp "$d"/*.h "$inc/"; break; fi
done
ks=$inc/../ksmedia.h
if [ -f "$ks" ] && ! grep -q 'ifdef _NTDDK_ /\* fix-ddk-headers' "$ks"; then
  awk '/^  TCHAR VolumeName\[40\];$/ {
         print "#ifdef _NTDDK_ /* fix-ddk-headers.sh: no TCHAR in kernel mode */"
         print "  CHAR VolumeName[40];"; print "#else"; print; print "#endif"
         next } { print }' "$ks" > "$ks.new"
  mv "$ks.new" "$ks"
fi
# ks.h's callbacks (AVStream's dispatch tables, intersect handlers ...):
# the DDK builds with cl -Gz, so KS.SYS calls them stdcall.
kh=$inc/../ks.h
if [ -f "$kh" ]; then
  sed 's/^typedef \([A-Za-z_][A-Za-z_0-9]*\) (\*PFN/typedef \1 (NTAPI *PFN/' \
    "$kh" > "$kh.new"
  mv "$kh.new" "$kh"
fi
sm=$inc/strmini.h
if [ -f "$sm" ]; then
  awk '/^StreamClass(AbortOutstandingRequests|GetNextEvent|QueryMasterClock|RegisterFilterWithNoKSPins)\($/ &&
       prev !~ /STREAMAPI/ { print "STREAMAPI" } { print; prev = $0 }' \
    "$sm" > "$sm.new"
  mv "$sm.new" "$sm"
fi
nd=$inc/ndis.h
if [ -f "$nd" ] && ! grep -q 'fix-ddk-headers.sh: ntddndis.h' "$nd"; then
  awk '/^typedef enum _NDIS_REQUEST_TYPE \{$/ {
         print "#ifndef _NTDDNDIS_ /* fix-ddk-headers.sh: ntddndis.h has it */"
         skip = 1 }
       { print }
       skip && /^\} NDIS_REQUEST_TYPE, \*PNDIS_REQUEST_TYPE;$/ {
         print "#endif"; skip = 0 }' "$nd" |
    sed 's/^\(\tIN NDIS_HANDLE  MiniportAdapterHandle\)$/\1,/' > "$nd.new"
  mv "$nd.new" "$nd"
fi
sed 's/NTDDI_VERSION >= NTDDI_WINVISTA)/NTDDI_VERSION >= NTDDI_VISTA)/' \
  "$inc/wdm.h" > "$inc/wdm.h.new"
mv "$inc/wdm.h.new" "$inc/wdm.h"
