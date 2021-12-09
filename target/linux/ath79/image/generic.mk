include ./common-buffalo.mk
include ./common-netgear.mk
include ./common-yuncore.mk

DEVICE_VARS += ADDPATTERN_ID ADDPATTERN_VERSION
DEVICE_VARS += SEAMA_SIGNATURE SEAMA_MTDBLOCK
DEVICE_VARS += KERNEL_INITRAMFS_PREFIX

define Build/cybertan-trx
	@echo -n '' > $@-empty.bin
	-$(STAGING_DIR_HOST)/bin/trx -o $@.new \
		-f $(IMAGE_KERNEL) -F $@-empty.bin \
		-x 32 -a 0x10000 -x -32 -f $@
	-mv "$@.new" "$@"
	-rm $@-empty.bin
endef

define Build/addpattern
	-$(STAGING_DIR_HOST)/bin/addpattern -B $(ADDPATTERN_ID) \
		-v v$(ADDPATTERN_VERSION) -i $@ -o $@.new
	-mv "$@.new" "$@"
endef

define Build/add-elecom-factory-initramfs
  $(eval edimax_model=$(word 1,$(1)))
  $(eval product=$(word 2,$(1)))

  $(STAGING_DIR_HOST)/bin/mkedimaximg \
	-b -s CSYS -m $(edimax_model) \
	-f 0x70000 -S 0x01100000 \
	-i $@ -o $@.factory

  ( \
	echo -n -e "ELECOM\x00\x00$(product)" | dd bs=40 count=1 conv=sync; \
	echo -n "0.00" | dd bs=16 count=1 conv=sync; \
	dd if=$@.factory; \
  ) > $@.factory.new

  if [ "$$(stat -c%s $@.factory.new)" -le $$(($(subst k,* 1024,$(subst m, * 1024k,$(IMAGE_SIZE))))) ]; then \
	mv $@.factory.new $(BIN_DIR)/$(KERNEL_INITRAMFS_PREFIX)-factory.bin; \
  else \
	echo "WARNING: initramfs kernel image too big, cannot generate factory image" >&2; \
  fi
endef

define Build/nec-enc
  $(STAGING_DIR_HOST)/bin/nec-enc \
    -i $@ -o $@.new -k $(1)
  mv $@.new $@
endef

define Build/nec-fw
  ( stat -c%s $@ | tr -d "\n" | dd bs=16 count=1 conv=sync; ) >> $@
  ( \
    echo -n -e "$(1)" | dd bs=16 count=1 conv=sync; \
    echo -n "0.0.00" | dd bs=16 count=1 conv=sync; \
    dd if=$@; \
  ) > $@.new
  mv $@.new $@
endef

define Device/seama
  KERNEL := kernel-bin | append-dtb | relocate-kernel | lzma
  KERNEL_INITRAMFS := $$(KERNEL) | seama
  IMAGES += factory.bin
  SEAMA_MTDBLOCK := 1

  # 64 bytes offset:
  # - 28 bytes seama_header
  # - 36 bytes of META data (4-bytes aligned)
  IMAGE/default := append-kernel | pad-offset $$$$(BLOCKSIZE) 64 | append-rootfs
  IMAGE/sysupgrade.bin := \
	$$(IMAGE/default) | seama | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := \
	$$(IMAGE/default) | pad-rootfs -x 64 | seama | seama-seal | check-size $$$$(IMAGE_SIZE)
  SEAMA_SIGNATURE :=
endef

define Device/8dev_carambola2
  ATH_SOC := ar9331
  DEVICE_TITLE := 8devices Carambola2
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-chipidea2
  IMAGE_SIZE := 16000k
  SUPPORTED_DEVICES += carambola2
endef
TARGET_DEVICES += 8dev_carambola2

define Device/aruba_ap-105
  ATH_SOC := ar7161
  DEVICE_TITLE := Aruba AP-105
  IMAGE_SIZE := 16000k
  DEVICE_PACKAGES := kmod-i2c-core kmod-i2c-gpio kmod-tpm-i2c-atmel
endef
TARGET_DEVICES += aruba_ap-105

