# Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE

# It is recommended to save any standard error messages by running script as
# 'bash setup_qsdk.sh 2>&1 | tee /var/tmp/setup_qsdk_run.log'

thisscript=$(basename $0)
timestring=$(date '+%Y%m%d_%H%M%S%z')
logdir=/var/tmp/qsdk_setup_${timestring}
mkdir -pv $logdir

echo "[$(date '+%D %T')] Start QSDK Setup Script: $thisscript"
echo "Log files will be stored at: $logdir"

echo "Checking for current user permissions."
if [[ $(id -u) -ne 0 ]] ; then echo "ERROR: You need sudo rights before proceeding. Run this script as sudo user or root!!!!" ; exit 1 ; fi

#check if git tool is installed on the host machine before proceeding
which git
if [ $? -eq 0 ]; then
	echo "$(git --version) found, Proceeding with setup..."
else
	echo "ERROR: git is not installed. Exiting the setup script"
	exit 1
fi
echo "Found $(lsb_release -d | awk -F: '{print $2}') OS installed on this system"

dpkg --list > ${logdir}/pkgs_found_before.log 2>&1

#Install all necessary packages for compiling qsdk code
apt-get -y install ocaml-findlib ocaml-base-nox ocaml-base libfindlib-ocaml-dev autoconf automake ccache cgroupfs-mount gcc g++ binutils patch bzip2 flex make gettext pkg-config unzip zlib1g-dev libc6-dev subversion libncurses5-dev gawk sharutils curl libxml-parser-perl python-yaml ocaml-nox ocaml libssl-dev libfdt-dev device-tree-compiler u-boot-tools parallel freetds-dev ocaml-native-compilers libpcre-ocaml libpcre-ocaml-dev libpycaml-ocaml-dev coccinelle

#for ubuntu-18, libssl1.0-dev needs to be installed
apt-get -y install libssl1.0-dev

#For open profile, coccienlle 1.0.7 needs to be installed from source code
apt-get -y remove --purge libparmap-ocaml

echo ""
echo "Installing and setting up coccienlle 1.0.7 from source code"

dpkg -r coccinelle
cd /tmp
rm -rf coccinelle
git clone https://github.com/coccinelle/coccinelle.git
cd /tmp/coccinelle
git checkout ed1eb8e06f800739d3992158d36945c0c4c6f0c7
./autogen
./configure
make
make install

#After installation of coccinelle, remove the downloaded code
cd /tmp
rm -rf coccinelle

dpkg --list > ${logdir}/pkgs_found_after.log 2>&1

gzip -9 ${logdir}/*

echo "[$(date '+%D %T')] End QSDK Setup Script: $thisscript"

#rm -rf ${logdir}
