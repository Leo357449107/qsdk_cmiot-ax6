/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2014, The Linux Foundation. All rights reserved.
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

#ifndef __PLATFORM_MSM_SHARED_SMEM_H
#define __PLATFORM_MSM_SHARED_SMEM_H

#include <sys/types.h>

#define INDEX_LENGTH			2
#define SEP1_LENGTH			1
#define VERSION_STRING_LENGTH		72
#define VARIANT_STRING_LENGTH		20
#define SEP2_LENGTH			1
#define OEM_VERSION_STRING_LENGTH	32
#define BUILD_ID_LEN			32
#define ALT_PART_NAME_LENGTH 	16
#define NUM_ALT_PARTITION	16

struct per_part_info
{
	char name[ALT_PART_NAME_LENGTH];
	uint32_t primaryboot;
};

typedef struct
{
#define _SMEM_DUAL_BOOTINFO_MAGIC_START         0xA3A2A1A0
#define _SMEM_DUAL_BOOTINFO_MAGIC_END           0xB3B2B1B0
	/* Magic number for identification when reading from flash */
	uint32_t magic_start;
	/* upgradeinprogress indicates to attempting the upgrade */
	uint32_t    age;
	/* numaltpart indicate number of alt partitions */
	uint32_t    numaltpart;

	struct per_part_info per_part_entry[NUM_ALT_PARTITION];

	uint32_t magic_end;
} qca_smem_bootconfig_info_t;

extern qca_smem_bootconfig_info_t qca_smem_bootconfig_info;

int smem_bootconfig_info(void);
unsigned int get_rootfs_active_partition(void);

#define SMEM_MAX_PMIC_DEVICES           3

struct smem_proc_comm {
	unsigned command;
	unsigned status;
	unsigned data1;
	unsigned data2;
};

struct smem_heap_info {
	unsigned initialized;
	unsigned free_offset;
	unsigned heap_remaining;
	unsigned reserved;
};

struct smem_alloc_info {
	unsigned allocated;
	unsigned offset;
	unsigned size;
	unsigned reserved;
};

struct smem_board_info_v2 {
	unsigned format;
	unsigned msm_id;
	unsigned msm_version;
	char build_id[32];
	unsigned raw_msm_id;
	unsigned raw_msm_version;
};

typedef enum
{
   PMIC_IS_PM6610,
   PMIC_IS_PM6620,
   PMIC_IS_PM6640,
   PMIC_IS_PM6650,
   PMIC_IS_PM7500,
   PMIC_IS_PANORAMIX,
   PMIC_IS_PM6652,
   PMIC_IS_PM6653,
   PMIC_IS_PM6658,
   PMIC_IS_EPIC,
   PMIC_IS_HAN,
   PMIC_IS_KIP,
   PMIC_IS_WOOKIE,
   PMIC_IS_PM8058,
   PMIC_IS_PM8028,
   PMIC_IS_PM8901,
   PMIC_IS_PM8027 ,
   PMIC_IS_ISL_9519,
   PMIC_IS_PM8921,
   PMIC_IS_PM8018,
   PMIC_IS_PM8015,
   PMIC_IS_PM8014,
   PMIC_IS_PM8821,
   PMIC_IS_PM8038,
   PMIC_IS_PM8922,
   PMIC_IS_PM8917,
   PMIC_IS_INVALID = 0xffffffff,
} pm_model_type;

struct smem_board_info_v3 {
	unsigned format;
	unsigned msm_id;
	unsigned msm_version;
	char build_id[32];
	unsigned raw_msm_id;
	unsigned raw_msm_version;
	unsigned hw_platform;
};

struct smem_board_info_v4 {
	struct smem_board_info_v3 board_info_v3;
	unsigned platform_version;
	unsigned buffer_align;	//Need for 8 bytes alignment while reading from shared memory.
};

struct smem_board_info_v5 {
	struct smem_board_info_v3 board_info_v3;
	unsigned platform_version;
	unsigned fused_chip;
};

struct smem_board_info_v6 {
	struct smem_board_info_v3 board_info_v3;
	unsigned platform_version;
	unsigned fused_chip;
	unsigned platform_subtype;
	unsigned buffer_align;	//Need for 8 bytes alignment while reading from shared memory.
};

