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
import linux_list as llist
import re
import shutil
import os
import platform
import subprocess

from print_out import print_out_str
from qdss import QDSSDump
from watchdog_v2 import TZRegDump_v2

MEMDUMPV2_MAGIC = 0x42445953

class client(object):
    MSM_DUMP_DATA_CPU_CTX = 0x00
    MSM_DUMP_DATA_L1_INST_CACHE = 0x60
    MSM_DUMP_DATA_L1_DATA_CACHE = 0x80
    MSM_DUMP_DATA_ETM_REG = 0xA0
    MSM_DUMP_DATA_L2_CACHE = 0xC0
    MSM_DUMP_DATA_L3_CACHE = 0xD0
    MSM_DUMP_DATA_OCMEM = 0xE0
    MSM_DUMP_DATA_TMC_ETF = 0xF0
    MSM_DUMP_DATA_TMC_REG = 0x100
    MSM_DUMP_DATA_TMC_ETF_REG = 0x101
    MSM_DUMP_DATA_LOG_BUF = 0x110
    MSM_DUMP_DATA_LOG_BUF_FIRST_IDX = 0x111
    MSM_DUMP_DATA_MAX = 0x112

client_table = {
    'MSM_DUMP_DATA_CPU_CTX': 'parse_cpu_ctx',
    'MSM_DUMP_DATA_L1_INST_CACHE': 'parse_l1_inst_cache',
    'MSM_DUMP_DATA_L1_DATA_CACHE': 'parse_l1_data_cache',
    'MSM_DUMP_DATA_ETM_REG': 'parse_qdss_common',
    'MSM_DUMP_DATA_L2_CACHE': 'parse_l2_cache',
    'MSM_DUMP_DATA_L3_CACHE': 'parse_l3_cache',
    'MSM_DUMP_DATA_OCMEM': 'parse_ocmem',
    'MSM_DUMP_DATA_TMC_ETF': 'parse_qdss_common',
    'MSM_DUMP_DATA_TMC_REG': 'parse_qdss_common',
}

qdss_tag_to_field_name = {
    'MSM_DUMP_DATA_TMC_REG': 'tmc_etr_start',
    'MSM_DUMP_DATA_TMC_ETF': 'etf_start',
    'MSM_ETM0_REG': 'etm_regs0',
    'MSM_ETM1_REG': 'etm_regs1',
    'MSM_ETM2_REG': 'etm_regs2',
    'MSM_ETM3_REG': 'etm_regs3',
}

