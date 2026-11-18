#!/bin/sh
# Build the test driver (hello.c) with every *-ddk.cfg, check its PE header
# and, for the 32-bit ones, its imports (check-imports.py --nt), and load it
# in Wine, which runs drivers as services: client.c opens its device and
# sends it an IOCTL. Each driver is test-signed (drvsign.py) before it
# is loaded, and a package of one with clanghello.inf gets a catalog. The
# NT 4.0 build is checked as a WDM driver for Windows 98 and ME too
# (check-imports.py --wdm), as is wdmaudio.c, a WDM audio driver (port
# class) for 98, 98 SE and ME, usbdrv.c (USB, -lusbd9x, and its .SYM
# file, with line numbers with -g), hidparse.c (HID report parsing),
# hidmini.c (a HID minidriver), fw1394.c (a 1394 client), streammini.c
# (stream class), avstream.c (AVStream) and ndismini.c (NDIS 5);
# display.c is built as an NT display driver with a DirectDraw and
# Direct3D HAL (win32k.sys, check-imports.py --nt); hello.c is built with
# a PDB for WinDbg (llvm-symbolizer reads it); and miniport.c is
# built as a SCSI miniport for Windows 9x (check-imports.py --miniport).
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
sign=$here/../../tools/drvsign.py
[ -f "$sign" ] || sign=$here/../drvsign.py
out=${WORK:-$PWD/build-ddk-tests}
mkdir -p "$out"
cd "$out" || exit 1
WINE=${WINE-wine}
command -v "$WINE" >/dev/null 2>&1 || WINE=
export WINEDEBUG=${WINEDEBUG:--all}
WINEPREFIX32=${WINEPREFIX32:-$out/wine32}
WINEPREFIX64=${WINEPREFIX64:-${WINEPREFIX:-$HOME/.wine}}
want="hello from clang 428571431 1 seh 1"
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
  # wineboot returns before Wine has finished setting up a new prefix, and
  # a driver started meanwhile can run without its device being reachable
  # (the client then gets ERROR_FILE_NOT_FOUND), so wait for the prefix's
  # wineserver to exit.
  wineserver=$(dirname "$(command -v "$WINE")")/wineserver
  [ -x "$wineserver" ] || wineserver=wineserver
  [ -d "$WINEPREFIX32" ] || { WINEPREFIX=$WINEPREFIX32 WINEARCH=win32 \
    "$WINE" wineboot -i >/dev/null 2>&1
    WINEPREFIX=$WINEPREFIX32 "$wineserver" -w; }
  [ -d "$WINEPREFIX64" ] || { WINEPREFIX=$WINEPREFIX64 \
    "$WINE" wineboot -i >/dev/null 2>&1
    WINEPREFIX=$WINEPREFIX64 "$wineserver" -w; }
  "$CFGDIR/clang" --config="$CFGDIR/winxp.cfg" -O2 "$here/client.c" \
    -o client32.exe >client32.log 2>&1 || bad client32 client32.log
  "$CFGDIR/clang" --config="$CFGDIR/win7-x64.cfg" -O2 "$here/client.c" \
    -o client64.exe >client64.log 2>&1 || bad client64 client64.log
fi

# A test certificate: each driver is signed with it (SHA-1 for the 32-bit
# ones and SHA-256 for the 64-bit ones) and then loaded, and the package
# of the last one (clanghello.inf and its catalog) checked.
${PYTHON:-python3} "$sign" newcert --bits 1024 test.pem >sign.log 2>&1 ||
  bad "test certificate" sign.log

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
    if [ $arch = 32 ]; then h=sha1; else h=sha256; fi
    if ! { ${PYTHON:-python3} "$sign" sign --cert test.pem --hash $h \
           "$t.sys" && ${PYTHON:-python3} "$sign" verify --cert test.pem \
           "$t.sys"; } >"$t.sig" 2>&1; then
      bad "$t sign" "$t.sig"; continue
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
# A driver package: the INF, the driver and a catalog of both.
last=$(ls -t *-ddk-O2.sys 2>/dev/null | head -n 1)
if [ -n "$last" ]; then
  mkdir -p pkg
  cp "$here/clanghello.inf" pkg/ && cp "$last" pkg/clanghello.sys
  if { ${PYTHON:-python3} "$sign" catalog --cert test.pem \
         -o pkg/clanghello.cat pkg/clanghello.inf pkg/clanghello.sys &&
       ${PYTHON:-python3} "$sign" verify --cert test.pem pkg/clanghello.cat
     } >pkg.log 2>&1
  then pass=$((pass + 1)); else bad "catalog" pkg.log; fi
