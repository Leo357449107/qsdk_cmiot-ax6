/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_mtd.h>
#include <linux/delay.h>
#include <linux/dma/qcom_bam_dma.h>

/* NAND_FLASH_CMD bits */
#define	PAGE_ACC			BIT(4)
#define	LAST_PAGE			BIT(5)

/* NAND_FLASH_CHIP_SELECT bits */
#define	NAND_DEV_SEL			0
#define	DM_EN				BIT(2)

/* NAND_FLASH_STATUS bits */
#define	FS_OP_ERR			BIT(4)
#define	FS_READY_BSY_N			BIT(5)
#define	FS_MPU_ERR			BIT(8)
#define	FS_DEVICE_STS_ERR		BIT(16)
#define	FS_DEVICE_WP			BIT(23)
#define FS_TIMEOUT_ERR			BIT(6)
#define FLASH_ERROR			(FS_OP_ERR | FS_MPU_ERR | FS_TIMEOUT_ERR)

/* NAND_BUFFER_STATUS bits */
#define	BS_UNCORRECTABLE_BIT		BIT(8)
#define	BS_CORRECTABLE_ERR_MSK		0x1f

/* NAND_DEVn_CFG0 bits */
#define	DISABLE_STATUS_AFTER_WRITE	4
#define	CW_PER_PAGE			6
#define	UD_SIZE_BYTES			9
#define	ECC_PARITY_SIZE_BYTES_RS	19
#define	SPARE_SIZE_BYTES		23
#define	NUM_ADDR_CYCLES			27
#define	NUM_ADDR_CYCLES_MASK		0x7
#define	NUM_ADDR_CYCLES_5		5
#define	STATUS_BFR_READ			30
#define	SET_RD_MODE_AFTER_STATUS	31

/* NAND_DEVn_CFG0 bits */
#define	DEV0_CFG1_ECC_DISABLE		0
#define	WIDE_FLASH			1
#define	NAND_RECOVERY_CYCLES		2
#define	CS_ACTIVE_BSY			5
#define	BAD_BLOCK_BYTE_NUM		6
#define	BAD_BLOCK_IN_SPARE_AREA		16
#define	WR_RD_BSY_GAP			17
#define	ENABLE_BCH_ECC			27

/* NAND_DEV0_ECC_CFG bits */
#define	ECC_CFG_ECC_DISABLE		0
#define	ECC_SW_RESET			1
#define	ECC_MODE			4
#define	ECC_PARITY_SIZE_BYTES_BCH	8
#define	ECC_NUM_DATA_BYTES		16
#define	ECC_FORCE_CLK_OPEN		30

/* NAND_DEV_CMD1 bits */
#define	READ_ADDR			0

/* NAND_DEV_CMD_VLD bits */
#define	READ_START_VLD			0

/* NAND_EBI2_ECC_BUF_CFG bits */
#define	NUM_STEPS			0

/* NAND_ERASED_CW_DETECT_CFG bits */
#define	ERASED_CW_ECC_MASK		1
#define	AUTO_DETECT_RES			0
#define	MASK_ECC			(1 << ERASED_CW_ECC_MASK)
#define	RESET_ERASED_DET		(1 << AUTO_DETECT_RES)
#define	ACTIVE_ERASED_DET		(0 << AUTO_DETECT_RES)
#define	CLR_ERASED_PAGE_DET		(RESET_ERASED_DET | MASK_ECC)
#define	SET_ERASED_PAGE_DET		(ACTIVE_ERASED_DET | MASK_ECC)

/* NAND_ERASED_CW_DETECT_STATUS bits */
#define	PAGE_ALL_ERASED			BIT(7)
#define	CODEWORD_ALL_ERASED		BIT(6)
#define	PAGE_ERASED			BIT(5)
#define	CODEWORD_ERASED			BIT(4)
#define	ERASED_PAGE			(PAGE_ALL_ERASED | PAGE_ERASED)
#define	ERASED_CW			(CODEWORD_ALL_ERASED | CODEWORD_ERASED)

/* NAND_READ_LOCATION_n bits */
#define READ_LOCATION_OFFSET		0
#define READ_LOCATION_SIZE		16
#define READ_LOCATION_LAST		31

/* Version Mask */
#define	NAND_VERSION_MAJOR_MASK		0xf0000000
#define	NAND_VERSION_MAJOR_SHIFT	28
#define	NAND_VERSION_MINOR_MASK		0x0fff0000
#define	NAND_VERSION_MINOR_SHIFT	16

/* NAND OP_CMDs */
#define	PAGE_READ			0x2
#define	PAGE_READ_WITH_ECC		0x3
#define	PAGE_READ_WITH_ECC_SPARE	0x4
#define	PROGRAM_PAGE			0x6
#define	PAGE_PROGRAM_WITH_ECC		0x7
#define	PROGRAM_PAGE_SPARE		0x9
#define	BLOCK_ERASE			0xa
#define	FETCH_ID			0xb
#define	RESET_DEVICE			0xd
#define ACC_FEATURE			0xE

/* NAND_CTRL bits */
#define	BAM_MODE_EN			BIT(0)
#define NANDC_READ_DELAY_COUNTER_VAL	0x340
#define BOOST_MODE			BIT(11)

/* Value for NAND_DEV_CMD_VLD */
#define NAND_DEV_CMD_VLD_VAL		(0x1D)

/* Qpic serial NAND mask */
#define NAND_DEV_CMD_VLD_SERIAL_VAL	(0xD)
#define NAND_DEV0_CFG0_VAL		0x1A5408D0
#define NAND_DEV0_CFG1_VAL		0x8147440
#define NAND_DEV0_CFG1_VAL_RST		0x87476B1
#define NAND_DEV0_ECC_CFG_SW_RST	0x42040702
#define NAND_DEV0_ECC_CFG_SW_RST_DIS	0x42040700
#define NAND_DEV0_CFG0_RESET		(NAND_DEV0_CFG0_VAL & 0x00ff00f0)
#define NAND_DEV0_ECC_CFG_RESET		(NAND_DEV0_ECC_CFG_SW_RST & 0x0f000000) | (1 << 0)
#define RESET_DEVICE_SERIAL		0x3800000D
#define SPI_CFG_CLK_CNTR_INIT_VAL_VEC	0x924
#define SPI_FLASH_MODE_EN		(1 << 0)
#define SPI_LOAD_CLK_CNTR_INIT_EN	(1 << 28)
#define SPI_FEA_STATUS_DEV_ADDR		0xc000
#define SPI_NUM_ADDR2_CYCLES		0x3
#define NAND_FLASH_DEV_CMD0_VAL		0x1080D8D8
#define NAND_FLASH_DEV_CMD1_VAL		0xF00F3000
#define NAND_FLASH_DEV_CMD2_VAL		0xF0FF709F
#define NAND_FLASH_DEV_CMD3_VAL		0x3F310015
#define NAND_FLASH_DEV_CMD3_MASK	0xfff0ffff
#define NAND_FLASH_DEV_CMD7_VAL		0x04061F0F
/* NAND_FLASH_DEV_CMD8 & NAND_FLASH_DEV_CMD9 register
 * default value is configured as per SPI configuration
 */
#define SPI_NO_OF_ADDR_CYCLE		0xDA4DB
#define SPI_BUSY_CHECK_WAIT_CNT         0x00000010
#define QPIC_MAX_SPI_MODES		2
#define QPIC_NUM_XFER_STEPS		7
#define QPIC_SET_FEATURE		(1 << 31)
#define QPIC_SPI_TRANSFER_MODE_x1	(1 << 29)
#define QPIC_SPI_TRANSFER_MODE_x4	(3 << 29)
#define QPIC_SPI_WP			(1 << 28)
#define QPIC_SPI_HOLD			(1 << 27)
#define SPI_FLASH_BLOCK_PROT_REG	0xA0
#define SPI_FLASH_FEATURE_REG		0xB0
#define BLOCK_PROTECTION_ENABLE		0x78
#define BLOCK_PROTECTION_DISABLE	0x00
#define SPI_FLASH_ECC_ENABLE		(1 << 4)
#define SPI_FLASH_ECC_DISABLE          	0x00
#define SPI_CFG_ENABLED			0xff
#define SPI_FLASH_QUAD_MODE		0x1
#define	SPI_NAND_FR_BUFF_ENABLE		(1 << 3)
#define SPI_FLASH_MICRON_ID		0x2c
#define SPI_FLASH_ESMT_DEVICE_ID	0x11
#define SPI_FLASH_WINBOND_ID		0xef
#define SPI_FLASH_GIGA_ID		0xc8
#define QPIC_VERSION_V2_0		0x2
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
/* Auto status enable for all the 6 status register
 * for now we are only using 3 status register
 * so enable auto status only for these three register
 */
#define QPIC_AUTO_STATUS_EN_MASK	0x000B000B
#define QPIC_PAGE_SCOPE_CMD_EN		(1 << 23)
#define QPIC_MULTI_PAGE_CMD_EN		(1 << 22)
#define QPIC_PAGE_SCOPE			(1 << 0)
#define QPIC_MULTI_PAGE			(1 << 1)
#define PAGE_SCOPE_MULTI_PAGE_CMD_EXE	(1 << 2)
#define PAGE_SCOPE_MULTI_PAGE_STATUS_DESC (1 << 3)

#define MAX_STATUS_REG			12
#define PAGE_2KiB_STATUS_BUF_SIZE	48
#define PAGE_4KiB_STATUS_BUF_SIZE	96
#define PAGE_8KiB_STATUS_BUF_SIZE	192
#define PAGE_2KiB			2048
#define PAGE_4KiB			4096
#define PAGE_8KiB			8192
#define GET_STATUS_BUFF_PARSE_SIZE_PER_PAGE(page_size, parse_size) ({	\
		switch(page_size) {					\
			case PAGE_2KiB:					\
				parse_size = PAGE_2KiB_STATUS_BUF_SIZE;	\
				break;					\
			case PAGE_4KiB:					\
				parse_size = PAGE_4KiB_STATUS_BUF_SIZE;	\
				break;					\
			case PAGE_8KiB:					\
				parse_size = PAGE_8KiB_STATUS_BUF_SIZE;	\
				break;					\
			default:					\
				break;					\
		}							\
	})
#define GET_STATUS_BUFF_ALLOC_SIZE(page_size, status_buf_size) ({	\
		switch(page_size) {					\
			case PAGE_2KiB:					\
				status_buf_size = PAGE_2KiB_STATUS_BUF_SIZE * \
							MAX_MULTI_PAGE;	\
				break;					\
			case PAGE_4KiB:					\
				status_buf_size = PAGE_4KiB_STATUS_BUF_SIZE * \
							MAX_MULTI_PAGE;	\
				break;					\
			case PAGE_8KiB:					\
				status_buf_size = PAGE_8KiB_STATUS_BUF_SIZE * \
							MAX_MULTI_PAGE;	\
				break;					\
			default:					\
				break;					\
		}							\
	})
#endif
#define UD_SIZE_BYTES_MASK	(0x3ff << UD_SIZE_BYTES)
#define SPARE_SIZE_BYTES_MASK	(0xf << SPARE_SIZE_BYTES)
#define ECC_NUM_DATA_BYTES_MASK	(0x3ff << ECC_NUM_DATA_BYTES)

/*
 * the NAND controller performs reads/writes with ECC in 516 byte chunks.
 * the driver calls the chunks 'step' or 'codeword' interchangeably
 */
#define	NANDC_STEP_SIZE			512

/*
 * the largest page size we support is 8K, this will have 16 steps/codewords
 * of 512 bytes each
 */
#define	MAX_NUM_STEPS			(SZ_8K / NANDC_STEP_SIZE)

/* we read at most 3 registers per codeword sncan */
#define	MAX_REG_RD			(3 * MAX_NUM_STEPS)

/* ECC modes supported by the controller */
#define	ECC_NONE	BIT(0)
#define	ECC_RS_4BIT	BIT(1)
#define	ECC_BCH_4BIT	BIT(2)
#define	ECC_BCH_8BIT	BIT(3)

/* Flags used for BAM DMA desc preparation*/
/* Don't set the EOT in current tx sgl */
#define DMA_DESC_FLAG_NO_EOT		(0x0001)
/* Set the NWD flag in current sgl */
#define DMA_DESC_FLAG_BAM_NWD		(0x0002)
/* Close current sgl and start writing in another sgl */
#define DMA_DESC_FLAG_BAM_NEXT_SGL	(0x0004)
/*
 * Erased codeword status is being used two times in single transfer so this
 * flag will determine the current value of erased codeword status register
 */
#define DMA_DESC_ERASED_CW_SET		(0x0008)

/* Returns the dma address for reg read buffer */
#define REG_BUF_DMA_ADDR(chip, vaddr) \
	((chip)->reg_read_buf_phys + \
	((uint8_t *)(vaddr) - (uint8_t *)(chip)->reg_read_buf))

/* Returns the nand register physical address */
#define NAND_REG_PHYS_ADDRESS(chip, addr) \
	((chip)->base_dma + (addr))

/* command element array size in bam transaction */
#define BAM_CMD_ELEMENT_SIZE	(256)
/* command sgl size in bam transaction */
#define BAM_CMD_SGL_SIZE	(256)
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
/* for 4KiB page and 8KiB page the total no of
 * code word will be more so change value 5 & 4
 * as well
 *
 * data sgl size in bam transaction */
#define BAM_DATA_SGL_SIZE	(10 * MAX_MULTI_PAGE)
/* status sgl size in bam transaction */
#define BAM_STATUS_SGL_SIZE	(8 * MAX_MULTI_PAGE)
#else
/* data sgl size in bam transaction */
#define BAM_DATA_SGL_SIZE	(128)
/* status sgl size in bam transaction */
#define BAM_STATUS_SGL_SIZE	(128)
#endif

#define QPIC_NAND_COMPLETION_TIMEOUT	msecs_to_jiffies(1000)

/* NANDc reg offsets enumeration */
enum {
	NAND_FLASH_CMD,
	NAND_ADDR0,
	NAND_ADDR1,
	NAND_FLASH_CHIP_SELECT,
	NAND_EXEC_CMD,
	NAND_FLASH_STATUS,
	NAND_BUFFER_STATUS,
	NAND_DEV0_CFG0,
	NAND_DEV0_CFG1,
	NAND_DEV0_ECC_CFG,
	NAND_DEV1_ECC_CFG,
	NAND_AUTO_STATUS_EN,
	NAND_DEV1_CFG0,
	NAND_DEV1_CFG1,
	NAND_READ_ID,
	NAND_READ_STATUS,
	NAND_DEV_CMD0,
	NAND_DEV_CMD1,
	NAND_DEV_CMD2,
	NAND_DEV_CMD_VLD,
	SFLASHC_BURST_CFG,
	NAND_ERASED_CW_DETECT_CFG,
	NAND_ERASED_CW_DETECT_STATUS,
	NAND_EBI2_ECC_BUF_CFG,
	FLASH_BUF_ACC,
	NAND_CTRL,
	NAND_VERSION,
	NAND_READ_LOCATION_0,
	NAND_READ_LOCATION_1,
	NAND_READ_LOCATION_2,
	NAND_READ_LOCATION_3,

	/* dummy register offsets, used by write_reg_dma */
	NAND_DEV_CMD1_RESTORE,
	NAND_DEV_CMD_VLD_RESTORE,
	NAND_READ_LOCATION_LAST_CW_0,
	NAND_READ_LOCATION_LAST_CW_1,
	NAND_READ_LOCATION_LAST_CW_2,
	NAND_READ_LOCATION_LAST_CW_3,
	NAND_MULTI_PAGE_CMD,
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	NAND_FLASH_XFR_STEP1,
	NAND_FLASH_XFR_STEP2,
	NAND_FLASH_XFR_STEP3,
	NAND_FLASH_XFR_STEP4,
	NAND_FLASH_XFR_STEP5,
	NAND_FLASH_XFR_STEP6,
	NAND_FLASH_XFR_STEP7,
	NAND_FLASH_FEATURES,
	NAND_DEV_CMD7,
	NAND_DEV_CMD8,
	NAND_DEV_CMD9,
	NAND_FLASH_SPI_CFG,
	NAND_SPI_NUM_ADDR_CYCLES,
	NAND_SPI_BUSY_CHECK_WAIT_CNT,
	NAND_DEV_CMD3,
	NAND_QSPI_MSTR_CONFIG,
#endif
};

