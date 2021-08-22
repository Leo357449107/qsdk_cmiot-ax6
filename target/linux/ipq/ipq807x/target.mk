
SUBTARGET:=ipq807x
BOARDNAME:=QCA IPQ807x(32bit) based boards
CPU_TYPE:=cortex-a7

DEFAULT_PACKAGES += \
	uboot-2016-ipq807x fwupgrade-tools kmod-usb-phy-ipq807x lk-ipq807x \
	uboot-2016-ipq807x_tiny kmod-usb-dwc3-of-simple

define Target/Description
	Build firmware image for IPQ807x SoC devices.
endef
