include $(TOPDIR)/rules.mk

PKG_NAME:=upx
PKG_VERSION:=3.96-amd64_linux

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://source.codeaurora.org/mirrored_source/quic/qsdk/
PKG_MD5SUM:=78a9650320a850868fc46cbb59a9150a

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)/$(PKG_NAME)-$(PKG_VERSION)
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=UPX Compression Tool
endef

define Packge/$(PKG_NAME)/description
  UPX Compression Tool
endef

Build/Compile=true
Host/Compile=true

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/bin
	$(CP) $(BUILD_DIR_HOST)/$(PKG_NAME)/$(PKG_NAME)-$(PKG_VERSION)/upx $(STAGING_DIR_HOST)/bin/
endef

define Host/Clean
	rm -rf $(STAGING_DIR_HOST)/bin/upx
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call HostBuild))
