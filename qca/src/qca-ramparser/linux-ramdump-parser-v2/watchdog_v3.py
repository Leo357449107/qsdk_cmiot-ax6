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
import re
from print_out import print_out_str

sysdbg_cpu64_register_names_default = [
    ('x0', 'x0', False),
    ('x1', 'x1', False),
    ('x2', 'x2', False),
    ('x3', 'x3', False),
    ('x4', 'x4', False),
    ('x5', 'x5', False),
    ('x6', 'x6', False),
    ('x7', 'x7', False),
    ('x8', 'x8', False),
    ('x9', 'x9', False),
    ('x10', 'x10', False),
    ('x11', 'x11', False),
    ('x12', 'x12', False),
    ('x13', 'x13', False),
    ('x14', 'x14', False),
    ('x15', 'x15', False),
    ('x16', 'x16', False),
    ('x17', 'x17', False),
    ('x18', 'x18', False),
    ('x19', 'x19', False),
    ('x20', 'x20', False),
    ('x21', 'x21', False),
    ('x22', 'x22', False),
    ('x23', 'x23', False),
    ('x24', 'x24', False),
    ('x25', 'x25', False),
    ('x26', 'x26', False),
    ('x27', 'x27', False),
    ('x28', 'x28', False),
    ('x29', 'x29', False),
    ('x30', 'x30', False),
    ('pc', 'pc', True),
    ('currentEL', None, False),
    ('sp_el3', 'sp_el3', False),
    ('elr_el3', 'elr_el3', False),
    ('spsr_el3', 'spsr_el3', False),
    ('sp_el2', 'sp_el2', False),
    ('elr_el2', 'elr_el2', False),
    ('spsr_el2', 'spsr_el2', False),
    ('sp_el1', 'sp_el1', False),
    ('elr_el1', 'elr_el1', False),
    ('spsr_el1', 'spsr_el1', False),
    ('sp_el0', 'sp_el0', False),
    ('__reserved1', '__reserved1', False),
    ('__reserved2', '__reserved2', False),
    ('__reserved3', '__reserved3', False),
    ('__reserved4', '__reserved4', False),
    ('__reserved5', '__reserved5', False),
    ('__reserved6', '__reserved6', False),
    ('__reserved7', '__reserved7', False),
    ('__reserved8', '__reserved8', False),
]

sysdbg_cpu64_ctxt_regs_type_default = ''.join([
    'Q',  # r0
    'Q',  # r1
    'Q',  # r2
    'Q',  # r3
    'Q',  # r4
    'Q',  # r5
    'Q',  # r6
    'Q',  # r7
    'Q',  # r8
    'Q',  # r9
    'Q',  # r10
    'Q',  # r11
    'Q',  # r12
    'Q',  # r13
    'Q',  # r14
    'Q',  # r15
    'Q',  # r16
    'Q',  # r17
    'Q',  # r18
    'Q',  # r19
    'Q',  # r20
    'Q',  # r21
    'Q',  # r22
    'Q',  # r23
    'Q',  # r24
    'Q',  # r25
    'Q',  # r26
    'Q',  # r27
    'Q',  # r28
    'Q',  # r29
    'Q',  # r30
    'Q',  # pc
    'Q',  # currentEL
    'Q',  # sp_el3
    'Q',  # elr_el3
    'Q',  # spsr_el3
    'Q',  # sp_el2
    'Q',  # elr_el2
    'Q',  # spsr_el2
    'Q',  # sp_el1
    'Q',  # elr_el1
    'Q',  # spsr_el1
    'Q',  # sp_el0
    'Q',  # __reserved1
    'Q',  # __reserved2
    'Q',  # __reserved3
    'Q',  # __reserved4
    'Q',  # __reserved5
    'Q',  # __reserved6
    'Q',  # __reserved7
    'Q',  # __reserved8
])

