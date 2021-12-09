# Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import string
import re
from print_out import print_out_str
from parser_util import register_parser, RamParser, cleanupString

def find_panic(ramdump, addr_stack, thread_task_name):
    if ramdump.arm64:
        increment = 8
    else:
        increment = 4
    for i in range(addr_stack, addr_stack + ramdump.thread_size, increment):
        if ramdump.arm64:
            pc = ramdump.read_word(i + 8) - 4
            fp = ramdump.read_word(i)
            spx = i + 16
            lr = 0
        else:
            pc = ramdump.read_word(i)
            lr = ramdump.read_word(i + 4)
            spx = i + 4
            fp = 0

        l = ramdump.unwind_lookup(pc,0,0)
        if l is not None and len(l) >= 3:
            s, offset, foo = l[:3]
            if s == 'panic':
                print_out_str('Faulting process found! Name {0})'.format(thread_task_name))
                ramdump.unwind.unwind_backtrace(spx, fp, pc, lr, '')
                regspanic = ramdump.open_file('regs_panic.cmm')
                if ramdump.arm64:
                    regspanic.write('r.s pc 0x{0:x}\n'.format(pc))
                    regspanic.write('r.s sp 0x{0:x}\n'.format(spx))
                else:
                    regspanic.write('r.s pc 0x{0:x}\n'.format(pc))
                    regspanic.write('r.s r13 0x{0:x}\n'.format(i + 4))
                regspanic.close()
                return True
    return False


def dump_thread_group(ramdump, thread_group, task_out, check_for_panic=0):
    PREEMPT_BITS = 8
    SOFTIRQ_BITS = 8
    PREEMPT_ACTIVE_MASK = 0x10000000
    PREEMPT_MASK = 0x000000ff
    SOFTIRQ_MASK = 0x0000ff00
    if re.search('3.4.\d', ramdump.version) is not None:
        HARDIRQ_MASK = 0x03ff0000
    else:
        HARDIRQ_MASK = 0x000f0000

    PREEMPT_SHIFT = 0
    SOFTIRQ_SHIFT = (PREEMPT_SHIFT + PREEMPT_BITS)
    HARDIRQ_SHIFT = (SOFTIRQ_SHIFT + SOFTIRQ_BITS)
    PREEMPT_ACTIVE_SHIFT = (HARDIRQ_SHIFT + SOFTIRQ_BITS + 4)
    offset_thread_group = ramdump.field_offset(
        'struct task_struct', 'thread_group')
    offset_comm = ramdump.field_offset('struct task_struct', 'comm')
    offset_pid = ramdump.field_offset('struct task_struct', 'pid')
    offset_stack = ramdump.field_offset('struct task_struct', 'stack')
    offset_state = ramdump.field_offset('struct task_struct', 'state')
    offset_exit_state = ramdump.field_offset(
        'struct task_struct', 'exit_state')
    offset_cpu = ramdump.field_offset('struct thread_info', 'cpu')
    task_flag = 0
    if offset_cpu is None:
        offset_cpu = ramdump.field_offset('struct task_struct', 'cpu')
	task_flag = 1
    offset_preempt_count = ramdump.field_offset('struct thread_info', 'preempt_count')
    orig_thread_group = thread_group
    first = 0
    seen_threads = []
    while True:
        next_thread_start = thread_group - offset_thread_group
        next_thread_comm = next_thread_start + offset_comm
        next_thread_pid = next_thread_start + offset_pid
        next_thread_stack = next_thread_start + offset_stack
        next_thread_state = next_thread_start + offset_state
	next_thread_cpu = next_thread_start + offset_cpu
        next_thread_exit_state = next_thread_start + offset_exit_state
        thread_task_name = cleanupString(
            ramdump.read_cstring(next_thread_comm, 16))
        if thread_task_name is None:
            return
        thread_task_pid = ramdump.read_int(next_thread_pid)
        if thread_task_pid is None:
            return
        task_state = ramdump.read_word(next_thread_state)
        if task_state is None:
            return
        task_exit_state = ramdump.read_int(next_thread_exit_state)
        if task_exit_state is None:
            return
        addr_stack = ramdump.read_word(next_thread_stack)
        if addr_stack is None:
            return
        threadinfo = addr_stack
        if threadinfo is None:
            return
	task_cpu = ''
	if task_flag == 0:
	    task_cpu = ramdump.read_int(threadinfo + offset_cpu)
	else:
	    task_cpu = ramdump.read_int(next_thread_cpu)
	if task_cpu is None:
	    return

        if not check_for_panic:
            preempt_cnt = ramdump.read_int(threadinfo + offset_preempt_count)
            if (preempt_cnt):
                cur_preempt_cnt = (preempt_cnt & PREEMPT_MASK)
                cur_softirq_cnt = (preempt_cnt & SOFTIRQ_MASK)
                cur_hardirq_cnt = (preempt_cnt & HARDIRQ_MASK)
                preempt_active = (preempt_cnt & PREEMPT_ACTIVE_MASK)
                if not first:
                    task_out.write('Process: {0}, cpu: {1},  pid: {2}, start: 0x{3:x}\n'.format(
                        thread_task_name, task_cpu, thread_task_pid, next_thread_start))
                    task_out.write(
                        '==============================================================================\n')
                    first = 1
                task_out.write('    Task name: {0}, pid: {1}, cpu: {2}, state: 0x{3:x}  exit_state: 0x{4:x} stack base: 0x{5:x}\n'.format(
                    thread_task_name, thread_task_pid, task_cpu, task_state, task_exit_state, addr_stack))
                task_out.write('    preempt_count: {0},  cur_softirq_cnt = {1}, cur_hardirq_cnt = {2}, preempt_active = {3}\n'.format(
                    cur_preempt_cnt, (cur_softirq_cnt >> SOFTIRQ_SHIFT), (cur_hardirq_cnt >> HARDIRQ_SHIFT), (preempt_active >> PREEMPT_ACTIVE_SHIFT )))
                task_out.write('    Stack:\n')
                ramdump.unwind.unwind_backtrace(
                     ramdump.thread_saved_sp(next_thread_start),
                     ramdump.thread_saved_fp(next_thread_start),
                     ramdump.thread_saved_pc(next_thread_start),
                     0, '    ', task_out)
                task_out.write(
                    '==============================================================================\n')
            else:
                if not first:
                    task_out.write('Process: {0}, cpu: {1},  pid: {2}, start: 0x{3:x}\n'.format(
                        thread_task_name, task_cpu, thread_task_pid, next_thread_start))
                    task_out.write(
                        '==============================================================================\n')
                    first = 1
                task_out.write('    Task name: {0}, pid: {1}, cpu: {2}, state: 0x{3:x}  exit_state: 0x{4:x} stack base: 0x{5:x}\n'.format(
                    thread_task_name, thread_task_pid, task_cpu, task_state, task_exit_state, addr_stack))
                task_out.write('    Stack:\n')
                ramdump.unwind.unwind_backtrace(
                     ramdump.thread_saved_sp(next_thread_start),
                     ramdump.thread_saved_fp(next_thread_start),
                     ramdump.thread_saved_pc(next_thread_start),
                     0, '    ', task_out)
                task_out.write(
                    '==============================================================================\n')
        else:
            find_panic(ramdump, addr_stack, thread_task_name)

        next_thr = ramdump.read_word(thread_group)
        if (next_thr == thread_group) and (next_thr != orig_thread_group):
            if not check_for_panic:
                task_out.write(
                    '!!!! Cycle in thread group! The list is corrupt!\n')
            break
        if (next_thr in seen_threads):
            break

        seen_threads.append(next_thr)
        thread_group = next_thr
        if thread_group == orig_thread_group:
            break


