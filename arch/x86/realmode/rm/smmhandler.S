/*
 *
 *	smmhandler.S	Derived from Trampoline.S
 *	
 *	There are enough important differences with trampoline
 *	that we needed to fork and change this. SMM is finicky
 *	and we don't want to have weird breakages.
 *	The biggest difference is that this needs to be SMP
 *	safe and that from the point of view of the kernel it
 *	is called asynchronously.
 *
 *	All the other comments from trampoline_64.S apply here too.
 *
 *	If you work on this file, check the object module with objdump
 *	--full-contents --reloc to make sure there are no relocation
 *	entries.
 */

#include <linux/linkage.h>
#include <asm/pgtable_types.h>
#include <asm/page_types.h>
#include <asm/msr.h>
#include <asm/segment.h>
#include <asm/processor-flags.h>
#include "realmode.h"

	.text
	.code16

	.balign	PAGE_SIZE
ENTRY(smm_start)
	rsm
	cli			# We should be safe anyway
	wbinvd

	LJMPW_RM(1f)
1:
	mov	%cs, %ax	# Code and data in the same place
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss

	movl	$0xA5A5A5A5, smm_status
	# write marker for master knows we're running

	# N.B. compiling this code in depends on long mode.
	# Hence we do not verify that we're on x86_64 here.
	# That would add a cpuid to the smm path and we don't
	# want to do that.
	# Setup stack
	# TODO- per core stacks here.
	movl	$rm_stack_end, %esp

	/*
	 * GDT tables in non default location kernel can be beyond 16MB and
	 * lgdt will not be able to load the address as in real mode default
	 * operand size is 16bit. Use lgdtl instead to force operand size
	 * to 32 bit.
	 */
.global tr_idt
	lidtl	tr_idt	# load idt with 0, 0
	lgdtl	smm_gdt	# load gdt with whatever is appropriate

	movw	$__KERNEL_DS, %dx	# Data segment descriptor

	# Enable protected mode
	movl	$X86_CR0_PE, %eax	# protected mode (PE) bit
	movl	%eax, %cr0		# into protected mode

	# flush prefetch and jump to smm_startup_32
	ljmpl	$__KERNEL32_CS, $pa_smm_startup_32

no_longmode:
	hlt
	jmp no_longmode
#include "../kernel/verify_cpu.S"

	.section ".text32","ax"
	.code32
	.balign 4
ENTRY(smm_startup_32)
#
	movl	%edx, %ss
	addl	$pa_real_mode_base, %esp
	movl	%edx, %ds
	movl	%edx, %es
	movl	%edx, %fs
	movl	%edx, %gs

	movl	pa_tr_cr4, %eax
	movl	%eax, %cr4		# Enable PAE mode

	# Setup smm 4 level pagetables
	movl	$pa_trampoline_pgd, %eax
	movl	%eax, %cr3

	# Set up EFER
	movl	pa_tr_efer, %eax
	movl	pa_tr_efer + 4, %edx
	movl	$MSR_EFER, %ecx
	wrmsr

1: jmp 1b // HERE
	# Enable paging and in turn activate Long Mode
	movl	$(X86_CR0_PG | X86_CR0_WP | X86_CR0_PE), %eax
	movl	%eax, %cr0

	/*
	 * At this point we're in long mode but in 32bit compatibility mode
	 * with EFER.LME = 1, CS.L = 0, CS.D = 1 (and in turn
	 * EFER.LMA = 1). Now we want to jump in 64bit mode, to do that we use
	 * the new gdt/idt that has __KERNEL_CS with CS.L = 1.
	 */
	ljmpl	$__KERNEL_CS, $smm_startup_64

	.section ".text64","ax"
	.code64
	.balign 4
ENTRY(smm_startup_64)
1: jmp 1b //FAIL
	# Now jump into the kernel using virtual addresses
	#callq	*smm_c(%rip)
1: jmp 1b
	rsm

	.section ".rodata","a"
	# Duplicate the global descriptor table
	# so the kernel can live anywhere
	.balign	16
	.globl smm_gdt
smm_gdt:
	.short	smm_gdt_end - smm_gdt - 1	# gdt limit
	.long	pa_smm_gdt
	.short	0
	.quad	0x00cf9b000000ffff	# __KERNEL32_CS
	.quad	0x00af9b000000ffff	# __KERNEL_CS
	.quad	0x00cf93000000ffff	# __KERNEL_DS
smm_gdt_end:

	.bss
GLOBAL(smm_status)	.space	4