define Device/avm_fritz300e
  ATH_SOC := ar7242
  DEVICE_TITLE := AVM FRITZ!WLAN Repeater 300E
  KERNEL := kernel-bin | append-dtb | lzma | eva-image
  KERNEL_INITRAMFS := $$(KERNEL)
  IMAGE_SIZE := 15232k
  IMAGE/sysupgrade.bin := append-kernel | pad-to 64k | \
	append-squashfs-fakeroot-be | pad-to 256 | \
	append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
  DEVICE_PACKAGES := fritz-tffs rssileds -swconfig
  SUPPORTED_DEVICES += fritz300e
endef
TARGET_DEVICES += avm_fritz300e

define Device/avm_fritz4020
  ATH_SOC := qca9561
  DEVICE_TITLE := AVM FRITZ!Box 4020
  IMAGE_SIZE := 15232k
  KERNEL := kernel-bin | append-dtb | lzma | eva-image
  KERNEL_INITRAMFS := $$(KERNEL)
  IMAGE/sysupgrade.bin := append-kernel | pad-to 64k | \
      append-squashfs-fakeroot-be | pad-to 256 | \
      append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
  DEVICE_PACKAGES := fritz-tffs
  SUPPORTED_DEVICES += fritz4020
endef
TARGET_DEVICES += avm_fritz4020

define Device/buffalo_bhr-4grv
  ATH_SOC := ar7242
  DEVICE_TITLE := Buffalo BHR-4GRV
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-ledtrig-usbport
  IMAGE_SIZE := 32256k
  IMAGES += factory.bin tftp.bin
  IMAGE/default := append-kernel | pad-to $$$$(BLOCKSIZE) | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := $$(IMAGE/default) | buffalo-enc BHR-4GRV 1.99 | buffalo-tag BHR-4GRV 3
  IMAGE/tftp.bin := $$(IMAGE/default) | buffalo-tftp-header
  SUPPORTED_DEVICES += wzr-hp-g450h
endef
TARGET_DEVICES += buffalo_bhr-4grv

define Device/buffalo_bhr-4grv2
  ATH_SOC := qca9557
  DEVICE_TITLE := Buffalo BHR-4GRV2
  IMAGE_SIZE := 16000k
endef
TARGET_DEVICES += buffalo_bhr-4grv2

define Device/buffalo_wzr-hp-ag300h
  ATH_SOC := ar7161
  DEVICE_TITLE := Buffalo WZR-HP-AG300H
  IMAGE_SIZE := 32256k
  IMAGES += factory.bin tftp.bin
  IMAGE/default := append-kernel | pad-to $$$$(BLOCKSIZE) | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := $$(IMAGE/default) | buffalo-enc WZR-HP-AG300H 1.99 | buffalo-tag WZR-HP-AG300H 3
  IMAGE/tftp.bin := $$(IMAGE/default) | buffalo-tftp-header
  DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-usb-ledtrig-usbport kmod-leds-reset kmod-owl-loader
  SUPPORTED_DEVICES += wzr-hp-ag300h
endef
TARGET_DEVICES += buffalo_wzr-hp-ag300h

define Device/buffalo_wzr-hp-g302h-a1a0
  ATH_SOC := ar7242
  DEVICE_TITLE := Buffalo WZR-HP-G302H A1A0
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-ledtrig-usbport
  IMAGE_SIZE := 32128k
  IMAGES += factory.bin tftp.bin
  IMAGE/default := append-kernel | pad-to $$$$(BLOCKSIZE) | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := $$(IMAGE/default) | buffalo-enc WZR-HP-G302H 1.99 | buffalo-tag WZR-HP-G302H 4
  IMAGE/tftp.bin := $$(IMAGE/default) | buffalo-tftp-header
  SUPPORTED_DEVICES += wzr-hp-g300nh2
endef
TARGET_DEVICES += buffalo_wzr-hp-g302h-a1a0

