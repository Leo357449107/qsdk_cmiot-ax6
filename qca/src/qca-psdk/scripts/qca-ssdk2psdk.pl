#!/usr/bin/env perl
# Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
# Permission to use, copy, modify, and/or distribute this software for
# any purpose with or without fee is hereby granted, provided that the
# above copyright notice and this permission notice appear in all copies.
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
# OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use warnings;
use strict;

my ($qca_ssdk_dir, $qca_ssdk_shell_dir) = ('../../qca-ssdk', '../../qca-ssdk-shell');
my ($drv_dir, $cli_dir) = ('drv', 'cli');
my $output_dir = '../';
my $temp_dir = 'temp';

sub qca808x_scan_output {
	my ($in_path, $out_path) = @_;
	my ($start_sig, $end_sig) = ("qca808x_start", "qca808x_end");
	my $dir = $in_path;
	my @files;
	my ($file_in, $file_out);
	my ($fh_in, $fh_out);

	@files = `find $in_path -name '*.c';find $in_path -name  '*.h'`;

	foreach $file_in (@files) {
	    chomp($file_in);
	    my $file_in_temp = $file_in;
	    $file_in_temp =~ s/$in_path//;
	    $file_out="$out_path/$file_in_temp";
	    print "$file_out\n";

	    open $fh_in, "<$file_in"  or die "$! $file_in"; ;

	    my $line;
	    my $record = undef;
	    my @content;
	    my $cp_right = undef;

	    while($line = <$fh_in>) {
		if(!defined $cp_right) {
			push @content, $line;
			$cp_right = 1;

		} elsif ($cp_right == 1) {
			push @content, $line;
			if($line =~ / \*\//) {
				push @content, "\n\n";
				$cp_right = 0;
			}
		}

		if($line =~ /$start_sig/) {
			$record = 1;

	        } elsif ($line =~ /$end_sig/) {
		    	$record = 0;

	        } else {
	            if(defined $record && $record) {
		        push @content, $line;
		    }
		}
	    }

	    @content = () if(!defined $record);

	    close($fh_in);
	    if($#content != -1) {
		my $file_out_path = `dirname $file_out`;
		chomp($file_out_path);
		unless (-d "$file_out_path") {
		    `mkdir -p $file_out_path`
		}

		open $fh_out, ">$file_out" or die "$! $file_out";
	        foreach my $write (@content) {
		    print $fh_out $write;
	    	}
	        close($fh_out);
	    }
	}
}

sub qca808x_copy_spec_files {
	my ($src, $dst, $files) = @_;

	foreach my $file_in (@{$files}) {
	    my $file_out="$dst/$file_in";
		my $file_out_path = `dirname $file_out`;
		chomp($file_out_path);
	    if (-d "$file_out_path") {
	    } else {
	        `mkdir -p $file_out_path`
	    }
	    $file_in = $src.'/'.$file_in;
	    `cp $file_in $file_out`;
	    print "cp $file_in $file_out\n";
	}
}

sub qca808x_drv_spec_copy {
	my ($src, $dst) = @_;

	my @files2copy = (
		"include/fal/fal_type.h",
		"include/sal/os/linux/aos_lock_pvt.h",
		"include/sal/os/linux/aos_mem_pvt.h",
		"include/sal/os/linux/aos_types_pvt.h",
		"include/sal/os/aos_lock.h",
		"include/sal/os/aos_types.h",
		"include/sal/os/aos_mem.h",
		"include/sal/os/linux/aos_timer_pvt.h",
		"include/sal/os/aos_timer.h",
		"include/sal/sd/linux/uk_interface/sw_api_ks.h",
		"include/hsl/hsl_lock.h",
		"include/hsl/phy/qca808x_phy.h",
		"include/hsl/hsl_port_prop.h",
		"include/hsl/scomphy/scomphy_init.h",
		"include/common/sw_error.h",
		"include/common/sw.h",
		"include/common/shared_func.h",
		"include/common/aos_head.h",
		"include/common/sw_config.h",
		"include/init/ssdk_phy_i2c.h",
		"include/api/api_access.h",
		"include/api/sw_api.h",
		"src/sal/sd/linux/uk_interface/sw_api_ks_ioctl.c",
		"src/hsl/hsl_port_prop.c",
		"src/hsl/hsl_lock.c",
		"src/hsl/hsl_api.c",
		"src/hsl/scomphy/scomphy_init.c",
	);

	qca808x_copy_spec_files($src, $dst, \@files2copy);
}

