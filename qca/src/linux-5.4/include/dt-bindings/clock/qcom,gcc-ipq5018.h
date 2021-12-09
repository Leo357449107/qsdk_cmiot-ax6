/* Copyright (c) 2016-2020, The Linux Foundation. All rights reserved.
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
 *
 */

#ifndef _DT_BINDINGS_CLOCK_IPQ_GCC_5018_H
#define _DT_BINDINGS_CLOCK_IPQ_GCC_5018_H

#define GPLL0_MAIN					0
#define GPLL0						1
#define GPLL2_MAIN					2
#define GPLL2						3
#define GPLL4_MAIN					4
#define GPLL4						5
#define UBI32_PLL_MAIN					6
#define UBI32_PLL					7
#define APSS_AHB_CLK_SRC				9
#define APSS_AHB_POSTDIV_CLK_SRC			10
#define APSS_AXI_CLK_SRC				11
#define BLSP1_QUP1_I2C_APPS_CLK_SRC			12
#define BLSP1_QUP1_SPI_APPS_CLK_SRC			13
#define BLSP1_QUP2_I2C_APPS_CLK_SRC			14
#define BLSP1_QUP2_SPI_APPS_CLK_SRC			15
#define BLSP1_QUP3_I2C_APPS_CLK_SRC			16
#define BLSP1_QUP3_SPI_APPS_CLK_SRC			17
#define BLSP1_UART1_APPS_CLK_SRC			18
#define BLSP1_UART2_APPS_CLK_SRC			19
#define CRYPTO_CLK_SRC					20
#define GCC_APSS_AHB_CLK				23
#define GCC_APSS_AXI_CLK				24
#define GCC_BLSP1_AHB_CLK				25
#define GCC_BLSP1_QUP1_I2C_APPS_CLK			26
#define GCC_BLSP1_QUP1_SPI_APPS_CLK			27
#define GCC_BLSP1_QUP2_I2C_APPS_CLK			28
#define GCC_BLSP1_QUP2_SPI_APPS_CLK			29
#define GCC_BLSP1_QUP3_I2C_APPS_CLK			30
#define GCC_BLSP1_QUP3_SPI_APPS_CLK			31
#define GCC_BLSP1_UART1_APPS_CLK			33
#define GCC_BLSP1_UART2_APPS_CLK			34
#define GCC_BTSS_LPO_CLK				36
#define GCC_CMN_BLK_AHB_CLK				40
#define GCC_CMN_BLK_SYS_CLK				41
#define GCC_CRYPTO_AHB_CLK				44
#define GCC_CRYPTO_AXI_CLK				45
#define GCC_CRYPTO_CLK					46
#define GCC_CRYPTO_PPE_CLK				47
#define GCC_DCC_CLK					48
#define GCC_GEPHY_RX_CLK				53
#define GCC_GEPHY_TX_CLK				54
#define GCC_GMAC0_CFG_CLK				55
#define GCC_GMAC0_PTP_CLK				56
#define GCC_GMAC0_RX_CLK				57
#define GCC_GMAC0_SYS_CLK				58
#define GCC_GMAC0_TX_CLK				59
#define GCC_GMAC1_CFG_CLK				60
#define GCC_GMAC1_PTP_CLK				61
#define GCC_GMAC1_RX_CLK				62
#define GCC_GMAC1_SYS_CLK				63
#define GCC_GMAC1_TX_CLK				64
#define GCC_GP1_CLK					65
#define GCC_GP2_CLK					66
#define GCC_GP3_CLK					67
#define GCC_LPASS_CORE_AXIM_CLK				69
#define GCC_LPASS_SWAY_CLK				70
#define GCC_MDIO0_AHB_CLK				71
#define GCC_MDIO1_AHB_CLK				72
#define GCC_PCIE0_AHB_CLK				74
#define GCC_PCIE0_AUX_CLK				75
#define GCC_PCIE0_AXI_M_CLK				76
#define GCC_PCIE0_AXI_S_BRIDGE_CLK			77
#define GCC_PCIE0_AXI_S_CLK				78
#define GCC_PCIE1_AHB_CLK				79
#define GCC_PCIE1_AUX_CLK				80
#define GCC_PCIE1_AXI_M_CLK				81
#define GCC_PCIE1_AXI_S_BRIDGE_CLK			82
#define GCC_PCIE1_AXI_S_CLK				83
#define GCC_PRNG_AHB_CLK				84
#define GCC_Q6_AXIM_CLK					85
#define GCC_Q6_AXIM2_CLK				86
#define GCC_Q6_AXIS_CLK					87
#define GCC_Q6_AHB_CLK					88
#define GCC_Q6_AHB_S_CLK				89
#define GCC_Q6_TSCTR_1TO2_CLK				90
#define GCC_Q6SS_ATBM_CLK				91
#define GCC_Q6SS_PCLKDBG_CLK				92
#define GCC_Q6SS_TRIG_CLK				93
#define GCC_QDSS_AT_CLK					94
#define GCC_QDSS_CFG_AHB_CLK				95
#define GCC_QDSS_DAP_AHB_CLK				96
#define GCC_QDSS_DAP_CLK				97
#define GCC_QDSS_ETR_USB_CLK				98
#define GCC_QDSS_EUD_AT_CLK				99
#define GCC_QDSS_STM_CLK				100
#define GCC_QDSS_TRACECLKIN_CLK				101
#define GCC_QDSS_TSCTR_DIV8_CLK				102
#define GCC_QPIC_AHB_CLK				103
#define GCC_QPIC_CLK					104
#define GCC_QPIC_IO_MACRO_CLK				105
#define GCC_SDCC1_AHB_CLK				107
#define GCC_SDCC1_APPS_CLK				108
#define GCC_SLEEP_CLK_SRC				109
#define GCC_SNOC_GMAC0_AHB_CLK				110
#define GCC_SNOC_GMAC0_AXI_CLK				111
#define GCC_SNOC_GMAC1_AHB_CLK				112
#define GCC_SNOC_GMAC1_AXI_CLK				113
#define GCC_SNOC_LPASS_AXIM_CLK				114
#define GCC_SNOC_LPASS_SWAY_CLK				115
#define GCC_SNOC_UBI0_AXI_CLK				118
#define GCC_SYS_NOC_PCIE0_AXI_CLK			119
#define GCC_SYS_NOC_PCIE1_AXI_CLK			120
#define GCC_SYS_NOC_QDSS_STM_AXI_CLK			121
#define GCC_SYS_NOC_USB0_AXI_CLK			123
#define GCC_SYS_NOC_WCSS_AHB_CLK			124
#define GCC_UBI0_AXI_CLK				128
#define GCC_UBI0_CFG_CLK				129
#define GCC_UBI0_CORE_CLK				130
#define GCC_UBI0_DBG_CLK				131
#define GCC_UBI0_NC_AXI_CLK				132
#define GCC_UBI0_UTCM_CLK				133
#define GCC_UNIPHY_AHB_CLK				134
#define GCC_UNIPHY_RX_CLK				135
#define GCC_UNIPHY_SYS_CLK				136
#define GCC_UNIPHY_TX_CLK				137
#define GCC_USB0_AUX_CLK				138
#define GCC_USB0_EUD_AT_CLK				139
#define GCC_USB0_LFPS_CLK				140
#define GCC_USB0_MASTER_CLK				141
#define GCC_USB0_MOCK_UTMI_CLK				142
#define GCC_USB0_PHY_CFG_AHB_CLK			143
#define GCC_USB0_SLEEP_CLK				144
#define GCC_WCSS_ACMT_CLK				145
#define GCC_WCSS_AHB_S_CLK				146
#define GCC_WCSS_AXI_M_CLK				147
#define GCC_WCSS_AXI_S_CLK				148
#define GCC_WCSS_DBG_IFC_APB_BDG_CLK			149
#define GCC_WCSS_DBG_IFC_APB_CLK			150
#define GCC_WCSS_DBG_IFC_ATB_BDG_CLK			151
#define GCC_WCSS_DBG_IFC_ATB_CLK			152
#define GCC_WCSS_DBG_IFC_DAPBUS_BDG_CLK			153
#define GCC_WCSS_DBG_IFC_DAPBUS_CLK			154
#define GCC_WCSS_DBG_IFC_NTS_BDG_CLK			155
#define GCC_WCSS_DBG_IFC_NTS_CLK			156
#define GCC_WCSS_ECAHB_CLK				157
#define GCC_XO_CLK					158
#define GCC_XO_CLK_SRC					159
#define GMAC0_RX_CLK_SRC				161
#define GMAC0_TX_CLK_SRC				162
#define GMAC1_RX_CLK_SRC				163
#define GMAC1_TX_CLK_SRC				164
#define GMAC_CLK_SRC					165
#define GP1_CLK_SRC					166
#define GP2_CLK_SRC					167
#define GP3_CLK_SRC					168
#define LPASS_AXIM_CLK_SRC				169
#define LPASS_SWAY_CLK_SRC				170
#define PCIE0_AUX_CLK_SRC				171
#define PCIE0_AXI_CLK_SRC				172
#define PCIE1_AUX_CLK_SRC				173
#define PCIE1_AXI_CLK_SRC				174
#define PCNOC_BFDCD_CLK_SRC				175
#define Q6_AXI_CLK_SRC					176
#define QDSS_AT_CLK_SRC					177
#define QDSS_STM_CLK_SRC				178
#define QDSS_TSCTR_CLK_SRC				179
#define QDSS_TRACECLKIN_CLK_SRC				180
#define QPIC_IO_MACRO_CLK_SRC				181
#define SDCC1_APPS_CLK_SRC				182
#define SYSTEM_NOC_BFDCD_CLK_SRC			184
#define UBI0_AXI_CLK_SRC				185
#define UBI0_CORE_CLK_SRC				186
#define USB0_AUX_CLK_SRC				187
#define USB0_LFPS_CLK_SRC				188
#define USB0_MASTER_CLK_SRC				189
#define USB0_MOCK_UTMI_CLK_SRC				190
#define WCSS_AHB_CLK_SRC				191
#define PCIE0_PIPE_CLK_SRC				192
#define PCIE1_PIPE_CLK_SRC				193
#define GCC_PCIE0_PIPE_CLK				194
#define GCC_PCIE1_PIPE_CLK				195
#define USB0_PIPE_CLK_SRC				196
#define GCC_USB0_PIPE_CLK				197
#define GMAC0_RX_DIV_CLK_SRC				198
#define GMAC0_TX_DIV_CLK_SRC				199
#define GMAC1_RX_DIV_CLK_SRC				200
#define GMAC1_TX_DIV_CLK_SRC				201
#endif
