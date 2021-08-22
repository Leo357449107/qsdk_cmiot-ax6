/*
 * Copyright (C) 2011 Google, Inc
 * Copyright (c) 2011-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <soc/qcom/secure_buffer.h>

DEFINE_MUTEX(secure_buffer_mutex);

static int secure_buffer_change_chunk(u32 chunks, u32 nchunks,
				      u32 chunk_size, int lock)
{
	int ret = 0;
	struct cp2_lock_req request;
	u32 resp;

	kmap_flush_unused();
	kmap_atomic_flush_unused();

	request.chunks.chunk_list = chunks;
	request.chunks.chunk_list_size = nchunks;
	request.chunks.chunk_size = chunk_size;
	/* Usage is now always 0 */
	request.mem_usage = 0;
	request.lock = lock;

	ret = qcom_scm_mem_protect_lock(&request, sizeof(request),
					&resp, sizeof(resp));

	return ret;
}

static int secure_buffer_change_table(struct sg_table *table, int lock)
{
	int i, j;
	int ret = -EINVAL;
	u32 *chunk_list;
	struct scatterlist *sg;

	for_each_sg(table->sgl, sg, table->nents, i) {
		int nchunks;
		int size = sg->length;
		int chunk_list_len;
		phys_addr_t chunk_list_phys;

		/*
		 * This should theoretically be a phys_addr_t but the protocol
		 * indicates this should be a u32.
		 */
		u32 base;
		u64 tmp = sg_dma_address(sg);
		WARN((tmp >> 32) & 0xffffffff,
			"%s: there are ones in the upper 32 bits of the sg at %p! They will be truncated! Address: 0x%llx\n",
			__func__, sg, tmp);
		if (unlikely(!size || (size % V2_CHUNK_SIZE))) {
			WARN(1,
				"%s: chunk %d has invalid size: 0x%x. Must be a multiple of 0x%x\n",
				__func__, i, size, V2_CHUNK_SIZE);
			return -EINVAL;
		}

		base = (u32)tmp;

		nchunks = size / V2_CHUNK_SIZE;
		chunk_list_len = sizeof(u32)*nchunks;

		chunk_list = kzalloc(chunk_list_len, GFP_KERNEL);

		if (!chunk_list)
			return -ENOMEM;

		chunk_list_phys = virt_to_phys(chunk_list);
		for (j = 0; j < nchunks; j++)
			chunk_list[j] = base + j * V2_CHUNK_SIZE;

		/*
		 * Flush the chunk list before sending the memory to the
		 * secure environment to ensure the data is actually present
		 * in RAM
		 */
		dmac_flush_range(chunk_list, chunk_list + chunk_list_len);

		ret = secure_buffer_change_chunk(virt_to_phys(chunk_list),
				nchunks, V2_CHUNK_SIZE, lock);

		if (!ret) {
			/*
			 * Set or clear the private page flag to communicate the
			 * status of the chunk to other entities
			 */
			if (lock)
				SetPagePrivate(sg_page(sg));
			else
				ClearPagePrivate(sg_page(sg));
		}

		kfree(chunk_list);
	}

	return ret;
}

int msm_secure_table(struct sg_table *table)
{
	int ret;

	mutex_lock(&secure_buffer_mutex);
	ret = secure_buffer_change_table(table, 1);
	mutex_unlock(&secure_buffer_mutex);

	return ret;

}

int msm_unsecure_table(struct sg_table *table)
{
	int ret;

	mutex_lock(&secure_buffer_mutex);
	ret = secure_buffer_change_table(table, 0);
	mutex_unlock(&secure_buffer_mutex);
	return ret;

}

static struct dest_vm_and_perm_info *
populate_dest_info(int *dest_vmids, int nelements, int *dest_perms,
		   size_t *size_in_bytes)
{
	struct dest_vm_and_perm_info *dest_info;
	int i;
	size_t size;

	/* Ensure allocated size is less than PAGE_ALLOC_COSTLY_ORDER */
	size = nelements * sizeof(*dest_info);
	if (size > PAGE_SIZE)
		return NULL;

	dest_info = kzalloc(size, GFP_KERNEL);
	if (!dest_info)
		return NULL;

	for (i = 0; i < nelements; i++) {
		dest_info[i].vm = dest_vmids[i];
		dest_info[i].perm = dest_perms[i];
		dest_info[i].ctx = 0x0;
		dest_info[i].ctx_size = 0;
	}

	*size_in_bytes = size;
	return dest_info;
}

/* Must hold secure_buffer_mutex while allocated buffer is in use */
unsigned int get_batches_from_sgl(struct mem_prot_info *sg_table_copy,
				  struct scatterlist *sgl,
				  struct scatterlist **next_sgl)
{
	u64 batch_size = 0;
	unsigned int i = 0;
	struct scatterlist *curr_sgl = sgl;

	/* Ensure no zero size batches */
	do {
		sg_table_copy[i].addr = page_to_phys(sg_page(curr_sgl));
		sg_table_copy[i].size = curr_sgl->length;
		batch_size += sg_table_copy[i].size;
		curr_sgl = sg_next(curr_sgl);
		i++;
	} while (curr_sgl && i < BATCH_MAX_SECTIONS &&
		 curr_sgl->length + batch_size < BATCH_MAX_SIZE);

	*next_sgl = curr_sgl;
	return i;
}
EXPORT_SYMBOL(get_batches_from_sgl);

