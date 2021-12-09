/*
 * Copyright (c) 2015 The Linux Foundation. All rights reserved.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <common.h>
#include <asm/global_data.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <spi_flash.h>
#include <asm/errno.h>
#include "spi_nand_dev.h"
#include <malloc.h>
#include "spi.h"
#include "sf_internal.h"
#include <watchdog.h>

#define CONFIG_SF_DEFAULT_SPEED		(48 * 1000 * 1000)
#define TIMEOUT		5000
#define MFID_GIGA	0xc8
#define MFID_ATO	0x9b
#define TOSHIBA_NORM_READ_MASK	0x1F

/* Macronix Specific Defines */
#define MFID_MACRONIX   	0xc2
#define MACRONIX_WRAP		((0 & 0x3) << 6)
#define MACRONIX_PLANE		((0 & 0x1) << 4)

#define MACRONIX_NORM_READ_MASK	(MACRONIX_WRAP | MACRONIX_PLANE | 0x0F)

/* Fidelix Specific Defines */
#define SET_CMD_PLANE_BIT(plane)	(plane << 4)
/* Introduce the 6th bit in the page address and insert the plane ID */
#define INSERT_PLANE_BIT(x, plane)	(((x << 1) & 0xFFFFFF80) | (x & 0x3F) \
								| (plane << 6))

#define GIGADEVICE_NORM_READ_MASK 0x0F

#define spi_print(...)  printf("spi_nand: " __VA_ARGS__)

DECLARE_GLOBAL_DATA_PTR;

struct nand_chip nand_chip[CONFIG_SYS_MAX_NAND_DEVICE];
int verify_3bit_ecc(int status);
int verify_2bit_ecc(int status);
int verify_dummy_ecc(int status);
int verify_2bit_toshiba_ecc(int status);
void gigadevice_norm_read_cmd(u8 *cmd, int column);
void gigadevice_norm_read_cmd_12bit_addr(u8 *cmd, int column);
void macronix_norm_read_cmd(u8 *cmd, int column);
void winbond_norm_read_cmd(u8 *cmd, int column);
void toshiba_norm_read_cmd(u8 *cmd, int column);
void fidelix_norm_read_cmd(u8 *cmd, int column);

int spi_nand_die_select(struct mtd_info *mtd, struct spi_flash *flash,
			int die_id);

#define mtd_to_ipq_info(m)	((struct nand_chip *)((m)->priv))->priv

