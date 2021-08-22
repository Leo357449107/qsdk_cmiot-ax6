/* Copyright (c) 2009-2015, 2017-2019 The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _PLATFORM_IPQ806X_IOMAP_H_
#define _PLATFORM_IPQ806X_IOMAP_H_

#include <sys/types.h>

#define CONFIG_SMEM_VER12
#define MSM_IOMAP_BASE              0x00000000
#define MSM_IOMAP_END               0x08000000

#define MSM_IMEM_BASE			0x08600000

#define GCNT_PSHOLD             	0x004AB000

#define MSM_SHARED_IMEM_BASE		0x08600000
#define MSM_APPS_DLOAD_MAGIC1_ADDR	(MSM_SHARED_IMEM_BASE)
#define MSM_APPS_DLOAD_MAGIC2_ADDR	(MSM_SHARED_IMEM_BASE + 0x4)
#define RESTART_REASON_ADDR		(MSM_SHARED_IMEM_BASE + 0x65C)
#define APPS_DLOAD_MAGIC		0x10

#define APPS_SS_BASE			0x0B000000
#define MSM_SHARED_BASE			0x4AA00000
#define CONFIG_QCA_SMEM_SIZE		0x100000

#define MSM_TCSR_BASE			0x194b000
#define TCSR_USB_HSPHY_CONFIG		0xC
#define TCSR_USB_HSPHY_DEVICE_MODE	0x00C700E7
#define MSM_GIC_DIST_BASE		0x0B000000
#define MSM_TMR_BASE			0x0B021000
#define MSM_GPT_BASE			(MSM_TMR_BASE + 0x04)
#define MSM_DGT_BASE			(MSM_TMR_BASE + 0x24)
#define SPSS_TIMER_STATUS		(MSM_TMR_BASE + 0x88)

#define GPT_REG(off)			(MSM_GPT_BASE + (off))
#define DGT_REG(off)			(MSM_DGT_BASE + (off))

#define GPT_MATCH_VAL			GPT_REG(0x0000)
#define GPT_COUNT_VAL			GPT_REG(0x0004)
#define GPT_ENABLE			GPT_REG(0x0008)
#define GPT_CLEAR			GPT_REG(0x000C)

#define DGT_MATCH_VAL			DGT_REG(0x0000)
#define DGT_COUNT_VAL			DGT_REG(0x0004)
#define DGT_ENABLE			DGT_REG(0x0008)
#define DGT_CLEAR			DGT_REG(0x000C)
#define DGT_CLK_CTL			DGT_REG(0x0010)

#define APPS_APCS_QTMR_AC_BASE      (APPS_SS_BASE + 0x00020000)
#define APPS_APCS_F0_QTMR_V1_BASE   (APPS_SS_BASE + 0x00021000)
#define QTMR_BASE                   APPS_APCS_F0_QTMR_V1_BASE
#define MSM_GIC_CPU_BASE		0x0B002000
#define MSM_VIC_BASE			0x0B080000
#define MSM_TCSR_SIZE			4096
#define MSM_USB_BASE			0x08A00000
#define TLMM_BASE_ADDR			0x1000000

/* USB30 base */
#define MSM_USB30_BASE			0x8A00000
#define MSM_USB30_QSCRATCH_BASE		0x8AF8800

#define TCSR_WDOG_CFG			0x30
#define MSM_WDT0_RST			(MSM_TMR_BASE + 0x38)
#define MSM_WDT0_EN			(MSM_TMR_BASE + 0x40)
#define MSM_WDT0_BT			(MSM_TMR_BASE + 0x4C)
#define MSM_PSHOLD_CTL_SU		(TLMM_BASE_ADDR + 0x820)

#define MSM_SDC1_BASE			0x07804000
#define MSM_SDC1_BAM_BASE		0x12402000
#define MSM_SDC1_DML_BASE		0x12400800
#define MSM_SDC1_SDHCI_BASE		0x07804000

#define MSM_SDC2_BASE			0x12140000
#define MSM_SDC2_BAM_BASE		0x12142000
#define MSM_SDC2_DML_BASE		0x12140800
#define MSM_SDC2_SDHCI_BASE		(0x00064900)

