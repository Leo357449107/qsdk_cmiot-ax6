/*
 * Copyright (c) 2017-2019, 2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <asm/global_data.h>
#include <net.h>
#include <asm-generic/errno.h>
#include <asm/io.h>
#include <malloc.h>
#include <phy.h>
#include <net.h>
#include <miiphy.h>
#include <asm/arch-ipq9574/edma_regs.h>
#include "ipq9574_edma.h"
#include "ipq9574_uniphy.h"
#include "ipq9574_ppe.h"
#include <fdtdec.h>
#include "ipq_phy.h"

extern int is_uniphy_enabled(int uniphy_index);

DECLARE_GLOBAL_DATA_PTR;
extern int ipq_mdio_write(int mii_id,
		int regnum, u16 value);
extern int ipq_mdio_read(int mii_id,
		int regnum, ushort *data);
extern void ipq9574_qca8075_phy_serdes_reset(u32 phy_id);

void csr1_write(int phy_id, int addr, int  value)
{
	int  addr_h, addr_l, ahb_h, ahb_l,  phy;
	phy=phy_id<<(0x10);
	addr_h=(addr&0xffffff)>>8;
	addr_l=((addr&0xff)<<2)|(0x20<<(0xa));
	ahb_l=(addr_l&0xffff)|(0x7A00000|phy);
	ahb_h=(0x7A083FC|phy);
	writel(addr_h,ahb_h);
	writel(value,ahb_l);
}

int  csr1_read(int phy_id, int  addr )
{
	int  addr_h ,addr_l,ahb_h, ahb_l, phy;
	phy=phy_id<<(0x10);
	addr_h=(addr&0xffffff)>>8;
	addr_l=((addr&0xff)<<2)|(0x20<<(0xa));
	ahb_l=(addr_l&0xffff)|(0x7A00000|phy);
	ahb_h=(0x7A083FC|phy);
	writel(addr_h, ahb_h);
	return  readl(ahb_l);
}

static int ppe_uniphy_calibration(uint32_t uniphy_index)
{
	int retries = 100, calibration_done = 0;
	uint32_t reg_value = 0;

	while(calibration_done != UNIPHY_CALIBRATION_DONE) {
		mdelay(1);
		if (retries-- == 0) {
			printf("uniphy callibration time out!\n");
			return -1;
		}
		reg_value = readl(PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
			+ PPE_UNIPHY_OFFSET_CALIB_4);
		calibration_done = (reg_value >> 0x7) & 0x1;
	}

	return 0;
}

static void ppe_uniphy_reset(enum uniphy_reset_type rst_type, bool enable)
{
	uint32_t mode, node;
	uint32_t reg_val, reg_val1;

	switch(rst_type) {
		case UNIPHY0_SOFT_RESET:
			node = fdt_path_offset(gd->fdt_blob, "/ess-switch");
			if (node < 0) {
				printf("\nError: ess-switch not specified in dts");
				return;
			}
			mode = fdtdec_get_uint(gd->fdt_blob, node, "switch_mac_mode1", -1);
			if (mode < 0) {
				printf("\nError: switch_mac_mode1 not specified in dts");
				return;
			}
			reg_val = readl(GCC_UNIPHY0_MISC);
			reg_val1 = readl(NSS_CC_UNIPHY_MISC_RESET);
			if (mode == EPORT_WRAPPER_MAX) {
				if (enable) {
					reg_val |= 0x1;
					reg_val1 |= 0xffc000;
				} else {
					reg_val &= ~0x1;
					reg_val1 &= ~0xffc000;
				}
			} else {
				if (enable) {
					reg_val |= 0x1;
					reg_val1 |= 0xff0000;
				} else {
					reg_val &= ~0x1;
					reg_val1 &= ~0xff0000;
				}
			}
			writel(reg_val, GCC_UNIPHY0_MISC);
			writel(reg_val1, NSS_CC_UNIPHY_MISC_RESET);
			break;
		case UNIPHY0_XPCS_RESET:
			reg_val = readl(GCC_UNIPHY0_MISC);
			if (enable)
				reg_val |= 0x4;
			else
				reg_val &= ~0x4;
			writel(reg_val, GCC_UNIPHY0_MISC);
			break;
		case UNIPHY1_SOFT_RESET:
			reg_val = readl(GCC_UNIPHY0_MISC + GCC_UNIPHY_REG_INC);
			reg_val1 = readl(NSS_CC_UNIPHY_MISC_RESET);
			if (enable) {
				reg_val |= 0x1;
				reg_val1 |= 0xC000;
			} else {
				reg_val &= ~0x1;
				reg_val1 &= ~0xC000;
			}
			writel(reg_val, GCC_UNIPHY0_MISC + GCC_UNIPHY_REG_INC);
			writel(reg_val1, NSS_CC_UNIPHY_MISC_RESET);
			break;
		case UNIPHY1_XPCS_RESET:
			reg_val = readl(GCC_UNIPHY0_MISC + GCC_UNIPHY_REG_INC);
			if (enable)
				reg_val |= 0x4;
			else
				reg_val &= ~0x4;
			writel(reg_val, GCC_UNIPHY0_MISC + GCC_UNIPHY_REG_INC);
			break;
		case UNIPHY2_SOFT_RESET:
			reg_val = readl(GCC_UNIPHY0_MISC + (2 * GCC_UNIPHY_REG_INC));
			reg_val1 = readl(NSS_CC_UNIPHY_MISC_RESET);
			if (enable) {
				reg_val |= 0x1;
				reg_val1 |= 0x3000;
			} else {
				reg_val &= ~0x1;
				reg_val1 &= ~0x3000;
			}
			writel(reg_val, GCC_UNIPHY0_MISC + (2 * GCC_UNIPHY_REG_INC));
			writel(reg_val1, NSS_CC_UNIPHY_MISC_RESET);
			break;
		case UNIPHY2_XPCS_RESET:
			reg_val = readl(GCC_UNIPHY0_MISC + (2 * GCC_UNIPHY_REG_INC));
			if (enable)
				reg_val |= 0x4;
			else
				reg_val &= ~0x4;
			writel(reg_val, GCC_UNIPHY0_MISC + (2 * GCC_UNIPHY_REG_INC));
			break;
		default:
			break;
	}
}

static void ppe_uniphy_psgmii_mode_set(uint32_t uniphy_index)
{
	if (uniphy_index == 0)
		ppe_uniphy_reset(UNIPHY0_XPCS_RESET, true);
	else if (uniphy_index == 1)
		ppe_uniphy_reset(UNIPHY1_XPCS_RESET, true);
	else
		ppe_uniphy_reset(UNIPHY2_XPCS_RESET, true);
	mdelay(100);

	writel(0x220, PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
			+ PPE_UNIPHY_MODE_CONTROL);

	if (uniphy_index == 0) {
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, false);
	} else if (uniphy_index == 1) {
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, false);
	} else {
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, false);
	}
	mdelay(100);
	ppe_uniphy_calibration(uniphy_index);
	ipq9574_qca8075_phy_serdes_reset(0x10);
}

static void ppe_uniphy_qsgmii_mode_set(uint32_t uniphy_index)
{
	if (uniphy_index == 0)
		ppe_uniphy_reset(UNIPHY0_XPCS_RESET, true);
	else if (uniphy_index == 1)
		ppe_uniphy_reset(UNIPHY1_XPCS_RESET, true);
	else
		ppe_uniphy_reset(UNIPHY2_XPCS_RESET, true);
	mdelay(100);

	writel(0x120, PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
			+ PPE_UNIPHY_MODE_CONTROL);
	if (uniphy_index == 0) {
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, false);
	} else if (uniphy_index == 1) {
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, false);
	} else {
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, false);
	}
	mdelay(100);
}

static void ppe_uniphy_sgmii_mode_set(uint32_t uniphy_index, uint32_t mode)
{
	writel(UNIPHY_MISC2_REG_SGMII_MODE, PPE_UNIPHY_BASE +
		(uniphy_index * PPE_UNIPHY_REG_INC) + UNIPHY_MISC2_REG_OFFSET);

	writel(UNIPHY_PLL_RESET_REG_VALUE, PPE_UNIPHY_BASE +
		(uniphy_index * PPE_UNIPHY_REG_INC) + UNIPHY_PLL_RESET_REG_OFFSET);
	mdelay(500);
	writel(UNIPHY_PLL_RESET_REG_DEFAULT_VALUE, PPE_UNIPHY_BASE +
		(uniphy_index * PPE_UNIPHY_REG_INC) + UNIPHY_PLL_RESET_REG_OFFSET);
	mdelay(500);
	if (uniphy_index == 0)
		ppe_uniphy_reset(UNIPHY0_XPCS_RESET, true);
	else if (uniphy_index == 1)
		ppe_uniphy_reset(UNIPHY1_XPCS_RESET, true);
	else
		ppe_uniphy_reset(UNIPHY2_XPCS_RESET, true);
	mdelay(100);

	if (uniphy_index == 1) {
		writel(0x0, NSS_CC_UNIPHY_PORT1_RX_CBCR + (PORT5 - 1) * 0x8);
		writel(0x0, NSS_CC_UNIPHY_PORT1_RX_CBCR + 0x4 + (PORT5 - 1) * 0x8);
		writel(0x0, NSS_CC_PORT1_RX_CBCR + (PORT5 - 1) * 0x8);
		writel(0x0, NSS_CC_PORT1_RX_CBCR + 0x4 + (PORT5 - 1) * 0x8);
	} else if (uniphy_index == 2) {
		writel(0x0, NSS_CC_UNIPHY_PORT1_RX_CBCR + (PORT6 - 1) * 0x8);
		writel(0x0, NSS_CC_UNIPHY_PORT1_RX_CBCR + 0x4 + (PORT6 - 1) * 8);
		writel(0x0, NSS_CC_PORT1_RX_CBCR + (PORT6 - 1) * 0x8);
		writel(0x0, NSS_CC_PORT1_RX_CBCR + 0x4 + (PORT6 - 1) * 0x8);
	}

	switch (mode) {
		case EPORT_WRAPPER_SGMII_FIBER:
			writel(0x400, PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
					 + PPE_UNIPHY_MODE_CONTROL);
			break;

		case EPORT_WRAPPER_SGMII0_RGMII4:
		case EPORT_WRAPPER_SGMII1_RGMII4:
		case EPORT_WRAPPER_SGMII4_RGMII4:
			writel(0x420, PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
					 + PPE_UNIPHY_MODE_CONTROL);
			break;

		case EPORT_WRAPPER_SGMII_PLUS:
			writel(0x820, PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
					 + PPE_UNIPHY_MODE_CONTROL);
			break;

		default:
			printf("SGMII Config. wrongly");
			break;
	}

	if (uniphy_index == 0) {
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, false);
	} else if (uniphy_index == 1) {
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, false);
	} else {
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, false);
	}
	mdelay(100);

	if (uniphy_index == 1) {
		writel(0x1, NSS_CC_UNIPHY_PORT1_RX_CBCR + (PORT5 - 1) * 0x8);
		writel(0x1, NSS_CC_UNIPHY_PORT1_RX_CBCR + 0x4 + (PORT5 - 1) * 0x8);
		writel(0x1, NSS_CC_PORT1_RX_CBCR + (PORT5 - 1) * 0x8);
		writel(0x1, NSS_CC_PORT1_RX_CBCR + 0x4 + (PORT5 - 1) * 0x8);
	} else if (uniphy_index == 2) {
		writel(0x1, NSS_CC_UNIPHY_PORT1_RX_CBCR + (PORT6 - 1) * 0x8);
		writel(0x1, NSS_CC_UNIPHY_PORT1_RX_CBCR + 0x4 + (PORT6 - 1) * 8);
		writel(0x1, NSS_CC_PORT1_RX_CBCR + (PORT6 - 1) * 0x8);
		writel(0x1, NSS_CC_PORT1_RX_CBCR + 0x4 + (PORT6 - 1) * 0x8);
	}

	ppe_uniphy_calibration(uniphy_index);
}

static int ppe_uniphy_10g_r_linkup(uint32_t uniphy_index)
{
	uint32_t reg_value = 0;
	uint32_t retries = 100, linkup = 0;

	while (linkup != UNIPHY_10GR_LINKUP) {
		mdelay(1);
		if (retries-- == 0)
			return -1;
		reg_value = csr1_read(uniphy_index, SR_XS_PCS_KR_STS1_ADDRESS);
		linkup = (reg_value >> 12) & UNIPHY_10GR_LINKUP;
	}
	mdelay(10);
	return 0;
}

static void ppe_uniphy_10g_r_mode_set(uint32_t uniphy_index)
{
	if (uniphy_index == 0)
		ppe_uniphy_reset(UNIPHY0_XPCS_RESET, true);
	else if (uniphy_index == 1)
		ppe_uniphy_reset(UNIPHY1_XPCS_RESET, true);
	else
		ppe_uniphy_reset(UNIPHY2_XPCS_RESET, true);

	writel(0x1021, PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
			 + PPE_UNIPHY_MODE_CONTROL);
	writel(0x1C0, PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
			 + UNIPHY_INSTANCE_LINK_DETECT);

	if (uniphy_index == 0) {
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, false);
	} else if (uniphy_index == 1) {
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, false);
	} else {
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, false);
	}
	mdelay(100);

	ppe_uniphy_calibration(uniphy_index);

	if (uniphy_index == 0)
		ppe_uniphy_reset(UNIPHY0_XPCS_RESET, false);
	else if (uniphy_index == 1)
		ppe_uniphy_reset(UNIPHY1_XPCS_RESET, false);
	else
		ppe_uniphy_reset(UNIPHY2_XPCS_RESET, false);
}


static void ppe_uniphy_usxgmii_mode_set(uint32_t uniphy_index)
{
	uint32_t reg_value = 0;

	writel(UNIPHY_MISC2_REG_VALUE, PPE_UNIPHY_BASE +
		(uniphy_index * PPE_UNIPHY_REG_INC) + UNIPHY_MISC2_REG_OFFSET);
	writel(UNIPHY_PLL_RESET_REG_VALUE, PPE_UNIPHY_BASE +
		(uniphy_index * PPE_UNIPHY_REG_INC) + UNIPHY_PLL_RESET_REG_OFFSET);
	mdelay(500);
	writel(UNIPHY_PLL_RESET_REG_DEFAULT_VALUE, PPE_UNIPHY_BASE +
		(uniphy_index * PPE_UNIPHY_REG_INC) + UNIPHY_PLL_RESET_REG_OFFSET);
	mdelay(500);

	if (uniphy_index == 0)
		ppe_uniphy_reset(UNIPHY0_XPCS_RESET, true);
	else if (uniphy_index == 1)
		ppe_uniphy_reset(UNIPHY1_XPCS_RESET, true);
	else
		ppe_uniphy_reset(UNIPHY2_XPCS_RESET, true);
	mdelay(100);

	writel(0x1021, PPE_UNIPHY_BASE + (uniphy_index * PPE_UNIPHY_REG_INC)
			 + PPE_UNIPHY_MODE_CONTROL);

	if (uniphy_index == 0) {
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY0_SOFT_RESET, false);
	} else if (uniphy_index == 1) {
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY1_SOFT_RESET, false);
	} else {
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, true);
		mdelay(100);
		ppe_uniphy_reset(UNIPHY2_SOFT_RESET, false);
	}
	mdelay(100);

	ppe_uniphy_calibration(uniphy_index);

	if (uniphy_index == 0)
		ppe_uniphy_reset(UNIPHY0_XPCS_RESET, false);
	else if (uniphy_index == 1)
		ppe_uniphy_reset(UNIPHY1_XPCS_RESET, false);
	else
		ppe_uniphy_reset(UNIPHY2_XPCS_RESET, false);
	mdelay(100);

	ppe_uniphy_10g_r_linkup(uniphy_index);
	reg_value = csr1_read(uniphy_index, VR_XS_PCS_DIG_CTRL1_ADDRESS);
	reg_value |= USXG_EN;
	csr1_write(uniphy_index, VR_XS_PCS_DIG_CTRL1_ADDRESS, reg_value);
	reg_value = csr1_read(uniphy_index, VR_MII_AN_CTRL_ADDRESS);
	reg_value |= MII_AN_INTR_EN;
	reg_value |= MII_CTRL;
	csr1_write(uniphy_index, VR_MII_AN_CTRL_ADDRESS, reg_value);
	reg_value = csr1_read(uniphy_index, SR_MII_CTRL_ADDRESS);
	reg_value |= AN_ENABLE;
	reg_value &= ~SS5;
	reg_value |= SS6 | SS13 | DUPLEX_MODE;
	csr1_write(uniphy_index, SR_MII_CTRL_ADDRESS, reg_value);
}

void ppe_uniphy_mode_set(uint32_t uniphy_index, uint32_t mode)
{
	if (!is_uniphy_enabled(uniphy_index)) {
		printf("Uniphy %u is disabled\n", uniphy_index);
		return;
	}

	switch(mode) {
		case EPORT_WRAPPER_PSGMII:
			ppe_uniphy_psgmii_mode_set(uniphy_index);
			break;
		case EPORT_WRAPPER_QSGMII:
			ppe_uniphy_qsgmii_mode_set(uniphy_index);
			break;
		case EPORT_WRAPPER_SGMII0_RGMII4:
		case EPORT_WRAPPER_SGMII1_RGMII4:
		case EPORT_WRAPPER_SGMII4_RGMII4:
		case EPORT_WRAPPER_SGMII_PLUS:
		case EPORT_WRAPPER_SGMII_FIBER:
			ppe_uniphy_sgmii_mode_set(uniphy_index, mode);
			break;
		case EPORT_WRAPPER_USXGMII:
			ppe_uniphy_usxgmii_mode_set(uniphy_index);
			break;
		case EPORT_WRAPPER_10GBASE_R:
			ppe_uniphy_10g_r_mode_set(uniphy_index);
			break;
		default:
			break;
	}
}

void ppe_uniphy_usxgmii_autoneg_completed(uint32_t uniphy_index)
{
	uint32_t autoneg_complete = 0, retries = 100;
	uint32_t reg_value = 0;

	while (autoneg_complete != 0x1) {
		mdelay(1);
		if (retries-- == 0)
		{
			return;
		}
		reg_value = csr1_read(uniphy_index, VR_MII_AN_INTR_STS);
		autoneg_complete = reg_value & 0x1;
	}
	reg_value &= ~CL37_ANCMPLT_INTR;
	csr1_write(uniphy_index, VR_MII_AN_INTR_STS, reg_value);
}

void ppe_uniphy_usxgmii_speed_set(uint32_t uniphy_index, int speed)
{
	uint32_t reg_value = 0;

	reg_value = csr1_read(uniphy_index, SR_MII_CTRL_ADDRESS);
	reg_value |= DUPLEX_MODE;

	switch(speed) {
	case 0:
		reg_value &=~SS5;
		reg_value &=~SS6;
		reg_value &=~SS13;
		break;
	case 1:
		reg_value &=~SS5;
		reg_value &=~SS6;
		reg_value |=SS13;
		break;
	case 2:
		reg_value &=~SS5;
		reg_value |=SS6;
		reg_value &=~SS13;
		break;
	case 3:
		reg_value &=~SS5;
		reg_value |=SS6;
		reg_value |=SS13;
		break;
	case 4:
		reg_value |=SS5;
		reg_value &=~SS6;
		reg_value &=~SS13;
		break;
	case 5:
		reg_value |=SS5;
		reg_value &=~SS6;
		reg_value |=SS13;
		break;
	}
	csr1_write(uniphy_index, SR_MII_CTRL_ADDRESS, reg_value);

}

void ppe_uniphy_usxgmii_duplex_set(uint32_t uniphy_index, int duplex)
{
	uint32_t reg_value = 0;

	reg_value = csr1_read(uniphy_index, SR_MII_CTRL_ADDRESS);

	if (duplex & 0x1)
		reg_value |= DUPLEX_MODE;
	else
		reg_value &= ~DUPLEX_MODE;

	csr1_write(uniphy_index, SR_MII_CTRL_ADDRESS, reg_value);
}

void ppe_uniphy_usxgmii_port_reset(uint32_t uniphy_index)
{
	uint32_t reg_value = 0;

	reg_value = csr1_read(uniphy_index, VR_XS_PCS_DIG_CTRL1_ADDRESS);
	reg_value |= USRA_RST;
	csr1_write(uniphy_index, VR_XS_PCS_DIG_CTRL1_ADDRESS, reg_value);
}