struct smem_board_info_v7 {
	struct smem_board_info_v3 board_info_v3;
	unsigned platform_version;
	unsigned fused_chip;
	unsigned platform_subtype;
	unsigned pmic_type;
	unsigned pmic_version;
	unsigned buffer_align;	//Need for 8 bytes alignment while reading from shared memory.
};

struct smem_pmic_info {
	unsigned pmic_type;
	unsigned pmic_version;
};

struct smem_board_info_v8 {
	struct smem_board_info_v3 board_info_v3;
	unsigned platform_version;
	unsigned fused_chip;
	unsigned platform_subtype;
	struct smem_pmic_info pmic_info[SMEM_MAX_PMIC_DEVICES];
	/*
	 * Need for 8 bytes alignment
	 * while reading from shared memory
	 */
	uint32_t foundry_id; /* Used as foundry_id only for v9 and used as an alignment field for v8 */
};

struct smem_board_info_v9 {
	struct smem_board_info_v3 board_info_v3;
	unsigned platform_version;
	unsigned fused_chip;
	unsigned platform_subtype;
	struct smem_pmic_info pmic_info[SMEM_MAX_PMIC_DEVICES];
	/*
	 * Need for 8 bytes alignment
	 * while reading from shared memory
	 */
	uint32_t foundry_id; /* Used as foundry_id only for v9 and used as an alignment field for v8 */
	unsigned chip_serial;
};

typedef struct {
	unsigned key_len;
	unsigned iv_len;
	unsigned char key[32];
	unsigned char iv[32];
} boot_symmetric_key_info;

typedef struct {
	unsigned int update_status;
	unsigned int bl_error_code;
} boot_ssd_status;

#if PLATFORM_MSM7X30

typedef struct {
	boot_symmetric_key_info key_info;
	uint32_t boot_flags;
	uint32_t boot_key_press[5];
	uint32_t time_tick;
	boot_ssd_status status;
	uint8_t buff_align[4];
} boot_info_for_apps;

#elif PLATFORM_MSM7K

typedef struct {
	uint32_t apps_img_start_addr;
	uint32_t boot_flags;
	boot_ssd_status status;
} boot_info_for_apps;

#elif PLATFORM_MSM7X27A

typedef struct {
	uint32_t apps_img_start_addr;
	uint32_t boot_flags;
	boot_ssd_status status;
	boot_symmetric_key_info key_info;
	uint16_t boot_key_press[10];
	uint32_t timetick;
	uint8_t PAD[28];
} boot_info_for_apps;

#else

/* Dummy structure to keep it for other targets */
typedef struct {
	uint32_t boot_flags;
	boot_ssd_status status;
} boot_info_for_apps;

#endif

/* chip information */
enum {
	UNKNOWN = 0,
	MDM9200 = 57,
	MDM9600 = 58,
	MSM8260 = 70,
	MSM8660 = 71,
	APQ8060 = 86,
	MSM8960 = 87,
	MSM7225A = 88,
	MSM7625A = 89,
	MSM7227A = 90,
	MSM7627A = 91,
	ESM7227A = 92,
	ESM7225A = 96,
	ESM7627A = 97,
	MSM7225AA = 98,
	MSM7625AA = 99,
	ESM7225AA = 100,
	MSM7227AA = 101,
	MSM7627AA = 102,
	ESM7227AA = 103,
	APQ8064 = 109,
	MSM8930 = 116,
	MSM8630 = 117,
	MSM8230 = 118,
	APQ8030 = 119,
	MSM8627 = 120,
	MSM8227 = 121,
	MSM8660A = 122,
	MSM8260A = 123,
	APQ8060A = 124,
	MSM8974 = 126,
	MSM8225 = 127,
	MSM8625 = 129,
	MPQ8064 = 130,
	MSM7225AB = 131,
	MSM7625AB = 132,
	ESM7225AB = 133,
	MDM9625   = 134,
	MSM7125A  = 135,
	MSM7127A  = 136,
	MSM8125A  = 137,
	MSM8960AB = 138,
	APQ8060AB = 139,
	MSM8260AB = 140,
	MSM8660AB = 141,
	MSM8930AA = 142,
	MSM8630AA = 143,
	MSM8230AA = 144,
	MDM9225   = 149,
	MDM9225M  = 150,
	MDM9625M  = 152,
	APQ8064AB = 153, /* aka V2 PRIME */
	MSM8930AB = 154,
	MSM8630AB = 155,
	MSM8230AB = 156,
	APQ8030AB = 157,
	APQ8030AA = 160,
	MSM8125   = 167,
	APQ8064AA = 172, /* aka V2 SLOW_PRIME */
	MSM8130   = 179,
	MSM8130AA = 180,
	MSM8130AB = 181,
	MSM8627AA = 182,
	MSM8227AA = 183,
	APQ8064AU = 244, /* aka Auto grade */
};

