/* Copyright (c) 2009-2015, The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PLATFORM_MSM8960_IOMAP_H_
#define _PLATFORM_MSM8960_IOMAP_H_

#define MSM_IOMAP_BASE      0x00100000
#define MSM_IOMAP_END       0x28000000

#define MSM_IMEM_BASE       0x2A000000

#define MSM_SHARED_IMEM_BASE 0x2A03F000
#define RESTART_REASON_ADDR  (MSM_SHARED_IMEM_BASE + 0x65C)

#define MSM_SHARED_BASE     0x80000000

#define MSM_TCSR_BASE       0x1A400000
#define MSM_GIC_DIST_BASE   0x02000000
#define MSM_TMR_BASE        0x0200A000
#define MSM_GPT_BASE        (MSM_TMR_BASE + 0x04)
#define MSM_DGT_BASE        (MSM_TMR_BASE + 0x24)
#define SPSS_TIMER_STATUS   (MSM_TMR_BASE + 0x88)

#define GPT_REG(off)        (MSM_GPT_BASE + (off))
#define DGT_REG(off)        (MSM_DGT_BASE + (off))

#define GPT_MATCH_VAL        GPT_REG(0x0000)
#define GPT_COUNT_VAL        GPT_REG(0x0004)
#define GPT_ENABLE           GPT_REG(0x0008)
#define GPT_CLEAR            GPT_REG(0x000C)

#define DGT_MATCH_VAL        DGT_REG(0x0000)
#define DGT_COUNT_VAL        DGT_REG(0x0004)
#define DGT_ENABLE           DGT_REG(0x0008)
#define DGT_CLEAR            DGT_REG(0x000C)
#define DGT_CLK_CTL          DGT_REG(0x0010)

#define MSM_GIC_CPU_BASE    0x02002000
#define MSM_VIC_BASE        0x02080000
#define MSM_TCSR_SIZE       4096
#define MSM_USB_BASE        0x12500000
#define TLMM_BASE_ADDR      0x00800000

#define TCSR_WDOG_CFG       0x30
#define MSM_WDT0_RST        (MSM_TMR_BASE + 0x38)
#define MSM_WDT0_EN         (MSM_TMR_BASE + 0x40)
#define MSM_WDT0_BT         (MSM_TMR_BASE + 0x4C)
#define MSM_PSHOLD_CTL_SU   (TLMM_BASE_ADDR + 0x820)

#define MSM_SDC1_BASE       0x12400000
#define MSM_SDC2_BASE       0x12140000
#define MSM_SDC3_BASE       0x12180000
#define MSM_SDC4_BASE       0x121C0000

#define GPIO_CONFIG_ADDR(x) (TLMM_BASE_ADDR + 0x1000 + (x)*0x10)
#define GPIO_IN_OUT_ADDR(x) (TLMM_BASE_ADDR + 0x1004 + (x)*0x10)
#define GSBIn_UART_I2C_SEL(x)   (TLMM_BASE_ADDR + 0x20D0 + (x)*0x4)


#define EBI2_CHIP_SELECT_CFG0   0x1A100000
#define EBI2_XMEM_CS3_CFG1      0x1A110034

#define MSM_CLK_CTL_BASE        0x00900000
#define MSM_MMSS_CLK_CTL_BASE 	0x04000000

#define MIPI_DSI_BASE                         (0x04700000)
#define REG_DSI(off)                          (MIPI_DSI_BASE + (off))

#define DSIPHY_REGULATOR_BASE                 (0x500)
#define DSIPHY_TIMING_BASE                    (0x440)
#define DSIPHY_CTRL_BASE                      (0x470)
#define DSIPHY_PLL_BASE                       (0x200)
#define DSIPHY_STRENGTH_BASE                  (0x480)

/* Range 0 - 4 */
#define DSIPHY_REGULATOR_CTRL(x) REG_DSI(DSIPHY_REGULATOR_BASE + (x) * 4)
/* Range 0 - 11 */
#define DSIPHY_TIMING_CTRL(x)    REG_DSI(DSIPHY_TIMING_BASE + (x) * 4)
/* Range 0 - 3 */
#define DSIPHY_CTRL(x)           REG_DSI(DSIPHY_CTRL_BASE + (x) * 4)
/* Range 0 - 2 */
#define DSIPHY_STRENGTH_CTRL(x)  REG_DSI(DSIPHY_STRENGTH_BASE + (x) * 4)
/* Range 0 - 19 */
#define DSIPHY_PLL_CTRL(x)       REG_DSI(DSIPHY_PLL_BASE + (x) * 4)

