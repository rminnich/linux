/*
 * realmode.lds.S
 *
 * Linker script for the real-mode code
 */

#include <asm/page_types.h>

#undef i386

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)

SECTIONS
{
	real_mode_seg = 0;

	. = 0;
	.header : {
		pa_real_mode_base = .;
		*(.header)
	}

	. = ALIGN(PAGE_SIZE);
	smmpa_text_start = .;
	.text : {
		*(.smm.smm.text)
		*(.smm.text.*)
	}

	.text32 : {
		*(.smm.text32)
		*(.smm.text32.*)
	}

	.text64 : {
		*(.smm.text64)
		*(.smm.text64.*)
	}
	smmpa_ro_end = .;

	. = ALIGN(PAGE_SIZE);
	.data : {
		*(.smm.data)
		*(.smm.data.*)
	}

	. = ALIGN(128);
	.bss : {
		*(.smm.bss*)
	}

	/* End signature for integrity checking */
	. = ALIGN(4);
	.signature : {
		*(.smm.signature)
	}

	. = ALIGN(4);
	HEAP = .;

	/DISCARD/ : {
		*(.smm.note*)
		*(.smm.debug*)
		*(.smm.eh_frame*)
	}

#include "smmpasyms.h"
}
