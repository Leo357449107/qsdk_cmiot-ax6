/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <soc/qcom/socinfo.h>
#include <linux/of_device.h>

#define PCIE_PHY_SW_RESET			0x600
#define PCIE_PHY_POWER_DOWN_CONTROL		0x604
#define PCIE_GEN3_PHY_PCS_STATUS		0x814
#define PCIE_GEN2_PHY_PCS_STATUS		0x974

#define PHY_DELAY_MS				0xFFFFFFFF
#define PHY_DELAY_TIME				1000
#define PHY_DELAY_MIN_US			995
#define PHY_DELAY_MAX_US			1005
#define PHY_RETRIES_COUNT			10

#define PIPE_CLK_DELAY_MIN_US			5000
#define PIPE_CLK_DELAY_MAX_US			5100
#define PIPE_CLK_RETRIES_COUNT			10

#define PCIE_USB3_PCS_POWER_DOWN_CONTROL		0x804
#define QSERDES_COM_BIAS_EN_CLKBUFLR_EN			0x34
#define QSERDES_COM_CLK_ENABLE1				0x38
#define QSERDES_COM_BG_TRIM				0x70
#define QSERDES_COM_LOCK_CMP_EN				0xC8
#define QSERDES_COM_VCO_TUNE_MAP			0x128
#define QSERDES_COM_VCO_TUNE_TIMER1			0x144
#define QSERDES_COM_VCO_TUNE_TIMER2			0x148
#define QSERDES_COM_CMN_CONFIG				0x194
#define QSERDES_COM_PLL_IVCO				0x48
#define QSERDES_COM_HSCLK_SEL				0x178
#define QSERDES_COM_SVS_MODE_CLK_SEL			0x19C
#define QSERDES_COM_CORE_CLK_EN				0x18C
#define QSERDES_COM_CORECLK_DIV				0x184
#define QSERDES_COM_RESETSM_CNTRL			0xB4
#define QSERDES_COM_BG_TIMER				0xC
#define QSERDES_COM_SYSCLK_EN_SEL			0xAC
#define QSERDES_COM_DEC_START_MODE0			0xD0
#define QSERDES_COM_DIV_FRAC_START3_MODE0		0xE4
#define QSERDES_COM_DIV_FRAC_START2_MODE0		0xE0
#define QSERDES_COM_DIV_FRAC_START1_MODE0		0xDC
#define QSERDES_COM_LOCK_CMP3_MODE0			0x54
#define QSERDES_COM_LOCK_CMP2_MODE0			0x50
#define QSERDES_COM_LOCK_CMP1_MODE0			0x4C
#define QSERDES_COM_CLK_SELECT				0x174
#define QSERDES_COM_SYS_CLK_CTRL			0x3C
#define QSERDES_COM_SYSCLK_BUF_ENABLE			0x40
#define QSERDES_COM_CP_CTRL_MODE0			0x78
#define QSERDES_COM_PLL_RCTRL_MODE0			0x84
#define QSERDES_COM_PLL_CCTRL_MODE0			0x90
#define QSERDES_COM_INTEGLOOP_GAIN1_MODE0		0x10C
#define QSERDES_COM_INTEGLOOP_GAIN0_MODE0		0x108
#define QSERDES_COM_BIAS_EN_CTRL_BY_PSM			0xA8
#define QSERDES_COM_SSC_EN_CENTER			0x10
#define QSERDES_COM_SSC_PER1				0x1C
#define QSERDES_COM_SSC_PER2				0x20
#define QSERDES_COM_SSC_ADJ_PER1			0x14
#define QSERDES_COM_SSC_ADJ_PER2			0x18
#define QSERDES_COM_SSC_STEP_SIZE1			0x24
#define QSERDES_COM_SSC_STEP_SIZE2			0x28
#define QSERDES_TX_HIGHZ_TRANSCEIVEREN_BIAS_DRVR_EN     0x268
#define QSERDES_TX_LANE_MODE                            0x294
#define QSERDES_TX_RES_CODE_LANE_OFFSET                 0x254
#define QSERDES_TX_RCV_DETECT_LVL_2                     0x2AC
#define QSERDES_RX_SIGDET_ENABLES                       0x510
#define QSERDES_RX_SIGDET_DEGLITCH_CNTRL                0x51C
#define QSERDES_RX_RX_EQU_ADAPTOR_CNTRL2                0x4D8
#define QSERDES_RX_RX_EQU_ADAPTOR_CNTRL3                0x4DC
#define QSERDES_RX_RX_EQU_ADAPTOR_CNTRL4                0x4E0
#define QSERDES_RX_UCDR_SO_SATURATION_AND_ENABLE        0x448
#define QSERDES_RX_UCDR_SO_GAIN                         0x41C
#define QSERDES_RX_UCDR_SO_GAIN_HALF                    0x410
#define QSERDES_COM_CLK_EP_DIV                          0x74
#define PCIE_USB3_PCS_ENDPOINT_REFCLK_DRIVE             0x854
#define PCIE_USB3_PCS_OSC_DTCT_ACTIONS                  0x9AC
#define PCIE_USB3_PCS_PWRUP_RESET_DLY_TIME_AUXCLK       0x8A0
#define PCIE_USB3_PCS_L1SS_WAKEUP_DLY_TIME_AUXCLK_MSB   0x9E0
#define PCIE_USB3_PCS_L1SS_WAKEUP_DLY_TIME_AUXCLK_LSB   0x9DC
#define PCIE_USB3_PCS_PLL_LOCK_CHK_DLY_TIME_AUXCLK_LSB  0x9A8
#define PCIE_USB3_PCS_LP_WAKEUP_DLY_TIME_AUXCLK         0x8A4
#define PCIE_USB3_PCS_PLL_LOCK_CHK_DLY_TIME             0x8A8
#define QSERDES_RX_SIGDET_CNTRL                         0x514
#define PCIE_USB3_PCS_RX_SIGDET_LVL                     0x9D8
#define PCIE_USB3_PCS_TXDEEMPH_M6DB_V0                  0x824
#define PCIE_USB3_PCS_TXDEEMPH_M3P5DB_V0                0x828
#define PCIE_USB3_PCS_SW_RESET                          0x800
#define PCIE_USB3_PCS_START_CONTROL                     0x808
#define PCIE_QSERDES_TX_TX_EMP_POST1_LVL		0x218
#define PCIE_QSERDES_TX_SLEW_CNTL			0x240

