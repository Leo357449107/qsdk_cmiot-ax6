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

import struct
import linux_list as llist
import re
import shutil
import os
import platform
import subprocess

from print_out import print_out_str
from watchdog_v3 import TZRegDump_v3

MEMDUMPV2_MAGIC = 0x42445953

class DebugImage_v3():

    def __init__(self):
        self.dump_type_lookup_table = []
        self.dump_table_id_lookup_table = []
        self.dump_data_id_lookup_table  = []

    def parse_cpu_ctx(self, version, start, end, client_id, ram_dump):
        if ram_dump.Is_Hawkeye():
            core = 0

        print_out_str(
           'Parsing CPU{2} context start 0x{0:08x} end 0x{1:x}'.format(
                                                             start, end, core))
        context_dump_pointer_address = ram_dump.read_u32(start, False)
        data_address = context_dump_pointer_address + 0x8
        data = ram_dump.read_cstring(data_address, 8, False)
        print_out_str('Reading from address (0x{1:x}) (WDT String) : {2}'.format(context_dump_pointer_address, data_address, data))
        data = ram_dump.read_u32(0x8600764, False)
        print_out_str('Reading from address 0x{0:x} : 0x{1:x}'.format(0x8600764, data))
        regs = TZRegDump_v3()
        if regs.init_regs(version, start, end, core, ram_dump) is False:
            print_out_str('!!! Could not get registers from TZ dump')
            return
        regs.dump_core_pc(ram_dump)
        regs.dump_all_regs(ram_dump)
