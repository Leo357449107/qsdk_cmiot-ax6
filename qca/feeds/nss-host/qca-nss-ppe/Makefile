include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-nss-ppe
PKG_SOURCE_PROTO:=git
PKG_BRANCH:=master
PKG_RELEASE:=2

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
  PKG_VERSION:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
endif

PKG_BUILD_DEPENDS :=
MAKE_OPTS:=

include $(INCLUDE_DIR)/package.mk

ifeq ($(CONFIG_TARGET_ipq),y)
subtarget:=$(SUBTARGET)
else
subtarget:=$(CONFIG_TARGET_BOARD)
endif

define KernelPackage/qca-nss-ppe-bridge-mgr
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Devices
  TITLE:=Kernel driver for NSS PPE bridge manager
  DEPENDS:=@TARGET_ipq95xx||TARGET_ipq95xx_32\
		+TARGET_ipq95xx:kmod-qca-nss-ppe-vlan-mgr \
		+PACKAGE_kmod-qca-nss-dp:kmod-qca-nss-dp \
		+kmod-qca-ssdk-nohnat \
		+PACKAGE_kmod-bonding:kmod-bonding
  FILES:=$(PKG_BUILD_DIR)/clients/bridge/qca-nss-ppe-bridge-mgr.ko
  AUTOLOAD:=$(call AutoLoad,51,qca-nss-ppe-bridge-mgr)
endef

define KernelPackage/qca-nss-ppe-bridge-mgr/Description
Kernel modules for NSS PPE bridge manager
endef

define KernelPackage/qca-nss-ppe-vlan-mgr
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Devices
  TITLE:=Kernel driver for NSS PPE vlan manager
  DEPENDS:=@TARGET_ipq95xx||TARGET_ipq95xx_32 \
	   +kmod-qca-ssdk-nohnat \
	   +PACKAGE_kmod-qca-nss-dp:kmod-qca-nss-dp \
	   +PACKAGE_kmod-bonding:kmod-bonding
  FILES:=$(PKG_BUILD_DIR)/clients/vlan/qca-nss-ppe-vlan.ko
  AUTOLOAD:=$(call AutoLoad,51,qca-nss-ppe-vlan)
endef

define KernelPackage/qca-nss-drv-vlan-mgr/Description
Kernel modules for NSS PPE vlan manager
endef

define Build/InstallDev/qca-nss-ppe
	$(INSTALL_DIR) $(1)/usr/include/qca-nss-ppe
	$(CP) $(PKG_BUILD_DIR)/exports/* $(1)/usr/include/qca-nss-ppe/
endef

define Build/InstallDev
	$(call Build/InstallDev/qca-nss-ppe,$(1))
endef

EXTRA_CFLAGS+= \
	-I$(STAGING_DIR)/usr/include/qca-ssdk \
	-I$(STAGING_DIR)/usr/include/qca-ssdk/fal \
	-I$(STAGING_DIR)/usr/include/qca-nss-dp \

ifneq ($(CONFIG_PACKAGE_kmod-qca-nss-ppe-bridge-mgr),)
MAKE_OPTS+=bridge-mgr=y
endif

ifneq ($(CONFIG_PACKAGE_kmod-qca-nss-ppe-vlan-mgr),)
MAKE_OPTS+=vlan-mgr=y
endif

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" $(strip $(MAKE_OPTS)) \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		SoC="$(subtarget)" \
		modules
endef

$(eval $(call KernelPackage,qca-nss-ppe-bridge-mgr))
$(eval $(call KernelPackage,qca-nss-ppe-vlan-mgr))
