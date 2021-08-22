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

import struct
import re
from print_out import print_out_str

sysdbg_cpu32_register_names_default_tst = [
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
    ('__reserved9', '__reserved9', False),
    ('__reserved10', '__reserved10', False),
]

sysdbg_cpu32_ctxt_regs_type_default_tst = ''.join([
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
    'Q',  # r13_usr
    'Q',  # r14_usr
    'Q',  # r13_hyp
    'Q',  # r14_irq
    'Q',  # r13_irq
    'Q',  # r14_svc
    'Q',  # r13_svc
    'Q',  # r14_abt
    'Q',  # r13_abt
    'Q',  # r14_und
    'Q',  # r13_und
    'Q',  # r8_fiq
    'Q',  # r9_fiq
    'Q',  # r10_fiq
    'Q',  # r11_fiq
    'Q',  # r12_fiq
    'Q',  # r13_fiq
    'Q',  # r14_fiq
    'Q',  # pc
    'Q',  # cpsr
    'Q',  # r13_mon
    'Q',  # r14_mon
    'Q',  # r14_hyp
    'Q',  # _reserved
    'Q',  # __reserved1
    'Q',  # __reserved2
    'Q',  # __reserved3
    'Q',  # __reserved4
    'Q',  # __reserved5
    'Q',  # __reserved6
    'Q',  # __reserved7
    'Q',  # __reserved8
    'Q',  # __reserved9
    'Q',  # __reserved10
])


sysdbg_cpu32_register_names_v12  = [
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
    ('spsr_irq', 'spsr_irq', False),
    ('r13_irq', 'r13_irq', False),
    ('r14_irq', 'r14_irq', True),
    ('spsr_svc', 'spsr_svc', False),
    ('r13_svc', 'r13_svc', False),
    ('r14_svc', 'r14_svc', True),
    ('spsr_abt', 'spsr_abt', False),
    ('r13_abt', 'r13_abt', False),
    ('r14_abt', 'r14_abt', True),
    ('spsr_und', 'spsr_und', False),
    ('r13_und', 'r13_und', False),
    ('r14_und', 'r14_und', True),
    ('spsr_fiq', 'spsr_fiq', False),
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
    ('__reserved4', '__reserved4', False)
]

sysdbg_cpu32_ctxt_regs_type_v12  = ''.join([
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I',
    'I'
])

# (name from tz dump, corresponding T32 register, whether or not to print_out_str (the function name))
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
    ('x30', 'x30', True),
    ('pc', 'pc', True),
    ('currentEL', None, False),
    ('sp_el3', 'sp_el3', False),
    ('elr_el3', 'elr_el3', True),
    ('spsr_el3', 'spsr_el3', False),
    ('sp_el2', 'sp_el2', False),
    ('elr_el2', 'elr_el2', True),
    ('spsr_el2', 'spsr_el2', False),
    ('sp_el1', 'sp_el1', False),
    ('elr_el1', 'elr_el1', True),
    ('spsr_el1', 'spsr_el1', False),
    ('sp_el0', 'sp_el0', False),
    ('__reserved1', '__reserved1', False),
    ('__reserved2', '__reserved2', False),
    ('__reserved3', '__reserved3', False),
    ('__reserved4', '__reserved4', False),
]

