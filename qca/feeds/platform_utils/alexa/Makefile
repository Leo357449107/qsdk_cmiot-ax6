include $(TOPDIR)/rules.mk

PKG_NAME:=avs-device-sdk
PKG_VERSION:=1.13
PKG_RELEASE:=0
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).$(PKG_RELEASE).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION).$(PKG_RELEASE)
PKG_SOURCE_URL:=https://github.com/alexa/avs-device-sdk.git
PKG_SOURCE_BRANCH:=github/master
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=9549185687fd963d0069a636807e0aee40023829

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)
CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/avs-device-sdk
  SECTION:=ALEXA
  CATEGORY:=ALEXA software
  TITLE:=Alexa
  URL:=
  DEPENDS:=+libcurl +glib2 +libgst1app +gst1-plugins-base +libgstreamer1 +libstdcpp +libsqlite3 +libopenssl
endef

TARGET_LDFLAGS += -lcurl -lsqlite3

CMAKE_OPTIONS += \
	-DCMAKE_BUILD_TYPE=Debug \
	-DACSDK_DEBUG_LOG_ENABLED=ON \
	-DA4B=ON \
	-DACSDK_EXCLUDE_TEST_FROM_ALL=ON \
	-DGSTREAMER_MEDIA_PLAYER=ON


define Package/avs-device-sdk/description
 Alexa voice assistance for AVS.
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/* \
		$(1)/usr/include/
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/*.so* \
		$(1)/usr/lib/
endef
define Package/avs-device-sdk/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/*.so* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
