include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=wigig-mac80211
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/wigig-mac80211
  SECTION:=QCA
  CATEGORY:=Network
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros
  TITLE:=mac80211 - wireless configuration scripts
endef

define Package/wigig-mac80211/description
  mac80211-scripts is the Linux wireless LAN (802.11) configuration scripts.
endef

Build/Compile=true

define Package/wigig-mac80211/install
	$(INSTALL_DIR) $(1)/lib/wifi
	$(INSTALL_DIR) $(1)/lib/wifi $(1)/lib/netifd/wireless
	$(INSTALL_DATA) ./files/lib/wifi/mac80211.sh $(1)/lib/wifi
	$(INSTALL_BIN) ./files/lib/netifd/wireless/mac80211.sh $(1)/lib/netifd/wireless
endef

$(eval $(call BuildPackage,wigig-mac80211))