#define QPIC_FLASH_XFR_STEP1_VAL	0x00E00080
#define QPIC_FLASH_XFR_STEP2_VAL	0x49F04998
#define QPIC_FLASH_XFR_STEP3_VAL	0x8DE08D80
#define QPIC_FLASH_XFR_STEP4_VAL	0xC000C000
#define QPIC_FLASH_XFR_STEP5_VAL	0xC000C000
#define QPIC_FLASH_XFR_STEP6_VAL	0xC000C000
#define QPIC_FLASH_XFR_STEP7_VAL	0xC000C000
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
/* SPI nand flash Table */
struct nand_flash_dev qspinand_flash_ids[] = {
	{"GD5F1GQ4RE9IG SPI NAND 1G 1.8V",
		{ .id = {0xc8, 0xc1} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 128, NAND_ECC_INFO(8, SZ_512), 0},
	{"GD5F1GQ4RE9IH SPI NAND 1G 1.8V",
		{ .id = {0xc8, 0xc9} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"GD5F2GQ5REYIH SPI NAND 2G",
		{ .id = {0xc8, 0x22} },
		SZ_2K, SZ_256, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"MT29F1G01ABBFDWB-IT SPI NAND 1G 1.8V",
		{ .id = {0x2c, 0x15} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 128, NAND_ECC_INFO(8, SZ_512), 0},
	{"W25N01JW SPI NAND 1.8V 1G-BIT",
		{ .id = {0xef, 0xbc} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"GD5F1GQ5REYIG SPI NAND 1G",
		{ .id = {0xc8, 0x41} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 128, NAND_ECC_INFO(8, SZ_512), 0},
	{"F50D1G41LB(2M) SPI NAND 1G 1.8V",
		{ .id = {0xc8, 0x11} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"GD5F1GQ5REYIH SPI NAND 1G 1.8V",
		{ .id = {0xc8, 0x21} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"GD5F4GQ6REYIHR SPI NAND 4G 1.8V",
		{ .id = {0xc8, 0x25} },
		SZ_2K, SZ_512, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"W25N02JWZEIF SPI NAND 2G 1.8V",
		{ .id = {0xef, 0xbf} },
		SZ_2K, SZ_256, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"MX35UF1GE4AC SPI NAND 1G 1.8V",
		{ .id = {0xc2, 0x92} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"F50D2G41KA-83YIG2V SPI NAND 2G 1.8V",
		{ .id = {0xc8, 0x51} },
		SZ_2K, SZ_256, SZ_128K, 0, 2, 128, NAND_ECC_INFO(8, SZ_512), 0},
	{"DS35M1GA SPI NAND 1G 1.8V",
		{ .id = {0xe5, 0x21} },
		SZ_2K, SZ_128, SZ_128K, 0, 2, 64, NAND_ECC_INFO(4, SZ_512), 0},
	{"GD5F2GQ5REYIG SPI NAND 2G",
		{ .id = {0xc8, 0x42} },
		SZ_2K, SZ_256, SZ_128K, 0, 2, 128, NAND_ECC_INFO(8, SZ_512), 0},
	{NULL}
};

struct qpic_nand_pltfm_data {
	u32 *io_macro_clk_table;
	unsigned char io_macro_clk_cnt;
};

#define FEEDBACK_CLK_EN		(1 << 4)
#endif
/*
 * This data type corresponds to the BAM transaction which will be used for any
 * nand request.
 * @bam_ce - the array of bam command elements
 * @cmd_sgl - sgl for nand bam command pipe
 * @tx_sgl - sgl for nand bam consumer pipe
 * @rx_sgl - sgl for nand bam producer pipe
 * @bam_ce_index - the index in bam_ce which is available for next sgl request
 * @pre_bam_ce_index - the index in bam_ce which marks the start position ce
 *                     for current sgl. It will be used for size calculation
 *                     for current sgl
 * @cmd_sgl_cnt - no of entries in command sgl.
 * @tx_sgl_cnt - no of entries in tx sgl.
 * @rx_sgl_cnt - no of entries in rx sgl.
 * @chans_cnt - no of QPIC BAM channels in which the descriptors have been
 *		submitted for this transaction.
 * @completed_chans - no of QPIC BAM channels in which all the descriptors have
 *		      been processed and completion callback has been triggered.
 * @txn_done - the completion used for complete QPIC NAND transfer.
 */
struct bam_transaction {
	struct bam_cmd_element bam_ce[BAM_CMD_ELEMENT_SIZE];
	struct qcom_bam_sgl cmd_sgl[BAM_CMD_SGL_SIZE];
	struct qcom_bam_sgl tx_sgl[BAM_DATA_SGL_SIZE];
	struct qcom_bam_sgl rx_sgl[BAM_DATA_SGL_SIZE];
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	struct qcom_bam_sgl status_sgl[BAM_DATA_SGL_SIZE];
#endif
	uint32_t bam_ce_index;
	uint32_t pre_bam_ce_index;
	uint32_t cmd_sgl_cnt;
	uint32_t tx_sgl_cnt;
	uint32_t rx_sgl_cnt;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	uint32_t status_sgl_cnt;
#endif
	uint32_t chans_cnt;
	uint32_t completed_chans;
	struct completion txn_done;
};

/**
 * This data type corresponds to the nand dma descriptor
 * @list - list for desc_info
 * @dir - DMA transfer direction
 * @sgl - sgl which will be used for single sgl dma descriptor
 * @dma_desc - low level dma engine descriptor
 * @bam_desc_data - used for bam desc mappings
 */
struct desc_info {
	struct list_head node;

	enum dma_data_direction dir;
	struct scatterlist sgl;
	struct dma_async_tx_descriptor *dma_desc;
	struct qcom_bam_custom_data bam_desc_data;
};

/*
 * holds the current register values that we want to write. acts as a contiguous
 * chunk of memory which we use to write the controller registers through DMA.
 */
struct nandc_regs {
	__le32 cmd;
	__le32 addr0;
	__le32 addr1;
	__le32 chip_sel;
	__le32 exec;

	__le32 cfg0;
	__le32 cfg1;
	__le32 ecc_bch_cfg;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	__le32 auto_status_en;
#endif
	__le32 clrflashstatus;
	__le32 clrreadstatus;

	__le32 cmd1;
	__le32 vld;

	__le32 orig_cmd1;
	__le32 orig_vld;

	__le32 ecc_buf_cfg;
	__le32 read_location0;
	__le32 read_location1;
	__le32 read_location2;
	__le32 read_location3;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	__le32 multi_page_cmd;
#endif
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	__le32 read_location_last0;
	__le32 read_location_last1;
	__le32 read_location_last2;
	__le32 read_location_last3;
	__le32 flash_feature;
	__le32 spi_cfg;
	__le32 spi_num_addr_cycle;
	__le32 spi_busy_wait_cnt;
	__le32 spi_mstr_cfg;
#endif
	__le32 erased_cw_detect_cfg_clr;
	__le32 erased_cw_detect_cfg_set;
};

/*
 * NAND controller data struct
 *
 * @controller:			base controller structure
 * @host_list:			list containing all the chips attached to the
 *				controller
 * @dev:			parent device
 * @base:			MMIO base
 * @base_dma:			physical base address of controller registers
 * @core_clk:			controller clock
 * @aon_clk:			another controller clock
 *
 * @chan:			dma channel
 * @bam_txn:			contains the bam transaction address
 * @cmd_crci:			ADM DMA CRCI for command flow control
 * @data_crci:			ADM DMA CRCI for data flow control
 * @desc_list:			DMA descriptor list (list of desc_infos)
 *
 * @data_buffer:		our local DMA buffer for page read/writes,
 *				used when we can't use the buffer provided
 *				by upper layers directly
 * @buf_size/count/start:	markers for chip->read_buf/write_buf functions
 * @reg_read_buf:		local buffer for reading back registers via DMA
 * @reg_read_buf_phys:		contains dma address for register read buffer
 * @reg_read_pos:		marker for data read in reg_read_buf
 *
 * @regs:			a contiguous chunk of memory for DMA register
 *				writes. contains the register values to be
 *				written to controller
 * @cmd1/vld:			some fixed controller register values
 * @ecc_modes:			supported ECC modes by the current controller,
 *				initialized via DT match data
 * @bch_enabled:		flag to tell whether BCH or RS ECC mode is used
 * @dma_bam_enabled:		flag to tell whether nand controller is using
 *				bam dma
 * @create_sys_boot_layout:	create sysfs entry for boot_layout
 * @boot_layout:		flag to tell whether current layout is boot
 *				layout
 * @regs_offsets:		register offset mapping array
 */
struct qcom_nand_controller {
	struct nand_hw_control controller;
	struct list_head host_list;

	struct device *dev;

	void __iomem *base;
	dma_addr_t base_dma;

	struct clk *core_clk;
	struct clk *aon_clk;
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	struct clk *io_macro_clk;
	struct qpic_nand_pltfm_data *pdata;
	u32 io_macro_max_clk;
	u32 phase;
#endif
	union {
		struct {
			struct dma_chan *tx_chan;
			struct dma_chan *rx_chan;
			struct dma_chan *cmd_chan;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
			struct dma_chan *status_chan;
#endif
		};
		struct {
			struct dma_chan *chan;
			unsigned int cmd_crci;
			unsigned int data_crci;
		};
	};

	struct list_head desc_list;
	struct bam_transaction *bam_txn;

	u8		*data_buffer;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	u8		*status_buf;
	int		ps_mp_flag;
	int		status_buf_size;
#endif
	bool		dma_bam_enabled;
	bool		create_sys_boot_layout;
	bool		boot_layout;
	int		buf_size;
	int		buf_count;
	int		buf_start;
	uint32_t	param_read_addr_cycle;

	__le32 *reg_read_buf;
	dma_addr_t reg_read_buf_phys;
	int reg_read_pos;

	struct nandc_regs *regs;

	u32 cmd1, vld;
	u32 ecc_modes;
	u32 *regs_offsets;
};

/*
 * NAND chip structure
 *
 * @chip:			base NAND chip structure
 * @node:			list node to add itself to host_list in
 *				qcom_nand_controller
 *
 * @cs:				chip select value for this chip
 * @cw_size:			the number of bytes in a single step/codeword
 *				of a page, consisting of all data, ecc, spare
 *				and reserved bytes
 * @cw_data:			the number of bytes within a codeword protected
 *				by ECC
 * @use_ecc:			request the controller to use ECC for the
 *				upcoming read/write
 * @bch_enabled:		flag to tell whether BCH ECC mode is used
 * @ecc_bytes_hw:		ECC bytes used by controller hardware for this
 *				chip
 * @status:			value to be returned if NAND_CMD_STATUS command
 *				is executed
 * @last_command:		keeps track of last command on this chip. used
 *				for reading correct status
 *
 * @cfg0, cfg1, cfg0_raw..:	NANDc register configurations needed for
 *				ecc/non-ecc mode for the current nand flash
 *				device
 *
 * @quad_mode:			Quad mode for serial nand device.
 *
 * @check_qe_bit:		This flag will decide wether to check device
 * 				configuration register for quad mode or not.
 *
 * @hw_version:			QPIC controller version.
 */
struct qcom_nand_host {
	struct nand_chip chip;
	struct list_head node;

	int cs;
	int cw_size;
	int cw_data;
	bool use_ecc;
	bool bch_enabled;
	int ecc_bytes_hw;
	int spare_bytes;
	int bbm_size;
	u8 status;
	int last_command;

	u32 cfg0, cfg1;
	u32 cfg0_raw, cfg1_raw;
	u32 ecc_buf_cfg;
	u32 ecc_bch_cfg;
	u32 clrflashstatus;
	u32 clrreadstatus;
	u32 hw_version;
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	bool quad_mode;
	bool check_qe_bit;
#endif
};

/*
 * This data type corresponds to the nand driver data which will be used at
 * driver probe time
 * @ecc_modes - ecc mode for nand
 * @regs_offsets - contains the register offsets array pointer.
 * @dma_bam_enabled - whether this driver is using bam
 * @create_sys_boot_layout - create sysfs entry for boot layout
 */
struct qcom_nand_driver_data {
	u32 ecc_modes;
	u32 *regs_offsets;
	bool dma_bam_enabled;
	bool create_sys_boot_layout;
};

/* Mapping tables which contains the actual register offsets */
/* NAND controller Version 1.4.0 mapping table */
u32 regs_offsets_v1_4_0[] = {
	[NAND_FLASH_CMD] = 0x00,
	[NAND_ADDR0] = 0x04,
	[NAND_ADDR1] = 0x08,
	[NAND_FLASH_CHIP_SELECT] = 0x0c,
	[NAND_EXEC_CMD] = 0x10,
	[NAND_FLASH_STATUS] = 0x14,
	[NAND_BUFFER_STATUS] = 0x18,
	[NAND_DEV0_CFG0] = 0x20,
	[NAND_DEV0_CFG1] = 0x24,
	[NAND_DEV0_ECC_CFG] = 0x28,
	[NAND_DEV1_ECC_CFG] = 0x2c,
	[NAND_DEV1_CFG0] = 0x30,
	[NAND_DEV1_CFG1] = 0x34,
	[NAND_READ_ID] = 0x40,
	[NAND_READ_STATUS] = 0x44,
	[NAND_DEV_CMD0] = 0xa0,
	[NAND_DEV_CMD1] = 0xa4,
	[NAND_DEV_CMD2] = 0xa8,
	[NAND_DEV_CMD_VLD] = 0xac,
	[SFLASHC_BURST_CFG] = 0xe0,
	[NAND_ERASED_CW_DETECT_CFG] = 0xe8,
	[NAND_ERASED_CW_DETECT_STATUS] = 0xec,
	[NAND_EBI2_ECC_BUF_CFG] = 0xf0,
	[FLASH_BUF_ACC] = 0x100,
	[NAND_CTRL] = 0xf00,
	[NAND_VERSION] = 0xf08,
	[NAND_READ_LOCATION_0] = 0xf20,
	[NAND_READ_LOCATION_1] = 0xf24,
	[NAND_READ_LOCATION_2] = 0xf28,
	[NAND_READ_LOCATION_3] = 0xf2c,
	[NAND_DEV_CMD1_RESTORE] = 0xdead,
	[NAND_DEV_CMD_VLD_RESTORE] = 0xbeef,
};

/* NAND controller Version 1.5.0 mapping table */
u32 regs_offsets_v1_5_0[] = {
	[NAND_FLASH_CMD] = 0x00,
	[NAND_ADDR0] = 0x04,
	[NAND_ADDR1] = 0x08,
	[NAND_FLASH_CHIP_SELECT] = 0x0c,
	[NAND_EXEC_CMD] = 0x10,
	[NAND_FLASH_STATUS] = 0x14,
	[NAND_BUFFER_STATUS] = 0x18,
	[NAND_DEV0_CFG0] = 0x20,
	[NAND_DEV0_CFG1] = 0x24,
	[NAND_DEV0_ECC_CFG] = 0x28,
	[NAND_DEV1_ECC_CFG] = 0x2c,
	[NAND_DEV1_CFG0] = 0x30,
	[NAND_DEV1_CFG1] = 0x34,
	[NAND_READ_ID] = 0x40,
	[NAND_READ_STATUS] = 0x44,
	[NAND_DEV_CMD0] = 0x70a0,
	[NAND_DEV_CMD1] = 0x70a4,
	[NAND_DEV_CMD2] = 0x70a8,
	[NAND_DEV_CMD_VLD] = 0x70ac,
	[SFLASHC_BURST_CFG] = 0xe0,
	[NAND_ERASED_CW_DETECT_CFG] = 0xe8,
	[NAND_ERASED_CW_DETECT_STATUS] = 0xec,
	[NAND_EBI2_ECC_BUF_CFG] = 0xf0,
	[FLASH_BUF_ACC] = 0x100,
	[NAND_CTRL] = 0xf00,
	[NAND_VERSION] = 0x4f08,
	[NAND_READ_LOCATION_0] = 0xf20,
	[NAND_READ_LOCATION_1] = 0xf24,
	[NAND_READ_LOCATION_2] = 0xf28,
	[NAND_READ_LOCATION_3] = 0xf2c,
	[NAND_DEV_CMD1_RESTORE] = 0xdead,
	[NAND_DEV_CMD_VLD_RESTORE] = 0xbeef,
};

/* NAND controller Version 2.1.1 mapping table */
u32 regs_offsets_v2_1_1[] = {
	[NAND_FLASH_CMD] = 0x00,
	[NAND_ADDR0] = 0x04,
	[NAND_ADDR1] = 0x08,
	[NAND_FLASH_CHIP_SELECT] = 0x0c,
	[NAND_EXEC_CMD] = 0x10,
	[NAND_FLASH_STATUS] = 0x14,
	[NAND_BUFFER_STATUS] = 0x18,
	[NAND_DEV0_CFG0] = 0x20,
	[NAND_DEV0_CFG1] = 0x24,
	[NAND_DEV0_ECC_CFG] = 0x28,
	[NAND_DEV1_ECC_CFG] = 0x602c,
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	[NAND_AUTO_STATUS_EN] = 0x2c,
#endif
	[NAND_DEV1_CFG0] = 0x30,
	[NAND_READ_ID] = 0x40,
	[NAND_READ_STATUS] = 0x44,
	[NAND_DEV_CMD0] = 0x70a0,
	[NAND_DEV_CMD1] = 0x70a4,
	[NAND_DEV_CMD2] = 0x70a8,
	[NAND_DEV_CMD_VLD] = 0x70ac,
	[NAND_ERASED_CW_DETECT_CFG] = 0xe8,
	[NAND_ERASED_CW_DETECT_STATUS] = 0xec,
	[NAND_EBI2_ECC_BUF_CFG] = 0xf0,
	[FLASH_BUF_ACC] = 0x100,
	[NAND_CTRL] = 0xf00,
	[NAND_VERSION] = 0x4f08,
	[NAND_READ_LOCATION_0] = 0xf20,
	[NAND_READ_LOCATION_1] = 0xf24,
	[NAND_READ_LOCATION_2] = 0xf28,
	[NAND_READ_LOCATION_3] = 0xf2c,
	[NAND_DEV_CMD1_RESTORE] = 0xdead,
	[NAND_DEV_CMD_VLD_RESTORE] = 0xbeef,
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	[NAND_MULTI_PAGE_CMD] = 0x0f60,
#endif
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	[NAND_FLASH_XFR_STEP1] = 0x7070,
	[NAND_FLASH_XFR_STEP2] = 0x7074,
	[NAND_FLASH_XFR_STEP3] = 0x7078,
	[NAND_FLASH_XFR_STEP4] = 0x707c,
	[NAND_FLASH_XFR_STEP5] = 0x7080,
	[NAND_FLASH_XFR_STEP6] = 0x7084,
	[NAND_FLASH_XFR_STEP7] = 0x7088,
	[NAND_FLASH_FEATURES] = 0x0F64,
	[NAND_DEV_CMD7] = 0x70b0,
	[NAND_DEV_CMD8] = 0x70b4,
	[NAND_DEV_CMD9] = 0x70b8,
	[NAND_FLASH_SPI_CFG] = 0x70c0,
	[NAND_SPI_NUM_ADDR_CYCLES] = 0x70c4,
	[NAND_SPI_BUSY_CHECK_WAIT_CNT] = 0x70c8,
	[NAND_DEV_CMD3] = 0x70d0,
	[NAND_READ_LOCATION_LAST_CW_0] = 0xf40,
	[NAND_READ_LOCATION_LAST_CW_1] = 0xf44,
	[NAND_READ_LOCATION_LAST_CW_2] = 0xf48,
	[NAND_READ_LOCATION_LAST_CW_3] = 0xf4c,
	[NAND_QSPI_MSTR_CONFIG] = 0x7f60,
#endif
};

/* Allocates and Initializes the BAM transaction */
struct bam_transaction *alloc_bam_transaction(
	struct qcom_nand_controller *nandc)
{
	struct bam_transaction *bam_txn;

	bam_txn = devm_kzalloc(nandc->dev, sizeof(*bam_txn), GFP_KERNEL);

	if (!bam_txn)
		return NULL;

	qcom_bam_sg_init_table(bam_txn->cmd_sgl, BAM_CMD_SGL_SIZE);
	qcom_bam_sg_init_table(bam_txn->tx_sgl, BAM_DATA_SGL_SIZE);
	qcom_bam_sg_init_table(bam_txn->rx_sgl, BAM_DATA_SGL_SIZE);
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	qcom_bam_sg_init_table(bam_txn->status_sgl, BAM_STATUS_SGL_SIZE);
#endif

	/* command channel will be always used so initialize with 1 */
	bam_txn->chans_cnt = 1;
	init_completion(&bam_txn->txn_done);

	return bam_txn;
}

/* Clears the BAM transaction index */
void clear_bam_transaction(struct qcom_nand_controller *nandc)
{
	struct bam_transaction *bam_txn = nandc->bam_txn;

	if (!nandc->dma_bam_enabled)
		return;

	bam_txn->bam_ce_index = 0;
	bam_txn->pre_bam_ce_index = 0;
	bam_txn->cmd_sgl_cnt = 0;
	bam_txn->tx_sgl_cnt = 0;
	bam_txn->rx_sgl_cnt = 0;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	bam_txn->status_sgl_cnt = 0;
#endif
	/* command channel will be always used so initialize with 1 */
	bam_txn->chans_cnt = 1;
	bam_txn->completed_chans = 0;

	reinit_completion(&bam_txn->txn_done);
}

/* Callback for DMA descriptor completion */
static void qpic_bam_dma_done(void *data)
{
	struct bam_transaction *bam_txn = data;

	bam_txn->completed_chans++;
	/*
	 * All the QPIC BAM channels will generate the callback upon completion
	 * of last descriptor. Schedule the complete when all the QPIC BAM
	 * channels have processed its descriptors.
	 */
	if (bam_txn->completed_chans == bam_txn->chans_cnt)
		complete(&bam_txn->txn_done);
}

static inline struct qcom_nand_host *to_qcom_nand_host(struct nand_chip *chip)
{
	return container_of(chip, struct qcom_nand_host, chip);
}

static inline struct qcom_nand_controller *
get_qcom_nand_controller(struct nand_chip *chip)
{
	return container_of(chip->controller, struct qcom_nand_controller,
			    controller);
}

static inline u32 nandc_read(struct qcom_nand_controller *nandc, int offset)
{
	return ioread32(nandc->base + nandc->regs_offsets[offset]);
}

static inline void nandc_write(struct qcom_nand_controller *nandc, int offset,
			       u32 val)
{
	iowrite32(val, nandc->base + nandc->regs_offsets[offset]);
}

static __le32 *offset_to_nandc_reg(struct nandc_regs *regs, int offset)
{
	switch (offset) {
	case NAND_FLASH_CMD:
		return &regs->cmd;
	case NAND_ADDR0:
		return &regs->addr0;
	case NAND_ADDR1:
		return &regs->addr1;
	case NAND_FLASH_CHIP_SELECT:
		return &regs->chip_sel;
	case NAND_EXEC_CMD:
		return &regs->exec;
	case NAND_FLASH_STATUS:
		return &regs->clrflashstatus;
	case NAND_DEV0_CFG0:
		return &regs->cfg0;
	case NAND_DEV0_CFG1:
		return &regs->cfg1;
	case NAND_DEV0_ECC_CFG:
		return &regs->ecc_bch_cfg;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	case NAND_AUTO_STATUS_EN:
		return &regs->auto_status_en;
#endif
	case NAND_READ_STATUS:
		return &regs->clrreadstatus;
	case NAND_DEV_CMD1:
		return &regs->cmd1;
	case NAND_DEV_CMD1_RESTORE:
		return &regs->orig_cmd1;
	case NAND_DEV_CMD_VLD:
		return &regs->vld;
	case NAND_DEV_CMD_VLD_RESTORE:
		return &regs->orig_vld;
	case NAND_EBI2_ECC_BUF_CFG:
		return &regs->ecc_buf_cfg;
	case NAND_BUFFER_STATUS:
		return &regs->clrreadstatus;
	case NAND_READ_LOCATION_0:
		return &regs->read_location0;
	case NAND_READ_LOCATION_1:
		return &regs->read_location1;
	case NAND_READ_LOCATION_2:
		return &regs->read_location2;
	case NAND_READ_LOCATION_3:
		return &regs->read_location3;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	case NAND_MULTI_PAGE_CMD:
		return &regs->multi_page_cmd;
#endif
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	case NAND_READ_LOCATION_LAST_CW_0:
		return &regs->read_location_last0;
	case NAND_READ_LOCATION_LAST_CW_1:
		return &regs->read_location_last1;
	case NAND_READ_LOCATION_LAST_CW_2:
		return &regs->read_location_last2;
	case NAND_READ_LOCATION_LAST_CW_3:
		return &regs->read_location_last3;
	case NAND_FLASH_FEATURES:
		return &regs->flash_feature;
	case NAND_FLASH_SPI_CFG:
		return &regs->spi_cfg;
	case NAND_SPI_NUM_ADDR_CYCLES:
		return &regs->spi_num_addr_cycle;
	case NAND_SPI_BUSY_CHECK_WAIT_CNT:
		return &regs->spi_busy_wait_cnt;
	case NAND_QSPI_MSTR_CONFIG:
		return &regs->spi_mstr_cfg;
#endif
	default:
		return NULL;
	}
}

static void nandc_set_reg(struct qcom_nand_controller *nandc, int offset,
			  u32 val)
{
	struct nandc_regs *regs = nandc->regs;
	__le32 *reg;

	reg = offset_to_nandc_reg(regs, offset);

	if (reg) {
#if IS_ENABLED(CONFIG_CPU_BIG_ENDIAN)
		*reg = val;
#else
		*reg = cpu_to_le32(val);
#endif
	}
}

/* helper to configure address register values */
static void set_address(struct qcom_nand_host *host, u16 column, int page)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	if (chip->options & NAND_BUSWIDTH_16)
		column >>= 1;
	nandc_set_reg(nandc, NAND_ADDR0, page << 16 | column);
	nandc_set_reg(nandc, NAND_ADDR1, page >> 16 & 0xff);
}

/*
 * update_rw_regs:	set up read/write register values, these will be
 *			written to the NAND controller registers via DMA
 *
 * @num_cw:		number of steps for the read/write operation
 * @read:		read or write operation
 */
static void update_rw_regs(struct qcom_nand_host *host, int num_cw, bool read)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u32 cmd, cfg0, cfg1, ecc_bch_cfg, read_location0, read_location0_last;

	cmd = PAGE_ACC | LAST_PAGE;

#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	/* For serial NAND the cmd register value as per HPG is
	 * 0x78000034 i.e opcode should be 0x4 = PAGE_READ_WITH_ECC_SPARE
	 * but in kernel we are using opcode 0x3 = PAGE_READ_WITH_ECC
	 * in u-boot 0x4 is being used from parallel nand, this value
	 * check later while debugging and adjust accordingly
	 */
	if (host->quad_mode)
		cmd |= QPIC_SPI_TRANSFER_MODE_x4;
	else
		cmd |= QPIC_SPI_TRANSFER_MODE_x1;
	cmd |= QPIC_SPI_WP | QPIC_SPI_HOLD;
#endif
	if (read) {
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
		if ((nandc->ps_mp_flag & QPIC_PAGE_SCOPE)) {
			cmd |= QPIC_PAGE_SCOPE_CMD_EN;
			cmd &= ~QPIC_MULTI_PAGE_CMD_EN;
		}
		if ((nandc->ps_mp_flag & QPIC_MULTI_PAGE)) {
			cmd |= QPIC_MULTI_PAGE_CMD_EN;
			cmd &= ~QPIC_PAGE_SCOPE_CMD_EN;
		}
#endif
		if (host->use_ecc) {
			cmd |= PAGE_READ_WITH_ECC;
		} else {
			cmd |= PAGE_READ;
		}
	} else {
			cmd |= PROGRAM_PAGE;
	}

	if (host->use_ecc) {
		cfg0 = (host->cfg0 & ~(7U << CW_PER_PAGE)) |
				(num_cw - 1) << CW_PER_PAGE;

		cfg1 = host->cfg1;
		ecc_bch_cfg = host->ecc_bch_cfg;
		if (read) {
			if (host->hw_version >= QPIC_VERSION_V2_0) {
				read_location0_last = (0 << READ_LOCATION_OFFSET) |
					(host->cw_data << READ_LOCATION_SIZE) |
					(1 << READ_LOCATION_LAST);
			}

			read_location0 = (0 << READ_LOCATION_OFFSET) |
				(host->cw_data << READ_LOCATION_SIZE) |
				(1 << READ_LOCATION_LAST);
		}
	} else {
		cfg0 = (host->cfg0_raw & ~(7U << CW_PER_PAGE)) |
				(num_cw - 1) << CW_PER_PAGE;

		cfg1 = host->cfg1_raw;
		ecc_bch_cfg = 1 << ECC_CFG_ECC_DISABLE;
		if (read) {
			if (host->hw_version >= QPIC_VERSION_V2_0) {
				read_location0_last = (0 << READ_LOCATION_OFFSET) |
					(host->cw_size << READ_LOCATION_SIZE) |
					(1 << READ_LOCATION_LAST);
			}

			read_location0 = (0 << READ_LOCATION_OFFSET) |
				(host->cw_size << READ_LOCATION_SIZE) |
				(1 << READ_LOCATION_LAST);
		}
	}

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd);
	nandc_set_reg(nandc, NAND_DEV0_CFG0, cfg0);
	nandc_set_reg(nandc, NAND_DEV0_CFG1, cfg1);
	nandc_set_reg(nandc, NAND_DEV0_ECC_CFG, ecc_bch_cfg);
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	if (nandc->ps_mp_flag & QPIC_PAGE_SCOPE ||
		nandc->ps_mp_flag & QPIC_MULTI_PAGE) {
			nandc_set_reg(nandc, NAND_AUTO_STATUS_EN,
		QPIC_AUTO_STATUS_EN_MASK);
	}
#endif
	nandc_set_reg(nandc, NAND_EBI2_ECC_BUF_CFG, host->ecc_buf_cfg);
	nandc_set_reg(nandc, NAND_FLASH_STATUS, host->clrflashstatus);
	nandc_set_reg(nandc, NAND_READ_STATUS, host->clrreadstatus);
	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	if (read) {
		if (host->hw_version >= QPIC_VERSION_V2_0) {
			nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_0,
					read_location0_last);
		}
		nandc_set_reg(nandc, NAND_READ_LOCATION_0, read_location0);
	}
}

/*
 * Prepares the command descriptor for BAM DMA which will be used for NAND
 * register read and write. The command descriptor requires the command
 * to be formed in command element type so this function uses the command
 * element from bam transaction ce array and fills the same with required
 * data. A single SGL can contain multiple command elements so
 * DMA_DESC_FLAG_BAM_NEXT_SGL will be used for starting the separate SGL
 * after the current command element.
 */
static int prep_dma_desc_command(struct qcom_nand_controller *nandc, bool read,
					int reg_off, const void *vaddr,
					int size, unsigned int flags)
{
	int bam_ce_size;
	int i;
	struct bam_cmd_element *bam_ce_buffer;
	struct bam_transaction *bam_txn = nandc->bam_txn;

	bam_ce_buffer = &bam_txn->bam_ce[bam_txn->bam_ce_index];

	/* fill the command desc */
	for (i = 0; i < size; i++) {
		if (read) {
			qcom_prep_bam_ce(&bam_ce_buffer[i],
				NAND_REG_PHYS_ADDRESS(nandc, reg_off + 4 * i),
				BAM_READ_COMMAND,
				REG_BUF_DMA_ADDR(nandc,
					(unsigned int *)vaddr + i));
		} else {
			qcom_prep_bam_ce(&bam_ce_buffer[i],
				NAND_REG_PHYS_ADDRESS(nandc, reg_off + 4 * i),
				BAM_WRITE_COMMAND,
				*((unsigned int *)vaddr + i));
		}
	}

	/* use the separate sgl after this command */
	if (flags & DMA_DESC_FLAG_BAM_NEXT_SGL) {
		bam_ce_buffer = &bam_txn->bam_ce[bam_txn->pre_bam_ce_index];
		bam_txn->bam_ce_index += size;
		bam_ce_size = (bam_txn->bam_ce_index -
				bam_txn->pre_bam_ce_index) *
				sizeof(struct bam_cmd_element);
		sg_set_buf(&bam_txn->cmd_sgl[bam_txn->cmd_sgl_cnt].sgl,
				bam_ce_buffer,
				bam_ce_size);
		if (flags & DMA_DESC_FLAG_BAM_NWD)
			bam_txn->cmd_sgl[bam_txn->cmd_sgl_cnt].dma_flags =
				DESC_FLAG_NWD | DESC_FLAG_CMD;
		else
			bam_txn->cmd_sgl[bam_txn->cmd_sgl_cnt].dma_flags =
				DESC_FLAG_CMD;

		bam_txn->cmd_sgl_cnt++;
		bam_txn->pre_bam_ce_index = bam_txn->bam_ce_index;
	} else {
		bam_txn->bam_ce_index += size;
	}

	return 0;
}

/*
 * Prepares the data descriptor for BAM DMA which will be used for NAND
 * data read and write.
 */
static int prep_dma_desc_data_bam(struct qcom_nand_controller *nandc, bool read,
					int reg_off, const void *vaddr,
					int size, unsigned int flags)
{
	struct bam_transaction *bam_txn = nandc->bam_txn;

	if (read) {
		sg_set_buf(&bam_txn->rx_sgl[bam_txn->rx_sgl_cnt].sgl,
				vaddr, size);
		bam_txn->rx_sgl[bam_txn->rx_sgl_cnt].dma_flags = 0;
		bam_txn->rx_sgl_cnt++;

	} else {
		sg_set_buf(&bam_txn->tx_sgl[bam_txn->tx_sgl_cnt].sgl,
				vaddr, size);
		if (flags & DMA_DESC_FLAG_NO_EOT)
			bam_txn->tx_sgl[bam_txn->tx_sgl_cnt].dma_flags = 0;
		else
			bam_txn->tx_sgl[bam_txn->tx_sgl_cnt].dma_flags =
				DESC_FLAG_EOT;

		bam_txn->tx_sgl_cnt++;
	}

	return 0;
}

/* Prepares the dma desciptor for adm dma engine */
static int prep_dma_desc(struct qcom_nand_controller *nandc, bool read,
			 int reg_off, const void *vaddr, int size,
			 bool flow_control)
{
	struct desc_info *desc;
	struct dma_async_tx_descriptor *dma_desc;
	struct scatterlist *sgl;
	struct dma_slave_config slave_conf;
	enum dma_transfer_direction dir_eng;
	int ret;

	desc = kzalloc(sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	sgl = &desc->sgl;

	sg_init_one(sgl, vaddr, size);

	if (read) {
		dir_eng = DMA_DEV_TO_MEM;
		desc->dir = DMA_FROM_DEVICE;
	} else {
		dir_eng = DMA_MEM_TO_DEV;
		desc->dir = DMA_TO_DEVICE;
	}

	ret = dma_map_sg(nandc->dev, sgl, 1, desc->dir);
	if (ret == 0) {
		ret = -ENOMEM;
		goto err;
	}

	memset(&slave_conf, 0x00, sizeof(slave_conf));

	slave_conf.device_fc = flow_control;
	if (read) {
		slave_conf.src_maxburst = 16;
		slave_conf.src_addr = nandc->base_dma + reg_off;
		slave_conf.slave_id = nandc->data_crci;
	} else {
		slave_conf.dst_maxburst = 16;
		slave_conf.dst_addr = nandc->base_dma + reg_off;
		slave_conf.slave_id = nandc->cmd_crci;
	}

	ret = dmaengine_slave_config(nandc->chan, &slave_conf);
	if (ret) {
		dev_err(nandc->dev, "failed to configure dma channel\n");
		goto err;
	}

	dma_desc = dmaengine_prep_slave_sg(nandc->chan, sgl, 1, dir_eng, 0);
	if (!dma_desc) {
		dev_err(nandc->dev, "failed to prepare desc\n");
		ret = -EINVAL;
		goto err;
	}

	desc->dma_desc = dma_desc;

	list_add_tail(&desc->node, &nandc->desc_list);

	return 0;
err:
	kfree(desc);

	return ret;
}

/*
 * read_reg_dma:	prepares a descriptor to read a given number of
 *			contiguous registers to the reg_read_buf pointer
 *
 * @first:		offset of the first register in the contiguous block
 * @num_regs:		number of registers to read
 */
static int read_reg_dma(struct qcom_nand_controller *nandc, int first,
			int num_regs, unsigned int flags)
{
	bool flow_control = false;
	void *vaddr;
	int size;

	if (first == NAND_READ_ID || first == NAND_FLASH_STATUS)
		flow_control = true;

	vaddr = nandc->reg_read_buf + nandc->reg_read_pos;
	nandc->reg_read_pos += num_regs;

	if (nandc->dma_bam_enabled) {
		size = num_regs;

		return prep_dma_desc_command(nandc, true,
				nandc->regs_offsets[first], vaddr, size,
				flags);
	}

	size = num_regs * sizeof(u32);

	return prep_dma_desc(nandc, true, nandc->regs_offsets[first], vaddr,
			size, flow_control);
}

/*
 * write_reg_dma:	prepares a descriptor to write a given number of
 *			contiguous registers
 *
 * @first:		offset of the first register in the contiguous block
 * @num_regs:		number of registers to write
 */
static int write_reg_dma(struct qcom_nand_controller *nandc, int first,
			 int num_regs, unsigned int flags)
{
	bool flow_control = false;
	struct nandc_regs *regs = nandc->regs;
	void *vaddr;
	int size;

	vaddr = offset_to_nandc_reg(regs, first);

	if (first == NAND_FLASH_CMD)
		flow_control = true;

	if (first == NAND_ERASED_CW_DETECT_CFG) {
		if (flags & DMA_DESC_ERASED_CW_SET)
			vaddr = &regs->erased_cw_detect_cfg_set;
		else
			vaddr = &regs->erased_cw_detect_cfg_clr;
	}

	if (first == NAND_EXEC_CMD)
		flags |= DMA_DESC_FLAG_BAM_NWD;

	if (first == NAND_DEV_CMD1_RESTORE)
		first = NAND_DEV_CMD1;

	if (first == NAND_DEV_CMD_VLD_RESTORE)
		first = NAND_DEV_CMD_VLD;

	if (nandc->dma_bam_enabled) {
		size = num_regs;

		return prep_dma_desc_command(nandc, false,
				nandc->regs_offsets[first], vaddr, size,
				flags);
	}

	size = num_regs * sizeof(u32);

	return prep_dma_desc(nandc, false, nandc->regs_offsets[first], vaddr,
			size, flow_control);
}

/*
 * read_data_dma:	prepares a DMA descriptor to transfer data from the
 *			controller's internal buffer to the buffer 'vaddr'
 *
 * @reg_off:		offset within the controller's data buffer
 * @vaddr:		virtual address of the buffer we want to write to
 * @size:		DMA transaction size in bytes
 */
static int read_data_dma(struct qcom_nand_controller *nandc, int reg_off,
			 const u8 *vaddr, int size, unsigned int flags)
{
	if (nandc->dma_bam_enabled)
		return prep_dma_desc_data_bam(nandc, true,
			nandc->regs_offsets[FLASH_BUF_ACC] + reg_off -
			FLASH_BUF_ACC, vaddr, size, flags);

	return prep_dma_desc(nandc, true,
		nandc->regs_offsets[FLASH_BUF_ACC] + reg_off - FLASH_BUF_ACC,
		vaddr, size, false);
}

/*
 * write_data_dma:	prepares a DMA descriptor to transfer data from
 *			'vaddr' to the controller's internal buffer
 *
 * @reg_off:		offset within the controller's data buffer
 * @vaddr:		virtual address of the buffer we want to read from
 * @size:		DMA transaction size in bytes
 */
static int write_data_dma(struct qcom_nand_controller *nandc, int reg_off,
			  const u8 *vaddr, int size, unsigned int flags)
{
	if (nandc->dma_bam_enabled)
		return prep_dma_desc_data_bam(nandc, false,
			nandc->regs_offsets[FLASH_BUF_ACC] + reg_off -
			FLASH_BUF_ACC, vaddr, size, flags);

	return prep_dma_desc(nandc, false,
		nandc->regs_offsets[FLASH_BUF_ACC] + reg_off - FLASH_BUF_ACC,
		vaddr, size, false);
}

/*
 * helper to prepare dma descriptors to configure registers needed for reading a
 * codeword/step in a page
 */
static void config_cw_read(struct qcom_nand_host *host,
		struct qcom_nand_controller *nandc, bool use_ecc)
{
	write_reg_dma(nandc, NAND_FLASH_CMD, 3, 0);
	write_reg_dma(nandc, NAND_DEV0_CFG0, 3, 0);
	write_reg_dma(nandc, NAND_EBI2_ECC_BUF_CFG, 1, 0);

	write_reg_dma(nandc, NAND_ERASED_CW_DETECT_CFG, 1, 0);
	write_reg_dma(nandc, NAND_ERASED_CW_DETECT_CFG, 1,
				DMA_DESC_ERASED_CW_SET);
	if (nandc->dma_bam_enabled) {
		if (host->hw_version >= QPIC_VERSION_V2_0)
			write_reg_dma(nandc, NAND_READ_LOCATION_LAST_CW_0, 1,
					DMA_DESC_FLAG_BAM_NEXT_SGL);
		else
			write_reg_dma(nandc, NAND_READ_LOCATION_0, 1,
					DMA_DESC_FLAG_BAM_NEXT_SGL);
	}

	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NWD |
				DMA_DESC_FLAG_BAM_NEXT_SGL);

	if (use_ecc) {
		read_reg_dma(nandc, NAND_FLASH_STATUS, 2, 0);
		read_reg_dma(nandc, NAND_ERASED_CW_DETECT_STATUS, 1,
			     DMA_DESC_FLAG_BAM_NEXT_SGL);
	} else {
		read_reg_dma(nandc, NAND_FLASH_STATUS, 1,
			     DMA_DESC_FLAG_BAM_NEXT_SGL);
	}
}

/*
 * Helpers to prepare DMA descriptors for configuring registers
 * before reading a NAND page with BAM.
 */
static void config_bam_page_read(struct qcom_nand_controller *nandc)
{
	uint32_t cnt = 0x3;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	cnt = 0x4;
#endif
	write_reg_dma(nandc, NAND_FLASH_CMD, 3, 0);
	write_reg_dma(nandc, NAND_DEV0_CFG0, cnt, 0);
	write_reg_dma(nandc, NAND_EBI2_ECC_BUF_CFG, 1, 0);
	write_reg_dma(nandc, NAND_ERASED_CW_DETECT_CFG, 1, 0);
	write_reg_dma(nandc, NAND_ERASED_CW_DETECT_CFG, 1,
				DMA_DESC_ERASED_CW_SET |
				DMA_DESC_FLAG_BAM_NEXT_SGL);
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	if (nandc->ps_mp_flag & QPIC_MULTI_PAGE)
		write_reg_dma(nandc, NAND_MULTI_PAGE_CMD, 1,
				DMA_DESC_FLAG_BAM_NEXT_SGL);
#endif
}

/*
 * Helpers to prepare DMA descriptors for configuring registers
 * before reading each codeword in NAND page with BAM.
 */
static void config_bam_cw_read(struct qcom_nand_controller *nandc, bool use_ecc)
{
	if (nandc->dma_bam_enabled) {
		write_reg_dma(nandc, NAND_READ_LOCATION_0, 4, 0);
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
		write_reg_dma(nandc, NAND_READ_LOCATION_LAST_CW_0, 4, 0);
#endif
	}
	/* for page scope and multi page read status will be read
	 * via separate status pipe so no need to queue descriptor
	 * to read the status register directaly.
	 * For page scope and multi page read command the exe pulse
	 * should be only once per page or for all page.
	 */
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	if ((nandc->ps_mp_flag & PAGE_SCOPE_MULTI_PAGE_CMD_EXE)) {

		write_reg_dma(nandc, NAND_FLASH_CMD, 1,
				DMA_DESC_FLAG_BAM_NEXT_SGL);
		write_reg_dma(nandc, NAND_EXEC_CMD, 1,
				DMA_DESC_FLAG_BAM_NEXT_SGL);
		nandc->ps_mp_flag &= ~PAGE_SCOPE_MULTI_PAGE_CMD_EXE;
	}
#else
	write_reg_dma(nandc, NAND_FLASH_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	if (use_ecc) {
		read_reg_dma(nandc, NAND_FLASH_STATUS, 2, 0);
		read_reg_dma(nandc, NAND_ERASED_CW_DETECT_STATUS, 1,
				DMA_DESC_FLAG_BAM_NEXT_SGL);
	} else {
		read_reg_dma(nandc, NAND_FLASH_STATUS, 1,
				DMA_DESC_FLAG_BAM_NEXT_SGL);
	}
#endif
}

/*
 * helpers to prepare dma descriptors used to configure registers needed for
 * writing a codeword/step in a page
 */
static void config_cw_write_pre(struct qcom_nand_controller *nandc)
{
	write_reg_dma(nandc, NAND_FLASH_CMD, 3, 0);
	write_reg_dma(nandc, NAND_DEV0_CFG0, 3, 0);
	write_reg_dma(nandc, NAND_EBI2_ECC_BUF_CFG, 1,
				DMA_DESC_FLAG_BAM_NEXT_SGL);
}

static void config_cw_write_post(struct qcom_nand_controller *nandc)
{
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_STATUS, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_STATUS, 1, 0);
	write_reg_dma(nandc, NAND_READ_STATUS, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
}

/*
 * the following functions are used within chip->cmdfunc() to perform different
 * NAND_CMD_* commands
 */

/* sets up descriptors for NAND_CMD_PARAM */
static int nandc_param(struct qcom_nand_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	clear_bam_transaction(nandc);

	/*
	 * NAND_CMD_PARAM is called before we know much about the FLASH chip
	 * in use. we configure the controller to perform a raw read of 512
	 * bytes to read onfi params
	 */
	nandc_set_reg(nandc, NAND_FLASH_CMD, PAGE_READ | PAGE_ACC | LAST_PAGE);
	nandc_set_reg(nandc, NAND_ADDR0, 0);
	nandc_set_reg(nandc, NAND_ADDR1, 0);
	nandc_set_reg(nandc, NAND_DEV0_CFG0, 0 << CW_PER_PAGE
					| 512 << UD_SIZE_BYTES
					| nandc->param_read_addr_cycle << NUM_ADDR_CYCLES
					| 0 << SPARE_SIZE_BYTES);
	nandc_set_reg(nandc, NAND_DEV0_CFG1, 7 << NAND_RECOVERY_CYCLES
					| 0 << CS_ACTIVE_BSY
					| 17 << BAD_BLOCK_BYTE_NUM
					| 1 << BAD_BLOCK_IN_SPARE_AREA
					| 2 << WR_RD_BSY_GAP
					| 0 << WIDE_FLASH
					| 1 << DEV0_CFG1_ECC_DISABLE);
	nandc_set_reg(nandc, NAND_EBI2_ECC_BUF_CFG, 1 << ECC_CFG_ECC_DISABLE);

	/* configure CMD1 and VLD for ONFI param probing */
	nandc_set_reg(nandc, NAND_DEV_CMD_VLD,
		      (nandc->vld & ~(1 << READ_START_VLD))
		      | 0 << READ_START_VLD);
	nandc_set_reg(nandc, NAND_DEV_CMD1,
		      (nandc->cmd1 & ~(0xFF << READ_ADDR))
		      | NAND_CMD_PARAM << READ_ADDR);

	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	nandc_set_reg(nandc, NAND_DEV_CMD1_RESTORE, nandc->cmd1);
	nandc_set_reg(nandc, NAND_DEV_CMD_VLD_RESTORE, nandc->vld);
	nandc_set_reg(nandc, NAND_READ_LOCATION_0,
				(0 << READ_LOCATION_OFFSET) |
				(512 << READ_LOCATION_SIZE) |
				(1 << READ_LOCATION_LAST));

	write_reg_dma(nandc, NAND_DEV_CMD_VLD, 1, 0);
	write_reg_dma(nandc, NAND_DEV_CMD1, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	nandc->buf_count = 512;
	memset(nandc->data_buffer, 0xff, nandc->buf_count);

	config_cw_read(host, nandc, false);

	read_data_dma(nandc, FLASH_BUF_ACC, nandc->data_buffer,
		      nandc->buf_count, 0);

	/* restore CMD1 and VLD regs */
	write_reg_dma(nandc, NAND_DEV_CMD1_RESTORE, 1, 0);
	write_reg_dma(nandc, NAND_DEV_CMD_VLD_RESTORE, 1,
				DMA_DESC_FLAG_BAM_NEXT_SGL);

	return 0;
}

/* sets up descriptors for NAND_CMD_ERASE1 */
static int erase_block(struct qcom_nand_host *host, int page_addr)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u32 erase_val = (BLOCK_ERASE | PAGE_ACC | LAST_PAGE);
	u32 addr1 = 0x0;

	clear_bam_transaction(nandc);

#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	erase_val = (BLOCK_ERASE | QPIC_SPI_TRANSFER_MODE_x1 |
			QPIC_SPI_WP | QPIC_SPI_HOLD);
	addr1 = (page_addr >> 16) & 0xffff;
	page_addr <<= 16;
#endif
	nandc_set_reg(nandc, NAND_FLASH_CMD, erase_val);
	nandc_set_reg(nandc, NAND_ADDR0, page_addr);
	nandc_set_reg(nandc, NAND_ADDR1, addr1);

	nandc_set_reg(nandc, NAND_DEV0_CFG0,
		      host->cfg0_raw & ~(7 << CW_PER_PAGE));
	nandc_set_reg(nandc, NAND_DEV0_CFG1, host->cfg1_raw);

	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	nandc_set_reg(nandc, NAND_FLASH_STATUS, host->clrflashstatus);
	nandc_set_reg(nandc, NAND_READ_STATUS, host->clrreadstatus);

	write_reg_dma(nandc, NAND_FLASH_CMD, 3, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_DEV0_CFG0, 2, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_STATUS, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_STATUS, 1, 0);
	write_reg_dma(nandc, NAND_READ_STATUS, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	return 0;
}

/* sets up descriptors for NAND_CMD_READID */
static int read_id(struct qcom_nand_host *host, int column)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u32 cmd = FETCH_ID;
	if (column == -1)
		return 0;

#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	cmd = (FETCH_ID | QPIC_SPI_TRANSFER_MODE_x1 |
			QPIC_SPI_WP | QPIC_SPI_HOLD);
	/* For spi nand read 2-bytes id only
	 * else if nandc->buf_count == 4; then the id value
	 * will repeat and the SLC device will detect as MLC.
	 * so set the nandc->buf_count == 2;
	 */
	nandc->buf_count = 2;
#endif
	clear_bam_transaction(nandc);

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd);
	nandc_set_reg(nandc, NAND_ADDR0, column);
	nandc_set_reg(nandc, NAND_ADDR1, 0);
	nandc_set_reg(nandc, NAND_FLASH_CHIP_SELECT,
			nandc->dma_bam_enabled ? 0 : DM_EN);
	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_FLASH_CMD, 4, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_READ_ID, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	return 0;
}

/* sets up descriptors for NAND_CMD_RESET */
static int reset(struct qcom_nand_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	clear_bam_transaction(nandc);
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	/* This reset sequence as per QPIC2.0 HPG
	 * NAND_DEV0_CFG = 0x005400D0
	 * NAND_DEVn_CFG1 = 0x87476B1
	 * NAND_DEV0_ECC_CFG = 0x02000001
	 * NAND_FLASH_CMD = 0x3800000D
	 * NAND_EXEC_CMD = 0x00000001
	 */
	nandc_set_reg(nandc, NAND_DEV0_CFG0, NAND_DEV0_CFG0_RESET);
	nandc_set_reg(nandc, NAND_DEV0_CFG1, NAND_DEV0_CFG1_VAL_RST);
	nandc_set_reg(nandc, NAND_DEV0_ECC_CFG, NAND_DEV0_ECC_CFG_RESET);
	nandc_set_reg(nandc, NAND_FLASH_CMD, RESET_DEVICE_SERIAL);
	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_DEV0_CFG0, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_DEV0_CFG1, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_DEV0_ECC_CFG, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_FLASH_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
#else
	nandc_set_reg(nandc, NAND_FLASH_CMD, RESET_DEVICE);
	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_FLASH_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

#endif
	read_reg_dma(nandc, NAND_FLASH_STATUS, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	return 0;
}

static int prepare_bam_async_desc(struct qcom_nand_controller *nandc,
				struct dma_chan *chan,
				struct qcom_bam_sgl *bam_sgl,
				int sgl_cnt,
				enum dma_transfer_direction direction)
{
	struct desc_info *desc;
	struct dma_async_tx_descriptor *dma_desc;

	if (!qcom_bam_map_sg(nandc->dev, bam_sgl, sgl_cnt, direction)) {
		dev_err(nandc->dev, "failure in mapping sgl\n");
		return -ENOMEM;
	}

	desc = kzalloc(sizeof(*desc), GFP_KERNEL);
	if (!desc) {
		qcom_bam_unmap_sg(nandc->dev, bam_sgl, sgl_cnt, direction);
		return -ENOMEM;
	}


	desc->bam_desc_data.dir = direction;
	desc->bam_desc_data.sgl_cnt = sgl_cnt;
	desc->bam_desc_data.bam_sgl = bam_sgl;

	dma_desc = dmaengine_prep_dma_custom_mapping(chan,
			&desc->bam_desc_data,
			0);

	if (!dma_desc) {
		dev_err(nandc->dev, "failure in cmd prep desc\n");
		qcom_bam_unmap_sg(nandc->dev, bam_sgl, sgl_cnt, direction);
		kfree(desc);
		return -EINVAL;
	}

	desc->dma_desc = dma_desc;
	dma_desc->callback = qpic_bam_dma_done;
	dma_desc->callback_param = nandc->bam_txn;

	list_add_tail(&desc->node, &nandc->desc_list);

	return 0;

}

/* helpers to submit/free our list of dma descriptors */
static int submit_descs(struct qcom_nand_controller *nandc)
{
	struct desc_info *desc;
	dma_cookie_t cookie = 0;
	struct bam_transaction *bam_txn = nandc->bam_txn;
	int r;

	if (nandc->dma_bam_enabled) {
		if (bam_txn->rx_sgl_cnt) {
			r = prepare_bam_async_desc(nandc, nandc->rx_chan,
				bam_txn->rx_sgl, bam_txn->rx_sgl_cnt,
				DMA_DEV_TO_MEM);
			if (r)
				return r;

			bam_txn->chans_cnt++;
		}
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
		if (bam_txn->status_sgl_cnt) {
			r = prepare_bam_async_desc(nandc, nandc->status_chan,
				bam_txn->status_sgl, bam_txn->status_sgl_cnt,
				DMA_DEV_TO_MEM);
			if (r)
				return r;

			bam_txn->chans_cnt++;
		}
#endif
		if (bam_txn->tx_sgl_cnt) {
			r = prepare_bam_async_desc(nandc, nandc->tx_chan,
				bam_txn->tx_sgl, bam_txn->tx_sgl_cnt,
				DMA_MEM_TO_DEV);
			if (r)
				return r;

			bam_txn->chans_cnt++;
		}

		r = prepare_bam_async_desc(nandc, nandc->cmd_chan,
			bam_txn->cmd_sgl, bam_txn->cmd_sgl_cnt,
			DMA_MEM_TO_DEV);
		if (r)
			return r;
	}

	list_for_each_entry(desc, &nandc->desc_list, node)
		cookie = dmaengine_submit(desc->dma_desc);

	if (nandc->dma_bam_enabled) {
		dma_async_issue_pending(nandc->tx_chan);
		dma_async_issue_pending(nandc->rx_chan);
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
		dma_async_issue_pending(nandc->status_chan);
#endif
		dma_async_issue_pending(nandc->cmd_chan);

		if (!wait_for_completion_timeout(&bam_txn->txn_done, QPIC_NAND_COMPLETION_TIMEOUT))
			return -ETIMEDOUT;
	} else {
		if (dma_sync_wait(nandc->chan, cookie) != DMA_COMPLETE)
			return -ETIMEDOUT;
	}

	return 0;
}

static void free_descs(struct qcom_nand_controller *nandc)
{
	struct desc_info *desc, *n;

	list_for_each_entry_safe(desc, n, &nandc->desc_list, node) {
		list_del(&desc->node);

		if (nandc->dma_bam_enabled)
			qcom_bam_unmap_sg(nandc->dev,
				desc->bam_desc_data.bam_sgl,
				desc->bam_desc_data.sgl_cnt,
				desc->bam_desc_data.dir);
		else
			dma_unmap_sg(nandc->dev, &desc->sgl, 1,
				desc->dir);

		kfree(desc);
	}
}

/* reset the register read buffer for next NAND operation */
static void clear_read_regs(struct qcom_nand_controller *nandc)
{
	nandc->reg_read_pos = 0;
	memset(nandc->reg_read_buf, 0,
	       MAX_REG_RD * sizeof(*nandc->reg_read_buf));
}

static void pre_command(struct qcom_nand_host *host, int command)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	nandc->buf_count = 0;
	nandc->buf_start = 0;
	host->use_ecc = false;
	host->last_command = command;

	clear_read_regs(nandc);
}

/*
 * this is called after NAND_CMD_PAGEPROG and NAND_CMD_ERASE1 to set our
 * privately maintained status byte, this status byte can be read after
 * NAND_CMD_STATUS is called
 */
static void parse_erase_write_errors(struct qcom_nand_host *host, int command)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int num_cw;
	int i;

