/*
 * (C) Copyright 2011
 * Graeme Russ, <graeme.russ@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/errno.h>
#include <asm/mtrr.h>

DECLARE_GLOBAL_DATA_PTR;

/* Get the top of usable RAM */
__weak ulong board_get_usable_ram_top(ulong total_size)
{
	return gd->ram_size;
}

int init_cache_f_r(void)
{
#if defined(CONFIG_X86_RESET_VECTOR) & !defined(CONFIG_HAVE_FSP)
	int ret;

	ret = mtrr_commit(false);
	/* If MTRR MSR is not implemented by the processor, just ignore it */
	if (ret && ret != -ENOSYS)
		return ret;
#endif
	/* Initialise the CPU cache(s) */
	return init_cache();
}

bd_t bd_data;

int init_bd_struct_r(void)
{
	gd->bd = &bd_data;
	memset(gd->bd, 0, sizeof(bd_t));

	return 0;
}
