include $(TOPDIR)/rules.mk

PKG_NAME:=civetweb
PKG_VERSION:=1.6
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/civetweb/civetweb/releases/tag/v1.6
PKG_MD5SUM:=c9a110edb2f7e35d031ea7c754d2b9af
PKG_BUILD_PARALLEL:=1

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=COPYRIGHT

include $(INCLUDE_DIR)/package.mk

define Package/civetweb
  SECTION:=Network
  TITLE:=civetweb small footprint HTTP Server
endef

define Package/civetweb/description
 Civetweb is a small footprint HTTP Server with optional Lua and SQLite support written in C
endef

define Package/civetweb/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/civetweb $(1)/bin/
endef

$(eval $(call BuildPackage,civetweb))
