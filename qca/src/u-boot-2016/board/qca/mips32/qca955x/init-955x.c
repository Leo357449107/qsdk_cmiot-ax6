/*
 * Copyright (c) 2014, 2016-2017 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <asm/addrspace.h>
#include <atheros.h>

#define ATH_MAX_DDR_SIZE		(256 * 1024 * 1024)
#define ATH_DDR_SIZE_INCR		(4 * 1024 * 1024)

int
ath_ddr_find_size(void)
{
	uint8_t  *p = (uint8_t *)KSEG1, pat = 0x77;
	int i;

#define max_i		(ATH_MAX_DDR_SIZE / ATH_DDR_SIZE_INCR)

	*p = pat;

	/*
	 * DDR wraps around. Write a pattern to 0x0000_0000. Write an
	 * address pattern at 4M, 8M, 16M etc. and check when
	 * 0x0000_0000 gets overwritten.
	 */
	for(i = 1; (i < max_i); i++) {
		*(p + i * ATH_DDR_SIZE_INCR) = (uint8_t)(i);
		if (*p != pat) {
			break;
		}
	}

	return ((i < max_i) ? (i * ATH_DDR_SIZE_INCR) : ATH_MAX_DDR_SIZE);
}

inline int
ath_ram_type(uint32_t bs)
{
	if (RST_BOOTSTRAP_DDR_SELECT_GET(bs)) {
		return ATH_MEM_DDR1;
	} else {
		return ATH_MEM_DDR2;
	}
}

#define CFG_DDR2_SCORPION_CAS_LATENCY	4

#ifdef CONFIG_TB614
#	define DDR_CONFIG2_SWAP_A26_A27_VAL	(0x1)
#else
#	define DDR_CONFIG2_SWAP_A26_A27_VAL	(0x0)
#endif

#if CFG_DDR2_SCORPION_CAS_LATENCY == 4
#define CFG_DDR2_CONFIG_VAL			DDR_CONFIG_CAS_LATENCY_MSB_SET(0x1) | \
						DDR_CONFIG_OPEN_PAGE_SET(0x1) | \
						DDR_CONFIG_CAS_LATENCY_SET(0x1) | \
						DDR_CONFIG_TMRD_SET(0xf) | \
						DDR_CONFIG_TRFC_SET(0x15) | \
						DDR_CONFIG_TRRD_SET(0x7) | \
						DDR_CONFIG_TRP_SET(0x9) | \
						DDR_CONFIG_TRCD_SET(0x9) | \
						DDR_CONFIG_TRAS_SET(0x1b)

#define CFG_DDR2_CONFIG2_VAL			DDR_CONFIG2_HALF_WIDTH_LOW_SET(0x1) | \
						DDR_CONFIG2_SWAP_A26_A27_SET(DDR_CONFIG2_SWAP_A26_A27_VAL) | \
						DDR_CONFIG2_GATE_OPEN_LATENCY_SET(0x8) | \
						DDR_CONFIG2_TWTR_SET(0x15) | \
						DDR_CONFIG2_TRTP_SET(0x9) | \
						DDR_CONFIG2_TRTW_SET(0xe) | \
						DDR_CONFIG2_TWR_SET(0x1) | \
						DDR_CONFIG2_CKE_SET(0x1) | \
						DDR_CONFIG2_CNTL_OE_EN_SET(0x1) | \
						DDR_CONFIG2_BURST_LENGTH_SET(0x8)

#define CFG_DDR2_CONFIG3_VAL			0x0000000a
#define CFG_DDR2_EXT_MODE_VAL			0x402
#define CFG_DDR2_MODE_VAL_INIT			0x143
#define CFG_DDR2_MODE_VAL			0x43
#define CFG_DDR2_TAP_VAL			0x10
#define CFG_DDR2_EN_TWL_VAL			0x0000167d
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16	0xffff
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32	0xff

#elif CFG_DDR2_SCORPION_CAS_LATENCY == 5

#define CFG_DDR2_CONFIG_VAL			DDR_CONFIG_CAS_LATENCY_MSB_SET(0x1) | \
						DDR_CONFIG_OPEN_PAGE_SET(0x1) | \
						DDR_CONFIG_CAS_LATENCY_SET(0x4) | \
						DDR_CONFIG_TMRD_SET(0xf) | \
						DDR_CONFIG_TRFC_SET(0x15) | \
						DDR_CONFIG_TRRD_SET(0x7) | \
						DDR_CONFIG_TRP_SET(0x9) | \
						DDR_CONFIG_TRCD_SET(0x9) | \
						DDR_CONFIG_TRAS_SET(0x1b)

