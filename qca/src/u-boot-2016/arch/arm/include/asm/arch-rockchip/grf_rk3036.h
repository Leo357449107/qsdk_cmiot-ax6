/*
 * (C) Copyright 2015 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#ifndef _ASM_ARCH_GRF_RK3036_H
#define _ASM_ARCH_GRF_RK3036_H

#include <common.h>

struct rk3036_grf {
	unsigned int reserved[0x2a];
	unsigned int gpio0a_iomux;
	unsigned int gpio0b_iomux;
	unsigned int gpio0c_iomux;
	unsigned int gpio0d_iomux;

	unsigned int gpio1a_iomux;
	unsigned int gpio1b_iomux;
	unsigned int gpio1c_iomux;
	unsigned int gpio1d_iomux;

	unsigned int gpio2a_iomux;
	unsigned int gpio2b_iomux;
	unsigned int gpio2c_iomux;
	unsigned int gpio2d_iomux;

	unsigned int reserved2[0x0a];
	unsigned int gpiods;
	unsigned int reserved3[0x05];
	unsigned int gpio0l_pull;
	unsigned int gpio0h_pull;
	unsigned int gpio1l_pull;
	unsigned int gpio1h_pull;
	unsigned int gpio2l_pull;
	unsigned int gpio2h_pull;
	unsigned int reserved4[4];
	unsigned int soc_con0;
	unsigned int soc_con1;
	unsigned int soc_con2;
	unsigned int soc_status0;
	unsigned int reserved5;
	unsigned int soc_con3;
	unsigned int reserved6;
	unsigned int dmac_con0;
	unsigned int dmac_con1;
	unsigned int dmac_con2;
	unsigned int reserved7[5];
	unsigned int uoc0_con5;
	unsigned int reserved8[4];
	unsigned int uoc1_con4;
	unsigned int uoc1_con5;
	unsigned int reserved9;
	unsigned int ddrc_stat;
	unsigned int uoc_con6;
	unsigned int soc_status1;
	unsigned int cpu_con0;
	unsigned int cpu_con1;
	unsigned int cpu_con2;
	unsigned int cpu_con3;
	unsigned int reserved10;
	unsigned int reserved11;
	unsigned int cpu_status0;
	unsigned int cpu_status1;
	unsigned int os_reg[8];
	unsigned int reserved12[6];
	unsigned int dll_con[4];
	unsigned int dll_status[4];
	unsigned int dfi_wrnum;
	unsigned int dfi_rdnum;
	unsigned int dfi_actnum;
	unsigned int dfi_timerval;
	unsigned int nfi_fifo[4];
	unsigned int reserved13[0x10];
	unsigned int usbphy0_con[8];
	unsigned int usbphy1_con[8];
	unsigned int reserved14[0x10];
	unsigned int chip_tag;
	unsigned int sdmmc_det_cnt;
};
check_member(rk3036_grf, sdmmc_det_cnt, 0x304);

/* GRF_GPIO0A_IOMUX */
enum {
	GPIO0A3_SHIFT		= 6,
	GPIO0A3_MASK		= 1,
	GPIO0A3_GPIO		= 0,
	GPIO0A3_I2C1_SDA,

	GPIO0A2_SHIFT		= 4,
	GPIO0A2_MASK		= 1,
	GPIO0A2_GPIO		= 0,
	GPIO0A2_I2C1_SCL,

	GPIO0A1_SHIFT		= 2,
	GPIO0A1_MASK		= 3,
	GPIO0A1_GPIO		= 0,
	GPIO0A1_I2C0_SDA,
	GPIO0A1_PWM2,

	GPIO0A0_SHIFT		= 0,
	GPIO0A0_MASK		= 3,
	GPIO0A0_GPIO		= 0,
	GPIO0A0_I2C0_SCL,
	GPIO0A0_PWM1,

};

/* GRF_GPIO0B_IOMUX */
enum {
	GPIO0B6_SHIFT		= 12,
	GPIO0B6_MASK		= 3,
	GPIO0B6_GPIO		= 0,
	GPIO0B6_MMC1_D3,
	GPIO0B6_I2S1_SCLK,

	GPIO0B5_SHIFT		= 10,
	GPIO0B5_MASK		= 3,
	GPIO0B5_GPIO		= 0,
	GPIO0B5_MMC1_D2,
	GPIO0B5_I2S1_SDI,

	GPIO0B4_SHIFT		= 8,
	GPIO0B4_MASK		= 3,
	GPIO0B4_GPIO		= 0,
	GPIO0B4_MMC1_D1,
	GPIO0B4_I2S1_LRCKTX,

