/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __EDMA_H__
#define __EDMA_H__

#include <linux/netdevice.h>
#include <nss_dp_arch.h>
#include <nss_dp_api_if.h>
#include <nss_dp_hal_if.h>
#include "edma_rx.h"
#include "edma_tx.h"

/*
 * The driver uses kernel DMA constructs that assume an architecture
 * where the view of physical addresses is consistent between SoC and
 * IO device(EDMA).
 * Note that this may not be compatible for platforms where this
 * assumption is not true, for example IO devices with IOMMU support.
 */
#if defined(CONFIG_ARM_SMMU) || \
	defined(CONFIG_IOMMU_SUPPORT)
#error "Build Error: Platform is enabled with IOMMU/SMMU support."
#endif

#define EDMA_HW_RESET_ID		"edma_rst"
#define EDMA_HW_CFG_RESET_ID		"edma_cfg_reset"
#define EDMA_BUF_SIZE			2000
#define EDMA_DEVICE_NODE_NAME		"edma"
#define EDMA_START_GMACS		NSS_DP_HAL_START_IFNUM
#define EDMA_MAX_GMACS			NSS_DP_HAL_MAX_PORTS
#define EDMA_IRQ_NAME_SIZE		32
#define EDMA_SC_BYPASS		1

/*
 * EDMA common clocks
 */
#define EDMA_CSR_CLK			"nss-csr-clk"
#define EDMA_NSSNOC_CSR_CLK		"nss-nssnoc-csr-clk"
#define EDMA_IMEM_QSB_CLK		"nss-imem-qsb-clk"
#define EDMA_NSSNOC_IMEM_QSB_CLK	"nss-nssnoc-imem-qsb-clk"
#define EDMA_IMEM_AHB_CLK		"nss-imem-ahb-clk"
#define EDMA_NSSNOC_IMEM_AHB_CLK	"nss-nssnoc-imem-ahb-clk"
#define EDMA_MEM_NOC_NSSNOC_CLK		"nss-mem-noc-nssnoc-clk"
#define EDMA_TBU_CLK			"nss-tbu-clk"
#define EDMA_TS_CLK			"nss-ts-clk"
#define EDMA_NSSCC_CLK			"nss-nsscc-clk"
#define EDMA_NSSCFG_CLK			"nss-nsscfg-clk"
#define EDMA_NSSCNOC_ATB_CLK		"nss-nsscnoc-atb-clk"
#define EDMA_NSSNOC_MEM_NOC_1_CLK	"nss-nssnoc-mem-noc-1-clk"
#define EDMA_NSSNOC_MEMNOC_CLK		"nss-nssnoc-memnoc-clk"
#define EDMA_NSSNOC_NSSCC_CLK		"nss-nssnoc-nsscc-clk"
#define EDMA_NSSNOC_PCNOC_1_CLK		"nss-nssnoc-pcnoc-1-clk"
#define EDMA_NSSNOC_QOSGEN_REF_CLK	"nss-nssnoc-qosgen-ref-clk"
#define EDMA_NSSNOC_SNOC_1_CLK		"nss-nssnoc-snoc-1-clk"
#define EDMA_NSSNOC_SNOC_CLK		"nss-nssnoc-snoc-clk"
#define EDMA_NSSNOC_TIMEOUT_REF_CLK	"nss-nssnoc-timeout-ref-clk"
#define EDMA_NSSNOC_XO_DCD_CLK		"nss-nssnoc-xo-dcd-clk"

/*
 * EDMA common clock's frequencies
 */
