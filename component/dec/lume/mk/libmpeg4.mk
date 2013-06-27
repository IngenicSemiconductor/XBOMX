LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
MPTOP := ../libmpeg4/
LUME_PATH := $(LUME_TOP)

#	LOCAL_MXU_SRC_FILES = file.c
#
#	LOCAL_MXU_CFLAGS = 
#
#	LOCAL_MXU_ASFLAGS = 

MLOCAL_SRC_FILES := mpeg4.c mpeg4_p0.c msmpeg4_p0.c 

JZC_CFG = $(LUME_PATH)/libmpeg4/mpeg4_config.h
JZC_CONFIG = jzconfig.h
LOCAL_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES)) 
LOCAL_MODULE := libstagefright_mpeg4

LOCAL_CFLAGS := $(PV_CFLAGS) -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -DVIDEO_CODEC -D__LINUX__ -imacros $(JZC_CFG) -D_LITTLE_ENDIAN=1 -fomit-frame-pointer -DARCH_IS_32BIT -DARCH_IS_LITTLE_ENDIAN -imacros $(JZC_CONFIG) -D_REENTRANT -D_LARGEFILE_SOURCE  -DHAVE_CONFIG_H -DHAVE_AV_CONFIG_H  -D_ISOC9X_SOURCE


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH)/libavcodec  \
	$(LUME_PATH)/libmpeg4/  \
	$(LUME_PATH)/libmpeg4/jzsoc  \
	$(LUME_PATH)  \
	$(LUME_PATH)/libavutil \
	$(LUME_PATH)/libmpcodecs \
	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := 	../libmpeg4/mpeg4.h

include $(BUILD_STATIC_LIBRARY)
LOCAL_COPY_DEPENDS_TO := etc
LOCAL_COPY_DEPENDS := $(LUME_PATH)/libmpeg4/jzsoc/mpeg4_p1.bin
include $(BUILD_COPY_DEPENDS)
