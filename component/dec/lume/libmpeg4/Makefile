################################## PLATFORM #######################################
JZC_LINUX 	= 1
JZC_UCOS 	= 0
JZC_FPGA		= 0
JZC_RTL		= 0

################################## TOOL CHAIN #####################################
AR 		= mipsel-linux-ar
LD 		= mipsel-linux-ld
CC 		= mipsel-linux-gcc 
RANLIB 		= mipsel-linux-ranlib

CFLAGS 		= -mips32 -O2 -fno-builtin \
		  -fno-exceptions -ffunction-sections -falign-functions=32 \
	   	  -fshort-wchar -fomit-frame-pointer -ffast-math -msoft-float -G 0 -Wall \
                     -mlong-calls -Wdisabled-optimization -Wno-switch \
		  -Wpointer-arith -Wredundant-decls -pipe -fomit-frame-pointer \
                     -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
                     -D_LARGEFILE64_SOURCE -DHAVE_CONFIG_H -DHAVE_AV_CONFIG_H \
                     -D_ISOC9X_SOURCE

ifeq 		($(JZC_LINUX),1)
CFLAGS		+= -fPIC -std=c99
CFLAGS		+= -D_LINUX_
endif

ifeq 		($(JZC_UCOS),1)
CFLAGS             += -mno-abicalls -fno-pic
CFLAGS		+= -D_UCOS_ 
endif

ifeq 		($(JZC_FPGA),1)
CFLAGS		+= -fPIC 
CFLAGS		+= -D_FPGA_TEST_
endif

ifeq 		($(JZC_RTL),1)
CFLAGS		+= -fPIC 
CFLAGS		+= -D_RTL_SIM_
endif

################################## COMPILE  DEF #####################################
SRCS_DIR 	= .
REL_DIR 		=./rel

JZC_CFG 		= mpeg4_config.h
LIB		= libmpeg4.a

SRCS_VD_VC1 	= $(SRCS_DIR)/mpeg4.c $(SRCS_DIR)/mpeg4_p0.c $(SRCS_DIR)/msmpeg4_p0.c 
#$(SRCS_DIR)/mpeg4_p1.c
OBJS_VD_VC1 	= $(addsuffix .o, $(basename $(SRCS_VD_VC1)) )

OBJS 		= $(OBJS_VD_VC1)
MIDS_VD_VC1 	= $(OBJS_VD_VC1:.o=.s)
ASMS_VD_VC1 	= $(OBJS_VD_VC1:.o=.mid)

CFLAGS 		+= -imacros $(JZC_CFG) -I. -I.. \
                   -I../libavcodec -I../libavutil -I./jzsoc/

STATIC_OBJS 	:= $(OBJS)

#EXT_BIN_DIR 	:= ./jzsoc/
#EXT_BIN_NAME 	:= vc1_aux_task.bin
#EXT_BIN		= $(join $(EXT_BIN_DIR),$(EXT_BIN_NAME))

all: 		$(REL_DIR) $(LIB) $(EXT_BIN) Makefile
		@test -d $(REL_DIR)
#		@cp $(EXT_BIN) $(REL_DIR) -f
#		cp $(LIB) $(REL_DIR)

$(LIB): 		$(STATIC_OBJS)
		rm -f $@
		$(AR) rc $@ $^ 
		$(RANLIB) $@

#$(EXT_BIN):
#		@for d in $(EXT_BIN_DIR);do make -C $$d;done

$(REL_DIR):
		test -d $(REL_DIR) || mkdir $(REL_DIR)

$(OBJS_VD_VC1):%.o:%.s
		$(CC) $(CFLAGS) -c -o $@ $<
$(MIDS_VD_VC1):%.s:%.mid
		mxu_as $< > $@
$(ASMS_VD_VC1):%.mid:%.c
		$(CC) $(CFLAGS) -S -o $@ $<

clean::
		rm -f *.o *~ *.a *.lib *.so *.so.* *.dylib *.dll *.s *.mid\
	      	*.def *.dll.a *.exp
		rm -fr $(REL_DIR)
#		@for d in $(EXT_BIN_DIR);do make -C $$d  clean;done
