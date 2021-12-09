/*
 * Copyright (C) 2013-2014 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include "axs10x.h"

DECLARE_GLOBAL_DATA_PTR;

int board_mmc_init(bd_t *bis)
{
	struct dwmci_host *host = NULL;

	host = malloc(sizeof(struct dwmci_host));
	if (!host) {
		printf("dwmci_host malloc fail!\n");
		return 1;
	}

	memset(host, 0, sizeof(struct dwmci_host));
	host->name = "Synopsys Mobile storage";
	host->ioaddr = (void *)ARC_DWMMC_BASE;
	host->buswidth = 4;
	host->dev_index = 0;
	host->bus_hz = 50000000;

	add_dwmci(host, host->bus_hz / 2, 400000);

	return 0;
}

#define AXS_MB_CREG	0xE0011000

int board_early_init_f(void)
{
	if (readl((void __iomem *)AXS_MB_CREG + 0x234) & (1 << 28))
		gd->board_type = AXS_MB_V3;
	else
		gd->board_type = AXS_MB_V2;

	return 0;
}

#ifdef CONFIG_ISA_ARCV2
#define RESET_VECTOR_ADDR	0x0

void smp_set_core_boot_addr(unsigned long addr, int corenr)
{
	/* All cores have reset vector pointing to 0 */
	writel(addr, (void __iomem *)RESET_VECTOR_ADDR);

	/* Make sure other cores see written value in memory */
	flush_dcache_range(RESET_VECTOR_ADDR, RESET_VECTOR_ADDR + sizeof(int));
}

void smp_kick_all_cpus(void)
{
/* CPU start CREG */
#define AXC003_CREG_CPU_START	0xF0001400

/* Bits positions in CPU start CREG */
#define BITS_START	0
#define BITS_POLARITY	8
#define BITS_CORE_SEL	9
#define BITS_MULTICORE	12

#define CMD	(1 << BITS_MULTICORE) | (1 << BITS_CORE_SEL) | \
		(1 << BITS_POLARITY) | (1 << BITS_START)

	writel(CMD, (void __iomem *)AXC003_CREG_CPU_START);
}
#endif
