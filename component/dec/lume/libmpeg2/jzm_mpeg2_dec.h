/****************************************************************
*****************************************************************/

#ifndef __JZM_MPEG2_DEC_H__
#define __JZM_MPEG2_DEC_H__
#include "../libjzcommon/jzm_vpu.h"

/************************************************************
 Chain Space Allocation
 ************************************************************/
#define TCSM1_BANK0         (0x132C0000)
#define SRAM_BANK0          (0x132F0000)

#define MPEG2_FIFO_DEP          16

#define MSRC_ONE_LEN            (1024)
#define MSRC_DEP                MPEG2_FIFO_DEP
#define MSRC_BUF_LEN            (MSRC_ONE_LEN*MSRC_DEP) /*0x4000*/

#define TCSM1_MSRC_BUF          SRAM_BANK0

#define MC_CHN_ONELEN           (32<<2)
#define MC_CHN_DEP              MPEG2_FIFO_DEP 
#define MC_CHN_LEN              (MC_CHN_ONELEN*MC_CHN_DEP) /*0x800*/
#define TCSM1_MOTION_DHA        (TCSM1_BANK0)
#define TCSM1_MOTION_DSA        (TCSM1_MOTION_DHA + MC_CHN_LEN- 4)

#define VMAU_CHN_ONELEN         (8<<2)
#define VMAU_CHN_DEP            MPEG2_FIFO_DEP
#define VMAU_CHN_LEN            (VMAU_CHN_ONELEN*VMAU_CHN_DEP) /*0x200*/
#define VMAU_CHN_BASE           (TCSM1_BANK0+ 0x800)
#define VMAU_END_FLAG           (VMAU_CHN_BASE+VMAU_CHN_LEN- 4)

#define DBLK_CHN_ONELEN         (2<<2)
#define DBLK_CHN_DEP            MPEG2_FIFO_DEP
#define DBLK_CHN_LEN            (DBLK_CHN_ONELEN*DBLK_CHN_DEP)
#define DBLK_CHN_BASE           (TCSM1_BANK0+ 0xa00)
#define DBLK_END_FLAG           (DBLK_CHN_BASE+DBLK_CHN_LEN)
#define DBLK_SEND_FLAG          (DBLK_END_FLAG + 4)

#define DBLK_DMA_END_BASE       DBLK_END_FLAG
#define DBLK_CHN_END_BASE       DBLK_SEND_FLAG

#define DOUT_Y_STRD  (16)
#define DOUT_C_STRD  (8)


typedef struct _M2D_BitStream {
  unsigned int buffer;    /*BS buffer physical address*/
  unsigned char  bit_ofst;  /*bit offset for first word*/
}_M2D_BitStream;


typedef struct _M2D_SliceInfo {
  //_M2D_Ctrl  m2d_ctrl;

  unsigned char mb_width; /*frame width(mb layer)*/
  unsigned char mb_height; /*frame height(mb layer)*/
  unsigned char mb_pos_x; /*slice_head position*/
  unsigned char mb_pos_y;

  unsigned int coef_qt[4][16]; /*qt table for vmau module*/
  unsigned int scan[16]; /*scan table*/
  
  unsigned char coding_type;  /*I_TYPE(1), P_TYPE(2), B_TYPE(3), D_TYPE(4) */
  unsigned char fr_pred_fr_dct; /*frame_pred_frame_dct*/
  unsigned char pic_type; /*TOP_FIELD(1), BOTTOM_FIELD(2), FRAME_PICTURE(3)*/
  unsigned char conceal_mv; /*concealment_motion_vectors*/
  unsigned char intra_dc_pre;  /*intra_dc_precision*/
  unsigned char intra_vlc_format; /*1, B15; 0, B14*/
  unsigned char mpeg1; /**/
  unsigned char top_fi_first; /*top field first*/
  unsigned char q_scale_type; 
  unsigned char qs_code; /*quantizer_scale_code*/
  unsigned char dmv_ofst; /*1, +1; 0, -1*/
  unsigned char sec_fld; /*current picture is second field*/

  unsigned char f_code[2][2]; /*{backward motion, forward motion}{1, 0}*/
  
  /*--------------------------------------------------------------------------------
   * pic_ref[a][b].
   * a | 0 | 1 |
   * --|---|---|---
   *   | C | Y |
   * ---------------------
   * b |  0  |  1   |    2    |    3    |    4    |    5    |    6    |   7     |
   *---|-----|------|---------|---------|---------|---------|---------|---------|-
   *   |f_frm| b_frm|f_top_fld|b_top_fld|f_bot_fld|b_bot_fld|c_top_fld|c_bot_fld|
   --------------------------------------------------------------------------------*/
  unsigned int pic_ref[2][8]; /*The phy_addr of frame reference picture*/
  unsigned int pic_cur[2]; /*the phy_addr of current picture. {C, Y}*/

  _M2D_BitStream  bs_buf;

  /*descriptor address*/
  unsigned int *des_va;
  unsigned int *des_pa;
  int y_stride;
  int c_stride;
}_M2D_SliceInfo;


