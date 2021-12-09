/*
 * Copyright 2013-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ARCH_FSL_LSCH2_IMMAP_H__
#define __ARCH_FSL_LSCH2_IMMAP_H__

#include <fsl_immap.h>

#define CONFIG_SYS_IMMR				0x01000000
#define CONFIG_SYS_DCSRBAR			0x20000000
#define CONFIG_SYS_DCSR_DCFG_ADDR	(CONFIG_SYS_DCSRBAR + 0x00140000)
#define CONFIG_SYS_DCSR_COP_CCP_ADDR	(CONFIG_SYS_DCSRBAR + 0x02008040)

#define CONFIG_SYS_FSL_DDR_ADDR			(CONFIG_SYS_IMMR + 0x00080000)
#define CONFIG_SYS_CCI400_ADDR			(CONFIG_SYS_IMMR + 0x00180000)
#define CONFIG_SYS_GIC400_ADDR			(CONFIG_SYS_IMMR + 0x00400000)
#define CONFIG_SYS_IFC_ADDR			(CONFIG_SYS_IMMR + 0x00530000)
#define CONFIG_SYS_FSL_ESDHC_ADDR		(CONFIG_SYS_IMMR + 0x00560000)
#define CONFIG_SYS_FSL_CSU_ADDR			(CONFIG_SYS_IMMR + 0x00510000)
#define CONFIG_SYS_FSL_GUTS_ADDR		(CONFIG_SYS_IMMR + 0x00ee0000)
#define CONFIG_SYS_FSL_RST_ADDR			(CONFIG_SYS_IMMR + 0x00ee00b0)
#define CONFIG_SYS_FSL_SCFG_ADDR		(CONFIG_SYS_IMMR + 0x00570000)
#define CONFIG_SYS_FSL_FMAN_ADDR		(CONFIG_SYS_IMMR + 0x00a00000)
#define CONFIG_SYS_FSL_SERDES_ADDR		(CONFIG_SYS_IMMR + 0x00ea0000)
#define CONFIG_SYS_FSL_DCFG_ADDR		(CONFIG_SYS_IMMR + 0x00ee0000)
#define CONFIG_SYS_FSL_CLK_ADDR			(CONFIG_SYS_IMMR + 0x00ee1000)
#define CONFIG_SYS_NS16550_COM1			(CONFIG_SYS_IMMR + 0x011c0500)
#define CONFIG_SYS_NS16550_COM2			(CONFIG_SYS_IMMR + 0x011c0600)
#define CONFIG_SYS_NS16550_COM3			(CONFIG_SYS_IMMR + 0x011d0500)
#define CONFIG_SYS_NS16550_COM4			(CONFIG_SYS_IMMR + 0x011d0600)
#define CONFIG_SYS_LS1043A_XHCI_USB1_ADDR	(CONFIG_SYS_IMMR + 0x01f00000)
#define CONFIG_SYS_LS1043A_XHCI_USB2_ADDR	(CONFIG_SYS_IMMR + 0x02000000)
#define CONFIG_SYS_LS1043A_XHCI_USB3_ADDR	(CONFIG_SYS_IMMR + 0x02100000)
#define CONFIG_SYS_PCIE1_ADDR			(CONFIG_SYS_IMMR + 0x2400000)
#define CONFIG_SYS_PCIE2_ADDR			(CONFIG_SYS_IMMR + 0x2500000)
#define CONFIG_SYS_PCIE3_ADDR			(CONFIG_SYS_IMMR + 0x2600000)
#define CONFIG_SYS_FSL_SEC_ADDR			(CONFIG_SYS_IMMR + 0x700000)
#define CONFIG_SYS_FSL_JR0_ADDR			(CONFIG_SYS_IMMR + 0x710000)
#define CONFIG_SYS_SEC_MON_ADDR			(CONFIG_SYS_IMMR + 0xe90000)
#define CONFIG_SYS_SFP_ADDR			(CONFIG_SYS_IMMR + 0xe80200)

#define CONFIG_SYS_FSL_TIMER_ADDR		0x02b00000

#define I2C1_BASE_ADDR				(CONFIG_SYS_IMMR + 0x01180000)
#define I2C2_BASE_ADDR				(CONFIG_SYS_IMMR + 0x01190000)
#define I2C3_BASE_ADDR				(CONFIG_SYS_IMMR + 0x011a0000)
#define I2C4_BASE_ADDR				(CONFIG_SYS_IMMR + 0x011b0000)

#define WDOG1_BASE_ADDR				(CONFIG_SYS_IMMR + 0x01ad0000)

#define QSPI0_BASE_ADDR				(CONFIG_SYS_IMMR + 0x00550000)
#define DSPI1_BASE_ADDR				(CONFIG_SYS_IMMR + 0x01100000)

#define LPUART_BASE				(CONFIG_SYS_IMMR + 0x01950000)

#define AHCI_BASE_ADDR				(CONFIG_SYS_IMMR + 0x02200000)

#define CONFIG_SYS_PCIE1_PHYS_ADDR		0x4000000000ULL
#define CONFIG_SYS_PCIE2_PHYS_ADDR		0x4800000000ULL
#define CONFIG_SYS_PCIE3_PHYS_ADDR		0x5000000000ULL
/* LUT registers */
#define PCIE_LUT_BASE				0x10000
#define PCIE_LUT_LCTRL0				0x7F8
#define PCIE_LUT_DBG				0x7FC

