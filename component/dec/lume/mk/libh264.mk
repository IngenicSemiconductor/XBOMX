LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
MPTOP := ../libh264/
LUME_PATH := $(LUME_TOP)

#	LOCAL_MXU_SRC_FILES = file.c
#
#	LOCAL_MXU_CFLAGS = 
#
#	LOCAL_MXU_ASFLAGS = 

MLOCAL_SRC_FILES := h264.c h264dsp.c cabac.c h264_loopfilter.c\
		  			h264_direct.c h264_ps.c h264_refs.c h264_sei.c \
		  			h264_cabac.c h264_cavlc.c
		  			# h264_cabac.c h264_bs_cabac.c svq3.c h264_cavlc.c h264_bs_cavlc.c

JZC_CFG = $(LUME_PATH)/libh264/h264_config.h
JZC_CONFIG = $(LUME_PATH)/jzconfig.h
LOCAL_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES)) 
LOCAL_MODULE := libstagefright_vlumeh264

LOCAL_CFLAGS := $(PV_CFLAGS) -DHAVE_AV_CONFIG_H -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-return-type -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -DVIDEO_CODEC -imacros $(JZC_CFG) -imacros $(JZC_CONFIG)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH)/libavcodec  \
	$(LUME_PATH)/libh264/jzsoc  \
	$(LUME_PATH)/libh264  \
	$(LUME_PATH)  \
	$(LUME_PATH)/libavutil \
	$(LUME_PATH)/libmpcodecs \
	$(TOP)/hardware/ingenic/xb4780/xbdemux/lume \
	$(TOP)/hardware/ingenic/xb4780/xbdemux/lume/stream \
	$(PV_INCLUDES)

include $(BUILD_STATIC_LIBRARY)
LOCAL_COPY_DEPENDS_TO := etc
LOCAL_COPY_DEPENDS := $(LUME_PATH)/libh264/jzsoc/h264_p1.bin \
		      $(LUME_PATH)/libh264/jzsoc/h264_cavlc_p1.bin
include $(BUILD_COPY_DEPENDS)
