#ifndef __MCE_H__
#define __MCE_H__

#include "config_jz_soc.h"

#define MCE_END_FLAG (0x80000000 | 0xFFFF)

typedef struct{
    uint32_t dha[200/4];//FIXME: define each int32; how large actually tasks?
}MCE_TASK_CHAIN;


typedef struct{
           //name    bits     Description
  uint32_t mvx      : 8;    //7:0    Frame height in pixel
  uint32_t mvy      : 8;    //15:8   Writing has no effect, read as zero
  uint32_t cost     : 16;   //31:16  Frame width in pixel
}MV_COST_BITS;

typedef union{
    uint32_t all;
    MV_COST_BITS bit;
}MV_COST;

typedef MV_COST MV_COST_REG;

typedef volatile struct{
    MV_COST mv_cost_bak;      //backup from tcsm1->mv_cost for entropy encoding

    MV_COST *mv_cost_ptr;

    uint8_t *raw_data_y_ptr;      //store in sram

  //uint8_t *raw_data_uv;     //store in sram

  //#ifndef MCE_OFA  
    uint8_t pred_out_y[256];  //FIXEME: when MCE_OFA = 1 mode, NO need these
    uint8_t pred_out_u[64];
    uint8_t pred_out_v[64];
  //#endif

    MCE_TASK_CHAIN task_chain;    
  //uint32_t end_flag;
}MCE_VAR;

int verify_mce_output();

int verify_mce_mv_cost( int8_t hw_mvx, int8_t hw_mvy, int16_t hw_cost, 
		        int8_t sw_mvx, int8_t sw_mvy, int16_t sw_cost, uint32_t *mce_end_flag_ptr);

typedef struct motion_config_t{
  uint32_t work_mode;
  uint8_t mb_height;
  uint8_t mb_width;
  uint16_t linesize; 

  //uint8_t* luma_ref_origin_addr[2][16];
  //uint8_t* chroma_ref_origin_addr[2][16];

  uint8_t* luma_ref_addr[2][2][16];
  uint8_t* chroma_ref_addr[2][2][16];

  int8_t luma_weight[2][16];
  int8_t luma_offset[2][16];

  int8_t chroma_weight[2][16][2];
  int8_t chroma_offset[2][16][2];
  uint8_t implicit_weight[16][16];
  uint8_t use_weight;
  uint8_t luma_denom;
  uint8_t use_weight_chroma;
  uint8_t chroma_denom;

  int ref_addr_index;
}motion_config_t;


#endif
