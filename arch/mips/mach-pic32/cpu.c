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
#include <mach/pic32.h>
#include <mach/ddr.h>
#include <dt-bindings/clock/microchip,clock.h>

/* Flash prefetch */
#define PRECON          0x00
#define PRECONCLR       (PRECON + _CLR_OFFSET)
#define PRECONSET       (PRECON + _SET_OFFSET)

/* Flash ECCCON */
#define ECC_MASK	0x03
#define ECC_SHIFT	4

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
#define _MHZ(x)  ((x) * 1000000)
	void __iomem *base;
	int v, nr_waits;
	ulong rate;

	rate = clk_get_cpu_rate();

	/* get flash ECC type */
	base = pic32_ioremap(PIC32_CFG_BASE);
	v = (readl(base + CFGCON) >> ECC_SHIFT) & ECC_MASK;
	if (v < 2) {
		if (rate < _MHZ(66))
			nr_waits = 0;
		else if (rate < _MHZ(133))
			nr_waits = 1;
		else
			nr_waits = 2;
	} else {
		if (rate <= _MHZ(83))
			nr_waits = 0;
		else if (rate <= _MHZ(166))
			nr_waits = 1;
		else
			nr_waits = 2;
	}

	base = pic32_ioremap(PREFETCH_BASE);
	writel(nr_waits, base + PRECON);

	/* Enable prefetch for all */
	writel(0x30, base + PRECONSET);
#undef _MHZ
}

/* arch specific CPU init after DM */
int arch_cpu_init_dm(void)
{
	/* flash prefetch */
	init_prefetch();
	return 0;
}

/* Un-gate DDR2 modules (gated by default) */
static void ddr2_pmd_ungate(void)
{
	void __iomem *regs;

	regs = pic32_ioremap(PIC32_CFG_BASE);
	writel(0, regs + PMD7);
}

/* initialize the DDR2 Controller and DDR2 PHY */
phys_size_t initdram(int board_type)
{
	ddr2_pmd_ungate();
	ddr2_phy_init();
	ddr2_ctrl_init();
	return ddr2_calculate_size();
}

int misc_init_r(void)
{
	set_io_port_base(0);
	return 0;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
const char *get_core_name(void)
{
	u32 proc_id;
	const char *str;

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
#define F_MHZ(x)	((x) / 1000000)
	int i, ret;
	struct udevice *dev;

	ret = uclass_get_device(UCLASS_CLK, 0, &dev);
	if (ret) {
		printf("clk-uclass not found\n");
		return ret;
	}

	printf("PLL Speed: %lu MHz\n", F_MHZ(clk_get_periph_rate(dev, PLLCLK)));
	printf("CPU Speed: %lu MHz\n", F_MHZ(clk_get_rate(dev)));
	printf("MPLL Speed: %lu MHz\n", F_MHZ(clk_get_periph_rate(dev, MPLL)));

	for (i = PB1CLK; i <= PB7CLK; i++)
		printf("PB%d Clock Speed: %lu MHz\n",
		       i - PB1CLK + 1, F_MHZ(clk_get_periph_rate(dev, i)));

	for (i = REF1CLK; i <= REF5CLK; i++)
		printf("REFO%d Clock Speed: %lu MHz\n", i - REF1CLK + 1,
		       F_MHZ(clk_get_periph_rate(dev, i)));
#undef F_MHZ
	return 0;
}
#endif
