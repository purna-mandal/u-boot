/*
 * Copyright (C) 2015
 * Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */
#include <common.h>
#include <dm.h>
#include <clk.h>
#include <debug_uart.h>
#include <linux/compiler.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/clock.h>

DECLARE_GLOBAL_DATA_PTR;

static ulong clk_get_cpu_rate(void)
{
	int ret;
	struct udevice *dev;

	ret = uclass_get_device(UCLASS_CLK, 0, &dev);
	if (ret) {
		panic("uclass-clk: device not found\n");
		return 0;
	}

	return clk_get_rate(dev);
}

/* initialize prefetch module related to cpu_clk */
static void init_prefetch(void)
{
	int v, nr_waits;
	ulong rate;

	rate = clk_get_cpu_rate();

	/* calc and apply waits based on dynamic ECC */
	v = (readl(CFGCON) >> 4) & 0x03;
	if (v < 2) {
		if (rate < 66000000)
			nr_waits = 0;
		else if (rate < 133000000)
			nr_waits = 1;
		else
			nr_waits = 2;
	} else {
		if (rate <= 83000000)
			nr_waits = 0;
		else if (rate <= 166000000)
			nr_waits = 1;
		else
			nr_waits = 2;
	}

	writel(nr_waits, PRECON);

	/* Enable prefetch for all */
	writel(0x30, PRECONSET);
}

/* arch specific CPU init after DM */
int arch_cpu_init_dm(void)
{
	/* flash prefetch */
	init_prefetch();

	return 0;
}

/* initializes board before relocation */
__attribute__ ((weak)) int board_early_init_f(void)
{
	return 0;
}

int misc_init_r(void)
{
	set_io_port_base(0);
	return 0;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
char *get_core_name(void)
{
	u32 proc_id;
	char *str;

	proc_id = read_c0_prid();
	switch (proc_id) {
	case 0x19e28:
		str = "PIC32MZ[DA]";
		break;
	default:
		str = "UNKNOWN";
	}

	return str;
}
#endif

#ifdef CONFIG_CMD_CLK
int soc_clk_dump(void)
{
	int i, ret;
	struct udevice *dev;

	ret = uclass_get_device(UCLASS_CLK, 0, &dev);
	if (ret) {
		printf("clk-uclass not found\n");
		return ret;
	}

	printf("PLL Speed: %lu MHz\n",
	       clk_get_periph_rate(dev, PLLCLK) / 1000000);

	printf("CPU Clock Speed: %lu MHz\n", clk_get_rate(dev) / 1000000);

	for (i = PB1CLK; i <= PB7CLK; i++)
		printf("PB%d Clock Speed: %lu MHz\n",
		       i - PB1CLK + 1, clk_get_periph_rate(dev, i) / 1000000);

	for (i = REF1CLK; i <= REF5CLK; i++)
		printf("REFO%d Clock Speed: %lu MHz\n", i - REF1CLK + 1,
		       clk_get_periph_rate(dev, i) / 1000000);
	return 0;
}
#endif
