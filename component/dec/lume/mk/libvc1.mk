LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
MPTOP := ../libvc1/
LUME_PATH := $(LUME_TOP)

#	LOCAL_MXU_SRC_FILES = file.c
#
#	LOCAL_MXU_CFLAGS = 
#
#	LOCAL_MXU_ASFLAGS = 

MLOCAL_SRC_FILES := 	vc1dec.c \
						vc1.c \
						vc1data.c \
						vc1dsp.c \
						vc1_parser.c 

JZC_CFG = $(LUME_PATH)/libvc1/vc1_config.h
JZC_CONFIG = jzconfig.h
LOCAL_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES)) 
LOCAL_MODULE := libstagefright_vlumevc1

LOCAL_CFLAGS := $(PV_CFLAGS) -DHAVE_AV_CONFIG_H -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -DVIDEO_CODEC -D__LINUX__ -imacros $(JZC_CFG) -imacros $(JZC_CONFIG) -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DHAVE_CONFIG_H   -D_ISOC9X_SOURCE


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH)/libavcodec  \
	$(LUME_PATH)/libvc1/jzsoc  \
	$(LUME_PATH)/libvc1  \
	$(LUME_PATH)  \
	$(LUME_PATH)/libavutil \
	$(LUME_PATH)/libmpcodecs \
	$(PV_INCLUDES)

include $(BUILD_STATIC_LIBRARY)
LOCAL_COPY_DEPENDS_TO := etc
LOCAL_COPY_DEPENDS := $(LUME_PATH)/libvc1/jzsoc/vc1_p1.bin
include $(BUILD_COPY_DEPENDS)
