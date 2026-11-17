#!/bin/sh
# Build the test VxD (hello.c) at -O0, -O2 and -Os as a dynamic, a static
# and a Windows 3.1 VxD, services.c's (VMM services, and init and pageable
# code and data) and provider.c's (services and entry points of its own)
# at each level as dynamic ones, linktest.c's, portdrv.c's (a VCOMM port
# driver) in C99, C17 and C23, and
# msddk.c's (msddk31.c's for the 3.1 one) with each ms-*-vxd.cfg (import-ms-sdk.py's VxD kits) in CFGDIR,
# and check each one with vxd-check.py. No emulator runs VxDs: with WIN9X_RUN,
# a command that takes a folder holding a VxD and CLIENT.EXE, runs
# "CLIENT.EXE NAME.VXD" on a real Windows 9x and prints what it wrote to
# RESULT.TXT, the dynamic ones are loaded too.
#
#   CFGDIR   a toolchain package's bin/ (clang, jwlink, the config files),
#            with the win9x-msvcrt sysroot in ../sysroots
#   WORK     build folder (default: ./build-vxd-tests)
: "${CFGDIR:?set CFGDIR}"
here=$(cd "$(dirname "$0")" && pwd)
tools=$here/../tools
[ -f "$tools/vxd-link.py" ] || tools=$here/..
out=${WORK:-$PWD/build-vxd-tests}
mkdir -p "$out"
cd "$out" || exit 1
PY=${PYTHON:-python3}
pass=0 fail=0
bad() { fail=$((fail + 1)); echo "FAIL $1"; sed 's/^/    /' "$2" | head -20; }
link="$PY $tools/vxd-link.py --jwlink $CFGDIR/jwlink --sysroot $CFGDIR/../sysroots/win9x-msvcrt"
for kind in dynamic static win31; do
  cfg=win95-vxd.cfg
  [ $kind = win31 ] && cfg=win31-vxd.cfg
  for opt in O0 O2 Os; do
    t=hello-$kind-$opt
    if ! "$CFGDIR/clang" --config="$CFGDIR/$cfg" -$opt -c "$here/hello.c" \
         -o "$t.o" >"$t.log" 2>&1; then
      bad "$t build" "$t.log"; continue
    fi
    flag=
    [ $kind != dynamic ] && flag=--$kind
    if ! $link $flag --name HELLO -o "$t.vxd" "$t.o" >"$t.log" 2>&1; then
      bad "$t link" "$t.log"; continue
    fi
    if ! $PY "$tools/vxd-check.py" --$kind "$t.vxd" >"$t.chk" 2>&1; then
      bad "$t vxd-check" "$t.chk"; continue
    fi
    pass=$((pass + 1))
  done
done
# One VxD: build SRC with the config and flags, link and check it.
vxd() { # NAME SRC CFG CHECK LINKFLAGS -- CFLAGS...
  t=$1 src=$2 cfg=$3 chk=$4 lf=$5
  shift 6
  if ! "$CFGDIR/clang" --config="$CFGDIR/$cfg" "$@" -c "$src" -o "$t.o" \
       >"$t.log" 2>&1; then
    bad "$t build" "$t.log"; return
  fi
  if ! $link $lf -o "$t.vxd" "$t.o" >"$t.log" 2>&1; then
    bad "$t link" "$t.log"; return
  fi
  if ! $PY "$tools/vxd-check.py" $chk "$t.vxd" >"$t.chk" 2>&1; then
    bad "$t vxd-check" "$t.chk"; return
  fi
  pass=$((pass + 1))
}
for opt in O0 O2 Os; do
  vxd svc-$opt "$here/services.c" win95-vxd.cfg --dynamic "--name SVCTEST" \
    -- -$opt -Wall -Werror
  # Each kind of object, as services.c asks for them.
  for k in "init code" "pageable code" "init data" "pageable data"; do
    grep -q "$k" svc-$opt.chk 2>/dev/null ||
      { echo "no $k object" >>svc-$opt.chk; bad "svc-$opt kinds" svc-$opt.chk; }
  done
