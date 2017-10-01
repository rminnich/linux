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
1: jmp 1b
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
.align 0x100
.code32
	/* Use flat data segment */
	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

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
	data32	ljmp $0x8, $0xa0100

.code32
smmend:
.globl smmend
