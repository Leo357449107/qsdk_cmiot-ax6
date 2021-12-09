ARCH:=x86_64
BOARDNAME:=x86_64
FEATURES += audio pci pcie usb
DEFAULT_PACKAGES += kmod-e1000e kmod-e1000 kmod-igb kmod-bnx2

define Target/Description
        Build images for 64 bit systems including virtualized guests.
endef