sysdbg_cpu64_ctxt_regs_type_default = ''.join([
    'Q',  # x0
    'Q',  # x1
    'Q',  # x2
    'Q',  # x3
    'Q',  # x4
    'Q',  # x5
    'Q',  # x6
    'Q',  # x7
    'Q',  # x8
    'Q',  # x9
    'Q',  # x10
    'Q',  # x11
    'Q',  # x12
    'Q',  # x13
    'Q',  # x14
    'Q',  # x15
    'Q',  # x16
    'Q',  # x17
    'Q',  # x18
    'Q',  # x19
    'Q',  # x20
    'Q',  # x21
    'Q',  # x22
    'Q',  # x23
    'Q',  # x24
    'Q',  # x25
    'Q',  # x26
    'Q',  # x27
    'Q',  # x28
    'Q',  # x29
    'Q',  # x30
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
]

sysdbg_cpu32_ctxt_regs_type_default = ''.join([
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
    'Q',  # r13_usr
    'Q',  # r14_usr
    'Q',  # r13_hyp
    'Q',  # r14_irq
    'Q',  # r13_irq
    'Q',  # r14_svc
    'Q',  # r13_svc
    'Q',  # r14_abt
    'Q',  # r13_abt
    'Q',  # r14_und
    'Q',  # r13_und
    'Q',  # r8_fiq
    'Q',  # r9_fiq
    'Q',  # r10_fiq
    'Q',  # r11_fiq
    'Q',  # r12_fiq
    'Q',  # r13_fiq
    'Q',  # r14_fiq
    'Q',  # pc
    'Q',  # cpsr
    'Q',  # r13_mon
    'Q',  # r14_mon
    'Q',  # r14_hyp
    'Q',  # _reserved
    'Q',  # __reserved1
    'Q',  # __reserved2
    'Q',  # __reserved3
    'Q',  # __reserved4
])

sysdbg_cpu64_register_names_v1_3 = [
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
    ('x30', 'x30', True),
    ('pc', 'pc', True),
    ('currentEL', None, False),
    ('sp_el3', 'sp_el3', False),
    ('elr_el3', 'elr_el3', True),
    ('spsr_el3', 'spsr_el3', False),
    ('sp_el2', 'sp_el2', False),
    ('elr_el2', 'elr_el2', True),
    ('spsr_el2', 'spsr_el2', False),
    ('sp_el1', 'sp_el1', False),
    ('elr_el1', 'elr_el1', True),
    ('spsr_el1', 'spsr_el1', False),
    ('sp_el0', 'sp_el0', False),
    ('cpumerrsr_el1', None, False),
    ('l2merrsr_el1',  None, False),
    ('__reserved1', '__reserved1', False),
    ('__reserved2', '__reserved2', False),
]

sysdbg_cpu64_ctxt_regs_type_v1_3 = ''.join([
    'Q',  # x0
    'Q',  # x1
    'Q',  # x2
    'Q',  # x3
    'Q',  # x4
    'Q',  # x5
    'Q',  # x6
    'Q',  # x7
    'Q',  # x8
    'Q',  # x9
    'Q',  # x10
    'Q',  # x11
    'Q',  # x12
    'Q',  # x13
    'Q',  # x14
    'Q',  # x15
    'Q',  # x16
    'Q',  # x17
    'Q',  # x18
    'Q',  # x19
    'Q',  # x20
    'Q',  # x21
    'Q',  # x22
    'Q',  # x23
    'Q',  # x24
    'Q',  # x25
    'Q',  # x26
    'Q',  # x27
    'Q',  # x28
    'Q',  # x29
    'Q',  # x30
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
    'Q',  # cpumerrsr_el1
    'Q',  # l2merrsr_el1
    'Q',  # __reserved1
    'Q',  # __reserved2
])

cpu_name = (
    'Invalid',
    'A53',
    'A57',
    'Hydra',
)

sysdbg_cpu64_register_names = {}
sysdbg_cpu64_ctxt_regs_type = {}
sysdbg_cpu32_register_names = {}
sysdbg_cpu32_ctxt_regs_type = {}

sysdbg_cpu64_register_names['default'] = sysdbg_cpu32_register_names_default_tst
sysdbg_cpu64_ctxt_regs_type['default'] = sysdbg_cpu32_ctxt_regs_type_default_tst
sysdbg_cpu32_register_names['default'] = sysdbg_cpu32_register_names_default
sysdbg_cpu32_ctxt_regs_type['default'] = sysdbg_cpu32_ctxt_regs_type_default

sysdbg_cpu64_register_names['12'] = sysdbg_cpu32_register_names_v12
sysdbg_cpu64_ctxt_regs_type['12'] = sysdbg_cpu32_ctxt_regs_type_v12

# Version 1.3
sysdbg_cpu64_register_names['1.3'] = sysdbg_cpu64_register_names_v1_3
sysdbg_cpu64_ctxt_regs_type['1.3'] = sysdbg_cpu64_ctxt_regs_type_v1_3

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

class TZCpuCtx_v2():

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
        for reg_name, t32_name, print_pc in register_names:
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
                        print_out_str('   {0:8} = 0x{1:016x} {2} [{3}.ko]'.format(reg_name, self.regs[reg_name],
                            pc_string, modname))
                    else:
                        print_out_str('   {0:8} = 0x{1:016x} {2}'.format(reg_name, self.regs[reg_name], pc_string))
            else:
                print_out_str('   {0:8} = 0x{1:016x}'.format(reg_name, self.regs[reg_name]))
            if t32_name is not None:
                outfile.write(
                    'r.s {0} 0x{1:x}\n'.format(t32_name, self.regs[reg_name]))

class TZRegDump_v2():
    def __init__(self):
        self.core_regs = []
        self.core_up = []
        self.sec_regs = None
        self.version = 0
        self.start_addr = 0
        self.end_addr = 0
        self.core = 0
        self.sc_status = []
        self.sc_regs = []

    def dump_all_regs(self, ram_dump):
        for i in range(0, self.ncores):
            if (self.core_up[i] != 1):
                continue

            coren_regs = ram_dump.open_file('core{0}_regs.cmm'.format(i))

            print_out_str('core{0} regs:'.format(i))
            self.core_regs[i].print_regs(coren_regs, ram_dump)
            coren_regs.close()

        if (ram_dump.hw_id == 8064):
            self.print_fiq_marker_details()

    def print_fiq_marker_details(self):
        print_out_str('\n======== entry/exit details of SGI and WDT interrupt ==========')
        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 4), False)
        print_out_str('   Generic FIQ Entry Counter {0}'.format(fiq_marker))

        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 8), False)
        if (fiq_marker == 1):
            print_out_str('   WDT interrupt occured on Core 0')
        else:
            print_out_str('   WDT interrupt not occured on Core 0')


        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 12), False)
        if (fiq_marker == 1):
            print_out_str('   WDT interrupt has occured on Core 0 and coredump is done ')
        else:
            print_out_str('   Core0 dump not done')

        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 16), False)
        if (fiq_marker == 1):
            print_out_str('   SGI is sent from Core 0 to Core 1 and is about to enter SGI FIQ handler ')
        else:
            if (self.sc_status[0] & v2tzbsp_sc_status_warm_boot == 0):
                if(self.sc_status[0] & v2tzbsp_sc_status_wdt):
                    print_out_str('   Core0 has sent SGI but Core1 has not received it')
                else:
                    print_out_str('   Core0 has not sent SGI')


        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 20), False)
        if (fiq_marker == 1):
            print_out_str('   SGI is received by Core 0 and coredump activity is done  ')
        else:
            print_out_str('   Core0 dump not done ')

        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 24), False)
        if (fiq_marker == 1):
            print_out_str('   WDT interrupt occured on Core 1 and is about to enter WDT FIQ handler')
        else:
            print_out_str('   WDT interrupt not occured on Core 1')

        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 28), False)
        if (fiq_marker == 1):
            print_out_str('   WDT interrupt has occured on Core 1 and coredump activity is done')
        else:
            print_out_str('   Core1 dump not done')

        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 32), False)
        if (fiq_marker == 1):
            print_out_str('   SGI is sent from Core 1 to Core 0 and is about to enter SGI FIQ')
        else:
            if (self.sc_status[1] & v2tzbsp_sc_status_warm_boot == 0):
                if(self.sc_status[1] & v2tzbsp_sc_status_wdt):
                    print_out_str('   Core1 has sent SGI but Core0 has not received it')
                else:
                    print_out_str('   Core1 has not sent SGI')


        fiq_marker = self.ramdump.read_word((self.ramdump.tz_addr + 36), False)
        if (fiq_marker == 1):
            print_out_str('   SGI is received by Core 1 and coredump activity is done ')
        else:
            print_out_str('   Core1 dump not done ')
        print_out_str('\n======== End of entry/exit details of SGI and WDT interrupt ======\n')

    def dump_core_pc(self, ram_dump):
        for i in range(0, self.ncores):
            if (self.core_up[i] !=1):
                continue

            pc = self.core_regs[i].regs['pc']
            lr = self.core_regs[i].regs['r14_svc']
            bt = self.core_regs[i].regs['r13_svc']
            fp = self.core_regs[i].regs['r11']

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
                print_out_str(
                    'Core {3} PC: {0}+0x{1:x}/0x{5:x} <{2:x}> [{4}.ko]'.format(symname, offset, pc, i, modname, symtab_st_size))
            elif modname is None and symtab_st_size is not None:
                fname = self.ramdump.get_file_name_from_addr(pc)
                if fname is not None:
                    print_out_str('Core {3} PC: {0}+0x{1:x}/0x{5:x} <0x{2:x}> [{4}]'.format(symname, offset, pc, i, fname, symtab_st_size))
                else:
                    print_out_str('{3}: [{0}+0x{1:x}/0x{4:x}] <0x{2:x}>'.format(symname, offset, pc, i, symtab_st_size))
            else:
                print_out_str('Core {3} PC: {0}+0x{1:x} <0x{2:x}>'.format(symname, offset, pc, i))

            a = ram_dump.unwind_lookup(lr)
            if a is not None and len(a) > 3:
                symname, offset, modname, symtab_st_size = a
            else:
                symname = 'UNKNOWN'
                offset = 0

            if modname is not None:
                print_out_str(
                    'Core {3} LR: {0}+0x{1:x}/0x{5:x} <{2:x}> [{4}.ko]'.format(symname, offset, lr, i, modname, symtab_st_size))
            elif modname is None and symtab_st_size is not None:
                fname = self.ramdump.get_file_name_from_addr(lr)
                if fname is not None:
                    print_out_str('Core {3} LR: {0}+0x{1:x}/0x{5:x} <0x{2:x}> [{4}]'.format(symname, offset, lr, i, fname, symtab_st_size))
                else:
                    print_out_str('{3}: [{0}+0x{1:x}/0x{4:x}] <0x{2:x}>'.format(symname, offset, lr, i, symtab_st_size))
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

        ebi_addr = start_addr

        for i in range(0, self.ncores):
            self.sc_status.append(self.ramdump.read_word(ebi_addr, False))
            ebi_addr += 4

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

        self.new_start_addr = 0x87B00000
        start_addr_offset = 40
        struct_size = 0x38

        sysdbgCPUDumpver = self.ramdump.read_word(self.new_start_addr, False)
        if(sysdbgCPUDumpver == 0x12):
            self.version = "12"
        else:
            self.version = 'default'
        for i in range(0, self.ncores):
            if(self.version == "12"):
                reg_ctx = []
                cpu_cntxt_start = self.ramdump.read_dword(self.new_start_addr + start_addr_offset, False)
                cpu_cntxt_start += 16
                reg_ctx = ram_dump.read_string(cpu_cntxt_start, sysdbg_cpu64_ctxt_regs_type[self.version], False)
                if reg_ctx is not None:
                    self.sc_regs.append(reg_ctx)
                    self.core_regs.append(TZCpuCtx_v2(self.version, self.sc_regs[i], ram_dump))
                    self.core_up.append(1)
                else:
                    print_out_str('Core {0} registers not available, may be core is down'.format(i))
                    self.core_up.append(0)

                self.new_start_addr += struct_size
            else:
                self.start_addr += 16

                self.sc_regs.append(ram_dump.read_string(
                    self.start_addr, sysdbg_cpu64_ctxt_regs_type[self.version], False))
                self.start_addr += struct.calcsize(sysdbg_cpu64_ctxt_regs_type[self.version])
                sc_secure = ram_dump.read_string(
                   self.start_addr, sysdbg_cpu64_ctxt_regs_type[self.version] , False)
                self.start_addr += struct.calcsize(sysdbg_cpu64_ctxt_regs_type[self.version])
                self.core_regs.append(TZCpuCtx_v2(self.version, self.sc_regs[i], ram_dump))
        return True