	num_cw = command == NAND_CMD_PAGEPROG ? ecc->steps : 1;

	for (i = 0; i < num_cw; i++) {
		u32 flash_status = le32_to_cpu(nandc->reg_read_buf[i]);

		if (flash_status & FS_MPU_ERR)
			host->status &= ~NAND_STATUS_WP;

		if (flash_status & FS_OP_ERR || (i == (num_cw - 1) &&
						 (flash_status &
						  FS_DEVICE_STS_ERR)))
			host->status |= NAND_STATUS_FAIL;
	}
}

static void post_command(struct qcom_nand_host *host, int command)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	switch (command) {
	case NAND_CMD_READID:
		memcpy(nandc->data_buffer, nandc->reg_read_buf,
		       nandc->buf_count);
		break;
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
		parse_erase_write_errors(host, command);
		break;
	default:
		break;
	}
}

/*
 * Implements chip->cmdfunc. It's  only used for a limited set of commands.
 * The rest of the commands wouldn't be called by upper layers. For example,
 * NAND_CMD_READOOB would never be called because we have our own versions
 * of read_oob ops for nand_ecc_ctrl.
 */
static void qcom_nandc_command(struct mtd_info *mtd, unsigned int command,
			       int column, int page_addr)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	bool wait = false;
	int ret = 0;

	pre_command(host, command);

	switch (command) {
	case NAND_CMD_RESET:
		ret = reset(host);
		wait = true;
		break;

	case NAND_CMD_READID:
		nandc->buf_count = 4;
		ret = read_id(host, column);
		wait = true;
		break;

	case NAND_CMD_PARAM:
		ret = nandc_param(host);
		wait = true;
		break;

	case NAND_CMD_ERASE1:
		ret = erase_block(host, page_addr);
		wait = true;
		break;

	case NAND_CMD_READ0:
		/* we read the entire page for now */
		WARN_ON(column != 0);

		host->use_ecc = true;
		set_address(host, 0, page_addr);
		update_rw_regs(host, ecc->steps, true);
		break;

	case NAND_CMD_SEQIN:
		WARN_ON(column != 0);
		set_address(host, 0, page_addr);
		break;

	case NAND_CMD_PAGEPROG:
	case NAND_CMD_STATUS:
	case NAND_CMD_NONE:
	default:
		break;
	}

	if (ret) {
		dev_err(nandc->dev, "failure executing command %d\n",
			command);
		free_descs(nandc);
		return;
	}

	if (wait) {
		ret = submit_descs(nandc);
		if (ret)
			dev_err(nandc->dev,
				"failure submitting descs for command %d\n",
				command);
	}
	free_descs(nandc);

	post_command(host, command);
}

