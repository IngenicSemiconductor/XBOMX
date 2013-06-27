#ifndef __TCSM0_H__
#define __TCSM0_H__

#include <linux/types.h>

#include "config_jz_soc.h" 
#include "vmau.h"

#define VMAU_RES_FIFO_DEP AUX_FIFO_DEP

typedef volatile struct{
  unsigned char y[256];
  unsigned char uv[128];
}RECON_MB_PXL;


typedef struct{
  VMAU_CBP_RES vmau_cbp_res_array[VMAU_RES_FIFO_DEP];

  RECON_MB_PXL recon_mb_pxl_array[VMAU_PIPE_DEP];
}TCSM0;


#endif //__TCSM0_H__
