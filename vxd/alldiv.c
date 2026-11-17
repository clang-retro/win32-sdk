/* libvxd.a: the 64-bit division helpers that clang calls for the
   Microsoft target (i386-pc-windows-msvc, the configs made from
   Microsoft's VxD DDKs), which take their operands on the stack and pop
   them: _alldiv, _aulldiv, _allrem and _aullrem, on top of libgcc's
   __divdi3 and the like.

   SPDX-License-Identifier: AGPL-3.0-only
   Copyright (C) clang-retro contributors. */
#define VXD_MSVC_DIV(NAME, GCC)                                             \
  __asm__(".text\n.globl " NAME "\n" NAME ":\n"                            \
          "  pushl 16(%esp)\n  pushl 16(%esp)\n"                            \
          "  pushl 16(%esp)\n  pushl 16(%esp)\n"                            \
          "  call " GCC "\n  addl $16, %esp\n  ret $16\n")

VXD_MSVC_DIV("__alldiv", "___divdi3");
VXD_MSVC_DIV("__aulldiv", "___udivdi3");
VXD_MSVC_DIV("__allrem", "___moddi3");
VXD_MSVC_DIV("__aullrem", "___umoddi3");
