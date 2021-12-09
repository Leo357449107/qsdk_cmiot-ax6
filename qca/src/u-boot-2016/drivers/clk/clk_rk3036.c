/*
 * (C) Copyright 2015 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <syscon.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/cru_rk3036.h>
#include <asm/arch/hardware.h>
#include <asm/arch/periph.h>
#include <dm/lists.h>

DECLARE_GLOBAL_DATA_PTR;

struct rk3036_clk_plat {
	enum rk_clk_id clk_id;
};

struct rk3036_clk_priv {
	struct rk3036_cru *cru;
	ulong rate;
};

enum {
	VCO_MAX_HZ	= 2400U * 1000000,
	VCO_MIN_HZ	= 600 * 1000000,
	OUTPUT_MAX_HZ	= 2400U * 1000000,
	OUTPUT_MIN_HZ	= 24 * 1000000,
};

#define RATE_TO_DIV(input_rate, output_rate) \
	((input_rate) / (output_rate) - 1);

#define DIV_TO_RATE(input_rate, div)	((input_rate) / ((div) + 1))

#define PLL_DIVISORS(hz, _refdiv, _postdiv1, _postdiv2) {\
	.refdiv = _refdiv,\
	.fbdiv = (u32)((u64)hz * _refdiv * _postdiv1 * _postdiv2 / OSC_HZ),\
	.postdiv1 = _postdiv1, .postdiv2 = _postdiv2};\
	_Static_assert(((u64)hz * _refdiv * _postdiv1 * _postdiv2 / OSC_HZ) *\
			 OSC_HZ / (_refdiv * _postdiv1 * _postdiv2) == hz,\
			 #hz "Hz cannot be hit with PLL "\
			 "divisors on line " __stringify(__LINE__));

/* use interge mode*/
static const struct pll_div apll_init_cfg = PLL_DIVISORS(APLL_HZ, 1, 3, 1);
static const struct pll_div gpll_init_cfg = PLL_DIVISORS(GPLL_HZ, 2, 2, 1);

static inline unsigned int log2(unsigned int value)
{
	return fls(value) - 1;
}

static int rkclk_set_pll(struct rk3036_cru *cru, enum rk_clk_id clk_id,
			 const struct pll_div *div)
{
	int pll_id = rk_pll_id(clk_id);
	struct rk3036_pll *pll = &cru->pll[pll_id];

	/* All PLLs have same VCO and output frequency range restrictions. */
	uint vco_hz = OSC_HZ / 1000 * div->fbdiv / div->refdiv * 1000;
	uint output_hz = vco_hz / div->postdiv1 / div->postdiv2;

	debug("PLL at %p: fbdiv=%d, refdiv=%d, postdiv1=%d, postdiv2=%d,\
		 vco=%u Hz, output=%u Hz\n",
			pll, div->fbdiv, div->refdiv, div->postdiv1,
			div->postdiv2, vco_hz, output_hz);
	assert(vco_hz >= VCO_MIN_HZ && vco_hz <= VCO_MAX_HZ &&
	       output_hz >= OUTPUT_MIN_HZ && output_hz <= OUTPUT_MAX_HZ);

	/* use interger mode */
	rk_clrreg(&pll->con1, 1 << PLL_DSMPD_SHIFT);

	rk_clrsetreg(&pll->con0,
		     PLL_POSTDIV1_MASK << PLL_POSTDIV1_SHIFT | PLL_FBDIV_MASK,
		     (div->postdiv1 << PLL_POSTDIV1_SHIFT) | div->fbdiv);
	rk_clrsetreg(&pll->con1, PLL_POSTDIV2_MASK << PLL_POSTDIV2_SHIFT |
			PLL_REFDIV_MASK << PLL_REFDIV_SHIFT,
			(div->postdiv2 << PLL_POSTDIV2_SHIFT |
			 div->refdiv << PLL_REFDIV_SHIFT));

	/* waiting for pll lock */
	while (readl(&pll->con1) & (1 << PLL_LOCK_STATUS_SHIFT))
		udelay(1);

	return 0;
}

