#!/usr/bin/env perl
# Copyright (c) 2014 The Linux Foundation. All rights reserved.
use strict;
use warnings;
use File::Temp qw/ :POSIX /;
use File::Copy;
use File::Basename;
use Excel::Writer::XLSX;
use Excel::Writer::XLSX::Utility;
use XML::Simple;
use FindBin;
use lib "$FindBin::Bin";
use metadata;
use Data::Dumper;

no warnings "uninitialized";
# %OPTS
# Hash of all the options provided through command line
my %OPTS;

#Array of all the configs provided through command line
my @INPUT_CONFIGS;

#output target folder in the ../out/ folder.
my $targetOutFolder;

# @CONFIGS
# Each element in CONFIGS contains a hash storing a config file content
my @CONFIGS;

# %QSDKPKGS
# Each element is a package from metadata enabled at least in one of the
# config files specified as an argument
my %QSDKPKGS;
my %Subsystem = (
    "art2" => "HalPhy",
    "ath10k_firmware" => "Wi-Fi FW",
    "athdiag" => "Wi-Fi host",
    "athtestcmd" => "HalPhy",
    "base" => "OpenWrt",
    "bootloader" => "BOOT",
    "hyfi" => "SON",
    "ieee1905_security" => "SON",
    "luci" => "OpenWrt",
    "packages" => "OpenWrt",
    "nss" => "NSS",
    "nss_host" => "NSS",
    "nss_cust" => "NSS",
    "networking" => "NSS",
    "qca_platform_utils" => "BOOT",
    "qca_lib" => "SON",
    "platform_utils" => "BOOT",
    "qca_IOT" => "OpenWrt",
    "qca_mcs" => "NSS",
    "qca" => "Wi-Fi Host",
    "qcom_utils_internal" => "BOOT",
    "qca_lit" => "Wi-Fi FW",
    "qca_hk" => "Wi-Fi FW",
    "qca_np" => "Wi-Fi FW",
    "routing" => "OpenWrt",
    "shortcut_fe" => "NSS",
    "sigma_dut" => "Wi-Fi Host",
    "ssdk" => "SSDK",
    "wlan_open" => "Ath10K",
    "wapid" => "Wi-Fi Host",
    "whc" => "SON"
);

sub get_profile_info($) {
    my $filename = shift;
    open(FILE, "<$filename") || die "File not found";
    my @lines = <FILE>;
    close(FILE);
    foreach my $line (@lines) {
        if ($line =~ m/^.*_target_(ipq.*)_QSDK_(.*)=y/i) {
            print "Generating for Profile: $2\n";
            return $2;
        }
    }
    print "Unable to find profile information from config file";
    return "";
}

sub get_target_info($) {
    my $filename = shift;
    print $filename;
    my ($target, $subtarget) = ("","");
    if($filename =~ m/^.*_ipq_(ipq.*)_QSDK_(.*)\.xlsx/i) {
        $target = 'ipq';
        $subtarget = lc $1;
        $targetOutFolder = $subtarget;
    }
    elsif($filename =~ m/^.*_(ipq.*)_(ipq.*)_QSDK_(.*)\.xlsx/i) {
        $target = lc $1;
        $subtarget = lc $2;
        $targetOutFolder = $subtarget;
    }
    elsif($filename =~ m/^.*_(ipq.*)_generic_QSDK_(.*)\.xlsx/i ){
        $target = lc $1;
        $subtarget = "generic";
        $targetOutFolder = $target."_64";
    }
    else {
        print "Unknown format passed as input, using default";
    }
    return ($target, $subtarget);
}

sub replace_target {
    my ($target, $subtarget, $profile, $file ) = @_;
    my $cmd = "echo \"CONFIG_TARGET_$target=y\" >> $file";
    print $cmd;
    system($cmd);
    $cmd = "echo \"CONFIG_TARGET_$target"."_$subtarget=y\" >> $file";
    print $cmd;
    system($cmd);
    $cmd = "echo \"CONFIG_TARGET_$target"."_$subtarget"."_QSDK_"."$profile=y\" >> $file";
    print $cmd;
    system($cmd);
}