#define EDMA_CSR_CLK_FREQ			100000000
#define EDMA_NSSNOC_CSR_CLK_FREQ		100000000
#define EDMA_IMEM_QSB_CLK_FREQ			353000000
#define EDMA_NSSNOC_IMEM_QSB_CLK_FREQ		353000000
#define EDMA_IMEM_AHB_CLK_FREQ			100000000
#define EDMA_NSSNOC_IMEM_AHB_CLK_FREQ		100000000
#define EDMA_MEM_NOC_NSSNOC_CLK_FREQ		533333333
#define EDMA_TBU_CLK_FREQ			533333333
#define EDMA_TS_CLK_FREQ			24000000
#define EDMA_NSSCC_CLK_FREQ			100000000
#define EDMA_NSSCFG_CLK_FREQ			100000000
#define EDMA_NSSCNOC_ATB_CLK_FREQ		240000000
#define EDMA_NSSNOC_MEM_NOC_1_CLK_FREQ		533333333
#define EDMA_NSSNOC_MEMNOC_CLK_FREQ		533333333
#define EDMA_NSSNOC_NSSCC_CLK_FREQ		100000000
#define EDMA_NSSNOC_PCNOC_1_CLK_FREQ		100000000
#define EDMA_NSSNOC_QOSGEN_REF_CLK_FREQ		6000000
#define EDMA_NSSNOC_SNOC_1_CLK_FREQ		342857143
#define EDMA_NSSNOC_SNOC_CLK_FREQ		342857143
#define EDMA_NSSNOC_TIMEOUT_REF_CLK_FREQ	6000000
#define EDMA_NSSNOC_XO_DCD_CLK_FREQ		24000000

#define EDMA_DESC_AVAIL_COUNT(head, tail, max) (((head) - (tail)) + (max)) & ((max) - 1)

/*
 * EDMA MISC status get macros
 */
#define EDMA_MISC_AXI_RD_ERR_STATUS_GET(x)	((x) & EDMA_MISC_AXI_RD_ERR_MASK)
#define EDMA_MISC_AXI_WR_ERR_STATUS_GET(x)	(((x) & EDMA_MISC_AXI_WR_ERR_MASK) >> 1)
#define EDMA_MISC_RX_DESC_FIFO_FULL_STATUS_GET(x)	(((x) & EDMA_MISC_RX_DESC_FIFO_FULL_MASK) >> 2)
#define EDMA_MISC_RX_ERR_BUF_SIZE_STATUS_GET(x)		(((x) & EDMA_MISC_RX_ERR_BUF_SIZE_MASK) >> 3)
#define EDMA_MISC_TX_SRAM_FULL_STATUS_GET(x)		(((x) & EDMA_MISC_TX_SRAM_FULL_MASK) >> 4)
#define EDMA_MISC_TX_CMPL_BUF_FULL_STATUS_GET(x)		(((x) & EDMA_MISC_TX_CMPL_BUF_FULL_MASK) >> 5)
#define EDMA_MISC_DATA_LEN_ERR_STATUS_GET(x)		(((x) & EDMA_MISC_DATA_LEN_ERR_MASK) >> 6)
#define EDMA_MISC_TX_TIMEOUT_STATUS_GET(x)		(((x) & EDMA_MISC_TX_TIMEOUT_MASK) >> 7)

/*
 * edma_pcpu_stats
 *	EDMA per cpu stats data structure
 */
struct edma_pcpu_stats {
	struct edma_rx_stats __percpu *rx_stats;
			/* Per CPU Rx statistics */
	struct edma_tx_stats __percpu *tx_stats;
			/* Per CPU Tx statistics */
};

/*
 * EDMA private data structure
 */
struct edma_gbl_ctx {
	struct net_device *netdev_arr[EDMA_MAX_GMACS];
			/* Net device for each GMAC port */
	struct device_node *device_node;
			/* Device tree node */
	struct platform_device *pdev;
			/* Platform device */
	void __iomem *reg_base;
			/* EDMA base register mapped address */
	struct resource *reg_resource;
			/* Memory resource */
	atomic_t active_port_count;
			/* Count of active number of ports */
	bool napi_added;
			/* NAPI flag */

	struct edma_rxfill_ring *rxfill_rings;
			/* Rx Fill Rings, SW is producer */
	struct edma_rxdesc_ring *rxdesc_rings;
			/* Rx Descriptor Rings, SW is consumer */
	struct edma_txdesc_ring *txdesc_rings;
			/* Tx Descriptor Ring, SW is producer */
	struct edma_txcmpl_ring *txcmpl_rings;
			/* Tx complete Ring, SW is consumer */