fi
# WDM drivers for Windows 98, 98 SE and ME: hello.c, built for NT 4.0's
# subsystem version, uses only what their WDM has; notwdm.c does not.
if [ -f "$CFGDIR/nt4-ddk.cfg" ] && [ -f nt4-ddk-O2.sys ]; then
  for v in 98 98se me; do
    if ${PYTHON:-python3} "$check" --wdm $v nt4-ddk-O2.sys >wdm-$v.chk 2>&1
    then pass=$((pass + 1)); else bad "wdm $v" wdm-$v.chk; fi
  done
  if ! "$CFGDIR/clang" --config="$CFGDIR/nt4-ddk.cfg" -O2 "$here/notwdm.c" \
       -o notwdm.sys >notwdm.log 2>&1; then
    bad "notwdm build" notwdm.log
  elif ${PYTHON:-python3} "$check" --wdm 98 notwdm.sys >notwdm.chk 2>&1 ||
       ! grep -q "PsGetVersion is not one of the WDM functions" notwdm.chk
  then
    bad "notwdm check-imports" notwdm.chk
  else
    pass=$((pass + 1))
  fi
fi
# A WDM audio driver (wdmaudio.c, port class) for 98, 98 SE and ME, and
# with -DME_ONLY one that imports what only ME's PORTCLS.SYS has.
if [ -f "$CFGDIR/nt4-ddk.cfg" ]; then
  for opt in O0 O2; do
    t=wdmaudio-$opt
    if ! "$CFGDIR/clang" --config="$CFGDIR/nt4-ddk.cfg" -$opt -Wall -Werror \
         "$here/wdmaudio.c" -lportcls -o $t.sys >$t.log 2>&1; then
      bad "$t build" $t.log; continue
    fi
    for v in 98 98se me; do
      if ${PYTHON:-python3} "$check" --wdm $v $t.sys >$t-$v.chk 2>&1
      then pass=$((pass + 1)); else bad "$t wdm $v" $t-$v.chk; fi
    done
  done
  if ! "$CFGDIR/clang" --config="$CFGDIR/nt4-ddk.cfg" -O2 -DME_ONLY \
       "$here/wdmaudio.c" -lportcls -o wdmaudio-me.sys >wdmaudio-me.log 2>&1
  then
    bad "wdmaudio-me build" wdmaudio-me.log
  elif ${PYTHON:-python3} "$check" --wdm 98se wdmaudio-me.sys \
         >wdmaudio-me.chk 2>&1 ||
       ! grep -q "PcRegisterIoTimeout is in the WDM of ME" wdmaudio-me.chk ||
       ! ${PYTHON:-python3} "$check" --wdm me wdmaudio-me.sys \
         >>wdmaudio-me.chk 2>&1
  then
    bad "wdmaudio-me check-imports" wdmaudio-me.chk
  else
    pass=$((pass + 1))
  fi
