/*
 * (c) 2015 Paul Thacker <paul.thacker@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __PIC32_REGS_H__
#define __PIC32_REGS_H__

#define _CLR_OFFSET	(0x4)
#define _SET_OFFSET	(0x8)
#define _INV_OFFSET	(0xc)

/* System Configuration */
#define PIC32_CFG_BASE(x) 0xbf800000
#define CFGCON		(PIC32_CFG_BASE(x))
#define DEVID		(PIC32_CFG_BASE(x) + 0x0020)
#define SYSKEY		(PIC32_CFG_BASE(x) + 0x0030)
#define PMD1		(PIC32_CFG_BASE(x) + 0x0040)
#define PMD7		(PIC32_CFG_BASE(x) + 0x00a0)
#define CFGEBIA		(PIC32_CFG_BASE(x) + 0x00c0)
#define CFGEBIC		(PIC32_CFG_BASE(x) + 0x00d0)
#define CFGPG		(PIC32_CFG_BASE(x) + 0x00e0)
#define CFGMPLL		(PIC32_CFG_BASE(x) + 0x0100)

/* Clock & Reset */
#define RESET_BASE	0xbf800000

/* Non Volatile Memory (NOR flash) */
#define PIC32_NVM_BASE  (RESET_BASE + 0x0600)

/* Reset Control Registers */
#define RSWRST		(RESET_BASE + 0x1250)

/* Oscillator Configuration */
#define OSCCON		(RESET_BASE + 0x1200)
#define SPLLCON		(RESET_BASE + 0x1220)
#define REFO1CON	(RESET_BASE + 0x1280)
#define REFO1TRIM	(RESET_BASE + 0x1290)
#define PB1DIV		(RESET_BASE + 0x1340)

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

/* Peripheral Pin Select Input */
#define PPS_IN_BASE	0xbf800000
#define U1RXR		(PPS_IN_BASE + 0x1468)
#define U2RXR		(PPS_IN_BASE + 0x1470)
#define SDI1R		(PPS_IN_BASE + 0x149c)
#define SDI2R		(PPS_IN_BASE + 0x14a8)

/* Peripheral Pin Select Output */
#define PPS_OUT_BASE	0xbf801500
#define PPS_OUT(prt, pi)(PPS_OUT_BASE + ((((prt) * 16) + (pi)) << 2))
#define RPA14R		PPS_OUT(PIC32_PORT_A, 14)
#define RPB0R		PPS_OUT(PIC32_PORT_B, 0)
#define RPB14R		PPS_OUT(PIC32_PORT_B, 14)
#define RPD0R		PPS_OUT(PIC32_PORT_D, 0)
#define RPD3R		PPS_OUT(PIC32_PORT_D, 3)
#define RPG8R		PPS_OUT(PIC32_PORT_G, 8)
#define RPG9R		PPS_OUT(PIC32_PORT_G, 9)

/* Peripheral Pin Control */
#define PINCTRL_BASE(x)	(0xbf860000 + (x * 0x0100))
#define ANSEL(x)	(PINCTRL_BASE(x) + 0x00)
#define ANSELCLR(x)	(ANSEL(x) + _CLR_OFFSET)
#define ANSELSET(x)	(ANSEL(x) + _SET_OFFSET)
#define TRIS(x)		(PINCTRL_BASE(x) + 0x10)
#define TRISCLR(x)	(TRIS(x) + _CLR_OFFSET)
#define TRISSET(x)	(TRIS(x) + _SET_OFFSET)
#define PORT(x)		(PINCTRL_BASE(x) + 0x20)
#define PORTCLR(x)	(PORT(x) + _CLR_OFFSET)
#define PORTSET(x)	(PORT(x) + _SET_OFFSET)
#define LAT(x)		(PINCTRL_BASE(x) + 0x30)
#define LATCLR(x)	(LAT(x) + _CLR_OFFSET)
#define LATSET(x)	(LAT(x) + _SET_OFFSET)
#define ODC(x)		(PINCTRL_BASE(x) + 0x40)
#define ODCCLR(x)	(ODC(x) + _CLR_OFFSET)
#define ODCSET(x)	(ODC(x) + _SET_OFFSET)
#define CNPU(x)		(PINCTRL_BASE(x) + 0x50)
#define CNPUCLR(x)	(CNPU(x) + _CLR_OFFSET)
#define CNPUSET(x)	(CNPU(x) + _SET_OFFSET)
#define CNPD(x)		(PINCTRL_BASE(x) + 0x60)
#define CNPDCLR(x)	(CNPD(x) + _CLR_OFFSET)
#define CNPDSET(x)	(CNPD(x) + _SET_OFFSET)
#define CNCON(x)	(PINCTRL_BASE(x) + 0x70)
#define CNCONCLR(x)	(CNCON(x) + _CLR_OFFSET)
#define CNCONSET(x)	(CNCON(x) + _SET_OFFSET)

/* Get gpio# from peripheral port# and pin# */
#define GPIO_PORT_PIN(_port, _pin) \
		(((_port) << 4) + (_pin))

/* USB Core */
#define PIC32_USB_CORE_BASE	0xbf8e3000
#define PIC32_USB_CTRL_BASE	0xbf884000

/* SPI1-SPI6 */
#define PIC32_SPI1_BASE		0xbf821000

/* Prefetch Module */
#define PREFETCH_BASE	0xbf8e0000
#define PRECON		(PREFETCH_BASE)
#define PRECONCLR	(PRECON + _CLR_OFFSET)
#define PRECONSET	(PRECON + _SET_OFFSET)
#define PRECONINV	(PRECON + _INV_OFFSET)

#define PRESTAT		(PREFETCH_BASE + 0x0010)
#define PRESTATCLR	(PRESTAT + _CLR_OFFSET)
#define PRESTATSET	(PRESTAT + _SET_OFFSET)
#define PRESTATINV	(PRESTAT + _INV_OFFSET)

/* DDR2 Controller */
#define PIC32_DDR2C_BASE	0xbf8e8000

/* DDR2 PHY */
#define PIC32_DDR2P_BASE	0xbf8e9100

/* EBI */
#define PIC32_EBI_BASE		0xbf8e1000

/* SQI */
#define PIC32_SQI_BASE		0xbf8e2000

struct pic32_reg_atomic {
	u32 raw;
	u32 clr;
	u32 set;
	u32 inv;
};

/* Core */
char *get_core_name(void);

#endif	/* __PIC32_REGS_H__ */
