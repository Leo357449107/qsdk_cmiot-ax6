/*
 * Copyright (c) 2014 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <inttypes.h>
#include <pci.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <dm/device-internal.h>
#if defined(CONFIG_X86) && defined(CONFIG_HAVE_FSP)
#include <asm/fsp/fsp_support.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static int pci_get_bus(int busnum, struct udevice **busp)
{
	int ret;

	ret = uclass_get_device_by_seq(UCLASS_PCI, busnum, busp);

	/* Since buses may not be numbered yet try a little harder with bus 0 */
	if (ret == -ENODEV) {
		ret = uclass_first_device(UCLASS_PCI, busp);
		if (ret)
			return ret;
		else if (!*busp)
			return -ENODEV;
		ret = uclass_get_device_by_seq(UCLASS_PCI, busnum, busp);
	}

	return ret;
}

struct pci_controller *pci_bus_to_hose(int busnum)
{
	struct udevice *bus;
	int ret;

	ret = pci_get_bus(busnum, &bus);
	if (ret) {
		debug("%s: Cannot get bus %d: ret=%d\n", __func__, busnum, ret);
		return NULL;
	}

	return dev_get_uclass_priv(bus);
}

struct udevice *pci_get_controller(struct udevice *dev)
{
	while (device_is_on_pci_bus(dev))
		dev = dev->parent;

	return dev;
}

pci_dev_t pci_get_bdf(struct udevice *dev)
{
	struct pci_child_platdata *pplat = dev_get_parent_platdata(dev);
	struct udevice *bus = dev->parent;

	return PCI_ADD_BUS(bus->seq, pplat->devfn);
}

/**
 * pci_get_bus_max() - returns the bus number of the last active bus
 *
 * @return last bus number, or -1 if no active buses
 */
static int pci_get_bus_max(void)
{
	struct udevice *bus;
	struct uclass *uc;
	int ret = -1;

	ret = uclass_get(UCLASS_PCI, &uc);
	uclass_foreach_dev(bus, uc) {
		if (bus->seq > ret)
			ret = bus->seq;
	}

	debug("%s: ret=%d\n", __func__, ret);

	return ret;
}

int pci_last_busno(void)
{
	return pci_get_bus_max();
}

int pci_get_ff(enum pci_size_t size)
{
	switch (size) {
	case PCI_SIZE_8:
		return 0xff;
	case PCI_SIZE_16:
		return 0xffff;
	default:
		return 0xffffffff;
	}
}

int pci_bus_find_devfn(struct udevice *bus, pci_dev_t find_devfn,
		       struct udevice **devp)
{
	struct udevice *dev;

	for (device_find_first_child(bus, &dev);
	     dev;
	     device_find_next_child(&dev)) {
		struct pci_child_platdata *pplat;

		pplat = dev_get_parent_platdata(dev);
		if (pplat && pplat->devfn == find_devfn) {
			*devp = dev;
			return 0;
		}
	}

	return -ENODEV;
}

int pci_bus_find_bdf(pci_dev_t bdf, struct udevice **devp)
{
	struct udevice *bus;
	int ret;

	ret = pci_get_bus(PCI_BUS(bdf), &bus);
	if (ret)
		return ret;
	return pci_bus_find_devfn(bus, PCI_MASK_BUS(bdf), devp);
}

static int pci_device_matches_ids(struct udevice *dev,
				  struct pci_device_id *ids)
{
	struct pci_child_platdata *pplat;
	int i;

	pplat = dev_get_parent_platdata(dev);
	if (!pplat)
		return -EINVAL;
	for (i = 0; ids[i].vendor != 0; i++) {
		if (pplat->vendor == ids[i].vendor &&
		    pplat->device == ids[i].device)
			return i;
	}

	return -EINVAL;
}

