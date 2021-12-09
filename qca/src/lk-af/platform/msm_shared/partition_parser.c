/* Copyright (c) 2011-2012, 2015 The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <crc32.h>
#include <app.h>
#include "mmc.h"
#include "mmc_wrapper.h"
#include "partition_parser.h"

#define GPT_HEADER_SIZE 92
#define GPT_LBA 1
#define PARTITION_ENTRY_SIZE 128
static bool flashing_gpt = 0;
static bool parse_secondary_gpt = 0;

char *ext3_partitions[] =
    { "system", "userdata", "persist", "cache", "tombstones" };
char *vfat_partitions[] = { "modem", "mdm", "NONE" };

unsigned int ext3_count = 0;
unsigned int vfat_count = 0;

struct partition_entry *partition_entries;
unsigned gpt_partitions_exist = 0;
unsigned partition_count = 0;

extern struct mmc_boot_card *get_mmc_card(void);

//TODO: Remove the dependency of mmc in these functions
unsigned int
partition_read_table(struct mmc_boot_host *mmc_host,
		     struct mmc_boot_card *mmc_card)
{
	unsigned int ret;

	/* Allocate partition entries array */
	partition_entries = (struct partition_entry *) calloc(NUM_PARTITIONS, 
			     sizeof(struct partition_entry));
	if (partition_entries == NULL) {
		dprintf(CRITICAL, "%s: calloc failed for partition entries\n",
				__func__);
		return -1;
	}

	/* Read MBR of the card */
	ret = mmc_boot_read_mbr(mmc_host, mmc_card);
	if (ret != MMC_BOOT_E_SUCCESS) {
		dprintf(CRITICAL, "MMC Boot: MBR read failed!\n");
		return MMC_BOOT_E_FAILURE;
	}

	/* Read GPT of the card if exist */
	if (gpt_partitions_exist) {
		ret = mmc_boot_read_gpt(mmc_host, mmc_card);
		if (ret != MMC_BOOT_E_SUCCESS) {
			dprintf(CRITICAL, "MMC Boot: GPT read failed!\n");
			return MMC_BOOT_E_FAILURE;
		}
	}
	return MMC_BOOT_E_SUCCESS;
}

/*
 * Read MBR from MMC card and fill partition table.
 */
unsigned int
mmc_boot_read_mbr(struct mmc_boot_host *mmc_host,
		  struct mmc_boot_card *mmc_card)
{
	unsigned char *buffer;
	unsigned int dtype;
	unsigned int dfirstsec;
	unsigned int EBR_first_sec;
	unsigned int EBR_current_sec;
	int ret = MMC_BOOT_E_SUCCESS;
	int idx, i;

	buffer = memalign(BLOCK_SIZE, ROUNDUP(BLOCK_SIZE, CACHE_LINE));
	if (!buffer)
		return MMC_BOOT_E_MEM_ALLOC_FAIL;

	/* Print out the MBR first */
	ret = mmc_boot_read_from_card(mmc_host, mmc_card, 0,
				      BLOCK_SIZE, (unsigned int *)buffer);
	if (ret) {
		dprintf(CRITICAL, "Could not read partition from mmc\n");
		goto err_out;
	}

	/* Check to see if signature exists */
	ret = partition_verify_mbr_signature(BLOCK_SIZE, buffer);
	if (ret) {
		goto err_out;
	}

	/*
	 * Process each of the four partitions in the MBR by reading the table
	 * information into our mbr table.
	 */
	partition_count = 0;
	idx = TABLE_ENTRY_0;
	for (i = 0; i < 4; i++) {
		/* Type 0xEE indicates end of MBR and GPT partitions exist */
		dtype = buffer[idx + i * TABLE_ENTRY_SIZE + OFFSET_TYPE];
		if (dtype == MBR_PROTECTED_TYPE) {
			gpt_partitions_exist = 1;
			goto err_out;
		}
		partition_entries[partition_count].dtype = dtype;
		partition_entries[partition_count].attribute_flag =
		    buffer[idx + i * TABLE_ENTRY_SIZE + OFFSET_STATUS];
		partition_entries[partition_count].first_lba =
		    GET_LWORD_FROM_BYTE(&buffer[idx +
						i * TABLE_ENTRY_SIZE +
						OFFSET_FIRST_SEC]);
		partition_entries[partition_count].size =
		    GET_LWORD_FROM_BYTE(&buffer[idx +
						i * TABLE_ENTRY_SIZE +
						OFFSET_SIZE]);
		dfirstsec = partition_entries[partition_count].first_lba;
		mbr_fill_name(&partition_entries[partition_count],
			      partition_entries[partition_count].dtype);
		partition_count++;
		if (partition_count == NUM_PARTITIONS)
			goto err_out;
	}

	/* See if the last partition is EBR, if not, parsing is done */
	if (dtype != MBR_EBR_TYPE) {
		goto err_out;
	}

	EBR_first_sec = dfirstsec;
	EBR_current_sec = dfirstsec;

