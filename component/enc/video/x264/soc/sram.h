#ifndef __SRAM_H__
#define __SRAM_H__

#include "config_jz_soc.h"

#define RAW_DATA_PIPE_DEP (4) //min need is 3, use 4 for opt update index

typedef struct{
  /* aligning the first member is a gcc hack to force the struct to be
   * 256 byte aligned, as well as force sizeof(struct) to be a multiple of 256 */

  //Do NOT seperate the two below
  ALIGNED_256(uint8_t y[256]);
  uint8_t uv[128];    
  //uint8_t for_align[128];
}ALIGNED_256(RAW_DATA);


typedef volatile  struct{
  RAW_DATA raw_data[RAW_DATA_PIPE_DEP];

  uint32_t raw_yuv422_data[RAW_DATA_PIPE_DEP][16][8];

}SRAM;

#endif//__SRAM_H__
