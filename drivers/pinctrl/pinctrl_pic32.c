/*
 * Pinctrl driver for Microchip PIC32 SoCs
 * Copyright (c) 2015 Microchip Technology Inc.
 * Written by Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <dm.h>
#include <errno.h>
#include <syscon.h>
#include <asm/io.h>
#include <dm/pinctrl.h>
#include <dm/root.h>
#include <asm/arch-pic32/pic32.h>

DECLARE_GLOBAL_DATA_PTR;

enum {
	PERIPH_ID_UART1,
	PERIPH_ID_UART2,
	PERIPH_ID_ETH,
	PERIPH_ID_USB,
	PERIPH_ID_SDHCI,
	PERIPH_ID_I2C1,
	PERIPH_ID_I2C2,
	PERIPH_ID_SPI1,
	PERIPH_ID_SPI2,
	PERIPH_ID_SQI,
};

static void _eth_pin_config(void)
{
	/*
	 * PORT D pin configuration settings
	 *
	 * Reg   Bit  I/O    Dig/Ana
	 * EMDC  RD11 Output Digital
	 * ETXEN RD6  Output Digital
	 *
	 */
	writel(0x0840, ANSELCLR(PIC32_PORT_D));  /* set to digital mode */
	writel(0x0840, TRISCLR(PIC32_PORT_D));   /* set to output mode  */

	/*
	 * PORT H pin configuration settings
	 *
	 * Reg    Bit  I/O    Dig/Ana   PullUp/Down
	 * ECRSDV RH13 Input  Digital
	 * ERXD0  RH8  Input  Digital   Down
	 * ERXD1  RH5  Input  Digital   Down
	 */
	writel(0x2120, ANSELCLR(PIC32_PORT_H));  /* set to digital mode */
	writel(0x2120, TRISSET(PIC32_PORT_H));   /* set to input mode */

	/*
	 * PORT J pin configuration settings
	 *
	 * Reg     Bit  I/O    Dig/Ana
	 * EREFCLK RJ11 Input  Digital
	 * ETXD1   RJ9  Output Digital
	 * ETXD0   RJ8  Output Digital
	 * EMDIO   RJ1  Input  Digital
	 *
	 */
	writel(0x0b02, ANSELCLR(PIC32_PORT_J)); /* set to digital mode */
	writel(0x0300, TRISCLR(PIC32_PORT_J));  /* set pins to output mode  */
	writel(0x0802, TRISSET(PIC32_PORT_J));  /* set pins to input mode  */

	/*
	 * PORT F pin configuration settings
	 * Reg    Bit  I/O    Dig/Ana
	 * ERXERR RF3  Input  Digital
	 */
	writel(0x10, ANSELCLR(PIC32_PORT_F));  /* set to digital mode */
	writel(0x10, TRISSET(PIC32_PORT_F));   /* set to input mode */
}

static int pic32_pinctrl_request(struct udevice *dev, int func, int flags)
{
	switch (func) {
	case PERIPH_ID_UART2:
		/* PPS for U2 RX/TX */
		writel(0x0002, RPG9R); /* G9 */
		writel(0x0005, U2RXR); /* B0 */
		/* digital mode */
		writel(0x0001, ANSELCLR(PIC32_PORT_B));
		writel(0x0200, ANSELCLR(PIC32_PORT_G));
		break;
	case PERIPH_ID_ETH:
		_eth_pin_config();
		break;
	case PERIPH_ID_SDHCI:
		break;
	case PERIPH_ID_USB:
		break;
	default:
		debug("%s: unknown-unhandled case\n", __func__);
		break;
	}

	return 0;
}

static int pic32_pinctrl_get_periph_id(struct udevice *dev,
				       struct udevice *periph)
{
	int ret;
	u32 cell[2];

	ret = fdtdec_get_int_array(gd->fdt_blob, periph->of_offset,
				   "interrupts", cell, ARRAY_SIZE(cell));
	if (ret < 0)
		return -EINVAL;

	/* interrupt number */
	switch (cell[0]) {
	case 112 ... 114:
		return PERIPH_ID_UART1;
	case 145 ... 147:
		return PERIPH_ID_UART2;
	case 109 ... 111:
		return PERIPH_ID_SPI1;
	case 142 ... 144:
		return PERIPH_ID_SPI2;
	case 115 ... 117:
		return PERIPH_ID_I2C1;
	case 148 ... 150:
		return PERIPH_ID_I2C2;
	case 132 ... 133:
		return PERIPH_ID_USB;
	case 169:
		return PERIPH_ID_SQI;
	case 191:
		return PERIPH_ID_SDHCI;
	case 153:
		return PERIPH_ID_ETH;
	default:
		break;
	}

	return -ENOENT;
}

static int pic32_pinctrl_set_state_simple(struct udevice *dev,
					  struct udevice *periph)
{
	int func;

	debug("%s: periph %s\n", __func__, periph->name);
	func = pic32_pinctrl_get_periph_id(dev, periph);
	if (func < 0)
		return func;
	return pic32_pinctrl_request(dev, func, 0);
}

static struct pinctrl_ops pic32_pinctrl_ops = {
	.set_state_simple	= pic32_pinctrl_set_state_simple,
	.request	= pic32_pinctrl_request,
	.get_periph_id	= pic32_pinctrl_get_periph_id,
};

static const struct udevice_id pic32_pinctrl_ids[] = {
	{ .compatible = "microchip,pic32mzda-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_pic32) = {
	.name		= "pinctrl_pic32",
	.id		= UCLASS_PINCTRL,
	.of_match	= pic32_pinctrl_ids,
	.ops		= &pic32_pinctrl_ops,
};