/* TZ Address Space Controller Definitions */
#define TZASC1_BASE			0x01100000	/* as per CCSR map. */
#define TZASC2_BASE			0x01110000	/* as per CCSR map. */
#define TZASC3_BASE			0x01120000	/* as per CCSR map. */
#define TZASC4_BASE			0x01130000	/* as per CCSR map. */
#define TZASC_BUILD_CONFIG_REG(x)	((TZASC1_BASE + (x * 0x10000)))
#define TZASC_ACTION_REG(x)		((TZASC1_BASE + (x * 0x10000)) + 0x004)
#define TZASC_GATE_KEEPER(x)		((TZASC1_BASE + (x * 0x10000)) + 0x008)
#define TZASC_REGION_BASE_LOW_0(x)	((TZASC1_BASE + (x * 0x10000)) + 0x100)
#define TZASC_REGION_BASE_HIGH_0(x)	((TZASC1_BASE + (x * 0x10000)) + 0x104)
#define TZASC_REGION_TOP_LOW_0(x)	((TZASC1_BASE + (x * 0x10000)) + 0x108)
#define TZASC_REGION_TOP_HIGH_0(x)	((TZASC1_BASE + (x * 0x10000)) + 0x10C)
#define TZASC_REGION_ATTRIBUTES_0(x)	((TZASC1_BASE + (x * 0x10000)) + 0x110)
#define TZASC_REGION_ID_ACCESS_0(x)	((TZASC1_BASE + (x * 0x10000)) + 0x114)

#define TP_ITYP_AV              0x00000001      /* Initiator available */
#define TP_ITYP_TYPE(x) (((x) & 0x6) >> 1)      /* Initiator Type */
#define TP_ITYP_TYPE_ARM        0x0
#define TP_ITYP_TYPE_PPC        0x1             /* PowerPC */
#define TP_ITYP_TYPE_OTHER      0x2             /* StarCore DSP */
#define TP_ITYP_TYPE_HA         0x3             /* HW Accelerator */
#define TP_ITYP_THDS(x) (((x) & 0x18) >> 3)     /* # threads */
#define TP_ITYP_VER(x)  (((x) & 0xe0) >> 5)     /* Initiator Version */
#define TY_ITYP_VER_A7          0x1
#define TY_ITYP_VER_A53         0x2
#define TY_ITYP_VER_A57         0x3

#define TP_CLUSTER_EOC		0xc0000000      /* end of clusters */
#define TP_CLUSTER_INIT_MASK    0x0000003f      /* initiator mask */
#define TP_INIT_PER_CLUSTER     4

