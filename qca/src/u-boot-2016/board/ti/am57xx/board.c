/*
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * Based on board/ti/dra7xx/evm.c
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <palmas.h>
#include <sata.h>
#include <usb.h>
#include <asm/omap_common.h>
#include <asm/emif.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/dra7xx_iodelay.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sata.h>
#include <asm/arch/gpio.h>
#include <asm/arch/omap.h>
#include <environment.h>
#include <usb.h>
#include <linux/usb/gadget.h>
#include <dwc3-uboot.h>
#include <dwc3-omap-uboot.h>
#include <ti-usb-phy-uboot.h>

#include "mux_data.h"

#ifdef CONFIG_DRIVER_TI_CPSW
#include <cpsw.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

/* GPIO 7_11 */
#define GPIO_DDR_VTT_EN 203

const struct omap_sysinfo sysinfo = {
	"Board: BeagleBoard x15\n"
};

static const struct dmm_lisa_map_regs beagle_x15_lisa_regs = {
	.dmm_lisa_map_3 = 0x80740300,
	.is_ma_present  = 0x1
};

void emif_get_dmm_regs(const struct dmm_lisa_map_regs **dmm_lisa_regs)
{
	*dmm_lisa_regs = &beagle_x15_lisa_regs;
}

static const struct emif_regs beagle_x15_emif1_ddr3_532mhz_emif_regs = {
	.sdram_config_init	= 0x61851b32,
	.sdram_config		= 0x61851b32,
	.sdram_config2		= 0x00000000,
	.ref_ctrl		= 0x000040F1,
	.ref_ctrl_final		= 0x00001035,
	.sdram_tim1		= 0xceef266b,
	.sdram_tim2		= 0x328f7fda,
	.sdram_tim3		= 0x027f88a8,
	.read_idle_ctrl		= 0x00050000,
	.zq_config		= 0x0007190b,
	.temp_alert_config	= 0x00000000,
	.emif_ddr_phy_ctlr_1_init = 0x0024400b,
	.emif_ddr_phy_ctlr_1	= 0x0e24400b,
	.emif_ddr_ext_phy_ctrl_1 = 0x10040100,
	.emif_ddr_ext_phy_ctrl_2 = 0x00740074,
	.emif_ddr_ext_phy_ctrl_3 = 0x00780078,
	.emif_ddr_ext_phy_ctrl_4 = 0x007c007c,
	.emif_ddr_ext_phy_ctrl_5 = 0x007b007b,
	.emif_rd_wr_lvl_rmp_win	= 0x00000000,
	.emif_rd_wr_lvl_rmp_ctl	= 0x80000000,
	.emif_rd_wr_lvl_ctl	= 0x00000000,
	.emif_rd_wr_exec_thresh	= 0x00000305
};

/* Ext phy ctrl regs 1-35 */
static const u32 beagle_x15_emif1_ddr3_ext_phy_ctrl_const_regs[] = {
	0x10040100,
	0x00740074,
	0x00780078,
	0x007c007c,
	0x007b007b,
	0x00800080,
	0x00360036,
	0x00340034,
	0x00360036,
	0x00350035,
	0x00350035,

	0x01ff01ff,
	0x01ff01ff,
	0x01ff01ff,
	0x01ff01ff,
	0x01ff01ff,

	0x00430043,
	0x003e003e,
	0x004a004a,
	0x00470047,
	0x00400040,

	0x00000000,
	0x00600020,
	0x40011080,
	0x08102040,

	0x00400040,
	0x00400040,
	0x00400040,
	0x00400040,
	0x00400040,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0
};

static const struct emif_regs beagle_x15_emif2_ddr3_532mhz_emif_regs = {
	.sdram_config_init	= 0x61851b32,
	.sdram_config		= 0x61851b32,
	.sdram_config2		= 0x00000000,
	.ref_ctrl		= 0x000040F1,
	.ref_ctrl_final		= 0x00001035,
	.sdram_tim1		= 0xceef266b,
	.sdram_tim2		= 0x328f7fda,
	.sdram_tim3		= 0x027f88a8,
	.read_idle_ctrl		= 0x00050000,
	.zq_config		= 0x0007190b,
	.temp_alert_config	= 0x00000000,
	.emif_ddr_phy_ctlr_1_init = 0x0024400b,
	.emif_ddr_phy_ctlr_1	= 0x0e24400b,
	.emif_ddr_ext_phy_ctrl_1 = 0x10040100,
	.emif_ddr_ext_phy_ctrl_2 = 0x00820082,
	.emif_ddr_ext_phy_ctrl_3 = 0x008b008b,
	.emif_ddr_ext_phy_ctrl_4 = 0x00800080,
	.emif_ddr_ext_phy_ctrl_5 = 0x007e007e,
	.emif_rd_wr_lvl_rmp_win	= 0x00000000,
	.emif_rd_wr_lvl_rmp_ctl	= 0x80000000,
	.emif_rd_wr_lvl_ctl	= 0x00000000,
	.emif_rd_wr_exec_thresh	= 0x00000305
};

