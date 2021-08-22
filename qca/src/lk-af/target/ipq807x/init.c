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
#include <kernel/mutex.h>
#include <malloc.h>

#define APPS_DLOAD_MAGIC			0x10
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

extern struct mmc_card *get_mmc_card();

extern void dmb(void);
static struct qup_i2c_dev *dev = NULL;
static int i2c_qup_initialized;

#define TZ_INFO_GET_DIAG_ID	0x2
#define SCM_SVC_INFO		0x6
#define TZ_HK			BIT(2)
#define TZBSP_DIAG_BUF_LEN	0x2000

struct tzbsp_log_pos_t {
        uint16_t wrap;          /* Ring buffer wrap-around ctr */
        uint16_t offset;        /* Ring buffer current position */
};

struct tzbsp_diag_log_t {
        struct tzbsp_log_pos_t log_pos; /* Ring buffer position mgmt */
        uint8_t log_buf[1];             /* Open ended array to the end
                                         * of the 4K IMEM buffer
                                         */
};

/* Below structure to support AARCH64 TZ */
struct ipq807x_tzbsp_diag_t_v8 {
        uint32_t unused[7];     /* Unused variable is to support the
                                 * corresponding structure in trustzone
                                 * and size is varying based on AARCH64 TZ
                                 */
        uint32_t ring_off;
        uint32_t unused1[571];
        struct tzbsp_diag_log_t log;
};

