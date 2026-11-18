#!/bin/sh
# hdaudio's tests that need no Windows:
#   graphtest  the codec parsing and output path choice, against a made-up
#              codec with mixers, a selector and several pins (on the host)
#   hdatest    hda.c on a PC that QEMU emulates, with each of its HDA
#              controllers and codecs: it plays a tone, mutes it and turns
#              it down, and checkwav.py checks the WAV file QEMU recorded
#
#   QEMU  optional: qemu-system-i386 (default: on PATH; hdatest is skipped
#         without it)
#   WORK  build folder (default: ./build-hdaudio-tests)
here=$(cd "$(dirname "$0")" && pwd)
out=${WORK:-$PWD/build-hdaudio-tests}
mkdir -p "$out"
QEMU=${QEMU:-qemu-system-i386}
pass=0 fail=0
bad() { fail=$((fail + 1)); echo "FAIL $1"; sed 's/^/    /' "$2" | head -30; }

if ${CC:-cc} -O1 -Wall -Wextra -Werror -DHDA_BARE_METAL \
     -o "$out/graphtest" "$here/graphtest.c" >"$out/graphtest.log" 2>&1 &&
   "$out/graphtest" >>"$out/graphtest.log" 2>&1
then pass=$((pass + 1)); else bad graphtest "$out/graphtest.log"; fi

if ! "$here/build.sh" "$out" >"$out/build.log" 2>&1; then
  bad "hdatest build" "$out/build.log"
elif ! command -v "$QEMU" >/dev/null 2>&1; then
  echo "skip hdatest: no $QEMU"
else
  for ctl in intel-hda ich9-intel-hda; do
    for codec in hda-output hda-duplex hda-micro hda-duplex,mixer=off; do
      t=$ctl-$(echo "$codec" | tr ',=' '--')
      rm -f "$out/$t.wav"
      timeout 120 "$QEMU" -display none -m 64 -serial "file:$out/$t.log" \
        -kernel "$out/hdatest.elf" -device isa-debug-exit,iobase=0xf4,iosize=4 \
        -audiodev wav,id=snd,path="$out/$t.wav" -device $ctl \
        -device $codec,audiodev=snd 2>"$out/$t.err"
      # isa-debug-exit: hdatest's 0 is QEMU's 1.
      if [ $? != 1 ] || ! grep -q '^done' "$out/$t.log"; then
        cat "$out/$t.err" >>"$out/$t.log"; bad "hdatest $t" "$out/$t.log"
        continue
      fi
      ints=$(sed -n 's/^interrupts \([0-9]*\) laps \([0-9]*\).*/\1 \2/p' \
        "$out/$t.log")
      set -- $ints
      # About 2 s of 10 ms fragments in 80 ms buffers (the TSC's idea of
      # 2 s on a busy host can be longer): 8 interrupts a lap, and about
      # 20 in the 200 ms after the restart.
      i=${1:-0} l=${2:-0}
      if [ $l -lt 22 ] || [ $l -gt 32 ] || [ $i -lt $((l * 8 - 8)) ] ||
         [ $i -gt $((l * 8 + 8)) ] ||
         ! grep -q '^restart position 0 .* interrupts [12][0-9]$' "$out/$t.log"
      then
        bad "hdatest $t interrupts" "$out/$t.log"; continue
      fi
      nv=; case $codec in *mixer=off) nv=--no-volume ;; esac
      if ${PYTHON:-python3} "$here/checkwav.py" "$out/$t.wav" $nv \
           >"$out/$t.chk" 2>&1
      then pass=$((pass + 1)); else bad "hdatest $t wav" "$out/$t.chk"; fi
    done
  done
fi
echo "hdaudio tests: $pass passed, $fail failed"
[ $fail = 0 ]
