#ifndef __JZ47P2_PMON_H__
#define __JZ47P2_PMON_H__
/* ===================================================================== */
/*  MAIN CPU PMON Macros                                                 */
/* ===================================================================== */
#include "jzasm.h"
#include "../soc/jz47p2_pmon.h"

#ifdef P2

#define PMON_GETRC2() i_mfc0_2(16, 6)
#define PMON_GETLC2() i_mfc0_2(16, 5)
#define PMON_GETRH2() (i_mfc0_2(16, 4) & 0xFFFF)
#define PMON_GETLH2() ((i_mfc0_2(16, 4)>>16) & 0xFFFF)

#ifdef STA_CCLK2
#define PMON_START(func)\
  ({  i_mtc0_2(0, 16, 4);\
    i_mtc0_2(0, 16, 5);\
    i_mtc0_2(0, 16, 6);\
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x0100, 16, 7);	\
  })
#define PMON_END(func)  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); func##_pm_val += PMON_GETRC2(); \
  func##_pm_val_ex += PMON_GETLC2();
#endif//STA_CCLK2

#define PMON_CRE(func)   \
  uint32_t func##_pm_val = 0; \
  uint32_t func##_pm_val_ex = 0; \
  unsigned long long func##_pm_val_1f = 0;

#define PMON_CLE(func)   \
  func##_pm_val = 0;	   \
  func##_pm_val_ex = 0;

#endif //P2

#endif /*__JZ47P2_PMON_H__*/
