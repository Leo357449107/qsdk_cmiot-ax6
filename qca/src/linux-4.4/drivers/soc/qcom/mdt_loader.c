/*
 * Qualcomm Peripheral Image Loader
 *
 * Copyright (C) 2016 Linaro Ltd
 * Copyright (C) 2015 Sony Mobile Communications Inc
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/elf.h>
#include <linux/firmware.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/qcom_scm.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/soc/qcom/mdt_loader.h>
#include <linux/property.h>
#include <linux/dma-mapping.h>

#define PDSEG_PAS_ID	0xD

static bool mdt_phdr_valid(const struct elf32_phdr *phdr)
{
	if (phdr->p_type != PT_LOAD)
		return false;

	if ((phdr->p_flags & QCOM_MDT_TYPE_MASK) == QCOM_MDT_TYPE_HASH)
		return false;

	if (!phdr->p_memsz)
		return false;

	return true;
}

/**
 * qcom_mdt_get_size() - acquire size of the memory region needed to load mdt
 * @fw:		firmware object for the mdt file
 *
 * Returns size of the loaded firmware blob, or -EINVAL on failure.
 */
ssize_t qcom_mdt_get_size(const struct firmware *fw)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_phdr *phdr;
	const struct elf32_hdr *ehdr;
	phys_addr_t min_addr = PHYS_ADDR_MAX;
	phys_addr_t max_addr = 0;
	int i;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);
	}

	return min_addr < max_addr ? max_addr - min_addr : -EINVAL;
}
EXPORT_SYMBOL_GPL(qcom_mdt_get_size);

int qcom_get_pd_segment_info(struct device *dev, const struct firmware *fw,
				phys_addr_t mem_phys, size_t mem_size, int pd)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_phdr *phdr;
	const struct elf32_hdr *ehdr;
	phys_addr_t mem_reloc;
	phys_addr_t min_addr = PHYS_ADDR_MAX;
	phys_addr_t max_addr = 0, start_addr = 0;
	size_t size = 0;
	ssize_t offset;
	bool relocate = false;
	int ret = 0;
	int i;

	if (!fw || !mem_phys || !mem_size || !pd)
		return -EINVAL;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		/*
		 * While doing PD specific reloading, load only that PD
		 * specific writeable entries. Skip others
		 */
		if ((QCOM_MDT_PF_ASID(phdr->p_flags) != pd) ||
				((phdr->p_flags & PF_W) == 0))
			continue;

		if (phdr->p_flags & QCOM_MDT_RELOCATABLE)
			relocate = true;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);
	}

	if (relocate) {
		/*
		 * The image is relocatable, so offset each segment based on
		 * the lowest segment address.
		 */
		mem_reloc = min_addr;
	} else {
		/*
		 * Image is not relocatable, so offset each segment based on
		 * the allocated physical chunk of memory.
		 */
		mem_reloc = mem_phys;
	}

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		/*
		 * While doing PD specific reloading, load only that PD
		 * specific writeable entries. Skip others
		 */
		if (QCOM_MDT_PF_ASID(phdr->p_flags) != pd)
			continue;

		if ((phdr->p_flags & PF_W) == 0)
			continue;

		offset = phdr->p_paddr - mem_reloc;
		if (offset < 0 || offset + phdr->p_memsz > mem_size) {
			dev_err(dev, "segment outside memory range\n");
			ret = -EINVAL;
			break;
		}

		if (!start_addr)
			start_addr = mem_phys + offset;
		size += phdr->p_memsz;
	}

	q6v5_wcss_store_pd_fw_info(dev, start_addr, size);
	return ret;
}
EXPORT_SYMBOL(qcom_get_pd_segment_info);

