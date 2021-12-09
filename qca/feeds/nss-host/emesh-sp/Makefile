include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=emesh-sp
PKG_RELEASE:=1

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
  PKG_VERSION:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
endif

include $(INCLUDE_DIR)/package.mk

define KernelPackage/emesh-sp
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Support
  URL:=http://www.qualcomm.com
  MAINTAINER:=Qualcomm Technologies, Inc.
  TITLE:=QTI Service Prioritization Support
  KCONFIG:=CONFIG_NETFILTER=y CONFIG_BRIDGE_NETFILTER=y
  FILES:=$(PKG_BUILD_DIR)/emesh-sp.ko
  AUTOLOAD:=$(call AutoLoad,50,emesh-sp)
endef

define KernelPackage/emesh-sp/description
 This is the kernel module for Service Prioritization in EasyMesh
endef

EMESH_SP_HEADERS= \
            $(PKG_BUILD_DIR)/sp_api.h

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/emesh-sp
	$(foreach header_file,$(EMESH_SP_HEADERS), $(CP) $(header_file) $(1)/usr/include/emesh-sp;)
endef

define Build/Compile
        $(MAKE) -C "$(LINUX_DIR)" \
                CROSS_COMPILE="$(TARGET_CROSS)" \
                ARCH="$(LINUX_KARCH)" \
                M="$(PKG_BUILD_DIR)" \
                modules
endef

$(eval $(call KernelPackage,emesh-sp))