#define PCS_COM_POWER_DOWN_CONTROL		0x840
#define PCIE_0_QSERDES_PLL_BIAS_EN_CLKBUFLR_EN	0x03C
#define PCIE_0_QSERDES_PLL_BIAS_EN_CTRL_BY_PSM	0x0A4
#define PCIE_0_QSERDES_PLL_CLK_SELECT		0x16C
#define PCIE_0_QSERDES_PLL_PLL_IVCO		0x050

#define PCIE_0_QSERDES_PLL_BG_TRIM		0x074
#define PCIE_0_QSERDES_PLL_CMN_CONFIG		0x18C
#define PCIE_0_QSERDES_PLL_LOCK_CMP_EN		0x0C4
#define PCIE_0_QSERDES_PLL_RESETSM_CNTRL	0x0B0
#define PCIE_0_QSERDES_PLL_SVS_MODE_CLK_SEL	0x194
#define PCIE_0_QSERDES_PLL_VCO_TUNE_MAP		0x120
#define PCIE_0_QSERDES_PLL_VCO_TUNE_TIMER1	0x13C
#define PCIE_0_QSERDES_PLL_VCO_TUNE_TIMER2	0x140
#define PCIE_0_QSERDES_PLL_CORE_CLK_EN		0x184
#define PCIE_0_QSERDES_PLL_HSCLK_SEL		0x170
#define PCIE_0_QSERDES_PLL_DEC_START_MODE0	0x0CC
#define PCIE_0_QSERDES_PLL_DIV_FRAC_START3_MODE0 0x0E0
#define PCIE_0_QSERDES_PLL_DIV_FRAC_START2_MODE0 0x0DC
#define PCIE_0_QSERDES_PLL_DIV_FRAC_START1_MODE0 0x0D8
#define PCIE_0_QSERDES_PLL_LOCK_CMP2_MODE0	0x058
#define PCIE_0_QSERDES_PLL_LOCK_CMP1_MODE0	0x054
#define PCIE_0_QSERDES_PLL_CP_CTRL_MODE0	0x080
#define PCIE_0_QSERDES_PLL_PLL_RCTRL_MODE0	0x088
#define PCIE_0_QSERDES_PLL_PLL_CCTRL_MODE0	0x090
#define PCIE_0_QSERDES_PLL_INTEGLOOP_GAIN1_MODE0 0x104
#define PCIE_0_QSERDES_PLL_INTEGLOOP_GAIN0_MODE0 0x100
#define PCIE_0_QSERDES_PLL_VCO_TUNE2_MODE0	0x128
#define PCIE_0_QSERDES_PLL_VCO_TUNE1_MODE0	0x124
#define PCIE_0_QSERDES_PLL_CORECLK_DIV		0x17C
#define PCIE_0_QSERDES_PLL_SYS_CLK_CTRL		0x044
#define PCIE_0_QSERDES_PLL_SYSCLK_BUF_ENABLE	0x048
#define PCIE_0_QSERDES_PLL_SYSCLK_EN_SEL	0x0A8
#define PCIE_0_QSERDES_PLL_BG_TIMER		0x00C
#define PCIE_0_QSERDES_PLL_DEC_START_MODE1	0x0D0
#define PCIE_0_QSERDES_PLL_DIV_FRAC_START3_MODE1 0x0EC
#define PCIE_0_QSERDES_PLL_DIV_FRAC_START2_MODE1 0x0E8
#define PCIE_0_QSERDES_PLL_DIV_FRAC_START1_MODE1 0x0E4
#define PCIE_0_QSERDES_PLL_LOCK_CMP2_MODE1	0x064
#define PCIE_0_QSERDES_PLL_LOCK_CMP1_MODE1	0x060
#define PCIE_0_QSERDES_PLL_CP_CTRL_MODE1	0x084
#define PCIE_0_QSERDES_PLL_PLL_RCTRL_MODE1	0x08C
#define PCIE_0_QSERDES_PLL_PLL_CCTRL_MODE1	0x094
#define PCIE_0_QSERDES_PLL_INTEGLOOP_GAIN1_MODE1 0x10C
#define PCIE_0_QSERDES_PLL_INTEGLOOP_GAIN0_MODE1 0x108
#define PCIE_0_QSERDES_PLL_VCO_TUNE2_MODE1	0x130
#define PCIE_0_QSERDES_PLL_VCO_TUNE1_MODE1	0x12C
#define PCIE_0_QSERDES_PLL_CORECLK_DIV_MODE1	0x1B4
#define PCIE_0_QSERDES_TX0_RES_CODE_LANE_OFFSET_TX 0x23C
#define PCIE_0_QSERDES_TX0_RCV_DETECT_LVL_2	0x29C
#define PCIE_0_QSERDES_TX0_HIGHZ_DRVR_EN	0x258
#define PCIE_0_QSERDES_TX0_LANE_MODE_1		0x284
#define PCIE_0_QSERDES_RX0_SIGDET_CNTRL		0x51C
#define	PCIE_0_QSERDES_RX0_SIGDET_ENABLES	0x518
#define PCIE_0_QSERDES_RX0_SIGDET_DEGLITCH_CNTRL 0x524
#define PCIE_0_QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL2 0x4EC
#define PCIE_0_QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL3 0x4F0
#define PCIE_0_QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL4 0x4F4
#define PCIE_0_QSERDES_RX0_DFE_EN_TIMER		0x5B4
#define PCIE_0_QSERDES_RX0_UCDR_SO_SATURATION_AND_ENABLE 0x434
#define PCIE_0_QSERDES_RX0_UCDR_PI_CONTROLS	0x444
#define PCIE_0_QSERDES_RX0_RX_EQ_OFFSET_ADAPTOR_CNTRL1 0x510
#define PCIE_0_QSERDES_RX0_RX_OFFSET_ADAPTOR_CNTRL2 0x514
#define PCIE_0_QSERDES_RX0_RX_MODE_10_LOW	0x598
#define PCIE_0_QSERDES_RX0_RX_MODE_10_HIGH	0x59C
#define PCIE_0_QSERDES_RX0_RX_MODE_10_HIGH2	0x5A0
#define PCIE_0_QSERDES_RX0_RX_MODE_10_HIGH3	0x5A4
#define PCIE_0_QSERDES_RX0_RX_MODE_10_HIGH4	0x5A8
#define PCIE_0_QSERDES_RX0_RX_MODE_01_LOW	0x584
#define PCIE_0_QSERDES_RX0_RX_MODE_01_HIGH	0x588
#define PCIE_0_QSERDES_RX0_RX_MODE_01_HIGH2	0x58C
#define PCIE_0_QSERDES_RX0_RX_MODE_01_HIGH3	0x590
#define PCIE_0_QSERDES_RX0_RX_MODE_01_HIGH4	0x594
#define PCIE_0_QSERDES_RX0_RX_MODE_00_LOW	0x570
#define PCIE_0_QSERDES_RX0_RX_MODE_00_HIGH	0x574
#define PCIE_0_QSERDES_RX0_RX_MODE_00_HIGH2	0x578
#define PCIE_0_QSERDES_RX0_RX_MODE_00_HIGH3	0x57C
#define PCIE_0_QSERDES_RX0_RX_MODE_00_HIGH4	0x580
#define PCIE_0_QSERDES_RX0_RX_IDAC_TSETTLE_HIGH 0x4FC
#define PCIE_0_QSERDES_RX0_RX_IDAC_TSETTLE_LOW	0x4F8