static int __qcom_mdt_load(struct device *dev, const struct firmware *fw,
			   const char *firmware, int pas_id, void *mem_region,
			   phys_addr_t mem_phys, size_t mem_size,
			   phys_addr_t *reloc_base, bool pas_init)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_phdr *phdr;
	const struct elf32_hdr *ehdr;
	const struct firmware *seg_fw;
	phys_addr_t mem_reloc;
	phys_addr_t min_addr = PHYS_ADDR_MAX;
	phys_addr_t max_addr = 0;
	size_t fw_name_len, size = 0;
	ssize_t offset;
	char *fw_name;
	bool relocate = false;
	void *ptr;
	int ret = 0;
	int i, pd, max_size = 0;
	struct device *dev_p = dev;
	dma_addr_t dma;
	int tmp = 0, blocks;
	dma_addr_t tz_dma = 0, dma_blk_arr_addr_phys = 0, dma_tmp = 0;
	u64 *dma_blk_arr_addr = NULL;
	struct region {
		u64 addr;
		unsigned blk_size;
	} *tz_addr;
	bool is_v2 = false;
	void **pt = NULL;

	if (!fw || !mem_region || !mem_phys || !mem_size)
		return -EINVAL;

	if (device_property_read_u32(dev, "qca,asid", &pd))
		pd = 0;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	fw_name_len = strlen(firmware);
	if (fw_name_len <= 4)
		return -EINVAL;

	fw_name = kstrdup(firmware, GFP_KERNEL);
	if (!fw_name)
		return -ENOMEM;

	if (pas_init && pd <= 0) {
		ret = qcom_scm_pas_init_image(pas_id, fw->data, fw->size);
		if (ret) {
			dev_err(dev, "invalid firmware metadata\n");
			goto out;
		}
	}

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		/*
		 * While doing PD specific reloading, load only that PD
		 * specific writeable entries. Skip others
		 */
		if ((pd > 0) && ((QCOM_MDT_PF_ASID(phdr->p_flags) != pd) ||
				 ((phdr->p_flags & PF_W) == 0)))
			continue;

		if (phdr->p_flags & QCOM_MDT_RELOCATABLE)
			relocate = true;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);

		if ((pd > 0) && (max_size < phdr->p_memsz))
			max_size = phdr->p_memsz;
	}

	if (pas_init && (pd > 0)) {
		is_v2 = qcom_scm_pdseg_memcpy_v2_available();

		if (is_v2) {
			blocks = max_size%PAGE_SIZE ? (max_size/PAGE_SIZE + 1) :
				(max_size/PAGE_SIZE);

			tz_addr = dma_alloc_coherent(dev, sizeof(struct region),
							&tz_dma, GFP_DMA);
			if (!tz_addr) {
				pr_err("Error in dma alloc\n");
				return -ENOMEM;
			}

			dma_blk_arr_addr = dma_alloc_coherent(dev,
					(blocks * sizeof(u64)),
					&dma_blk_arr_addr_phys, GFP_DMA);
			if (!dma_blk_arr_addr) {
				pr_err("Error in dma alloc\n");
				goto free_tz_dma_alloc;
			}
			memcpy(&tz_addr->addr, &dma_blk_arr_addr_phys,
						sizeof(dma_addr_t));

			pt = kzalloc(blocks * sizeof(void *), GFP_KERNEL);
			if (!pt) {
				pr_err("Error in memory alloc\n");
				goto free_dma_blk_arr_alloc;
			}

			for (i = 0; i < blocks; i++) {
				pt[i] = dma_alloc_coherent(dev, PAGE_SIZE,
							&dma_tmp, GFP_DMA);
				if (!pt[i]) {
					pr_err("Error in dma alloc\n");
					goto free_mem_alloc;
				}
				memcpy(&dma_blk_arr_addr[i], &dma_tmp,
							sizeof(dma_addr_t));
			}
			tz_addr->blk_size = PAGE_SIZE;
		} else {
			ptr = dma_alloc_coherent(dev, max_size, &dma,
								GFP_KERNEL);
			if (!ptr) {
				pr_err("Error in dma alloc\n");
				return -ENOMEM;
			}
		}
	}

	if (relocate) {
		/*
		 * The image is relocatable, so offset each segment based on
		 * the lowest segment address.
		 */
		mem_reloc = min_addr;
	} else {
		/*
		 * Image is not relocatable, so offset each segment based on
		 * the allocated physical chunk of memory.
		 */
		mem_reloc = mem_phys;
	}

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		/*
		 * While doing PD specific reloading, load only that PD
		 * specific writeable entries. Skip others
		 */
		if (pd > 0) {
			if (QCOM_MDT_PF_ASID(phdr->p_flags) != pd)
				continue;

			if ((phdr->p_flags & PF_W) == 0)
				continue;
		}

		offset = phdr->p_paddr - mem_reloc;
		if (offset < 0 || offset + phdr->p_memsz > mem_size) {
			dev_err(dev, "segment outside memory range\n");
			ret = -EINVAL;
			break;
		}

		if (!(pas_init && (pd > 0)))
			ptr = mem_region + offset;

		if (phdr->p_filesz) {
			snprintf(fw_name + fw_name_len - 3, (size_t)4, "b%02d", i);
			if (pd)
				dev_p = dev->parent;
			ret = request_firmware(&seg_fw, fw_name, dev_p);
			if (ret) {
				dev_err(dev, "failed to load %s\n", fw_name);
				break;
			}

			if (is_v2) {
				int offset_tmp = 0;

				size = seg_fw->size < PAGE_SIZE ?
					seg_fw->size : PAGE_SIZE;
				tmp = 0;
				while (tmp < blocks && size) {
					memset_io(pt[tmp], 0,
							PAGE_SIZE);
					memcpy_toio(pt[tmp],
						seg_fw->data + offset_tmp,
						size);
					tmp++;
					offset_tmp += size;
					if ((seg_fw->size - offset_tmp) < PAGE_SIZE)
						size = seg_fw->size - offset_tmp;
				}
			} else
				memcpy_toio(ptr, seg_fw->data, seg_fw->size);

			if (pas_init && (pd > 0))
				size = seg_fw->size;

			release_firmware(seg_fw);
		}

		if (pas_init && (pd > 0)) {
			if (is_v2)
				ret = qcom_scm_pdseg_memcpy_v2(PDSEG_PAS_ID, i,
								tz_dma, tmp);
			else
				ret = qcom_scm_pdseg_memcpy(PDSEG_PAS_ID, i,
								dma, size);
			if (ret) {
				dev_err(dev, "pd seg memcpy scm failed\n");
				break;
			}
			size = 0;
			continue;
		}

		if (phdr->p_memsz > phdr->p_filesz)
			memset(ptr + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
	}

	if (pas_init && (pd > 0)) {
		if (is_v2) {
			for (i = 0; i < blocks; i++) {
				memcpy(&dma_tmp, &dma_blk_arr_addr[i],
						sizeof(dma_addr_t));
				dma_free_coherent(dev, PAGE_SIZE, pt[i],
								dma_tmp);
			}

			dma_free_coherent(dev, (blocks * sizeof(u64)),
				dma_blk_arr_addr, dma_blk_arr_addr_phys);

			dma_free_coherent(dev, sizeof(struct region), tz_addr,
								tz_dma);
			kfree(pt);
		} else
			dma_free_coherent(dev, max_size, ptr, dma);
	}

	if (reloc_base)
		*reloc_base = mem_reloc;

