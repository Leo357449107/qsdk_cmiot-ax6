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

from tempfile import NamedTemporaryFile

from print_out import print_out_str
from parser_util import register_parser, RamParser

# struct msm_rtb_layout {
#        unsigned char sentinel[3];
#        unsigned char log_type;
#        void *caller;
#        unsigned long idx;
#        void *data;
#} __attribute__ ((__packed__));

print_table = {
    'LOGK_NONE': 'print_none',
    'LOGK_READL': 'print_readlwritel',
    'LOGK_WRITEL': 'print_readlwritel',
    'LOGK_LOGBUF': 'print_logbuf',
    'LOGK_HOTPLUG': 'print_hotplug',
    'LOGK_CTXID': 'print_ctxid',
    'LOGK_TIMESTAMP': 'print_timestamp',
    'LOGK_L2CPREAD': 'print_cp_rw',
    'LOGK_L2CPWRITE': 'print_cp_rw',
    'LOGK_IRQ': 'print_irq',
}


@register_parser('--print-rtb', 'Print RTB (if enabled)', shortopt='-r')
class RTB(RamParser):

    def __init__(self, *args):
        super(RTB, self).__init__(*args)
        self.name_lookup_table = []

    def get_caller(self, caller):
        return self.ramdump.gdbmi.get_func_info(caller)

    def get_fun_name(self, addr):
        l = self.ramdump.unwind_lookup(addr)
        if l is not None  and len(l) > 3:
            symname, offset, mname, symtab_st_size = l
        else:
            symname = 'Unknown function'
        return symname

    def print_none(self, rtbout, rtb_ptr, logtype):
        rtbout.write('{0} No data\n'.format(logtype).encode('ascii', 'ignore'))

    def print_readlwritel(self, rtbout, rtb_ptr, logtype):
        data = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'data')
        caller = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'caller')
        func = self.get_fun_name(caller)
        line = self.get_caller(caller)
        rtbout.write('{0} from address {1:x} called from addr {2:x} {3} {4}\n'.format(
            logtype, data, caller, func, line).encode('ascii', 'ignore'))

    def print_logbuf(self, rtbout, rtb_ptr, logtype):
        data = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'data')
        caller = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'caller')
        func = self.get_fun_name(caller)
        line = self.get_caller(caller)
        rtbout.write('{0} log end {1:x} called from addr {2:x} {3} {4}\n'.format(
            logtype, data, caller, func, line).encode('ascii', 'ignore'))

    def print_hotplug(self, rtbout, rtb_ptr, logtype):
        data = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'data')
        caller = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'caller')
        func = self.get_fun_name(caller)
        line = self.get_caller(caller)
        rtbout.write('{0} cpu data {1:x} called from addr {2:x} {3} {4}\n'.format(
            logtype, data, caller, func, line).encode('ascii', 'ignore'))

    def print_ctxid(self, rtbout, rtb_ptr, logtype):
        data = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'data')
        caller = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'caller')
        func = self.get_fun_name(caller)
        line = self.get_caller(caller)
        rtbout.write('{0} context id {1:x} called from addr {2:x} {3} {4}\n'.format(
            logtype, data, caller, func, line).encode('ascii', 'ignore'))

    def print_timestamp(self, rtbout, rtb_ptr, logtype):
        data = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'data')
        caller = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'caller')
        rtbout.write('{0} Timestamp: {1:x}{2:x}\n'.format(
            logtype, data, caller).encode('ascii', 'ignore'))

    def print_cp_rw(self, rtbout, rtb_ptr, logtype):
        data = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'data')
        caller = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'caller')
        func = self.get_fun_name(caller)
        line = self.get_caller(caller)
        rtbout.write('{0} from offset {1:x} called from addr {2:x} {3} {4}\n'.format(
            logtype, data, caller, func, line).encode('ascii', 'ignore'))

    def print_irq(self, rtbout, rtb_ptr, logtype):
        data = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'data')
        caller = self.ramdump.read_structure_field(rtb_ptr, 'struct msm_rtb_layout', 'caller')
        func = self.get_fun_name(caller)
        line = self.get_caller(caller)
        rtbout.write('{0} interrupt {1:x} handled from addr {2:x} {3} {4}\n'.format(
            logtype, data, caller, func, line).encode('ascii', 'ignore'))

    def parse(self):
        rtb = self.ramdump.addr_lookup('msm_rtb')
        if rtb is None:
            print_out_str(
                '[!] RTB was not enabled in this build. No RTB files will be generated')
            return
        self.name_lookup_table = self.ramdump.gdbmi.get_enum_lookup_table(
            'logk_event_type', 32)
        step_size_offset = self.ramdump.field_offset(
            'struct msm_rtb_state', 'step_size')
        nentries_offset = self.ramdump.field_offset(
            'struct msm_rtb_state', 'nentries')
        rtb_entry_offset = self.ramdump.field_offset(
            'struct msm_rtb_state', 'rtb')
        idx_offset = self.ramdump.field_offset('struct msm_rtb_layout', 'idx')
        rtb_entry_size = self.ramdump.sizeof('struct msm_rtb_layout')
        step_size = self.ramdump.read_u32(rtb + step_size_offset)
        total_entries = self.ramdump.read_int(rtb + nentries_offset)
        rtb_read_ptr = self.ramdump.read_word(rtb + rtb_entry_offset)
        if step_size is None or step_size > self.ramdump.get_num_cpus():
            print_out_str('RTB dump looks corrupt! Got step_size=%s' %
                          hex(step_size) if step_size is not None else None)
            return
        for i in range(0, step_size):
            rtb_out = self.ramdump.open_file('msm_rtb{0}.txt'.format(i))
            gdb_cmd = NamedTemporaryFile(mode='w+t', delete=False)
            gdb_out = NamedTemporaryFile(mode='w+t', delete=False)
            mask = self.ramdump.read_int(rtb + nentries_offset) - 1
            if step_size == 1:
                last = self.ramdump.read_int(
                    self.ramdump.addr_lookup('msm_rtb_idx'))
            else:
                last = self.ramdump.read_int(self.ramdump.addr_lookup(
                    'msm_rtb_idx_cpu'), cpu=i )
            last = last & mask
            last_ptr = 0
            next_ptr = 0
            next_entry = 0
            while True:
                next_entry = (last + step_size) & mask
                last_ptr = rtb_read_ptr + last * rtb_entry_size + idx_offset
                next_ptr = rtb_read_ptr + next_entry * \
                    rtb_entry_size + idx_offset
                a = self.ramdump.read_int(last_ptr)
                b = self.ramdump.read_int(next_ptr)
                if a < b:
                    last = next_entry
                if next_entry != last:
                    break
            stop = 0
            rtb_logtype_offset = self.ramdump.field_offset(
                'struct msm_rtb_layout', 'log_type')
            rtb_idx_offset = self.ramdump.field_offset(
                'struct msm_rtb_layout', 'idx')
            while True:
                ptr = rtb_read_ptr + next_entry * rtb_entry_size
                stamp = self.ramdump.read_int(ptr + rtb_idx_offset)
                if stamp is None:
                    break
                rtb_out.write('{0:x} '.format(stamp).encode('ascii', 'ignore'))
                item = self.ramdump.read_byte(ptr + rtb_logtype_offset)
                item = item & 0x7F
                name_str = '(unknown)'
                if item >= len(self.name_lookup_table) or item < 0:
                    self.print_none(rtb_out, ptr, None)
                else:
                    name_str = self.name_lookup_table[item]
                    if name_str not in print_table:
                        self.print_none(rtb_out, ptr, None)
                    else:
                        func = print_table[name_str]
                        getattr(RTB, func)(self, rtb_out, ptr, name_str)
                if next_entry == last:
                    stop = 1
                next_entry = (next_entry + step_size) & mask
                if (stop == 1):
                    break
            print_out_str('Wrote RTB to msm_rtb{0}.txt'.format(i))
            rtb_out.close()