def do_dump_stacks(ramdump, check_for_panic=0):
    offset_tasks = ramdump.field_offset('struct task_struct', 'tasks')
    offset_comm = ramdump.field_offset('struct task_struct', 'comm')
    offset_stack = ramdump.field_offset('struct task_struct', 'stack')
    offset_thread_group = ramdump.field_offset(
        'struct task_struct', 'thread_group')
    offset_pid = ramdump.field_offset('struct task_struct', 'pid')
    offset_state = ramdump.field_offset('struct task_struct', 'state')
    offset_exit_state = ramdump.field_offset(
        'struct task_struct', 'exit_state')
    init_addr = ramdump.addr_lookup('init_task')
    init_next_task = init_addr + offset_tasks
    orig_init_next_task = init_next_task
    init_thread_group = init_addr + offset_thread_group
    seen_tasks = []
    if check_for_panic == 0:
        task_out = ramdump.open_file('tasks.txt')
    else:
        task_out = None
    while True:
        dump_thread_group(ramdump, init_thread_group,
                          task_out, check_for_panic)
        next_task = ramdump.read_word(init_next_task)
        if next_task is None:
            return

        if (next_task == init_next_task) and (next_task != orig_init_next_task):
            if not check_for_panic:
                task_out.write(
                    '!!!! Cycle in task list! The list is corrupt!\n')
            break

        if (next_task in seen_tasks):
            break

        seen_tasks.append(next_task)

        init_next_task = next_task
        init_thread_group = init_next_task - offset_tasks + offset_thread_group
        if init_next_task == orig_init_next_task:
            break
    if check_for_panic == 0:
        task_out.close()
        print_out_str('---wrote tasks to tasks.txt')


@register_parser('--print-tasks', 'Print all the task information', shortopt='-t')
class DumpTasks(RamParser):

    def parse(self):
        do_dump_stacks(self.ramdump, 0)


