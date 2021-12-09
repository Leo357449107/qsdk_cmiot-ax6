obj-m += $(TARGET).o

$(TARGET)-objs := \
		./src/fal/fal_port_ctrl.o	\
        ./src/fal/fal_reg_access.o	\
        ./src/fal/fal_init.o	\
        ./src/hsl/hsl_port_prop.o	\
        ./src/hsl/qca808x_phy.o	\
        ./src/hsl/hsl_phy.o	\
        ./src/hsl/hsl_dev.o	\
        ./src/hsl/hsl_lock.o	\
        ./src/hsl/hsl_api.o	\
        ./src/hsl/scomphy_init.o	\
        ./src/hsl/scomphy_port_ctrl.o	\
        ./src/hsl/scomphy_reg_access.o	\
        ./src/sw_api_ks_ioctl.o	\
        ./src/sd.o	\
        ./src/ssdk_plat.o	\
        ./src/ssdk_phy_i2c.o	\
        ./src/ssdk_init.o	\
        ./src/api_access.o
