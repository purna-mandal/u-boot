/*
 * Copyright (C) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <dm.h>
#include <clk.h>
#include <div64.h>
#include <dm/lists.h>
#include <asm/io.h>
#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/clock.h>

DECLARE_GLOBAL_DATA_PTR;

/* Primary oscillator */
#define SYS_POSC_CLK_HZ	24000000

/* Fixed clk rate */
#define SYS_FRC_CLK_HZ	8000000

/* PLL */
#define ICLK_MASK	0x00000080
#define PLLIDIV_MASK	0x00000007
#define PLLODIV_MASK	0x00000007
#define CUROSC_MASK	0x00000007
#define PLLMUL_MASK	0x0000007F
#define FRCDIV_MASK	0x00000007

/* PBCLK */
#define PBDIV_MASK	0x00000007

/* SYSCLK MUX */
#define SCLK_SRC_FRC1	0
#define SCLK_SRC_SPLL	1
#define SCLK_SRC_POSC	2
#define SCLK_SRC_FRC2	7

/* Reference Oscillator Control Reg fields */
#define REFO_SEL_MASK	0x0f
#define REFO_SEL_SHIFT	0
#define REFO_ACTIVE	0x0100
#define REFO_DIVSW_EN	0x0200
#define REFO_OE		0x1000
#define REFO_ON		0x8000
#define REFO_DIV_SHIFT	16
#define REFO_DIV_MASK	0x7fff

/* Reference Oscillator Trim Register Fields */
#define REFO_TRIM_REG	0x10
#define REFO_TRIM_MASK	0x1ff
#define REFO_TRIM_SHIFT	23
#define REFO_TRIM_MAX	511

#define ROCLK_SRC_SCLK		0x0
#define ROCLK_SRC_SPLL		0x7
#define ROCLK_SRC_ROCLKI	0x8

/* Memory PLL */
#define MPLL_IDIV		0x03
#define MPLL_MULT		0x32
#define MPLL_ODIV1		0x02
#define MPLL_ODIV2		0x01
#define MPLL_VREG_RDY		0x00800000
#define MPLL_RDY		0x80000000
#define MPLL_IDIV_SHIFT		0
#define MPLL_MULT_SHIFT		8
#define MPLL_ODIV1_SHIFT	24
#define MPLL_ODIV2_SHIFT	27

static ulong pic32_get_pll_rate(void)
{
	ulong plliclk, v;
	u32 iclk, idiv, odiv, mult;

	v = readl(SPLLCON);
	iclk = (v & ICLK_MASK);
	idiv = ((v >> 8) & PLLIDIV_MASK) + 1;
	odiv = ((v >> 24) & PLLODIV_MASK);
	mult = ((v >> 16) & PLLMUL_MASK) + 1;

	plliclk = iclk ? SYS_FRC_CLK_HZ : SYS_POSC_CLK_HZ;

	if (odiv < 2)
		odiv = 2;
	else if (odiv < 5)
		odiv = (1 << odiv);
	else
		odiv = 32;

	return ((plliclk / idiv) * mult) / odiv;
}

static ulong pic32_get_sysclk(void)
{
	ulong hz;
	ulong div, frcdiv;
	ulong v  = readl(OSCCON);
	ulong curr_osc;

	/* get clk source */
	v = readl(OSCCON);
	curr_osc = (v >> 12) & CUROSC_MASK;
	switch (curr_osc) {
	case SCLK_SRC_FRC1:
	case SCLK_SRC_FRC2:
		frcdiv = ((v >> 24) & FRCDIV_MASK);
		div = ((1 << frcdiv) + 1) + (128 * (frcdiv == 7));
		hz = SYS_FRC_CLK_HZ / div;
		break;

	case SCLK_SRC_SPLL:
		hz = pic32_get_pll_rate();
		break;

	case SCLK_SRC_POSC:
		hz = SYS_POSC_CLK_HZ;
		break;

	default:
		hz = 0;
		printf("clk: unknown sclk_src.\n");
		break;
	}

	return hz;
}

