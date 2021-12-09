/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/arch/mp.h>
#include <asm/arch/soc.h>

DECLARE_GLOBAL_DATA_PTR;

void *get_spin_tbl_addr(void)
{
	return &__spin_table;
}

phys_addr_t determine_mp_bootpg(void)
{
	return (phys_addr_t)&secondary_boot_code;
}

int fsl_layerscape_wake_seconday_cores(void)
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
#ifdef CONFIG_FSL_LSCH3
	struct ccsr_reset __iomem *rst = (void *)(CONFIG_SYS_FSL_RST_ADDR);
#elif defined(CONFIG_FSL_LSCH2)
	struct ccsr_scfg __iomem *scfg = (void *)(CONFIG_SYS_FSL_SCFG_ADDR);
#endif
	u32 cores, cpu_up_mask = 1;
	int i, timeout = 10;
	u64 *table = get_spin_tbl_addr();

#ifdef COUNTER_FREQUENCY_REAL
	/* update for secondary cores */
	__real_cntfrq = COUNTER_FREQUENCY_REAL;
	flush_dcache_range((unsigned long)&__real_cntfrq,
			   (unsigned long)&__real_cntfrq + 8);
#endif

	cores = cpu_mask();
	/* Clear spin table so that secondary processors
	 * observe the correct value after waking up from wfe.
	 */
	memset(table, 0, CONFIG_MAX_CPUS*SPIN_TABLE_ELEM_SIZE);
	flush_dcache_range((unsigned long)table,
			   (unsigned long)table +
			   (CONFIG_MAX_CPUS*SPIN_TABLE_ELEM_SIZE));

	printf("Waking secondary cores to start from %lx\n", gd->relocaddr);

#ifdef CONFIG_FSL_LSCH3
	gur_out32(&gur->bootlocptrh, (u32)(gd->relocaddr >> 32));
	gur_out32(&gur->bootlocptrl, (u32)gd->relocaddr);
	gur_out32(&gur->scratchrw[6], 1);
	asm volatile("dsb st" : : : "memory");
	rst->brrl = cores;
	asm volatile("dsb st" : : : "memory");
#elif defined(CONFIG_FSL_LSCH2)
	scfg_out32(&scfg->scratchrw[0], (u32)(gd->relocaddr >> 32));
	scfg_out32(&scfg->scratchrw[1], (u32)gd->relocaddr);
	asm volatile("dsb st" : : : "memory");
	gur_out32(&gur->brrl, cores);
	asm volatile("dsb st" : : : "memory");

	/* Bootup online cores */
	scfg_out32(&scfg->corebcr, cores);
#endif
	/* This is needed as a precautionary measure.
	 * If some code before this has accidentally  released the secondary
	 * cores then the pre-bootloader code will trap them in a "wfe" unless
	 * the scratchrw[6] is set. In this case we need a sev here to get these
	 * cores moving again.
	 */
	asm volatile("sev");

	while (timeout--) {
		flush_dcache_range((unsigned long)table, (unsigned long)table +
				   CONFIG_MAX_CPUS * 64);
		for (i = 1; i < CONFIG_MAX_CPUS; i++) {
			if (table[i * WORDS_PER_SPIN_TABLE_ENTRY +
					SPIN_TABLE_ELEM_STATUS_IDX])
				cpu_up_mask |= 1 << i;
		}
		if (hweight32(cpu_up_mask) == hweight32(cores))
			break;
		udelay(10);
	}
	if (timeout <= 0) {
		printf("Not all cores (0x%x) are up (0x%x)\n",
		       cores, cpu_up_mask);
		return 1;
	}
	printf("All (%d) cores are up.\n", hweight32(cores));

	return 0;
}

int is_core_valid(unsigned int core)
{
	return !!((1 << core) & cpu_mask());
}

int is_core_online(u64 cpu_id)
{
	u64 *table;
	int pos = id_to_core(cpu_id);
	table = (u64 *)get_spin_tbl_addr() + pos * WORDS_PER_SPIN_TABLE_ENTRY;
	return table[SPIN_TABLE_ELEM_STATUS_IDX] == 1;
}

int cpu_reset(int nr)
{
	puts("Feature is not implemented.\n");

	return 0;
}

int cpu_disable(int nr)
{
	puts("Feature is not implemented.\n");

	return 0;
}

int core_to_pos(int nr)
{
	u32 cores = cpu_mask();
	int i, count = 0;

	if (nr == 0) {
		return 0;
	} else if (nr >= hweight32(cores)) {
		puts("Not a valid core number.\n");
		return -1;
	}

	for (i = 1; i < 32; i++) {
		if (is_core_valid(i)) {
			count++;
			if (count == nr)
				break;
		}
	}

	return count;
}

int cpu_status(int nr)
{
	u64 *table;
	int pos;

	if (nr == 0) {
		table = (u64 *)get_spin_tbl_addr();
		printf("table base @ 0x%p\n", table);
	} else {
		pos = core_to_pos(nr);
		if (pos < 0)
			return -1;
		table = (u64 *)get_spin_tbl_addr() + pos *
			WORDS_PER_SPIN_TABLE_ENTRY;
		printf("table @ 0x%p\n", table);
		printf("   addr - 0x%016llx\n",
		       table[SPIN_TABLE_ELEM_ENTRY_ADDR_IDX]);
		printf("   status   - 0x%016llx\n",
		       table[SPIN_TABLE_ELEM_STATUS_IDX]);
		printf("   lpid  - 0x%016llx\n",
		       table[SPIN_TABLE_ELEM_LPID_IDX]);
	}

	return 0;
}

int cpu_release(int nr, int argc, char * const argv[])
{
	u64 boot_addr;
	u64 *table = (u64 *)get_spin_tbl_addr();
	int pos;

	pos = core_to_pos(nr);
	if (pos <= 0)
		return -1;

	table += pos * WORDS_PER_SPIN_TABLE_ENTRY;
	boot_addr = simple_strtoull(argv[0], NULL, 16);
	table[SPIN_TABLE_ELEM_ENTRY_ADDR_IDX] = boot_addr;
	flush_dcache_range((unsigned long)table,
			   (unsigned long)table + SPIN_TABLE_ELEM_SIZE);
	asm volatile("dsb st");
	smp_kick_all_cpus();	/* only those with entry addr set will run */
	/*
	 * When the first release command runs, all cores are set to go. Those
	 * without a valid entry address will be trapped by "wfe". "sev" kicks
	 * them off to check the address again. When set, they continue to run.
	 */
	asm volatile("sev");

	return 0;
}