@register_parser('--check-for-panic', 'Check if a kernel panic occured', shortopt='-p')
class CheckForPanic(RamParser):

    def parse(self):
        if (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (5, 4):
            addr = self.ramdump.addr_lookup('tlv_msg')
            offset_is_panic = self.ramdump.field_offset('struct ctx_save_tlv_msg', 'is_panic')
            result = self.ramdump.read_byte(addr + offset_is_panic)
        else:
            addr = self.ramdump.addr_lookup('in_panic')
            result = self.ramdump.read_word(addr)

        if result == 1:
            print_out_str('-------------------------------------------------')
            print_out_str('[!] KERNEL PANIC detected!')
            print_out_str('-------------------------------------------------')
            do_dump_stacks(self.ramdump, 1)
        else:
            print_out_str('No kernel panic detected')

@register_parser('--check-for-deadlock-using-spinlock', 'check-for-deadlock-using-spinlock', shortopt='-k')
class CheckForDeadlock(RamParser):

    def parse(self):
        if self.ramdump.is_config_defined('CONFIG_DEBUG_SPINLOCK'):
            output = self.get_spin_lock_debug_message(self.ramdump.outdir + '/dmesg.txt')
            if output is not None:
                print_out_str('--------------------------')
                print_out_str('[!] DEADLOCK detected!')
                print_out_str('--------------------------\n')
                print_out_str('{0}'.format(output))

    def get_spin_lock_debug_message(self, fname):
        pattern=re.compile(r'.* lock: *')
        if pattern is None:
            return None
        if pattern is not None:
            ver_str = None
            fhandle = open(fname, 'rb')

            for line in fhandle:
                found=pattern.findall(line)
                for a in found:
                    start_pos = line.index(" lock:")
                    end_pos = line.index(" .owner_cpu:")
                    ver_str = line[start_pos:end_pos+15]
                    return ver_str
        return None

@register_parser('--check-for-register_info_', 'check-for-register_info', shortopt='-l')
class CheckForRegister(RamParser):

    def parse(self):
        if(self.ramdump.arm64 and (self.ramdump.kernel_version[0], self.ramdump.kernel_version[1]) >= (5, 4)):
            print_out_str("Check-for-register info disabled in kernel 5.4 for 64 bit")
            return None
        reg_addr = self.get_pc_register(self.ramdump.outdir + '/dmesg.txt')
        if reg_addr is not None:
            pc_addr, lr_addr = reg_addr
            pc_addr_hex = int(pc_addr, 16)
            lr_addr_hex = int(lr_addr, 16)
            self.parse_reg(pc_addr_hex, 'PC')
            self.parse_reg(lr_addr_hex, 'LR')
        return None

    def get_pc_register(self, fname):
        temp_list = []
        internal_error = 0
        ver_str = None
        ver_str1 = None
        pattern=re.compile(r'.* pc : *')
        if pattern is None:
            return None
        fh = open(fname, 'rb')
        if fh is None:
           return None

        for line in fh:
            if internal_error is 1:
                temp_list.append(line)
                continue
            if re.match('(.*)Internal error:(.*)',line):
                internal_error = 1

        if temp_list:
            for line in temp_list:
                found = pattern.findall(line)
                for a in found:
                    start_pos = line.index(" pc : [<")
                    end_pos = line.index(">]")
                    if(self.ramdump.isELF32()):
                        ver_str = line[start_pos+8:end_pos]
                        ver_str1 = line[start_pos+29:end_pos+21]
                    else:
                        ver_str = line[start_pos+8:end_pos]
                        ver_str1 = line[start_pos+34:end_pos+26]
                    return (ver_str, ver_str1)
        return None

    def parse_reg(self, reg_addr, reg_name):
        if(self.ramdump.isELF32()):
            if(self.ramdump.Is_Alder()):
                c = 0x40000000
                d = 0x50000000
            elif(self.ramdump.Is_Hawkeye()):
                c = 0x80000000
                d = 0x90000000
            else:
                c = 0xc0000000
                d = 0xd0000000
        else:
            c = 0xffffffc000000000
            d = 0xffffffd000000000
        stext = self.ramdump.addr_lookup('_text')
        etext = self.ramdump.addr_lookup('_etext')

        if not ((reg_addr & c) == c or ( reg_addr & d) == d):
            return
        if ((reg_addr & d) == d):
            return
        if not (stext <= reg_addr < etext):
            return
        modname = None
        offset = None
        symtab_st_size = None
        symname = None
        a = self.ramdump.unwind_lookup(reg_addr)
        if a is not None and len(a) > 3:
            symname, offset, modname, symtab_st_size = a
        elif a is not None:
            symname, offset, modname = a
        else:
            symname = 'UNKNOWN'
            offset = 0

        if (modname is not None):
            return

        if (modname is None and symtab_st_size is not None):
            fname = self.ramdump.get_file_name_from_addr(reg_addr)
            if fname is not None:
                print_out_str(
                    '    {4}: 0x{2:x} [{0}+0x{1:x}/0x{5:x}] [{3}]'.format(symname, offset, reg_addr, fname, reg_name, symtab_st_size))
            else:
               print_out_str(
                   '    {3}: 0x{2:x} [{0}+0x{1:x}/0x{4:x}]'.format(symname, offset, reg_addr, reg_name, symtab_st_size))
        return None