sub load_config() {
    my $target;
    my $subtarget;
    my $profile;
    for ( @{ $OPTS{o} } ) {
        if (/^.*\.xlsx$/) { ($target, $subtarget) = get_target_info($_); next; }
    }

    foreach my $defconfig (@INPUT_CONFIGS) {
        my $dotconfig = tmpnam();
        next if($defconfig =~ m/.*_debug/ or $defconfig =~ m/.*_ioe_.*/ or $defconfig =~ m/.*ar71xx_open\..*/
          or $defconfig =~ m/.*ar71xx_premium\..*/ or $defconfig =~ m/.*_upstream\..*/ or $defconfig =~ m/.*ar71xx_wireless\..*/ or $defconfig =~ m/.*_caf.*/);

            # Create a temporary file and extrapolate it with default values
            copy( $defconfig, $dotconfig ) or die "Error when copying $defconfig: $!";

            #In the dotconfig, replace the target with required target before doing make defconfig
            $profile = get_profile_info($dotconfig);
            if( $profile ne "" and $target ne "" and $subtarget ne "" ) {
                replace_target($target, $subtarget, $profile, $dotconfig);
            }
            else {
                print "Incomplete info for replacing target, doing defconfig with default config file passed as input: $defconfig \n";
            }
            system( "./scripts/config/conf "
                  . "--defconfig=$dotconfig "
                  . "-w $dotconfig "
                  . "Config.in " );

            # Load the content in a config hash
            my %config;
            open FILE, "<$dotconfig" or return;
            while (<FILE>) {
                /^CONFIG_DEFAULT_(.+?)=y$/ and $config{$1} = { default => 1 };
                /^CONFIG_PACKAGE_(.+?)=m$/ and $config{$1} = { loadable => 1};
                /^CONFIG_PACKAGE_(.+?)=y$/ and $config{$1} = {};
            }
            close FILE;
            $config{name} = basename( $defconfig, ".config" );

            # Store the config hash in @CONFIGS and clean-up the FS
            push( @CONFIGS, \%config );
            unlink $dotconfig;
    }
}

sub xref_packages() {
    load_config();
    foreach my $config (@CONFIGS) {
        foreach my $pkg ( keys %{$config} ) {

            # Some config options created through "Package/config" still
            # match the CONFIG_PACKAGE_ namespace so we'll ignore them here
            # It's the case for LuCI, as an example
            # (PACKAGE_luci-lib-core_source, PACKAGE_luci-lib-core_compiled...)
            next if !exists( $package{$pkg} );

            $QSDKPKGS{$pkg} = {
                src     => $package{$pkg}->{src},
                name    => $package{$pkg}->{name},
                variant => $package{$pkg}->{variant},
                # Feeds (subdir) is set to an empty string for packages in openwrt.git
                subdir      => $package{$pkg}->{subdir},
                version     => $package{$pkg}->{version},
                license     => $package{$pkg}->{license},
                description => $package{$pkg}->{description},
                source      => $package{$pkg}->{source},
                isIntTarball      => $package{$pkg}->{isIntTarball},
            }
            unless exists( $QSDKPKGS{$pkg} );

            # Linux & kmods version starts with the string <LINUX_VERSION>
            # We remove this prefix here for better readability
            $QSDKPKGS{$pkg}->{version} =~ s/^<LINUX_VERSION>-$/KERNEL/;
            $QSDKPKGS{$pkg}->{version} =~ s/<LINUX_VERSION>[\+-](.+)/$1/;

            if ( exists( $config->{$pkg}->{default} ) ) {
                push( @{ $QSDKPKGS{$pkg}->{DefaultConfigs} }, $config->{name} );
            }
            elsif( exists( $config->{$pkg}->{loadable} ) ) {
                push( @{ $QSDKPKGS{$pkg}->{LoadConfigs} }, $config->{name} );
            }
            else {
                push( @{ $QSDKPKGS{$pkg}->{EnabledConfigs} }, $config->{name} );
            }
        }
    }
}

sub write_output_xlsx($) {

    my $filename = shift;
    # Create a new workbook and add a worksheet
    my $workbook  = Excel::Writer::XLSX->new($filename);

    my @out = `ls -d ../out/*/ | cut -d'/' -f3`;
    chomp @out;

    # Print to check the folders exists while build. (Remove this part once clarified - PREM - START)
    my $worksheet = $workbook->add_worksheet('OUT - FOLDERS');
    my $tempRow = 0;
    foreach my $list (@out) {
        chomp $list;
        $worksheet->write( $tempRow++, 0, $list );
    }
    # PREM : END

    &WriteDataToExcel($workbook, 0);
    &WriteDataToExcel($workbook, 1);
    WriteDownloadedFilesToExcel($workbook);
    $workbook->close();
}

