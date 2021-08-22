
include $(TOPDIR)/rules.mk

PKG_NAME:=openjdk
PKG_VERSION:=1.8.0.161-3

PKG_SOURCE:=java-1.8.0-$(PKG_NAME)-$(PKG_VERSION).b14.el6_9.x86_64.zip
PKG_SOURCE_URL:=https://github.com/ojdkbuild/ojdkbuild/releases/download/1.8.0.161-1/
PKG_MD5SUM:=efd639c0aba726ffec4a17e446d4cc1c

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/java-1.8.0-$(PKG_NAME)-$(PKG_VERSION).b14.el6_9.x86_64
include $(INCLUDE_DIR)/host-build.mk


Host/Compile:=true

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/bin/openjdk-1.8-native
	$(CP) $(HOST_BUILD_DIR)/* $(STAGING_DIR_HOST)/bin/openjdk-1.8-native
endef


$(eval $(call HostBuild))
