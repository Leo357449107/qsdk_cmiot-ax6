
ARCH:=aarch64
CPU_TYPE:=cortex-a53
SUBTARGET:=ipq60xx_64
BOARDNAME:=QCA IPQ60XX(64bit) based boards
KERNELNAME:=Image dtbs

DEFAULT_PACKAGES += \
	sysupgrade-helper kmod-usb-phy-ipq807x kmod-usb-dwc3-qcom

define Target/Description
        Build images for IPQ60xx 64 bit system.
endef