	ret = mmc_boot_read_from_card(mmc_host, mmc_card,
				      (EBR_first_sec * 512),
				      BLOCK_SIZE, (unsigned int *)buffer);
	if (ret) {
		goto err_out;
	}
	/* Loop to parse the EBR */
	for (i = 0;; i++) {
		ret = partition_verify_mbr_signature(BLOCK_SIZE, buffer);
		if (ret) {
			ret = MMC_BOOT_E_SUCCESS;
			break;
		}
		partition_entries[partition_count].attribute_flag =
		    buffer[TABLE_ENTRY_0 + OFFSET_STATUS];
		partition_entries[partition_count].dtype =
		    buffer[TABLE_ENTRY_0 + OFFSET_TYPE];
		partition_entries[partition_count].first_lba =
		    GET_LWORD_FROM_BYTE(&buffer[TABLE_ENTRY_0 +
						OFFSET_FIRST_SEC]) +
		    EBR_current_sec;
		partition_entries[partition_count].size =
		    GET_LWORD_FROM_BYTE(&buffer[TABLE_ENTRY_0 + OFFSET_SIZE]);
		mbr_fill_name(&(partition_entries[partition_count]),
			      partition_entries[partition_count].dtype);
		partition_count++;
		if (partition_count == NUM_PARTITIONS)
			goto err_out;

		dfirstsec =
		    GET_LWORD_FROM_BYTE(&buffer
					[TABLE_ENTRY_1 + OFFSET_FIRST_SEC]);
		if (dfirstsec == 0) {
			/* Getting to the end of the EBR tables */
			break;
		}
		/* More EBR to follow - read in the next EBR sector */
		dprintf(SPEW, "Reading EBR block from 0x%X\n", EBR_first_sec
			+ dfirstsec);
		ret = mmc_boot_read_from_card(mmc_host, mmc_card,
					      ((EBR_first_sec +
						dfirstsec) * 512), BLOCK_SIZE,
					      (unsigned int *)buffer);
		if (ret) {
			goto err_out;
		}
		EBR_current_sec = EBR_first_sec + dfirstsec;
	}
err_out:
	free(buffer);
	return ret;
}

/*
 * Read GPT from MMC and fill partition table
 */
unsigned int
mmc_boot_read_gpt(struct mmc_boot_host *mmc_host,
		  struct mmc_boot_card *mmc_card)
{

	int ret = MMC_BOOT_E_SUCCESS;
	unsigned int header_size;
	unsigned long long first_usable_lba;
	unsigned long long backup_header_lba;
	unsigned long long card_size_sec;
	unsigned int max_partition_count = 0;
	unsigned int partition_entry_size;
	unsigned char *data;
	unsigned int i = 0;	/* Counter for each 512 block */
	unsigned int j = 0;	/* Counter for each 128 entry in the 512 block */
	unsigned int n = 0;	/* Counter for UTF-16 -> 8 conversion */
	unsigned char UTF16_name[MAX_GPT_NAME_SIZE];
	/* LBA of first partition -- 1 Block after Protected MBR + 1 for PT */
	unsigned long long partition_0;
	partition_count = 0;

	data = memalign(BLOCK_SIZE, ROUNDUP(BLOCK_SIZE, CACHE_LINE));
	if (!data)
		return MMC_BOOT_E_MEM_ALLOC_FAIL;
	/* Print out the GPT first */
	ret = mmc_boot_read_from_card(mmc_host, mmc_card,
				      PROTECTIVE_MBR_SIZE,
				      BLOCK_SIZE, (unsigned int *)data);
	if (ret){
		dprintf(CRITICAL, "GPT: Could not read primary gpt from mmc\n");
	}

	ret = partition_parse_gpt_header(data, &first_usable_lba,
					 &partition_entry_size, &header_size,
					 &max_partition_count);
	if (ret) {
		dprintf(INFO, "GPT: (WARNING) Primary signature invalid\n");

		/* Check the backup gpt */

		/* Get size of MMC */
		card_size_sec = (mmc_card->capacity) / BLOCK_SIZE;
		ASSERT (card_size_sec > 0);

		backup_header_lba = card_size_sec - 1;
		ret =
		    mmc_boot_read_from_card(mmc_host, mmc_card,
					    (backup_header_lba * BLOCK_SIZE),
					    BLOCK_SIZE, (unsigned int *)data);

		if (ret) {
			dprintf(CRITICAL,
				"GPT: Could not read backup gpt from mmc\n");
			goto err_out;
		}
		parse_secondary_gpt = 1;
		ret = partition_parse_gpt_header(data, &first_usable_lba,
						 &partition_entry_size,
						 &header_size,
						 &max_partition_count);
		if (ret) {
			dprintf(CRITICAL,
				"GPT: Primary and backup signatures invalid\n");
			goto err_out;
		}
		parse_secondary_gpt = 0;
	}
	partition_0 = GET_LLWORD_FROM_BYTE(&data[PARTITION_ENTRIES_OFFSET]);
	/* Read GPT Entries */
	for (i = 0; i < (max_partition_count / 4); i++) {
		ASSERT(partition_count < NUM_PARTITIONS);
		ret = mmc_boot_read_from_card(mmc_host, mmc_card,
					      (partition_0 * BLOCK_SIZE) +
					      (i * BLOCK_SIZE),
					      BLOCK_SIZE, (uint32_t *) data);

		if (ret) {
			dprintf(CRITICAL,
				"GPT: mmc read card failed reading partition entries.\n");
			goto err_out;
		}

		for (j = 0; j < 4; j++) {
			memcpy(&(partition_entries[partition_count].type_guid),
			       &data[(j * partition_entry_size)],
			       PARTITION_TYPE_GUID_SIZE);
			if (partition_entries[partition_count].type_guid[0] ==
			    0x00
			    && partition_entries[partition_count].
			    type_guid[1] == 0x00) {
				i = max_partition_count;
				break;
			}
			memcpy(&
			       (partition_entries[partition_count].
				unique_partition_guid),
			       &data[(j * partition_entry_size) +
				     UNIQUE_GUID_OFFSET],
			       UNIQUE_PARTITION_GUID_SIZE);
			partition_entries[partition_count].first_lba =
			    GET_LLWORD_FROM_BYTE(&data
						 [(j * partition_entry_size) +
						  FIRST_LBA_OFFSET]);
			partition_entries[partition_count].last_lba =
			    GET_LLWORD_FROM_BYTE(&data
						 [(j * partition_entry_size) +
						  LAST_LBA_OFFSET]);
			partition_entries[partition_count].size =
			    partition_entries[partition_count].last_lba -
			    partition_entries[partition_count].first_lba + 1;
			partition_entries[partition_count].attribute_flag =
			    GET_LLWORD_FROM_BYTE(&data
						 [(j * partition_entry_size) +
						  ATTRIBUTE_FLAG_OFFSET]);

			memset(&UTF16_name, 0x00, MAX_GPT_NAME_SIZE);
			memcpy(UTF16_name, &data[(j * partition_entry_size) +
						 PARTITION_NAME_OFFSET],
			       MAX_GPT_NAME_SIZE);
			/*
			 * Currently partition names in *.xml are UTF-8 and lowercase
			 * Only supporting english for now so removing 2nd byte of UTF-16
			 */
			for (n = 0; n < MAX_GPT_NAME_SIZE / 2; n++) {
				partition_entries[partition_count].name[n] =
				    UTF16_name[n * 2];
			}
			partition_count++;
		}
	}
err_out:
	free(data);
	return ret;
}

