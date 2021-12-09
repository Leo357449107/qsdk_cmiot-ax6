#
# Copyright (C) 2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

# Note: include this after `include $(TOPDIR)/rules.mk in your package Makefile
#       if `python3-package.mk` is included, this will already be included

ifneq ($(__python3_host_mk_inc),1)
__python3_host_mk_inc=1

# For PYTHON3_VERSION
python3_mk_path:=$(dir $(lastword $(MAKEFILE_LIST)))
include $(python3_mk_path)python3-version.mk

HOST_PYTHON3_DIR:=$(STAGING_DIR_HOSTPKG)
HOST_PYTHON3_INC_DIR:=$(HOST_PYTHON3_DIR)/include/python$(PYTHON3_VERSION)
HOST_PYTHON3_LIB_DIR:=$(HOST_PYTHON3_DIR)/lib/python$(PYTHON3_VERSION)

HOST_PYTHON3_PKG_DIR:=$(HOST_PYTHON3_DIR)/lib/python$(PYTHON3_VERSION)/site-packages

HOST_PYTHON3_BIN:=$(HOST_PYTHON3_DIR)/bin/python$(PYTHON3_VERSION)

HOST_PYTHON3PATH:=$(HOST_PYTHON3_LIB_DIR):$(HOST_PYTHON3_PKG_DIR)

define HostPython3
	if [ "$(strip $(3))" == "HOST" ]; then \
		export PYTHONPATH="$(HOST_PYTHON3PATH)"; \
		export PYTHONDONTWRITEBYTECODE=0; \
	else \
		export PYTHONPATH="$(PYTHON3PATH)"; \
		export PYTHONDONTWRITEBYTECODE=1; \
		export _python_sysroot="$(STAGING_DIR)"; \
		export _python_prefix="/usr"; \
		export _python_exec_prefix="/usr"; \
	fi; \
	export PYTHONOPTIMIZE=""; \
	$(1) \
	$(HOST_PYTHON3_BIN) $(2);
endef

define host_python3_settings
	ARCH="$(HOST_ARCH)" \
	CC="$(HOSTCC)" \
	CCSHARED="$(HOSTCC) $(HOST_FPIC)" \
	CXX="$(HOSTCXX)" \
	LD="$(HOSTCC)" \
	LDSHARED="$(HOSTCC) -shared" \
	CFLAGS="$(HOST_CFLAGS)" \
	CPPFLAGS="$(HOST_CPPFLAGS) -I$(HOST_PYTHON3_INC_DIR)" \
	LDFLAGS="$(HOST_LDFLAGS) -lpython$(PYTHON3_VERSION) -Wl$(comma)-rpath$(comma)$(STAGING_DIR_HOSTPKG)/lib"
endef

# $(1) => commands to execute before running pythons script
# $(2) => python script and its arguments
# $(3) => additional variables
define Build/Compile/HostPy3RunHost
	$(call HostPython3, \
		$(if $(1),$(1);) \
		$(call host_python3_settings) \
		$(3) \
		, \
		$(2) \
		, \
		HOST \
	)
endef

# Note: I shamelessly copied this from Yousong's logic (from python-packages);
HOST_PYTHON3_PIP:=$(STAGING_DIR_HOSTPKG)/bin/pip$(PYTHON3_VERSION)

# $(1) => packages to install
define Build/Compile/HostPy3PipInstall
	$(call locked, \
		$(call host_python3_settings) \
		$(HOST_PYTHON3_PIP) \
			--disable-pip-version-check \
			--cache-dir "$(DL_DIR)/pip-cache" \
			install \
			--no-binary :all: \
			$(1), \
		pip \
	)
endef

# $(1) => build subdir
# $(2) => additional arguments to setup.py
# $(3) => additional variables
define Build/Compile/HostPy3Mod
	$(call Build/Compile/HostPy3RunHost, \
		cd $(HOST_BUILD_DIR)/$(strip $(1)), \
		./setup.py $(2), \
		$(3))
endef

endif # __python3_host_mk_inc
