/*
 * (c) 2015 Paul Thacker <paul.thacker@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */
#include <common.h>
#include <dm.h>
#include <clk.h>
#include <errno.h>
#include <config.h>
#include <serial.h>
#include <linux/bitops.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/clock.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_ENABLE		BIT(15)
#define UART_ENABLE_RX		BIT(12)
#define UART_ENABLE_TX		BIT(10)
#define UART_RX_DATA_AVAIL	BIT(0)
#define UART_RX_OERR		BIT(1)
#define UART_TX_FULL		BIT(9)

/* UART Control */
#define U_BASE(x)	(x)
#define U_MODE(x)       U_BASE(x)
#define U_MODECLR(x)    (U_MODE(x) + _CLR_OFFSET)
#define U_MODESET(x)    (U_MODE(x) + _SET_OFFSET)
#define U_STA(x)        (U_BASE(x) + 0x10)
#define U_STACLR(x)     (U_STA(x) + _CLR_OFFSET)
#define U_STASET(x)     (U_STA(x) + _SET_OFFSET)
#define U_TXREG(x)      (U_BASE(x) + 0x20)
#define U_RXREG(x)      (U_BASE(x) + 0x30)
#define U_BRG(x)        (U_BASE(x) + 0x40)

struct pic32_uart_priv {
	void __iomem *regs;
	ulong uartclk;
};

static void pic32_serial_setbrg(void __iomem *regs, ulong uart_clk, u32 baud)
{
	writel(0, U_BRG(regs));
	writel((uart_clk / baud / 16) - 1, U_BRG(regs));
	udelay(100);
}

/*
 * Initialize the serial port with the given baudrate.
 * The settings are always 8 data bits, no parity, 1 stop bit, no start bits.
 */
static int pic32_serial_init(void __iomem *regs, ulong clk, u32 baudrate)
{
	/* disable and clear mode */
	writel(0, U_MODE(regs));
	writel(0, U_STA(regs));

	/* set baud rate generator */
	pic32_serial_setbrg(regs, clk, baudrate);

	/* enable the UART for TX and RX */
	writel(UART_ENABLE_TX | UART_ENABLE_RX, U_STASET(regs));

	/* enable the UART */
	writel(UART_ENABLE, U_MODESET(regs));
	return 0;
}

/* Output a single byte to the serial port */
static void pic32_serial_putc(void __iomem *regs, const char c)
{
	/* if \n, then add a \r */
	if (c == '\n')
		pic32_serial_putc(regs, '\r');

	/* Wait for Tx FIFO not full */
	while (readl(U_STA(regs)) & UART_TX_FULL)
		;

	/* stuff the tx buffer with the character */
	writel(c, U_TXREG(regs));
}

/* Test whether a character is in the RX buffer */
static int pic32_serial_tstc(void __iomem *regs)
{
	/* check if rcv buf overrun error has occurred */
	if (readl(U_STA(regs)) & UART_RX_OERR) {
		readl(U_RXREG(regs));

		/* clear OERR to keep receiving */
		writel(UART_RX_OERR, U_STACLR(regs));
	}

	if (readl(U_STA(regs)) & UART_RX_DATA_AVAIL)
		return 1;	/* yes, there is data in rcv buffer */
	else
		return 0;	/* no data in rcv buffer */
}

/*
 * Read a single byte from the rx buffer.
 * Blocking: waits until a character is received, then returns.
 * Return the character read directly from the UART's receive register.
 *
 */
static int pic32_serial_getc(void __iomem *regs)
{
	/* wait here until data is available */
	while (!pic32_serial_tstc(regs))
		;

	/* read the character from the rcv buffer */
	return readl(U_RXREG(regs));
}

static int pic32_uart_pending(struct udevice *dev, bool input)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	return pic32_serial_tstc(priv->regs);
}

static int pic32_uart_setbrg(struct udevice *dev, int baudrate)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	pic32_serial_init(priv->regs, priv->uartclk, baudrate);
	return 0;
}

static int pic32_uart_putc(struct udevice *dev, const char ch)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	pic32_serial_putc(priv->regs, ch);
	return 0;
}

static int pic32_uart_getc(struct udevice *dev)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	return pic32_serial_getc(priv->regs);
}

static int pic32_uart_probe(struct udevice *dev)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);

	pic32_serial_init(priv->regs, priv->uartclk, CONFIG_BAUDRATE);
	return 0;
}

static int pic32_uart_ofdata_to_platdata(struct udevice *dev)
{
	struct pic32_uart_priv *priv = dev_get_platdata(dev);
	struct udevice *clkdev;
	int ret;

	priv->regs = map_physmem(dev_get_addr(dev),
				 0x100,
				 MAP_NOCACHE);

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
	.putc = pic32_uart_putc,
	.pending = pic32_uart_pending,
	.getc = pic32_uart_getc,
	.setbrg = pic32_uart_setbrg,
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
	void __iomem *regs = (void __iomem *)CONFIG_DEBUG_UART_BASE;

	pic32_serial_init(regs, CONFIG_DEBUG_UART_CLOCK, CONFIG_BAUDRATE);
}

static inline void _debug_uart_putc(int ch)
{
	writel(ch, U_TXREG(CONFIG_DEBUG_UART_BASE));
}

DEBUG_UART_FUNCS

#endif
