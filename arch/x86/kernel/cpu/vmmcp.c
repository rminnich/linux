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

/* I have not quite worked out what "detection" means for VMMCP mode.
 * For now, if it's compiled in, you're it.
 */
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
	x86_platform.calibrate_tsc = vmmcp_get_tsc_khz;
}

static uint32_t __init vmmcp_platform(void)
{
	printk("VMMCP platform\n");
	return 1<<30;
}

/* This is a best guess until we know what we ought to provide. */
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