	GPIO0B3_SHIFT		= 6,
	GPIO0B3_MASK		= 3,
	GPIO0B3_GPIO		= 0,
	GPIO0B3_MMC1_D0,
	GPIO0B3_I2S1_LRCKRX,

	GPIO0B1_SHIFT		= 2,
	GPIO0B1_MASK		= 3,
	GPIO0B1_GPIO		= 0,
	GPIO0B1_MMC1_CLKOUT,
	GPIO0B1_I2S1_MCLK,

	GPIO0B0_SHIFT		= 0,
	GPIO0B0_MASK		= 3,
	GPIO0B0_GPIO		= 0,
	GPIO0B0_MMC1_CMD,
	GPIO0B0_I2S1_SDO,
};

/* GRF_GPIO0C_IOMUX */
enum {
	GPIO0C4_SHIFT		= 8,
	GPIO0C4_MASK		= 1,
	GPIO0C4_GPIO		= 0,
	GPIO0C4_DRIVE_VBUS,

	GPIO0C3_SHIFT		= 6,
	GPIO0C3_MASK		= 1,
	GPIO0C3_GPIO		= 0,
	GPIO0C3_UART0_CTSN,

	GPIO0C2_SHIFT		= 4,
	GPIO0C2_MASK		= 1,
	GPIO0C2_GPIO		= 0,
	GPIO0C2_UART0_RTSN,

	GPIO0C1_SHIFT		= 2,
	GPIO0C1_MASK		= 1,
	GPIO0C1_GPIO		= 0,
	GPIO0C1_UART0_SIN,


	GPIO0C0_SHIFT		= 0,
	GPIO0C0_MASK		= 1,
	GPIO0C0_GPIO		= 0,
	GPIO0C0_UART0_SOUT,
};

/* GRF_GPIO0D_IOMUX */
enum {
	GPIO0D4_SHIFT		= 8,
	GPIO0D4_MASK		= 1,
	GPIO0D4_GPIO		= 0,
	GPIO0D4_SPDIF,

	GPIO0D3_SHIFT		= 6,
	GPIO0D3_MASK		= 1,
	GPIO0D3_GPIO		= 0,
	GPIO0D3_PWM3,

	GPIO0D2_SHIFT		= 4,
	GPIO0D2_MASK		= 1,
	GPIO0D2_GPIO		= 0,
	GPIO0D2_PWM0,
};

/* GRF_GPIO1A_IOMUX */
enum {
	GPIO1A5_SHIFT		= 10,
	GPIO1A5_MASK		= 1,
	GPIO1A5_GPIO		= 0,
	GPIO1A5_I2S_SDI,

	GPIO1A4_SHIFT		= 8,
	GPIO1A4_MASK		= 1,
	GPIO1A4_GPIO		= 0,
	GPIO1A4_I2S_SD0,

	GPIO1A3_SHIFT		= 6,
	GPIO1A3_MASK		= 1,
	GPIO1A3_GPIO		= 0,
	GPIO1A3_I2S_LRCKTX,

	GPIO1A2_SHIFT		= 4,
	GPIO1A2_MASK		= 6,
	GPIO1A2_GPIO		= 0,
	GPIO1A2_I2S_LRCKRX,
	GPIO1A2_I2S_PWM1_0,

	GPIO1A1_SHIFT		= 2,
	GPIO1A1_MASK		= 1,
	GPIO1A1_GPIO		= 0,
	GPIO1A1_I2S_SCLK,

	GPIO1A0_SHIFT		= 0,
	GPIO1A0_MASK		= 1,
	GPIO1A0_GPIO		= 0,
	GPIO1A0_I2S_MCLK,

};

/* GRF_GPIO1B_IOMUX */
enum {
	GPIO1B7_SHIFT		= 14,
	GPIO1B7_MASK		= 1,
	GPIO1B7_GPIO		= 0,
	GPIO1B7_MMC0_CMD,

	GPIO1B3_SHIFT		= 6,
	GPIO1B3_MASK		= 1,
	GPIO1B3_GPIO		= 0,
	GPIO1B3_HDMI_HPD,

	GPIO1B2_SHIFT		= 4,
	GPIO1B2_MASK		= 1,
	GPIO1B2_GPIO		= 0,
	GPIO1B2_HDMI_SCL,

	GPIO1B1_SHIFT		= 2,
	GPIO1B1_MASK		= 1,
	GPIO1B1_GPIO		= 0,
	GPIO1B1_HDMI_SDA,

