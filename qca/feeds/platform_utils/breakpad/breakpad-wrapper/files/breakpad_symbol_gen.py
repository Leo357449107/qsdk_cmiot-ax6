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

#Constants
RM = "rm -rf"
MAKE_SYMS = "make_syms"
HOST_TOOLS_PATH_TAIL = "staging_dir/host/usr/bin/breakpad"
SYMBOLS = "symbols"

def get_qsdk_path():
    #Function to get the top qsdk path
    return (os.getcwd().rsplit("qsdk",1)[0]) + "/qsdk/"

def is_exec(fpath):
            return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

def generate_breakpad_symbols(buildtargetPath, hostToolsPath):
    #Cleanup existing symbols if already present
    CMD_RM_SYMBOLS = RM + " " + hostToolsPath + "/"  + SYMBOLS
    print(CMD_RM_SYMBOLS)
    os.system(CMD_RM_SYMBOLS)

    #Traverse through all files in build_dir and generate symbols
    for root, dirs, files in os.walk(buildtargetPath):
        for filename in files:
            exec_file = (os.path.join(root, filename))
            if is_exec(exec_file):
                CMD_MAKE_SYMBOLS = hostToolsPath + "/"  + MAKE_SYMS + " " + exec_file
                print(CMD_MAKE_SYMBOLS)
                os.system(CMD_MAKE_SYMBOLS)

def main():
    #Parsing the Arguments
    parser = argparse.ArgumentParser(description='Generate Breakpad Symbols for the entire Build Folder')
    parser.add_argument('build_target', help='Provide the build_target folder. E.g. build_dir/target-arm_cortex-a7_musl-1.1.16_eabi')
    args = parser.parse_args()

    #Setting the Host Tools Path
    hostpath = get_qsdk_path()
    hostpath = hostpath + HOST_TOOLS_PATH_TAIL

    #Generating Breakpad Symbol Files for the full Build
    print ("Starting Symbol Generation")
    build_dir_path = args.build_target
    if path.exists(build_dir_path):
        generate_breakpad_symbols(buildtargetPath=build_dir_path, hostToolsPath=hostpath)
    else:
        print("The Path " + build_dir_path + " does not exist.")
        print("Breakpad Symbol Generation has failed.")
        exit(0)

    print ("Ending Symbol Generation")
    return

if __name__ == "__main__":
    main()
