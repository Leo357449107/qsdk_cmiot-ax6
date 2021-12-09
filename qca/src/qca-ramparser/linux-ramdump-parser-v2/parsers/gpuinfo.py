# Copyright (c) 2013, The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

from print_out import print_out_str
from parser_util import register_parser, RamParser


@register_parser('--print-gpuinfo', 'print gpu info like ringbuffer,snapshot and pointer addresses', optional=True)
class GPUinfo(RamParser):

    def parse(self):
            if not self.ramdump.is_config_defined('CONFIG_MSM_KGSL'):
                print_out_str(
                    'No GPU support detected... Skipping GPUinfo parser.')
                return
            adreno_dev_addr = self.ramdump.addr_lookup('device_3d0')
            kgsl_dev_addr = adreno_dev_addr + self.ramdump.field_offset(
                'struct adreno_device', 'dev')
            snapshot = self.ramdump.read_word(kgsl_dev_addr + self.ramdump.field_offset(
                'struct kgsl_device', 'snapshot'))
            snapshot_size = self.ramdump.read_word(kgsl_dev_addr +
                                                   self.ramdump.field_offset(
                                                       'struct kgsl_device', 'snapshot_size'))
            snapshot_timestamp = self.ramdump.read_word(kgsl_dev_addr +
                                                        self.ramdump.field_offset(
                                                            'struct kgsl_device',
                                                            'snapshot_timestamp'))
            ringbuffer_offset = self.ramdump.field_offset(
                'struct adreno_device', 'ringbuffer')
            ringbuffer_addr = self.ramdump.read_word(adreno_dev_addr +
                                                     ringbuffer_offset +
                                                     self.ramdump.field_offset(
                                                         'struct adreno_ringbuffer', 'buffer_desc') +
                                                     self.ramdump.field_offset(
                                                         'struct kgsl_memdesc', 'physaddr'))
            memptrs_addr = self.ramdump.read_word(adreno_dev_addr + ringbuffer_offset +
                                                  self.ramdump.field_offset(
                                                      'struct adreno_ringbuffer', 'memptrs_desc') +
                                                  self.ramdump.field_offset(
                                                      'struct kgsl_memdesc', 'physaddr'))
            memstore_addr = self.ramdump.read_word(kgsl_dev_addr +
                                                   self.ramdump.field_offset(
                                                       'struct kgsl_device', 'memstore') +
                                                   self.ramdump.field_offset(
                                                       'struct kgsl_memdesc', 'physaddr'))
            ringbuffer_size = self.ramdump.read_word(adreno_dev_addr +
                                                     ringbuffer_offset +
                                                     self.ramdump.field_offset(
                                                         'struct adreno_ringbuffer', 'sizedwords'))
            print_out_str('Ringbuffer address: {0:x}, Ringbuffer sizedwords: '
                          '{1:x}, Memstore address: {2:x}, Memptrs address: '
                          '{3:x}'.format(ringbuffer_addr, ringbuffer_size,
                                         memstore_addr, memptrs_addr))
            print_out_str('Sanpshot addr: {0:x}, Snapshot size: {1:x}, '
                          'Snapshot timestamp:{2:x}'.format(snapshot,
                                                            snapshot_size, snapshot_timestamp))
            current_context = self.ramdump.read_word(
                int(memstore_addr) + 32, False)
            retired_timestamp = self.ramdump.read_word(
                int(memstore_addr) + 8, False)
            i = 0
            for i in range(0, int(ringbuffer_size)):
                    data = self.ramdump.read_word(
                        int(ringbuffer_addr) + (i * 4), False)
                    if int(data) == int(retired_timestamp):
                            break
            i = i * 4
            print_out_str('Current context: {0:x}, Global eoptimestamp: {1:x} '
                          'found at Ringbuffer[{2:x}]'.format(current_context,
                                                              retired_timestamp, i))
