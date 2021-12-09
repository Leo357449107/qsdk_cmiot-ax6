# Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
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

from print_out import print_out_str
from collections import namedtuple
from parser_util import register_parser, RamParser

ELF32HEADERFORMAT = '<16sHHIIIIIHHHHHH'
ELF32HEADERSIZE = struct.calcsize(ELF32HEADERFORMAT)
PRG32HEADERFORMAT = 'IIIIIIII'
PRG32HEADERSIZE = struct.calcsize(PRG32HEADERFORMAT)
PF_W = 2


@register_parser('--check-rodata', 'check rodata in dump against the static image')
class ROData(RamParser):

    def parse(self):
        print_out_str('check rodata disabled')
        return
        stext = self.ramdump.addr_lookup('stext')
        etext = self.ramdump.addr_lookup('_etext')

        with self.ramdump.open_file('roareadiff.txt') as roarea_out:

            fd = open(self.ramdump.vmlinux, 'rb')
            if not fd:
                print_out_str('Could not open {0}.'.format(file_path))
                return

            ElfHeader = namedtuple(
                'ElfHeader', 'ident type machine version entry phoff shoff flags ehsize phentsize phnum shentsize shnum shstrndx')
            raw_elfheader = fd.read(ELF32HEADERSIZE)
            elfheader = ElfHeader._make(
                struct.unpack(ELF32HEADERFORMAT, raw_elfheader))

            PrgHeader = namedtuple(
                'Prgheader', 'type offset vaddr paddr filesz memsz flags align')
            for i in range(elfheader.phnum):
                fd.seek(elfheader.phoff + i * PRG32HEADERSIZE)
                raw_prgheader = fd.read(PRG32HEADERSIZE)
                prgheader = PrgHeader._make(
                    struct.unpack(PRG32HEADERFORMAT, raw_prgheader))

                if not prgheader.flags & PF_W:
                    count = prgheader.vaddr
                    detect = 0
                    printed_once = False
                    while count < prgheader.vaddr + prgheader.memsz:
                        fd.seek(prgheader.offset + (count - prgheader.vaddr))
                        ram_value = self.ramdump.read_word(count)
                        vm_value = struct.unpack('I', fd.read(4))[0]
                        if ram_value is None:
                            break

                        if detect == 0 and vm_value != ram_value:
                            if not printed_once:
                                print_out_str(
                                    'Differences found! Differences written to roareadiff.txt')
                            printed_once = True
                            ddr_str = 'detect RO area differences between vmlinux and DDR at 0x{0:0>8x}\n'.format(
                                count)
                            ddr_str += 'from DDR:\n'
                            ddr_str += '{0:0>8x}  *{1:0>8x}'.format(count,
                                                                    ram_value)
                            vmlinux_str = 'from vmlinux:\n'
                            vmlinux_str += '{0:0>8x}  *{1:0>8x}'.format(count,
                                                                        vm_value)
                            detect += 1
                        elif 0 < detect and detect < 64:
                            if detect % 8 == 0:
                                ddr_str += '\n{0:0>8x} '.format(count)
                                vmlinux_str += '\n{0:0>8x} '.format(count)
                            ddr_str += ' '
                            vmlinux_str += ' '
                            if vm_value != ram_value:
                                ddr_str += '*'
                                vmlinux_str += '*'
                            else:
                                ddr_str += ' '
                                vmlinux_str += ' '
                            ddr_str += '{0:0>8x}'.format(ram_value)
                            vmlinux_str += '{0:0>8x}'.format(vm_value)
                            detect += 1
                        elif detect == 64:
                            ddr_str += '\n\n'
                            vmlinux_str += '\n\n'
                            roarea_out.write(ddr_str)
                            roarea_out.write(vmlinux_str)
                            detect = 0
                            continue

                        count += 4

            fd.close()