int pci_bus_find_devices(struct udevice *bus, struct pci_device_id *ids,
			 int *indexp, struct udevice **devp)
{
	struct udevice *dev;

	/* Scan all devices on this bus */
	for (device_find_first_child(bus, &dev);
	     dev;
	     device_find_next_child(&dev)) {
		if (pci_device_matches_ids(dev, ids) >= 0) {
			if ((*indexp)-- <= 0) {
				*devp = dev;
				return 0;
			}
		}
	}

	return -ENODEV;
}

int pci_find_device_id(struct pci_device_id *ids, int index,
		       struct udevice **devp)
{
	struct udevice *bus;

	/* Scan all known buses */
	for (uclass_first_device(UCLASS_PCI, &bus);
	     bus;
	     uclass_next_device(&bus)) {
		if (!pci_bus_find_devices(bus, ids, &index, devp))
			return 0;
	}
	*devp = NULL;

	return -ENODEV;
}

int pci_bus_write_config(struct udevice *bus, pci_dev_t bdf, int offset,
			 unsigned long value, enum pci_size_t size)
{
	struct dm_pci_ops *ops;

	ops = pci_get_ops(bus);
	if (!ops->write_config)
		return -ENOSYS;
	return ops->write_config(bus, bdf, offset, value, size);
}

int pci_write_config(pci_dev_t bdf, int offset, unsigned long value,
		     enum pci_size_t size)
{
	struct udevice *bus;
	int ret;

	ret = pci_get_bus(PCI_BUS(bdf), &bus);
	if (ret)
		return ret;

	return pci_bus_write_config(bus, bdf, offset, value, size);
}

int dm_pci_write_config(struct udevice *dev, int offset, unsigned long value,
			enum pci_size_t size)
{
	struct udevice *bus;

	for (bus = dev; device_is_on_pci_bus(bus);)
		bus = bus->parent;
	return pci_bus_write_config(bus, pci_get_bdf(dev), offset, value, size);
}


int pci_write_config32(pci_dev_t bdf, int offset, u32 value)
{
	return pci_write_config(bdf, offset, value, PCI_SIZE_32);
}

int pci_write_config16(pci_dev_t bdf, int offset, u16 value)
{
	return pci_write_config(bdf, offset, value, PCI_SIZE_16);
}

int pci_write_config8(pci_dev_t bdf, int offset, u8 value)
{
	return pci_write_config(bdf, offset, value, PCI_SIZE_8);
}

int dm_pci_write_config8(struct udevice *dev, int offset, u8 value)
{
	return dm_pci_write_config(dev, offset, value, PCI_SIZE_8);
}

int dm_pci_write_config16(struct udevice *dev, int offset, u16 value)
{
	return dm_pci_write_config(dev, offset, value, PCI_SIZE_16);
}

int dm_pci_write_config32(struct udevice *dev, int offset, u32 value)
{
	return dm_pci_write_config(dev, offset, value, PCI_SIZE_32);
}

int pci_bus_read_config(struct udevice *bus, pci_dev_t bdf, int offset,
			unsigned long *valuep, enum pci_size_t size)
{
	struct dm_pci_ops *ops;

	ops = pci_get_ops(bus);
	if (!ops->read_config)
		return -ENOSYS;
	return ops->read_config(bus, bdf, offset, valuep, size);
}

int pci_read_config(pci_dev_t bdf, int offset, unsigned long *valuep,
		    enum pci_size_t size)
{
	struct udevice *bus;
	int ret;

	ret = pci_get_bus(PCI_BUS(bdf), &bus);
	if (ret)
		return ret;

	return pci_bus_read_config(bus, bdf, offset, valuep, size);
}

int dm_pci_read_config(struct udevice *dev, int offset, unsigned long *valuep,
		       enum pci_size_t size)
{
	struct udevice *bus;

	for (bus = dev; device_is_on_pci_bus(bus);)
		bus = bus->parent;
	return pci_bus_read_config(bus, pci_get_bdf(dev), offset, valuep,
				   size);
}

