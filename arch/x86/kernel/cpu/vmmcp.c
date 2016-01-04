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
	printk("%s: MIRACLE\n", __func__);
	set_bit(data->irq, vmmcp_data.blocked_interrupts);
}

static void enable_vmmcp_irq(struct irq_data *data)
{
	printk("%s: MIRACLE\n", __func__);
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
	return false;
}

const __refconst struct hypervisor_x86 x86_hyper_vmmcp = {
	.name			= "Vmmcp",
	.detect			= vmmcp_platform,
	.set_cpu_features	= vmmcp_set_cpu_features,
	.init_platform		= vmmcp_platform_setup,
	.x2apic_available	= vmmcp_legacy_x2apic_available,
};
EXPORT_SYMBOL(x86_hyper_vmmcp);
