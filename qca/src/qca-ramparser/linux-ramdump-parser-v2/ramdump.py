# Copyright (c) 2012-2015,2019 The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import sys
import re
import os
import struct
import gzip
import functools
import string
import random
import platform
import stat
import glob
import shutil

from boards import get_supported_boards, get_supported_ids
from tempfile import NamedTemporaryFile

import gdbmi
from print_out import print_out_str
from mmu import Armv7MMU, Armv7LPAEMMU, Armv8MMU
import parser_util
import linux_list as llist

FP = 11
SP = 13
LR = 14
PC = 15

# The smem code is very stable and unlikely to go away or be changed.
# Rather than go through the hassel of parsing the id through gdb,
# just hard code it

SMEM_HW_SW_BUILD_ID = 0x89
BUILD_ID_LENGTH = 32

first_mem_file_names = ['EBICS0.BIN',
                        'EBI1.BIN', 'DDRCS0.BIN', 'ebi1_cs0.bin', 'DDRCS0_0.BIN']
extra_mem_file_names = ['EBI1CS1.BIN', 'DDRCS1.BIN', 'ebi1_cs1.bin', 'DDRCS0_1.BIN', 'DDRCS1_0.BIN', 'DDRCS1_1.BIN']


class RamDump():

    class Unwinder ():

        class Stackframe ():

            def __init__(self, fp, sp, lr, pc):
                self.fp = fp
                self.sp = sp
                self.lr = lr
                self.pc = pc

        class UnwindCtrlBlock ():

            def __init__(self):
                self.vrs = 16 * [0]
                self.insn = 0
                self.entries = -1
                self.byte = -1
                self.index = 0

        def __init__(self, ramdump):
            start = ramdump.addr_lookup('__start_unwind_idx')
            end = ramdump.addr_lookup('__stop_unwind_idx')
            self.ramdump = ramdump
            if (start is None) or (end is None):
                if ramdump.arm64:
                    self.unwind_frame = self.unwind_frame_generic64
                else:
                    self.unwind_frame = self.unwind_frame_generic
                return None
            # addresses
            self.unwind_frame = self.unwind_frame_tables
            self.start_idx = start
            self.stop_idx = end
            self.unwind_table = []
            i = 0
            for addr in range(start, end, 8):
                r = ramdump.read_string(addr, '<II')
                if r is None:
                    break
                (a, b) = r
                self.unwind_table.append((a, b, start + 8 * i))
                i += 1

            ver = ramdump.version
            if re.search('3.0.\d', ver) is not None:
                self.search_idx = self.search_idx_3_0
            else:
                self.search_idx = self.search_idx_3_4
                # index into the table
                self.origin = self.unwind_find_origin()

        def unwind_find_origin(self):
            start = 0
            stop = len(self.unwind_table)
            while (start < stop):
                mid = start + ((stop - start) >> 1)
                if (self.unwind_table[mid][0] >= 0x40000000):
                    start = mid + 1
                else:
                    stop = mid
            return stop

        def unwind_frame_generic64(self, frame, trace=False):
            fp = frame.fp
            low = frame.sp
            mask = (self.ramdump.thread_size) - 1
            high = (low + mask) & (~mask)

            if (fp < low or fp > high or fp & 0xf):
                return

            frame.sp = fp + 0x10
            frame.fp = self.ramdump.read_word(fp)
            frame.pc = self.ramdump.read_word(fp + 8)
            return 0

        def unwind_frame_generic(self, frame, trace=False):
            high = 0
            fp = frame.fp

            low = frame.sp
            mask = (self.ramdump.thread_size) - 1

            high = (low + mask) & (~mask)  # ALIGN(low, THREAD_SIZE)

            # /* check current frame pointer is within bounds */
            if (fp < (low + 12) or fp + 4 >= high):
                return -1

            fp_is_at = self.ramdump.read_word(frame.fp - 12)
            sp_is_at = self.ramdump.read_word(frame.fp - 8)
            pc_is_at = self.ramdump.read_word(frame.fp - 4)

            frame.fp = fp_is_at
            frame.sp = sp_is_at
            frame.pc = pc_is_at

            return 0

        def walk_stackframe_generic(self, frame):
            while True:
                symname = self.ramdump.addr_to_symbol(frame.pc)
                print_out_str(symname)

                ret = self.unwind_frame_generic(frame)
                if ret < 0:
                    break

        def unwind_backtrace_generic(self, sp, fp, pc):
            frame = self.Stackframe()
            frame.fp = fp
            frame.pc = pc
            frame.sp = sp
            walk_stackframe_generic(frame)

        def search_idx_3_4(self, addr):
            start = 0
            stop = len(self.unwind_table)
            orig = addr

            if (addr < self.start_idx):
                stop = self.origin
            else:
                start = self.origin

            if (start >= stop):
                return None

            addr = (addr - self.unwind_table[start][2]) & 0x7fffffff

            while (start < (stop - 1)):
                mid = start + ((stop - start) >> 1)

                dif = (self.unwind_table[mid][2]
                       - self.unwind_table[start][2])
                if ((addr - dif) < self.unwind_table[mid][0]):
                    stop = mid
                else:
                    addr = addr - dif
                    start = mid

            if self.unwind_table[start][0] <= addr:
                return self.unwind_table[start]
            else:
                return None

        def search_idx_3_0(self, addr):
            first = 0
            last = len(self.unwind_table)
            while (first < last - 1):
                mid = first + ((last - first + 1) >> 1)
                if (addr < self.unwind_table[mid][0]):
                    last = mid
                else:
                    first = mid

            return self.unwind_table[first]

        def unwind_get_byte(self, ctrl):

            if (ctrl.entries <= 0):
                print_out_str('unwind: Corrupt unwind table')
                return 0

            val = self.ramdump.read_word(ctrl.insn)

            ret = (val >> (ctrl.byte * 8)) & 0xff

            if (ctrl.byte == 0):
                ctrl.insn += 4
                ctrl.entries -= 1
                ctrl.byte = 3
            else:
                ctrl.byte -= 1

            return ret

        def unwind_exec_insn(self, ctrl, trace=False):
            insn = self.unwind_get_byte(ctrl)

            if ((insn & 0xc0) == 0x00):
                ctrl.vrs[SP] += ((insn & 0x3f) << 2) + 4
                if trace:
                    print_out_str(
                        '    add {0} to stack'.format(((insn & 0x3f) << 2) + 4))
            elif ((insn & 0xc0) == 0x40):
                ctrl.vrs[SP] -= ((insn & 0x3f) << 2) + 4
                if trace:
                    print_out_str(
                        '    subtract {0} from stack'.format(((insn & 0x3f) << 2) + 4))
            elif ((insn & 0xf0) == 0x80):
                vsp = ctrl.vrs[SP]
                reg = 4

                insn = (insn << 8) | self.unwind_get_byte(ctrl)
                mask = insn & 0x0fff
                if (mask == 0):
                    print_out_str("unwind: 'Refuse to unwind' instruction")
                    return -1

                # pop R4-R15 according to mask */
                load_sp = mask & (1 << (13 - 4))
                while (mask):
                    if (mask & 1):
                        ctrl.vrs[reg] = self.ramdump.read_word(vsp)
                        if trace:
                            print_out_str(
                                '    pop r{0} from stack'.format(reg))
                        if ctrl.vrs[reg] is None:
                            return -1
                        vsp += 4
                    mask >>= 1
                    reg += 1
                if not load_sp:
                    ctrl.vrs[SP] = vsp

            elif ((insn & 0xf0) == 0x90 and (insn & 0x0d) != 0x0d):
                if trace:
                    print_out_str(
                        '    set SP with the value from {0}'.format(insn & 0x0f))
                ctrl.vrs[SP] = ctrl.vrs[insn & 0x0f]
            elif ((insn & 0xf0) == 0xa0):
                vsp = ctrl.vrs[SP]
                a = list(range(4, 4 + (insn & 7)))
                a.append(4 + (insn & 7))
                # pop R4-R[4+bbb] */
                for reg in (a):
                    ctrl.vrs[reg] = self.ramdump.read_word(vsp)
                    if trace:
                        print_out_str('    pop r{0} from stack'.format(reg))

                    if ctrl.vrs[reg] is None:
                        return -1
                    vsp += 4
                if (insn & 0x80):
                    if trace:
                        print_out_str('    set LR from the stack')
                    ctrl.vrs[14] = self.ramdump.read_word(vsp)
                    if ctrl.vrs[14] is None:
                        return -1
                    vsp += 4
                ctrl.vrs[SP] = vsp
            elif (insn == 0xb0):
                if trace:
                    print_out_str('    set pc = lr')
                if (ctrl.vrs[PC] == 0):
                    ctrl.vrs[PC] = ctrl.vrs[LR]
                ctrl.entries = 0
            elif (insn == 0xb1):
                mask = self.unwind_get_byte(ctrl)
                vsp = ctrl.vrs[SP]
                reg = 0

                if (mask == 0 or mask & 0xf0):
                    print_out_str('unwind: Spare encoding')
                    return -1

                # pop R0-R3 according to mask
                while mask:
                    if (mask & 1):
                        ctrl.vrs[reg] = self.ramdump.read_word(vsp)
                        if trace:
                            print_out_str(
                                '    pop r{0} from stack'.format(reg))
                        if ctrl.vrs[reg] is None:
                            return -1
                        vsp += 4
                    mask >>= 1
                    reg += 1
                ctrl.vrs[SP] = vsp
            elif (insn == 0xb2):
                uleb128 = self.unwind_get_byte(ctrl)
                if trace:
                    print_out_str(
                        '    Adjust sp by {0}'.format(0x204 + (uleb128 << 2)))

                ctrl.vrs[SP] += 0x204 + (uleb128 << 2)
            else:
                print_out_str('unwind: Unhandled instruction')
                return -1

            return 0

        def prel31_to_addr(self, addr):
            value = self.ramdump.read_word(addr)
            # offset = (value << 1) >> 1
            # C wants this sign extended. Python doesn't do that.
            # Sign extend manually.
            if (value & 0x40000000):
                offset = value | 0x80000000
            else:
                offset = value

            # This addition relies on integer overflow
            # Emulate this behavior
            temp = addr + offset
            return (temp & 0xffffffff) + ((temp >> 32) & 0xffffffff)

        def unwind_frame_tables(self, frame, trace=False):
            low = frame.sp
            high = ((low + (self.ramdump.thread_size - 1)) & \
                ~(self.ramdump.thread_size - 1)) + self.ramdump.thread_size
            idx = self.search_idx(frame.pc)

            if (idx is None):
                if trace:
                    print_out_str("can't find %x" % frame.pc)
                return -1

            ctrl = self.UnwindCtrlBlock()
            ctrl.vrs[FP] = frame.fp
            ctrl.vrs[SP] = frame.sp
            ctrl.vrs[LR] = frame.lr
            ctrl.vrs[PC] = 0

            if (idx[1] == 1):
                return -1

            elif ((idx[1] & 0x80000000) == 0):
                ctrl.insn = self.prel31_to_addr(idx[2] + 4)

            elif (idx[1] & 0xff000000) == 0x80000000:
                ctrl.insn = idx[2] + 4
            else:
                print_out_str('not supported')
                return -1

            val = self.ramdump.read_word(ctrl.insn)

            if ((val & 0xff000000) == 0x80000000):
                ctrl.byte = 2
                ctrl.entries = 1
            elif ((val & 0xff000000) == 0x81000000):
                ctrl.byte = 1
                ctrl.entries = 1 + ((val & 0x00ff0000) >> 16)
            else:
                return -1

            while (ctrl.entries > 0):
                urc = self.unwind_exec_insn(ctrl, trace)
                if (urc < 0):
                    return urc
                if (ctrl.vrs[SP] < low or ctrl.vrs[SP] >= high):
                    return -1

            if (ctrl.vrs[PC] == 0):
                ctrl.vrs[PC] = ctrl.vrs[LR]

            # check for infinite loop */
            if (frame.pc == ctrl.vrs[PC]):
                return -1

            frame.fp = ctrl.vrs[FP]
            frame.sp = ctrl.vrs[SP]
            frame.lr = ctrl.vrs[LR]
            frame.pc = ctrl.vrs[PC]

            return 0

        def unwind_backtrace(self, sp, fp, pc, lr, extra_str='', out_file=None, trace=False):
            offset = 0
            frame = self.Stackframe(fp, sp, lr, pc)
            frame.fp = fp
            frame.sp = sp
            frame.lr = lr
            frame.pc = pc

            while True:
                where = frame.pc
                offset = 0

                if frame.pc is None:
                    break

                modname = None
                symname = None
                symtab_st_size = None
                r = self.ramdump.unwind_lookup(frame.pc)
                if r is None:
                    symname = 'UNKNOWN'
                    offset = 0x0
                if r is not None and len(r) > 3:
                    symname, offset, modname, symtab_st_size = r

                if (modname is not None and symtab_st_size is not None):
                    pstring = (
                        extra_str + '[<0x{0:x}>] {1}+0x{2:x}/0x{4:x} [{3}.ko]'.format(frame.pc, symname, offset, modname, symtab_st_size))
                elif (modname is None and symtab_st_size is not None):
                    pstring = (
                        extra_str + '[<0x{0:x}>] {1}+0x{2:x}/0x{3:x}'.format(frame.pc, symname, offset, symtab_st_size))
                else:
                    pstring = (
                        extra_str + '[<0x{0:x}>] {1}+0x{2:x}'.format(frame.pc, symname, offset))
                if out_file:
                    out_file.write(pstring + '\n')
                else:
                    print_out_str(pstring)

                urc = self.unwind_frame(frame, trace)
                if urc < 0:
                    break

        def arm_symbol_mapping(self, sym):
            sym1="atd"
            if len(sym)>=3:

                if (sym[0] == '$' and sym1.find(sym[1]) and (sym[2] == '\0' or sym[2] == '.')):
                    return 1
                else:
                    return 0
            else:
                return 0

        def mod_get_symbol(self, mod_list, mod_sec_addr, val):
            if (re.search('3.14.77', self.ramdump.version) is not None or (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (4, 4)):
                if self.ramdump.kallsyms_offset >= 0:
                    kallsyms = self.ramdump.read_word(mod_list + self.ramdump.kallsyms_offset);
                    module_symtab_count = self.ramdump.read_u32(kallsyms + self.ramdump.module_symtab_count_offset)
                    module_strtab = self.ramdump.read_word(kallsyms + self.ramdump.module_strtab_offset)
                    module_symtab = self.ramdump.read_word(kallsyms + self.ramdump.module_symtab_offset)
                else:
                    kallsyms = -1
                    module_symtab_count = -1
                    module_strtab = -1
                    module_symtab = -1
            else:
                module_symtab_count = self.ramdump.read_word(mod_list + self.ramdump.module_symtab_count_offset)
                module_symtab = self.ramdump.read_word(mod_list + self.ramdump.module_symtab_offset)
                module_strtab = self.ramdump.read_word(mod_list + self.ramdump.module_strtab_offset)

            if (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (5, 4):
                module_init_text_size = self.ramdump.read_u32(mod_list + self.ramdump.module_layout_init_offset + self.ramdump.module_text_size_offset)
                module_core_text_size = self.ramdump.read_u32(mod_list + self.ramdump.module_layout_core_offset + self.ramdump.module_text_size_offset)
            else:
                module_init_text_size = self.ramdump.read_u32(mod_list + self.ramdump.module_init_text_size_offset)
                module_core_text_size = self.ramdump.read_u32(mod_list + self.ramdump.module_core_text_size_offset)
            name = self.mod_addr_name
            best = 0
            addr = self.mod_addr
            module_symtab_orig = module_symtab
            strtab_name = None

            if (val == 0):
                nextval = self.ramdump.read_u32(mod_sec_addr + module_init_text_size)
            else:
                nextval = self.ramdump.read_u32(mod_sec_addr + module_core_text_size)
            try:
                for i in range(1, module_symtab_count):
                    module_symtab += self.ramdump.symtab_size
                    symtab_st_shndx = self.ramdump.read_u16(module_symtab + self.ramdump.symtab_st_shndx_offset);
                    if (self.ramdump.isELF32()):
                        symtab_st_info = self.ramdump.read_byte(module_symtab + self.ramdump.symtab_st_info_offset)
                    else:
                        symtab_st_info = self.ramdump.read_u16(module_symtab + self.ramdump.symtab_st_info_offset);


                    if (symtab_st_shndx != 0 and symtab_st_info != 'U'):
                        module_best_symtab = module_symtab_orig + (best * self.ramdump.symtab_size)
                        symtab_best_st_value = self.ramdump.read_word(module_best_symtab + self.ramdump.symtab_st_value_offset)
                        symtab_st_value = self.ramdump.read_word(module_symtab + self.ramdump.symtab_st_value_offset)
                        symtab_st_name = self.ramdump.read_u32(module_symtab + self.ramdump.symtab_st_name_offset)
                        strtab_name = self.ramdump.read_cstring(module_strtab + symtab_st_name, 40)

                    if (strtab_name):
                        if (symtab_st_value <= addr and symtab_st_value > symtab_best_st_value and
                            strtab_name[0] != '\0' and self.arm_symbol_mapping(strtab_name) == 0):
                            best = i

                        if (symtab_st_value > addr and symtab_st_value < nextval and strtab_name[0] != '\0'
                           and self.arm_symbol_mapping(strtab_name) == 0):
                           nextval = symtab_st_value
            except MemoryError:
                 pass #print_out_str('MemoryError caught here')
            if (best == 0):
                gs = self.ramdump.gdbmi.get_symbol_info(addr)
                if gs is not None:
                    self.sym_name = gs.symbol
                    self.sym_off = gs.offset
                else:
                    self.sym_name = "UNKNOWN"
                    self.sym_off = 0
                #print_out_str('not able to resolve addr 0x{0} in module section'.format(addr))
                #return None
            else:
                module_best_symtab = module_symtab_orig + (best * self.ramdump.symtab_size)
                symtab_best_st_name = self.ramdump.read_u32(module_best_symtab + self.ramdump.symtab_st_name_offset)
                symtab_best_st_value = self.ramdump.read_word(module_best_symtab + self.ramdump.symtab_st_value_offset)
                symtab_st_size = self.ramdump.read_word(module_best_symtab + self.ramdump.symtab_st_size_offset)
                offset = addr - symtab_best_st_value
                symbol = self.ramdump.read_cstring(module_strtab + symtab_best_st_name, 50)
                self.sym_name = symbol
                self.sym_off = offset
                self.symtab_st_size = symtab_st_size

        def mod_addr_func(self, mod_list):

            high_mem_addr = self.ramdump.addr_lookup('high_memory')
            vmalloc_offset = 0x800000
            vmalloc_start = self.ramdump.read_u32(high_mem_addr) + vmalloc_offset & (~int(vmalloc_offset - 0x1))

            if(self.ramdump.Is_Hawkeye() and self.ramdump.isELF64() and (mod_list & 0xfff0000000 != self.ramdump.mod_start_addr)):
                return
            elif(self.ramdump.isELF32() and self.ramdump.Is_Hawkeye() and mod_list & 0xff000000 != self.ramdump.mod_start_addr and not ((vmalloc_start & 0xff000000 <= mod_list & 0xff000000) and (mod_list & 0xff000000 <= 0xff000000))):
                return
            elif(not self.ramdump.Is_Hawkeye() and self.ramdump.isELF32() and (mod_list & 0xff000000 != self.ramdump.mod_start_addr)):
                return

            name = self.ramdump.read_cstring(mod_list + self.ramdump.mod_name_offset, 30)

            if name is None or len(name) <= 1:
                return

            if (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (5, 4):
                module_init_addr = self.ramdump.read_word(mod_list + self.ramdump.module_layout_init_offset + self.ramdump.module_offset)
                module_init_size = self.ramdump.read_u32(mod_list + self.ramdump.module_layout_init_offset + self.ramdump.module_size_offset)
                module_core_addr = self.ramdump.read_word(mod_list + self.ramdump.module_layout_core_offset + self.ramdump.module_offset)
                module_core_size = self.ramdump.read_u32(mod_list + self.ramdump.module_layout_core_offset + self.ramdump.module_size_offset)
            else:
                module_init_addr = self.ramdump.read_word(mod_list + self.ramdump.module_init_offset)
                module_init_size = self.ramdump.read_u32(mod_list + self.ramdump.module_init_size_offset)
                module_core_addr = self.ramdump.read_word(mod_list + self.ramdump.module_core_offset)
                module_core_size = self.ramdump.read_u32(mod_list + self.ramdump.module_core_size_offset)

            if ((module_init_size > 0) and (module_init_addr <= self.mod_addr) and (self.mod_addr < (module_init_addr + module_init_size))):
                    self.mod_addr_name = name
                    self.mod_get_symbol(mod_list, module_init_addr, 0)
            else:
                if (module_core_addr <= self.mod_addr and self.mod_addr < (module_core_addr + module_core_size)):
                    self.mod_addr_name = name
                    self.mod_get_symbol(mod_list, module_core_addr, 1)


        def get_module_name_from_addr(self, addr):

            if (self.ramdump.mod_start == 0 or self.ramdump.mod_start is None):
                print_out_str("cannot get the modules start addr");
                return None

            self.mod_addr = addr
            self.mod_addr_name = None
            self.sym_name = None
            self.sym_off = 0
            self.symtab_st_size = 0

            list_walker = llist.ListWalker(self.ramdump, self.ramdump.mod_start, self.ramdump.next_mod_offset)
            list_walker.walk(self.ramdump.mod_start, self.mod_addr_func)

            #if (self.sym_name is not None and self.sym_off != 0 and self.mod_addr_name is not None):
            if (self.sym_name is not None and self.mod_addr_name is not None):
                return(self.sym_name, self.sym_off, self.mod_addr_name, self.symtab_st_size)
            else:
                return None

    def __init__(self, vmlinux_path, nm_path, gdb_path, readelf_path, ko_path, objdump_path, ebi,
                 file_path, phys_offset, outdir,qtf_path, custom, cpu0_reg_path=None, cpu1_reg_path=None,
                 hw_id=None,hw_version=None, arm64=False, page_offset=None,
                 qtf=False, t32_host_system=None, ath11k=None):
        self.ebi_files = []
        self.phys_offset = None
        self.tz_start = 0
        self.ebi_start = 0
        self.cpu_type = None
        self.hw_id = hw_id
        self.hw_version = hw_version
        self.offset_table = []
        self.vmlinux = vmlinux_path
        self.nm_path = nm_path
        self.ko_path = ko_path
        self.gdb_path = gdb_path
        self.readelf_path = readelf_path
        self.objdump_path = objdump_path
        self.outdir = outdir
        self.imem_fname = None
        self.gdbmi = gdbmi.GdbMI(self.gdb_path, self.vmlinux)
        self.gdbmi.open()
        self.arm64 = arm64
        self.thread_size = 8192
        self.qtf_path = qtf_path
        self.qtf = qtf
        self.t32_host_system = t32_host_system
        self.cpu0_reg_path = cpu0_reg_path
        self.cpu1_reg_path = cpu1_reg_path
        self.custom = custom
        self.kernel_version = (0, 0, 0)
        self.ath11k = ath11k

        if self.Is_Ath11k() and readelf_path is not None:
            self.ath11k_path = self.ko_path + "/ath11k.ko"
            if os.path.isfile(self.ath11k_path):
                self.ath11k_gnu_linkonce_this_size = self.get_gnu_linkonce_size(self.readelf_path, self.ath11k_path)
                seg_info_cmd = '{0} {1} --quiet -ex "print &ath11k_coredump_seg_info" -ex "quit" '.format(self.gdb_path, self.ath11k_path)
                seg_info_nm_cmd = '{0} {1} | grep "B ath11k_coredump_seg_info"'.format(self.nm_path, self.ath11k_path)
                fd = os.popen(seg_info_cmd)
                fd_nm = os.popen(seg_info_nm_cmd)
                ret_nm = fd_nm.read()
                ret = fd.read()
                try:
                    start_pos = ret.index(") 0x")
                    self.seg_info_offset = ret[start_pos+2:].strip().split(' ')[0]
                    self.seg_info_offset = self.seg_info_offset and ret_nm[2:].strip().split(' ')[0]
                except:
                    self.seg_info_offset = None
            else:
                self.ath11k_gnu_linkonce_this_size = None
                self.seg_info_offset = None
                print_out_str("ath11k module file not present")

        if self.phys_offset is None:
            self.get_hw_id()

        if self.Is_Hawkeye() and self.isELF32():
            self.page_offset = 0x80000000
            if self.hw_id == 9574:
                self.page_offset = 0x40000000
        else:
            self.page_offset = 0xc0000000

        if self.ko_path is not None and readelf_path is not None:
            #nss driver module path
            self.qca_nss_drv_path = self.ko_path + "/qca-nss-drv.ko"
            if os.path.isfile(self.qca_nss_drv_path):
                #readelf command
                self.gnu_linkonce_this_size = self.get_gnu_linkonce_size(self.readelf_path, self.qca_nss_drv_path)
                #Get offset of  stats_drv[21] variables using gdb command
                nss_top_main_stats_drv_cmd = '{0} {1} --quiet -ex "print &nss_top_main->stats_drv[21]" -ex "quit" '.format(self.gdb_path, self.qca_nss_drv_path)
                f2 = os.popen(nss_top_main_stats_drv_cmd)
                ret2 = f2.read()
                try:
                    start_pos2 = ret2.index(") 0x")
                    self.stats_drv_offset = ret2[start_pos2+2:].strip()
                except:
                    self.stats_drv_offset = None
            else:
                self.gnu_linkonce_this_size = None
                self.stats_drv_offset = None
                print_out_str("qca-nss-drv module file not present")
        else:
            self.gnu_linkonce_this_size = None
            self.stats_drv_offset = None


        if ebi is not None:
            # TODO sanity check to make sure the memory regions don't overlap
            for file_path, start, end in ebi:
                fd = open(file_path, 'rb')
                if not fd:
                    print_out_str(
                        'Could not open {0}. Will not be part of dump'.format(file_path))
                    continue
                self.ebi_files.append((fd, start, end, file_path))
        else:
            if not self.auto_parse(file_path):
                return None
        if self.ebi_start == 0:
            self.ebi_start = self.ebi_files[0][1]
        if phys_offset is not None:
            print_out_str(
                '[!!!] Phys offset was set to {0:x}'.format(phys_offset))
            self.phys_offset = phys_offset
        self.lookup_table = []
        self.stackinfo_cache = {}
        self.config = []
        self.CONFIG_SPARSEMEM = False
        self.CONFIG_SLUB_DEBUG = False
        self.CONFIG_SLUB_DEBUG_ON = False
        self.CONFIG_HIGHMEM = False
        self.CONFIG_DONT_MAP_HOLE_AFTER_MEMBANK0 = False

        if not self.get_version_from_vmlinux():
            print_out_str('!!! Could not get the Linux version from vmlinux!')
            print_out_str('!!! Exiting now')
            sys.exit(1)
        if self.arm64:
            if (self.kernel_version[0], self.kernel_version[1]) >= (5, 4):
                self.page_offset = 0xffffffc010000000
            else:
                self.page_offset = 0xffffffc000000000
            self.thread_size = 16384
        print_out_str('PageOffset was set to {0:x}'.format(self.page_offset))

        if page_offset is not None:
            print_out_str(
                '[!!!] Page offset was set to {0:x}'.format(page_offset))
            self.page_offset = page_offset
        self.setup_symbol_tables()

        if not self.get_version():
            print_out_str('!!! Could not get the Linux version!')
            # self.ebi_start - Starting address of EBICS0.bin
            ebi_filePath = self.ebi_files[0][3]
            fd = open(ebi_filePath, 'rb')
            file_content = fd.read()
            lv = re.search("Linux version", file_content)
            if lv is not None:
                banner_addr_phys = int(hex(lv.start()), 16)
                banner_addr_virt = self.addr_lookup('linux_banner')
                phys_offset = banner_addr_phys - banner_addr_virt + self.ebi_start + self.page_offset
                if self.phys_offset != phys_offset:
                    self.phys_offset = phys_offset
                    print_out_str('[!!!] Physical offset was changed to {0:x}'.format(self.phys_offset))
                    print_out_str('Check for Linux banner match with changed physical offset')
                    if not self.get_version():
                        print_out_str('!!! Your vmlinux is probably wrong for these dumps')
                        print_out_str('!!! Exiting now')
                        sys.exit(1)
                else:
                    print_out_str('!!! Your vmlinux is probably wrong for these dumps')
                    print_out_str('!!! Exiting now')
                    sys.exit(1)
            else:
                print_out_str('!!! Linux banner is not present in dumps')
                print_out_str('!!! Exiting now')
                sys.exit(1)

        # The address of swapper_pg_dir can be used to determine
        # whether or not we're running with LPAE enabled since an
        # extra 4k is needed for LPAE. If it's 0x5000 below
        # PAGE_OFFSET + TEXT_OFFSET then we know we're using LPAE. For
        # non-LPAE it should be 0x4000 below PAGE_OFFSET + TEXT_OFFSET
        self.swapper_pg_dir = self.addr_lookup('swapper_pg_dir')
        if self.swapper_pg_dir is None:
            print_out_str('!!! Could not get the swapper page directory!')
            print_out_str(
                '!!! Your vmlinux is probably wrong for these dumps')
            print_out_str('!!! Exiting now')
            sys.exit(1)
        self.swapper_pg_dir_addr =  self.swapper_pg_dir - self.page_offset
        self.kernel_text_offset = self.addr_lookup('stext') - self.page_offset
        pg_dir_size = self.kernel_text_offset - self.swapper_pg_dir_addr
        if self.arm64:
            print_out_str('Using 64bit MMU')
            self.mmu = Armv8MMU(self)
        elif pg_dir_size == 0x4000:
            print_out_str('Using non-LPAE MMU')
            self.mmu = Armv7MMU(self)
        elif pg_dir_size == 0x5000:
            print_out_str('Using LPAE MMU')
            text_offset = 0x8000
            pg_dir_size = 0x5000    # 0x4000 for non-LPAE
            swapper_pg_dir_addr = self.phys_offset + text_offset - pg_dir_size

            # We deduce ttbr1 and ttbcr.t1sz based on the value of
            # PAGE_OFFSET. This is based on v7_ttb_setup in
            # arch/arm/mm/proc-v7-3level.S:

            # * TTBR0/TTBR1 split (PAGE_OFFSET):
            # *   0x40000000: T0SZ = 2, T1SZ = 0 (not used)
            # *   0x80000000: T0SZ = 0, T1SZ = 1
            # *   0xc0000000: T0SZ = 0, T1SZ = 2
            if self.page_offset == 0x40000000:
                t1sz = 0
            elif self.page_offset == 0x80000000:
                t1sz = 1
            elif self.page_offset == 0xc0000000:
                t1sz = 2
                # need to fixup ttbr1 since we'll be skipping the
                # first-level lookup (see v7_ttb_setup):
                # /* PAGE_OFFSET == 0xc0000000, T1SZ == 2 */
                # add      \ttbr1, \ttbr1, #4096 * (1 + 3) @ only L2 used, skip
                # pgd+3*pmd
                swapper_pg_dir_addr += (4096 * (1 + 3))
            else:
                raise Exception(
                    'Invalid phys_offset for page_table_walk: 0x%x'
                    % self.page_offset)
            self.mmu = Armv7LPAEMMU(self, swapper_pg_dir_addr, t1sz)
        else:
            print_out_str(
                "!!! Couldn't determine whether or not we're using LPAE!")
            print_out_str(
                '!!! This is a BUG in the parser and should be reported.')
            sys.exit(1)

        if not self.get_config():
            print_out_str('!!! Could not get saved configuration')
            print_out_str(
                '!!! This is really bad and probably indicates RAM corruption')
            print_out_str('!!! Some features may be disabled!')
        self.unwind = self.Unwinder(self)

        self.next_mod_offset = self.field_offset('struct module','list')
        self.mod_start = self.read_word('modules')
        self.mod_name_offset = self.field_offset('struct module', 'name')
        if (self.kernel_version[0], self.kernel_version[1]) >= (5, 4):
            self.module_layout_init_offset = self.field_offset('struct module', 'init_layout')
            self.module_layout_core_offset = self.field_offset('struct module', 'core_layout')
            self.module_offset = self.field_offset('struct module_layout', 'base')
            self.module_size_offset = self.field_offset('struct module_layout', 'size')
            self.module_text_size_offset = self.field_offset('struct module_layout', 'text_size')
        else:
            self.module_init_offset = self.field_offset('struct module','module_init')
            self.module_core_offset = self.field_offset('struct module','module_core')
            self.module_init_size_offset = self.field_offset('struct module','init_size')
            self.module_core_size_offset = self.field_offset('struct module','core_size')
            self.module_init_text_size_offset = self.field_offset('struct module','init_text_size')
            self.module_core_text_size_offset = self.field_offset('struct module','core_text_size')
        if (re.search('3.14.77', self.version) is not None or (self.kernel_version[0], self.kernel_version[1]) >= (4, 4)):
            self.kallsyms_offset = self.field_offset('struct module', 'kallsyms')
            if self.kallsyms_offset >= 0:
                self.module_symtab_offset = self.field_offset('struct mod_kallsyms','symtab')
                self.module_strtab_offset = self.field_offset('struct mod_kallsyms','strtab')
                self.module_symtab_count_offset = self.field_offset('struct mod_kallsyms','num_symtab')
                if self.ko_path is not None:
                    print_out_str("CONFIG_KALLSYMS is set. Hence not parsing ko modules generically. Modules would be loaded for specific cases")
            else:
                # CONFIG_KALLSYMS is not set
                self.syms_offset = self.field_offset('struct module', 'syms')
                self.num_syms_offset = self.field_offset('struct module', 'num_syms')
                self.module_symtab_offset = -1
                self.module_strtab_offset = -1
                self.module_symtab_count_offset = -1
                print_out_str("CONFIG_KALLSYMS not set")
                if self.ko_path is not None:
                    list_walker = llist.ListWalker(self, self.mod_start, self.next_mod_offset)
                    list_walker.walk(self.mod_start, self.add_sym_file)

        else:
            self.module_symtab_offset = self.field_offset('struct module','symtab')
            self.module_strtab_offset = self.field_offset('struct module','strtab')
            self.module_symtab_count_offset = self.field_offset('struct module','num_symtab')
        if(self.isELF64()):
            self.symtab_st_shndx_offset = self.field_offset('struct elf64_sym', 'st_shndx')
            self.symtab_st_value_offset = self.field_offset('struct elf64_sym', 'st_value')
            self.symtab_st_info_offset = self.field_offset('struct elf64_sym', 'st_info')
            self.symtab_st_name_offset = self.field_offset('struct elf64_sym', 'st_name')
            self.symtab_st_size_offset = self.field_offset('struct elf64_sym', 'st_size')
        else:
            self.symtab_st_shndx_offset = self.field_offset('struct elf32_sym', 'st_shndx')
            self.symtab_st_value_offset = self.field_offset('struct elf32_sym', 'st_value')
            self.symtab_st_name_offset = self.field_offset('struct elf32_sym', 'st_name')
            self.symtab_st_info_offset = self.field_offset('struct elf32_sym', 'st_info')
            self.symtab_st_size_offset = self.field_offset('struct elf32_sym', 'st_size')

        if (self.isELF64() and (self.kernel_version[0], self.kernel_version[1]) >= (5, 4)):
            self.mod_start_addr = 0xc000000000
        elif(self.isELF64()):
            self.mod_start_addr = 0xbff0000000
        else:
            self.mod_start_addr = 0x7f000000

        if(self.isELF64()):
            self.symtab_size = self.sizeof('struct elf64_sym')
        else:
            self.symtab_size = self.sizeof('struct elf32_sym')

    def add_sym_file(self, mod_list):

        name = self.read_cstring(mod_list + self.mod_name_offset, 50)

        if name is None or len(name) <= 1:
            return

        name = self.ko_path + "/" + name + ".ko"
        g = glob.glob(name.replace("_", "?"))
        if len(g) < 1:
            return

        if (self.kernel_version[0], self.kernel_version[1]) >= (5, 4):
            module_core_addr = self.read_word(mod_list + self.module_layout_core_offset + self.module_offset)
        else:
            module_core_addr = self.read_word(mod_list + self.module_core_offset)
        self.gdbmi.add_sym_file(g[0], module_core_addr)

    def __del__(self):
        self.gdbmi.close()

    def open_file(self, file_name, mode='wb'):
        file_path = os.path.join(self.outdir, file_name)
        f = None
        try:
            f = open(file_path, mode)
        except:
            print_out_str('Could not open path {0}'.format(file_path))
            print_out_str('Do you have write/read permissions on the path?')
            sys.exit(1)
        return f

    def get_config(self):
        kconfig_addr = self.addr_lookup('kernel_config_data')
        if kconfig_addr is None:
            return
        if (self.kernel_version[0], self.kernel_version[1]) >= (5, 4):
            kconfig_addr_end = self.addr_lookup('kernel_config_data_end')
            if kconfig_addr_end is None:
                return
            # kconfig_size doesn't include magic strings
            kconfig_size = kconfig_addr_end - kconfig_addr
            # magic is 8 bytes before kconfig_addr and data
            # starts at kconfig_addr for kernel > 5.4
            # subtract 8 bytes to perform sanity check
            kconfig_addr -= 8
        else:
            kconfig_size = self.sizeof('kernel_config_data')
            # size includes magic, offset from it
            kconfig_size = kconfig_size - 16 - 1
        zconfig = NamedTemporaryFile(mode='wb', delete=False)
        # kconfig data starts with magic 8 byte string, go past that
        s = self.read_cstring(kconfig_addr, 8)
        if s != 'IKCFG_ST':
            return
        kconfig_addr = kconfig_addr + 8
        for i in range(0, kconfig_size):
            val = self.read_byte(kconfig_addr + i)
            zconfig.write(struct.pack('<B', val))

        zconfig.close()
        zconfig_in = gzip.open(zconfig.name, 'rb')
        try:
            t = zconfig_in.readlines()
        except:
            return False
        zconfig_in.close()
        os.remove(zconfig.name)
        for l in t:
            self.config.append(l.rstrip().decode('ascii', 'ignore'))

        if self.is_config_defined('CONFIG_SPARSEMEM'):
            self.CONFIG_SPARSEMEM = True

        if self.is_config_defined('CONFIG_SLUB_DEBUG'):
            self.CONFIG_SLUB_DEBUG = True

        if self.is_config_defined('CONFIG_SLUB_DEBUG_ON'):
            self.CONFIG_SLUB_DEBUG_ON = True

        if self.is_config_defined('CONFIG_HIGHMEM'):
            self.CONFIG_HIGHMEM = True

        if self.is_config_defined('CONFIG_DONT_MAP_HOLE_AFTER_MEMBANK0'):
            self.CONFIG_DONT_MAP_HOLE_AFTER_MEMBANK0 = True

        return True

    def is_config_defined(self, config):
        s = config + '=y'
        return s in self.config

    def get_version_from_vmlinux(self):
        s = '{0} -ex "print linux_banner" -ex "quit" {1}'.format(self.gdb_path, self.vmlinux)
        f = os.popen(s)
        now = f.read()
        try:
                start_pos = now.index("Linux version")
                self.banner = now[start_pos:]
                self.flen = len(self.banner)
                self.flen = self.flen - 4
        except:
                print('not able to find linux banner')
		return False

	v = re.search('Linux version (\d{0,2}\.\d{0,2}\.\d{0,3})', self.banner)
        if v is None:
            print_out_str('!!! Could not match version! {0}'.format(self.banner))
            return False
        self.version = v.group(1)
        match = re.search('(\d+)\.(\d+)\.(\d+)', self.version)
        if match is not None:
            self.kernel_version = tuple(map(int, match.groups()))
        else:
            print_out_str('!!! Could not extract version info! {0}'.format(self.version))
        return True

    def get_version(self):
        banner_addr = self.addr_lookup('linux_banner')
        if banner_addr is not None:
            # Don't try virt to phys yet, compute manually
            banner_addr = banner_addr - self.page_offset + self.phys_offset
            b = self.read_cstring(banner_addr, 256, False)
            if b is None:
                print_out_str('!!! Could not read banner address!')
                return False

            if (format(self.banner[0:self.flen]) != format(b[0:self.flen])):
                # special custom case for gale issues
                if (self.custom is not None and self.custom.lower() == "gale".lower()):
                    print_out_str ("!!! It is a gale issue!")
                    try:
                        #use readbanner utility to read linux_banner
                        lbc = 'readbanner {0} linux_banner'.format(self.vmlinux)
                        lbp = os.popen(lbc)
                        lb = lbp.read()
                        self.banner = lb
                        if (format(self.banner[0:self.flen]) == format(b[0:self.flen])):
                            print_out_str ('readbanner={0}'.format(self.banner))
                        else:
                            print_out_str('!!! linux banner version mismatch!')
                            print_out_str('!!! In vmlinux : {0}'.format(self.banner[0:self.flen]))
                            print_out_str('!!! In Dump    : {0}'.format(b[0:self.flen]))
                            return False
                    except:
                        print_out_str ('!!! Unable to read linux_banner by readbanner utility !!')
                        print_out_str('!!! linux banner version mismatch!')
                        print_out_str('!!! In vmlinux : {0}'.format(self.banner[0:self.flen]))
                        print_out_str('!!! In Dump    : {0}'.format(b[0:self.flen]))
                        return False
                else:
                    print_out_str('!!! linux banner version mismatch!')
                    print_out_str('!!! In vmlinux : {0}'.format(self.banner[0:self.flen]))
                    print_out_str('!!! In Dump    : {0}'.format(b[0:self.flen]))
                    return False

            print_out_str('Linux Banner: ' + b.rstrip())
            print_out_str('version = {0}'.format(self.version))
            return True
        else:
            print_out_str('!!! Could not lookup banner address')
            return False

    def get_command_line(self):
        command_addr = self.addr_lookup('saved_command_line')
        if command_addr is not None:
            command_addr = self.read_word(command_addr)
            cmdline = self.read_cstring(command_addr, 2048)
            if cmdline is None:
                print_out_str('!!! could not read saved command line address')
                return ""
            else:
                return cmdline

    def print_command_line(self):
        cmdline = self.get_command_line()
        if cmdline is not None:
            print_out_str('Command Line: ' + cmdline)
            return True
        else:
            print_out_str('!!! Could not lookup saved command line address')
            return False

    # From the provided device-tree node, will return the values in 'reg' field
    # as a list of strings or as tuples depending on number of groups present
    def dts_lookup(self, node):
        reg = None
        regex=r'0[xX][0-9a-fA-F]+'
        dts_file_path = os.path.join(self.outdir, "devicetree.dts")
        try:
            lookup = False
            with open(dts_file_path, "r") as dts_file:
                for lnum, line in enumerate(dts_file, 1):
                    if node in line:
                        lookup = True
                    if lookup:
                        if '};' in line:
                            lookup = False
                        if 'reg' in line:
                            reg = re.findall(regex, line)
                            lookup = False
            return reg
        except:
            print_out_str("\n!!! Cannot get '{0}' from device tree at {1}".format(node, dts_file_path))
        return None

    def get_q6_etr(self, etr_addr, etr_size):
        fd = None
        for a in self.ebi_files:
            f, start, end, path = a
            # Check if the required etr buffer is within the dump range
            if etr_addr >= start and etr_addr + etr_size <= end:
                fd = f
                offset = etr_addr - start
                break

        end = etr_addr + etr_size

        if fd is None:
            print_out_str("\n!!! etr dump is not found in system dump")
            return None

        try:
            fd.seek(offset)
            dump = fd.read(etr_size)
            #Look for dump Magic, 0xdeadbeef in Little Endian
	    head_pos = dump.find(b'\xef\xbe\xad\xde')
            if head_pos < 0:
                print_out_str("-- Magic '0xdeadbeef' is not found in range")
                return None

	    fd.seek(head_pos + offset)
            magic, status, read_ptr, write_ptr = struct.unpack("<IIII", fd.read(16))

            etr_file_path = os.path.join(self.outdir, "q6_etr.bin")
            if os.path.exists(etr_file_path):
                try:
                    os.remove(etr_file_path)
                except:
                    print_out_str("!!! Cannot delete old etr dump")
            try:
                #Write etr dump to a file
                with open(etr_file_path, 'ab') as etr_file:
                    etr_file.truncate(0)
                    if read_ptr == write_ptr and status & 0x10000 == 1:
                        print_out_str("-- etr buffer is empty")
                    elif read_ptr < write_ptr:
                        fd.seek(read_ptr - start)
                        etr_file.write(fd.read(write_ptr - read_ptr))
                    elif read_ptr > write_ptr or (read_ptr == write_ptr and status & 0x1 == 1):
                        fd.seek(read_ptr - start)
                        etr_file.write(fd.read(end - read_ptr))
                        fd.seek(offset)
                        etr_file.write(fd.read(write_ptr - etr_addr))
                print_out_str("etr binary generated at " + etr_file_path)
                return True
            except:
                print_out_str("!!! Cannot write etr dump to output file")
                return None
        except:
            print_out_str("!!! File operation failed....")
        return None

    def get_dtb(self):
        command_addr = self.addr_lookup('initial_boot_params')
        if command_addr is not None:
            command_addr = self.read_word(command_addr)
            signature = self.read_int(command_addr)
            # signature is 0xd00dfeed in big endian format
            if signature != 0xedfe0dd0:
                print_out_str('dtb signature is invalid')
                return False

            length = self.read_byte(command_addr + 4)
            length = length * 256 + self.read_byte(command_addr + 5)
            length = length * 256 + self.read_byte(command_addr + 6)
            length = length * 256 + self.read_byte(command_addr + 7)
            # length does not include the dtb header 'd00dfeed'
            blob = self.read_physical(self.virt_to_phys(command_addr), length + 4, False)

            return blob
        else:
            print_out_str('!!! Cannot read dtb start address')

        return None

    def __get_section_file(self, sec_type):
        switcher = {
                0: "paging.bin",
                1: "fwdump.bin",
                2: "remote.bin",
                }
        return switcher.get(sec_type, None)

    def __dump_rddm_segments(self, dump_data_vaddr, dump_path, paging_header=False):
        PAGING_SEC = 0x0
        SRAM_SEC = 0x1
        REMOTE_SEC = 0x2

        dump_seg = self.read_word(dump_data_vaddr)
        if dump_seg is None:
            return

        if self.Is_Ath11k():
            seg_address = self.read_structure_field(dump_seg, "struct ath11k_dump_segment", "addr")
        else:
            seg_address = self.read_structure_field(dump_seg, "struct cnss_dump_seg", "address")
        if seg_address is 0 or seg_address is None:
            return

        if not os.path.exists(dump_path):
            print_out_str('!!! RDDM binaries extracted to {0}'.format(dump_path))
            os.makedirs(dump_path)

        if paging_header:
            seg_file = self.__get_section_file(PAGING_SEC)
            seg_file = os.path.join(dump_path, seg_file)
            with open(seg_file, 'wb') as fp:
                fp.write("\0" * 512)
                offset = 0
                fp.seek(offset)
                fp.write(struct.pack('<Q', 1))
                offset = offset + 8

        index = 0
        paging_seg_count = 0
        while seg_address is not 0 and seg_address is not None:
            if self.Is_Ath11k():
                seg_size = self.read_structure_field(dump_seg, "struct ath11k_dump_segment", "len")
                seg_type = self.read_structure_field(dump_seg, "struct ath11k_dump_segment", "type")
                next_dump_seg = dump_seg + self.sizeof("struct ath11k_dump_segment")
                next_seg_address = self.read_structure_field(next_dump_seg, "struct ath11k_dump_segment", "addr")
            else:
                seg_size = self.read_structure_field(dump_seg, "struct cnss_dump_seg", "size")
                seg_type = self.read_structure_field(dump_seg, "struct cnss_dump_seg", "type")
                next_dump_seg = dump_seg + self.sizeof("struct cnss_dump_seg")
                next_seg_address = self.read_structure_field(next_dump_seg, "struct cnss_dump_seg", "address")

            seg_file = self.__get_section_file(seg_type)
            seg_file = os.path.join(dump_path, seg_file)

            if paging_header:
                if seg_type == PAGING_SEC:
                    paging_seg_count = paging_seg_count + 1
                    with open(seg_file, 'r+b') as fp:
                        offset = offset + 8
                        fp.seek(offset)
                        fp.write(struct.pack('<Q', seg_address))
                        offset = offset + 8
                        fp.seek(offset)
                        fp.write(struct.pack('<Q', seg_size))
            else:
                seg = self.read_physical(seg_address, seg_size, False)
                with open(seg_file, 'ab') as fp:
                    fp.write(seg)

            dump_seg = next_dump_seg
            seg_address = next_seg_address
            index = index + 1

        if paging_header:
            seg_file = self.__get_section_file(PAGING_SEC)
            seg_file = os.path.join(dump_path, seg_file)
            with open(seg_file, 'r+b') as fp:
                fp.seek(8)
                fp.write(struct.pack('<Q', paging_seg_count))

    def get_mod_func(self, mod_list):
        high_mem_addr = self.addr_lookup('high_memory')
        vmalloc_offset = 0x800000
        vmalloc_start = self.read_u32(high_mem_addr) + vmalloc_offset & (~int(vmalloc_offset - 0x1))

        if(self.isELF64() and (mod_list & 0xfff0000000 != self.mod_start_addr)):
            return
        elif (self.isELF32() and self.Is_Hawkeye() and mod_list & 0xff000000 != self.mod_start_addr and not ((vmalloc_start & 0xff000000 <= mod_list & 0xff000000) and (mod_list & 0xff000000 <= 0xff000000))):
            return
        elif(self.isELF32() and not self.Is_Hawkeye() and mod_list & 0xff000000 != self.mod_start_addr):
            return
        name = self.read_cstring(mod_list + self.mod_name_offset, 30)
        if (name is None):
            return
        if len(name) < 1 and name.isalpha() is False:
            return
        if (name == self.mod_name):
            self.mod_list_addr = mod_list

    def get_module(self, name):
        if (self.mod_start is None):
           print_out_str('module variable not valid')
           return

        self.mod_name = name
        #simple walk through to get address of module
        name_list_walker = llist.ListWalker(self, self.mod_start, self.next_mod_offset)
        name_list_walker.walk(self.mod_start, self.get_mod_func)

    def get_rddm_dump(self, outdir):
        if self.Is_Ath11k():
            self.get_module("ath11k")

            if self.mod_list_addr is None:
                print_out_str('Unable to get ath11k module address')
                return

            if self.ath11k_gnu_linkonce_this_size is None:
                print_out_str('Unable to get gnu linkonce size')
                return

            if self.seg_info_offset is None:
                print_out_str('Unable to get segment info address')
                return

            seg_info = self.mod_list_addr + int (self.seg_info_offset, 16) + int (self.ath11k_gnu_linkonce_this_size, 16)

            self.gdbmi.close()

            self.gdbmi = gdbmi.GdbMI(self.gdb_path, self.ath11k_path)
            self.gdbmi.open()

            qrtr_node_id = self.read_structure_field(seg_info, "struct ath11k_coredump_segment_info", "qrtr_id")
            if qrtr_node_id is not 0:
                print_out_str('!!! Found RDDM dumps with qrtr node id {0}'.format(qrtr_node_id))

                dump_path = os.path.join(outdir, "rddm_dump")

                if os.path.exists(dump_path):
                    shutil.rmtree(dump_path)

                dump_seg_off = self.field_offset("struct ath11k_coredump_segment_info", "seg")
                dump_seg = seg_info + dump_seg_off

                self.__dump_rddm_segments(dump_seg, dump_path, True)
                self.__dump_rddm_segments(dump_seg, dump_path, False)

            self.gdbmi.close()

            self.gdbmi = gdbmi.GdbMI(self.gdb_path, self.vmlinux)
            self.gdbmi.open()
        else:
            plat_env_index = self.addr_lookup('plat_env_index')

            if plat_env_index is not None:
                plat_env_index = self.read_int(plat_env_index)

            dump_data_vaddr_off = self.field_offset("struct cnss_ramdump_info_v2", "dump_data_vaddr")
            dump_data_vaddr_off = dump_data_vaddr_off + self.field_offset("struct cnss_plat_data", "ramdump_info_v2")

            for i in range(plat_env_index):
                plat_env = self.addr_lookup("plat_env[{0}]".format(i))
                if plat_env is not None:
                    plat_env = self.read_word(plat_env)

                qrtr_node_id = self.read_structure_field(plat_env, "struct cnss_plat_data", "qrtr_node_id")

                dump_path = os.path.join(outdir, "rddm_dump_id_{0}".format(qrtr_node_id))

                if os.path.exists(dump_path):
                    shutil.rmtree(dump_path)

                dump_data_vaddr = plat_env + dump_data_vaddr_off

                self.__dump_rddm_segments(dump_data_vaddr, dump_path, True)
                self.__dump_rddm_segments(dump_data_vaddr, dump_path, False)

    def parse_struct_rpm_cmd_log(self, ptr, length, file_path):
        if os.path.exists(file_path):
            os.remove(file_path)

        print_out_str('!!! Generating {0}'.format(file_path))
        for i in range(length):
            timestamp = self.read_structure_field(ptr, "struct rpm_cmd_log", "timestamp")
            cmd = self.read_structure_field(ptr, "struct rpm_cmd_log", "cmd")
            param1 = self.read_structure_field(ptr, "struct rpm_cmd_log", "param1")
            param2 = self.read_structure_field(ptr, "struct rpm_cmd_log", "param2")
            rxtail = self.read_structure_field(ptr, "struct rpm_cmd_log", "rxtail")
            rxhead = self.read_structure_field(ptr, "struct rpm_cmd_log", "rxhead")
            global_timer_lo = self.read_structure_field(ptr, "struct rpm_cmd_log", "global_timer_lo")
            global_timer_hi = self.read_structure_field(ptr, "struct rpm_cmd_log", "global_timer_hi")
            hdr = self.read_structure_field(ptr, "struct rpm_cmd_log", "hdr")
            hdr = self.read_physical(self.virt_to_phys(ptr + self.field_offset("struct rpm_cmd_log", "hdr")), 60, False)
            Ahdr = ["{:02x}".format(ord(c)) for c in hdr]

            ptr = ptr + self.sizeof("struct rpm_cmd_log")

            with open(file_path, 'a') as fp:
                fp.write("timestamp = {0}; cmd = {1}; param1 = {2}; param2 = {3}; rxtail = {4}; rxhead = {5}; global_timer_lo = {6}; global_timer_hi = {7}; hdr[60] = {8};\n".format(timestamp, cmd, param1, param2, rxtail, rxhead, global_timer_lo, global_timer_hi, Ahdr))

    def parse_struct_glinkwork(self, ptr, length, file_path):
        if os.path.exists(file_path):
            os.remove(file_path)

        print_out_str('!!! Generating {0}'.format(file_path))
        for i in range(length):
            timestamp = self.read_structure_field(ptr, "struct glinkwork", "timestamp")
            cmd = self.read_structure_field(ptr, "struct glinkwork", "cmd")
            param1 = self.read_structure_field(ptr, "struct glinkwork", "param1")
            param2 = self.read_structure_field(ptr, "struct glinkwork", "param2")

            ptr = ptr + self.sizeof("struct glinkwork")

            with open(file_path, 'a') as fp:
                fp.write("timestamp = {0}; cmd = {1}; param1 = {2}; param2 = {3};\n".format(timestamp, cmd, param1, param2))

    def parse_struct_glinkwork_sche_cancel(self, ptr, length, file_path):
        if os.path.exists(file_path):
            os.remove(file_path)

        print_out_str('!!! Generating {0}'.format(file_path))
        for i in range(length):
            timestamp = self.read_structure_field(ptr, "struct work_queue_timelog", "timestamp")

            ptr = ptr + self.sizeof("struct work_queue_timelog")

            with open(file_path, 'a') as fp:
                fp.write("timestamp = {0};\n".format(timestamp))

    def parse_struct_smp2pintr(self, ptr, length, file_path):
        if os.path.exists(file_path):
            os.remove(file_path)

        print_out_str('!!! Generating {0}'.format(file_path))
        for i in range(length):
            timestamp = self.read_structure_field(ptr, "struct smp2p_log", "timestamp")
            value = self.read_structure_field(ptr, "struct smp2p_log", "value")
            last_value = self.read_structure_field(ptr, "struct smp2p_log", "last_value")
            status = self.read_structure_field(ptr, "struct smp2p_log", "status")

            ptr = ptr + self.sizeof("struct smp2p_log")

            with open(file_path, 'a') as fp:
                fp.write("timestamp = {0}; value = {1}; last_value = {2}; status = {3};\n".format(timestamp, value,  last_value, status))

    def get_glink_logging(self,  outdir):
        RPMLOG_SIZE = 256

        glinkintr = self.addr_lookup('glinkintr')
        glinkintrindex = self.addr_lookup('glinkintrindex')

        glinksend = self.addr_lookup('glinksend')
        glinksendindex = self.addr_lookup('glinksendindex')

        glinkwork = self.addr_lookup('glink_work')
        glinkworkindex = self.addr_lookup('glinkworkindex')

	glinkworksche = self.addr_lookup('glinkwork_schedule')
	glinkworkscheindex = self.addr_lookup('glinkwork_sche_index')

	glinkworkcancel = self.addr_lookup('glinkwork_cancel')
	glinkworkcancelindex = self.addr_lookup('glinkwork_cancel_index')

        if glinkintrindex is None or glinksendindex is None or glinkworkindex is None \
        or glinkworkscheindex is None or glinkworkcancelindex is None \
        or glinkintr is None or glinksend is None or glinkwork is None \
        or glinkworksche is None or glinkworkcancel is None:
            print_out_str('!!! Required symbol(s) not found! Skipping..')
            return

        glinkintrindex = self.read_int(glinkintrindex)
        glinksendindex = self.read_int(glinksendindex)
        glinkworkindex = self.read_int(glinkworkindex)
        glinkworkscheindex = self.read_int(glinkworkscheindex)
        glinkworkcancelindex = self.read_int(glinkworkcancelindex)

        file_path = os.path.join(outdir, "glinkintr.txt")
        self.parse_struct_rpm_cmd_log(glinkintr, glinkintrindex, file_path)

        file_path = os.path.join(outdir, "glinksend.txt")
        self.parse_struct_rpm_cmd_log(glinksend, glinksendindex, file_path)

        file_path = os.path.join(outdir, "glinkwork.txt")
        self.parse_struct_glinkwork(glinkwork, glinkworkindex, file_path)

        file_path = os.path.join(outdir, "glinkwork-schedule.txt")
        self.parse_struct_glinkwork_sche_cancel(glinkworksche, glinkworkscheindex, file_path)

        file_path = os.path.join(outdir, "glinkwork-cancel.txt")
        self.parse_struct_glinkwork_sche_cancel(glinkworkcancel, glinkworkcancelindex, file_path)

    def get_smp2p_logging(self,  outdir):
        SMP2PLOG_SIZE = 256

        smp2pintr = self.addr_lookup('smp2pintr')
        smp2pintrindex = self.addr_lookup('smp2pintrindex')

        if smp2pintrindex is None or smp2pintr is None:
            print_out_str('!!! Required symbol(s) not found! Skipping..')
            return

        smp2pintrindex = self.read_int(smp2pintrindex)

        file_path = os.path.join(outdir, "smp2pintr.txt")
        self.parse_struct_smp2pintr(smp2pintr, smp2pintrindex, file_path)

    def extract_modules_from_console(self, file_path):
        print_out_str("Extracting functions and modules from Hex symbols in console log!!")
        consoleLogString = ""

        if self.isELF64():
            ptr_re = "[0-9a-f]{16}"
        else:
            ptr_re = "[0-9a-f]{8}"

        PCLRPattern = "pc : \[<" + ptr_re + ">\].* lr : \[<" + ptr_re + ">\].*"
        FunctionPattern = "Function entered at \[<" + ptr_re + ">\].* from \[<" + ptr_re + ">\].*"

        with open(file_path, 'r') as Lines:
            for partial in Lines:
                if (self.kallsyms_offset < 0 and not (self.arm64 and (self.kernel_version[0], self.kernel_version[1]) >= (5, 4)) and
                    (re.search(PCLRPattern, partial) or re.search(FunctionPattern, partial))):
                    x = re.findall(ptr_re, partial)
                    x0, x1 = x[0], x[1]

                    try:
                        s = self.gdbmi.get_symbol_info(int(x0, 16))
                    except:
                        s = None
                    if s is not None:
                        if len(s.mod):
                            s.mod = " " + s.mod
                        x0 = s.symbol + "+" + hex(s.offset) + s.mod
                    else:
                        x0 = "0x" + x0

                    try:
                        s1 = self.gdbmi.get_symbol_info(int(x1, 16))
                    except:
                        s1 = None
                    if s1 is not None:
                        if len(s1.mod):
                            s1.mod = " " + s1.mod
                        x1 = s1.symbol + "+" + hex(s1.offset) + s1.mod
                    else:
                        x1 = "0x" + x1

                    if re.search(PCLRPattern, partial):
                        timestamp = partial.split("pc :")[0]
                        f = '{0}{1}\n'.format(timestamp, "PC is at " + str(x0))
                        consoleLogString += f
                        f = '{0}{1}\n'.format(timestamp, "LR is at " + str(x1))
                        consoleLogString += f
                    else:
                        timestamp = partial.split("Function entered at")[0]
                        if s is not None and s1 is not None:
                            partial = timestamp + "[<" + x[0] + ">] (" + x0 + ") from [<" + x[1] + ">] (" + x1 + ")\n"

                f = '{0}'.format(partial)
                consoleLogString += f
            with open(file_path, 'w') as console_log:
                console_log.write(consoleLogString)

    def auto_parse(self):
        first_mem_path = None

        for f in first_mem_file_names:
            test_path = file_path + '/' + f
            if os.path.exists(test_path):
                first_mem_path = test_path
                break

        if first_mem_path is None:
            print_out_str('!!! Could not open a memory file. I give up')
            sys.exit(1)

        first_mem = open(first_mem_path, 'rb')
        # put some dummy data in for now
        self.ebi_files = [(first_mem, 0, 0xffff0000, first_mem_path)]
        if not self.get_hw_id(add_offset=False):
            return False
        first_mem_end = self.ebi_start + os.path.getsize(first_mem_path) - 1
        self.ebi_files = [
            (first_mem, self.ebi_start, first_mem_end, first_mem_path)]
        print_out_str(
            'Adding {0} {1:x}--{2:x}'.format(first_mem_path, self.ebi_start, first_mem_end))
        self.ebi_start = self.ebi_start + os.path.getsize(first_mem_path)

        for f in extra_mem_file_names:
            extra_path = file_path + '/' + f

            if os.path.exists(extra_path):
                extra = open(extra_path, 'rb')
                extra_start = self.ebi_start
                extra_end = extra_start + os.path.getsize(extra_path) - 1
                self.ebi_start = extra_end + 1
                print_out_str(
                    'Adding {0} {1:x}--{2:x}'.format(extra_path, extra_start, extra_end))
                self.ebi_files.append(
                    (extra, extra_start, extra_end, extra_path))

        if self.imem_fname is not None:
            imemc_path = file_path + '/' + self.imem_fname
            if os.path.exists(imemc_path):
                imemc = open(imemc_path, 'rb')
                imemc_start = self.tz_start
                imemc_end = imemc_start + os.path.getsize(imemc_path) - 1
                print_out_str(
                    'Adding {0} {1:x}--{2:x}'.format(imemc_path, imemc_start, imemc_end))
                self.ebi_files.append(
                    (imemc, imemc_start, imemc_end, imemc_path))
        return True

    def create_t32_launcher(self):
        out_path = self.outdir

        t32_host_system = self.t32_host_system or platform.system()

        launch_config = open(out_path + '/t32_config.t32', 'wb')
        launch_config.write('OS=\n')
        launch_config.write('ID=T32_1000002\n')

        launch_config.write('TMP=C:\\TEMP\n')
        launch_config.write('SYS=C:\\T32\n')
        launch_config.write('HELP=C:\\T32\\pdf\n')
        launch_config.write('\n')
        launch_config.write('PBI=SIM\n')
        launch_config.write('\n')
        launch_config.write('SCREEN=\n')
        launch_config.write('FONT=SMALL\n')
        launch_config.write('HEADER=Trace32-ScorpionSimulator\n')
        launch_config.write('\n')
        launch_config.write('PRINTER=WINDOWS\n')
        launch_config.write('\n')
        launch_config.write('RCL=NETASSIST\n')
        launch_config.write('PACKLEN=1024\n')
        launch_config.write('PORT=%d\n' % random.randint(20000, 30000))
        launch_config.write('\n')

        launch_config.close()

        startup_script = open(out_path + '/t32_startup_script.cmm', 'wb')

        startup_script.write(('title \"' + out_path + '\"\n').encode('ascii', 'ignore'))

        is_cortex_a53 = self.hw_id == 8916 or self.hw_id == 8939 or self.hw_id == 8936

        if self.arm64 and is_cortex_a53:
            startup_script.write('sys.cpu CORTEXA53\n'.encode('ascii', 'ignore'))
        else:
            startup_script.write('sys.cpu {0}\n'.format(self.cpu_type).encode('ascii', 'ignore'))
        startup_script.write('sys.up\n'.encode('ascii', 'ignore'))

        local = 'LOCAL '
        for ram in self.ebi_files:
            ebi_name = os.path.basename(ram[3])
            local = local + '&' + ebi_name.split('.')[0] + 'File '
        local = local + '&ELFFile'
        startup_script.write('{0}\n'.format(local))
        entry = 'ENTRY '
        for ram in self.ebi_files:
            ebi_name = os.path.basename(ram[3])
            entry = entry + '&' + ebi_name.split('.')[0] + 'File '
        entry = entry + '&ELFFile'
        startup_script.write('{0}\n'.format(entry))
        check = 'IF '
        for ram in self.ebi_files:
            ebi_name = os.path.basename(ram[3])
            check = check + 'STRing.ComPare("&' + ebi_name.split('.')[0] + 'File", "")||'
        check = check + 'STRing.ComPare("&ELFFile", "")'
        startup_script.write('{0}\n'.format(check))
        startup_script.write('(\n'.encode('ascii', 'ignore'))
        startup_script.write('    print "Choose the ELF & Dump files in dialog box."\n'.encode('ascii', 'ignore'))
        startup_script.write('    DIALOG.view file_select.dlg\n'.encode('ascii', 'ignore'))

        # Default values
        for ram in self.ebi_files:
            ebi_path = os.path.basename(ram[3])
            ebi_file = ebi_path.split('.')[0] + 'File'
            startup_script.write('    DIALOG.SET {0} "{1}"\n'.format(ebi_file, ebi_path))
        vmlinux_name = os.path.basename(self.vmlinux)
        startup_script.write('    DIALOG.SET {0} "{1}"\n'.format('ELFFile', vmlinux_name))
        startup_script.write('    STOP\n')

        for ram in self.ebi_files:
            ebi_path = os.path.basename(ram[3])
            ebi_file = ebi_path.split('.')[0] + 'File'
            startup_script.write('    &{0}=DIALOG.STRing({1})\n'.format(ebi_file, ebi_file))
        startup_script.write('    &{0}=DIALOG.STRing({1})\n'.format('ELFFile', 'ELFFile'))

        dialog_config = open(out_path + '/file_select.dlg', 'wb')
        files = []
        for ram in self.ebi_files:
            ebi_path = os.path.basename(ram[3])
            ebi = ebi_path.split('.')[0]
            files.append(ebi)
        files.append('ELF')
        self.create_dialog_file(dialog_config, files)

        startup_script.write('    DIALOG.END\n'.encode('ascii', 'ignore'))
        startup_script.write(')\n'.encode('ascii', 'ignore'))
        dialog_config.close()

        for ram in self.ebi_files:
            ebi_path = os.path.basename(ram[3])
            ebi_file = ebi_path.split('.')[0] + 'File'
            startup_script.write('data.load.binary &{0} 0x{1:x}\n'.format(
                ebi_file, ram[1]).encode('ascii', 'ignore'))
        startup_script.write(
            ('data.load.elf &ELFFile /nocode\n').encode('ascii', 'ignore'))
        if self.arm64 and self.Is_Hawkeye() == True:
            #startup_script.write('Register.Set NS 1\n'.encode('ascii', 'ignore'))
            startup_script.write('r.s M 0x05\n'.encode('ascii', 'ignore'))
            startup_script.write('Data.Set SPR:0x30201 %Quad 0x{0:x}\n'.format(self.swapper_pg_dir_addr + self.phys_offset).encode('ascii', 'ignore'))

            startup_script.write('Data.Set SPR:0x30202 %Quad 0x00000012B5193519\n'.encode('ascii', 'ignore'))
            startup_script.write('Data.Set SPR:0x30A20 %Quad 0x000000FF440C0400\n'.encode('ascii', 'ignore'))
            startup_script.write('Data.Set SPR:0x30A30 %Quad 0x0000000000000000\n'.encode('ascii', 'ignore'))
            startup_script.write('Data.Set SPR:0x30100 %Quad 0x0000000034D5D91D\n'.encode('ascii', 'ignore'))

            #startup_script.write('Register.Set CPSR 0x3C5\n'.encode('ascii', 'ignore'))
            #startup_script.write('MMU.Delete\n'.encode('ascii', 'ignore'))
            startup_script.write('MMU.SCAN PT 0xFFFFFF8000000000--0xFFFFFFFFFFFFFFFF\n'.encode('ascii', 'ignore'))
            startup_script.write('mmu.on\n'.encode('ascii', 'ignore'))
            #startup_script.write('mmu.pt.list 0xffffff8000000000\n'.encode('ascii', 'ignore'))
        elif self.Is_Hawkeye() and self.isELF32():
                startup_script.write('r.s M 0x13\n'.encode('ascii', 'ignore'))
                startup_script.write('PER.Set.simple SPR:0x30200 %Quad 0x{0:x}\n'.format(self.swapper_pg_dir_addr + self.phys_offset).encode('ascii', 'ignore'))
                startup_script.write('PER.Set.simple C15:0x1 %Long 0x1025\n'.encode('ascii', 'ignore'))
                startup_script.write('Data.Set SPR:0x36110 %Quad 0x535\n'.encode('ascii', 'ignore'))
                startup_script.write('mmu.on\n'.encode('ascii', 'ignore'))
                startup_script.write('mmu.scan\n'.encode('ascii', 'ignore'))
        else:
            startup_script.write(
                'PER.S.F C15:0x2 %L 0x{0:x}\n'.format(self.mmu.ttbr).encode('ascii', 'ignore'))
            if isinstance(self.mmu, Armv7LPAEMMU):
                # TTBR1. This gets setup once and never change again even if TTBR0
                # changes
                startup_script.write('PER.S.F C15:0x102 %L 0x{0:x}\n'.format(
                    self.mmu.ttbr + 0x4000).encode('ascii', 'ignore'))
                # TTBCR with EAE and T1SZ set approprately
                startup_script.write(
                    'PER.S.F C15:0x202 %L 0x80030000\n'.encode('ascii', 'ignore'))
            startup_script.write('mmu.on\n'.encode('ascii', 'ignore'))
            startup_script.write('mmu.scan\n'.encode('ascii', 'ignore'))

        if self.arm64:
            startup_script.write(
                 'task.config C:\\T32\\demo\\arm64\\kernel\\linux\\linux-3.x\\linux3.t32\n'.encode('ascii', 'ignore'))
            startup_script.write(
                 'menu.reprogram C:\\T32\\demo\\arm64\\kernel\\linux\\linux-3.x\\linux.men\n'.encode('ascii', 'ignore'))
        else:
            startup_script.write(
                'task.config c:\\t32\\demo\\arm\\kernel\\linux\\linux-3.x\\linux3.t32\n'.encode('ascii', 'ignore'))
            startup_script.write(
                'menu.reprogram c:\\t32\\demo\\arm\\kernel\\linux\\linux-3.x\\linux.men\n'.encode('ascii', 'ignore'))

        startup_script.write('task.dtask\n'.encode('ascii', 'ignore'))
        startup_script.write(
            'v.v  %ASCII %STRING linux_banner\n'.encode('ascii', 'ignore'))
        if os.path.exists(out_path + '/regs_panic.cmm'):
            startup_script.write(
                'do {0}\n'.format('regs_panic.cmm').encode('ascii', 'ignore'))
        elif os.path.exists(out_path + '/core0_regs.cmm'):
            startup_script.write(
                'do {0}\n'.format('core0_regs.cmm').encode('ascii', 'ignore'))
        startup_script.close()

        t32_bat = open(out_path + '/launch_t32.bat', 'wb')
        if self.arm64:
            t32_binary = 'C:\\T32\\bin\\windows64\\t32MARM64.exe'
        elif self.Is_Hawkeye():
            t32_binary = 'C:\\T32\\bin\\windows64\\t32MARM64.exe'
        elif is_cortex_a53:
            t32_binary = 'C:\\T32\\bin\\windows64\\t32MARM.exe'
        else:
            t32_binary = 'C:\\T32\\bin\\windows64\\t32MARM.exe'

        line = 'start ' + t32_binary + ' -c t32_config.t32 -s t32_startup_script.cmm'
        for i in range(len(self.ebi_files)+1):
            line = line + ' %' + str(i+1)
        t32_bat.write(line.encode('ascii', 'ignore'))

        t32_bat.close()
        print_out_str(
            '--- Created a T32 Simulator launcher (run {0}/launch_t32.bat)'.format(out_path))

    def create_dialog_file(self, dialog_config, files):
        dialog_config.write('NAME "File selection"\n'.encode('ascii', 'ignore'))
        dialog_config.write('HEADER "Dump & ELF Files selection"\n'.encode('ascii', 'ignore'))
        text_obj_pos_start = [2, 0]
        text_obj_dimensions = [35, 1]
        browse_obj_pos_start = [38, 1]
        browse_obj_dimensions = [10, 1]

        text_obj = text_obj_pos_start
        text_obj.extend(text_obj_dimensions)
        browse_obj = browse_obj_pos_start
        browse_obj.extend(browse_obj_dimensions)
        for req_file in files:
            obj_pos = text_obj
            dialog_config.write('\tPOS {0}. {1}. {2}. {3}.\n'.
                format(obj_pos[0], obj_pos[1], obj_pos[2], obj_pos[3]))
            dialog_config.write('\tTEXT "Select {0} FILE"\n'.format(req_file))
            dialog_config.write('{0}File:  EDIT "" ""\n'.format(req_file))
            obj_pos = browse_obj
            dialog_config.write('\tPOS {0}. {1}. {2}. {3}.\n'.
                format(obj_pos[0], obj_pos[1], obj_pos[2], obj_pos[3]))
            dialog_config.write('\tBUTTON "Browse"\n')
            dialog_config.write('\t(\n')
            dialog_config.write('\t\tDIALOG.SetFile {0}File .\*\n'.format(req_file))
            dialog_config.write('\t)\n\n')
            text_obj[1] = text_obj[1] + 2
            browse_obj[1] = browse_obj[1] + 2

        # Place OK button at the position of next BROWSE object
        obj_pos = browse_obj
        dialog_config.write('\tPOS {0}. {1}. {2}. {3}.\n'.
            format(obj_pos[0], obj_pos[1], obj_pos[2], obj_pos[3]))
        dialog_config.write('\tDEFBUTTON "OK" "CONTinue"')
        dialog_config.write('\n')

    def read_tz_offset(self):
        if self.tz_addr == 0:
            print_out_str(
                'No TZ address was given, cannot read the magic value!')
            return None
        else:
            return self.read_word(self.tz_addr, False)

    def get_hw_id(self, add_offset=True):
        socinfo_format = -1
        socinfo_id = -1
        socinfo_version = 0
        socinfo_build_id = 'DUMMY'
        chosen_board = None

        boards = get_supported_boards()

        if (self.hw_id is None):
            heap_toc_offset = self.field_offset('struct smem_shared', 'heap_toc')
            if heap_toc_offset is None:
                print_out_str(
                    '!!!! Could not get a necessary offset for auto detection!')
                print_out_str(
                    '!!!! Please check the gdb path which is used for offsets!')
                print_out_str('!!!! Also check that the vmlinux is not stripped')
                print_out_str('!!!! Exiting...')
                sys.exit(1)

            smem_heap_entry_size = self.sizeof('struct smem_heap_entry')
            offset_offset = self.field_offset('struct smem_heap_entry', 'offset')
            for board in boards:
                trace = board.trace_soc
                if trace:
                    print_out_str('board_num = {0}'.format(board.board_num))
                    print_out_str('smem_addr = {0:x}'.format(board.smem_addr))

                socinfo_start_addr = board.smem_addr + heap_toc_offset + smem_heap_entry_size * SMEM_HW_SW_BUILD_ID + offset_offset
                if add_offset:
                    socinfo_start_addr += board.ram_start
                soc_start = self.read_int(socinfo_start_addr, False)
                if trace is True:
                    print_out_str('Read from {0:x}'.format(socinfo_start_addr))
                    if soc_start is None:
                        print_out_str('Result is None! Not this!')
                    else:
                        print_out_str('soc_start {0:x}'.format(soc_start))
                if soc_start is None:
                    continue

                socinfo_start = board.smem_addr + soc_start
                if add_offset:
                    socinfo_start += board.ram_start
                if trace:
                    print_out_str('socinfo_start {0:x}'.format(socinfo_start))

                socinfo_id = self.read_int(socinfo_start + 4, False)
                if trace:
                   print_out_str('socinfo_id = {0} check against {1}'.format(socinfo_id, board.socid))
                if socinfo_id != board.socid:
                    continue

                socinfo_format = self.read_int(socinfo_start, False)
                socinfo_version = self.read_int(socinfo_start + 8, False)
                socinfo_build_id = self.read_cstring(
                    socinfo_start + 12, BUILD_ID_LENGTH, virtual=False)

                chosen_board = board
                break

            if chosen_board is None:
                print_out_str('!!!! Could not find hardware')
                print_out_str("!!!! The SMEM didn't match anything")
                print_out_str(
                    '!!!! You can use --force-hardware to use a specific set of values')
                sys.exit(1)

        else:
            for board in boards:
                if self.hw_id == board.board_num:
                    print_out_str(
                        '!!! Hardware id found! The socinfo values given are bogus')
                    print_out_str('!!! Proceed with caution!')
                    chosen_board = board
                    break
            if chosen_board is None:
                print_out_str(
                    '!!! A bogus hardware id was specified: {0}'.format(self.hw_id))
                print_out_str('!!! Supported ids:')
                for b in get_supported_ids():
                    print_out_str('    {0}'.format(b))
                sys.exit(1)

        print_out_str('\nHardware match: {0}'.format(board.board_num))
        print_out_str('Socinfo id = {0}, version {1:x}.{2:x}'.format(
            socinfo_id, socinfo_version >> 16, socinfo_version & 0xFFFF))
        print_out_str('Socinfo build = {0}'.format(socinfo_build_id))
        print_out_str(
            'Now setting phys_offset to {0:x}'.format(board.phys_offset))
        if board.wdog_addr is not None:
            print_out_str(
            'TZ address: {0:x}'.format(board.wdog_addr))
        self.tz_addr = board.wdog_addr
        self.ebi_start = board.ram_start
        self.tz_start = board.imem_start
        self.hw_id = board.board_num
        self.phys_offset = board.phys_offset
        if self.hw_id == 9574 and not self.arm64:
            self.phys_offset = 0x40000000
        self.cpu_type = board.cpu
        self.imem_fname = board.imem_file_name
        return True

    def virt_to_phys(self, virt):
        if isinstance(virt, basestring):
            virt = self.addr_lookup(virt)
            if virt is None:
                return
        return self.mmu.virt_to_phys(virt)

    def setup_symbol_tables(self):
        stream = os.popen(self.nm_path + ' -nS ' + self.vmlinux)
        symbols = stream.readlines()
        for line in symbols:
            s = line.split(' ')
            if len(s) == 4:
                self.lookup_table.append((int(s[0], 16), s[3].rstrip(), int(s[1], 16)))
        stream.close()

    def addr_lookup(self, symbol):
        try:
            return self.gdbmi.address_of(symbol)
        except gdbmi.GdbMIException:
            pass

    def symbol_lookup_fail_safe(self, addr):
        try:
            return self.gdbmi.get_symbol_info_fail_safe(addr)
        except gdbmi.GdbMIException:
            pass

    def symbol_lookup(self, addr):
        try:
            return self.gdbmi.symbol_at(addr).symbol
        except gdbmi.GdbMIException:
            pass

    def sizeof(self, the_type):
        try:
            return self.gdbmi.sizeof(the_type)
        except gdbmi.GdbMIException:
            pass

    def array_index(self, addr, the_type, index):
        """Index into the array of type `the_type' located at `addr'.

        I.e.:

            Given:

                int my_arr[3];
                my_arr[2] = 42;


            The following:

                my_arr_addr = dump.addr_lookup("my_arr")
                dump.read_word(dump.array_index(my_arr_addr, "int", 2))

        will return 42.

        """
        offset = self.gdbmi.sizeof(the_type) * index
        return addr + offset

    def field_offset(self, the_type, field):
        try:
            return self.gdbmi.field_offset(the_type, field)
        except gdbmi.GdbMIException:
            pass

    def container_of(self, ptr, the_type, member):
        try:
            return self.gdbmi.container_of(ptr, the_type, member)
        except gdbmi.GdbMIException:
            pass

    def sibling_field_addr(self, ptr, parent_type, member, sibling):
        try:
            return self.gdbmi.sibling_field_addr(ptr, parent_type, member, sibling)
        except gdbmi.GdbMIException:
            pass

    def unwind_lookup(self, addr, symbol_size=1, check_modules=1):
        if self.stackinfo_cache.has_key(addr):
            stackinfo = self.stackinfo_cache[addr]
            if symbol_size == 0 and stackinfo is not None:
                return (stackinfo[0], stackinfo[1], stackinfo[2])
            return stackinfo

        stackinfo = self.__unwind_lookup(addr, check_modules)
        self.stackinfo_cache[addr] = stackinfo
        if symbol_size == 0 and stackinfo is not None:
            return (stackinfo[0], stackinfo[1], stackinfo[2])
        return stackinfo

    def __unwind_lookup(self, addr, check_modules=1):
        if (addr is None):
            return ('(Invalid address)', 0x0, None)

        high_mem_addr = self.addr_lookup('high_memory')
        vmalloc_offset = 0x800000
        vmalloc_start = self.read_u32(high_mem_addr) + vmalloc_offset & (~int(vmalloc_offset - 0x1))

        if(self.Is_Hawkeye() and self.isELF64() and check_modules == 1):
            if (self.kernel_version[0], self.kernel_version[1]) >= (5, 4) and (0xffffffc008000000 <= addr < 0xffffffc010000000):
                return self.unwind.get_module_name_from_addr(addr)
            elif (0xffffffbffc000000 <= addr < 0xffffffc000000000):
                return self.unwind.get_module_name_from_addr(addr)
        elif(self.Is_Hawkeye() and self.isELF32() and check_modules == 1 and (0x7f000000 <= addr < 0x7fe00000)):
            return self.unwind.get_module_name_from_addr(addr)
        elif(self.Is_Hawkeye() and self.isELF32() and check_modules == 1 and self.is_config_defined('CONFIG_ARM_MODULE_PLTS') and (vmalloc_start <= addr < 0xff800000)):
            return self.unwind.get_module_name_from_addr(addr)
        if (check_modules == 1 and (0xbf000000 <= addr < 0xbfe00000)):
            return self.unwind.get_module_name_from_addr(addr)

        if (addr < self.page_offset):
            return ('(No symbol for address 0x{0:x})'.format(addr), 0x0, None)

        low = 0
        high = len(self.lookup_table)
        # Python now complains about division producing floats
        mid = (low + high) >> 1
        premid = 0

        while(not(addr >= self.lookup_table[mid][0] and addr < self.lookup_table[mid + 1][0])):

            if(addr < self.lookup_table[mid][0]):
                high = mid - 1

            if(addr > self.lookup_table[mid][0]):
                low = mid + 1

            mid = (high + low) >> 1

            if(mid == premid):
                return None
            if (mid + 1) >= len(self.lookup_table) or mid < 0:
                return None

            premid = mid

        return (self.lookup_table[mid][1], addr - self.lookup_table[mid][0], None, self.lookup_table[mid][2])

    def read_physical(self, addr, length, trace=False):
        ebi = (-1, -1, -1)
        for a in self.ebi_files:
            fd, start, end, path = a
            if addr >= start and addr <= end:
                ebi = a
                break
        if ebi[0] is -1:
            if trace:
                if addr is None:
                    print_out_str('None was passed to read_physical')
                else:
                    print_out_str('addr {0:x} out of bounds'.format(addr))
            return None
        if trace:
            print_out_str('reading from {0}'.format(ebi[0]))
            print_out_str('start = {0:x}'.format(ebi[1]))
            print_out_str('end = {0:x}'.format(ebi[2]))
            print_out_str('length = {0:x}'.format(length))
        offset = addr - ebi[1]
        if trace:
            print_out_str('offset = {0:x}'.format(offset))
        ebi[0].seek(offset)
        a = ebi[0].read(length)
        if trace:
            print_out_str('result = {0}'.format(parser_util.cleanupString(a)))
            print_out_str('lenght = {0}'.format(len(a)))
        return a

    def read_dword(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        s = self.read_string(address, '<Q', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    # returns a word size (pointer) read from ramdump
    def read_word(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        if self.arm64:
            s = self.read_string(address, '<Q', virtual, trace, cpu)
        else:
            s = self.read_string(address, '<I', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    # returns a value corresponding to half the word size
    def read_halfword(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        if self.arm64:
            s = self.read_string(address, '<I', virtual, trace, cpu)
        else:
            s = self.read_string(address, '<H', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    def read_byte(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        s = self.read_string(address, '<B', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    def read_bool(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        s = self.read_string(address, '<?', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    # returns a value guaranteed to be 64 bits
    def read_s64(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        s = self.read_string(address, '<q', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    # returns a value guaranteed to be 64 bits
    def read_u64(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        s = self.read_string(address, '<Q', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    # returns a value guaranteed to be 32 bits
    def read_s32(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        s = self.read_string(address, '<i', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    # returns a value guaranteed to be 32 bits
    def read_u32(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        s = self.read_string(address, '<I', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    def read_int(self, address, virtual=True, trace=False,  cpu=None):
        return self.read_u32(address, virtual, trace, cpu)

    # returns a value guaranteed to be 16 bits
    def read_u16(self, address, virtual=True, trace=False, cpu=None):
        if trace:
            print_out_str('reading {0:x}'.format(address))
        s = self.read_string(address, '<H', virtual, trace, cpu)
        if s is None:
            return None
        else:
            return s[0]

    # reads a 4 or 8 byte field from a structure
    def read_structure_field(self, address, struct_name, field):
        size = self.sizeof("(({0} *)0)->{1}".format(struct_name, field))
        if size == 4:
            return self.read_u32(address + self.field_offset(struct_name, field))
        if size == 8:
            return self.read_u64(address + self.field_offset(struct_name, field))
        return None

    def deference_variable(self, virt_or_name):
        """deference if the input is not a pointer."""
        if not isinstance(virt_or_name, basestring):
            return virt_or_name
        return self.addr_lookup(virt_or_name)

    def read_structure_cstring(self, addr_or_name, struct_name, field,
                               max_length=100):
        """reads a C string from a structure field.  The C string field will be
        dereferenced before reading, so it will be always ``char *``.
        """
        virt = self.deference_variable(addr_or_name)
        cstring_addr = virt + self.field_offset(struct_name, field)
        return self.read_cstring(self.read_word(cstring_addr), max_length)

    def read_cstring(self, address, max_length, virtual=True, cpu=None, trace=False):
        addr = address
        if virtual:
            if cpu is not None:
                address += pcpu_offset + self.per_cpu_offset(cpu)
            addr = self.virt_to_phys(address)
            if trace:
                if address is None:
                    print_out_str('None was passed as address')
                elif addr is None:
                    print_out_str('virt to phys failed on {0:x}'.format(address))
                else:
                    print_out_str('addr {0:x} -> {1:x}'.format(address, addr))
        s = self.read_physical(addr, max_length, trace)
        if s is not None:
            a = s.decode('ascii', 'ignore')
            return a.split('\0')[0]
        else:
            return s

    # returns a tuple of the result from reading from the specified fromat string
    # return None on failure
    def read_string(self, address, format_string, virtual=True, trace=False, cpu=None):
        addr = address
        per_cpu_string = ''
        if virtual:
            if cpu is not None:
                pcpu_offset = self.per_cpu_offset(cpu)
                address += pcpu_offset
                per_cpu_string = ' with per-cpu offset of ' + hex(pcpu_offset)
            addr = self.virt_to_phys(address)
        if trace:
            if addr is not None:
                print_out_str('reading from phys {0:x}{1}'.format(addr,
                                                              per_cpu_string))
        s = self.read_physical(addr, struct.calcsize(format_string), trace)
        if (s is None) or (s == ''):
            if trace and addr is not None:
                print_out_str(
                    'address {0:x} failed hard core (v {1} t{2})'.format(addr, virtual, trace))
            return None
        return struct.unpack(format_string, s)

    def Is_Dakota(self):
        if (self.hw_id == 4018):
            return True
        else:
            return False

    def Is_Hawkeye(self):
        if (self.hw_id == 8074 or self.Is_Alder()):
            return True
        else:
            return False

    def Is_Alder(self):
        if (self.hw_id == 9574):
            return True
        else:
            return False

    def isELF32(self):
        if self.arm64 == False or self.arm64 is None:
            return True
        else:
            return False

    def isELF64(self):
        if self.arm64 == True:
            return True
        else:
            return False

    def get_file_name_from_addr(self, addr):
        start_pos = 0
        exe_start_pos = 0
        gdbstr = self.gdb_path

        stext = self.addr_lookup('stext')
        etext = self.addr_lookup('_etext')

        if (stext is None):
           return None

        if (etext is None):
           return None

        if not (stext <= addr < etext):
           return None

        try:
           exe_start_pos = gdbstr.index("exe")

        except:
           # could be case of linux
           # print "exe extension not found .. trying for linux tool"
           pass

        try:
           start_pos = gdbstr.index("gdb")

        except:
           print "gdb not found in gdbpath"
           return None

        a2l = gdbstr[0:start_pos]
        a2l = '{0}addr2line'.format(a2l)

        if (exe_start_pos != 0):
            a2l = '{0}.exe'.format(a2l)

        if not os.path.exists(a2l):
           print_out_str("warning addr2line tool does not exist..")
        else:
           s = '{0} -e {1} 0x{2:x}'.format(a2l, self.vmlinux, addr)
           f = os.popen(s)
           now = f.read()
           try:
              start_pos = now.index("/qsdk/")
              flen = len(now) - 1
              return now[start_pos:flen]
           except:
              print_out_str('Not able to resolve 0x{0:x} to filename'.format(addr))
              return None

    def hexdump(self, address, length, virtual=True, file_object=None):
        """Returns a string with a hexdump (in the format of `xxd').

        `length' is in bytes.

        Example (intentionally not in doctest format since it would require
        a specific dump to be loaded to pass as a doctest):

        PY>> print(dump.hexdump(dump.addr_lookup('linux_banner') - 0x100, 0x200))
             c0afff6b: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0afff7b: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0afff8b: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0afff9b: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0afffab: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0afffbb: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0afffcb: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0afffdb: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0afffeb: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0affffb: 0000 0000 0069 6e69 7463 616c 6c5f 6465  .....initcall_de
             c0b0000b: 6275 6700 646f 5f6f 6e65 5f69 6e69 7463  bug.do_one_initc
             c0b0001b: 616c 6c5f 6465 6275 6700 2573 2076 6572  all_debug.%s ver
             c0b0002b: 7369 6f6e 2025 7320 286c 6e78 6275 696c  sion %s (lnxbuil
             c0b0003b: 6440 6162 6169 7431 3532 2d73 642d 6c6e  d@abait152-sd-ln
             c0b0004b: 7829 2028 6763 6320 7665 7273 696f 6e20  x) (gcc version
             c0b0005b: 342e 3720 2847 4343 2920 2920 2573 0a00  4.7 (GCC) ) %s..
             c0b0006b: 4c69 6e75 7820 7665 7273 696f 6e20 332e  Linux version 3.
             c0b0007b: 3130 2e30 2d67 6137 3362 3831 622d 3030  10.0-ga73b81b-00
             c0b0008b: 3030 392d 6732 6262 6331 3235 2028 6c6e  009-g2bbc125 (ln
             c0b0009b: 7862 7569 6c64 4061 6261 6974 3135 322d  xbuild@abait152-
             c0b000ab: 7364 2d6c 6e78 2920 2867 6363 2076 6572  sd-lnx) (gcc ver
             c0b000bb: 7369 6f6e 2034 2e37 2028 4743 4329 2029  sion 4.7 (GCC) )
             c0b000cb: 2023 3120 534d 5020 5052 4545 4d50 5420   #1 SMP PREEMPT
             c0b000db: 5765 6420 4170 7220 3136 2031 333a 3037  Wed Apr 16 13:07
             c0b000eb: 3a30 3420 5044 5420 3230 3134 0a00 7c2f  :04 PDT 2014..|/
             c0b000fb: 2d5c 0000 0000 0000 00d4 7525 c0c8 7625  -\........u%..v%
             c0b0010b: c000 0000 0000 0000 0000 0000 0000 0000  ................
             c0b0011b: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0b0012b: 00e0 0b10 c000 0000 0094 7025 c000 0000  ..........p%....
             c0b0013b: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0b0014b: 0000 0000 0000 0000 0000 0000 0000 0000  ................
             c0b0015b: 0000 0000 0000 0000 0000 0000 0000 0000  ................

        """
        import StringIO
        sio = StringIO.StringIO()
        parser_util.xxd(
            address,
            [self.read_byte(address + i, virtual=virtual) or 0
             for i in xrange(length)],
            file_object=sio)
        ret = sio.getvalue()
        sio.close()
        return ret

    def per_cpu_offset(self, cpu):
        per_cpu_offset_addr = self.addr_lookup('__per_cpu_offset')
        if per_cpu_offset_addr is None:
            return 0
        per_cpu_offset_addr_indexed = self.array_index(
            per_cpu_offset_addr, 'unsigned long', cpu)
        return self.read_word(per_cpu_offset_addr_indexed)

    def get_num_cpus(self):
        if (self.kernel_version[0], self.kernel_version[1]) >= (5, 4):
            cpu_present_bits_addr = self.addr_lookup('__cpu_present_mask')
        else:
            cpu_present_bits_addr = self.addr_lookup('cpu_present_bits')
        cpu_present_bits = self.read_word(cpu_present_bits_addr)

        b = self.get_command_line()
        maxcpus = b.find('maxcpus=')

        if maxcpus == -1:
            return bin(cpu_present_bits).count('1')
        else:
            return int((b[maxcpus + 8:].partition(' '))[0])

    def iter_cpus(self):
        return xrange(self.get_num_cpus())

    def thread_saved_field_common_32(self, task, reg_offset):
        thread_info = self.read_word(task + self.field_offset('struct task_struct', 'stack'))
        cpu_context_offset = self.field_offset('struct thread_info', 'cpu_context')
        val = self.read_word(thread_info + cpu_context_offset + reg_offset)
        return val

    def thread_saved_field_common_64(self, task, reg_offset):
        thread_offset = self.field_offset('struct task_struct', 'thread')
        cpu_context_offset = self.field_offset('struct thread_struct', 'cpu_context')
        val = self.read_word(task + thread_offset + cpu_context_offset + reg_offset)
        return val

    def thread_saved_pc(self, task):
        if self.arm64:
            return self.thread_saved_field_common_64(task, self.field_offset('struct cpu_context', 'pc'))
        else:
            return self.thread_saved_field_common_32(task, self.field_offset('struct cpu_context_save', 'pc'))

    def thread_saved_sp(self, task):
        if self.arm64:
            return self.thread_saved_field_common_64(task, self.field_offset('struct cpu_context', 'sp'))
        else:
            return self.thread_saved_field_common_32(task, self.field_offset('struct cpu_context_save', 'sp'))

    def thread_saved_fp(self, task):
        if self.arm64:
            return self.thread_saved_field_common_64(task, self.field_offset('struct cpu_context', 'fp'))
        else:
            return self.thread_saved_field_common_32(task, self.field_offset('struct cpu_context_save', 'fp'))

    def check_addr_validity(self, addr, ebi_files):
        for i in range(len(ebi_files) - 1):
            if ((hex(addr) >= hex(ebi_files[i][1])) and (hex(addr) <= hex(ebi_files[i][2]))):
                return 1
        return 0

    def Is_Ath11k(self):
        if self.ath11k is not None:
            return True
        else:
            return False

    def get_gnu_linkonce_size(self, elf_path, bin_path):
        cmd = '{0} -S {1}'.format(elf_path, bin_path)
        fd = os.popen(cmd)
        rd = fd.read()
        try:
            # get string after this word from readelf output
            part = rd.split(".gnu.linkonce.thi")[1]
            temp = re.sub('\s+', ' ', part ).strip()
            f_hex = temp.split(" ")[3]
            #get size of .gnu.linkonce.thi from section header
            return f_hex.strip()
        except:
            return None
