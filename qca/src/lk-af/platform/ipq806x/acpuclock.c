/*
 * Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of The Linux Foundation nor
 *     the names of its contributors may be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <debug.h>
#include <reg.h>
#include <platform/iomap.h>
#include <platform/clock.h>
#include <uart_dm.h>
#include <gsbi.h>
#include <mmc.h>
#include <clock.h>
#include <board.h>
#include <smem.h>

/* Set rate and enable the clock */
static void clock_config(uint32_t ns, uint32_t md, uint32_t ns_addr, uint32_t md_addr)
{
	unsigned int val = 0;

	/* Activate the reset for the M/N Counter */
	val = 1 << 7;
	writel(val, ns_addr);

	/* Write the MD value into the MD register */
	if (md_addr != 0x0)
		writel(md, md_addr);

	/* Write the ns value, and active reset for M/N Counter, again */
	val = 1 << 7;
	val |= ns;
	writel(val, ns_addr);

	/* De-activate the reset for M/N Counter */
	val = 1 << 7;
	val = ~val;
	val = val & readl(ns_addr);
	writel(val, ns_addr);

	/* Enable the Clock Root */
	val = 1 << 11;
	val = val | readl(ns_addr);
	writel(val, ns_addr);

	/* Enable the Clock Branch */
	val = 1 << 9;
	val = val | readl(ns_addr);
	writel(val, ns_addr);

	/* Enable the M/N Counter */
	val = 1 << 8;
	val = val | readl(ns_addr);
	writel(val, ns_addr);
}

/* Write the M,N,D values and enable the MMSS Clocks */
void config_mmss_clk(uint32_t ns,
		     uint32_t md,
		     uint32_t cc,
		     uint32_t ns_addr, uint32_t md_addr, uint32_t cc_addr)
{
	unsigned int val = 0;

	clock_config(ns, md, ns_addr, md_addr);

	/* Enable MND counter */
	val = cc | (1 << 5);
	val = val | readl(cc_addr);
	writel(val, cc_addr);

	/* Enable the root of the clock tree */
	val = 1 << 2;
	val = val | readl(cc_addr);
	writel(val, cc_addr);

	/* Enable the Pixel Clock */
	val = 1 << 0;
	val = val | readl(cc_addr);
	writel(val, cc_addr);

	/* Force On */
	val = 1 << 31;
	val = val | readl(cc_addr);
	writel(val, cc_addr);
}

void hsusb_clock_init(void)
{
	clk_get_set_enable("usb_hs_clk", 60000000, 1);
}

/* Configure UART clock - based on the gsbi id */
void clock_config_uart_dm(uint8_t id)
{
	char gsbi_uart_clk_id[64];
	char gsbi_p_clk_id[64];

	snprintf(gsbi_uart_clk_id, 64,"gsbi%u_uart_clk", id);
	clk_get_set_enable(gsbi_uart_clk_id, 7372800, 1);

	snprintf(gsbi_p_clk_id, 64,"gsbi%u_pclk", id);
	clk_get_set_enable(gsbi_p_clk_id, 0, 1);
}

/* Configure i2c clock */
void clock_config_i2c(uint8_t id, uint32_t freq)
{
	char gsbi_qup_clk_id[64];
	char gsbi_p_clk_id[64];

	snprintf(gsbi_qup_clk_id, 64,"gsbi%u_qup_clk", id);
	clk_get_set_enable(gsbi_qup_clk_id, freq, 1);

	snprintf(gsbi_p_clk_id, 64,"gsbi%u_pclk", id);
	clk_get_set_enable(gsbi_p_clk_id, 0, 1);
}

/* Intialize MMC clock */
void clock_init_mmc(uint32_t id)
{
	char sdc_clk_id[64];

	snprintf(sdc_clk_id, sizeof(sdc_clk_id),"sdc%u_clk", id);
	clk_get_set_enable(sdc_clk_id, 400000, 1);
}

/* Configure MMC clock */
void clock_config_mmc(uint32_t interface, uint32_t freq)
{
	char sdc_clk[64];
	unsigned rate;

	snprintf(sdc_clk, 64, "sdc%u_clk", interface);

	/* Disalbe MCI_CLK before changing the sdcc clock */
	mmc_boot_mci_clk_disable();

	switch(freq)
	{
	case MMC_CLK_400KHZ:
		rate = 400000;
		break;
	case MMC_CLK_48MHZ:
	case MMC_CLK_50MHZ: /* Max supported is 48MHZ */
		rate = 48000000;
		break;
	default:
		ASSERT(0);
		return;

	};

	clk_get_set_enable(sdc_clk, rate, 1);

	/* Enable MCI clk */
	mmc_boot_mci_clk_enable();
}

/* Configure crypto engine clock */
void ce_clock_init(void)
{
    uint32_t platform_id;

    platform_id = board_platform_id();

	if ((platform_id == APQ8064) || (platform_id == APQ8064AA)
		|| (platform_id == APQ8064AB)|| (platform_id == APQ8064AU))
	{
		/* Enable HCLK for CE3 */
		clk_get_set_enable("ce3_pclk", 0, 1);

		/* Enable core clk for CE3 */
		clk_get_set_enable("ce3_clk", 0, 1);
	}
	else
	{
		/* Enable CE1 CLK branch */
		writel((0x10 | readl(CE1_HCLK_CTL_REG)), CE1_HCLK_CTL_REG);

		/* Enable HCLK for CE1 */
		clk_get_set_enable("ce1_pclk", 0, 1);

		/* Enable core clk for CE3 */
		clk_get_set_enable("ce1_clk", 0, 1);
	}
}
/* Async Reset CE1 */
void ce_async_reset()
{
	/* Enable Async reset bit for HCLK CE1 */
	writel((1<<7) | (1 << 4), CE1_HCLK_CTL_REG);
	/* Enable Async reset bit for core clk for CE1 */
	writel((1<<7) | (1 << 4), CE1_CORE_CLK_CTL_REG);

	/* Add a small delay between switching the
	 * async intput from high to low
	 */
	udelay(2);

	/* Disable Async reset bit for HCLK for CE1 */
	writel((1 << 4), CE1_HCLK_CTL_REG);
	/* Disable Async reset bit for core clk for CE1 */
	writel((1 << 4), CE1_CORE_CLK_CTL_REG);

	return;
}

