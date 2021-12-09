LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += \
			-I$(LOCAL_DIR)/include -I$(LK_TOP_DIR)/dev/panel/msm

DEFINES += $(TARGET_XRES)
DEFINES += $(TARGET_YRES)

OBJS += \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/smem.o \
	$(LOCAL_DIR)/smem_ptable.o \
	$(LOCAL_DIR)/hsusb.o \
	$(LOCAL_DIR)/jtag_hook.o \
	$(LOCAL_DIR)/jtag.o \
	$(LOCAL_DIR)/nand.o \
	$(LOCAL_DIR)/partition_parser.o \
	$(LOCAL_DIR)/crc32.o

ifeq ($(ENABLE_SDHCI_SUPPORT),1)
OBJS += \
	$(LOCAL_DIR)/sdhci.o \
	$(LOCAL_DIR)/sdhci_msm.o \
	$(LOCAL_DIR)/mmc_sdhci.o \
	$(LOCAL_DIR)/mmc_wrapper.o
else
OBJS += \
	$(LOCAL_DIR)/mmc.o
endif

ifeq ($(PLATFORM),msm8x60)
	OBJS += $(LOCAL_DIR)/mipi_dsi.o \
			$(LOCAL_DIR)/i2c_qup.o \
			$(LOCAL_DIR)/uart_dm.o \
			$(LOCAL_DIR)/crypto_eng.o \
			$(LOCAL_DIR)/crypto_hash.o \
			$(LOCAL_DIR)/scm.o \
			$(LOCAL_DIR)/lcdc.o \
			$(LOCAL_DIR)/mddi.o \
			$(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/mdp4.o \
			$(LOCAL_DIR)/certificate.o \
			$(LOCAL_DIR)/image_verify.o \
			$(LOCAL_DIR)/hdmi.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/timer.o
endif

ifeq ($(PLATFORM),msm8960)
	OBJS += $(LOCAL_DIR)/hdmi.o \
			$(LOCAL_DIR)/mipi_dsi.o \
			$(LOCAL_DIR)/i2c_qup.o \
			$(LOCAL_DIR)/uart_dm.o \
			$(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/mdp4.o \
			$(LOCAL_DIR)/crypto4_eng.o \
			$(LOCAL_DIR)/crypto_hash.o \
			$(LOCAL_DIR)/certificate.o \
			$(LOCAL_DIR)/image_verify.o \
			$(LOCAL_DIR)/scm.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/clock-local.o \
			$(LOCAL_DIR)/clock.o \
			$(LOCAL_DIR)/clock_pll.o \
			$(LOCAL_DIR)/board.o \
			$(LOCAL_DIR)/display.o \
			$(LOCAL_DIR)/lvds.o \
			$(LOCAL_DIR)/mipi_dsi_phy.o \
			$(LOCAL_DIR)/timer.o \
			$(LOCAL_DIR)/mdp_lcdc.o
endif

ifeq ($(PLATFORM),ipq6018)
	OBJS +=	$(LOCAL_DIR)/qtimer.o \
			$(LOCAL_DIR)/i2c_qup.o \
			$(LOCAL_DIR)/qtimer_cp15.o \
			$(LOCAL_DIR)/uart_dm.o \
			$(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/crypto5_eng.o \
			$(LOCAL_DIR)/crypto5_wrapper.o \
			$(LOCAL_DIR)/crypto_hash.o \
			$(LOCAL_DIR)/certificate.o \
			$(LOCAL_DIR)/image_verify.o \
			$(LOCAL_DIR)/scm.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/clock-local.o \
			$(LOCAL_DIR)/clock.o \
			$(LOCAL_DIR)/clock_pll.o \
			$(LOCAL_DIR)/board.o \
			$(LOCAL_DIR)/display.o \
			$(LOCAL_DIR)/bam_dma.o \
			$(LOCAL_DIR)/fuse_blower.o
endif

ifeq ($(PLATFORM),ipq807x)
	OBJS +=	$(LOCAL_DIR)/qtimer.o \
			$(LOCAL_DIR)/i2c_qup.o \
			$(LOCAL_DIR)/qtimer_cp15.o \
			$(LOCAL_DIR)/uart_dm.o \
			$(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/crypto5_eng.o \
			$(LOCAL_DIR)/crypto5_wrapper.o \
			$(LOCAL_DIR)/crypto_hash.o \
			$(LOCAL_DIR)/certificate.o \
			$(LOCAL_DIR)/image_verify.o \
			$(LOCAL_DIR)/scm.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/clock-local.o \
			$(LOCAL_DIR)/clock.o \
			$(LOCAL_DIR)/clock_pll.o \
			$(LOCAL_DIR)/board.o \
			$(LOCAL_DIR)/display.o \
			$(LOCAL_DIR)/bam_dma.o \
			$(LOCAL_DIR)/fuse_blower.o
endif

ifeq ($(PLATFORM),ipq40xx)
	OBJS +=	$(LOCAL_DIR)/qtimer.o \
			$(LOCAL_DIR)/qtimer_cp15.o \
			$(LOCAL_DIR)/uart_dm.o \
			$(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/crypto5_eng.o \
			$(LOCAL_DIR)/crypto5_wrapper.o \
			$(LOCAL_DIR)/crypto_hash.o \
			$(LOCAL_DIR)/certificate.o \
			$(LOCAL_DIR)/image_verify.o \
			$(LOCAL_DIR)/scm.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/clock-local.o \
			$(LOCAL_DIR)/clock.o \
			$(LOCAL_DIR)/clock_pll.o \
			$(LOCAL_DIR)/board.o \
			$(LOCAL_DIR)/display.o \
			$(LOCAL_DIR)/bam_dma.o \
			$(LOCAL_DIR)/fuse_blower.o
endif

ifeq ($(PLATFORM),ipq806x)
	OBJS += $(LOCAL_DIR)/uart_dm.o \
			$(LOCAL_DIR)/i2c_qup.o \
			$(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/crypto4_eng.o \
			$(LOCAL_DIR)/crypto_hash.o \
			$(LOCAL_DIR)/certificate.o \
			$(LOCAL_DIR)/image_verify.o \
			$(LOCAL_DIR)/scm.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/clock-local.o \
			$(LOCAL_DIR)/clock.o \
			$(LOCAL_DIR)/clock_pll.o \
			$(LOCAL_DIR)/board.o \
			$(LOCAL_DIR)/display.o \
			$(LOCAL_DIR)/bam.o \
			$(LOCAL_DIR)/timer.o \
			$(LOCAL_DIR)/fuse_blower.o
endif

ifeq ($(PLATFORM),copper)
	OBJS += $(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/qtimer.o \
			$(LOCAL_DIR)/qtimer_mmap.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/clock.o \
			$(LOCAL_DIR)/clock_pll.o \
			$(LOCAL_DIR)/clock_lib2.o \
			$(LOCAL_DIR)/uart_dm.o \
			$(LOCAL_DIR)/board.o \
			$(LOCAL_DIR)/spmi.o \
			$(LOCAL_DIR)/bam.o
endif

ifeq ($(PLATFORM),msm7x27a)
	OBJS += $(LOCAL_DIR)/uart.o \
			$(LOCAL_DIR)/proc_comm.o \
			$(LOCAL_DIR)/mdp3.o \
			$(LOCAL_DIR)/mipi_dsi.o \
			$(LOCAL_DIR)/crypto_eng.o \
			$(LOCAL_DIR)/crypto_hash.o \
			$(LOCAL_DIR)/certificate.o \
			$(LOCAL_DIR)/image_verify.o \
			$(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/timer.o \
			$(LOCAL_DIR)/display.o \
			$(LOCAL_DIR)/mipi_dsi_phy.o \
			$(LOCAL_DIR)/mdp_lcdc.o \
			$(LOCAL_DIR)/spi.o
endif

ifeq ($(PLATFORM),msm7k)
	OBJS += $(LOCAL_DIR)/uart.o \
			$(LOCAL_DIR)/proc_comm.o \
			$(LOCAL_DIR)/lcdc.o \
			$(LOCAL_DIR)/mddi.o \
			$(LOCAL_DIR)/timer.o
endif

ifeq ($(PLATFORM),msm7x30)
	OBJS += $(LOCAL_DIR)/crypto_eng.o \
			$(LOCAL_DIR)/crypto_hash.o \
			$(LOCAL_DIR)/uart.o \
			$(LOCAL_DIR)/proc_comm.o \
			$(LOCAL_DIR)/lcdc.o \
			$(LOCAL_DIR)/mddi.o \
			$(LOCAL_DIR)/certificate.o \
			$(LOCAL_DIR)/image_verify.o \
			$(LOCAL_DIR)/timer.o
endif

ifeq ($(PLATFORM),mdm9x15)
	OBJS += $(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/uart_dm.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/timer.o
endif

ifeq ($(PLATFORM),mdm9x25)
	OBJS += $(LOCAL_DIR)/qgic.o \
			$(LOCAL_DIR)/interrupts.o \
			$(LOCAL_DIR)/qtimer.o \
			$(LOCAL_DIR)/qtimer_mmap.o \
			$(LOCAL_DIR)/board.o
endif

ifeq ($(ENABLE_USB30_SUPPORT),1)
	OBJS += \
		$(LOCAL_DIR)/usb30_dwc.o \
		$(LOCAL_DIR)/usb30_dwc_hw.o \
		$(LOCAL_DIR)/usb30_udc.o \
		$(LOCAL_DIR)/usb30_wrapper.o
endif