static void rkclk_init(struct rk3036_cru *cru)
{
	u32 aclk_div;
	u32 hclk_div;
	u32 pclk_div;

	/* pll enter slow-mode */
	rk_clrsetreg(&cru->cru_mode_con,
		     GPLL_MODE_MASK << GPLL_MODE_SHIFT |
		     APLL_MODE_MASK << APLL_MODE_SHIFT,
		     GPLL_MODE_SLOW << GPLL_MODE_SHIFT |
		     APLL_MODE_SLOW << APLL_MODE_SHIFT);

	/* init pll */
	rkclk_set_pll(cru, CLK_ARM, &apll_init_cfg);
	rkclk_set_pll(cru, CLK_GENERAL, &gpll_init_cfg);

	/*
	 * select apll as core clock pll source and
	 * set up dependent divisors for PCLK/HCLK and ACLK clocks.
	 * core hz : apll = 1:1
	 */
	aclk_div = APLL_HZ / CORE_ACLK_HZ - 1;
	assert((aclk_div + 1) * CORE_ACLK_HZ == APLL_HZ && aclk_div < 0x7);

	pclk_div = APLL_HZ / CORE_PERI_HZ - 1;
	assert((pclk_div + 1) * CORE_PERI_HZ == APLL_HZ && pclk_div < 0xf);

	rk_clrsetreg(&cru->cru_clksel_con[0],
		     CORE_CLK_PLL_SEL_MASK << CORE_CLK_PLL_SEL_SHIFT |
		     CORE_DIV_CON_MASK << CORE_DIV_CON_SHIFT,
		     CORE_CLK_PLL_SEL_APLL << CORE_CLK_PLL_SEL_SHIFT |
		     0 << CORE_DIV_CON_SHIFT);

	rk_clrsetreg(&cru->cru_clksel_con[1],
		     CORE_ACLK_DIV_MASK << CORE_ACLK_DIV_SHIFT |
		     CORE_PERI_DIV_MASK << CORE_PERI_DIV_SHIFT,
		     aclk_div << CORE_ACLK_DIV_SHIFT |
		     pclk_div << CORE_PERI_DIV_SHIFT);

	/*
	 * select apll as cpu clock pll source and
	 * set up dependent divisors for PCLK/HCLK and ACLK clocks.
	 */
	aclk_div = APLL_HZ / CPU_ACLK_HZ - 1;
	assert((aclk_div + 1) * CPU_ACLK_HZ == APLL_HZ && aclk_div < 0x1f);

	pclk_div = APLL_HZ / CPU_PCLK_HZ - 1;
	assert((pclk_div + 1) * CPU_PCLK_HZ == APLL_HZ && pclk_div < 0x7);

	hclk_div = APLL_HZ / CPU_HCLK_HZ - 1;
	assert((hclk_div + 1) * CPU_HCLK_HZ == APLL_HZ && hclk_div < 0x3);

	rk_clrsetreg(&cru->cru_clksel_con[0],
		     CPU_CLK_PLL_SEL_MASK << CPU_CLK_PLL_SEL_SHIFT |
		     ACLK_CPU_DIV_MASK << ACLK_CPU_DIV_SHIFT,
		     CPU_CLK_PLL_SEL_APLL << CPU_CLK_PLL_SEL_SHIFT |
		     aclk_div << ACLK_CPU_DIV_SHIFT);

	rk_clrsetreg(&cru->cru_clksel_con[1],
		     CPU_PCLK_DIV_MASK << CPU_PCLK_DIV_SHIFT |
		     CPU_HCLK_DIV_MASK << CPU_HCLK_DIV_SHIFT,
		     pclk_div << CPU_PCLK_DIV_SHIFT |
		     hclk_div << CPU_HCLK_DIV_SHIFT);

	/*
	 * select gpll as peri clock pll source and
	 * set up dependent divisors for PCLK/HCLK and ACLK clocks.
	 */
	aclk_div = GPLL_HZ / PERI_ACLK_HZ - 1;
	assert((aclk_div + 1) * PERI_ACLK_HZ == GPLL_HZ && aclk_div < 0x1f);

	hclk_div = log2(PERI_ACLK_HZ / PERI_HCLK_HZ);
	assert((1 << hclk_div) * PERI_HCLK_HZ ==
		PERI_ACLK_HZ && (pclk_div < 0x4));

	pclk_div = log2(PERI_ACLK_HZ / PERI_PCLK_HZ);
	assert((1 << pclk_div) * PERI_PCLK_HZ ==
		PERI_ACLK_HZ && pclk_div < 0x8);

	rk_clrsetreg(&cru->cru_clksel_con[10],
		     PERI_PLL_SEL_MASK << PERI_PLL_SEL_SHIFT |
		     PERI_PCLK_DIV_MASK << PERI_PCLK_DIV_SHIFT |
		     PERI_HCLK_DIV_MASK << PERI_HCLK_DIV_SHIFT |
		     PERI_ACLK_DIV_MASK << PERI_ACLK_DIV_SHIFT,
		     PERI_PLL_GPLL << PERI_PLL_SEL_SHIFT |
		     pclk_div << PERI_PCLK_DIV_SHIFT |
		     hclk_div << PERI_HCLK_DIV_SHIFT |
		     aclk_div << PERI_ACLK_DIV_SHIFT);

	/* PLL enter normal-mode */
	rk_clrsetreg(&cru->cru_mode_con,
		     GPLL_MODE_MASK << GPLL_MODE_SHIFT |
		     APLL_MODE_MASK << APLL_MODE_SHIFT,
		     GPLL_MODE_NORM << GPLL_MODE_SHIFT |
		     APLL_MODE_NORM << APLL_MODE_SHIFT);
}

