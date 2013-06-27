LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
MPTOP := ../
LUME_PATH := $(LUME_TOP)
MLOCAL_SRC_FILES := jz47_vae_map.cpp libjzcommon/jzm_intp.c


LOCAL_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES)) 
LOCAL_MODULE := libstagefright_mphwapp

JZC_CFG := jzconfig.h
LOCAL_CFLAGS := $(PV_CFLAGS) -DHAVE_AV_CONFIG_H -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -imacros $(JZC_CFG)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH)  \
	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := \

include $(BUILD_STATIC_LIBRARY)
