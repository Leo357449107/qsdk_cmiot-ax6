
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=accel-pptp
PKG_VERSION:=0.8.5
PKG_RELEASE:=2

PPPD_VERSION:=2.4.7

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://downloads.sourceforge.net/project/accel-pptp/accel-pptp/
PKG_MD5SUM:=061ca3991fb69e0e79cb872b0aaf1d89

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
include $(INCLUDE_DIR)/package.mk

define Package/accel_pptpd
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+ppp-mod-pptp
  TITLE:=Accel pptp server
  URL:=$(PKG_SOURCE_URL)
  SUBMENU:=VPN
endef

CONFIGURE_ARGS += \
	--enable-bcrelay \

TARGET_CFLAGS += \
	-DBUILD_PPTP_DRIVER=FALSE
TARGET_CPPFLAGS+= -fpie
TARGET_LDFLAGS += -pie

CONFIGURE_PATH=pptpd-1.3.3
MAKE_PATH=pptpd-1.3.3
CONFIGURE_ARGS += \
		--target=$(KEXEC_TARGET_NAME) \
		--host=$(REAL_GNU_TARGET_NAME) \
		--build=$(GNU_HOST_NAME) \
		KDIR="$(LINUX_DIR)/user_headers" \
		--exec-prefix="$(PKG_INSTALL_DIR)"

define Package/accel_pptpd/conffiles
/etc/pptpd.conf
/etc/ppp/options.pptpd
endef

define Package/accel_pptpd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/sbin/bcrelay \
		$(PKG_INSTALL_DIR)/usr/sbin/pptpctrl \
		$(PKG_INSTALL_DIR)/usr/sbin/pptpd \
		$(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/usr/lib/pptpd
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/lib/pptpd/* $(1)/usr/lib/pptpd/

	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) ./files/pptpd.conf $(1)/etc/
	$(INSTALL_DIR) $(1)/etc/ppp
	$(INSTALL_DATA) ./files/options.pptpd $(1)/etc/ppp/
endef

$(eval $(call BuildPackage,accel_pptpd))
