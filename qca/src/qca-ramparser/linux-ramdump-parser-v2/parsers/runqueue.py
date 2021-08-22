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

import rb_tree
import re
import os
from print_out import print_out_str
from parser_util import register_parser, RamParser


@register_parser('--print-runqueues', 'Print the runqueue status')
class RunQueues(RamParser):

    def __init__(self, *args):
        super(RunQueues, self).__init__(*args)
        self.domain_list = []
        self.tab_offset = 0

    def ffs(self, num):
        # Check there is at least one bit set.
        if num is None or num == 0:
            return None
        # Right-shift until we have the first set bit in the LSB position.
        i = 0
        while (num % 2) == 0:
            i += 1
            num = num >> 1
        return i

    def print_out_str_with_tab(self, string):
        string = self.tab_offset * ' |  ' + ' |--' + string
        print_out_str(string)

    def print_task_state(self, status, task_addr):
        pid_offset = self.ramdump.field_offset('struct task_struct', 'pid')
        comm_offset = self.ramdump.field_offset('struct task_struct', 'comm')

        if 0 < task_addr:
            pid = self.ramdump.read_int(task_addr + pid_offset)
            taskname = self.ramdump.read_cstring(task_addr + comm_offset, 16)
            self.print_out_str_with_tab(
                '{0}: {1}({2})'.format(status, taskname, pid))
        else:
            self.print_out_str_with_tab('{0}: None(0)'.format(status))

    def print_cgroup_state(self, status, se_addr):
        se_offset = self.ramdump.field_offset('struct task_struct', 'se')
        cfs_nr_running_offset = self.ramdump.field_offset(
            'struct cfs_rq', 'nr_running')
        if cfs_nr_running_offset < 0:
            print_out_str("cfs_rq not available")
            return

        my_q_offset = self.ramdump.field_offset('struct sched_entity', 'my_q')

        if se_addr == 0 or se_addr is None or my_q_offset == 0 or my_q_offset is None:
            self.print_task_state(status, se_addr)
        else:
            my_q_addr = self.ramdump.read_word(se_addr + my_q_offset)
            if my_q_addr == 0:
                self.print_task_state(status, se_addr - se_offset)
            else:
                cfs_nr_running = self.ramdump.read_int(
                    my_q_addr + cfs_nr_running_offset)
                self.print_out_str_with_tab(
                    '{0}: {1} process is grouping'.format(status, cfs_nr_running))
                self.tab_offset += 1
                self.print_cfs_state(my_q_addr)
                self.tab_offset -= 1

    def cfs_node_func(self, node, extra):
        run_node_offset = self.ramdump.field_offset(
            'struct sched_entity', 'run_node')

        task_se = node - run_node_offset
        self.print_cgroup_state('pend', task_se)

    def print_cfs_state(self, cfs_rq_addr):
        tasks_timeline_offset = self.ramdump.field_offset(
            'struct cfs_rq', 'tasks_timeline')
        curr_offset = self.ramdump.field_offset('struct cfs_rq', 'curr')
        next_offset = self.ramdump.field_offset('struct cfs_rq', 'next')
        last_offset = self.ramdump.field_offset('struct cfs_rq', 'last')
        skip_offset = self.ramdump.field_offset('struct cfs_rq', 'skip')

        tasks_timeline_addr = self.ramdump.read_word(
            cfs_rq_addr + tasks_timeline_offset)

        curr_se = self.ramdump.read_word(cfs_rq_addr + curr_offset)
        self.print_cgroup_state('curr', curr_se)
        next_se = self.ramdump.read_word(cfs_rq_addr + next_offset)
        self.print_cgroup_state('next', next_se)
        last_se = self.ramdump.read_word(cfs_rq_addr + last_offset)
        self.print_cgroup_state('last', last_se)
        skip_se = self.ramdump.read_word(cfs_rq_addr + skip_offset)
        self.print_cgroup_state('skip', skip_se)

        rb_walker = rb_tree.RbTreeWalker(self.ramdump)
        rb_walker.walk(tasks_timeline_addr, self.cfs_node_func)

    def print_rt_cgroup_state(self, status, rt_addr):
        rt_offset = self.ramdump.field_offset('struct task_struct', 'rt')
        rt_nr_running_offset = self.ramdump.field_offset(
            'struct rt_rq', 'nr_running')
        my_q_offset = self.ramdump.field_offset(
            'struct sched_rt_entity', 'my_q')

        if rt_addr == 0:
            self.print_task_state(status, se_addr)
        else:
            my_q_addr = self.ramdump.read_word(rt_addr + my_q_offset)
            if my_q_addr == 0:
                self.print_task_state(status, rt_addr - rt_offset)
            else:
                rt_nr_running = self.ramdump.read_word(
                    my_q_addr + rt_nr_running_offset)
                self.print_out_str_with_tab(
                    '{0}: {1} process is grouping'.format(status, rt_nr_running))
                self.tab_offset += 1
                self.print_rt_state(my_q_addr)
                self.tab_offset -= 1

    def print_rt_state(self, rt_rq_addr):
        active_offset = self.ramdump.field_offset('struct rt_rq', 'active')
        queue_offset = self.ramdump.field_offset(
            'struct rt_prio_array', 'queue')
        if queue_offset < 0:
            print_out_str("rt_prio_array not available")
            return

        rt_offset = self.ramdump.field_offset('struct task_struct', 'rt')

        array_addr = rt_rq_addr + active_offset

        if self.ramdump.arm64:
            bitmap_range = 2
            array_wsize = 8
            idx_size = 64
        else:
            bitmap_range = 4
            array_wsize = 4
            idx_size = 32

        seen_nodes = set()
        for i in range(0, bitmap_range):
            bitmap = self.ramdump.read_word(array_addr + i * array_wsize)
            while True:
                idx = self.ffs(bitmap)
                if idx is not None and idx + i * 32 < 100:
                    bitmap &= ~(1 << idx)
                    idx = (idx + i * 32) * 4 * 2
                    queue_addr = self.ramdump.read_word(
                        array_addr + queue_offset + idx)
                    while queue_addr != array_addr + queue_offset + idx:
                        if queue_addr in seen_nodes:
                            break
                        seen_nodes.add(queue_addr)
                        task_addr = queue_addr - rt_offset
                        self.print_task_state('pend', task_addr)
                        queue_addr = self.ramdump.read_word(queue_addr)
                else:
                    break

    def print_latest_callstack_maybe(self, task_addr):
        text_start_addr = self.ramdump.addr_lookup('_text')
        text_end_addr = self.ramdump.addr_lookup('_etext')
        stack_offset = self.ramdump.field_offset('struct task_struct', 'stack')
        if(task_addr is None):
            return
        stack_addr = self.ramdump.read_word(task_addr + stack_offset)
        print_out_str('current callstack is maybe:')
        if (self.ramdump.isELF64()):
            incr = 8
        else:
            incr = 4
        for i in range(stack_addr, stack_addr + self.ramdump.thread_size, incr):
            callstack_addr = self.ramdump.read_word(i)
            if (text_start_addr <= callstack_addr < text_end_addr) or \
               (0xbf000000 <= callstack_addr < 0xbfe00000) or (self.ramdump.isELF64() and (0xffffffbffc000000 <= callstack_addr < 0xffffffbffe000000)):
                modname = None
                symname = None
                symtab_st_size = None
                wname = self.ramdump.unwind_lookup(callstack_addr)
                if wname is not None and len(wname) > 3:
                    symname, offset, modname, symtab_st_size = wname
                else:
                    symtab_st_size = None
                    symname = 'UNKNOWN'
                    offset = 0

                if (modname is not None and symtab_st_size is not None):
                    print_out_str('<0x{0:x}>:{1}+0x{2:x}/0x{5:x} [0x{3:x}] <{4}.ko>'.format(i, symname, offset, callstack_addr, modname, symtab_st_size))
                elif (modname is None and symtab_st_size is not None):
                    print_out_str('<0x{0:x}>:{1}+0x{2:x}/0x{4:x} [0x{3:x}]'.format(i, symname, offset, callstack_addr, symtab_st_size))
                else:
                    print_out_str('<0x{0:x}>:{1} [0x{2:x}]'.format(i, symname, callstack_addr))

    def stack_dump_trace(self, svc_r13_core):
        stext = self.ramdump.addr_lookup('_stext')
        etext = self.ramdump.addr_lookup('_etext')
        stack_offset = self.ramdump.field_offset('struct task_struct', 'stack')
        if(self.ramdump.isELF64()):
            b = 0xffffffb000000000
            c = 0xffffffc000000000
            d = 0xffffffd000000000
        else:
            b = 0xb0000000
            c = 0xc0000000
            d = 0xd0000000
        svc_r13_core = int(svc_r13_core, 16)
        if (self.ramdump.isELF64()):
            incr = 8
        else:
            incr = 4
        for i in range(svc_r13_core, svc_r13_core + self.ramdump.thread_size, incr):
             stack_addr = self.ramdump.read_word(i)
             if stack_addr is None:
                 continue
             if not ((stack_addr & b) == b or (stack_addr & c) == c):
                 continue
             if ((stack_addr & d) == d):
                 continue
             if ((stack_addr & c) == c):
                 if not (stext <= stack_addr < etext):
                     continue
             if (self.ramdump.isELF64() and ((stack_addr & b) == b)):
                 if not (0xffffffbffc000000 <= stack_addr < 0xffffffbffe000000):
                     continue

             elif ((stack_addr & b) == b):
                 if not (0xbf000000 <= stack_addr < 0xbfe00000):
                     continue

             modname = None
             symname = None
             symtab_st_size = None
             offset = None
             wname = self.ramdump.unwind_lookup(stack_addr)
             if wname is not None and len(wname) > 3:
                 symname, offset, modname, symtab_st_size = wname
             else:
                 symname = 'UNKNOWN'
                 offset = 0
                 symtab_st_size = None

             if (modname is not None and symtab_st_size is not None):
                 print_out_str('<0x{0:x}>:{1}+0x{2:x}/0x{5:x} [0x{3:x}] <{4}.ko>'.format(i, symname, offset, stack_addr, modname, symtab_st_size))
             elif (modname is None and symtab_st_size is not None):
                 print_out_str('<0x{0:x}>:{1}+0x{2:x}/0x{4:x} [0x{3:x}]'.format(i, symname, offset, stack_addr, symtab_st_size))
             else:
                 print_out_str('<0x{0:x}>:{1} [0x{2:x}]'.format(i, symname, stack_addr))

    def get_stack_pointer(self, reg_fname):
        if (self.ramdump.Is_Hawkeye() and self.ramdump.isELF32()):
            pattern=re.compile(r'.x19')
        elif (self.ramdump.Is_Hawkeye() and self.ramdump.isELF64()):
            pattern=re.compile(r'.sp_el1')
        else:
            pattern=re.compile(r'.*r.s r13_svc*')
        ver_str = None
        fhandle = open(reg_fname, 'rb')
        for line in fhandle:
            found=pattern.findall(line)
            for a in found:
                start_pos = line.index("0")
                if (self.ramdump.Is_Hawkeye()):
                    end_pos = start_pos + 18
                else:
                    end_pos = start_pos + 10
                ver_str = line[start_pos:end_pos]
        return ver_str

    def parse(self):
        print_out_str(
            '======================= RUNQUEUE STATE ============================')
        runqueues_addr = self.ramdump.addr_lookup('runqueues')
        nr_running_offset = self.ramdump.field_offset(
            'struct rq', 'nr_running')
        curr_offset = self.ramdump.field_offset('struct rq', 'curr')
        idle_offset = self.ramdump.field_offset('struct rq', 'idle')
        stop_offset = self.ramdump.field_offset('struct rq', 'stop')
        cfs_rq_offset = self.ramdump.field_offset('struct rq', 'cfs')
        rt_rq_offset = self.ramdump.field_offset('struct rq', 'rt')
        cfs_nr_running_offset = self.ramdump.field_offset(
            'struct cfs_rq', 'nr_running')
        rt_nr_running_offset = self.ramdump.field_offset(
            'struct rt_rq', 'rt_nr_running')

        svc_r13_core0 = None
        svc_r13_core1 = None
        if os.path.exists(self.ramdump.outdir + '/core0_regs.cmm'):
            svc_r13_core0 = self.get_stack_pointer(self.ramdump.outdir + '/core0_regs.cmm')

        if os.path.exists(self.ramdump.outdir + '/core1_regs.cmm'):
            svc_r13_core1 = self.get_stack_pointer(self.ramdump.outdir + '/core1_regs.cmm')

        if self.ramdump.Is_Dakota() or self.ramdump.Is_Hawkeye():
            svc_r13_core2 = None
            svc_r13_core3 = None
            if os.path.exists(self.ramdump.outdir + '/core2_regs.cmm'):
                svc_r13_core2 = self.get_stack_pointer(self.ramdump.outdir + '/core2_regs.cmm')
            if os.path.exists(self.ramdump.outdir + '/core3_regs.cmm'):
                svc_r13_core3 = self.get_stack_pointer(self.ramdump.outdir + '/core3_regs.cmm')
        print_out_str('\n')

        for i in self.ramdump.iter_cpus():
            rq_addr = runqueues_addr + self.ramdump.per_cpu_offset(i)
            nr_running = self.ramdump.read_int(rq_addr + nr_running_offset)
            print_out_str(
                'CPU{0} {1} process is running'.format(i, nr_running))
            curr_addr = self.ramdump.read_word(rq_addr + curr_offset)
            self.print_task_state('curr', curr_addr)
            idle_addr = self.ramdump.read_word(rq_addr + idle_offset)
            self.print_task_state('idle', idle_addr)
            stop_addr = self.ramdump.read_word(rq_addr + stop_offset)
            self.print_task_state('stop', stop_addr)

            cfs_rq_addr = rq_addr + cfs_rq_offset
            cfs_nr_running = self.ramdump.read_int(
                cfs_rq_addr + cfs_nr_running_offset)
            print_out_str('CFS {0} process is pending'.format(cfs_nr_running))
            self.print_cfs_state(cfs_rq_addr)

            rt_rq_addr = rq_addr + rt_rq_offset
            rt_nr_running = self.ramdump.read_int(
                rt_rq_addr + rt_nr_running_offset)
            print_out_str('RT {0} process is pending'.format(rt_nr_running))
            self.print_rt_state(rt_rq_addr)

            if i is 0:
                if svc_r13_core0 is not None:
                    print_out_str('\n Stack dump back trace for core0\n')
                    self.stack_dump_trace(svc_r13_core0)
                else:
                    self.print_latest_callstack_maybe(curr_addr)
            if i is 1:
                if svc_r13_core1 is not None:
                    print_out_str('\n Stack dump back trace for core1\n')
                    self.stack_dump_trace(svc_r13_core1)
                else:
                    self.print_latest_callstack_maybe(curr_addr)

            if self.ramdump.Is_Dakota() or self.ramdump.Is_Hawkeye():
                if i is 2:
                    if svc_r13_core2 is not None:
                        print_out_str('\n Stack dump back trace for core2\n')
                        self.stack_dump_trace(svc_r13_core2)
                    else:
                        self.print_latest_callstack_maybe(curr_addr)

                if i is 3:
                    if svc_r13_core3 is not None:
                        print_out_str('\n Stack dump back trace for core3\n')
                        self.stack_dump_trace(svc_r13_core3)
                    else:
                        self.print_latest_callstack_maybe(curr_addr)

            print_out_str('')
