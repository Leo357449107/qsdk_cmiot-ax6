#
# Copyright (C) 2009-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=squashfs4
PKG_VERSION:=4.2

PKG_SOURCE:=squashfs$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/squashfs
PKG_MD5SUM:=1b7a781fb4cf8938842279bd3e8ee852
PKG_CAT:=zcat

PKG_BUILD_DIR:=$(BUILD_DIR)/squashfs$(PKG_VERSION)
include $(INCLUDE_DIR)/package.mk


define Package/squashfs4
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=squashfs tools
  DEPENDS:=+liblzma +zlib
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/squashfs-tools \
		CC="$(TARGET_CC)" \
		XZ_SUPPORT=1 \
		LZMA_XZ_SUPPORT=1 \
		XATTR_SUPPORT= \
		EXTRA_CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)" \
		EXTRA_LDFLAGS="$(TARGET_LDFLAGS)" \
		mksquashfs unsquashfs
endef

define Package/squashfs4/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/squashfs-tools/mksquashfs $(1)/usr/sbin/mksquashfs4
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/squashfs-tools/unsquashfs $(1)/usr/sbin/unsquashfs4
endef

$(eval $(call BuildPackage,squashfs4))
