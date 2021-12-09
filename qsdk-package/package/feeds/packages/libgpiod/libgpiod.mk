#
# Recipe extension for libgopid
#

define libgpiod_append
	DEPENDS:=$$(filter-out @(LINUX_4_9||LINUX_4_14),$$(DEPENDS))
endef

Package/libgpiod += $(newline)$(libgpiod_append)