/*
 * when using BCH ECC, the HW flags an error in NAND_FLASH_STATUS if it read
 * an erased CW, and reports an erased CW in NAND_ERASED_CW_DETECT_STATUS.
 *
 * when using RS ECC, the HW reports the same erros when reading an erased CW,
 * but it notifies that it is an erased CW by placing special characters at
 * certain offsets in the buffer.
 *
 * verify if the page is erased or not, and fix up the page for RS ECC by
 * replacing the special characters with 0xff.
 */
static bool erased_chunk_check_and_fixup(u8 *data_buf, int data_len)
{
	u8 empty1, empty2;

	/*
	 * an erased page flags an error in NAND_FLASH_STATUS, check if the page
	 * is erased by looking for 0x54s at offsets 3 and 175 from the
	 * beginning of each codeword
	 */

	empty1 = data_buf[3];
	empty2 = data_buf[175];

	/*
	 * if the erased codework markers, if they exist override them with
	 * 0xffs
	 */
	if ((empty1 == 0x54 && empty2 == 0xff) ||
	    (empty1 == 0xff && empty2 == 0x54)) {
		data_buf[3] = 0xff;
		data_buf[175] = 0xff;
	}

	/*
	 * check if the entire chunk contains 0xffs or not. if it doesn't, then
	 * restore the original values at the special offsets
	 */
	if (memchr_inv(data_buf, 0xff, data_len)) {
		data_buf[3] = empty1;
		data_buf[175] = empty2;

		return false;
	}

	return true;
}

struct read_stats {
	__le32 flash;
	__le32 buffer;
	__le32 erased_cw;
};

/* reads back FLASH_STATUS register set by the controller */
static int check_flash_errors(struct qcom_nand_host *host, int cw_cnt)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int i;

	for (i = 0; i < cw_cnt; i++) {
		u32 flash = le32_to_cpu(nandc->reg_read_buf[i]);

		if (flash & (FS_OP_ERR | FS_MPU_ERR))
			return -EIO;
	}

	return 0;
}

#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
static void parse_status_buf(struct qcom_nand_controller *nandc,
		u8 *status_buf, int n_iter)
{
	int i, j = 0;
	__le32 tmp = 0x0;
	u8 *tmp_buf = status_buf;

	for (i = 0; i < n_iter; i++) {
		tmp |= tmp_buf[i++];
		tmp <<= 8;
		tmp |= tmp_buf[i++];
		tmp <<= 8;
		tmp |= tmp_buf[i++];
		tmp <<= 8;
		tmp |= tmp_buf[i];
		tmp = ((tmp >> 24) & 0xff) |
			((tmp << 8) & 0xff0000) |
			((tmp >> 8) & 0xff00) |
			((tmp << 24) & 0xff000000);
		nandc->reg_read_buf[j++] = tmp;
		tmp = 0x0;
	}
}

static int prep_dma_desc_status_bam(struct qcom_nand_controller *nandc,
		const void *vaddr, int size, unsigned int flags)
{
	struct bam_transaction *bam_txn = nandc->bam_txn;

	sg_set_buf(&bam_txn->status_sgl[bam_txn->status_sgl_cnt].sgl,
			vaddr, size);
	bam_txn->status_sgl[bam_txn->status_sgl_cnt].dma_flags = 0;
	bam_txn->status_sgl_cnt++;

	return 0;
}
#endif

/*
 * Helper to perform the page raw read operation. The read_cw_mask will be
 * used to specify the codewords for which the data should be read. The
 * single page contains multiple CW. Sometime, only few CW data is required
 * in complete page. Also, the starting start address will be determined with
 * this CW mask to skip unnecessary data copy from NAND flash device. Then,
 * actual data copy from NAND controller to data buffer will be done only
 * for the CWs which have the mask set.
 */
static int nandc_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
		    uint8_t *data_buf, uint8_t *oob_buf,
		    int page, unsigned long read_cw_mask)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int i, ret;
	int read_location, start_step, last_step;

#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	u8 *tmp_status_buf = NULL;
	int status_buf_parse_size = 0x0;
	memset(nandc->status_buf, 0, nandc->status_buf_size);
	tmp_status_buf = nandc->status_buf;
	nandc->ps_mp_flag |= QPIC_PAGE_SCOPE;
	nandc->ps_mp_flag &= ~QPIC_MULTI_PAGE;
	/*Here once again upadate the register value , because
	 * for each read access upper layer will call the bad block
	 * check and for bad block check we are using legacy page read
	 * instead of page scope and multi page read.
	 */
	update_rw_regs(host, ecc->steps, true);
