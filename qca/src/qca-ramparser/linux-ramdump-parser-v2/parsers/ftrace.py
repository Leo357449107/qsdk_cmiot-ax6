# Copyright (c) 2012-2014,2019 The Linux Foundation. All rights reserved.
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

@register_parser('--print-ftrace', 'Print the trace of fucntion', shortopt='-m')
class Ftrace(RamParser):
    def __init__(self, ramdump):
        self.ramdump = ramdump

        self.index_offset = 0
        self.r_offset = 0
        self.r_pa_offset = 0
        self.ncpu_offset = 0
        self.cpu_offset = 0

        #struct srd_record
        self.ip_offset = 0
        self.srd_percpu_size = 0

        #struct srd_percpu
        self.srd_percpu_index_offset = 0
        self.srd_percpu_size = 0

        self.srd_ncpu = 0

    def getsrd(self):
        #struct silent_reboot_debug
        self.index_offset = self.ramdump.field_offset('struct silent_reboot_debug','index')
        self.r_offset = self.ramdump.field_offset('struct silent_reboot_debug','r')
        self.r_pa_offset = self.ramdump.field_offset('struct silent_reboot_debug','r_pa')
        self.ncpu_offset = self.ramdump.field_offset('struct silent_reboot_debug','ncpu')
        self.cpu_offset = self.ramdump.field_offset('struct silent_reboot_debug','cpu')

        #struct srd_record
        self.ip_offset = self.ramdump.field_offset('struct srd_record', 'ip')
        self.srd_percpu_size = self.ramdump.sizeof('struct srd_percpu')

        #struct srd_percpu
        self.srd_percpu_index_offset = self.ramdump.field_offset('struct srd_percpu', 'index')
        self.srd_percpu_size = self.ramdump.sizeof('struct srd_percpu')

        #srd is a pointer to struct silent_reboot_debug
        srd_ptr = self.ramdump.addr_lookup('srd')
        if (srd_ptr is None):
            return;
        srd = self.ramdump.read_word(srd_ptr);

        srd_index = self.ramdump.read_word(srd + self.index_offset)

        srd_r = self.ramdump.read_word(srd + self.r_offset) # r is pointer

        self.srd_r_pa = self.ramdump.read_word(srd + self.r_pa_offset)
        print_out_str ('srd_r_pa = 0x{0:X}'.format(self.srd_r_pa))

        self.srd_ncpu = self.ramdump.read_word(srd + self.ncpu_offset)
        print_out_str ('srd_ncpu = 0x{0:X}'.format(self.srd_ncpu))

        srd_cpu_array = srd + self.cpu_offset #cpu is an array
        self.ring_buffer_index = []
        for i in range(0, self.srd_ncpu):
            index = self.ramdump.read_word(srd_cpu_array)
            index %= 8192
            print_out_str ('trace info index for core {0} = {1}'.format(i, index))
            self.ring_buffer_index.append(index)
            srd_cpu_array += self.srd_percpu_size

    #pa - physical address, core number
    def readtrace(self, pa, corenum):
        if (self.srd_r_pa is None):
            return;
        ftrace_size  = 0x7FFF # 32768/4
        start = ( ftrace_size + 1 ) * corenum
        end = start + ftrace_size
        array = []
        temp = []
        for i in range(start, end, 4):
            symaddr = self.ramdump.read_word(pa+i, False)
            array.append(symaddr)
        #rearrange array
        temp.extend(array[self.ring_buffer_index[corenum]:])
        temp.extend(array[:self.ring_buffer_index[corenum]])

        #write into file
        tfile = self.ramdump.open_file('ftrace{0}.txt'.format(corenum))
        for symaddr in temp:
            look = self.ramdump.unwind_lookup(symaddr)
            #print ('{0:X} ====> {1}'.format(symaddr, look))
            tfile.write('{0:X} ====> {1}\n'.format(symaddr, look))
        tfile.close()
        print_out_str('check ftrace{0}.txt for details'.format(corenum))

    def parse(self):
        if not self.ramdump.is_config_defined('CONFIG_FUNCTION_TRACER') or not self.ramdump.is_config_defined('CONFIG_HAVE_FUNCTION_TRACER'):
            print_out_str ("function tracer is not enabled")
            return
        self.getsrd()
        for core in range(0, self.srd_ncpu):
            self.readtrace(self.srd_r_pa, core)
