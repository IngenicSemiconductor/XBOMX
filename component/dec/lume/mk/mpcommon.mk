LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
MPTOP := ../

LUME_PATH := $(LUME_TOP)

MLOCAL_SRC_FILES := codec-cfg.c \
		mp_msg.c \
		cpudetect.c \
		fate.c \
		libvo/aclib.c \
		libmpcodecs/img_format.c \
		libmpcodecs/mp_image.c \
		libmpcodecs/pullup.c \
	    	loadfile.c \
		demux_fate.c
	 	#m_struct.c \
	 	#m_option.c \
	 	#sub_cc.c


LOCAL_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES)) 
LOCAL_MODULE := libstagefright_ffmpcommon

JZC_CFG := jzconfig.h
LOCAL_CFLAGS := -DHAVE_AV_CONFIG_H -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -imacros $(JZC_CFG)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH) \
	$(LUME_PATH)/libavutil \
	$(LUME_PATH)/libmpcodecs \
	$(TOP)/hardware/ingenic/xb4780/xbdemux/lume \
	$(TOP)/hardware/ingenic/xb4780/xbdemux/lume/stream

LOCAL_COPY_HEADERS_TO :=

LOCAL_COPY_HEADERS :=

include $(BUILD_STATIC_LIBRARY)
