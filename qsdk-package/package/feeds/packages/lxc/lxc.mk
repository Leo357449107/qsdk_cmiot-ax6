#
# Makefile extension for lxc
#

LXC_DIR:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))

define lxc-templates_install_append
	$(INSTALL_DIR) $(1)/usr/share/lxc/templates/
	$(CP) $(LXC_DIR)/files/lxc-qsdk \
		$(1)/usr/share/lxc/templates/lxc-qsdk
endef

define Package/lxc-auto/install
	$(INSTALL_DIR) $(1)/etc/config $(1)/etc/init.d
	$(INSTALL_CONF) $(LXC_DIR)/files/lxc-auto.config $(1)/etc/config/lxc-auto
	$(INSTALL_BIN) ./files/lxc-auto.init $(1)/etc/init.d/lxc-auto
endef

Package/lxc-templates/install += $(newline)$(lxc-templates_install_append)