/*
 * Define default values for some CCSR macros to make header files cleaner*
 *
 * To completely disable CCSR relocation in a board header file, define
 * CONFIG_SYS_CCSR_DO_NOT_RELOCATE.  This will force CONFIG_SYS_CCSRBAR_PHYS
 * to a value that is the same as CONFIG_SYS_CCSRBAR.
 */

#ifdef CONFIG_SYS_CCSRBAR_PHYS
#error "Do not define CONFIG_SYS_CCSRBAR_PHYS directly.  Use \
CONFIG_SYS_CCSRBAR_PHYS_LOW and/or CONFIG_SYS_CCSRBAR_PHYS_HIGH instead."
#endif

#ifdef CONFIG_SYS_CCSR_DO_NOT_RELOCATE
#undef CONFIG_SYS_CCSRBAR_PHYS_HIGH
#undef CONFIG_SYS_CCSRBAR_PHYS_LOW
#define CONFIG_SYS_CCSRBAR_PHYS_HIGH	0
#endif

#ifndef CONFIG_SYS_CCSRBAR
#define CONFIG_SYS_CCSRBAR		CONFIG_SYS_CCSRBAR_DEFAULT
#endif

#ifndef CONFIG_SYS_CCSRBAR_PHYS_HIGH
#define CONFIG_SYS_CCSRBAR_PHYS_HIGH	0
#endif

#ifndef CONFIG_SYS_CCSRBAR_PHYS_LOW
#define CONFIG_SYS_CCSRBAR_PHYS_LOW	CONFIG_SYS_CCSRBAR_DEFAULT
#endif

#define CONFIG_SYS_CCSRBAR_PHYS ((CONFIG_SYS_CCSRBAR_PHYS_HIGH * 1ull) << 32 | \
				 CONFIG_SYS_CCSRBAR_PHYS_LOW)

struct sys_info {
	unsigned long freq_processor[CONFIG_MAX_CPUS];
	unsigned long freq_systembus;
	unsigned long freq_ddrbus;
	unsigned long freq_localbus;
	unsigned long freq_sdhc;
#ifdef CONFIG_SYS_DPAA_FMAN
	unsigned long freq_fman[CONFIG_SYS_NUM_FMAN];
#endif
	unsigned long freq_qman;
};

#define CONFIG_SYS_FSL_FM1_OFFSET		0xa00000
#define CONFIG_SYS_FSL_FM1_RX0_1G_OFFSET	0xa88000
#define CONFIG_SYS_FSL_FM1_RX1_1G_OFFSET	0xa89000
#define CONFIG_SYS_FSL_FM1_RX2_1G_OFFSET	0xa8a000
#define CONFIG_SYS_FSL_FM1_RX3_1G_OFFSET	0xa8b000
#define CONFIG_SYS_FSL_FM1_RX4_1G_OFFSET	0xa8c000
#define CONFIG_SYS_FSL_FM1_RX5_1G_OFFSET	0xa8d000

#define CONFIG_SYS_FSL_FM1_DTSEC1_OFFSET	0xae0000
#define CONFIG_SYS_FSL_FM1_ADDR			\
		(CONFIG_SYS_IMMR + CONFIG_SYS_FSL_FM1_OFFSET)
#define CONFIG_SYS_FSL_FM1_DTSEC1_ADDR		\
		(CONFIG_SYS_IMMR + CONFIG_SYS_FSL_FM1_DTSEC1_OFFSET)

