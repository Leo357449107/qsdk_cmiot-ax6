/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 * Copyright (c) 2009-2017, 2019 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include <debug.h>
#include <stdlib.h>
#include <lib/console.h>
#include <lib/ptable.h>
#include <smem.h>
#include <baseband.h>
#include <platform/iomap.h>
#include <platform/clock.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <platform/irqs.h>
#include <platform/interrupts.h>
#include <reg.h>
#include <i2c_qup.h>
#include <gsbi.h>
#include <target.h>
#include <platform.h>
#include <uart_dm.h>
#include <crypto_hash.h>
#include <board.h>
#include <target/board.h>
#include <scm.h>
#include <mmc.h>
#include <mmc_sdhci.h>
#include <sdhci_msm.h>
#include <stdlib.h>
#include <mmc_wrapper.h>
#include <partition_parser.h>
#include <usb30_wrapper.h>
#include <string.h>

#define CLEAR_MAGIC				0x0
#define SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID 	0x9
#define SCM_CMD_TZ_FORCE_DLOAD_ID 		0x10
#define CDUMP_MODE				0x1

#define CE1_INSTANCE		1
#define CE_EE			1
#define CE_FIFO_SIZE		512
#define CE_READ_PIPE		3
#define CE_WRITE_PIPE		2
#define CE_READ_PIPE_LOCK_GRP	0
#define CE_WRITE_PIPE_LOCK_GRP	0
#define CE_ARRAY_SIZE		20
#define MSM_CE1_BAM_BASE	0x00704000
#define MSM_CE1_BASE		0x0073a000
#define I2C_STATUS_ERROR_MASK 0x38000FC

extern struct mmc_card *get_mmc_card();

extern void dmb(void);
static struct qup_i2c_dev *dev = NULL;
static int i2c_qup_initialized = -1;

#define TZ_INFO_GET_DIAG_ID	0x2
#define SCM_SVC_INFO		0x6
#define TZBSP_DIAG_BUF_LEN	0x3000

/**
 * struct tzbsp_log_pos_t - log position structure
 * @wrap: Ring buffer wrap-around ctr
 * @offset: Ring buffer current position
 */
struct tzbsp_log_pos_t {
	uint16_t wrap;
	uint16_t offset;
};

/**
 * struct tzbsp_diag_log_t - log structure
 * @log_pos: Ring buffer position mgmt
 * @log_buf: Open ended array to the end of the 4K IMEM buffer
 */
struct tzbsp_diag_log_t {
	struct tzbsp_log_pos_t log_pos;
	uint8_t log_buf[1];
};

/**
 * struct ipq6018_tzbsp_diag_t_v8 -  tz diag log structure
 * @unused: Unused variable is to support the corresponding structure in
 *		trustzone and size is varying based on AARCH64 TZ
 */
struct ipq6018_tzbsp_diag_t_v8 {
	uint32_t unused[7];
	uint32_t ring_off;
	uint32_t unused1[802];
	struct tzbsp_diag_log_t log;
};

/* Setting this variable to different values defines the
 * behavior of CE engine:
 * platform_ce_type = CRYPTO_ENGINE_TYPE_NONE : No CE engine
 * platform_ce_type = CRYPTO_ENGINE_TYPE_SW : Software CE engine
 * platform_ce_type = CRYPTO_ENGINE_TYPE_HW : Hardware CE engine
 * Behavior is determined in the target code.
 */
static crypto_engine_type platform_ce_type = CRYPTO_ENGINE_TYPE_HW;

static void target_uart_init(void);
static void target_crypto_init_params();
#define DELAY 1

static void set_sdc_power_ctrl(void){}

static struct mmc_device *mmc_dev;

static uint32_t mmc_pwrctl_base[] =
	{ MSM_SDC1_BASE, MSM_SDC2_BASE };

static uint32_t mmc_sdhci_base[] =
	{ MSM_SDC1_SDHCI_BASE, MSM_SDC2_SDHCI_BASE };

static uint32_t  mmc_sdc_pwrctl_irq[] =
	{ SDCC1_PWRCTL_IRQ, SDCC2_PWRCTL_IRQ };


