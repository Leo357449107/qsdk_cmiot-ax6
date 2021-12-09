# Copyright (c) 2019 The Linux Foundation. All rights reserved.
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
import ramdump
import collections

symbols = {}

nss_modules = [ "qca_nss_dp", "qca_nss_drv", "qca_nss_qdisc" , "qca_nss_crypto",
                "qca_nss_cfi_ocf", "ecm", "qca_ssdk", "nf_conntrack_ipv4",
                "nf_conntrack", "x_tables", "ip_tables" ]
wifi_modules = [ "qdf", "wifi_3_0", "qca_ol", "umac", "cfg80211",
                 "qca_spectral", "mac80211", "ath11k_pci", "ath11k" ]

def symbol_info_str(addr, function, offset, size, module):
    if addr is None:
        return ""
    if size == 0:
        return " [<{0}>] {1}".format(hex(addr)[2:], hex(addr))
    elif module == None:
        return " [<{0}>] {1}+{2}/{3}".format(hex(addr)[2:], function, hex(offset), hex(size))
    else:
        return " [<{0}>] {1}+{2}/{3} [{4}]".format(hex(addr)[2:], function, hex(offset), hex(size), module)

def gen_symbol_info(nm_path="nm", elf="vmlinux"):
    if len(symbols) != 0:
        return

    symbolfile = elf + ".syms"
    if os.path.exists(symbolfile) == False:
        os.system(nm_path + " --defined-only -l " + elf + " > " + symbolfile)

    if os.path.exists(symbolfile):
        fd = open(symbolfile, "r")
        data = fd.readlines()
        for symbolline in data:
            info = (symbolline.split(" "))[2].split("\t")
            if len(info) == 2 and info[0].find("$") == -1:
                symbols[info[0]] = info[1].strip()

def filename_from_vmlinux(function):
    if function.strip() == "":
        return ""

    if symbols.has_key(function):
        return symbols[function]

    return ""

class meminfo_ranked:
    def __init__(self, ramdump):
        self.ramdump = ramdump
        self.meminfos = {}

    def __str__(self):
        s = ""
        for addr_str in self.meminfos:
            s += str(self.meminfos[addr_str])

        return s

    def insert(self, names, pfns, addrs, size, flags, free=0):
        addr_str = ""
        for addr in addrs:
            if addr is not None:
                addr_str += str(hex(addr)) + " "
        if self.meminfos.has_key(addr_str):
            self.meminfos[addr_str].update_meminfo(names, pfns, addrs, size, flags)
        else:
            mi = meminfo(self.ramdump, names, pfns, addrs, size, flags, free)
            self.meminfos[addr_str] = mi

    def sort_by_size(self):
        meminfo_sorted = collections.OrderedDict(
                           sorted(self.meminfos.items(),
                                  key = lambda page : page[1].total_size,
                                  reverse=True))
        return meminfo_sorted

class meminfo:
    def __init__(self, ramdump, names, pfns, addrs, size, flags, free):
        self.ramdump = ramdump
        if self.ramdump.arm64:
            self.ULONG_MAX = 0xffffffffffffffff
        else:
            self.ULONG_MAX = 0xffffffff
        self.pfns = []
        self.names = []
        self.size = []
        self.total_size = 0
        self.functions = []
        self.modules = []
        self.stack_str = ""
        self.allocation_type = ""
        self.category = ""
        self.subcategory = ""

        self.addrs = addrs
        self.flags = flags
        self.free = free

        self.extract_callstack()
        self.update_meminfo(names, pfns, addrs, size, flags)

        self.classify()

    def extract_callstack(self):
        for addr in self.addrs:
            try:
                stackinfo = self.ramdump.unwind_lookup(addr)
                if stackinfo is None:
                    continue
                function, offset, module, fnsize = stackinfo
            except ValueError:
                function = 'UNKNOWN'
                fnsize = 0
                module = None
                offset = 0
            except TypeError:
                function = 'UNKNOWN'
                fnsize = 0
                module = None
                offset = 0

            self.stack_str += symbol_info_str(addr, function, offset, fnsize,
                                              module) + "\n"
            self.functions.append(function)
            if module != None:
                self.modules.append(module)

    def obj_in_str(self, include_meta=False):
        uniq_size_str = []
        uniq_size = set(self.size)
        for us in uniq_size:
            uniq_size_str.append("{0}*{1}".format(self.size.count(us), us))

        s = "Size: " + str(uniq_size_str) + "\n"
        s += "Total Size: " + str(self.total_size) + "\n"
        s += "Allocation Type: " + self.allocation_type
        if self.free == 1:
            s += " [Free]\n"
        else:
            s += "\n"
        s += "Flags: " + self.flags + "\n"
        s += "Category: " + self.category + "\n"
        s += "Subcategory: " + self.subcategory + "\n"
        s += "Modules: " + str(list(set(self.modules))) + "\n"
        if len(self.names) > 0:
            uniq_names = list(set(self.names))
            s += "Slab Name: " + str(uniq_names) + "\n"
        if include_meta:
            if len(self.pfns) > 0:
                s += "PFNs: " + str(self.pfns) + "\n"
        s += self.stack_str
        s += "\n"
        return s

    def __str__(self):
        return self.obj_in_str(False)

    def update_meminfo(self, names, pfns, addrs, size, flags):
        for pfn in pfns:
            self.pfns.append(pfn)
        self.size.append(size)
        for name in names:
            self.names.append(name)
        self.total_size += size

    def classify(self):
        if self.is_kernel_private():
            self.allocation_type = "Fallback"
        else:
            self.find_alloc_type()
            self.find_is_network_stack()

    def is_kernel_private(self):
        if len(self.addrs) == 1 and self.addrs[0] == self.ULONG_MAX:
            return True
        elif self.stack_str.find("page_ext_init") >= 0:
            return True
        return False

    def find_is_network_stack(self):
        for mod in self.modules:
            for nwmod in nss_modules:
                if mod == nwmod:
                    self.category = "Networking"
                    self.subcategory = "NSS"
                    return

        for mod in self.modules:
            for nwmod in wifi_modules:
                if mod == nwmod:
                    self.category = "Networking"
                    self.subcategory = "WiFi"
                    return

        for function in self.functions:
            filename = filename_from_vmlinux(function)
            if filename.find("net") >= 0:
                self.category = "Networking"
                self.subcategory = "Kernel"
                return

    def find_alloc_type(self):
        if self.flags.find("S") >= 0:
            self.allocation_type = "SLUB Allocation"
            return

        if self.flags.find("L") >= 0:
            self.allocation_type = "Page Cache"
            return

        s = self.stack_str
        if self.flags == "":
            if s.find("pte_alloc_kernel") >= 0:
                self.allocation_type = "IO Remapped Allocation"
            elif s.find("dma_alloc") >= 0:
                self.allocation_type = "DMA Allocation"
            elif s.find("vmalloc") >= 0:
                self.allocation_type = "VMalloc Allocation"
            else:
                self.allocation_type = "Alloc Pages allocation"
        elif (s.find("do_page_fault") >= 0) or (s.find("handle_mm_fault") >= 0):
            self.allocation_type = "User Space Allocation"

        return