/*
 * uuid_bin_to_str() - convert big endian binary data to string UUID or GUID.
 * @param uuid_bin - pointer to binary data of UUID (big endian) [16B]
 * @param uuid_str - pointer to allocated array for output string [37B]
 */
void uuid_bin_to_str(unsigned char *uuid_bin, char *uuid_str)
{
	const unsigned char guid_char_order[UUID_BIN_LEN] = {3, 2, 1, 0, 5, 4, 7, 6, 8,
						9, 10, 11, 12, 13, 14, 15};
	const unsigned char *char_order;
	int i;

	/*
	 * UUID and GUID bin data - always in big endian:
	 * 4B-2B-2B-2B-6B
	 * be be be be be
	 */
	char_order = guid_char_order;

	for (i = 0; i < 16; i++) {
		sprintf(uuid_str, "%02x", uuid_bin[char_order[i]]);
		uuid_str += 2;
		switch (i) {
		case 3:
		case 5:
		case 7:
		case 9:
			*uuid_str++ = '-';
			break;
		}
	}
}

int get_partition_info_efi(int part_no, disk_partition_t *info)
{
	if (!info)
		return -1;

	if (part_no > NUM_PARTITIONS) {
		dprintf(INFO, "******invalid part number***: %d\n", part_no);
		return -1;
	}

	/* update UUID of all the partition */
	uuid_bin_to_str(partition_entries[part_no].unique_partition_guid,
			info->uuid);

	/* update name of partition */
	memcpy(info->part_name, partition_entries[part_no].name,
			UNIQUE_PARTITION_GUID_SIZE);

	return 0;
}


int get_partition_info_efi_by_name(const char *name, disk_partition_t *info)
{
	int ret;
	int i;
	for (i = 1; i < NUM_PARTITIONS; i++) {
		ret = get_partition_info_efi(i, info);
		if (ret != 0) {
			/* no more entries in table */
			return -1;
		}
		if (strcmp(name, (const char *)info->part_name) == 0) {
			/* matched */
			return 0;
		}
	}
	return -2;
}

static unsigned int write_mbr_in_blocks(unsigned size, unsigned char *mbrImage)
{
	unsigned int dtype;
	unsigned int dfirstsec;
	unsigned int ebrSectorOffset;
	unsigned char *ebrImage;
	unsigned char *lastAddress;
	int idx, i;
	unsigned int ret;

	/* Write the first block */
	ret = mmc_write(0, BLOCK_SIZE, (unsigned int *)mbrImage);
	if (ret) {
		dprintf(CRITICAL, "Failed to write mbr partition\n");
		goto end;
	}
	dprintf(SPEW, "write of first MBR block ok\n");
	/*
	   Loop through the MBR table to see if there is an EBR.
	   If found, then figure out where to write the first EBR
	 */
	idx = TABLE_ENTRY_0;
	for (i = 0; i < 4; i++) {
		dtype = mbrImage[idx + i * TABLE_ENTRY_SIZE + OFFSET_TYPE];
		if (MBR_EBR_TYPE == dtype) {
			dprintf(SPEW, "EBR found.\n");
			break;
		}
	}
	if (MBR_EBR_TYPE != dtype) {
		dprintf(SPEW, "No EBR in this image\n");
		goto end;
	}
	/* EBR exists.  Write each EBR block to mmc */
	ebrImage = mbrImage + BLOCK_SIZE;
	ebrSectorOffset =
	    GET_LWORD_FROM_BYTE(&mbrImage
				[idx + i * TABLE_ENTRY_SIZE +
				 OFFSET_FIRST_SEC]);
	dfirstsec = 0;
	dprintf(SPEW, "first EBR to be written at sector 0x%X\n", dfirstsec);
	lastAddress = mbrImage + size;
	while ((ebrImage + BLOCK_SIZE) < lastAddress) {
		dprintf(SPEW, "writing to 0x%X\n",
			(ebrSectorOffset + dfirstsec) * BLOCK_SIZE);
		ret =
		    mmc_write((ebrSectorOffset + dfirstsec) * BLOCK_SIZE,
			      BLOCK_SIZE, (unsigned int *)ebrImage);
		if (ret) {
			dprintf(CRITICAL,
				"Failed to write EBR block to sector 0x%X\n",
				dfirstsec);
			goto end;
		}
		dfirstsec =
		    GET_LWORD_FROM_BYTE(&ebrImage
					[TABLE_ENTRY_1 + OFFSET_FIRST_SEC]);
		ebrImage += BLOCK_SIZE;
	}
	dprintf(INFO, "MBR written to mmc successfully\n");
 end:
	return ret;
}

