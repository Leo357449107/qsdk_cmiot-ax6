# Copyright (c) 2014 The Linux Foundation. All rights reserved.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

UBIFS_OPTS = -m 2048 -e 124KiB -c 4096 -U -F
UBI_OPTS = -m 2048 -p 128KiB


ifeq ($(strip $(call CompareKernelPatchVer,$(KERNEL_PATCHVER),eq,3.18)),1)
IPQ806X_KERNEL_LOADADDR = 0x42008000
IPQ40XX_KERNEL_LOADADDR = 0x80008000
else
IPQ806X_KERNEL_LOADADDR = 0x42208000
IPQ40XX_KERNEL_LOADADDR = 0x80208000
endif

KIMAGE = Image.gz
COMP_TYPE=gzip

ifneq ($(CONFIG_KERNEL_IPQ_FLASH_16M_PROFILE_DEBUG),)
IPQ50XX_16M_KERNEL_SIZE=2624k
IPQ50XX_16M_ROOTFS_SIZE=7296k
else
IPQ50XX_16M_KERNEL_SIZE=2624k
IPQ50XX_16M_ROOTFS_SIZE=5428k
endif

IPQ807X_KERNEL_LOADADDR = 0x41208000
IPQ807X_64_KERNEL_LOADADDR = 0x41080000
IPQ60XX_KERNEL_LOADADDR = 0x41008000
IPQ60XX_64_KERNEL_LOADADDR = 0x41080000
IPQ50XX_KERNEL_LOADADDR = 0x41208000
IPQ50XX_64_KERNEL_LOADADDR = 0x41080000

ifneq ($(CONFIG_TARGET_ROOTFS_UBIFS),)
define Image/mkfs/ubifs_fit
	$(call Image/ubifs_128MB,$(1))
	$(call Image/ubifs_m4096_p256KiB,$(1))
	$(call Image/ubifs_m4096_p512KiB,$(1))
endef
endif

define sizecheck
	@[ $$(($(subst k,* 1024,$(subst m, * 1024k,$(1))))) -ge "$$(stat -c%s $(2))" ] || { \
		echo "WARNING: Image file $(2) is too big, size is $$(stat -c%s $(2)) compare to max allowed $(1)" >&2; \
		rm -f $(2); \
	}
endef

define CompressLzma
	$(STAGING_DIR_HOST)/bin/lzma e $(1) -lc1 -lp2 -pb2 $(2)
endef

define Image/BuildKernel/FIT
	$(CP) $(KDIR)/Image $(BIN_DIR)/$(IMG_PREFIX)-vmlinux.bin
	gzip -9 -c $(KDIR)/Image > $(KDIR)/Image.gz
	$(call CompressLzma,$(KDIR)/Image,$(KDIR)/Image.lzma)
	$(call Image/BuildKernel/MkFIT,$(1), $(KDIR)/$(KIMAGE), $(DTS_DIR)/$(1).dtb,$(COMP_TYPE),$(2),$(2))
	$(CP) $(KDIR)/fit-$(1).itb $(BIN_DIR)/$(IMG_PREFIX)-$(1)-fit-uImage.itb
	mkdir -p $(BIN_DIR)/dtbs/
	$(CP) $(DTS_DIR)/qcom-ipq*.dtb $(BIN_DIR)/dtbs/
	$(CP) $(KDIR)/Image $(BIN_DIR)/dtbs/
endef

define Image/BuildKernel/FITInitramfs
	$(CP) $(KDIR)/Image-initramfs $(BIN_DIR)/dtbs/
	$(CP) $(KDIR)/Image-initramfs $(BIN_DIR)/$(IMG_PREFIX)-vmlinux-initramfs.bin
	$(call Image/BuildKernel/MkFIT,$(1), $(KDIR)/Image-initramfs, $(DTS_DIR)/$(1).dtb, none,$(2),$(2))
	$(CP) $(KDIR)/fit-$(1).itb $(BIN_DIR)/$(IMG_PREFIX)-$(1)-fit-uImage-initramfs.itb
endef

define Image/Prepare
	$(CP) $(LINUX_DIR)/vmlinux $(KDIR)/$(IMG_PREFIX)-vmlinux.elf
endef

define Image/BuildKernel/GenericFIT
	$(CP) $(KDIR)/Image $(BIN_DIR)/$(IMG_PREFIX)-vmlinux.bin
	gzip -9 -c $(KDIR)/Image > $(KDIR)/Image.gz
	$(call CompressLzma,$(KDIR)/Image,$(KDIR)/Image.lzma)
	$(call Image/BuildKernel/MkFIT,$(1), $(KDIR)/Image.gz,,gzip,$(2),$(2))
	$(CP) $(KDIR)/fit-$(1).itb $(BIN_DIR)/$(IMG_PREFIX)-$(1)-fit-uImage.itb
endef