sub qca808x_cli_spec_copy {
	my ($src, $dst) = @_;

	my @files2copy = (
		"include/fal/fal_uk_if.h",
		"include/fal/fal_type.h",
		"include/shell/shell_config.h",
		"include/shell/shell_lib.h",
		"include/shell/shell.h",
		"include/sal/os/aos_types.h",
		"include/sal/os/linux_user/aos_mem_pvt.h",
		"include/sal/os/linux_user/aos_types_pvt.h",
		"include/sal/os/aos_mem.h",
		"include/sal/sd/linux/uk_interface/sw_api_us.h",
		"include/common/sw_error.h",
		"include/common/sw.h",
		"include/common/shared_func.h",
		"include/common/sw_config.h",
		"include/api/api_access.h",
		"include/api/sw_api.h",
		"src/shell/shell_lib.c",
		"src/sal/sd/linux/uk_interface/sw_api_us_ioctl.c",
		"src/fal_uk/fal_uk_if.c",
	);
	qca808x_copy_spec_files($src, $dst, \@files2copy);

}

sub qca808x_drv_reconstruct {
	my ($src, $dst) = @_;

	system("mkdir -p $dst/src/fal");
	system("mkdir -p $dst/src/hsl");
	system("cp $src/src/fal/*.c $dst/src/fal/");
	system("cp $src/src/hsl/*.c $dst/src/hsl/");
	system("cp $src/src/hsl/phy/*.c $dst/src/hsl/");
	system("cp $src/src/hsl/scomphy/*.c $dst/src/hsl/");
	system("cp $src/src/sal/sd/linux/uk_interface/*.c $dst/src/");
	system("cp $src/src/sal/sd/*.c $dst/src/");
	system("cp $src/src/init/*.c $dst/src/");
	system("cp $src/src/api/*.c $dst/src/");

	system("mkdir -p $dst/include/fal");
	system("mkdir -p $dst/include/hsl");
	system("mkdir -p $dst/include/osal");
	system("cp $src/include/fal/*.h $dst/include/fal/");
	system("cp $src/include/hsl/*.h $dst/include/hsl/");
	system("cp $src/include/hsl/phy/*.h $dst/include/hsl/");
	system("cp $src/include/hsl/scomphy/*.h $dst/include/hsl/");
	system("cp $src/include/init/*.h $dst/include/");
	system("cp $src/include/api/*.h $dst/include/");
	system("cp $src/include/common/*.h $dst/include/");
	system("cp $src/include/sal/os/*.h $dst/include/osal/");
	system("cp $src/include/sal/os/linux/*.h $dst/include/osal/");
	system("cp $src/include/sal/sd/*.h $dst/include/");
	system("cp $src/include/sal/sd/linux/uk_interface/*.h $dst/include/");

	system("mv $dst/include/aos_head.h $dst/include/osal/aos_head.h");
}

sub qca808x_cli_reconstruct {
	my ($src, $dst) = @_;

	system("mkdir -p $dst/src/fal");
	system("mkdir -p $dst/src/shell");
	system("cp $src/src/fal_uk/*.c $dst/src/fal/");
	system("cp $src/src/sal/sd/linux/uk_interface/*.c $dst/src/");
	system("cp $src/src/shell/*.c $dst/src/shell/");
	system("cp $src/src/api/*.c $dst/src/");

	system("mkdir -p $dst/include/fal");
	system("mkdir -p $dst/include/shell");
	system("mkdir -p $dst/include/osal");
	system("cp $src/include/fal/*.h $dst/include/fal/");
	system("cp $src/include/api/*.h $dst/include/");
	system("cp $src/include/common/*.h $dst/include/");
	system("cp $src/include/init/*.h $dst/include/");
	system("cp $src/include/shell/*.h $dst/include/shell/");
	system("cp $src/include/sal/os/*.h $dst/include/osal/");
	system("cp $src/include/sal/os/linux_user/*.h $dst/include/osal/");
	system("cp $src/include/sal/sd/linux/uk_interface/*.h $dst/include/");

	system("mv $dst/include/aos_head.h $dst/include/osal/aos_head.h");
}

sub qca808x_prepare {
	system("cp $qca_ssdk_dir . -a");
	system("cp $qca_ssdk_shell_dir . -a");

	system("rm -rf ./$temp_dir");
	system("mkdir -p ./$temp_dir/$drv_dir/");
	system("mkdir -p ./$temp_dir/$cli_dir/");
}

sub qca808x_cleanup {
	system("rm -rf ./qca-ssdk ./qca-ssdk-shell ./$temp_dir");
}

sub main {
	qca808x_prepare();

	qca808x_scan_output("./qca-ssdk", "./$temp_dir/$drv_dir");
	qca808x_drv_spec_copy("./qca-ssdk", "./$temp_dir/$drv_dir");
	qca808x_drv_reconstruct("./$temp_dir/$drv_dir/", "./$output_dir/$drv_dir");

	qca808x_scan_output("./qca-ssdk-shell", "./$temp_dir/$cli_dir");
	qca808x_cli_spec_copy("./qca-ssdk-shell", "./$temp_dir/$cli_dir");
	qca808x_cli_reconstruct("./$temp_dir/$cli_dir", "./$output_dir/$cli_dir");

	qca808x_cleanup();
}

main();