#define CFG_DDR2_CONFIG2_VAL			DDR_CONFIG2_HALF_WIDTH_LOW_SET(0x1) | \
						DDR_CONFIG2_SWAP_A26_A27_SET(DDR_CONFIG2_SWAP_A26_A27_VAL) | \
						DDR_CONFIG2_GATE_OPEN_LATENCY_SET(0xb) | \
						DDR_CONFIG2_TWTR_SET(0x15) | \
						DDR_CONFIG2_TRTP_SET(0x9) | \
						DDR_CONFIG2_TRTW_SET(0xe) | \
						DDR_CONFIG2_TWR_SET(0x1) | \
						DDR_CONFIG2_CKE_SET(0x1) | \
						DDR_CONFIG2_CNTL_OE_EN_SET(0x1) | \
						DDR_CONFIG2_BURST_LENGTH_SET(0x8)

#define CFG_DDR2_CONFIG3_VAL			0x0000000a
#define CFG_DDR2_EXT_MODE_VAL			0x402
#define CFG_DDR2_MODE_VAL_INIT			0x153
#define CFG_DDR2_MODE_VAL			0x53
#define CFG_DDR2_TAP_VAL			0x10
#define CFG_DDR2_EN_TWL_VAL			0x00001e7d
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16	0xffff
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32	0xff
#endif

#define CFG_DDR1_CONFIG_VAL			DDR_CONFIG_OPEN_PAGE_SET(0x1) | \
						DDR_CONFIG_CAS_LATENCY_SET(0x7) | \
						DDR_CONFIG_TMRD_SET(0x5) | \
						DDR_CONFIG_TRFC_SET(0x7) | \
						DDR_CONFIG_TRRD_SET(0x4) | \
						DDR_CONFIG_TRP_SET(0x6) | \
						DDR_CONFIG_TRCD_SET(0x6) | \
						DDR_CONFIG_TRAS_SET(0x10)

#define CFG_DDR1_CONFIG2_VAL			DDR_CONFIG2_HALF_WIDTH_LOW_SET(0x1) | \
						DDR_CONFIG2_GATE_OPEN_LATENCY_SET(0x6) | \
						DDR_CONFIG2_TWTR_SET(0xe) | \
						DDR_CONFIG2_TRTP_SET(0x8) | \
						DDR_CONFIG2_TRTW_SET(0xe) | \
						DDR_CONFIG2_TWR_SET(0xd) | \
						DDR_CONFIG2_CKE_SET(0x1) | \
						DDR_CONFIG2_CNTL_OE_EN_SET(0x1) | \
						DDR_CONFIG2_BURST_LENGTH_SET(0x8)
#define CFG_DDR1_CONFIG3_VAL			0x0
#define CFG_DDR1_EXT_MODE_VAL			0x0
#define CFG_DDR1_MODE_VAL_INIT			0x133
#define CFG_DDR1_MODE_VAL			0x33
#define CFG_DDR1_RD_DATA_THIS_CYCLE_VAL_16	0xffff
#define CFG_DDR1_RD_DATA_THIS_CYCLE_VAL_32	0xff
#define CFG_DDR1_TAP_VAL			0x20

#define CFG_DDR_CTL_CONFIG			DDR_CTL_CONFIG_SRAM_TSEL_SET(0x1) | \
						DDR_CTL_CONFIG_GE0_SRAM_SYNC_SET(0x1) | \
						DDR_CTL_CONFIG_GE1_SRAM_SYNC_SET(0x1) | \
						DDR_CTL_CONFIG_USB_SRAM_SYNC_SET(0x1) | \
						DDR_CTL_CONFIG_PCIE_SRAM_SYNC_SET(0x1) | \
						DDR_CTL_CONFIG_WMAC_SRAM_SYNC_SET(0x1) | \
						DDR_CTL_CONFIG_MISC_SRC1_SRAM_SYNC_SET(0x1) | \
						DDR_CTL_CONFIG_MISC_SRC2_SRAM_SYNC_SET(0x1)