define Image/BuildKernel/MultiDTBFIT
	$(CP) $(KDIR)/Image $(BIN_DIR)/$(IMG_PREFIX)-vmlinux.bin
	gzip -9 -c $(KDIR)/Image > $(KDIR)/Image.gz
	$(foreach dtb, $(2), gzip -9 -c $(DTS_DIR)/$(dtb).dtb > $(DTS_DIR)/$(dtb).dtb.gz;)
	$(call CompressLzma,$(KDIR)/Image,$(KDIR)/Image.lzma)
	$(call Image/BuildKernel/MkFITMulti,$(1),$(KDIR)/$(KIMAGE),$(2),$(COMP_TYPE),$(3),$(3),$(4),$(5))
	$$(if $$(CONFIG_PACKAGE_opkg), ,$$(call sizecheck, $(IPQ50XX_16M_KERNEL_SIZE), $(KDIR)/fit-$(1).itb))
	$$(if $$(CONFIG_KERNEL_IPQ_FLASH_16M_PROFILE_DEBUG),$$(call sizecheck, $(IPQ50XX_16M_KERNEL_SIZE), $(KDIR)/fit-$(1).itb))
	$(CP) $(KDIR)/fit-$(1).itb $(BIN_DIR)/$(IMG_PREFIX)-$(1)-fit-uImage.itb
endef

# some tree's don't have all the device tree's so we filter out
# the ones that are not present
# $(1) is the filter to apply
FindDeviceTrees ?= $(notdir $(basename $(shell find $(DTS_DIR)/ -name $(1)*.dtb $(2) 2>/dev/null)))

define Image/BuildKernel/template
	$(CP) $(KDIR)/vmlinux$(subst openwrt-ipq$(if $(SUBTARGET),-$(SUBTARGET)),,$(IMG_PREFIX)).debug $(BIN_DIR)/$(IMG_PREFIX)-vmlinux.elf
endef

