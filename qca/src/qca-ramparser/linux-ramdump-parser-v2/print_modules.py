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
from print_out import print_out_str
from parser_util import register_parser, RamParser
import linux_list as llist

@register_parser('--print-modules', 'print module information')

class Modules(RamParser):

       def mod_list_func(self, mod_list):
               name_offset = self.ramdump.field_offset('struct module', 'name')
               name = self.ramdump.read_cstring(mod_list + name_offset, 30)
               if len(name) > 1:
                       self.module_out.write('0x{0:x}   {1}\n'.format(mod_list, name))

       def print_modules(self):
               module_out = self.ramdump.open_file('modules.txt')
               self.module_out = module_out
               next_mod_offset = self.ramdump.field_offset('struct module','list')
               mod_start = self.ramdump.read_word('modules')
               self.module_out.write("Module list\n")
               self.module_out.write("-----------\n")
               self.module_out.write("Address      Name\n")
               self.module_out.write("------------------------\n")
               list_walker = llist.ListWalker(self.ramdump, mod_start, next_mod_offset)
               list_walker.walk(mod_start, self.mod_list_func)
               module_out.close()

       def parse(self):
               self.print_modules()
               print_out_str("check modules.txt for the module list")