void display_tzlog(void)
{
	char *tz_buf;
	char *copy_buf, *tmp_buf;
	uint32_t buf_len, copy_len = 0;
	uint16_t wrap, ring, offset;
	struct tzbsp_diag_log_t *log;
	struct ipq6018_tzbsp_diag_t_v8 *ipq6018_diag_buf;
	int ret;

	buf_len = TZBSP_DIAG_BUF_LEN;
	tz_buf = malloc(buf_len);
	if (!tz_buf) {
		dprintf(CRITICAL, "malloc failed for tz_buf\n");
		goto err_tzbuf;
	}

	copy_buf = malloc(buf_len);
	if (!copy_buf) {
		dprintf(CRITICAL, "malloc failed for copy_buf\n");
		goto err_copybuf;
	}
	tmp_buf = copy_buf;
	memset((void *)copy_buf, 0, (size_t)buf_len);
	memset((void *)tz_buf, 0, (size_t)buf_len);


	/* SCM call to TZ to get the tz log */
	ret = qca_scm_tz_log(SCM_SVC_INFO, TZ_INFO_GET_DIAG_ID,
					tz_buf, buf_len);
	if (ret != 0) {
		dprintf(CRITICAL, "Error in getting tz log\n");
		goto err_scm;
	}

	ipq6018_diag_buf = (struct ipq6018_tzbsp_diag_t_v8 *)tz_buf;
	ring = ipq6018_diag_buf->ring_off;
	log = &ipq6018_diag_buf->log;

	offset = log->log_pos.offset;
	wrap = log->log_pos.wrap;

	if (wrap != 0) {
		memcpy(copy_buf, (tz_buf + offset + ring),
			(buf_len - offset - ring));
		memcpy(copy_buf + (buf_len - offset - ring),
				(tz_buf + ring), offset);
		copy_len = (buf_len - offset - ring) + offset;
	}
	else {
		memcpy(copy_buf, (tz_buf + ring), offset);
		copy_len = offset;
	}

	/* display buffer to console*/
	dprintf(INFO, "TZ LOG:\n\n");
	for (uint32_t i = 0; i < copy_len; i++) {
		printf("%c", (char)*copy_buf);
		copy_buf += 1;
	}
	printf("\n");

err_scm:
	free(tmp_buf);
err_copybuf:
	free(tz_buf);
err_tzbuf:
	return;
}

int target_is_emmc_boot(void)
{
	return 1;
}

void target_early_init(void)
{
	target_uart_init();
}

void shutdown_device(void)
{
	dprintf(CRITICAL, "Shutdown system.\n");

	reboot_device(0);

	dprintf(CRITICAL, "Shutdown failed.\n");
}
void target_sdc_init()
{
	struct mmc_config_data config;

	/* Set drive strength & pull ctrl values */
	set_sdc_power_ctrl();

	config.bus_width = DATA_BUS_WIDTH_8BIT;
	config.max_clk_rate = MMC_CLK_200MHZ;

	/* Try slot 1 */
	config.slot         = 1;
	config.sdhc_base    = mmc_sdhci_base[config.slot - 1];
	config.pwrctl_base  = mmc_pwrctl_base[config.slot - 1];
	config.pwr_irq      = mmc_sdc_pwrctl_irq[config.slot - 1];
	config.hs400_support = 0;
	config.hs200_support = 0;
	config.ddr_support = 0;
	config.use_io_switch = 1;

	if (!(mmc_dev = mmc_init(&config))) {
		dprintf(CRITICAL, "mmc init failed!");
		ASSERT(0);
	}
}

void *target_mmc_device()
{
	return (void *) mmc_dev;
}

void target_mmc_deinit()
{
	sdhci_mode_disable(&(mmc_dev->host));
}

uint8_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *val, unsigned short len)
{
	uint8_t ret = 0;
	unsigned char buf[2];

	buf[0] = (reg & 0xFF00) >> 8;
	buf[1] = (reg & 0xFF);

	if (dev) {
		struct i2c_msg rd_buf[] = {
			{addr, I2C_M_WR, 2, buf},
			{addr, I2C_M_RD, len, val}
		};

		int err = qup_i2c_xfer(dev, rd_buf, 2);
		if (err < 0) {
			dprintf(INFO, "Read reg %x failed\n", err);
			return err;
		}
		else {
			ret = err & I2C_STATUS_ERROR_MASK;
		}
	} else {
		dprintf(INFO, "eeprom_read_reg: qup_i2c_init() failed\n");
		return -1;
	}

	return ret;
}

