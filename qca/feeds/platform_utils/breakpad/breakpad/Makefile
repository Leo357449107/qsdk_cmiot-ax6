include $(TOPDIR)/rules.mk

PKG_NAME:=breakpad
PKG_VERSION:=0.1
PKG_RELEASE=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://chromium.googlesource.com/breakpad/breakpad
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=c46151db0ffd1a8dae914e45f1212ef427f61ed3
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_MD5SUM:=skip

PKG_BUILD_DEPENDS:=breakpad/host
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)
HOST_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/$(PKG_NAME)
  SECTION:=BREAKPAD
  CATEGORY:=Breakpad Software Utilities
  URL:=https://chromium.googlesource.com/breakpad/breakpad
  MAINTAINER:=Google
  TITLE:=breakpad
  SUBMENU:=Breakpad
  DEPENDS:= +libstdcpp
endef

define Download/lss
  PROTO=git
  URL=https://chromium.googlesource.com/linux-syscall-support
  SUBDIR=lss-1.0
  FILE=lss-1.0.tar.gz
  VERSION=8048ece6c16c91acfe0d36d1d3cc0890ab6e945c
endef

TARGET_CFLAGS+=-I$(STAGING_DIR)/usr/include \
	-DTARGET="native"  \
	-DTARGET_NAME=\''native'\' \
	-I$(STAGING_DIR)/usr/include/ \
	-I$(STAGING_DIR)/usr/include/breakpad

define Build/Prepare
	$(TAR) xzvf $(TOPDIR)/dl/$(PKG_SOURCE) -C $(TOPDIR)/dl
	$(CP) $(TOPDIR)/dl/$(PKG_NAME)-$(PKG_VERSION)/* $(PKG_BUILD_DIR)
	$(eval $(call Download,lss))
	$(TAR) xzvf $(TOPDIR)/dl/lss-1.0.tar.gz -C $(TOPDIR)/dl
	mkdir -p $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)/src/third_party/lss
	$(CP) $(TOPDIR)/dl/lss-1.0/* $(PKG_BUILD_DIR)/src/third_party/lss/
	$(Build/Patch)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad

	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad/client/linux/crash_generation
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/crash_generation/crash_generation_client.h $(STAGING_DIR)/usr/include/breakpad/client/linux/crash_generation

	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad/client/linux/handler
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/handler/exception_handler.h $(STAGING_DIR)/usr/include/breakpad/client/linux/handler
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/handler/minidump_descriptor.h $(STAGING_DIR)/usr/include/breakpad/client/linux/handler
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/handler/microdump_extra_info.h $(STAGING_DIR)/usr/include/breakpad/client/linux/handler

	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad/client/linux/dump_writer_common
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/dump_writer_common/mapping_info.h $(STAGING_DIR)/usr/include/breakpad/client/linux/dump_writer_common
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/dump_writer_common/thread_info.h $(STAGING_DIR)/usr/include/breakpad/client/linux/dump_writer_common
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/dump_writer_common/raw_context_cpu.h $(STAGING_DIR)/usr/include/breakpad/client/linux/dump_writer_common

	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad/client/linux/minidump_writer
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/minidump_writer/linux_dumper.h $(STAGING_DIR)/usr/include/breakpad/client/linux/minidump_writer
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/minidump_writer/minidump_writer.h $(STAGING_DIR)/usr/include/breakpad/client/linux/minidump_writer

	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/common/memory_range.h $(STAGING_DIR)/usr/include/breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/common/memory_allocator.h $(STAGING_DIR)/usr/include/breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/common/scoped_ptr.h $(STAGING_DIR)/usr/include/breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/common/using_std_string.h $(STAGING_DIR)/usr/include/breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/common/basictypes.h $(STAGING_DIR)/usr/include/breakpad/common

	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad/common/linux
	$(CP) $(PKG_BUILD_DIR)/src/common/linux/file_id.h $(STAGING_DIR)/usr/include/breakpad/common/linux
	$(CP) $(PKG_BUILD_DIR)/src/common/linux/guid_creator.h $(STAGING_DIR)/usr/include/breakpad/common/linux

	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/breakpad_types.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_format.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_cpu_amd64.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_cpu_arm.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_cpu_arm64.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_cpu_mips.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_cpu_ppc64.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_cpu_ppc.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_cpu_sparc.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_cpu_x86.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_exception_linux.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_exception_mac.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_exception_ps3.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_exception_solaris.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common
	$(CP) $(PKG_BUILD_DIR)/src/google_breakpad/common/minidump_exception_win32.h $(STAGING_DIR)/usr/include/breakpad/google_breakpad/common

	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/breakpad/third_party/lss
	$(CP) $(PKG_BUILD_DIR)/src/third_party/lss/linux_syscall_support.h $(STAGING_DIR)/usr/include/breakpad/third_party/lss
	$(CP) $(PKG_BUILD_DIR)/src/client/linux/libbreakpad_client.a $(STAGING_DIR)/usr/lib
endef

define Package/${PKG_NAME}/install
	$(INSTALL_DIR) $(1)/usr/bin/breakpad
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/tools/linux/md2core/minidump-2-core $(1)/usr/bin/breakpad
endef

define Host/Prepare
	$(TAR) xzvf $(TOPDIR)/dl/$(PKG_SOURCE) -C $(TOPDIR)/dl
	$(CP) $(TOPDIR)/dl/$(PKG_NAME)-$(PKG_VERSION)/* $(HOST_BUILD_DIR)
	$(eval $(call Download,lss))
	$(TAR) xzvf $(TOPDIR)/dl/lss-1.0.tar.gz -C $(TOPDIR)/dl
	mkdir -p $(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)/src/third_party/lss
	$(CP) $(TOPDIR)/dl/lss-1.0/* $(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)/src/third_party/lss
	$(Host/Patch)
endef

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/usr/bin/breakpad
	$(CP) $(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)/src/tools/linux/dump_syms/dump_syms $(STAGING_DIR_HOST)/usr/bin/breakpad
	$(CP) $(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)/src/processor/minidump_stackwalk $(STAGING_DIR_HOST)/usr/bin/breakpad
	$(CP) ./files/make_syms $(STAGING_DIR_HOST)/usr/bin/breakpad
	#$(CP) $(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)/src/tools/linux/core2md/core2md $(STAGING_DIR_HOST)/usr/bin/breakpad
	#$(CP) $(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)/src/tools/linux/symupload/minidump_upload $(STAGING_DIR_HOST)/usr/bin/breakpad
	#$(CP) $(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)/src/tools/linux/symupload/sym_upload $(STAGING_DIR_HOST)/usr/bin/breakpad
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call HostBuild))
