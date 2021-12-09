#!/usr/bin/env python2

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

# this script requires python2. However, we'd like to be able to print
# an informative message to a user who might be unknowingly running
# python3 so we can't allow any python2 print statements to sneak in
# since they result in syntax errors in python3. By importing
# print_function we are requiring ourselves to use the python3 syntax.
from __future__ import print_function

import sys
import os
import os.path as path
import struct
import re
import time
import subprocess
from optparse import OptionParser, OptionValueError

import parser_util
from ramdump import RamDump
from print_out import print_out_str, set_outfile, print_out_section, print_out_exception, flush_outfile

import gdbmi

# Please update version when something is changed!'
VERSION = '2.0.27'

# quick check of system requirements:
major, minor = sys.version_info[:2]
if major != 2:
    print("This script requires python2 to run!\n")
    print("You seem to be running: " + sys.version)
    print()
    sys.exit(1)
if minor != 7 and '--force-26' not in sys.argv:
    from textwrap import dedent
    print(dedent("""
    WARNING! This script is developed and tested with Python 2.7.
    You might be able to get things working on 2.6 by installing
    a few dependencies (most notably, OrderedDict [1])
    and then passing --force-26 to bypass this version check, but
    the recommended and supported approach is to install python2.7.

    If you already have python2.7 installed but it's not the default
    python2 interpreter on your system (e.g. if python2 points to
    python2.6) then you'll need to invoke the scripts with python2.7
    explicitly, for example:

        $ python2.7 $(which ramparse.py) ...

    instead of:

        $ ramparse.py ...

    [1] https://pypi.python.org/pypi/ordereddict"""))
    sys.exit(1)
if '--force-26' in sys.argv:
    sys.argv.remove('--force-26')

def q6_wlanfw_get_address_of_symbol(wlanfw):
    if not options.gdb:
        print_out_str("--gdb-path not specified")
        sys.exit(1)
    wlanfwelf = gdbmi.GdbMI(options.gdb, wlanfw)
    wlanfwelf.open()

    paddr = wlanfwelf.address_of('g_plat_ctxt.qdss_trace_ctrl.qdss_mem_info.mem_seg[0].addr')
    psize = wlanfwelf.address_of('g_plat_ctxt.qdss_trace_ctrl.qdss_mem_info.mem_seg[0].size')
    return (paddr, psize)

def elf_get_pheaders(fd):
    e_ident = fd.read(8)
    e_magic, e_class, e_data, e_version, e_osabi = struct.unpack("<IBBBB", e_ident)

    if e_magic != 0x464c457f and e_class != 1 and e_data != 1:
        print_out_str("!!!ELF magic not matched or format not supported")
        return None

    # e_phoff - Points to the start of the program header table.
    fd.seek(0x1C)
    e_phoff, = struct.unpack("<I", fd.read(4))

    # e_phentsize - Contains the size of a program header table entry.
    fd.seek(0x2A)
    e_phentsize, = struct.unpack("<H", fd.read(2))

    # e_phnum - Contains the number of entries in the program header table.
    fd.seek(0x2C)
    e_phnum, = struct.unpack("<H", fd.read(2))

    fd.seek(e_phoff)
    count = 0
    elf_pheaders = []
    while count < e_phnum:
        curr_offset = e_phoff + count * e_phentsize

        # p_offset - Offset of the segment in the file image.
        fd.seek(curr_offset + 0x04)
        p_offset, = struct.unpack("<I", fd.read(4))

        # p_paddr - Reserved for segment's physical address.
        fd.seek(curr_offset + 0x0C)
        p_paddr, = struct.unpack("<I", fd.read(4))

        # p_filesz - Size in bytes of the segment in the file image.
        fd.seek(curr_offset + 0x10)
        p_filesz, = struct.unpack("<I", fd.read(4))

        elf_pheaders.append((p_paddr, p_filesz, p_offset))
        count += 1

    return elf_pheaders

