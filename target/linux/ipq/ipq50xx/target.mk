
SUBTARGET:=ipq50xx
BOARDNAME:=QCA IPQ50XX(32bit) based boards
CPU_TYPE:=cortex-a7

DEFAULT_PACKAGES += \
	uboot-2016-ipq5018 uboot-2016-ipq5018_tiny uboot-2016-ipq5018_tiny_debug fwupgrade-tools \
	kmod-usb-phy-ipq5018 kmod-usb-dwc3-qcom kmod-bt_tty kmod-clk-test

define Target/Description
	Build firmware image for IPQ50xx SoC devices.
endef
