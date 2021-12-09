/*
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * Based on board/ti/dra7xx/evm.c
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _MUX_DATA_BEAGLE_X15_H_
#define _MUX_DATA_BEAGLE_X15_H_

#include <asm/arch/mux_dra7xx.h>

const struct pad_conf_entry core_padconf_array_essential[] = {
	{GPMC_AD0, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad0.vin3a_d0 */
	{GPMC_AD1, (M2 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* gpmc_ad1.vin3a_d1 */
	{GPMC_AD2, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad2.vin3a_d2 */
	{GPMC_AD3, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad3.vin3a_d3 */
	{GPMC_AD4, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad4.vin3a_d4 */
	{GPMC_AD5, (M2 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* gpmc_ad5.vin3a_d5 */
	{GPMC_AD6, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad6.vin3a_d6 */
	{GPMC_AD7, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad7.vin3a_d7 */
	{GPMC_AD8, (M2 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* gpmc_ad8.vin3a_d8 */
	{GPMC_AD9, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad9.vin3a_d9 */
	{GPMC_AD10, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad10.vin3a_d10 */
	{GPMC_AD11, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad11.vin3a_d11 */
	{GPMC_AD12, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad12.vin3a_d12 */
	{GPMC_AD13, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad13.vin3a_d13 */
	{GPMC_AD14, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_ad14.vin3a_d14 */
	{GPMC_AD15, (M2 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* gpmc_ad15.vin3a_d15 */
	{GPMC_A0, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a0.vin3a_d16 */
	{GPMC_A1, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a1.vin3a_d17 */
	{GPMC_A2, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a2.vin3a_d18 */
	{GPMC_A3, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a3.vin3a_d19 */
	{GPMC_A4, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a4.vin3a_d20 */
	{GPMC_A5, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a5.vin3a_d21 */
	{GPMC_A6, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a6.vin3a_d22 */
	{GPMC_A7, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a7.vin3a_d23 */
	{GPMC_A8, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a8.vin3a_hsync0 */
	{GPMC_A9, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a9.vin3a_vsync0 */
	{GPMC_A10, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a10.vin3a_de0 */
	{GPMC_A11, (M2 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* gpmc_a11.vin3a_fld0 */
	{GPMC_A12, (M14 | PIN_INPUT_PULLUP)},	/* gpmc_a12.gpio2_2 */
	{GPMC_A13, (M14 | PIN_INPUT_PULLDOWN)},	/* gpmc_a13.gpio2_3 */
	{GPMC_A14, (M14 | PIN_INPUT_PULLUP)},	/* gpmc_a14.gpio2_4 */
	{GPMC_A15, (M14 | PIN_INPUT_PULLDOWN)},	/* gpmc_a15.gpio2_5 */
	{GPMC_A16, (M14 | PIN_INPUT_PULLDOWN)},	/* gpmc_a16.gpio2_6 */
	{GPMC_A17, (M14 | PIN_INPUT_PULLDOWN)},	/* gpmc_a17.gpio2_7 */
	{GPMC_A18, (M14 | PIN_INPUT_PULLUP)},	/* gpmc_a18.gpio2_8 */
	{GPMC_A19, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a19.mmc2_dat4 */
	{GPMC_A20, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a20.mmc2_dat5 */
	{GPMC_A21, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a21.mmc2_dat6 */
	{GPMC_A22, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a22.mmc2_dat7 */
	{GPMC_A23, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a23.mmc2_clk */
	{GPMC_A24, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a24.mmc2_dat0 */
	{GPMC_A25, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a25.mmc2_dat1 */
	{GPMC_A26, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a26.mmc2_dat2 */
	{GPMC_A27, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_a27.mmc2_dat3 */
	{GPMC_CS1, (M1 | PIN_INPUT_PULLUP)},	/* gpmc_cs1.mmc2_cmd */
	{GPMC_CS0, (M14 | PIN_INPUT_PULLDOWN)},	/* gpmc_cs0.gpio2_19 */
	{GPMC_CS2, (M14 | PIN_INPUT_PULLUP)},	/* gpmc_cs2.gpio2_20 */
	{GPMC_CS3, (M2 | PIN_INPUT_PULLDOWN)},	/* gpmc_cs3.vin3a_clk0 */
	{GPMC_CLK, (M9 | PIN_INPUT_PULLDOWN)},	/* gpmc_clk.dma_evt1 */
	{GPMC_ADVN_ALE, (M14 | PIN_INPUT_PULLUP)},	/* gpmc_advn_ale.gpio2_23 */
	{GPMC_OEN_REN, (M14 | PIN_INPUT_PULLUP)},	/* gpmc_oen_ren.gpio2_24 */
	{GPMC_WEN, (M14 | PIN_INPUT_PULLUP)},	/* gpmc_wen.gpio2_25 */
	{GPMC_BEN0, (M9 | PIN_INPUT_PULLDOWN)},	/* gpmc_ben0.dma_evt3 */
	{GPMC_BEN1, (M9 | PIN_INPUT_PULLDOWN)},	/* gpmc_ben1.dma_evt4 */
	{GPMC_WAIT0, (M14 | PIN_INPUT_PULLUP)},	/* gpmc_wait0.gpio2_28 */
	{VIN1B_CLK1, (M14 | PIN_INPUT_SLEW)},	/* vin1b_clk1.gpio2_31 */
	{VIN1A_D2, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d2.gpio3_6 */
	{VIN1A_D3, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d3.gpio3_7 */
	{VIN1A_D4, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d4.gpio3_8 */
	{VIN1A_D5, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d5.gpio3_9 */
	{VIN1A_D6, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d6.gpio3_10 */
	{VIN1A_D7, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d7.gpio3_11 */
	{VIN1A_D8, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d8.gpio3_12 */
	{VIN1A_D10, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d10.gpio3_14 */
	{VIN1A_D11, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d11.gpio3_15 */
	{VIN1A_D12, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d12.gpio3_16 */
	{VIN1A_D14, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d14.gpio3_18 */
	{VIN1A_D16, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d16.gpio3_20 */
	{VIN1A_D19, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d19.gpio3_23 */
	{VIN1A_D20, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d20.gpio3_24 */
	{VIN1A_D21, (M0 | PIN_INPUT_PULLDOWN)},	/* vin1a_d21.vin1a_d21 */
	{VIN1A_D22, (M14 | PIN_INPUT_PULLDOWN)},	/* vin1a_d22.gpio3_26 */
	{VIN2A_CLK0, (M14 | PIN_INPUT_PULLDOWN)},	/* vin2a_clk0.gpio3_28 */
	{VIN2A_DE0, (M14 | PIN_INPUT_PULLDOWN)},	/* vin2a_de0.gpio3_29 */
	{VIN2A_FLD0, (M14 | PIN_INPUT_PULLDOWN)},	/* vin2a_fld0.gpio3_30 */
	{VIN2A_HSYNC0, (M11 | PIN_INPUT_PULLDOWN)},	/* vin2a_hsync0.pr1_uart0_cts_n */
	{VIN2A_VSYNC0, (M11 | PIN_INPUT_PULLUP)},	/* vin2a_vsync0.pr1_uart0_rts_n */
	{VIN2A_D0, (M11 | PIN_INPUT_PULLDOWN)},	/* vin2a_d0.pr1_uart0_rxd */
	{VIN2A_D1, (M11 | PIN_INPUT_PULLDOWN)},	/* vin2a_d1.pr1_uart0_txd */
	{VIN2A_D2, (M8 | PIN_INPUT_PULLDOWN)},	/* vin2a_d2.uart10_rxd */
	{VIN2A_D3, (M8 | PIN_INPUT_PULLDOWN)},	/* vin2a_d3.uart10_txd */
	{VIN2A_D4, (M8 | PIN_INPUT_PULLDOWN)},	/* vin2a_d4.uart10_ctsn */
	{VIN2A_D5, (M8 | PIN_INPUT_PULLDOWN)},	/* vin2a_d5.uart10_rtsn */
	{VIN2A_D6, (M14 | PIN_INPUT_PULLDOWN)},	/* vin2a_d6.gpio4_7 */
	{VIN2A_D7, (M14 | PIN_INPUT_PULLDOWN)},	/* vin2a_d7.gpio4_8 */
	{VIN2A_D8, (M14 | PIN_INPUT_PULLDOWN)},	/* vin2a_d8.gpio4_9 */
	{VIN2A_D9, (M14 | PIN_INPUT_PULLDOWN)},	/* vin2a_d9.gpio4_10 */
	{VIN2A_D10, (M10 | PIN_INPUT_PULLDOWN)},	/* vin2a_d10.ehrpwm2B */
	{VIN2A_D11, (M10 | PIN_INPUT_PULLDOWN)},	/* vin2a_d11.ehrpwm2_tripzone_input */
	{VIN2A_D12, (M3 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* vin2a_d12.rgmii1_txc */
	{VIN2A_D13, (M3 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* vin2a_d13.rgmii1_txctl */
	{VIN2A_D14, (M3 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* vin2a_d14.rgmii1_txd3 */
	{VIN2A_D15, (M3 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* vin2a_d15.rgmii1_txd2 */
	{VIN2A_D16, (M3 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* vin2a_d16.rgmii1_txd1 */
	{VIN2A_D17, (M3 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* vin2a_d17.rgmii1_txd0 */
	{VIN2A_D18, (M3 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* vin2a_d18.rgmii1_rxc */
	{VIN2A_D19, (M3 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* vin2a_d19.rgmii1_rxctl */
	{VIN2A_D20, (M3 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* vin2a_d20.rgmii1_rxd3 */
	{VIN2A_D21, (M3 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* vin2a_d21.rgmii1_rxd2 */
	{VIN2A_D22, (M3 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* vin2a_d22.rgmii1_rxd1 */
	{VIN2A_D23, (M3 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* vin2a_d23.rgmii1_rxd0 */
	{VOUT1_CLK, (M0 | PIN_OUTPUT)},		/* vout1_clk.vout1_clk */
	{VOUT1_DE, (M0 | PIN_OUTPUT)},		/* vout1_de.vout1_de */
	{VOUT1_FLD, (M14 | PIN_INPUT)},		/* vout1_fld.gpio4_21 */
	{VOUT1_HSYNC, (M0 | PIN_OUTPUT)},	/* vout1_hsync.vout1_hsync */
	{VOUT1_VSYNC, (M0 | PIN_OUTPUT)},	/* vout1_vsync.vout1_vsync */
	{VOUT1_D0, (M0 | PIN_OUTPUT)},		/* vout1_d0.vout1_d0 */
	{VOUT1_D1, (M0 | PIN_OUTPUT)},		/* vout1_d1.vout1_d1 */
	{VOUT1_D2, (M0 | PIN_OUTPUT)},		/* vout1_d2.vout1_d2 */
	{VOUT1_D3, (M0 | PIN_OUTPUT)},		/* vout1_d3.vout1_d3 */
	{VOUT1_D4, (M0 | PIN_OUTPUT)},		/* vout1_d4.vout1_d4 */
	{VOUT1_D5, (M0 | PIN_OUTPUT)},		/* vout1_d5.vout1_d5 */
	{VOUT1_D6, (M0 | PIN_OUTPUT)},		/* vout1_d6.vout1_d6 */
	{VOUT1_D7, (M0 | PIN_OUTPUT)},		/* vout1_d7.vout1_d7 */
	{VOUT1_D8, (M0 | PIN_OUTPUT)},		/* vout1_d8.vout1_d8 */
	{VOUT1_D9, (M0 | PIN_OUTPUT)},		/* vout1_d9.vout1_d9 */
	{VOUT1_D10, (M0 | PIN_OUTPUT)},		/* vout1_d10.vout1_d10 */
	{VOUT1_D11, (M0 | PIN_OUTPUT)},		/* vout1_d11.vout1_d11 */
	{VOUT1_D12, (M0 | PIN_OUTPUT)},		/* vout1_d12.vout1_d12 */
	{VOUT1_D13, (M0 | PIN_OUTPUT)},		/* vout1_d13.vout1_d13 */
	{VOUT1_D14, (M0 | PIN_OUTPUT)},		/* vout1_d14.vout1_d14 */
	{VOUT1_D15, (M0 | PIN_OUTPUT)},		/* vout1_d15.vout1_d15 */
	{VOUT1_D16, (M0 | PIN_OUTPUT)},		/* vout1_d16.vout1_d16 */
	{VOUT1_D17, (M0 | PIN_OUTPUT)},		/* vout1_d17.vout1_d17 */
	{VOUT1_D18, (M0 | PIN_OUTPUT)},		/* vout1_d18.vout1_d18 */
	{VOUT1_D19, (M0 | PIN_OUTPUT)},		/* vout1_d19.vout1_d19 */
	{VOUT1_D20, (M0 | PIN_OUTPUT)},		/* vout1_d20.vout1_d20 */
	{VOUT1_D21, (M0 | PIN_OUTPUT)},		/* vout1_d21.vout1_d21 */
	{VOUT1_D22, (M0 | PIN_OUTPUT)},		/* vout1_d22.vout1_d22 */
	{VOUT1_D23, (M0 | PIN_OUTPUT)},		/* vout1_d23.vout1_d23 */
	{MDIO_MCLK, (M0 | PIN_INPUT_PULLUP)},	/* mdio_mclk.mdio_mclk */
	{MDIO_D, (M0 | PIN_INPUT_PULLUP)},	/* mdio_d.mdio_d */
	{RMII_MHZ_50_CLK, (M14 | PIN_INPUT_PULLUP)},	/* RMII_MHZ_50_CLK.gpio5_17 */
	{UART3_RXD, (M14 | PIN_INPUT_PULLDOWN)},	/* uart3_rxd.gpio5_18 */
	{UART3_TXD, (M14 | PIN_INPUT_PULLDOWN)},	/* uart3_txd.gpio5_19 */
	{RGMII0_TXC, (M0 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* rgmii0_txc.rgmii0_txc */
	{RGMII0_TXCTL, (M0 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* rgmii0_txctl.rgmii0_txctl */
	{RGMII0_TXD3, (M0 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* rgmii0_txd3.rgmii0_txd3 */
	{RGMII0_TXD2, (M0 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* rgmii0_txd2.rgmii0_txd2 */
	{RGMII0_TXD1, (M0 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* rgmii0_txd1.rgmii0_txd1 */
	{RGMII0_TXD0, (M0 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* rgmii0_txd0.rgmii0_txd0 */
	{RGMII0_RXC, (M0 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* rgmii0_rxc.rgmii0_rxc */
	{RGMII0_RXCTL, (M0 | PIN_INPUT_PULLDOWN | MANUAL_MODE)},	/* rgmii0_rxctl.rgmii0_rxctl */
	{RGMII0_RXD3, (M0 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* rgmii0_rxd3.rgmii0_rxd3 */
	{RGMII0_RXD2, (M0 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* rgmii0_rxd2.rgmii0_rxd2 */
	{RGMII0_RXD1, (M0 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* rgmii0_rxd1.rgmii0_rxd1 */
	{RGMII0_RXD0, (M0 | PIN_INPUT_PULLUP | MANUAL_MODE)},	/* rgmii0_rxd0.rgmii0_rxd0 */
	{USB1_DRVVBUS, (M0 | PIN_INPUT_SLEW)},	/* usb1_drvvbus.usb1_drvvbus */
	{USB2_DRVVBUS, (M0 | PIN_INPUT_SLEW)},	/* usb2_drvvbus.usb2_drvvbus */
	{GPIO6_14, (M10 | PIN_INPUT_PULLUP)},	/* gpio6_14.timer1 */
	{GPIO6_15, (M10 | PIN_INPUT_PULLUP)},	/* gpio6_15.timer2 */
	{GPIO6_16, (M10 | PIN_INPUT_PULLUP)},	/* gpio6_16.timer3 */
	{XREF_CLK0, (M9 | PIN_INPUT_PULLDOWN)},	/* xref_clk0.clkout2 */
	{XREF_CLK1, (M14 | PIN_INPUT_PULLDOWN)},	/* xref_clk1.gpio6_18 */
	{XREF_CLK2, (M14 | PIN_INPUT_PULLDOWN)},	/* xref_clk2.gpio6_19 */
	{XREF_CLK3, (M9 | PIN_INPUT_PULLDOWN)},	/* xref_clk3.clkout3 */
	{MCASP1_ACLKX, (M10 | PIN_INPUT_PULLUP)},	/* mcasp1_aclkx.i2c3_sda */
	{MCASP1_FSX, (M10 | PIN_INPUT_PULLUP)},	/* mcasp1_fsx.i2c3_scl */
	{MCASP1_ACLKR, (M10 | PIN_INPUT_PULLUP)},	/* mcasp1_aclkr.i2c4_sda */
	{MCASP1_FSR, (M10 | PIN_INPUT_PULLUP)},	/* mcasp1_fsr.i2c4_scl */
	{MCASP1_AXR0, (M10 | PIN_INPUT_PULLUP | SLEWCONTROL)},	/* mcasp1_axr0.i2c5_sda */
	{MCASP1_AXR1, (M10 | PIN_INPUT_PULLUP | SLEWCONTROL)},	/* mcasp1_axr1.i2c5_scl */
	{MCASP1_AXR2, (M14 | PIN_INPUT_PULLDOWN)},	/* mcasp1_axr2.gpio5_4 */
	{MCASP1_AXR3, (M14 | PIN_INPUT_PULLDOWN)},	/* mcasp1_axr3.gpio5_5 */
	{MCASP1_AXR4, (M14 | PIN_INPUT_PULLDOWN)},	/* mcasp1_axr4.gpio5_6 */
	{MCASP1_AXR5, (M14 | PIN_INPUT_PULLDOWN)},	/* mcasp1_axr5.gpio5_7 */
	{MCASP1_AXR6, (M14 | PIN_INPUT_PULLDOWN)},	/* mcasp1_axr6.gpio5_8 */
	{MCASP1_AXR7, (M14 | PIN_INPUT_PULLDOWN)},	/* mcasp1_axr7.gpio5_9 */
	{MCASP1_AXR8, (M14 | PIN_INPUT_SLEW)},	/* mcasp1_axr8.gpio5_10 */
	{MCASP1_AXR9, (M14 | PIN_INPUT_SLEW)},	/* mcasp1_axr9.gpio5_11 */
	{MCASP1_AXR10, (M14 | PIN_INPUT_SLEW)},	/* mcasp1_axr10.gpio5_12 */
	{MCASP1_AXR11, (M14 | PIN_INPUT_PULLUP | SLEWCONTROL)},	/* mcasp1_axr11.gpio4_17 */
	{MCASP1_AXR12, (M1 | PIN_INPUT_SLEW)},	/* mcasp1_axr12.mcasp7_axr0 */
	{MCASP1_AXR13, (M1 | PIN_INPUT_SLEW)},	/* mcasp1_axr13.mcasp7_axr1 */
	{MCASP1_AXR14, (M1 | PIN_INPUT_SLEW)},	/* mcasp1_axr14.mcasp7_aclkx */
	{MCASP1_AXR15, (M1 | PIN_INPUT_SLEW)},	/* mcasp1_axr15.mcasp7_fsx */
	{MCASP2_ACLKX, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_aclkx.mcasp2_aclkx */
	{MCASP2_FSX, (M0 | PIN_INPUT_SLEW)},	/* mcasp2_fsx.mcasp2_fsx */
	{MCASP2_ACLKR, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_aclkr.mcasp2_aclkr */
	{MCASP2_FSR, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_fsr.mcasp2_fsr */
	{MCASP2_AXR0, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_axr0.mcasp2_axr0 */
	{MCASP2_AXR1, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_axr1.mcasp2_axr1 */
	{MCASP2_AXR2, (M0 | PIN_INPUT_SLEW)},	/* mcasp2_axr2.mcasp2_axr2 */
	{MCASP2_AXR3, (M0 | PIN_INPUT_SLEW)},	/* mcasp2_axr3.mcasp2_axr3 */
	{MCASP2_AXR4, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_axr4.mcasp2_axr4 */
	{MCASP2_AXR5, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_axr5.mcasp2_axr5 */
	{MCASP2_AXR6, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_axr6.mcasp2_axr6 */
	{MCASP2_AXR7, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp2_axr7.mcasp2_axr7 */
	{MCASP3_ACLKX, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp3_aclkx.mcasp3_aclkx */
	{MCASP3_FSX, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp3_fsx.mcasp3_fsx */
	{MCASP3_AXR0, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp3_axr0.mcasp3_axr0 */
	{MCASP3_AXR1, (M0 | PIN_INPUT_PULLDOWN)},	/* mcasp3_axr1.mcasp3_axr1 */
	{MCASP4_ACLKX, (M3 | PIN_INPUT_PULLDOWN)},	/* mcasp4_aclkx.uart8_rxd */
	{MCASP4_FSX, (M3 | PIN_INPUT_PULLDOWN)},	/* mcasp4_fsx.uart8_txd */
	{MCASP4_AXR0, (M3 | PIN_INPUT_PULLDOWN)},	/* mcasp4_axr0.uart8_ctsn */
	{MCASP4_AXR1, (M3 | PIN_INPUT_PULLUP)},	/* mcasp4_axr1.uart8_rtsn */
	{MCASP5_ACLKX, (M3 | PIN_INPUT_PULLDOWN)},	/* mcasp5_aclkx.uart9_rxd */
	{MCASP5_FSX, (M3 | PIN_INPUT_PULLDOWN)},	/* mcasp5_fsx.uart9_txd */
	{MCASP5_AXR0, (M3 | PIN_INPUT_PULLDOWN)},	/* mcasp5_axr0.uart9_ctsn */
	{MCASP5_AXR1, (M3 | PIN_INPUT_PULLUP)},	/* mcasp5_axr1.uart9_rtsn */
	{MMC1_CLK, (M0 | PIN_INPUT_PULLUP)},	/* mmc1_clk.mmc1_clk */
	{MMC1_CMD, (M0 | PIN_INPUT_PULLUP)},	/* mmc1_cmd.mmc1_cmd */
	{MMC1_DAT0, (M0 | PIN_INPUT_PULLUP)},	/* mmc1_dat0.mmc1_dat0 */
	{MMC1_DAT1, (M0 | PIN_INPUT_PULLUP)},	/* mmc1_dat1.mmc1_dat1 */
	{MMC1_DAT2, (M0 | PIN_INPUT_PULLUP)},	/* mmc1_dat2.mmc1_dat2 */
	{MMC1_DAT3, (M0 | PIN_INPUT_PULLUP)},	/* mmc1_dat3.mmc1_dat3 */
	{MMC1_SDCD, (M0 | PIN_INPUT_PULLUP)},	/* mmc1_sdcd.mmc1_sdcd */
	{MMC1_SDWP, (M14 | PIN_OUTPUT)},	/* mmc1_sdwp.gpio6_28 */
	{GPIO6_10, (M10 | PIN_INPUT_PULLDOWN)},	/* gpio6_10.ehrpwm2A */
	{GPIO6_11, (M14 | PIN_INPUT_PULLUP)},	/* gpio6_11.gpio6_11 */
	{MMC3_CLK, (M0 | PIN_INPUT_PULLUP)},	/* mmc3_clk.mmc3_clk */
	{MMC3_CMD, (M0 | PIN_INPUT_PULLUP)},	/* mmc3_cmd.mmc3_cmd */
	{MMC3_DAT0, (M0 | PIN_INPUT_PULLUP)},	/* mmc3_dat0.mmc3_dat0 */
	{MMC3_DAT1, (M0 | PIN_INPUT_PULLUP)},	/* mmc3_dat1.mmc3_dat1 */
	{MMC3_DAT2, (M0 | PIN_INPUT_PULLUP)},	/* mmc3_dat2.mmc3_dat2 */
	{MMC3_DAT3, (M0 | PIN_INPUT_PULLUP)},	/* mmc3_dat3.mmc3_dat3 */
	{MMC3_DAT4, (M1 | PIN_INPUT_PULLDOWN)},	/* mmc3_dat4.spi4_sclk */
	{MMC3_DAT5, (M1 | PIN_INPUT_PULLDOWN)},	/* mmc3_dat5.spi4_d1 */
	{MMC3_DAT6, (M1 | PIN_INPUT_PULLDOWN)},	/* mmc3_dat6.spi4_d0 */
	{MMC3_DAT7, (M1 | PIN_INPUT_PULLUP)},	/* mmc3_dat7.spi4_cs0 */
	{SPI1_SCLK, (M14 | PIN_INPUT_PULLDOWN)},	/* spi1_sclk.gpio7_7 */
	{SPI1_D1, (M14 | PIN_INPUT_PULLDOWN)},	/* spi1_d1.gpio7_8 */
	{SPI1_D0, (M14 | PIN_INPUT_PULLDOWN)},	/* spi1_d0.gpio7_9 */
	{SPI1_CS0, (M14 | PIN_OUTPUT)},		/* spi1_cs0.gpio7_10 */
	{SPI1_CS1, (M14 | PIN_OUTPUT_PULLUP)},	/* spi1_cs1.gpio7_11 */
	{SPI1_CS2, (M14 | PIN_INPUT_PULLDOWN)},	/* spi1_cs2.gpio7_12 */
	{SPI1_CS3, (M6 | PIN_INPUT_PULLUP | SLEWCONTROL)},	/* spi1_cs3.hdmi1_cec */
	{SPI2_SCLK, (M14 | PIN_INPUT_PULLDOWN)},	/* spi2_sclk.gpio7_14 */
	{SPI2_D1, (M14 | PIN_INPUT_PULLDOWN)},	/* spi2_d1.gpio7_15 */
	{SPI2_D0, (M14 | PIN_INPUT_PULLUP)},	/* spi2_d0.gpio7_16 */
	{SPI2_CS0, (M14 | PIN_INPUT_PULLUP | SLEWCONTROL)},	/* spi2_cs0.gpio7_17 */
	{DCAN1_TX, (M15 | PULL_UP)},	/* dcan1_tx.safe for dcan1_tx */
	{DCAN1_RX, (M15 | PULL_UP)},	/* dcan1_rx.safe for dcan1_rx */
	{UART1_RXD, (M0 | PIN_INPUT_SLEW)},	/* uart1_rxd.uart1_rxd */
	{UART1_TXD, (M0 | PIN_INPUT_SLEW)},	/* uart1_txd.uart1_txd */
	{UART1_CTSN, (M15 | PIN_INPUT_PULLDOWN)},	/* uart1_ctsn.Driveroff */
	{UART2_RXD, (M15 | PIN_INPUT_PULLDOWN)},	/* N/A.Driveroff */
	{UART2_TXD, (M15 | PIN_INPUT_PULLDOWN)},	/* uart2_txd.Driveroff */
	{UART2_CTSN, (M2 | PIN_INPUT_SLEW)},	/* uart2_ctsn.uart3_rxd */
	{UART2_RTSN, (M1 | PIN_INPUT_SLEW)},	/* uart2_rtsn.uart3_txd */
	{I2C2_SDA, (M1 | PIN_INPUT)},		/* i2c2_sda.hdmi1_ddc_scl */
	{I2C2_SCL, (M1 | PIN_INPUT)},		/* i2c2_scl.hdmi1_ddc_sda */
	{WAKEUP0, (M0 | PULL_UP)},		/* Wakeup0.Wakeup0 */
	{WAKEUP1, (M0)},			/* Wakeup1.Wakeup1 */
	{WAKEUP2, (M0)},			/* Wakeup2.Wakeup2 */
	{WAKEUP3, (M0 | PULL_UP)},		/* Wakeup3.Wakeup3 */
	{ON_OFF, (M1 | PIN_OUTPUT_PULLUP)},	/* on_off.on_off */
	{RTC_PORZ, (M0 | PIN_OUTPUT_PULLDOWN)},	/* rtc_porz.rtc_porz */
	{RTCK, (M0 | PIN_INPUT_PULLDOWN)},	/* rtck.rtck */
};

const struct pad_conf_entry early_padconf[] = {
	{UART2_CTSN, (M2 | PIN_INPUT_SLEW)},	/* uart2_ctsn.uart3_rxd */
	{UART2_RTSN, (M1 | PIN_INPUT_SLEW)},	/* uart2_rtsn.uart3_txd */
	{I2C1_SDA, (PIN_INPUT_PULLUP | M0)},	/* I2C1_SDA */
	{I2C1_SCL, (PIN_INPUT_PULLUP | M0)},	/* I2C1_SCL */
};

#ifdef CONFIG_IODELAY_RECALIBRATION
const struct iodelay_cfg_entry iodelay_cfg_array[] = {
	{0x0114, 2980, 0},	/* CFG_GPMC_A0_IN */
	{0x0120, 2648, 0},	/* CFG_GPMC_A10_IN */
	{0x012C, 2918, 0},	/* CFG_GPMC_A11_IN */
	{0x0198, 2917, 0},	/* CFG_GPMC_A1_IN */
	{0x0204, 3156, 178},	/* CFG_GPMC_A2_IN */
	{0x0210, 3109, 246},	/* CFG_GPMC_A3_IN */
	{0x021C, 3142, 100},	/* CFG_GPMC_A4_IN */
	{0x0228, 3084, 33},	/* CFG_GPMC_A5_IN */
	{0x0234, 2778, 0},	/* CFG_GPMC_A6_IN */
	{0x0240, 3110, 0},	/* CFG_GPMC_A7_IN */
	{0x024C, 2874, 0},	/* CFG_GPMC_A8_IN */
	{0x0258, 3072, 0},	/* CFG_GPMC_A9_IN */
	{0x0264, 2466, 0},	/* CFG_GPMC_AD0_IN */
	{0x0270, 2523, 0},	/* CFG_GPMC_AD10_IN */
	{0x027C, 2453, 0},	/* CFG_GPMC_AD11_IN */
	{0x0288, 2285, 0},	/* CFG_GPMC_AD12_IN */
	{0x0294, 2206, 0},	/* CFG_GPMC_AD13_IN */
	{0x02A0, 1898, 0},	/* CFG_GPMC_AD14_IN */
	{0x02AC, 2473, 0},	/* CFG_GPMC_AD15_IN */
	{0x02B8, 2307, 0},	/* CFG_GPMC_AD1_IN */
	{0x02C4, 2691, 0},	/* CFG_GPMC_AD2_IN */
	{0x02D0, 2384, 0},	/* CFG_GPMC_AD3_IN */
	{0x02DC, 2462, 0},	/* CFG_GPMC_AD4_IN */
	{0x02E8, 2335, 0},	/* CFG_GPMC_AD5_IN */
	{0x02F4, 2370, 0},	/* CFG_GPMC_AD6_IN */
	{0x0300, 2389, 0},	/* CFG_GPMC_AD7_IN */
	{0x030C, 2672, 0},	/* CFG_GPMC_AD8_IN */
	{0x0318, 2334, 0},	/* CFG_GPMC_AD9_IN */
	{0x06F0, 480, 0},	/* CFG_RGMII0_RXC_IN */
	{0x06FC, 111, 1641},	/* CFG_RGMII0_RXCTL_IN */
	{0x0708, 272, 1116},	/* CFG_RGMII0_RXD0_IN */
	{0x0714, 243, 1260},	/* CFG_RGMII0_RXD1_IN */
	{0x0720, 0, 1614},	/* CFG_RGMII0_RXD2_IN */
	{0x072C, 105, 1673},	/* CFG_RGMII0_RXD3_IN */
	{0x0740, 531, 120},	/* CFG_RGMII0_TXC_OUT */
	{0x074C, 11, 60},	/* CFG_RGMII0_TXCTL_OUT */
	{0x0758, 7, 120},	/* CFG_RGMII0_TXD0_OUT */
	{0x0764, 0, 0},		/* CFG_RGMII0_TXD1_OUT */
	{0x0770, 276, 120},	/* CFG_RGMII0_TXD2_OUT */
	{0x077C, 440, 120},	/* CFG_RGMII0_TXD3_OUT */
	{0x0A70, 1551, 115},	/* CFG_VIN2A_D12_OUT */
	{0x0A7C, 816, 0},	/* CFG_VIN2A_D13_OUT */
	{0x0A88, 876, 0},	/* CFG_VIN2A_D14_OUT */
	{0x0A94, 312, 0},	/* CFG_VIN2A_D15_OUT */
	{0x0AA0, 58, 0},	/* CFG_VIN2A_D16_OUT */
	{0x0AAC, 0, 0},		/* CFG_VIN2A_D17_OUT */
	{0x0AB0, 702, 0},	/* CFG_VIN2A_D18_IN */
	{0x0ABC, 136, 976},	/* CFG_VIN2A_D19_IN */
	{0x0AD4, 210, 1357},	/* CFG_VIN2A_D20_IN */
	{0x0AE0, 189, 1462},	/* CFG_VIN2A_D21_IN */
	{0x0AEC, 232, 1278},	/* CFG_VIN2A_D22_IN */
	{0x0AF8, 0, 1397},	/* CFG_VIN2A_D23_IN */
};
#endif
#endif /* _MUX_DATA_BEAGLE_X15_H_ */
