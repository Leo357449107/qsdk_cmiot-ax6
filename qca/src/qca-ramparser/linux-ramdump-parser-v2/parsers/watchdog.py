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

import sys
import os
import re
import struct
import string
from print_out import print_out_str
from parser_util import register_parser, RamParser
import linux_list as llist

# (name from tz dump, corresponding T32 register, whether or not to print_out_str (the function name))
tzbsp_register_names = [
    ('mon_lr', 'pc', True),
    ('mon_spsr', None, False),
    ('usr_r0', 'r0', False),
    ('usr_r1', 'r1', False),
    ('usr_r2', 'r2', False),
    ('usr_r3', 'r3', False),
    ('usr_r4', 'r4', False),
    ('usr_r5', 'r5', False),
    ('usr_r6', 'r6', False),
    ('usr_r7', 'r7', False),
    ('usr_r8', 'r8', False),
    ('usr_r9', 'r9', False),
    ('usr_r10', 'r10', False),
    ('usr_r11', 'r11', False),
    ('usr_r12', 'r12', False),
    ('usr_r13', 'r13_usr', False),
    ('usr_r14', 'r14_usr', False),
    ('irq_spsr', 'spsr_irq', False),
    ('irq_r13', 'r13_irq', False),
    ('irq_r14', 'r14_irq', True),
    ('svc_spsr', 'spsr', False),
    ('svc_r13', 'r13_svc', False),
    ('svc_r14', 'r14_svc', True),
    ('abt_spsr', 'spsr_abt', False),
    ('abt_r13', 'r13_abt', False),
    ('abt_r14', 'r14_abt', True),
    ('und_spsr', 'spsr_und', False),
    ('und_r13', 'r13_und', False),
    ('und_r14', 'r14_und', True),
    ('fiq_spsr', 'spsr_fiq', False),
    ('fiq_r8', 'r8_fiq', False),
    ('fiq_r9', 'r9_fiq', False),
    ('fiq_r10', 'r10_fiq', False),
    ('fiq_r11', 'r11_fiq', False),
    ('fiq_r12', 'r12_fiq', False),
    ('fiq_r13', 'r13_fiq', False),
    ('fiq_r14', 'r14_fiq', False),
]


tzbsp_dump_cpu_ctx_t = ''.join([
    'I',  # mon_lr
    'I',  # mon_spsr
    'I',  # usr_r0
    'I',  # usr_r1
    'I',  # usr_r2
    'I',  # usr_r3
    'I',  # usr_r4
    'I',  # usr_r5
    'I',  # usr_r6
    'I',  # usr_r7
    'I',  # usr_r8
    'I',  # usr_r9
    'I',  # usr_r10
    'I',  # usr_r11
    'I',  # usr_r12
    'I',  # usr_r13
    'I',  # usr_r14
    'I',  # irq_spsr
    'I',  # irq_r13
    'I',  # irq_r14
    'I',  # svc_spsr
    'I',  # svc_r13
    'I',  # svc_r14
    'I',  # abt_spsr
    'I',  # abt_r13
    'I',  # abt_r14
    'I',  # und_spsr
    'I',  # und_r13
    'I',  # und_r14
    'I',  # fiq_spsr
    'I',  # fiq_r8
    'I',  # fiq_r9
    'I',  # fiq_r10
    'I',  # fiq_r11
    'I',  # fiq_r12
    'I',  # fiq_r13
    'I',  # fiq_r14
])

tzbsp_dump_buf_t_v2 = ''.join([
    'I',  # magic
    'I',  # version
    'I',  # cpu count
])

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


