/* Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of The Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
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
 *
 */

#if !DEVICE_TREE /* If not using device tree */

#include <reg.h>
#include <debug.h>
#include <smem.h>
#include <platform/iomap.h>
#include <stdint.h>

#define SIZE_1M             (1024 * 1024)
#define SIZE_2M             (2 * SIZE_1M)
#define SIZE_256M           (256 * SIZE_1M)
#define SIZE_512M           (512 * SIZE_1M)

#define ATAG_MEM            0x54410002

typedef struct {
	uint32_t size;
	uint32_t start_addr;
}mem_info;


mem_info copper_default_first_512M[] = {
	{	.size = (250 * SIZE_1M),
		.start_addr = SDRAM_START_ADDR
	},
	{	.size = (240 * SIZE_1M),
		.start_addr = SDRAM_START_ADDR +
				(16 * SIZE_1M) +
				(256 * SIZE_1M)
	}
};

unsigned *target_mem_atag_create(unsigned *ptr, uint32_t size, uint32_t addr)
{
    *ptr++ = 4;
    *ptr++ = ATAG_MEM;
    *ptr++ = size;
    *ptr++ = addr;

    return ptr;
}


unsigned *target_atag_create(unsigned *ptr,
	mem_info usable_mem_map[], unsigned num_regions)
{
	unsigned int i;

	ASSERT(num_regions);

	dprintf(SPEW, "Number of HLOS regions in 1st bank = %u\n", num_regions);

	for (i = 0; i < num_regions; i++)
	{
		ptr = target_mem_atag_create(ptr,
			usable_mem_map[i].size,
			usable_mem_map[i].start_addr);
	}
	return ptr;
}

unsigned *target_atag_mem(unsigned *ptr)
{
    struct smem_ram_ptable ram_ptable;
    uint8_t i = 0;

	/* Make sure RAM partition table is initialized */
	ASSERT(smem_ram_ptable_init(&ram_ptable));

	for (i = 0; i < ram_ptable.len; i++)
	{
		if (ram_ptable.parts[i].category == SDRAM &&
			(ram_ptable.parts[i].type == SYS_MEMORY) &&
			(ram_ptable.parts[i].start == SDRAM_START_ADDR))
		{
			ASSERT(ram_ptable.parts[i].size >= SIZE_512M);

			if (ram_ptable.parts[i].start == SDRAM_START_ADDR)
				ptr = target_atag_create(ptr,
					copper_default_first_512M,
					ARRAY_SIZE(copper_default_first_512M));

		}

		/* Pass along all other usable memory regions to Linux */
		if (ram_ptable.parts[i].category == SDRAM &&
			(ram_ptable.parts[i].type == SYS_MEMORY) &&
			(ram_ptable.parts[i].start != SDRAM_START_ADDR))
		{
			ptr = target_mem_atag_create(ptr,
				ram_ptable.parts[i].size,
				ram_ptable.parts[i].start);
		}
	}

    return ptr;
}

void *target_get_scratch_address(void)
{
	return ((void *)SCRATCH_ADDR);
}

#endif /* DEVICE_TREE */