# Supported for 32-bit ELF, Little-Endian format only
def q6_etr_from_q6mem_elf(fd, paddr, psize, outdir):
    elf_pheaders = elf_get_pheaders(fd)

    for p_paddr, p_filesz, p_offset in elf_pheaders:
        p_end = p_paddr + p_filesz
        if paddr >= p_paddr and paddr <= p_end and psize >= p_paddr and psize <= p_end:
            fd.seek(paddr - p_paddr + p_offset)
            etr_addr, = struct.unpack("<Q", fd.read(8))

            fd.seek(psize - p_paddr + p_offset)
            etr_size, = struct.unpack("<I", fd.read(4))
            break

    for p_paddr, p_filesz, p_offset in elf_pheaders:
        if p_paddr == etr_addr and p_filesz == etr_size:
            break

    if p_paddr != etr_addr and p_filesz != etr_size:
        print_out_str('etr region not found in the program header')
        return None

    offset = p_offset
    start = p_paddr - offset
    end = start + p_filesz

    try:
        fd.seek(offset)
        dump = fd.read(etr_size)
        #Look for dump Magic, 0xdeadbeef in Little Endian
        head_pos = dump.find(b'\xef\xbe\xad\xde')
        if head_pos < 0:
            print_out_str("-- Magic '0xdeadbeef' is not found in range")
            return None

        fd.seek(head_pos + offset)
        magic, status, read_ptr, write_ptr = struct.unpack("<IIII", fd.read(16))

        etr_file_path = os.path.join(outdir, "q6_etr.bin")
        if os.path.exists(etr_file_path):
            try:
                os.remove(etr_file_path)
            except:
                print_out_str("!!! Cannot delete old etr dump")
        try:
            #Write etr dump to a file
            with open(etr_file_path, 'ab') as etr_file:
                etr_file.truncate(0)
                if read_ptr == write_ptr and status & 0x10000 == 1:
                    print_out_str("-- etr buffer is empty")
                elif read_ptr < write_ptr:
                    fd.seek(read_ptr - start)
                    etr_file.write(fd.read(write_ptr - read_ptr))
                elif read_ptr > write_ptr or (read_ptr == write_ptr and status & 0x1 == 1):
                    fd.seek(read_ptr - start)
                    etr_file.write(fd.read(end - read_ptr))
                    fd.seek(offset)
                    etr_file.write(fd.read(write_ptr - etr_addr))
            print_out_str("etr binary generated at " + etr_file_path)
            print("etr binary generated at " + etr_file_path)
            return True
        except:
            print_out_str("!!! Cannot write etr dump to output file")
            return None
    except:
        print_out_str("!!! File operation failed..")

    return None

def parse_etr_option(option, opt_str, value, parser):
    a = getattr(parser.values, option.dest)
    temp = []

    for arg in parser.rargs:
        if arg[:2] == '--':
            break
        if arg[:1] == '-' and len(arg) > 1:
            break
        temp.append(arg)

    if len(temp) is 2:
	a = []
	a.append((temp[0], temp[1]))
	setattr(parser.values, option.dest, a)
    elif len(temp) is 0:
	a = "EBICS"
	setattr(parser.values, option.dest, a)
    else:
        raise OptionValueError("--dump_q6_etr option should have either no argument or if specified, in 'q6mem-path, wlanfw.elf-path' format")

def parse_ram_file(option, opt_str, value, parser):
    a = getattr(parser.values, option.dest)
    if a is None:
        a = []
    temp = []
    for arg in parser.rargs:
        if arg[:2] == '--':
            break
        if arg[:1] == '-' and len(arg) > 1:
            break
        temp.append(arg)

    if len(temp) is not 3:
        raise OptionValueError(
            "Ram files must be specified in 'name, start, end' format")

    a.append((temp[0], int(temp[1], 16), int(temp[2], 16)))
    setattr(parser.values, option.dest, a)

