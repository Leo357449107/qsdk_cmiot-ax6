
include $(TOPDIR)/rules.mk

PKG_NAME:=bazel
PKG_VERSION:=0.23.2

PKG_SOURCE:=bazel-$(PKG_VERSION)-dist.zip
PKG_SOURCE_URL:=https://github.com/bazelbuild/bazel/releases/download/0.23.2/
PKG_MD5SUM:=b860a9df5743d6d5b422d356ad7ef461

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/bazel-$(PKG_VERSION)-dist/bazel-0.23.2
include $(INCLUDE_DIR)/host-build.mk

define Host/Clean
	rm -rf $(BUILD_DIR_HOST)/bazel-*
endef

export JAVA_HOME=$(STAGING_DIR_HOST)/bin/openjdk-1.8-native
define Host/Compile
	$(HOST_BUILD_DIR)/../compile.sh
endef

define Host/Install
	$(CP) $(HOST_BUILD_DIR)/../output/bazel $(STAGING_DIR_HOST)/bin/
endef


$(eval $(call HostBuild))