class TZCpuCtx():

    def __init__(self, regs_t, version, alt_pc):
        i = 0
        self.regs = {}
        for r in regs_t:
            # in version 2, mon_lr is not actually useful for HLOS
            # debugging. Swap it with the alternate if necessary
            if (version > 1) and i == 0 and alt_pc is not None:
                r = alt_pc
            self.regs[tzbsp_register_names[i][0]] = r
            i += 1

    def print_regs(self, outfile, ramdump):
        for reg_name, t32_name, print_pc in tzbsp_register_names:
            if print_pc:
                modname = None
                symname = None
                symtab_st_size = None
                offset = None
                a = ramdump.unwind_lookup(self.regs[reg_name])
                if a is not None and len(a) > 3:
                    symname, offset, modname, symtab_st_size = a
                    if (modname is not None and symtab_st_size is not None):
                        pc_string = '[{0}+0x{1:x}/0x{3:x}] <{2}.ko>'.format(symname, offset, modname, symtab_st_size)
                    else:
                        pc_string = '[{0}+0x{1:x}/0x{2:x}]'.format(symname, offset, symtab_st_size)

                elif a is not None:
                    symname, offset, modname = a
                    if (modname is not None):
                        pc_string = '[{0}+0x{1:x}] <{2}.ko>'.format(symname, offset, modname)
                    else:
                        pc_string = '[{0}+0x{1:x}]'.format(symname, offset)
                else:
                    pc_string = ''
            else:
                pc_string = ''
            print_out_str('   {0} = 0x{1:08x} {2}'.format(
                reg_name, self.regs[reg_name], pc_string))
            if t32_name is not None:
                outfile.write(
                    'r.s {0} 0x{1:x}\n'.format(t32_name, self.regs[reg_name]))


