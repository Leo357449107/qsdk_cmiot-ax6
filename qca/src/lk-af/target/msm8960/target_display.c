/* Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
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
#include <debug.h>
#include <msm_panel.h>
#include <dev/pm8921.h>
#include <board.h>
#include <mdp4.h>
#include <target/display.h>
#include <target/board.h>
#include <platform/timer.h>
#include <platform/iomap.h>
#include <platform/clock.h>
#include <reg.h>
#include <bits.h>

#define FRAMEBUFFER_PHY_OFFSET 0x89600000

static struct msm_fb_panel_data panel;

static struct msm_fb_panel_data hdmi_panel;

static uint8_t display_enable;

#if PRI_DISPLAY_ENABLED
	static uint32_t pri_display_enable = 1;
#else
	static uint32_t pri_display_enable = 0;
#endif

#if SEC_DISPLAY_ENABLED
	static uint32_t sec_display_enable = 1;
#else
	static uint32_t sec_display_enable = 0;
#endif

#if TER_DISPLAY_ENABLED
	static uint32_t ter_display_enable = 1;
#else
	static uint32_t ter_display_enable = 0;
#endif

#ifdef WITH_SPLASH_SCREEN_MARKER
#include <reg.h>
#define MPM_SCLK_COUNT_VAL    0x00200024
#define TIMER_KHZ 32768

static unsigned int lk_splash_val;
static unsigned int update_splash_val;
static unsigned int lk_splash_val_set = 0;
static unsigned int lk_splash_val_en = 1;

extern void get_lk_splash_val(unsigned int *val);

static unsigned int place_marker(char *marker_name)
{
	unsigned int marker_value;

	marker_value = readl(MPM_SCLK_COUNT_VAL);
	dprintf(INFO, "marker name=%s; marker value=%u.%03u seconds\n",
			marker_name, marker_value/TIMER_KHZ,
			(((marker_value % TIMER_KHZ)
			* 1000) / TIMER_KHZ));
	return marker_value;
}

void get_lk_splash_val(unsigned int *val)
{
	if (val != NULL)
		*val = update_splash_val;
}

static void set_lk_splash_val(unsigned int val)
{
	update_splash_val = val;
}

static void write_lk_splash_time(void)
{
	if (lk_splash_val_en && !lk_splash_val_set) {
		lk_splash_val = place_marker("splash screen");
		set_lk_splash_val(lk_splash_val);
		lk_splash_val_set = 1;
	}
}
#else
static void write_lk_splash_time(void)
{
}
#endif

extern int msm_display_init(struct msm_fb_panel_data *pdata);
extern int msm_display_off();

static void apq8064_lvds_fpdlink3_on(void)
{
	struct pm8921_gpio pdb_pin = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = 0,
		.output_value = 1,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_1,
		.inv_int_pol = 0,
	};
	int rc = pm8921_gpio_config(PM_GPIO(37),&pdb_pin);
	if (rc)
		dprintf(CRITICAL, "FAIL pm8921_gpio_config(): rc=%d.\n", rc);
}

static int apq8064_lvds_panel_power(int enable)
{
	if (enable) {
		/* Enable LVS7 */
		pm8921_low_voltage_switch_enable(lvs_7);
		/* Set and enabale LDO2 1.2V for  VDDA_LVDS_PLL*/
		pm8921_ldo_set_voltage(LDO_2, LDO_VOLTAGE_1_2V);

		/* Enable Ext 3.3V -  MSM GPIO 77*/
		/* TODO: IS this really needed? This wasn't even implemented correctly.
		 * GPIO enable was not happening.
		 */
		apq8064_ext_3p3V_enable();

		apq8064_display_gpio_init();

		/* Configure PMM MPP  3*/
		pm8921_mpp_set_digital_output(mpp_3);

		/*
		 * drive gpio high to enable adp_2 board
		 * todo: link this to board id
		 */
		apq8064_lvds_fpdlink3_on();
	}

	return 0;
}

static int apq8064_lvds_clock(int enable,
				struct msm_panel_info *pinfo)
{
	if (enable)
		mdp_clock_init();

	return 0;
}