sub WriteDataToExcel
{
    my($workbook, $loadConfigMode) = @_;
    my $configMode = ($loadConfigMode == 1) ? 'LoadConfigs' : 'EnabledConfigs';

    my $worksheet = $workbook->add_worksheet($configMode);

    # Init the worksheet (set columns width, create colors & formats)
    $worksheet->set_column( 0, 1, 28 );    # Column A,B width set to 28
    $worksheet->set_column( 2, 3, 14 );    # Column C,D width set to 14
    $worksheet->set_column( 4, 4, 28 );    # Column E width set to 28
    $worksheet->set_column( 5, 5, 14 );    # Column E width set to 14
    $worksheet->set_column( 6, 6, 12 );    # Column F width set to 12
    my $c_yellow = $workbook->set_custom_color( 36, 255, 255, 153 );
    my $c_orange = $workbook->set_custom_color( 40, 247, 150, 70 );
    my $c_green  = $workbook->set_custom_color( 41, 196, 215, 155 );
    my $c_red    = $workbook->set_custom_color( 42, 218, 150, 148 );
    my $f_title  = $workbook->add_format(
        align    => 'center',
        valign   => 'vcenter',
        bold     => 1,
        border   => 2,                     # Continuous, Weight=2
        bg_color => $c_orange,
    );
    my $f_data = $workbook->add_format(
        align  => 'center',
        valign => 'vcenter',
        border => 1,
    );
    my $f_green_data = $workbook->add_format(
        align    => 'center',
        valign   => 'vcenter',
        border   => 1,
        bg_color => $c_green,
    );
    my $f_red_data = $workbook->add_format(
        align    => 'center',
        valign   => 'vcenter',
        border   => 1,
        bg_color => $c_red,
    );
    my $f_yellow_data = $workbook->add_format(
        align    => 'center',
        valign   => 'vcenter',
        border   => 1,
        bg_color => $c_yellow,
    );
    my $f_desc = $workbook->add_format(
        align  => 'fill',
        border => 1,
    );

    # Fill-in the titles
    my @col = (
        "SRC", "PACKAGE", "VARIANT", "FEED", "SUBSYSTEM",
        "TARBALL", "VERSION", "LICENSE", "DESCRIPTION", "FLASHSIZE", "DDRSIZE"
    );
    my $colid = 0;
    foreach (@col) {
        $worksheet->write( 0, $colid, $col[$colid], $f_title );
        $colid++;
    }
    foreach ( sort { $a->{name} cmp $b->{name} } @CONFIGS ) {
        $worksheet->write( 0, $colid, $_->{name}, $f_title );
        $worksheet->set_column( $colid, $colid, 20 );
        $colid++;
    }

    # Now we're ready. Let's start adding the data
    my $row = 1;
    my $prevpkg, my $start_merge = 0;
    my ($curFeed, $prevFeed, $prevSubsystem);
    foreach my $cur (
        sort { $QSDKPKGS{$a}->{src} cmp $QSDKPKGS{$b}->{src} }
        keys %QSDKPKGS
    )
    {
        if(exists $QSDKPKGS{$cur}->{$configMode}) {
            my $col    = 0;
            my $curpkg = $QSDKPKGS{$cur};
            my $pwd = `pwd`;
            chomp($pwd);

            $worksheet->write( $row, $col++, $curpkg->{src},     $f_data );
            $worksheet->write( $row, $col++, $curpkg->{name},    $f_data );
            $worksheet->write( $row, $col++, $curpkg->{variant}, $f_data );
            $curFeed = length( $curpkg->{subdir} ) ? basename( $curpkg->{subdir} ) : "openwrt";
            my $cmd = "find ./package/feeds/ -name $curpkg->{src}";
            my @isFeed = `$cmd`;
            $curFeed = (scalar @isFeed == 0) ? 'base' : $curFeed;
            $worksheet->write( $row, $col++, $curFeed, $f_data );
            if(exists $Subsystem{lc $curFeed}) {
                $worksheet->write( $row, $col++, $Subsystem{lc $curFeed}, $f_data );
            } else {
                $worksheet->write_blank($row, $col++, $f_data);
            }
            if( $curpkg->{isIntTarball} eq "False") {
                $worksheet->write( $row, $col++, $curpkg->{source},  $f_data );
            } else {
                $worksheet->write_blank($row, $col++, $f_data);
            }
            $worksheet->write_string( $row, $col++, $curpkg->{version}, $f_data )
            unless !exists( $curpkg->{version} );
            $worksheet->write_string( $row, $col++, $curpkg->{license}, $f_data )
            unless !exists( $curpkg->{license} );

            $worksheet->write( $row, $col++, $curpkg->{description}, $f_desc );

            my ($fileSize, @findFile, $fcolor);
            $curpkg->{name} = 'kmod-fs-configfs' if($curpkg->{name} eq 'kmod-usb-configfs');
            $cmd = "find ../out/$targetOutFolder/packages/ -name $curpkg->{name}_*.ipk -ls 2> /dev/null";
            @findFile = `$cmd`;
            @findFile = grep /\S/, @findFile;
            if($#findFile != -1) {
                $fcolor = $f_green_data;
                goto FLASHSIZE;
            }

            $cmd = "find ./bin/ipq*/packages/ -name $curpkg->{name}_*.ipk -ls 2> /dev/null";
            @findFile = `$cmd`;
            @findFile = grep /\S/, @findFile;
            $fcolor = $f_yellow_data;

FLASHSIZE:
            if($#findFile == 0) {
                $findFile[0] =~ s/^\s+|\s+$//g;
                $fileSize = (split(/\s+/, $findFile[0]))[6];
            }

            if($#findFile > 0) {
                #print "Excel Write Warning: More than one file matched for $curpkg->{name}. Please check correct filesize and write manually to Report !!!\n";
                $worksheet->write( $row, $col++, 'MORE_THAN_ONE_FILE', $f_data );
            }
            elsif($fileSize ne '' and $fileSize =~ m/\d+/) {
                $worksheet->write( $row, $col++, $fileSize, $fcolor );
            } else {
                #print "Excel Write Warning: $curpkg->{name} file not found in packages\n";
                $fileSize = "NOTFOUND";
                $worksheet->write( $row, $col++, $fileSize, $f_data );
            }

DDRSIZE:
            my ($ddrSize, @paths, $status, $srcName, $pkgName);
            $pkgName = $curpkg->{name};
            $srcName = $curpkg->{src};
            ($status, @paths) = &getDDRSize($srcName, $pkgName, $curpkg->{version});
            if($status eq "FAIL") {
                # special exceptions for Package Name
                if($srcName =~ m/libreadline/) { $srcName = "readline"; }
                elsif($srcName =~ m/libevent2/) { $srcName = "libevent"; }
                elsif($srcName =~ m/libjson-c/) { $srcName = "json-c"; }
                elsif($srcName =~ m/simulated-driver/) { $srcName = "shortcut-fe-simulated-driver"; }
                elsif($srcName =~ m/iwinfo/) { $srcName = "libiwinfo"; }
                elsif($srcName =~ m/wireless-tools/) { $srcName = "wireless_tools"; }
                elsif($srcName =~ m/uboot-envtools/ or $srcName =~ m/uboot-qca/) { $srcName = "u-boot";}
                else { $srcName = $pkgName; }
                ($status, @paths) = &getDDRSize($srcName, $pkgName, $curpkg->{version});
                if($status eq 'FAIL') {
                    undef @paths;
                }
            }

            if($#paths == 0) {
                $cmd = "find $paths[0] -type f -executable | xargs size -t 2> /dev/null | grep '(TOTALS)'";
                $cmd = "find $paths[0] -type f -name '*.ko' | xargs size -t 2> /dev/null | grep '(TOTALS)'" if($curpkg->{name} =~ m/^kmod-/i);
                $ddrSize = `$cmd`;
                $ddrSize =~ s/^\s+|\s+$//g;
                $ddrSize = (split(/\s+/, $ddrSize))[3];
            }
            elsif($#paths > 0) {
                my ($iproutePaths, $data, @data, @out);
                if(lc $curpkg->{src} =~ m/iproute2/ and lc $curpkg->{name} =~ m/tc/) {
                    $iproutePaths = join(' ', @paths);
                    $cmd = "find $iproutePaths -type f -executable | xargs size 2> /dev/null";
                    @out = `$cmd`;
                    shift(@out);
                    foreach $data (@out) {
                        $data =~ s/^\s+|\s+$//g;
                        push (@data, (split(/\s+/, $data))[3]);
                    }
                    @data = reverse sort @data;
                    $ddrSize = ($data[0] ne '') ? $data[0] : 0;
                }
                elsif(lc $curpkg->{src} =~ m/qca-hostap/ and lc $curpkg->{name} =~ m/qca-wpa-cli/) {
                    foreach my $path (@paths) {
                        if($path =~ m/default/) {
                            $cmd = "find $path -type f -executable | xargs size 2> /dev/null";
                            @out = `$cmd`;
                            shift @out;
                            $out[0] =~ s/^\s+|\s+$//g;
                            $ddrSize = (split(/\s+/, $out[0]))[3];
                            last;
                        }
                    }
                }
                else {
                    $ddrSize = 'MANUAL';
                }
            }

            if($ddrSize ne '') {
                $worksheet->write( $row, $col++, $ddrSize, $f_green_data );
            } elsif($ddrSize eq 'MANUAL') {
                $worksheet->write( $row, $col++, "MORE THAN ONE PACKAGE MATCHED", $f_yellow_data );
            } else {
                $worksheet->write( $row, $col++, "NOTFOUND", $f_data );
            }

            my %pkgconfigs = map { $_ => 1 } @{ $curpkg->{$configMode} };
            foreach my $conf ( sort { $a->{name} cmp $b->{name} } @CONFIGS ) {
                $worksheet->write( $row, $col++, "x", $f_green_data )
                if exists( $pkgconfigs{ $conf->{name} } );
                $worksheet->write_blank( $row, $col++, $f_red_data )
                if !exists( $pkgconfigs{ $conf->{name} } );
            }

            # If the same package defines multiple BuildPackage/KernelPackage,
            # we want to merge the src cells over multiple rows.
            if ( $start_merge == 0 and exists( $prevpkg->{src} ) and $curpkg->{src} eq $prevpkg->{src} ) {
                $start_merge = $row - 1;
            }
            if ( $start_merge != 0 and $curpkg->{src} ne $prevpkg->{src} ) {
                # We want to merge the src, version and feeds columns
                $worksheet->merge_range( $start_merge, 0, $row - 1, 0, $prevpkg->{src}, $f_data );
                $worksheet->merge_range( $start_merge, 3, $row - 1, 3, $prevFeed, $f_data );
                $worksheet->merge_range( $start_merge, 4, $row - 1, 4, $prevSubsystem, $f_data );
                if( $prevpkg->{isIntTarball} eq "False") {
                    $worksheet->merge_range( $start_merge, 5, $row - 1, 5, $prevpkg->{source}, $f_data );
                }
                $worksheet->merge_range( $start_merge, 6, $row - 1, 6, $prevpkg->{version}, $f_data );
                $worksheet->merge_range( $start_merge, 7, $row - 1, 7, $prevpkg->{license}, $f_data );
                $start_merge = 0;
            }
            $prevFeed = $curFeed;
            $prevSubsystem = $Subsystem{lc $curFeed};
            $prevpkg = $curpkg;
            $row++;
        }
    }
}