/* Device Configuration and Pin Control */
struct ccsr_gur {
	u32     porsr1;         /* POR status 1 */
#define FSL_CHASSIS2_CCSR_PORSR1_RCW_MASK	0xFF800000
	u32     porsr2;         /* POR status 2 */
	u8      res_008[0x20-0x8];
	u32     gpporcr1;       /* General-purpose POR configuration */
	u32	gpporcr2;
#define FSL_CHASSIS2_DCFG_FUSESR_VID_SHIFT	25
#define FSL_CHASSIS2_DCFG_FUSESR_VID_MASK	0x1F
#define FSL_CHASSIS2_DCFG_FUSESR_ALTVID_SHIFT	20
#define FSL_CHASSIS2_DCFG_FUSESR_ALTVID_MASK	0x1F
	u32     dcfg_fusesr;    /* Fuse status register */
	u8      res_02c[0x70-0x2c];
	u32     devdisr;        /* Device disable control */
#define FSL_CHASSIS2_DEVDISR2_DTSEC1_1	0x80000000
#define FSL_CHASSIS2_DEVDISR2_DTSEC1_2	0x40000000
#define FSL_CHASSIS2_DEVDISR2_DTSEC1_3	0x20000000
#define FSL_CHASSIS2_DEVDISR2_DTSEC1_4	0x10000000
#define FSL_CHASSIS2_DEVDISR2_DTSEC1_5	0x08000000
#define FSL_CHASSIS2_DEVDISR2_DTSEC1_6	0x04000000
#define FSL_CHASSIS2_DEVDISR2_DTSEC1_9	0x00800000
#define FSL_CHASSIS2_DEVDISR2_DTSEC1_10	0x00400000
#define FSL_CHASSIS2_DEVDISR2_10GEC1_1	0x00800000
#define FSL_CHASSIS2_DEVDISR2_10GEC1_2	0x00400000
#define FSL_CHASSIS2_DEVDISR2_10GEC1_3	0x80000000
#define FSL_CHASSIS2_DEVDISR2_10GEC1_4	0x40000000
	u32     devdisr2;       /* Device disable control 2 */
	u32     devdisr3;       /* Device disable control 3 */
	u32     devdisr4;       /* Device disable control 4 */
	u32     devdisr5;       /* Device disable control 5 */
	u32     devdisr6;       /* Device disable control 6 */
	u32     devdisr7;       /* Device disable control 7 */
	u8      res_08c[0x94-0x8c];
	u32     coredisru;      /* uppper portion for support of 64 cores */
	u32     coredisrl;      /* lower portion for support of 64 cores */
	u8      res_09c[0xa0-0x9c];
	u32     pvr;            /* Processor version */
	u32     svr;            /* System version */
	u32     mvr;            /* Manufacturing version */
	u8	res_0ac[0xb0-0xac];
	u32	rstcr;		/* Reset control */
	u32	rstrqpblsr;	/* Reset request preboot loader status */
	u8	res_0b8[0xc0-0xb8];
	u32	rstrqmr1;	/* Reset request mask */
	u8	res_0c4[0xc8-0xc4];
	u32	rstrqsr1;	/* Reset request status */
	u8	res_0cc[0xd4-0xcc];
	u32	rstrqwdtmrl;	/* Reset request WDT mask */
	u8	res_0d8[0xdc-0xd8];
	u32	rstrqwdtsrl;	/* Reset request WDT status */
	u8	res_0e0[0xe4-0xe0];
	u32	brrl;		/* Boot release */
	u8      res_0e8[0x100-0xe8];
	u32     rcwsr[16];      /* Reset control word status */
#define FSL_CHASSIS2_RCWSR0_SYS_PLL_RAT_SHIFT	25
#define FSL_CHASSIS2_RCWSR0_SYS_PLL_RAT_MASK	0x1f
#define FSL_CHASSIS2_RCWSR0_MEM_PLL_RAT_SHIFT	16
#define FSL_CHASSIS2_RCWSR0_MEM_PLL_RAT_MASK	0x3f
#define FSL_CHASSIS2_RCWSR4_SRDS1_PRTCL_MASK	0xffff0000
#define FSL_CHASSIS2_RCWSR4_SRDS1_PRTCL_SHIFT	16
	u8      res_140[0x200-0x140];
	u32     scratchrw[4];  /* Scratch Read/Write */
	u8      res_210[0x300-0x210];
	u32     scratchw1r[4];  /* Scratch Read (Write once) */
	u8      res_310[0x400-0x310];
	u32	crstsr[12];
	u8	res_430[0x500-0x430];

