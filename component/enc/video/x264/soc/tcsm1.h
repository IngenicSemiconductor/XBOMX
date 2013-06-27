#ifndef __TCSM1_H__
#define __TCSM1_H__

#include <linux/types.h>

#include "config_jz_soc.h"  //for vmau fifo dep define

#include "tcsm0.h"
#include "sram.h"
#include "vmau.h"
#include "jz477x_mce.h"

#define X264_SCAN8_SIZE (6*8)
#define X264_SCAN8_0 (4+1*8)

typedef struct{
  vmau_reg *vmau_reg_phy;
  TCSM0 *tcsm0_phy;
  SRAM *sram_phy;
  void *tcsm1_phy_addr;

  int i_slice_type;

  int i_mb_stride;

  int i_mb_width;
  int i_mb_height;

  int i_first_mb;
  int i_last_mb;

  int i_fmv_range;

  int vmau_quant_para;

  int i_enc_stride[3];//raw data stride
  int i_dec_stride;   //reconstructed pxl stride

  uint8_t *raw_plane_y_phy_ptr;
  uint8_t *raw_plane_u_phy_ptr;
  uint8_t *raw_plane_v_phy_ptr;

  uint8_t *recon_pxl_y_phy_ptr;//reconstructed
  uint8_t *recon_pxl_uv_phy_ptr;
  int ref_frame_index;

  uint8_t *raw_yuv422_phy_ptr;
  int i_csp;          //add #define X264_CSP_YUYV 0x0005  /* yuv 4:2:2 packed */ for android camera
}X264_SLICE_VAR;

typedef struct{
  int i_mb_x;
  int i_mb_y;
  int i_mb_xy;

  unsigned int i_neighbour;

  volatile int i_intra16x16_pred_mode;
  volatile int i_chroma_pred_mode;

  ALIGNED_4( int16_t pskip_mv[2] );

  volatile ALIGNED_4( int16_t mvd[2]);
}X264_MB_VAR;

typedef struct{
  ALIGNED_4( int16_t mv[2]);
  ALIGNED_4( int16_t mvp[2]);

  /* Allowed qpel MV range to stay within the picture + emulated edge pixels */
  int     mv_min[2];
  int     mv_max[2];
  /* Subpel MV range for motion search.
  * same mv_min/max but includes levels' i_mv_range. */
  int     mv_min_spel[2];
  int     mv_max_spel[2];
  /* Fullpel MV range for motion search */
  int     mv_min_fpel[2];
  int     mv_max_fpel[2];
}X264_MV_VAR;

typedef volatile struct{
  void *src_phy_addr;
  void *dst_phy_addr;
  uint32_t stride;      //FIXME: define bit field
  uint32_t link_flag;
}DMA_TASK_NODE;

#define DMA_TASK_CHAIN_NODE_NUM (4)

typedef volatile struct{
  DMA_TASK_NODE node[DMA_TASK_CHAIN_NODE_NUM];
}DMA_TASK_CHAIN;

#define TOP_LINE_MV_NUM (1280/16 + 4)

typedef struct{
  uint32_t aux_text_bss[1024 + 1024];//FIXME: use link _bss_end para to decide aux_text_bss len

  ALIGNED_1K(MV_COST mv_cost);

  MCE_VAR mce_var[MCE_PIPE_DEP];
  volatile int mce_end_flag;

  uint32_t raw_yuv422_data[RAW_DATA_PIPE_DEP][16][8];
  RAW_DATA raw_data[DMA_PIPE_DEP];

  VMAU_VAR vmau_var[VMAU_PIPE_DEP];
  volatile int vmau_dec_end_flag;
  volatile int vmau_enc_end_flag;    


  int pipe_index;
  int vmau_pipe_index;

  int aux_fifo_index;
  int mce_aux_fifo_index;
  int last_mce_aux_fifo_index;
  int vmau_aux_fifo_index;
  int gp0_aux_fifo_index;
  int gp1_aux_fifo_index;


  volatile int aux_fifo_read_num;
  volatile int aux_fifo_write_num;


  DMA_TASK_CHAIN gp0_task_chain_array[DMA_PIPE_DEP];
  DMA_TASK_CHAIN gp1_task_chain_array[DMA_PIPE_DEP];
  DMA_TASK_CHAIN gp2_task_chain_array[DMA_PIPE_DEP];
  int gp0_pipe_index;
  int gp1_pipe_index;
  int gp2_pipe_index;

  int gp1_raw_data_index;
  int gp2_raw_data_index;
  int mce_raw_data_index;
  int vmau_raw_data_index;

  int i_next_mb_x;
  int i_next_mb_y;
  //int i_next_mb_xy;

  int i_gp1_mb_x;
  int i_gp1_mb_y;


  X264_SLICE_VAR slice_var;

  //X264_MB_VAR mb;
  X264_MB_VAR mb_array[AUX_FIFO_DEP];

  volatile int fifo_left_num;

  X264_MV_VAR mv_var;

  ALIGNED_4( int16_t  left_mv[2]);
  ALIGNED_4( int16_t  topright_mv[2]);

  //ALIGNED_4( int16_t top_line_mv[TOP_LINE_MV_NUM * 2]);
  int32_t top_line_mv[TOP_LINE_MV_NUM];

  volatile int debug_buf[10];

  int rsvd_stack[256 + 64];
}TCSM1;

 
//#if (sizeof(TCSM1) > TCSM1__SIZE)
//#error TCSM1 out of limit
//#endif

#endif //__TCSM1_H__

 
