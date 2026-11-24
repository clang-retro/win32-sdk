#!/bin/sh
# Add GCC's C++ library (libstdc++ and libsupc++) to a Windows sysroot, next
# to libc++. The library is compiled by clang, not by GCC. Programs select it
# with -stdlib=libstdc++.
#
#   GCC_SRC   unpacked GCC release (gcc-15.2.0); only libstdc++-v3, libgcc,
#             include and config are used
#   BIN       directory with clang, clang++, llvm-ar, llvm-ranlib, llvm-nm
#   SYSROOT   sysroot from build-win9x.sh or build-modern-sysroot.sh, with
#             libunwind (build-cxx.sh or build-modern-sysroot.sh)
#   ARCH      i686 (default) or x86_64
#   CPU       -march for the library (default: i686 for i686, none for x86_64)
#   WINNT     _WIN32_WINNT for the library (default: the sysroot's). Below
#             0x0600 (Vista), the thread functions are gthr-win32-compat.c,
#             which works from Windows 95 and NT 3.1; at 0x0600 and above,
#             they are GCC's. Below 0x0501 (XP), the library does without
#             what 2000 and older lack (see libstdcxx/gcc-*.patch).
#   WORK      build directory (default: ./build-libstdcxx)
#
# The library is static. Exceptions use the unwinder of the sysroot
# (libunwind, which is also its libgcc_eh.a): DWARF for i686, SEH for
# x86_64. The thread functions are put in libstdc++.a, because the sysroot
# has compiler-rt in place of libgcc. docs/libstdcxx.md tells more.
#
# The files are installed as a GCC installation in the sysroot, where clang
# finds them:
#   $SYSROOT/include/c++/VERSION/          headers
#   $SYSROOT/lib/gcc/mingw32/VERSION/      libstdc++.a, libsupc++.a
#
# Use it with:
#   clang++ --target=i686-pc-windows5.1-gnu --sysroot=$SYSROOT \
#     -fuse-ld=lld -stdlib=libstdc++ hello.cpp -o hello.exe
set -e
: "${GCC_SRC:?set GCC_SRC}" "${BIN:?set BIN}" "${SYSROOT:?set SYSROOT}"
ARCH=${ARCH:-i686}
case $ARCH in
i686) CPU=${CPU-i686} ;;
x86_64) CPU=${CPU-} ;;
*) echo "unknown ARCH $ARCH" >&2; exit 1 ;;
esac
WORK=${WORK:-$PWD/build-libstdcxx}
mkdir -p "$WORK"
WORK=$(cd "$WORK" && pwd)
SYSROOT=$(cd "$SYSROOT" && pwd)
GCC_SRC=$(cd "$GCC_SRC" && pwd)
here=$(cd "$(dirname "$0")" && pwd)

GNU_TRIPLE=$ARCH-w64-mingw32
TARGET=$ARCH-w64-windows-gnu
VERSION=$(cat "$GCC_SRC/gcc/BASE-VER")
# clang looks in lib/gcc/mingw32 for every -march (i386-, i586- and
# i686-w64-mingw32 are the other names it tries), and for the target's
# headers in include/c++/VERSION/mingw32.
GCCLIB=$SYSROOT/lib/gcc/mingw32/$VERSION

# The changes to GCC's sources for clang and for Windows before Vista,
# made once.
if [ ! -f "$GCC_SRC/.clang-retro-patched" ]; then
  patch -d "$GCC_SRC" -p1 -N <"$here/libstdcxx/gcc-$VERSION.patch"
  touch "$GCC_SRC/.clang-retro-patched"
fi

# The sysroot's _WIN32_WINNT, from the default of its _mingw.h.
if [ -z "$WINNT" ]; then
  WINNT=$(sed -n 's/^#define _WIN32_WINNT \(0x[0-9a-fA-F]*\).*/\1/p' \
    "$SYSROOT/include/_mingw.h" | head -n 1)
fi
: "${WINNT:?cannot find the sysroot's _WIN32_WINNT; set WINNT}"

# Emulated TLS before Vista, as build-cxx.sh: thread_local in a DLL that
# is loaded at run time does not work with native TLS there.
# The library itself does not use thread_local there (--disable-tls): its
# exception globals are kept with TlsAlloc, which still works after
# compiler-rt's emulated TLS is freed at exit, when the standard streams
# are flushed.
TLS=-femulated-tls CONF_TLS=--disable-tls
[ $((WINNT)) -ge $((0x0600)) ] && TLS= CONF_TLS=
FLAGS="${CPU:+-march=$CPU} $TLS -D_WIN32_WINNT=$WINNT"

# libstdc++ asks for the init_priority values that GCC keeps for itself
# (the standard streams are made first), and for GCC's warnings.
WARN="-Wno-error=init-priority-reserved -Wno-init-priority-reserved"
WARN="$WARN -Wno-unknown-warning-option -Wno-unknown-attributes"
WARN="$WARN -Wno-mismatched-tags -Wno-invalid-constexpr"
# clang turns sized deallocation off for MinGW, but the library calls the
# sized operator delete.
WARN="$WARN -fsized-deallocation"

