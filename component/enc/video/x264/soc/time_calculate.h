#ifndef TIME_CAL_H
#define TIME_CAL_H

#define P2
#define STA_CCLK2

#include "stdint.h"
#include "jz47p2_pmon.h"

//extern uint64_t func##_cycle;

#define START(func)   \
  extern uint32_t func##_cycle;\
  extern int func##_calls;  \
  PMON_CRE(func);     \
  PMON_START(func);

#define END(func)                 \
  PMON_END(func);                   \
  func##_cycle += func##_pm_val;  \
  func##_calls++;                   \
  PMON_CLE(func);

//extern uint64_t func##_cycle;    

#define EXTERN_STA(func)           \
  extern uint32_t func##_cycle;    \
  extern int func##_calls         
  
#define TOTAL_MB ( param.i_width * param.i_height * param.i_frame_total / 256 )


#define FORMAT_PRINT(func, times)\
    if( func##_calls == times )\
    {\
        printf(#func" per call(average of-"#times"-calls):%u pmon\n", func##_cycle / times );\
        func##_cycle = 0;\
        func##_calls = 0;\
    }\

#endif//time_cal

/*
---------------------------------------------------
Use following code inserting around the target func:

#ifdef P2 
#include "time_calculate.h"
START_C(func)
#endif


#ifdef JZC_PMON_P2 
END_C(func)
#endif

---------------------------------------------------

*/