#endif
	host->use_ecc = false;
	start_step = ffs(read_cw_mask) - 1;
	last_step = fls(read_cw_mask);

	clear_bam_transaction(nandc);
	set_address(host, host->cw_size * start_step, page);
	update_rw_regs(host, last_step - start_step, true);

	if (nandc->dma_bam_enabled)
		config_bam_page_read(nandc);

	for (i = start_step; i < last_step; i++) {
		int data_size1, data_size2, oob_size1, oob_size2;
		int reg_off = FLASH_BUF_ACC;

		data_size1 = mtd->writesize - host->cw_size * (ecc->steps - 1);
		oob_size1 = host->bbm_size;

		if ((i == (ecc->steps - 1)) &&  !nandc->boot_layout) {
			data_size2 = ecc->size - data_size1 -
				     ((ecc->steps - 1) << 2);
			oob_size2 = (ecc->steps << 2) + host->ecc_bytes_hw +
				    host->spare_bytes;
		} else {
			data_size2 = host->cw_data - data_size1;
			oob_size2 = host->ecc_bytes_hw + host->spare_bytes;
		}

		if (!(read_cw_mask & BIT(i))) {
			if (nandc->dma_bam_enabled) {
				nandc_set_reg(nandc, NAND_READ_LOCATION_0,
					(0 << READ_LOCATION_OFFSET) |
					(0 << READ_LOCATION_SIZE) |
					(1 << READ_LOCATION_LAST));
				config_bam_cw_read(nandc, false);
			} else {
				config_cw_read(host, nandc, false);
			}

			data_buf += data_size1 + data_size2;
			oob_buf += oob_size1 + oob_size2;

			continue;
		}

		if (nandc->dma_bam_enabled) {
			read_location = 0;
			if ((i == (ecc->steps - 1)) && host->hw_version >= QPIC_VERSION_V2_0) {
				nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_0,
					(read_location << READ_LOCATION_OFFSET) |
					(data_size1 << READ_LOCATION_SIZE) |
					(0 << READ_LOCATION_LAST));
					read_location += data_size1;

				nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_1,
					(read_location << READ_LOCATION_OFFSET) |
					(oob_size1 << READ_LOCATION_SIZE) |
					(0 << READ_LOCATION_LAST));
					read_location += oob_size1;

				nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_2,
					(read_location << READ_LOCATION_OFFSET) |
					(data_size2 << READ_LOCATION_SIZE) |
					(0 << READ_LOCATION_LAST));
					read_location += data_size2;

				nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_3,
					(read_location << READ_LOCATION_OFFSET) |
					(oob_size2 << READ_LOCATION_SIZE) |
					(1 << READ_LOCATION_LAST));
			}

			nandc_set_reg(nandc, NAND_READ_LOCATION_0,
				(read_location << READ_LOCATION_OFFSET) |
				(data_size1 << READ_LOCATION_SIZE) |
				(0 << READ_LOCATION_LAST));
				read_location += data_size1;

			nandc_set_reg(nandc, NAND_READ_LOCATION_1,
				(read_location << READ_LOCATION_OFFSET) |
				(oob_size1 << READ_LOCATION_SIZE) |
				(0 << READ_LOCATION_LAST));
				read_location += oob_size1;

			nandc_set_reg(nandc, NAND_READ_LOCATION_2,
				(read_location << READ_LOCATION_OFFSET) |
				(data_size2 << READ_LOCATION_SIZE) |
				(0 << READ_LOCATION_LAST));
				read_location += data_size2;

			nandc_set_reg(nandc, NAND_READ_LOCATION_3,
				(read_location << READ_LOCATION_OFFSET) |
				(oob_size2 << READ_LOCATION_SIZE) |
				(1 << READ_LOCATION_LAST));
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
			if (i == (last_step - 1))
				nandc->ps_mp_flag |= PAGE_SCOPE_MULTI_PAGE_CMD_EXE;
#endif
			config_bam_cw_read(nandc, false);
		} else {
			config_cw_read(host, nandc, false);
		}

		read_data_dma(nandc, reg_off, data_buf, data_size1, 0);
		reg_off += data_size1;
		data_buf += data_size1;

		read_data_dma(nandc, reg_off, oob_buf, oob_size1, 0);
		reg_off += oob_size1;
		oob_buf += oob_size1;

		read_data_dma(nandc, reg_off, data_buf, data_size2, 0);
		reg_off += data_size2;
		data_buf += data_size2;

		read_data_dma(nandc, reg_off, oob_buf, oob_size2, 0);
		oob_buf += oob_size2;

#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
		if (tmp_status_buf) {
			prep_dma_desc_status_bam(nandc, (void *)tmp_status_buf,
					MAX_STATUS_REG, 0);
			tmp_status_buf += MAX_STATUS_REG;
		}
#endif
	}

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure to read raw page\n");

	free_descs(nandc);

	if (!ret) {
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	GET_STATUS_BUFF_PARSE_SIZE_PER_PAGE(mtd->writesize,
			status_buf_parse_size);
	parse_status_buf(nandc, nandc->status_buf,
			status_buf_parse_size);
#endif
		ret = check_flash_errors(host, last_step - start_step);
	}

	return 0;
}

/*
 * Bitflips can happened in erased codewords also so this function counts the
 * number of 0 in each CW for which ECC engine returns the uncorrectable
 * error. The page will be assumed as erased if this count is less than or
 * equal to the ecc->strength for each CW.
 *
 * 1. Both DATA and OOB need to be checked for number of 0. The
 *    top-level API can be called with only data buf or oob buf so use
 *    chip->buffers->databuf if data buf is null and chip->oob_poi if oob buf
 *    is null for copying the raw bytes.
 * 2. Perform raw read for all the CW which has uncorrectable errors.
 * 3. For each CW, check the number of 0 in cw_data and usable oob bytes.
 *    The bbm and spare bytes bit flip won’t affect the ECC so don’t check
 *    the number of bitflips in this area.
 */
static int
check_for_erased_page(struct qcom_nand_host *host, u8 *data_buf,
		      u8 *oob_buf, unsigned long uncorrectable_err_cws,
		      int page, unsigned int max_bitflips, bool last_cw)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int i, start_step, last_step, ret = 0;

	start_step = ffs(uncorrectable_err_cws) - 1;
	last_step = fls(uncorrectable_err_cws);

	if (!last_cw) {
		if (!data_buf)
			data_buf = chip->buffers->databuf;
		if (!oob_buf)
			oob_buf = chip->oob_poi;
		data_buf += start_step * host->cw_data;
		oob_buf += start_step * ecc->bytes;
	}

	clear_read_regs(nandc);
	nandc_read_page_raw(mtd, chip, data_buf, oob_buf, page,
			    uncorrectable_err_cws);

	for (i = start_step; i < last_step; i++) {
		int data_size, oob_size;

		if ((i == (ecc->steps - 1)) && !nandc->boot_layout) {
			data_size = ecc->size - ((ecc->steps - 1) << 2);
			oob_size = (ecc->steps << 2) + host->ecc_bytes_hw;
		} else {
			data_size = host->cw_data;
			oob_size = host->ecc_bytes_hw;
		}

		if (uncorrectable_err_cws & BIT(i)) {
			/*
			 * make sure it isn't an erased page reported
			 * as not-erased by HW because of a few bitflips
			 */
			ret = nand_check_erased_ecc_chunk(data_buf,
				data_size, oob_buf + host->bbm_size,
				oob_size, NULL,
				0, ecc->strength);
			if (ret < 0) {
				mtd->ecc_stats.failed++;
			} else {
				mtd->ecc_stats.corrected += ret;
				max_bitflips =
					max_t(unsigned int, max_bitflips, ret);
			}
		}

		data_buf += data_size;
		oob_buf += ecc->bytes;
	}

	return max_bitflips;
}

/*
 * reads back status registers set by the controller to notify page read
 * errors. this is equivalent to what 'ecc->correct()' would do.
 */
static int parse_read_errors(struct qcom_nand_host *host, u8 *data_buf,
			     u8 *oob_buf, bool last_cw, int page)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	unsigned int max_bitflips = 0, uncorrectable_err_cws = 0;
	struct read_stats *buf;
	bool flash_op_err = false;
	u8 *data_buf_start = data_buf, *oob_buf_start = oob_buf;
	int i, cw_cnt, start_cw;

	buf = (struct read_stats *)nandc->reg_read_buf;
	if (last_cw) {
		start_cw = ecc->steps - 1;
		cw_cnt = 1;
	} else {
		start_cw = 0;
		cw_cnt = ecc->steps;
	}

	for (i = start_cw; i < start_cw + cw_cnt; i++, buf++) {
		u32 flash, buffer, erased_cw;
		int data_len, oob_len;

		if (i == (ecc->steps - 1)) {
			data_len = ecc->size - ((ecc->steps - 1) << 2);
			oob_len = ecc->steps << 2;
		} else {
			data_len = host->cw_data;
			oob_len = 0;
		}

		flash = le32_to_cpu(buf->flash);
		buffer = le32_to_cpu(buf->buffer);
		erased_cw = le32_to_cpu(buf->erased_cw);

		if ((flash & FS_OP_ERR) && (buffer & BS_UNCORRECTABLE_BIT)) {
			bool erased;

			/* ignore erased codeword errors */
			if (host->bch_enabled) {
				erased = (erased_cw & ERASED_CW) == ERASED_CW ?
					 true : false;
			} else if (data_buf) {
				erased = erased_chunk_check_and_fixup(data_buf,
								      data_len);
			} else {
				erased = false;
			}

			if (!erased)
				uncorrectable_err_cws |= BIT(i);
		} else if (flash & (FS_OP_ERR | FS_MPU_ERR)) {
			flash_op_err = true;
		} else {
			unsigned int stat;

			stat = buffer & BS_CORRECTABLE_ERR_MSK;
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max(max_bitflips, stat);
		}

		if (data_buf)
			data_buf += data_len;
		if (oob_buf)
			oob_buf += oob_len + ecc->bytes;
	}

	if (flash_op_err)
		return -EIO;

	if (!uncorrectable_err_cws)
		return max_bitflips;

	return check_for_erased_page(host, data_buf_start, oob_buf_start,
				     uncorrectable_err_cws, page,
				     max_bitflips, last_cw);
}

/*
 * helper to perform the actual page read operation, used by ecc->read_page(),
 * ecc->read_oob()
 */
static int read_page_ecc(struct mtd_info *mtd, struct qcom_nand_host *host,
		u8 *data_buf, u8 *oob_buf, int page)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *data_buf_start = data_buf, *oob_buf_start = oob_buf;
	int i, ret;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	u8 *tmp_status_buf = NULL;
	int status_buf_parse_size = 0x0;
	memset(nandc->status_buf, 0, nandc->status_buf_size);
	tmp_status_buf = nandc->status_buf;
	nandc->ps_mp_flag |= QPIC_PAGE_SCOPE;
	nandc->ps_mp_flag &= ~QPIC_MULTI_PAGE;
	/*Here once again upadate the register value , because
	 * for each read access upper layer will call the bad block
	 * check and for bad block check we are using legacy page read
	 * instead of page scope and multi page read.
	 */
	update_rw_regs(host, ecc->steps, true);
#endif

	if (nandc->dma_bam_enabled)
		config_bam_page_read(nandc);

	/* queue cmd descs for each codeword */
	for (i = 0; i < ecc->steps; i++) {
		int data_size, oob_size;

		if ((i == (ecc->steps - 1)) && !nandc->boot_layout) {
			data_size = ecc->size - ((ecc->steps - 1) << 2);
			oob_size = (ecc->steps << 2) + host->ecc_bytes_hw +
				   host->spare_bytes;
		} else {
			data_size = host->cw_data;
			oob_size = host->ecc_bytes_hw + host->spare_bytes;
		}

		if (nandc->dma_bam_enabled) {
			if (data_buf && oob_buf) {
				nandc_set_reg(nandc, NAND_READ_LOCATION_0,
					(0 << READ_LOCATION_OFFSET) |
					(data_size << READ_LOCATION_SIZE) |
					(0 << READ_LOCATION_LAST));
				nandc_set_reg(nandc, NAND_READ_LOCATION_1,
					(data_size << READ_LOCATION_OFFSET) |
					(oob_size << READ_LOCATION_SIZE) |
					(1 << READ_LOCATION_LAST));
			} else if (data_buf) {
				if (host->hw_version >= QPIC_VERSION_V2_0) {
					if (i == (ecc->steps - 1)) {
						nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_0,
							(0 << READ_LOCATION_OFFSET) |
							(data_size << READ_LOCATION_SIZE) |
							(1 << READ_LOCATION_LAST));
					} else {
						nandc_set_reg(nandc, NAND_READ_LOCATION_0,
						(0 << READ_LOCATION_OFFSET) |
						(data_size << READ_LOCATION_SIZE) |
						(1 << READ_LOCATION_LAST));
					}
				} else {
					nandc_set_reg(nandc, NAND_READ_LOCATION_0,
						(0 << READ_LOCATION_OFFSET) |
						(data_size << READ_LOCATION_SIZE) |
						(1 << READ_LOCATION_LAST));
				}
			} else {
				if (host->hw_version >= QPIC_VERSION_V2_0) {
					if (i == (ecc->steps - 1)) {
						nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_0,
							(data_size << READ_LOCATION_OFFSET) |
							(oob_size << READ_LOCATION_SIZE) |
							(1 << READ_LOCATION_LAST));
					} else {
						nandc_set_reg(nandc, NAND_READ_LOCATION_0,
							(data_size << READ_LOCATION_OFFSET) |
							(oob_size << READ_LOCATION_SIZE) |
							(1 << READ_LOCATION_LAST));
					}
				} else {
					nandc_set_reg(nandc, NAND_READ_LOCATION_0,
						(data_size << READ_LOCATION_OFFSET) |
						(oob_size << READ_LOCATION_SIZE) |
						(1 << READ_LOCATION_LAST));
				}
			}
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
			if (i == (ecc->steps - 1))
				nandc->ps_mp_flag |= PAGE_SCOPE_MULTI_PAGE_CMD_EXE;
#endif
			config_bam_cw_read(nandc, true);
		} else {
			config_cw_read(host, nandc, true);
		}

		if (data_buf)
			read_data_dma(nandc, FLASH_BUF_ACC, data_buf,
				      data_size, 0);

#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
		if (tmp_status_buf) {
			prep_dma_desc_status_bam(nandc, (void *)tmp_status_buf,
					MAX_STATUS_REG, 0);
			tmp_status_buf += MAX_STATUS_REG;
		}
#endif
		/*
		 * when ecc is enabled, the controller doesn't read the real
		 * or dummy bad block markers in each chunk. To maintain a
		 * consistent layout across RAW and ECC reads, we just
		 * leave the real/dummy BBM offsets empty (i.e, filled with
		 * 0xffs)
		 */
		if (oob_buf) {
			int j;

			for (j = 0; j < host->bbm_size; j++)
				*oob_buf++ = 0xff;

			read_data_dma(nandc, FLASH_BUF_ACC + data_size,
				      oob_buf, oob_size, 0);
		}

		if (data_buf)
			data_buf += data_size;
		if (oob_buf)
			oob_buf += oob_size;
	}

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure to read page/oob\n");

	free_descs(nandc);

#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	GET_STATUS_BUFF_PARSE_SIZE_PER_PAGE(mtd->writesize,
			status_buf_parse_size);
	parse_status_buf(nandc, nandc->status_buf,
			status_buf_parse_size);
#endif
	if (!ret)
		ret = parse_read_errors(host, data_buf_start, oob_buf_start,
					false, page);

	return ret;
}

/*
 * a helper that copies the last step/codeword of a page (containing free oob)
 * into our local buffer
 */
static int copy_last_cw(struct qcom_nand_host *host, int page)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int size;
	int ret;
	u32 last_cw_reg;

	clear_read_regs(nandc);

	size = host->use_ecc ? host->cw_data : host->cw_size;

	/* prepare a clean read buffer */
	memset(nandc->data_buffer, 0xff, size);

	set_address(host, host->cw_size * (ecc->steps - 1), page);
	/* If page scope and multi page config is enable then
	 * to read only last code wrod no need to set page
	 * scope and multi page bit of command register.
	 * just fall back to legacy page read mode.
	 */
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	nandc->ps_mp_flag &= ~QPIC_PAGE_SCOPE;
	nandc->ps_mp_flag &= ~QPIC_MULTI_PAGE;
#endif
	update_rw_regs(host, 1, true);

	if (host->hw_version >= QPIC_VERSION_V2_0)
		last_cw_reg = NAND_READ_LOCATION_LAST_CW_0;
	else
		last_cw_reg = NAND_READ_LOCATION_0;

	nandc_set_reg(nandc, last_cw_reg,
			(0 << READ_LOCATION_OFFSET) |
			(size << READ_LOCATION_SIZE) |
			(1 << READ_LOCATION_LAST));

	config_cw_read(host, nandc, host->use_ecc);

	read_data_dma(nandc, FLASH_BUF_ACC, nandc->data_buffer, size, 0);

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failed to copy last codeword\n");

	free_descs(nandc);

	if (!ret) {
		if (host->use_ecc)
			ret = parse_read_errors(host, nandc->data_buffer,
						nandc->data_buffer + size,
						true, page);
		else
			ret = check_flash_errors(host, 1);
	}

	return ret;
}
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)

static int qcom_nandc_read_multi_page(struct mtd_info *mtd, struct nand_chip *chip,
			uint8_t *buf, int oob_required, int page, int num_pages)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int i, ret, j;

	u8 *data_buf, *oob_buf = NULL;
	u8 *tmp_status_buf = NULL;
	int status_buf_parse_size =0x0;
	u8 *data_buf_start = NULL;
	u8 *oob_buf_start = NULL;
	data_buf = buf;
	oob_buf = oob_required ? chip->oob_poi : NULL;
	data_buf_start = data_buf;
	oob_buf_start = oob_buf;

	clear_bam_transaction(nandc);

	memset(nandc->status_buf, 0, nandc->status_buf_size);
	tmp_status_buf = nandc->status_buf;
	nandc->ps_mp_flag |= QPIC_MULTI_PAGE;
	nandc->ps_mp_flag &= ~QPIC_PAGE_SCOPE;
	/*Here once again upadate the register value , because
	 * for each read access upper layer will call the bad block
	 * check and for bad block check we are using legacy page read
	 * instead of page scope and multi page read.
	 */
	update_rw_regs(host, ecc->steps, true);
	/* set number of pages for this transaction*/
	nandc_set_reg(nandc, NAND_MULTI_PAGE_CMD, (num_pages - 1));

	if (nandc->dma_bam_enabled)
		config_bam_page_read(nandc);

	for (j = 0; j < num_pages; j++) {
		/* queue cmd descs for each codeword */
		for (i = 0; i < ecc->steps; i++) {
			int data_size, oob_size;

			if ((i == (ecc->steps - 1)) && !nandc->boot_layout) {
				data_size = ecc->size - ((ecc->steps - 1) << 2);
				oob_size = (ecc->steps << 2) + host->ecc_bytes_hw +
					host->spare_bytes;
			} else {
				data_size = host->cw_data;
				oob_size = host->ecc_bytes_hw + host->spare_bytes;
			}

			if (data_buf && oob_buf) {
				nandc_set_reg(nandc, NAND_READ_LOCATION_0,
					(0 << READ_LOCATION_OFFSET) |
					(data_size << READ_LOCATION_SIZE) |
					(0 << READ_LOCATION_LAST));
				nandc_set_reg(nandc, NAND_READ_LOCATION_1,
					(data_size << READ_LOCATION_OFFSET) |
					(oob_size << READ_LOCATION_SIZE) |
					(1 << READ_LOCATION_LAST));
			} else if (data_buf) {
				if (i == (ecc->steps - 1)) {
					nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_0,
						(0 << READ_LOCATION_OFFSET) |
						(data_size << READ_LOCATION_SIZE) |
						(1 << READ_LOCATION_LAST));
				} else {
					nandc_set_reg(nandc, NAND_READ_LOCATION_0,
						(0 << READ_LOCATION_OFFSET) |
						(data_size << READ_LOCATION_SIZE) |
						(1 << READ_LOCATION_LAST));
				}
			} else {
				if (i == (ecc->steps - 1)) {
					nandc_set_reg(nandc, NAND_READ_LOCATION_LAST_CW_0,
						(data_size << READ_LOCATION_OFFSET) |
						(oob_size << READ_LOCATION_SIZE) |
						(1 << READ_LOCATION_LAST));
				} else {
					nandc_set_reg(nandc, NAND_READ_LOCATION_0,
						(data_size << READ_LOCATION_OFFSET) |
						(oob_size << READ_LOCATION_SIZE) |
						(1 << READ_LOCATION_LAST));
				}
			}

			if (i == (ecc->steps - 1) && (j == num_pages - 1)) {
				nandc->ps_mp_flag |= PAGE_SCOPE_MULTI_PAGE_CMD_EXE;
				config_bam_cw_read(nandc, true);
			} else if (i == 0 && j == 0) {
				config_bam_cw_read(nandc, true);
			}

			if (data_buf)
				read_data_dma(nandc, FLASH_BUF_ACC, data_buf,
				      data_size, 0);
			if (tmp_status_buf) {
				prep_dma_desc_status_bam(nandc, (void *)tmp_status_buf,
						MAX_STATUS_REG, 0);

				tmp_status_buf += MAX_STATUS_REG;
			}
			/*
			 * when ecc is enabled, the controller doesn't read the real
			 * or dummy bad block markers in each chunk. To maintain a
			 * consistent layout across RAW and ECC reads, we just
			 * leave the real/dummy BBM offsets empty (i.e, filled with
			 * 0xffs)
			 */
			if (oob_buf) {
				int j;

				for (j = 0; j < host->bbm_size; j++)
					*oob_buf++ = 0xff;

				read_data_dma(nandc, FLASH_BUF_ACC + data_size,
				      oob_buf, oob_size, 0);
			}

			if (data_buf)
				data_buf += data_size;
			if (oob_buf)
				oob_buf += oob_size;
		}
	}

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure to read page/oob\n");

	free_descs(nandc);

	GET_STATUS_BUFF_PARSE_SIZE_PER_PAGE(mtd->writesize,
			status_buf_parse_size);

	/* Check the error for each code word */
	for (j = 0; j < num_pages; j++) {
		parse_status_buf(nandc, nandc->status_buf + j * status_buf_parse_size,
				status_buf_parse_size);

		if (!ret)
			ret = parse_read_errors(host, data_buf_start + j * mtd->writesize, oob_buf_start,
					false, page + j);
	}

	return ret;
}
#endif
/* implements ecc->read_page() */
static int qcom_nandc_read_page(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int oob_required, int page)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u8 *data_buf, *oob_buf = NULL;

	data_buf = buf;
	oob_buf = oob_required ? chip->oob_poi : NULL;

	clear_bam_transaction(nandc);

	return read_page_ecc(mtd, host, data_buf, oob_buf, page);
}

