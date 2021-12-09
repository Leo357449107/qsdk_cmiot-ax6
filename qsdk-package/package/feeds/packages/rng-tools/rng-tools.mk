#
# Recipe extension for rng-tools
#

RNG_TOOLS_DIR:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
define rng_tools_install_append
	cat $(RNG_TOOLS_DIR)/files/rngd_uci_settings >> $(1)/etc/uci-defaults/rngd
endef

Package/rng-tools/install += $(newline)$(rng_tools_install_append)
