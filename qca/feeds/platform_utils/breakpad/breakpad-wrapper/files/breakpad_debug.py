#!/usr/bin/python
# Copyright (c) 2019, The Linux Foundation. All rights reserved.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import os
import os.path
from os import path
import sys
import argparse

#Default Arguments
DEFAULT_BIN_PATH = "build_dir/target-arm_cortex-a7_musl-1.1.16_eabi/testAppUsingBreakpad-1.0/testAppUsingBreakpad"

#Constants
RM = "rm -rf"
MAKE_SYMS = "make_syms"
STACK_WALK = "minidump_stackwalk"
HOST_TOOLS_PATH_TAIL = "staging_dir/host/usr/bin/breakpad"
SYMBOLS = "symbols"

def get_qsdk_path():
    #Function to get the top qsdk path
    return (os.getcwd().rsplit("qsdk",1)[0]) + "qsdk/"

def run_commands(binPath, dumpPath, hostToolsPath):
    #Function to run breakpad stack-trace commands
    #CMD_RM_SYMBOLS = RM + " " + hostToolsPath + "/"  + SYMBOLS
    #print(CMD_RM_SYMBOLS)
    #os.system(CMD_RM_SYMBOLS)
    if path.exists(binPath):
        CMD_MAKE_SYMBOLS = hostToolsPath + "/"  + MAKE_SYMS + " " + binPath
        print(CMD_MAKE_SYMBOLS)
        os.system(CMD_MAKE_SYMBOLS)
    else:
        print("Error: Unable to create breakpad symbols for binary: " + binPath)
        print("The Path " + binPath + " does not exist.")
        print("Use --binpath option to set the binary path")
        exit(0)
    if path.exists(dumpPath):
        CMD_STACK_TRACE = hostToolsPath + "/"  + STACK_WALK + " " + dumpPath + " " + hostToolsPath + "/"  + SYMBOLS
        print(CMD_STACK_TRACE)
        os.system(CMD_STACK_TRACE)
        return
    else:
        print("Error: Unable to find minidump file: " + dumpPath)
        print("The Path " + dumpPath + " does not exist.")
        exit(0)

def main():
    #Parsing the Arguments
    parser = argparse.ArgumentParser(description=' Stack-trace minidumps created through breakpad Application')
    parser.add_argument('--binpath', dest='binpath', action='store', default=DEFAULT_BIN_PATH, help='Provide the Full Path to the Binary')
    parser.add_argument('dump', help='Provide the Full Path to the Dump File')
    args = parser.parse_args()

    #Setting the Host Tools Path
    hostpath = get_qsdk_path()
    hostpath = hostpath + HOST_TOOLS_PATH_TAIL

    #Executing Minidump Tracing Steps
    print ("Starting Trace")
    run_commands(binPath=args.binpath, dumpPath=args.dump, hostToolsPath=hostpath)
    print ("Ending Trace")
    return

if __name__ == "__main__":
    main()