int /* ram type */
ath_ddr_initial_config(uint32_t refresh)
{
#if !defined(CONFIG_ATH_NAND_BR) && !defined(CONFIG_ATH_EMULATION)
	int		ddr_config, ddr_config2, ddr_config3, ext_mod, mod_val,
			mod_val_init, cycle_val, tap_val, type, ctl_config;
	uint32_t	*pll = (unsigned *)PLL_CONFIG_VAL_F;
	uint32_t	bootstrap;

#if !defined(CONFIG_DISPLAY_BOARDINFO)
	prmsg("\nsri\n");
	prmsg("Scorpion 1.%d\n", ath_reg_rd(RST_REVISION_ID_ADDRESS) & 0xf);
#endif

	bootstrap = ath_reg_rd(RST_BOOTSTRAP_ADDRESS);

	switch(type = ath_ram_type(bootstrap)) {
	case ATH_MEM_DDR2:
		ddr_config	= CFG_DDR2_CONFIG_VAL;
		ddr_config2	= CFG_DDR2_CONFIG2_VAL;
		ddr_config3	= CFG_DDR2_CONFIG3_VAL;
		ext_mod		= CFG_DDR2_EXT_MODE_VAL;
		mod_val_init	= CFG_DDR2_MODE_VAL_INIT;
		mod_val		= CFG_DDR2_MODE_VAL;
		tap_val		= CFG_DDR2_TAP_VAL;

		ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x10);
		udelay(10);
		ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x20);
		udelay(10);
		prmsg("%s(%d): (", __func__, __LINE__);

		if (RST_BOOTSTRAP_DDR_WIDTH_GET(bootstrap)) {
			prmsg("32");
			ctl_config =	CFG_DDR_CTL_CONFIG |
					DDR_CTL_CONFIG_PAD_DDR2_SEL_SET(0x1);

			cycle_val = CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32;
		} else {
			prmsg("16");
			ctl_config =	CFG_DDR_CTL_CONFIG |
					DDR_CTL_CONFIG_PAD_DDR2_SEL_SET(0x1) |
					DDR_CTL_CONFIG_HALF_WIDTH_SET(0x1);

			cycle_val = CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16;
		}

		ctl_config |= CPU_DDR_SYNC_MODE;

		ath_reg_wr_nf(DDR_CTL_CONFIG_ADDRESS, ctl_config);

		prmsg("bit) ddr2 init\n");
		udelay(10);
		break;
	case ATH_MEM_DDR1:
		ddr_config	= CFG_DDR1_CONFIG_VAL;
		ddr_config2	= CFG_DDR1_CONFIG2_VAL;
		ddr_config3	= CFG_DDR1_CONFIG3_VAL;
		ext_mod		= CFG_DDR1_EXT_MODE_VAL;
		mod_val_init	= CFG_DDR1_MODE_VAL_INIT;
		mod_val		= CFG_DDR1_MODE_VAL;
		tap_val		= CFG_DDR1_TAP_VAL;

		prmsg("%s(%d): (", __func__, __LINE__);
		if (RST_BOOTSTRAP_DDR_WIDTH_GET(bootstrap)) {
			prmsg("32");
			ctl_config = CFG_DDR_CTL_CONFIG;
			cycle_val = CFG_DDR1_RD_DATA_THIS_CYCLE_VAL_32;
		} else {
			prmsg("16");
			cycle_val = CFG_DDR1_RD_DATA_THIS_CYCLE_VAL_16;
			ctl_config = 0;
		}

		ctl_config |= CPU_DDR_SYNC_MODE;

		ath_reg_wr_nf(DDR_CTL_CONFIG_ADDRESS, ctl_config);
		udelay(10);
		prmsg("bit) ddr1 init\n");

		break;
	}
#if 0
	if (*pll == PLL_MAGIC) {
		uint32_t cas = pll[5];
		if (cas == 3 || cas == 4) {
			cas = (cas * 2) + 2;
			ddr_config &= ~(DDR_CONFIG_CAS_LATENCY_MSB_MASK |
					DDR_CONFIG_CAS_LATENCY_MASK);
			ddr_config |= DDR_CONFIG_CAS_LATENCY_SET(cas & 0x7) |
				DDR_CONFIG_CAS_LATENCY_MSB_SET((cas >> 3) & 1);

			cas = pll[5];

			ddr_config2 &= ~DDR_CONFIG2_GATE_OPEN_LATENCY_MASK;
			ddr_config2 |= DDR_CONFIG2_GATE_OPEN_LATENCY_SET((2 * cas) + 1);

			if (type == ATH_MEM_DDR2) {
				uint32_t tmp;
				tmp = ath_reg_rd(DDR2_CONFIG_ADDRESS);
				tmp &= ~DDR2_CONFIG_DDR2_TWL_MASK;
				tmp |= DDR2_CONFIG_DDR2_TWL_SET(cas == 3 ? 3 : 5);
				ath_reg_wr_nf(DDR2_CONFIG_ADDRESS, tmp);
			}

			mod_val = (cas == 3 ? 0x33 : 0x43);
			mod_val_init = 0x100 | mod_val;
		}
	}
