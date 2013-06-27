ifeq (1, 1)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	HardAVCEncoder.cpp \
	x264/x264.c \
	x264/common/mc.c \
	x264/common/predict.c \
	x264/common/pixel.c \
	x264/common/macroblock.c \
	x264/common/frame.c \
	x264/common/dct.c \
	x264/common/cpu.c \
	x264/common/cabac.c \
	x264/common/common.c \
	x264/common/mdate.c \
	x264/common/set.c \
	x264/common/quant.c \
	x264/common/vlc.c \
	x264/encoder/analyse.c \
	x264/encoder/me.c \
	x264/encoder/ratecontrol.c \
	x264/encoder/set.c \
	x264/encoder/macroblock.c \
	x264/encoder/cabac.c \
	x264/encoder/cavlc.c \
	x264/encoder/encoder.c \
	x264/encoder/lookahead.c \
	x264/soc/jz47xx_pmon.c \
	x264/soc/crc.c  \
	../../dec/lume/libjzcommon/jzm_intp.c

# LOCAL_SRC_FILES +=                 \
#         $(TOP)/frameworks/av/media/libstagefright/ESDS.cpp                  \
#         $(TOP)/frameworks/av/media/libstagefright/MediaBuffer.cpp           \
#         $(TOP)/frameworks/av/media/libstagefright/MediaBufferGroup.cpp      \
#         $(TOP)/frameworks/av/media/libstagefright/MediaDefs.cpp             \
#         $(TOP)/frameworks/av/media/libstagefright/MediaSource.cpp           \
#         $(TOP)/frameworks/av/media/libstagefright/MetaData.cpp              \
#         $(TOP)/frameworks/av/media/libstagefright/Utils.cpp      



LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../dec/lume/libjzcommon \
    $(TOP)/hardware/ingenic/xb4780/xbomx/component/common \
    $(TOP)/frameworks/av/media/libstagefright/codecs/avc/common/include \
    $(TOP)/frameworks/av/media/libstagefright/include \
    $(TOP)/frameworks/native/include/media/hardware \
    $(TOP)/frameworks/native/include/media/openmax \
    $(TOP)/frameworks/av/include/media/stagefright \
    $(LOCAL_PATH)/x264 \
    $(LOCAL_PATH)/x264/common \
    $(LOCAL_PATH)/x264/encoder \
    $(LOCAL_PATH)/x264/soc \
    $(TOP)/hardware/ingenic/xb4780/libdmmu 

LOCAL_CFLAGS := \
    -DOSCL_IMPORT_REF= -DOSCL_UNUSED_ARG= -DOSCL_EXPORT_REF=

LOCAL_CFLAGS += $(STAGEFRIGHT_FLAGS) -I$(TOP)/frameworks/av/media/libstagefright/lume/libjzcommon/

LOCAL_SHARED_LIBRARIES := \
        libstagefright_foundation \
        libstagefright_hard_vlume \
        libbinder       \
	libcutils	\
	libdmmu		\
        libstagefright \
        libstagefright_omx \
        libutils \
        libui

LOCAL_STATIC_LIBRARIES := libOMX_Basecomponent
LOCAL_MODULE := libstagefright_hard_x264hwenc
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

LOCAL_COPY_DEPENDS_TO := etc
LOCAL_COPY_DEPENDS := x264/soc/x264_p1.bin
include $(BUILD_COPY_DEPENDS)
endif