enum platform {
	HW_PLATFORM_UNKNOWN = 0,
	HW_PLATFORM_SURF = 1,
	HW_PLATFORM_FFA = 2,
	HW_PLATFORM_FLUID = 3,
	HW_PLATFORM_SVLTE = 4,
	HW_PLATFORM_QT = 6,
	HW_PLATFORM_MTP_MDM = 7,
	HW_PLATFORM_MTP = 8,
	HW_PLATFORM_LIQUID = 9,
	HW_PLATFORM_DRAGON = 10,
	HW_PLATFORM_QRD = 11,
	HW_PLATFORM_HRD = 13,
	HW_PLATFORM_DTV = 14,
    HW_PLATFORM_RUMI   = 15,
    HW_PLATFORM_VIRTIO = 16,
    HW_PLATFORM_OEM = 25,
	HW_PLATFORM_32BITS = 0x7FFFFFFF,
};

enum platform_subtype {
	HW_PLATFORM_SUBTYPE_UNKNOWN = 0,
	HW_PLATFORM_SUBTYPE_MDM = 1,
	HW_PLATFORM_SUBTYPE_CSFB = 1,
	HW_PLATFORM_SUBTYPE_SVLTE1 = 2,
	HW_PLATFORM_SUBTYPE_SVLTE2A = 3,
	HW_PLATFORM_SUBTYPE_SGLTE = 6,
	HW_PLATFORM_SUBTYPE_DSDA = 7,
	HW_PLATFORM_SUBTYPE_DSDA2 = 8,
	HW_PLATFORM_SUBTYPE_SGLTE2 = 9,
	HW_PLATFORM_SUBTYPE_32BITS = 0x7FFFFFFF
};

enum platform_version {
	HW_PLATFORM_VERSION_UNKNOWN = 0,
};

typedef enum {
	SMEM_SPINLOCK_ARRAY = 7,

	SMEM_AARM_PARTITION_TABLE = 9,

	SMEM_APPS_BOOT_MODE = 106,

	SMEM_BOARD_INFO_LOCATION = 137,

	SMEM_USABLE_RAM_PARTITION_TABLE = 402,

	SMEM_POWER_ON_STATUS_INFO = 403,

	SMEM_RLOCK_AREA = 404,

	SMEM_BOOT_INFO_FOR_APPS = 418,

	SMEM_BOOT_FLASH_TYPE = 421,

	SMEM_BOOT_FLASH_INDEX = 422,

	SMEM_BOOT_FLASH_CHIP_SELECT = 423,

	SMEM_BOOT_FLASH_BLOCK_SIZE = 424,

	SMEM_MACHID_INFO_LOCATION = 425,

	SMEM_IMAGE_VERSION_TABLE = 469,

	SMEM_BOOT_DUALPARTINFO = 503,

	SMEM_PARTITION_TABLE_OFFSET = 509,

	SMEM_FIRST_VALID_TYPE = SMEM_SPINLOCK_ARRAY,
	SMEM_LAST_VALID_TYPE = SMEM_PARTITION_TABLE_OFFSET,

	SMEM_MAX_SIZE = SMEM_LAST_VALID_TYPE + 1,
} smem_mem_type_t;

/* Note: buf MUST be 4byte aligned, and max_len MUST be a multiple of 4. */
unsigned smem_read_alloc_entry(smem_mem_type_t type, void *buf, int max_len);
unsigned smem_read_alloc_entry_offset(smem_mem_type_t type, void *buf, int len, int offset);
int smem_get_build_version(char *version_name, int buf_size, int index);
int ipq_smem_get_boot_version(char *version_name, int buf_size);
int ipq_get_tz_version(char *version_name, int buf_size);

