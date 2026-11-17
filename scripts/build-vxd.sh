#!/bin/sh
# Add the VxD support to a Windows 9x sysroot (build-win9x.sh's):
#   $SYSROOT/include/vxd.h     the DDB, control messages, VMM calls
#   $SYSROOT/lib/libvxd.a      memcpy, memset and the like for VxDs
# Programs are built with --config=win95-vxd.cfg -c and linked with
# tools/vxd-link.py (see README.md, "VxDs").
#
#   BIN, SYSROOT as for build-win9x.sh; WORK (default: ./build-vxd)
set -e
: "${BIN:?set BIN}" "${SYSROOT:?set SYSROOT}"
here=$(cd "$(dirname "$0")/.." && pwd)
WORK=${WORK:-$PWD/build-vxd}
mkdir -p "$WORK" "$SYSROOT/include" "$SYSROOT/lib"
cp "$here/vxd/vxd.h" "$SYSROOT/include/"
"$BIN/clang" --target=i386-pc-windows4.0-gnu --sysroot="$SYSROOT" \
  -ffreestanding -fno-builtin -mno-sse -mno-mmx -fno-stack-protector \
  -fno-asynchronous-unwind-tables -O2 -c "$here/vxd/vxdlib.c" \
  -o "$WORK/vxdlib.o"
rm -f "$SYSROOT/lib/libvxd.a"
"$BIN/llvm-ar" rcs "$SYSROOT/lib/libvxd.a" "$WORK/vxdlib.o"
echo "VxD support installed in $SYSROOT"