/* Write the MBR/EBR to the MMC. */
unsigned int
write_mbr(unsigned size, unsigned char *mbrImage,
	  struct mmc_boot_host *mmc_host, struct mmc_boot_card *mmc_card)
{
	unsigned int ret;

	/* Verify that passed in block is a valid MBR */
	ret = partition_verify_mbr_signature(size, mbrImage);
	if (ret) {
		goto end;
	}

	/* Write the MBR/EBR to mmc */
	ret = write_mbr_in_blocks(size, mbrImage);
	if (ret) {
		dprintf(CRITICAL, "Failed to write MBR block to mmc.\n");
		goto end;
	}
	/* Re-read the MBR partition into mbr table */
	ret = mmc_boot_read_mbr(mmc_host, mmc_card);
	if (ret) {
		dprintf(CRITICAL, "Failed to re-read mbr partition.\n");
		goto end;
	}
	partition_dump();
 end:
	return ret;
}

/*
 * A8h reflected is 15h, i.e. 10101000 <--> 00010101
*/
int reflect(int data, int len)
{
	int ref = 0;

	for (int i = 0; i < len; i++) {
		if (data & 0x1) {
			ref |= (1 << ((len - 1) - i));
		}
		data = (data >> 1);
	}

	return ref;
}

/*
* Function to calculate the CRC32
*/
unsigned int calculate_crc32(unsigned char *buffer, int len)
{
	int byte_length = 8;	/*length of unit (i.e. byte) */
	int msb = 0;
	int polynomial = (int)0x104C11DB7;	/* IEEE 32bit polynomial */
	unsigned int regs = 0xFFFFFFFF;	/* init to all ones */
	int regs_mask = 0xFFFFFFFF;	/* ensure only 32 bit answer */
	int regs_msb = 0;
	unsigned int reflected_regs;

	for (int i = 0; i < len; i++) {
		int data_byte = buffer[i];
		data_byte = reflect(data_byte, 8);
		for (int j = 0; j < byte_length; j++) {
			msb = data_byte >> (byte_length - 1);	/* get MSB */
			msb &= 1;	/* ensure just 1 bit */
			regs_msb = (regs >> 31) & 1;	/* MSB of regs */
			regs = regs << 1;	/* shift regs for CRC-CCITT */
			if (regs_msb ^ msb) {	/* MSB is a 1 */
				regs = regs ^ polynomial;	/* XOR with generator poly */
			}
			regs = regs & regs_mask;	/* Mask off excess upper bits */
			data_byte <<= 1;	/* get to next bit */
		}
	}
	regs = regs & regs_mask;
	reflected_regs = reflect(regs, 32) ^ 0xFFFFFFFF;

	return reflected_regs;
}

/*
 * Write the GPT Partition Entry Array to the MMC.
 */
static unsigned int
write_gpt_partition_array(unsigned char *header,
			  unsigned int partition_array_start,
			  unsigned int array_size)
{
	unsigned int ret = MMC_BOOT_E_INVAL;
	unsigned long long partition_entry_lba;
	unsigned long long partition_entry_array_start_location;

	partition_entry_lba =
	    GET_LLWORD_FROM_BYTE(&header[PARTITION_ENTRIES_OFFSET]);
	partition_entry_array_start_location = partition_entry_lba * BLOCK_SIZE;

	ret = mmc_write(partition_entry_array_start_location, array_size,
			(unsigned int *)partition_array_start);
	if (ret) {
		dprintf(CRITICAL,
			"GPT: FAILED to write the partition entry array\n");
		goto end;
	}

 end:
	return ret;
}

