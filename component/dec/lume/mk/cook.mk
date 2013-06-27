LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
MPTOP := ../ra8lbr/
LUNE_PATH := $(LUME_TOP)

#	LOCAL_MXU_SRC_FILES = file.c
#
#	LOCAL_MXU_CFLAGS = 
#
#	LOCAL_MXU_ASFLAGS = 

MLOCAL_SRC_FILES := decoder/gecko2codec.c decoder/bitpack.c decoder/buffers.c decoder/category.c \
decoder/couple.c decoder/envelope.c decoder/fft.c decoder/gainctrl.c decoder/huffman.c \
decoder/hufftabs.c decoder/mlt.c decoder/sqvh.c decoder/trigtabs.c ra8lbr_decode.c

JZC_CFG = 
LOCAL_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES)) 
LOCAL_MODULE := libstagefright_realcook
JZC_CFG := jzconfig.h
LOCAL_CFLAGS := $(PV_CFLAGS) -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -DAUDIO_CODEC -D__LINUX__  -D_LITTLE_ENDIAN=1 -fomit-frame-pointer -imacros $(JZC_CFG)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUNE_PATH)/libavcodec  \
	$(LUNE_PATH)/ra8lbr  \
	$(LUNE_PATH)/ra8lbr/decoder  \
	$(LUNE_PATH)/ra8lbr/codec_include  \
	$(LUNE_PATH)/ra8lbr/include  \
	$(LUNE_PATH)  \
	$(LUNE_PATH)/libavutil \
	$(LUNE_PATH)/libmpcodecs \
	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := 

include $(BUILD_STATIC_LIBRARY)
LOCAL_COPY_DEPENDS_TO := etc
LOCAL_COPY_DEPENDS := 
include $(BUILD_COPY_DEPENDS)
