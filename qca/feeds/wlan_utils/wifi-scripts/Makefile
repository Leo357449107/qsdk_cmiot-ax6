
include $(TOPDIR)/rules.mk

PKG:=qca-wifi-scripts
PKG_NAME:=$(PKG)
PKG_VERSION:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=QCA
  CATEGORY:=QCA software
  URL:=http://www.qca.qualcomm.com
  MAINTAINER:=Qualcomm Atheros
  TITLE:= QCA wifi scripts for kernel5.4
  DEPENDS:=@TARGET_ipq807x||TARGET_ipq50xx||TARGET_ipq60xx||TARGET_ipq95xx
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib/preinit
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN)  ./files/81_load_wifi_board_bin $(1)/lib/preinit
	$(INSTALL_DATA) ./files/read_caldata_to_fs.sh $(1)/lib
	$(INSTALL_DATA) ./files/wifi_interface_helper.sh $(1)/lib
	$(INSTALL_DATA) ./files/update_smp_affinity.sh $(1)/lib
endef
Build/Compile=true
$(eval $(call BuildPackage,$(PKG_NAME)))

