/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
 *               2012 secunet Security Networks AG
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

#include <linux/console.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <asm/pci_x86.h>
#include "rm/smm.h"
#include "rm/chipset/i82801ix.h"
#include "rm/chipset/i82801ixnvs.h"

/* I945/GM45 */
#define SMRAM		0x9d
#define   D_OPEN	(1 << 6)
#define   D_CLS		(1 << 5)
//#define   D_LCK		(1 << 4)
//#define   G_SMRAME	(1 << 3)
#define   D_LCK		(0 << 4)
#define   G_SMRAME	(0 << 3)
#define   C_BASE_SEG	((0 << 2) | (1 << 1) | (0 << 0))

/* While we read PMBASE dynamically in case it changed, let's
 * initialize it with a sane value
 */
static u16 pmbase = DEFAULT_PMBASE;

// This longjmp is used to get us back the relocation code.
// Hence our relocation works whether SMI was set up in firmware
// or not -- assuming, of course, we know SMBASE. For now we assume
// 0xa0000, but that's got to be fixed soon.
static u8 longjmp0x30000x8000[] = { 0xEA, 0x00, 0x80, 0x00, 0x30 };
static u8 longjmpsmmhandler[] = { 0xEA, 0x00, 0x00, 0x00, 0x00 };


/**
 * @brief read and clear PM1_STS
 * @return PM1_STS register
 */
static u16 reset_pm1_status(void)
{
	u16 reg16;

	reg16 = inw(pmbase + PM1_STS);
	/* set status bits are cleared by writing 1 to them */
	outw(reg16, pmbase + PM1_STS);

	return reg16;
}

static void dump_pm1_status(u16 pm1_sts)
{
	printk("PM1_STS: ");
	if (pm1_sts & (1 << 15)) printk("WAK ");
	if (pm1_sts & (1 << 14)) printk("PCIEXPWAK ");
	if (pm1_sts & (1 << 11)) printk("PRBTNOR ");
	if (pm1_sts & (1 << 10)) printk("RTC ");
	if (pm1_sts & (1 <<  8)) printk("PWRBTN ");
	if (pm1_sts & (1 <<  5)) printk("GBL ");
	if (pm1_sts & (1 <<  4)) printk("BM ");
	if (pm1_sts & (1 <<  0)) printk("TMROF ");
	printk("\n");
}

/**
 * @brief read and clear SMI_STS
 * @return SMI_STS register
 */
static u32 reset_smi_status(void)
{
	u32 reg32;

	reg32 = inl(pmbase + SMI_STS);
	/* set status bits are cleared by writing 1 to them */
	outl(reg32, pmbase + SMI_STS);

	return reg32;
}

static void dump_smi_status(u32 smi_sts)
{
	printk("SMI_STS: ");
	if (smi_sts & (1 << 27)) printk("GPIO_UNLOCK ");
	if (smi_sts & (1 << 26)) printk("SPI ");
	if (smi_sts & (1 << 21)) printk("MONITOR ");
	if (smi_sts & (1 << 20)) printk("PCI_EXP_SMI ");
	if (smi_sts & (1 << 18)) printk("INTEL_USB2 ");
	if (smi_sts & (1 << 17)) printk("LEGACY_USB2 ");
	if (smi_sts & (1 << 16)) printk("SMBUS_SMI ");
	if (smi_sts & (1 << 15)) printk("SERIRQ_SMI ");
	if (smi_sts & (1 << 14)) printk("PERIODIC ");
	if (smi_sts & (1 << 13)) printk("TCO ");
	if (smi_sts & (1 << 12)) printk("DEVMON ");
	if (smi_sts & (1 << 11)) printk("MCSMI ");
	if (smi_sts & (1 << 10)) printk("GPI ");
	if (smi_sts & (1 <<  9)) printk("GPE0 ");
	if (smi_sts & (1 <<  8)) printk("PM1 ");
	if (smi_sts & (1 <<  6)) printk("SWSMI_TMR ");
	if (smi_sts & (1 <<  5)) printk("APM ");
	if (smi_sts & (1 <<  4)) printk("SLP_SMI ");
	if (smi_sts & (1 <<  3)) printk("LEGACY_USB ");
	if (smi_sts & (1 <<  2)) printk("BIOS ");
	printk("\n");
}