define Image/BuildKernel/ipq806x

	$(foreach device, $(call FindDeviceTrees, qcom-ipq8064-), \
		$(call Image/BuildKernel/$(1),$(device),$(IPQ806X_KERNEL_LOADADDR));)

	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq806x-akxx, \
		$(call FindDeviceTrees, qcom-ipq806?-), \
		$(IPQ806X_KERNEL_LOADADDR),gzip,0x43F00000)
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-ap148_1xx, \
		$(call FindDeviceTrees, qcom-ipq8064-*ap148_1xx), \
		$(IPQ806X_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-ap148, \
		$(call FindDeviceTrees, qcom-ipq8064-*ap148,-not -name *ap148_1xx* -not -name *ap148-nand*), \
		$(IPQ806X_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-db149, \
		$(call FindDeviceTrees, qcom-ipq8064-*db149), \
		$(IPQ806X_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-ak01, \
		$(call FindDeviceTrees, qcom-ipq8064-*ak01), \
		$(IPQ806X_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-ap145_1xx, \
		$(call FindDeviceTrees, qcom-ipq8064-*ap145_1xx), \
		$(IPQ806X_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-ap145, \
		$(call FindDeviceTrees, qcom-ipq8064-*ap145,-not -name *ap145_1xx*), \
		$(IPQ806X_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-ap160_2xx, \
		$(call FindDeviceTrees, qcom-ipq8064-*ap160_2xx), \
		$(IPQ806X_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-ap160, \
		$(call FindDeviceTrees, qcom-ipq8064-*ap160,-not -name *ap160_2xx*), \
		$(IPQ806X_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq8064-ap161, \
		$(call FindDeviceTrees, qcom-ipq8064-*ap161), \
		$(IPQ806X_KERNEL_LOADADDR))
endef

define Image/BuildKernel/ipq40xx
	$(foreach device, $(call FindDeviceTrees, qcom-ipq40??-), \
		$(call Image/BuildKernel/$(1),$(device),$(IPQ40XX_KERNEL_LOADADDR));)

	$(call Image/BuildKernel/GenericFIT,qcom-ipq40xx,$(IPQ40XX_KERNEL_LOADADDR))
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq40xx-ap.dkxx, \
		$(call FindDeviceTrees, qcom-ipq40??-ap) $(call FindDeviceTrees, qcom-ipq40??-db), \
		$(IPQ40XX_KERNEL_LOADADDR))
endef

define Image/BuildKernel/ipq807x
	$(foreach device, $(call FindDeviceTrees, qcom-ipq807x-), \
		$(call Image/BuildKernel/$(1),$(device),$(IPQ807X_KERNEL_LOADADDR));)

	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq807x-hkxx, \
		$(call FindDeviceTrees, qcom-ipq807x-), $(IPQ807X_KERNEL_LOADADDR))
endef

define Image/BuildKernel/ipq807x_64
	$(foreach device, $(call FindDeviceTrees, qcom-ipq807x-), \
		$(call Image/BuildKernel/$(1),$(device),$(IPQ807X_64_KERNEL_LOADADDR));)

	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq807x-hkxx, \
		$(call FindDeviceTrees, qcom-ipq807x-), $(IPQ807X_64_KERNEL_LOADADDR))
endef

define Image/BuildKernel/ipq60xx
	$(foreach device, $(call FindDeviceTrees, qcom-ipq6018-), \
		$(call Image/BuildKernel/$(1),$(device),$(IPQ60XX_KERNEL_LOADADDR));)

	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq60xx-cpxx, \
		$(call FindDeviceTrees, qcom-ipq6018-), $(IPQ60XX_KERNEL_LOADADDR))
endef

define Image/BuildKernel/ipq60xx_64
	$(foreach device, $(call FindDeviceTrees, qcom-ipq6018-), \
		$(call Image/BuildKernel/$(1),$(device),$(IPQ60XX_64_KERNEL_LOADADDR));)
	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq60xx-cpxx, \
		$(call FindDeviceTrees, qcom-ipq6018-), $(IPQ60XX_64_KERNEL_LOADADDR))
endef

define Image/BuildKernel/ipq50xx
	$(eval KIMAGE:="Image.lzma")
	$(eval COMP_TYPE:="lzma")
	$(foreach device, $(call FindDeviceTrees, qcom-ipq5018-), \
		$(call Image/BuildKernel/$(1),$(device),$(IPQ50XX_KERNEL_LOADADDR));)

	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq50xx-mpxx, \
		$(call FindDeviceTrees, qcom-ipq5018-), $(IPQ50XX_KERNEL_LOADADDR))
endef

define Image/BuildKernel/ipq50xx_64
	$(eval KIMAGE:="Image.lzma")
	$(eval COMP_TYPE:="lzma")
	$(foreach device, $(call FindDeviceTrees, qcom-ipq5018-), \
		$(call Image/BuildKernel/$(1),$(device),$(IPQ50XX_64_KERNEL_LOADADDR));)

	$(call Image/BuildKernel/MultiDTBFIT,qcom-ipq50xx-mpxx, \
		$(call FindDeviceTrees, qcom-ipq5018-), $(IPQ50XX_64_KERNEL_LOADADDR))
endef

define Image/BuildKernel
	$(call Image/BuildKernel/template,FIT)
	$(call Image/BuildKernel/$(SUBTARGET),FIT)
endef

define Image/BuildKernel/Initramfs
	$(CP) $(KDIR)/vmlinux-initramfs$(subst openwrt-ipq$(if $(SUBTARGET),-$(SUBTARGET)),,$(IMG_PREFIX)).debug $(BIN_DIR)/$(IMG_PREFIX)-vmlinux-initramfs.elf
	$(call Image/BuildKernel/template,FITInitramfs)
	$(call Image/BuildKernel/$(SUBTARGET),FITInitramfs)
endef

define Image/ubifs_m4096_p512KiB

  ifeq ($(CONFIG_TARGET_ipq_ipq806x),)
	$(eval UBI_OPTS:="-m 4096 -p 512KiB")
	$(call Image/mkfs/ubifs/generate,$(1))
	$(call Image/Build,ubi,$(1),-m4096-p512KiB)
  endif
endef

define Image/ubifs_m4096_p256KiB

  ifeq ($(CONFIG_TARGET_ipq_ipq806x),)
	$(eval UBI_OPTS:="-m 4096 -p 256KiB")
	$(call Image/mkfs/ubifs/generate,$(1))
	$(call Image/Build,ubi,$(1),-m4096-p256KiB)
  endif
endef

define Image/ubifs_128MB
	$(eval UBI_OPTS:="-m 2048 -p 128KiB")
	$(call Image/mkfs/ubifs/generate,$(1))
	$(call Image/Build,ubi,$(1))
endef

define Image/Build/ubifs
	$(call Image/mkfs/ubifs_fit,-$(SUBTARGET))
endef

ifeq ($(CONFIG_LOWMEM_FLASH),)
define Image/Build/squashfs
	$(call prepare_generic_squashfs,$(KDIR)/root.squashfs)
endef
endif

define Image/Build
	$(call Image/Build/$(1),$(1))
	$$(if $(CONFIG_PACKAGE_opkg), ,$$(call sizecheck, $(IPQ50XX_16M_ROOTFS_SIZE), $(KDIR)/root$(2).$(1)))
	$$(if $(CONFIG_KERNEL_IPQ_FLASH_16M_PROFILE_DEBUG),$$(call sizecheck, $(IPQ50XX_16M_ROOTFS_SIZE), $(KDIR)/root$(2).$(1)))
	dd if=$(KDIR)/root$(2).$(1) of=$(BIN_DIR)/$(IMG_PREFIX)-$(1)-root$(3).img bs=2k conv=sync
endef

$(eval $(call BuildImage))
