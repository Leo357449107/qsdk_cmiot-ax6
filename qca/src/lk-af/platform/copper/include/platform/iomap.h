/* Copyright (c) 2012, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 */

#ifndef _PLATFORM_MSMCOPPER_IOMAP_H_
#define _PLATFORM_MSMCOPPER_IOMAP_H_

#define MSM_IOMAP_BASE              0xF9000000
#define MSM_IOMAP_END               0xFEFFFFFF

#define SDRAM_START_ADDR            0x00000000
#define SDRAM_SEC_BANK_START_ADDR   0x10000000

#define MSM_SHARED_BASE             0x0FA00000

#define RPM_MSG_RAM_BASE            0xFC42B000
#define RESTART_REASON_ADDR         (RPM_MSG_RAM_BASE + 0x65C)

#define KPSS_BASE                   0xF9000000

#define MSM_GIC_DIST_BASE           KPSS_BASE
#define MSM_GIC_CPU_BASE            (KPSS_BASE + 0x2000)
#define APCS_KPSS_ACS_BASE          (KPSS_BASE + 0x00008000)
#define APCS_APC_KPSS_PLL_BASE      (KPSS_BASE + 0x0000A000)
#define APCS_KPSS_CFG_BASE          (KPSS_BASE + 0x00010000)
#define APCS_KPSS_WDT_BASE          (KPSS_BASE + 0x00017000)
#define KPSS_APCS_QTMR_AC_BASE      (KPSS_BASE + 0x00020000)
#define KPSS_APCS_F0_QTMR_V1_BASE   (KPSS_BASE + 0x00021000)
#define QTMR_BASE                   KPSS_APCS_F0_QTMR_V1_BASE

#define PERIPH_SS_BASE              0xF9800000

#define MSM_SDC1_BAM_BASE           (PERIPH_SS_BASE + 0x00004000)
#define MSM_SDC1_BASE               (PERIPH_SS_BASE + 0x00024000)
#define MSM_SDC1_DML_BASE           (PERIPH_SS_BASE + 0x00024800)
#define MSM_SDC3_BAM_BASE           (PERIPH_SS_BASE + 0x00044000)
#define MSM_SDC3_BASE               (PERIPH_SS_BASE + 0x00064000)
#define MSM_SDC3_DML_BASE           (PERIPH_SS_BASE + 0x00064800)
#define MSM_SDC2_BAM_BASE           (PERIPH_SS_BASE + 0x00084000)
#define MSM_SDC2_BASE               (PERIPH_SS_BASE + 0x000A4000)
#define MSM_SDC2_DML_BASE           (PERIPH_SS_BASE + 0x000A4800)
#define MSM_SDC4_BAM_BASE           (PERIPH_SS_BASE + 0x000C4000)
#define MSM_SDC4_BASE               (PERIPH_SS_BASE + 0x000E4000)
#define MSM_SDC4_DML_BASE           (PERIPH_SS_BASE + 0x000E4800)

#define BLSP1_UART0_BASE            (PERIPH_SS_BASE + 0x0011D000)
#define BLSP1_UART1_BASE            (PERIPH_SS_BASE + 0x0011E000)
#define BLSP1_UART2_BASE            (PERIPH_SS_BASE + 0x0011F000)
#define BLSP1_UART3_BASE            (PERIPH_SS_BASE + 0x00120000)
#define BLSP1_UART4_BASE            (PERIPH_SS_BASE + 0x00121000)
#define BLSP1_UART5_BASE            (PERIPH_SS_BASE + 0x00122000)
#define MSM_USB_BASE                (PERIPH_SS_BASE + 0x00255000)

#define CLK_CTL_BASE                0xFC400000
#define USB_HS_BCR                  (CLK_CTL_BASE + 0x480)
#define USB_BOOT_CLOCK_CTL          (CLK_CTL_BASE + 0x1A00)

#define SPMI_BASE                   0xFC4C0000
#define SPMI_GENI_BASE              (SPMI_BASE + 0xA000)
#define SPMI_PIC_BASE               (SPMI_BASE + 0xB000)

#define TLMM_BASE_ADDR              0xFD500000
#define GPIO_CONFIG_ADDR(x)         (TLMM_BASE_ADDR + 0x1000 + (x)*0x10)
#define GPIO_IN_OUT_ADDR(x)         (TLMM_BASE_ADDR + 0x1004 + (x)*0x10)

#define MPM2_MPM_CTRL_BASE          0xFC4A1000
#define MPM2_MPM_PS_HOLD            0xFC4AB000

/* GPLL */
#define GPLL0_STATUS                (CLK_CTL_BASE + 0x001C)
#define APCS_GPLL_ENA_VOTE          (CLK_CTL_BASE + 0x1480)

/* SDCC */
#define SDCC1_BCR                   (CLK_CTL_BASE + 0x4C0) /* block reset */
#define SDCC1_APPS_CBCR             (CLK_CTL_BASE + 0x4C4) /* branch control */
#define SDCC1_AHB_CBCR              (CLK_CTL_BASE + 0x4C8)
#define SDCC1_INACTIVITY_TIMER_CBCR (CLK_CTL_BASE + 0x4CC)
#define SDCC1_CMD_RCGR              (CLK_CTL_BASE + 0x4D0) /* cmd */
#define SDCC1_CFG_RCGR              (CLK_CTL_BASE + 0x4D4) /* cfg */
#define SDCC1_M                     (CLK_CTL_BASE + 0x4D8) /* m */
#define SDCC1_N                     (CLK_CTL_BASE + 0x4DC) /* n */
#define SDCC1_D                     (CLK_CTL_BASE + 0x4E0) /* d */

/* UART */
#define BLSP1_UART1_APPS_CBCR       (CLK_CTL_BASE + 0x684)
#define BLSP1_UART1_APPS_CMD_RCGR   (CLK_CTL_BASE + 0x68C)
#define BLSP1_UART1_APPS_CFG_RCGR   (CLK_CTL_BASE + 0x690)
#define BLSP1_UART1_APPS_M          (CLK_CTL_BASE + 0x694)
#define BLSP1_UART1_APPS_N          (CLK_CTL_BASE + 0x698)
#define BLSP1_UART1_APPS_D          (CLK_CTL_BASE + 0x69C)

#define BLSP1_UART3_APPS_CBCR       (CLK_CTL_BASE + 0x784)
#define BLSP1_UART3_APPS_CMD_RCGR   (CLK_CTL_BASE + 0x78C)
#define BLSP1_UART3_APPS_CFG_RCGR   (CLK_CTL_BASE + 0x790)
#define BLSP1_UART3_APPS_M          (CLK_CTL_BASE + 0x794)
#define BLSP1_UART3_APPS_N          (CLK_CTL_BASE + 0x798)
#define BLSP1_UART3_APPS_D          (CLK_CTL_BASE + 0x79C)

/* USB */
#define USB_HS_SYSTEM_CBCR          (CLK_CTL_BASE + 0x484)
#define USB_HS_AHB_CBCR             (CLK_CTL_BASE + 0x488)
#define USB_HS_SYSTEM_CMD_RCGR      (CLK_CTL_BASE + 0x490)
#define USB_HS_SYSTEM_CFG_RCGR      (CLK_CTL_BASE + 0x494)

#endif