static int fusion3_mtp_panel_power(int enable)
{
	if (enable) {
		/* Enable LVS7 */
		pm8921_low_voltage_switch_enable(7);

		/* Set and enabale LDO2 1.2V for  VDDA_MIPI_DSI0/1_PLL */
		pm8921_ldo_set_voltage(LDO_2, LDO_VOLTAGE_1_2V);

		/* Set and enabale LDO11 3.0V for  LCD1_MIPI_AVDD */
		pm8921_ldo_set_voltage(LDO_11, LDO_VOLTAGE_3_0V);

		apq8064_display_gpio_init();
	}

	return 0;
}

static int fusion3_mtp_clock(int enable,
				struct msm_panel_info *pinfo)
{
	if (enable) {
		mdp_clock_init();
		mmss_clock_init();
	} else if(!target_cont_splash_screen()) {
		mmss_clock_disable();
	}

	return 0;
}

static void msm8960_backlight_on(void)
{
	struct pm8921_gpio backlight_pwm = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = 0,
		.output_value = 0,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_1,
		.inv_int_pol = 0,
	};

	int rc = pm8921_gpio_config(PM_GPIO(24), &backlight_pwm);
	if (rc)
		dprintf(CRITICAL, "FAIL pm8921_gpio_config(): rc=%d.\n", rc);
}

/* Pull DISP_RST_N high to get panel out of reset */
static void msm8960_mipi_panel_reset(void)
{
	struct pm8921_gpio gpio43_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = 0,
		.output_value = 1,
		.pull = PM_GPIO_PULL_UP_30,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_PAIRED,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	pm8921_gpio_config(PM_GPIO(43), &gpio43_param);
}

static int msm8960_mipi_panel_clock(int enable,
					struct msm_panel_info *pinfo)
{
	if (enable) {
		int target_id = board_target_id();
		mdp_clock_init();
		if ((target_id == LINUX_MACHTYPE_8064_ADP_2)||
			(target_id == LINUX_MACHTYPE_8064_ADP_2_ES2) ||
			(target_id == LINUX_MACHTYPE_8064_ADP_2_ES2P5)) {
			adp_mmss_clock_init(pinfo);
		}
		else
			mmss_clock_init();
	} else if(!target_cont_splash_screen()) {
			mmss_clock_disable();
	}

	return 0;
}

static int mpq8064_hdmi_panel_clock(int enable,
					struct msm_panel_info *pinfo)
{
	if (MPLATFORM()) {
		int target_id = board_target_id();
		if (enable) {
			if (target_id != LINUX_MACHTYPE_8064_LIQUID ||
				target_id != LINUX_MACHTYPE_8064_CDP) {
				mdp_clock_init();
			}
			hdmi_app_clk_init(enable);
		}
		return 0;
	} else {
		if (enable)
			mdp_clock_init();

		hdmi_app_clk_init(enable);

		return 0;
	}
}

static int mpq8064_hdmi_panel_power(int enable)
{
	if (enable)
		hdmi_power_init();

	return 0;
}

static int msm8960_liquid_mipi_panel_clock(int enable,
						struct msm_panel_info *pinfo)
{
	if (enable) {
		mdp_clock_init();
		liquid_mmss_clock_init(); /* 240 MHZ MIPI-DSI clk */
	} else if(!target_cont_splash_screen()) {
			mmss_clock_disable();
	}

	return 0;
}

static int msm8960_mipi_panel_power(int enable)
{
	if (enable) {
		msm8960_backlight_on();

		/* Turn on LDO8 for lcd1 mipi vdd */
		pm8921_ldo_set_voltage(LDO_8, LDO_VOLTAGE_3_0V);

		/* Turn on LDO23 for lcd1 mipi vddio */
		pm8921_ldo_set_voltage(LDO_23, LDO_VOLTAGE_1_8V);

		/* Turn on LDO2 for vdda_mipi_dsi */
		pm8921_ldo_set_voltage(LDO_2, LDO_VOLTAGE_1_2V);

		msm8960_mipi_panel_reset();
	}

	return 0;
}

