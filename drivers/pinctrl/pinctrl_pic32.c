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
#include <asm/io.h>
#include <dm/pinctrl.h>
#include <dm/root.h>
#include <mach/pic32.h>

DECLARE_GLOBAL_DATA_PTR;

/* Peripheral PORTA-PORTK / PORT0-PORT9 */
enum {
	PIC32_PORT_A = 0,
	PIC32_PORT_B = 1,
	PIC32_PORT_C = 2,
	PIC32_PORT_D = 3,
	PIC32_PORT_E = 4,
	PIC32_PORT_F = 5,
	PIC32_PORT_G = 6,
	PIC32_PORT_H = 7,
	PIC32_PORT_J = 8, /* no PORT_I */
	PIC32_PORT_K = 9,
	PIC32_PORT_MAX
};

/* Input pinmux reg offset */
#define U1RXR		0x0068
#define U2RXR		0x0070
#define SDI1R		0x009c
#define SDI2R		0x00a8

/* Output pinmux reg offset */
#define PPS_OUT(__port, __pin) (((__port) * 16 + (__pin)) << 2)

/* Port config/control registers */
struct pic32_reg_port {
	struct pic32_reg_atomic ansel;
	struct pic32_reg_atomic tris;
	struct pic32_reg_atomic port;
	struct pic32_reg_atomic lat;
	struct pic32_reg_atomic odc;
	struct pic32_reg_atomic cnpu;
	struct pic32_reg_atomic cnpd;
	struct pic32_reg_atomic cncon;
};

#define PIN_CONFIG_PIC32_DIGITAL	(PIN_CONFIG_END + 1)
#define PIN_CONFIG_PIC32_ANALOG		(PIN_CONFIG_END + 2)

struct pic32_pin_config {
	u16 port;
	u16 pin;
	u32 flags;
};

#define PIN_CONFIG(_prt, _pin, _cfg) \
	{.port = (_prt), .pin = (_pin), .flags = (_cfg), }

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

struct pic32_pinctrl_priv {
	void __iomem *mux_in;
	void __iomem *mux_out;
	void __iomem *pinconf;
};

static int pic32_pinconfig_one(struct pic32_pinctrl_priv *priv,
			       u32 port, u32 pin, u32 param)
{
	struct pic32_reg_port *regs;

	regs = (struct pic32_reg_port *)(priv->pinconf + port * 0x100);
	switch (param) {
	case PIN_CONFIG_PIC32_DIGITAL:
		writel(BIT(pin), &regs->ansel.clr);
		break;
	case PIN_CONFIG_PIC32_ANALOG:
		writel(BIT(pin), &regs->ansel.set);
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		writel(BIT(pin), &regs->tris.set);
		break;
	case PIN_CONFIG_OUTPUT:
		writel(BIT(pin), &regs->tris.clr);
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		writel(BIT(pin), &regs->cnpu.set);
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		writel(BIT(pin), &regs->cnpd.set);
		break;
	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
		writel(BIT(pin), &regs->odc.set);
		break;
	default:
		break;
	}

	return 0;
}

static int pic32_pinconfig_set(struct pic32_pinctrl_priv *priv,
			       struct pic32_pin_config *list, int count)
{
	int i;

	for (i = 0 ; i < count; i++)
		pic32_pinconfig_one(priv, list[i].port,
				    list[i].pin, list[i].flags);

	return 0;
}

static void _eth_pin_config(struct udevice *dev)
{
	struct pic32_pinctrl_priv *priv = dev_get_priv(dev);
	struct pic32_pin_config configs[] = {
		/* EMDC - D11 */
		PIN_CONFIG(PIC32_PORT_D, 11, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_D, 11, PIN_CONFIG_OUTPUT),
		/* ETXEN */
		PIN_CONFIG(PIC32_PORT_D, 6, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_D, 6, PIN_CONFIG_OUTPUT),
		/* ECRSDV */
		PIN_CONFIG(PIC32_PORT_H, 13, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_H, 13, PIN_CONFIG_INPUT_ENABLE),
		/* ERXD0 */
		PIN_CONFIG(PIC32_PORT_H, 8, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_H, 8, PIN_CONFIG_INPUT_ENABLE),
		PIN_CONFIG(PIC32_PORT_H, 8, PIN_CONFIG_BIAS_PULL_DOWN),
		/* ERXD1 */
		PIN_CONFIG(PIC32_PORT_H, 5, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_H, 5, PIN_CONFIG_INPUT_ENABLE),
		PIN_CONFIG(PIC32_PORT_H, 5, PIN_CONFIG_BIAS_PULL_DOWN),
		/* EREFCLK */
		PIN_CONFIG(PIC32_PORT_J, 11, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_J, 11, PIN_CONFIG_INPUT_ENABLE),
		/* ETXD1 */
		PIN_CONFIG(PIC32_PORT_J, 9, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_J, 9, PIN_CONFIG_OUTPUT),
		/* ETXD0 */
		PIN_CONFIG(PIC32_PORT_J, 8, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_J, 8, PIN_CONFIG_OUTPUT),
		/* EMDIO */
		PIN_CONFIG(PIC32_PORT_J, 1, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_J, 1, PIN_CONFIG_INPUT_ENABLE),
		/* ERXERR */
		PIN_CONFIG(PIC32_PORT_F, 3, PIN_CONFIG_PIC32_DIGITAL),
		PIN_CONFIG(PIC32_PORT_F, 3, PIN_CONFIG_INPUT_ENABLE),
	};

	pic32_pinconfig_set(priv, configs, ARRAY_SIZE(configs));
}

static int pic32_pinctrl_request(struct udevice *dev, int func, int flags)
{
	struct pic32_pinctrl_priv *priv = dev_get_priv(dev);

	switch (func) {
	case PERIPH_ID_UART2:
		/* PPS for U2 RX/TX */
		writel(0x02, priv->mux_out + PPS_OUT(PIC32_PORT_G, 9));
		writel(0x05, priv->mux_in + U2RXR); /* B0 */
		/* set digital mode */
		pic32_pinconfig_one(priv, PIC32_PORT_G, 9,
				    PIN_CONFIG_PIC32_DIGITAL);
		pic32_pinconfig_one(priv, PIC32_PORT_B, 0,
				    PIN_CONFIG_PIC32_DIGITAL);
		break;
	case PERIPH_ID_ETH:
		_eth_pin_config(dev);
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
	.request		= pic32_pinctrl_request,
	.get_periph_id		= pic32_pinctrl_get_periph_id,
};

static int pic32_pinctrl_probe(struct udevice *dev)
{
	struct pic32_pinctrl_priv *priv = dev_get_priv(dev);

	priv->mux_in = pic32_ioremap(PPS_IN_BASE);
	priv->mux_out = pic32_ioremap(PPS_OUT_BASE);
	priv->pinconf = pic32_ioremap(PINCTRL_BASE);

	return 0;
}

static const struct udevice_id pic32_pinctrl_ids[] = {
	{ .compatible = "microchip,pic32mzda-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_pic32) = {
	.name		= "pinctrl_pic32",
	.id		= UCLASS_PINCTRL,
	.of_match	= pic32_pinctrl_ids,
	.ops		= &pic32_pinctrl_ops,
	.probe		= pic32_pinctrl_probe,
	.priv_auto_alloc_size = sizeof(struct pic32_pinctrl_priv),
};
