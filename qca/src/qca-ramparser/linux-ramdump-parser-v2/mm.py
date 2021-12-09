# Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

from array import *


def page_buddy(ramdump, page):
    mapcount_offset = ramdump.field_offset('struct page', '_mapcount')
    val = ramdump.read_int(page + mapcount_offset)
    # -128 is the magic for in the buddy allocator
    return val == 0xffffff80


def page_zonenum(page_flags):
    # save this in a variable somewhere...
    return (page_flags >> 26) & 3


def page_to_nid(page_flags):
    return 0


def page_zone(ramdump, page):
    contig_page_data = ramdump.addr_lookup('contig_page_data')
    node_zones_offset = ramdump.field_offset(
        'struct pglist_data', 'node_zones')
    page_flags_offset = ramdump.field_offset('struct page', 'flags')
    zone_size = ramdump.sizeof('struct zone')
    page_flags = ramdump.read_word(page + page_flags_offset)
    if page_flags is None:
        return None
    zone = contig_page_data + node_zones_offset + \
        (page_zonenum(page_flags) * zone_size)
    return zone


def zone_is_highmem(ramdump, zone):
    if not ramdump.CONFIG_HIGHMEM:
        return False

    if zone is None:
        return False
    # not at all how linux does it but it works for our purposes...
    zone_name_offset = ramdump.field_offset('struct zone', 'name')
    zone_name_addr = ramdump.read_word(zone + zone_name_offset)
    if zone_name_addr is None:
        return False
    zone_name = ramdump.read_cstring(zone_name_addr, 48)
    if zone_name is None:
        # XXX do something?
        return False
    if zone_name == 'HighMem':
        return True
    else:
        return False


def hash32(val, bits):
    chash = c_uint(val * 0x9e370001).value
    return chash >> (32 - bits)


def page_slot(ramdump, page):
    hashed = hash32(page, 7)
    htable = ramdump.addr_lookup('page_address_htable')
    htable_size = ramdump.sizeof('page_address_htable[0]')
    return htable + htable_size * hashed


def page_to_section(page_flags):
    # again savefn8n variable
    return (page_flags >> 28) & 0xF


def nr_to_section(ramdump, sec_num):
    memsection_struct_size = ramdump.sizeof('struct mem_section')
    sections_per_root = 4096 / memsection_struct_size
    sect_nr_to_root = sec_num / sections_per_root
    masked = sec_num & (sections_per_root - 1)
    mem_section_addr = ramdump.addr_lookup('mem_section')
    mem_section = ramdump.read_word(mem_section_addr)
    if mem_section is None:
        return None
    return mem_section + memsection_struct_size * (sect_nr_to_root * sections_per_root + masked)


def section_mem_map_addr(ramdump, section):
    map_offset = ramdump.field_offset('struct mem_section', 'section_mem_map')
    result = ramdump.read_word(section + map_offset)
    return result & ~((1 << 2) - 1)


def pfn_to_section_nr(pfn):
    return pfn >> (30 - 12)


def pfn_to_section(ramdump, pfn):
    return nr_to_section(ramdump, pfn_to_section_nr(pfn))


def pfn_to_page_sparse(ramdump, pfn):
    sec = pfn_to_section(ramdump, pfn)
    sizeof_page = ramdump.sizeof('struct page')
    return section_mem_map_addr(ramdump, sec) + pfn * sizeof_page


def page_to_pfn_sparse(ramdump, page):
    page_flags_offset = ramdump.field_offset('struct page', 'flags')
    sizeof_page = ramdump.sizeof('struct page')
    flags = ramdump.read_word(page + page_flags_offset)
    if flags is None:
        return 0
    section = page_to_section(flags)
    nr = nr_to_section(ramdump, section)
    addr = section_mem_map_addr(ramdump, nr)
    # divide by struct page size for division fun
    return (page - addr) / sizeof_page


def get_vmemmap(ramdump):
    return ramdump.read_u64('vmemmap')