int pci_read_config32(pci_dev_t bdf, int offset, u32 *valuep)
{
	unsigned long value;
	int ret;

	ret = pci_read_config(bdf, offset, &value, PCI_SIZE_32);
	if (ret)
		return ret;
	*valuep = value;

	return 0;
}

int pci_read_config16(pci_dev_t bdf, int offset, u16 *valuep)
{
	unsigned long value;
	int ret;

	ret = pci_read_config(bdf, offset, &value, PCI_SIZE_16);
	if (ret)
		return ret;
	*valuep = value;

	return 0;
}

int pci_read_config8(pci_dev_t bdf, int offset, u8 *valuep)
{
	unsigned long value;
	int ret;

	ret = pci_read_config(bdf, offset, &value, PCI_SIZE_8);
	if (ret)
		return ret;
	*valuep = value;

	return 0;
}

int dm_pci_read_config8(struct udevice *dev, int offset, u8 *valuep)
{
	unsigned long value;
	int ret;

	ret = dm_pci_read_config(dev, offset, &value, PCI_SIZE_8);
	if (ret)
		return ret;
	*valuep = value;

	return 0;
}

int dm_pci_read_config16(struct udevice *dev, int offset, u16 *valuep)
{
	unsigned long value;
	int ret;

	ret = dm_pci_read_config(dev, offset, &value, PCI_SIZE_16);
	if (ret)
		return ret;
	*valuep = value;

	return 0;
}

int dm_pci_read_config32(struct udevice *dev, int offset, u32 *valuep)
{
	unsigned long value;
	int ret;

	ret = dm_pci_read_config(dev, offset, &value, PCI_SIZE_32);
	if (ret)
		return ret;
	*valuep = value;

	return 0;
}

static void set_vga_bridge_bits(struct udevice *dev)
{
	struct udevice *parent = dev->parent;
	u16 bc;

	while (parent->seq != 0) {
		dm_pci_read_config16(parent, PCI_BRIDGE_CONTROL, &bc);
		bc |= PCI_BRIDGE_CTL_VGA;
		dm_pci_write_config16(parent, PCI_BRIDGE_CONTROL, bc);
		parent = parent->parent;
	}
}

int pci_auto_config_devices(struct udevice *bus)
{
	struct pci_controller *hose = bus->uclass_priv;
	struct pci_child_platdata *pplat;
	unsigned int sub_bus;
	struct udevice *dev;
	int ret;

	sub_bus = bus->seq;
	debug("%s: start\n", __func__);
	pciauto_config_init(hose);
	for (ret = device_find_first_child(bus, &dev);
	     !ret && dev;
	     ret = device_find_next_child(&dev)) {
		unsigned int max_bus;
		int ret;

		debug("%s: device %s\n", __func__, dev->name);
		ret = pciauto_config_device(hose, pci_get_bdf(dev));
		if (ret < 0)
			return ret;
		max_bus = ret;
		sub_bus = max(sub_bus, max_bus);

		pplat = dev_get_parent_platdata(dev);
		if (pplat->class == (PCI_CLASS_DISPLAY_VGA << 8))
			set_vga_bridge_bits(dev);
	}
	debug("%s: done\n", __func__);

	return sub_bus;
}

int dm_pci_hose_probe_bus(struct pci_controller *hose, pci_dev_t bdf)
{
	struct udevice *parent, *bus;
	int sub_bus;
	int ret;

	debug("%s\n", __func__);
	parent = hose->bus;

	/* Find the bus within the parent */
	ret = pci_bus_find_devfn(parent, PCI_MASK_BUS(bdf), &bus);
	if (ret) {
		debug("%s: Cannot find device %x on bus %s: %d\n", __func__,
		      bdf, parent->name, ret);
		return ret;
	}

	sub_bus = pci_get_bus_max() + 1;
	debug("%s: bus = %d/%s\n", __func__, sub_bus, bus->name);
	pciauto_prescan_setup_bridge(hose, bdf, sub_bus);

	ret = device_probe(bus);
	if (ret) {
		debug("%s: Cannot probe bus %s: %d\n", __func__, bus->name,
		      ret);
		return ret;
	}
	if (sub_bus != bus->seq) {
		printf("%s: Internal error, bus '%s' got seq %d, expected %d\n",
		       __func__, bus->name, bus->seq, sub_bus);
		return -EPIPE;
	}
	sub_bus = pci_get_bus_max();
	pciauto_postscan_setup_bridge(hose, bdf, sub_bus);

	return sub_bus;
}