static struct spi_nand_flash_params spi_nand_flash_tbl[] = {
	{
		.id = { 0xc8, 0xb1, 0x48, 0xc8 },
		.page_size = 2048,
		.erase_size = 0x00020000,
		.no_of_dies = 1,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x10000,
		.pages_per_sector = 64,
		.nr_sectors = 1024,
		.oob_size = 128,
		.protec_bpx = 0xC7,
		.norm_read_cmd = gigadevice_norm_read_cmd,
		.verify_ecc = verify_3bit_ecc,
		.die_select = NULL,
		.name = "GD5F1GQ4XC",
	},
	{
		.id = { 0xc8, 0xc1, 0xc8, 0xc1 },
		.page_size = 2048,
		.erase_size = 0x00020000,
		.no_of_dies = 1,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x10000,
		.pages_per_sector = 64,
		.nr_sectors = 1024,
		.oob_size = 128,
		.protec_bpx = 0xC7,
		.norm_read_cmd = gigadevice_norm_read_cmd_12bit_addr,
		.verify_ecc = verify_3bit_ecc,
		.die_select = NULL,
		.name = "GD5F1GQ4R",
	},
	{
		.id = { 0xc8, 0xb4, 0x68, 0xc8 },
		.page_size = 4096,
		.erase_size = 0x00040000,
		.no_of_dies = 1,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x20000,
		.pages_per_sector = 64,
		.nr_sectors = 2048,
		.oob_size = 256,
		.protec_bpx = 0xC7,
		.norm_read_cmd = gigadevice_norm_read_cmd,
		.verify_ecc = verify_3bit_ecc,
		.die_select = NULL,
		.name = "GD5F4GQ4XC",
	},
	{
		.id = { 0xff, 0x9b, 0x12 , 0x9b },
		.page_size = 2048,
		.erase_size = 0x00020000,
		.no_of_dies = 1,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x10000,
		.pages_per_sector = 64,
		.nr_sectors = 1024,
		.oob_size = 64,
		.protec_bpx = 0xC7,
		.norm_read_cmd = gigadevice_norm_read_cmd,
		.verify_ecc = verify_dummy_ecc,
		.die_select = NULL,
		.name = "ATO25D1GA",
	},
	{
		.id = { 0x00, 0xc2, 0x12, 0xc2 },
		.page_size = 2048,
		.erase_size = 0x00020000,
		.no_of_dies = 1,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x10000,
		.pages_per_sector = 64,
		.nr_sectors = 1024,
		.oob_size = 64,
		.protec_bpx = 0xC7,
		.norm_read_cmd = macronix_norm_read_cmd,
		.verify_ecc = verify_2bit_ecc,
		.die_select = NULL,
		.name = "MX35LFxGE4AB",
	},
	{
		.id = { 0x00, 0xef, 0xaa, 0x21 },
		.page_size = 2048,
		.erase_size = 0x00020000,
		.no_of_dies = 1,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x10000,
		.pages_per_sector = 64,
		.nr_sectors = 1024,
		.oob_size = 64,
		.protec_bpx = 0x87,
		.norm_read_cmd = winbond_norm_read_cmd,
		.verify_ecc = verify_2bit_ecc,
		.die_select = NULL,
		.name = "W25N01GV",
        },
	{
		.id = { 0x00, 0xef, 0xab, 0x21 },
		.page_size = 2048,
		.erase_size = 0x00020000,
		.no_of_dies = 2,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x10000,
		.pages_per_sector = 64,
		.nr_sectors = 2048,
		.oob_size = 64,
		.protec_bpx = 0x87,
		.norm_read_cmd = winbond_norm_read_cmd,
		.verify_ecc = verify_2bit_ecc,
		.die_select = spi_nand_die_select,
		.name = "W25M02GV",
        },
	{
		.id = { 0x00, 0x98, 0xcd, 0x98 },
		.page_size = 4096,
		.erase_size = 0x00040000,
		.no_of_dies = 1,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x20000,
		.pages_per_sector = 64,
		.nr_sectors = 2048,
		.oob_size = 128,
		.protec_bpx = 0xC7,
		.norm_read_cmd = toshiba_norm_read_cmd,
		.verify_ecc = verify_2bit_toshiba_ecc,
		.die_select = NULL,
		.name = "TC58CVG2S0F",
	},
	{
		.id = { 0x00, 0xe5, 0x72, 0xe5 },
		.page_size = 2048,
		.erase_size = 0x00020000,
		.no_of_dies = 2,
		.prev_die_id = INT_MAX,
		.plane_id = 0,
		.pages_per_die = 0x10000,
		.pages_per_sector = 64,
		.nr_sectors = 2048,
		.oob_size = 64,
		.protec_bpx = 0xC7,
		.norm_read_cmd = fidelix_norm_read_cmd,
		.verify_ecc = verify_2bit_ecc,
		.die_select = NULL,
		.name = "FM35X2GA",
        },
};

struct spi_nand_flash_params *params;
void spinand_internal_ecc(struct mtd_info *mtd, int enable);

void gigadevice_norm_read_cmd(u8 *cmd, int column)
{
	cmd[0] = IPQ40XX_SPINAND_CMD_NORM_READ;
	cmd[1] = 0;
	cmd[2] = (u8)(column >> 8);
	cmd[3] = (u8)(column);
}

void gigadevice_norm_read_cmd_12bit_addr(u8 *cmd, int column)
{
	cmd[0] = IPQ40XX_SPINAND_CMD_NORM_READ;
	cmd[1] = ((u8)(column >> 8) & GIGADEVICE_NORM_READ_MASK);
	cmd[2] = (u8)(column);
	cmd[3] = 0;
}

void macronix_norm_read_cmd(u8 *cmd, int column)
{
	cmd[0] = IPQ40XX_SPINAND_CMD_NORM_READ;
	cmd[1] = ((u8)(column >> 8) & MACRONIX_NORM_READ_MASK);
	cmd[2] = (u8)(column);
	cmd[3] = 0;
}

void fidelix_norm_read_cmd(u8 *cmd, int column)
{
	cmd[0] = IPQ40XX_SPINAND_CMD_NORM_READ;
	cmd[1] = ((u8)(column >> 8)) | SET_CMD_PLANE_BIT(params->plane_id);
	cmd[2] = (u8)(column);
	cmd[3] = 0;
}

void winbond_norm_read_cmd(u8 *cmd, int column)
{
	cmd[0] = IPQ40XX_SPINAND_CMD_NORM_READ;
	cmd[1] = (u8)(column >> 8);
	cmd[2] = (u8)(column);
	cmd[3] = 0;
}

