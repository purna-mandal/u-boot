/*
 * Microchip PIC32MZ[DA] Starter Kit board
 *
 * Copyright (C) 2015, Microchip Technology Inc.
 * Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <dm.h>
#include <clk.h>
#include <asm/arch-pic32/pic32.h>

#include "ddr.h"

/* initialize the DDR and PHY */
phys_size_t initdram(int board_type)
{
	ddr_phy_init();
	ddr_init();
	return CONFIG_SYS_MEM_SIZE;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	ulong rate = 0;
	struct udevice *dev;

	printf("Core: %s\n", get_core_name());

	if (!uclass_get_device(UCLASS_CLK, 0, &dev)) {
		rate = clk_get_rate(dev);
		printf("CPU Speed: %lu MHz\n", rate / 1000000);
	}

	return 0;
}
#endif
