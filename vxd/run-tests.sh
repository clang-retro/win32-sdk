#!/bin/sh
# Build the test VxD (hello.c) at -O0, -O2 and -Os as a dynamic, a static
# and a Windows 3.1 VxD, and check each one with vxd-check.py. No emulator
# runs VxDs: with WIN9X_RUN, a command that takes a folder holding
# HELLO.VXD and CLIENT.EXE, runs CLIENT.EXE on a real Windows 9x and prints
# what it wrote to RESULT.TXT, the dynamic ones are loaded too.
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
  done
fi
echo "vxd tests: $pass passed, $fail failed"
[ $fail = 0 ]