void toshiba_norm_read_cmd(u8 *cmd, int column)
{
	cmd[0] = IPQ40XX_SPINAND_CMD_NORM_READ;
	cmd[1] = ((u8)(column >> 8) & TOSHIBA_NORM_READ_MASK);
	cmd[2] = (u8)(column);
	cmd[3] = 0;
}

int spi_nand_flash_cmd_poll_bit(struct spi_flash *flash, unsigned long timeout,
				u8 cmd, u8 poll_bit, u8 *status)
{
	struct spi_slave *spi = flash->spi;
	unsigned long timebase;
	u8 cmd_buf[2];

	cmd_buf[0] = 0x0F;
	cmd_buf[1] = cmd;

	timebase = get_timer(0);
	do {
		WATCHDOG_RESET();

		spi_flash_cmd_read(spi, cmd_buf, 2, status, 1);
		if ((*status & poll_bit) == 0)
			break;

	} while (get_timer(timebase) < timeout);

	if ((*status & poll_bit) == 0)
		return 0;

	/* Timed out */
	debug("SF: time out!\n");
	return -1;
}

int spi_nand_flash_cmd_wait_ready(struct spi_flash *flash, u8 status_bit, u8 *status,
					unsigned long timeout)
{
	return spi_nand_flash_cmd_poll_bit(flash, timeout,
				0xC0, status_bit, status);
}

static int spinand_waitfunc(struct mtd_info *mtd, u8 val, u8 *status)
{
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;
	int ret;

	ret = spi_nand_flash_cmd_wait_ready(flash, val, status, TIMEOUT);
	if (ret) {
		printf("%s Operation Timeout\n", __func__);
		return -1;
	}

	return 0;
}

static int check_offset(struct mtd_info *mtd, loff_t offs)
{
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct nand_chip *chip = info->chip;
	int ret = 0;

	/* Start address must align on block boundary */
	if (offs & ((1 << chip->phys_erase_shift) - 1)) {
		printf("%s: unaligned address\n", __func__);
		ret = -EINVAL;
	}

	return ret;
}

static int get_die_id(struct ipq40xx_spinand_info *info, u32 page)
{
	int die_id;

	die_id = page / info->params->pages_per_die;

	if (die_id > info->params->no_of_dies) {
		printf("Invalid Die ID: %d\n", die_id);
		return -EINVAL;
	}

	return die_id;
}

static int get_plane_id(struct ipq40xx_spinand_info *info, u32 page)
{
	if (info->params->no_of_dies == 2 && info->params->die_select == NULL) {
		params->plane_id = page / info->params->pages_per_die;

		if (params->plane_id > info->params->no_of_dies) {
			printf("Invalid Plane ID: %d\n", params->plane_id);
			return -EINVAL;
		}
	}
	return 0;
}

int spi_nand_die_select(struct mtd_info *mtd, struct spi_flash *flash,
			int die_id)
{
	u8 cmd[8];
	u8 status;
	int ret = 0;

	if (die_id < 0)
		return -EINVAL;

	if (params->prev_die_id == die_id)
		return ret;

	cmd[0] = IPQ40XX_SPINAND_CMD_DIESELECT;
	cmd[1] = die_id;
	ret = spi_flash_cmd_write(flash->spi, cmd, 2, NULL, 0);
	if (ret) {
		printf("%s  failed for die select :\n", __func__);
		return ret;
	}
	ret = spinand_waitfunc(mtd, 0x01, &status);
	if (ret) {
		printf("Operation timeout\n");
		return ret;
	}
	params->prev_die_id = die_id;

	return ret;
}

static inline int do_die_select(struct mtd_info *mtd, struct spi_flash *flash,
			struct ipq40xx_spinand_info *info, u32 page)
{
	u32 ret;

	if (info->params->die_select == NULL)
		return 0;

	ret = info->params->die_select(mtd, flash, get_die_id(info, page));

	return ret;
}

static int fidelix_plane_select(u32 page)
{
	if (params->no_of_dies == 2 && params->die_select == NULL) {
		if (params->plane_id)
			page = page - params->pages_per_die;
		page = INSERT_PLANE_BIT(page, params->plane_id);
	}
	return page;
}

