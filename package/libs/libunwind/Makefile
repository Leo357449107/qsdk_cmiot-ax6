include $(TOPDIR)/rules.mk

PKG_NAME:=libunwind
PKG_VERSION:=1.2.1
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://download.savannah.gnu.org/releases/libunwind
PKG_MD5SUM:=06ba9e60d92fd6f55cd9dadb084df19e
PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

TARGET_CFLAGS += $(FPIC)

include $(INCLUDE_DIR)/package.mk

define Package/libunwind
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=portable, efficient C(API) to determine call-chain
  DEPENDS:=+liblzma
endef

define Package/libunwind-dev/description
  Its goal is  portable & efficient C(API) to determine call-chain of a program
 The API supports both local (same-process) & remote (across-process) operation
endef

CONFIGURE_ARGS += \
	--enable-shared

TARGET_LDFLAGS += \
	-L$(STAGING_DIR)/usr/lib \
	-llzma

define Build/InstallDev
	$(CP) $(PKG_INSTALL_DIR)/* $(1)/
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
endef

define Package/libunwind/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/libunwind*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libunwind))
