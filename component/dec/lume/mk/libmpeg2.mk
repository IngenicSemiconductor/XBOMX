LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
MPTOP := ../libmpeg2/
LUME_PATH := $(LUME_TOP)

#	LOCAL_MXU_SRC_FILES = file.c
#
#	LOCAL_MXU_CFLAGS = 
#
#	LOCAL_MXU_ASFLAGS = 

MLOCAL_SRC_FILES :=  slice.c alloc.c  idct.c motion_comp.c\
		  cpu_accel.c cpu_state.c decode.c  header.c jzm_mpeg2_dec.c

JZC_CFG = $(LUME_PATH)/libjzcommon/com_config.h
LOCAL_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES)) 
LOCAL_MODULE := libstagefright_mpeg2

LOCAL_CFLAGS := $(PV_CFLAGS) -DHAVE_AV_CONFIG_H -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-return-type -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -DVIDEO_CODEC -imacros $(JZC_CFG) -D_REENTRANT -D_LARGEFILE_SOURCE  -DHAVE_CONFIG_H -DHAVE_AV_CONFIG_H  -D_ISOC9X_SOURCE


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH)/libavcodec  \
	$(LUME_PATH)/libmpeg2  \
	$(LUME_PATH)  \
	$(LUME_PATH)/libavutil \
	$(LUME_PATH)/libmpcodecs \
	$(PV_INCLUDES)

include $(BUILD_STATIC_LIBRARY)