/**
 * pci_match_one_device - Tell if a PCI device structure has a matching
 *                        PCI device id structure
 * @id: single PCI device id structure to match
 * @dev: the PCI device structure to match against
 *
 * Returns the matching pci_device_id structure or %NULL if there is no match.
 */
static bool pci_match_one_id(const struct pci_device_id *id,
			     const struct pci_device_id *find)
{
	if ((id->vendor == PCI_ANY_ID || id->vendor == find->vendor) &&
	    (id->device == PCI_ANY_ID || id->device == find->device) &&
	    (id->subvendor == PCI_ANY_ID || id->subvendor == find->subvendor) &&
	    (id->subdevice == PCI_ANY_ID || id->subdevice == find->subdevice) &&
	    !((id->class ^ find->class) & id->class_mask))
		return true;

	return false;
}

/**
 * pci_find_and_bind_driver() - Find and bind the right PCI driver
 *
 * This only looks at certain fields in the descriptor.
 *
 * @parent:	Parent bus
 * @find_id:	Specification of the driver to find
 * @bdf:	Bus/device/function addreess - see PCI_BDF()
 * @devp:	Returns a pointer to the device created
 * @return 0 if OK, -EPERM if the device is not needed before relocation and
 *	   therefore was not created, other -ve value on error
 */
static int pci_find_and_bind_driver(struct udevice *parent,
				    struct pci_device_id *find_id,
				    pci_dev_t bdf, struct udevice **devp)
{
	struct pci_driver_entry *start, *entry;
	const char *drv;
	int n_ents;
	int ret;
	char name[30], *str;
	bool bridge;

	*devp = NULL;

	debug("%s: Searching for driver: vendor=%x, device=%x\n", __func__,
	      find_id->vendor, find_id->device);
	start = ll_entry_start(struct pci_driver_entry, pci_driver_entry);
	n_ents = ll_entry_count(struct pci_driver_entry, pci_driver_entry);
	for (entry = start; entry != start + n_ents; entry++) {
		const struct pci_device_id *id;
		struct udevice *dev;
		const struct driver *drv;

		for (id = entry->match;
		     id->vendor || id->subvendor || id->class_mask;
		     id++) {
			if (!pci_match_one_id(id, find_id))
				continue;

			drv = entry->driver;

			/*
			 * In the pre-relocation phase, we only bind devices
			 * whose driver has the DM_FLAG_PRE_RELOC set, to save
			 * precious memory space as on some platforms as that
			 * space is pretty limited (ie: using Cache As RAM).
			 */
			if (!(gd->flags & GD_FLG_RELOC) &&
			    !(drv->flags & DM_FLAG_PRE_RELOC))
				return -EPERM;

			/*
			 * We could pass the descriptor to the driver as
			 * platdata (instead of NULL) and allow its bind()
			 * method to return -ENOENT if it doesn't support this
			 * device. That way we could continue the search to
			 * find another driver. For now this doesn't seem
			 * necesssary, so just bind the first match.
			 */
			ret = device_bind(parent, drv, drv->name, NULL, -1,
					  &dev);
			if (ret)
				goto error;
			debug("%s: Match found: %s\n", __func__, drv->name);
			dev->driver_data = find_id->driver_data;
			*devp = dev;
			return 0;
		}
	}

	bridge = (find_id->class >> 8) == PCI_CLASS_BRIDGE_PCI;
	/*
	 * In the pre-relocation phase, we only bind bridge devices to save
	 * precious memory space as on some platforms as that space is pretty
	 * limited (ie: using Cache As RAM).
	 */
	if (!(gd->flags & GD_FLG_RELOC) && !bridge)
		return -EPERM;

	/* Bind a generic driver so that the device can be used */
	sprintf(name, "pci_%x:%x.%x", parent->seq, PCI_DEV(bdf),
		PCI_FUNC(bdf));
	str = strdup(name);
	if (!str)
		return -ENOMEM;
	drv = bridge ? "pci_bridge_drv" : "pci_generic_drv";

	ret = device_bind_driver(parent, drv, str, devp);
	if (ret) {
		debug("%s: Failed to bind generic driver: %d\n", __func__, ret);
		return ret;
	}
	debug("%s: No match found: bound generic driver instead\n", __func__);

	return 0;

error:
	debug("%s: No match found: error %d\n", __func__, ret);
	return ret;
}

