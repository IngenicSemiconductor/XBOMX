LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
MPTOP := ../madlib/libmad-0.15.1b/
LUME_PATH := $(LUME_TOP)

#	LOCAL_MXU_SRC_FILES = file.c
#
#	LOCAL_MXU_CFLAGS = 
#
#	LOCAL_MXU_ASFLAGS = 

MLOCAL_SRC_FILES := version.c fixed.c bit.c timer.c stream.c frame.c  \
			synth.c decoder.c layer12.c layer3.c huffman.c 

JZC_CFG = jzconfig.h
LOCAL_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES)) 
LOCAL_MODULE := libstagefright_mad

LOCAL_CFLAGS := $(PV_CFLAGS) -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -DAUDIO_CODEC -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DHAVE_CONFIG_H -D__LINUX__  -D_REENTRANT -D_LITTLE_ENDIAN=1 -fomit-frame-pointer -DFPM_DEFAULT -imacros $(JZC_CFG)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH)/libavcodec  \
	$(LUME_PATH)/madlib/libmad-0.15.1b/  \
	$(LUME_PATH)  \
	$(LUME_PATH)/libavutil \
	$(LUME_PATH)/libmpcodecs \
	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := 	../madlib/libmad-0.15.1b/mad.h

include $(BUILD_STATIC_LIBRARY)
LOCAL_COPY_DEPENDS_TO := etc
LOCAL_COPY_DEPENDS := 
include $(BUILD_COPY_DEPENDS)