	GPIO1B0_SHIFT		= 0,
	GPIO1B0_MASK		= 1,
	GPIO1B0_GPIO		= 0,
	GPIO1B0_HDMI_CEC,
};

/* GRF_GPIO1C_IOMUX */
enum {
	GPIO1C5_SHIFT		= 10,
	GPIO1C5_MASK		= 3,
	GPIO1C5_GPIO		= 0,
	GPIO1C5_MMC0_D3,
	GPIO1C5_JTAG_TMS,

	GPIO1C4_SHIFT		= 8,
	GPIO1C4_MASK		= 3,
	GPIO1C4_GPIO		= 0,
	GPIO1C4_MMC0_D2,
	GPIO1C4_JTAG_TCK,

	GPIO1C3_SHIFT		= 6,
	GPIO1C3_MASK		= 3,
	GPIO1C3_GPIO		= 0,
	GPIO1C3_MMC0_D1,
	GPIO1C3_UART2_SOUT,

	GPIO1C2_SHIFT		= 4,
	GPIO1C2_MASK		= 3,
	GPIO1C2_GPIO		= 0,
	GPIO1C2_MMC0_D0,
	GPIO1C2_UART2_SIN,

	GPIO1C1_SHIFT		= 2,
	GPIO1C1_MASK		= 1,
	GPIO1C1_GPIO		= 0,
	GPIO1C1_MMC0_DETN,

	GPIO1C0_SHIFT		= 0,
	GPIO1C0_MASK		= 1,
	GPIO1C0_GPIO		= 0,
	GPIO1C0_MMC0_CLKOUT,
};

/* GRF_GPIO1D_IOMUX */
enum {
	GPIO1D7_SHIFT		= 14,
	GPIO1D7_MASK		= 3,
	GPIO1D7_GPIO		= 0,
	GPIO1D7_NAND_D7,
	GPIO1D7_EMMC_D7,
	GPIO1D7_SPI_CSN1,

	GPIO1D6_SHIFT		= 12,
	GPIO1D6_MASK		= 3,
	GPIO1D6_GPIO		= 0,
	GPIO1D6_NAND_D6,
	GPIO1D6_EMMC_D6,
	GPIO1D6_SPI_CSN0,

	GPIO1D5_SHIFT		= 10,
	GPIO1D5_MASK		= 3,
	GPIO1D5_GPIO		= 0,
	GPIO1D5_NAND_D5,
	GPIO1D5_EMMC_D5,
	GPIO1D5_SPI_TXD,

	GPIO1D4_SHIFT		= 8,
	GPIO1D4_MASK		= 3,
	GPIO1D4_GPIO		= 0,
	GPIO1D4_NAND_D4,
	GPIO1D4_EMMC_D4,
	GPIO1D4_SPI_RXD,

	GPIO1D3_SHIFT		= 6,
	GPIO1D3_MASK		= 3,
	GPIO1D3_GPIO		= 0,
	GPIO1D3_NAND_D3,
	GPIO1D3_EMMC_D3,
	GPIO1D3_SFC_SIO3,

	GPIO1D2_SHIFT		= 4,
	GPIO1D2_MASK		= 3,
	GPIO1D2_GPIO		= 0,
	GPIO1D2_NAND_D2,
	GPIO1D2_EMMC_D2,
	GPIO1D2_SFC_SIO2,

	GPIO1D1_SHIFT		= 2,
	GPIO1D1_MASK		= 3,
	GPIO1D1_GPIO		= 0,
	GPIO1D1_NAND_D1,
	GPIO1D1_EMMC_D1,
	GPIO1D1_SFC_SIO1,

	GPIO1D0_SHIFT		= 0,
	GPIO1D0_MASK		= 3,
	GPIO1D0_GPIO		= 0,
	GPIO1D0_NAND_D0,
	GPIO1D0_EMMC_D0,
	GPIO1D0_SFC_SIO0,
};

/* GRF_GPIO2A_IOMUX */
enum {
	GPIO2A7_SHIFT		= 14,
	GPIO2A7_MASK		= 1,
	GPIO2A7_GPIO		= 0,
	GPIO2A7_TESTCLK_OUT,

	GPIO2A6_SHIFT		= 12,
	GPIO2A6_MASK		= 1,
	GPIO2A6_GPIO		= 0,
	GPIO2A6_NAND_CS0,

	GPIO2A4_SHIFT		= 8,
	GPIO2A4_MASK		= 3,
	GPIO2A4_GPIO		= 0,
	GPIO2A4_NAND_RDY,
	GPIO2A4_EMMC_CMD,
	GPIO2A3_SFC_CLK,

