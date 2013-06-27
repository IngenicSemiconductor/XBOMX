LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

MPTOP := ../libfaad2/
LUME_PATH := $(LUME_TOP)
include $(LUME_PATH)/codec_config.mak

MLOCAL_SRC_FILES = bits.c \
              cfft.c \
              common.c \
              decoder.c \
              drc.c \
              drm_dec.c \
              error.c \
              filtbank.c \
              hcr.c \
              huffman.c \
              ic_predict.c \
              is.c \
              lt_predict.c \
              mdct.c \
              mp4.c \
              ms.c \
              output.c \
              pns.c \
              ps_dec.c \
              ps_syntax.c  \
              pulse.c \
              rvlc.c \
              sbr_dct.c \
              sbr_dec.c \
              sbr_e_nf.c \
              sbr_fbt.c \
              sbr_hfadj.c \
              sbr_hfgen.c \
              sbr_huff.c \
              sbr_qmf.c \
              sbr_syntax.c \
              sbr_tf_grid.c \
              specrec.c \
              ssr.c \
              ssr_fb.c \
              ssr_ipqf.c \
              syntax.c \
              tns.c \

MXU_SRC_FILES = 

JZC_CFG = 

MY_SRC_FILES := $(addprefix $(MPTOP),$(MLOCAL_SRC_FILES))

MY_SRC_FILES_SORT = $(sort $(MY_SRC_FILES) )

MXU_SRC_FILES_PATH = $(addprefix $(MPTOP),$(MXU_SRC_FILES))
LOCAL_MXU_SRC_FILES =$(sort $(MXU_SRC_FILES_PATH) )

LOCAL_SRC_FILES = $(filter-out $(LOCAL_MXU_SRC_FILES),$(MY_SRC_FILES_SORT))

LOCAL_MODULE := libstagefright_faad2
JZC_CFG = $(LUME_PATH)/libjzcommon/com_config.h

LOCAL_CFLAGS := $(PV_CFLAGS) -DHAVE_CONFIG_H -DHAVE_AV_CONFIG_H -ffunction-sections  -Wmissing-prototypes -Wundef -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -UNDEBUG -UDEBUG -fno-builtin -D_GNU_SOURCE -DFIXED_POINT -imacros $(JZC_CFG)

LOCAL_MXU_CFLAGS = $(LOCAL_CFLAGS)
LOCAL_MXU_ASFLAGS = $(LOCAL_CFLAGS)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(LUME_PATH)/libavcodec  \
	$(LUME_PATH)/libfaad2  \
	$(LUME_PATH)  \
	$(LUME_PATH)/libavutil \
	$(LUME_PATH)/libmpcodecs \
	$(PV_INCLUDES)


LOCAL_COPY_DEPENDS_TO := lib
LOCAL_COPY_DEPENDS := 

include $(BUILD_STATIC_LIBRARY)
