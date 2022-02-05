include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qdl
PKG_VERSION:=1.0
PKG_RELEASE:=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=http://source.codeaurora.org/quic/qsdk/oss/tools/qdl
PKG_SOURCE_VERSION:=0fcf944ab5b8903249d888230ef2e68e83286256
PKG_SOURCE_BRANCH:=qdl/scorching/master

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_MD5SUM:=skip

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=qdl software
  DEPENDS:=@TARGET_ipq95xx||TARGET_ipq_ipq807x||TARGET_ipq_ipq807x_64||TARGET_ipq807x +libudev-fbsd +libxml2
endef

define Package/$(PKG_NAME)/Description
  This package contains a qdl user space application
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/qdl $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
