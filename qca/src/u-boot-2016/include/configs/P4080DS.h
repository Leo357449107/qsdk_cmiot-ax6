/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * P4080 DS board configuration file
 * Also supports P4040 DS
 */
#define CONFIG_P4080DS
#define CONFIG_PHYS_64BIT
#define CONFIG_PPC_P4080

#define CONFIG_FSL_NGPIXIS		/* use common ngPIXIS code */

#define CONFIG_MMC
#define CONFIG_PCIE3

#define CONFIG_CMD_SATA
#define CONFIG_SATA_SIL
#define CONFIG_SYS_SATA_MAX_DEVICE  2
#define CONFIG_LIBATA
#define CONFIG_LBA48

#define CONFIG_SYS_SRIO
#define CONFIG_SRIO1			/* SRIO port 1 */
#define CONFIG_SRIO2			/* SRIO port 2 */
#define CONFIG_SRIO_PCIE_BOOT_MASTER
#define CONFIG_ICS307_REFCLK_HZ		33333000  /* ICS307 ref clk freq */

#include "corenet_ds.h"
