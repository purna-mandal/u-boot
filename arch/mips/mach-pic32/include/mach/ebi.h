/*
 * (c) 2016 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __PIC32_EBI_H__
#define __PIC32_EBI_H__

/* External Bus Interface configuration */
struct pic32_ebi_conf {
	u32 csaddr[4];
	u32 addrmsk[4];
	u32 smtiming[4];
	u32 smwidth;
	u32 addrcfg;
	u32 ctrlcfg;
};

/* EBI memory device size */
#define EBI_MEMSZ_1M	0x005
#define EBI_MEMSZ_2M	0x006
#define EBI_MEMSZ_4M	0x007
#define EBI_MEMSZ_8M	0x008
#define EBI_MEMSZ_16M	0x009

/* EBI device type */
#define EBI_NOR_FLASH	0x040
#define EBI_SRAM	0x020

/* EBI static memory timing */
#define EBI_SMT0	0x0000
#define EBI_SMT1	0x0100
#define EBI_SMT2	0x0200

void setup_ebi_sram(struct pic32_ebi_conf *ebi);
void run_memory_test(u32 size, void *base);

#endif /* __PIC32_EBI_H__ */
