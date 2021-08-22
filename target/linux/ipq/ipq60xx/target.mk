
SUBTARGET:=ipq60xx
BOARDNAME:=QCA IPQ60xx(32bit) based boards
CPU_TYPE:=cortex-a7

DEFAULT_PACKAGES += \
	uboot-2016-ipq6018 fwupgrade-tools kmod-usb-phy-ipq807x \
	kmod-usb-dwc3-qcom lk-ipq6018

define Target/Description
	Build firmware image for IPQ60xx SoC devices.
endef
