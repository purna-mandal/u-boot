/*
 * (c) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * Microchip PIC32MZ[DA] Starter Kit.
 */

#ifndef __PIC32MZDASK_CONFIG_H
#define __PIC32MZDASK_CONFIG_H

/* System Configuration */
#define CONFIG_SYS_TEXT_BASE		0x80008000 //0x9d004000 /* .text */
#define CONFIG_DISPLAY_BOARDINFO

/*--------------------------------------------
 * CPU configuration
 */
/* CPU Timer rate */
#define CONFIG_SYS_MIPS_TIMER_FREQ	100000000

/* Cache Configuration */
#define CONFIG_SYS_MIPS_CACHE_MODE	CONF_CM_CACHABLE_NONCOHERENT
#undef CONFIG_SKIP_LOWLEVEL_INIT

/*----------------------------------------------------------------------
 * Memory Layout
 */
#define CONFIG_SYS_SRAM_BASE		0x80000000
#define CONFIG_SYS_SRAM_SIZE		0x00080000 /* 512K */

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
#define CONFIG_SYS_MALLOC_LEN		(256 << 10)
#define CONFIG_SYS_BOOTPARAMS_LEN	(4 << 10)
#define CONFIG_STACKSIZE		(4 << 10) /* regular stack */

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
#define CONFIG_SYS_NO_FLASH

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

/*-----------------------------------------------------------------------
 * Networking Configuration
 */
#define CONFIG_MII
#define CONFIG_PHY_SMSC
#define CONFIG_PHY_ADDR			0 /* LAN87XX */
#define CONFIG_SYS_RX_ETH_BUFFER	8
#define CONFIG_NET_RETRY_COUNT		20
#define CONFIG_ARP_TIMEOUT		500 /* millisec */

#define CONFIG_CMD_MII
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/*
 * Handover flattened device tree (dtb file) to Linux kernel
 */
#define CONFIG_OF_LIBFDT	1

/*-----------------------------------------------------------------------
 * SDHC Configuration
 */
#define CONFIG_SDHCI
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_MMC

/*-----------------------------------------------------------------------
 * File System Configuration
 */
/* FAT FS */
#define CONFIG_DOS_PARTITION
#define CONFIG_PARTITION_UUIDS
#define CONFIG_SUPPORT_VFAT
#define CONFIG_FS_FAT
#define CONFIG_FAT_WRITE
#define CONFIG_CMD_FS_GENERIC
#define CONFIG_CMD_PART
#define CONFIG_CMD_FAT

/* EXT4 FS */
#define CONFIG_FS_EXT4
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE

/* -------------------------------------------------
 * Environment
 */
#define CONFIG_ENV_IS_NOWHERE	1
#define CONFIG_ENV_SIZE		0x4000

/* ---------------------------------------------------------------------
 * Board boot configuration
 */
#define CONFIG_TIMESTAMP	/* Print image info with timestamp */
#define CONFIG_BOOTDELAY	5 /* autoboot after X seconds     */
#undef	CONFIG_BOOTARGS

#define CONFIG_EXTRA_ENV_SETTINGS				\
	"loadaddr="__stringify(CONFIG_SYS_LOAD_ADDR)"\0"	\
	"uenvfile=uEnv.txt\0"					\
	"uenvaddr="__stringify(CONFIG_SYS_ENV_ADDR)"\0"		\
	"scriptfile=boot.scr\0"					\
	"ubootfile=u-boot.bin\0"				\
	"ethaddr=00:04:A3:3E:37:D2\0"		\
	"serverip=10.41.20.11\0"		\
	"gatewayip=10.41.21.1\0"		\
	"ipaddr=10.41.21.200\0"			\
	"netmask=255.255.0.0\0"			\
	"importbootenv= "					\
		"env import -t -r ${uenvaddr} ${filesize};\0"	\
								\
	"tftploadenv=tftp ${uenvaddr} ${uenvfile} \0"		\
	"tftploadscr=tftp ${uenvaddr} ${scriptfile} \0"		\
	"tftploadub=tftp ${loadaddr} ${ubootfile} \0"		\
								\
	"mmcloadenv=fatload mmc 0 ${uenvaddr} ${uenvfile}\0"	\
	"mmcloadscr=fatload mmc 0 ${uenvaddr} ${scriptfile}\0"	\
	"mmcloadub=fatload mmc 0 ${loadaddr} ${ubootfile}\0"	\
								\
	"loadbootenv=run mmcloadenv || run tftploadenv\0"	\
	"loadbootscr=run mmcloadscr || run tftploadscr\0"	\
	"bootcmd_root= "					\
		"if run loadbootenv; then "			\
			"echo Loaded environment ${uenvfile}; "	\
			"run importbootenv; "			\
		"fi; "						\
		"if test -n \"${bootcmd_uenv}\" ; then "	\
			"echo Running bootcmd_uenv ...; "	\
			"run bootcmd_uenv; "			\
		"fi; "						\
		"if run loadbootscr; then "			\
			"echo Jumping to ${scriptfile}; "	\
			"source ${uenvaddr}; "			\
		"fi; "						\
		"echo Custom environment or script not found. "	\
			"Aborting auto booting...; \0"		\
	""

#define CONFIG_BOOTCOMMAND		"run bootcmd_root"

#endif	/* __PIC32MZDASK_CONFIG_H */