static int spi_nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	u8 cmd[8];
	u8 status;
	u32 ret;
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;
	struct nand_chip *chip = info->chip;
	u32 page = (int)(instr->addr >> chip->page_shift);

	if (check_offset(mtd, instr->addr))
		return -1;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		printf("SF: Unable to claim SPI bus\n");
		return ret;
	}

	ret = do_die_select(mtd, flash, info, page);
	if (ret)
		goto out;

	if (get_plane_id(info, page))
		goto out;

	ret = spi_flash_cmd(flash->spi, IPQ40XX_SPINAND_CMD_WREN, NULL, 0);
	if (ret) {
		printf ("Write enable failed %s\n", __func__);
		goto out;
	}
	ret = spinand_waitfunc(mtd, 0x01, &status);
	if (ret) {
		goto out;
	}

	page = fidelix_plane_select(page);

	cmd[0] = IPQ40XX_SPINAND_CMD_ERASE;
	cmd[1] = (u8)(page >> 16);
	cmd[2] = (u8)(page >> 8);
	cmd[3] = (u8)(page);

	ret = spi_flash_cmd_write(flash->spi, cmd, 4, NULL, 0);
	if (ret) {
		printf("%s  failed for offset:%ld\n", __func__, (long)instr->addr);
		goto out;
	}
	ret = spinand_waitfunc(mtd, 0x01, &status);
	if (ret) {
		printf("Operation timeout\n");
		goto out;
	}

	if (status & STATUS_E_FAIL) {
		printf("Erase operation failed for 0x%x\n", page);
		ret = -EIO;
		goto out;
	}

	ret = spi_flash_cmd(flash->spi, IPQ40XX_SPINAND_CMD_WRDI, NULL, 0);

	ret = spinand_waitfunc(mtd, 0x01, &status);
	if (ret) {
		printf("Write disable failed\n");
	}

out:
	spi_release_bus(flash->spi);

	return ret;
}

static int spi_nand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;
	struct nand_chip *chip = info->chip;
	u8 cmd[8];
	u8 status;
	u32 value = 0xff;
	int page, column, ret;

	page = (int)(offs >> chip->page_shift) & chip->pagemask;
	column = mtd->writesize + chip->badblockpos;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		printf ("Claim bus failed. %s\n", __func__);
		return -1;
	}

	ret = do_die_select(mtd, flash, info, page);
	if (ret)
		goto out;

	if (get_plane_id(info, page))
		goto out;
	page = fidelix_plane_select(page);

	cmd[0] = IPQ40XX_SPINAND_CMD_READ;
	cmd[1] = (u8)(page >> 16);
	cmd[2] = (u8)(page >> 8);
	cmd[3] = (u8)(page);
	ret = spi_flash_cmd_write(flash->spi, cmd, 4, NULL, 0);
	if (ret) {
		printf("%s: write command failed\n", __func__);
		goto out;
	}
	ret = spinand_waitfunc(mtd, 0x01, &status);
	if (ret) {
		printf("Operation timeout\n");
		goto out;
	}

	info->params->norm_read_cmd(cmd, column);

	ret = spi_flash_cmd_read(flash->spi, cmd, 4, &value, 1);
	if (ret) {
		printf("%s: read data failed\n", __func__);
		goto out;
	}

	if (value != 0xFF) {
		ret = 1;
		goto out;
	}

out:
	spi_release_bus(flash->spi);
	return ret;
}


static int spinand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
				int page, struct mtd_oob_ops *ops)
{
	int column, ret = 0;
	u_char *wbuf;
	u8 cmd[8], status;
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;

	wbuf = chip->oob_poi;
	column = mtd->writesize;

	ret = do_die_select(mtd, flash, info, page);
	if (ret)
		goto out;

	if (get_plane_id(info, page))
		goto out;

	ret = spi_flash_cmd(flash->spi, IPQ40XX_SPINAND_CMD_WREN, NULL, 0);
	if (ret) {
		printf("Write enable failed in %s\n", __func__);
		goto out;
	}

	cmd[0] = IPQ40XX_SPINAND_CMD_PLOAD;
	cmd[1] = ((u8)(column >> 8)) | SET_CMD_PLANE_BIT(params->plane_id);
	cmd[2] = (u8)(column);

	ret = spi_flash_cmd_write(flash->spi, cmd, 3, wbuf, ops->ooblen);
	if (ret) {
		printf("%s: write command failed\n", __func__);
		ret = 1;
		goto out;
	}

	page = fidelix_plane_select(page);

	cmd[0] = IPQ40XX_SPINAND_CMD_PROG;
	cmd[1] = (u8)(page >> 16);
	cmd[2] = (u8)(page >> 8);
	cmd[3] = (u8)(page);

	ret = spi_flash_cmd_write(flash->spi, cmd, 4, NULL, 0);
	if (ret) {
		printf("PLOG failed\n");
		goto out;
	}

	ret = spinand_waitfunc(mtd, 0x01, &status);
	if (ret) {
		if (status)
			printf("Program failed\n");
	}

	ret = spi_flash_cmd(flash->spi, IPQ40XX_SPINAND_CMD_WRDI, NULL, 0);
	if (ret)
		printf("Write disable failed in %s\n", __func__);

out:
	spi_release_bus(flash->spi);
	return ret;
}

