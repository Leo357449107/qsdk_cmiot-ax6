include $(TOPDIR)/rules.mk

PKG_NAME:=coremark
PKG_VERSION:=1.01

PKG_SOURCE:=v$(PKG_VERSION).zip
PKG_SOURCE_URL:=https://github.com/eembc/coremark/archive
PKG_MD5SUM:=a87572b06cc99c0fcef3501ec45bd0fd
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/coremark
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=CoreMark EEMBC Benchmark
  URL:=https://github.com/eembc/coremark
endef

define Package/coremark/description
  CoreMark is a simple, yet sophisticated benchmark that is designed specifically to test the functionality of a processor core.
endef

ifeq ($(LINUX_KARCH),arm64)
PORT_DIR=linux64
else
PORT_DIR=linux
endif

XCFLAGS=-DMULTITHREAD=4 -DUSE_PTHREAD=1

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) PORT_DIR=$(PORT_DIR) XCFLAGS="$(XCFLAGS)" $(MAKE_FLAGS) compile
endef

define Package/coremark/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/coremark.exe $(1)/bin/coremark
endef

$(eval $(call BuildPackage,coremark))