sub getDDRSize {
    my ($srcName, $pkgName, $version) = @_;
    my ($cmd, @paths, @findFile);
    $cmd = "find build_dir/target*/ -name \'$pkgName\' -type d | grep \"ipkg*\"";
    @findFile = `$cmd`;
    @findFile = grep /\S/, @findFile;
    chomp @findFile;

    @paths = grep (m/^build_dir\/target.*\/$srcName\b[-|\d|\.]*.*\/ipkg-(ipq|install|all)\/$pkgName\s*$/i, @findFile);
    @paths = @findFile if($srcName =~ m/^linux/i or $srcName =~ /art2/i or $srcName =~ m/qca-iface-mgr/i or $srcName =~ m/qca-mcs-lkm/i);

    if($srcName =~ m/qca-iface-mgr/ or $srcName =~ m/qca-mcs-lkm/) {
        my @temp = split('-', $srcName);
        pop @temp;
        $srcName = join('-', @temp);
        @temp = split('-', $version);
        pop @temp;
        $version = join('-', @temp);
        foreach my $path (@paths) {
            if($path =~ m/$version/ and $path =~ m/$srcName/) {
            return ("SUCCESS", $path);
            }
        }
        return ("FAIL", @paths);
    }

    if(grep(m/ipkg-ipq/, @paths)) {
        @paths = grep(m/ipkg-ipq/, @paths);
    } elsif(grep(m/ipkg-install/, @paths)) {
        @paths = grep(m/ipkg-install/, @paths);
    } elsif(grep(m/ipkg-all/, @paths)) {
        @paths = grep(m/ipkg-all/, @paths);
    } else {
        return ("FAIL", @paths);
    }
    return ("SUCCESS", @paths);
}