static void
patch_gpt(unsigned char *gptImage,
	  struct mmc_boot_card *mmc_card,
	  unsigned int array_size,
	  unsigned int max_part_count, unsigned int part_entry_size)
{
	unsigned char *partition_entry_array_start;
	unsigned char *primary_gpt_header;
	unsigned char *secondary_gpt_header;
	unsigned int offset;
	unsigned long long card_size_sec;
	int total_part = 0;
	unsigned int last_part_offset;
	unsigned int crc_value;

	/* Get size of MMC */
	card_size_sec = (mmc_card->capacity) / 512;
	/* Working around cap at 4GB */
	if (card_size_sec == 0) {
		card_size_sec = 4 * 1024 * 1024 * 2 - 1;
	}

	/* Patching primary header */
	primary_gpt_header = (gptImage + PROTECTIVE_MBR_SIZE);
	PUT_LONG_LONG(primary_gpt_header + BACKUP_HEADER_OFFSET,
		      ((long long)(card_size_sec - 1)));
	PUT_LONG_LONG(primary_gpt_header + LAST_USABLE_LBA_OFFSET,
		      ((long long)(card_size_sec - 34)));

	/* Patching backup GPT */
	offset = (2 * array_size);
	secondary_gpt_header = offset + BLOCK_SIZE + primary_gpt_header;
	PUT_LONG_LONG(secondary_gpt_header + PRIMARY_HEADER_OFFSET,
		      ((long long)(card_size_sec - 1)));
	PUT_LONG_LONG(secondary_gpt_header + LAST_USABLE_LBA_OFFSET,
		      ((long long)(card_size_sec - 34)));
	PUT_LONG_LONG(secondary_gpt_header + PARTITION_ENTRIES_OFFSET,
		      ((long long)(card_size_sec - 33)));

	/* Find last partition */
	while (*(primary_gpt_header + BLOCK_SIZE + total_part * ENTRY_SIZE) !=
	       0) {
		total_part++;
	}

	/* Patching last partition */
	last_part_offset =
	    (total_part - 1) * ENTRY_SIZE + PARTITION_ENTRY_LAST_LBA;
	PUT_LONG_LONG(primary_gpt_header + BLOCK_SIZE + last_part_offset,
		      (long long)(card_size_sec - 34));
	PUT_LONG_LONG(primary_gpt_header + BLOCK_SIZE + last_part_offset +
		      array_size, (long long)(card_size_sec - 34));

	/* Updating CRC of the Partition entry array in both headers */
	partition_entry_array_start = primary_gpt_header + BLOCK_SIZE;
	crc_value = calculate_crc32(partition_entry_array_start,
				    max_part_count * part_entry_size);
	PUT_LONG(primary_gpt_header + PARTITION_CRC_OFFSET, crc_value);

	crc_value = calculate_crc32(partition_entry_array_start + array_size,
				    max_part_count * part_entry_size);
	PUT_LONG(secondary_gpt_header + PARTITION_CRC_OFFSET, crc_value);

	/* Clearing CRC fields to calculate */
	PUT_LONG(primary_gpt_header + HEADER_CRC_OFFSET, 0);
	crc_value = calculate_crc32(primary_gpt_header, 92);
	PUT_LONG(primary_gpt_header + HEADER_CRC_OFFSET, crc_value);

	PUT_LONG(secondary_gpt_header + HEADER_CRC_OFFSET, 0);
	crc_value = (calculate_crc32(secondary_gpt_header, 92));
	PUT_LONG(secondary_gpt_header + HEADER_CRC_OFFSET, crc_value);

}

/*
 * Write the GPT to the MMC.
 */
unsigned int
write_gpt(unsigned size, unsigned char *gptImage,
	  struct mmc_boot_host *mmc_host, struct mmc_boot_card *mmc_card)
{
	unsigned int ret = MMC_BOOT_E_INVAL;
	unsigned int header_size;
	unsigned long long first_usable_lba;
	unsigned long long backup_header_lba;
	unsigned int max_partition_count = 0;
	unsigned int partition_entry_size;
	unsigned char *partition_entry_array_start;
	unsigned char *primary_gpt_header;
	unsigned char *secondary_gpt_header;
	unsigned int offset;
	unsigned int partition_entry_array_size;
	unsigned long long primary_header_location;	/* address on the emmc card */
	unsigned long long secondary_header_location;	/* address on the emmc card */

	/* Verify that passed block has a valid GPT primary header */
	primary_gpt_header = (gptImage + PROTECTIVE_MBR_SIZE);
	ret = partition_parse_gpt_header(primary_gpt_header, &first_usable_lba,
					 &partition_entry_size, &header_size,
					 &max_partition_count);
	if (ret) {
		dprintf(CRITICAL,
			"GPT: Primary signature invalid cannot write GPT\n");
		goto end;
	}

	/* Verify that passed block has a valid backup GPT HEADER */
	partition_entry_array_size = partition_entry_size * max_partition_count;
	if (partition_entry_array_size < MIN_PARTITION_ARRAY_SIZE) {
		partition_entry_array_size = MIN_PARTITION_ARRAY_SIZE;
	}
	offset = (2 * partition_entry_array_size);

	secondary_gpt_header = offset + BLOCK_SIZE + primary_gpt_header;
	parse_secondary_gpt = 1;
	ret =
	    partition_parse_gpt_header(secondary_gpt_header, &first_usable_lba,
				       &partition_entry_size, &header_size,
				       &max_partition_count);
	parse_secondary_gpt = 0;
	if (ret) {
		dprintf(CRITICAL,
			"GPT: Backup signature invalid cannot write GPT\n");
		goto end;
	}

	/* Patching the primary and the backup header of the GPT table */
	patch_gpt(gptImage, mmc_card, partition_entry_array_size,
		  max_partition_count, partition_entry_size);

	/* Erasing the eMMC card before writing */
	ret = mmc_erase_card(0x00000000, mmc_card->capacity);
	if (ret) {
		dprintf(CRITICAL, "Failed to erase the eMMC card\n");
		goto end;
	}

	/* Writing protective MBR */
	ret = mmc_write(0, PROTECTIVE_MBR_SIZE, (unsigned int *)gptImage);
	if (ret) {
		dprintf(CRITICAL, "Failed to write Protective MBR\n");
		goto end;
	}
	/* Writing the primary GPT header */
	primary_header_location = PROTECTIVE_MBR_SIZE;
	ret = mmc_write(primary_header_location, BLOCK_SIZE,
			(unsigned int *)primary_gpt_header);
	if (ret) {
		dprintf(CRITICAL, "Failed to write GPT header\n");
		goto end;
	}