define Device/buffalo_wzr-hp-g450h
  ATH_SOC := ar7242
  DEVICE_TITLE := Buffalo WZR-HP-G450H/WZR-450HP
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-ledtrig-usbport
  IMAGE_SIZE := 32256k
  IMAGES += factory.bin tftp.bin
  IMAGE/default := append-kernel | pad-to $$$$(BLOCKSIZE) | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := $$(IMAGE/default) | buffalo-enc WZR-HP-G450H 1.99 | buffalo-tag WZR-HP-G450H 3
  IMAGE/tftp.bin := $$(IMAGE/default) | buffalo-tftp-header
  SUPPORTED_DEVICES += wzr-hp-g450h
endef
TARGET_DEVICES += buffalo_wzr-hp-g450h

define Device/comfast_cf-e110n-v2
  ATH_SOC := qca9533
  DEVICE_TITLE := COMFAST CF-E110N v2
  DEVICE_PACKAGES := rssileds kmod-leds-gpio -swconfig -uboot-envtools
  IMAGE_SIZE := 16192k
endef
TARGET_DEVICES += comfast_cf-e110n-v2

define Device/comfast_cf-e120a-v3
  ATH_SOC := ar9344
  DEVICE_TITLE := COMFAST CF-E120A v3
  DEVICE_PACKAGES := rssileds kmod-leds-gpio -uboot-envtools
  IMAGE_SIZE := 8000k
endef
TARGET_DEVICES += comfast_cf-e120a-v3

define Device/comfast_cf-e5
  ATH_SOC := qca9531
  DEVICE_TITLE := COMFAST CF-E5/E7
  DEVICE_PACKAGES := rssileds kmod-leds-gpio kmod-usb-core kmod-usb2 kmod-usb-net \
	kmod-usb-net-qmi-wwan -swconfig -uboot-envtools
  IMAGE_SIZE := 16192k
endef
TARGET_DEVICES += comfast_cf-e5

define Device/devolo_dvl1200e
  ATH_SOC := qca9558
  DEVICE_TITLE := devolo WiFi pro 1200e
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
  IMAGE_SIZE := 15936k
endef
TARGET_DEVICES += devolo_dvl1200e

define Device/devolo_dvl1200i
  ATH_SOC := qca9558
  DEVICE_TITLE := devolo WiFi pro 1200i
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
  IMAGE_SIZE := 15936k
endef
TARGET_DEVICES += devolo_dvl1200i

define Device/devolo_dvl1750c
  ATH_SOC := qca9558
  DEVICE_TITLE := devolo WiFi pro 1750c
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
  IMAGE_SIZE := 15936k
endef
TARGET_DEVICES += devolo_dvl1750c

define Device/devolo_dvl1750e
  ATH_SOC := qca9558
  DEVICE_TITLE := devolo WiFi pro 1750e
  DEVICE_PACKAGES := kmod-usb2 kmod-ath10k-ct ath10k-firmware-qca988x-ct
  IMAGE_SIZE := 15936k
endef
TARGET_DEVICES += devolo_dvl1750e

define Device/devolo_dvl1750i
  ATH_SOC := qca9558
  DEVICE_TITLE := devolo WiFi pro 1750i
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
  IMAGE_SIZE := 15936k
endef
TARGET_DEVICES += devolo_dvl1750i

define Device/devolo_dvl1750x
  ATH_SOC := qca9558
  DEVICE_TITLE := devolo WiFi pro 1750x
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
  IMAGE_SIZE := 15936k
endef
TARGET_DEVICES += devolo_dvl1750x

define Device/dlink_dir-825-b1
  ATH_SOC := ar7161
  DEVICE_TITLE := D-LINK DIR-825 B1
  IMAGE_SIZE := 6208k
  IMAGE/sysupgrade.bin := append-kernel | append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
  DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-usb-ledtrig-usbport kmod-leds-reset kmod-owl-loader
  SUPPORTED_DEVICES += dir-825-b1
endef
TARGET_DEVICES += dlink_dir-825-b1

define Device/dlink_dir-825-c1
  ATH_SOC := ar9344
  DEVICE_TITLE := D-LINK DIR-825 C1
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-ledtrig-usbport kmod-leds-reset kmod-owl-loader
  SUPPORTED_DEVICES += dir-825-c1
  IMAGE_SIZE := 15936k
  IMAGES := factory.bin sysupgrade.bin
  IMAGE/default := append-kernel | pad-to $$$$(BLOCKSIZE) | append-rootfs | pad-rootfs
  IMAGE/factory.bin := $$(IMAGE/default) | pad-offset $$$$(IMAGE_SIZE) 26 | \
	append-string 00DB120AR9344-RT-101214-00 | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := $$(IMAGE/default) | append-metadata | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += dlink_dir-825-c1

