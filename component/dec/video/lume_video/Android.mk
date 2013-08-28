#ifeq (true, false)
LOCAL_PATH:= $(call my-dir)
# XBOMX_FLAGS := -DPREFETCHER_DEPACK_NAL -D_GNU_SOURCE -DIPU_4780BUG_ALIGN=2048
XBOMX_FLAGS := -D_GNU_SOURCE -DIPU_4780BUG_ALIGN=2048
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		lume_decoder.cpp \
		lume_dec.cpp     \
		mpeg2_decoder.cpp 

LOCAL_C_INCLUDES := \
		frameworks/av/media/libstagefright/include \
		frameworks/native/include/media/openmax    \
		$(LOCAL_PATH)/include                      \
		$(LOCAL_PATH)/../../include                      \
		$(LOCAL_PATH)/../../lume/libavutil           \
		$(LOCAL_PATH)/../../lume/libavcodec          \
		$(LOCAL_PATH)/../../lume/libmpcodecs         \
		$(LOCAL_PATH)/../../lume/libmpeg2	           \
		$(LOCAL_PATH)/../../lume	                   \
		$(LOCAL_PATH)/../../	                   \
		$(LOCAL_PATH)/../../lume/libjzcommon         \
		hardware/ingenic/xb4780/core/libdmmu \
		hardware/ingenic/xb4780/xbdemux/lume/stream \
		hardware/ingenic/xb4780/xbdemux/lume/libmpdemux \
		$(LOCAL_PATH)/../include

LOCAL_CFLAGS := \
        -DUSE_IPU_THROUGH_MODE $(XBOMX_FLAGS)

LOCAL_MODULE_TAGS := optional

ifdef LUME_VIDEO_MAX_WIDTH
  LOCAL_CFLAGS += -DLUME_VIDEO_MAX_WIDTH=$(LUME_VIDEO_MAX_WIDTH)
else
  LOCAL_CFLAGS += -DLUME_VIDEO_MAX_WIDTH=1280
endif

ifdef LUME_VIDEO_MAX_HEIGHT
  LOCAL_CFLAGS += -DLUME_VIDEO_MAX_HEIGHT=$(LUME_VIDEO_MAX_HEIGHT)
else
  LOCAL_CFLAGS += -DLUME_VIDEO_MAX_HEIGHT=720
endif

LOCAL_MODULE := libstagefright_vlume_codec
include $(BUILD_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
#endif
