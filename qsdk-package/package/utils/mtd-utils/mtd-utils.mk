# Recipe extension for package : mtd-utils

define nand-utils_append
   DEPENDS+=+zlib
endef

define nand-utils_install_append
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/mkfs.jffs2 $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/flash_eraseall $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/flash_erase $(1)/usr/sbin/
endef

Package/nand-utils += $(newline)$(nand-utils_append)
Package/nand-utils/install += $(newline)$(nand-utils_install_append)
