include $(TOPDIR)/rules.mk

PKG_NAME:=tinymembench
PKG_VERSION:=0.4
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/ssvb/tinymembench/archive/refs/tags/
PKG_MD5SUM:=77ef28fd930bc31ffd174dc4254981e4
PKG_BUILD_PARALLEL:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/tinymembench
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=tinymembench
endef

define Package/tinymembench/description
  This is a simple memory benchmark program, which tries to measure the peak
  bandwidth of sequential memory accesses and the latency of random memory
  accesses. Bandwidth is measured by running different assembly code for
  the aligned memory blocks and attempting different prefetch strategies.
endef

define Package/tinymembench/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tinymembench $(1)/usr/sbin/

endef

$(eval $(call BuildPackage,tinymembench))
