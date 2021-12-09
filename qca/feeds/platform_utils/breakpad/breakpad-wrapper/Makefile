include $(TOPDIR)/rules.mk

PKG_NAME:=breakpad-wrapper
PKG_VERSION:=1.0
PKG_RELEASE:=1

LOCAL_SRC:=./src
PKG_BUILD_DEPENDS:=breakpad

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=breakpad-wrapper
	CATEGORY:=breakpad-wrapper
	TITLE:=Breakpad Wrapper for QSDK Platform.
	DEPENDS:= +breakpad +libstdcpp
endef

define Package/$(PKG_NAME)/description
	Breakpad Wrapper Shared Library for QSDK.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp $(LOCAL_SRC)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include/ \
	-I$(STAGING_DIR)/usr/include/breakpad \
	-I$(TOOLCHAIN_DIR)/arm-openwrt-linux-muslgnueabi/include/c++/5.2.0 \
	-I$(TOOLCHAIN_DIR)/arm-openwrt-linux-muslgnueabi/include/c++/5.2.0/arm-openwrt-linux-muslgnueabi \
	-g3

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/breakpad_qcawrapper.h $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/libbreakpad_qcawrapper.so $(STAGING_DIR)/usr/lib
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libbreakpad_qcawrapper.so $(1)/usr/lib

	$(INSTALL_DIR) $(1)/usr/bin/breakpad
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/breakpad_reporter $(1)/usr/bin/breakpad

	$(INSTALL_DIR) $(1)/etc/hotplug.d/dump
	$(INSTALL_BIN) ./files/breakpad_crash_reporter $(1)/etc/hotplug.d/dump/10-breakpad-dump

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/breakpad_reporter_init $(1)/etc/init.d/breakpad

endef

$(eval $(call BuildPackage,$(PKG_NAME)))
