# Recipe extension for perf

EXTRA_CFLAGS += $(FPIC) \
		-I$(PKG_BUILD_DIR)

MAKE_FLAGS += \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)"
