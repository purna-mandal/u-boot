/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>

/*
 * mips_io_port_base is the begin of the address space to which x86 style
 * I/O ports are mapped.
 */
const unsigned long mips_io_port_base = -1;

static const void __iomem *addr1 = (const void __iomem *)CKSEG1ADDR(0x80000000);
static void __iomem *addr2 = (void __iomem *)CKSEG1ADDR(0x80001000);

void test_readb(void)
{
	u8 val = readb(addr1);
	writeb(val, addr2);
}

void test_readw(void)
{
	u16 val = readw(addr1);
	writew(val, addr2);
}

void test_readl(void)
{
	u32 val = readl(addr1);
	writel(val, addr2);
}

void test_readq(void)
{
	u32 val = readq(addr1);
	writeq(val, addr2);
}

void test_readb_be(void)
{
	u8 val = readb_be(addr1);
	writeb_be(val, addr2);
}

void test_readw_be(void)
{
	u16 val = readw_be(addr1);
	writew_be(val, addr2);
}

void test_readl_be(void)
{
	u32 val = readl_be(addr1);
	writel_be(val, addr2);
}

void test_readq_be(void)
{
	u32 val = readq_be(addr1);
	writeq_be(val, addr2);
}

void test_raw_readb(void)
{
	u8 val = __raw_readb(addr1);
	__raw_writeb(val, addr2);
}

void test_raw_readw(void)
{
	u16 val = __raw_readw(addr1);
	__raw_writew(val, addr2);
}

void test_raw_readl(void)
{
	u32 val = __raw_readl(addr1);
	__raw_writel(val, addr2);
}

void test_raw_readq(void)
{
	u32 val = __raw_readq(addr1);
	__raw_writeq(val, addr2);
}

void test_clrbits_8(void)
{
	clrbits_8(addr2, BIT(2));
}

void test_setbits_8(void)
{
	setbits_8(addr2, BIT(3));
}

void test_clrsetbits_8(void)
{
	clrsetbits_8(addr2, BIT(2), BIT(3));
}

void test_clrbits_le16(void)
{
	clrbits_le16(addr2, BIT(9));
}

void test_setbits_le16(void)
{
	setbits_le16(addr2, BIT(10));
}

void test_clrsetbits_le16(void)
{
	clrsetbits_le16(addr2, BIT(9), BIT(10));
}

void test_clrbits_be16(void)
{
	clrbits_be16(addr2, BIT(9));
}

void test_setbits_be16(void)
{
	setbits_be16(addr2, BIT(10));
}

void test_clrsetbits_be16(void)
{
	clrsetbits_be16(addr2, BIT(9), BIT(10));
}

void test_clrbits_le32(void)
{
	clrbits_le32(addr2, BIT(9));
}

void test_setbits_le32(void)
{
	setbits_le32(addr2, BIT(10));
}

void test_clrsetbits_le32(void)
{
	clrsetbits_le32(addr2, BIT(9), BIT(10));
}

void test_clrbits_be32(void)
{
	clrbits_be32(addr2, BIT(9));
}

void test_setbits_be32(void)
{
	setbits_be32(addr2, BIT(10));
}

void test_clrsetbits_be32(void)
{
	clrsetbits_be32(addr2, BIT(9), BIT(10));
}

void test_clrbits_le64(void)
{
	clrbits_le64(addr2, BIT(9));
}

void test_setbits_le64(void)
{
	setbits_le64(addr2, BIT(10));
}

void test_clrsetbits_le64(void)
{
	clrsetbits_le64(addr2, BIT(9), BIT(10));
}

void test_clrbits_be64(void)
{
	clrbits_be64(addr2, BIT(9));
}

void test_setbits_be64(void)
{
	setbits_be64(addr2, BIT(10));
}

void test_clrsetbits_be64(void)
{
	clrsetbits_be64(addr2, BIT(9), BIT(10));
}