def page_to_pfn_vmemmap(ramdump, page):
    if ramdump.kernel_version >= (5, 4, 0):
        mem_map = get_vmemmap(ramdump)
    else:
        mem_map = 0xffffffbdbf000000
    page_size = ramdump.sizeof('struct page')
    return ((page - mem_map) / page_size)


def pfn_to_page_vmemmap(ramdump, pfn):
    if ramdump.kernel_version >= (5, 4, 0):
        mem_map = get_vmemmap(ramdump)
    else:
        mem_map = 0xffffffbdbf000000
    page_size = ramdump.sizeof('struct page')
    pfn_offset = ramdump.phys_offset >> 12
    return mem_map + (pfn * page_size)


def page_to_pfn_flat(ramdump, page):
    mem_map_addr = ramdump.addr_lookup('mem_map')
    mem_map = ramdump.read_word(mem_map_addr)
    page_size = ramdump.sizeof('struct page')
    # XXX Needs to change for LPAE
    pfn_offset = ramdump.phys_offset >> 12
    return ((page - mem_map) / page_size) + pfn_offset


def pfn_to_page_flat(ramdump, pfn):
    mem_map_addr = ramdump.addr_lookup('mem_map')
    mem_map = ramdump.read_word(mem_map_addr)
    page_size = ramdump.sizeof('struct page')
    # XXX Needs to change for LPAE
    pfn_offset = ramdump.phys_offset >> 12
    return mem_map + ((pfn - pfn_offset) * page_size)


def page_to_pfn(ramdump, page):
    if ramdump.arm64:
        return page_to_pfn_vmemmap(ramdump, page)
    if ramdump.CONFIG_SPARSEMEM:
        return page_to_pfn_sparse(ramdump, page)
    else:
        return page_to_pfn_flat(ramdump, page)


def pfn_to_page(ramdump, pfn):
    if ramdump.arm64:
        return pfn_to_page_vmemmap(ramdump, pfn)
    if ramdump.CONFIG_SPARSEMEM:
        return pfn_to_page_sparse(ramdump, pfn)
    else:
        return pfn_to_page_flat(ramdump, pfn)


def sparsemem_lowmem_page_address(ramdump, page):
    membank1_start = ramdump.read_word(ramdump.addr_lookup('membank1_start'))
    membank0_size = ramdump.read_word(ramdump.addr_lookup('membank0_size'))
    # XXX currently magic
    membank0_phys_offset = ramdump.phys_offset
    membank0_page_offset = ramdump.page_offset
    membank1_phys_offset = membank1_start
    membank1_page_offset = membank0_page_offset + membank0_size
    phys = page_to_pfn(ramdump, page) << 12
    if phys >= membank1_start:
        return phys - membank1_phys_offset + membank1_page_offset
    else:
        return phys - membank0_phys_offset + membank0_page_offset


def dont_map_hole_lowmem_page_address(ramdump, page):
    phys = page_to_pfn(ramdump, page) << 12
    hole_end_addr = ramdump.addr_lookup('memory_hole_end')
    if hole_end_addr is None:
        hole_end_addr = ramdump.addr_lookup('membank1_start')
    hole_offset_addr = ramdump.addr_lookup('memory_hole_offset')
    if hole_offset_addr is None:
        hole_offset_addr = ramdump.addr_lookup('membank0_size')
    hole_end = ramdump.read_word(hole_end_addr)
    hole_offset = ramdump.read_word(hole_offset_addr)
    if hole_end != 0 and phys >= hole_end:
        return phys - hole_end + hole_offset + ramdump.page_offset
    else:
        return phys - ramdump.phys_offset + ramdump.page_offset


def normal_lowmem_page_address(ramdump, page):
    phys = page_to_pfn(ramdump, page) << 12
    if ramdump.arm64:
        if ramdump.kernel_version >= (5, 4, 0):
            phys_addr = phys - ramdump.read_s64('physvirt_offset')
            if phys_addr < 0:
             phys_addr = phys_addr +  (1 << 64)
            return phys_addr
        else:
            memstart_addr = ramdump.read_s64('memstart_addr')
            return phys - memstart_addr + ramdump.page_offset
    else:
        return phys - ramdump.phys_offset + ramdump.page_offset