static ulong pic32_get_pbclk(int bus)
{
	ulong div, clk_freq;
	void __iomem *reg;

	clk_freq = pic32_get_sysclk();

	reg = (void __iomem *)PB1DIV + (bus * 0x10);
	div = (readl(reg) & PBDIV_MASK) + 1;

	return clk_freq / div;
}

static ulong pic32_get_cpuclk(void)
{
	return pic32_get_pbclk(6);
}

static ulong pic32_set_refclk(int bus, int parent_rate, int rate, int parent_id)
{
	void __iomem *reg;
	u32 div, trim, v;
	u64 frac;
	ulong base;

	/* calculate dividers,
	 *   rate = parent_rate / [2 * (div + (trim / 512))]
	 */
	if (parent_rate <= rate) {
		div = 0;
		trim = 0;
	} else {
		div = parent_rate / (rate << 1);
		frac = parent_rate;
		frac <<= 8;
		do_div(frac, rate);
		frac -= (u64)(div << 9);
		trim = (frac >= REFO_TRIM_MAX) ? REFO_TRIM_MAX : (u32)frac;
	}

	reg = (void __iomem *)(REFO1CON + bus * 0x20);

	/* disable clk */
	writel(REFO_ON | REFO_OE, reg + _CLR_OFFSET);

	/* wait till previous src change is active */
	base = get_timer(0);
	for (;;) {
		v = readl(reg);
		if ((v & (REFO_DIVSW_EN | REFO_ACTIVE)) == 0)
			break;
		if (get_timer(base) >= get_tbclk()) {
			printf("refclk: tmout while active\n");
			break;
		}
	}

	/* parent_id */
	v &= ~(REFO_SEL_MASK << REFO_SEL_SHIFT);
	v |= (parent_id << REFO_SEL_SHIFT);

	/* apply rodiv */
	v &= ~(REFO_DIV_MASK << REFO_DIV_SHIFT);
	v |= (div << REFO_DIV_SHIFT);
	writel(v, reg);

	/* apply trim */
	v = readl(reg + REFO_TRIM_REG);
	v &= ~(REFO_TRIM_MASK << REFO_TRIM_SHIFT);
	v |= (trim << REFO_TRIM_SHIFT);
	writel(v, reg + REFO_TRIM_REG);

	/* enable clk */
	writel(REFO_ON | REFO_OE, reg + _SET_OFFSET);

	/* switch divider */
	writel(REFO_DIVSW_EN, reg + _SET_OFFSET);

	base = get_timer(0);
	for (;;) {
		if (!(readl(reg) & REFO_DIVSW_EN))
			break;

		if (get_timer(base) >= get_tbclk()) {
			printf("refclk: tmout while switching.\n");
			break;
		}
	}

	return 0;
}

static ulong pic32_get_refclk(int bus)
{
	void __iomem *reg;
	u32 rodiv, rotrim, rosel, v, parent_rate;
	u64 rate64;

	reg  = (void __iomem *)(REFO1CON + bus * 0x20);
	v = readl(reg);

	/* get rosel */
	rosel = (v >> REFO_SEL_SHIFT) & REFO_SEL_MASK;
	/* get div */
	rodiv = (v >> REFO_DIV_SHIFT) & REFO_DIV_MASK;
	/* get trim */
	v = readl(reg + REFO_TRIM_REG);
	rotrim = (v >> REFO_TRIM_SHIFT) & REFO_TRIM_MASK;

	if (!rodiv)
		return 0;

	/* calc rate */
	switch (rosel) {
	default:
	case ROCLK_SRC_SCLK:
		parent_rate = pic32_get_cpuclk();
		break;
	case ROCLK_SRC_SPLL:
		parent_rate = pic32_get_pll_rate();
		break;
	}

	/* Calculation
	 * rate = parent_rate / [2 * (div + (trim / 512))]
	 */
	if (rotrim) {
		rodiv <<= 9;
		rodiv += rotrim;
		rate64 = parent_rate;
		rate64 <<= 8;
		do_div(rate64, rodiv);
		v = (u32)rate64;
	} else {
		v = parent_rate / (rodiv << 1);
	}
	return v;
}