static void fill_oob_data(struct mtd_info *mtd, uint8_t *oob,
        size_t len, struct mtd_oob_ops *ops)
{
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct nand_chip *chip = info->chip;

	memset(chip->oob_poi, 0xff, mtd->oobsize);
	memcpy(chip->oob_poi + ops->ooboffs, oob, len);

	return;
}

static int spi_nand_write_oob_data(struct mtd_info *mtd, loff_t to,
			      struct mtd_oob_ops *ops)
{
	int page;
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct nand_chip *chip = info->chip;

	/* Shift to get page */
	page = (int)(to >> chip->page_shift);

	fill_oob_data(mtd, ops->oobbuf, ops->ooblen, ops);

	return spinand_write_oob_std(mtd, chip,
			page & chip->pagemask, ops);
}

static int spi_nand_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	uint8_t buf[2]= { 0, 0 };
	int ret;
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;
	struct nand_chip *chip = info->chip;
	struct mtd_oob_ops ops;
	struct erase_info einfo;

	ret = spi_nand_block_isbad(mtd, offs);
	if (ret) {
		if (ret > 0)
			return 0;
		return ret;
	}

	/* Attempt erase before marking OOB */
	memset(&einfo, 0, sizeof(einfo));
	einfo.mtd = mtd;
	einfo.addr = offs;
	einfo.len = 1 << chip->phys_erase_shift;
	spi_nand_erase(mtd, &einfo);

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		printf ("Claim bus failed. %s\n", __func__);
		return -1;
	}

	ops.datbuf = NULL;
	ops.oobbuf = buf;
	ops.ooboffs = chip->badblockpos;
	ops.len = ops.ooblen = 1;

	ret = spi_nand_write_oob_data(mtd, offs, &ops);
	if (ret)
		goto out;

out:
	spi_release_bus(flash->spi);
	return ret;
}

int verify_3bit_ecc(int status)
{
	int ecc_status = (status & SPINAND_3BIT_ECC_MASK);

	if (ecc_status == SPINAND_3BIT_ECC_ERROR)
		return ECC_ERR;
	else if (ecc_status >= SPINAND_3BIT_ECC_BF_THRESHOLD)
		return ECC_CORRECTED;
	else
		return 0;
}

int verify_2bit_ecc(int status)
{
	int ecc_status = (status & SPINAND_2BIT_ECC_MASK);

	if ((ecc_status == SPINAND_2BIT_ECC_ERROR) ||
	    (ecc_status == SPINAND_2BIT_ECC_MASK))
		return ECC_ERR;
	else if (ecc_status == SPINAND_2BIT_ECC_CORRECTED)
		return ECC_CORRECTED;
	else
		return 0;
}

int verify_2bit_toshiba_ecc(int status)
{
	int ecc_status = (status & SPINAND_2BIT_ECC_MASK);

	if (ecc_status == SPINAND_2BIT_ECC_ERROR)
		return ECC_ERR;
	else if (ecc_status == SPINAND_2BIT_ECC_CORRECTED_TOSHIBA)
		return ECC_CORRECTED;
	else
		return 0;
}

int verify_dummy_ecc(int status)
{
	return 0;
}

