#!/bin/sh
# Fix mistakes in the DDK headers of mingw-w64 that build-win9x.sh and
# build-modern-sysroot.sh installed in SYSROOT/include/ddk, for drivers built
# with the *-ddk.cfg config files:
#
#   wdm.h tests NTDDI_WINVISTA, which no header defines (sdkddkver.h has
#   NTDDI_VISTA), so drivers for Windows 2000 and XP get Vista's IRP layout
#   with SystemPowerStateContext and fail to compile.
#
#   fix-ddk-headers.sh SYSROOT
set -e
inc=${1:?usage: fix-ddk-headers.sh SYSROOT}/include/ddk
[ -f "$inc/wdm.h" ] || exit 0
sed 's/NTDDI_VERSION >= NTDDI_WINVISTA)/NTDDI_VERSION >= NTDDI_VISTA)/' \
  "$inc/wdm.h" > "$inc/wdm.h.new"
mv "$inc/wdm.h.new" "$inc/wdm.h"