static const u32 beagle_x15_emif2_ddr3_ext_phy_ctrl_const_regs[] = {
	0x10040100,
	0x00820082,
	0x008b008b,
	0x00800080,
	0x007e007e,
	0x00800080,
	0x00370037,
	0x00390039,
	0x00360036,
	0x00370037,
	0x00350035,
	0x01ff01ff,
	0x01ff01ff,
	0x01ff01ff,
	0x01ff01ff,
	0x01ff01ff,
	0x00540054,
	0x00540054,
	0x004e004e,
	0x004c004c,
	0x00400040,

	0x00000000,
	0x00600020,
	0x40011080,
	0x08102040,

	0x00400040,
	0x00400040,
	0x00400040,
	0x00400040,
	0x00400040,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0
};

void emif_get_reg_dump(u32 emif_nr, const struct emif_regs **regs)
{
	switch (emif_nr) {
	case 1:
		*regs = &beagle_x15_emif1_ddr3_532mhz_emif_regs;
		break;
	case 2:
		*regs = &beagle_x15_emif2_ddr3_532mhz_emif_regs;
		break;
	}
}

void emif_get_ext_phy_ctrl_const_regs(u32 emif_nr, const u32 **regs, u32 *size)
{
	switch (emif_nr) {
	case 1:
		*regs = beagle_x15_emif1_ddr3_ext_phy_ctrl_const_regs;
		*size = ARRAY_SIZE(beagle_x15_emif1_ddr3_ext_phy_ctrl_const_regs);
		break;
	case 2:
		*regs = beagle_x15_emif2_ddr3_ext_phy_ctrl_const_regs;
		*size = ARRAY_SIZE(beagle_x15_emif2_ddr3_ext_phy_ctrl_const_regs);
		break;
	}
}

struct vcores_data beagle_x15_volts = {
	.mpu.value		= VDD_MPU_DRA752,
	.mpu.efuse.reg		= STD_FUSE_OPP_VMIN_MPU_NOM,
	.mpu.efuse.reg_bits     = DRA752_EFUSE_REGBITS,
	.mpu.addr		= TPS659038_REG_ADDR_SMPS12,
	.mpu.pmic		= &tps659038,

	.eve.value		= VDD_EVE_DRA752,
	.eve.efuse.reg		= STD_FUSE_OPP_VMIN_DSPEVE_NOM,
	.eve.efuse.reg_bits	= DRA752_EFUSE_REGBITS,
	.eve.addr		= TPS659038_REG_ADDR_SMPS45,
	.eve.pmic		= &tps659038,

	.gpu.value		= VDD_GPU_DRA752,
	.gpu.efuse.reg		= STD_FUSE_OPP_VMIN_GPU_NOM,
	.gpu.efuse.reg_bits	= DRA752_EFUSE_REGBITS,
	.gpu.addr		= TPS659038_REG_ADDR_SMPS45,
	.gpu.pmic		= &tps659038,

	.core.value		= VDD_CORE_DRA752,
	.core.efuse.reg		= STD_FUSE_OPP_VMIN_CORE_NOM,
	.core.efuse.reg_bits	= DRA752_EFUSE_REGBITS,
	.core.addr		= TPS659038_REG_ADDR_SMPS6,
	.core.pmic		= &tps659038,

	.iva.value		= VDD_IVA_DRA752,
	.iva.efuse.reg		= STD_FUSE_OPP_VMIN_IVA_NOM,
	.iva.efuse.reg_bits	= DRA752_EFUSE_REGBITS,
	.iva.addr		= TPS659038_REG_ADDR_SMPS45,
	.iva.pmic		= &tps659038,
};

void hw_data_init(void)
{
	*prcm = &dra7xx_prcm;
	*dplls_data = &dra7xx_dplls;
	*omap_vcores = &beagle_x15_volts;
	*ctrl = &dra7xx_ctrl;
}

int board_init(void)
{
	gpmc_init();
	gd->bd->bi_boot_params = (CONFIG_SYS_SDRAM_BASE + 0x100);

	return 0;
}

int board_late_init(void)
{
	init_sata(0);
	/*
	 * DEV_CTRL.DEV_ON = 1 please - else palmas switches off in 8 seconds
	 * This is the POWERHOLD-in-Low behavior.
	 */
	palmas_i2c_write_u8(TPS65903X_CHIP_P1, 0xA0, 0x1);
	return 0;
}

void set_muxconf_regs_essential(void)
{
	do_set_mux32((*ctrl)->control_padconf_core_base,
		     early_padconf, ARRAY_SIZE(early_padconf));
}