def lowmem_page_address(ramdump, page):
    if ramdump.CONFIG_SPARSEMEM and not ramdump.arm64:
        return sparsemem_lowmem_page_address(ramdump, page)
    elif ramdump.CONFIG_DONT_MAP_HOLE_AFTER_MEMBANK0:
        return dont_map_hole_lowmem_page_address(ramdump, page)
    else:
        return normal_lowmem_page_address(ramdump, page)


def page_address(ramdump, page):
    if not zone_is_highmem(ramdump, page_zone(ramdump, page)):
        return lowmem_page_address(ramdump, page)

    pas = page_slot(ramdump, page)
    lh_offset = ramdump.field_offset('struct page_address_slot', 'lh')
    start = pas + lh_offset
    pam = start
    while True:
        pam = pam - lh_offset
        pam_page_offset = ramdump.field_offset(
            'struct page_address_map', 'page')
        pam_virtual_offset = ramdump.field_offset(
            'struct page_address_map', 'virtual')
        pam_page = ramdump.read_word(pam + pam_page_offset)
        if pam_page == page:
            ret = ramdump.read_word(pam + pam_virtual_offset)
            return ret
        pam = ramdump.read_word(pam + lh_offset)
        if pam == start:
            return None


class mm_page_ext:
    def __init__(self, ramdump):
        self.ramdump = ramdump
        self.page_ext_size = ramdump.sizeof('struct page_ext')
        self.contig_page_data = ramdump.addr_lookup('contig_page_data')
        self.node_start_pfn_offset = ramdump.field_offset(
                                         'struct pglist_data', 'node_start_pfn')
        self.node_start_pfn = ramdump.read_word(self.contig_page_data +
                                  self.node_start_pfn_offset)

        if ramdump.arm64:
            self.page_ext_offset = ramdump.field_offset('struct mem_section',
                                                        'page_ext')

            mem_section_addr = ramdump.addr_lookup('mem_section')
            mem_section = ramdump.read_word(mem_section_addr)

            self.memsection_struct_size = ramdump.sizeof('struct mem_section')
            self.sections_per_root = 4096 / self.memsection_struct_size

            min_pfn = self.get_min_pfn()
            min_sec_nr = pfn_to_section_nr(min_pfn)
            min_sec_nr_root = min_sec_nr / self.sections_per_root
            min_mask = min_sec_nr & (self.sections_per_root - 1)

            max_pfn = self.get_max_pfn()
            max_sec_nr = pfn_to_section_nr(max_pfn)
            max_sec_nr_root = max_sec_nr / self.sections_per_root
            max_mask = max_sec_nr & (self.sections_per_root - 1)

            self.page_ext = []
            for s in range(min_sec_nr_root, max_sec_nr_root + 1):
                page_exts = [0] * (max_mask + 1)
                for m in range(min_mask, max_mask + 1):
                    section = mem_section + (self.memsection_struct_size
                                            * (s * self.sections_per_root + m))
                    page_exts[m] = self.ramdump.read_word(section +
                                     self.page_ext_offset)
                self.page_ext.insert(s, page_exts)
        else:
            self.page_ext_offset = ramdump.field_offset('struct pglist_data',
                                                        'node_page_ext')
            self.page_ext = ramdump.read_word(self.contig_page_data +
                                              self.page_ext_offset)


    def lookup_page_ext(self, pfn):
        if self.ramdump.arm64:
            sec_nr_root = pfn_to_section_nr(pfn) / self.sections_per_root
            sec_mask = pfn_to_section_nr(pfn) & (self.sections_per_root - 1)
            return self.page_ext[sec_nr_root][sec_mask] + pfn * self.page_ext_size
        else:
            offset = pfn - self.node_start_pfn
            return self.page_ext + offset * self.page_ext_size


    def get_min_pfn(self):
        return self.node_start_pfn


    def get_max_pfn(self):
        max_pfn_addr = self.ramdump.addr_lookup('max_pfn')
        max_pfn = self.ramdump.read_word(max_pfn_addr)
        return max_pfn