/* implements ecc->read_page_raw() */
static int qcom_nandc_read_page_raw(struct mtd_info *mtd,
				    struct nand_chip *chip, uint8_t *buf,
				    int oob_required, int page)
{
	return nandc_read_page_raw(mtd, chip, buf, chip->oob_poi, page,
				   BIT(chip->ecc.steps) - 1);
}

/* implements ecc->read_oob() */
static int qcom_nandc_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			       int page)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	host->use_ecc = true;
	set_address(host, 0, page);
	update_rw_regs(host, ecc->steps, true);

	return read_page_ecc(mtd, host, NULL, chip->oob_poi, page);
}

/* implements ecc->write_page() */
static int qcom_nandc_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				 const uint8_t *buf, int oob_required, int page)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *data_buf, *oob_buf;
	int i, ret;

	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	data_buf = (u8 *)buf;
	oob_buf = chip->oob_poi;

	host->use_ecc = true;
	update_rw_regs(host, ecc->steps, false);

	for (i = 0; i < ecc->steps; i++) {
		int data_size, oob_size;

		if ((i == (ecc->steps - 1)) && !nandc->boot_layout) {
			data_size = ecc->size - ((ecc->steps - 1) << 2);
			oob_size = (ecc->steps << 2) + host->ecc_bytes_hw +
				   host->spare_bytes;
		} else {
			data_size = host->cw_data;
			oob_size = ecc->bytes;
		}

		config_cw_write_pre(nandc);

		write_data_dma(nandc, FLASH_BUF_ACC, data_buf, data_size,
			i == (ecc->steps - 1) ? DMA_DESC_FLAG_NO_EOT : 0);

		/*
		 * when ECC is enabled, we don't really need to write anything
		 * to oob for the first n - 1 codewords since these oob regions
		 * just contain ECC bytes that's written by the controller
		 * itself. For the last codeword, we skip the bbm positions and
		 * write to the free oob area.
		 */
		if (i == (ecc->steps - 1)) {
			oob_buf += host->bbm_size;

			write_data_dma(nandc, FLASH_BUF_ACC + data_size,
				       oob_buf, oob_size, 0);
		}

		config_cw_write_post(nandc);

		data_buf += data_size;
		oob_buf += oob_size;
	}

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure to write page\n");

	free_descs(nandc);
	return ret;
}

/* implements ecc->write_page_raw() */
static int qcom_nandc_write_page_raw(struct mtd_info *mtd,
				     struct nand_chip *chip, const uint8_t *buf,
				     int oob_required, int page)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *data_buf, *oob_buf;
	int i, ret;

	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	data_buf = (u8 *)buf;
	oob_buf = chip->oob_poi;

	host->use_ecc = false;
	update_rw_regs(host, ecc->steps, false);

	for (i = 0; i < ecc->steps; i++) {
		int data_size1, data_size2, oob_size1, oob_size2;
		int reg_off = FLASH_BUF_ACC;

		data_size1 = mtd->writesize - host->cw_size * (ecc->steps - 1);
		oob_size1 = host->bbm_size;

		if ((i == (ecc->steps - 1)) && !nandc->boot_layout) {
			data_size2 = ecc->size - data_size1 -
				     ((ecc->steps - 1) << 2);
			oob_size2 = (ecc->steps << 2) + host->ecc_bytes_hw +
				    host->spare_bytes;
		} else {
			data_size2 = host->cw_data - data_size1;
			oob_size2 = host->ecc_bytes_hw + host->spare_bytes;
		}

		config_cw_write_pre(nandc);

		write_data_dma(nandc, reg_off, data_buf, data_size1,
					DMA_DESC_FLAG_NO_EOT);
		reg_off += data_size1;
		data_buf += data_size1;

		write_data_dma(nandc, reg_off, oob_buf, oob_size1,
					DMA_DESC_FLAG_NO_EOT);
		reg_off += oob_size1;
		oob_buf += oob_size1;

		write_data_dma(nandc, reg_off, data_buf, data_size2,
					DMA_DESC_FLAG_NO_EOT);
		reg_off += data_size2;
		data_buf += data_size2;

		write_data_dma(nandc, reg_off, oob_buf, oob_size2, 0);
		oob_buf += oob_size2;

		config_cw_write_post(nandc);
	}

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure to write raw page\n");

	free_descs(nandc);

	return ret;
}

/*
 * implements ecc->write_oob()
 *
 * the NAND controller cannot write only data or only oob within a codeword,
 * since ecc is calculated for the combined codeword. we first copy the
 * entire contents for the last codeword(data + oob), replace the old oob
 * with the new one in chip->oob_poi, and then write the entire codeword.
 * this read-copy-write operation results in a slight performance loss.
 */
static int qcom_nandc_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
				int page)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *oob = chip->oob_poi;
	int free_boff;
	int data_size, oob_size;
	int ret, status = 0;

	host->use_ecc = true;

	clear_bam_transaction(nandc);
	ret = copy_last_cw(host, page);
	if (ret)
		return ret;

	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	/* calculate the data and oob size for the last codeword/step */
	data_size = ecc->size - ((ecc->steps - 1) << 2);
	oob_size = ecc->steps << 2;

	free_boff = ecc->layout->oobfree[0].offset;

	/* override new oob content to last codeword */
	memcpy(nandc->data_buffer + data_size, oob + free_boff, oob_size);

	set_address(host, host->cw_size * (ecc->steps - 1), page);
	update_rw_regs(host, 1, false);

	config_cw_write_pre(nandc);
	write_data_dma(nandc, FLASH_BUF_ACC, nandc->data_buffer,
		       data_size + oob_size, 0);
	config_cw_write_post(nandc);

	ret = submit_descs(nandc);

	free_descs(nandc);

	if (ret) {
		dev_err(nandc->dev, "failure to write oob\n");
		return -EIO;
	}

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

static int qcom_nandc_block_bad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int page, ret, bbpos, bad = 0;
	u32 flash_status;

	page = (int)(ofs >> chip->page_shift) & chip->pagemask;
	/*
	 * configure registers for a raw sub page read, the address is set to
	 * the beginning of the last codeword, we don't care about reading ecc
	 * portion of oob. we just want the first few bytes from this codeword
	 * that contains the BBM
	 */
	host->use_ecc = false;

	clear_bam_transaction(nandc);
	ret = copy_last_cw(host, page);
	if (ret)
		goto err;

	flash_status = le32_to_cpu(nandc->reg_read_buf[0]);

	if (flash_status & (FS_OP_ERR | FS_MPU_ERR)) {
		dev_warn(nandc->dev, "error when trying to read BBM\n");
		goto err;
	}

	bbpos = mtd->writesize - host->cw_size * (ecc->steps - 1);
	bad = nandc->data_buffer[bbpos] != 0xff;

	if (chip->options & NAND_BUSWIDTH_16)
		bad = bad || (nandc->data_buffer[bbpos + 1] != 0xff);
err:
	return bad;
}

static int qcom_nandc_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int page, ret, status = 0;

	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	/*
	 * to mark the BBM as bad, we flash the entire last codeword with 0s.
	 * we don't care about the rest of the content in the codeword since
	 * we aren't going to use this block again
	 */
	memset(nandc->data_buffer, 0x00, host->cw_size);

	page = (int)(ofs >> chip->page_shift) & chip->pagemask;

	/* prepare write */
	host->use_ecc = false;
	set_address(host, host->cw_size * (ecc->steps - 1), page);
	update_rw_regs(host, 1, false);

	config_cw_write_pre(nandc);
	write_data_dma(nandc, FLASH_BUF_ACC, nandc->data_buffer,
				host->cw_size, 0);
	config_cw_write_post(nandc);

	ret = submit_descs(nandc);

	free_descs(nandc);

	if (ret) {
		dev_err(nandc->dev, "failure to update BBM\n");
		return -EIO;
	}

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/*
 * the three functions below implement chip->read_byte(), chip->read_buf()
 * and chip->write_buf() respectively. these aren't used for
 * reading/writing page data, they are used for smaller data like reading
 * id, status etc
 */
static uint8_t qcom_nandc_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u8 *buf = nandc->data_buffer;
	u8 ret = 0x0;

	if (host->last_command == NAND_CMD_STATUS) {
		ret = host->status;

		host->status = NAND_STATUS_READY | NAND_STATUS_WP;

		return ret;
	}

	if (nandc->buf_start < nandc->buf_count)
		ret = buf[nandc->buf_start++];

	return ret;
}

static void qcom_nandc_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int real_len = min_t(size_t, len, nandc->buf_count - nandc->buf_start);

	memcpy(buf, nandc->data_buffer + nandc->buf_start, real_len);
	nandc->buf_start += real_len;
}

static void qcom_nandc_write_buf(struct mtd_info *mtd, const uint8_t *buf,
				 int len)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int real_len = min_t(size_t, len, nandc->buf_count - nandc->buf_start);

	memcpy(nandc->data_buffer + nandc->buf_start, buf, real_len);

	nandc->buf_start += real_len;
}

/* we support only one external chip for now */
static void qcom_nandc_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	if (chipnr <= 0)
		return;

	dev_warn(nandc->dev, "invalid chip select\n");
}

/*
 * NAND controller page layout info
 *
 * Layout with ECC enabled:
 *
 * |----------------------|  |---------------------------------|
 * |           xx.......yy|  |             *********xx.......yy|
 * |    DATA   xx..ECC..yy|  |    DATA     **SPARE**xx..ECC..yy|
 * |   (516)   xx.......yy|  |  (516-n*4)  **(n*4)**xx.......yy|
 * |           xx.......yy|  |             *********xx.......yy|
 * |----------------------|  |---------------------------------|
 *     codeword 1,2..n-1                  codeword n
 *  <---(528/532 Bytes)-->    <-------(528/532 Bytes)--------->
 *
 * n = Number of codewords in the page
 * . = ECC bytes
 * * = Spare/free bytes
 * x = Unused byte(s)
 * y = Reserved byte(s)
 *
 * 2K page: n = 4, spare = 16 bytes
 * 4K page: n = 8, spare = 32 bytes
 * 8K page: n = 16, spare = 64 bytes
 *
 * the qcom nand controller operates at a sub page/codeword level. each
 * codeword is 528 and 532 bytes for 4 bit and 8 bit ECC modes respectively.
 * the number of ECC bytes vary based on the ECC strength and the bus width.
 *
 * the first n - 1 codewords contains 516 bytes of user data, the remaining
 * 12/16 bytes consist of ECC and reserved data. The nth codeword contains
 * both user data and spare(oobavail) bytes that sum up to 516 bytes.
 *
 * When we access a page with ECC enabled, the reserved bytes(s) are not
 * accessible at all. When reading, we fill up these unreadable positions
 * with 0xffs. When writing, the controller skips writing the inaccessible
 * bytes.
 *
 * Layout with ECC disabled:
 *
 * |------------------------------|  |---------------------------------------|
 * |         yy          xx.......|  |         bb          *********xx.......|
 * |  DATA1  yy  DATA2   xx..ECC..|  |  DATA1  bb  DATA2   **SPARE**xx..ECC..|
 * | (size1) yy (size2)  xx.......|  | (size1) bb (size2)  **(n*4)**xx.......|
 * |         yy          xx.......|  |         bb          *********xx.......|
 * |------------------------------|  |---------------------------------------|
 *         codeword 1,2..n-1                        codeword n
 *  <-------(528/532 Bytes)------>    <-----------(528/532 Bytes)----------->
 *
 * n = Number of codewords in the page
 * . = ECC bytes
 * * = Spare/free bytes
 * x = Unused byte(s)
 * y = Dummy Bad Bock byte(s)
 * b = Real Bad Block byte(s)
 * size1/size2 = function of codeword size and 'n'
 *
 * when the ECC block is disabled, one reserved byte (or two for 16 bit bus
 * width) is now accessible. For the first n - 1 codewords, these are dummy Bad
 * Block Markers. In the last codeword, this position contains the real BBM
 *
 * In order to have a consistent layout between RAW and ECC modes, we assume
 * the following OOB layout arrangement:
 *
 * |-----------|  |--------------------|
 * |yyxx.......|  |bb*********xx.......|
 * |yyxx..ECC..|  |bb*FREEOOB*xx..ECC..|
 * |yyxx.......|  |bb*********xx.......|
 * |yyxx.......|  |bb*********xx.......|
 * |-----------|  |--------------------|
 *  first n - 1       nth OOB region
 *  OOB regions
 *
 * n = Number of codewords in the page
 * . = ECC bytes
 * * = FREE OOB bytes
 * y = Dummy bad block byte(s) (inaccessible when ECC enabled)
 * x = Unused byte(s)
 * b = Real bad block byte(s) (inaccessible when ECC enabled)
 *
 * This layout is read as is when ECC is disabled. When ECC is enabled, the
 * inaccessible Bad Block byte(s) are ignored when we write to a page/oob,
 * and assumed as 0xffs when we read a page/oob. The ECC, unused and
 * dummy/real bad block bytes are grouped as ecc bytes in nand_ecclayout (i.e,
 * ecc->bytes is the sum of the three).
 */

static struct nand_ecclayout *
qcom_nand_create_layout(struct qcom_nand_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	struct nand_ecclayout *layout;
	int i, j, steps, pos = 0, shift = 0;

	layout = devm_kzalloc(nandc->dev, sizeof(*layout), GFP_KERNEL);
	if (!layout)
		return NULL;

	steps = mtd->writesize / ecc->size;
	layout->eccbytes = steps * ecc->bytes;

	layout->oobfree[0].offset = (steps - 1) * ecc->bytes + host->bbm_size;
	layout->oobfree[0].length = steps << 2;

	/*
	 * the oob bytes in the first n - 1 codewords are all grouped together
	 * in the format:
	 * DUMMY_BBM + UNUSED + ECC
	 */
	for (i = 0; i < steps - 1; i++) {
		for (j = 0; j < ecc->bytes; j++)
			layout->eccpos[pos++] = i * ecc->bytes + j;
	}

	/*
	 * the oob bytes in the last codeword are grouped in the format:
	 * BBM + FREE OOB + UNUSED + ECC
	 */

	/* fill up the bbm positions */
	for (j = 0; j < host->bbm_size; j++)
		layout->eccpos[pos++] = i * ecc->bytes + j;

	/*
	 * fill up the ecc and reserved positions, their indices are offseted
	 * by the free oob region
	 */
	shift = layout->oobfree[0].length + host->bbm_size;

	for (j = 0; j < (host->ecc_bytes_hw + host->spare_bytes); j++)
		layout->eccpos[pos++] = i * ecc->bytes + shift + j;

	return layout;
}

static int qcom_nand_host_setup(struct qcom_nand_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int cwperpage, bad_block_byte;
	bool wide_bus;
	int ecc_mode = 1;
	int no_of_addr_cycle = 0x5;
	int disable_status_after_write = 0x0;
	int wr_rd_busy_gap = 0x2;
	int nand_recovery_cycle = 0x7;

	/*
	 * the controller requires each step consists of 512 bytes of data.
	 * bail out if DT has populated a wrong step size.
	 */
	if (ecc->size != NANDC_STEP_SIZE) {
		dev_err(nandc->dev, "invalid ecc size\n");
		return -EINVAL;
	}

	/*
	 * Read the required ecc strength from NAND device and overwrite the
	 * device tree ecc strength
	 */
	if (chip->ecc_strength_ds >= 8)
		ecc->strength = 8;

	wide_bus = chip->options & NAND_BUSWIDTH_16 ? true : false;

	if (ecc->strength >= 8) {
		/* 8 bit ECC defaults to BCH ECC on all platforms */
		host->bch_enabled = true;
		ecc_mode = 1;

		if (wide_bus) {
			host->ecc_bytes_hw = 14;
			host->spare_bytes = 0;
			host->bbm_size = 2;
		} else {
			host->ecc_bytes_hw = 13;
			host->spare_bytes = 2;
			host->bbm_size = 1;
		}
	} else {
		/*
		 * if the controller supports BCH for 4 bit ECC, the controller
		 * uses lesser bytes for ECC. If RS is used, the ECC bytes is
		 * always 10 bytes
		 */
		if (nandc->ecc_modes & ECC_BCH_4BIT) {
			/* BCH */
			host->bch_enabled = true;
			ecc_mode = 0;

			if (wide_bus) {
				host->ecc_bytes_hw = 8;
				host->spare_bytes = 2;
				host->bbm_size = 2;
			} else {
				host->ecc_bytes_hw = 7;
				host->spare_bytes = 4;
				host->bbm_size = 1;
			}
		} else {
			/* RS */
			host->ecc_bytes_hw = 10;

			if (wide_bus) {
				host->spare_bytes = 0;
				host->bbm_size = 2;
			} else {
				host->spare_bytes = 1;
				host->bbm_size = 1;
			}
		}
	}

	/*
	 * we consider ecc->bytes as the sum of all the non-data content in a
	 * step. It gives us a clean representation of the oob area (even if
	 * all the bytes aren't used for ECC).It is always 16 bytes for 8 bit
	 * ECC and 12 bytes for 4 bit ECC
	 */
	ecc->bytes = host->ecc_bytes_hw + host->spare_bytes + host->bbm_size;

	ecc->read_page		= qcom_nandc_read_page;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	ecc->read_multi_page	= qcom_nandc_read_multi_page;
#endif
	ecc->read_page_raw	= qcom_nandc_read_page_raw;
	ecc->read_oob		= qcom_nandc_read_oob;
	ecc->write_page		= qcom_nandc_write_page;
	ecc->write_page_raw	= qcom_nandc_write_page_raw;
	ecc->write_oob		= qcom_nandc_write_oob;

	ecc->mode = NAND_ECC_HW;

	ecc->layout = qcom_nand_create_layout(host);
	if (!ecc->layout)
		return -ENOMEM;

	cwperpage = mtd->writesize / ecc->size;

	/*
	 * DATA_UD_BYTES varies based on whether the read/write command protects
	 * spare data with ECC too. We protect spare data by default, so we set
	 * it to main + spare data, which are 512 and 4 bytes respectively.
	 */
	host->cw_data = 516;

	/*
	 * total bytes in a step, either 528 bytes for 4 bit ECC, or 532 bytes
	 * for 8 bit ECC
	 */
	host->cw_size = host->cw_data + ecc->bytes;

	if (ecc->bytes * (mtd->writesize / ecc->size) > mtd->oobsize) {
		dev_err(nandc->dev, "ecc data doesn't fit in OOB area\n");
		return -EINVAL;
	}

	bad_block_byte = mtd->writesize - host->cw_size * (cwperpage - 1) + 1;

#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	/* As per HPG below value should be valid for Serial NAND
	 * configuration
	 */
	no_of_addr_cycle = 3;
	disable_status_after_write = 1;
	wr_rd_busy_gap = 20;
	nand_recovery_cycle = 0x0;
#endif
	host->cfg0 = (cwperpage - 1) << CW_PER_PAGE
				| host->cw_data << UD_SIZE_BYTES
				| disable_status_after_write << DISABLE_STATUS_AFTER_WRITE
				| no_of_addr_cycle << NUM_ADDR_CYCLES
				| host->ecc_bytes_hw << ECC_PARITY_SIZE_BYTES_RS
				| 0 << STATUS_BFR_READ
				| 1 << SET_RD_MODE_AFTER_STATUS
				| host->spare_bytes << SPARE_SIZE_BYTES;

	host->cfg1 = nand_recovery_cycle << NAND_RECOVERY_CYCLES
				| 0 <<  CS_ACTIVE_BSY
				| bad_block_byte << BAD_BLOCK_BYTE_NUM
				| 0 << BAD_BLOCK_IN_SPARE_AREA
				| wr_rd_busy_gap << WR_RD_BSY_GAP
				| wide_bus << WIDE_FLASH
				| host->bch_enabled << ENABLE_BCH_ECC;

	host->cfg0_raw = (cwperpage - 1) << CW_PER_PAGE
				| host->cw_size << UD_SIZE_BYTES
				| no_of_addr_cycle << NUM_ADDR_CYCLES
				| 0 << SPARE_SIZE_BYTES;

	host->cfg1_raw = nand_recovery_cycle << NAND_RECOVERY_CYCLES
				| 0 << CS_ACTIVE_BSY
				| 17 << BAD_BLOCK_BYTE_NUM
				| 1 << BAD_BLOCK_IN_SPARE_AREA
				| wr_rd_busy_gap << WR_RD_BSY_GAP
				| wide_bus << WIDE_FLASH
				| 1 << DEV0_CFG1_ECC_DISABLE;

	host->ecc_bch_cfg = !host->bch_enabled << ECC_CFG_ECC_DISABLE
				| 0 << ECC_SW_RESET
				| host->cw_data << ECC_NUM_DATA_BYTES
				| 1 << ECC_FORCE_CLK_OPEN
				| ecc_mode << ECC_MODE
				| host->ecc_bytes_hw << ECC_PARITY_SIZE_BYTES_BCH;

	host->ecc_buf_cfg = 0x203 << NUM_STEPS;

	host->clrflashstatus = FS_READY_BSY_N;
	host->clrreadstatus = 0xc0;
	nandc->regs->erased_cw_detect_cfg_clr = CLR_ERASED_PAGE_DET;
	nandc->regs->erased_cw_detect_cfg_set = SET_ERASED_PAGE_DET;

	dev_dbg(nandc->dev,
		"cfg0 %x cfg1 %x ecc_buf_cfg %x ecc_bch cfg %x cw_size %d cw_data %d strength %d parity_bytes %d steps %d\n",
		host->cfg0, host->cfg1, host->ecc_buf_cfg, host->ecc_bch_cfg,
		host->cw_size, host->cw_data, ecc->strength, ecc->bytes,
		cwperpage);

	return 0;
}

