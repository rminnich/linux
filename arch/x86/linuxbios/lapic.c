/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/console.h>
#include <asm/io.h>
#include <asm/msr.h>
#include "ioapic.h"
#include "lapic.h"

void *lapic_base;

void post_code(uint8_t code)
{
	outb(code, 0x80);
}

void setup_lapic(void)
{
	u64 msr;
	extern void *lapic_base;

	lapic_base = ioremap(LAPIC_DEFAULT_BASE, 4096);
	if (! lapic_base) {
		early_printk("remap for lapic_base failed\n");
		return;
	}
	/* this is so interrupts work. This is very limited scope --
	 * linux will do better later, we hope ...
	 */
	/* this is the first way we learned to do it. It fails on real SMP
	 * stuff. So we have to do things differently ...
	 * see the Intel mp1.4 spec, page A-3
	 */

	/* Only Pentium Pro and later have those MSR stuff */

	early_printk("Setting up local APIC...");
	
	/* Enable the local APIC */
	rdmsrl(LAPIC_BASE_MSR, msr);
	msr |= LAPIC_BASE_MSR_ENABLE;
	msr &= ~LAPIC_BASE_MSR_ADDR_MASK;
	msr |= LAPIC_DEFAULT_BASE;
	wrmsrl(LAPIC_BASE_MSR, msr);

	/*
	 * Set Task Priority to 'accept all'.
	 */
	lapic_write_around(LAPIC_TASKPRI,
		lapic_read_around(LAPIC_TASKPRI) & ~LAPIC_TPRI_MASK);

	/* Put the local APIC in virtual wire mode */
	lapic_write_around(LAPIC_SPIV,
		(lapic_read_around(LAPIC_SPIV) & ~(LAPIC_VECTOR_MASK))
		| LAPIC_SPIV_ENABLE);
	lapic_write_around(LAPIC_LVT0,
		(lapic_read_around(LAPIC_LVT0) &
			~(LAPIC_LVT_MASKED | LAPIC_LVT_LEVEL_TRIGGER |
				LAPIC_LVT_REMOTE_IRR | LAPIC_INPUT_POLARITY |
				LAPIC_SEND_PENDING | LAPIC_LVT_RESERVED_1 |
				LAPIC_DELIVERY_MODE_MASK))
		| (LAPIC_LVT_REMOTE_IRR | LAPIC_SEND_PENDING |
			LAPIC_DELIVERY_MODE_EXTINT)
		);
	lapic_write_around(LAPIC_LVT1,
		(lapic_read_around(LAPIC_LVT1) &
			~(LAPIC_LVT_MASKED | LAPIC_LVT_LEVEL_TRIGGER |
				LAPIC_LVT_REMOTE_IRR | LAPIC_INPUT_POLARITY |
				LAPIC_SEND_PENDING | LAPIC_LVT_RESERVED_1 |
				LAPIC_DELIVERY_MODE_MASK))
		| (LAPIC_LVT_REMOTE_IRR | LAPIC_SEND_PENDING |
			LAPIC_DELIVERY_MODE_NMI)
		);

	early_printk(" apic_id: 0x%02lx ", lapicid());

	early_printk("done.\n");
	post_code(0x9b);
}
