/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <reg.h>
#include <sys/types.h>
#include <platform/iomap.h>
#include <scm.h>
#include <board.h>
#include <smem.h>
#include <string.h>
#include <err.h>
#include <endian.h>

#include "smem.h"

qca_smem_bootconfig_info_t qca_smem_bootconfig_info;

#ifdef CONFIG_SMEM_VER12
struct smem_partition_header *smem_cmn_partition;
unsigned char smem_enumeration_status = smem_enu_no_init;

/* Pointer to the one and only smem handle */

static struct smem_private_entry *
phdr_to_first_private_entry(struct smem_partition_header *phdr)
{
	void *p = phdr;

	return p + sizeof(*phdr);
}

static struct smem_private_entry *
phdr_to_last_private_entry(struct smem_partition_header *phdr)
{
	void *p = phdr;

	return p + LE32(phdr->offset_free_uncached);
}

static struct smem_private_entry *
private_entry_next(struct smem_private_entry *e)
{
	void *p = e;

	return p + sizeof(*e) + LE16(e->padding_hdr) +
				LE32(e->size);
}

static void *entry_to_item(struct smem_private_entry *e)
{
	void *p = e;

	return p + sizeof(*e) + LE16(e->padding_hdr);
}

static void *qti_smem_get_private(struct smem_partition_header *phdr,
							unsigned item,
							size_t *size)
{
	struct smem_private_entry *e, *end;

	e = phdr_to_first_private_entry(phdr);
	end = phdr_to_last_private_entry(phdr);

	while (e < end) {
		if (e->canary != SMEM_PRIVATE_CANARY) {
			return ERR_PTR(ERR_NOT_VALID);
		}
		if (LE16(e->item) == item) {
			if (size != NULL)
				*size = LE32(e->size) -
					LE16(e->padding_data);

			return entry_to_item(e);
		}

		e = private_entry_next(e);
	}
	return ERR_PTR(ERR_NOT_FOUND);
}

static int qti_smem_enumerate_partitions(void)
{
	struct smem_partition_header *header;
	struct smem_ptable_entry *entry;
	struct smem_private_ptable *ptable;
	unsigned int version, host0, host1;
	unsigned int i;

	ptable = (struct smem_private_ptable*) (MSM_SHARED_BASE + \
					CONFIG_QCA_SMEM_SIZE - SZ_4K);
	if (memcmp(ptable->magic, SMEM_PTABLE_MAGIC, sizeof(ptable->magic)))
		return ERR_NOT_VALID;

	version = LE32(ptable->version);
	if (version != 1) {
		return ERR_NOT_VALID;
	}
	for (i = 0; i < LE32(ptable->num_entries); i++) {
		entry = &ptable->entry[i];
		host0 = LE16(entry->host0);
		host1 = LE16(entry->host1);

		if (host0 != SMEM_COMMON_HOST || host1 != SMEM_COMMON_HOST)
			continue;

		if (!LE32(entry->offset))
			continue;

		if (!LE32(entry->size))
			continue;

		if (smem_cmn_partition) {
			return ERR_NOT_VALID;
		}

		header = (struct smem_partition_header*) (CONFIG_QCA_SMEM_BASE + \
							LE32(entry->offset));
		host0 = LE16(header->host0);
		host1 = LE16(header->host1);

		if (memcmp(header->magic, SMEM_PART_MAGIC,
			    sizeof(header->magic))) {
			return ERR_NOT_VALID;
		}

		if (host0 != SMEM_COMMON_HOST || host1 != SMEM_COMMON_HOST) {
			return ERR_NOT_VALID;
		}

		if (header->size != entry->size) {
			return ERR_NOT_VALID;
		}

		if (LE32(header->offset_free_uncached) > LE32(header->size)) {
			return ERR_NOT_VALID;
		}

		smem_cmn_partition = header;
	}
	smem_enumeration_status = smem_enu_sucess;
	return 0;
}

unsigned smem_read_alloc_entry(smem_mem_type_t type, void *buf, int len)
{
	return smem_read_alloc_entry_offset(type, buf, len, 0);
}

unsigned
smem_read_alloc_entry_offset(smem_mem_type_t type, void *buf, int len,
			     int offset)
{
	unsigned *dest = buf;
	unsigned size;
	void *ptr;

	if (((len & 0x3) != 0) || (((unsigned)buf & 0x3) != 0))
		return 1;

	if (type < SMEM_FIRST_VALID_TYPE || type > SMEM_LAST_VALID_TYPE)
		return 1;

	if(smem_enumeration_status == smem_enu_no_init) {
		if (qti_smem_enumerate_partitions()) {
			smem_enumeration_status = smem_enu_failed;
			return 1;
		}
	}

        if ( smem_enumeration_status == smem_enu_sucess) {
		ptr = qti_smem_get_private(smem_cmn_partition,type, &size);
		if (IS_ERR(ptr)) {
			return 1;
		}
		memcpy(dest, ptr + offset, len);
		return 0;
	}
	return 1;
}
#else

