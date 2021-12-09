#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ifneq ($(DUMP),)


define SOURCE_INFO
$(if $(PKG_BUILD_DEPENDS),Build-Depends: $(PKG_BUILD_DEPENDS)
)$(if $(HOST_BUILD_DEPENDS),Build-Depends/host: $(HOST_BUILD_DEPENDS)
)$(if $(BUILD_TYPES),Build-Types: $(BUILD_TYPES)
)

endef

FindPackage?=$(strip $(shell find $(TOPDIR)/qsdk-package -name $(1) 2>/dev/null))

define Dumpinfo/Package
$(eval -include $(wildcard $(call FindPackage, $(1))/$(1).mk))
$(info $(SOURCE_INFO)Package: $(1)
$(if $(MENU),Menu: $(MENU)
)$(if $(SUBMENU),Submenu: $(SUBMENU)
)$(if $(SUBMENUDEP),Submenu-Depends: $(SUBMENUDEP)
)$(if $(DEFAULT),Default: $(DEFAULT)
)$(if $(findstring $(PREREQ_CHECK),1),Prereq-Check: 1
)Version: $(VERSION)
$(if $(ABI_VERSION),ABIVersion: $(ABI_VERSION)
)Depends: $(call PKG_FIXUP_DEPENDS,$(1),$(DEPENDS))
Conflicts: $(CONFLICTS)
Menu-Depends: $(MDEPENDS)
Provides: $(PROVIDES)
$(if $(VARIANT),Build-Variant: $(VARIANT)
$(if $(DEFAULT_VARIANT),Default-Variant: $(VARIANT)
))Section: $(SECTION)
Category: $(CATEGORY)
$(if $(filter nonshared,$(PKGFLAGS)),,Repository: $(if $(FEED),$(FEED),base)
)Title: $(TITLE)
Maintainer: $(MAINTAINER)
$(if $(USERID),Require-User: $(USERID)
)Source: $(PKG_SOURCE)
$(if $(LICENSE),License: $(LICENSE)
)$(if $(LICENSE_FILES),LicenseFiles: $(LICENSE_FILES)
)Type: $(if $(Package/$(1)/targets),$(Package/$(1)/targets),$(if $(PKG_TARGETS),$(PKG_TARGETS),ipkg))
$(if $(KCONFIG),Kernel-Config: $(KCONFIG)
)$(if $(BUILDONLY),Build-Only: $(BUILDONLY)
)$(if $(HIDDEN),Hidden: $(HIDDEN)
)Description: $(if $(Package/$(1)/description),$(Package/$(1)/description),$(TITLE))
$(if $(URL),$(URL)
)$(MAINTAINER)
@@
$(if $(Package/$(1)/config),Config:
$(Package/$(1)/overrideconfig)
$(Package/$(1)/config)
@@
))
SOURCE_INFO :=
endef

dumpinfo: FORCE
	$(if $(SOURCE_INFO),$(info $(SOURCE_INFO)))

endif