	/* PCI Express n Logical I/O Device Number register */
	u32 dcfg_ccsr_pex1liodnr;
	u32 dcfg_ccsr_pex2liodnr;
	u32 dcfg_ccsr_pex3liodnr;
	u32 dcfg_ccsr_pex4liodnr;
	/* RIO n Logical I/O Device Number register */
	u32 dcfg_ccsr_rio1liodnr;
	u32 dcfg_ccsr_rio2liodnr;
	u32 dcfg_ccsr_rio3liodnr;
	u32 dcfg_ccsr_rio4liodnr;
	/* USB Logical I/O Device Number register */
	u32 dcfg_ccsr_usb1liodnr;
	u32 dcfg_ccsr_usb2liodnr;
	u32 dcfg_ccsr_usb3liodnr;
	u32 dcfg_ccsr_usb4liodnr;
	/* SD/MMC Logical I/O Device Number register */
	u32 dcfg_ccsr_sdmmc1liodnr;
	u32 dcfg_ccsr_sdmmc2liodnr;
	u32 dcfg_ccsr_sdmmc3liodnr;
	u32 dcfg_ccsr_sdmmc4liodnr;
	/* RIO Message Unit Logical I/O Device Number register */
	u32 dcfg_ccsr_riomaintliodnr;

	u8      res_544[0x550-0x544];
	u32	sataliodnr[4];
	u8	res_560[0x570-0x560];

	u32 dcfg_ccsr_misc1liodnr;
	u32 dcfg_ccsr_misc2liodnr;
	u32 dcfg_ccsr_misc3liodnr;
	u32 dcfg_ccsr_misc4liodnr;
	u32 dcfg_ccsr_dma1liodnr;
	u32 dcfg_ccsr_dma2liodnr;
	u32 dcfg_ccsr_dma3liodnr;
	u32 dcfg_ccsr_dma4liodnr;
	u32 dcfg_ccsr_spare1liodnr;
	u32 dcfg_ccsr_spare2liodnr;
	u32 dcfg_ccsr_spare3liodnr;
	u32 dcfg_ccsr_spare4liodnr;
	u8	res_5a0[0x600-0x5a0];
	u32 dcfg_ccsr_pblsr;

	u32	pamubypenr;
	u32	dmacr1;

	u8	res_60c[0x610-0x60c];
	u32 dcfg_ccsr_gensr1;
	u32 dcfg_ccsr_gensr2;
	u32 dcfg_ccsr_gensr3;
	u32 dcfg_ccsr_gensr4;
	u32 dcfg_ccsr_gencr1;
	u32 dcfg_ccsr_gencr2;
	u32 dcfg_ccsr_gencr3;
	u32 dcfg_ccsr_gencr4;
	u32 dcfg_ccsr_gencr5;
	u32 dcfg_ccsr_gencr6;
	u32 dcfg_ccsr_gencr7;
	u8	res_63c[0x658-0x63c];
	u32 dcfg_ccsr_cgensr1;
	u32 dcfg_ccsr_cgensr0;
	u8	res_660[0x678-0x660];
	u32 dcfg_ccsr_cgencr1;

	u32 dcfg_ccsr_cgencr0;
	u8	res_680[0x700-0x680];
	u32 dcfg_ccsr_sriopstecr;
	u32 dcfg_ccsr_dcsrcr;

	u8      res_708[0x740-0x708];   /* add more registers when needed */
	u32     tp_ityp[64];    /* Topology Initiator Type Register */
	struct {
		u32     upper;
		u32     lower;
	} tp_cluster[16];
	u8      res_8c0[0xa00-0x8c0];   /* add more registers when needed */
	u32 dcfg_ccsr_qmbm_warmrst;
	u8      res_a04[0xa20-0xa04];   /* add more registers when needed */
	u32 dcfg_ccsr_reserved0;
	u32 dcfg_ccsr_reserved1;
};