define Device/dlink_dir-835-a1
  ATH_SOC := ar9344
  DEVICE_TITLE := D-LINK DIR-835 A1
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-leds-reset kmod-owl-loader
  SUPPORTED_DEVICES += dir-835-a1
  IMAGE_SIZE := 15936k
  IMAGES := factory.bin sysupgrade.bin
  IMAGE/default := append-kernel | pad-to $$$$(BLOCKSIZE) | append-rootfs | pad-rootfs
  IMAGE/factory.bin := $$(IMAGE/default) | pad-offset $$$$(IMAGE_SIZE) 26 | \
	append-string 00DB120AR9344-RT-101214-00 | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := $$(IMAGE/default) | append-metadata | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += dlink_dir-835-a1

define Device/dlink_dir-859-a1
  $(Device/seama)
  ATH_SOC := qca9563
  DEVICE_TITLE := D-LINK DIR-859 A1
  IMAGE_SIZE := 15872k
  DEVICE_PACKAGES :=  kmod-usb-core kmod-usb2 kmod-ath10k-ct-smallbuffers ath10k-firmware-qca988x-ct
  SEAMA_SIGNATURE := wrgac37_dlink.2013gui_dir859
endef
TARGET_DEVICES += dlink_dir-859-a1

define Device/elecom_wrc-1750ghbk2-i
  ATH_SOC := qca9563
  DEVICE_TITLE := ELECOM WRC-1750GHBK2-I/C
  IMAGE_SIZE := 15808k
  KERNEL_INITRAMFS := $$(KERNEL) | pad-to 2 | \
	add-elecom-factory-initramfs RN68 WRC-1750GHBK2
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
endef
TARGET_DEVICES += elecom_wrc-1750ghbk2-i

define Device/elecom_wrc-300ghbk2-i
  ATH_SOC := qca9563
  DEVICE_TITLE := ELECOM WRC-300GHBK2-I
  IMAGE_SIZE := 7616k
  KERNEL_INITRAMFS := $$(KERNEL) | pad-to 2 | \
	add-elecom-factory-initramfs RN51 WRC-300GHBK2-I
endef
TARGET_DEVICES += elecom_wrc-300ghbk2-i

define Device/embeddedwireless_dorin
  ATH_SOC := ar9331
  DEVICE_TITLE := Embedded Wireless Dorin
  DEVICE_PACKAGES := kmod-usb-chipidea2
  IMAGE_SIZE := 16000k
endef
TARGET_DEVICES += embeddedwireless_dorin

define Device/engenius_ecb1750
  ATH_SOC := qca9558
  DEVICE_TITLE := EnGenius ECB1750
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
  IMAGE_SIZE := 15680k
  IMAGE/factory.bin := append-kernel | pad-to $$$$(BLOCKSIZE) | \
    append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE) | \
    senao-header -r 0x101 -p 0x6d -t 2
  IMAGE/sysupgrade.bin := append-kernel | append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += engenius_ecb1750

define Device/engenius_epg5000
  ATH_SOC := qca9558
  DEVICE_TITLE := EnGenius EPG5000
  DEVICE_PACKAGES := ath10k-firmware-qca988x-ct kmod-ath10k-ct kmod-usb2
  IMAGE_SIZE := 14656k
  IMAGES += factory.dlf
  IMAGE/factory.dlf := append-kernel | pad-to $$$$(BLOCKSIZE) | \
	append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE) | \
	senao-header -r 0x101 -p 0x71 -t 2
  SUPPORTED_DEVICES += epg5000
endef
TARGET_DEVICES += engenius_epg5000

define Device/engenius_ews511ap
  ATH_SOC := qca9531
  DEVICE_TITLE := EnGenius EWS511AP
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca9887-ct
  IMAGE_SIZE := 16000k
endef
TARGET_DEVICES += engenius_ews511ap