/* SMEM RAM Partition */
enum {
	DEFAULT_ATTRB = ~0x0,
	READ_ONLY = 0x0,
	READWRITE,
};

enum {
	DEFAULT_CATEGORY = ~0x0,
	SMI = 0x0,
	EBI1,
	EBI2,
	QDSP6,
	IRAM,
	IMEM,
	EBI0_CS0,
	EBI0_CS1,
	EBI1_CS0,
	EBI1_CS1,
	SDRAM = 0xE,
};

enum {
	DEFAULT_DOMAIN = 0x0,
	APPS_DOMAIN,
	MODEM_DOMAIN,
	SHARED_DOMAIN,
};

enum {
	SYS_MEMORY = 1,		/* system memory */
	BOOT_REGION_MEMORY1,	/* boot loader memory 1 */
	BOOT_REGION_MEMORY2,	/* boot loader memory 2,reserved */
	APPSBL_MEMORY,		/* apps boot loader memory */
	APPS_MEMORY,		/* apps  usage memory */
};

struct smem {
	struct smem_proc_comm proc_comm[4];
	unsigned version_info[32];
	struct smem_heap_info heap_info;
	struct smem_alloc_info alloc_info[SMEM_MAX_SIZE];
};

#define _SMEM_RAM_PTABLE_MAGIC_1 0x9DA5E0A8
#define _SMEM_RAM_PTABLE_MAGIC_2 0xAF9EC4E2

struct smem_ram_ptn {
	char name[16];
	unsigned start;
	unsigned size;

	/* RAM Partition attribute: READ_ONLY, READWRITE etc.  */
	unsigned attr;

	/* RAM Partition category: EBI0, EBI1, IRAM, IMEM */
	unsigned category;

	/* RAM Partition domain: APPS, MODEM, APPS & MODEM (SHARED) etc. */
	unsigned domain;

	/* RAM Partition type: system, bootloader, appsboot, apps etc. */
	unsigned type;

	/* reserved for future expansion without changing version number */
	unsigned reserved2, reserved3, reserved4, reserved5;
} __attribute__ ((__packed__));

struct smem_ram_ptn_v1 {
	char name[16];
	unsigned long long start;
	unsigned long long size;

	/* RAM Partition attribute: READ_ONLY, READWRITE etc.  */
	unsigned attr;

	/* RAM Partition category: EBI0, EBI1, IRAM, IMEM */
	unsigned category;

	/* RAM Partition domain: APPS, MODEM, APPS & MODEM (SHARED) etc. */
	unsigned domain;

	/* RAM Partition type: system, bootloader, appsboot, apps etc. */
	unsigned type;

	/* reserved for future expansion without changing version number */
	unsigned reserved2, reserved3, reserved4, reserved5;
} __attribute__ ((__packed__));


struct smem_ram_ptable {
	unsigned magic[2];
	unsigned version;
	unsigned reserved1;
	unsigned len;
	struct smem_ram_ptn parts[32];
	unsigned buf;
} __attribute__ ((__packed__));

struct smem_ram_ptable_v1 {
	unsigned magic[2];
	unsigned version;
	unsigned reserved1;
	unsigned len;
	unsigned buf;
	struct smem_ram_ptn_v1 parts[32];
} __attribute__ ((__packed__));


/* Power on reason/status info */
#define PWR_ON_EVENT_RTC_ALARM 0x2
#define PWR_ON_EVENT_USB_CHG   0x20
#define PWR_ON_EVENT_WALL_CHG  0x40

#define SMEM_PTABLE_MAX_PARTS_V3  16
#define SMEM_PTABLE_MAX_PARTS_V4  32
#define SMEM_PTABLE_MAX_PARTS     SMEM_PTABLE_MAX_PARTS_V4

#define SMEM_PTABLE_HDR_LEN    (4*sizeof(unsigned))

struct smem_ptn {
	char name[16];
	unsigned start;
	unsigned size;
	unsigned attr;
} __attribute__ ((__packed__));