int pci_bind_bus_devices(struct udevice *bus)
{
	ulong vendor, device;
	ulong header_type;
	pci_dev_t bdf, end;
	bool found_multi;
	int ret;

	found_multi = false;
	end = PCI_BDF(bus->seq, PCI_MAX_PCI_DEVICES - 1,
		      PCI_MAX_PCI_FUNCTIONS - 1);
	for (bdf = PCI_BDF(bus->seq, 0, 0); bdf < end;
	     bdf += PCI_BDF(0, 0, 1)) {
		struct pci_child_platdata *pplat;
		struct udevice *dev;
		ulong class;

		if (PCI_FUNC(bdf) && !found_multi)
			continue;
		/* Check only the first access, we don't expect problems */
		ret = pci_bus_read_config(bus, bdf, PCI_HEADER_TYPE,
					  &header_type, PCI_SIZE_8);
		if (ret)
			goto error;
		pci_bus_read_config(bus, bdf, PCI_VENDOR_ID, &vendor,
				    PCI_SIZE_16);
		if (vendor == 0xffff || vendor == 0x0000)
			continue;

		if (!PCI_FUNC(bdf))
			found_multi = header_type & 0x80;

		debug("%s: bus %d/%s: found device %x, function %d\n", __func__,
		      bus->seq, bus->name, PCI_DEV(bdf), PCI_FUNC(bdf));
		pci_bus_read_config(bus, bdf, PCI_DEVICE_ID, &device,
				    PCI_SIZE_16);
		pci_bus_read_config(bus, bdf, PCI_CLASS_REVISION, &class,
				    PCI_SIZE_32);
		class >>= 8;

		/* Find this device in the device tree */
		ret = pci_bus_find_devfn(bus, PCI_MASK_BUS(bdf), &dev);

		/* Search for a driver */

		/* If nothing in the device tree, bind a generic device */
		if (ret == -ENODEV) {
			struct pci_device_id find_id;
			ulong val;

			memset(&find_id, '\0', sizeof(find_id));
			find_id.vendor = vendor;
			find_id.device = device;
			find_id.class = class;
			if ((header_type & 0x7f) == PCI_HEADER_TYPE_NORMAL) {
				pci_bus_read_config(bus, bdf,
						    PCI_SUBSYSTEM_VENDOR_ID,
						    &val, PCI_SIZE_32);
				find_id.subvendor = val & 0xffff;
				find_id.subdevice = val >> 16;
			}
			ret = pci_find_and_bind_driver(bus, &find_id, bdf,
						       &dev);
		}
		if (ret == -EPERM)
			continue;
		else if (ret)
			return ret;

		/* Update the platform data */
		pplat = dev_get_parent_platdata(dev);
		pplat->devfn = PCI_MASK_BUS(bdf);
		pplat->vendor = vendor;
		pplat->device = device;
		pplat->class = class;
	}

	return 0;
error:
	printf("Cannot read bus configuration: %d\n", ret);

	return ret;
}

