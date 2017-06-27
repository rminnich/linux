/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 ChromeOS Authors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * This is some nasty code, sorry. It is hand-assembled to run at 0xa0000.
 * Now you may have read about all kinds of restrictons around where SMM
 * code can run, and state save area sizing and location, and so on and so forth.
 * Since this SMM is intended to be
 * an integrated part of Linux, not some sort of separate OS in ring -1, guess what:
 * none of the old rules apply. We can be a LOT smarter about using what
 * Linux has to offer. Simple example: we don't need to compute a stack
 * in this code. As long as we save enough state to get back to the rsm,
 * we can do all the nasty stuff later. That means that Linux can allocate
 * the SMM stacks using standard mechanisms; further, it reduces attack
 * surface greatly since the SMM code is no longer at a fixed place, is replaced
 * on boot, and the state save area is also much harder for an attacker to find.
 * Hence, this stub, originally from coreboot, is much reduced. Sorry
 * for the by-hand static linking, we'll be cleaning that up but job 1
 * was to get to something that worked.
 *
 * The stub is a generic wrapper for bootstrapping a C-based SMM handler. Its
 * primary purpose is to put the CPU into protected mode with a stack and call
 * into the C handler.
 */

.text
.align 65536
.globl smmstart
smmstart:
.code16
jmp 9f

.align 4
	/* The first GDT entry is used for the lgdt instruction. */
	.word	0x17
	.long	0xa0004
	.word	0x0000

	/* gdt selector 0x08, flat code segment */
	.word	0xffff, 0x0000
	.byte	0x00, 0x9b, 0xcf, 0x00 /* G=1 and 0x0f, 4GB limit */

	/* gdt selector 0x10, flat data segment */
	.word	0xffff, 0x0000
	.byte	0x00, 0x93, 0xcf, 0x00
.align 0x20
// entry params
4:
.long 0
// stack size
5:
.long 0
// arg
6:
.long 0
// handler
7:
.long 0
/* struct smm_runtime begins here. */
smm_runtime:
smbase:
.long 0
save_state_size:
.long 0
.align 0x100
/* apic_to_cpu_num is a table mapping the default APIC id to CPU num. If the
 * APIC id is found at the given index, the contiguous CPU number is index
 * into the table. */
8:
.fill CONFIG_NR_CPUS,1,0xff
/* end struct smm_runtime */

.align 0x400
.code32
	/* Use flat data segment */
	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

	/* The CPU number is calculated by reading the initial APIC id. Since
	 * the OS can manipulate the APIC id use the non-changing cpuid result
	 * for APIC id (ebx[31:24]). A table is used to handle a discontiguous
	 * APIC id space.  */
	mov	$1, %eax
	cpuid
	bswap	%ebx	/* Default APIC id in bl. */
	mov	$0xa0100, %eax
	xor	%ecx, %ecx
1:
	cmp	(%eax, %ecx, 1), %bl
	je	2f
	inc	%ecx
	cmp	$CONFIG_NR_CPUS, %ecx
	jne	1b
	/* This is bad. One cannot find a stack entry because a CPU num could
	 * not be assigned. Use the fallback stack and check this condition in
	 * C handler. */
	movl	$0xaf000, %esp
2:
	/* Call into the c-based SMM relocation function with the platform
	 * parameters. Equivalent to:
	 *   struct arg = { c_handler_params, cpu_num, smm_runtime {;
	 *   c_handler(&arg)
	 */
	push	$0xa0020
	push	%ecx
	push	0xa0024
	push	%esp
	mov	0xa0028, %eax
	/* TODO: if we want to call into the kernel, we'll need to go to
	 * Long Mode. */
	/* TODO: we need SOMETHING to fill in 0xa0028 with a pointer to C
	 * code! */
	//call	*%eax

	/* Exit from SM mode. */
	rsm
.code16
9:
	movl	$0xa0004, %ebx
	data32	lgdt (%ebx)

	movl	%cr0, %eax
	andl	$0x1FFAFFD1, %eax /* CD,NW,PG,AM,WP,NE,TS,EM,MP = 0 */
	orl	$0x1, %eax        /* PE = 1 */
	movl	%eax, %cr0

	/* Enable protected mode */
	data32	ljmp $0x8, $0xa0400

.code32
smmend:
.globl smmend