out:
	kfree(fw_name);

	return ret;

free_mem_alloc:
	i = 0;
	while (i < blocks && pt[i]) {
		memcpy(&dma_tmp, &dma_blk_arr_addr[i], sizeof(dma_addr_t));
		dma_free_coherent(dev, PAGE_SIZE, pt[i], dma_tmp);
		i++;
	}
	kfree(pt);
free_dma_blk_arr_alloc:
	dma_free_coherent(dev, (blocks * sizeof(u64)), dma_blk_arr_addr,
						dma_blk_arr_addr_phys);
free_tz_dma_alloc:
	dma_free_coherent(dev, sizeof(struct region), tz_addr, tz_dma);

	return -ENOMEM;
}

/**
 * qcom_mdt_load() - load the firmware which header is loaded as fw
 * @dev:	device handle to associate resources with
 * @fw:		firmware object for the mdt file
 * @firmware:	name of the firmware, for construction of segment file names
 * @pas_id:	PAS identifier
 * @mem_region:	allocated memory region to load firmware into
 * @mem_phys:	physical address of allocated memory region
 * @mem_size:	size of the allocated memory region
 * @reloc_base:	adjusted physical address after relocation
 *
 * Returns 0 on success, negative errno otherwise.
 */
int qcom_mdt_load(struct device *dev, const struct firmware *fw,
		  const char *firmware, int pas_id, void *mem_region,
		  phys_addr_t mem_phys, size_t mem_size,
		  phys_addr_t *reloc_base)
{
	return __qcom_mdt_load(dev, fw, firmware, pas_id, mem_region, mem_phys,
			       mem_size, reloc_base, true);
}
EXPORT_SYMBOL_GPL(qcom_mdt_load);

/**
 * qcom_mdt_load_no_init() - load the firmware which header is loaded as fw
 * @dev:	device handle to associate resources with
 * @fw:		firmware object for the mdt file
 * @firmware:	name of the firmware, for construction of segment file names
 * @pas_id:	PAS identifier
 * @mem_region:	allocated memory region to load firmware into
 * @mem_phys:	physical address of allocated memory region
 * @mem_size:	size of the allocated memory region
 * @reloc_base:	adjusted physical address after relocation
 *
 * Returns 0 on success, negative errno otherwise.
 */
int qcom_mdt_load_no_init(struct device *dev, const struct firmware *fw,
			  const char *firmware, int pas_id,
			  void *mem_region, phys_addr_t mem_phys,
			  size_t mem_size, phys_addr_t *reloc_base)
{
	return __qcom_mdt_load(dev, fw, firmware, pas_id, mem_region, mem_phys,
			       mem_size, reloc_base, false);
}
EXPORT_SYMBOL_GPL(qcom_mdt_load_no_init);

MODULE_DESCRIPTION("Firmware parser for Qualcomm MDT format");
MODULE_LICENSE("GPL v2");
