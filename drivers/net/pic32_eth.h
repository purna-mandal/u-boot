/*
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef PIC32_ETH_H
#define PIC32_ETH_H

#include <stddef.h>
#include <asm/arch-pic32/pic32.h>

/* Ethernet */
struct pic32_ectl_regs {
	struct pic32_reg_atomic con1; /* 0x00 */
	struct pic32_reg_atomic con2; /* 0x10 */
	struct pic32_reg_atomic txst; /* 0x20 */
	struct pic32_reg_atomic rxst; /* 0x30 */
	struct pic32_reg_atomic ht0;  /* 0x40 */
	struct pic32_reg_atomic ht1;  /* 0x50 */
	struct pic32_reg_atomic pmm0; /* 0x60 */
	struct pic32_reg_atomic pmm1; /* 0x70 */
	struct pic32_reg_atomic pmcs; /* 0x80 */
	struct pic32_reg_atomic pmo;  /* 0x90 */
	struct pic32_reg_atomic rxfc; /* 0xA0 */
	struct pic32_reg_atomic rxwm; /* 0xB0 */
	struct pic32_reg_atomic ien;  /* 0xC0 */
	struct pic32_reg_atomic irq;  /* 0xD0 */
	struct pic32_reg_atomic stat; /* 0xE0 */
};

struct pic32_mii_regs {
	struct pic32_reg_atomic mcfg; /* 0x280 */
	struct pic32_reg_atomic mcmd; /* 0x290 */
	struct pic32_reg_atomic madr; /* 0x2a0 */
	struct pic32_reg_atomic mwtd; /* 0x2b0 */
	struct pic32_reg_atomic mrdd; /* 0x2c0 */
	struct pic32_reg_atomic mind; /* 0x2d0 */
};

struct pic32_emac_regs {
	struct pic32_reg_atomic cfg1; /* 0x200*/
	struct pic32_reg_atomic cfg2; /* 0x210*/
	struct pic32_reg_atomic ipgt; /* 0x220*/
	struct pic32_reg_atomic ipgr; /* 0x230*/
	struct pic32_reg_atomic clrt; /* 0x240*/
	struct pic32_reg_atomic maxf; /* 0x250*/
	struct pic32_reg_atomic supp; /* 0x260*/
	struct pic32_reg_atomic test; /* 0x270*/
	struct pic32_mii_regs mii; /* 0x280 - 0x2d0 */
	struct pic32_reg_atomic resv1[2]; /* 0x2e0 - 0x2f0 */
	struct pic32_reg_atomic sa0;  /* 0x300 */
	struct pic32_reg_atomic sa1;  /* 0x310 */
	struct pic32_reg_atomic sa2;  /* 0x320 */
};

/* ETHCON1 Reg field */
#define ETHCON_ON		0x8000
#define ETHCON_TXRTS		0x0200
#define ETHCON_RXEN		0x0100
#define ETHCON_BUFCDEC		0x0001

/* ETHCON2 Reg field */
#define ETHCON_RXBUFSZ		0x7f
#define ETHCON_RXBUFSZ_SHFT	0x00000004

/* ETHSTAT Reg field */
#define ETHSTAT_BUSY		0x00000080
#define ETHSTAT_BUFCNT		0x00FF0000

/* ETHRXFC Register fields */
#define ETHRXFC_CRCOKEN	0x0040
#define ETHRXFC_RUNTEN	0x0010
#define ETHRXFC_UCEN	0x0008
#define ETHRXFC_MCEN	0x0002
#define ETHRXFC_BCEN	0x0001

/* EMAC1CFG1 register offset */
#define PIC32_EMAC1CFG1	0x0200

/* EMAC1CFG1 register fields */
#define EMAC_SOFTRESET	0x8000
#define EMAC_TXPAUSE	0x0008
#define EMAC_RXPAUSE	0x0004
#define EMAC_RXENABLE	0x0001

