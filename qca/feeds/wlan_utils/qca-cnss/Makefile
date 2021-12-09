include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-cnss
PKG_SOURCE_PROTO:=git
PKG_BRANCH:=master
PKG_RELEASE:=1

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
	PKG_VERSION:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
endif

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
	SECTION:=kernel
	CATEGORY:=Kernel modules
	SUBMENU:=Network Devices
	TITLE:=CNSS Platform Driver
	FILES:=$(PKG_BUILD_DIR)/cnss2.ko
endef

define KernelPackage/$(PKG_NAME)/Description
	This package contains CNSS Platform Driver for QTI WLAN Chipsets
endef

define KernelPackage/$(PKG_NAME)/config
	config CNSS2_MODULE
		bool "Support compilation of CNSS2 as DLKM"
		default n
		help
		This option enables compilation of CNSS2 as a Dynamically-loaded Kernel Module
endef

define Build/InstallDev
	mkdir -p $(1)/usr/include/qca-cnss/
	$(CP) $(PKG_BUILD_DIR)/include/cnss2.h $(1)/usr/include/qca-cnss/
endef

CONFIG_BUILD_QSDK=y

define Build/Compile
	export CONFIG_BUILD_QSDK="$(CONFIG_BUILD_QSDK)"; \
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		M="$(PKG_BUILD_DIR)"
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
