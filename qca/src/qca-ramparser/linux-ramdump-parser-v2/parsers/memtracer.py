# Copyright (c) 2020 The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import math
import hashlib

from mm import pfn_to_page
from parser_util import register_parser, RamParser
from print_out import print_out_str

#Defined in lib/debugobjects.c
BUCKET_SIZE = 16384

@register_parser('--memtracer', 'Display memory usage statistics', optional=True)
class memtracer_summary(RamParser):
    stack_trace_list = []
    dict_modules = {}
    dictionary_trace = {}
    dictionary_size = {}

    module_updated = 0
    print_mem = 0


    def update_driver_lists(self, modname, Alloc_size):
        if modname != None:
           if self.print_mem == 0:
              if self.module_updated != 1:
                 self.module_updated = 1

                 if modname in self.dict_modules.keys():
                    self.dict_modules[modname] = self.dict_modules[modname] + Alloc_size
                 else:
                    self.dict_modules[modname] = Alloc_size
        else:
           if 'Network stack' in self.dict_modules.keys():
              self.dict_modules['Network stack'] = self.dict_modules['Network stack'] + Alloc_size
           else:
              self.dict_modules['Network stack'] = Alloc_size

    def validate_stack(self, stack_trace, offset, stack_trace_addr, Alloc_size):
        if stack_trace == None:
           modname = None
           offset = -1
           symbol = None
           symtab_st_size = None
           r = self.ramdump.unwind_lookup(stack_trace_addr)
           if r is None:
              stack_trace = "[" + str(hex(stack_trace_addr)) + "]"
              offset = 0x0
           elif r is not None and len(r) > 3:
              symbol, offset, modname, symtab_st_size = r

           if self.print_mem == 0:
              self.update_driver_lists(modname, Alloc_size)

           if modname != None and offset != -1:
              stack_trace = symbol + " + 0x" + str(offset) + " [" + modname + "]"

           if stack_trace == None:
              stack_trace = "No symbol matches " + str(hex(stack_trace_addr))
        elif stack_trace != None and offset != None:
           stack_trace = stack_trace + " + " + str(hex(int(offset)))

        return stack_trace

    def checksum(self, debug_obj, stack_trace_offset, Alloc_size):
        str_hash = ""
        self.stack_trace_list = []

        #pointer size in bytes
        if self.ramdump.arm64:
            ptr_size = 8
        else:
            ptr_size = 4

        for k in range(1, 9):
            if self.module_updated != 1:
               self.module_updated = 0

            stack_trace_addr = self.ramdump.read_word(
                                debug_obj + stack_trace_offset + (ptr_size*k))
            stack_trace = self.ramdump.symbol_lookup_fail_safe(stack_trace_addr).symbol
            offset = self.ramdump.symbol_lookup_fail_safe(stack_trace_addr).offset
            stack_trace = self.validate_stack(stack_trace, offset, stack_trace_addr, Alloc_size)
            if self.print_mem == 0:
               self.stack_trace_list.append(stack_trace)
            str_hash = str_hash + stack_trace
        self.module_updated = 0
        checksum = hashlib.md5(str_hash.encode())
        return checksum.hexdigest()


    def calc_chksum(self, debug_obj, stack_trace_offset, Alloc_size):
        result = self.checksum(debug_obj, stack_trace_offset, Alloc_size)
        if result in self.dictionary_trace.keys() or result in self.dictionary_size.keys():
           self.dictionary_size[result] = self.dictionary_size[result] + Alloc_size
           temp_list = self.dictionary_trace[result]
           temp_list[-1] = temp_list[-1] + 1
           self.dictionary_trace[result] = temp_list
        else:
           self.stack_trace_list.append(1)
           self.dictionary_trace[result] = self.stack_trace_list
           self.dictionary_size[result] = Alloc_size

    def bytes_conversion(self, size):
        power = 2**10
        power_labels = {0: 'Bytes', 1: 'KB', 2: 'MB', 3: 'GB'}
        n = 0
        while size > power:
            size /= power
            n += 1
        Actual_size = str(size) + ' '  + power_labels[n]
        return Actual_size

    def print_mem_usage(self, mem_usage_out):
        format_string = '\n\n{0:60} {1:10} {2:10}'
        format_string1 = '\n{0:60}'
        format_string4 = '{0:60}\n\n'
        format_string2 = '{0:30} {1:10}\n'
        format_string3 = '{0:30} {1:10}\n\n\n'
        self.print_mem = 1
        network_usage = 0
        mem_usage_out.write('{0:20}'.format("Memoy usage summary:\n"))
        mem_usage_out.write('{0:20}'.format("====================\n"))

        sort_orders = sorted(self.dict_modules.items(), key=lambda x: x[1], reverse=True)
        size = 0
        for i in sort_orders:
            size = size + i[1]
        Actual_size = self.bytes_conversion(size)
        mem_usage_out.write('{0:20}'.format("Total Memory in Use: "))
        mem_usage_out.write(format_string4.format(Actual_size))

        for i in sort_orders:
            size = i[1]
            Actual_size = self.bytes_conversion(size)
            if i[0] == 'Network stack':
               network_usage = Actual_size
               continue
            mem_usage_out.write(format_string2.format(i[0], Actual_size))
        mem_usage_out.write(format_string3.format("Others (Network stack, etc.,)", network_usage))

        mem_usage_out.write(
                 '{0:60} {1:10} {2:10}'.format(
                                    "STACK TRACE", "OCCURENCE", "ALLOC SIZE"))
        sort_orders = sorted(self.dictionary_size.items(), key=lambda x: x[1], reverse=True)
        for i in sort_orders:
            stack_trace = self.dictionary_trace[i[0]]
            size = self.dictionary_size[i[0]]
            mem_usage_out.write(format_string.format(stack_trace[0], stack_trace[-1], size))
            for j in range(1, 8):
                mem_usage_out.write(format_string1.format(stack_trace[j]))
        return

    def parse_mem_objects(self):
        self.print_mem = 0
        bucket_addr = self.ramdump.addr_lookup('mem_trace_hash')
        obj_bucket = self.ramdump.read_word(bucket_addr)
        hlist_bucket_offset = self.ramdump.field_offset(
            'struct debug_bucket', 'list')
        lock_bucket_offset = self.ramdump.field_offset(
            'struct debug_bucket', 'lock')
        obj_node_offset = self.ramdump.field_offset(
            'struct debug_obj_trace', 'node')
        alloc_addr_offset = self.ramdump.field_offset(
            'struct debug_obj_trace', 'addr')
        stack_trace_offset = self.ramdump.field_offset(
            'struct debug_obj_trace', 'stack')
        alloc_size_offset = self.ramdump.field_offset(
            'struct debug_obj_trace', 'size')

        for i in range(0, BUCKET_SIZE):
            if obj_bucket == 0:
               bucket_addr = bucket_addr + lock_bucket_offset + lock_bucket_offset;
               obj_bucket = self.ramdump.read_word(bucket_addr)
               continue
            debug_obj = obj_bucket
            while debug_obj != 0:
                debug_obj = debug_obj - obj_node_offset
                Alloc_size = self.ramdump.read_word(
                                       debug_obj + alloc_size_offset)
                Alloc_addr = self.ramdump.read_word(
                                       debug_obj + alloc_addr_offset)
                if Alloc_size == 0:
                   debug_obj = self.ramdump.read_word(debug_obj + obj_node_offset)
                   continue
                self.calc_chksum(debug_obj, stack_trace_offset, Alloc_size)
                debug_obj = self.ramdump.read_word(debug_obj + obj_node_offset)

            bucket_addr = bucket_addr + lock_bucket_offset + lock_bucket_offset;
            obj_bucket = self.ramdump.read_word(bucket_addr)

    def parse(self):
        if not self.ramdump.is_config_defined('CONFIG_DEBUG_MEM_USAGE'):
            print_out_str ("CONFIG_DEBUG_MEM_USAGE is not enabled")
            return
        mem_usage_out = self.ramdump.open_file('memory_tracing.txt')
        self.parse_mem_objects()
        self.print_mem_usage(mem_usage_out)
        mem_usage_out.close()