	/* Writing the backup GPT header */
	backup_header_lba = GET_LLWORD_FROM_BYTE
	    (&primary_gpt_header[BACKUP_HEADER_OFFSET]);
	secondary_header_location = backup_header_lba * BLOCK_SIZE;
	ret = mmc_write(secondary_header_location, BLOCK_SIZE,
			(unsigned int *)secondary_gpt_header);
	if (ret) {
		dprintf(CRITICAL, "Failed to write GPT backup header\n");
		goto end;
	}

	/* Writing the partition entries array for the primary header */
	partition_entry_array_start = primary_gpt_header + BLOCK_SIZE;
	ret = write_gpt_partition_array(primary_gpt_header,
					(unsigned int)partition_entry_array_start,
					partition_entry_array_size);
	if (ret) {
		dprintf(CRITICAL,
			"GPT: Could not write GPT Partition entries array\n");
		goto end;
	}

	/*Writing the partition entries array for the backup header */
	partition_entry_array_start = primary_gpt_header + BLOCK_SIZE +
	    partition_entry_array_size;
	ret = write_gpt_partition_array(secondary_gpt_header,
					(unsigned int)partition_entry_array_start,
					partition_entry_array_size);
	if (ret) {
		dprintf(CRITICAL,
			"GPT: Could not write GPT Partition entries array\n");
		goto end;
	}

	/* Re-read the GPT partition table */
	dprintf(INFO, "Re-reading the GPT Partition Table\n");
	ret = mmc_boot_read_gpt(mmc_host, mmc_card);
	if (ret) {
		dprintf(CRITICAL,
			"GPT: Failure to re- read the GPT Partition table\n");
		goto end;
	}
	flashing_gpt = 0;

	partition_dump();
	dprintf(CRITICAL, "GPT: Partition Table written\n");
	memset(primary_gpt_header, 0x00, size);

 end:
	return ret;
}

unsigned int write_partition(unsigned size, unsigned char *partition)
{
	unsigned int ret = MMC_BOOT_E_INVAL;
	unsigned int partition_type;
	struct mmc_boot_host *mmc_host;
	struct mmc_boot_card *mmc_card;

	if (partition == 0) {
		dprintf(CRITICAL, "NULL partition\n");
		goto end;
	}

	ret = partition_get_type(size, partition, &partition_type);
	if (ret != MMC_BOOT_E_SUCCESS) {
		goto end;
	}

	mmc_host = get_mmc_host();
	mmc_card = get_mmc_card();

	switch (partition_type) {
	case PARTITION_TYPE_MBR:
		dprintf(INFO, "Writing MBR partition\n");
		ret = write_mbr(size, partition, mmc_host, mmc_card);
		break;

	case PARTITION_TYPE_GPT:
		dprintf(INFO, "Writing GPT partition\n");

		flashing_gpt = 1;
		ret = write_gpt(size, partition, mmc_host, mmc_card);
		dprintf(CRITICAL, "Re-Flash all the partitions\n");
		break;

	default:
		dprintf(CRITICAL, "Invalid partition\n");
		ret = MMC_BOOT_E_INVAL;
		goto end;
	}

 end:
	return ret;
}

/*
 * Fill name for android partition found.
 */
static void
mbr_fill_name(struct partition_entry *partition_ent, unsigned int type)
{
	memset(partition_ent->name, 0, MAX_GPT_NAME_SIZE);
	switch (type) {
	case MBR_MODEM_TYPE:
	case MBR_MODEM_TYPE2:
		/* if already assigned last name available then return */
		if (!strcmp((const char *)vfat_partitions[vfat_count], "NONE"))
			return;
		strlcpy((char *)partition_ent->name,
			(const char *)vfat_partitions[vfat_count],
			sizeof(partition_ent->name));
		vfat_count++;
		break;
	case MBR_SBL1_TYPE:
		memcpy(partition_ent->name, "sbl1", 4);
		break;
	case MBR_SBL2_TYPE:
		memcpy(partition_ent->name, "sbl2", 4);
		break;
	case MBR_SBL3_TYPE:
		memcpy(partition_ent->name, "sbl3", 4);
		break;
	case MBR_RPM_TYPE:
		memcpy(partition_ent->name, "rpm", 3);
		break;
	case MBR_TZ_TYPE:
		memcpy(partition_ent->name, "tz", 2);
		break;
	case MBR_ABOOT_TYPE:
#if PLATFORM_MSM7X27A
		memcpy(partition_ent->name, "FOTA", 4);
#else
		memcpy(partition_ent->name, "aboot", 5);
#endif
		break;
	case MBR_BOOT_TYPE:
		memcpy(partition_ent->name, "boot", 4);
		break;
	case MBR_MODEM_ST1_TYPE:
		memcpy(partition_ent->name, "modem_st1", 9);
		break;
	case MBR_MODEM_ST2_TYPE:
		memcpy(partition_ent->name, "modem_st2", 9);
		break;
	case MBR_EFS2_TYPE:
		memcpy(partition_ent->name, "efs2", 4);
		break;
	case MBR_USERDATA_TYPE:
		if (ext3_count == sizeof(ext3_partitions) / sizeof(char *))
			return;
		strlcpy((char *)partition_ent->name,
			(const char *)ext3_partitions[ext3_count],
			sizeof(partition_ent->name));
		ext3_count++;
		break;
	case MBR_RECOVERY_TYPE:
		memcpy(partition_ent->name, "recovery", 8);
		break;
	case MBR_MISC_TYPE:
		memcpy(partition_ent->name, "misc", 4);
		break;
	case MBR_SSD_TYPE:
		memcpy(partition_ent->name, "ssd", 3);
		break;
	};
}

