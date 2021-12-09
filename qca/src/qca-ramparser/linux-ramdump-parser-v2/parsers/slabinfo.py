# Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import os
import sys

from mm import page_address, pfn_to_page
from print_out import print_out_str
from parser_util import register_parser, RamParser
import meminfo
import operator

SLAB_RED_ZONE = 0x400
SLAB_POISON = 0x800
SLAB_STORE_USER = 0x10000
OBJECT_POISON = 0x80000000

SLUB_RED_INACTIVE = 0xbb
SLUB_RED_ACTIVE = 0xcc
POISON_INUSE = 0x5a
POISON_FREE = 0x6b
POISON_END = 0xa5



class kmem_cache(object):
    def __init__(self, ramdump, addr):
        self.valid = False

        self.flags = ramdump.read_word(addr + g_offsetof.kmemcache_flags_offset)
        if self.flags is None:
            return

        self.size = ramdump.read_int(addr + g_offsetof.kmemcache_size_offset)
        if self.size is None:
            return

        self.object_size = ramdump.read_int(addr + g_offsetof.kmemcahce_object_size_offset)
        if self.object_size is None:
            return

        self.offset = ramdump.read_int(addr + g_offsetof.kmemcahce_offsetfield_offset)
        if self.offset is None:
            return

        self.inuse = ramdump.read_int(addr + g_offsetof.kmemcache_inuse_offset)
        if self.inuse is None:
            return

        if addr is None or g_offsetof.red_left_pad_offset is None:
            self.red_left_pad = 0
        else:
            self.red_left_pad = ramdump.read_int(addr + g_offsetof.red_left_pad_offset)
            if self.red_left_pad is None:
                self.red_left_pad = 0

        self.addr = addr
        self.valid = True


class struct_member_offset(object):
    def __init__(self, ramdump):
        self.kmemcache_flags_offset = ramdump.field_offset(
            'struct kmem_cache', 'flags')
        self.kmemcache_size_offset = ramdump.field_offset(
            'struct kmem_cache', 'size')
        self.kmemcahce_object_size_offset = ramdump.field_offset(
            'struct kmem_cache', 'object_size')
        self.kmemcahce_offsetfield_offset = ramdump.field_offset(
            'struct kmem_cache', 'offset')
        self.kmemcache_inuse_offset = ramdump.field_offset(
            'struct kmem_cache', 'inuse')
        self.kmemcache_list = ramdump.field_offset(
            'struct kmem_cache', 'list')
        self.kmemcache_name = ramdump.field_offset(
            'struct kmem_cache', 'name')
        self.kmemcache_node = ramdump.field_offset(
            'struct kmem_cache', 'node')
        self.red_left_pad_offset = ramdump.field_offset(
             'struct kmem_cache', 'red_left_pad')
        self.kmemcache_cpu_page = ramdump.field_offset(
            'struct kmem_cache_cpu', 'page')
        self.kmemcpucache_cpu_slab = ramdump.field_offset(
            'struct kmem_cache', 'cpu_slab')
        self.kmemcachenode_partial = ramdump.field_offset(
            'struct kmem_cache_node', 'partial')
        self.kmemcachenode_full = ramdump.field_offset(
                            'struct kmem_cache_node', 'full')
        self.kmemcachenode_nr_total_objs_offset = ramdump.field_offset(
            'struct kmem_cache_node', 'total_objects')
        self.page_lru = ramdump.field_offset(
                            'struct page', 'lru')
        self.page_flags = ramdump.field_offset(
                            'struct page', 'flags')
        if (ramdump.kernel_version <= (4, 14)):
            self.page_mapcount = ramdump.field_offset(
                                'struct page', '_mapcount')
        else:
            self.page_mapcount = ramdump.field_offset(
                                'struct page', 'counters')
        self.track_addrs = ramdump.field_offset(
                            'struct track', 'addrs')
        self.page_freelist = ramdump.field_offset(
                            'struct page', 'freelist')
        self.sizeof_struct_track = ramdump.sizeof(
                                            'struct track')
        self.sizeof_void_pointer = ramdump.sizeof(
                                             "void *")
        self.sizeof_unsignedlong = ramdump.sizeof(
                                             "unsigned long")