# The compiler commands. libstdc++'s configure reads the thread model from
# "$CXX -v", and clang always says posix; the wrapper says win32. clang does
# not use the sysroot's libc++ headers or link a C++ library here.
mkdir -p "$WORK/bin"
for tool in cc cxx; do
  case $tool in
  cc) drv=clang extra= ;;
  cxx) drv=clang++ extra="-nostdinc++ -nostdlib++" ;;
  esac
  cat >"$WORK/bin/$GNU_TRIPLE-$tool" <<EOF
#!/bin/sh
if [ "\$*" = -v ]; then
  "$BIN/$drv" --target=$TARGET --sysroot="$SYSROOT" -v 2>&1 |
    sed 's/^Thread model: .*/Thread model: win32/' >&2
  exit 0
fi
exec "$BIN/$drv" --target=$TARGET --sysroot="$SYSROOT" -fuse-ld=lld \\
  $extra "\$@"
EOF
  chmod +x "$WORK/bin/$GNU_TRIPLE-$tool"
done

# The thread functions. From Vista, GCC's (libgcc's gthr-win32*.c); below
# Vista, gthr-win32-compat.c, which works on every version.
# __GTHREAD_WIN32_COMPAT tells the headers that the library has condition
# variables, and so std::thread, below Vista.
cd "$WORK"
rm -rf gthr gthr-inc build
mkdir -p gthr gthr-inc
if [ $((WINNT)) -ge $((0x0600)) ]; then
  GTHR=
  for f in gthr-win32.c gthr-win32-cond.c gthr-win32-thread.c; do
    "$WORK/bin/$GNU_TRIPLE-cc" $FLAGS -O2 -I"$GCC_SRC/libgcc" \
      -c "$GCC_SRC/libgcc/config/i386/$f" -o "gthr/${f%.c}.o"
  done
else
  GTHR=-D__GTHREAD_WIN32_COMPAT=1
  "$WORK/bin/$GNU_TRIPLE-cc" $FLAGS -O2 -I"$GCC_SRC/libgcc" \
    -I"$GCC_SRC/libgcc/config/i386" \
    -c "$here/libstdcxx/gthr-win32-compat.c" -o gthr/gthr-win32-compat.o
fi
# configure looks for gthr-default.h next to libgcc's gthr.h, where a GCC
# build makes it.
cp "$GCC_SRC/libgcc/config/i386/gthr-win32.h" gthr-inc/gthr-default.h
GTHR="$GTHR -D__GTHREAD_HIDE_WIN32API=1 -I$WORK/gthr-inc"

mkdir -p build
cd build
# --host is GCC's name for the target, for configure's mingw-w64 settings.
"$GCC_SRC/libstdc++-v3/configure" \
  --build="$("$GCC_SRC/config.guess")" --host=$GNU_TRIPLE \
  --prefix=/usr --with-gxx-include-dir=/include/c++/$VERSION \
  --disable-shared --enable-static --disable-multilib --disable-nls \
  --disable-libstdcxx-pch --enable-threads=win32 $CONF_TLS \
  CC="$WORK/bin/$GNU_TRIPLE-cc" CXX="$WORK/bin/$GNU_TRIPLE-cxx" \
  AR="$BIN/llvm-ar" RANLIB="$BIN/llvm-ranlib" NM="$BIN/llvm-nm" \
  CFLAGS="$FLAGS -O2" CXXFLAGS="$FLAGS -O2 $WARN $GTHR" \
  >"$WORK/configure.log" 2>&1 || {
  tail -n 40 config.log >&2
  exit 1
}
make -j"$(nproc)" >"$WORK/build.log" 2>&1 || {
  grep -n -B2 -A8 'error' "$WORK/build.log" | head -n 80 >&2
  exit 1
}
rm -rf "$WORK/stage"
make install DESTDIR="$WORK/stage" >"$WORK/install.log" 2>&1
mkdir -p "$GCCLIB" "$SYSROOT/include/c++"
rm -rf "$SYSROOT/include/c++/$VERSION"
cp -R "$WORK/stage/include/c++/$VERSION" "$SYSROOT/include/c++/"
mv "$SYSROOT/include/c++/$VERSION/$GNU_TRIPLE" \
  "$SYSROOT/include/c++/$VERSION/mingw32"
if [ $((WINNT)) -lt $((0x0600)) ]; then
  hdr=$SYSROOT/include/c++/$VERSION/mingw32/bits/gthr-default.h
  sed '/^#define GCC_GTHR_WIN32_H$/a\
/* This libstdc++ has the threads of gthr-win32-compat.c (clang-retro).  */\
#define __GTHREAD_WIN32_COMPAT 1' "$hdr" >"$hdr.new"
  grep -q '^#define __GTHREAD_WIN32_COMPAT 1' "$hdr.new"
  mv "$hdr.new" "$hdr"
fi
cp "$WORK"/stage/usr/lib/*.a "$GCCLIB/"
"$BIN/llvm-ar" rs "$GCCLIB/libstdc++.a" "$WORK"/gthr/*.o

# The licences: GPL 3 with the GCC Runtime Library Exception.
lic=$SYSROOT/share/licenses/libstdc++
mkdir -p "$lic"
cp "$GCC_SRC/COPYING3" "$GCC_SRC/COPYING.RUNTIME" "$lic/"

echo "libstdc++ $VERSION installed in $SYSROOT"