#define PCIE_0_PCS_COM_FLL_CNTRL2		0x89C
#define PCIE_0_PCS_COM_FLL_CNT_VAL_L		0x8A0
#define PCIE_0_PCS_COM_FLL_CNT_VAL_H_TOL	0x8A4
#define PCIE_0_PCS_COM_FLL_MAN_CODE		0x8A8
#define PCIE_0_PCS_COM_FLL_CNTRL1		0x898

#define PCIE_0_PCS_COM_P2U3_WAKEUP_DLY_TIME_AUXCLK_H 0x9A8
#define PCIE_0_PCS_COM_P2U3_WAKEUP_DLY_TIME_AUXCLK_L 0x9A4

#define PCIE_0_PCS_PCIE_OSC_DTCT_ACTIONS	0xC90
#define PCIE_0_PCS_PCIE_L1P1_WAKEUP_DLY_TIME_AUXCLK_H 0xC44
#define PCIE_0_PCS_PCIE_L1P1_WAKEUP_DLY_TIME_AUXCLK_L 0xC40
#define PCIE_0_PCS_PCIE_L1P2_WAKEUP_DLY_TIME_AUXCLK_H 0xC4C
#define PCIE_0_PCS_PCIE_L1P2_WAKEUP_DLY_TIME_AUXCLK_L 0xC48
#define PCIE_0_PCS_PCIE_EQ_CONFIG1		0xCA0
#define PCIE_0_PCS_PCIE_EQ_CONFIG2		0xCA4