sysdbg_cpu32_register_names_default = [
    ('r0', 'r0', False),
    ('r1', 'r1', False),
    ('r2', 'r2', False),
    ('r3', 'r3', False),
    ('r4', 'r4', False),
    ('r5', 'r5', False),
    ('r6', 'r6', False),
    ('r7', 'r7', False),
    ('r8', 'r8', False),
    ('r9', 'r9', False),
    ('r10', 'r10', False),
    ('r11', 'r11', False),
    ('r12', 'r12', False),
    ('r13_usr', 'r13_usr', False),
    ('r14_usr', 'r14_usr', False),
    ('r13_hyp', 'r13_hyp', False),
    ('r14_irq', 'r14_irq', True),
    ('r13_irq', 'r13_irq', False),
    ('r14_svc', 'r14_svc', True),
    ('r13_svc', 'r13_svc', False),
    ('r14_abt', 'r14_abt', True),
    ('r13_abt', 'r13_abt', False),
    ('r14_und', 'r14_und', True),
    ('r13_und', 'r13_und', False),
    ('r8_fiq', 'r8_fiq', False),
    ('r9_fiq', 'r9_fiq', False),
    ('r10_fiq', 'r10_fiq', False),
    ('r11_fiq', 'r11_fiq', False),
    ('r12_fiq', 'r12_fiq', False),
    ('r13_fiq', 'r13_fiq', False),
    ('r14_fiq', 'r14_fiq', True),
    ('pc', 'pc', True),
    ('cpsr', 'cpsr', False),
    ('r13_mon', 'r13_mon', False),
    ('r14_mon', 'r14_mon', True),
    ('r14_hyp', 'elr_hyp', True),
    ('_reserved', '_reserved', False),
    ('__reserved1', '__reserved1', False),
    ('__reserved2', '__reserved2', False),
    ('__reserved3', '__reserved3', False),
    ('__reserved4', '__reserved4', False),
    ('__reserved5', '__reserved5', False),
    ('__reserved6', '__reserved6', False),
    ('__reserved7', '__reserved7', False),
    ('__reserved8', '__reserved8', False),
]

sysdbg_cpu32_ctxt_regs_type_default = ''.join([
    'I',  # r0
    'I',  # r1
    'I',  # r2
    'I',  # r3
    'I',  # r4
    'I',  # r5
    'I',  # r6
    'I',  # r7
    'I',  # r8
    'I',  # r9
    'I',  # r10
    'I',  # r11
    'I',  # r12
    'I',  # r13_usr
    'I',  # r14_usr
    'I',  # r13_hyp
    'I',  # r14_irq
    'I',  # r13_irq
    'I',  # r14_svc
    'I',  # r13_svc
    'I',  # r14_abt
    'I',  # r13_abt
    'I',  # r14_und
    'I',  # r13_und
    'I',  # r8_fiq
    'I',  # r9_fiq
    'I',  # r10_fiq
    'I',  # r11_fiq
    'I',  # r12_fiq
    'I',  # r13_fiq
    'I',  # r14_fiq
    'I',  # pc
    'I',  # cpsr
    'I',  # r13_mon
    'I',  # r14_mon
    'I',  # r14_hyp
    'I',  # _reserved
    'I',  # __reserved1
    'I',  # __reserved2
    'I',  # __reserved3
    'I',  # __reserved4
    'I',  # __reserved5
    'I',  # __reserved6
    'I',  # __reserved7
    'I',  # __reserved8
])


sysdbg_cpu64_register_names = {}
sysdbg_cpu64_ctxt_regs_type = {}
sysdbg_cpu32_register_names = {}
sysdbg_cpu32_ctxt_regs_type = {}

sysdbg_cpu64_register_names['default'] = sysdbg_cpu64_register_names_default
sysdbg_cpu64_ctxt_regs_type['default'] = sysdbg_cpu64_ctxt_regs_type_default
sysdbg_cpu32_register_names['default'] = sysdbg_cpu32_register_names_default
sysdbg_cpu32_ctxt_regs_type['default'] = sysdbg_cpu32_ctxt_regs_type_default


tz_sc_status_ns = 1
tz_sc_status_wdt = 2
tz_sc_status_sgi = 4