define Device/etactica_eg200
  ATH_SOC := ar9331
  DEVICE_TITLE := eTactica EG200
  DEVICE_PACKAGES := kmod-usb-chipidea2 kmod-ledtrig-oneshot \
	kmod-usb-serial kmod-usb-serial-ftdi kmod-usb-storage  kmod-fs-ext4
  SUPPORTED_DEVICES += rme-eg200
endef
TARGET_DEVICES += etactica_eg200

define Device/glinet_gl-ar150
  ATH_SOC := ar9330
  DEVICE_TITLE := GL.iNet GL-AR150
  DEVICE_PACKAGES := kmod-usb-chipidea2
  IMAGE_SIZE := 16000k
  SUPPORTED_DEVICES += gl-ar150
endef
TARGET_DEVICES += glinet_gl-ar150

define Device/glinet_gl-ar300m-common-nor
  ATH_SOC := qca9531
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  IMAGE_SIZE := 16000k
  SUPPORTED_DEVICES += gl-ar300m
endef

define Device/glinet_gl-ar300m-lite
  $(Device/glinet_gl-ar300m-common-nor)
  DEVICE_TITLE := GL.iNet GL-AR300M-Lite
endef
TARGET_DEVICES += glinet_gl-ar300m-lite

define Device/glinet_gl-ar300m-nor
  $(Device/glinet_gl-ar300m-common-nor)
  DEVICE_TITLE := GL.iNet GL-AR300M
endef
#TARGET_DEVICES += glinet_gl-ar300m-nor

define Device/glinet_gl-ar750s
  ATH_SOC := qca9563
  DEVICE_TITLE := GL.iNet GL-AR750S
  DEVICE_PACKAGES := kmod-usb2 kmod-ath10k-ct ath10k-firmware-qca9887-ct block-mount
  IMAGE_SIZE := 16000k
  SUPPORTED_DEVICES += gl-ar750s
endef
#TARGET_DEVICES += glinet_gl-ar750s

define Device/glinet_gl-x750
  ATH_SOC := qca9531
  DEVICE_TITLE := GL.iNet GL-X750
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-ath10k-ct ath10k-firmware-qca9887-ct
  IMAGE_SIZE := 16000k
endef
TARGET_DEVICES += glinet_gl-x750

define Device/iodata_etg3-r
  ATH_SOC := ar9342
  DEVICE_TITLE := I-O DATA ETG3-R
  IMAGE_SIZE := 7680k
  DEVICE_PACKAGES := -iwinfo -kmod-ath9k -wpad-basic
endef
TARGET_DEVICES += iodata_etg3-r

define Device/iodata_wn-ac1167dgr
  ATH_SOC := qca9557
  DEVICE_TITLE := I-O DATA WN-AC1167DGR
  IMAGE_SIZE := 14656k
  IMAGES += factory.bin
  IMAGE/factory.bin := append-kernel | pad-to $$$$(BLOCKSIZE) | \
    append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE) | \
    senao-header -r 0x30a -p 0x61 -t 2
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-ath10k-ct ath10k-firmware-qca988x-ct
endef
TARGET_DEVICES += iodata_wn-ac1167dgr

define Device/iodata_wn-ac1600dgr
  ATH_SOC := qca9557
  DEVICE_TITLE := I-O DATA WN-AC1600DGR
  IMAGE_SIZE := 14656k
  IMAGES += factory.bin
  IMAGE/factory.bin := append-kernel | pad-to $$$$(BLOCKSIZE) | \
    append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE) | \
    senao-header -r 0x30a -p 0x60 -t 2 -v 200
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-ath10k-ct ath10k-firmware-qca988x-ct
endef
TARGET_DEVICES += iodata_wn-ac1600dgr

define Device/iodata_wn-ac1600dgr2
  ATH_SOC := qca9557
  DEVICE_TITLE := I-O DATA WN-AC1600DGR2
  IMAGE_SIZE := 14656k
  IMAGES += factory.bin
  IMAGE/factory.bin := append-kernel | pad-to $$$$(BLOCKSIZE) | \
    append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE) | \
    senao-header -r 0x30a -p 0x60 -t 2 -v 200
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-ath10k-ct ath10k-firmware-qca988x-ct
endef
TARGET_DEVICES += iodata_wn-ac1600dgr2

