# Recipe extension for package : busybox

BUSYBOX_DIR:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))

PKG_BUILD_DEPENDS+=opkg/host

define busybox_config_append
	source "$(BUSYBOX_DIR)/Config.in"
endef

define busybox_install_append
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(BUSYBOX_DIR)/files/inetd $(1)/etc/init.d/inetd
ifneq ($(CONFIG_BUSYBOX_CONFIG_FEATURE_USE_INITTAB),)
	mkdir -p $(1)/etc/init.d
	$(CP) $(BUSYBOX_DIR)/files/etc/inittab $(1)/etc/inittab
	$(CP) $(BUSYBOX_DIR)/files/etc/init.d/rcS $(1)/etc/init.d/rcS
endif
endef

Package/busybox/install += $(newline)$(busybox_install_append)
Package/busybox/overrideconfig = $(newline)$(busybox_config_append)