void i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t *val, unsigned short len)
{
	if (dev) {
		unsigned char *buf;

		buf = malloc(len + 2);
		if (!buf)
			return;

		buf[0] = (reg & 0xFF00) >> 8;
		buf[1] = (reg & 0xFF);
		memcpy(buf+2, val, len);
		struct i2c_msg msg_buf[] = {
			{addr, I2C_M_WR, len+2, buf},
		};

		int err = qup_i2c_xfer(dev, msg_buf, 1);
		if (err < 0) {
			dprintf(INFO, "Write reg %x failed\n", reg);
			free(buf);
			return;
		}
		free(buf);
	} else
		dprintf(INFO, "eeprom_write_reg: qup_i2c_init() failed\n");
}


void i2c_ipq6018_init(uint8_t blsp_id, uint8_t qup_id)
{
	dev = qup_blsp_i2c_init(blsp_id, qup_id, 400000, 24000000);
	if(!dev) {
		dprintf(INFO, "qup_blsp_i2c_init() failed\n");
		return;
	}
	i2c_qup_initialized = qup_id;
}

static void initialize_crashdump(void)
{
	int ret;
	ret = qca_scm_call_write(SCM_SVC_IO_ACCESS, SCM_IO_WRITE,
				(uint32_t *)0x193D100, APPS_DLOAD_MAGIC);
	if (ret)
		dprintf(CRITICAL, "Error setting crashdump magic\n");
}

void reset_crashdump(bool sdi_only)
{
	int ret;

	if (is_scm_armv8())
		qca_scm_sdi_v8(SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID);

	if(!sdi_only) {
		ret = qca_scm_call_write(SCM_SVC_IO_ACCESS, SCM_IO_WRITE,
				(uint32_t *)0x193D100, CLEAR_MAGIC);
		if (ret) {
			dprintf(CRITICAL, "Error resetting crashdump magic\n");
			return;
		}
	}
}

static enum handler_return tzerr_irq_handler(void *arg)
{
	dprintf(CRITICAL, "NOC Error detected, Halting...\n");
	display_tzlog();
	halt();
	return IRQ_HANDLED;
}

void enable_noc_error_detection(void)
{
	dprintf(INFO, "Enable tzerr IRQ\n");
	register_int_handler(TZ_ERR_IRQ, tzerr_irq_handler, 0);
	unmask_interrupt(TZ_ERR_IRQ);
}

void target_init(void)
{
	unsigned platform_id = board_platform_id();
	struct mmc_boot_host *mmc_host = get_mmc_host();
	struct mmc_card *mmc_card = get_mmc_card();

	dprintf(INFO, "target_init()\n");
	dprintf(INFO, "board platform id is 0x%x\n",  platform_id);
	dprintf(INFO, "board platform verson is 0x%x\n",  board_platform_ver());

	initialize_crashdump();
	enable_noc_error_detection();
	target_sdc_init();
	if (partition_read_table(mmc_host, (struct mmc_boot_card *)mmc_card))
	{
		dprintf(CRITICAL, "Error reading the partition table info\n");
		ASSERT(0);
	}
	target_crypto_init_params();
}

unsigned board_machtype(void)
{
	return board_target_id();
}

crypto_engine_type board_ce_type(void)
{
	return platform_ce_type;
}

void reboot_device(unsigned reboot_reason)
{
	writel(reboot_reason, RESTART_REASON_ADDR);

	if (reboot_reason == CDUMP_MODE) {
		dprintf(INFO, "CDUMP mode enabled\n");
		reset_crashdump(1);
	} else {
		reset_crashdump(0);
	}

	writel(0, GCNT_PSHOLD);
	mdelay(10000);

	dprintf(CRITICAL, "Rebooting failed\n");
}

unsigned check_reboot_mode(void)
{
	unsigned restart_reason;

	/* Read reboot reason and scrub it */
	restart_reason = readl(RESTART_REASON_ADDR);
	writel(0x00, RESTART_REASON_ADDR);

	return restart_reason;
}