#define MSM_SDC3_BASE			0x12180000
#define MSM_SDC3_BAM_BASE		0x12182000
#define MSM_SDC3_DML_BASE		0x12180800

#define MSM_SDC4_BASE			0x121c0000
#define MSM_SDC4_BAM_BASE		0x121c2000
#define MSM_SDC4_DML_BASE		0x121c0800

#define GPIO_CONFIG_ADDR(x)		(TLMM_BASE_ADDR + 0x1000 * (x))
#define GPIO_IN_OUT_ADDR(x)		(TLMM_BASE_ADDR + 0x1000 * (x) + 4)
#define GSBIn_UART_I2C_SEL(x)		(TLMM_BASE_ADDR + 0x20D0 * (x) + 4)

#define UART0_DM_BASE			0x078AF000
#define UART1_DM_BASE			0x078B0000
#define UART2_DM_BASE			0x078B1000
#define UART3_DM_BASE			0x078B2000
#define UART4_DM_BASE			0x078B3000

#define BLSP_QUP_BASE(qup_id)  (0x078B5000 + (qup_id * 0x1000))

#define EBI2_CHIP_SELECT_CFG0		0x1A100000
#define EBI2_XMEM_CS3_CFG1		0x1A110034

#define MSM_CLK_CTL_BASE		0x01800000

#define CE1_CRYPTO4_BASE		(0x18500000)
#define CE3_CRYPTO4_BASE		(0x11000000)

#define NSS_GMAC0_BASE			0x37000000
#define NSS_GMAC1_BASE			0x37200000
#define NSS_GMAC2_BASE			0x37400000
#define NSS_GMAC3_BASE			0x37600000

#define GMAC_CORE_RESET(n)	((void *)(0x903CBC + ((n) * 0x20)))
#define GMACSEC_CORE_RESET(n)	((void *)(0x903E28 + ((n - 1) * 4)))
#define GMAC_AHB_RESET		0x903E24


#define ATF_FUSE	0x000A4188
#define ATF_ENABLED	BIT(9)

/*
 * GCC-SDCC Registers
 */
#define GCC_SDCC1_BCR		0x1842000
#define GCC_SDCC1_MISC		0x1842020
#define GCC_SDCC1_APPS_CBCR	0x1842018
#define GCC_SDCC1_AHB_CBCR	0x0184201C
#define GCC_SDCC1_APPS_CFG_RCGR	0x1842008
#define GCC_SDCC1_APPS_CMD_RCGR	0x1842004
#define GCC_SDCC1_APPS_CFG_RCGR_SRC_SEL	(2 << 8)
#define GCC_SDCC1_APPS_CFG_RCGR_SRC_DIV	(0xB << 0)
#define GCC_SDCC1_APPS_M	0x184200C
#define GCC_SDCC1_APPS_N	0x1842010
#define GCC_SDCC1_APPS_D	0x1842014
#define SDCC1_M_VAL		0x1
#define SDCC1_N_VAL		0xFC
#define SDCC1_D_VAL		0xFD

#define MMC_IDENTIFY_MODE		0x0
#define MMC_DATA_TRANSFER_MODE		0x1

#define GCC_USB_RST_CTRL_1		0x181E038

/* SDHCI */
#define SDCC_MCI_REMOVED
#define SDCC_HC_PWRCTL_STATUS_REG	(0x00000240)
#define SDCC_HC_PWRCTL_MASK_REG		(0x00000244)
#define SDCC_HC_PWRCTL_CLEAR_REG	(0x00000248)
#define SDCC_HC_PWRCTL_CTL_REG		(0x0000024C)

enum {
        QUP_ID_0 = 0,
        QUP_ID_1,
        QUP_ID_2,
        QUP_ID_3,
        QUP_ID_4,
        QUP_ID_5,
} qup_instance;

enum {
        BLSP_ID_1 = 1,
        BLSP_ID_2,
} blsp_instance;

void setbits_le32(uint32_t addr, uint32_t val);
void clrbits_le32(uint32_t addr, uint32_t val);
#endif