#define PCIE_0_PCS_PCIE_POWER_STATE_CONFIG4	0xC14
#define PCIE_0_PCS_PCIE_OSC_DTCT_CONFIG2	0xC5C
#define PCIE_0_PCS_PCIE_OSC_DTCT_MODE2_CONFIG2	0xC78
#define PCIE_0_PCS_PCIE_OSC_DTCT_MODE2_CONFIG4	0xC80
#define PCIE_0_PCS_PCIE_OSC_DTCT_MODE2_CONFIG5	0xC84

#define PCIE_0_QSERDES_PLL_CLK_EP_DIV_MODE0	0x078
#define PCIE_0_QSERDES_PLL_CLK_EP_DIV_MODE1	0x07C
#define PCIE_0_QSERDES_PLL_CLK_ENABLE1		0x040

#define PCIE_0_PCS_COM_POWER_DOWN_CONTROL	0x840
#define PCIE_0_PCS_PCIE_ENDPOINT_REFCLK_DRIVE	0xC1C
#define PCIE_0_PCS_COM_RX_DCC_CAL_CONFIG	0x9D8
#define PCIE_0_PCS_COM_RX_SIGDET_LVL		0x988
#define PCIE_0_PCS_COM_REFGEN_REQ_CONFIG1	0x8DC
#define PCIE_0_PCS_COM_REFGEN_REQ_CONFIG1	0x8DC
#define PCIE_0_PCS_COM_SW_RESET			0x800
#define PCIE_0_PCS_COM_START_CONTROL		0x844

enum qca_pcie_phy_type {
	PHY_TYPE_PCIE,
	PHY_TYPE_PCIE_GEN2,
	PHY_TYPE_PCIE_GEN3,
};


struct qca_pcie_qmp_phy {
	void __iomem *base;
	struct clk *pipe_clk;
	struct reset_control *res_phy;
	struct reset_control *res_phy_phy;
	struct device *dev;
	u32 is_phy_gen3;
	enum qca_pcie_phy_type phy_type;
	unsigned int *qmp_pcie_phy_init_seq;
	int init_seq_len;
};

struct phy_regs {
	u32 reg_offset;
	u32 val;
};

