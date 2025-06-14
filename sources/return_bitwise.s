	.globl main
	.text
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16,	%rsp
	;start register parameters
	;start stack parameters
	;end function parameters
	;unary_instruction
	movl $12, -4(%rbp)
	notl	-4(%rbp)
	;return_instruction
	movl -4(%rbp), %eax
	movq	%rbp, %rsp
	popq	%rbp
	ret
	;return_instruction
	movl $0, %eax
	movq	%rbp, %rsp
	popq	%rbp
	ret
	.section .note.GNU-stack,"",@progbits
