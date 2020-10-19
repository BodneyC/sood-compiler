	.text
	.file	"mod_main"
	.p2align	4, 0x90         # -- Begin function main
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	movq	$10, -8(%rsp)
	movq	$0, -16(%rsp)
	movl	$10, %eax
	testq	%rax, %rax
	jle	.LBB0_3
	.p2align	4, 0x90
.LBB0_2:                                # %while_block
                                        # =>This Inner Loop Header: Depth=1
	movq	-16(%rsp), %rcx
	incq	%rcx
	movq	%rcx, -16(%rsp)
	testq	%rax, %rax
	jg	.LBB0_2
.LBB0_3:                                # %while_aftr
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.Lnumeric_fmt_spc,@object # @numeric_fmt_spc
	.section	.rodata.str1.1,"aMS",@progbits,1
.Lnumeric_fmt_spc:
	.asciz	"%d"
	.size	.Lnumeric_fmt_spc, 3

	.type	.Lstring_fmt_spc,@object # @string_fmt_spc
.Lstring_fmt_spc:
	.asciz	"%s"
	.size	.Lstring_fmt_spc, 3

	.section	".note.GNU-stack","",@progbits