class DebugImage_v2():

    def __init__(self):
        self.qdss = QDSSDump()
        self.dump_type_lookup_table = []
        self.dump_table_id_lookup_table = []
        self.dump_data_id_lookup_table  = []

    def parse_cpu_ctx(self, version,  start, end, client_id, ram_dump):
        core = client_id - client.MSM_DUMP_DATA_CPU_CTX
        if ram_dump.Is_Dakota():
            core = 0

        print_out_str(
            'Parsing CPU{2} context start {0:x} end {1:x}'.format(start, end, core))

        regs = TZRegDump_v2()
        if regs.init_regs(version, start, end, core, ram_dump) is False:
            print_out_str('!!! Could not get registers from TZ dump')
            return
        regs.dump_core_pc(ram_dump)
        regs.dump_all_regs(ram_dump)

    def parse_qdss_common(self, start, end, client_id, ram_dump):
        client_name = self.dump_data_id_lookup_table[client_id]

        print_out_str(
            'Parsing {0} context start {1:x} end {2:x}'.format(client_name, start, end))

        if client_id == client.MSM_DUMP_DATA_TMC_ETF_REG:
            setattr(self.qdss, 'tmc_etf_start', start)
        else:
            setattr(self.qdss, qdss_tag_to_field_name[client_name], start)

    def ftrace_field_func(self, common_list, ram_dump):
        name_offset = ram_dump.field_offset('struct ftrace_event_field', 'name')
        type_offset = ram_dump.field_offset('struct ftrace_event_field', 'type')
        filter_type_offset = ram_dump.field_offset('struct ftrace_event_field', 'filter_type')
        field_offset = ram_dump.field_offset('struct ftrace_event_field', 'offset')
        size_offset = ram_dump.field_offset('struct ftrace_event_field', 'size')
        signed_offset = ram_dump.field_offset('struct ftrace_event_field', 'is_signed')

        name = ram_dump.read_word(common_list + name_offset)
        field_name = ram_dump.read_cstring(name, 256)
        type_name = ram_dump.read_word(common_list + type_offset)
        type_str = ram_dump.read_cstring(type_name, 256)
        offset = ram_dump.read_u32(common_list + field_offset)
        size = ram_dump.read_u32(common_list + size_offset)
        signed = ram_dump.read_u32(common_list + signed_offset)

        if re.match('(.*)\[(.*)', type_str) and not(re.match('__data_loc', type_str)):
            s = re.split('\[', type_str)
            s[1] = '[' + s[1]
            self.formats_out.write("\tfield:{0} {1}{2};\toffset:{3};\tsize:{4};\tsigned:{5};\n".format(s[0], field_name, s[1], offset, size, signed))
        else:
            self.formats_out.write("\tfield:{0} {1};\toffset:{2};\tsize:{3};\tsigned:{4};\n".format(type_str, field_name, offset, size, signed))

    def ftrace_events_func(self, ftrace_list, ram_dump):
        name_offset = ram_dump.field_offset('struct ftrace_event_call', 'name')
        event_offset = ram_dump.field_offset('struct ftrace_event_call', 'event')
        fmt_offset = ram_dump.field_offset('struct ftrace_event_call', 'print_fmt')
        class_offset = ram_dump.field_offset('struct ftrace_event_call', 'class')
        type_offset = ram_dump.field_offset('struct trace_event', 'type')
        fields_offset = ram_dump.field_offset('struct ftrace_event_class', 'fields')
        common_field_list = ram_dump.addr_lookup('ftrace_common_fields')
        field_next_offset = ram_dump.field_offset('struct ftrace_event_field', 'link')

        name = ram_dump.read_word(ftrace_list + name_offset)
        name_str = ram_dump.read_cstring(name, 512)
        event_id = ram_dump.read_word(ftrace_list + event_offset + type_offset)
        fmt = ram_dump.read_word(ftrace_list + fmt_offset)
        fmt_str = ram_dump.read_cstring(fmt, 2048)

        self.formats_out.write("name: {0}\n".format(name_str))
        self.formats_out.write("ID: {0}\n".format(event_id))
        self.formats_out.write("format:\n")

        list_walker = llist.ListWalker(ram_dump, common_field_list, field_next_offset)
        list_walker.walk_prev(common_field_list, self.ftrace_field_func, ram_dump)
        self.formats_out.write("\n")

        event_class = ram_dump.read_word(ftrace_list + class_offset)
        field_list =  event_class + fields_offset
        list_walker = llist.ListWalker(ram_dump, field_list, field_next_offset)
        list_walker.walk_prev(field_list, self.ftrace_field_func, ram_dump)
        self.formats_out.write("\n")
        self.formats_out.write("print fmt: {0}\n".format(fmt_str))

    def collect_ftrace_format(self, ram_dump):
        formats = os.path.join('qtf', 'map_files', 'formats.txt')
        formats_out = ram_dump.open_file(formats)
        self.formats_out = formats_out

        ftrace_events_list = ram_dump.addr_lookup('ftrace_events')
        next_offset = ram_dump.field_offset('struct ftrace_event_call', 'list')
        list_walker = llist.ListWalker(ram_dump, ftrace_events_list, next_offset)
        list_walker.walk_prev(ftrace_events_list, self.ftrace_events_func, ram_dump)

        self.formats_out.close

    def parse_qtf(self, ram_dump):
        out_dir = ram_dump.outdir
        if platform.system() != 'Windows':
            return

        qtf_path = ram_dump.qtf_path
        if qtf_path is None:
            try:
                import local_settings
                try:
                    qtf_path = local_settings.qtf_path
                except AttributeError as e:
                    print_out_str("attribute qtf_path in local_settings.py looks bogus. Please see README.txt")
                    print_out_str("Full message: %s" % e.message)
                    return
            except ImportError:
                print_out_str("missing qtf_path local_settings.")
                print_out_str("Please see the README for instructions on setting up local_settings.py")
                return

        if qtf_path is None:
            print_out_str("!!! Incorrect path for qtf specified.")
            print_out_str("!!! Please see the README for instructions on setting up local_settings.py")
            return

        if not os.path.exists(qtf_path):
            print_out_str("!!! qtf_path {0} does not exist! Check your settings!".format(qtf_path))
            return

        if not os.access(qtf_path, os.X_OK):
            print_out_str("!!! No execute permissions on qtf path {0}".format(qtf_path))
            return

        if os.path.getsize(os.path.join(out_dir, 'tmc-etf.bin')) > 0:
            trace_file = os.path.join(out_dir, 'tmc-etf.bin')
        elif os.path.getsize(os.path.join(out_dir, 'tmc-etr.bin')) > 0:
            trace_file = os.path.join(out_dir, 'tmc-etr.bin')
        else:
            return

        port = 12345
        qtf_dir = os.path.join(out_dir, 'qtf')
        workspace = os.path.join(qtf_dir, 'qtf.workspace')
        qtf_out = os.path.join(out_dir, 'qtf.txt')
        chipset = 'msm' + str(ram_dump.hw_id)
        hlos = 'LA'

        p = subprocess.Popen([qtf_path, '-s', '{0}'.format(port)])
        subprocess.call('{0} -c {1} new workspace {2} {3} {4}'.format(qtf_path, port, qtf_dir, chipset, hlos))

        self.collect_ftrace_format(ram_dump)

        subprocess.call('{0} -c {1} open workspace {2}'.format(qtf_path, port, workspace))
        subprocess.call('{0} -c {1} open bin {2}'.format(qtf_path, port, trace_file))
        subprocess.call('{0} -c {1} stream trace table {2}'.format(qtf_path, port, qtf_out))
        subprocess.call('{0} -c {1} exit'.format(qtf_path, port))
        p.communicate('quit')

    def parse_dump_v2(self, ram_dump):
        self.dump_type_lookup_table = ram_dump.gdbmi.get_enum_lookup_table(
            'msm_dump_type', 2)
        self.dump_table_id_lookup_table = ram_dump.gdbmi.get_enum_lookup_table(
            'msm_dump_table_ids', 0x110)
        self.dump_data_id_lookup_table = ram_dump.gdbmi.get_enum_lookup_table(
            'msm_dump_data_ids', 0x112)
        cpu_present_bits = ram_dump.read_word('cpu_present_bits')
        cpus = bin(cpu_present_bits).count('1')
        # per cpu entries
        for i in range(1, cpus):

                self.dump_data_id_lookup_table[
                    client.MSM_DUMP_DATA_CPU_CTX + i] = 'MSM_DUMP_DATA_CPU_CTX'
                self.dump_data_id_lookup_table[
                    client.MSM_DUMP_DATA_L1_INST_CACHE + i] = 'MSM_DUMP_DATA_L1_INST_CACHE'
                self.dump_data_id_lookup_table[
                    client.MSM_DUMP_DATA_L1_DATA_CACHE + i] = 'MSM_DUMP_DATA_L1_DATA_CACHE'
                self.dump_data_id_lookup_table[
                    client.MSM_DUMP_DATA_ETM_REG + i] = 'MSM_DUMP_DATA_ETM_REG'
        # 0x100 - tmc-etr registers and 0x101 - for tmc-etf registers
        self.dump_data_id_lookup_table[
            client.MSM_DUMP_DATA_TMC_REG + 1] = 'MSM_DUMP_DATA_TMC_REG'
        self.dump_data_id_lookup_table[
            client.MSM_DUMP_DATA_LOG_BUF] = 'MSM_DUMP_DATA_LOG_BUF'
        self.dump_data_id_lookup_table[
            client.MSM_DUMP_DATA_LOG_BUF_FIRST_IDX] = 'MSM_DUMP_DATA_LOG_BUF_FIRST_IDX'
        dump_table_ptr_offset = ram_dump.field_offset(
            'struct msm_memory_dump', 'table')
        dump_table_version_offset = ram_dump.field_offset(
            'struct msm_dump_table', 'version')
        dump_table_num_entry_offset = ram_dump.field_offset(
            'struct msm_dump_table', 'num_entries')
        dump_table_entry_offset = ram_dump.field_offset(
            'struct msm_dump_table', 'entries')
        dump_entry_id_offset = ram_dump.field_offset(
            'struct msm_dump_entry', 'id')
        dump_entry_name_offset = ram_dump.field_offset(
            'struct msm_dump_entry', 'name')
        dump_entry_type_offset = ram_dump.field_offset(
            'struct msm_dump_entry', 'type')
        dump_entry_addr_offset = ram_dump.field_offset(
            'struct msm_dump_entry', 'addr')
        dump_data_version_offset = ram_dump.field_offset(
            'struct msm_dump_data', 'version')
        dump_data_magic_offset =  ram_dump.field_offset(
            'struct msm_dump_data', 'magic')
        dump_data_name_offset = ram_dump.field_offset(
            'struct msm_dump_data', 'name')
        dump_data_addr_offset = ram_dump.field_offset(
            'struct msm_dump_data', 'addr')
        dump_data_len_offset = ram_dump.field_offset(
            'struct msm_dump_data', 'len')
        dump_data_reserved_offset = ram_dump.field_offset(
            'struct msm_dump_data', 'reserved')
        dump_entry_size = ram_dump.sizeof('struct msm_dump_entry')
        dump_data_size = ram_dump.sizeof('struct msm_dump_data')

        mem_dump_data = ram_dump.addr_lookup('memdump')

        mem_dump_table = ram_dump.read_word(
            mem_dump_data + dump_table_ptr_offset)

        mem_table_version = ram_dump.read_u32(
            mem_dump_table + dump_table_version_offset)
        if mem_table_version is None:
            print_out_str('Version is bogus! Can\'t parse debug image')
            return
        mem_table_num_entry = ram_dump.read_u32(
            mem_dump_table + dump_table_num_entry_offset)
        if mem_table_num_entry is None or mem_table_num_entry > 100:
            print_out_str('num_entries is bogus! Can\'t parse debug image')
            return

        print_out_str('\nDebug image version: {0}.{1} Number of table entries {2}'.format(
            mem_table_version >> 20, mem_table_version & 0xFFFFF, mem_table_num_entry))
        print_out_str('--------')

        for i in range(0, mem_table_num_entry):
            this_entry = mem_dump_table + dump_table_entry_offset + \
                i * dump_entry_size
            entry_id = ram_dump.read_u32(this_entry + dump_entry_id_offset)
            entry_type = ram_dump.read_u32(this_entry + dump_entry_type_offset)
            entry_addr = ram_dump.read_word(this_entry + dump_entry_addr_offset)

            if entry_id < 0 or entry_id > len(self.dump_table_id_lookup_table):
                print_out_str(
                    '!!! Invalid dump table entry id found {0:x}'.format(entry_id))
                continue

            if entry_type > len(self.dump_type_lookup_table):
                print_out_str(
                    '!!! Invalid dump table entry type found {0:x}'.format(entry_type))
                continue

            table_version = ram_dump.read_u32(
                entry_addr + dump_table_version_offset, False)
            if table_version is None:
                print_out_str('Dump table entry version is bogus! Can\'t parse debug image')
                return
            table_num_entries = ram_dump.read_u32(
                entry_addr + dump_table_num_entry_offset, False)
            if table_num_entries is None or table_num_entries > 100:
                print_out_str('Dump table entry num_entries is bogus! Can\'t parse debug image')
                return

            print_out_str(
                'Debug image version: {0}.{1} Entry id: {2} Entry type: {3} Number of entries: {4}'.format(
                    table_version >> 20, table_version & 0xFFFFF, self.dump_table_id_lookup_table[entry_id],
                    self.dump_type_lookup_table[entry_type], table_num_entries))

            for j in range(0, table_num_entries):
                print_out_str('--------')
                client_entry = entry_addr + dump_table_entry_offset + j * dump_entry_size
                client_id = ram_dump.read_u32(client_entry + dump_entry_id_offset, False)
                client_type =  ram_dump.read_u32(client_entry + dump_entry_type_offset, False)
                client_addr = ram_dump.read_word(client_entry + dump_entry_addr_offset, False)

                if client_id < 0 or client_id > len(self.dump_data_id_lookup_table):
                    print_out_str(
                        '!!! Invalid dump client id found {0:x}'.format(client_id))
                    continue

                if client_type > len(self.dump_type_lookup_table):
                    print_out_str(
                        '!!! Invalid dump client type found {0:x}'.format(client_type))
                    continue

                dump_data_magic = ram_dump.read_u32(client_addr + dump_data_magic_offset, False)
                dump_data_version = ram_dump.read_u32(client_addr + dump_data_version_offset, False)
                dump_data_addr = ram_dump.read_dword(client_addr + dump_data_addr_offset, False)
                dump_data_len = ram_dump.read_dword(client_addr + dump_data_len_offset, False)

                client_name = self.dump_data_id_lookup_table[client_id]
                if client_name not in client_table:
                    print_out_str(
                         '!!! {0} Does not have an associated function. The parser needs to be updated!'.format(client_name))
                else:
                    print_out_str('Parsing debug information for {0}. Version: {1} Magic: {2:x}'.format(
                       client_name, dump_data_version, dump_data_magic))

                    if dump_data_magic is None:
                        print_out_str(
                            "!!! Address {0:x} is bogus! Can't parse!".format(start))
                        continue

                    if dump_data_magic != MEMDUMPV2_MAGIC:
                        print_out_str(
                        "!!! Magic {0:x} doesn't match! No context will be parsed".format(dump_data_magic))
                        continue

                    func = client_table[client_name]
                    getattr(DebugImage_v2, func)(self, dump_data_addr, dump_data_addr + dump_data_len,
                                                 client_id, ram_dump)

            self.qdss.dump_all(ram_dump)
            if ram_dump.qtf:
                self.parse_qtf(ram_dump)
