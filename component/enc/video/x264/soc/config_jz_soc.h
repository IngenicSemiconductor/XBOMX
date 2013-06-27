
#ifndef __CONFIG_JZ_SOC_H__
#define __CONFIG_JZ_SOC_H__

#define DECLARE_ALIGNED( var, n ) var __attribute__((aligned(n)))
#define ALIGNED_1K( var )  DECLARE_ALIGNED( var, 1024 )
#define ALIGNED_256( var ) DECLARE_ALIGNED( var, 256 )
#define ALIGNED_16( var )  DECLARE_ALIGNED( var, 16 )
#define ALIGNED_8( var )   DECLARE_ALIGNED( var, 8 )
#define ALIGNED_4( var )   DECLARE_ALIGNED( var, 4 )

#define ALWAYS_INLINE __attribute__((always_inline)) inline


#define HW_4780
//#define CRC_CHECK
/***********   PMON   ****************************/ 
//#define JZC_PMON_P0

#define STA_CCLK
//#define STA_DCC
//#define JZC_PMON_P1

#ifdef JZC_PMON_P0
#include "jz4760e_pmon.h"
//#include "jz47xx_pmon.h"
#endif //JZC_PMON_P0

#if defined(JZC_PMON_P0) && defined(JZC_PMON_P1)
#error JZC_PMON_P0 and JZC_PMON_P1 can define only one 
#endif

#ifdef JZC_PMON_P0
#if defined(STA_INSN)
# define PMON_P0_FILE_NAME "jz4760e_pmon_p0.insn"
#elif defined(STA_UINSN)
# define PMON_P0_FILE_NAME "jz4760e_pmon_p0.insn"
#elif defined(STA_CCLK)
# define PMON_P0_FILE_NAME "jz4760e_pmon_p0.cclk"
#elif defined(STA_DCC)
# define PMON_P0_FILE_NAME "jz4760e_pmon_p0.cc"
#elif defined(STA_ICC)
# define PMON_P0_FILE_NAME "jz4760e_pmon_p0.cc"
#elif defined(STA_TLB)
# define PMON_P0_FILE_NAME "jz4760e_pmon_p0.tlb"
#else
#error "If JZC_PMON_P0 defined, one of STA_INSN/STA_CCLK/STA_DCC/STA_ICC must be defined!"
#endif
#endif // JZC_PMON_P0

   
#endif//__CONFIG_JZ_SOC_H__