define Device/iodata_wn-ag300dgr
  ATH_SOC := ar1022
  DEVICE_TITLE := I-O DATA WN-AG300DGR
  IMAGE_SIZE := 15424k
  IMAGES += factory.bin
  IMAGE/factory.bin := append-kernel | pad-to $$$$(BLOCKSIZE) | \
    append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE) | \
    senao-header -r 0x30a -p 0x47 -t 2
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
endef
TARGET_DEVICES += iodata_wn-ag300dgr

define Device/jjplus_ja76pf2
  ATH_SOC := ar7161
  DEVICE_TITLE := jjPlus JA76PF2
  DEVICE_PACKAGES += -kmod-ath9k -swconfig -wpad-mini -uboot-envtools fconfig
  IMAGE/sysupgrade.bin := append-rootfs | pad-rootfs | combined-image | append-metadata | check-size $$$$(IMAGE_SIZE)
#  IMAGE/sysupgrade.bin := append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE) | sysupgrade-tar rootfs=$$$$@ | append-metadata
  KERNEL := kernel-bin | append-dtb | lzma | pad-to $$(BLOCKSIZE)
  KERNEL_INITRAMFS := kernel-bin | append-dtb
  IMAGE_SIZE := 16000k
  SUPPORTED_DEVICES += ja76pf2
endef
TARGET_DEVICES += jjplus_ja76pf2

define Device/librerouter_librerouter-v1
  ATH_SOC := qca9558
  DEVICE_TITLE := LibreRouter v1
  IMAGE_SIZE := 7936k
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
endef
TARGET_DEVICES += librerouter_librerouter-v1

define Device/nec_wg1200cr
  ATH_SOC := qca9563
  DEVICE_TITLE := NEC Aterm WG1200CR
  IMAGE_SIZE := 7616k
  SEAMA_MTDBLOCK := 6
  SEAMA_SIGNATURE := wrgac72_necpf.2016gui_wg1200cr
  IMAGES += factory.bin
  IMAGE/default := \
    append-kernel | pad-offset $$$$(BLOCKSIZE) 64 | append-rootfs
  IMAGE/sysupgrade.bin := \
    $$(IMAGE/default) | seama | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := \
    $$(IMAGE/default) | pad-rootfs -x 64 | seama | seama-seal | nec-enc 9gsiy9nzep452pad | \
    check-size $$$$(IMAGE_SIZE)
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca9888-ct
endef
TARGET_DEVICES += nec_wg1200cr

define Device/nec_wg800hp
  ATH_SOC := qca9563
  DEVICE_TITLE := NEC Aterm WG800HP
  IMAGE_SIZE := 7104k
  IMAGES += factory.bin
  IMAGE/factory.bin := append-kernel | pad-to $$$$(BLOCKSIZE) | \
    append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE) | \
    xor-image -p 6A57190601121E4C004C1E1201061957 -x | \
    nec-fw LASER_ATERM
  DEVICE_PACKAGES := kmod-ath10k-ct-smallbuffers ath10k-firmware-qca9887-ct-htt
endef
TARGET_DEVICES += nec_wg800hp

define Device/ocedo_koala
  ATH_SOC := qca9558
  DEVICE_TITLE := OCEDO Koala
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
  SUPPORTED_DEVICES += koala
  IMAGE_SIZE := 7424k
  IMAGE/sysupgrade.bin := append-kernel | append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += ocedo_koala

define Device/ocedo_raccoon
  ATH_SOC := ar9344
  DEVICE_TITLE := OCEDO Raccoon
  IMAGE_SIZE := 7424k
  IMAGE/sysupgrade.bin := append-kernel | append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += ocedo_raccoon

define Device/ocedo_ursus
  ATH_SOC := qca9558
  DEVICE_TITLE := OCEDO Ursus
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct
  IMAGE_SIZE := 7424k
  IMAGE/sysupgrade.bin := append-kernel | append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += ocedo_ursus

