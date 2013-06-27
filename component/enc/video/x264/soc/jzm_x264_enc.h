/****************************************************************
*****************************************************************/

#ifndef __JZM_X264_ENC_H__
#define __JZM_X264_ENC_H__
#include "jzm_vpu.h"

#define SCH_FIFO_DEPTH      16 

/************************************************************
 CHN Space Allocation
 ************************************************************/
#define VRAM_DUMMY          (VPU_BASE | 0xFFFFC)

#define VRAM_MAU_RESA       (VPU_BASE | 0xC0000)  //residual address

#define VRAM_RAWY_BA        (VPU_BASE | 0xF0000)
#define VRAM_RAWC_BA        (VRAM_RAWY_BA + 256)
#define VRAM_RAW_SIZE       (SCH_FIFO_DEPTH*128*4)

#define VRAM_TOPMV_BA       (VRAM_RAWY_BA+VRAM_RAW_SIZE)  //recover address
#define VRAM_TOPPA_BA       (VRAM_TOPMV_BA+VPU_MAX_MB_WIDTH*4)

#define VRAM_MAU_CHN_BASE   (VRAM_TOPPA_BA+VPU_MAX_MB_WIDTH*4)
#define VRAM_MAU_CHN_SIZE   (SCH_FIFO_DEPTH*16*4)
#define VRAM_DBLK_CHN_BASE  (VRAM_MAU_CHN_BASE + VRAM_MAU_CHN_SIZE)
#define VRAM_DBLK_CHN_SIZE  (SCH_FIFO_DEPTH*16*4)
#define VRAM_ME_CHN_BASE    (VRAM_DBLK_CHN_BASE + VRAM_DBLK_CHN_SIZE)
#define VRAM_ME_CHN_SIZE    (SCH_FIFO_DEPTH*8*4)
#define VRAM_SDE_CHN_BASE   (VRAM_ME_CHN_BASE + VRAM_ME_CHN_SIZE)
#define VRAM_SDE_CHN_SIZE   (SCH_FIFO_DEPTH*8*4)

#define VRAM_ME_DSA         DSA_SCH_CH1
#define VRAM_MAU_DEC_SYNA   DSA_SCH_CH2
#define VRAM_DBLK_CHN_SYNA  DSA_SCH_CH3
#define VRAM_SDE_SYNA       DSA_SCH_CH4

#define VRAM_MAU_ENC_SYNA   VRAM_DUMMY
#define VRAM_DBLK_DOUT_SYNA VRAM_DUMMY

#define VRAM_ME_MVPA        (VPU_BASE | REG_EFE_MVRP)

#define __ALN32__ __attribute__ ((aligned(4)))

/*
  _H264E_SliceInfo:
  H264 Encoder Slice Level Information
 */
typedef struct _H264E_SliceInfo {
  /*basic*/
  uint8_t frame_type;
  uint8_t mb_width;
  uint8_t mb_height;
  uint8_t first_mby;
  uint8_t last_mby;  //for multi-slice

  /*vmau scaling list*/
  uint8_t __ALN32__ scaling_list[4][16];

  /*loop filter*/
  uint8_t deblock;      // DBLK CTRL : enable deblock
  uint8_t rotate;       // DBLK CTRL : rotate
  int8_t alpha_c0_offset;   // cavlc use, can find in bs.h
  int8_t beta_offset;

  /*cabac*/   // current hw only use cabac, no cavlc
  uint8_t *state;
  unsigned int bs;          /* encode bitstream start address */
  uint8_t qp;

  /*frame buffer address: all of the buffers should be 256byte aligned!*/
  unsigned int fb[3][2];       /*{curr, ref, raw}{tile_y, tile_c}*/
  /* fb[0] : DBLK output Y/C address
   * fb[1] : MCE reference Y/C address
   * fb[2] : EFE input Y/C buffer address
   */

  /*descriptor address*/
  unsigned int * des_va, des_pa;

  /*TLB address*/
  unsigned int tlba;

}_H264E_SliceInfo;

typedef struct HwInfo{
    unsigned int * vdma_config;
    unsigned char * fb_ptr[3][2];     /*{curr, ref, raw}{tile_y, tile_c}*/
    unsigned char * bs_ptr;
    _H264E_SliceInfo H264E_SliceInfo;
}HwInfo_t;

__place_k0_data__
static uint32_t lps_range[64] = {
  0xeeceaefc,  0xe1c3a5fc,  0xd6b99cfc,  0xcbb094f2,
  0xc1a78ce4,  0xb79e85da,  0xad967ece,  0xa48e78c4,
  0x9c8772ba,  0x94806cb0,  0x8c7966a6,  0x8573619e,
  0x7e6d5c96,  0x7867578e,  0x72625386,  0x6c5d4e80,
  0x66584a78,  0x61544672,  0x5c4f436c,  0x574b3f66,
  0x53473c62,  0x4e43395c,  0x4a403658,  0x463d3352,
  0x4339304e,  0x3f362e4a,  0x3c342b46,  0x39312942,
  0x362e273e,  0x332c253c,  0x30292338,  0x2e272136,
  0x2b251f32,  0x29231d30,  0x27211c2c,  0x251f1a2a,
  0x231e1928,  0x211c1826,  0x1f1b1624,  0x1d191522,
  0x1c181420,  0x1a17131e,  0x1915121c,  0x1714111a,
  0x16131018,  0x15120f18,  0x14110e16,  0x13100d14,
  0x120f0c14,  0x110e0c12,  0x100d0b12,  0x0f0d0a10,
  0x0e0c0a10,  0x0d0b090e,  0x0c0a090e,  0x0c0a080c,
  0x0b09070c,  0x0a09070a,  0x0a08070a,  0x0908060a,
  0x09070608,  0x08070508,  0x07060508,  0x00000000,
};

/*
  H264E_SliceInit(_H264E_SliceInfo *s)
  @param s: slice information structure
 */
#endif /*__JZM_H264E_H__*/
