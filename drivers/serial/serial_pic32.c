/*
 * (c) 2015 Paul Thacker <paul.thacker@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */
#include <common.h>
#include <config.h>
#include <dm.h>
#include <clk.h>
#include <serial.h>
#include <wait_bit.h>
#include <mach/pic32.h>
#include <dt-bindings/clock/microchip,clock.h>

DECLARE_GLOBAL_DATA_PTR;

/* UART Control Registers */
#define U_MOD		0x00
#define U_MODCLR	(U_MOD + _CLR_OFFSET)
#define U_MODSET	(U_MOD + _SET_OFFSET)
#define U_STA		0x10
#define U_STACLR	(U_STA + _CLR_OFFSET)
#define U_STASET	(U_STA + _SET_OFFSET)
#define U_TXR		0x20
#define U_RXR		0x30
#define U_BRG		0x40

/* U_MOD bits */
#define UART_ENABLE		BIT(15)

/* U_STA bits */
#define UART_RX_ENABLE		BIT(12)
#define UART_TX_BRK		BIT(11)
#define UART_TX_ENABLE		BIT(10)
#define UART_TX_FULL		BIT(9)
#define UART_TX_EMPTY		BIT(8)
#define UART_RX_OERR		BIT(1)
#define UART_RX_DATA_AVAIL	BIT(0)

struct pic32_uart_priv {
	void __iomem *base;
	ulong uartclk;
};

static void pic32_serial_setbrg(void __iomem *base, ulong uart_clk, u32 baud)
{
	writel(0, base + U_BRG);
	writel((uart_clk / baud / 16) - 1, base + U_BRG);
}

/*
 * Initialize the serial port with the given baudrate.
 * The settings are always 8 data bits, no parity, 1 stop bit, no start bits.
 */
static int pic32_serial_init(void __iomem *base, ulong clk, u32 baudrate)
{
	/* wait for TX FIFO to empty */
	wait_for_bit(__func__, base + U_STA, UART_TX_EMPTY,
		     true, CONFIG_SYS_HZ, false);

	/* send break */
	writel(UART_TX_BRK, base + U_STASET);

	/* disable and clear mode */
	writel(0, base + U_MOD);
	writel(0, base + U_STA);

	/* set baud rate generator */
	pic32_serial_setbrg(base, clk, baudrate);

	/* enable the UART for TX and RX */
	writel(UART_TX_ENABLE | UART_RX_ENABLE, base + U_STASET);

	/* enable the UART */
	writel(UART_ENABLE, base + U_MODSET);
	return 0;
}

/* Output a single byte to the serial port */
static void pic32_serial_putc(void __iomem *base, const char c)
{
	/* if \n, then add a \r */
	if (c == '\n')
		pic32_serial_putc(base, '\r');

	/* Wait for Tx FIFO not full */
	wait_for_bit(__func__, base + U_STA, UART_TX_FULL,
		     false, CONFIG_SYS_HZ, false);

	/* stuff the tx buffer with the character */
	writel(c, base + U_TXR);
}

/* Check number of characters in RX fifo */
static int pic32_serial_tstc(void __iomem *base)
{
	/* check if rx buffer overrun error has occurred */
	if (readl(base + U_STA) & UART_RX_OERR) {
		readl(base + U_RXR);

		/* clear OERR to keep receiving */
		writel(UART_RX_OERR, base + U_STACLR);
	}

	return readl(base + U_STA) & UART_RX_DATA_AVAIL;
}

/*
 * Read a single byte from rx fifo.
 * Blocking: waits until a character is received, then returns.
 * Return the character read directly from the UART's receive register.
 */
static int pic32_serial_getc(void __iomem *base)
{
	/* return error until data is available */
	if (!pic32_serial_tstc(base))
		return -EAGAIN;

	/* read the character from rx buffer */
	return readl(base + U_RXR) & 0xff;
}

static int pic32_uart_pending(struct udevice *dev, bool input)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	if (input)
		return pic32_serial_tstc(priv->base);

	return !(readl(priv->base + U_STA) & UART_TX_EMPTY);
}

static int pic32_uart_setbrg(struct udevice *dev, int baudrate)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	pic32_serial_init(priv->base, priv->uartclk, baudrate);
	return 0;
}

static int pic32_uart_putc(struct udevice *dev, const char ch)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	pic32_serial_putc(priv->base, ch);
	return 0;
}

static int pic32_uart_getc(struct udevice *dev)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	return pic32_serial_getc(priv->base);
}

static int pic32_uart_probe(struct udevice *dev)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	pic32_serial_init(priv->base, priv->uartclk, CONFIG_BAUDRATE);
	return 0;
}

static int pic32_uart_ofdata_to_platdata(struct udevice *dev)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);
	struct udevice *clkdev;
	fdt_addr_t addr;
	int ret;

	/* get address */
	addr = dev_get_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->base = pic32_ioremap(addr);

	/* get clock rate */
	ret = uclass_get_device(UCLASS_CLK, 0, &clkdev);
	if (ret) {
		printf("clk class not found, %d\n", ret);
		return ret;
	}

	priv->uartclk = clk_get_periph_rate(clkdev, PB2CLK);

	return 0;
}

static const struct dm_serial_ops pic32_uart_ops = {
	.putc		= pic32_uart_putc,
	.pending	= pic32_uart_pending,
	.getc		= pic32_uart_getc,
	.setbrg		= pic32_uart_setbrg,
};

static const struct udevice_id pic32_uart_ids[] = {
	{ .compatible = "microchip,pic32mzda-uart" },
	{}
};

U_BOOT_DRIVER(pic32_serial) = {
	.name		= "pic32-uart",
	.id		= UCLASS_SERIAL,
	.of_match	= pic32_uart_ids,
	.probe		= pic32_uart_probe,
	.ops		= &pic32_uart_ops,
	.flags		= DM_FLAG_PRE_RELOC,
	.ofdata_to_platdata = pic32_uart_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct pic32_uart_priv),
};

#ifdef CONFIG_DEBUG_UART_PIC32
#include <debug_uart.h>

static inline void _debug_uart_init(void)
{
	void __iomem *base = (void __iomem *)CONFIG_DEBUG_UART_BASE;

	pic32_serial_init(base, CONFIG_DEBUG_UART_CLOCK, CONFIG_BAUDRATE);
}

static inline void _debug_uart_putc(int ch)
{
	writel(ch, CONFIG_DEBUG_UART_BASE + U_TXR);
}

DEBUG_UART_FUNCS
#endif
