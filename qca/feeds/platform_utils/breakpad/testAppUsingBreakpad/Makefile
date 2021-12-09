include $(TOPDIR)/rules.mk

PKG_NAME:=testAppUsingBreakpad
PKG_VERSION:=1.0
PKG_RELEASE:=1

LOCAL_SRC:=./src

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=testAppUsingBreakpad
	CATEGORY:=testAppUsingBreakpad
	TITLE:=Test Application using Breakpad Functionality!
	DEPENDS:=+breakpad-wrapper
endef

define Package/$(PKG_NAME)/description
	Test Application using Breakpad Functionality.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp $(LOCAL_SRC)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

TARGET_CFLAGS += \
	-DINCLUDE_BREAKPAD \
	-g3

TARGET_LDFLAGS += \
	-lbreakpad_qcawrapper

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/testAppUsingBreakpad $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