/*
 * Find index of parition in array of partition entries
 */
unsigned partition_get_index(const char *name)
{
	unsigned int input_string_length = strlen(name);
	unsigned n;

	if( partition_count >= NUM_PARTITIONS)
	{
		return INVALID_PTN;
	}
	for (n = 0; n < partition_count; n++) {
		if ((input_string_length == strlen((const char *)&partition_entries[n].name))
			&& !memcmp(name, &partition_entries[n].name, input_string_length)) {
			return n;
		}
	}
	return INVALID_PTN;
}

/* Get size of the partition */
unsigned long long partition_get_size(int index)
{
	if (index == INVALID_PTN)
		return 0;
	else {
		return partition_entries[index].size * BLOCK_SIZE;
	}
}

/* Get offset of the partition */
unsigned long long partition_get_offset(int index)
{
	if (index == INVALID_PTN)
		return 0;
	else {
		return partition_entries[index].first_lba * BLOCK_SIZE;
	}
}

/* Debug: Print all parsed partitions */
void partition_dump()
{
	unsigned i = 0;
	for (i = 0; i < partition_count; i++) {
		dprintf(SPEW,
			"ptn[%d]:Name[%s] Size[%llu] Type[%u] First[%llu] Last[%llu]\n",
			i, partition_entries[i].name, partition_entries[i].size,
			partition_entries[i].dtype,
			partition_entries[i].first_lba,
			partition_entries[i].last_lba);
	}
}

unsigned int
partition_verify_mbr_signature(unsigned size, unsigned char *buffer)
{
	/* Avoid checking past end of buffer */
	if ((TABLE_SIGNATURE + 1) > size) {
		return MMC_BOOT_E_FAILURE;
	}
	/* Check to see if signature exists */
	if ((buffer[TABLE_SIGNATURE] != MMC_MBR_SIGNATURE_BYTE_0) ||
	    (buffer[TABLE_SIGNATURE + 1] != MMC_MBR_SIGNATURE_BYTE_1)) {
		dprintf(CRITICAL, "MBR signature does not match.\n");
		return MMC_BOOT_E_FAILURE;
	}
	return MMC_BOOT_E_SUCCESS;
}

unsigned int
mbr_partition_get_type(unsigned size, unsigned char *partition,
		       unsigned int *partition_type)
{
	unsigned int type_offset = TABLE_ENTRY_0 + OFFSET_TYPE;

	if (size < (type_offset + sizeof (*partition_type))) {
		goto end;
	}

	*partition_type = partition[type_offset];
 end:
	return MMC_BOOT_E_SUCCESS;
}

unsigned int
partition_get_type(unsigned size, unsigned char *partition,
		   unsigned int *partition_type)
{
	unsigned int ret = MMC_BOOT_E_SUCCESS;

	/*
	 * If the block contains the MBR signature, then it's likely either
	 * MBR or MBR with protective type (GPT).  If the MBR signature is
	 * not there, then it could be the GPT backup.
	 */

	/* First check the MBR signature */
	ret = partition_verify_mbr_signature(size, partition);
	if (ret == MMC_BOOT_E_SUCCESS) {
		unsigned int mbr_partition_type = PARTITION_TYPE_MBR;

		/* MBR signature verified.  This could be MBR, MBR + EBR, or GPT */
		ret =
		    mbr_partition_get_type(size, partition,
					   &mbr_partition_type);
		if (ret != MMC_BOOT_E_SUCCESS) {
			dprintf(CRITICAL, "Cannot get TYPE of partition");
		} else if (MBR_PROTECTED_TYPE == mbr_partition_type) {
			*partition_type = PARTITION_TYPE_GPT;
		} else {
			*partition_type = PARTITION_TYPE_MBR;
		}
	} else {
		/*
		 * This could be the GPT backup.  Make that assumption for now.
		 * Anybody who treats the block as GPT backup should check the
		 * signature.
		 */
		*partition_type = PARTITION_TYPE_GPT_BACKUP;
	}
	return ret;
}

/*
 * Parse the gpt header and get the required header fields
 * Return 0 on valid signature
 */
