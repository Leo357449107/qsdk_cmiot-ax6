/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <nand.h>
#include <malloc.h>
#include <spi_flash.h>
#include <linux/mtd/nand.h>
#include <spi.h>
#include <asm/io.h>
#include <asm/errno.h>

#define spi_print(...)	printf("ipq_spi: " __VA_ARGS__)

static int ipq_spi_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int ret;

	ret = spi_flash_erase(mtd->priv, instr->addr, instr->len);

	if (ret)
		instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;

	if (instr->callback)
		instr->callback(instr);

	return ret;
}

static int ipq_spi_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	return 0;
}

static int ipq_spi_read(struct mtd_info *mtd, loff_t from, size_t len,
			 size_t *retlen, u_char *buf)
{
	int ret;
	size_t unaligned_len;
	u_char *data;

	/* Get the unaligned no of bytes equivalent to len % mtd->erasesize */
	unaligned_len = len & (mtd->erasesize - 1);
	len = len - unaligned_len;

	*retlen = 0;

	if (len) {
		ret = spi_flash_read(mtd->priv, from, len, buf);
		if (ret)
			return 0;
		else
			*retlen = len;
	}

	if (unaligned_len) {
		data = (u_char *) malloc(mtd->erasesize);
		if (data == NULL) {
			/* retlen will have the length of the data read above */
			return 0;
		}

		from = from + len;
		ret = spi_flash_read(mtd->priv, from, mtd->erasesize, data);
		if (!ret) {
			memcpy(buf + len, data, unaligned_len);
			*retlen += unaligned_len;
		}

		free(data);
	}
	return 0;
}

static int ipq_spi_write(struct mtd_info *mtd, loff_t to, size_t len,
			  size_t *retlen, const u_char *buf)
{
	int ret;

	ret = spi_flash_write(mtd->priv, to, len, buf);
	if (ret)
		*retlen = 0;
	else
		*retlen = len;

	return 0;
}

static int ipq_spi_read_oob(struct mtd_info *mtd, loff_t from,
			     struct mtd_oob_ops *ops)
{
	printf("oobbuf = %p ooblen = %d\n", ops->oobbuf, ops->ooblen);
	if (!ops->ooblen) {
		return ipq_spi_read(mtd, from, ops->len,
			 		&ops->retlen, ops->datbuf);
	}

	return -EINVAL;
}

static int ipq_spi_write_oob(struct mtd_info *mtd, loff_t to,
			      struct mtd_oob_ops *ops)
{
	if (!ops->ooblen) {
		return ipq_spi_write(mtd, to, ops->len,
			 		&ops->retlen, ops->datbuf);
	}

	return -EINVAL;
}

static int ipq_spi_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	return -EINVAL;
}

/*
 * Initialize controller and register as an MTD device.
 */
int ipq_spi_init(u16 idx)
{
	struct spi_flash *flash;
	int ret = 0;
	struct mtd_info *mtd;

	flash = spi_flash_probe(CONFIG_SF_DEFAULT_BUS,
				CONFIG_SF_DEFAULT_CS,
				CONFIG_SF_DEFAULT_SPEED,
				CONFIG_SF_DEFAULT_MODE);

	if (!flash) {
		spi_print("SPI Flash not found (bus/cs/speed/mode) ="
			" (%d/%d/%d/%d)\n", CONFIG_SF_DEFAULT_BUS,
			CONFIG_SF_DEFAULT_CS, CONFIG_SF_DEFAULT_SPEED,
			CONFIG_SF_DEFAULT_MODE);
		return 1;
	}

	mtd = &nand_info[idx];
	mtd->priv = flash;
	mtd->writesize = flash->page_size;
	mtd->erasesize = flash->sector_size;
	mtd->oobsize = 0;
	mtd->size = flash->size;

	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;

	mtd->_erase = ipq_spi_erase;
	mtd->_read = ipq_spi_read;
	mtd->_write = ipq_spi_write;
	mtd->_read_oob = ipq_spi_read_oob;
	mtd->_write_oob = ipq_spi_write_oob;
	mtd->_block_isbad = ipq_spi_block_isbad;
	mtd->_block_markbad = ipq_spi_block_markbad;
#ifdef CONFIG_MTD_DEVICE
	if ((ret = nand_register(idx)) < 0) {
		spi_print("Failed to register with MTD subsystem\n");
		return ret;
	}
#endif
	spi_print("page_size: 0x%x, sector_size: 0x%x, size: 0x%x\n",
		flash->page_size, flash->sector_size, flash->size);

	return ret;
}
