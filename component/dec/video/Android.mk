LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	HWDec.cpp \
	HardwareRenderer_FrameBuffer.cpp \
	../../../../xbdemux/LUMEDefs.cpp

LOCAL_SHARED_LIBRARIES := \
	libstagefright \
	libstagefright_omx \
	libstagefright_foundation \
	libcutils	\
	libutils \
	libbinder	\
	libdl \
	libui \
	libjzipu \
	libdmmu

LOCAL_STATIC_LIBRARIES := \
	libstagefright_vlume_codec

LOCAL_STATIC_LIBRARIES += \
		libstagefright_mpeg2 \
		libstagefright_vlumedecoder \
		libstagefright_vlumevc1  \
		libstagefright_realvideo  \
		libstagefright_mpeg4  \
		libstagefright_vlumeh264  \
		libstagefright_ffmpcommon \
		libstagefright_ffavutil  \
		libstagefright_ffavcore  \
		libOMX_Basecomponent \
		libstagefright_mphwapp\
	        libstagefright_jzmpeg2 


LOCAL_MODULE := libstagefright_hard_vlume

LOCAL_C_INCLUDES := $(LOCAL_PATH)/./include \
	$(LOCAL_PATH)/../include \
	frameworks/av/media/libstagefright/include \
	frameworks/native/include/utils \
	frameworks/native/include/media/openmax \
	frameworks/native/include/media/hardware   \
	hardware/ingenic/xb4780/libdmmu \
	hardware/ingenic/xb4780/libjzipu \
        hardware/ingenic/xb4780/xbdemux/lume/stream \
        hardware/ingenic/xb4780/xbdemux/lume/libmpdemux \
	hardware/ingenic/xb4780/xbomx/component/common \
        hardware/ingenic/xb4780/xbomx/component/dec/video/lume_video/include \
        hardware/ingenic/xb4780/xbomx/component/dec/lume/libavutil \
        hardware/ingenic/xb4780/xbomx/component/dec/lume/libavcodec \
        hardware/ingenic/xb4780/xbomx/component/dec/lume/libmpcodecs \
        hardware/ingenic/xb4780/xbomx/component/dec/lume/libjzcommon \
        hardware/ingenic/xb4780/xbomx/component/dec/lume/

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))