static int spi_nand_read_std(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;
	struct nand_chip *chip = info->chip;
	u32 ret;
	u8 cmd[8];
	u8 status;
	int realpage, page, readlen, bytes, column, bytes_oob;
	int ecc_corrected = 0;
	column = mtd->writesize;

	realpage = (int)(from >> chip->page_shift);
	page = realpage & chip->pagemask;
	readlen = ops->len;
	ret = spi_claim_bus(flash->spi);
	if (ret) {
		printf ("Claim bus failed. %s\n", __func__);
		return -1;
	}
	while (1) {
		ret = do_die_select(mtd, flash, info, page);
		if (ret)
			goto out;

		if (get_plane_id(info, page))
			goto out;
		page = fidelix_plane_select(page);

		cmd[0] = IPQ40XX_SPINAND_CMD_READ;
		cmd[1] = (u8)(page >> 16);
		cmd[2] = (u8)(page >> 8);
		cmd[3] = (u8)(page);
		ret = spi_flash_cmd_write(flash->spi, cmd, 4, NULL, 0);
		if (ret) {
			printf("%s: write command failed\n", __func__);
			goto out;
		}
		ret = spinand_waitfunc(mtd, 0x01, &status);
		if (ret) {
			goto out;
		}

		ret = info->params->verify_ecc(status);
		if (ret == ECC_ERR) {
			mtd->ecc_stats.failed++;
			printf("ecc err(0x%x) for page read\n", status);
			ret = -EBADMSG;
			goto out;
		} else if (ret == ECC_CORRECTED) {
			mtd->ecc_stats.corrected++;
			ecc_corrected = 1;
		}

		bytes = ((readlen < mtd->writesize) ? readlen : mtd->writesize);
		bytes_oob = ops->ooblen;

		/* Read Data */
		if (bytes) {
			info->params->norm_read_cmd(cmd, 0);
			ret = spi_flash_cmd_read(flash->spi, cmd, 4, ops->datbuf, bytes);
			if (ret) {
				printf("%s: read data failed\n", __func__);
				return -1;
			}
			ops->retlen += bytes;
		}

		/* Read OOB */
		if (bytes_oob) {
			info->params->norm_read_cmd(cmd, column);
			ret = spi_flash_cmd_read(flash->spi, cmd, 4, ops->oobbuf, ops->ooblen);
			if (ret) {
				printf("%s: read data failed\n", __func__);
				return -1;
			}
			ops->oobretlen += ops->ooblen;
		}

		readlen -= bytes;
		if (readlen <= 0)
			break;
		ops->datbuf += bytes;
		realpage++;
		page = realpage & chip->pagemask;
	}

	if ((ret == 0) && (ecc_corrected))
		ret = -EUCLEAN;
out:
	spi_release_bus(flash->spi);
	return ret;
}

static int spi_nand_read(struct mtd_info *mtd, loff_t from, size_t len,
			 size_t *retlen, u_char *buf)
{
	struct mtd_oob_ops ops = {0};
	u32 ret;

	ops.mode = MTD_OPS_AUTO_OOB;
	ops.len = len;
	ops.datbuf = (uint8_t *)buf;
	ret = spi_nand_read_std(mtd, from, &ops);
	*retlen = ops.retlen;
	return ret;
}

static int spi_nand_read_oob(struct mtd_info *mtd, loff_t from,
			     struct mtd_oob_ops *ops)
{
	u32 ret;

	/* Disable ECC */
	spinand_internal_ecc(mtd, 0);
	ret = spi_nand_read_std(mtd, from,ops);
	/* Enable ECC */
	spinand_internal_ecc(mtd, 1);
	return ret;
}

static int spi_nand_write_std(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;
	struct nand_chip *chip = info->chip;
	u8 cmd[8];
	u8 status;
	u32 ret;
	const u_char *wbuf;
	const u_char *buf;
	int realpage, page, bytes, write_len;
	write_len = ops->len;
	buf = ops->datbuf;
	bytes = mtd->writesize;

	/* Check whether page is aligned */
	if (((to & (mtd->writesize -1)) !=0) ||
		((ops->len & (mtd->writesize -1)) != 0)) {
		printf("Attempt to write to non page aligned data\n");
		return -EINVAL;
	}

	realpage = (int)(to >> chip->page_shift);
	page = realpage & chip->pagemask;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		printf ("Write enable failed %s\n", __func__);
		return -1;
	}
	while (1) {
		wbuf = buf;

		ret = do_die_select(mtd, flash, info, page);
		if (ret)
			goto out;

		if (get_plane_id(info, page))
			goto out;

		ret = spi_flash_cmd(flash->spi, IPQ40XX_SPINAND_CMD_WREN, NULL, 0);
		if (ret) {
			printf("Write enable failed\n");
			goto out;
                }

		/* buffer to be transmittted here */
		cmd[0] = IPQ40XX_SPINAND_CMD_PLOAD;
		cmd[1] = SET_CMD_PLANE_BIT(params->plane_id);
		cmd[2] = 0;
		ret = spi_flash_cmd_write(flash->spi, cmd, 3, wbuf, bytes);
		if (ret) {
			printf("%s: write command failed\n", __func__);
			goto out;
		}

		page = fidelix_plane_select(page);

		cmd[0] = IPQ40XX_SPINAND_CMD_PROG;
		cmd[1] = (u8)(page >> 16);
		cmd[2] = (u8)(page >> 8);
		cmd[3] = (u8)(page);

		ret = spi_flash_cmd_write(flash->spi, cmd, 4, NULL, 0);
		if (ret) {
			printf("PLOG failed\n");
			goto out;
		}

		ret = spinand_waitfunc(mtd, 0x01, &status);
		if (ret) {
			printf("Operation timeout\n");
			goto out;
		}

		if (status & STATUS_P_FAIL) {
			printf("Program failure (0x%x)\n", status);
			ret = -EIO;
			goto out;
		}

		ret = spi_flash_cmd(flash->spi, IPQ40XX_SPINAND_CMD_WRDI, NULL, 0);
		if (ret) {
			printf("Write disable failed\n");
			goto out;
		}
		if (ops->ooblen)
			ret = spi_nand_write_oob_data(mtd, to, ops);

		ops->retlen += bytes;
		write_len -= bytes;
		if (!write_len)
			break;
		buf += bytes;
		realpage++;
		page = realpage & chip->pagemask;

	}