/* Get pll rate by id */
static uint32_t rkclk_pll_get_rate(struct rk3036_cru *cru,
				   enum rk_clk_id clk_id)
{
	uint32_t refdiv, fbdiv, postdiv1, postdiv2;
	uint32_t con;
	int pll_id = rk_pll_id(clk_id);
	struct rk3036_pll *pll = &cru->pll[pll_id];
	static u8 clk_shift[CLK_COUNT] = {
		0xff, APLL_MODE_SHIFT, DPLL_MODE_SHIFT, 0xff,
		GPLL_MODE_SHIFT, 0xff
	};
	static u8 clk_mask[CLK_COUNT] = {
		0xff, APLL_MODE_MASK, DPLL_MODE_MASK, 0xff,
		GPLL_MODE_MASK, 0xff
	};
	uint shift;
	uint mask;

	con = readl(&cru->cru_mode_con);
	shift = clk_shift[clk_id];
	mask = clk_mask[clk_id];

	switch ((con >> shift) & mask) {
	case GPLL_MODE_SLOW:
		return OSC_HZ;
	case GPLL_MODE_NORM:

		/* normal mode */
		con = readl(&pll->con0);
		postdiv1 = (con >> PLL_POSTDIV1_SHIFT) & PLL_POSTDIV1_MASK;
		fbdiv = (con >> PLL_FBDIV_SHIFT) & PLL_FBDIV_MASK;
		con = readl(&pll->con1);
		postdiv2 = (con >> PLL_POSTDIV2_SHIFT) & PLL_POSTDIV2_MASK;
		refdiv = (con >> PLL_REFDIV_SHIFT) & PLL_REFDIV_MASK;
		return (24 * fbdiv / (refdiv * postdiv1 * postdiv2)) * 1000000;
	case GPLL_MODE_DEEP:
	default:
		return 32768;
	}
}

static ulong rockchip_mmc_get_clk(struct rk3036_cru *cru, uint clk_general_rate,
				  enum periph_id periph)
{
	uint src_rate;
	uint div, mux;
	u32 con;

	switch (periph) {
	case PERIPH_ID_EMMC:
		con = readl(&cru->cru_clksel_con[12]);
		mux = (con >> EMMC_PLL_SHIFT) & EMMC_PLL_MASK;
		div = (con >> EMMC_DIV_SHIFT) & EMMC_DIV_MASK;
		break;
	case PERIPH_ID_SDCARD:
		con = readl(&cru->cru_clksel_con[12]);
		mux = (con >> MMC0_PLL_SHIFT) & MMC0_PLL_MASK;
		div = (con >> MMC0_DIV_SHIFT) & MMC0_DIV_MASK;
		break;
	default:
		return -EINVAL;
	}

	src_rate = mux == EMMC_SEL_24M ? OSC_HZ : clk_general_rate;
	return DIV_TO_RATE(src_rate, div);
}

static ulong rockchip_mmc_set_clk(struct rk3036_cru *cru, uint clk_general_rate,
				  enum periph_id periph, uint freq)
{
	int src_clk_div;
	int mux;

	debug("%s: clk_general_rate=%u\n", __func__, clk_general_rate);

	/* mmc clock auto divide 2 in internal */
	src_clk_div = (clk_general_rate / 2 + freq - 1) / freq;

	if (src_clk_div > 0x7f) {
		src_clk_div = (OSC_HZ / 2 + freq - 1) / freq;
		mux = EMMC_SEL_24M;
	} else {
		mux = EMMC_SEL_GPLL;
	}