#define MDP_BASE                              (0x05100000)
#define REG_MDP(off)                          (MDP_BASE + (off))

//TODO: Where does this go?
#define MMSS_SFPB_GPREG                       (0x05700058)

/* HDMI base addresses */
#define MSM_HDMI_BASE           0x04A00000
#define DTV_BASE                0xD0000

#define HDMI_USEC_REFTIMER      (MSM_HDMI_BASE + 0x0208)
#define HDMI_CTRL               (MSM_HDMI_BASE + 0x0000)
#define HDMI_HPD_INT_STATUS     (MSM_HDMI_BASE + 0x0250)
#define HDMI_HPD_INT_CTRL       (MSM_HDMI_BASE + 0x0254)
#define HDMI_HPD_CTRL           (MSM_HDMI_BASE + 0x0258)
#define HDMI_PHY_CTRL           (MSM_HDMI_BASE + 0x000002D4)
#define HDMI_PHY_REG_0          (MSM_HDMI_BASE + 0x00000400)
#define HDMI_PHY_REG_1          (MSM_HDMI_BASE + 0x00000404)
#define HDMI_PHY_REG_2          (MSM_HDMI_BASE + 0x00000408)
#define HDMI_PHY_REG_3          (MSM_HDMI_BASE + 0x0000040c)
#define HDMI_PHY_REG_4          (MSM_HDMI_BASE + 0x00000410)
#define HDMI_PHY_REG_9          (MSM_HDMI_BASE + 0x00000424)
#define HDMI_PHY_REG_11         (MSM_HDMI_BASE + 0x0000042c)
#define HDMI_PHY_REG_12         (MSM_HDMI_BASE + 0x00000430)
#define HDMI_TOTAL              (MSM_HDMI_BASE + 0x000002C0)
#define HDMI_ACTIVE_HSYNC       (MSM_HDMI_BASE + 0x000002B4)
#define HDMI_ACTIVE_VSYNC       (MSM_HDMI_BASE + 0x000002B8)
#define HDMI_VSYNC_TOTAL_F2     (MSM_HDMI_BASE + 0x000002C4)
#define HDMI_VSYNC_ACTIVE_F2    (MSM_HDMI_BASE + 0x000002BC)
#define HDMI_FRAME_CTRL         (MSM_HDMI_BASE + 0x000002C8)
#define CE1_CRYPTO4_BASE        (0x18500000)
#define CE3_CRYPTO4_BASE        (0x11000000)
#define LCDC_BASE               (0x000C0000)
#define GSBI_BASE(id)           (id < 10)? GSBI_BASE1(id):SPS_GSBI_BASE(id)
#define QUP_BASE(id)            (id < 10)? QUP_BASE1(id):SPS_QUP_BASE(id)
#define GSBI_UART_DM_BASE(id)   (id < 10)? GSBI_UART_DM_BASE1(id):SPS_GSBI_UART_DM_BASE(id)

#define GSBI_BASE1(id)           (0x16000000 + ((id-1) * 0x100000))
#define GSBI_UART_DM_BASE1(id)   (GSBI_BASE1(id) + 0x40000)
#define QUP_BASE1(id)            (GSBI_BASE1(id) + 0x80000)

#define SPS_GSBI_BASE(id)           (0x12440000 + ((id-1) * 0x40000))
#define SPS_GSBI_UART_DM_BASE(id)   (SPS_GSBI_BASE(id) + 0x10000)
#define SPS_QUP_BASE(id)            (SPS_GSBI_BASE(id) + 0x20000)
#endif