/**
 * @brief read and clear GPE0_STS
 * @return GPE0_STS register
 */
static u64 reset_gpe0_status(void)
{
	u32 reg_h, reg_l;

	reg_l = inl(pmbase + GPE0_STS);
	reg_h = inl(pmbase + GPE0_STS + 4);
	/* set status bits are cleared by writing 1 to them */
	outl(reg_l, pmbase + GPE0_STS);
	outl(reg_h, pmbase + GPE0_STS + 4);

	return (((u64)reg_h) << 32) | reg_l;
}

static void dump_gpe0_status(u64 gpe0_sts)
{
	int i;
	printk("GPE0_STS: ");
	if (gpe0_sts & (1LL << 32)) printk("USB6 ");
	for (i=31; i>= 16; i--) {
		if (gpe0_sts & (1 << i)) printk("GPIO%d ", (i-16));
	}
	if (gpe0_sts & (1 << 14)) printk("USB4 ");
	if (gpe0_sts & (1 << 13)) printk("PME_B0 ");
	if (gpe0_sts & (1 << 12)) printk("USB3 ");
	if (gpe0_sts & (1 << 11)) printk("PME ");
	if (gpe0_sts & (1 << 10)) printk("EL_SCI/BATLOW ");
	if (gpe0_sts & (1 <<  9)) printk("PCI_EXP ");
	if (gpe0_sts & (1 <<  8)) printk("RI ");
	if (gpe0_sts & (1 <<  7)) printk("SMB_WAK ");
	if (gpe0_sts & (1 <<  6)) printk("TCO_SCI ");
	if (gpe0_sts & (1 <<  5)) printk("USB5 ");
	if (gpe0_sts & (1 <<  4)) printk("USB2 ");
	if (gpe0_sts & (1 <<  3)) printk("USB1 ");
	if (gpe0_sts & (1 <<  2)) printk("SWGPE ");
	if (gpe0_sts & (1 <<  1)) printk("HOT_PLUG ");
	if (gpe0_sts & (1 <<  0)) printk("THRM ");
	printk("\n");
}


/**
 * @brief read and clear ALT_GP_SMI_STS
 * @return ALT_GP_SMI_STS register
 */
static u16 reset_alt_gp_smi_status(void)
{
	u16 reg16;

	reg16 = inl(pmbase + ALT_GP_SMI_STS);
	/* set status bits are cleared by writing 1 to them */
	outl(reg16, pmbase + ALT_GP_SMI_STS);

	return reg16;
}

static void dump_alt_gp_smi_status(u16 alt_gp_smi_sts)
{
	int i;
	printk("ALT_GP_SMI_STS: ");
	for (i=15; i>= 0; i--) {
		if (alt_gp_smi_sts & (1 << i)) printk("GPI%d ", i);
	}
	printk("\n");
}



/**
 * @brief read and clear TCOx_STS
 * @return TCOx_STS registers
 */
static u32 reset_tco_status(void)
{
	u32 tcobase = pmbase + 0x60;
	u32 reg32;

	reg32 = inl(tcobase + 0x04);
	/* set status bits are cleared by writing 1 to them */
	outl(reg32 & ~(1<<18), tcobase + 0x04); //  Don't clear BOOT_STS before SECOND_TO_STS
	if (reg32 & (1 << 18))
		outl(reg32 & (1<<18), tcobase + 0x04); // clear BOOT_STS

	return reg32;
}


