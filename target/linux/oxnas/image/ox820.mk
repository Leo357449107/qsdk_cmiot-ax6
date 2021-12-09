UBIFS_OPTS = -m 2048 -e 126KiB -c 4096
DEVICE_VARS += DTS UBIFS_OPTS
KERNEL_LOADADDR := 0x60008000

define Device/Default
  KERNEL_NAME := zImage
  KERNEL_SUFFIX := -uImage
  KERNEL_INSTALL := 1
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  SUBPAGESIZE := 512
  FILESYSTEMS := squashfs ubifs
  PROFILES = Default $$(DTS)
  SUPPORTED_DEVICES := $(subst _,$(comma),$(1))
  DEVICE_DTS := ox820-$(subst _,-,$(1))
  KERNEL := kernel-bin | append-dtb | uImage none
  IMAGES := ubinized.bin sysupgrade.tar
  IMAGE/ubinized.bin := append-ubi
  IMAGE/sysupgrade.tar := sysupgrade-tar | append-metadata
endef

define Build/omninas-factory
	rm -rf $@.tmp $@.dummy $@.dummy.gz
	mkdir -p $@.tmp
	$(CP) $@ $@.tmp/uImage
	dd if=/dev/zero bs=64k count=4 of=$@.dummy
	gzip $@.dummy
	mkimage -A arm -T ramdisk -C gzip -n "dummy" \
		-d $@.dummy.gz \
		$@.tmp/rdimg.gz
	echo 2.35.20140102 > $@.tmp/version ; echo >> $@.tmp/version
	chmod 0744 $@.tmp/*
	$(TAR) -C $@.tmp -czvf $@ \
		$(if $(SOURCE_DATE_EPOCH),--mtime="@$(SOURCE_DATE_EPOCH)") .
endef

define Build/encrypt-3des
	openssl enc -des3 -a -k $(1) -in $@ -out $@.new && mv $@.new $@
endef

define Device/akitio_mycloud
  DEVICE_TITLE := Akition myCloud (mini) / SilverStone DC01
  SUPPORTED_DEVICES += akitio
  DEVICE_PACKAGES := kmod-usb2-oxnas kmod-ata-oxnas-sata kmod-usb-ledtrig-usbport \
                     kmod-i2c-gpio kmod-rtc-ds1307
endef
TARGET_DEVICES += akitio_mycloud

define Device/cloudengines_pogoplugpro
  DEVICE_TITLE := Cloud Engines PogoPlug Pro (with mPCIe)
  SUPPORTED_DEVICES += pogoplug-pro
  DEVICE_PACKAGES := kmod-usb2-oxnas kmod-usb-ledtrig-usbport kmod-rt2800-pci wpad-basic
endef
TARGET_DEVICES += cloudengines_pogoplugpro

define Device/cloudengines_pogoplug-series-3
  DEVICE_TITLE := Cloud Engines PogoPlug Series V3 (without mPCIe)
  SUPPORTED_DEVICES += cloudengines,pogoplugv3 pogoplug-v3
  DEVICE_PACKAGES := kmod-usb2-oxnas kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += cloudengines_pogoplug-series-3

define Device/shuttle_kd20
  DEVICE_TITLE := Shuttle KD20
  SUPPORTED_DEVICES += kd20
  KERNEL := kernel-bin | append-dtb | uImage none
  KERNEL_INITRAMFS_PREFIX = $$(IMAGE_PREFIX)-factory
  KERNEL_INITRAMFS_SUFFIX := .tar.gz
  KERNEL_INITRAMFS = kernel-bin | append-dtb | uImage none | omninas-factory | encrypt-3des sohmuntitnlaes
  DEVICE_PACKAGES := kmod-usb2-oxnas kmod-ata-oxnas-sata kmod-usb-ledtrig-usbport \
                     kmod-usb3 kmod-i2c-gpio kmod-rtc-pcf8563 kmod-gpio-beeper \
                     kmod-hwmon-core kmod-hwmon-gpiofan \
                     kmod-md-mod kmod-md-raid0 kmod-md-raid1 kmod-fs-ext4 kmod-fs-xfs
endef
TARGET_DEVICES += shuttle_kd20

define Device/mitrastar_stg-212
  DEVICE_TITLE := MitraStar STG-212
  SUPPORTED_DEVICES += stg212
  KERNEL := kernel-bin | append-dtb | uImage none
  DEVICE_PACKAGES := kmod-usb2-oxnas kmod-ata-oxnas-sata kmod-fs-ext4 kmod-fs-xfs \
                     kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += mitrastar_stg-212
