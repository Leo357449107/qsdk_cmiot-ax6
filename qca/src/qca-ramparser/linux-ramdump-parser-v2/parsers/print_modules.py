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

import os
import struct
import string
import re
from print_out import print_out_str
from parser_util import register_parser, RamParser
import linux_list as llist

@register_parser('--print-modules', 'print module information')

class Modules(RamParser):

    #simple light function which prints only the skb count;not module sysmbols
    def mod_light_func(self, mod_list):
        if(self.ramdump.isELF64() and (mod_list & 0xfff0000000 != self.ramdump.mod_start_addr)):
            return
        elif(self.ramdump.isELF32() and mod_list & 0xff000000 != self.ramdump.mod_start_addr):
            return
        name = self.ramdump.read_cstring(mod_list + self.name_offset, 30)
        if (name is None):
            return
        if len(name) < 1 and name.isalpha() is False:
            return
        if ((name == 'qca_nss_drv' or name == 'qca-nss-drv') and self.ramdump.stats_drv_offset is not None and self.ramdump.gnu_linkonce_this_size is not None):
            self.skb_count_addr = mod_list + int (self.ramdump.stats_drv_offset, 16) + int(self.ramdump.gnu_linkonce_this_size, 16)
            self.skb_count_addr = (self.skb_count_addr + 0x7) & ~0x7
            self.skb_count = self.ramdump.read_dword(self.skb_count_addr)
            print_out_str('stat_drv NSS_SKB_COUNT = {0}'.format(self.skb_count))


    def mod_list_func(self, mod_list):
        high_mem_addr = self.ramdump.addr_lookup('high_memory')
        vmalloc_offset = 0x800000
        vmalloc_start = self.ramdump.read_u32(high_mem_addr) + vmalloc_offset & (~int(vmalloc_offset - 0x1))
        if(self.ramdump.isELF64() and (mod_list & 0xfff0000000 != self.ramdump.mod_start_addr)):
            self.module_out.write('module list reached end: 0x{0:x}'.format(mod_list))
            return
        elif (self.ramdump.isELF32() and self.ramdump.Is_Hawkeye() and mod_list & 0xff000000 != self.ramdump.mod_start_addr and not ((vmalloc_start & 0xff000000 <= mod_list & 0xff000000) and (mod_list & 0xff000000 <= 0xff000000))):
            self.module_out.write('module list reached end: 0x{0:x}'.format(mod_list))
            return
        elif (self.ramdump.isELF32() and not self.ramdump.Is_Hawkeye() and mod_list & 0xff000000 != self.ramdump.mod_start_addr):
            self.module_out.write('module list reached end: 0x{0:x}'.format(mod_list))
            return

        name = self.ramdump.read_cstring(mod_list + self.name_offset, 30)

        if (name is None):
           return

        if len(name) < 1 and name.isalpha() is False:
           return

        if (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (5, 4):
           module_init_size = self.ramdump.read_u32(mod_list + self.ramdump.module_layout_init_offset + self.ramdump.module_size_offset)
           module_core_size = self.ramdump.read_u32(mod_list + self.ramdump.module_layout_core_offset + self.ramdump.module_size_offset)
           module_core = self.ramdump.read_word(mod_list + self.ramdump.module_layout_core_offset + self.ramdump.module_offset)
        else:
           module_init_size = self.ramdump.read_u32(mod_list + self.ramdump.module_init_size_offset)
           module_core_size = self.ramdump.read_u32(mod_list + self.ramdump.module_core_size_offset)
           module_core = self.ramdump.read_word(mod_list + self.ramdump.module_core_offset)
        mod_size = module_init_size + module_core_size

        self.module_out.write('------------------------------------------------------\n')
        self.module_out.write('{0:20} {1:25} {2:7} {3:12}\n'.format("Address","Name","Size","Core"))
        self.module_out.write('------------------------------------------------------\n')
        mod_nm = name+'.ko'
        self.module_out.write('0x{0:<18x} {1:25} {2:<7} 0x{3:x}\n\n'.format(mod_list, mod_nm, mod_size, module_core))

        self.module_out.write('------------------------------------------\n')
        self.module_out.write('module symbol information\n')
        self.module_out.write('------------------------------------------\n')
        if (re.search('3.14.77', self.ramdump.version) is not None or (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (4, 4)):
            if self.kallsyms_offset >= 0:
                kallsyms = self.ramdump.read_word(mod_list + self.kallsyms_offset)
                module_symtab_count = self.ramdump.read_u32(kallsyms + self.module_symtab_count_offset)
                module_symtab = self.ramdump.read_word(kallsyms + self.module_symtab_offset)
                module_strtab = self.ramdump.read_word(kallsyms + self.module_strtab_offset)
            else:
                kallsyms = -1
                module_symtab_count = -1
                module_symtab = -1
                module_strtab = -1
                print_out_str("CONFIG_KALLSYMS not available, tryin syms")
                syms_offset = self.ramdump.field_offset('struct module', 'syms')
                num_syms_offset = self.ramdump.field_offset('struct module', 'num_syms')
                module_sym_count = self.ramdump.read_u32(mod_list + num_syms_offset)
                module_sym = self.ramdump.read_u32(mod_list + syms_offset)
        else:
            module_symtab_count = self.ramdump.read_word(mod_list + self.module_symtab_count_offset)
            module_symtab = self.ramdump.read_word(mod_list + self.module_symtab_offset)
            module_strtab = self.ramdump.read_word(mod_list + self.module_strtab_offset)

        try:
            for i in range(1, module_symtab_count):
                module_symtab += self.symtab_size
                symtab_st_shndx = self.ramdump.read_u16(module_symtab + self.symtab_st_shndx_offset)
                if (self.ramdump.isELF32()):
                    symtab_st_info = self.ramdump.read_byte(module_symtab + self.symtab_st_info_offset)
                else:
                    symtab_st_info = self.ramdump.read_u16(module_symtab + self.symtab_st_info_offset)

                if (symtab_st_shndx != 0 and symtab_st_info != 'U'):
                    symtab_st_value = self.ramdump.read_word(module_symtab + self.symtab_st_value_offset)
                    symtab_st_name = self.ramdump.read_u32(module_symtab + self.symtab_st_name_offset)
                    symtab_st_size = self.ramdump.read_word(module_symtab + self.symtab_st_size_offset);
                    if(symtab_st_name is None):
                        continue
                    strtab_name = self.ramdump.read_cstring(module_strtab + symtab_st_name, 40)
                try:
                    self.module_out.write('{0:4}  0x{1:8x}  {2}/0x{3:x}\n'.format(i, symtab_st_value, strtab_name, symtab_st_size))
                except:
                    self.module_out.write('{0:4} 0x{1:8} {2}/0x{3}\n'.format(i, symtab_st_value, strtab_name, symtab_st_size))

            if module_symtab_count < 0:
                for i in range(1, module_sym_count):
                    if (self.ramdump.isELF32()):
                        name = self.ramdump.read_u32(module_sym + 4)
                        val = self.ramdump.read_u32(module_sym)
                        module_sym += 8
                    else:
                        name = self.ramdump.read_u32(module_sym + 8)
                        val = self.ramdump.read_u32(module_sym)
                        module_sym += 16

                    strtab_name = self.ramdump.read_cstring(name, 40)
                    try:
                        self.module_out.write('{0:4}  0x{1:8x}  {2}\n'.format(i, val, strtab_name))
                    except:
                        self.module_out.write('{0:4} 0x{1:8} {2}\n'.format(i, val, strtab_name))

	except MemoryError:
	    self.module_out.write ('MemoryError caught at module\n')
	except Exception as e:
	    self.module_out.write ('Exception caught at module : ' + format(e) + '\n')
	    self.module_out.write ('Exception found for Kallsyms : {}, Module Count Offset : {}, Module Count : {}\n'. format(kallsyms, self.module_symtab_count_offset, module_symtab_count))
        if ((mod_nm == 'qca_nss_drv.ko' or mod_nm == 'qca-nss-drv.ko') and self.ramdump.stats_drv_offset is not None and self.ramdump.gnu_linkonce_this_size is not None):
            self.skb_count_addr = mod_list + int (self.ramdump.stats_drv_offset, 16) + int(self.ramdump.gnu_linkonce_this_size, 16)
            self.skb_count_addr = (self.skb_count_addr + 0x7) & ~0x7
            self.skb_count = self.ramdump.read_dword(self.skb_count_addr)
            self.module_out.write (' stats_drv NSS_SKB_COUNT = {0}\n'.format(self.skb_count))
        self.module_out.write('------------------------------------------\n\n\n')

    def print_modules(self):
        module_out = self.ramdump.open_file('modules.txt')
        self.module_out = module_out
        next_mod_offset = self.ramdump.field_offset('struct module','list')

        if (self.ramdump.mod_start is None):
           self.module_out.write("module variable not valid")
           module_out.close()
           return

        self.module_out.write("Module list\n")
        self.module_out.write("-----------\n\n\n")

        self.name_offset = self.ramdump.field_offset('struct module', 'name')
        if (re.search('3.14.77', self.ramdump.version) is not None or (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (4, 4)):
            self.kallsyms_offset = self.ramdump.field_offset('struct module', 'kallsyms')
            self.module_symtab_offset = self.ramdump.field_offset('struct mod_kallsyms','symtab')
            self.module_strtab_offset = self.ramdump.field_offset('struct mod_kallsyms','strtab')
            self.module_symtab_count_offset = self.ramdump.field_offset('struct mod_kallsyms','num_symtab')
        else:
            self.module_symtab_offset = self.ramdump.field_offset('struct module','symtab')
            self.module_strtab_offset = self.ramdump.field_offset('struct module','strtab')
            self.module_symtab_count_offset = self.ramdump.field_offset('struct module','num_symtab')

        if(self.ramdump.isELF64()):
            self.symtab_st_shndx_offset = self.ramdump.field_offset('struct elf64_sym', 'st_shndx')
            self.symtab_st_value_offset = self.ramdump.field_offset('struct elf64_sym', 'st_value')
            self.symtab_st_info_offset = self.ramdump.field_offset('struct elf64_sym', 'st_info')
            self.symtab_st_name_offset = self.ramdump.field_offset('struct elf64_sym', 'st_name')
            self.symtab_st_size_offset = self.ramdump.field_offset('struct elf64_sym', 'st_size')
        else:
            self.symtab_st_shndx_offset = self.ramdump.field_offset('struct elf32_sym', 'st_shndx')
            self.symtab_st_value_offset = self.ramdump.field_offset('struct elf32_sym', 'st_value')
            self.symtab_st_info_offset = self.ramdump.field_offset('struct elf32_sym', 'st_info')
            self.symtab_st_name_offset = self.ramdump.field_offset('struct elf32_sym', 'st_name')
            self.symtab_st_size_offset = self.ramdump.field_offset('struct elf32_sym', 'st_size')

        if(self.ramdump.isELF64()):
            self.symtab_size = self.ramdump.sizeof('struct elf64_sym')
        else:
            self.symtab_size = self.ramdump.sizeof('struct elf32_sym')

        #simple walk through to get address of module
        name_list_walker = llist.ListWalker(self.ramdump, self.ramdump.mod_start, next_mod_offset)
        name_list_walker.walk(self.ramdump.mod_start, self.mod_light_func)

        list_walker = llist.ListWalker(self.ramdump, self.ramdump.mod_start, next_mod_offset)
        list_walker.walk(self.ramdump.mod_start, self.mod_list_func)
        module_out.close()

    def parse(self):
        self.print_modules()
        print_out_str("check modules.txt for the module list")