static int qcom_nandc_alloc(struct qcom_nand_controller *nandc)
{
	int ret;

	ret = dma_set_coherent_mask(nandc->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(nandc->dev, "failed to set DMA mask\n");
		return ret;
	}

	/*
	 * we use the internal buffer for reading ONFI params, reading small
	 * data like ID and status, and preforming read-copy-write operations
	 * when writing to a codeword partially. 532 is the maximum possible
	 * size of a codeword for our nand controller
	 *
	 * NOTE: Change this buffer size for serial NAND if 16-bit ECC will
	 * be supported, nandc->buf_size = 544
	 */
	nandc->buf_size = 532;

	nandc->data_buffer = devm_kzalloc(nandc->dev, nandc->buf_size,
					GFP_KERNEL);
	if (!nandc->data_buffer)
		return -ENOMEM;

	nandc->regs = devm_kzalloc(nandc->dev, sizeof(*nandc->regs),
					GFP_KERNEL);
	if (!nandc->regs)
		return -ENOMEM;


	if (!nandc->dma_bam_enabled) {
		nandc->reg_read_buf = devm_kzalloc(nandc->dev,
					MAX_REG_RD *
					sizeof(*nandc->reg_read_buf),
					GFP_KERNEL);

		if (!nandc->reg_read_buf)
			return -ENOMEM;

		nandc->chan = dma_request_slave_channel(nandc->dev, "rxtx");
		if (!nandc->chan) {
			dev_err(nandc->dev, "failed to request slave channel\n");
			return -ENODEV;
		}
	} else {
		nandc->reg_read_buf = dmam_alloc_coherent(nandc->dev,
					MAX_REG_RD *
					sizeof(*nandc->reg_read_buf),
					&nandc->reg_read_buf_phys, GFP_KERNEL);

		if (!nandc->reg_read_buf)
			return -ENOMEM;

		nandc->tx_chan = dma_request_slave_channel(nandc->dev, "tx");
		if (!nandc->tx_chan) {
			dev_err(nandc->dev, "failed to request tx channel\n");
			return -ENODEV;
		}

		nandc->rx_chan = dma_request_slave_channel(nandc->dev, "rx");
		if (!nandc->rx_chan) {
			dev_err(nandc->dev, "failed to request rx channel\n");
			return -ENODEV;
		}

		nandc->cmd_chan = dma_request_slave_channel(nandc->dev, "cmd");
		if (!nandc->cmd_chan) {
			dev_err(nandc->dev, "failed to request cmd channel\n");
			return -ENODEV;
		}
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
		nandc->status_chan = dma_request_slave_channel(nandc->dev, "status");
		if (!nandc->status_chan) {
			dev_err(nandc->dev, "failed to request status channel\n");
			return -ENODEV;
		}
#endif
		nandc->bam_txn = alloc_bam_transaction(nandc);
		if (!nandc->bam_txn) {
			dev_err(nandc->dev, "failed to allocate bam transaction\n");
			return -ENOMEM;
		}
	}

	INIT_LIST_HEAD(&nandc->desc_list);
	INIT_LIST_HEAD(&nandc->host_list);

	spin_lock_init(&nandc->controller.lock);
	init_waitqueue_head(&nandc->controller.wq);

	return 0;
}

static void qcom_nandc_unalloc(struct qcom_nand_controller *nandc)
{
	if (nandc->dma_bam_enabled) {
		if (nandc->tx_chan)
			dma_release_channel(nandc->tx_chan);

		if (nandc->rx_chan)
			dma_release_channel(nandc->rx_chan);

		if (nandc->cmd_chan)
			dma_release_channel(nandc->cmd_chan);

		if (nandc->reg_read_buf)
			dmam_free_coherent(nandc->dev, MAX_REG_RD *
				sizeof(*nandc->reg_read_buf),
				nandc->reg_read_buf,
				nandc->reg_read_buf_phys);
	} else {
		if (nandc->chan)
			dma_release_channel(nandc->chan);

	}
}

/* one time setup of a few nand controller registers */
static int qcom_nandc_setup(struct qcom_nand_controller *nandc)
{
	u32 nand_ctrl;
	nandc_write(nandc, NAND_DEV_CMD_VLD, NAND_DEV_CMD_VLD_VAL);

	/* enable ADM or BAM DMA */
	if (!nandc->dma_bam_enabled) {
		nandc_write(nandc, NAND_FLASH_CHIP_SELECT, DM_EN);
	} else {
		nand_ctrl = nandc_read(nandc, NAND_CTRL);
		/* For serila NAND as per HPG the value of this register should
		 * be 0x341
		 */
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
		nand_ctrl |= NANDC_READ_DELAY_COUNTER_VAL;
#endif
		/*
		 * Once BAM_MODE_EN bit is set then QPIC_NAND_CTRL register
		 * should be written with BAM instead of writel.
		 * Check if BAM_MODE_EN is already set by bootloader and write
		 * only if this bit is not set.
		 */
		if (!(nand_ctrl & BAM_MODE_EN))
			nandc_write(nandc, NAND_CTRL, nand_ctrl | BAM_MODE_EN);
	}

	/* save the original values of these registers */
	nandc->cmd1 = nandc_read(nandc, NAND_DEV_CMD1);
	nandc->vld = nandc_read(nandc, NAND_DEV_CMD_VLD);

	return 0;
}

#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
static void qspi_write_reg_bam(struct qcom_nand_controller *nandc,
		unsigned int val, unsigned int reg)
{
	int ret;

	/* Clear the BAM transaction index */
	clear_bam_transaction(nandc);

	nandc_set_reg(nandc, reg, val);

	write_reg_dma(nandc, reg, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	/* submit the descriptor to bam for execution*/
	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "Error in submitting descriptor to write config reg\n");
	free_descs(nandc);
}

static int qpic_serial_check_status(__le32 *status)
{
	if (*(__le32 *)status & FLASH_ERROR) {
		if (*(__le32 *)status & FS_MPU_ERR)
			return -EPERM;
		if (*(__le32 *)status & FS_TIMEOUT_ERR)
			return -ETIMEDOUT;
		if (*(__le32 *)status & FS_OP_ERR)
			return -EIO;
	}
	return 0;
}

static int qcom_serial_get_feature(struct qcom_nand_controller *nandc,
		struct qcom_nand_host *host, u32 faddr)
{
	u32 cmd_val = 0x0;
	u32 command = NAND_CMD_GET_FEATURE_SERIAL;
	int ret;

	/* Clear the BAM transaction index */
	clear_bam_transaction(nandc);

	cmd_val = (QPIC_SPI_TRANSFER_MODE_x1 | QPIC_SPI_WP | QPIC_SPI_HOLD |
			ACC_FEATURE);

	pre_command(host, command);

	nandc->buf_count = 4;

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd_val);
	nandc_set_reg(nandc, NAND_ADDR0, faddr);
	nandc_set_reg(nandc, NAND_ADDR1, 0);
	/* Clear the feature register value to get correct feature value */
	nandc_set_reg(nandc, NAND_FLASH_FEATURES, 0);

	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_FLASH_CMD, 3, DMA_DESC_FLAG_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_FEATURES, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_FEATURES, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);
	/* submit the descriptor to bam for execution*/
	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "Error in submitting descriptor for command:%d\n",
				command);
	free_descs(nandc);
	/* read_reg_dma will read data in to nandc->reg_read_buf
	 * so after issueing command in read_reg_dma function read reg_read_buf
	 * buffer
	 */
	ret = *(__le32 *)nandc->reg_read_buf;
	return ret;
}

static int qcom_serial_set_feature(struct qcom_nand_controller *nandc,
		struct qcom_nand_host *host, u32 faddr, u32 fval)
{
	int ret;
	u32 command = NAND_CMD_SET_FEATURE_SERIAL;
	u32 cmd_val = (QPIC_SPI_TRANSFER_MODE_x1 | QPIC_SPI_WP | QPIC_SPI_HOLD |
			ACC_FEATURE | QPIC_SET_FEATURE);

	/* Clear the BAM transaction index */
	clear_bam_transaction(nandc);

	pre_command(host, command);

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd_val);
	nandc_set_reg(nandc, NAND_ADDR0, faddr);
	nandc_set_reg(nandc, NAND_ADDR1, 0);
	nandc_set_reg(nandc, NAND_FLASH_FEATURES, fval);

	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_FLASH_CMD, 3, DMA_DESC_FLAG_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_FEATURES, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_EXEC_CMD, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_STATUS, 1, DMA_DESC_FLAG_BAM_NEXT_SGL);

	/* submit the descriptor to bam for execution*/
	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "Error in submitting descriptor for command:%d\n",
				command);
	free_descs(nandc);

	/* read_reg_dma will read data in to nandc->reg_read_buf
	 * so after issueing command in read_reg_dma function read reg_read_buf
	 * buffer
	 */
	ret = qpic_serial_check_status(nandc->reg_read_buf);
	if (ret) {
		dev_err(nandc->dev, "Error in executing command:%d\n",command);
		return ret;
	}
	return ret;
}

static void qcom_check_quad_mode(struct mtd_info *mtd, struct qcom_nand_host *host)
{
	int i, ret;
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	unsigned int command = NAND_CMD_READID_SERIAL;
	u8 id_data[3];
	u32 cmd3_val;

	pre_command(host, command);

	/* get the device id from device */
	nandc->buf_count = 4;
	read_id(host, 0x00);

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev,
				"failure submitting descs for command %d\n",
				command);
	free_descs(nandc);

	post_command(host, command);

	/* Read Id bytes */
	for (i = 0; i < 2; i++)
		id_data[i] = chip->read_byte(mtd);
	if (id_data[0] == SPI_FLASH_MICRON_ID) {
		cmd3_val = NAND_FLASH_DEV_CMD3_VAL & NAND_FLASH_DEV_CMD3_MASK;
		host->check_qe_bit = false;
		nandc_write(nandc, NAND_DEV_CMD3, cmd3_val);
	} else if (id_data[0] == SPI_FLASH_GIGA_ID &&
			id_data[1] == SPI_FLASH_ESMT_DEVICE_ID) {
	       host->check_qe_bit = false;
	} else
		host->check_qe_bit = true;
}

static int qpic_nand_dt_array(struct device *dev, const char *prop_name,
		u32 **out, int *len)
{
	int ret = 0;
	struct device_node *np = dev->of_node;
	size_t sz;
	u32 *arr = NULL;

	if (!of_get_property(np, prop_name, len)) {
		ret = -EINVAL;
		goto out;
	}
	sz = *len = *len / sizeof(*arr);
	if (sz <= 0) {
		dev_err(dev, "%s invalid size\n", prop_name);
		ret = -EINVAL;
		goto out;
	}

	arr = devm_kzalloc(dev, sz * sizeof(*arr), GFP_KERNEL);
	if (!arr) {
		dev_err(dev, "%s failed allocating memory\n", prop_name);
		ret = -ENOMEM;
		goto out;
	}

	ret = of_property_read_u32_array(np, prop_name, arr, sz);
	if (ret < 0) {
		dev_err(dev, "%s failed reading array %d\n", prop_name, ret);
		goto out;
	}
	*out = arr;

out:
	if (ret)
		*len = 0;
	return ret;
}

static struct qpic_nand_pltfm_data *qpic_nand_populate_pdata(struct device *dev,
		struct qcom_nand_controller *nandc)
{
	struct qpic_nand_pltfm_data *pdata = NULL;
	int io_macro_clk_len, i;
	u32 *io_macro_clk_table = NULL;
	u32 io_macromax_clk = 0, phase = 0;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "failed to allocate memory for platform data\n");
		goto out;
	}

	if(qpic_nand_dt_array(dev, "qpic,io_macro_clk_rates",
				&io_macro_clk_table, &io_macro_clk_len)) {
		dev_err(dev, "failed parsing supported io_macro clock rates\n");
		goto out;
	}

	pdata->io_macro_clk_table = io_macro_clk_table;
	pdata->io_macro_clk_cnt = io_macro_clk_len;

	/* Read phase value */
	if (!of_property_read_u32(dev->of_node, "qcom,phase", &phase)) {
		if (phase) {
			nandc->phase = phase;
		} else {
			pr_err("phase not found in dts, falling to 50MHz\n");
			goto out;
		}
	} else {
		pr_err("qcom,phase property not found in dts\n");
		goto out;
	}

	/* Read the io_macro_max_clk & phase value patched from u-boot */
	if (!of_property_read_u32(dev->of_node, "qcom,iomacromax_clk",
				&io_macromax_clk)) {
		if (io_macromax_clk) {
			for (i = 0; i < io_macro_clk_len; i++) {
				if (pdata->io_macro_clk_table[i] == io_macromax_clk) {
					nandc->io_macro_max_clk = io_macromax_clk;
					break;
				}
			}
		} else {
			pr_err("qcom,iomacromax_clk not found in dts, falling to 50MHz\n");
			goto out;
		}
		/* clear the feedback clock bit */
		if (nandc->io_macro_max_clk > pdata->io_macro_clk_table[2]) {
			qspi_write_reg_bam(nandc, (nandc_read(nandc,
				NAND_QSPI_MSTR_CONFIG) & ~FEEDBACK_CLK_EN), NAND_QSPI_MSTR_CONFIG);
		}
	} else {
		pr_err("qcom,iomacromax_clk not found in dts, falling to 50MHz\n");
		goto out;
	}

	return pdata;
out:
	/* set 50MHz clock with feedback clock bit enabled */
	nandc->io_macro_max_clk = pdata->io_macro_clk_table[2];

	return NULL;
}

static bool Is_Device_Winbond(struct mtd_info *mtd, struct qcom_nand_host *host)
{
	int i, ret;
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	unsigned int command = NAND_CMD_READID_SERIAL;
	u8 id_data[3];

	pre_command(host, command);

	/* get the device id from device */
	nandc->buf_count = 4;
	read_id(host, 0x00);

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure submitting descs for command %d\n",
				command);
	free_descs(nandc);

	post_command(host, command);

	/* Read Id bytes */
	for (i = 0; i < 2; i++)
		id_data[i] = chip->read_byte(mtd);

	if (id_data[0] == SPI_FLASH_WINBOND_ID)
		return true;

	return false;
}

int qcom_serial_device_config(struct mtd_info *mtd, struct qcom_nand_host *host)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	int status;
	/* One more time read device here to know the quad mode bit has to
	 * enable or not in device configuration register. In Micron default
	 * quad mode is enabled so no need to check the device configuration for
	 * quad mode enablement.
	 */
	qcom_check_quad_mode(mtd, host);

	/* Get the Block protection status of device. On power on serial
	 * nand device all block will be protected and we acn not erased
	 * a protected block. To check the device feature issue get feature
	 * command. To set a feature issue set feature command to device.
	 */
	status = qcom_serial_get_feature(nandc, host, SPI_FLASH_BLOCK_PROT_REG);
	if (status < 0) {
		dev_err(nandc->dev,"Error in getting feature block protection.");
		return status;
	}

	if ((status >> 8) & BLOCK_PROTECTION_ENABLE) {

		/* Call set feature function to disable block protection
		 */
		status = qcom_serial_set_feature(nandc, host, SPI_FLASH_BLOCK_PROT_REG,
				BLOCK_PROTECTION_DISABLE);
		if (status < 0) {
			 dev_err(nandc->dev,"Error in setting feature block protection.");
			 return status;
		}
		/* After disabling the block protection again issue the get feature
		 * command to read the status
		 */
		status = qcom_serial_get_feature(nandc, host, SPI_FLASH_BLOCK_PROT_REG);
		if (status < 0) {
			dev_err(nandc->dev,"Error in getting feature block protection.");
			return status;
		}
		if ((status >> 8) & BLOCK_PROTECTION_ENABLE) {
			dev_err(nandc->dev,"Block protection enabled");
			return -EIO;
		}
	}

	/* Get device internal ECC status. if device internal ECC is enabled then
	 * issue set feature command to disable it.
	 */
	status = qcom_serial_get_feature(nandc, host, SPI_FLASH_FEATURE_REG);
	if (status < 0) {
		dev_err(nandc->dev,"Error in getting feature Internal ECC.");
		return status;
	}

	if (((status >> 8) & SPI_FLASH_ECC_ENABLE) || ((status >> 8) & SPI_CFG_ENABLED)) {
		/* Device Internal ECC enabled call set feature command to
		 * disable internal ECC
		 */
		status = qcom_serial_set_feature(nandc, host, SPI_FLASH_FEATURE_REG,
				SPI_FLASH_ECC_DISABLE);
		if (status < 0) {
			dev_err(nandc->dev,"Error in setting feature internal ecc disable.");
			return status;
		}
		/* Again check if internal ecc is disabled or not using get feature command
		 */

		status = qcom_serial_get_feature(nandc, host, SPI_FLASH_FEATURE_REG);
		if (status < 0) {
			dev_err(nandc->dev,"Error in getting feature Internal ECC.");
			return status;
		}
		if ((status >> 8) & SPI_FLASH_ECC_ENABLE) {
			dev_err(nandc->dev,"Device Internal ecc is enabled.");
			return -EIO;
		}
	}

	if (!host->check_qe_bit) {
		host->quad_mode = true;
		return 0;
	}

	/* Check for QUAD mode if device supported and enable it if not enabled*/
	status = qcom_serial_get_feature(nandc, host, SPI_FLASH_FEATURE_REG);
	if (status < 0) {
		dev_err(nandc->dev,"Error in getting feature Quad mode");
		return status;
	}

	if (!((status >> 8) & SPI_FLASH_QUAD_MODE)) {
		/* Quad mode bit not enabled issue set feature command
		 * to enable quad mode bit of flash device.
		 */
		status = qcom_serial_set_feature(nandc, host, SPI_FLASH_FEATURE_REG,
				SPI_FLASH_QUAD_MODE);
		if (status < 0) {
			dev_err(nandc->dev,"Error in setting feature Quad mode.");
			return status;
		}
		/* again issue the get feature command to check if quad
		 * mode is enabled or not
		 */
		status = qcom_serial_get_feature(nandc, host, SPI_FLASH_FEATURE_REG);
		if (status < 0) {
			dev_err(nandc->dev,"Error in getting feature Quad mode");
			return status;
		}

		if ((status >> 8) & SPI_FLASH_QUAD_MODE) {
			host->quad_mode = true;
		} else {
			host->quad_mode = false;
			dev_err(nandc->dev,"Quad mode not enabled using X1 mode.\n");
		}
	}

	/* Enable continous BUFF bit only for Winbond device
	 * Read the id and compare for winbond device id
	 */
	if (Is_Device_Winbond(mtd, host)) {
		status = qcom_serial_get_feature(nandc, host, SPI_FLASH_FEATURE_REG);
		if (status < 0) {
			dev_err(nandc->dev,"Error in getting feature Continous buff");
			return status;
		}

		if (!((status >> 8) & SPI_NAND_FR_BUFF_ENABLE)) {
			dev_dbg(nandc->dev, "Continous buffer mode not enabled on power on\n");
			dev_dbg(nandc->dev, "Issuing set feature command enbale it\n");
			status = qcom_serial_set_feature(nandc, host, SPI_FLASH_FEATURE_REG,
					SPI_NAND_FR_BUFF_ENABLE | (status >> 8));
			if (status < 0) {
				dev_err(nandc->dev,"Error in setting feature Quad mode.");
				return status;
			}
		} else {
			dev_dbg(nandc->dev, "Continous buffer mode enabled on power on\n");
		}
	}

	return 0;
}