static void dump_tco_status(u32 tco_sts)
{
	printk("TCO_STS: ");
	if (tco_sts & (1 << 20)) printk("SMLINK_SLV ");
	if (tco_sts & (1 << 18)) printk("BOOT ");
	if (tco_sts & (1 << 17)) printk("SECOND_TO ");
	if (tco_sts & (1 << 16)) printk("INTRD_DET ");
	if (tco_sts & (1 << 12)) printk("DMISERR ");
	if (tco_sts & (1 << 10)) printk("DMISMI ");
	if (tco_sts & (1 <<  9)) printk("DMISCI ");
	if (tco_sts & (1 <<  8)) printk("BIOSWR ");
	if (tco_sts & (1 <<  7)) printk("NEWCENTURY ");
	if (tco_sts & (1 <<  3)) printk("TIMEOUT ");
	if (tco_sts & (1 <<  2)) printk("TCO_INT ");
	if (tco_sts & (1 <<  1)) printk("SW_TCO ");
	if (tco_sts & (1 <<  0)) printk("NMI2SMI ");
	printk("\n");
}


/**
 * @brief Set the EOS bit
 */
static void smi_set_eos(void)
{
	u8 reg8;

	reg8 = inb(pmbase + SMI_EN);
	reg8 |= EOS;
	outb(reg8, pmbase + SMI_EN);
}

//extern uint8_t smm_relocation_start, smm_relocation_end;

static void smm_relocate(void)
{
	u32 smi_en;
	u16 pm1_en;
	u32 pmb;
	void *v;
	uint32_t pa = real_mode_header->smm_start;
	uint16_t seg, off;

	printk("i80801lix Initializing SMM handler...");

	if (! pa) 
		panic("pa is 0!");

	pci_direct_conf1.read(0, 0, 0x1f, D31F0_PMBASE, 2, &pmb);
	pmbase = (u16) pmb & 0xfffc;

	printk(" ... pmbase = 0x%04x\n", pmbase);

	smi_en = inl(pmbase + SMI_EN);
	if (smi_en & GBL_SMI_EN) {
		printk("SMI# handler already enabled.\n");
	}
	smi_en &= ~GBL_SMI_EN;
	outl(smi_en, pmbase + SMI_EN);

	/* copy the SMM relocation code */
	v = phys_to_virt(0x38000);
	printk("memcopy(%p, %p, %#x)\n", v,
	       __va(real_mode_header->smmreloc_start),
	       real_mode_header->smmreloc_end - real_mode_header->smmreloc_start);

	memcpy(v, __va(real_mode_header->smmreloc_start),
	       16 + (real_mode_header->smmreloc_end - real_mode_header->smmreloc_start));
	wbinvd();

	printk("\n");
	dump_smi_status(reset_smi_status());
	dump_pm1_status(reset_pm1_status());
	dump_gpe0_status(reset_gpe0_status());
	dump_alt_gp_smi_status(reset_alt_gp_smi_status());
	dump_tco_status(reset_tco_status());

	/* Enable SMI generation:
	 *  - on TCO events
	 *  - on APMC writes (io 0xb2)
	 *  - on writes to GBL_RLS (bios commands)
	 * No SMIs:
	 *  - on microcontroller writes (io 0x62/0x66)
	 */

	smi_en = 0; /* reset SMI enables */

	smi_en |= TCO_EN;
	smi_en |= APMC_EN;
#if DEBUG_PERIODIC_SMIS
	/* Set DEBUG_PERIODIC_SMIS in i82801ix.h to debug using
	 * periodic SMIs.
	 */
	smi_en |= PERIODIC_EN;
#endif
	smi_en |= BIOS_EN;

	/* The following need to be on for SMIs to happen */
	smi_en |= EOS | GBL_SMI_EN;

	outl(smi_en, pmbase + SMI_EN);

	pm1_en = 0;
	pm1_en |= PWRBTN_EN;
	pm1_en |= GBL_EN;
	outw(pm1_en, pmbase + PM1_EN);

	/**
	 * There are several methods of raising a controlled SMI# via
	 * software, among them:
	 *  - Writes to io 0xb2 (APMC)
	 *  - Writes to the Local Apic ICR with Delivery mode SMI.
	 *
	 * Using the local apic is a bit more tricky. According to
	 * AMD Family 11 Processor BKDG no destination shorthand must be
	 * used.
	 * The whole SMM initialization is quite a bit hardware specific, so
	 * I'm not too worried about the better of the methods at the moment
	 */

	/* raise an SMI interrupt */
	printk("  ... raise SMI#\n");
	outb(0x00, 0xb2);
	printk("smm relocated. Now change code to point to 0x%x.\n", pa);
	seg = (uint16_t)(pa>>4) & 0xf000;
	off = (uint16_t)pa;
	printk("smm seg is 0x%x, off is 0x%x\n", seg, off);
	longjmpsmmhandler[3] = (uint8_t)seg;
	longjmpsmmhandler[4] = (uint8_t)(seg>>8);
	longjmpsmmhandler[1] = (uint8_t)off;
	longjmpsmmhandler[2] = (uint8_t)(off>>8);
	if (seg == 0)
		panic("seg is 0");
	printk("lj %02x %02x %02x %02x %02x\n", 
		longjmpsmmhandler[0],
		longjmpsmmhandler[1],
		longjmpsmmhandler[2],
		longjmpsmmhandler[3],
		longjmpsmmhandler[4]);
//	longjmpsmmhandler[1] = 0xfe;
//	longjmpsmmhandler[0] = 0xeb;
	memcpy(__va(0xa0000), longjmpsmmhandler, sizeof(longjmpsmmhandler));
	printk("  ... raise SMI#\n");
	outb(0x00, 0xb2);
	
}

