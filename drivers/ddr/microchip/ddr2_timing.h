/*
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __MICROCHIP_DDR2_TIMING_H
#define __MICROCHIP_DDR2_TIMING_H

/* MPLL freq is 400MHz */
#define tCK		2500    /* 2500 psec */
#define tCK_CTRL	(tCK * 2)

/* Burst length in cycles */
#define BL		2
/* default CAS latency for all speed grades */
#define RL		5
/* default write latency for all speed grades = CL-1 */
#define WL		4

/* From Micron MT47H64M16HR-3 data sheet */
#define tRFC_MIN	127500	/* psec */
#define tWR		15000	/* psec */
#define tRP		12500	/* psec */
#define tRCD		12500	/* psec */
#define tRRD		7500	/* psec */
/* tRRD_TCK is minimum of 2 clk periods, regardless of freq */
#define tRRD_TCK	2
#define tWTR		7500	/* psec */
/* tWTR_TCK is minimum of 2 clk periods, regardless of freq */
#define tWTR_TCK	2
#define tRTP		7500	/* psec */
#define tRTP_TCK	(BL / 2)
#define tXP_TCK		2	/* clocks */
#define tCKE_TCK	3	/* clocks */
#define tXSNR		(tRFC_MIN + 10000) /* psec */
#define tDLLK		200     /* clocks */
#define tRAS_MIN	45000   /* psec */
#define tRC		57500   /* psec */
#define tFAW		35000   /* psec */
#define tMRD_TCK	2       /* clocks */
#define tRFI		7800000 /* psec */

/* DDR Addressing */
#define COL_BITS	10
#define BA_BITS		3
#define ROW_BITS	13
#define CS_BITS		1

/* DDR Addressing scheme: {CS, ROW, BA, COL} */
#define COL_HI_RSHFT	0
#define COL_HI_MASK	0
#define COL_LO_MASK	((1 << COL_BITS) - 1)

#define BA_RSHFT	COL_BITS
#define BA_MASK		((1 << BA_BITS) - 1)

#define ROW_ADDR_RSHIFT	(BA_RSHFT + BA_BITS)
#define ROW_ADDR_MASK	((1 << ROW_BITS) - 1)

#define CS_ADDR_RSHIFT	0
#define CS_ADDR_MASK	0

#endif	/* __MICROCHIP_DDR2_TIMING_H */
