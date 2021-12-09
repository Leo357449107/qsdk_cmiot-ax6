/*
 * Copyright (C) 2012-2015 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/io.h>
#include <asm/armv7.h>
#include <mach/ssc-regs.h>

#ifdef CONFIG_UNIPHIER_L2CACHE_ON
static void uniphier_cache_maint_all(u32 operation)
{
	/* try until the command is successfully set */
	do {
		writel(SSCOQM_S_ALL | SSCOQM_CE | operation, SSCOQM);
	} while (readl(SSCOPPQSEF) & (SSCOPPQSEF_FE | SSCOPPQSEF_OE));

	/* wait until the operation is completed */
	while (readl(SSCOLPQS) != SSCOLPQS_EF)
		;

	/* clear the complete notification flag */
	writel(SSCOLPQS_EF, SSCOLPQS);

	writel(SSCOPE_CM_SYNC, SSCOPE); /* drain internal buffers */
	readl(SSCOPE); /* need a read back to confirm */
}

void v7_outer_cache_flush_all(void)
{
	uniphier_cache_maint_all(SSCOQM_CM_WB_INV);
}

void v7_outer_cache_inval_all(void)
{
	uniphier_cache_maint_all(SSCOQM_CM_INV);
}

static void __uniphier_cache_maint_range(u32 start, u32 size, u32 operation)
{
	/* try until the command is successfully set */
	do {
		writel(SSCOQM_S_ADDRESS | SSCOQM_CE | operation, SSCOQM);
		writel(start, SSCOQAD);
		writel(size, SSCOQSZ);

	} while (readl(SSCOPPQSEF) & (SSCOPPQSEF_FE | SSCOPPQSEF_OE));

	/* wait until the operation is completed */
	while (readl(SSCOLPQS) != SSCOLPQS_EF)
		;

	/* clear the complete notification flag */
	writel(SSCOLPQS_EF, SSCOLPQS);
}

static void uniphier_cache_maint_range(u32 start, u32 end, u32 operation)
{
	u32 size;

	/*
	 * If start address is not aligned to cache-line,
	 * do cache operation for the first cache-line
	 */
	start = start & ~(SSC_LINE_SIZE - 1);

	if (start == 0 && end >= (u32)(-SSC_LINE_SIZE)) {
		/* this means cache operation for all range */
		uniphier_cache_maint_all(operation);
		return;
	}

	/*
	 * If end address is not aligned to cache-line,
	 * do cache operation for the last cache-line
	 */
	size = (end - start + SSC_LINE_SIZE - 1) & ~(SSC_LINE_SIZE - 1);

	while (size) {
		u32 chunk_size = size > SSC_RANGE_OP_MAX_SIZE ?
						SSC_RANGE_OP_MAX_SIZE : size;
		__uniphier_cache_maint_range(start, chunk_size, operation);

		start += chunk_size;
		size -= chunk_size;
	}

	writel(SSCOPE_CM_SYNC, SSCOPE); /* drain internal buffers */
	readl(SSCOPE); /* need a read back to confirm */
}

void v7_outer_cache_flush_range(u32 start, u32 end)
{
	uniphier_cache_maint_range(start, end, SSCOQM_CM_WB_INV);
}

void v7_outer_cache_inval_range(u32 start, u32 end)
{
	uniphier_cache_maint_range(start, end, SSCOQM_CM_INV);
}

void v7_outer_cache_enable(void)
{
	u32 tmp;
	tmp = readl(SSCC);
	tmp |= SSCC_ON;
	writel(tmp, SSCC);
}
#endif

void v7_outer_cache_disable(void)
{
	u32 tmp;
	tmp = readl(SSCC);
	tmp &= ~SSCC_ON;
	writel(tmp, SSCC);
}

void enable_caches(void)
{
	dcache_enable();
}
