LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
XBOMX_FLAGS := -D_GNU_SOURCE -DIPU_4780BUG_ALIGN=2048
#include $(call all-makefiles-under,$(LOCAL_PATH))
AV_INCLUDES := $(TOP)/frameworks/av/include/
LUME_TOP := $(LOCAL_PATH)

PV_CFLAGS := -DUSE_IPU_THROUGH_MODE $(XBOMX_FLAGS)

include $(LUME_TOP)/mk/avutil/Android.mk
include $(LUME_TOP)/mk/avcore/Android.mk

include $(LUME_TOP)/mk/avcodec/audio.mk
include $(LUME_TOP)/mk/avcodec/video.mk
include $(LUME_TOP)/mk/mpcommon.mk
include $(LUME_TOP)/mk/hwapp.mk
include $(LUME_TOP)/mk/cook.mk
include $(LUME_TOP)/mk/libh264.mk
include $(LUME_TOP)/mk/librv9.mk
include $(LUME_TOP)/mk/libvc1.mk
include $(LUME_TOP)/mk/libmp3.mk
include $(LUME_TOP)/mk/libmpeg2.mk
include $(LUME_TOP)/mk/libmpeg4.mk
include $(LUME_TOP)/mk/libfaad2.mk
include $(LUME_TOP)/mk/libjzmpeg2.mk