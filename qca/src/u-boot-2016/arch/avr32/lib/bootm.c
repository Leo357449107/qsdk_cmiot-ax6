/*
 * Copyright (C) 2004-2006 Atmel Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/byteorder.h>
#include <asm/arch/addrspace.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/arch/clk.h>

DECLARE_GLOBAL_DATA_PTR;

/* CPU-specific hook to allow flushing of caches, etc. */
extern void prepare_to_boot(void);

static struct tag *setup_start_tag(struct tag *params)
{
	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 4096;
	params->u.core.rootdev = 0;

	return tag_next(params);
}

static struct tag *setup_memory_tags(struct tag *params)
{
	bd_t *bd = gd->bd;
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size(tag_mem_range);

		params->u.mem_range.addr = bd->bi_dram[i].start;
		params->u.mem_range.size = bd->bi_dram[i].size;

		params = tag_next(params);
	}

	return params;
}

static struct tag *setup_commandline_tag(struct tag *params, char *cmdline)
{
	if (!cmdline)
		return params;

	/* eat leading white space */
	while (*cmdline == ' ') cmdline++;

	/*
	 * Don't include tags for empty command lines; let the kernel
	 * use its default command line.
	 */
	if (*cmdline == '\0')
		return params;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen(cmdline) + 1 + 3) >> 2;
	strcpy(params->u.cmdline.cmdline, cmdline);

	return tag_next(params);
}

static struct tag *setup_ramdisk_tag(struct tag *params,
				     unsigned long rd_start,
				     unsigned long rd_end)
{
	if (rd_start == rd_end)
		return params;

	params->hdr.tag = ATAG_RDIMG;
	params->hdr.size = tag_size(tag_mem_range);

	params->u.mem_range.addr = rd_start;
	params->u.mem_range.size = rd_end - rd_start;

	return tag_next(params);
}

static struct tag *setup_clock_tags(struct tag *params)
{
	params->hdr.tag = ATAG_CLOCK;
	params->hdr.size = tag_size(tag_clock);
	params->u.clock.clock_id = ACLOCK_BOOTCPU;
	params->u.clock.clock_flags = 0;
	params->u.clock.clock_hz = gd->arch.cpu_hz;

#ifdef CONFIG_AT32AP7000
	/*
	 * New kernels don't need this, but we should be backwards
	 * compatible for a while...
	 */
	params = tag_next(params);

	params->hdr.tag = ATAG_CLOCK;
	params->hdr.size = tag_size(tag_clock);
	params->u.clock.clock_id = ACLOCK_HSB;
	params->u.clock.clock_flags = 0;
	params->u.clock.clock_hz = get_hsb_clk_rate();
#endif

	return tag_next(params);
}

static struct tag *setup_ethernet_tag(struct tag *params,
				      char *addr, int index)
{
	char *s, *e;
	int i;

	params->hdr.tag = ATAG_ETHERNET;
	params->hdr.size = tag_size(tag_ethernet);

	params->u.ethernet.mac_index = index;
	params->u.ethernet.mii_phy_addr = gd->bd->bi_phy_id[index];

	s = addr;
	for (i = 0; i < 6; i++) {
		params->u.ethernet.hw_address[i] = simple_strtoul(s, &e, 16);
		s = e + 1;
	}

	return tag_next(params);
}

static struct tag *setup_ethernet_tags(struct tag *params)
{
	char name[16] = "ethaddr";
	char *addr;
	int i = 0;

	do {
		addr = getenv(name);
		if (addr)
			params = setup_ethernet_tag(params, addr, i);
		sprintf(name, "eth%daddr", ++i);
	} while (i < 4);

	return params;
}

static struct tag *setup_boardinfo_tag(struct tag *params)
{
	params->hdr.tag = ATAG_BOARDINFO;
	params->hdr.size = tag_size(tag_boardinfo);

	params->u.boardinfo.board_number = gd->bd->bi_board_number;

	return tag_next(params);
}

static void setup_end_tag(struct tag *params)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

int do_bootm_linux(int flag, int argc, char * const argv[], bootm_headers_t *images)
{
	void	(*theKernel)(int magic, void *tagtable);
	struct	tag *params, *params_start;
	char	*commandline = getenv("bootargs");

	/*
	 * allow the PREP bootm subcommand, it is required for bootm to work
	 *
	 * TODO: Andreas Bießmann <andreas.devel@googlemail.com> refactor the
	 * do_bootm_linux() for avr32
	 */
	if (flag & BOOTM_STATE_OS_PREP)
		return 0;

	if ((flag != 0) && (flag != BOOTM_STATE_OS_GO))
		return 1;

	theKernel = (void *)images->ep;

	bootstage_mark(BOOTSTAGE_ID_RUN_OS);

	params = params_start = (struct tag *)gd->bd->bi_boot_params;
	params = setup_start_tag(params);
	params = setup_memory_tags(params);
	if (images->rd_start) {
		params = setup_ramdisk_tag(params,
					   PHYSADDR(images->rd_start),
					   PHYSADDR(images->rd_end));
	}
	params = setup_commandline_tag(params, commandline);
	params = setup_clock_tags(params);
	params = setup_ethernet_tags(params);
	params = setup_boardinfo_tag(params);
	setup_end_tag(params);

	printf("\nStarting kernel at %p (params at %p)...\n\n",
	       theKernel, params_start);

	prepare_to_boot();

	theKernel(ATAG_MAGIC, params_start);
	/* does not return */

	return 1;
}
