include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qrtr
PKG_VERSION:=2019-07-24
PKG_RELEASE:=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://github.com/andersson/qrtr.git
PKG_SOURCE_VERSION:=cd6bedd5d00f211e6c1e3803ff2f9f53c246435e
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=qrtr software
  DEPENDS:=@TARGET_ipq_ipq807x||TARGET_ipq_ipq807x_64||TARGET_ipq807x||TARGET_ipq_ipq60xx||TARGET_ipq_ipq60xx_64||TARGET_ipq60xx||TARGET_ipq_ipq50xx||TARGET_ipq_ipq50xx_64||TARGET_ipq50xx +libpthread
endef

define Package/$(PKG_NAME)/Description
  This package contains a qrtr user space application
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		LD="$(TARGET_CC)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(LINUX_DIR)/user_headers/include -I$(LINUX_DIR)/include/uapi/ -Ilib" \
		all
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/qrtr-cfg $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/qrtr-ns $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/qrtr-lookup $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/qrtr.init $(1)/etc/init.d/qrtr
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