sub write_output_xml($) {
    my @pkgs_list = values %QSDKPKGS;
    my $xml_list = { package => \@pkgs_list } ;
    XMLout( $xml_list,
            OutputFile => $_,
            XMLDecl => 1,
            RootName => 'release');
}

sub write_output() {
    for ( @{ $OPTS{o} } ) {
        if (/^.*\.xlsx$/) { write_output_xlsx($_); next; }
        if (/^.*\.xml$/)  { write_output_xml($_);  next; }
        print("$_: file format not recognized\n");
    }
}

sub show_help() {
    print <<EOF
Usage: $0 [options] config1 config2 config3 ...
Available Options:
    -o Output_filename (format: <prefix>_<target>_<subtarget>_QSDK_<Profile>.xlsx)
    -h (for help on usage)
Examples:
$0 -o custom_ipq_50xx_QSDK_Open.xlsx qca/configs/qsdk/ipq_premium.config
$0 -o APSS.LN.11.4_ipq_ipq807x_64_QSDK_Premium.xlsx qca/configs/qsdk/*.config
EOF
}

sub show_help_and_exit($) {
    my $opt = shift;
    print "Error: -$opt not defined\n" if $opt ne "h";
    show_help();
    exit 1;
}

sub parse_command() {
    while ( my $opt = shift @ARGV ) {
        if ( $opt =~ /^-o$/ ) { push( @{ $OPTS{o} }, shift(@ARGV) ) }
        elsif ( $opt =~ /^-(.*)$/ ) {
            show_help_and_exit($1);
        }
        else { push( @INPUT_CONFIGS, $opt); }
    }

    if ( !defined( $OPTS{o} ) ) {
        push( @{ $OPTS{o} }, "openwrt.xlsx" );
    }
}

sub WriteDownloadedFilesToExcel {
    my $workbook = shift;
    my $worksheet = $workbook->add_worksheet("downloadedFiles");
    $worksheet->set_column( 0, 0, 28 );
    my $c_orange = $workbook->set_custom_color( 40, 247, 150, 70 );
    my $f_title  = $workbook->add_format(
        align    => 'center',
        valign   => 'vcenter',
        bold     => 1,
        border   => 2,                     # Continuous, Weight=2
        bg_color => $c_orange,
    );
    my $f_data = $workbook->add_format(
        align  => 'center',
        valign => 'vcenter',
        border => 1,
    );

    my $row_id = 0;
    $worksheet->write( $row_id, 0, "Downloaded FileName", $f_title );
    $row_id++;
    my $dl_folder_files = "dlFiles.txt";
    system "ls dl/ -p | grep -v / > dlFiles.txt";
    open(my $fh,"<", "dlFiles.txt") or die "cant open the download list files";
    while(my $dlFile = <$fh>) {
        my $addFlag = "True";
        if( index($dlFile,"qca-wifi-fw") != -1) {
            next;
        }
        chomp($dlFile);
        foreach my $pkg (keys %QSDKPKGS) {
            if( ($QSDKPKGS{$pkg}->{source} eq $dlFile) and ($QSDKPKGS{$pkg}->{isIntTarball} eq "True")) {
                 #this is not an opensource tarball, do not add
                 $addFlag = "False";
                 last;
            }
            if(($QSDKPKGS{$pkg}->{source} eq $dlFile) and ($QSDKPKGS{$pkg}->{isIntTarball} eq "False")) {
                 #opensource, exit the loop to save iterations
                 last;
            }
        }
        if( $addFlag ne "False") {
            $worksheet->write( $row_id, 0, $dlFile, $f_data );
            $row_id++;
        }
    }
    close($fh);
    unlink $dl_folder_files;
}
parse_command();
parse_package_metadata("tmp/.packageinfo");
xref_packages();
write_output();
