include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rmnetctl
PKG_RELEASE:=1

LOCAL_SRC:=$(TOPDIR)/qca/src/data-oss/rmnetctl

include $(INCLUDE_DIR)/local-development.mk
ifeq ($(DUMP)$(PKG_VERSION),)
  PKG_REV:=$(shell git ls-remote $(PKG_SOURCE_URL) $(PKG_BRANCH) | cut -b -7)
  PKG_VERSION:=g$(PKG_REV)
endif
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/rmnetctl
 SECTION:=Dataservices
 CATEGORY:=Data Services Open Source Software
 TITLE:=RMnet Data Driver User space Library
endef

define Package/rmnetctl/Description
 Data Services Open Source Software
endef

TARGET_CFLAGS += -Werror -Wall\
	-I$(LINUX_DIR)/user_headers/include

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/inc/*.h $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/lib/*.so $(1)/usr/lib/
endef

define Package/rmnetctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/lib/librmnetctl.so $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/rmnetcli $(1)/usr/bin
endef

$(eval $(call BuildPackage,rmnetctl))