/* EMAC1CFG2 register fields */
#define EMAC_EXCESS	0x4000
#define EMAC_AUTOPAD	0x0080
#define EMAC_PADENABLE	0x0020
#define EMAC_CRCENABLE	0x0010
#define EMAC_LENGTHCK	0x0002
#define EMAC_FULLDUP	0x0001

/* EMAC1IPGT register magic */
#define FULLDUP_GAP_TIME	0x15
#define HALFDUP_GAP_TIME	0x12

/* EMAC1SUPP register fields */
#define EMAC_RMII_SPD100	0x0100
#define EMAC_RMII_RESET		0x0800

/* MII Management Configuration Register */
#define MIIMCFG_RSTMGMT		0x8000
#define MIIMCFG_CLKSEL_DIV40	0x0020	/* 100Mhz / 40 */

/* MII Management Command Register */
#define MIIMCMD_READ		0x01
#define MIIMCMD_SCAN		0x02

/* MII Management Address Register */
#define MIIMADD_PHYADDR_SHIFT	8
#define MIIMADD_REGADDR		0x1f
#define MIIMADD_REGADDR_SHIFT	0

/* MII Management Indicator Register */
#define MIIMIND_BUSY		0x01
#define MIIMIND_NOTVALID	0x04
#define MIIMIND_LINKFAIL	0x08

/* Packet Descriptor
 *
 * Descriptor of a packet accepted by the TX/RX Ethernet engine.
 * ref: PIC32 Family Reference Manual Table 35-7
 *
 * A packet handled by the Ethernet TX/RX engine is a list of buffer
 * descriptors. A packet may consist of more than one buffer.
 * Each buffer needs a descriptor.
 *
 */

/* Received Packet Status */
#define _RSV1_PKT_CSUM	0xffff
#define _RSV2_RX_OK	(1 << 23)
#define _RSV2_LEN_ERR	(1 << 21)
#define _RSV2_CRC_ERR	(1 << 20)
#define _RSV2_RX_COUNT	0xffff

#define RSV_RX_CSUM(__rsv1)	((__rsv1) & _RSV1_PKT_CSUM)
#define RSV_RX_COUNT(__rsv2)	((__rsv2) & _RSV2_RX_COUNT)
#define RSV_RX_OK(__rsv2)	((__rsv2) & _RSV2_RX_OK)
#define RSV_CRC_ERR(__rsv2)	((__rsv2) & _RSV2_CRC_ERR)

/* Ethernet Hardware Descriptor Header bits */
#define EDH_EOWN	0x00000080
#define EDH_NPV		0x00000100
#define EDH_STICKY	0x00000200
#define _EDH_BCOUNT	0x07ff0000
#define EDH_EOP		0x40000000
#define EDH_SOP		0x80000000
#define EDH_BCOUNT_SHIFT	16
#define EDH_BCOUNT(len)	((len) << EDH_BCOUNT_SHIFT)

/* Ethernet Hardware Descriptors
 * ref: PIC32 Family Reference Manual Table 35-7
 *
 * This structure represents the layout of the DMA
 * memory shared between the CPU and the Ethernet
 * controller.
 */
/* TX/RX DMA descriptor */
struct eth_dma_desc {
	u32 hdr;	/* header */
	u32 data_buff;	/* data buffer address */
	u32 stat1;	/* transmit/receive packet status */
	u32 stat2;	/* transmit/receive packet status */
	u32 next_ed;	/* next descriptor */
};

/* Delay/Timeout range */
#define DELAY_10MSEC	10000UL

/* cache ops helper */
#define __dcache_flush(__a, __l) \
	flush_dcache_range((ulong)(__a),  ((__l) + (ulong)(__a)))

#define __dcache_invalidate(__a, __l) \
	invalidate_dcache_range((ulong)(__a),  ((__l) + (ulong)(__a)))

#define PIC32_MDIO_NAME "PIC32-EMAC"
int pic32_mdio_init(const char *name, ulong ioaddr);

#endif /**/