#endif

	ath_reg_wr_nf(DDR_RD_DATA_THIS_CYCLE_ADDRESS, cycle_val);
	udelay(100);
	ath_reg_wr_nf(DDR_BURST_ADDRESS, 0x74444444);
	udelay(100);
	ath_reg_wr_nf(DDR_BURST2_ADDRESS, 0x44444444);
	udelay(100);
	ath_reg_wr_nf(DDR_AHB_MASTER_TIMEOUT_MAX_ADDRESS, 0xfffff);
	udelay(100);
	ath_reg_wr_nf(DDR_CONFIG_ADDRESS, ddr_config);
	udelay(100);
	ath_reg_wr_nf(DDR_CONFIG2_ADDRESS, ddr_config2);
	udelay(100);
	ath_reg_wr(DDR_CONFIG_3_ADDRESS, ddr_config3);
	udelay(100);

	if (type == ATH_MEM_DDR2) {
		ath_reg_wr_nf(DDR2_CONFIG_ADDRESS, CFG_DDR2_EN_TWL_VAL);
		udelay(100);
	}

	ath_reg_wr_nf(DDR_CONFIG2_ADDRESS, ddr_config2 | 0x80);	// CKE Enable
	udelay(100);

	ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x8);	// Precharge
	udelay(10);

	if (type == ATH_MEM_DDR2) {
		ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x10);	// EMR2
		udelay(10);
		ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x20);	// EMR3
		udelay(10);
	}

	if (type == ATH_MEM_DDR1 || type == ATH_MEM_DDR2) {
		ath_reg_wr_nf(DDR_EXTENDED_MODE_REGISTER_ADDRESS, CFG_DDR2_EXT_MODE_VAL); // EMR DLL enable, Reduced Driver Impedance control, Differential DQS disabled
		udelay(100);
		ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x2); // EMR write
		udelay(10);
	}

	ath_reg_wr_nf(DDR_MODE_REGISTER_ADDRESS, mod_val_init);
	udelay(1000);

	ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x1);	// MR Write
	udelay(10);

	ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x8);	// Precharge
	udelay(10);

	ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x4);	// Auto Refresh
	udelay(10);

	ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x4);	// Auto Refresh
	udelay(10);

	// Issue MRS to remove DLL out-of-reset
	ath_reg_wr_nf(DDR_MODE_REGISTER_ADDRESS, mod_val);
	udelay(100);

	ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x1); // MR write
	udelay(100);

	if (type == ATH_MEM_DDR2) {
		ath_reg_wr_nf(DDR_EXTENDED_MODE_REGISTER_ADDRESS, 0x782);
		udelay(100);

		ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x2); // EMR write
		udelay(100);

		ath_reg_wr_nf(DDR_EXTENDED_MODE_REGISTER_ADDRESS, CFG_DDR2_EXT_MODE_VAL);
		udelay(100);

		ath_reg_wr_nf(DDR_CONTROL_ADDRESS, 0x2); // EMR write
		udelay(100);
	}

	ath_reg_wr_nf(DDR_REFRESH_ADDRESS, refresh);
	udelay(100);

	ath_reg_wr(TAP_CONTROL_0_ADDRESS, tap_val);
	ath_reg_wr(TAP_CONTROL_1_ADDRESS, tap_val);

	if (RST_BOOTSTRAP_DDR_WIDTH_GET(bootstrap)) {
		ath_reg_wr (TAP_CONTROL_2_ADDRESS, tap_val);
		ath_reg_wr (TAP_CONTROL_3_ADDRESS, tap_val);
	}

	if (type == ATH_MEM_DDR2) {
		ath_reg_wr(PMU1_ADDRESS, 0x633c8176);
		// Set DDR2 Voltage to 1.8 volts
		ath_reg_wr(PMU2_ADDRESS, PMU2_LDO_TUNE_SET(3) |
					 PMU2_PGM_SET(0x1));
	}

	/*
	 * Based on SGMII validation for stucks, packet errors were  observed and it was
	 * mostly due to noise pickup on SGMII lines. Switching regulator register is to
	 * be programmed with proper setting to avoid such stucks.
	 */
	ath_reg_rmw_clear(PMU1_ADDRESS, (7<<1));
	ath_reg_rmw_set(PMU1_ADDRESS, (1<<3));

	ath_sys_frequency();

	return type;
