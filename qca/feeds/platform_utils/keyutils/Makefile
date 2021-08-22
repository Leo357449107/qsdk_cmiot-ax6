
include $(TOPDIR)/rules.mk

PKG_NAME:=keyutils
PKG_VERSION:=1.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://people.redhat.com/~dhowells/keyutils/
PKG_MD5SUM:=0d4c59f3567619a5cff0abbd23c784f4

include $(INCLUDE_DIR)/package.mk

define Package/keyutils
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=keyutils
endef

define Package/keyutils/description
  Keyutils is a set of utilities for managing the key retention facility
  in the kernel, which can be used by filesystems, block devices and more
  to gain and retain the authorization and encryption keys required to perform
  secure operations.
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		LD="$(TARGET_CC)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		all
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/libkeyutils* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/keyutils.h $(1)/usr/include/
endef

define Package/keyutils/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libkeyutils* $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/keyctl $(1)/usr/bin

endef

$(eval $(call BuildPackage,keyutils))