unsigned int
partition_parse_gpt_header(unsigned char *buffer,
			   unsigned long long *first_usable_lba,
			   unsigned int *partition_entry_size,
			   unsigned int *header_size,
			   unsigned int *max_partition_count)
{
	uint32_t crc_val_org = 0;
	uint32_t crc_val = 0;
	uint32_t ret = 0;
	uint32_t partitions_for_block = 0;
	uint32_t blocks_to_read = 0;
	unsigned char *new_buffer = NULL;
	unsigned long long last_usable_lba = 0;
	unsigned long long partition_0 = 0;
	unsigned long long current_lba = 0;

	/* Get the density of the mmc device */
	uint64_t device_density = mmc_get_device_capacity();

	/* Check GPT Signature */
	if (((uint32_t *) buffer)[0] != GPT_SIGNATURE_2 ||
	    ((uint32_t *) buffer)[1] != GPT_SIGNATURE_1)
		return 1;

	*header_size = GET_LWORD_FROM_BYTE(&buffer[HEADER_SIZE_OFFSET]);
	/*check for header size too small*/
	if (*header_size < GPT_HEADER_SIZE) {
		dprintf(CRITICAL,"GPT Header size is too small\n");
		return 1;
	}
	/*check for header size too large*/
	if (*header_size > BLOCK_SIZE) {
		dprintf(CRITICAL,"GPT Header size is too large\n");
		return 1;
	}

	crc_val_org = GET_LWORD_FROM_BYTE(&buffer[HEADER_CRC_OFFSET]);
	/*Write CRC to 0 before we calculate the crc of the GPT header*/
	crc_val = 0;
	PUT_LONG(&buffer[HEADER_CRC_OFFSET], crc_val);

	crc_val  = crc32(~0L,buffer, *header_size) ^ (~0L);
	if (crc_val != crc_val_org) {
		dprintf(CRITICAL,"Header crc mismatch crc_val = %u with crc_val_org = %u\n", crc_val,crc_val_org);
		return 1;
	}
	else
		PUT_LONG(&buffer[HEADER_CRC_OFFSET], crc_val);

	current_lba =
	    GET_LLWORD_FROM_BYTE(&buffer[PRIMARY_HEADER_OFFSET]);
	*first_usable_lba =
	    GET_LLWORD_FROM_BYTE(&buffer[FIRST_USABLE_LBA_OFFSET]);
	*max_partition_count =
	    GET_LWORD_FROM_BYTE(&buffer[PARTITION_COUNT_OFFSET]);
	*partition_entry_size =
	    GET_LWORD_FROM_BYTE(&buffer[PENTRY_SIZE_OFFSET]);
	last_usable_lba =
	    GET_LLWORD_FROM_BYTE(&buffer[LAST_USABLE_LBA_OFFSET]);

	/*current lba and GPT lba should be same*/
	if (!parse_secondary_gpt) {
		if (current_lba != GPT_LBA) {
			dprintf(CRITICAL,"GPT first usable LBA mismatch\n");
			return 1;
		}
	}
	/*check for first lba should be with in the valid range*/
	if (*first_usable_lba > (device_density/BLOCK_SIZE)) {
		dprintf(CRITICAL,"Invalid first_usable_lba\n");
		return 1;
	}
	/*check for last lba should be with in the valid range*/
	if (last_usable_lba > (device_density/BLOCK_SIZE)) {
		dprintf(CRITICAL,"Invalid last_usable_lba\n");
		return 1;
	}
	/*check for partition entry size*/
	if (*partition_entry_size != PARTITION_ENTRY_SIZE) {
		dprintf(CRITICAL,"Invalid parition entry size\n");
		return 1;
	}

	if ((*max_partition_count) > (MIN_PARTITION_ARRAY_SIZE /(*partition_entry_size))) {
		dprintf(CRITICAL, "Invalid maximum partition count\n");
		return 1;
	}

	partitions_for_block = BLOCK_SIZE / (*partition_entry_size);

	blocks_to_read = (*max_partition_count)/ partitions_for_block;
	if ((*max_partition_count) % partitions_for_block) {
		blocks_to_read += 1;
	}

	new_buffer = (uint8_t *)memalign(BLOCK_SIZE,
					ROUNDUP((blocks_to_read * BLOCK_SIZE),
					CACHE_LINE));

	if (!new_buffer)
	{
		dprintf(CRITICAL, "Failed to Allocate memory to read partition table\n");
		return 1;
	}

	if (!flashing_gpt) {
		partition_0 = GET_LLWORD_FROM_BYTE(&buffer[PARTITION_ENTRIES_OFFSET]);
		/*start LBA should always be 2 in primary GPT*/
		if(partition_0 != 0x2) {
			dprintf(CRITICAL, "Starting LBA mismatch\n");
			goto fail;

		}
		/*read the partition entries to new_buffer*/
		ret = mmc_read((partition_0) * (BLOCK_SIZE), (unsigned int *)new_buffer, (blocks_to_read * BLOCK_SIZE));
		if (ret)
		{
			dprintf(CRITICAL, "GPT: Could not read primary gpt from mmc\n");
			goto fail;
		}
		crc_val_org = GET_LWORD_FROM_BYTE(&buffer[PARTITION_CRC_OFFSET]);

		crc_val  = crc32(~0L,new_buffer, ((*max_partition_count) * (*partition_entry_size))) ^ (~0L);
		if (crc_val != crc_val_org) {
			dprintf(CRITICAL,"Partition entires crc mismatch crc_val= %u with crc_val_org= %u\n",crc_val,crc_val_org);
			ret = 1;
		}
	}
fail:
	free(new_buffer);
	return ret;
}

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

static int cmd_part(int argc, const cmd_args *argv)
{
	unsigned i;

	printf("No.: Type   Start      End        Size       Name\n");

	for (i = 0; i < partition_count; i++) {
		printf("%3d: 0x%04x 0x%08llx 0x%08llx 0x%08llx %s\n", i,
			partition_entries[i].dtype,
			partition_entries[i].first_lba,
			partition_entries[i].last_lba,
			partition_entries[i].size,
			partition_entries[i].name);
	}

	return 0;
}

STATIC_COMMAND_START
        { "part", "Print partition table", &cmd_part },
STATIC_COMMAND_END(part);
#endif /* defined(WITH_LIB_CONSOLE) */