static const struct phy_regs pcie_phy_regs[] = {
	{ PCIE_USB3_PCS_POWER_DOWN_CONTROL,			0x00000003 },
	{ QSERDES_COM_BIAS_EN_CLKBUFLR_EN,			0x00000018 },
	{ QSERDES_COM_CLK_ENABLE1,				0x00000010 },
	{ QSERDES_COM_BG_TRIM,					0x0000000f },
	{ QSERDES_COM_LOCK_CMP_EN,				0x00000001 },
	{ QSERDES_COM_VCO_TUNE_MAP,				0x00000000 },
	{ QSERDES_COM_VCO_TUNE_TIMER1,				0x000000ff },
	{ QSERDES_COM_VCO_TUNE_TIMER2,				0x0000001f },
	{ QSERDES_COM_CMN_CONFIG,				0x00000006 },
	{ QSERDES_COM_PLL_IVCO,					0x0000000f },
	{ QSERDES_COM_HSCLK_SEL,				0x00000000 },
	{ QSERDES_COM_SVS_MODE_CLK_SEL,				0x00000001 },
	{ QSERDES_COM_CORE_CLK_EN,				0x00000020 },
	{ QSERDES_COM_CORECLK_DIV,				0x0000000a },
	{ QSERDES_COM_RESETSM_CNTRL,				0x00000020 },
	{ QSERDES_COM_BG_TIMER,					0x0000000a },
	{ QSERDES_COM_SYSCLK_EN_SEL,				0x0000000a },
	{ QSERDES_COM_DEC_START_MODE0,				0x00000082 },
	{ QSERDES_COM_DIV_FRAC_START3_MODE0,			0x00000003 },
	{ QSERDES_COM_DIV_FRAC_START2_MODE0,			0x00000055 },
	{ QSERDES_COM_DIV_FRAC_START1_MODE0,			0x00000055 },
	{ QSERDES_COM_LOCK_CMP3_MODE0,				0x00000000 },
	{ QSERDES_COM_LOCK_CMP2_MODE0,				0x0000000D },
	{ QSERDES_COM_LOCK_CMP1_MODE0,				0x00000D04 },
	{ QSERDES_COM_CLK_SELECT,				0x00000033 },
	{ QSERDES_COM_SYS_CLK_CTRL,				0x00000002 },
	{ QSERDES_COM_SYSCLK_BUF_ENABLE,			0x0000001f },
	{ QSERDES_COM_CP_CTRL_MODE0,				0x0000000b },
	{ QSERDES_COM_PLL_RCTRL_MODE0,				0x00000016 },
	{ QSERDES_COM_PLL_CCTRL_MODE0,				0x00000028 },
	{ QSERDES_COM_INTEGLOOP_GAIN1_MODE0,			0x00000000 },
	{ QSERDES_COM_INTEGLOOP_GAIN0_MODE0,			0x00000080 },
	{ QSERDES_COM_BIAS_EN_CTRL_BY_PSM,			0x00000001 },
	{ QSERDES_COM_SSC_EN_CENTER,				0x00000001 },
	{ QSERDES_COM_SSC_PER1,					0x00000031 },
	{ QSERDES_COM_SSC_PER2,					0x00000001 },
	{ QSERDES_COM_SSC_ADJ_PER1,				0x00000002 },
	{ QSERDES_COM_SSC_ADJ_PER2,				0x00000000 },
	{ QSERDES_COM_SSC_STEP_SIZE1,				0x0000002f },
	{ QSERDES_COM_SSC_STEP_SIZE2,				0x00000019 },
	{ QSERDES_TX_HIGHZ_TRANSCEIVEREN_BIAS_DRVR_EN,		0x00000045 },
	{ QSERDES_TX_LANE_MODE,					0x00000006 },
	{ QSERDES_TX_RES_CODE_LANE_OFFSET,			0x00000002 },
	{ QSERDES_TX_RCV_DETECT_LVL_2,				0x00000012 },
	{ QSERDES_RX_SIGDET_ENABLES,				0x0000001c },
	{ QSERDES_RX_SIGDET_DEGLITCH_CNTRL,			0x00000014 },
	{ QSERDES_RX_RX_EQU_ADAPTOR_CNTRL2,			0x00000001 },
	{ QSERDES_RX_RX_EQU_ADAPTOR_CNTRL3,			0x00000000 },
	{ QSERDES_RX_RX_EQU_ADAPTOR_CNTRL4,			0x000000db },
	{ QSERDES_RX_UCDR_SO_SATURATION_AND_ENABLE,		0x0000004b },
	{ QSERDES_RX_UCDR_SO_GAIN,				0x00000004 },
	{ QSERDES_RX_UCDR_SO_GAIN_HALF,				0x00000004 },
	{ QSERDES_COM_CLK_EP_DIV,				0x00000019 },
	{ PCIE_USB3_PCS_ENDPOINT_REFCLK_DRIVE,			0x00000004 },
	{ PCIE_USB3_PCS_OSC_DTCT_ACTIONS,			0x00000000 },
	{ PCIE_USB3_PCS_PWRUP_RESET_DLY_TIME_AUXCLK,		0x00000040 },
	{ PCIE_USB3_PCS_L1SS_WAKEUP_DLY_TIME_AUXCLK_MSB,	0x00000000 },
	{ PCIE_USB3_PCS_L1SS_WAKEUP_DLY_TIME_AUXCLK_LSB,	0x00000040 },
	{ PCIE_USB3_PCS_PLL_LOCK_CHK_DLY_TIME_AUXCLK_LSB,	0x00000000 },
	{ PCIE_USB3_PCS_LP_WAKEUP_DLY_TIME_AUXCLK,		0x00000040 },
	{ PCIE_USB3_PCS_PLL_LOCK_CHK_DLY_TIME,			0x00000073 },
	{ QSERDES_RX_SIGDET_CNTRL,				0x00000007 },
	{ PCIE_USB3_PCS_RX_SIGDET_LVL,				0x00000099 },
	{ PCIE_USB3_PCS_TXDEEMPH_M6DB_V0,			0x00000015 },
	{ PCIE_USB3_PCS_TXDEEMPH_M3P5DB_V0,			0x0000000e },
	{ PCIE_USB3_PCS_SW_RESET,				0x00000000 },
	{ PCIE_USB3_PCS_START_CONTROL,				0x00000003 },
	{ PCIE_QSERDES_TX_TX_EMP_POST1_LVL,			0x00000036 },
	{ PCIE_QSERDES_TX_SLEW_CNTL,				0x0000000A },
};

static int qca_pcie_qmp_phy_init(struct qca_pcie_qmp_phy *pcie)
{
	const struct phy_regs *regs = pcie_phy_regs;
	int i;

	for (i = 0; i < ARRAY_SIZE(pcie_phy_regs); i++)
		writel(regs[i].val, pcie->base + regs[i].reg_offset);

	return 0;
}

static inline void qca_pcie_write_reg(void *base, u32 offset, u32 value)
{
        writel(value, base + offset);
}