	switch (periph) {
	case PERIPH_ID_EMMC:
		rk_clrsetreg(&cru->cru_clksel_con[12],
			     EMMC_PLL_MASK << EMMC_PLL_SHIFT |
			     EMMC_DIV_MASK << EMMC_DIV_SHIFT,
			     mux << EMMC_PLL_SHIFT |
			     (src_clk_div - 1) << EMMC_DIV_SHIFT);
		break;
	case PERIPH_ID_SDCARD:
		rk_clrsetreg(&cru->cru_clksel_con[11],
			     MMC0_PLL_MASK << MMC0_PLL_SHIFT |
			     MMC0_DIV_MASK << MMC0_DIV_SHIFT,
			     mux << MMC0_PLL_SHIFT |
			     (src_clk_div - 1) << MMC0_DIV_SHIFT);
		break;
	default:
		return -EINVAL;
	}

	return rockchip_mmc_get_clk(cru, clk_general_rate, periph);
}

static ulong rk3036_clk_get_rate(struct udevice *dev)
{
	struct rk3036_clk_plat *plat = dev_get_platdata(dev);
	struct rk3036_clk_priv *priv = dev_get_priv(dev);

	debug("%s\n", dev->name);
	return rkclk_pll_get_rate(priv->cru, plat->clk_id);
}

static ulong rk3036_clk_set_rate(struct udevice *dev, ulong rate)
{
	debug("%s\n", dev->name);

	return 0;
}

ulong rk3036_set_periph_rate(struct udevice *dev, int periph, ulong rate)
{
	struct rk3036_clk_priv *priv = dev_get_priv(dev);
	ulong new_rate;

	switch (periph) {
	case PERIPH_ID_EMMC:
		new_rate = rockchip_mmc_set_clk(priv->cru, clk_get_rate(dev),
						periph, rate);
		break;
	default:
		return -ENOENT;
	}

	return new_rate;
}

static struct clk_ops rk3036_clk_ops = {
	.get_rate	= rk3036_clk_get_rate,
	.set_rate	= rk3036_clk_set_rate,
	.set_periph_rate = rk3036_set_periph_rate,
};

static int rk3036_clk_probe(struct udevice *dev)
{
	struct rk3036_clk_plat *plat = dev_get_platdata(dev);
	struct rk3036_clk_priv *priv = dev_get_priv(dev);

	if (plat->clk_id != CLK_OSC) {
		struct rk3036_clk_priv *parent_priv = dev_get_priv(dev->parent);

		priv->cru = parent_priv->cru;
		return 0;
	}
	priv->cru = (struct rk3036_cru *)dev_get_addr(dev);
	rkclk_init(priv->cru);

	return 0;
}

static const char *const clk_name[] = {
	"osc",
	"apll",
	"dpll",
	"cpll",
	"gpll",
	"mpll",
};

static int rk3036_clk_bind(struct udevice *dev)
{
	struct rk3036_clk_plat *plat = dev_get_platdata(dev);
	int pll, ret;

	/* We only need to set up the root clock */
	if (dev->of_offset == -1) {
		plat->clk_id = CLK_OSC;
		return 0;
	}

	/* Create devices for P main clocks */
	for (pll = 1; pll < CLK_COUNT; pll++) {
		struct udevice *child;
		struct rk3036_clk_plat *cplat;

		debug("%s %s\n", __func__, clk_name[pll]);
		ret = device_bind_driver(dev, "clk_rk3036", clk_name[pll],
					 &child);
		if (ret)
			return ret;

		cplat = dev_get_platdata(child);
		cplat->clk_id = pll;
	}

	/* The reset driver does not have a device node, so bind it here */
	ret = device_bind_driver(gd->dm_root, "rk3036_reset", "reset", &dev);
	if (ret)
		debug("Warning: No RK3036 reset driver: ret=%d\n", ret);

	return 0;
}

static const struct udevice_id rk3036_clk_ids[] = {
	{ .compatible = "rockchip,rk3036-cru" },
	{ }
};

U_BOOT_DRIVER(clk_rk3036) = {
	.name		= "clk_rk3036",
	.id		= UCLASS_CLK,
	.of_match	= rk3036_clk_ids,
	.priv_auto_alloc_size = sizeof(struct rk3036_clk_priv),
	.platdata_auto_alloc_size = sizeof(struct rk3036_clk_plat),
	.ops		= &rk3036_clk_ops,
	.bind		= rk3036_clk_bind,
	.probe		= rk3036_clk_probe,
};
