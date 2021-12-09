LIBNL_DIR:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))

define libnl_dev_append
	$(CP) $(PKG_BUILD_DIR)/include/* $(1)/usr/include/libnl3/
endef

Build/InstallDev += $(newline)$(libnl_dev_append)