out:
	spi_release_bus(flash->spi);
	return ret;
}

static int spi_nand_write(struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const u_char *buf)
{
	struct mtd_oob_ops ops = { 0 };
	int ret;

	ops.len = len;
	ops.datbuf = (uint8_t *)buf;
	ret = spi_nand_write_std(mtd, to, &ops);
	*retlen = ops.retlen;

	return ret;
}

static int spi_nand_write_oob(struct mtd_info *mtd, loff_t to,
				struct mtd_oob_ops *ops)
{
	int ret;

	if(ops->datbuf == NULL)
		return -EINVAL;
	spinand_internal_ecc(mtd, 0);
	ret = spi_nand_write_std(mtd, to, ops);
	spinand_internal_ecc(mtd, 1);

	return ret;
}

int spi_nand_flash_probe(struct spi_slave *spi, struct spi_flash *flash,
							u8 *idcode)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(spi_nand_flash_tbl); i++) {
		params = &spi_nand_flash_tbl[i];

		if ((params->id[0] == idcode[0]) &&
		    (params->id[1] == idcode[1]) &&
		    (params->id[2] == idcode[2]) &&
		    (params->id[3] == idcode[3])) {
			spi_print("%s SF NAND ID %x:%x:%x:%x\n",
				__func__, idcode[0], idcode[1], idcode[2], idcode[3]);
			break;
		}
	}

	if (i == ARRAY_SIZE(spi_nand_flash_tbl)) {
		return -EINVAL;
	}

	flash->name = params->name;
	flash->page_size = params->page_size;
	flash->sector_size = params->page_size;
	flash->erase_size = params->erase_size;
	flash->size = (params->page_size * params->nr_sectors * params->pages_per_sector);

	return 0;
}

static int spinand_unlock_protect(struct mtd_info *mtd)
{
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;
	int status;
	int ret;
	int i;
	u8 cmd[3];

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		printf ("Write enable failed %s %d\n", __func__, __LINE__);
		return -1;
	}

	cmd[0] = IPQ40XX_SPINAND_CMD_GETFEA;
	cmd[1] = IPQ40XX_SPINAND_PROTEC_REG;

	ret = spi_flash_cmd_read(flash->spi, cmd, 2, &status, 1);
	if (ret) {
		printf("Failed to read status register");
		goto out;
	}

	for (i = 0; i < info->params->no_of_dies; i++) {
		if (info->params->die_select != NULL) {
			ret = info->params->die_select(mtd, flash, i);
			if (ret)
				goto out;
		}

		status &= (info->params->protec_bpx);
		cmd[0] = IPQ40XX_SPINAND_CMD_SETFEA;
		cmd[1] = IPQ40XX_SPINAND_PROTEC_REG;
		cmd[2] = (u8)status;
		ret = spi_flash_cmd_write(flash->spi, cmd, 3, NULL, 0);
		if (ret) {
			printf("Failed to unblock sectors\n");
			goto out;
		}
	}
out:
	spi_release_bus(flash->spi);
	return ret;
}

