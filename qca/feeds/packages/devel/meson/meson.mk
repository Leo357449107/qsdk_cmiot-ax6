# To build your package using meson:
#
# PKG_BUILD_DEPENDS:=meson/host
# include ../../devel/meson/meson.mk
# MESON_ARGS+=-Dfoo -Dbar=baz
#
# To pass additional environment variables to meson:
#
# MESON_VARS+=FOO=bar
#
# Default configure/compile/install targets are provided, but can be
# overwritten if required:
#
# define Build/Configure
#   $(call Build/Configure/Meson)
#   ...
# endef
#
# same for Build/Compile and Build/Install
#
# Host packages are built in the same fashion, just use these vars instead:
#
# HOST_BUILD_DEPENDS:=meson/host
# MESON_HOST_ARGS+=-Dfoo -Dbar=baz
# MESON_HOST_VARS+=FOO=bar

include $(dir $(lastword $(MAKEFILE_LIST)))/../../devel/ninja/ninja.mk

MESON_DIR:=$(STAGING_DIR_HOSTPKG)/lib/meson

MESON_HOST_BUILD_DIR:=$(HOST_BUILD_DIR)/openwrt-build
MESON_HOST_VARS:=
MESON_HOST_ARGS:=

MESON_BUILD_DIR:=$(PKG_BUILD_DIR)/openwrt-build
MESON_VARS:=
MESON_ARGS:=

define Meson
	$(2) $(HOST_PYTHON3_BIN) $(MESON_DIR)/meson.py $(1)
endef

define Meson/CreateNativeFile
	$(STAGING_DIR_HOST)/bin/sed \
		-e "s|@CC@|$(foreach BIN,$(HOSTCC),'$(BIN)',)|" \
		-e "s|@CXX@|$(foreach BIN,$(HOSTCXX),'$(BIN)',)|" \
		-e "s|@PKGCONFIG@|$(PKG_CONFIG)|" \
		-e "s|@CFLAGS@|$(foreach FLAG,$(HOST_CFLAGS) $(HOST_CPPFLAGS),'$(FLAG)',)|" \
		-e "s|@CXXFLAGS@|$(foreach FLAG,$(HOST_CXXFLAGS) $(HOST_CPPFLAGS),'$(FLAG)',)|" \
		-e "s|@LDFLAGS@|$(foreach FLAG,$(HOST_LDFLAGS),'$(FLAG)',)|" \
		-e "s|@PREFIX@|$(STAGING_DIR_HOSTPKG)|" \
		< $(MESON_DIR)/openwrt-native.txt.in \
		> $(1)
endef

define Meson/CreateCrossFile
	$(STAGING_DIR_HOST)/bin/sed \
		-e "s|@CC@|$(foreach BIN,$(TARGET_CC),'$(BIN)',)|" \
		-e "s|@CXX@|$(foreach BIN,$(TARGET_CXX),'$(BIN)',)|" \
		-e "s|@AR@|$(TARGET_AR)|" \
		-e "s|@STRIP@|$(TARGET_CROSS)strip|" \
		-e "s|@NM@|$(TARGET_NM)|" \
		-e "s|@LD@|$(TARGET_CROSS)ld|" \
		-e "s|@PKGCONFIG@|$(PKG_CONFIG)|" \
		-e "s|@CFLAGS@|$(foreach FLAG,$(TARGET_CFLAGS) $(EXTRA_CFLAGS) $(TARGET_CPPFLAGS) $(EXTRA_CPPFLAGS),'$(FLAG)',)|" \
		-e "s|@CXXFLAGS@|$(foreach FLAG,$(TARGET_CXXFLAGS) $(EXTRA_CXXFLAGS) $(TARGET_CPPFLAGS) $(EXTRA_CPPFLAGS),'$(FLAG)',)|" \
		-e "s|@LDFLAGS@|$(foreach FLAG,$(TARGET_LDFLAGS) $(EXTRA_LDFLAGS),'$(FLAG)',)|" \
		-e "s|@ARCH@|$(ARCH)|" \
		-e "s|@CPU@|$(CONFIG_TARGET_SUBTARGET)|" \
		-e "s|@ENDIAN@|$(if $(CONFIG_BIG_ENDIAN),big,little)|" \
		< $(MESON_DIR)/openwrt-cross.txt.in \
		> $(1)
endef

define Host/Configure/Meson
	$(call Meson/CreateNativeFile,$(HOST_BUILD_DIR)/openwrt-native.txt)
	$(call Meson, \
		--native-file $(HOST_BUILD_DIR)/openwrt-native.txt \
		$(MESON_HOST_ARGS) \
		$(MESON_HOST_BUILD_DIR) \
		$(HOST_BUILD_DIR), \
		$(MESON_HOST_VARS))
endef

define Host/Compile/Meson
	$(call Ninja,-C $(MESON_HOST_BUILD_DIR),)
endef

define Host/Install/Meson
	$(call Ninja,-C $(MESON_HOST_BUILD_DIR) install,)
endef

define Host/Uninstall/Meson
	-$(call Ninja,-C $(MESON_HOST_BUILD_DIR) uninstall,)
endef

define Build/Configure/Meson
	$(call Meson/CreateNativeFile,$(PKG_BUILD_DIR)/openwrt-native.txt)
	$(call Meson/CreateCrossFile,$(PKG_BUILD_DIR)/openwrt-cross.txt)
	$(call Meson, \
		--buildtype plain \
		--native-file $(PKG_BUILD_DIR)/openwrt-native.txt \
		--cross-file $(PKG_BUILD_DIR)/openwrt-cross.txt \
		$(MESON_ARGS) \
		$(MESON_BUILD_DIR) \
		$(MESON_BUILD_DIR)/.., \
		$(MESON_VARS))
endef

define Build/Compile/Meson
	$(call Ninja,-C $(MESON_BUILD_DIR),)
endef

define Build/Install/Meson
	$(call Ninja,-C $(MESON_BUILD_DIR) install,DESTDIR="$(PKG_INSTALL_DIR)")
endef

Host/Configure=$(call Host/Configure/Meson)
Host/Compile=$(call Host/Compile/Meson)
Host/Install=$(call Host/Install/Meson)
Host/Uninstall=$(call Host/Uninstall/Meson)
Build/Configure=$(call Build/Configure/Meson)
Build/Compile=$(call Build/Compile/Meson)
Build/Install=$(call Build/Install/Meson)