if __name__ == '__main__':
    usage = 'usage: %prog [options to print]. Run with --help for more details'
    parser = OptionParser(usage)
    parser.add_option('', '--print-watchdog-time', action='store_true',
                      dest='watchdog_time', help='Print watchdog timing information', default=False)
    parser.add_option('-e', '--ram-file', dest='ram_addr',
                      help='List of ram files (name, start, end)', action='callback', callback=parse_ram_file)
    parser.add_option('-v', '--vmlinux', dest='vmlinux', help='vmlinux path')
    parser.add_option('-n', '--nm-path', dest='nm', help='nm path')
    parser.add_option('-g', '--gdb-path', dest='gdb', help='gdb path')
    parser.add_option('-j', '--objdump-path', dest='objdump', help='objdump path')
    parser.add_option('-a', '--auto-dump', dest='autodump',
                      help='Auto find ram dumps from the path')
    parser.add_option('-o', '--outdir', dest='outdir', help='Output directory')
    parser.add_option('-s', '--t32launcher', action='store_true',
                      dest='t32launcher', help='Create T32 simulator launcher', default=False)
    parser.add_option('-x', '--everything', action='store_true',
                      dest='everything', help='Output everything (may be slow')
    parser.add_option('-f', '--output-file', dest='outfile',
                      help='Name of file to save output')
    parser.add_option('', '--stdout', action='store_true',
                      dest='stdout', help='Dump to stdout instead of the file')
    parser.add_option('', '--phys-offset', type='int',
                      dest='phys_offset', help='use custom phys offset')
    parser.add_option('', '--page-offset', type='int',
                      dest='page_offset', help='use custom page offset')
    parser.add_option('', '--force-hardware', type='int',
                      dest='force_hardware', help='Force the hardware detection')
    parser.add_option(
        '', '--force-version', type='int', dest='force_hardware_version',
        help='Force the hardware detection to a specific hardware version')
    parser.add_option('', '--parse-qdss', action='store_true',
                      dest='qdss', help='Parse QDSS (deprecated)')
    parser.add_option('', '--64-bit', action='store_true', dest='arm64',
                      help='Parse dumps as 64-bit dumps')
    parser.add_option('', '--shell', action='store_true',
                      help='Run an interactive python interpreter with the ramdump loaded')
    parser.add_option('', '--classic-shell', action='store_true',
                      help='Like --shell, but forces the use of the classic python shell, even if ipython is installed')
    parser.add_option('', '--qtf', action='store_true', dest='qtf',
                      help='Use QTF tool to parse and save QDSS trace data')
    parser.add_option('', '--qtf-path', dest='qtf_path',
                      help='QTF tool executable')
    parser.add_option('', '--cpu0-reg-path', dest='cpu0_reg_path',
                      help='CPU 0 Registers')
    parser.add_option('', '--cpu1-reg-path', dest='cpu1_reg_path',
                      help='CPU 1 Registers')
    parser.add_option('', '--ko-path', dest='ko_path',
                      help='*.ko files path')
    parser.add_option('', '--readelf-path', dest='readelf_path',
                      help='readelf path')
    parser.add_option('', '--custom', dest='custom',
                      help='custom specific issue')
    parser.add_option('', '--dump_dts', action='store_true',
                      dest='dump_dts', help='Dump the Device Tree Blob and Source', default=False)
    parser.add_option('', '--dump_q6_etr', action='callback',
                      dest='dump_q6_etr', help='etr region (q6mem dump path, wlanfw elf path) to extract etr dump q6_etr.bin',
                      callback=parse_etr_option, default=False)
    parser.add_option('', '--parse-rddm', action='store_true',
                      dest='rddm', help='Extract RDDM dumps')
    parser.add_option('', '--ath11k', action='store_true', dest='ath11k', help='ath11k specific parse')
    parser.add_option('', '--console-log', dest='console_log', help='parse console logs to extract functions and modules')

    for p in parser_util.get_parsers():
        parser.add_option(p.shortopt or '',
                          p.longopt,
                          dest=p.cls.__name__,
                          help=p.desc,
                          action='store_true')

    (options, args) = parser.parse_args()

    if options.outdir:
        if not os.path.exists(options.outdir):
            print ('!!! Out directory does not exist. Creating...')
            try:
                os.makedirs(options.outdir)
            except:
                print ("Failed to create %s. You probably don't have permissions there. Bailing." % options.outdir)
                sys.exit(1)
    else:
        options.outdir = '.'

    if options.outfile is None:
        # dmesg_TZ is a very non-descriptive name and should be changed
        # sometime in the future
        options.outfile = 'dmesg_TZ.txt'

    if not options.stdout:
        set_outfile(options.outdir + '/' + options.outfile)

    print_out_str('Linux Ram Dump Parser Version %s' % VERSION)

    if options.dump_q6_etr and options.dump_q6_etr != "EBICS":
        fd = None
        etr, = options.dump_q6_etr
        print_out_str('\n--------- begin q6_etr extraction (q6mem)---------')
        if os.path.isfile(etr[0]) is True and os.path.isfile(etr[1]) is True:
            try:
                fd = open(etr[0], 'rb')
            except:
                fd.close()
                print_out_str("!!! File {0} cannot be opened".format(etr[0]))
                sys.exit(1)

            paddr, psize = q6_wlanfw_get_address_of_symbol(etr[1])
            q6_etr_from_q6mem_elf(fd, paddr, psize, options.outdir)
        print_out_str('--------- end q6_etr extraction ---------')

    args = ''
    for arg in sys.argv:
        args = args + arg + ' '

    print_out_str('Arguments: {0}'.format(args))

    system_type = parser_util.get_system_type()

    if options.autodump is not None:
        if os.path.exists(options.autodump):
            print_out_str(
                'Looking for Ram dumps in {0}'.format(options.autodump))
        else:
            print_out_str(
                'Path {0} does not exist for Ram dumps. Exiting...'.format(options.autodump))
            sys.exit(1)

    if options.vmlinux is None:
        if options.autodump is None:
            print_out_str("No vmlinux or autodump dir given. I can't proceed!")
            if options.dump_q6_etr is None or options.dump_q6_etr == "EBICS":
                parser.print_usage()
            sys.exit(1)
        autovm = os.path.join(options.autodump, 'vmlinux')
        if os.path.isfile(autovm):
            options.vmlinux = autovm
        else:
            print_out_str("No vmlinux given or found in autodump dir. I can't proceed!")
            parser.print_usage()
            sys.exit(1)

    if not os.path.exists(options.vmlinux):
        print_out_str(
            '{0} does not exist. Cannot proceed without vmlinux. Exiting...'.format(options.vmlinux))
        sys.exit(1)
    elif not os.path.isfile(options.vmlinux):
        print_out_str(
            '{0} is not a file. Did you pass the ram file directory instead of the vmlinux?'.format(options.vmlinux))
        sys.exit(1)

    print_out_str('using vmlinux file {0}'.format(options.vmlinux))

    if options.ram_addr is None and options.autodump is None:
        print_out_str('Need one of --auto-dump or at least one --ram-file')
        sys.exit(1)

    if options.ram_addr is not None:
        count = 0
        for a in options.ram_addr:
            if os.path.exists(a[0]):
                dump_file = a[0]
                compressed = dump_file.find('.gz')
                if compressed > 0:
                    a = (dump_file[:-3], a[1], a[2])
                    cmd = ["gzip -cdk " + dump_file + ">" + a[0]]
                    ret = subprocess.call(cmd, shell=True)
                    if ret != 0:
                        print_out_str('Error executing gzip')
                        sys.exit(1)

                    options.ram_addr[count] = (a[0], a[1], a[2])
                    count = count + 1

                print_out_str(
                    'Loading Ram file {0} from {1:x}--{2:x}'.format(a[0], a[1], a[2]))
            else:
                print_out_str(
                    'Ram file {0} does not exist. Exiting...'.format(a[0]))
                sys.exit(1)

    gdb_path = options.gdb
    nm_path = options.nm
    objdump_path = options.objdump

    try:
        import local_settings
        try:
            if options.arm64:
                gdb_path = gdb_path or local_settings.gdb64_path
                nm_path = nm_path or local_settings.nm64_path
                objdump_path = objdump_path or local_settings.objdump64_path
            else:
                gdb_path = gdb_path or local_settings.gdb_path
                nm_path = nm_path or local_settings.nm_path
                objdump_path = objdump_path or local_settings.objdump_path
        except AttributeError as e:
            print_out_str("local_settings.py looks bogus. Please see README.txt")
            missing_attr = re.sub(".*has no attribute '(.*)'", '\\1', e.message)
            print_out_str("Specifically, looks like you're missing `%s'\n" % missing_attr)
            print_out_str("Full message: %s" % e.message)
            sys.exit(1)
    except ImportError:
        cross_compile = os.environ.get('CROSS_COMPILE')
        if cross_compile is not None:
            gdb_path = gdb_path or cross_compile+"gdb"
            nm_path = nm_path or cross_compile+"nm"

    if gdb_path is None or nm_path is None:
        print_out_str("!!! Incorrect path for toolchain specified.")
        print_out_str("!!! Please see the README for instructions on setting up local_settings.py or CROSS_COMPILE")
        sys.exit(1)

    print_out_str("Using gdb path {0}".format(gdb_path))
    print_out_str("Using nm path {0}".format(nm_path))

    if not os.path.exists(gdb_path):
        print_out_str("!!! gdb_path {0} does not exist! Check your settings!".format(gdb_path))
        sys.exit(1)

    if not os.access(gdb_path, os.X_OK):
        print_out_str("!!! No execute permissions on gdb path {0}".format(gdb_path))
        print_out_str("!!! Please check the path settings")
        print_out_str("!!! If this tool is being run from a shared location, contact the maintainer")
        sys.exit(1)

    if not os.path.exists(nm_path):
        print_out_str("!!! nm_path {0} does not exist! Check your settings!".format(nm_path))
        sys.exit(1)

    if not os.access(nm_path, os.X_OK):
        print_out_str("!!! No execute permissions on nm path {0}".format(nm_path))
        print_out_str("!!! Please check the path settings")
        print_out_str("!!! If this tool is being run from a shared location, contact the maintainer")
        sys.exit(1)

    if options.everything:
        options.qtf = True

    if options.ath11k:
        print_out_str("using ath11k module")
        if options.readelf_path is None or  options.ko_path is None:
            print_out_str("!!! Missing --readelf-path or --ko-path option")
            sys.exit(1)

    #path to nss driver
    ko_path = options.ko_path
    readelf_path = options.readelf_path

    dump = RamDump(options.vmlinux, nm_path, gdb_path, readelf_path, ko_path, objdump_path, options.ram_addr,
                   options.autodump, options.phys_offset, options.outdir, options.qtf_path, options.custom,
                   options.cpu0_reg_path, options.cpu1_reg_path,
                   options.force_hardware, options.force_hardware_version,
                   arm64=options.arm64,
                   page_offset=options.page_offset, qtf=options.qtf, ath11k=options.ath11k)

    if options.shell or options.classic_shell:
        print("Entering interactive shell mode.")
        print("The RamDump instance is available in the `dump' variable\n")
        do_fallback = options.classic_shell
        if not do_fallback:
            try:
                from IPython import embed
                embed()
            except ImportError:
                do_fallback = True

        if do_fallback:
            import code
            import readline
            import rlcompleter
            vars = globals()
            vars.update(locals())
            readline.set_completer(rlcompleter.Completer(vars).complete)
            readline.parse_and_bind("tab: complete")
            shell = code.InteractiveConsole(vars)
            shell.interact()
        sys.exit(0)

    if not dump.print_command_line():
        print_out_str('!!! Error printing saved command line.')
        print_out_str('!!! The vmlinux is probably wrong for the ramdumps')
        print_out_str('!!! Exiting now...')
        sys.exit(1)

    if options.dump_dts:
        print_out_str('\n--------- begin dtb extraction ---------')
        dtb = dump.get_dtb()
        if dtb == None:
            print("Cannot locate dtb. Exiting now...")
            print_out_str('Cannot locate dtb ... Exiting now ...')
            sys.exit(1)
        else:
            dtb_file_path = path.join(options.outdir, "devicetree.dtb")
            dts_file_path = path.join(options.outdir, "devicetree.dts")
            try:
                with open(dtb_file_path, "wb") as dtb_file:
                    dtb_file.write(dtb)
            except:
                print_out_str('!!! Error writing dtb to file')

            ret = os.system("dtc -I dtb -O dts -f " + dtb_file_path + " -o " + dts_file_path)
            if ret != 0:
                print("dtc failed with error: " + str(ret) + ". Install dtc and run ")
                print("dtc -I dtb -O dts -f " + dtb_file_path + " -o " + dts_file_path)
        print_out_str('\n--------- end dtb extraction ---------')

    if options.dump_dts and options.dump_q6_etr == "EBICS":
        print_out_str('\n--------- begin q6_etr extraction (EBICS)---------')
        etr_reg = dump.dts_lookup("q6_etr_dump")
        if etr_reg is not None:
            etr_addr = int(etr_reg[1], 16)
            etr_size = int(etr_reg[3], 16)

            etr = dump.get_q6_etr(etr_addr, etr_size)
            if etr is None:
                print_out_str("!!! etr dump not available")
        else:
            print_out_str('!!! etr region not found in device-tree')
        print_out_str('--------- end q6_etr extraction ---------')

    print_out_str('\n--------- begin glink log parsing ---------\n')
    dump.get_glink_logging(options.outdir)
    print_out_str('\n--------- end glink log parsing ---------\n')

    print_out_str('\n--------- begin smp2p log parsing ---------\n')
    dump.get_smp2p_logging(options.outdir)
    print_out_str('\n--------- end smp2p log parsing ---------\n')

    if options.rddm:
        print_out_str('\n--------- begin RDDM extraction ---------\n')
        dump.get_rddm_dump(options.outdir)
        print_out_str('\n--------- end RDDM extraction ---------\n')

    if options.qdss:
        print_out_str('!!! --parse-qdss is now deprecated')
        print_out_str(
            '!!! Please just use --parse-debug-image to get QDSS information')

    if options.watchdog_time:
        print_out_str('\n--------- watchdog time -------')
        get_wdog_timing(dump)
        print_out_str('---------- end watchdog time-----')

    if options.console_log:
        print_out_str('\n--------- begin console log extraction -------')
        dump.extract_modules_from_console(options.console_log)
        print_out_str('--------- end console log extraction -------')

    # we called parser.add_option with dest=p.cls.__name__ above,
    # so if the user passed that option then `options' will have a
    # p.cls.__name__ attribute.
    parsers_to_run = [p for p in parser_util.get_parsers()
                      if getattr(options, p.cls.__name__)
                      or (options.everything and not p.optional)]
    for i,p in enumerate(parsers_to_run):
        if i == 0:
            sys.stderr.write("\n")
        sys.stderr.write("    [%d/%d] %s ... " %
                         (i + 1, len(parsers_to_run), p.longopt))
        before = time.time()
        with print_out_section(p.cls.__name__):
            try:
                p.cls(dump).parse()
            except:
                print_out_str('!!! Exception while running {0}'.format(p.cls.__name__))
                print_out_exception()
                sys.stderr.write("FAILED! ")
        sys.stderr.write("%fs\n" % (time.time() - before))
        sys.stderr.flush()
        flush_outfile()

    sys.stderr.write("\n")

    if options.t32launcher or options.everything:
        dump.create_t32_launcher()