static int qca_pcie_qmp_phy_v2_init(struct qca_pcie_qmp_phy *pcie)
{
	if (pcie->qmp_pcie_phy_init_seq) {
		const struct phy_regs *regs = NULL;
		int i;

		regs = (const struct phy_regs *)pcie->qmp_pcie_phy_init_seq;
		for (i = 0; i < pcie->init_seq_len; i++) {
			if (regs[i].reg_offset == PHY_DELAY_MS)
				usleep_range(PHY_DELAY_TIME * regs[i].val,
					     PHY_DELAY_TIME * regs[i].val + 10);
			else
				qca_pcie_write_reg(pcie->base,
						   regs[i].reg_offset,
						   regs[i].val);
		}
	} else {
		qca_pcie_write_reg(pcie->base, PCS_COM_POWER_DOWN_CONTROL, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_BIAS_EN_CLKBUFLR_EN, 0x18);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_BIAS_EN_CTRL_BY_PSM, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CLK_SELECT, 0x31);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_PLL_IVCO, 0xF);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_BG_TRIM, 0xF);
		usleep_range(PHY_DELAY_MIN_US, PHY_DELAY_MAX_US);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CMN_CONFIG, 0x6);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_LOCK_CMP_EN, 0x42);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_RESETSM_CNTRL, 0x20);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_SVS_MODE_CLK_SEL, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_VCO_TUNE_MAP, 0x4);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_SVS_MODE_CLK_SEL, 0x5);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_VCO_TUNE_TIMER1, 0xFF);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_VCO_TUNE_TIMER2, 0x3F);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CORE_CLK_EN, 0x30);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_HSCLK_SEL, 0x21);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_DEC_START_MODE0, 0x82);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_DIV_FRAC_START3_MODE0, 0x3);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_DIV_FRAC_START2_MODE0, 0x355);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_DIV_FRAC_START1_MODE0, 0x35555);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_LOCK_CMP2_MODE0, 0x1A);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_LOCK_CMP1_MODE0, 0x1A0A);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CP_CTRL_MODE0, 0xB);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_PLL_RCTRL_MODE0, 0x16);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_PLL_CCTRL_MODE0, 0x28);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_INTEGLOOP_GAIN1_MODE0, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_INTEGLOOP_GAIN0_MODE0, 0x40);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_VCO_TUNE2_MODE0, 0x2);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_VCO_TUNE1_MODE0, 0x24);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_SVS_MODE_CLK_SEL, 0x5);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CORE_CLK_EN, 0x20);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CORECLK_DIV, 0xA);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CLK_SELECT, 0x32);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_SYS_CLK_CTRL, 0x2);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_SYSCLK_BUF_ENABLE, 0x7);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_SYSCLK_EN_SEL, 0x8);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_BG_TIMER, 0xA);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_HSCLK_SEL, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_DEC_START_MODE1, 0x68);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_DIV_FRAC_START3_MODE1, 0x2);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_DIV_FRAC_START2_MODE1, 0x2AA);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_DIV_FRAC_START1_MODE1, 0x2AAAB);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_LOCK_CMP2_MODE1, 0x34);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_LOCK_CMP1_MODE1, 0x3414);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CP_CTRL_MODE1, 0xB);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_PLL_RCTRL_MODE1, 0x16);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_PLL_CCTRL_MODE1, 0x28);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_INTEGLOOP_GAIN1_MODE1, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_INTEGLOOP_GAIN0_MODE1, 0x40);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_VCO_TUNE2_MODE1, 0x3);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_VCO_TUNE1_MODE1, 0xB4);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_SVS_MODE_CLK_SEL, 0x5);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CORE_CLK_EN, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CORECLK_DIV_MODE1, 0x8);

		/*qmp_tx_init*/
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_TX0_RES_CODE_LANE_OFFSET_TX, 0x2);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_TX0_RCV_DETECT_LVL_2, 0x12);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_TX0_HIGHZ_DRVR_EN, 0x10);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_TX0_LANE_MODE_1, 0x6);

		/*qmp_rx_init*/
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_SIGDET_CNTRL, 0x3);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_SIGDET_ENABLES, 0x1C);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_SIGDET_DEGLITCH_CNTRL, 0x14);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL2, 0xE);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL3, 0x4);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL4, 0x1B);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_DFE_EN_TIMER, 0x4);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_UCDR_SO_SATURATION_AND_ENABLE, 0x7F);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_UCDR_PI_CONTROLS, 0x70);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_EQ_OFFSET_ADAPTOR_CNTRL1, 0x73);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_OFFSET_ADAPTOR_CNTRL2, 0x80);

		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_10_LOW, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_10_HIGH, 0x2);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_10_HIGH2, 0xC8);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_10_HIGH3, 0x9);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_10_HIGH4, 0xB1);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_01_LOW, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_01_HIGH, 0x2);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_01_HIGH2, 0xC8);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_01_HIGH3, 0x9);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_01_HIGH4, 0xB1);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_00_LOW, 0xF0);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_00_HIGH, 0x2);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_00_HIGH2, 0x2F);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_00_HIGH3, 0xD3);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_MODE_00_HIGH4, 0x40);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_IDAC_TSETTLE_HIGH, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_RX0_RX_IDAC_TSETTLE_LOW, 0xC0);

		/* pcie fll config*/
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_FLL_CNTRL2, 0x83);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_FLL_CNT_VAL_L, 0x9);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_FLL_CNT_VAL_H_TOL, 0xA2);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_FLL_MAN_CODE, 0x40);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_FLL_CNTRL1, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_P2U3_WAKEUP_DLY_TIME_AUXCLK_H, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_P2U3_WAKEUP_DLY_TIME_AUXCLK_L, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_OSC_DTCT_ACTIONS, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_L1P1_WAKEUP_DLY_TIME_AUXCLK_H, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_L1P1_WAKEUP_DLY_TIME_AUXCLK_L, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_L1P2_WAKEUP_DLY_TIME_AUXCLK_H, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_L1P2_WAKEUP_DLY_TIME_AUXCLK_L, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_EQ_CONFIG1, 0x11);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_EQ_CONFIG2, 0xB);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_POWER_STATE_CONFIG4, 0x7);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_OSC_DTCT_CONFIG2, 0x52);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_OSC_DTCT_MODE2_CONFIG2, 0x50);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_OSC_DTCT_MODE2_CONFIG4, 0x1A);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_OSC_DTCT_MODE2_CONFIG5, 0x6);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CLK_EP_DIV_MODE0, 0x19);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CLK_EP_DIV_MODE1, 0x28);
		qca_pcie_write_reg(pcie->base, PCIE_0_QSERDES_PLL_CLK_ENABLE1, 0x90);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_POWER_DOWN_CONTROL, 0x3);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_PCIE_ENDPOINT_REFCLK_DRIVE, 0xC1);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_RX_DCC_CAL_CONFIG, 0x1);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_RX_SIGDET_LVL, 0xAA);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_REFGEN_REQ_CONFIG1, 0xD);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_SW_RESET, 0x0);
		qca_pcie_write_reg(pcie->base, PCIE_0_PCS_COM_START_CONTROL, 0x3);
	}

	usleep_range(PHY_DELAY_MIN_US, PHY_DELAY_MAX_US);
	return 0;
}