#ifdef CONFIG_IODELAY_RECALIBRATION
void recalibrate_iodelay(void)
{
	__recalibrate_iodelay(core_padconf_array_essential,
			      ARRAY_SIZE(core_padconf_array_essential),
			      iodelay_cfg_array, ARRAY_SIZE(iodelay_cfg_array));
}
#endif

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_GENERIC_MMC)
int board_mmc_init(bd_t *bis)
{
	omap_mmc_init(0, 0, 0, -1, -1);
	omap_mmc_init(1, 0, 0, -1, -1);
	return 0;
}
#endif

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_OS_BOOT)
int spl_start_uboot(void)
{
	/* break into full u-boot on 'c' */
	if (serial_tstc() && serial_getc() == 'c')
		return 1;

#ifdef CONFIG_SPL_ENV_SUPPORT
	env_init();
	env_relocate_spec();
	if (getenv_yesno("boot_os") != 1)
		return 1;
#endif

	return 0;
}
#endif

#ifdef CONFIG_USB_DWC3
static struct dwc3_device usb_otg_ss1 = {
	.maximum_speed = USB_SPEED_SUPER,
	.base = DRA7_USB_OTG_SS1_BASE,
	.tx_fifo_resize = false,
	.index = 0,
};

static struct dwc3_omap_device usb_otg_ss1_glue = {
	.base = (void *)DRA7_USB_OTG_SS1_GLUE_BASE,
	.utmi_mode = DWC3_OMAP_UTMI_MODE_SW,
	.index = 0,
};

static struct ti_usb_phy_device usb_phy1_device = {
	.pll_ctrl_base = (void *)DRA7_USB3_PHY1_PLL_CTRL,
	.usb2_phy_power = (void *)DRA7_USB2_PHY1_POWER,
	.usb3_phy_power = (void *)DRA7_USB3_PHY1_POWER,
	.index = 0,
};

static struct dwc3_device usb_otg_ss2 = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = DRA7_USB_OTG_SS2_BASE,
	.tx_fifo_resize = false,
	.index = 1,
};

static struct dwc3_omap_device usb_otg_ss2_glue = {
	.base = (void *)DRA7_USB_OTG_SS2_GLUE_BASE,
	.utmi_mode = DWC3_OMAP_UTMI_MODE_SW,
	.index = 1,
};

static struct ti_usb_phy_device usb_phy2_device = {
	.usb2_phy_power = (void *)DRA7_USB2_PHY2_POWER,
	.index = 1,
};

int board_usb_init(int index, enum usb_init_type init)
{
	enable_usb_clocks(index);
	switch (index) {
	case 0:
		if (init == USB_INIT_DEVICE) {
			printf("port %d can't be used as device\n", index);
			disable_usb_clocks(index);
			return -EINVAL;
		} else {
			usb_otg_ss1.dr_mode = USB_DR_MODE_HOST;
			usb_otg_ss1_glue.vbus_id_status = OMAP_DWC3_ID_GROUND;
			setbits_le32((*prcm)->cm_l3init_usb_otg_ss1_clkctrl,
				     OTG_SS_CLKCTRL_MODULEMODE_HW |
				     OPTFCLKEN_REFCLK960M);
		}

		ti_usb_phy_uboot_init(&usb_phy1_device);
		dwc3_omap_uboot_init(&usb_otg_ss1_glue);
		dwc3_uboot_init(&usb_otg_ss1);
		break;
	case 1:
		if (init == USB_INIT_DEVICE) {
			usb_otg_ss2.dr_mode = USB_DR_MODE_PERIPHERAL;
			usb_otg_ss2_glue.vbus_id_status = OMAP_DWC3_VBUS_VALID;
		} else {
			printf("port %d can't be used as host\n", index);
			disable_usb_clocks(index);
			return -EINVAL;
		}

		ti_usb_phy_uboot_init(&usb_phy2_device);
		dwc3_omap_uboot_init(&usb_otg_ss2_glue);
		dwc3_uboot_init(&usb_otg_ss2);
		break;
	default:
		printf("Invalid Controller Index\n");
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	switch (index) {
	case 0:
	case 1:
		ti_usb_phy_uboot_exit(index);
		dwc3_uboot_exit(index);
		dwc3_omap_uboot_exit(index);
		break;
	default:
		printf("Invalid Controller Index\n");
	}
	disable_usb_clocks(index);
	return 0;
}

int usb_gadget_handle_interrupts(int index)
{
	u32 status;

	status = dwc3_omap_uboot_interrupt_status(index);
	if (status)
		dwc3_uboot_handle_interrupt(index);

	return 0;
}
#endif

#ifdef CONFIG_DRIVER_TI_CPSW

/* Delay value to add to calibrated value */
#define RGMII0_TXCTL_DLY_VAL		((0x3 << 5) + 0x8)
#define RGMII0_TXD0_DLY_VAL		((0x3 << 5) + 0x8)
#define RGMII0_TXD1_DLY_VAL		((0x3 << 5) + 0x2)
#define RGMII0_TXD2_DLY_VAL		((0x4 << 5) + 0x0)
#define RGMII0_TXD3_DLY_VAL		((0x4 << 5) + 0x0)
#define VIN2A_D13_DLY_VAL		((0x3 << 5) + 0x8)
#define VIN2A_D17_DLY_VAL		((0x3 << 5) + 0x8)
#define VIN2A_D16_DLY_VAL		((0x3 << 5) + 0x2)
#define VIN2A_D15_DLY_VAL		((0x4 << 5) + 0x0)
#define VIN2A_D14_DLY_VAL		((0x4 << 5) + 0x0)

static void cpsw_control(int enabled)
{
	/* VTP can be added here */
}

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.slave_reg_ofs	= 0x208,
		.sliver_reg_ofs	= 0xd80,
		.phy_addr	= 1,
	},
	{
		.slave_reg_ofs	= 0x308,
		.sliver_reg_ofs	= 0xdc0,
		.phy_addr	= 2,
	},
};