void target_serialno(unsigned char *buf)
{
	unsigned int serialno;
	if (target_is_emmc_boot()) {
		serialno = mmc_get_psn();
		snprintf((char *)buf, 13, "%x", serialno);
	}
}

/* Do any target specific intialization needed before entering fastboot mode */
void target_fastboot_init(void)
{
}

void target_uart_init(void)
{
	uart_cfg_t *uart;

	get_board_param(board_machtype());

	uart = gboard_param->console_uart_cfg;

	uart_dm_init(uart->base, uart->gsbi_base, uart->uart_dm_base);
}

/* Detect the target type */
void target_detect(struct board_data *board)
{
	/* machid is already determined */
}

/* Detect the modem type */
void target_baseband_detect(struct board_data *board)
{
	board->baseband = -1;
}

unsigned target_baseband()
{
	return -1;
}

/* Returns 1 if target supports continuous splash screen. */
int target_cont_splash_screen()
{
	return 0;
}

/* identify the usb controller to be used for the target */
const char * target_usb_controller()
{
	return "dwc";
}

void setbits_le32(uint32_t addr, uint32_t val)
{
	uint32_t reg = readl(addr);

	writel(reg | val, addr);
}

void clrbits_le32(uint32_t addr, uint32_t val)
{
	uint32_t reg = readl(addr);

	writel(reg & ~(val), addr);
}

void target_usb_phy_init(void)
{
	uint32_t pll_status;
	uint32_t phy_base = USB30_PHY_1_QUSB2PHY_BASE;

	/* Enable QUSB2PHY Power down */
	setbits_le32(phy_base + 0xB4, 0x1);

	/* PHY Config Sequence */
	/* QUSB2PHY_PLL:PLL Feedback Divider Value */
	writel(0x14, phy_base);
	/* QUSB2PHY_PORT_TUNE1: USB Product Application Tuning Register A */
	writel(0xF8, phy_base + 0x80);
	/* QUSB2PHY_PORT_TUNE2: USB Product Application Tuning Register B */
	writel(0xB3, phy_base + 0x84);
	/* QUSB2PHY_PORT_TUNE3: USB Product Application Tuning Register C */
	writel(0x83, phy_base + 0x88);
	/* QUSB2PHY_PORT_TUNE4: USB Product Application Tuning Register D */
	writel(0xC0, phy_base + 0x8C);
	/* QUSB2PHY_PORT_TEST2 */
	writel(0x14, phy_base + 0x9C);
	/* QUSB2PHY_PLL_TUNE: PLL Test Configuration */
	writel(0x30, phy_base + 0x08);
	/* QUSB2PHY_PLL_USER_CTL1: PLL Control Configuration */
	writel(0x79, phy_base + 0x0C);
	/* QUSB2PHY_PLL_USER_CTL2: PLL Control Configuration */
	writel(0x21, phy_base + 0x10);
	/* QUSB2PHY_PORT_TUNE5 */
	writel(0x00, phy_base + 0x90);
	/* QUSB2PHY_PLL_PWR_CTL: PLL Manual SW Programming
	 * and Biasing Power Options */
	writel(0x00, phy_base + 0x18);
	/* QUSB2PHY_PLL_AUTOPGM_CTL1: Auto vs. Manual PLL/Power-mode
	 * programming State Machine Control Options */
	writel(0x9F, phy_base + 0x1C);
	/* QUSB2PHY_PLL_TEST: PLL Test Configuration-Disable diff ended clock */
	writel(0x80, phy_base + 0x04);

	/* Disable QUSB2PHY Power down */
	clrbits_le32(phy_base + 0xB4, 0x1);

	mdelay(10);

	/* QUSB2PHY_PLL_TEST: PLL Test Configuration-Disable diff ended clock */
	writel(0x80, phy_base + 0x04);
	mdelay(10);

	/* Get QUSB2PHY_PLL_STATUS */
	pll_status = readl(phy_base + 0x38) & (1 << 5);
	if (!pll_status)
		dprintf(INFO, "QUSB PHY PLL LOCK failed 0x%08x\n", readl(0x00079038));

	/* HS Only: Section 2.2.1 8.a to 8.f */
	setbits_le32(USB30_1_GENERAL_CFG, PIPE_UTMI_CLK_DIS);
	udelay(100);
	setbits_le32(USB30_1_GENERAL_CFG, PIPE_UTMI_CLK_SEL);
	setbits_le32(USB30_1_GENERAL_CFG, PIPE3_PHYSTATUS_SW);
	udelay(100);
	clrbits_le32(USB30_1_GENERAL_CFG, PIPE_UTMI_CLK_DIS);
}