static int pci_uclass_post_bind(struct udevice *bus)
{
	/*
	 * If there is no pci device listed in the device tree,
	 * don't bother scanning the device tree.
	 */
	if (bus->of_offset == -1)
		return 0;

	/*
	 * Scan the device tree for devices. This does not probe the PCI bus,
	 * as this is not permitted while binding. It just finds devices
	 * mentioned in the device tree.
	 *
	 * Before relocation, only bind devices marked for pre-relocation
	 * use.
	 */
	return dm_scan_fdt_node(bus, gd->fdt_blob, bus->of_offset,
				gd->flags & GD_FLG_RELOC ? false : true);
}

static int decode_regions(struct pci_controller *hose, const void *blob,
			  int parent_node, int node)
{
	int pci_addr_cells, addr_cells, size_cells;
	phys_addr_t base = 0, size;
	int cells_per_record;
	const u32 *prop;
	int len;
	int i;

	prop = fdt_getprop(blob, node, "ranges", &len);
	if (!prop)
		return -EINVAL;
	pci_addr_cells = fdt_address_cells(blob, node);
	addr_cells = fdt_address_cells(blob, parent_node);
	size_cells = fdt_size_cells(blob, node);

	/* PCI addresses are always 3-cells */
	len /= sizeof(u32);
	cells_per_record = pci_addr_cells + addr_cells + size_cells;
	hose->region_count = 0;
	debug("%s: len=%d, cells_per_record=%d\n", __func__, len,
	      cells_per_record);
	for (i = 0; i < MAX_PCI_REGIONS; i++, len -= cells_per_record) {
		u64 pci_addr, addr, size;
		int space_code;
		u32 flags;
		int type;
		int pos;

		if (len < cells_per_record)
			break;
		flags = fdt32_to_cpu(prop[0]);
		space_code = (flags >> 24) & 3;
		pci_addr = fdtdec_get_number(prop + 1, 2);
		prop += pci_addr_cells;
		addr = fdtdec_get_number(prop, addr_cells);
		prop += addr_cells;
		size = fdtdec_get_number(prop, size_cells);
		prop += size_cells;
		debug("%s: region %d, pci_addr=%" PRIx64 ", addr=%" PRIx64
		      ", size=%" PRIx64 ", space_code=%d\n", __func__,
		      hose->region_count, pci_addr, addr, size, space_code);
		if (space_code & 2) {
			type = flags & (1U << 30) ? PCI_REGION_PREFETCH :
					PCI_REGION_MEM;
		} else if (space_code & 1) {
			type = PCI_REGION_IO;
		} else {
			continue;
		}
		pos = -1;
		for (i = 0; i < hose->region_count; i++) {
			if (hose->regions[i].flags == type)
				pos = i;
		}
		if (pos == -1)
			pos = hose->region_count++;
		debug(" - type=%d, pos=%d\n", type, pos);
		pci_set_region(hose->regions + pos, pci_addr, addr, size, type);
	}

	/* Add a region for our local memory */
	size = gd->ram_size;
#ifdef CONFIG_SYS_SDRAM_BASE
	base = CONFIG_SYS_SDRAM_BASE;
#endif
	if (gd->pci_ram_top && gd->pci_ram_top < base + size)
		size = gd->pci_ram_top - base;
	pci_set_region(hose->regions + hose->region_count++, base, base,
		       size, PCI_REGION_MEM | PCI_REGION_SYS_MEMORY);

	return 0;
}

static int pci_uclass_pre_probe(struct udevice *bus)
{
	struct pci_controller *hose;
	int ret;

	debug("%s, bus=%d/%s, parent=%s\n", __func__, bus->seq, bus->name,
	      bus->parent->name);
	hose = bus->uclass_priv;

	/* For bridges, use the top-level PCI controller */
	if (device_get_uclass_id(bus->parent) == UCLASS_ROOT) {
		hose->ctlr = bus;
		ret = decode_regions(hose, gd->fdt_blob, bus->parent->of_offset,
				bus->of_offset);
		if (ret) {
			debug("%s: Cannot decode regions\n", __func__);
			return ret;
		}
	} else {
		struct pci_controller *parent_hose;

		parent_hose = dev_get_uclass_priv(bus->parent);
		hose->ctlr = parent_hose->bus;
	}
	hose->bus = bus;
	hose->first_busno = bus->seq;
	hose->last_busno = bus->seq;

	return 0;
}