void spinand_internal_ecc(struct mtd_info *mtd, int enable)
{
	struct ipq40xx_spinand_info *info = mtd_to_ipq_info(mtd);
	struct spi_flash *flash = info->flash;
	int status;
	int ret;
	int i;
	u8 cmd[3];

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		printf ("Write enable failed %s %d\n", __func__, __LINE__);
		return;
	}

	for (i = 0; i < info->params->no_of_dies; i++) {
		if (info->params->die_select != NULL) {
			ret = info->params->die_select(mtd, flash, i);
			if (ret)
				goto out;
		}

		cmd[0] = IPQ40XX_SPINAND_CMD_GETFEA;
		cmd[1] = IPQ40XX_SPINAND_FEATURE_REG;

		ret = spi_flash_cmd_read(flash->spi, cmd, 2, &status, 1);
		if (ret) {
			printf("%s: read data failed\n", __func__);
			goto out;
		}

		cmd[0] = IPQ40XX_SPINAND_CMD_SETFEA;
		cmd[1] = IPQ40XX_SPINAND_FEATURE_REG;
		if (enable) {
			cmd[2] = status | IPQ40XX_SPINAND_FEATURE_ECC_EN;
		} else {
			cmd[2] = status & ~(IPQ40XX_SPINAND_FEATURE_ECC_EN);
		}

		ret = spi_flash_cmd_write(flash->spi, cmd, 3, NULL, 0);
		if (ret) {
			printf("Internal ECC enable failed\n");
			goto out;
		}
	}
out:
	spi_release_bus(flash->spi);
	return;
}

static int spi_nand_scan_bbt_nop(struct mtd_info *mtd)
{
	return 0;
}

int spi_nand_init(void)
{
	struct spi_flash *flash;
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct ipq40xx_spinand_info *info;
	int ret;
	int node;
	u8 bus = CONFIG_SF_DEFAULT_BUS;
	u8 cs = CONFIG_SF_SPI_NAND_CS;

	info = (struct ipq40xx_spinand_info *)malloc(
			sizeof(struct ipq40xx_spinand_info));
	if (!info) {
		printf ("Error in allocating mem\n");
		return -ENOMEM;
	}
	memset(info, '0', sizeof(struct ipq40xx_spinand_info));

	node = fdt_path_offset(gd->fdt_blob, "/spi/spi_nand");
	if (node >= 0) {
		bus = fdtdec_get_uint(gd->fdt_blob,
			node, "bus-num", CONFIG_SF_DEFAULT_BUS);
		cs = fdtdec_get_uint(gd->fdt_blob,
			node, "cs", CONFIG_SF_SPI_NAND_CS);
	}
	flash = spi_flash_probe(bus, cs, CONFIG_SF_DEFAULT_SPEED,
					CONFIG_SF_DEFAULT_MODE);
	if (!flash) {
	    	free(info);
		spi_print("Id could not be mapped\n");
		return -ENODEV;
	}

	mtd = &nand_info[CONFIG_IPQ_SPI_NAND_INFO_IDX];
	chip = &nand_chip[CONFIG_IPQ_SPI_NAND_INFO_IDX];

	mtd->priv = chip;
	mtd->writesize = flash->page_size;
	mtd->writebufsize = mtd->writesize;
	mtd->erasesize = params->erase_size;
	mtd->oobsize = params->oob_size;
	mtd->size = flash->size;
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->_read = spi_nand_read;
	mtd->_write = spi_nand_write;
	mtd->_erase = spi_nand_erase;
	mtd->_read_oob = spi_nand_read_oob;
	mtd->_write_oob = spi_nand_write_oob;
	mtd->_block_isbad = spi_nand_block_isbad;
	mtd->_block_markbad = spi_nand_block_markbad;

	chip->page_shift = ffs(mtd->writesize) - 1;
	chip->phys_erase_shift = ffs(mtd->erasesize) - 1;
	chip->chipsize = flash->size;
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;
	chip->badblockpos = 0;
	chip->buffers = kmalloc(sizeof(*chip->buffers), GFP_KERNEL);
	chip->oob_poi = chip->buffers->databuf + mtd->writesize;

	/* One of the NAND layer functions that the command layer
	 * tries to access directly.
	 */
	chip->scan_bbt = spi_nand_scan_bbt_nop;

	info->params = params;
	info->flash = flash;
	info->mtd = mtd;
	info->chip = chip;
	chip->priv = info;

	if ((ret = nand_register(CONFIG_IPQ_SPI_NAND_INFO_IDX)) < 0) {
		free(info);
		spi_print("Failed to register with MTD subsystem\n");
		return ret;
	}

	ret = spinand_unlock_protect(mtd);
	if (ret) {
	    	free(info);
		printf("Failed to unlock blocks\n");
		return -1;
	}

	spinand_internal_ecc(mtd, 1);

	return 0;
}