v2tzbsp_sc_status_ns_bit = 1
v2tzbsp_sc_status_wdt = 2
v2tzbsp_sc_status_sgi = 4
v2tzbsp_sc_status_warm_boot = 8

tz_sc_ignore_status = 0x10


v3tzbsp_cpu_dump_status = 0x20
v3sdi_cpu_dump_status = 0x10

class TZCpuCtx_v3():

    def __init__(self, version, regs_t, ramdump):
        i = 0
        self.regs = {}
        self.version = version
        register_name = sysdbg_cpu64_register_names[self.version]

        if regs_t is not None:
            for r in regs_t:
                self.regs[register_name[i][0]] = r
                i += 1

    def print_regs(self, outfile, ramdump):
        register_names = sysdbg_cpu64_register_names[self.version]
        #print ("++++++++++++++++++++++++++++++++++{0}\n".format(register_names))
        for reg_name, t32_name, print_pc in register_names:
            #print('{0:8} = {1} == {2}'.format(reg_name, t32_name, print_pc)) # self.regs[reg_name]))
            #continue
            if re.match('(.*)reserved(.*)', reg_name):
                print_out_str('{0:8} = 0x{1:016x}'.format(reg_name, self.regs[reg_name]))
                continue
            if print_pc:
                a = ramdump.unwind_lookup(self.regs[reg_name])
                modname = None
                offset = 0
                symtab_st_size = 0
                if a is not None and len(a) > 3:
                    symname, offset, modname, symtab_st_size = a
                    pc_string = '[{0}+0x{1:x}/0x{2:x}]'.format(symname, offset, symtab_st_size)
                    if (modname is not None):
                        print_out_str('   {0:8} = 0x{1:016x} {2} [{3}.ko]'.format(reg_name, self.regs[reg_name], pc_string, modname))
                    else:
                        print_out_str('   {0:8} = 0x{1:016x} {2}'.format(reg_name, self.regs[reg_name], pc_string))
                else:
                    print_out_str('   {0:8} = 0x{1:016x}'.format(reg_name, self.regs[reg_name]))
            else:
                print_out_str('   {0:8} = 0x{1:016x}'.format(reg_name, self.regs[reg_name]))
            if t32_name is not None:
                outfile.write('r.s {0} 0x{1:x}\n'.format(t32_name, self.regs[reg_name]))


