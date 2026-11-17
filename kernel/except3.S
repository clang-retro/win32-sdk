/* _except_handler3, the handler of the frames a 32-bit x86 driver's SEH
   __try blocks register (the scope tables of cl and of clang, which uses
   the same layout), for drivers built with the *-ddk.cfg config files.
   Windows 2000's kernel and later ones export it, NT 3.1 to 4.0's export
   only _except_handler2, and NTKERN gives 98 and ME's WDM drivers neither:
   Microsoft's DDKs linked it from their C library, and libntseh.a does
   that for these configs. It needs only RtlUnwind from the kernel.

   The frame, at REG (fs:0 points to it):
     REG-8   the ESP of the function, for its __except blocks
     REG-4   the EXCEPTION_POINTERS of the exception being handled
     REG+0   the next frame
     REG+4   _except_handler3
     REG+8   the scope table
     REG+12  the try level: the index of the innermost __try the code is in,
             or -1
   and the function's EBP is REG+16. Each scope table entry is
     +0  the enclosing try level
     +4  the filter, or 0 for a __finally
     +8  the __except block, or the __finally funclet
   Filters and __finally funclets are called with EBP the function's, the
   __except block jumped to with it.

   It works as cl's does (Matt Pietrek, "A Crash Course on the Depths of
   Win32 Structured Exception Handling", MSJ, January 1997): while the
   exception is dispatched, it calls the filters from the innermost __try
   out; a filter's EXCEPTION_EXECUTE_HANDLER (1) unwinds the frames above
   (RtlUnwind), runs the __finally blocks inside that __try and jumps to
   its __except block, EXCEPTION_CONTINUE_EXECUTION (-1) resumes, and 0
   asks the next one. While a frame is unwound, it runs its __finally
   blocks. */

	.text
	.globl	__except_handler3
	.def	__except_handler3; .scl 2; .type 32; .endef
__except_handler3:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$8, %esp		/* EXCEPTION_POINTERS */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	cld
	movl	12(%ebp), %ebx		/* REG */
	movl	8(%ebp), %eax		/* EXCEPTION_RECORD */
	testl	$6, 4(%eax)		/* EXCEPTION_UNWINDING | EXIT_UNWIND */
	jnz	.Lunwinding
	movl	%eax, -8(%ebp)
	movl	16(%ebp), %ecx		/* CONTEXT */
	movl	%ecx, -4(%ebp)
	leal	-8(%ebp), %ecx
	movl	%ecx, -4(%ebx)
	movl	12(%ebx), %esi		/* try level */
.Lfilter:
	cmpl	$-1, %esi
	je	.Lsearch
	movl	8(%ebx), %edi
	leal	(%esi,%esi,2), %ecx
	leal	(%edi,%ecx,4), %edi	/* its scope table entry */
	movl	4(%edi), %eax
	testl	%eax, %eax
	jz	.Lnext			/* a __finally */
	pushl	%ebp
	leal	16(%ebx), %ebp
	call	*%eax
	popl	%ebp
	testl	%eax, %eax
	js	.Lresume
	jg	.Lexecute
.Lnext:
	movl	(%edi), %esi
	jmp	.Lfilter
.Lsearch:
	movl	$1, %eax		/* ExceptionContinueSearch */
	jmp	.Lreturn
.Lresume:
	xorl	%eax, %eax		/* ExceptionContinueExecution */
.Lreturn:
	popl	%edi
	popl	%esi
	popl	%ebx
	movl	%ebp, %esp
	popl	%ebp
	ret
.Lunwinding:
	pushl	$-1
	pushl	%ebx
	call	__ntseh_local_unwind
	addl	$8, %esp
	jmp	.Lsearch
.Lexecute:
	/* Unwind the frames above this one: their handlers run their
	   __finally blocks. */
	pushl	$0
	pushl	8(%ebp)
	pushl	$0
	pushl	%ebx
	call	_RtlUnwind@16
	/* Then this function's __finally blocks inside the __try. */
	pushl	(%edi)
	pushl	%ebx
	call	__ntseh_local_unwind
	addl	$8, %esp
	movl	(%edi), %eax
	movl	%eax, 12(%ebx)
	movl	8(%edi), %eax
	leal	16(%ebx), %ebp
	jmp	*%eax

/* Run the __finally blocks of the frame at REG from its try level out to
   STOP (not including it), each with the try level set to the enclosing
   one first, so that an exception in one does not run it again. */
	.globl	__ntseh_local_unwind
	.def	__ntseh_local_unwind; .scl 2; .type 32; .endef
__ntseh_local_unwind:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	20(%esp), %ebx		/* REG */
.Llocal:
	movl	12(%ebx), %esi
	cmpl	$-1, %esi
	je	.Llocal_done
	cmpl	24(%esp), %esi		/* STOP */
	je	.Llocal_done
	movl	8(%ebx), %edi
	leal	(%esi,%esi,2), %ecx
	leal	(%edi,%ecx,4), %edi
	movl	(%edi), %eax
	movl	%eax, 12(%ebx)
	cmpl	$0, 4(%edi)
	jne	.Llocal
	movl	8(%edi), %eax
	pushl	%ebp
	leal	16(%ebx), %ebp
	call	*%eax
	popl	%ebp
	jmp	.Llocal
.Llocal_done:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