void target_usb_phy_reset(void)
{
	setbits_le32(GCC_QUSB2_0_PHY_BCR, 0x1);
	setbits_le32(GCC_USB0_PHY_BCR, 0x1);
	mdelay(10);

	clrbits_le32(GCC_USB0_PHY_BCR, 0x1);
	clrbits_le32(GCC_QUSB2_0_PHY_BCR, 0x1);
	mdelay(10);
}

/* Do target specific usb initialization */
void target_usb_init(void)
{
	int ret;
	/* Select USB 2.0 */
	ret = scm_call_atomic2(SCM_SVC_IO_ACCESS,SCM_IO_WRITE,
			TCSR_USB_CONTROLLER_TYPE_SEL, USB_CONT_TYPE_USB_20);
	if (ret) {
		dprintf(CRITICAL, "Failed to select USB controller type as USB2.0, scm call returned error (0x%x)\n", ret);
	}
}

/* Initialize target specific USB handlers */
target_usb_iface_t *target_usb30_init()
{
	target_usb_iface_t *t_usb_iface;

	t_usb_iface = calloc(1, sizeof(target_usb_iface_t));
	ASSERT(t_usb_iface);

	t_usb_iface->phy_reset = target_usb_phy_reset;
	t_usb_iface->phy_init = target_usb_phy_init;
	t_usb_iface->clock_init = clock_usb30_init;
	t_usb_iface->vbus_override = 1;

	return t_usb_iface;
}

void target_mmc_init(unsigned char slot, unsigned int base)
{
	get_board_param(board_machtype());
	ipq_configure_gpio(gboard_param->emmc_gpio,
                gboard_param->emmc_gpio_count);
}

int target_mmc_bus_width()
{
	return MMC_BOOT_BUS_WIDTH_8_BIT;
}

struct crypto_bam_pipes
{
	uint8_t read_pipe;
	uint8_t write_pipe;
	uint8_t read_pipe_grp;
	uint8_t write_pipe_grp;
};

struct crypto_init_params
{
	uint32_t                crypto_base;
	uint32_t                crypto_instance;
	uint32_t                bam_base;
	uint32_t                bam_ee;
	uint32_t                num_ce;
	uint32_t                read_fifo_size;
	uint32_t                write_fifo_size;
	uint8_t                 do_bam_init;
	struct crypto_bam_pipes pipes;
};

extern void crypto_init_params(struct crypto_init_params * params);

/* Set up params for h/w CE. */
static void target_crypto_init_params()
{
	struct crypto_init_params ce_params;

	/* Set up base addresses and instance. */
	ce_params.crypto_instance  = CE1_INSTANCE;
	ce_params.crypto_base      = MSM_CE1_BASE;
	ce_params.bam_base         = MSM_CE1_BAM_BASE;

	/* Set up BAM config. */
	ce_params.bam_ee               = CE_EE;
	ce_params.pipes.read_pipe      = CE_READ_PIPE;
	ce_params.pipes.write_pipe     = CE_WRITE_PIPE;
	ce_params.pipes.read_pipe_grp  = CE_READ_PIPE_LOCK_GRP;
	ce_params.pipes.write_pipe_grp = CE_WRITE_PIPE_LOCK_GRP;

	/* Assign buffer sizes. */
	ce_params.num_ce           = CE_ARRAY_SIZE;
	ce_params.read_fifo_size   = CE_FIFO_SIZE;
	ce_params.write_fifo_size  = CE_FIFO_SIZE;

	/* BAM is initialized by TZ for this platform.
	 * Do not do it again as the initialization address space
	 * is locked.
	 */
	ce_params.do_bam_init      = 0;

	crypto_init_params(&ce_params);
}