static bool qca_pcie_qmp_phy_is_ready(struct qca_pcie_qmp_phy *pcie)
{
	u32 val;

	if (pcie->is_phy_gen3)
		val = readl(pcie->base + PCIE_GEN3_PHY_PCS_STATUS);
	else
		val = readl(pcie->base + PCIE_GEN2_PHY_PCS_STATUS);

	return val & BIT(6) ? false : true;
}

static int qca_pcie_qmp_phy_reset(struct qca_pcie_qmp_phy *pcie)
{
	reset_control_assert(pcie->res_phy);
	reset_control_assert(pcie->res_phy_phy);

	usleep_range(100, 150);

	reset_control_deassert(pcie->res_phy);
	reset_control_deassert(pcie->res_phy_phy);

	return 0;
}

static int qca_pcie_qmp_phy_power_on(struct phy *phy)
{
	struct qca_pcie_qmp_phy *pcie = phy_get_drvdata(phy);
	int ret, retries;

	qca_pcie_qmp_phy_reset(pcie);
	if (pcie->is_phy_gen3)
		clk_set_rate(pcie->pipe_clk, 250000000);
	else
		clk_set_rate(pcie->pipe_clk, 125000000);

	usleep_range(PIPE_CLK_DELAY_MIN_US, PIPE_CLK_DELAY_MAX_US);
	clk_prepare_enable(pcie->pipe_clk);
	if (pcie->is_phy_gen3)
		qca_pcie_qmp_phy_v2_init(pcie);
	else {
		qca_pcie_qmp_phy_init(pcie);
	}

	retries = PHY_RETRIES_COUNT;
	do {
		ret = qca_pcie_qmp_phy_is_ready(pcie);
		if (ret)
			break;
		usleep_range(PHY_DELAY_MIN_US, PHY_DELAY_MAX_US);
	} while (retries--);

	if (retries < 0) {
		dev_err(pcie->dev, "phy failed to come up\n");
		ret = -ETIMEDOUT;
		goto fail;
	}

	return 0;

fail:
	clk_disable_unprepare(pcie->pipe_clk);
	reset_control_assert(pcie->res_phy);
	reset_control_assert(pcie->res_phy_phy);

	return ret;
}

static int qca_pcie_qmp_phy_power_off(struct phy *phy)
{
	struct qca_pcie_qmp_phy *pcie = phy_get_drvdata(phy);

	writel(1, pcie->base + PCIE_PHY_SW_RESET);
	writel(0, pcie->base + PCIE_PHY_POWER_DOWN_CONTROL);

	reset_control_assert(pcie->res_phy);
	reset_control_assert(pcie->res_phy_phy);
	clk_disable_unprepare(pcie->pipe_clk);

	return 0;
}

static struct phy_ops qca_pcie_qmp_phy_ops = {
	.power_on = qca_pcie_qmp_phy_power_on,
	.power_off = qca_pcie_qmp_phy_power_off,
	.owner = THIS_MODULE,
};

