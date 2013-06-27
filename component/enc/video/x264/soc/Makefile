include ../config.mak

LD 		= mipsel-linux-ld
#CC 		= mipsel-linux-gcc 
COPY		= mipsel-linux-objcopy

#CC 		= $(ICSYS_DIR)./mipsisa32-gcc-3.3
#LD		= $(ICSYS_DIR)./mipsisa32-ld
#COPY		= $(ICSYS_DIR)./mipsisa32-objcopy

#P1_SRC		= x264_p1.c
P1_BIN		= x264_p1.bin
P1_LIB		= x264_p1.lib
P1_OBJ		= x264_p1.o 
P1_MID		= $(P1_OBJ:.o=.mid.s)
P1_ASM		= $(P1_OBJ:.o=.s)


GCC_ENDIAN_OPT	= -mel
LD_ENDIAN_OPT	= -EL
ABI_DIR		= el/mabi32

#GCC_EXTRA_OPT   = -Wall -Wno-switch -mips32 -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer
GCC_EXTRA_OPT   = -Wall -Wno-switch -mips32 -mno-abicalls -fno-pic -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer

#============================= Internal macro define 

ifeq ($(JZ47_LINUX),1)
else
GCC_EXTRA_OPT += -DMINIOS
endif

GCC_OPT		= $(GCC_ENDIAN_OPT)  $(GCC_EXTRA_OPT) -fno-schedule-insns -fno-schedule-insns2

#============================= Make action define
libs: $(P1_BIN)

$(P1_OBJ):%.o:%.mid.s
	$(CC) $(GCC_OPT) -c -o 	$@ 	$<
$(P1_MID):%.mid.s:%.s
	source jz47xx_p1_gp $< >| tmp
	mxu_as tmp >| $@
	rm -f tmp
$(P1_ASM):%.s:%.c
	$(CC) $(GCC_OPT) -S -o 	$@ 	$<

$(P1_LIB): $(P1_OBJ) 
#	$(LD) -T x264_p1.ld -EL -nostdlib -o $@ $^
	$(LD) -T jz47xx_p1.ld -static -o $@ $^ -lc
$(P1_BIN): $(P1_LIB)
	$(COPY) -O binary $< 	$@


#auto dep
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ | sed 's/\.o/\.s/g' > $@; \
	rm -f $@.$$$$;

sources = x264_p1.c
sinclude $(sources:.c=.d)

clean::
	rm -f *.o *~ *.s *.bin *.lib *.d*

.PHONY: libs clean
