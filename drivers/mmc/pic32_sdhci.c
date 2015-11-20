/*
 * Support of SDHCI devices for Microchip PIC32 SoC.
 *
 * Copyright (C) 2015 Microchip Technology Inc.
 * Andrei Pistirica <andrei.pistirica@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <sdhci.h>
#include <asm/errno.h>
#include <asm/arch-pic32/pic32.h>

DECLARE_GLOBAL_DATA_PTR;

/* SDHCI capabilities bits */
#define SDHCI_CAPS_SLOT_TYPE_MASK		0xC0000000
#define  SLOT_TYPE_REMOVABLE			0x0
#define  SLOT_TYPE_EMBEDDED			0x1
#define  SLOT_TYPE_SHARED_BUS			0x2

/* SDHCI Shared Bus Control */
#define SDHCI_SHARED_BUS_CTRL			0xE0
#define  SDHCI_SHARED_BUS_NR_CLK_PINS_MASK	0x7
#define  SDHCI_SHARED_BUS_NR_IRQ_PINS_MASK	0x30
#define  SDHCI_SHARED_BUS_CLK_PINS		0x10
#define  SDHCI_SHARED_BUS_IRQ_PINS		0x14

static int pic32_sdhci_set_shared(struct sdhci_host *host, u32 clk, u32 irq)
{
	unsigned int caps;
	u32 bus, caps_slot_type;
	u32 clk_pins, irq_pins;

	/* Card slot connected on shared bus? */
	caps = sdhci_readl(host, SDHCI_CAPABILITIES);
	caps_slot_type = ((caps & SDHCI_CAPS_SLOT_TYPE_MASK) >> 30);
	if (caps_slot_type != SLOT_TYPE_SHARED_BUS)
		return 0;

	bus = sdhci_readl(host, SDHCI_SHARED_BUS_CTRL);
	clk_pins = (bus & SDHCI_SHARED_BUS_NR_CLK_PINS_MASK) >> 0;
	irq_pins = (bus & SDHCI_SHARED_BUS_NR_IRQ_PINS_MASK) >> 4;

	/* Select first clock */
	if (clk_pins & clk)
		bus |= (clk << SDHCI_SHARED_BUS_CLK_PINS);

	/* Select first interrupt */
	if (irq_pins & irq)
		bus |= (irq << SDHCI_SHARED_BUS_IRQ_PINS);

	sdhci_writel(host, bus, SDHCI_SHARED_BUS_CTRL);
	return 0;
}

static int pic32_sdhci_probe(struct udevice *dev)
{
	struct sdhci_host *host = dev_get_priv(dev);
	u32 minmax[2], shared_clk_irq[2];
	int ret;

	ret = fdtdec_get_int_array(gd->fdt_blob, dev->of_offset,
				   "clock-freq-min-max", minmax, 2);
	if (ret)
		goto _out;

	ret = fdtdec_get_int_array(gd->fdt_blob, dev->of_offset,
				   "clock-irq-pins", shared_clk_irq, 2);
	if (!ret)
		ret = pic32_sdhci_set_shared(host, shared_clk_irq[1],
					     shared_clk_irq[0]);

	if (ret)
		goto _out;

	return add_sdhci(host, minmax[1], minmax[0]);

_out:
	return ret;
}


static int pic32_sdhci_ofdata_to_platdata(struct udevice *dev)
{
	struct sdhci_host *host = dev_get_priv(dev);

	host->name	= (char *)dev->name;
	host->ioaddr	= (void *)dev_get_addr(dev);
	host->quirks	= SDHCI_QUIRK_NO_HISPD_BIT;
	host->bus_width	= fdtdec_get_int(gd->fdt_blob, dev->of_offset,
					"bus-width", 4);
	return 0;
}
static const struct udevice_id pic32_sdhci_ids[] = {
	{ .compatible = "microchip,pic32mzda-sdhci" },
	{ }
};

U_BOOT_DRIVER(pic32_sdhci_drv) = {
	.name			= "pic32_sdhci",
	.id			= UCLASS_MMC,
	.of_match		= pic32_sdhci_ids,
	.probe			= pic32_sdhci_probe,
	.ofdata_to_platdata	= pic32_sdhci_ofdata_to_platdata,
	.priv_auto_alloc_size	= sizeof(struct sdhci_host),
};
