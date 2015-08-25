/*
 * VMMCP Detection code.
 *
 * Copyright 2015 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <asm/apic.h>
#include <asm/div64.h>
#include <asm/desc.h>
#include <asm/x86_init.h>
#include <asm/hypervisor.h>

// I have no idea what I'm doing. I took this from lguest. 

struct vmmcp_data {
	DECLARE_BITMAP(blocked_interrupts, 64);
};

struct vmmcp_data vmmcp_data = {
	.blocked_interrupts = { 1 }, /* Block timer interrupts */
};
// FROM LGUEST
/*
 * The Unadvanced Programmable Interrupt Controller.
 *
 * This is an attempt to implement the simplest possible interrupt controller.
 * I spent some time looking though routines like set_irq_chip_and_handler,
 * set_irq_chip_and_handler_name, set_irq_chip_data and set_phasers_to_stun and
 * I *think* this is as simple as it gets.
 *
 * We can tell the Host what interrupts we want blocked ready for using the
 * lguest_data.interrupts bitmap, so disabling (aka "masking") them is as
 * simple as setting a bit.  We don't actually "ack" interrupts as such, we
 * just mask and unmask them.  I wonder if we should be cleverer?
 */
static void disable_vmmcp_irq(struct irq_data *data)
{
	set_bit(data->irq, vmmcp_data.blocked_interrupts);
}

static void enable_vmmcp_irq(struct irq_data *data)
{
	clear_bit(data->irq, vmmcp_data.blocked_interrupts);
}

/* This structure describes the vmmcp IRQ controller. */
static struct irq_chip vmmcp_irq_controller = {
	.name		= "vmmcp",
	.irq_mask	= disable_vmmcp_irq,
	.irq_mask_ack	= disable_vmmcp_irq,
	.irq_unmask	= enable_vmmcp_irq,
};

#if 0
static int vmmcp_enable_irq(struct pci_dev *dev)
{
	u8 line = 0;

	/* We literally use the PCI interrupt line as the irq number. */
	pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &line);
	irq_set_chip_and_handler_name(line, &vmmcp_irq_controller,
				      handle_level_irq, "level");
	dev->irq = line;
	return 0;
}

/* We don't do hotplug PCI, so this shouldn't be called. */
static void vmmcp_disable_irq(struct pci_dev *dev)
{
	WARN_ON(1);
}
#endif
/*
 * This sets up the Interrupt Descriptor Table (IDT) entry for each hardware
 * interrupt (except 128, which is used for system calls), and then tells the
 * Linux infrastructure that each interrupt is controlled by our level-based
 * vmmcp interrupt controller.
 */
static void __init vmmcp_init_IRQ(void)
{
	unsigned int i;

	for (i = FIRST_EXTERNAL_VECTOR; i < FIRST_SYSTEM_VECTOR; i++) {
		printk("%s: grab irq %d\n", __func__, i);
		/* Some systems map "vectors" to interrupts weirdly.  Not us! */
		__this_cpu_write(vector_irq[i], i - FIRST_EXTERNAL_VECTOR);
		if (i != IA32_SYSCALL_VECTOR)
			set_intr_gate(i, irq_entries_start +
					8 * (i - FIRST_EXTERNAL_VECTOR));
	}

	/*
	 * This call is required to set up for 4k stacks, where we have
	 * separate stacks for hard and soft interrupts.
	 */
	irq_ctx_init(smp_processor_id());
}

/*
 * Interrupt descriptors are allocated as-needed, but low-numbered ones are
 * reserved by the generic x86 code.  So we ignore irq_alloc_desc_at if it
 * tells us the irq is already used: other errors (ie. ENOMEM) we take
 * seriously.
 */
int vmmcp_setup_irq(unsigned int irq)
{
	int err;
	printk("%s: %d\n", __func__, irq);
	/* Returns -ve error or vector number. */
	err = irq_alloc_desc_at(irq, 0);
	if (err < 0 && err != -EEXIST) {
		printk("%s: ask for %d: err 0x%x, -EEXIST 0x%x\n", __func__, irq, err, -EEXIST);
		return err;
	}

	irq_set_chip_and_handler_name(irq, &vmmcp_irq_controller,
				      handle_level_irq, "level");
	return 0;
}

static void lguest_apic_write(u32 reg, u32 v)
{
}

static u32 lguest_apic_read(u32 reg)
{
	return 0;
}

static u64 lguest_apic_icr_read(void)
{
	return 0;
}

static void lguest_apic_icr_write(u32 low, u32 id)
{
	/* Warn to see if there's any stray references */
	WARN_ON(1);
}

static void lguest_apic_wait_icr_idle(void)
{
	return;
}

static u32 lguest_apic_safe_wait_icr_idle(void)
{
	return 0;
}

static void set_lguest_basic_apic_ops(void)
{
	apic->read = lguest_apic_read;
	apic->write = lguest_apic_write;
	apic->icr_read = lguest_apic_icr_read;
	apic->icr_write = lguest_apic_icr_write;
	apic->wait_icr_idle = lguest_apic_wait_icr_idle;
	apic->safe_wait_icr_idle = lguest_apic_safe_wait_icr_idle;
}

// FROM LGUEST

static inline int __vmmcp_platform(void)
{
	return 1;
}

static unsigned long vmmcp_get_tsc_khz(void)
{
	return 2UL*1024*1048576;
}

static void __init vmmcp_platform_setup(void)
{
	printk("vmmcp platform setup ...\n");
	x86_platform.calibrate_tsc = vmmcp_get_tsc_khz;
	//vmmcp_init_IRQ();
	/* APIC read/write intercepts */
	//set_lguest_basic_apic_ops();
	printk(" DONE vmmcp platform setup ...\n");
}

static uint32_t __init vmmcp_platform(void)
{
	printk("VMMCP platform\n");
	/* sooner or later we need to fix cpuid ... later. */
	return 1<<30;
}

/* no idea ... */
static void vmmcp_set_cpu_features(struct cpuinfo_x86 *c)
{
	set_cpu_cap(c, X86_FEATURE_CONSTANT_TSC);
	set_cpu_cap(c, X86_FEATURE_TSC_RELIABLE);
}

/* Checks if hypervisor supports x2apic without VT-D interrupt remapping. */
static bool __init vmmcp_legacy_x2apic_available(void)
{
	return true;
}

const __refconst struct hypervisor_x86 x86_hyper_vmmcp = {
	.name			= "Vmmcp",
	.detect			= vmmcp_platform,
	.set_cpu_features	= vmmcp_set_cpu_features,
	.init_platform		= vmmcp_platform_setup,
	.x2apic_available	= vmmcp_legacy_x2apic_available,
};
EXPORT_SYMBOL(x86_hyper_vmmcp);
