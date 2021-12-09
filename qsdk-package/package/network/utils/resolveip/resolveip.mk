# Recipe extension for resolveip

define resolveip_compile_append
   $(TARGET_LDFLAGS)
endef

Build/Compile += $(resolveip_compile_append)
