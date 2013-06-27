#include "stdint.h"

 // uint64_t func##_cycle = 0;
#define VAR_C(func) \
  uint32_t func##_cycle = 0;\
  int func##_calls = 0

VAR_C(lr16_o);
VAR_C(lr16);
VAR_C(bs);
VAR_C(vlc);
VAR_C(last4);
VAR_C(cavlc);
VAR_C(ones);
VAR_C(ones_fi);
VAR_C(rcmb);
VAR_C(mball);
VAR_C(zigzag);


VAR_C(ad);
VAR_C(store_tcsm);
VAR_C(load_tcsm);
VAR_C(store_tmp);
VAR_C(tmp);



VAR_C(mb_intra_analy);