static int pci_uclass_post_probe(struct udevice *bus)
{
	int ret;

	debug("%s: probing bus %d\n", __func__, bus->seq);
	ret = pci_bind_bus_devices(bus);
	if (ret)
		return ret;

#ifdef CONFIG_PCI_PNP
	ret = pci_auto_config_devices(bus);
	if (ret < 0)
		return ret;
#endif

#if defined(CONFIG_X86) && defined(CONFIG_HAVE_FSP)
	/*
	 * Per Intel FSP specification, we should call FSP notify API to
	 * inform FSP that PCI enumeration has been done so that FSP will
	 * do any necessary initialization as required by the chipset's
	 * BIOS Writer's Guide (BWG).
	 *
	 * Unfortunately we have to put this call here as with driver model,
	 * the enumeration is all done on a lazy basis as needed, so until
	 * something is touched on PCI it won't happen.
	 *
	 * Note we only call this 1) after U-Boot is relocated, and 2)
	 * root bus has finished probing.
	 */
	if ((gd->flags & GD_FLG_RELOC) && (bus->seq == 0)) {
		ret = fsp_init_phase_pci();
		if (ret)
			return ret;
	}
#endif

	return 0;
}

static int pci_uclass_child_post_bind(struct udevice *dev)
{
	struct pci_child_platdata *pplat;
	struct fdt_pci_addr addr;
	int ret;

	if (dev->of_offset == -1)
		return 0;

	/*
	 * We could read vendor, device, class if available. But for now we
	 * just check the address.
	 */
	pplat = dev_get_parent_platdata(dev);
	ret = fdtdec_get_pci_addr(gd->fdt_blob, dev->of_offset,
				  FDT_PCI_SPACE_CONFIG, "reg", &addr);

	if (ret) {
		if (ret != -ENOENT)
			return -EINVAL;
	} else {
		/* extract the devfn from fdt_pci_addr */
		pplat->devfn = addr.phys_hi & 0xff00;
	}

	return 0;
}

static int pci_bridge_read_config(struct udevice *bus, pci_dev_t bdf,
				  uint offset, ulong *valuep,
				  enum pci_size_t size)
{
	struct pci_controller *hose = bus->uclass_priv;

	return pci_bus_read_config(hose->ctlr, bdf, offset, valuep, size);
}

static int pci_bridge_write_config(struct udevice *bus, pci_dev_t bdf,
				   uint offset, ulong value,
				   enum pci_size_t size)
{
	struct pci_controller *hose = bus->uclass_priv;

	return pci_bus_write_config(hose->ctlr, bdf, offset, value, size);
}

static int skip_to_next_device(struct udevice *bus, struct udevice **devp)
{
	struct udevice *dev;
	int ret = 0;

	/*
	 * Scan through all the PCI controllers. On x86 there will only be one
	 * but that is not necessarily true on other hardware.
	 */
	do {
		device_find_first_child(bus, &dev);
		if (dev) {
			*devp = dev;
			return 0;
		}
		ret = uclass_next_device(&bus);
		if (ret)
			return ret;
	} while (bus);

	return 0;
}

int pci_find_next_device(struct udevice **devp)
{
	struct udevice *child = *devp;
	struct udevice *bus = child->parent;
	int ret;

	/* First try all the siblings */
	*devp = NULL;
	while (child) {
		device_find_next_child(&child);
		if (child) {
			*devp = child;
			return 0;
		}
	}

	/* We ran out of siblings. Try the next bus */
	ret = uclass_next_device(&bus);
	if (ret)
		return ret;

	return bus ? skip_to_next_device(bus, devp) : 0;
}

