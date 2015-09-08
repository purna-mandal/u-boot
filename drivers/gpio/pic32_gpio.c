/*
 * Copyright (c) 2015 Microchip Technology Inc
 * Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/compat.h>
#include <dt-bindings/gpio/gpio.h>
#include <asm/arch-pic32/pic32.h>

/* Peripheral Pin Control */
struct pic32_gpio_regs {
	struct pic32_reg_atomic ansel;
	struct pic32_reg_atomic tris;
	struct pic32_reg_atomic port;
	struct pic32_reg_atomic lat;
	struct pic32_reg_atomic open_drain;
	struct pic32_reg_atomic cnpu;
	struct pic32_reg_atomic cnpd;
	struct pic32_reg_atomic cncon;
};

enum {
	MICROCHIP_GPIO_DIR_OUT,
	MICROCHIP_GPIO_DIR_IN,
	MICROCHIP_GPIOS_PER_BANK = 16,
};

struct pic32_gpio_priv {
	struct pic32_gpio_regs *regs;
	char name[2];
};

static int pic32_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct pic32_gpio_priv *priv = dev_get_priv(dev);

	return !!(readl(&priv->regs->port.raw) & BIT(offset));
}

static int pic32_gpio_set_value(struct udevice *dev, unsigned offset,
				int value)
{
	struct pic32_gpio_priv *priv = dev_get_priv(dev);
	int mask = BIT(offset);

	if (value)
		writel(mask, &priv->regs->port.set);
	else
		writel(mask, &priv->regs->port.clr);

	return 0;
}

static int pic32_gpio_direction(struct udevice *dev, unsigned offset)
{
	struct pic32_gpio_priv *priv = dev_get_priv(dev);

	if (readl(&priv->regs->ansel.raw) & BIT(offset))
		return -1;

	if (readl(&priv->regs->tris.raw) & BIT(offset))
		return MICROCHIP_GPIO_DIR_IN;
	else
		return MICROCHIP_GPIO_DIR_OUT;
}

static int pic32_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	struct pic32_gpio_priv *priv = dev_get_priv(dev);
	int mask = BIT(offset);

	writel(mask, &priv->regs->ansel.clr);
	writel(mask, &priv->regs->tris.set);

	return 0;
}

static int pic32_gpio_direction_output(struct udevice *dev,
				       unsigned offset, int value)
{
	struct pic32_gpio_priv *priv = dev_get_priv(dev);
	int mask = BIT(offset);

	writel(mask, &priv->regs->ansel.clr);
	writel(mask, &priv->regs->tris.clr);

	pic32_gpio_set_value(dev, offset, value);
	return 0;
}

static int pic32_gpio_xlate(struct udevice *dev, struct gpio_desc *desc,
			    struct fdtdec_phandle_args *args)
{
	desc->offset = args->args[0];
	desc->flags = args->args[1] & GPIO_ACTIVE_LOW ? GPIOD_ACTIVE_LOW : 0;

	return 0;
}

static int pic32_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct pic32_gpio_priv *priv = dev_get_priv(dev);
	char *end;
	int bank;

	priv->regs = (struct pic32_gpio_regs *)dev_get_addr(dev);
	uc_priv->gpio_count = MICROCHIP_GPIOS_PER_BANK;
	end = strrchr(dev->name, '@');
	bank = trailing_strtoln(dev->name, end);
	priv->name[0] = 'A' + bank;
	uc_priv->bank_name = priv->name;

	return 0;
}

static int pic32_gpio_get_function(struct udevice *dev, unsigned offset)
{
	int ret = GPIOF_UNUSED;

	switch (pic32_gpio_direction(dev, offset)) {
	case MICROCHIP_GPIO_DIR_OUT:
		ret = GPIOF_OUTPUT;
		break;
	case MICROCHIP_GPIO_DIR_IN:
		ret = GPIOF_INPUT;
		break;
	default:
		ret = GPIOF_UNUSED;
		break;
	}
	return ret;
}

static const struct dm_gpio_ops gpio_pic32_ops = {
	.direction_input	= pic32_gpio_direction_input,
	.direction_output	= pic32_gpio_direction_output,
	.get_value		= pic32_gpio_get_value,
	.set_value		= pic32_gpio_set_value,
	.get_function		= pic32_gpio_get_function,
	.xlate			= pic32_gpio_xlate,
};

static const struct udevice_id pic32_gpio_ids[] = {
	{ .compatible = "microchip,gpio-bank" },
	{ }
};

U_BOOT_DRIVER(gpio_pic32) = {
	.name		= "gpio_pic32",
	.id		= UCLASS_GPIO,
	.of_match	= pic32_gpio_ids,
	.ops		= &gpio_pic32_ops,
	.probe		= pic32_gpio_probe,
	.priv_auto_alloc_size	= sizeof(struct pic32_gpio_priv),
};
