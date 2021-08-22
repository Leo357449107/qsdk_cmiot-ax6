include $(TOPDIR)/rules.mk

PKG_NAME:=jsoncpp
PKG_VERSION:=1.7.7
PKG_RELEASE:=0

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).zip
PKG_SOURCE_URL:=https://github.com/open-source-parsers/jsoncpp/archive/
PKG_HASH:=e739bf5efa9d180c4c9ed0b2486d8ce1932fa9bb32676a886b5403c2f33704ae

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=COPYING

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += -DJSONCPP_WITH_TESTS=OFF -DBUILD_STATIC_LIBS=ON \
		-DBUILD_SHARED_LIBS=ON

define Package/libjsoncpp
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=javascript object notation cpp
  DEPENDS:=+libstdcpp
endef

define Package/libjsoncpp/description
 This package contains a library for javascript object notation for C++.
 This provides library for reading and writing JSON for C++
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/jsoncpp
	$(CP) $(PKG_INSTALL_DIR)/usr/include/json $(1)/usr/include/jsoncpp/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libjsoncpp.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/jsoncpp.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libjsoncpp/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libjsoncpp.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libjsoncpp))
