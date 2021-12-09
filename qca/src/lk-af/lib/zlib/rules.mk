LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/zutil.o \
	$(LOCAL_DIR)/crc32_zlib.o \
	$(LOCAL_DIR)/adler32.o \
	$(LOCAL_DIR)/inftrees.o \
	$(LOCAL_DIR)/inflate.o \
	$(LOCAL_DIR)/inffast.o \
	$(LOCAL_DIR)/decompress.o \
	$(LOCAL_DIR)/trees.o \
	$(LOCAL_DIR)/deflate.o
