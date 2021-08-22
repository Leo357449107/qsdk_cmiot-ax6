PRJ_HOME = $(PWD)/../../../
LINUX_DIR = $(PRJ_HOME)/build_dir/target-arm_cortex-a7_musl-1.1.16_eabi/linux-ipq_ipq807x/linux-4.4.60
TOOL_PATH = $(PRJ_HOME)/staging_dir/toolchain-arm_cortex-a7_gcc-5.2.0_musl-1.1.16_eabi/bin
CROSS_COMPILE = arm-openwrt-linux-muslgnueabi-
ARCH = arm
export STAGING_DIR = "$(PRJ_HOME)/staging_dir"