#define SCFG_QSPI_CLKSEL		0x40100000
#define SCFG_USBDRVVBUS_SELCR_USB1	0x00000000
#define SCFG_USBDRVVBUS_SELCR_USB2	0x00000001
#define SCFG_USBDRVVBUS_SELCR_USB3	0x00000002
#define SCFG_USBPWRFAULT_INACTIVE	0x00000000
#define SCFG_USBPWRFAULT_SHARED		0x00000001
#define SCFG_USBPWRFAULT_DEDICATED	0x00000002
#define SCFG_USBPWRFAULT_USB3_SHIFT	4
#define SCFG_USBPWRFAULT_USB2_SHIFT	2
#define SCFG_USBPWRFAULT_USB1_SHIFT	0

#define SCFG_SNPCNFGCR_SECRDSNP		0x80000000
#define SCFG_SNPCNFGCR_SECWRSNP		0x40000000

/* Supplemental Configuration Unit */
struct ccsr_scfg {
	u8 res_000[0x100-0x000];
	u32 usb2_icid;
	u32 usb3_icid;
	u8 res_108[0x114-0x108];
	u32 dma_icid;
	u32 sata_icid;
	u32 usb1_icid;
	u32 qe_icid;
	u32 sdhc_icid;
	u32 edma_icid;
	u32 etr_icid;
	u32 core_sft_rst[4];
	u8 res_140[0x158-0x140];
	u32 altcbar;
	u32 qspi_cfg;
	u8 res_160[0x180-0x160];
	u32 dmamcr;
	u8 res_184[0x18c-0x184];
	u32 debug_icid;
	u8 res_190[0x1a4-0x190];
	u32 snpcnfgcr;
	u8 res_1a8[0x1ac-0x1a8];
	u32 intpcr;
	u8 res_1b0[0x204-0x1b0];
	u32 coresrencr;
	u8 res_208[0x220-0x208];
	u32 rvbar0_0;
	u32 rvbar0_1;
	u32 rvbar1_0;
	u32 rvbar1_1;
	u32 rvbar2_0;
	u32 rvbar2_1;
	u32 rvbar3_0;
	u32 rvbar3_1;
	u32 lpmcsr;
	u8 res_244[0x400-0x244];
	u32 qspidqscr;
	u32 ecgtxcmcr;
	u32 sdhciovselcr;
	u32 rcwpmuxcr0;
	u32 usbdrvvbus_selcr;
	u32 usbpwrfault_selcr;
	u32 usb_refclk_selcr1;
	u32 usb_refclk_selcr2;
	u32 usb_refclk_selcr3;
	u8 res_424[0x600-0x424];
	u32 scratchrw[4];
	u8 res_610[0x680-0x610];
	u32 corebcr;
	u8 res_684[0x1000-0x684];
	u32 pex1msiir;
	u32 pex1msir;
	u8 res_1008[0x2000-0x1008];
	u32 pex2;
	u32 pex2msir;
	u8 res_2008[0x3000-0x2008];
	u32 pex3msiir;
	u32 pex3msir;
};

/* Clocking */
struct ccsr_clk {
	struct {
		u32 clkcncsr;	/* core cluster n clock control status */
		u8  res_004[0x0c];
		u32 clkcghwacsr; /* Clock generator n hardware accelerator */
		u8  res_014[0x0c];
	} clkcsr[4];
	u8	res_040[0x780]; /* 0x100 */
	struct {
		u32 pllcngsr;
		u8 res_804[0x1c];
	} pllcgsr[2];
	u8	res_840[0x1c0];
	u32	clkpcsr;	/* 0xa00 Platform clock domain control/status */
	u8	res_a04[0x1fc];
	u32	pllpgsr;	/* 0xc00 Platform PLL General Status */
	u8	res_c04[0x1c];
	u32	plldgsr;	/* 0xc20 DDR PLL General Status */
	u8	res_c24[0x3dc];
};

