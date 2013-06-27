LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        HardOMXComponent.cpp \
	SimpleHardOMXComponent.cpp

LOCAL_C_INCLUDES += \
	$(TOP)/frameworks/native/include/media/hardware \
        $(TOP)/frameworks/native/include/media/openmax

LOCAL_SHARED_LIBRARIES :=               \
        libbinder                       \
        libmedia                        \
        libutils                        \
        libui                           \
        libcutils                       \
        libstagefright_foundation       \
        libdl

LOCAL_MODULE:= libOMX_Basecomponent

include $(BUILD_STATIC_LIBRARY)
