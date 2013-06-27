LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	OMX_Core.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libOMX_Core

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := 
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils \

LOCAL_C_INCLUDES := \
	$(TOP)/frameworks/native/include/media/openmax

include $(BUILD_SHARED_LIBRARY)