#else	// !nand flash and !emulation
	return 0;
#endif
}

int
ath_uart_freq(void)
{
	if (ath_reg_rd(RST_BOOTSTRAP_ADDRESS) & RST_BOOTSTRAP_REF_CLK_MASK) {
		return 40 * 1000 * 1000;
	} else {
		return 25 * 1000 * 1000;
	}
}

void ath_sys_frequency()
{
#if !defined(CONFIG_ATH_EMULATION)
	uint32_t pll, out_div, ref_div, nint, frac, clk_ctrl;
#endif
	uint32_t ref = ath_uart_freq();
	uint32_t ath_cpu_freq = 0, ath_ddr_freq = 0, ath_ahb_freq = 0;

	if (ath_cpu_freq)
		goto done;

#ifdef CONFIG_ATH_EMULATION
	ath_cpu_freq = 80000000;
	ath_ddr_freq = 80000000;
	ath_ahb_freq = 40000000;
#else
	prmsg("%s: ", __func__);

	clk_ctrl = ath_reg_rd(ATH_DDR_CLK_CTRL);

#if 0
	pll = ath_reg_rd(CPU_DPLL2_ADDRESS);
	if (CPU_DPLL2_LOCAL_PLL_GET(pll)) {
		out_div	= CPU_DPLL2_OUTDIV_GET(pll);

		pll = ath_reg_rd(CPU_DPLL_ADDRESS);
		nint = CPU_DPLL_NINT_GET(pll);
		frac = CPU_DPLL_NFRAC_GET(pll);
		ref_div = CPU_DPLL_REFDIV_GET(pll);
		pll = ref >> 18;
		frac	= frac * pll / ref_div;
		prmsg("cpu srif ");
	} else {
#endif
		pll = ath_reg_rd(ATH_PLL_CONFIG);
		out_div	= CPU_PLL_CONFIG_OUTDIV_GET(pll);
		ref_div	= CPU_PLL_CONFIG_REFDIV_GET(pll);
		nint	= CPU_PLL_CONFIG_NINT_GET(pll);
		frac	= CPU_PLL_CONFIG_NFRAC_GET(pll);
		pll = ref >> 6;
		frac	= frac * pll / ref_div;
//	}
	ath_cpu_freq = (((nint * (ref / ref_div)) + frac) >> out_div) /
			(CPU_DDR_CLOCK_CONTROL_CPU_POST_DIV_GET(clk_ctrl) + 1);

#if 0
	pll = ath_reg_rd(DDR_DPLL2_ADDRESS);
	if (DDR_DPLL2_LOCAL_PLL_GET(pll)) {
		out_div	= DDR_DPLL2_OUTDIV_GET(pll);

		pll = ath_reg_rd(DDR_DPLL_ADDRESS);
		nint = DDR_DPLL_NINT_GET(pll);
		frac = DDR_DPLL_NFRAC_GET(pll);
		ref_div = DDR_DPLL_REFDIV_GET(pll);
		pll = ref >> 18;
		frac	= frac * pll / ref_div;
		prmsg("ddr srif ");
	} else {
#endif
		pll = ath_reg_rd(ATH_DDR_PLL_CONFIG);
		out_div	= DDR_PLL_CONFIG_OUTDIV_GET(pll);
		ref_div	= DDR_PLL_CONFIG_REFDIV_GET(pll);
		nint	= DDR_PLL_CONFIG_NINT_GET(pll);
		frac	= DDR_PLL_CONFIG_NFRAC_GET(pll);
		pll = ref >> 10;
		frac	= frac * pll / ref_div;
//	}
	ath_ddr_freq = (((nint * (ref / ref_div)) + frac) >> out_div) /
			(CPU_DDR_CLOCK_CONTROL_DDR_POST_DIV_GET(clk_ctrl) + 1);

	if (CPU_DDR_CLOCK_CONTROL_AHBCLK_FROM_DDRPLL_GET(clk_ctrl)) {
		ath_ahb_freq = ath_ddr_freq /
			(CPU_DDR_CLOCK_CONTROL_AHB_POST_DIV_GET(clk_ctrl) + 1);
	} else {
		ath_ahb_freq = ath_cpu_freq /
			(CPU_DDR_CLOCK_CONTROL_AHB_POST_DIV_GET(clk_ctrl) + 1);
	}
#endif
done:
		prmsg("cpu %u ddr %u ahb %u\n",
			ath_cpu_freq / 1000000,
			ath_ddr_freq / 1000000,
			ath_ahb_freq / 1000000);
}
