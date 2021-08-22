
include $(TOPDIR)/rules.mk

PKG_NAME:=python-future
PKG_VERSION:=0.16.0
PKG_RELEASE:=1

PKG_SOURCE:=v$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/PythonCharmers/python-future/archive/
PKG_MD5SUM:=f90a7fe297c6bd0003fba23b7c5e2957

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk
$(call include_mk, python-package.mk)
PKG_BUILD_DEPENDS:=python/host

define Package/python-future
	SECTION:=lang
	CATEGORY:=Languages
	SUBMENU:=Python
	TITLE:=python-future
	DEPENDS:=+python-light
endef

define Package/python-future/description
future is  is the missing compatibility layer between Python 2 and Python 3.
It allows you to use a single, clean Python 3.x-compatible codebase to support
both Python 2 and Python 3 with minimal overhead.
endef

define Build/Compile
	$(call Build/Compile/PyMod,,install --prefix="/usr" --root="$(PKG_INSTALL_DIR)")
endef

define Host/Install
	$(call Build/Compile/PyMod,,install --prefix="host" --root="$(STAGING_DIR)")
endef

define Build/InstallDev
	$(INSTALL_DIR) $(HOST_PYTHON_LIB_DIR)
	$(CP) $(PKG_INSTALL_DIR)/usr/* $(STAGING_DIR_HOST)/
endef

$(eval $(call HostBuild))
$(eval $(call PyPackage,python-future))
$(eval $(call BuildPackage,python-future))