define Device/openmesh_om5p-ac-v2
  ATH_SOC := qca9558
  DEVICE_TITLE := OpenMesh OM5P-AC v2
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca988x-ct om-watchdog
  IMAGE_SIZE := 7808k
  SUPPORTED_DEVICES += om5p-acv2
endef
TARGET_DEVICES += openmesh_om5p-ac-v2

define Device/pcs_cap324
  ATH_SOC := ar9344
  DEVICE_TITLE := PowerCloud Systems CAP324
  IMAGE_SIZE := 16000k
  SUPPORTED_DEVICES += cap324
endef
TARGET_DEVICES += pcs_cap324

define Device/pcs_cr3000
  ATH_SOC := ar9341
  DEVICE_TITLE := PowerCloud Systems CR3000
  IMAGE_SIZE := 7808k
  SUPPORTED_DEVICES += cr3000
endef
TARGET_DEVICES += pcs_cr3000

define Device/pcs_cr5000
  ATH_SOC := ar9344
  DEVICE_TITLE := PowerCloud Systems CR5000
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-core
  IMAGE_SIZE := 7808k
  SUPPORTED_DEVICES += cr5000
endef
TARGET_DEVICES += pcs_cr5000

define Device/netgear_wndr3x00
  ATH_SOC := ar7161
  IMAGE/default := append-kernel | pad-to $$$$(BLOCKSIZE) | netgear-squashfs | append-rootfs | pad-rootfs
  DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-usb-ledtrig-usbport kmod-leds-reset kmod-owl-loader
  $(Device/netgear_ath79)
endef

define Device/netgear_ex7300_ex6400
  ATH_SOC := qca9558
  NETGEAR_KERNEL_MAGIC := 0x27051956
  NETGEAR_BOARD_ID := EX7300series
  NETGEAR_HW_ID := 29765104+16+0+128
  IMAGE_SIZE := 15552k
  IMAGE/default := append-kernel | pad-offset $$$$(BLOCKSIZE) 64 | netgear-rootfs | pad-rootfs
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca99x0-ct
  $(Device/netgear_ath79)
endef

define Device/netgear_ex6400
  $(Device/netgear_ex7300_ex6400)
  DEVICE_TITLE := NETGEAR EX6400
endef
TARGET_DEVICES += netgear_ex6400

define Device/netgear_ex7300
  $(Device/netgear_ex7300_ex6400)
  DEVICE_TITLE := NETGEAR EX7300
endef
TARGET_DEVICES += netgear_ex7300

define Device/netgear_wndr3700
  $(Device/netgear_wndr3x00)
  DEVICE_TITLE := NETGEAR WNDR3700
  NETGEAR_KERNEL_MAGIC := 0x33373030
  NETGEAR_BOARD_ID := WNDR3700
  IMAGE_SIZE := 7680k
  IMAGES += factory-NA.img
  IMAGE/factory-NA.img := $$(IMAGE/default) | netgear-dni NA | check-size $$$$(IMAGE_SIZE)
  SUPPORTED_DEVICES += wndr3700
endef
TARGET_DEVICES += netgear_wndr3700

define Device/netgear_wndr3700v2
  $(Device/netgear_wndr3x00)
  DEVICE_TITLE := NETGEAR WNDR3700v2
  NETGEAR_KERNEL_MAGIC := 0x33373031
  NETGEAR_BOARD_ID := WNDR3700v2
  NETGEAR_HW_ID := 29763654+16+64
  IMAGE_SIZE := 15872k
  SUPPORTED_DEVICES += wndr3700
endef
TARGET_DEVICES += netgear_wndr3700v2

define Device/pisen_wmm003n
  $(Device/tplink-8mlzma)
  ATH_SOC := ar9331
  DEVICE_TITLE := Pisen WMM003N (Cloud Easy Power)
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-chipidea2
  TPLINK_HWID := 0x07030101
endef
TARGET_DEVICES += pisen_wmm003n

define Device/netgear_wndr3800
  $(Device/netgear_wndr3x00)
  DEVICE_TITLE := NETGEAR WNDR3800
  NETGEAR_KERNEL_MAGIC := 0x33373031
  NETGEAR_BOARD_ID := WNDR3800
  NETGEAR_HW_ID := 29763654+16+128
  IMAGE_SIZE := 15872k
  SUPPORTED_DEVICES += wndr3700
