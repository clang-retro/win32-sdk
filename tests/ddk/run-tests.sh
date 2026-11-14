#!/bin/sh
# Build the test driver (hello.c) with every *-ddk.cfg, check its PE header
# and, for the 32-bit ones, its imports (check-imports.py --nt), and load it
# in Wine, which runs drivers as services: client.c opens its device and
# sends it an IOCTL.
#
#   CFGDIR   folder with clang and the config files (a toolchain package's
#            bin/), with the win32-sdk sysroots next to it in ../sysroots
#   WINE     optional: wine (default: wine on PATH); 32-bit drivers need a
#            32-bit prefix (WINEPREFIX32, made with WINEARCH=win32 if it
#            does not exist), 64-bit ones a 64-bit one (WINEPREFIX64,
#            default: WINEPREFIX or ~/.wine)
#   CONFIGS  the configs (default: every *-ddk.cfg in CFGDIR but the ms-*
#            ones of imported Microsoft DDKs, which tests/ms-sdk/kits-test.py
#            tests)
#   WORK     build folder (default: ./build-ddk-tests)
: "${CFGDIR:?set CFGDIR}"
here=$(cd "$(dirname "$0")" && pwd)
check=$here/../../tools/check-imports.py
[ -f "$check" ] || check=$here/../check-imports.py
out=${WORK:-$PWD/build-ddk-tests}
mkdir -p "$out"
cd "$out" || exit 1
WINE=${WINE-wine}
command -v "$WINE" >/dev/null 2>&1 || WINE=
export WINEDEBUG=${WINEDEBUG:--all}
WINEPREFIX32=${WINEPREFIX32:-$out/wine32}
WINEPREFIX64=${WINEPREFIX64:-${WINEPREFIX:-$HOME/.wine}}
want="hello from clang 428571431 1"
pass=0 fail=0 skip=0
bad() { fail=$((fail + 1)); echo "FAIL $1"; sed 's/^/    /' "$2" | head -20; }

# Load DRIVER as a demand-start kernel service in the prefix, run CLIENT,
# stop the service.
rundrv() {
  prefix=$1 sys=$2 client=$3
  d=$prefix/drive_c/windows/system32/drivers
  cp "$sys" "$d/clanghello.sys" || return 1
  env WINEPREFIX="$prefix" "$WINE" sc delete clanghello >/dev/null 2>&1
  env WINEPREFIX="$prefix" "$WINE" sc create clanghello type= kernel \
    start= demand binpath= 'C:\windows\system32\drivers\clanghello.sys' \
    >/dev/null 2>&1
  # services.exe may still be starting.
  for i in 1 2 3 4 5; do
    env WINEPREFIX="$prefix" "$WINE" sc start clanghello >/dev/null 2>&1 &&
      break
    sleep 1
  done
  env WINEPREFIX="$prefix" "$WINE" "$client"
  r=$?
  env WINEPREFIX="$prefix" "$WINE" sc stop clanghello >/dev/null 2>&1
  return $r
}

if [ -n "$WINE" ]; then
  # A new prefix: let the wineserver that made it exit first, as drivers
  # started under it do not stay loaded.
  if [ ! -d "$WINEPREFIX32" ]; then
    WINEPREFIX=$WINEPREFIX32 WINEARCH=win32 "$WINE" wineboot -i >/dev/null 2>&1
    WINEPREFIX=$WINEPREFIX32 "${WINESERVER:-wineserver}" -w >/dev/null 2>&1
  fi
  if [ ! -d "$WINEPREFIX64" ]; then
    WINEPREFIX=$WINEPREFIX64 "$WINE" wineboot -i >/dev/null 2>&1
    WINEPREFIX=$WINEPREFIX64 "${WINESERVER:-wineserver}" -w >/dev/null 2>&1
  fi
  "$CFGDIR/clang" --config="$CFGDIR/winxp.cfg" -O2 "$here/client.c" \
    -o client32.exe >client32.log 2>&1 || bad client32 client32.log
  "$CFGDIR/clang" --config="$CFGDIR/win7-x64.cfg" -O2 "$here/client.c" \
    -o client64.exe >client64.log 2>&1 || bad client64 client64.log
fi

for cfg in ${CONFIGS:-$(cd "$CFGDIR" && ls *-ddk.cfg | grep -v "^ms-")}; do
  name=${cfg%.cfg}
  for opt in O0 O2; do
    t=$name-$opt
    if ! "$CFGDIR/clang" --config="$CFGDIR/$cfg" -$opt "$here/hello.c" \
         -o "$t.sys" >"$t.log" 2>&1; then
      bad "$t build" "$t.log"; continue
    fi
    case $name in *-x64-*) arch=64 ;; *) arch=32 ;; esac
    # The version the config's triple names, for the checker.
    v=$(sed -n 's/^--target=.*-windows\([0-9.]*\)-.*/\1/p' \
        "$CFGDIR/${name%-ddk}.cfg")
    if [ $arch = 32 ]; then
      if ! ${PYTHON:-python3} "$check" --nt "$v" "$t.sys" >"$t.chk" 2>&1
      then
        bad "$t check-imports" "$t.chk"; continue
      fi
    fi
    if [ -z "$WINE" ]; then skip=$((skip + 1)); pass=$((pass + 1)); continue; fi
    if [ $arch = 32 ]; then prefix=$WINEPREFIX32; else prefix=$WINEPREFIX64; fi
    got=$(rundrv "$prefix" "$t.sys" client$arch.exe 2>"$t.err" | tr -d '\r')
    if [ "$got" = "$want" ]; then
      pass=$((pass + 1))
    else
      echo "got: $got" >>"$t.err"; bad "$t run" "$t.err"
    fi
  done
done
echo "ddk tests: $pass passed, $fail failed, $skip not run in Wine"
[ $fail = 0 ]
