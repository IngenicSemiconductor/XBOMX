LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LUME_PATH := $(TOP)/hardware/ingenic/xb4780/xbomx/component/dec/lume/

LOCAL_SRC_FILES := \
		lume_audio_dec.cpp \
		lume_decoder.cpp \
		faad_decoder.cpp \
		mp3_decoder.cpp \
		pcm_decoder.cpp \
	        dvdpcm_decoder.cpp

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/../../include \
		$(LUME_PATH)/libmpcodecs	    \
		$(LUME_PATH)	    \
		$(LUME_PATH)/libavcodec	    \
		$(LUME_PATH)/libavutil	    \
		$(LUME_PATH)/libaf	    \
		$(LUME_PATH)/libfaad2	    \
		$(LUME_PATH)/libjzcommon	\
		$(TOP)/hardware/ingenic/xb4780/xbdemux/lume \
		$(TOP)/hardware/ingenic/xb4780/xbdemux/lume/stream \
		$(TOP)/hardware/ingenic/xb4780/xbdemux/lume/libmpdemux \
		$(TOP)/frameworks/av/media/libstagefright/include \
		$(TOP)/frameworks/native/include/media/openmax

LOCAL_CFLAGS := \
        $(STAGEFRIGHT_FLAGS) \
	-DOSCL_UNUSED_ARG=


LOCAL_MODULE := libstagefright_alume_codec

include $(BUILD_STATIC_LIBRARY)