endef
TARGET_DEVICES += netgear_wndr3800

define Device/phicomm_k2t
  ATH_SOC := qca9563
  DEVICE_TITLE := Phicomm K2T
  IMAGE_SIZE := 15744k
  IMAGE/sysupgrade.bin := append-kernel | append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
  DEVICE_PACKAGES := kmod-leds-reset kmod-ath10k-ct-smallbuffers ath10k-firmware-qca9888-ct
endef
TARGET_DEVICES += phicomm_k2t

define Device/rosinson_wr818
  ATH_SOC := qca9563
  DEVICE_TITLE := ROSINSON WR818
  IMAGE_SIZE := 15872k
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += rosinson_wr818

define Device/wd_mynet-n750
  $(Device/seama)
  ATH_SOC := ar9344
  DEVICE_TITLE := Western Digital My Net N750
  IMAGE_SIZE := 15872k
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  SEAMA_SIGNATURE := wrgnd13_wd_av
  SUPPORTED_DEVICES += mynet-n750
endef
TARGET_DEVICES += wd_mynet-n750

define Device/wd_mynet-wifi-rangeextender
  ATH_SOC := ar9344
  DEVICE_TITLE := Western Digital My Net Wi-Fi Range Extender
  DEVICE_PACKAGES := rssileds nvram -swconfig
  IMAGE_SIZE := 7808k
  ADDPATTERN_ID := mynet-rext
  ADDPATTERN_VERSION := 1.00.01
  IMAGE/sysupgrade.bin := append-rootfs | pad-rootfs | cybertan-trx | \
	addpattern | append-metadata
  SUPPORTED_DEVICES += mynet-rext
endef
TARGET_DEVICES += wd_mynet-wifi-rangeextender

define Device/winchannel_wb2000
  ATH_SOC := ar9344
  DEVICE_TITLE := Winchannel WB2000
  IMAGE_SIZE := 15872k
  DEVICE_PACKAGES := kmod-i2c-core kmod-i2c-gpio kmod-rtc-ds1307 kmod-usb2 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += winchannel_wb2000

define Device/xiaomi_mi-router-4q
  ATH_SOC := qca9561
  DEVICE_TITLE := Xiaomi Mi Router 4Q
  IMAGE_SIZE := 14336k
endef
TARGET_DEVICES += xiaomi_mi-router-4q

define Device/yuncore_a770
  ATH_SOC := qca9531
  DEVICE_TITLE := YunCore A770
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca9887-ct
  IMAGE_SIZE := 16000k
  IMAGES += tftp.bin
  IMAGE/tftp.bin := $$(IMAGE/sysupgrade.bin) | yuncore-tftp-header-16m
endef
TARGET_DEVICES += yuncore_a770

define Device/yuncore_a782
  ATH_SOC := qca9563
  DEVICE_TITLE := YunCore A782
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca9888-ct
  IMAGE_SIZE := 16000k
  IMAGES += tftp.bin
  IMAGE/tftp.bin := $$(IMAGE/sysupgrade.bin) | yuncore-tftp-header-16m
endef
TARGET_DEVICES += yuncore_a782

define Device/yuncore_xd4200
  ATH_SOC := qca9563
  DEVICE_TITLE := YunCore XD4200
  DEVICE_PACKAGES := kmod-ath10k-ct ath10k-firmware-qca9888-ct
  IMAGE_SIZE := 16000k
  IMAGES += tftp.bin
  IMAGE/tftp.bin := $$(IMAGE/sysupgrade.bin) | yuncore-tftp-header-16m
endef
TARGET_DEVICES += yuncore_xd4200

define Device/zbtlink_zbt-wd323
  ATH_SOC := ar9344
  DEVICE_TITLE := ZBT WD323
  IMAGE_SIZE := 16000k
  DEVICE_PACKAGES := kmod-usb2 kmod-i2c-core kmod-i2c-gpio kmod-rtc-pcf8563 \
		     kmod-usb-serial kmod-usb-serial-cp210x uqmi
endef
TARGET_DEVICES += zbtlink_zbt-wd323
