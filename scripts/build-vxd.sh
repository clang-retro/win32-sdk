#!/bin/sh
# Add the VxD support to a Windows 9x sysroot (build-win9x.sh's):
#   $SYSROOT/include/vxd.h     the DDB, control messages, VMM calls
#   $SYSROOT/include/vxd/      the subsystems' headers: vcomm.h (what a
#                              VCOMM port driver hands to VCOMM) and those
#                              tools/gen-vxd-headers.py made (configmg.h,
#                              ifs.h, ios.h, ndis.h ...), which
#                              win95-vxd.cfg adds to the include path
#   $SYSROOT/include/miniport9x/  the SCSI miniport headers, and
#   $SYSROOT/lib/libscsiport9x.a  their import library (win95-miniport.cfg)
#   $SYSROOT/lib/libportcls.a  PORTCLS.SYS's, for WDM audio drivers
#   $SYSROOT/lib/libusbd9x.a   USBD.SYS's as Windows 98 has it, for WDM
#                              USB drivers
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
mkdir -p "$WORK" "$SYSROOT/include/vxd" "$SYSROOT/lib"
cp "$here/vxd/vxd.h" "$SYSROOT/include/"
cp "$here/vxd/vcomm.h" "$here"/vxd/include/*.h "$SYSROOT/include/vxd/"
# SCSI miniports (win95-miniport.cfg): the 9x miniport.h, srb.h and scsi.h,
# and libscsiport9x.a, the import library of the SCSIPORT.SYS functions
# 9x's SCSIPORT.PDR has.
mkdir -p "$SYSROOT/include/miniport9x"
cp "$here"/miniport/include/*.h "$SYSROOT/include/miniport9x/"
{ echo "LIBRARY SCSIPORT.SYS"; echo "EXPORTS"
  grep -v '^#' "$here/tools/scsiport9x-exports.txt"; } >"$WORK/scsiport9x.def"
"$BIN/llvm-dlltool" -m i386 -k -d "$WORK/scsiport9x.def" \
  -l "$SYSROOT/lib/libscsiport9x.a"
# WDM audio drivers (nt4-ddk.cfg, -lportcls): the import library of the
# PORTCLS.SYS functions 98, 98 SE and ME have, which mingw-w64 lacks.
{ echo "LIBRARY PORTCLS.SYS"; echo "EXPORTS"
  grep -v '^#' "$here/tools/portcls9x-exports.txt"; } >"$WORK/portcls.def"
"$BIN/llvm-dlltool" -m i386 -k -d "$WORK/portcls.def" \
  -l "$SYSROOT/lib/libportcls.a"
# WDM USB drivers (nt4-ddk.cfg, -lusbd9x): USBD.SYS's import library as
# the 98 DDK's, which imports three functions by the decorated names that
# 98 and 98 SE export (mingw-w64's libusbd.a works only on ME and later).
{ echo "LIBRARY USBD.SYS"; echo "EXPORTS"
  grep -v '^#' "$here/tools/usbd9x-exports.txt"; } >"$WORK/usbd9x.def"
"$BIN/llvm-dlltool" -m i386 -k -d "$WORK/usbd9x.def" \
  -l "$SYSROOT/lib/libusbd9x.a"
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
