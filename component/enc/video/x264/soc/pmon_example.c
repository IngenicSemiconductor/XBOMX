/* ===================================================================== */
/*  MAIN CPU PMON Macros                                                 */
/* ===================================================================== */
#include <sys/types.h>

#include "jzasm.h"
#include "config_jz_soc.h"
//#include "jz47xx_pmon.h"

#if defined(JZC_PMON_P0) || defined(JZC_PMON_P1)

#define PMON_GETRC() i_mfc0_2(16, 6)
#define PMON_GETLC() i_mfc0_2(16, 5)
#define PMON_GETRH() (i_mfc0_2(16, 4) & 0xFFFF)
#define PMON_GETLH() ((i_mfc0_2(16, 4)>>16) & 0xFFFF)

static unsigned int pmon_val; 
static unsigned int pmon_val_ex; 

void ALWAYS_INLINE pmon_reset(){
  pmon_val = 0; 
  pmon_val_ex = 0; 
}

unsigned int  ALWAYS_INLINE get_pmon_val(){
  return pmon_val;
}

unsigned int  ALWAYS_INLINE get_pmon_val_ex(){
  return pmon_val_ex;
}

#ifdef STA_CCLK
void ALWAYS_INLINE pmon_on(){
    i_mtc0_2(0, 16, 4); 
    i_mtc0_2(0, 16, 5);
    i_mtc0_2(0, 16, 6);
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x0100, 16, 7);   
}

void ALWAYS_INLINE pmon_off(){
  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); 
  pmon_val += PMON_GETRC();
  pmon_val_ex += PMON_GETLC();
}


#elif defined(STA_DCC)
void ALWAYS_INLINE pmon_on(){
    i_mtc0_2(0, 16, 4);
    i_mtc0_2(0, 16, 5);
    i_mtc0_2(0, 16, 6);
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x1100, 16, 7); 
}

void ALWAYS_INLINE pmon_off(){
  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); 
  pmon_val += PMON_GETRC();		\
  pmon_val_ex += PMON_GETLC();s
}


#elif defined(STA_ICC)
void ALWAYS_INLINE pmon_on(){
    i_mtc0_2(0, 16, 4);
    i_mtc0_2(0, 16, 5);
    i_mtc0_2(0, 16, 6);
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x1100, 16, 7);	
}

void ALWAYS_INLINE pmon_off(){
  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7);
  pmon_val += PMON_GETRC();
  pmon_val_ex += PMON_GETLC();

}


#elif defined(STA_INSN)
void ALWAYS_INLINE pmon_on(){
    i_mtc0_2(0, 16, 4);
    i_mtc0_2(0, 16, 5);
    i_mtc0_2(0, 16, 6);
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x2100, 16, 7); 
}

void ALWAYS_INLINE pmon_off(){
  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7);
  pmon_val += PMON_GETRC();
  pmon_val_ex += PMON_GETLC();
}


#elif defined(STA_UINSN) /*useless instruction*/
void ALWAYS_INLINE pmon_on(){
    i_mtc0_2(0, 16, 4);	
    i_mtc0_2(0, 16, 5);
    i_mtc0_2(0, 16, 6);
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x2100, 16, 7); 
}

void ALWAYS_INLINE pmon_off(){
  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7);
  pmon_val += PMON_GETRC();
  pmon_val_ex += PMON_GETLC();
}


#elif defined(STA_TLB) /*useless instruction*/
void ALWAYS_INLINE pmon_on(){
    i_mtc0_2(0, 16, 4);
    i_mtc0_2(0, 16, 5);
    i_mtc0_2(0, 16, 6);
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x3100, 16, 7); 
}

void ALWAYS_INLINE pmon_off(){
  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7);
  pmon_val += PMON_GETRC();
  pmon_val_ex += PMON_GETLC();
}

#else
#error "must define one of STA_CCLK, STA_DCC ..."
#endif

#else //!(defined(JZC_PMON_P0) || defined(JZC_PMON_P1))

void ALWAYS_INLINE pmon_reset(){}

void ALWAYS_INLINE pmon_on(){}

void ALWAYS_INLINE pmon_off(){}

void ALWAYS_INLINE get_pmon_val(){}
void ALWAYS_INLINE get_pmon_val_ex(){}


#endif //(defined(JZC_PMON_P0) || defined(JZC_PMON_P1))
