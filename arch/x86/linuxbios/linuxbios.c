
/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 coresystems GmbH
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
#include <asm/setup.h>

static int __init setup_linuxbios(char *_)
{
	void monitor(void);
	printk("========================================\n");
	monitor();
	return 0;
}

early_param("linuxbios", setup_linuxbios);