	uint32_t rxfill_ring_map[EDMA_RXFILL_RING_PER_CORE_MAX][NR_CPUS];
			/* Rx Fill ring per-core mapping from device tree */
	uint32_t rxdesc_ring_map[EDMA_RXDESC_RING_PER_CORE_MAX][NR_CPUS];
			/* Rx Descriptor ring per-core mapping from device tree */
	int32_t tx_to_txcmpl_map[EDMA_MAX_TXDESC_RINGS];
			/* Tx ring to Tx complete ring mapping */
	int32_t tx_map[EDMA_TX_RING_PER_CORE_MAX][NR_CPUS];
			/* Per core Tx ring to core mapping */
	int32_t txcmpl_map[EDMA_TXCMPL_RING_PER_CORE_MAX][NR_CPUS];
			/* Tx complete ring to core mapping */

	uint32_t tx_priority_level;
			/* Tx priority level per port */
	uint32_t rx_priority_level;
			/* Rx priority level per core */
	uint32_t num_txdesc_rings;
			/* Number of TxDesc rings */
	uint32_t txdesc_ring_start;
			/* Id of first TXDESC ring */
	uint32_t txdesc_ring_end;
			/* Id of the last TXDESC ring */
	uint32_t num_txcmpl_rings;
			/* Number of TxCmpl rings */
	uint32_t txcmpl_ring_start;
			/* Id of first TXCMPL ring */
	uint32_t txcmpl_ring_end;
			/* Id of last TXCMPL ring */
	uint32_t num_rxfill_rings;
			/* Number of RxFill rings */
	uint32_t rxfill_ring_start;
			/* Id of first RxFill ring */
	uint32_t rxfill_ring_end;
			/* Id of last RxFill ring */
	uint32_t num_rxdesc_rings;
			/* Number of RxDesc rings */
	uint32_t rxdesc_ring_start;
			/* Id of first RxDesc ring */
	uint32_t rxdesc_ring_end;
			/* Id of last RxDesc ring */
	uint32_t txcmpl_intr[EDMA_MAX_TXCMPL_RINGS];
			/* TxCmpl ring IRQ numbers */
	uint32_t rxfill_intr[EDMA_MAX_RXFILL_RINGS];
			/* Rx fill ring IRQ numbers */
	uint32_t rxdesc_intr[EDMA_MAX_RXDESC_RINGS];
			/* Rx desc ring IRQ numbers */
	uint32_t misc_intr;
			/* Misc IRQ number */

	uint32_t rxfill_intr_mask;
			/* Rx fill ring interrupt mask */
	uint32_t rxdesc_intr_mask;
			/* Rx Desc ring interrupt mask */
	uint32_t txcmpl_intr_mask;
			/* Tx Cmpl ring interrupt mask */
	uint32_t misc_intr_mask;
			/* Misc interrupt interrupt mask */
	uint32_t dp_override_cnt;
			/* Number of interfaces overriden */
	bool edma_initialized;
			/* Flag to check initialization status */
};

extern struct edma_gbl_ctx edma_gbl_ctx;

int edma_irq_init(void);
irqreturn_t edma_misc_handle_irq(int irq, void *ctx);
void edma_enable_interrupts(struct edma_gbl_ctx *egc);
void edma_disable_interrupts(struct edma_gbl_ctx *egc);

/*
 * edma_reg_read()
 *	Read EDMA register
 */
static inline uint32_t edma_reg_read(uint32_t reg_off)
{
	return hal_read_reg(edma_gbl_ctx.reg_base, reg_off);
}

/*
 * edma_reg_write()
 *	Write EDMA register
 */
static inline void edma_reg_write(uint32_t reg_off, uint32_t val)
{
	hal_write_reg(edma_gbl_ctx.reg_base, reg_off, val);
}

#endif	/* __EDMA_H__ */