	GPIO2A3_SHIFT		= 6,
	GPIO2A3_MASK		= 3,
	GPIO2A3_GPIO		= 0,
	GPIO2A3_NAND_RDN,
	GPIO2A4_SFC_CSN1,

	GPIO2A2_SHIFT		= 4,
	GPIO2A2_MASK		= 3,
	GPIO2A2_GPIO		= 0,
	GPIO2A2_NAND_WRN,
	GPIO2A4_SFC_CSN0,

	GPIO2A1_SHIFT		= 2,
	GPIO2A1_MASK		= 3,
	GPIO2A1_GPIO		= 0,
	GPIO2A1_NAND_CLE,
	GPIO2A1_EMMC_CLKOUT,

	GPIO2A0_SHIFT		= 0,
	GPIO2A0_MASK		= 3,
	GPIO2A0_GPIO		= 0,
	GPIO2A0_NAND_ALE,
	GPIO2A0_SPI_CLK,
};

/* GRF_GPIO2B_IOMUX */
enum {
	GPIO2B7_SHIFT		= 14,
	GPIO2B7_MASK		= 1,
	GPIO2B7_GPIO		= 0,
	GPIO2B7_MAC_RXER,

	GPIO2B6_SHIFT		= 12,
	GPIO2B6_MASK		= 3,
	GPIO2B6_GPIO		= 0,
	GPIO2B6_MAC_CLKOUT,
	GPIO2B6_MAC_CLKIN,

	GPIO2B5_SHIFT		= 10,
	GPIO2B5_MASK		= 1,
	GPIO2B5_GPIO		= 0,
	GPIO2B5_MAC_TXEN,

	GPIO2B4_SHIFT		= 8,
	GPIO2B4_MASK		= 1,
	GPIO2B4_GPIO		= 0,
	GPIO2B4_MAC_MDIO,

	GPIO2B2_SHIFT		= 4,
	GPIO2B2_MASK		= 1,
	GPIO2B2_GPIO		= 0,
	GPIO2B2_MAC_CRS,
};

/* GRF_GPIO2C_IOMUX */
enum {
	GPIO2C7_SHIFT		= 14,
	GPIO2C7_MASK		= 3,
	GPIO2C7_GPIO		= 0,
	GPIO2C7_UART1_SOUT,
	GPIO2C7_TESTCLK_OUT1,

	GPIO2C6_SHIFT		= 12,
	GPIO2C6_MASK		= 1,
	GPIO2C6_GPIO		= 0,
	GPIO2C6_UART1_SIN,

	GPIO2C5_SHIFT		= 10,
	GPIO2C5_MASK		= 1,
	GPIO2C5_GPIO		= 0,
	GPIO2C5_I2C2_SCL,

	GPIO2C4_SHIFT		= 8,
	GPIO2C4_MASK		= 1,
	GPIO2C4_GPIO		= 0,
	GPIO2C4_I2C2_SDA,

	GPIO2C3_SHIFT		= 6,
	GPIO2C3_MASK		= 1,
	GPIO2C3_GPIO		= 0,
	GPIO2C3_MAC_TXD0,

	GPIO2C2_SHIFT		= 4,
	GPIO2C2_MASK		= 1,
	GPIO2C2_GPIO		= 0,
	GPIO2C2_MAC_TXD1,

	GPIO2C1_SHIFT		= 2,
	GPIO2C1_MASK		= 1,
	GPIO2C1_GPIO		= 0,
	GPIO2C1_MAC_RXD0,

	GPIO2C0_SHIFT		= 0,
	GPIO2C0_MASK		= 1,
	GPIO2C0_GPIO		= 0,
	GPIO2C0_MAC_RXD1,
};

/* GRF_GPIO2D_IOMUX */
enum {
	GPIO2D6_SHIFT		= 12,
	GPIO2D6_MASK		= 1,
	GPIO2D6_GPIO		= 0,
	GPIO2D6_I2S_SDO1,

	GPIO2D5_SHIFT		= 10,
	GPIO2D5_MASK		= 1,
	GPIO2D5_GPIO		= 0,
	GPIO2D5_I2S_SDO2,

	GPIO2D4_SHIFT		= 8,
	GPIO2D4_MASK		= 1,
	GPIO2D4_GPIO		= 0,
	GPIO2D4_I2S_SDO3,

	GPIO2D1_SHIFT		= 2,
	GPIO2D1_MASK		= 1,
	GPIO2D1_GPIO		= 0,
	GPIO2D1_MAC_MDC,
};
#endif
