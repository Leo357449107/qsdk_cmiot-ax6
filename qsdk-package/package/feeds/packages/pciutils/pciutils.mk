#
# Recipe extension for pciutils
#

PCI_IDS_FILE:=v$(PCI_IDS_VER).tar.gz

define Download/pci_ids
  FILE:=$(PCI_IDS_FILE)
  URL_FILE:=
  URL:=https://github.com/vcrhonek/hwdata/archive/
  HASH:=711e5e6f1d9397604ebe68693a060eb9b99df8a84d20ecfc0efa2b3be0d32d50
endef

define pciutils_prepare
	$(TAR) xzvf $(DL_DIR)/$(PCI_IDS_FILE) -C $(PKG_BUILD_DIR)
	$(CP) $(PKG_BUILD_DIR)/hwdata-$(PCI_IDS_VER)/pci.ids $(PKG_BUILD_DIR)/pci.ids
endef

Build/Prepare += $(newline)$(pciutils_prepare)
