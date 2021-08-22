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

#!/usr/bin/env python

import subprocess
import sys
import time
import os
import re
Latest_Tool_Path = "\\\\chewinlnx02\\workspace\\tools\\ramdump-parser"
pattern=re.compile(r'.*VERSION.*=')
def get_version(verfile):
    i=1
    ver_str = None
    fhandle = open(verfile)
    for line in fhandle:
        found=pattern.findall(line)
        for a in found:
            start_pos = line.index("'")
            start_pos = start_pos + 1
            ver_start  = line[start_pos:]
            end_pos = ver_start.index("'")
	    end_pos = start_pos + end_pos + 1
            ver_str = line[start_pos:end_pos]
        i=i+1
    return ver_str

curr_version = get_version("ramparse.py")
if (curr_version is not None):
    print "Current version : ", curr_version
share_path="//chewinlnx02/workspace/tools/ramdump-parser/latest.txt" 
share_version = get_version(share_path)

print("Checking for updated version....")
if (curr_version != share_version):
  #  print "Found new version ", share_version, " of ramparser"
    print('!!! Latest version {0} is available at {1}'.format(share_version,Latest_Tool_Path))
    print('!!! Proceeding with the old version {0}'.format(curr_version))
    time.sleep(2)
    cmdargs = str(sys.argv)
    cmd = 'python ramparse.py {0} {1} {2} {3} {4} {5} {6} {7} {8} {9} {10} {11} {12} {13} {14} {15} {16} {17} {18} {19} {20}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8],sys.argv[9],sys.argv[10],sys.argv[11],sys.argv[12],sys.argv[13],sys.argv[14],sys.argv[15],sys.argv[16],sys.argv[17],sys.argv[18],sys.argv[19],sys.argv[20],sys.argv[21])
    p=subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
    p.wait()
    sys.exit(0)

else:
	print ('!!! Using the Updated version {0}'. format(temp))
	#print('!!! Latest version {0} is available at {1}'.format(share_version,Latest_Tool_Path))
	#print "Found new version ", curr_version, " of ramparser"
	cmdargs = str(sys.argv)
	cmd = 'python ramparse.py {0} {1} {2} {3} {4} {5} {6} {7} {8} {9} {10} {11} {12} {13} {14} {15} {16} {17} {18} {19} {20}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8],sys.argv[9],sys.argv[10],sys.argv[11],sys.argv[12],sys.argv[13],sys.argv[14],sys.argv[15],sys.argv[16],sys.argv[17],sys.argv[18],sys.argv[19],sys.argv[20],sys.argv[21])
	p=subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
	p.wait()
	sys.exit(0)

