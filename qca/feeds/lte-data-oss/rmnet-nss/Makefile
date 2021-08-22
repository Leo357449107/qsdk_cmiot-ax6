include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rmnet-nss
PKG_RELEASE:=1

LOCAL_SRC:=$(TOPDIR)/qca/src/data-kernel/drivers/rmnet-nss

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
  PKG_REV:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
  PKG_VERSION:=g$(PKG_REV)
endif

include $(INCLUDE_DIR)/package.mk

define KernelPackage/rmnet-nss
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Support
  DEPENDS:=@TARGET_ipq806x||TARGET_ipq807x||TARGET_ipq_ipq807x||TARGET_ipq_ipq807x_64||TARGET_ipq_ipq50xx||TARGET_ipq_ipq50xx_64 +kmod-qca-nss-drv +TARGET_ipq807x:kmod-rmnet-core
  TITLE:=RMNET NSS interface module
  FILES:=$(PKG_BUILD_DIR)/rmnet_nss.ko
  KCONFIG:=CONFIG_RMNET_DATA=y
  AUTOLOAD:=$(call AutoLoad,42,rmnet-nss)
endef

define KernelPackage/rmnet-nss/Description
RMNET-NSS is the modular interface from the RMNET driver to the NSS driver.
endef


define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="-I$(STAGING_DIR)/usr/include/qca-nss-drv \
			     $(EXTRA_CFLAGS)" \
		modules
endef

$(eval $(call KernelPackage,rmnet-nss))
