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

from print_out import print_out_str
from parser_util import register_parser, RamParser


@register_parser('--print-vmstats', 'Print the information similar to /proc/zoneinfo and /proc/vmstat')
class ZoneInfo(RamParser):

    def print_atomic_long_counters(self, stats_array, addr, num):
        for i in xrange(0, num):
            print_out_str('{0:30}: {1:8}'.format(stats_array[i], self.ramdump.read_word(
                self.ramdump.array_index(addr, 'atomic_long_t', i))))

    def print_zone_stats(self, zone, vmstat_names, max_zone_stats):
        nr_watermark = self.ramdump.gdbmi.get_value_of('NR_WMARK')
        wmark_names = self.ramdump.gdbmi.get_enum_lookup_table(
            'zone_watermarks', nr_watermark)

        zone_name_offset = self.ramdump.field_offset('struct zone', 'name')
        zname_addr = self.ramdump.read_word(zone + zone_name_offset)
        zname = self.ramdump.read_cstring(zname_addr, 12)

        zstats_addr = zone + \
            self.ramdump.field_offset('struct zone', 'vm_stat')

        if (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (5, 4):
             zwatermark_addr = zone + \
                self.ramdump.field_offset('struct zone', '_watermark')
        else:
             zwatermark_addr = zone + \
                self.ramdump.field_offset('struct zone', 'watermark')

        print_out_str('\nZone {0:8}'.format(zname))
        self.print_atomic_long_counters(vmstat_names, zstats_addr, max_zone_stats)
        self.print_atomic_long_counters(wmark_names, zwatermark_addr, nr_watermark)

    def parse(self):
        max_zone_stats = self.ramdump.gdbmi.get_value_of(
            'NR_VM_ZONE_STAT_ITEMS')
        vmstat_names = self.ramdump.gdbmi.get_enum_lookup_table(
            'zone_stat_item', max_zone_stats)
        max_nr_zones = self.ramdump.gdbmi.get_value_of('__MAX_NR_ZONES')

        contig_page_data = self.ramdump.addr_lookup('contig_page_data')
        node_zones_offset = self.ramdump.field_offset(
            'struct pglist_data', 'node_zones')
        present_pages_offset = self.ramdump.field_offset(
            'struct zone', 'present_pages')
        sizeofzone = self.ramdump.sizeof('struct zone')
        zone = contig_page_data + node_zones_offset

        while zone < (contig_page_data + node_zones_offset + max_nr_zones * sizeofzone):
            present_pages = self.ramdump.read_word(zone + present_pages_offset)
            if not not present_pages:
                self.print_zone_stats(zone, vmstat_names, max_zone_stats)

            zone = zone + sizeofzone

        print_out_str('\nGlobal Stats')
        if (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (5, 4):
            max_node_stats = self.ramdump.gdbmi.get_value_of(
                'NR_VM_NODE_STAT_ITEMS')
            vmnodestat_names = self.ramdump.gdbmi.get_enum_lookup_table(
                'node_stat_item', max_node_stats)

            print_out_str("Zone Stats")
            vmstats_addr = self.ramdump.addr_lookup('vm_zone_stat')
            self.print_atomic_long_counters(vmstat_names, vmstats_addr, max_zone_stats)

            print_out_str("\nNode Stats")
            vmnodestats_addr = self.ramdump.addr_lookup('vm_node_stat')
            self.print_atomic_long_counters(vmnodestat_names, vmnodestats_addr, max_node_stats)
            total_ram_pages = self.ramdump.read_word(self.ramdump.addr_lookup('_totalram_pages'))
        else:
            vmstats_addr = self.ramdump.addr_lookup('vm_stat')
            self.print_atomic_long_counters(vmstat_names, vmstats_addr, max_zone_stats)
            total_ram_pages = self.ramdump.read_word(self.ramdump.addr_lookup('totalram_pages'))
        print_out_str('Total system pages: {0}'.format(total_ram_pages))