static int batched_hyp_assign(struct sg_table *table, u32 *source_vm_copy,
			      size_t source_vm_copy_size,
			      struct dest_vm_and_perm_info *dest_vm_copy,
			      size_t dest_vm_copy_size,
			      u32 *resp, size_t resp_size)
{
	int ret = 0;
	struct mem_prot_info *sg_table_copy = kcalloc(BATCH_MAX_SECTIONS,
						      sizeof(*sg_table_copy),
						      GFP_KERNEL);

	if (!sg_table_copy)
		return -ENOMEM;

	ret = qcom_scm_mem_prot_assign(table, source_vm_copy,
				       source_vm_copy_size,
				       dest_vm_copy, dest_vm_copy_size,
				       sg_table_copy, resp, sizeof(resp));

	if (ret)
		pr_info("%s: Failed to assign memory protection, ret = %d\n",
			__func__, ret);

	kfree(sg_table_copy);
	return ret;
}

int hyp_assign_table(struct sg_table *table,
			u32 *source_vm_list, int source_nelems,
			int *dest_vmids, int *dest_perms,
			int dest_nelems)
{
	int ret = 0;
	u32 *source_vm_copy;
	size_t source_vm_copy_size;
	struct dest_vm_and_perm_info *dest_vm_copy;
	size_t dest_vm_copy_size;
	u32 resp;

	if (!table || !table->sgl || !source_vm_list || !source_nelems ||
	    !dest_vmids || !dest_perms || !dest_nelems)
		return -EINVAL;

	/*
	 * We can only pass cache-aligned sizes to hypervisor, so we need
	 * to kmalloc and memcpy the source_vm_list here.
	 */
	source_vm_copy_size = sizeof(*source_vm_copy) * source_nelems;
	source_vm_copy = kzalloc(source_vm_copy_size, GFP_KERNEL);
	if (!source_vm_copy)
		return -ENOMEM;

	memcpy(source_vm_copy, source_vm_list, source_vm_copy_size);


	dest_vm_copy = populate_dest_info(dest_vmids, dest_nelems, dest_perms,
					  &dest_vm_copy_size);
	if (!dest_vm_copy) {
		ret = -ENOMEM;
		goto out_free_source;
	}

	mutex_lock(&secure_buffer_mutex);

	ret = batched_hyp_assign(table, source_vm_copy,
				 source_vm_copy_size,
				 dest_vm_copy, dest_vm_copy_size,
				 &resp, sizeof(resp));

	mutex_unlock(&secure_buffer_mutex);
	kfree(dest_vm_copy);

out_free_source:
	kfree(source_vm_copy);
	return ret;
}

int hyp_assign_phys(phys_addr_t addr, u64 size, u32 *source_vm_list,
			int source_nelems, int *dest_vmids,
			int *dest_perms, int dest_nelems)
{
	struct sg_table table;
	int ret;

	ret = sg_alloc_table(&table, 1, GFP_KERNEL);
	if (ret)
		return ret;

	sg_set_page(table.sgl, phys_to_page(addr), size, 0);

	ret = hyp_assign_table(&table, source_vm_list, source_nelems,
			       dest_vmids, dest_perms, dest_nelems);

	sg_free_table(&table);
	return ret;
}
EXPORT_SYMBOL(hyp_assign_phys);

const char *msm_secure_vmid_to_string(int secure_vmid)
{
	switch (secure_vmid) {
	case VMID_HLOS:
		return "VMID_HLOS";
	case VMID_CP_TOUCH:
		return "VMID_CP_TOUCH";
	case VMID_CP_BITSTREAM:
		return "VMID_CP_BITSTREAM";
	case VMID_CP_PIXEL:
		return "VMID_CP_PIXEL";
	case VMID_CP_NON_PIXEL:
		return "VMID_CP_NON_PIXEL";
	case VMID_CP_CAMERA:
		return "VMID_CP_CAMERA";
	case VMID_HLOS_FREE:
		return "VMID_HLOS_FREE";
	case VMID_MSS_MSA:
		return "VMID_MSS_MSA";
	case VMID_MSS_NONMSA:
		return "VMID_MSS_NONMSA";
	case VMID_CP_SEC_DISPLAY:
		return "VMID_CP_SEC_DISPLAY";
	case VMID_CP_APP:
		return "VMID_CP_APP";
	case VMID_WLAN:
		return "VMID_WLAN";
	case VMID_WLAN_CE:
		return "VMID_WLAN_CE";
	case VMID_CP_CAMERA_PREVIEW:
		return "VMID_CP_CAMERA_PREVIEW";
	case VMID_CP_SPSS_SP_SHARED:
		return "VMID_CP_SPSS_SP_SHARED";
	case VMID_INVAL:
		return "VMID_INVAL";
	default:
		return "Unknown VMID";
	}
}

#define MAKE_CP_VERSION(major, minor, patch) \
	(((major & 0x3FF) << 22) | ((minor & 0x3FF) << 12) | (patch & 0xFFF))

bool msm_secure_v2_is_supported(void)
{
	u64 version;
	int ret = qcom_scm_get_feat_version(FEATURE_ID_CP, &version);

	/*
	 * if the version is < 1.1.0 then dynamic buffer allocation is
	 * not supported
	 */
	return (ret == 0) && (version >= MAKE_CP_VERSION(1, 1, 0));
}