@register_parser('--check-for-watchdog', 'Check for an FIQ watchdog', shortopt='-w')
class TZRegDump(RamParser):

    def __init__(self, *args):
        super(TZRegDump, self).__init__(*args)
        self.sc_status = []
        self.core_regs = []
        self.wdt0_status = []
        self.core_regs = []
        self.sec_regs = None
        self.ncores = 0
        self.version = 0
        self.mon_sp = []
        self.wdog_pc = []
        self.sc_regs = []

    def dump_all_regs(self):
        for i in range(0, self.ncores):
            coren_regs = self.ramdump.open_file('core{0}_regs.cmm'.format(i))

            if (self.sc_status[i] & tz_sc_ignore_status) == 0:
                if self.version == 0:
                    if (self.sc_status[i] & tz_sc_status_ns):
                        print_out_str(
                            'Core {0} was in the non-secure world'.format(i))

                    if (self.sc_status[i] & tz_sc_status_wdt):
                        print_out_str(
                            'Core {0} experienced a watchdog timeout'.format(i))

                    if (self.sc_status[i] & tz_sc_status_sgi):
                        print_out_str(
                            'Core {0} did not experience a watchdog timeout but some other core did'.format(i))
                else:
                    if (self.sc_status[i] & v2tzbsp_sc_status_ns_bit):
                        print_out_str(
                            'Core {0} was in the non-secure world'.format(i))
                    else:
                        print_out_str(
                            'Core {0} was in the secure world'.format(i))

                    if (self.sc_status[i] & v2tzbsp_sc_status_sgi):
                        print_out_str(
                            'Core {0} received an SGI interrupt'.format(i))

                    if (self.sc_status[i] & v2tzbsp_sc_status_wdt):
                        print_out_str(
                            'Core {0} recieved the watchdog interrupt'.format(i))

                    if (self.sc_status[i] & v2tzbsp_sc_status_warm_boot):
                        print_out_str(
                            'core {0} has the warm boot flag set'.format(i))

                    print_out_str(
                        'Core {0} WDT status: {1:x}'.format(i, self.wdt0_status[i]))

            if (self.version >= 3):
                print_out_str('status:{0}'.format(hex(self.sc_status[i])))
                if (self.sc_status[i] & v3sdi_cpu_dump_status):
                    print_out_str(
                        'SDI dumped CPU context for core {0}'.format(i))
                elif (self.sc_status[i] & v3tzbsp_cpu_dump_status):
                    print_out_str(
                        'TZ dumped CPU context for core {0}'.format(i))

            print_out_str('core{0} regs:'.format(i))
            self.core_regs[i].print_regs(coren_regs, self.ramdump)
            coren_regs.close()

        secure_regs = self.ramdump.open_file('secure_world_regs.cmm')
        print_out_str('\n=============== secure contex ===========')
        self.sec_regs.print_regs(secure_regs, self.ramdump)
        print_out_str('============ end secure context ===========')
        secure_regs.close()
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

    def dump_core_pc(self, core):
        if self.version > 1:
            pc = self.wdog_pc[core]
        else:
            pc = self.core_regs[core].regs['mon_lr']
        lr = self.core_regs[core].regs['svc_r14']
        modname = None
        symname = None
        symtab_st_size = None
        offset = None

        a = self.ramdump.unwind_lookup(pc)
        if a is not None and len(a) > 3:
            symname, offset, modname, symtab_st_size = a
        elif a is not None:
            symname, offset, modname = a
        else:
            offset = 0

        if (modname is not None and symtab_st_size is not None):
            print_out_str(
                'Core {3} PC: {0}+0x{1:x}/0x{5:x} <0x{2:x}> [{4}.ko]'.format(symname, offset, pc, core, modname, symtab_st_size))

        elif (modname is None and symtab_st_size is not None):
            fname = self.ramdump.get_file_name_from_addr(pc)
            if fname is not None:
                print_out_str(
                    'Core {3} PC: {0}+0x{1:x}/0x{5:x} <0x{2:x}> [{4}]'.format(symname, offset, pc, core, fname, symtab_st_size))
            else:
                print_out_str(
                    '{3}: [{0}+0x{1:x}/0x{4:x}] <0x{2:x}>'.format(symname, offset, pc, core, symtab_st_size))

        else:
            print_out_str(
                  'Core {3} PC: {0}+0x{1:x} <0x{2:x}>'.format(symname, offset, pc, core))

        modname = None
        symtab_st_size = None
        a = self.ramdump.unwind_lookup(lr)
        if a is not None and len(a) > 3:
            symname, offset, modname, symtab_st_size = a
        elif a is not None:
            symname, offset, modname = a
        else:
            symname = 'UNKNOWN'
            offset = 0
        if (modname is not None and symtab_st_size is not None):
            print_out_str(
                'Core {3} LR: {0}+0x{1:x}/0x{5:x} <0x{2:x}> [{4}.ko]'.format(symname, offset, lr, core, modname, symtab_st_size))
        elif (modname is None and symtab_st_size is not None):
            fname = self.ramdump.get_file_name_from_addr(lr)
            if fname is not None:
                print_out_str(
                    'Core {3} LR: {0}+0x{1:x}/0x{5:x} <0x{2:x}> [{4}]'.format(symname, offset, lr, core, fname, symtab_st_size))
            else:
                print_out_str(
                    '{3}: [{0}+0x{1:x}/0x{4:x}]  <0x{2:x}>'.format(symname, offset, lr, core, symtab_st_size))

        else:
            print_out_str(
                'Core {3} LR: {0}+0x{1:x} <0x{2:x}>'.format(symname, offset, lr, core))

        print_out_str('')
        self.ramdump.unwind.unwind_backtrace(
            self.core_regs[core].regs['svc_r13'], 0, pc, lr, '')
        print_out_str('')

    def host_order(self, s):
        r = s[6]+s[7]+s[4]+s[5]+s[2]+s[3]+s[0]+s[1]
        return r


    def parse_reg(self, j, buf, reg_name):
        b = 0xb0000000
        c = 0xc0000000
        d = 0xd0000000
        reg_val  = buf[j:].encode('hex')
        reg_addr = self.host_order(reg_val[0:8])
        reg_add_hex_str = '0x{0}'.format(reg_addr)
        reg_add_hex = int(reg_add_hex_str, 16)
        if not ((reg_add_hex & b) == b or (reg_add_hex & c) == c or ( reg_add_hex & d) == d):
            print_out_str("    {0} = 0x{1:x}".format(reg_name,reg_add_hex))
            return

        modname = None
        offset = None
        symtab_st_size = None
        symname = None
        a = self.ramdump.unwind_lookup(reg_add_hex)
        if a is not None and len(a) > 3:
            symname, offset, modname, symtab_st_size = a
        elif a is not None:
            symname, offset, modname = a
        else:
            symname = 'UNKNOWN'
            offset = 0

        if (modname is not None and symtab_st_size is not None):
            print_out_str(
                '    {4} =  0x{2:x}  [{0}+0x{1:x}/0x{5:x}] [{3}.ko]'.format(symname, offset, reg_add_hex, modname, reg_name, symtab_st_size))
        elif (modname is None and symtab_st_size is not None):
            fname = self.ramdump.get_file_name_from_addr(reg_add_hex)
            if fname is not None:
                print_out_str(
                    '    {4} = 0x{2:x} [{0}+0x{1:x}/0x{5:x}] [{3}]'.format(symname, offset, reg_add_hex, fname, reg_name, symtab_st_size))
            else:
               print_out_str(
                   '    {3} = 0x{2:x} [{0}+0x{1:x}/0x{4:x}]'.format(symname, offset, reg_add_hex, reg_name, symtab_st_size))
        else:
           print_out_str(
                   '    {3} = 0x{2:x} [{0}+0x{1:x}]'.format(symname, offset, reg_add_hex, reg_name))

    def svc_regs(self, buf):
        svc_str = "SVC: R1-R14"
        try:
                svc_ind = buf.index(svc_str)

        except:
                print_out_str('SVC Mode registers not found')
                return
        j = svc_ind + len(svc_str) + 5
        reg_hex = buf[j:].encode('hex')

        for i in range(1, 14):
                reg_val  = buf[j:].encode('hex')
                print_out_str("    SVC_R{0} = 0x{1}".format(i, self.host_order(reg_val[0:8])))
                j = j + 4

        self.parse_reg(j, buf, 'SVC_R14')
        return True

    def sys_regs(self, buf):
        sys_str = "SYS:R13-R14"
        try:
                sys_ind = buf.index(sys_str)

        except:
                print_out_str('SYS Mode registers not found')
                return
        j = sys_ind + len(sys_str) + 1
        print_out_str('\n')
        reg_val  = buf[j:].encode('hex')
        print_out_str("    SYS_R13 = 0x{0}".format(self.host_order(reg_val[0:8])))
        self.parse_reg(j+4, buf, 'SYS_R14')
        return True

    def irq_regs(self, buf):
        irq_str = "IRQ:R13-R14"
        try:
                irq_ind = buf.index(irq_str)

        except:
                print_out_str('IRQ Mode registers not found')
                return
        j = irq_ind + len(irq_str) + 1
        print_out_str('\n')
        reg_val  = buf[j:].encode('hex')
        print_out_str("    IRQ_R13 = 0x{0}".format(self.host_order(reg_val[0:8])))
        self.parse_reg(j+4, buf, 'IRQ_R14')
        return True

    def abt_regs(self, buf):
        abt_str = "ABT:R13-R14"
        try:
                abt_ind = buf.index(abt_str)

        except:
                print_out_str('ABT Mode registers not found')
                return
        j = abt_ind + len(abt_str) + 1
        print_out_str('\n')
        reg_val  = buf[j:].encode('hex')
        print_out_str("    ABT_R13 = 0x{0}".format(self.host_order(reg_val[0:8])))
        self.parse_reg(j+4, buf, 'ABT_R14')
        return True

    def und_regs(self, buf):
        und_str = "UND:R13-R14"
        try:
                und_ind = buf.index(und_str)

        except:
                print_out_str('UND Mode registers not found')
                return
        j = und_ind + len(und_str) + 1
        print_out_str('\n')
        reg_val  = buf[j:].encode('hex')
        print_out_str("    UND_R13 = 0x{0}".format(self.host_order(reg_val[0:8])))
        self.parse_reg(j+4, buf, 'UND_R14')
        return True

    def dump_cpu_regs(self, cpu_reg_path, cpu_number):
        if cpu_reg_path is not None:
            if os.path.exists(cpu_reg_path):
                fh = open(cpu_reg_path, 'rb')
            else:
                return
        else:
            return
        buf = fh.read(172)
        print_out_str(" --begin CORE{0}_REGS from SBL--".format(cpu_number))
        self.svc_regs(buf)
       # self.sys_regs(buf)
        self.irq_regs(buf)
        self.abt_regs(buf)
        self.und_regs(buf)
        print_out_str(" --end CORE{0}_REGS from SBL--".format(cpu_number))
        print_out_str("\n")
        fh.close()

        return True

    def init_regs(self, ebi_addr):
        cpu_count = 0
        status = self.ramdump.read_string(ebi_addr, tzbsp_dump_buf_t_v2, False)

        if status is None:
            print_out_str("!!! Couldn't read from {0:x}!".format(ebi_addr))
            print_out_str('!!! No FIQ information will be parsed')
            print_out_str(
                '!!! Do you need to specify a different offset for TZ?')
            return False

        if status[0] == 0x44434151:
            cpu_count = status[2]
            version = status[1]
            ebi_addr += struct.calcsize(tzbsp_dump_buf_t_v2)
        else:
            cpu_count = 2
            version = 0

        print_out_str('running dump version {0}'.format(version))
        for i in range(0, cpu_count):
            self.sc_status.append(self.ramdump.read_word(ebi_addr, False))
            ebi_addr += 4

        for i in range(0, cpu_count):
            self.sc_regs.append(
                self.ramdump.read_string(ebi_addr, tzbsp_dump_cpu_ctx_t, False))
            ebi_addr += struct.calcsize(tzbsp_dump_cpu_ctx_t)
            # new versions have extra data
            if version > 1:
                self.mon_sp.append(self.ramdump.read_word(ebi_addr, False))
                self.wdog_pc.append(
                    self.ramdump.read_word(ebi_addr + 4, False))
                ebi_addr += 8

        sc_secure = self.ramdump.read_string(
            ebi_addr, tzbsp_dump_cpu_ctx_t, False)
        ebi_addr += struct.calcsize(tzbsp_dump_cpu_ctx_t)

        for i in range(0, cpu_count):
            self.wdt0_status.append(self.ramdump.read_word(ebi_addr, False))
            ebi_addr += 4

        if version > 1:
            for regs, p in zip(self.sc_regs, self.wdog_pc):
                self.core_regs.append(TZCpuCtx(regs, version, p))
        else:
            for regs in self.sc_regs:
                self.core_regs.append(TZCpuCtx(regs, version, None))
        self.sec_regs = TZCpuCtx(sc_secure, version, None)
        self.ncores = cpu_count
        self.version = version
        return True

    def print_wdog_pet_details(self):
        if re.search('3.4.\d', self.ramdump.version) is not None:
            last_pet_addr = self.ramdump.addr_lookup('last_pet')
            if last_pet_addr is not None:
                last_pet = self.ramdump.read_dword(last_pet_addr)
                print_out_str('Most recent time that the watchdog was pet (last pet) : {0}.{1:6}'
                                            .format(last_pet / 1000000000, last_pet % 1000000000))

        jiffies_addr = self.ramdump.addr_lookup('jiffies')
        jiffies = self.ramdump.read_word(jiffies_addr)
        print_out_str('System up time (jiffies) : {0}.{1:2}\n'.format((jiffies / 100)+300,
                                    jiffies % 100))

    def parse(self):
        self.print_wdog_pet_details()
        ebi_addr = self.ramdump.read_tz_offset()
        if ebi_addr is None:
            print_out_str(
                '!!! Could not read from IMEM at address {0:x}'.format(self.ramdump.tz_addr))
            return None

        if (ebi_addr == 0):
            self.dump_cpu_regs(self.ramdump.cpu0_reg_path, 0)
            self.dump_cpu_regs(self.ramdump.cpu1_reg_path, 1)

            print_out_str(
                '!!! No EBI address at IMEM location {0:x}.'.format(self.ramdump.tz_addr))
            print_out_str('!!! No FIQ occured on this system')
            return None

        print_out_str(
            '[!!!!] Read {0:x} from IMEM successfully!'.format(ebi_addr))
        if self.ramdump.Is_Hawkeye():
            return
        print_out_str('[!!!!] An FIQ occured on the system!')

        # The debug image will be responsible for printing out the register
        # information, no need to print it twice
        if self.ramdump.is_config_defined('CONFIG_MSM_DEBUG_IMAGE'):
            print_out_str(
                '[!!!!] Debug image was enabled, the contexts will be printed there')
            return

        regs = self.init_regs(ebi_addr)
        if regs is False:
            print_out_str('!!! Could not get registers from TZ dump')
            return

        for i in range(self.ncores):
            self.dump_core_pc(i)
        self.dump_all_regs()