fi
# WDM drivers for 98, 98 SE and ME: usbdrv.c with -lusbd9x (and with
# mingw-w64's -lusbd, which 98 SE's USBD.SYS cannot load); hidparse.c,
# also with -DSE_ONLY, which only 98 SE and ME can load; hidmini.c, a HID
# minidriver (<hidport.h>, -lhidclass); fw1394.c, a 1394 client (<1394.h>);
# streammini.c, a stream class minidriver (-lstream), also with -DME_ONLY,
# which only ME can load; avstream.c, an AVStream minidriver (-lks),
# which 98 SE and ME can load, also with -DME_ONLY (ME's stream
# pointers); and ndismini.c, an NDIS 5 miniport (-lndis),
# checked against 98 SE's NDIS.VXD (98 and ME have no list, so they only
# warn), also with -DXP_ONLY, which 98 SE cannot load.
wdm_check() { # NAME FILE.sys VERSIONS-THAT-LOAD-IT VERSIONS-THAT-DO-NOT
  t=$1 sys=$2
  ok=1
  for v in $3; do
    ${PYTHON:-python3} "$check" --wdm $v $sys >>$t.chk 2>&1 || ok=0
  done
  for v in $4; do
    ! ${PYTHON:-python3} "$check" --wdm $v $sys >>$t.chk 2>&1 || ok=0
  done
  if [ $ok = 1 ]; then pass=$((pass + 1)); else bad "$t wdm" $t.chk; fi
}
if [ -f "$CFGDIR/nt4-ddk.cfg" ]; then
  for t in usbdrv:-lusbd9x:"98 98se me": usbdrv-mingw:-lusbd:me:"98 98se" \
           hidparse:-lhidparse:"98 98se me": \
           "hidparse-se:-lhidparse -DSE_ONLY:98se me:98" \
           hidmini:-lhidclass:"98 98se me": fw1394::"98 98se me": \
           streammini:-lstream:"98 98se me": \
           "streammini-me:-lstream -DME_ONLY:me:98 98se" \
           ndismini:-lndis:"98 98se me": "ndismini-xp:-lndis -DXP_ONLY::98se" \
           avstream:-lks:"98se me":98 "avstream-me:-lks -DME_ONLY:me:98 98se"; do
    n=${t%%:*} r=${t#*:}
    args=${r%%:*} r=${r#*:}
    yes=${r%%:*} no=${r#*:}
    rm -f $n.chk
    if "$CFGDIR/clang" --config="$CFGDIR/nt4-ddk.cfg" -O2 -Wall -Werror \
         "$here/${n%%-*}.c" $args -o $n.sys >$n.log 2>&1; then
      wdm_check $n $n.sys "$yes" "$no"
    else
      bad "$n build" $n.log
    fi
  done
fi
# usbdrv.sys's .SYM file (mapsym.py, from its COFF symbol table): its
# functions, the static one too, in .text.
if [ -f usbdrv.sys ]; then
  if ${PYTHON:-python3} "$(dirname "$check")/mapsym.py" usbdrv.sys \
       -o usbdrv.sym >usbdrv-sym.log 2>&1 &&
     ${PYTHON:-python3} "$(dirname "$check")/mapsym.py" --dump usbdrv.sym \
       >>usbdrv-sym.log 2>&1 &&
     grep -q "^module USBDRV$" usbdrv-sym.log &&
     grep -q "^segment 1 .text:" usbdrv-sym.log &&
     grep -q "^  0001:[0-9a-f]* _DriverEntry@8$" usbdrv-sym.log &&
     grep -q "^  0001:[0-9a-f]* _Unload@4$" usbdrv-sym.log; then
    pass=$((pass + 1))
  else
    bad "usbdrv sym" usbdrv-sym.log
  fi
fi
# With -g, its .SYM file has usbdrv.c's line numbers (from the image's
# DWARF), one of them where DriverEntry starts.
if [ -f "$CFGDIR/nt4-ddk.cfg" ]; then
  if "$CFGDIR/clang" --config="$CFGDIR/nt4-ddk.cfg" -O2 -g -Wall -Werror \
       "$here/usbdrv.c" -lusbd9x -o usbdrv-g.sys >usbdrv-g.log 2>&1 &&
     ${PYTHON:-python3} "$(dirname "$check")/mapsym.py" usbdrv-g.sys \
       >>usbdrv-g.log 2>&1 &&
     ${PYTHON:-python3} "$(dirname "$check")/mapsym.py" --dump --lines \
       usbdrv-g.sym >>usbdrv-g.log 2>&1 &&
     grep -q "^  lines .*usbdrv\.c: [1-9]" usbdrv-g.log &&
     at=$(grep " _DriverEntry@8\$" usbdrv-g.log | awk '{print $1}') &&
     grep -q "^    $at [1-9]" usbdrv-g.log; then
    pass=$((pass + 1))
  else
    bad "usbdrv-g sym lines" usbdrv-g.log
  fi
fi
# A display driver (display.c, win32k.sys's DDI) for NT 4.0 and later,
# with a DirectDraw and Direct3D HAL, also with -DNT5_ONLY (EngAlphaBlend),
# which NT 4.0 cannot load; and the layouts of the generated headers
# (hidport.h, 1394.h, dvp.h, ddrawint.h, d3dnthal.h) against the 2000 and
# XP DDKs.
if [ -f "$CFGDIR/nt4-ddk.cfg" ]; then
  for t in display::"4.0 5.0": "display-nt5:-DNT5_ONLY:5.0:4.0"; do
    n=${t%%:*} r=${t#*:}
    args=${r%%:*} r=${r#*:}
    yes=${r%%:*} no=${r#*:}
    ok=1
    rm -f $n.chk
    if "$CFGDIR/clang" --config="$CFGDIR/nt4-ddk.cfg" -O2 -Wall -Werror \
         -shared -Wl,--entry,_DrvEnableDriver@12 "$here/display.c" $args \
         -lwin32k -o $n.dll >$n.log 2>&1; then
      for v in $yes; do
        ${PYTHON:-python3} "$check" --nt $v $n.dll >>$n.chk 2>&1 || ok=0
      done
      for v in $no; do
        ! ${PYTHON:-python3} "$check" --nt $v $n.dll >>$n.chk 2>&1 || ok=0
      done
      if [ $ok = 1 ]; then pass=$((pass + 1)); else bad "$n nt" $n.chk; fi
    else
      bad "$n build" $n.log
    fi
  done
  for l in "$here"/wdm-layout/*.c; do
    t=wdm-layout-$(basename "$l" .c)
    for std in "clang -std=c99" "clang -std=c17" "clang -std=c23" \
               "clang++ -x c++"; do
      if "$CFGDIR/${std%% *}" --config="$CFGDIR/nt4-ddk.cfg" -U_WIN32_WINNT \
           ${std#* } -Wall -Werror -fsyntax-only "$l" >$t.log 2>&1
      then pass=$((pass + 1)); else bad "$t ${std#* }" $t.log; fi
    done
  done
fi
# WinDbg symbols: with -g -gcodeview and --pdb, lld writes a PDB, and
# the image's debug directory names it (only its name, with
# -pdbaltpath:%_PDB%, so that WinDbg looks for it on its symbol path).
# llvm-symbolizer reads the PDB to find the entry point's function and
# source line, as WinDbg would.
for c in nt4-ddk win10-x64-ddk; do
  [ -f "$CFGDIR/$c.cfg" ] || continue
  t=pdb-$c
  if "$CFGDIR/clang" --config="$CFGDIR/$c.cfg" -O2 -g -gcodeview \
       "$here/hello.c" -o $t.sys -Wl,--pdb=$t.pdb \
       -Wl,--Xlink=-pdbaltpath:%_PDB% >$t.log 2>&1 &&
     "$CFGDIR/llvm-readobj" --file-headers --coff-debug-directory $t.sys \
       >>$t.log 2>&1 &&
     grep -q "PDBFileName: $t.pdb\$" $t.log &&
     base=$(awk '/ImageBase:/ {print $2; exit}' $t.log) &&
     entry=$(awk '/AddressOfEntryPoint:/ {print $2; exit}' $t.log) &&
     "$CFGDIR/llvm-symbolizer" --obj=$t.sys --pdb=$t.pdb \
       $(printf '0x%x' $((base + entry))) >$t.sym 2>&1 &&
     cat $t.sym >>$t.log &&
     sed -n 1p $t.sym | grep -q "^DriverEntry$" &&
     sed -n 2p $t.sym | grep -q "hello\.c:[1-9]"; then
    pass=$((pass + 1))
  else
    bad "$t" $t.log
  fi
done
# SCSI miniports for Windows 9x (win95-miniport.cfg): miniport.c at each
# level, checked to import only what SCSIPORT.PDR has, as a native
# subsystem 4.0 image, and the layouts of the miniport headers.
if [ -f "$CFGDIR/win95-miniport.cfg" ]; then
  for opt in O0 O2 Os; do
    t=miniport-$opt
    if ! "$CFGDIR/clang" --config="$CFGDIR/win95-miniport.cfg" -$opt -Wall \
         -Wextra -Werror "$here/miniport.c" -o $t.mpd >$t.log 2>&1; then
      bad "$t build" $t.log
    elif ! ${PYTHON:-python3} "$check" --miniport $t.mpd >$t.chk 2>&1; then
      bad "$t check-imports" $t.chk
    else
      pass=$((pass + 1))
    fi
  done
  for l in "$here"/miniport-layout/*.c; do
    t=layout-$(basename "$l" .c)
    for cc in clang clang++; do
      x=; [ $cc = clang++ ] && x="-x c++"
      if "$CFGDIR/$cc" --config="$CFGDIR/win95-miniport.cfg" $x -Wall \
           -Werror -Wsystem-headers -fsyntax-only "$l" >$t.log 2>&1
      then pass=$((pass + 1)); else bad "$t $cc" $t.log; fi
    done
  done
fi
echo "ddk tests: $pass passed, $fail failed, $skip not run in Wine"
[ $fail = 0 ]