static void mpll_init(void)
{
	u32 v, mask;
	ulong base;

	/* initialize */
	v = (MPLL_IDIV << MPLL_IDIV_SHIFT) |
	    (MPLL_MULT << MPLL_MULT_SHIFT) |
	    (MPLL_ODIV1 << MPLL_ODIV1_SHIFT) |
	    (MPLL_ODIV2 << MPLL_ODIV2_SHIFT);

	writel(v, (void __iomem *)CFGMPLL);

	/* Wait for ready */
	mask = MPLL_RDY | MPLL_VREG_RDY;
	base = get_timer(0);
	for (;;) {
		v = readl((void __iomem *)CFGMPLL);
		if ((v & mask) == mask)
			break;

		if (get_timer(base) >= get_tbclk()) {
			printf("refclk: tmout while waiting MPLL ready.\n");
			break;
		}
	}
}

static void pic32_clk_init(struct udevice *dev)
{
	ulong rate, pll_hz = pic32_get_pll_rate();
	const void *blob = gd->fdt_blob;
	char propname[] = "microchip,refo%d-frequency12345";
	int i;

	/* Initialize REFOs as not initialized and enabled on reset. */
	for (i = 0; i < 5; i++) {
		snprintf(propname, sizeof(propname),
			 "microchip,refo%d-frequency", i + 1);

		rate = fdtdec_get_int(blob, dev->of_offset, propname, 0);
		if (rate)
			pic32_set_refclk(i, pll_hz, rate, ROCLK_SRC_SPLL);
	}

	/* Memory PLL */
	mpll_init();
}

struct pic32_clk_platdata {
	u32 clk_id;
};

static ulong pic32_clk_get_rate(struct udevice *dev)
{
	return pic32_get_cpuclk();
}

static ulong pic32_get_periph_rate(struct udevice *dev, int periph)
{
	ulong rate;

	switch (periph) {
	case PB1CLK ... PB7CLK:
		rate = pic32_get_pbclk(periph - PB1CLK);
		break;
	case REF1CLK ... REF5CLK:
		rate = pic32_get_refclk(periph - REF1CLK);
		break;
	case PLLCLK:
		rate = pic32_get_pll_rate();
		break;
	default:
		rate = 0;
		break;
	}

	return rate;
}

static ulong pic32_set_periph_rate(struct udevice *dev, int periph, ulong rate)
{
	ulong pll_hz;

	switch (periph) {
	case REF1CLK ... REF5CLK:
		pll_hz = pic32_get_pll_rate();
		pic32_set_refclk(periph - REF1CLK,
				 pll_hz, rate, ROCLK_SRC_SPLL);
		break;
	default:
		break;
	}

	return rate;
}

static struct clk_ops pic32_pic32_clk_ops = {
	.get_rate = pic32_clk_get_rate,
	.set_periph_rate = pic32_set_periph_rate,
	.get_periph_rate = pic32_get_periph_rate,
};

static int pic32_clk_probe(struct udevice *dev)
{
	struct pic32_clk_platdata *plat = dev_get_platdata(dev);

	if (plat->clk_id != BASECLK)
		goto out;

	/* initialize only once */
	plat->clk_id |= 0xd0ae0000;
	pic32_clk_init(dev);
out:
	return 0;
}

static int pic32_clk_bind(struct udevice *dev)
{
	struct pic32_clk_platdata *plat = dev_get_platdata(dev);

	if (dev->of_offset == -1) {
		plat->clk_id = BASECLK;
		return 0;
	}

	return 0;
}

static const struct udevice_id pic32_clk_ids[] = {
	{ .compatible = "microchip,pic32mzda_clk"},
	{}
};

U_BOOT_DRIVER(pic32_clk) = {
	.name			= "pic32_clk",
	.id			= UCLASS_CLK,
	.of_match		= pic32_clk_ids,
	.platdata_auto_alloc_size = sizeof(struct pic32_clk_platdata),
	.ops			= &pic32_pic32_clk_ops,
	.bind			= pic32_clk_bind,
	.probe			= pic32_clk_probe,
};
