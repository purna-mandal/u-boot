/*
 * Copyright (C) 2015
 * Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <mach/ebi.h>
#include <mach/pic32.h>

/* EBI Registers */
#define EBICS0		0x14
#define EBICS1		0x18
#define EBICS2		0x1C
#define EBICS3		0x20
#define EBIMSK0		0x54
#define EBIMSK1		0x58
#define EBIMSK2		0x5C
#define EBIMSK3		0x60
#define EBISMT0		0x94
#define EBISMT1		0x98
#define EBISMT2		0x9C
#define EBISMT3		0xA0
#define EBISMCON	0xA4

void setup_ebi_sram(struct pic32_ebi_conf *ebi_conf)
{
	void *ebi_base = (void __iomem *)ioremap(PIC32_EBI_BASE, 0x100);
	void *cfg_base = (void __iomem *)ioremap(PIC32_CFG_BASE, 0x100);

	/*
	 * Enable address lines [0:23]
	 * Controls access of pins shared with PMP
	 */
	writel(0x80FFFFFF, cfg_base + CFGEBIA);

	/*
	 * Enable write enable pin
	 * Enable output enable pin
	 * Enable byte select pin 0
	 * Enable byte select pin 1
	 * Enable byte select pin 2
	 * Enable byte select pin 3
	 * Enable Chip Select 0
	 * Enable data pins [0:15]
	 */
	writel(0x000033F3, cfg_base + CFGEBIC);

	/*
	 * Connect CS0/CS1/CS2/CS3 to physical address
	 */
	writel(ebi_conf->csaddr[0], ebi_base + EBICS0);
	writel(ebi_conf->csaddr[1], ebi_base + EBICS1);
	writel(ebi_conf->csaddr[2], ebi_base + EBICS2);
	writel(ebi_conf->csaddr[3], ebi_base + EBICS3);

	/*
	 * Set memory size, memory type.
	 * Uses timing numbers from EBISMT0-3
	 */
	writel(ebi_conf->addrmsk[0], ebi_base + EBIMSK0);
	writel(ebi_conf->addrmsk[1], ebi_base + EBIMSK1);
	writel(ebi_conf->addrmsk[2], ebi_base + EBIMSK2);
	writel(ebi_conf->addrmsk[3], ebi_base + EBIMSK3);

	/*
	 * Configure EBISMTx
	 * ISSI device has read cycles time of 6 ns
	 * ISSI device has address setup time of 0ns
	 * ISSI device has address/data hold time of 2.5 ns
	 * ISSI device has Write Cycle Time of 6 ns
	 * Bus turnaround time is 0 ns
	 * No page mode
	 * No page size
	 * No RDY pin
	 */
	writel(ebi_conf->smtiming[0], ebi_base + EBISMT0);
	writel(ebi_conf->smtiming[1], ebi_base + EBISMT1);
	writel(ebi_conf->smtiming[2], ebi_base + EBISMT2);
	writel(ebi_conf->smtiming[3], ebi_base + EBISMT3);

	/*
	 * Keep default data width to 16-bits
	 */
	writel(ebi_conf->smwidth, ebi_base + EBISMCON);
}

#ifdef CONFIG_SYS_DRAM_TEST
static void write_pattern(u32 p, u32 size, void *base)
{
	u32 *addr = (u32 *)base;
	u32 loop;

	printf("mem: write test pattern 0x%x ...", p);

	for (loop = 0; loop < size / 4; loop++)
		*addr++ = p;

	printf("finished\n");
}

static void read_pattern(u32 p, u32 size, void *base)
{
	u32 *addr = (u32 *)base;
	u32 loop;
	u32 val;

	printf("mem: read test pattern 0x%x ...", p);

	for (loop = 0 ; loop < size / 4; loop++) {
		val = *addr++;
		if (val != p) {
			printf("pattern 0x%x failed at %p\n",
			       p, (void *)addr);
			panic("mem: is bad");
		}
	}
	printf("success\n");
}

void run_memory_test(u32 size, void *base)
{
	u32 *addr;
	u32 loop;
	u32 val;
	u32 count = 0;

	printf("mem: running pattern test on [%p - %p]\n",
	       base, base + size - 1);

	write_pattern(0xFFFFFFFF, size, base);
	read_pattern(0xFFFFFFFF, size, base);

	write_pattern(0xa5a5a5a5, size, base);
	read_pattern(0xa5a5a5a5, size, base);

	write_pattern(0x5a5a5a5a, size, base);
	read_pattern(0x5a5a5a5a, size, base);

	write_pattern(0x5555aaaa, size, base);
	read_pattern(0x5555aaaa, size, base);

	write_pattern(0xaaaaaaaa, size, base);
	read_pattern(0xaaaaaaaa, size, base);

	write_pattern(0x55555555, size, base);
	read_pattern(0x55555555, size, base);

	write_pattern(0x33333333, size, base);
	read_pattern(0x33333333, size, base);

	write_pattern(0xcccccccc, size, base);
	read_pattern(0xcccccccc, size, base);

	write_pattern(0x0F0F0F0F, size, base);
	read_pattern(0x0F0F0F0F, size, base);

	write_pattern(0xF0F0F0F0, size, base);
	read_pattern(0xF0F0F0F0, size, base);

	write_pattern(0xFF00FF00, size, base);
	read_pattern(0xFF00FF00, size, base);

	write_pattern(0x00FF00FF, size, base);
	read_pattern(0x00FF00FF, size, base);

	write_pattern(0x0, size, base);
	read_pattern(0x0, size, base);

	printf("mem: write test running...");


	/* address test */
	count = 0;
	addr = (u32 *)base;
	for (loop = 0; loop < size / 4; loop++)
		*addr++ = count++;

	printf("finished\n");
	printf("mem: read test running...");

	count = 0;
	addr = (u32 *)base;
	for (loop = 0 ; loop < size / 4; loop++) {
		val = *addr++;
		if (val != count) {
			printf("failed at 0x%x: 0x%x != 0x%x\n",
			       loop * 4, val, count);
			panic("mem: is bad");
		}
		count++;
	}
	printf("success\n");
}
#endif

