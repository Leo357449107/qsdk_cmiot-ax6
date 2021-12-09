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

from parser_util import register_parser, RamParser
from print_out import print_out_str
import sys

# Disabled cpr-reg extraction for all the chipsets
# @register_parser('--cpr-reg', 'Print the cpr register info')

class CPR(RamParser):
    def parse(self):
        if not self.ramdump.Is_Hawkeye():
           print_out_str("--- CPR register extraction only for Hawkeye dumps!!")
           return

        cpr_out = self.ramdump.open_file('cpr_regs.txt')

        dump_offset = 0x8600658
        regs = CPRRegDump()
        init_state = regs.init_cpr_regs(self.ramdump, dump_offset)
        if not init_state:
            print_out_str("CPR Registers not found")
            return
        self.dump_cpr_regs(regs, cpr_out)

        cpr_out.close()
        print_out_str("--- wrote cpr registers into cpr_regs.txt")

    def dump_cpr_regs(self, cpr_regs, cpr_out):
        cpr_out.write("CPR Registers Info:\n")
        cpr_out.write("--------------------------\n")
        cpr_out.write("Version: {0}\n".format(cpr_regs.version))
        cpr_out.write("Start Address: 0x{0:x} Count: 0x{1:x}\n\n".format(cpr_regs.start_addr, cpr_regs.count))

        cpr_out.write("======================================================\n")
        cpr_out.write("	CPR Register		Value\n")
        cpr_out.write("======================================================\n")
        for i in xrange(cpr_regs.count):
            cpr_out.write("	  0x{0:x}		0x{1:x}\n".format(cpr_regs.cpr_regs[i][0], cpr_regs.cpr_regs[i][1]))
        cpr_out.write("======================================================\n")

class CPRRegDump():

    def __init__(self):
        self.version = 0
        self.start_addr = 0
        self.count = 0
        self.cpr_regs = []
        self.cpr_dump_addr = 0
        self.cpr_dump_offset = 0x65438

    def init_cpr_regs(self, ramdump, dump_offset):
        crash_dump_addr = ramdump.read_dword(dump_offset, False)
        self.cpr_dump_addr = crash_dump_addr + self.cpr_dump_offset

        addr_validity = ramdump.check_addr_validity(self.cpr_dump_addr, ramdump.ebi_files)
        if not addr_validity:
            return 0
        self.version = ramdump.read_u32(self.cpr_dump_addr, False)
        self.start_addr = ramdump.read_u64(self.cpr_dump_addr + 40, False)
        self.count = ramdump.read_u32(self.cpr_dump_addr + 56, False)

	regs_start_addr = self.start_addr
	reg_offset = 0x4
        value_offset = 0x8
        next_reg_offset = 0x8
        curr_addr = regs_start_addr
        for i in xrange(self.count):
            addr_validity = ramdump.check_addr_validity(curr_addr + reg_offset, ramdump.ebi_files)
            if not addr_validity:
                return 0
            reg_val = ramdump.read_u32(curr_addr + reg_offset, False)
            value = ramdump.read_u32(curr_addr + value_offset, False)
            reg = [reg_val, value]
            self.cpr_regs.append(reg)
            curr_addr = curr_addr + next_reg_offset
        return 1