struct smem_ptable {
#define _SMEM_PTABLE_MAGIC_1 0x55ee73aa
#define _SMEM_PTABLE_MAGIC_2 0xe35ebddb
	unsigned magic[2];
	unsigned version;
	unsigned len;
	struct smem_ptn parts[SMEM_PTABLE_MAX_PARTS];
} __attribute__ ((__packed__));

int smem_ram_ptable_init(struct smem_ram_ptable *smem_ram_ptable);
int smem_ram_ptable_init_v1(struct smem_ram_ptable_v1 *smem_ram_ptable);

struct smem_machid_info {
	unsigned format;
	unsigned machid;
};

struct version_entry
{
	char index[INDEX_LENGTH];
	char colon_sep1[SEP1_LENGTH];
	char version_string[VERSION_STRING_LENGTH];
	char variant_string[VARIANT_STRING_LENGTH];
	char colon_sep2[SEP2_LENGTH];
	char oem_version_string[OEM_VERSION_STRING_LENGTH];
};

#ifdef CONFIG_SMEM_VER12

#define SMEM_COMMON_HOST	0xFFFE
#define CONFIG_QCA_SMEM_BASE 	0x4AA00000
#define SZ_4K   		0x00001000
#define MAX_ERRNO		4095

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

enum {
	smem_enu_sucess,
	smem_enu_failed,
	smem_enu_no_init
};

/**
 * struct smem_ptable_entry - one entry in the @smem_ptable list
 * @offset:	offset, within the main shared memory region, of the partition
 * @size:	size of the partition
 * @flags:	flags for the partition (currently unused)
 * @host0:	first processor/host with access to this partition
 * @host1:	second processor/host with access to this partition
 * @reserved:	reserved entries for later use
 */
struct smem_ptable_entry {
	unsigned int offset;
	unsigned int size;
	unsigned int flags;
	unsigned short host0;
	unsigned short host1;
	unsigned int reserved[8];
};

/**
 * struct smem_private_ptable - partition table for the private partitions
 * @magic:      magic number, must be SMEM_PTABLE_MAGIC
 * @version:    version of the partition table
 * @num_entries: number of partitions in the table
 * @reserved:   for now reserved entries
 * @entry:      list of @smem_ptable_entry for the @num_entries partitions
 */
struct smem_private_ptable {
	unsigned char magic[4];
	unsigned int version;
	unsigned int num_entries;
	unsigned int reserved[5];
	struct smem_ptable_entry entry[];
};

/**
 * struct smem_private_entry - header of each item in the private partition
 * @canary:     magic number, must be SMEM_PRIVATE_CANARY
 * @item:       identifying number of the smem item
 * @size:       size of the data, including padding bytes
 * @padding_data: number of bytes of padding of data
 * @padding_hdr: number of bytes of padding between the header and the data
 * @reserved:   for now reserved entry
 */
struct smem_private_entry {
	unsigned short canary; /* bytes are the same so no swapping needed */
	unsigned short item;
	unsigned int size; /* includes padding bytes */
	unsigned short padding_data;
	unsigned short padding_hdr;
	unsigned int reserved;
};

#define SMEM_PRIVATE_CANARY	0xa5a5

static const unsigned char SMEM_PTABLE_MAGIC[] = { 0x24, 0x54, 0x4f, 0x43 }; /* "$TOC" */
/**
 * struct smem_partition_header - header of the partitions
 * @magic:	magic number, must be SMEM_PART_MAGIC
 * @host0:	first processor/host with access to this partition
 * @host1:	second processor/host with access to this partition
 * @size:	size of the partition
 * @offset_free_uncached: offset to the first free byte of uncached memory in
 *		this partition
 * @offset_free_cached: offset to the first free byte of cached memory in this
 *		partition
 * @reserved:	for now reserved entries
 */
struct smem_partition_header {
	unsigned char magic[4];
	unsigned short host0;
	unsigned short host1;
	unsigned int size;
	unsigned int offset_free_uncached;
	unsigned int offset_free_cached;
	unsigned int reserved[3];
};

static const unsigned char SMEM_PART_MAGIC[] = { 0x24, 0x50, 0x52, 0x54 };		/*$PRT*/
#endif

#endif				/* __PLATFORM_MSM_SHARED_SMEM_H */