void qcom_serial_nand_init(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	/* Configure the NAND_FLASH_SPI_CFG to load the timer CLK_CNTR_INIT_VAL_VEC value,
	 * enable the LOAD_CLK_CNTR_INIT_EN bit and enable SPI_CFG mode.
	 * Reset the NAND_FLASH_SPI_CFG.LOAD_CLK_CNTR_INIT_EN to generate a pulse,
	 * with CLK_CNTR_INIT_VAL_VEC loaded and SPI_CFG enabled
	 *
	 * Initialization sequence as per HPG.
	 */
	u32 spi_cfg_val = 0x0;
	u32 nand_ctrl;
	u32 busy_wait_check_cnt = SPI_BUSY_CHECK_WAIT_CNT;

	u32 cmd0_val, cmd1_val, cmd2_val, cmd3_val, cmd7_val;

	cmd0_val = NAND_FLASH_DEV_CMD0_VAL;
	cmd1_val = NAND_FLASH_DEV_CMD1_VAL;
	cmd2_val = NAND_FLASH_DEV_CMD2_VAL;
	cmd3_val = NAND_FLASH_DEV_CMD3_VAL;
	cmd7_val = NAND_FLASH_DEV_CMD7_VAL;

	spi_cfg_val |= (SPI_LOAD_CLK_CNTR_INIT_EN | SPI_FLASH_MODE_EN |
			SPI_FEA_STATUS_DEV_ADDR| (SPI_NUM_ADDR2_CYCLES << 2));

	if (nandc->phase == 0) {
		spi_cfg_val |= SPI_CFG_CLK_CNTR_INIT_VAL_VEC << 16;
	} else {
		spi_cfg_val |= ((nandc->phase << 16) | (nandc->phase << 19) |
				(nandc->phase << 22) | (nandc->phase << 25));
	}

	nand_ctrl = nandc_read(nandc, NAND_CTRL);
	if (nand_ctrl & BAM_MODE_EN) {
		/* write register via BAM */
		qspi_write_reg_bam(nandc, 0x0, NAND_FLASH_SPI_CFG);
		qspi_write_reg_bam(nandc, spi_cfg_val, NAND_FLASH_SPI_CFG);

		spi_cfg_val &= ~(SPI_LOAD_CLK_CNTR_INIT_EN);
		qspi_write_reg_bam(nandc, spi_cfg_val, NAND_FLASH_SPI_CFG);
	} else {
		nandc_write(nandc, NAND_FLASH_SPI_CFG , 0x0);
		nandc_write(nandc, NAND_FLASH_SPI_CFG , spi_cfg_val);

		spi_cfg_val &= ~(SPI_LOAD_CLK_CNTR_INIT_EN);
		nandc_write(nandc, NAND_FLASH_SPI_CFG , spi_cfg_val);
	}

	/* According to HPG Setting Xfer steps and spi_num_addr_cycles
	 * is part of initialization flow before reset.However these
	 * values differ from NAND part to part.sitting in QPIC layer
	 * we won't know which NAND we don't know which NAND is connected.
	 * So we are not following HPG init sequence.Instead we reset and
	 * read id of NAND,then based on NAND ID we get Xfer steps
	 * and spi_num_addr_cycles and configure them in this function.Since
	 * Xfer steps and spi_num_addr_cycles are required for read/write/erase
	 * functionality.
	 *
	 * NOTE: For now address cycle is same for Giga devices & Micron devices
	 * so we can configure no of addess cycle here only
	 * The NAND_FLASH_XFR_STEP register also fixed for both the devices so we
	 * can configure this register here only . later change this logic as per
	 * device
	 *
	 * NOTE: For now XFER register value is fixed so use that fixed value.
	 *
	 */
	nandc_write(nandc, NAND_FLASH_XFR_STEP1, QPIC_FLASH_XFR_STEP1_VAL);
	nandc_write(nandc, NAND_FLASH_XFR_STEP2, QPIC_FLASH_XFR_STEP2_VAL);
	nandc_write(nandc, NAND_FLASH_XFR_STEP3, QPIC_FLASH_XFR_STEP3_VAL);
	nandc_write(nandc, NAND_FLASH_XFR_STEP4, QPIC_FLASH_XFR_STEP4_VAL);
	nandc_write(nandc, NAND_FLASH_XFR_STEP5, QPIC_FLASH_XFR_STEP5_VAL);
	nandc_write(nandc, NAND_FLASH_XFR_STEP6, QPIC_FLASH_XFR_STEP6_VAL);
	nandc_write(nandc, NAND_FLASH_XFR_STEP7, QPIC_FLASH_XFR_STEP7_VAL);

	nandc_write(nandc, NAND_DEV_CMD0, cmd0_val);
	nandc_write(nandc, NAND_DEV_CMD1, cmd1_val);
	nandc_write(nandc, NAND_DEV_CMD2, cmd2_val);
	nandc_write(nandc, NAND_DEV_CMD3, cmd3_val);
	nandc_write(nandc, NAND_DEV_CMD7, cmd7_val);
	/* CMD8 & CMD9 value default value should be used*/

	nandc_write(nandc, NAND_DEV_CMD_VLD, NAND_DEV_CMD_VLD_SERIAL_VAL);
	if (nand_ctrl & BAM_MODE_EN) {
		/* write register via bam */
		qspi_write_reg_bam(nandc, SPI_NO_OF_ADDR_CYCLE, NAND_SPI_NUM_ADDR_CYCLES);
		qspi_write_reg_bam(nandc, busy_wait_check_cnt, NAND_SPI_BUSY_CHECK_WAIT_CNT);
	} else {
		nandc_write(nandc, NAND_SPI_NUM_ADDR_CYCLES, SPI_NO_OF_ADDR_CYCLE);
		nandc_write(nandc, NAND_SPI_BUSY_CHECK_WAIT_CNT, busy_wait_check_cnt);
	}
}
#endif

#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
static int qpic_alloc_status_buff(struct qcom_nand_controller *nandc,
		struct mtd_info *mtd)
{
	GET_STATUS_BUFF_ALLOC_SIZE(mtd->writesize,
			nandc->status_buf_size);

	nandc->status_buf = devm_kzalloc(nandc->dev, nandc->status_buf_size,
				GFP_KERNEL);
		if (!nandc->status_buf)
			return -ENOMEM;

		nandc->ps_mp_flag = 0x0;

	return 0;
}
#endif

static int qcom_nand_host_init(struct qcom_nand_controller *nandc,
			       struct qcom_nand_host *host,
			       struct device_node *dn)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct device *dev = nandc->dev;
	struct mtd_part_parser_data ppdata = { .of_node = dn };
	int ret;

	ret = of_property_read_u32(dn, "reg", &host->cs);
	if (ret) {
		dev_err(dev, "can't get chip-select\n");
		return -ENXIO;
	}

	/* Read the QPIC controller version register */
	host->hw_version = nandc_read(nandc, NAND_VERSION);
	pr_info("QPIC controller hw version Major:%d, Minor:%d\n",
			((host->hw_version & NAND_VERSION_MAJOR_MASK) >> NAND_VERSION_MAJOR_SHIFT),
			((host->hw_version & NAND_VERSION_MINOR_MASK) >> NAND_VERSION_MINOR_SHIFT));

	host->hw_version = ((host->hw_version & NAND_VERSION_MAJOR_MASK) >> NAND_VERSION_MAJOR_SHIFT);
	if (host->hw_version >= QPIC_VERSION_V2_0)
		pr_info("QPIC controller support serial nand.\n");

	nand_set_flash_node(chip, dn);
	mtd->name = devm_kasprintf(dev, GFP_KERNEL, "qcom_nand.%d", host->cs);
	mtd->owner = THIS_MODULE;
	mtd->dev.parent = dev;
	mtd->priv = chip;
	chip->priv = nandc;

	chip->cmdfunc		= qcom_nandc_command;
	chip->select_chip	= qcom_nandc_select_chip;
	chip->read_byte		= qcom_nandc_read_byte;
	chip->read_buf		= qcom_nandc_read_buf;
	chip->write_buf		= qcom_nandc_write_buf;

	/*
	 * the bad block marker is readable only when we read the last codeword
	 * of a page with ECC disabled. currently, the nand_base and nand_bbt
	 * helpers don't allow us to read BB from a nand chip with ECC
	 * disabled (MTD_OPS_PLACE_OOB is set by default). use the block_bad
	 * and block_markbad helpers until we permanently switch to using
	 * MTD_OPS_RAW for all drivers (with the help of badblockbits)
	 */
	chip->block_bad		= qcom_nandc_block_bad;
	chip->block_markbad	= qcom_nandc_block_markbad;

	chip->controller = &nandc->controller;
	chip->options |= NAND_NO_SUBPAGE_WRITE | NAND_USE_BOUNCE_BUFFER |
			 NAND_SKIP_BBTSCAN;

	/* set up initial status value */
	host->status = NAND_STATUS_READY | NAND_STATUS_WP;

#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	qcom_serial_nand_init(mtd);

	ret = nand_scan_ident(mtd, 1, qspinand_flash_ids);
#else
	ret = nand_scan_ident(mtd, 1, NULL);
#endif
	if (ret)
		return ret;
#if IS_ENABLED(CONFIG_PAGE_SCOPE_MULTI_PAGE_READ)
	/* allocate memory for status buffer. we are doing
	 * this here because we do not know the device page
	 * siz ein advance if nand flash is parallel nand and ONFI
	 * complaint. so status buffer size will vary based on page size
	 * e.g if page size is 2KiB then status buffer size for one page
	 * will be 48-bytes similary for 4KiB page , status buffer size
	 * will be 96-bytes for one page and so on.
	 * QPIC controller support max page isze is 8 KiB now so maximum
	 * status buffer size for one page will be 192-bytes. for multi page
	 * read the status buffer size will be multiple of maximum pages supported
	 * in multipage.
	 */
	ret = qpic_alloc_status_buff(nandc, mtd);
	if (ret)
		return ret;
#endif

	ret = qcom_nand_host_setup(host);
	if (ret)
		return ret;

#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	ret = qcom_serial_device_config(mtd, host);
	if (ret)
		return ret;
#endif
	ret = nand_scan_tail(mtd);
	if (ret)
		return ret;

	return mtd_device_parse_register(mtd, NULL, &ppdata, NULL, 0);
}

/* parse custom DT properties here */
static int qcom_nandc_parse_dt(struct platform_device *pdev)
{
	struct qcom_nand_controller *nandc = platform_get_drvdata(pdev);
	struct device_node *np = nandc->dev->of_node;
	int ret;

	if (!nandc->dma_bam_enabled) {
		ret = of_property_read_u32(np, "qcom,cmd-crci",
				&nandc->cmd_crci);
		if (ret) {
			dev_err(nandc->dev, "command CRCI unspecified\n");
			return ret;
		}

		ret = of_property_read_u32(np, "qcom,data-crci",
				&nandc->data_crci);
		if (ret) {
			dev_err(nandc->dev, "data CRCI unspecified\n");
			return ret;
		}
	}

	return 0;
}

static ssize_t boot_layout_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct qcom_nand_controller *nandc = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", nandc->boot_layout);
}

static ssize_t boot_layout_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t n)
{
	struct qcom_nand_controller *nandc = dev_get_drvdata(dev);
	struct qcom_nand_host *host;
	int ret;

	ret = kstrtobool(buf, &nandc->boot_layout);
	if (ret) {
		dev_err(dev, "invalid boot_layout\n");
		return ret;
	}

	list_for_each_entry(host, &nandc->host_list, node) {
		host->cw_data = nandc->boot_layout ? 512 : 516;
		host->spare_bytes = host->cw_size - host->ecc_bytes_hw -
				    host->bbm_size - host->cw_data;

		host->cfg0 &= ~(SPARE_SIZE_BYTES_MASK | UD_SIZE_BYTES_MASK);
		host->cfg0 |= host->spare_bytes << SPARE_SIZE_BYTES |
			      host->cw_data << UD_SIZE_BYTES;

		host->ecc_bch_cfg &= ~ECC_NUM_DATA_BYTES_MASK;
		host->ecc_bch_cfg |= host->cw_data << ECC_NUM_DATA_BYTES;
		host->ecc_buf_cfg = (nandc->boot_layout ? 0x1ff : 0x203) <<
				     NUM_STEPS;
	}

	return n;
}

static const DEVICE_ATTR(boot_layout, 0644, boot_layout_show,
			 boot_layout_store);

static int qcom_nandc_probe(struct platform_device *pdev)
{
	struct qcom_nand_controller *nandc;
	struct qcom_nand_host *host;
	const void *dev_data;
	struct device *dev = &pdev->dev;
	struct device_node *dn = dev->of_node, *child;
	struct resource *res;
	int ret;
	struct qcom_nand_driver_data *driver_data;

	nandc = devm_kzalloc(&pdev->dev, sizeof(*nandc), GFP_KERNEL);
	if (!nandc)
		return -ENOMEM;

	platform_set_drvdata(pdev, nandc);
	nandc->dev = dev;

	dev_data = of_device_get_match_data(dev);
	if (!dev_data) {
		dev_err(&pdev->dev, "failed to get device data\n");
		return -ENODEV;
	}

	driver_data = (struct qcom_nand_driver_data *)dev_data;

	nandc->param_read_addr_cycle = NUM_ADDR_CYCLES_5;
	device_property_read_u32(dev, "param-read-addr-cycle",
				 &nandc->param_read_addr_cycle);
	nandc->param_read_addr_cycle &= NUM_ADDR_CYCLES_MASK;

	nandc->ecc_modes = driver_data->ecc_modes;
	nandc->dma_bam_enabled = driver_data->dma_bam_enabled;
	nandc->regs_offsets = driver_data->regs_offsets;
	nandc->create_sys_boot_layout = driver_data->create_sys_boot_layout;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	nandc->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(nandc->base))
		return PTR_ERR(nandc->base);

	nandc->base_dma = phys_to_dma(dev, (phys_addr_t)res->start);

	nandc->core_clk = devm_clk_get(dev, "core");
	if (IS_ERR(nandc->core_clk))
		return PTR_ERR(nandc->core_clk);

	nandc->aon_clk = devm_clk_get(dev, "aon");
	if (IS_ERR(nandc->aon_clk))
		return PTR_ERR(nandc->aon_clk);

	ret = qcom_nandc_parse_dt(pdev);
	if (ret)
		return ret;

	ret = qcom_nandc_alloc(nandc);
	if (ret)
		goto err_core_clk;

	ret = clk_prepare_enable(nandc->core_clk);
	if (ret)
		goto err_core_clk;

	ret = clk_prepare_enable(nandc->aon_clk);
	if (ret)
		goto err_aon_clk;

	ret = qcom_nandc_setup(nandc);
	if (ret)
		goto err_setup;

#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
	/* Initially enable feedback clock bit to avoid serial
	 * tarining done by software manually
	 * for feed back clock the maximum IO_MACRO_CLK should
	 * be 50MHz. so supply 200MHz as input source to IO_MACRO
	 * to enable feedback clock bit write into register
	 * NAND_QSPI_MSTR_CONFIG.
	 */
	qspi_write_reg_bam(nandc, (nandc_read(nandc,
		NAND_QSPI_MSTR_CONFIG) | FEEDBACK_CLK_EN), NAND_QSPI_MSTR_CONFIG);

	nandc->pdata = qpic_nand_populate_pdata(nandc->dev, nandc);
	if (!nandc->pdata) {
		pr_err("QPIC platform data not populated\n");
	}

	nandc->io_macro_clk = devm_clk_get(dev, "io_macro");
	if (IS_ERR(nandc->io_macro_clk))
		return PTR_ERR(nandc->io_macro_clk);

	/* set the clock io_macro rate and then enable
	 * the io_macro clock
	 */
	ret =  clk_set_rate(nandc->io_macro_clk, nandc->io_macro_max_clk);
	if (ret)
		goto err_io_macro_clk;

	ret = clk_prepare_enable(nandc->io_macro_clk);
	if (ret)
		goto err_io_macro_clk;
#endif
	for_each_available_child_of_node(dn, child) {
		if (of_device_is_compatible(child, "qcom,nandcs")) {
			host = devm_kzalloc(dev, sizeof(*host), GFP_KERNEL);
			if (!host) {
				of_node_put(child);
				ret = -ENOMEM;
				goto err_cs_init;
			}

			ret = qcom_nand_host_init(nandc, host, child);
			if (ret) {
				devm_kfree(dev, host);
				continue;
			}

			list_add_tail(&host->node, &nandc->host_list);
		}
	}

	if (list_empty(&nandc->host_list)) {
		ret = -ENODEV;
		goto err_cs_init;
	}

	if (nandc->create_sys_boot_layout) {
		ret = sysfs_create_file(&pdev->dev.kobj,
					&dev_attr_boot_layout.attr);
		if (ret)
			goto err_cs_init;
	}

	return 0;

err_cs_init:
	list_for_each_entry(host, &nandc->host_list, node)
		nand_release(nand_to_mtd(&host->chip));
#if IS_ENABLED(CONFIG_MTD_NAND_SERIAL)
err_io_macro_clk:
	clk_disable_unprepare(nandc->io_macro_clk);
#endif
err_setup:
	clk_disable_unprepare(nandc->aon_clk);
err_aon_clk:
	clk_disable_unprepare(nandc->core_clk);
err_core_clk:
	qcom_nandc_unalloc(nandc);

	return ret;
}

static int qcom_nandc_remove(struct platform_device *pdev)
{
	struct qcom_nand_controller *nandc = platform_get_drvdata(pdev);
	struct qcom_nand_host *host;

	list_for_each_entry(host, &nandc->host_list, node)
		nand_release(nand_to_mtd(&host->chip));

	if (nandc->create_sys_boot_layout)
		sysfs_remove_file(&pdev->dev.kobj, &dev_attr_boot_layout.attr);

	qcom_nandc_unalloc(nandc);

	clk_disable_unprepare(nandc->aon_clk);
	clk_disable_unprepare(nandc->core_clk);

	return 0;
}

struct qcom_nand_driver_data ebi2_nandc_bam_data = {
	.ecc_modes = (ECC_BCH_4BIT | ECC_BCH_8BIT),
	.dma_bam_enabled = true,
	.regs_offsets = regs_offsets_v1_4_0,
};

struct qcom_nand_driver_data ebi2_nandc_data = {
	.ecc_modes = (ECC_RS_4BIT | ECC_BCH_8BIT),
	.dma_bam_enabled = false,
	.create_sys_boot_layout = true,
	.regs_offsets = regs_offsets_v1_4_0,
};

struct qcom_nand_driver_data ebi2_nandc_bam_v1_5_0_data = {
	.ecc_modes = (ECC_BCH_4BIT | ECC_BCH_8BIT),
	.dma_bam_enabled = true,
	.regs_offsets = regs_offsets_v1_5_0,
};

struct qcom_nand_driver_data ebi2_nandc_bam_v2_1_1_data = {
	.ecc_modes = (ECC_BCH_4BIT | ECC_BCH_8BIT),
	.dma_bam_enabled = true,
	.regs_offsets = regs_offsets_v2_1_1,
};

/*
 * data will hold a struct pointer containing more differences once we support
 * more controller variants
 */
static const struct of_device_id qcom_nandc_of_match[] = {
	{	.compatible = "qcom,ipq806x-nand",
		.data = (void *) &ebi2_nandc_data,
	},
	{	.compatible = "qcom,ebi2-nandc-bam",
		.data = (void *) &ebi2_nandc_bam_data,
	},
	{	.compatible = "qcom,ebi2-nandc-bam-v1.5.0",
		.data = (void *) &ebi2_nandc_bam_v1_5_0_data,
	},
	{	.compatible = "qcom,ebi2-nandc-bam-v2.1.1",
		.data = (void *) &ebi2_nandc_bam_v2_1_1_data,
	},
	{}
};
MODULE_DEVICE_TABLE(of, qcom_nandc_of_match);

static struct platform_driver qcom_nandc_driver = {
	.driver = {
		.name = "qcom-nandc",
		.of_match_table = qcom_nandc_of_match,
	},
	.probe   = qcom_nandc_probe,
	.remove  = qcom_nandc_remove,
};
module_platform_driver(qcom_nandc_driver);

MODULE_AUTHOR("Archit Taneja <architt@codeaurora.org>");
MODULE_DESCRIPTION("Qualcomm NAND Controller driver");
MODULE_LICENSE("GPL v2");