static struct smem *smem = (void *)(MSM_SHARED_BASE);

/* buf MUST be 4byte aligned, and len MUST be a multiple of 8. */
unsigned smem_read_alloc_entry(smem_mem_type_t type, void *buf, int len)
{
	struct smem_alloc_info *ainfo;
	unsigned *dest = buf;
	unsigned src;
	unsigned size;

	if (((len & 0x3) != 0) || (((unsigned)buf & 0x3) != 0))
		return 1;

	if (type < SMEM_FIRST_VALID_TYPE || type > SMEM_LAST_VALID_TYPE)
		return 1;

	/* TODO: Use smem spinlocks */
	ainfo = &smem->alloc_info[type];
	if (readl(&ainfo->allocated) == 0)
		return 1;

	size = readl(&ainfo->size);
	if (size != (unsigned)((len + 7) & ~0x00000007))
		return 1;

	src = MSM_SHARED_BASE + readl(&ainfo->offset);
	for (; len > 0; src += 4, len -= 4)
		*(dest++) = readl(src);

	return 0;
}

unsigned
smem_read_alloc_entry_offset(smem_mem_type_t type, void *buf, int len,
			     int offset)
{
	struct smem_alloc_info *ainfo;
	unsigned *dest = buf;
	unsigned src;
	unsigned size = len;

	if (((len & 0x3) != 0) || (((unsigned)buf & 0x3) != 0))
		return 1;

	if (type < SMEM_FIRST_VALID_TYPE || type > SMEM_LAST_VALID_TYPE)
		return 1;

	ainfo = &smem->alloc_info[type];
	if (readl(&ainfo->allocated) == 0)
		return 1;

	src = MSM_SHARED_BASE + readl(&ainfo->offset) + offset;
	for (; size > 0; src += 4, size -= 4)
		*(dest++) = readl(src);

	return 0;
}
#endif

int smem_get_build_version(char *version_name, int buf_size, int index)
{
	int ret;
	struct version_entry version_entry[32];

	ret = smem_read_alloc_entry(SMEM_IMAGE_VERSION_TABLE,
				    &version_entry, sizeof(version_entry));
	if (ret != 0)
		return 1;

	snprintf(version_name, buf_size, "%s-%s\n",
			version_entry[index].oem_version_string,
			version_entry[index].version_string);

	return ret;
}

int ipq_smem_get_boot_version(char *version_name, int buf_size)
{
	int ret;
	struct smem_board_info_v6 board_info_v6;

	ret = smem_read_alloc_entry(SMEM_BOARD_INFO_LOCATION,
				    &board_info_v6, sizeof(board_info_v6));
	if (ret != 0)
		return 1;

	snprintf(version_name, buf_size, "%s\n", board_info_v6.board_info_v3.build_id);
	return ret;
}

int ipq_get_tz_version(char *version_name, int buf_size)
{
	int ret;

	ret = scm_call(SCM_SVC_INFO, TZBSP_BUILD_VER_QUERY_CMD, NULL,
					0, version_name, BUILD_ID_LEN);
	if (ret != 0)
		return 1;

	snprintf(version_name, buf_size, "%s\n", version_name);
	return ret;
}

/*
 * smem_bootconfig_info - retrieve bootconfig flags
 */
int smem_bootconfig_info(void)
{
	unsigned ret;
	ret = smem_read_alloc_entry(SMEM_BOOT_DUALPARTINFO,
			&qca_smem_bootconfig_info, sizeof(qca_smem_bootconfig_info_t));

	if ((ret != 0) ||
		(qca_smem_bootconfig_info.magic_start != _SMEM_DUAL_BOOTINFO_MAGIC_START) ||
		(qca_smem_bootconfig_info.magic_end != _SMEM_DUAL_BOOTINFO_MAGIC_END))
			return ERR_ENOMSG;

	return 0;
}

unsigned int get_rootfs_active_partition(void)
{
	unsigned int i;

	for (i = 0; i < qca_smem_bootconfig_info.numaltpart; i++) {
		if (strncmp("rootfs", qca_smem_bootconfig_info.per_part_entry[i].name,
					ALT_PART_NAME_LENGTH) == 0)
			return qca_smem_bootconfig_info.per_part_entry[i].primaryboot;
	}

	return 0; /* alt partition not available */
}