@register_parser('--slabinfo', 'print information about slabs', optional=True)
class Slabinfo(RamParser):
    g_offsetof = None

    def __init__(self, ramdump):
        self.ramdump = ramdump
        self.slabinfo_ranked = meminfo.meminfo_ranked(ramdump)

    def get_free_pointer(self, ramdump, s, obj):
        # just like validate_slab_slab!
        return self.ramdump.read_word(obj + s.offset)

    def slab_index(self, ramdump, p, addr, slab):
        return (p - addr) / slab.size

    def get_map(self, ramdump, slab, page, bitarray):
        freelist = self.ramdump.read_word(page + g_offsetof.page_freelist)
        p = freelist
        addr = page_address(self.ramdump, page)
        seen = {}
        if addr is None:
            return
        while p != 0 and p is not None and p not in seen:
            idx = self.slab_index(self.ramdump, p, addr, slab)
            if idx >= len(bitarray) or idx < 0:
                return
            bitarray[idx] = 1
            seen[p] = 1
            p = self.get_free_pointer(self.ramdump, slab, p)

    def get_track(self, ramdump, slab, obj, track_type):
        track_size = g_offsetof.sizeof_struct_track
        if slab.offset != 0:
            p = obj + slab.red_left_pad + slab.offset + g_offsetof.sizeof_void_pointer
        else:
            p = obj + slab.red_left_pad + slab.inuse
        return p + track_type * track_size

    def print_track(self, ramdump, slab_name, slab, obj, track_type, out_file):
        stack = []
        stackstr = ""
        track_addrs_offset = g_offsetof.track_addrs
        pointer_size = g_offsetof.sizeof_unsignedlong

        p = self.get_track(ramdump, slab, obj, track_type)
        start = p + track_addrs_offset
        for i in range(0, 16):
            a = self.ramdump.read_word(start + pointer_size * i)
            if a == 0:
                continue
            stack += [a]
            stackstr += str(a)

        if len(stack) != 0:
            self.slabinfo_ranked.insert([slab_name], [p], stack, slab.size, "S", track_type)


    def print_slab(self, ramdump, slab_name, slab, page, out_file, map_fn, out_slabs_addrs):
        if page is None:
            return

        page_addr = page_address(ramdump, page)
        p = page_addr
        n_objects = self.ramdump.read_word(page + g_offsetof.page_mapcount)
        if n_objects is None:
            return

        n_objects = (n_objects >> 16) & 0x00007FFF
        bitarray = [0] * n_objects
        self.get_map(self.ramdump, slab, page, bitarray)

        while p < page_addr + (n_objects * slab.size):
            bitidx = self.slab_index(self.ramdump, p, page_addr, slab)
            if bitidx >= n_objects or bitidx < 0:
                return

            map_fn(ramdump, slab_name, p, bitarray[bitidx], slab, page, out_file,
                   out_slabs_addrs)
            p = p + slab.size

    def printsummary(self, slabs_output_summary):
        sorted_meminfo = self.slabinfo_ranked.sort_by_size()
        for info in sorted_meminfo:
            m = sorted_meminfo[info]
            slabs_output_summary.write(str(m))

        alloc_size = 0

        network_alloc_size = 0
        wifi_alloc_size = 0
        nss_alloc_size = 0

        slub_free_pending = 0

        for info in sorted_meminfo:
            m = sorted_meminfo[info]

            if m.allocation_type == "SLUB Allocation [Free]":
                slub_free_pending += m.total_size
                continue

            alloc_size += m.total_size

            if m.category == "Networking":
                if m.subcategory == "WiFi":
                    wifi_alloc_size += m.total_size
                if m.subcategory == "NSS":
                    nss_alloc_size += m.total_size

        network_alloc_size = wifi_alloc_size + nss_alloc_size

        print_out_str("Total allocated size: {0} KB".format(alloc_size / 1024))

        print_out_str("\tNetwork allocation: {0} KB".format(network_alloc_size / 1024))
        print_out_str("\t\tWiFi allocation: {0} KB".format(wifi_alloc_size / 1024))
        print_out_str("\t\tNSS allocation: {0} KB".format(nss_alloc_size / 1024))

    def print_slab_page_info(
                self, ramdump, slab_name, slab_obj, slab_node, start,
                out_file, map_fn, out_slabs_addrs):
        page = self.ramdump.read_word(start)
        if page == 0:
            return
        seen = {}
        max_pfn_addr = self.ramdump.addr_lookup('max_pfn')
        max_pfn = self.ramdump.read_word(max_pfn_addr)
        max_page = pfn_to_page(ramdump, max_pfn)
        while page != start:
            if page is None:
                return
            if page in seen:
                return
            if page > max_page:
                return
            seen[page] = 1
            page = page - g_offsetof.page_lru
            self.print_slab(
                self.ramdump, slab_name, slab_obj, page, out_file, map_fn, out_slabs_addrs)
            page = self.ramdump.read_word(page + g_offsetof.page_lru)

    def print_per_cpu_slab_info(
            self, ramdump, slab_name, slab, slab_node, start, out_file, map_fn):
        page = self.ramdump.read_word(start)
        if page == 0:
            return
        if page is None:
            return
        page_addr = page_address(self.ramdump, page)
        self.print_slab(
            self.ramdump, slab_name, page_addr, slab, page, out_file, map_fn)

    def print_all_objects(self, ramdump, slab_name, p, free, slab, page, out_file, out_slabs_addrs):
        if free:
            out_slabs_addrs.write(
                    '\n   Object {0:x}-{1:x} FREE\n'.format(
                                        p, p + slab.size))
            self.print_track(ramdump, slab_name, slab, p, 1, out_file)
        else:
            out_file.write(
                    '\n   Object {0:x}-{1:x} ALLOCATED\n'.format(
                                    p, p + slab.size))
            self.print_track(ramdump, slab_name, slab, p, 0, out_file)

    def print_check_poison(self, p, free, slab, page, out_file):
        if free:
            self.check_object(slab, page, p, SLUB_RED_INACTIVE, out_file)
        else:
            self.check_object(slab, page, p, SLUB_RED_ACTIVE, out_file)

    def initializeOffset(self):
        global g_offsetof
        g_offsetof = struct_member_offset(self.ramdump)

    # based on validate_slab_cache. Currently assuming there
    # is only one numa node in the system because the code to
    # do that correctly is a big pain. This will
    # need to be changed if we ever do NUMA properly.
    def validate_slab_cache(self, slab_out, input_slabname,  map_fn):
        slab_name_found = False

        offsetof = struct_member_offset(self.ramdump)
        self.initializeOffset()
        slab_list_offset = g_offsetof.kmemcache_list
        slab_name_offset = g_offsetof.kmemcache_name
        slab_node_offset = g_offsetof.kmemcache_node
        cpu_slab_offset = g_offsetof.kmemcpucache_cpu_slab
        slab_partial_offset = g_offsetof.kmemcachenode_partial
        slab_full_offset = g_offsetof.kmemcachenode_full

        original_slab = self.ramdump.addr_lookup('slab_caches')
        slab = self.ramdump.read_word(original_slab)

        slabs_output_summary = self.ramdump.open_file('slabs_output.txt')
        out_slabs_addrs = self.ramdump.open_file('out_slabs_addrs.txt')

        while slab != original_slab:
            slab = slab - slab_list_offset
            slab_obj = kmem_cache(self.ramdump, slab)
            if not slab_obj.valid:
                slab_out.write('Invalid slab object {0:x}'.format(slab))
                slab = self.ramdump.read_word(slab + slab_list_offset)
                continue

            slab_name_addr = self.ramdump.read_word(slab + slab_name_offset)
            slab_name = self.ramdump.read_cstring(slab_name_addr, 48)
            if input_slabname is not None:
                if input_slabname != slab_name:
                    slab = self.ramdump.read_word(slab + slab_list_offset)
                    continue
                else:
                    slab_name_found = True

            # actually an array but again, no numa
            slab_node_addr = self.ramdump.read_word(slab + slab_node_offset)
            slab_node = self.ramdump.read_word(slab_node_addr)
            print_out_str('\nExtracting slab details of : {0}'.format(slab_name))
            cpu_slab_addr = self.ramdump.read_word(slab + cpu_slab_offset)
            nr_total_objects = self.ramdump.read_word(slab_node_addr +
                        g_offsetof.kmemcachenode_nr_total_objs_offset)
            slab_out.write(
                '\n {0:x} slab {1} {2:x}  total objects: {3}\n'.format(
                        slab, slab_name, slab_node_addr, nr_total_objects))

            self.print_slab_page_info(
                self.ramdump, slab_name, slab_obj, slab_node,
                slab_node_addr + slab_partial_offset,
                slab_out, map_fn, out_slabs_addrs)
            if self.ramdump.CONFIG_SLUB_DEBUG:
                self.print_slab_page_info(
                    self.ramdump, slab_name, slab_obj, slab_node,
                    slab_node_addr + slab_full_offset,
                    slab_out, map_fn, out_slabs_addrs)

            # per cpu slab
            cpus = self.ramdump.get_num_cpus()
            for i in range(0, cpus):
                cpu_slabn_addr = self.ramdump.read_word(
                                                cpu_slab_addr, cpu=i)
                if cpu_slabn_addr == 0 or None:
                    break
                self.print_per_cpu_slab_info(
                    self.ramdump, slab_name, slab_obj,
                    slab_node, cpu_slabn_addr + offsetof.cpu_cache_page_offset,
                    slab_out, map_fn)

            if slab_name_found is True:
                break
            slab = self.ramdump.read_word(slab + slab_list_offset)

        self.printsummary(slabs_output_summary)

        out_slabs_addrs.close()
        slabs_output_summary.close()

    def parse(self):
        if (len(self.ramdump.config) != 0) and (not self.ramdump.CONFIG_SLUB_DEBUG or not self.ramdump.is_config_defined('CONFIG_STACKTRACE')):
            print_out_str ("either slub_debug_on or stacktrace is not enabled")
            return
        cmdline = self.ramdump.get_command_line()
        if cmdline.find("slub_debug=FZPU") == -1:
            print_out_str ("slub_debug=FZPU is not present in command line. Boot args is not properly set")
            return
        slabname = None
        for arg in sys.argv:
            if 'slabname=' in arg:
                k, slabname = arg.split('=')
        slab_out = self.ramdump.open_file('slabs.txt')
        self.validate_slab_cache(slab_out, slabname, self.print_all_objects)
        slab_out.close()


