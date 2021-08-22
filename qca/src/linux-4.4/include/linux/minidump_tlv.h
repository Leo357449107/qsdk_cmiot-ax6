/* Copyright (c) 2019 The Linux Foundation. All rights reserved.
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
#ifndef MINIDUMP_H
#define MINIDUMP_H

#include <linux/module.h>
typedef struct qcom_wdt_scm_tlv_msg {
	unsigned char *msg_buffer;
	unsigned char *cur_msg_buffer_pos;
	unsigned int len;
	spinlock_t minidump_tlv_spinlock;
} qcom_wdt_scm_tlv_msg_t;

struct minidump_tlv_info {
	uint64_t start;
	uint64_t size;
};

/* Metadata List for bookkeeping and managing entries and invalidation of
* TLVs into the global crashdump buffer and the Metadata text file
*/
struct minidump_metadata_list {
	struct list_head list;	/*kernel’s list structure*/
	unsigned long va;		/* Virtual address of TLV. Set to 0 if invalid*/
	unsigned long pa;		/*Physical address of TLV segment*/
	unsigned long modinfo_offset; /* Offset associated with the entry for
				* module information in Metadata text file
				*/
	unsigned long size; /*size associated with TLV entry */
	unsigned char *tlv_offset;	/* Offset associated with the TLV entry in
					* the crashdump buffer
					*/
	unsigned long mmuinfo_offset; /* Offset associated with the entry for
				* mmu information in MMU Metadata text file
				*/
	unsigned char type;
	#ifdef CONFIG_QCA_MINIDUMP_DEBUG
	char *name;  /* Name associated with the TLV */
	#endif
};

#define QCOM_WDT_SCM_TLV_TYPE_SIZE	1
#define QCOM_WDT_SCM_TLV_LEN_SIZE	2
#define QCOM_WDT_SCM_TLV_TYPE_LEN_SIZE (QCOM_WDT_SCM_TLV_TYPE_SIZE + QCOM_WDT_SCM_TLV_LEN_SIZE)
#define INVALID 0

/* Module Meta Data File is currently set to 12K size
* by default, where (12K / 50) = 245 entries can be supported.
* To support max capacity of 646 entries,please modify
* METADATA_FILE_SZ from 12K to 32K.
*
* MMU Meta Data File is currently set to 12K size
* by default, where (12K / 33) = 372 entries can be supported.
* To support max capacity of 646 entries , please modify
* MMU_FILE_SZ from 12K to 21K.
*/

#define METADATA_FILE_SZ 12288
#define METADATA_FILE_ENTRY_LEN 50
#define NAME_LEN 28
#define MINIDUMP_MODULE_COUNT 4

#define MMU_FILE_SZ 12288
#define MMU_FILE_ENTRY_LEN 33

/* TLV_Types */
typedef enum {
    QCA_WDT_LOG_DUMP_TYPE_INVALID,
    QCA_WDT_LOG_DUMP_TYPE_UNAME,
    QCA_WDT_LOG_DUMP_TYPE_DMESG,
    QCA_WDT_LOG_DUMP_TYPE_LEVEL1_PT,
    QCA_WDT_LOG_DUMP_TYPE_WLAN_MOD,
    QCA_WDT_LOG_DUMP_TYPE_WLAN_MOD_DEBUGFS,
    QCA_WDT_LOG_DUMP_TYPE_WLAN_MOD_INFO,
    QCA_WDT_LOG_DUMP_TYPE_WLAN_MMU_INFO,
    QCA_WDT_LOG_DUMP_TYPE_EMPTY,
} minidump_tlv_type_t;

int minidump_fill_segments(const uint64_t start_addr, uint64_t size, minidump_tlv_type_t type, const char *name);
int minidump_store_module_info(const char *name , const unsigned long va, const unsigned long pa, minidump_tlv_type_t type);
int minidump_store_mmu_info(const unsigned long va, const unsigned long pa);
int minidump_remove_segments(const uint64_t virtual_address);
int do_minidump(void);

struct module_sect_attr {
	struct module_attribute mattr;
	char *name;
	unsigned long address;
};

struct module_sect_attrs {
	struct attribute_group grp;
	unsigned int nsections;
	struct module_sect_attr attrs[0];
};

#endif /*MINIDUMP_H*/
