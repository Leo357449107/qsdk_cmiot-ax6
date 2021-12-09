/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/debugfs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include "edma.h"
#include "edma_regs.h"
#include "edma_debug.h"

/*
 * edma_misc_handle_irq()
 *	Process miscellaneous interrupts from EDMA
 */
irqreturn_t edma_misc_handle_irq(int irq, void *ctx)
{
	uint32_t misc_intr_status, reg_data;
	struct edma_gbl_ctx *egc = &edma_gbl_ctx;

	/*
	 * Read Misc intr status
	 */
	reg_data = edma_reg_read(EDMA_REG_MISC_INT_STAT);
	misc_intr_status = reg_data & egc->misc_intr_mask;

	edma_debug("Received misc irq %d, status: %d\n", irq, misc_intr_status);

	/*
	 * TODO:
	 * Add the MISC statistics in the EDMA common statistics
	 * framework and update the MISC statistics in it.
	 */
	if (EDMA_MISC_AXI_RD_ERR_STATUS_GET(misc_intr_status)) {
		edma_err("MISC AXI read error received\n");
	}

	if (EDMA_MISC_AXI_WR_ERR_STATUS_GET(misc_intr_status)) {
		edma_err("MISC AXI write error received\n");
	}

	if (EDMA_MISC_RX_DESC_FIFO_FULL_STATUS_GET(misc_intr_status)) {
		if (net_ratelimit()) {
			edma_err("MISC Rx descriptor fifo full error received\n");
		}
	}

	if (EDMA_MISC_RX_ERR_BUF_SIZE_STATUS_GET(misc_intr_status)) {
		if (net_ratelimit()) {
			edma_err("MISC Rx buffer size error received\n");
		}
	}

	if (EDMA_MISC_TX_SRAM_FULL_STATUS_GET(misc_intr_status)) {
		if (net_ratelimit()) {
			edma_err("MISC Tx SRAM full error received\n");
		}
	}

	if (EDMA_MISC_TX_CMPL_BUF_FULL_STATUS_GET(misc_intr_status)) {
		if (net_ratelimit()) {
			edma_err("MISC Tx complete buffer full error received\n");
		}
	}

	if (EDMA_MISC_DATA_LEN_ERR_STATUS_GET(misc_intr_status)) {
		if (net_ratelimit()) {
			edma_err("MISC data length error received\n");
		}
	}

	if (EDMA_MISC_TX_TIMEOUT_STATUS_GET(misc_intr_status)) {
		if (net_ratelimit()) {
			edma_err("MISC Tx timeout error received\n");
		}
	}

	return IRQ_HANDLED;
}
