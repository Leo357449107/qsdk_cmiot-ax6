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

# Disabled pmic-reg extraction for all the chipsets
# @register_parser('--pmic-reg', 'Print the pmic register info')

class PMIC(RamParser):
    def parse(self):
        if not self.ramdump.Is_Hawkeye():
           print_out_str("--- PMIC register extraction only for Hawkeye dumps!!")
           return

        pmic_out = self.ramdump.open_file('pmic_regs.txt')

        dump_offset = 0x8600658
        regs = PMICRegDump()
        init_state = regs.init_pmic_regs(self.ramdump, dump_offset)
        if not init_state:
            print_out_str("PMIC Registers not found")
            return
        self.dump_pmic_regs(regs, pmic_out)

        pmic_out.close()
        print_out_str("--- wrote pmic registers into pmic_regs.txt")

    def dump_pmic_regs(self, pmic_regs, pmic_out):
        pmic_out.write("PMIC Registers Info:\n")
        pmic_out.write("--------------------------\n")
        pmic_out.write("Version: {0} Magic: {1} Name: {2}\n".format(pmic_regs.version, pmic_regs.magic, pmic_regs.name))
        pmic_out.write("Start Address: 0x{0:x} Length: 0x{1:x} Count: 0x{2:x}\n\n".format(pmic_regs.start_addr, pmic_regs.length, pmic_regs.count))

        pmic_out.write("======================================================\n")
        pmic_out.write("Slave ID	PMIC Register		Value\n")
        pmic_out.write("======================================================\n")
        for i in xrange(pmic_regs.count):
            pmic_out.write("0x{0:x}		0x{1:x}			0x{2:x}\n".format(pmic_regs.pmic_regs[i][0], pmic_regs.pmic_regs[i][1], pmic_regs.pmic_regs[i][2]))
        pmic_out.write("======================================================\n")

class PMICRegDump():

    def __init__(self):
        self.magic = None
        self.version = 0
        self.name = None
        self.start_addr = 0
        self.length = 0
        self.count = 0
        self.pmic_regs = []
        self.pmic_dump_addr = 0
        self.pmic_dump_offset = 0x62000

    def init_pmic_regs(self, ramdump, dump_offset):
        crash_dump_addr = ramdump.read_dword(dump_offset, False)
        self.pmic_dump_addr = crash_dump_addr + self.pmic_dump_offset

        addr_validity = ramdump.check_addr_validity(self.pmic_dump_addr, ramdump.ebi_files)
        if not addr_validity:
            return 0
        self.version = ramdump.read_u32(self.pmic_dump_addr, False)
        self.magic = ramdump.read_cstring(self.pmic_dump_addr + 4, 4, False)
        self.name = ramdump.read_cstring(self.pmic_dump_addr + 8, 32, False)

        self.start_addr = ramdump.read_u64(self.pmic_dump_addr + 40, False)
        self.length = ramdump.read_u64(self.pmic_dump_addr + 48, False)
        self.count = ramdump.read_u32(self.pmic_dump_addr + 56, False)

        regs_start_addr = self.pmic_dump_addr + 60
        reg_offset = 0x4
        value_offset = 0x8
        next_reg_offset = 0xc
        curr_addr = regs_start_addr
        for i in xrange(self.count):
            addr_validity = ramdump.check_addr_validity(curr_addr + reg_offset, ramdump.ebi_files)
            if not addr_validity:
                return 0
            slaveid = ramdump.read_u32(curr_addr, False)
            reg_val = ramdump.read_u32(curr_addr + reg_offset, False)
            value = ramdump.read_u32(curr_addr + value_offset, False)
            reg = [slaveid, reg_val, value]
            self.pmic_regs.append(reg)
            #print("Index : {0} : {1}".format(i, reg));
            curr_addr = curr_addr + next_reg_offset
        return 1