/* System Counter */
struct sctr_regs {
	u32 cntcr;
	u32 cntsr;
	u32 cntcv1;
	u32 cntcv2;
	u32 resv1[4];
	u32 cntfid0;
	u32 cntfid1;
	u32 resv2[1002];
	u32 counterid[12];
};

#define SRDS_MAX_LANES		4
struct ccsr_serdes {
	struct {
		u32	rstctl;	/* Reset Control Register */
#define SRDS_RSTCTL_RST		0x80000000
#define SRDS_RSTCTL_RSTDONE	0x40000000
#define SRDS_RSTCTL_RSTERR	0x20000000
#define SRDS_RSTCTL_SWRST	0x10000000
#define SRDS_RSTCTL_SDEN	0x00000020
#define SRDS_RSTCTL_SDRST_B	0x00000040
#define SRDS_RSTCTL_PLLRST_B	0x00000080
		u32	pllcr0; /* PLL Control Register 0 */
#define SRDS_PLLCR0_POFF		0x80000000
#define SRDS_PLLCR0_RFCK_SEL_MASK	0x70000000
#define SRDS_PLLCR0_RFCK_SEL_100	0x00000000
#define SRDS_PLLCR0_RFCK_SEL_125	0x10000000
#define SRDS_PLLCR0_RFCK_SEL_156_25	0x20000000
#define SRDS_PLLCR0_RFCK_SEL_150	0x30000000
#define SRDS_PLLCR0_RFCK_SEL_161_13	0x40000000
#define SRDS_PLLCR0_RFCK_SEL_122_88	0x50000000
#define SRDS_PLLCR0_PLL_LCK		0x00800000
#define SRDS_PLLCR0_FRATE_SEL_MASK	0x000f0000
#define SRDS_PLLCR0_FRATE_SEL_5		0x00000000
#define SRDS_PLLCR0_FRATE_SEL_3_75	0x00050000
#define SRDS_PLLCR0_FRATE_SEL_5_15	0x00060000
#define SRDS_PLLCR0_FRATE_SEL_4		0x00070000
#define SRDS_PLLCR0_FRATE_SEL_3_12	0x00090000
#define SRDS_PLLCR0_FRATE_SEL_3		0x000a0000
		u32	pllcr1; /* PLL Control Register 1 */
#define SRDS_PLLCR1_PLL_BWSEL	0x08000000
		u32	res_0c;	/* 0x00c */
		u32	pllcr3;
		u32	pllcr4;
		u8	res_18[0x20-0x18];
	} bank[2];
	u8	res_40[0x90-0x40];
	u32	srdstcalcr;	/* 0x90 TX Calibration Control */
	u8	res_94[0xa0-0x94];
	u32	srdsrcalcr;	/* 0xa0 RX Calibration Control */
	u8	res_a4[0xb0-0xa4];
	u32	srdsgr0;	/* 0xb0 General Register 0 */
	u8	res_b4[0xe0-0xb4];
	u32	srdspccr0;	/* 0xe0 Protocol Converter Config 0 */
	u32	srdspccr1;	/* 0xe4 Protocol Converter Config 1 */
	u32	srdspccr2;	/* 0xe8 Protocol Converter Config 2 */
	u32	srdspccr3;	/* 0xec Protocol Converter Config 3 */
	u32	srdspccr4;	/* 0xf0 Protocol Converter Config 4 */
	u8	res_f4[0x100-0xf4];
	struct {
		u32	lnpssr;	/* 0x100, 0x120, ..., 0x1e0 */
		u8	res_104[0x120-0x104];
	} srdslnpssr[4];
	u8	res_180[0x300-0x180];
	u32	srdspexeqcr;
	u32	srdspexeqpcr[11];
	u8	res_330[0x400-0x330];
	u32	srdspexapcr;
	u8	res_404[0x440-0x404];
	u32	srdspexbpcr;
	u8	res_444[0x800-0x444];
	struct {
		u32	gcr0;	/* 0x800 General Control Register 0 */
		u32	gcr1;	/* 0x804 General Control Register 1 */
		u32	gcr2;	/* 0x808 General Control Register 2 */
		u32	sscr0;
		u32	recr0;	/* 0x810 Receive Equalization Control */
		u32	recr1;
		u32	tecr0;	/* 0x818 Transmit Equalization Control */
		u32	sscr1;
		u32	ttlcr0;	/* 0x820 Transition Tracking Loop Ctrl 0 */
		u8	res_824[0x83c-0x824];
		u32	tcsr3;
	} lane[4];	/* Lane A, B, C, D, E, F, G, H */
	u8	res_a00[0x1000-0xa00];	/* from 0xa00 to 0xfff */
};

