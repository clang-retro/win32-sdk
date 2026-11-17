#!/bin/sh
# Add the VxD support to a Windows 9x sysroot (build-win9x.sh's):
#   $SYSROOT/include/vxd.h     the DDB, control messages, VMM calls
#   $SYSROOT/include/vxdsvc.h  the services' numbers (tools/vxd-services.txt)
#   $SYSROOT/lib/libvxd.a      memcpy, memset and the like for VxDs, the
#                              Microsoft target's 64-bit division helpers,
#                              and a stub for each service called like a C
#                              function
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
rm -rf "$WORK/stubs"
python3 "$here/tools/gen-vxd-wraps.py" --header "$SYSROOT/include/vxdsvc.h" \
  --stubs "$WORK/stubs"
"$BIN/clang" --target=i386-pc-windows4.0-gnu --sysroot="$SYSROOT" \
  -ffreestanding -fno-builtin -mno-sse -mno-mmx -fno-stack-protector \
  -fno-asynchronous-unwind-tables -O2 -c "$here/vxd/vxdlib.c" \
  -o "$WORK/vxdlib.o"
"$BIN/clang" --target=i386-pc-windows4.0-gnu -c "$here/vxd/alldiv.c" \
  -o "$WORK/alldiv.o"
# One object for each stub, so that a VxD links only the ones it calls.
(cd "$WORK/stubs" && ls | sed 's/\.s$//' |
  xargs -P "$(nproc 2>/dev/null || echo 4)" -I{} \
    "$BIN/clang" --target=i386-pc-windows4.0-gnu -c {}.s -o {}.o)
rm -f "$SYSROOT/lib/libvxd.a"
"$BIN/llvm-ar" rcs "$SYSROOT/lib/libvxd.a" "$WORK/vxdlib.o" "$WORK/alldiv.o" \
  "$WORK"/stubs/*.o
echo "VxD support installed in $SYSROOT"