done
# A VxD with a V86 and a PM entry point, services and Win32 services, and
# a version resource; the checker follows each to locked code.
for opt in O0 O2 Os; do
  vxd provide-$opt "$here/provider.c" win95-vxd.cfg --dynamic \
    "--name PROVIDE --version 1.0.0.42 --company clang-retro" \
    -- -$opt -Wall -Werror
  for k in "V86 API procedure" "PM API procedure" "service table: 3" \
           "version resource: 1.0.0.42"; do
    grep -q "$k" provide-$opt.chk 2>/dev/null ||
      { echo "no $k" >>provide-$opt.chk; bad "provide-$opt" provide-$opt.chk; }
  done
done
# A VCOMM port driver, in each C standard (<vcomm.h>'s layouts are
# asserted in it).
for std in c99 c17 c23; do
  vxd portdrv-$std "$here/portdrv.c" win95-vxd.cfg --dynamic "--name PORTDRV" \
    -- -O2 -std=$std -Wall -Wextra -Werror
done
vxd linktest "$here/linktest.c" win95-vxd.cfg --dynamic "" \
  -- -O2 -DVXD_WINVER=0x045A -Wall -Werror
# The VxD kits import-ms-sdk.py made from Microsoft's DDKs, if any: a VxD
# written the DDK's way, with its headers and VXDWRAPS.CLB.
# The 3.1 kit has C headers made from MASM ones, and no VXDWRAPS.CLB.
for c in "$CFGDIR"/ms-*-vxd.cfg; do
  [ -f "$c" ] || continue
  kit=$(basename "$c" .cfg)
  if [ $kit = ms-w31-vxd ]; then
    vxd $kit "$here/msddk31.c" "$kit.cfg" --win31 "--win31 --name MSDDK31" \
      -- -O2 -Wall -Werror
    continue
  fi
  vxd $kit "$here/msddk.c" "$kit.cfg" --dynamic "--name MSDDK --kit $kit" \
    -- -O2 -Wall -Werror
done
# The programs that call provider.c's VxD: w32svc.c (Win32 services) and,
# with a DOS config, apitest.c (the V86 entry point). They run on Windows
# by hand (see provider.c).
if "$CFGDIR/clang" --config="$CFGDIR/win98.cfg" -O2 -Wall -Werror \
     "$here/w32svc.c" -o w32svc.exe >w32svc.log 2>&1; then
  pass=$((pass + 1))
else
  bad w32svc w32svc.log
fi
if [ -f "$CFGDIR/dos.cfg" ]; then
  if "$CFGDIR/clang" --config="$CFGDIR/dos.cfg" -mcmodel=tiny -Os -Wall \
       -Werror "$here/apitest.c" -o apitest.com >apitest.log 2>&1; then
    pass=$((pass + 1))
  else
    bad apitest apitest.log
  fi
fi
if [ -n "${WIN9X_RUN:-}" ]; then
  "$CFGDIR/clang" --config="$CFGDIR/win98.cfg" -O2 "$here/client.c" \
    -o client.exe >client.log 2>&1 || bad client client.log
  for opt in O0 O2 Os; do
    t=run-$opt
    rm -rf "$t" && mkdir "$t"
    cp "hello-dynamic-$opt.vxd" "$t/HELLO.VXD"
    cp client.exe "$t/CLIENT.EXE"
    got=$($WIN9X_RUN "$t" 2>"$t.err" | tr -d '\r')
    case $got in
      "hello from clang vxd 428571431 4."*) pass=$((pass + 1)) ;;
      *) echo "got: $got" >>"$t.err"; bad "$t run" "$t.err" ;;
    esac
    t=run-svc-$opt
    rm -rf "$t" && mkdir "$t"
    cp "svc-$opt.vxd" "$t/SVCTEST.VXD"
    cp client.exe "$t/CLIENT.EXE"
    got=$($WIN9X_RUN "$t" SVCTEST.VXD 2>"$t.err" | tr -d '\r')
    case $got in
      "services heap 1 reg 1234 timer 1 event 1 page 42 init 7")
        pass=$((pass + 1)) ;;
      *) echo "got: $got" >>"$t.err"; bad "$t run" "$t.err" ;;
    esac
  done
fi
echo "vxd tests: $pass passed, $fail failed"
[ $fail = 0 ]