#define uarg(x)		(argv[x].u)
#define ullarg(x)	((unsigned long long)argv[x].u)

static int cmd_fuseipq(int argc, const cmd_args *argv)
{
	uint32_t address, ret = -1;

	if (argc != 2) {
		printf("Invalid number of arguments\n");
		printf("Command format: fuseipq <address>\n");
		return 1;
	}

	address = uarg(1);
	ret = fuseipq(address);
	return ret;
}

static int cmd_hash_find(int argc, const cmd_args *argv)
{
	unsigned msg_len, alg, i;
	unsigned char digest_buf[32]={0};
	unsigned char *msg_buf;

	msg_buf = (unsigned char*)uarg(1);
	msg_len = (unsigned)uarg(2);
	alg = (unsigned)uarg(3);

	hash_find(msg_buf, msg_len, digest_buf, alg);
	printf("\n");
	for(i=0; i<32; i++)
	{
		printf("%02x",digest_buf[i]);
	}
	printf("\n");

	return 0;
}

static int cmd_i2c(int argc, const cmd_args *argv)
{
	int err;

	if (strcmp(argv[1].str, "probe")) {
		if (argc < 5) {
			printf("not enough arguments\n");
usage:
			printf("%s read <ddr addr> <i2c address> <register> <reg_len>\n", argv[0].str);
			printf("%s write <ddr addr> <i2c address> <register> <reg_len>\n", argv[0].str);
			return -1;
		}
	}


	if (!strcmp(argv[1].str, "probe")) {
		unsigned int j;
		uint8_t val;
		uint8_t qup_id = QUP_ID_5;

		if (argc > 2) {
			if (argv[2].u == QUP_ID_2 || argv[2].u == QUP_ID_5)
				qup_id = argv[2].u;
			else
				printf("QUP id allowed are %d & %d using default %d\n",
							QUP_ID_2, QUP_ID_5, qup_id);
		}

		if (i2c_qup_initialized != qup_id)
			i2c_ipq6018_init(BLSP_ID_1, qup_id);

		for (j = 0; j < 128; j++) {
			if (i2c_read_reg(j, 0, &val, 1) == 0)
				dprintf(INFO, "I2C slave addr: 0x%x CONNECTED !!!!\n", j);
		}
	} else if (!strcmp(argv[1].str, "read")) {
		unsigned *ptr = (unsigned *)argv[2].u;
		uint8_t i2c_address = argv[3].u;
		uint8_t reg = argv[4].u;
		unsigned int reg_len = argv[5].u;

		uint8_t *buf = NULL;

		memset(ptr, 0x0, reg_len);
		buf = malloc(reg_len);
		if (!buf)
			return -1;

		memset(buf, 0x0, reg_len);

		err = i2c_read_reg(i2c_address, reg, buf, reg_len);
		if (err) {
			dprintf(INFO, "error in read_reg %d\n", err);
		} else {
			memcpy(ptr, buf, reg_len);
		}
		free(buf);
		dprintf(INFO, "\n");
	} else if (!strcmp(argv[1].str, "write")) {
		unsigned *ptr = (unsigned *)argv[2].u;
		uint8_t i2c_address = argv[3].u;
		uint8_t reg = argv[4].u;
		unsigned int reg_len = argv[5].u;
		uint8_t *buf = NULL;

		if (reg_len > 32) {
			dprintf(INFO, "max write supported is 32\n");
			return -1;
		}
		buf = malloc(reg_len);
		if (!buf)
			return -1;

		memcpy(buf, ptr, reg_len);
		i2c_write_reg(i2c_address, reg, buf, reg_len);
		free(buf);
		dprintf(INFO, "\n");
	} else {
		dprintf(INFO, "unrecognized subcommand\n");
		goto usage;
	}

	return 0;
}

STATIC_COMMAND_START
	{ "i2c", "i2c probe/read/write commands", &cmd_i2c },
	{ "hash", "Find Hash", &cmd_hash_find },
	{ "fuseipq", "fuse QFPROM registers from memory\n", &cmd_fuseipq },
STATIC_COMMAND_END(hash);