#define CCI400_CTRLORD_TERM_BARRIER	0x00000008
#define CCI400_CTRLORD_EN_BARRIER	0
#define CCI400_SHAORD_NON_SHAREABLE	0x00000002
#define CCI400_DVM_MESSAGE_REQ_EN	0x00000002
#define CCI400_SNOOP_REQ_EN		0x00000001

/* CCI-400 registers */
struct ccsr_cci400 {
	u32 ctrl_ord;			/* Control Override */
	u32 spec_ctrl;			/* Speculation Control */
	u32 secure_access;		/* Secure Access */
	u32 status;			/* Status */
	u32 impr_err;			/* Imprecise Error */
	u8 res_14[0x100 - 0x14];
	u32 pmcr;			/* Performance Monitor Control */
	u8 res_104[0xfd0 - 0x104];
	u32 pid[8];			/* Peripheral ID */
	u32 cid[4];			/* Component ID */
	struct {
		u32 snoop_ctrl;		/* Snoop Control */
		u32 sha_ord;		/* Shareable Override */
		u8 res_1008[0x1100 - 0x1008];
		u32 rc_qos_ord;		/* read channel QoS Value Override */
		u32 wc_qos_ord;		/* read channel QoS Value Override */
		u8 res_1108[0x110c - 0x1108];
		u32 qos_ctrl;		/* QoS Control */
		u32 max_ot;		/* Max OT */
		u8 res_1114[0x1130 - 0x1114];
		u32 target_lat;		/* Target Latency */
		u32 latency_regu;	/* Latency Regulation */
		u32 qos_range;		/* QoS Range */
		u8 res_113c[0x2000 - 0x113c];
	} slave[5];			/* Slave Interface */
	u8 res_6000[0x9004 - 0x6000];
	u32 cycle_counter;		/* Cycle counter */
	u32 count_ctrl;			/* Count Control */
	u32 overflow_status;		/* Overflow Flag Status */
	u8 res_9010[0xa000 - 0x9010];
	struct {
		u32 event_select;	/* Event Select */
		u32 event_count;	/* Event Count */
		u32 counter_ctrl;	/* Counter Control */
		u32 overflow_status;	/* Overflow Flag Status */
		u8 res_a010[0xb000 - 0xa010];
	} pcounter[4];			/* Performance Counter */
	u8 res_e004[0x10000 - 0xe004];
};

/* MMU 500 */
#define SMMU_SCR0			(SMMU_BASE + 0x0)
#define SMMU_SCR1			(SMMU_BASE + 0x4)
#define SMMU_SCR2			(SMMU_BASE + 0x8)
#define SMMU_SACR			(SMMU_BASE + 0x10)
#define SMMU_IDR0			(SMMU_BASE + 0x20)
#define SMMU_IDR1			(SMMU_BASE + 0x24)

#define SMMU_NSCR0			(SMMU_BASE + 0x400)
#define SMMU_NSCR2			(SMMU_BASE + 0x408)
#define SMMU_NSACR			(SMMU_BASE + 0x410)

#define SCR0_CLIENTPD_MASK		0x00000001
#define SCR0_USFCFG_MASK		0x00000400

#endif	/* __ARCH_FSL_LSCH2_IMMAP_H__*/