@register_parser('--slabpoison', 'check slab poison', optional=True)
class Slabpoison(Slabinfo):
    """Note that this will NOT find any slab errors which are printed out by the
    kernel, because the slab object is removed from the freelist while being
    processed"""

    def print_section(self, text, addr, length, out_file):
        out_file.write('{}\n'.format(text))
        output = self.ramdump.hexdump(addr, length)
        out_file.write(output)

    def print_trailer(self, s, page, p, out_file):
        addr = page_address(self.ramdump, page)

        if self.ramdump.CONFIG_SLUB_DEBUG_ON:
            self.print_track(self.ramdump, '', s.addr, p, 0, out_file)
            self.print_track(self.ramdump, '', s.addr, p, 1, out_file)

        out_file.write('INFO: Object 0x{:x} @offset=0x{:x} fp=0x{:x}\n\n'.format(
            p, p - addr, self.get_free_pointer(self.ramdump, s, p)))

        if (p > addr + 16):
            self.print_section('Bytes b4 ', p - 16, 16, out_file)

        self.print_section('Object ', p, min(s.object_size, 4096), out_file)
        if (s.flags & SLAB_RED_ZONE):
            self.print_section('Redzone ', p + s.object_size,
                s.inuse - s.object_size, out_file)

        if (s.offset):
            off = s.offset + self.ramdump.sizeof('void *')
        else:
            off = s.inuse

        if (s.flags & SLAB_STORE_USER):
            off += 2 * self.ramdump.sizeof('struct track')

        if (off != s.size):
            # Beginning of the filler is the free pointer
            self.print_section('Padding ', p + off, s.size - off, out_file)

    def memchr_inv(self, addr, value, size):
        data = self.read_byte_array(addr, size)
        if data is None:
            return 0
        for i in range(len(data)):
            if data[i] != value:
                return i + addr
        return 0

    def check_bytes_and_report(self, s, page, object, what, start,
             value, bytes, out_file):
        fault = self.memchr_inv(start, value, bytes)
        if (not fault):
            return True

        end = start + bytes
        while (end > fault and (self.read_byte_array(end - 1, 1)[0]) == value):
            end -= 1

        out_file.write('{0} overwritten\n'.format(what))
        out_file.write('INFO: 0x{:x}-0x{:x}. First byte 0x{:x} instead of 0x{:x}\n'.format(
            fault, end - 1, self.ramdump.read_byte(fault), value))

        self.print_trailer(s, page, object, out_file)
        return False

    def check_pad_bytes(self, s, page, p, out_file):
        off = s.inuse

        if (s.offset):
            # Freepointer is placed after the object
            off += self.ramdump.sizeof('void *')

        if (s.flags & SLAB_STORE_USER):
            # We also have user information there
            off += 2 * self.ramdump.sizeof('struct track')

        if (s.size == off):
            return True

        return self.check_bytes_and_report(s, page, p, 'Object padding',
            p + off, POISON_INUSE, s.size - off, out_file)

    def check_object(self, s, page, object, val, out_file):
        p = object
        endobject = object + s.object_size

        if (s.flags & SLAB_RED_ZONE):
            if (not self.check_bytes_and_report(s, page, object, 'Redzone',
                endobject, val, s.inuse - s.object_size, out_file)):
                return
        else:
            if ((s.flags & SLAB_POISON) and s.object_size < s.inuse):
                self.check_bytes_and_report(s, page, p, 'Alignment padding',
                    endobject, POISON_INUSE,
                    s.inuse - s.object_size, out_file)

        if (s.flags & SLAB_POISON):
            if (val != SLUB_RED_ACTIVE and (s.flags & OBJECT_POISON) and
                (not self.check_bytes_and_report(s, page, p, 'Poison', p,
                    POISON_FREE, s.object_size - 1, out_file) or
                not self.check_bytes_and_report(s, page, p, 'Poison',
                    p + s.object_size - 1, POISON_END, 1, out_file))):
                return

            # check_pad_bytes cleans up on its own.
            self.check_pad_bytes(s, page, p, out_file)

    # since slabs are relatively "packed", caching has a large
    # performance benefit
    def read_byte_array(self, addr, size):
        page_addr = addr & -0x1000
        end_page_addr = (addr + size) & -0x1000
        # in cache
        if page_addr == end_page_addr and page_addr == self.cache_addr:
            idx = addr - self.cache_addr
            return self.cache[idx:idx + size]
        # accessing only one page
        elif page_addr == end_page_addr:
            fmtstr = '<{}B'.format(4096)
            self.cache = self.ramdump.read_string(page_addr, fmtstr)
            self.cache_addr = page_addr
            idx = addr - self.cache_addr
            return self.cache[idx:idx + size]
        else:
            fmtstr = '<{}B'.format(size)
            return self.ramdump.read_string(addr, fmtstr)

    def parse(self):
        self.cache = None
        self.cache_addr = None
        slab_out = self.ramdump.open_file('slabpoison.txt')
        self.validate_slab_cache(slab_out, None, self.print_check_poison)
        print_out_str('---wrote slab information to slabpoison.txt')
