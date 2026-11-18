#!/bin/sh
# Build hdatest.elf, hda.c on a bare PC, with the host's clang and ld.lld:
#   build.sh OUTDIR
set -e
here=$(cd "$(dirname "$0")" && pwd)
out=${1:?usage: build.sh OUTDIR}
mkdir -p "$out"
CC=${CC:-clang}
LD=${LD:-ld.lld}
flags="--target=i686-elf -march=i686 -ffreestanding -fno-pic -fno-stack-protector -fno-builtin -O2 -Wall -Wextra -DHDA_BARE_METAL"
$CC $flags -c "$here/hdatest.c" -o "$out/hdatest.o"
$CC $flags -c "$here/../hda.c" -o "$out/hda.o"
$CC --target=i686-elf -c "$here/boot.S" -o "$out/boot.o"
$LD -m elf_i386 -T "$here/link.ld" "$out/boot.o" "$out/hdatest.o" \
  "$out/hda.o" -o "$out/hdatest.elf"
