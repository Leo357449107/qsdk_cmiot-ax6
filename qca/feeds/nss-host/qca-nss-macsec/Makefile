include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-nss-macsec
PKG_SOURCE_PROTO:=git
PKG_BRANCH:=master
PKG_RELEASE:=1

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
  PKG_VERSION:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
endif

include $(INCLUDE_DIR)/package.mk

define KernelPackage/qca-nss-macsec
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Devices
  DEPENDS:=@TARGET_ipq806x||TARGET_ipq_ipq806x||TARGET_ipq_ipq807x||TARGET_ipq_ipq807x_64||TARGET_ipq807x||TARGET_ipq807x_64||TARGET_ipq_ipq60xx||TARGET_ipq_ipq60xx_64||TARGET_ipq60xx||TARGET_ipq60xx_64||TARGET_ipq_ipq50xx||TARGET_ipq_ipq50xx_64||TARGET_ipq50xx \
		+libpthread +libc @!LINUX_3_18
  TITLE:=Kernel driver for NSS macsec
  FILES:=$(PKG_BUILD_DIR)/qca-nss-macsec.ko
  AUTOLOAD:=$(call AutoLoad,52,qca-nss-macsec)
endef

define KernelPackage/qca-nss-macsec/Description
This package contains a MACSEC driver for QCA chipset
endef

QCA_NSS_MACSEC_CONFIG_OPTS+= TOOL_PATH=$(TOOLCHAIN_DIR)/bin/ \
                SYS_PATH=$(LINUX_DIR) \
                TOOLPREFIX=$(TARGET_CROSS) \
                KVER=$(LINUX_VERSION) \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
                ARCH=$(LINUX_KARCH)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/qca-nss-macsec
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/include/nss_macsec_types.h $(1)/usr/include/qca-nss-macsec
	$(CP) $(PKG_BUILD_DIR)/include/nss_macsec_secy.h $(1)/usr/include/qca-nss-macsec
	$(CP) $(PKG_BUILD_DIR)/include/nss_macsec_secy_rx.h $(1)/usr/include/qca-nss-macsec
	$(CP) $(PKG_BUILD_DIR)/include/nss_macsec_secy_tx.h $(1)/usr/include/qca-nss-macsec
	$(CP) $(PKG_BUILD_DIR)/libfal.so $(1)/usr/lib
endef

ifeq ($(CONFIG_TARGET_ipq),y)
subtarget:=$(SUBTARGET)
else
subtarget:=$(CONFIG_TARGET_BOARD)
endif

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		M="$(PKG_BUILD_DIR)" \
		SoC="$(subtarget)" \
		modules
	$(MAKE) -C $(PKG_BUILD_DIR) $(strip $(QCA_NSS_MACSEC_CONFIG_OPTS)) -f Makefile.shell
endef

define KernelPackage/qca-nss-macsec/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/*.so $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/macsec_shell $(1)/usr/sbin/
endef

$(eval $(call KernelPackage,qca-nss-macsec))