struct tz_log_struct {
	char *ker_buf;
	char *copy_buf;
	int buf_len;
	int copy_len;
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


void i2c_ipq807x_init(uint8_t blsp_id, uint8_t qup_id)
{
	dev = qup_blsp_i2c_init(blsp_id, qup_id, 400000, 19200000);
	if(!dev) {
		dprintf(INFO, "qup_blsp_i2c_init() failed\n");
		return;
	}
	i2c_qup_initialized = 1;
}

void crashdump_init(void)
{
	int ret;
	ret = qca_scm_call_write(SCM_SVC_IO_ACCESS, SCM_IO_WRITE,
				(uint32_t *)0x193D100, APPS_DLOAD_MAGIC);
	if (ret)
		dprintf(CRITICAL, "Error setting crashdump magic\n");
	else
		dprintf(INFO, "Crashdump MAGIC set\n");
}

int display_tzlog(void)
{
	char *ker_buf;
	char *tmp_buf;
	uint32_t buf_len, copy_len = 0;
	uint16_t wrap, ring, offset;
	struct tzbsp_diag_log_t *log;
	struct ipq807x_tzbsp_diag_t_v8 *ipq807x_diag_buf;
	int ret;

	buf_len = TZBSP_DIAG_BUF_LEN;
	ker_buf = malloc(buf_len);
	if (!ker_buf) {
		dprintf(CRITICAL, "malloc failed for ker_buf\n");
		goto err_ker_buf;
	}

	tmp_buf = malloc(buf_len);
	if (!tmp_buf) {
		dprintf(CRITICAL, "malloc failed for tmp_buf\n");
		goto err_tmp_buf;
	}
	memset((void *)tmp_buf, 0, (size_t)buf_len);
	memset((void *)ker_buf, 0, (size_t)buf_len);


	/* SCM call to TZ to get the tz log */
	ret = qca_scm_tz_log(SCM_SVC_INFO, TZ_INFO_GET_DIAG_ID,
					ker_buf, buf_len);
	if (ret != 0) {
		dprintf(CRITICAL, "Error in getting tz log\n");
		goto err_scm;
	}

	ipq807x_diag_buf = (struct ipq807x_tzbsp_diag_t_v8 *)ker_buf;
	ring = ipq807x_diag_buf->ring_off;
	log = &ipq807x_diag_buf->log;

	offset = log->log_pos.offset;
	wrap = log->log_pos.wrap;

	if (wrap != 0) {
		memcpy(tmp_buf, (ker_buf + offset + ring),
			(buf_len - offset - ring));
		memcpy(tmp_buf + (buf_len - offset - ring),
				(ker_buf + ring), offset);
		copy_len = (buf_len - offset - ring) + offset;
	}
	else {
		memcpy(tmp_buf, (ker_buf + ring), offset);
		copy_len = offset;
	}

	/* display buffer to console*/
	dprintf(INFO, "TZ LOG:\n\n");
	for (uint32_t i = 0; i < copy_len; i++) {
		printf("%c", (char)*tmp_buf);
		tmp_buf += 1;
	}
	printf("\n");

err_scm:
	free(tmp_buf);
err_tmp_buf:
	free(ker_buf);
err_ker_buf:
	return 0;
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

static void reset_crashdump(void)
{
	if (is_scm_armv8())
		qca_scm_sdi_v8(SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID);
}

void reboot_device(unsigned reboot_reason)
{
	writel(reboot_reason, RESTART_REASON_ADDR);

	if (reboot_reason == CDUMP_MODE) {
		dprintf(INFO, "CDUMP mode enabled\n");
	}
	else {
		int ret;
		uint32_t reset_type = 0x0;

		ret = qca_scm_call_write(SCM_SVC_IO_ACCESS, SCM_IO_WRITE,
					 (uint32_t *)0x193D100, CLEAR_MAGIC);
		if (ret) {
			dprintf(CRITICAL, "Error resetting crashdump magic\n");
			return;
		}

		reset_crashdump();
		ret = qca_scm_set_resettype(reset_type);
		if (ret) {
			dprintf(CRITICAL, "Error setting reset type\n");
			return;
		}

	}

	writel(1, MSM_WDT0_RST);
	writel(0, MSM_WDT0_EN);
	/* Set BITE_TIME to be 128ms*/
	writel(0x1000, MSM_WDT0_BITE_TIME);
	writel(1, MSM_WDT0_EN);
	dmb();
	mdelay(1000);

	dprintf(CRITICAL, "Rebooting failed\n");
}

int apps_iscrashed(void)
{
	uint32_t dmagic = readl(0x193D100);

	if (dmagic == APPS_DLOAD_MAGIC)
		return 1;

	return 0;
}

unsigned check_reboot_mode(void)
{
	unsigned restart_reason = 0;
	int ret;

	/*
	 * The kernel did not shutdown properly in the previous boot.
	 * The SBLs would not have loaded QSEE firmware, proceeding with
	 * the boot is not possible. Reboot the system cleanly.
	 */
	if(apps_iscrashed()) {
		ret = qca_scm_call_write(SCM_SVC_IO_ACCESS, SCM_IO_WRITE,
					 (uint32_t *)0x193D100, CLEAR_MAGIC);
		if (ret) {
			dprintf(CRITICAL, "Error resetting crashdump magic\n");
			return ret;
		}
		dprintf(INFO, "Apps Dload Magic set. Rebooting...\n");
		reboot_device(0);
	}
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

	/* Config user control register */
	writel(0x0c80c010, USB30_1_GUCTL);

	/* Enable USB2 PHY Power down */
	setbits_le32(phy_base+0xB4, 0x1);

	/* PHY Config Sequence */
	setbits_le32(phy_base+0x80, 0xF8);
	setbits_le32(phy_base+0x84, 0x83);
	setbits_le32(phy_base+0x88, 0x83);
	setbits_le32(phy_base+0x8C, 0xC0);
	setbits_le32(phy_base+0x9C, 0x14);
	setbits_le32(phy_base+0x08, 0x30);
	setbits_le32(phy_base+0x0C, 0x79);
	setbits_le32(phy_base+0x10, 0x21);
	setbits_le32(phy_base+0x90, 0x00);
	setbits_le32(phy_base+0x18, 0x00);
	setbits_le32(phy_base+0x1C, 0x9F);
	setbits_le32(phy_base+0x04, 0x80);

	/* Disable USB2 PHY Power down */
	clrbits_le32(phy_base+0xB4, 0x1);

	mdelay(10);

	setbits_le32(0x00079004, 0x80);
	mdelay(10);

	/* Get QUSB2PHY_PLL_STATUS */
	pll_status = readl(0x00079038) & (1 << 5);
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

		if (!i2c_qup_initialized)
			i2c_ipq807x_init(BLSP_ID_1, QUP_ID_1);

		for (j = 0; j < 128; j++) {
			if (i2c_read_reg(j, 0, &val, 1) == 0)
				printf(" %02X", j);
		}
		dprintf(INFO, "\n");
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
STATIC_COMMAND_END(hash);