def get_wdog_timing(ramdump):
    print_out_str('!!!!!!!!!!!! Calling from get_wdog_timing')
    jiffies_addr = ramdump.addr_lookup('jiffies')
    last_ns_addr = ramdump.addr_lookup('last_ns')
    last_pet_addr = ramdump.addr_lookup('last_pet')
    pet_delay_time_addr = ramdump.addr_lookup('delay_time')
    dogstruct_addr = ramdump.addr_lookup('dogwork_struct')

    timer_offset = ramdump.field_offset('struct delayed_work', 'timer')
    timer_expires_offset = ramdump.field_offset(
        'struct timer_list', 'expires')

    timer_expires = ramdump.read_word(
        dogstruct_addr + timer_offset + timer_expires_offset)
    jiffies = ramdump.read_word(jiffies_addr)
    if (timer_expires > jiffies):
        print_out_str('Pet function to be scheduled in {0} seconds'.format(
            (timer_expires - jiffies) * 0.01))
    else:
        print_out_str('Pet function may have been blocked.')
        print_out_str('Jiffies is {0} seconds after when  it was supposed to be scheduled'.format(
            (jiffies - timer_expires) * 0.01))

    if (last_ns_addr != 0) and (last_pet_addr != 0) and (pet_delay_time_addr != 0):
        last_ns = ramdump.read_dword(last_ns_addr)
        last_pet = ramdump.read_dword(last_pet_addr)
        pet_delay_time = ramdump.read_word(pet_delay_time_addr)

        diff = last_ns - last_pet

        print_out_str('Most recent time that the watchdog was pet: {0}.{1:6}'.format(
            last_pet / 1000000000, last_pet % 1000000000))
        print_out_str('Most recent timestamp read from the timer hardware: {0}.{1:6}'.format(
            last_ns / 1000000000, last_ns % 1000000000))
        if (last_ns > last_pet):
            print_out_str(
                'Pet delay time in this build is {0} seconds'.format((pet_delay_time * 10) / 1000))
            print_out_str('The watchdog was not pet for at least {0}.{1:6} seconds'.format(
                diff / 1000000000, diff % 1000000000))

        print_out_str(
            '!!! The difference between the two timestamps above is NOT')
        print_out_str(
            '!!! to be used to determine whether a watchdog bite occured')

    elif (last_pet_addr):
        last_pet = ramdump.read_dword(last_pet_addr)
        print_out_str('Most recent time that the watchdgo was pet: {0}.{1:6}'.format(
            last_pet / 1000000000, last_pet % 1000000000))
        print_out_str('The watchdog was not pet for at least {0}.{1:6} seconds'.format(
            pet_delay_diff / 1000000000, pet_delay_diff % 1000000000))
