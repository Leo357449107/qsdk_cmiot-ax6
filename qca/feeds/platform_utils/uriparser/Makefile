include $(TOPDIR)/rules.mk

PKG_NAME:=uriparser
PKG_VERSION:=0.9
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).$(PKG_RELEASE).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION).$(PKG_RELEASE)
PKG_SOURCE_URL:=https://github.com/uriparser/uriparser.git
PKG_SOURCE_BRANCH:=github/master
PKG_HASH:=6cef39d6eaf1a48504ee0264ce85f078758057dafb1edd0a898183b55ff76014

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += \
	-DURIPARSER_BUILD_DOCS:BOOL=OFF \
	-DURIPARSER_BUILD_TESTS:BOOL=OFF

define Package/uriparser/Default
  URL:=https://github.com/uriparser/uriparser.git
endef

define Package/liburiparser
  $(call Package/uriparser/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=uriparser library
endef

define Package/uriparser
  $(call Package/uriparser/Default)
  DEPENDS:=+liburiparser
  SECTION:=utils
  TITLE:=Uriparser
endef

define Package/uriparser/description
 Uriparser is a URI parsing and handling library and it supports Unicode.
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/liburiparser.so* $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/liburiparser.pc $(1)/usr/lib/pkgconfig
endef

define Package/liburiparser/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/liburiparser.so* $(1)/usr/lib/
endef

define Package/uriparser/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/uriparse $(1)/usr/bin
endef

$(eval $(call BuildPackage,liburiparser))
$(eval $(call BuildPackage,$(PKG_NAME)))