static struct cpsw_platform_data cpsw_data = {
	.mdio_base		= CPSW_MDIO_BASE,
	.cpsw_base		= CPSW_BASE,
	.mdio_div		= 0xff,
	.channels		= 8,
	.cpdma_reg_ofs		= 0x800,
	.slaves			= 1,
	.slave_data		= cpsw_slaves,
	.ale_reg_ofs		= 0xd00,
	.ale_entries		= 1024,
	.host_port_reg_ofs	= 0x108,
	.hw_stats_reg_ofs	= 0x900,
	.bd_ram_ofs		= 0x2000,
	.mac_control		= (1 << 5),
	.control		= cpsw_control,
	.host_port_num		= 0,
	.version		= CPSW_CTRL_VERSION_2,
};

int board_eth_init(bd_t *bis)
{
	int ret;
	uint8_t mac_addr[6];
	uint32_t mac_hi, mac_lo;
	uint32_t ctrl_val;

	/* try reading mac address from efuse */
	mac_lo = readl((*ctrl)->control_core_mac_id_0_lo);
	mac_hi = readl((*ctrl)->control_core_mac_id_0_hi);
	mac_addr[0] = (mac_hi & 0xFF0000) >> 16;
	mac_addr[1] = (mac_hi & 0xFF00) >> 8;
	mac_addr[2] = mac_hi & 0xFF;
	mac_addr[3] = (mac_lo & 0xFF0000) >> 16;
	mac_addr[4] = (mac_lo & 0xFF00) >> 8;
	mac_addr[5] = mac_lo & 0xFF;

	if (!getenv("ethaddr")) {
		printf("<ethaddr> not set. Validating first E-fuse MAC\n");

		if (is_valid_ethaddr(mac_addr))
			eth_setenv_enetaddr("ethaddr", mac_addr);
	}

	mac_lo = readl((*ctrl)->control_core_mac_id_1_lo);
	mac_hi = readl((*ctrl)->control_core_mac_id_1_hi);
	mac_addr[0] = (mac_hi & 0xFF0000) >> 16;
	mac_addr[1] = (mac_hi & 0xFF00) >> 8;
	mac_addr[2] = mac_hi & 0xFF;
	mac_addr[3] = (mac_lo & 0xFF0000) >> 16;
	mac_addr[4] = (mac_lo & 0xFF00) >> 8;
	mac_addr[5] = mac_lo & 0xFF;

	if (!getenv("eth1addr")) {
		if (is_valid_ethaddr(mac_addr))
			eth_setenv_enetaddr("eth1addr", mac_addr);
	}

	ctrl_val = readl((*ctrl)->control_core_control_io1) & (~0x33);
	ctrl_val |= 0x22;
	writel(ctrl_val, (*ctrl)->control_core_control_io1);

	ret = cpsw_register(&cpsw_data);
	if (ret < 0)
		printf("Error %d registering CPSW switch\n", ret);

	return ret;
}
#endif

#ifdef CONFIG_BOARD_EARLY_INIT_F
/* VTT regulator enable */
static inline void vtt_regulator_enable(void)
{
	if (omap_hw_init_context() == OMAP_INIT_CONTEXT_UBOOT_AFTER_SPL)
		return;

	gpio_request(GPIO_DDR_VTT_EN, "ddr_vtt_en");
	gpio_direction_output(GPIO_DDR_VTT_EN, 1);
}

int board_early_init_f(void)
{
	vtt_regulator_enable();
	return 0;
}
#endif