class TZRegDump_v3():
    def __init__(self):
        self.core_regs = []
        self.core_up = []
        self.sec_regs = []
        self.version = 0
        self.start_addr = 0
        self.end_addr = 0
        self.core = 0
        self.sc_status = []
        self.sc_regs = []
        self.temp = []

    def dump_all_regs(self, ram_dump):
        kernel_addr = self.linux_kernel_addr
        for i in range(0, self.ncores):
            if (self.core_up[i] != 1):
                continue

            sysdbgCPUDumpver = self.ramdump.read_u32(kernel_addr, False)
            sysdbgmagic = self.ramdump.read_u32(kernel_addr + 4, False)
            if(sysdbgCPUDumpver != 0x14 or sysdbgmagic != 0x42445953):
                kernel_addr += self.struct_size
                continue
            coren_regs = ram_dump.open_file('core{0}_regs.cmm'.format(i))
            print_out_str('core{0} regs non_secure:'.format(i))
            self.core_regs[i].print_regs(coren_regs, ram_dump)
            coren_regs.close()
            secn_regs = ram_dump.open_file('core{0}_regs_sec.cmm'.format(i))
            print_out_str('core{0} regs secure_world:'.format(i))
            self.sec_regs[i].print_regs(secn_regs, ram_dump)
            secn_regs.close()
            kernel_addr += self.struct_size

    def print_status_register_details(self):
        print_out_str('\n======== entry/exit details of SGI and WDT interrupt ==========\n')
        kernel_addr = self.linux_kernel_addr
        for i in range(0, self.ncores):
            sysdbgCPUDumpver = self.ramdump.read_u32(kernel_addr, False)
            sysdbgmagic = self.ramdump.read_u32(kernel_addr + 4, False)
            if(sysdbgCPUDumpver != 0x14 or sysdbgmagic != 0x42445953):
                kernel_addr += self.struct_size
                continue
            cpu_cntxt_start_addr = self.ramdump.read_dword(kernel_addr + self.start_addr_offset, False)
            status_value = self.ramdump.read_word(cpu_cntxt_start_addr+4, False)
            if(status_value & 0x01 == 0x01):
                print_out_str('Core {0} WDT in Non secure world'.format(i, status_value))
            if(status_value & 0x02 == 0x02):
                print_out_str('Core {0} Reboot due to WDT'.format(i, status_value))
            if(status_value & 0x04 == 0x04):
                print_out_str('Core {0} SGI interrupt'.format(i, status_value))
            if(status_value & 0x08 == 0x08):
                print_out_str('Core {0} WDT during Warm boot'.format(i, status_value))
            if(status_value & 0x20 == 0x20):
                print_out_str('Core {0} CPU context is completed by TZ'.format(i, status_value))
            print_out_str('Core {0} status = 0x{1:x}'.format(i, status_value))
            print_out_str('')
            kernel_addr += self.struct_size
        print_out_str('======== End of entry/exit details of SGI and WDT interrupt ======\n')

    def dump_core_pc(self, ram_dump):
        for i in range(0, self.ncores):
            if (self.core_up[i] !=1 ):
                continue

            if ram_dump.arm64 == False or ram_dump.arm64 is None:
                pc = self.core_regs[i].regs['pc']
                lr = self.core_regs[i].regs['x18']
                bt = self.core_regs[i].regs['x19']
                fp = self.core_regs[i].regs['x11']
            else:
                pc = self.core_regs[i].regs['pc']
                lr = self.core_regs[i].regs['x30']
                bt = self.core_regs[i].regs['x29']
                fp = self.core_regs[i].regs['x29']

            a = ram_dump.unwind_lookup(pc)
            modname = None
            offset = 0
            symtab_st_size = 0
            if a is not None and len(a) > 3:
                symname, offset, modname, symtab_st_size = a
            else:
                symname = 'UNKNOWN'
                offset = 0
            if modname is not None:
                print_out_str('Core {3} PC: {0}+0x{1:x}/0x{5:x} <{2:x}> [{4}.ko]'.format(symname, offset, pc, i, modname, symtab_st_size))
            elif modname is None and symtab_st_size is not None:
                fname = self.ramdump.get_file_name_from_addr(pc)
                if fname is not None:
                    print_out_str('Core {3} PC: {0}+0x{1:x}/0x{5:x} <0x{2:x}> [{4}]'.format(symname, offset, pc, i, fname, symtab_st_size))
                else:
                    print_out_str('Core {3} PC: [{0}+0x{1:x}/0x{4:x}] <0x{2:x}>'.format(symname, offset, pc, i, symtab_st_size))
            else:
                print_out_str('Core {3} PC: {0}+0x{1:x} <0x{2:x}>'.format(symname, offset, pc, i))

            a = ram_dump.unwind_lookup(lr)
            if a is not None and len(a) > 3:
                symname, offset, modname, symtab_st_size = a
            else:
                symname = 'UNKNOWN'
                offset = 0

            if modname is not None:
                print_out_str('Core {3} LR: {0}+0x{1:x}/0x{5:x} <{2:x}> [{4}.ko]'.format(symname, offset, lr, i, modname, symtab_st_size))
            elif modname is None and symtab_st_size is not None:
                fname = self.ramdump.get_file_name_from_addr(lr)
                if fname is not None:
                    print_out_str('Core {3} LR: {0}+0x{1:x}/0x{5:x} <0x{2:x}> [{4}]'.format(symname, offset, lr, i, fname, symtab_st_size))
                else:
                    print_out_str('Core {3} LR: [{0}+0x{1:x}/0x{4:x}] <0x{2:x}>'.format(symname, offset, lr, i, symtab_st_size))
            else:
                print_out_str('Core {3} LR: {0}+0x{1:x} <0x{2:x}>'.format(symname, offset, lr, i))

            print_out_str('')
            ram_dump.unwind.unwind_backtrace(bt, fp, pc, lr, '')
            print_out_str('')


    def init_regs(self, version, start_addr, end_addr, core, ram_dump):
        self.start_addr = start_addr
        self.end_addr = end_addr
        self.core = core

        register_names = sysdbg_cpu64_register_names
        self.version = 'default'
        self.ncores = ram_dump.get_num_cpus()
        self.ramdump = ram_dump

        # uint32 status[4]; -- status fields
        # sdi_cpu_ctxt_regs_type cpu_regs; -- ctxt for all cpus
        # sdi_cpu_ctxt_regs_type __reserved3; -- secure ctxt

        # struct {
        #     uint32 version;
        #     uint32 magic;
        #     char name[DUMP_DATA_TYPE_NAME_SZ]; 32 bytes
        #     uint64 start_addr;
        #     uint64 len;
        #  }

        self.new_start_addr = self.ramdump.tz_addr #0x08600658
        self.start_addr_offset = 40
        self.struct_size = 0x38

        self.linux_kernel_addr = self.ramdump.read_word(self.new_start_addr, False)
        #print ('Linux virtual address = 0x{0:X}\n'.format(self.linux_kernel_addr))
        #sysdbgCPUDumpver = self.ramdump.read_u32(self.linux_kernel_addr, False)
        #print ('sysdbgCPUDumpver = {0}\n'.format(sysdbgCPUDumpver))
        #if(sysdbgCPUDumpver == 0x14):
            #self.version = 'default'
            #print (sysdbg_cpu64_ctxt_regs_type)
            #print (register_names)
        #magic = self.ramdump.read_word(self.linux_kernel_addr+4, False)
        #print ('magic = {0}\n'.format(magic))
        #name = self.ramdump.read_cstring(self.linux_kernel_addr+8, 32, False)
        #print ('name = {0}\n'.format(name))
        #leng = self.ramdump.read_dword(self.linux_kernel_addr+48, False)
        #print ('leng = {0}\n'.format(leng))
        self.print_status_register_details()
        dump_data_type_addr = self.linux_kernel_addr

        for i in range(0, self.ncores):
            reg_ctx = []
            sysdbgCPUDumpver = self.ramdump.read_u32(dump_data_type_addr, False)
            sysdbgmagic = self.ramdump.read_u32(dump_data_type_addr + 4, False)
            if(sysdbgCPUDumpver == 0x14 and sysdbgmagic == 0x42445953):
                self.version = 'default'
            else:
                dump_data_type_addr += self.struct_size
                print_out_str('!!! Core {0} - DumpVer: {1}, dbgMagic: {2} mismatch!!'.format(i, sysdbgCPUDumpver, sysdbgmagic))
            cpu_cntxt_start = self.ramdump.read_dword(dump_data_type_addr + self.start_addr_offset, False)
            #status register offset
            cpu_cntxt_start += 16
            #print (sysdbg_cpu64_register_names[self.version])
            reg_ctx = ram_dump.read_string(cpu_cntxt_start, sysdbg_cpu64_ctxt_regs_type[self.version], False)
            if reg_ctx is not None:
                self.sc_regs.append(reg_ctx)
                self.core_regs.append(TZCpuCtx_v3(self.version, self.sc_regs[i], ram_dump))
                self.core_up.append(1)
            else:
                print_out_str('Core {0} registers not available, may be core is down'.format(i))
                self.core_up.append(0)

            cpu_cntxt_start += struct.calcsize(sysdbg_cpu64_ctxt_regs_type[self.version])
            self.temp = ram_dump.read_string(cpu_cntxt_start, sysdbg_cpu64_ctxt_regs_type[self.version] , False)
            self.sec_regs.append(TZCpuCtx_v3(self.version, self.temp, ram_dump))

            dump_data_type_addr += self.struct_size
            #print ('sc_regs =  {0}'.format(self.sc_regs))
            #print ("\n")
        return True