static int apq8064_mipi_panel_power(int enable)
{
	if (enable) {

		/* Turn on LVS_7 for mipi vdd */
		pm8921_low_voltage_switch_enable(lvs_7);

		/* Turn on LDO2 for lcd1 mipi vddio */
		pm8921_ldo_set_voltage(LDO_2, LDO_VOLTAGE_1_2V);

		/* Turn on LDO11 for vdda_mipi_dsi */
		pm8921_ldo_set_voltage(LDO_11, LDO_VOLTAGE_3_0V);
	}

	return 0;
}

#define	PM_GPIO_VIN_VPH			0 /* 3v ~ 4.4v */
#define	PM_GPIO_VIN_BB			1 /* ~3.3v */
#define	PM_GPIO_VIN_S4			2 /* 1.8v */
#define	PM_GPIO_VIN_L15			3

static int msm8960_liquid_mipi_panel_power(int enable)
{
	if (enable) {
		static int gpio17, gpio21, gpio43 ;
		int rc;

		struct pm8921_gpio gpio_config = {
			.direction = PM_GPIO_DIR_OUT,
			.output_buffer = 0,
			.output_value = 1,
			.pull = PM_GPIO_PULL_NO,
			.vin_sel = PM_GPIO_VIN_S4,
			.out_strength = PM_GPIO_STRENGTH_HIGH,
			.function = PM_GPIO_FUNC_NORMAL,
			.inv_int_pol = 0,
			.disable_pin = 0,
		};

		/* Note: PWM is controlled by PM-GPIO#24 */
		gpio17 = PM_GPIO(17); /* ext_3p3v */
		gpio21 = PM_GPIO(21); /* disp power enable_n , vin=VPH-PWR */
		gpio43 = PM_GPIO(43); /* Displays Enable (rst_n) */

		gpio_config.output_value = 1;
		rc = pm8921_gpio_config(gpio17, &gpio_config);
		mdelay(100);
		gpio_config.output_value = 0;
		/* disp disable (resx=0) */
		rc = pm8921_gpio_config(gpio43, &gpio_config);
		mdelay(100);
		gpio_config.output_value = 0;
		gpio_config.vin_sel = PM_GPIO_VIN_VPH; /* VPH_PWR */
		/* disp power enable_n */
		rc = pm8921_gpio_config(gpio21, &gpio_config);
		mdelay(100);
		gpio_config.output_value = 1;
		gpio_config.vin_sel = PM_GPIO_VIN_S4;
		/* disp enable */
		rc = pm8921_gpio_config(gpio43, &gpio_config);
		mdelay(100);

		pm8921_low_voltage_switch_enable(lvs_4); /* S4 1.8 V */

		/* Turn on LDO2 for vdda_mipi_dsi */
		pm8921_ldo_set_voltage(LDO_2, LDO_VOLTAGE_1_2V);

		msm8960_backlight_on();
	}

	return 0;
}