static int smm_handler_copied = 0;

static void smm_install(void)
{
	//int i;
	uint8_t b;
	
	/* The first CPU running this gets to copy the SMM handler. But not all
	 * of them. This code works only because when the BSP gets here, the APs are
	 * not running.
	 */
	if (smm_handler_copied)
		return;
	smm_handler_copied = 1;

	/* enable the SMM memory window */
	b = D_OPEN | /*G_SMRAME | */C_BASE_SEG;
	pci_direct_conf1.write(0, 0, 0, SMRAM, 1, b);

	/* if we're resuming from S3, the SMM code is already in place,
	 * so don't copy it again to keep the current SMM state */

	if (1) { //!acpi_is_wakeup_s3()) {
		void *smmstart = longjmp0x30000x8000;
		u32 *v = ioremap(0xa0000, 65536);
		int i;
		printk("v is %p and *v is %#x\n", v, *v);
		printk("smmstart is %p\n", smmstart);
		/* copy the real SMM handler */
		memcpy(v, longjmp0x30000x8000, sizeof(longjmp0x30000x8000));
		wbinvd();
		for(i = 0; i < sizeof(longjmp0x30000x8000); i++)
			printk("%p %08x, ", v, v[i]);
	}
}

void smm_init(void)
{
	// clear it out.
	u32 smi_en;

	printk("smmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm init\n");
	smi_en = inl(pmbase + SMI_EN);
	/* Put SMM code to 0xa0000 */
	smm_install();

	/* Put relocation code to 0x38000 and relocate SMBASE */
	smm_relocate();

	/* We're done. Make sure SMIs can happen! */
	smi_set_eos();
	printk("smmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm init DONE\n");
}

void smm_setup_structures(void *gnvs, void *tcg, void *smi1)
{
	// later.
	/* The GDT or coreboot table is going to live here. But a long time
	 * after we relocated the GNVS, so this is not troublesome.
	 */
	//*(u32 *)(u64)0x500 = (u32)gnvs;
	//*(u32 *)(u64)0x504 = (u32)tcg;
	//*(u32 *)(u64)0x508 = (u32)smi1;
	//outb(APM_CNT_GNVS_UPDATE, 0xb2);
	printk("WARNING: NOT DOING %s\n", __func__);
}
