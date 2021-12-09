include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kernel-tests
PKG_SOURCE_PROTO:=git
PKG_BRANCH:=master
PKG_RELEASE:=1

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
  PKG_VERSION:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
endif
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=$(PKG_VERSION)
KERNEL_DIR=$(LINUX_DIR)

include $(INCLUDE_DIR)/package.mk

define Package/kernel-tests
  SECTION:=QTI
  CATEGORY:=$(QTI_SOFTWARE_CATEGORY)
  TITLE:=QTI Linux Kernel tests
  DEPENDS:=@TARGET_ipq807x||TARGET_ipq95xx +libpthread
endef

define Package/kernel-test/Description
  Consists of various apps and scripts for testing various kernel features.
endef

# export KERNEL_FLAGS="ARCH=$(LINUX_KARCH)"
CONFIGURE_CMD = ./configure

CONFIGURE_ARGS += \
	--with-kernel=$(LINUX_DIR) \
	--with-kflags="ARCH=$(LINUX_KARCH)" \
	--disable-adc \
	--disable-abnormal_reset \
	--disable-llcc \
	--disable-llcc_debug \
	--disable-atomic_test \
	--disable-bam_dmux_loopback \
	--disable-bus-timeout \
	--disable-cacheflush \
	--disable-cache_irritators \
	--disable-clk \
	--disable-clocksource \
	--disable-coresight \
	--disable-cpu \
	--disable-cpufreq \
	--disable-dmov \
	--disable-eud \
	--disable-gemnoc \
	--disable-glink \
	--disable-gpio_tlmm \
	--disable-hdcp_test \
	--disable-hrtimer \
	--disable-i2c-msm \
	--disable-ion \
	--disable-ip_accelerator \
	--disable-ipcc \
	--disable-ipc_logging \
	--disable-ipi \
	--disable-irq \
	--disable-kprobes \
	--disable-ktime \
	--disable-ldrex_strex \
	--disable-memory_prof \
	--disable-memory_tracker \
	--disable-modsign \
	--disable-mpp \
	--disable-msm_bus \
	--disable-msm_iommu \
	--disable-msm_uart \
	--disable-msm_uart_hs \
	--disable-mtd \
	--disable-ocmem \
	--disable-page-nom \
	--disable-pan_test \
	--disable-pdr \
	--disable-qcedev \
	--disable-qpic_display \
	--disable-qrng \
	--disable-reg_access \
	--disable-rtc \
	--disable-sdcc \
	--disable-smd \
	--disable-socinfo \
	--disable-spcom \
	--disable-spi \
	--disable-spinlock \
	--disable-ssr \
	--disable-swp \
	--disable-timer \
	--disable-usf \
	--disable-v4l2apps \
	--disable-vfp \
	--disable-vfp-pctest \
	--disable-vreg \
	--disable-watchdog

define Build/InstallDev
	mkdir -p $(1)/data/kernel-tests/modules
	$(INSTALL_DIR) $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/arm/arm_float $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/arm/arm_neon $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/arm/*.sh $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uart/*.sh $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uart/serial_opts $(1)/data/kernel-tests

endef

define Package/kernel-tests/install
	mkdir -p $(1)/data/kernel-tests/modules
	$(INSTALL_DIR) $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/arm/arm_float $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/arm/arm_neon $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/arm/*.sh $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uart/*.sh $(1)/data/kernel-tests
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uart/serial_opts $(1)/data/kernel-tests

endef

$(eval $(call BuildPackage,kernel-tests))