void display_init(void)
{
	int target_id = board_target_id();
	memset(&panel, 0, sizeof(panel));

	dprintf(INFO, "display_init(),target_id=%d.\n", target_id);

	switch (target_id) {
	case LINUX_MACHTYPE_8960_LIQUID:
		mipi_chimei_video_wxga_init(&(panel.panel_info));
		/*
		 * mipi_chimei_wxga panel not supported yet in LK.
		 * However, MIPI clocks and power should be set in LK.
		 */
		panel.clk_func = msm8960_liquid_mipi_panel_clock;
		panel.power_func = msm8960_liquid_mipi_panel_power;
		panel.fb.base = MIPI_FB_ADDR;
		panel.fb.width =  panel.panel_info.xres;
		panel.fb.height =  panel.panel_info.yres;
		panel.fb.stride =  panel.panel_info.xres;
		panel.fb.bpp =  panel.panel_info.bpp;
		panel.fb.format = FB_FORMAT_RGB888;
		panel.mdp_rev = MDP_REV_44;
		break;
	case LINUX_MACHTYPE_8064_CDP:
		lvds_chimei_wxga_init(&(panel.panel_info));
		panel.clk_func = apq8064_lvds_clock;
		panel.power_func = apq8064_lvds_panel_power;
		panel.fb.base = 0x89000000;
		panel.fb.width =  panel.panel_info.xres;
		panel.fb.height =  panel.panel_info.yres;
		panel.fb.stride =  panel.panel_info.xres;
		panel.fb.bpp = panel.panel_info.bpp;
		panel.fb.format = FB_FORMAT_RGB888;
		panel.mdp_rev = MDP_REV_44;
		break;
	case LINUX_MACHTYPE_8064_ADP_2:
	case LINUX_MACHTYPE_8064_ADP_2_ES2:
	case LINUX_MACHTYPE_8064_ADP_2_ES2P5:
		if (MPLATFORM()) {
			panel.fb.base = 0x8ec00000;
		} else {
			panel.fb.base = FRAMEBUFFER_PHY_OFFSET;
		}
		if (pri_display_enable) {
			lvds_chimei_wxga_init(&(panel.panel_info));
			panel.clk_func = apq8064_lvds_clock;
			panel.power_func = apq8064_lvds_panel_power;
			panel.fb.width =  panel.panel_info.xres;
			panel.fb.height =  panel.panel_info.yres;
			panel.fb.stride =  panel.panel_info.xres;
			panel.fb.bpp =  panel.panel_info.bpp;
			panel.fb.format = FB_FORMAT_RGB888;
			panel.mdp_rev = MDP_REV_44;
			if (MPLATFORM()) {
				hdmi_msm_panel_init(&(hdmi_panel.panel_info));
				hdmi_panel.clk_func = mpq8064_hdmi_panel_clock;
				hdmi_panel.power_func = mpq8064_hdmi_panel_power;
				hdmi_panel.fb.base = 0x89000000;
				hdmi_panel.fb.width = hdmi_panel.panel_info.xres;
				hdmi_panel.fb.height = hdmi_panel.panel_info.yres;
				hdmi_panel.fb.stride = hdmi_panel.panel_info.xres;
				hdmi_panel.fb.bpp = hdmi_panel.panel_info.bpp;
				hdmi_panel.fb.format = FB_FORMAT_RGB888;
				hdmi_panel.mdp_rev = MDP_REV_44;
				hdmi_set_fb_addr(hdmi_panel.fb.base);
			}
		}
		break;
	case LINUX_MACHTYPE_8064_MTP:
		mipi_toshiba_video_wsvga_init(&(panel.panel_info));
		panel.clk_func = fusion3_mtp_clock;
		panel.power_func = fusion3_mtp_panel_power;
		panel.fb.base = 0x89000000;
		panel.fb.width =  panel.panel_info.xres;
		panel.fb.height =  panel.panel_info.yres;
		panel.fb.stride =  panel.panel_info.xres;
		panel.fb.bpp =  panel.panel_info.bpp;
		panel.fb.format = FB_FORMAT_RGB888;
		panel.mdp_rev = MDP_REV_44;
		break;
	case LINUX_MACHTYPE_8960_CDP:
	case LINUX_MACHTYPE_8960_MTP:
	case LINUX_MACHTYPE_8960_FLUID:
		mipi_toshiba_video_wsvga_init(&(panel.panel_info));
		panel.clk_func = msm8960_mipi_panel_clock;
		panel.power_func = msm8960_mipi_panel_power;
		panel.fb.base = 0x89000000;
		panel.fb.width =  panel.panel_info.xres;
		panel.fb.height =  panel.panel_info.yres;
		panel.fb.stride =  panel.panel_info.xres;
		panel.fb.bpp =  panel.panel_info.bpp;
		panel.fb.format = FB_FORMAT_RGB888;
		panel.mdp_rev = MDP_REV_42;
		break;
	case LINUX_MACHTYPE_8064_MPQ_CDP:
	case LINUX_MACHTYPE_8064_MPQ_HRD:
	case LINUX_MACHTYPE_8064_MPQ_DTV:
		hdmi_msm_panel_init(&panel.panel_info);

		panel.clk_func   = mpq8064_hdmi_panel_clock;
		panel.power_func = mpq8064_hdmi_panel_power;
		panel.fb.base    = 0x89000000;
		panel.fb.width   = panel.panel_info.xres;
		panel.fb.height  = panel.panel_info.yres;
		panel.fb.stride  = panel.panel_info.xres;
		panel.fb.bpp     = panel.panel_info.bpp;
		panel.fb.format  = FB_FORMAT_RGB565;
		panel.mdp_rev    = MDP_REV_44;

		hdmi_set_fb_addr(panel.fb.base);
		break;
	default:
		return;
	};

	if (pri_display_enable) {
		if (msm_display_init(&panel)) {
			dprintf(CRITICAL, "Display init failed!\n");
			return;
		}
		write_lk_splash_time();
	}

	if ((target_id == LINUX_MACHTYPE_8064_ADP_2)||
		(target_id == LINUX_MACHTYPE_8064_ADP_2_ES2) ||
		(target_id == LINUX_MACHTYPE_8064_ADP_2_ES2P5)) {
		if (sec_display_enable) {
			unsigned fb_hdmi_offset = panel.panel_info.xres *
				panel.panel_info.yres * (panel.panel_info.bpp / 8);
			fb_hdmi_offset = (fb_hdmi_offset + 0xFFF) & (~0xFFF);
			hdmi_msm_panel_init(&panel.panel_info);
			panel.clk_func   = mpq8064_hdmi_panel_clock;
			panel.power_func = mpq8064_hdmi_panel_power;
			panel.fb.base   += fb_hdmi_offset;
			panel.fb.width   = panel.panel_info.xres;
			panel.fb.height  = panel.panel_info.yres;
			panel.fb.stride  = panel.panel_info.xres;
			panel.fb.bpp     = panel.panel_info.bpp;
			panel.fb.format  = FB_FORMAT_RGB888;
			panel.mdp_rev    = MDP_REV_44;
			hdmi_set_fb_addr(panel.fb.base);
			if (MPLATFORM()) {
				if (msm_display_init(&hdmi_panel)) {
					dprintf(CRITICAL, "HDMI init failed!\n");
					return;
				}
			} else {
				if (msm_display_init(&panel)) {
					dprintf(CRITICAL, "HDMI init failed!\n");
					return;
				}
			}
			write_lk_splash_time();
		}
	}

	/* adding dsi panel to auto platform*/
	if ((target_id == LINUX_MACHTYPE_8064_ADP_2)||
		(target_id == LINUX_MACHTYPE_8064_ADP_2_ES2) ||
		(target_id == LINUX_MACHTYPE_8064_ADP_2_ES2P5)) {
		if (ter_display_enable) {
			uint32_t ahb_en_reg, ahb_enabled;
			ahb_en_reg = readl(AHB_EN_REG);
			ahb_enabled = ahb_en_reg & BIT(4);
			if (!ahb_enabled) {
				writel(ahb_en_reg | BIT(4), AHB_EN_REG);
				/* Make sure iface clock is enabled before register access */
				udelay(10);
			}
			unsigned fb_dsi_offset = panel.panel_info.xres *
				panel.panel_info.yres * (panel.panel_info.bpp / 8);
			fb_dsi_offset = (fb_dsi_offset + 0xFFF) & (~0xFFF);
			mipi_dsi_i2c_video_wvga_init(&(panel.panel_info));
			panel.clk_func   = msm8960_mipi_panel_clock;
			panel.power_func = apq8064_mipi_panel_power;
			panel.fb.base += fb_dsi_offset;
			panel.fb.width =  panel.panel_info.xres;
			panel.fb.height =  panel.panel_info.yres;
			panel.fb.stride =  panel.panel_info.xres;
			panel.fb.bpp =  panel.panel_info.bpp;
			panel.fb.format = FB_FORMAT_RGB888;
			panel.mdp_rev = MDP_REV_44;
			if (msm_display_init(&panel)) {
				dprintf(CRITICAL, "MIPI init failed!\n");
				return;
			}
			write_lk_splash_time();
		}
	}

	if (!MPLATFORM()) {
		display_enable = 1;
	}
}

void display_shutdown(void)
{
	if (display_enable) {
		msm_display_off();
	}
}
