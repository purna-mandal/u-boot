/*
 * (c) 2015 Paul Thacker <paul.thacker@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __PIC32_REGS_H__
#define __PIC32_REGS_H__

#include <asm/io.h>

/* System Configuration */
#define PIC32_CFG_BASE		0x1f800000

/* System config register offsets */
#define CFGCON		0x0000
#define DEVID		0x0020
#define SYSKEY		0x0030
#define PMD1		0x0040
#define PMD7		0x00a0
#define CFGEBIA		0x00c0
#define CFGEBIC		0x00d0
#define CFGPG		0x00e0
#define CFGMPLL		0x0100

/* Non Volatile Memory (NOR flash) */
#define PIC32_NVM_BASE		(PIC32_CFG_BASE + 0x0600)
/* Oscillator Configuration */
#define PIC32_OSC_BASE		(PIC32_CFG_BASE + 0x1200)
/* Peripheral Pin Select Input */
#define PPS_IN_BASE		0x1f801400
/* Peripheral Pin Select Output */
#define PPS_OUT_BASE		0x1f801500
/* Pin Config */
#define PINCTRL_BASE		0x1f860000

/* USB Core */
#define PIC32_USB_CORE_BASE	0x1f8e3000
#define PIC32_USB_CTRL_BASE	0x1f884000

/* SPI1-SPI6 */
#define PIC32_SPI1_BASE		0x1f821000

/* Prefetch Module */
#define PREFETCH_BASE		0x1f8e0000

/* DDR2 Controller */
#define PIC32_DDR2C_BASE	0x1f8e8000

/* DDR2 PHY */
#define PIC32_DDR2P_BASE	0x1f8e9100

/* EBI */
#define PIC32_EBI_BASE		0x1f8e1000

/* SQI */
#define PIC32_SQI_BASE		0x1f8e2000

struct pic32_reg_atomic {
	u32 raw;
	u32 clr;
	u32 set;
	u32 inv;
};

#define _CLR_OFFSET	0x04
#define _SET_OFFSET	0x08
#define _INV_OFFSET	0x0c

static inline void __iomem *pic32_get_syscfg_base(void)
{
	return (void __iomem *)CKSEG1ADDR(PIC32_CFG_BASE);
}

/* Core */
const char *get_core_name(void);

/* EBI */
void setup_ebi_sram(void);
void run_memory_test(u32 size, void *base);

/* MMU */
void write_one_tlb(int index, u32 pagemask, u32 hi, u32 low0, u32 low1);

static inline unsigned long pic32_virt_to_uncac(unsigned long addr)
{
	if ((KSEGX(addr) == KSEG2) || (KSEGX(addr) == KSEG3))
		return CKSEG3ADDR(addr);

	return CKSEG1ADDR(addr);
}

static inline unsigned long pic32_virt_to_phys(void *address)
{
	unsigned long ret;

	ret = (unsigned long)virt_to_phys(address);
	if ((KSEGX(address) == KSEG2) || (KSEGX(address) == KSEG3))
		ret |= 0x20000000;

	return ret;
}

#define TLB_ENTRYLO(_a, _cm, _f) (((_a) >> 6) | ((_cm) << 3) | (_f))
#define ENTRYLO_CAC(_pa) TLB_ENTRYLO((_pa), CONFIG_SYS_MIPS_CACHE_MODE, 0x7)
#define ENTRYLO_UNC(_pa) TLB_ENTRYLO((_pa), CONF_CM_UNCACHED, 0x7)

#endif	/* __PIC32_REGS_H__ */