int pci_find_first_device(struct udevice **devp)
{
	struct udevice *bus;
	int ret;

	*devp = NULL;
	ret = uclass_first_device(UCLASS_PCI, &bus);
	if (ret)
		return ret;

	return skip_to_next_device(bus, devp);
}

ulong pci_conv_32_to_size(ulong value, uint offset, enum pci_size_t size)
{
	switch (size) {
	case PCI_SIZE_8:
		return (value >> ((offset & 3) * 8)) & 0xff;
	case PCI_SIZE_16:
		return (value >> ((offset & 2) * 8)) & 0xffff;
	default:
		return value;
	}
}

ulong pci_conv_size_to_32(ulong old, ulong value, uint offset,
			  enum pci_size_t size)
{
	uint off_mask;
	uint val_mask, shift;
	ulong ldata, mask;

	switch (size) {
	case PCI_SIZE_8:
		off_mask = 3;
		val_mask = 0xff;
		break;
	case PCI_SIZE_16:
		off_mask = 2;
		val_mask = 0xffff;
		break;
	default:
		return value;
	}
	shift = (offset & off_mask) * 8;
	ldata = (value & val_mask) << shift;
	mask = val_mask << shift;
	value = (old & ~mask) | ldata;

	return value;
}

int pci_get_regions(struct udevice *dev, struct pci_region **iop,
		    struct pci_region **memp, struct pci_region **prefp)
{
	struct udevice *bus = pci_get_controller(dev);
	struct pci_controller *hose = dev_get_uclass_priv(bus);
	int i;

	*iop = NULL;
	*memp = NULL;
	*prefp = NULL;
	for (i = 0; i < hose->region_count; i++) {
		switch (hose->regions[i].flags) {
		case PCI_REGION_IO:
			if (!*iop || (*iop)->size < hose->regions[i].size)
				*iop = hose->regions + i;
			break;
		case PCI_REGION_MEM:
			if (!*memp || (*memp)->size < hose->regions[i].size)
				*memp = hose->regions + i;
			break;
		case (PCI_REGION_MEM | PCI_REGION_PREFETCH):
			if (!*prefp || (*prefp)->size < hose->regions[i].size)
				*prefp = hose->regions + i;
			break;
		}
	}

	return (*iop != NULL) + (*memp != NULL) + (*prefp != NULL);
}

UCLASS_DRIVER(pci) = {
	.id		= UCLASS_PCI,
	.name		= "pci",
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
	.post_bind	= pci_uclass_post_bind,
	.pre_probe	= pci_uclass_pre_probe,
	.post_probe	= pci_uclass_post_probe,
	.child_post_bind = pci_uclass_child_post_bind,
	.per_device_auto_alloc_size = sizeof(struct pci_controller),
	.per_child_platdata_auto_alloc_size =
			sizeof(struct pci_child_platdata),
};

static const struct dm_pci_ops pci_bridge_ops = {
	.read_config	= pci_bridge_read_config,
	.write_config	= pci_bridge_write_config,
};

static const struct udevice_id pci_bridge_ids[] = {
	{ .compatible = "pci-bridge" },
	{ }
};

U_BOOT_DRIVER(pci_bridge_drv) = {
	.name		= "pci_bridge_drv",
	.id		= UCLASS_PCI,
	.of_match	= pci_bridge_ids,
	.ops		= &pci_bridge_ops,
};

UCLASS_DRIVER(pci_generic) = {
	.id		= UCLASS_PCI_GENERIC,
	.name		= "pci_generic",
};

static const struct udevice_id pci_generic_ids[] = {
	{ .compatible = "pci-generic" },
	{ }
};

U_BOOT_DRIVER(pci_generic_drv) = {
	.name		= "pci_generic_drv",
	.id		= UCLASS_PCI_GENERIC,
	.of_match	= pci_generic_ids,
};
