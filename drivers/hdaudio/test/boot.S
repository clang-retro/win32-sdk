/* A multiboot (version 1) entry for hdatest: QEMU's -kernel loads it in
   32-bit protected mode with paging off. */
	.section .multiboot, "a"
	.align 4
	.long 0x1BADB002, 0, -0x1BADB002
	.text
	.globl _start
_start:
	mov $stack_top, %esp
	call main
1:	cli
	hlt
	jmp 1b
	.bss
	.align 16
	.space 65536
stack_top:
