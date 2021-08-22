# Copyright (c) 2012,2014 The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

from print_out import print_out_str
from parser_util import register_parser, RamParser
from mm import pfn_to_page, mm_page_ext
import meminfo

def order_to_size(order):
    if order < 0:
       return 0
    return (1 << order) * 4096

@register_parser('--print-pagetracking', 'print page tracking information (if available)')
class PageTracking(RamParser):
    def __init__(self, ramdump):
        self.ramdump = ramdump
        self.pageflags = {}

    def get_flags_str(self, flags):
        flags_str = ""
        for i in self.pageflags:
            if flags & (1 << i):
                flags_str += self.pageflags[i]

        return flags_str

    def parse(self):
        ramdump = self.ramdump

        if not ramdump.is_config_defined('CONFIG_PAGE_OWNER'):
            return

        cmdline = ramdump.get_command_line()
        if cmdline.find("page_owner=on") == -1:
            return

        pageflags_table = ramdump.gdbmi.get_enum_lookup_table(
            'pageflags', 26)
        self.pageflags[pageflags_table.index("PG_slab")] = 'S'
        self.pageflags[pageflags_table.index("PG_lru")] = 'L'

        page_ext_flags_table = ramdump.gdbmi.get_enum_lookup_table(
            'page_ext_flags', 5)
        PAGE_EXT_OWNER = (1 << page_ext_flags_table.index('PAGE_EXT_OWNER'))

        page_ext_obj = mm_page_ext(ramdump)

        meminfo.gen_symbol_info(ramdump.nm_path, ramdump.vmlinux)

        min_pfn = page_ext_obj.get_min_pfn()
        max_pfn = page_ext_obj.get_max_pfn()

        page_flags_offset = ramdump.field_offset('struct page', 'flags')
        order_offset = ramdump.field_offset('struct page_ext', 'order')
        page_ext_flags_offset = ramdump.field_offset('struct page_ext', 'flags')
        nr_entries_offset = ramdump.field_offset(
            'struct page_ext', 'nr_entries')
        trace_entries_offset = ramdump.field_offset(
            'struct page_ext', 'trace_entries')
        trace_entry_size = ramdump.sizeof("void *")

        out_tracking = ramdump.open_file('page_tracking.txt')
        out_tracking_all = ramdump.open_file('page_tracking_all.txt')
        page_info = meminfo.meminfo_ranked(self.ramdump)

        pfn = min_pfn
        while pfn < max_pfn:
            order = 0
            trace_entries = []
            page_ext = page_ext_obj.lookup_page_ext(pfn)
            page_ext_flags = ramdump.read_word(page_ext + page_ext_flags_offset)

            if ((page_ext_flags & PAGE_EXT_OWNER) == PAGE_EXT_OWNER):
                page = pfn_to_page(ramdump, pfn)
                page_flags = ramdump.read_u32(page + page_flags_offset)

                order = ramdump.read_u32(page_ext + order_offset)
                nr_trace_entries = ramdump.read_int(page_ext + nr_entries_offset)

                flags = self.get_flags_str(page_flags)
                size = order_to_size(order)

                for i in range(0, nr_trace_entries):
                    entry = ramdump.read_word(
                        page_ext + trace_entries_offset + i * trace_entry_size)
                    trace_entries.append(entry)
                page_info.insert([], range(pfn, pfn + (1 << order)), trace_entries, size, flags)

            pfn += (1 << order)

        alloc_size = 0

        dma_size = 0
        network_dma_size = 0
        wifi_dma_size = 0
        nss_dma_size = 0

        direct_page_alloc_size = 0
        network_direct_page_alloc_size = 0
        wifi_direct_page_alloc_size = 0
        nss_direct_page_alloc_size = 0

        slub_alloc_size = 0
        other_alloc_size = 0

        kernel_used_size = 0

        sorted_meminfo = page_info.sort_by_size()
        for info in sorted_meminfo:
            m = sorted_meminfo[info]
            out_tracking_all.write(m.obj_in_str(True))

            if m.allocation_type == "Fallback":
                kernel_used_size += m.total_size
                continue

            if m.allocation_type == "IO Remapped Allocation":
                alloc_size += m.total_size
                continue

            alloc_size += m.total_size

            if m.flags == "" and m.allocation_type == "DMA Allocation":
                dma_size += m.total_size
                if m.category == "Networking":
                    if m.subcategory == "WiFi":
                        wifi_dma_size += m.total_size
                    if m.subcategory == "NSS":
                        nss_dma_size += m.total_size
            elif m.allocation_type == "Alloc Pages allocation":
                direct_page_alloc_size += m.total_size
                if m.category == "Networking":
                    if m.subcategory == "WiFi":
                        wifi_direct_page_alloc_size += m.total_size
                    if m.subcategory == "NSS":
                        nss_direct_page_alloc_size += m.total_size

            if m.flags.find("S") >= 0:
                slub_alloc_size += m.total_size

            out_tracking.write(str(m))

        network_dma_size = wifi_dma_size + nss_dma_size
        network_direct_page_alloc_size = (wifi_direct_page_alloc_size +
                                          nss_direct_page_alloc_size)
        other_alloc_size = alloc_size - (dma_size + direct_page_alloc_size +
                                         slub_alloc_size)

        out_tracking_all.close()
        out_tracking.close()

        print_out_str("Total pages allocated: {0} KB".format(alloc_size / 1024))

        print_out_str("\tTotal DMA allocation: {0} KB".format(dma_size / 1024))
        print_out_str("\t\tNetwork DMA allocation: {0} KB".format(network_dma_size/ 1024))
        print_out_str("\t\t\tWiFi DMA allocation: {0} KB".format(wifi_dma_size / 1024))
        print_out_str("\t\t\tNSS DMA allocation: {0} KB".format(nss_dma_size / 1024))

        print_out_str("\tDirect page allocation: {0} KB".format(direct_page_alloc_size / 1024))
        print_out_str("\t\tNetwork direct page allocation: {0} KB".format(network_direct_page_alloc_size / 1024))
        print_out_str("\t\t\tWiFi direct page allocation: {0} KB".format(wifi_direct_page_alloc_size / 1024))
        print_out_str("\t\t\tNSS direct page allocation: {0} KB".format(nss_direct_page_alloc_size / 1024))

        print_out_str("\tTotal SLUB allocation: {0} KB".format(slub_alloc_size / 1024))
        print_out_str("\tOther allocation: {0} KB".format(other_alloc_size / 1024))

        print_out_str("Kernel private: {0} KB\n".format(kernel_used_size / 1024))

        print_out_str(
            '---wrote page tracking information to page_tracking.txt')
