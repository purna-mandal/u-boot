/*
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * Microchip PIC32MZ[DA] StarterKit.
 */

#ifndef __PIC32MZDASK_CONFIG_H
#define __PIC32MZDASK_CONFIG_H

/* System Configuration */
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_DISPLAY_BOARDINFO

/*--------------------------------------------
 * CPU configuration
 */
#define CONFIG_MIPS	        1
#define CONFIG_MIPS32		1  /* MIPS32 CPU core	*/

/* CPU Timer rate */
#define CONFIG_SYS_MIPS_TIMER_FREQ	100000000

/* Cache Configuration */
#define CONFIG_SYS_DCACHE_SIZE		32768
#define CONFIG_SYS_ICACHE_SIZE		32768
#define CONFIG_SYS_CACHELINE_SIZE	16
#define CONFIG_SYS_MIPS_CACHE_MODE	CONF_CM_CACHABLE_NONCOHERENT
#undef CONFIG_SKIP_LOWLEVEL_INIT

/*----------------------------------------------------------------------
 * Memory Layout
 */
#define CONFIG_SYS_SRAM_BASE		0x80000000
#define CONFIG_SYS_SRAM_SIZE		0x00080000 /* 512K in PIC32MZSK */

/* Initial RAM for temporary stack, global data */
#define CONFIG_SYS_INIT_RAM_SIZE	0x10000
#define CONFIG_SYS_INIT_RAM_ADDR	\
	(CONFIG_SYS_SRAM_BASE + CONFIG_SYS_SRAM_SIZE - CONFIG_SYS_INIT_RAM_SIZE)
#define CONFIG_SYS_GBL_DATA_OFFSET	\
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_OFFSET	CONFIG_SYS_GBL_DATA_OFFSET
#define CONFIG_SYS_INIT_SP_ADDR		\
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/* SDRAM Configuration (for final code, data, stack, heap) */
#define CONFIG_SYS_SDRAM_BASE		0x88000000
#define CONFIG_SYS_MEM_SIZE		(128 << 20) /* 128M */

#define CONFIG_SYS_MALLOC_LEN		(256 << 10)
#define CONFIG_SYS_BOOTPARAMS_LEN	(4 << 10)
#define CONFIG_STACKSIZE		(4 << 10) /* regular stack */
#define CONFIG_SYS_EXCEPTION_ADDR	0xA000100 /* EBASE ADDR */

#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_MONITOR_LEN		(192 << 10)

#define CONFIG_SYS_LOAD_ADDR		0x88500000 /* default load address */
#define CONFIG_SYS_ENV_ADDR		0x88300000

/* Memory Test */
#define CONFIG_SYS_MEMTEST_START	0x88000000
#define CONFIG_SYS_MEMTEST_END		0x88080000

/*----------------------------------------------------------------------
 * Commands
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_CMD_CLK
#define CONFIG_CMD_MEMTEST
#define CONFIG_CMD_MEMINFO

/*-------------------------------------------------
 * FLASH configuration
 */
#define CONFIG_SYS_MAX_FLASH_BANKS	2  /* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_SECT	64 /* max number of sectors */
#define CONFIG_SYS_FLASH_SIZE		(1 << 20) /* 1M, size of one bank */
#define PHYS_FLASH_1			0x1D000000 /* Flash Bank #1 */
#define PHYS_FLASH_2			0x1D100000 /* Flash Bank #2 */
#define CONFIG_SYS_FLASH_BANKS_LIST	{PHYS_FLASH_1, PHYS_FLASH_2}
#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH_1
#define PHYS_FLASH_SECT_SIZE		\
	(CONFIG_SYS_FLASH_SIZE / CONFIG_SYS_MAX_FLASH_SECT)

/* FLASH erase/programming timeout (in ticks) */
#define CONFIG_SYS_FLASH_ERASE_TOUT	(2 * CONFIG_SYS_HZ)
#define CONFIG_SYS_FLASH_WRITE_TOUT	(25 * CONFIG_SYS_HZ)

/*------------------------------------------------------------
 * Console Configuration
 */
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}
#define CONFIG_SYS_CBSIZE		1024 /* Console I/O Buffer Size   */
#define CONFIG_SYS_MAXARGS		16   /* max number of command args*/
#define CONFIG_SYS_PBSIZE		\
		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_CMDLINE_EDITING		1

/*
 * Handover flattened device tree (dtb file) to Linux kernel
 */
#define CONFIG_OF_LIBFDT	1

/* -------------------------------------------------
 * Environment
 */
#define CONFIG_ENV_IS_IN_FLASH	1
#define CONFIG_ENV_SECT_SIZE	0x4000 /* 16K(one sector) for env */
#define CONFIG_ENV_SIZE		0x4000
#define CONFIG_ENV_ADDR		0x9d0fc000 /* Last sector from Bank 0 */

/* ---------------------------------------------------------------------
 * Board boot configuration
 */
#define CONFIG_TIMESTAMP	/* Print image info with timestamp */
#define CONFIG_BOOTDELAY	5 /* autoboot after X seconds     */
#undef	CONFIG_BOOTARGS

#define CONFIG_MEMSIZE_IN_BYTES		/* pass 'memsize=' in bytes */
#endif	/* __PIC32MZDASK_CONFIG_H */