//mba
#define MBA_5_BASE  0x1f0 //0x1f0-0x1f7a
#define MBA_5_LEN  8 //0x1f0-0x1f7a
#define MBA_11_BASE  0x080 //0x080-0x0bf
#define MBA_11_LEN  64 //0x080-0x0bf
//mb
#define MB_I_BASE 0x222 //0x222
#define MB_I_LEN 1
#define MB_P_BASE 0x1a0 //0x1a0-0x1af
#define MB_P_LEN 16 //0x1a0-0x1af
#define MB_B_BASE 0x100 //0x100-0x11f
#define MB_B_LEN 32 //0x100-0x11f
//cbp
#define CBP_7_BASE 0x0c0 //0x0c0-0x0ff
#define CBP_7_LEN 64 //0x0c0-0x0ff
#define CBP_9_BASE 0x120 //0x120-0x13f
#define CBP_9_LEN 32 //0x120-0x13f
//dc
#define DC_lum_5_BASE 0x1f8 // 0x1f8-0x1ff
#define DC_lum_5_LEN 8 // 0x1f8-0x1ff
#define DC_long_BASE 0x200 // 0x200-0x207
#define DC_long_LEN 8 // 0x200-0x207
#define DC_chrom_5_BASE 0x208 // 0x208-0x20f
#define DC_chrom_5_LEN 8 // 0x208-0x20f
//dct
#define DCT_B15_8_BASE	0x000 //0x000-0x07f
#define DCT_B15_8_LEN	128 //0x000-0x07f
#define DCT_B15_10_BASE	0x210 //0x210-0x217
#define DCT_B15_10_LEN	8 //0x210-0x217
#define DCT_13_BASE	0x140 // 0x140-0x15f
#define DCT_13_LEN	32 // 0x140-0x15f
#define DCT_15_BASE	0x160 //0x160-0x17f
#define DCT_15_LEN	32 //0x160-0x17f
#define DCT_16_BASE	0x1b0 //0x1b0-0x1bf
#define DCT_16_LEN	16 //0x1b0-0x1bf
#define DCT_B14AC_5_BASE  0x1c0 //0x1c0-0x1cf
#define DCT_B14AC_5_LEN  16 //0x1c0-0x1cf
#define DCT_B14DC_5_BASE  0x1d0 //0x1d0-0x1df
#define DCT_B14DC_5_LEN  16 //0x1d0-0x1df
#define DCT_B14_8_BASE	  0x180//0x180-0x19f
#define DCT_B14_8_LEN	  32//0x180-0x19f
#define DCT_B14_10_BASE	0x218 //0x218-0x21f
#define DCT_B14_10_LEN	8 //0x218-0x21f
// mvd
#define MV_4_BASE	0x220//0x220-0x221
#define MV_4_LEN	2//0x220-0x221
#define MV_10_BASE	0x1e0//0x1e0-0x1ef
#define MV_10_LEN	16//0x1e0-0x1ef
// dmv
#define DMV_2_BASE	0x223//0x223
#define DMV_2_LEN	1//0x223

#define CODING_TYPE_MSK       7
#define CODING_TYPE_SFT       0
#define FR_PRED_FR_DCT_MSK    1
#define FR_PRED_FR_DCT_SFT    3
#define PIC_TYPE_MSK          3
#define PIC_TYPE_SFT          4
#define CONCEAL_MV_MSK        1
#define CONCEAL_MV_SFT        6
#define INTRA_DC_PRE_MSK      7
#define INTRA_DC_PRE_SFT      7
#define INTRA_VLC_FORMAT_MSK  1
#define INTRA_VLC_FORMAT_SFT  10
#define MPEG1_MSK             1
#define MPEG1_SFT             11
#define TOP_FI_FIRST_MSK      1
#define TOP_FI_FIRST_SFT      12
#define QS_TYPE_MSK           1               
#define QS_TYPE_SFT           13
#define SEC_FLD_MSK           1
#define SEC_FLD_SFT           14
#define F_CODE_F0_MSK         0xf
#define F_CODE_F0_SFT         16
#define F_CODE_F1_MSK         0xf
#define F_CODE_F1_SFT         20
#define F_CODE_B0_MSK         0xf
#define F_CODE_B0_SFT         24
#define F_CODE_B1_MSK         0xf
#define F_CODE_B1_SFT         28

#define GET_SL_INFO(coding_type, fr_pred_fr_dct, pic_type, conceal_mv, intra_dc_pre, intra_vlc_format, mpeg1, \
		    top_fi_first, q_scale_type, sec_fld, f_code_f0, f_code_f1, f_code_b0, f_code_b1) \
  ({									\
	      ((coding_type & CODING_TYPE_MSK) << CODING_TYPE_SFT |	\
	       (fr_pred_fr_dct& FR_PRED_FR_DCT_MSK) << FR_PRED_FR_DCT_SFT | \
	       (pic_type& PIC_TYPE_MSK) << PIC_TYPE_SFT |		\
	       (conceal_mv& CONCEAL_MV_MSK) << CONCEAL_MV_SFT |		\
	       (intra_dc_pre& INTRA_DC_PRE_MSK) << INTRA_DC_PRE_SFT |	\
	       (intra_vlc_format& INTRA_VLC_FORMAT_MSK) << INTRA_VLC_FORMAT_SFT | \
	       (mpeg1& MPEG1_MSK) << MPEG1_SFT |			\
	       (top_fi_first& TOP_FI_FIRST_MSK) << TOP_FI_FIRST_SFT |	\
	       ((q_scale_type !=0)& QS_TYPE_MSK) << QS_TYPE_SFT |	\
	       (sec_fld& SEC_FLD_MSK) << SEC_FLD_SFT |			\
	       (f_code_f0& F_CODE_F0_MSK) << F_CODE_F0_SFT |		\
	       (f_code_f1& F_CODE_F1_MSK) << F_CODE_F1_SFT |		\
	       (f_code_b0& F_CODE_B0_MSK) << F_CODE_B0_SFT |		\
	       (f_code_b1& F_CODE_B1_MSK) << F_CODE_B1_SFT);		\
  })									

#define SCH1_DSA  0x13200070
#define SCH2_DSA  0x13200074
#define SCH3_DSA  0x13200078
#define SCH4_DSA  0x1320007C

#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

int M2D_SliceInit(_M2D_SliceInfo *s);
int M2D_SliceInit_ext(_M2D_SliceInfo *s);

#endif
