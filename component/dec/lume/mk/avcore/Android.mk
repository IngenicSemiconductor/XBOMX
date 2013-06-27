LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
OBJS-yes:=

LUME_PATH := $(LUME_TOP)

include $(LUME_PATH)/codec_config.mak
include $(LOCAL_PATH)/core.mak

SUBDIR := libavcore/
OBJS      += $(OBJS-yes)
OBJS := $(sort $(OBJS) )

OBJS      := $(addprefix $(SUBDIR),$(OBJS))


LOCAL_SRC_FILES := $(OBJS:.o=.c)

LOCAL_MODULE := libstagefright_ffavcore

JZC_CFG := jzconfig.h
LOCAL_CFLAGS := -DHAVE_AV_CONFIG_H -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -imacros $(JZC_CFG)

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH)\
	$(LUME_PATH)/libavutil

LOCAL_COPY_HEADERS_TO :=

LOCAL_COPY_HEADERS :=

include $(BUILD_STATIC_LIBRARY)
