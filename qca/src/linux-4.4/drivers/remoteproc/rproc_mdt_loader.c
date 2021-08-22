/* Copyright (c) 2016, 2018 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include "remoteproc_internal.h"
#include <linux/soc/qcom/smem.h>
#include <linux/soc/qcom/smem_state.h>
#include <linux/qcom_scm.h>
#include <linux/elf.h>
#include <soc/qcom/subsystem_restart.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <uapi/linux/major.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/stringify.h>

#define QCA_MDT_TYPE_MASK      (7 << 24)
#define QCA_MDT_TYPE_HASH      (2 << 24)

int mdt_load(struct rproc *rproc, const struct firmware *fw)
{
	int ret = 0;
	size_t name_len;
	const char *fname = rproc->firmware;
	char *segment_name;
	struct device *dev_rproc = rproc->dev.parent;
	struct elf32_hdr *ehdr;
	int i = 0;
	const u8 *elf_data;
	struct elf32_phdr *phdr;

	name_len = strlen(fname);
	if (name_len <= 4) {
		dev_err(dev_rproc, "Firmware name should be >4bytes (*.mdt)\n");
		return -EINVAL;
	}

	segment_name = kstrdup(fname, GFP_KERNEL);
	if (!segment_name)
		return -ENOMEM;

	if (!fw) {
		dev_err(dev_rproc, "failed to load %s\n", fname);
		return -EINVAL;
	}

	elf_data = fw->data;
	ehdr = (struct elf32_hdr *)fw->data;
	phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);

	/* go through the available ELF segments and load it */
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		u32 pa = phdr->p_paddr;
		u32 memsz = phdr->p_memsz;
		u32 filesz = phdr->p_filesz;
		void *ptr;

		if ((phdr->p_type != PT_LOAD) ||
				((phdr->p_flags & QCA_MDT_TYPE_MASK)
				 == QCA_MDT_TYPE_HASH) || (!phdr->p_memsz))
			continue;

		/* grab the kernel address for this device address */
		ptr = rproc_da_to_va(rproc, pa, memsz);
		if (!ptr) {
			dev_err(dev_rproc, "bad phdr pa 0x%x mem 0x%x\n", pa,
					memsz);
			ret = -EINVAL;
			return ret;
		}

		/* put the segment where the remote processor expects it */
		if (phdr->p_filesz) {
			/* The firmware file has <fname>.mdt as the ELF header,
			 * hash segment, <fname>.b00, <fname>.b01, etc
			 * for every ELF segment of the firmware file. The
			 * rproc loads the first <fname>.mdt file, and for the
			 * ELF segments that we need to load, we make the
			 * filename as <fname>.b"segment_number"
			 */
			snprintf(segment_name + name_len - 3,
				 (size_t)4,
				 "b%02d", i);
			ret = request_firmware(&fw, segment_name, dev_rproc);
			if (ret) {
				dev_err(dev_rproc, "can't to load %s\n",
						segment_name);
				iounmap(ptr);
				break;
			}
			memcpy_toio(ptr, fw->data, fw->size);
			release_firmware(fw);
		}

		/* for .bss and sections that needs to be memset to zero */
		if (memsz > filesz)
			memset_io(ptr + filesz, 0, memsz - filesz);

		iounmap(ptr);
	}
	kfree(segment_name);
	return ret;
}