static int qca_pcie_qmp_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy_provider *provider;
	struct qca_pcie_qmp_phy *pcie;
	struct resource *res;
	struct phy *phy;
	u32 soc_version_major;
	const char *name;
	int size = 0, ret;

	pcie = devm_kzalloc(dev, sizeof(*pcie), GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	ret = of_property_read_string(dev->of_node, "phy-type",&name);
	if (!ret) {
		if (!strcmp(name, "gen3")) {
			pcie->phy_type = PHY_TYPE_PCIE_GEN3;
		} else {
			dev_err(dev, "%s, unknown gen type\n", __func__);
			goto err;
		}
	} else if (ret == -EINVAL) {
		soc_version_major = read_ipq_soc_version_major();
		BUG_ON(soc_version_major <= 0);

		pcie->phy_type = (enum qca_pcie_phy_type)of_device_get_match_data(dev);
		if ((soc_version_major == 1) && ( pcie->phy_type == PHY_TYPE_PCIE_GEN3)) {
			goto err;
		} else if ((soc_version_major == 2) && ( pcie->phy_type == PHY_TYPE_PCIE_GEN2)) {
			goto err;
		}
	} else {
		dev_err(dev, "%s, unknown gen type\n", __func__);
		goto err;
	}

	if (pcie->phy_type == PHY_TYPE_PCIE_GEN3)
		pcie->is_phy_gen3 = 1;
	else
		pcie->is_phy_gen3 = 0;

	of_get_property(dev->of_node, "qcom,qmp-pcie-phy-init-seq", &size);
	if (size) {
		pcie->qmp_pcie_phy_init_seq = devm_kzalloc(dev,
				size, GFP_KERNEL);
		if (pcie->qmp_pcie_phy_init_seq) {
			pcie->init_seq_len =
				size / sizeof(*pcie->qmp_pcie_phy_init_seq) / 2;
			of_property_read_u32_array(dev->of_node,
					"qcom,qmp-pcie-phy-init-seq",
					pcie->qmp_pcie_phy_init_seq,
					pcie->init_seq_len * 2);
		} else {
			dev_err(dev, "%s: error allocating memory for phy_init_seq\n", __func__);
		}
	}

	pcie->dev = &pdev->dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pcie->base = devm_ioremap_resource(pcie->dev, res);
	if (IS_ERR(pcie->base))
		return PTR_ERR(pcie->base);

	pcie->pipe_clk = devm_clk_get(dev, "pipe_clk");
	if (IS_ERR(pcie->pipe_clk)) {
		return PTR_ERR(pcie->pipe_clk);
	}

	pcie->res_phy = devm_reset_control_get(dev, "phy");
	if (IS_ERR(pcie->res_phy)) {
		dev_err(dev, "cannot get phy reset controller");
		return PTR_ERR(pcie->res_phy);
	}

	pcie->res_phy_phy = devm_reset_control_get(dev, "phy_phy");
	if (IS_ERR(pcie->res_phy_phy)) {
		dev_err(dev, "cannot get phy_phy reset controller");
		return PTR_ERR(pcie->res_phy_phy);
	}

	phy = devm_phy_create(pcie->dev, NULL, &qca_pcie_qmp_phy_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "failed to create phy\n");
		return PTR_ERR(phy);
	}

	phy_set_drvdata(phy, pcie);
	platform_set_drvdata(pdev, pcie);
	provider = devm_of_phy_provider_register(pcie->dev,
				of_phy_simple_xlate);
	if (IS_ERR(provider))
		return PTR_ERR(provider);

	return 0;
err:
	devm_kfree(dev, pcie);
	return 0;
}

static int qca_pcie_qmp_phy_remove(struct platform_device *pdev)
{
	struct qca_pcie_qmp_phy *pcie = platform_get_drvdata(pdev);

	clk_disable_unprepare(pcie->pipe_clk);

	return 0;
}

static const struct of_device_id qca_pcie_qmp_phy_of_match[] = {
	{ .compatible = "qca,pcie-qmp-phy", .data = (void *) PHY_TYPE_PCIE},
	{ .compatible = "qca,pcie-qmp-phy-gen2", .data = (void *) PHY_TYPE_PCIE_GEN2},
	{ .compatible = "qca,pcie-qmp-phy-gen3", .data = (void *) PHY_TYPE_PCIE_GEN3},
	{ },
};
MODULE_DEVICE_TABLE(of, qca_pcie_qmp_phy_of_match);

static struct platform_driver qca_pcie_qmp_phy_driver = {
	.probe = qca_pcie_qmp_phy_probe,
	.remove = qca_pcie_qmp_phy_remove,
	.driver = {
		.name = "pcie-phy",
		.of_match_table = qca_pcie_qmp_phy_of_match,
	}
};
module_platform_driver(qca_pcie_qmp_phy_driver);

MODULE_DESCRIPTION("PCIe QMP PHY driver");
MODULE_ALIAS("platform:pcie-phy");
