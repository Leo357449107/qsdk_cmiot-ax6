include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=nssinfo
PKG_RELEASE:=1
NSSINFO_BUILD_DIR:=$(TOPDIR)/qca/src/nss-userspace-oss

include $(INCLUDE_DIR)/package.mk

define Package/nssinfo
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Userspace utility for fetching stats from NSS
  DEPENDS:=+libpthread +libnl +kmod-qca-nss-drv-netlink +libncurses +glib2
endef

define Package/nssinfo/description
 A userspace utility for fetching stats from NSS.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/nssinfo
	mkdir -p $(PKG_BUILD_DIR)/lib
	$(CP) $(NSSINFO_BUILD_DIR)/nssinfo/*  $(PKG_BUILD_DIR)/nssinfo/
	$(CP) $(NSSINFO_BUILD_DIR)/lib/*  $(PKG_BUILD_DIR)/lib/
endef

TOOL_CFLAGS:= -I$(STAGING_DIR)/usr/include/qca-nss-clients \
	      -I$(STAGING_DIR)/usr/include/qca-nss-drv \
	      -I$(STAGING_DIR)/usr/include/libnl3 \
	      -I$(STAGING_DIR)/usr/include/nssinfo

TOOL_LDFLAGS:= -L$(STAGING_DIR)/lib

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/lib \
		CC="$(TARGET_CC)" \
		AR="$(TARGET_AR) " \
		CFLAGS="$(TOOL_CFLAGS)" \
		LD_LIBRARY_PATH="$(TOOL_LDFLAGS)"

	$(MAKE) -C $(PKG_BUILD_DIR)/nssinfo \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TOOL_CFLAGS)" \
		LD_LIBRARY_PATH="$(TOOL_LDFLAGS)"
endef

define Package/nssinfo/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/lib/obj/libnl-nss.so $(1)/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/nssinfo/obj/nssinfo $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,nssinfo))
