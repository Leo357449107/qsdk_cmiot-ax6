#
# Recipe extension for samba4
#

define samba4_libs_append
	DEPENDS:=$$(filter-out +PACKAGE_libpam:libpam,$$(DEPENDS))
	DEPENDS+= +libpam
endef

Package/samba4-libs += $(newline)$(samba4_libs_append)
