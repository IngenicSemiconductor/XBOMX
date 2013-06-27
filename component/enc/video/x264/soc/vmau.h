#ifndef __VMAU_H__
#define __VMAU_H__

#include <linux/types.h>
#include "bits_mask_def.h"

typedef enum{H264 = 1, REAL = 2, VC1 = 3, MPEG2 = 4, MPEG4 = 5}video_type_e;

#define M4x4 0
#define M4x4L 1
#define M8x8 2
#define M16x16 3

#define VMAU_SLV_MCBP (0*4)
#define VMAU_SLV_QTPARA (1*4)
#define VMAU_SLV_MAIN_ADDR (2*4)
#define VMAU_SLV_NCCHN_ADDR (3*4)
#define VMAU_SLV_CHN_LEN (4*4)

#define VMAU_SLV_ACBP (5*4)
#define VMAU_SLV_CPREDM_TLV (6*4)
#define VMAU_SLV_YPREDM0 (7*4)
#define VMAU_SLV_YPREDM1 (8*4)


#define VMAU_SLV_GBL_RUN (0x10*4)
#define VMAU_SLV_GBL_CTR (0x11*4)
#define VMAU_SLV_STATUS (0x12*4)
#define VMAU_SLV_CCHN_ADDR (0x13*4)
#define VMAU_SLV_VIDEO_TYPE (0x14*4)


#define VMAU_SLV_Y_GS (0x15*4)

#define VMAU_SLV_DEC_DONE (0x16*4)
#define VMAU_SLV_ENC_DONE (0x17*4)

#define VMAU_SLV_POS (0x18*4)
#define VMAU_SLV_MCF_STA (0x19*4)

#define VMAU_SLV_DEC_YADDR (0x1a*4)
#define VMAU_SLV_DEC_UADDR (0x1b*4)
#define VMAU_SLV_DEC_VADDR (0x1c*4)
#define VMAU_SLV_DEC_STR (0x1d*4)

#define MAU_MTX_MSK 0x3
#define MAU_MTX_WID 2
#define MAU_MTX_SFT (24)

#define MAU_Y_ERR_MSK 1
#define MAU_Y_ERR_SFT (MAU_MTX_WID+MAU_MTX_SFT) //26

#define MAU_Y_PREDE_MSK 1
#define MAU_Y_PREDE_SFT (MAU_Y_ERR_SFT+MAU_Y_ERR_MSK) //27

#define MAU_Y_DC_DCT_MSK 1
#define MAU_Y_DC_DCT_SFT (MAU_Y_PREDE_SFT+MAU_Y_PREDE_MSK) //28

#define MAU_C_ERR_MSK 1
#define MAU_C_ERR_SFT (MAU_Y_DC_DCT_MSK+MAU_Y_DC_DCT_SFT) //29

#define MAU_C_PREDE_MSK 1
#define MAU_C_PREDE_SFT (MAU_C_ERR_SFT+MAU_C_ERR_MSK) //30

#define MAU_C_DC_DCT_MSK 1
#define MAU_C_DC_DCT_SFT (MAU_C_PREDE_MSK+MAU_C_PREDE_SFT) //31

#define MAU_U_DC_DCT_MSK 1
#define MAU_U_DC_DCT_SFT (24) 

#define MAU_V_DC_DCT_MSK 1
#define MAU_V_DC_DCT_SFT (25) 

#define MAU_ENC_RESULT_U_DC_MSK 24
#define MAU_ENC_RESULT_V_DC_MSK 25

#define MAU_VIDEO_MSK 0x7
#define MAU_VIDEO_WID 3
#define MAU_VIDEO_SFT 0 //0

#define MAU_TYPE_MSK 0xF
#define MAU_TYPE_WID 4
#define MAU_TYPE_SFT (MAU_VIDEO_WID + MAU_VIDEO_SFT) //3

#define MAU_SFT_MSK 0xF
#define MAU_SFT_WID 4
#define MAU_SFT_SFT (MAU_TYPE_WID + MAU_TYPE_SFT) //7

#define MAU_ENC_MSK 0x1
#define MAU_ENC_WID 1
#define MAU_ENC_SFT (MAU_SFT_SFT + MAU_SFT_WID) //11


#define QUANT_QP_MSK 0x3F
#define QUANT_QP_WID 6
#define QUANT_QP_SFT 0

#define QUANT_CQP_MSK 0x3F
#define QUANT_CQP_WID 6
#define QUANT_CQP_SFT (QUANT_QP_SFT + QUANT_QP_WID)

#define QUANT_C1QP_MSK 0x3F
#define QUANT_C1QP_WID 6
#define QUANT_C1QP_SFT (QUANT_CQP_SFT + QUANT_CQP_WID)

#define MAU_MP2_QTYPE_MSK 0x1
#define MAU_MP2_QTYPE_WID 1
#define MAU_MP2_QTYPE_SFT (23)

#define MAU_MP2_MMOD_MSK 0x1
#define MAU_MP2_MMOD_WID 1
#define MAU_MP2_MMOD_SFT (24)

#define MAU_MP2_SCAN_MSK 0x1
#define MAU_MP2_SCAN_WID 1
#define MAU_MP2_SCAN_SFT (25)


#define VMAU_ENC_Y8x8_0_MSK (BIT0 | BIT1 | BIT2 | BIT3)
#define VMAU_ENC_Y8x8_1_MSK (BIT4 | BIT5 | BIT6 | BIT7)
#define VMAU_ENC_Y8x8_2_MSK (BIT8 | BIT9 | BIT10 | BIT11)
#define VMAU_ENC_Y8x8_3_MSK (BIT12 | BIT13 | BIT14 | BIT15)

//#define VMAU_ENC_Y_MSK    (0xFFFF) //bit 15 ~ 0
#define VMAU_ENC_Y_MSK      (VMAU_ENC_Y8x8_0_MSK | VMAU_ENC_Y8x8_1_MSK |  \
                             VMAU_ENC_Y8x8_2_MSK | VMAU_ENC_Y8x8_3_MSK)   //bit 15 ~ 0

#define VMAU_ENC_U_MSK      (BIT16 | BIT17 | BIT18 | BIT19)
#define VMAU_ENC_V_MSK      (BIT20 | BIT21 | BIT22 | BIT23)
#define VMAU_ENC_UV_MSK     (VMAU_ENC_U_MSK | VMAU_ENC_V_MSK)       //23~16
#define VMAU_ENC_U_DC_MSK   (BIT24)
#define VMAU_ENC_V_DC_MSK   (BIT25)
#define VMAU_ENC_UV_DC_MSK  (VMAU_ENC_U_DC_MSK | VMAU_ENC_V_DC_MSK) //25~24
#define VMAU_ENC_Y_DC_MSK   (BIT28)

#define VMAU_CBP_Y(vmau_cbp) ((vmau_cbp) & VMAU_ENC_Y_MSK)         //bit 15~0
#define VMAU_CBP_UV(vmau_cbp) ((vmau_cbp) & VMAU_ENC_UV_MSK)       //bit 23~16
#define VMAU_CBP_UV_DC(vmau_cbp) ((vmau_cbp) & VMAU_ENC_UV_DC_MSK) //bit 25~24


typedef struct{
           //name                 bits  Description
  uint32_t src_len               : 10;  //9:0   Main source data length. (Unit: Byte)
  uint32_t rsvd1                 : 6;   //15:10 Writing has no effect, read as zero
  uint32_t tsk_chn_id            : 8;   //23:16 The id to source descript chain
  uint32_t rsvd2                 : 7;   //30:24 Writing has no effect, read as zero
                                        //Next decoder result's address increase trigger,
  uint32_t dec_out_addr_inc_flag : 1;   //31, 1:addr incre, 0:addr go to DEC_YADDR,DEC_UADDR,DEC_VADDR
}SRC_LEN_ID_DEC_INC_BITS;

typedef union{
  uint32_t all;
  SRC_LEN_ID_DEC_INC_BITS bits;
}SRC_LEN_ID_DEC_INC_REG;


typedef struct{
           //name    bits    Description
  uint32_t c_type0  : 4;   //3:0    The nth matrix type in chroma planar 
  uint32_t c_type1  : 4;   //7:4 
  uint32_t c_type2  : 4;   //11:8
  uint32_t c_type3  : 4;   //15:12 
  uint32_t tlr      : 4;   //19:16  Top left and right valid
  uint32_t rsvd     : 12;  //31:20  Writing has no effect, read as zero
}CPRED_MODE_TLR_REG_BITS;

typedef union{
  uint32_t all;
  CPRED_MODE_TLR_REG_BITS bits;
}CPRED_MODE_TLR_REG; //TLR: TOP_LEFT_RIGHT


typedef volatile struct{
  unsigned int main_cbp ;                //0x0
  unsigned int quant_para;               //0x4
  unsigned int *main_addr;                //0x8
  unsigned int *ncchn_addr;               //0xC
  SRC_LEN_ID_DEC_INC_REG src_len_id_inc; //0x10
  unsigned int *aux_cbp ;                 //0x14
  CPRED_MODE_TLR_REG c_pred_mode_tlr;    //0x18
  unsigned int y_pred_mode[2];           //0x1C ~ 0x20
}VMAU_TASK_CHAIN;

#define VMAU_MSRC_SIZE (16*24)
#define VMAU_ENCDST_SIZE (24*16*2 + 16*2 + 4*2*2 + 4)
#define VMAU_DECDST_SIZE (24*16)

typedef struct{//DO NOT seperate the two variables below
    unsigned int  cbp;
    short res[(VMAU_ENCDST_SIZE - 4)/(sizeof(short))];//-4 for cbp
}VMAU_CBP_RES;

typedef struct{
  VMAU_TASK_CHAIN task_chain;
    
  //VMAU_CBP_RES enc_dst;

  unsigned char dec_dst_y[256];
  unsigned char dec_dst_uv[128];
}VMAU_VAR;


#define QUANT_DEQEN_SFT (31)
#define QUANT_DEQEN_MSK (31)

#define VMAU_MEML_BASE (0x4000) //mc_fifo
#define VMAU_QT_BASE (0x8000)

#define VMAU_RUN 1
#define VMAU_STOP 2
#define VMAU_RESET 4



#define H264_YQP_DIV6_SFT 0x0
#define H264_YQP_DIV6_MSK 0xf

#define H264_YQP_MOD6_SFT 0x4
#define H264_YQP_MOD6_MSK 0x7

#define H264_CQP_DIV6_SFT 0x7
#define H264_CQP_DIV6_MSK 0xf

#define H264_CQP_MOD6_SFT 11
#define H264_CQP_MOD6_MSK 0x7

#define H264_C1QP_DIV6_SFT 14
#define H264_C1QP_DIV6_MSK 0xf

#define H264_C1QP_MOD6_SFT 18
#define H264_C1QP_MOD6_MSK 0x7



/*-------CTRL------------*/
#define VMAU_CTRL_FIFO_M 0x1
#define VMAU_CTRL_IRQ_EN 0x10
#define VMAU_CTRL_SLPOW 0x10000
/*--------TRI-------------*/
#define VMAU_TRI_IRQ_CLR 0x8
#define VMAU_TRI_MCYFW_WR 0x10000
#define VMAU_TRI_MCCFW_WR 0x30000
#define VMAU_TRI_MCF_CLR 0x40000


#define VMAU_MC_FIFO_DEPTH 5

typedef volatile struct{
  uint32_t  y[VMAU_MC_FIFO_DEPTH][ 256 / sizeof(uint32_t) ];
  uint32_t uv[VMAU_MC_FIFO_DEPTH][ 128 / sizeof(uint32_t) ];
}MC_FIFO;


typedef struct{
           //name    bits    Description
  uint32_t height  : 14;   //13:0   Frame height in pixel
  uint32_t rsvd1   : 2;    //15:14  Writing has no effect, read as zero
  uint32_t width   : 14;   //29:16  Frame width in pixel
  uint32_t rsvd2   : 2;    //31:30  Writing has no effect, read as zero
}FRAME_SIZE_BITS;

typedef union{
  uint32_t all;
  FRAME_SIZE_BITS bits;
}FRAME_SIZE_REG; //TLR: TOP_LEFT_RIGHT


//NOTE: assume sizeof(void *) = sizeof(int32_t)
typedef struct{
  uint32_t main_cbp;                 //0x0
  uint32_t quant_param;              //0x4
  uint32_t src_phy_addr;             //0x8
  volatile uint32_t *next_task_chain_phy_addr; //0xC
  uint32_t src_len;                  //0x10
  uint32_t acbp;                     //0x14
  uint32_t top_left_and_c_pred;      //0x18
  uint32_t y_pred0;                  //0x1C
  uint32_t y_pred1;                  //0x20
  uint32_t enc_addr_out;             //0x24
  uint32_t rsvd1;                    //0x28
  uint32_t rsvd2;                    //0x2C
  uint32_t rsvd3;                    //0x30  
  uint32_t rsvd4;                    //0x34
  uint32_t rsvd5;                    //0x38
  uint32_t rsvd6;                    //0x3C
  volatile uint32_t glb_trigger;     //0x40
  uint32_t glb_enable;               //0x44
  uint32_t status;                   //0x48
  uint32_t task_chain_phy_addr;      //0x4C
  uint32_t video_type;               //0x50
  FRAME_SIZE_REG frame_size;         //0x54
  volatile int32_t* dec_end_flag_phy_addr;    //0x58
  volatile int32_t* enc_end_flag_phy_addr;    //0x5C
  uint32_t mb_pos;                   //0x60
  uint32_t mcf_status;               //0x64
  uint8_t *dec_y_phy_addr;           //0x68
  uint8_t *dec_u_phy_addr;           //0x6C
  uint8_t *dec_v_phy_addr;           //0x70
  uint32_t dec_stride;               //0x74
  uint32_t rsvd8[(0x4000 - 0x78)/sizeof(uint32_t)];  //0x78 ~ 0x3FFC ; FIXME: using offset instead 0x78

  MC_FIFO mc_fifo;
  uint32_t rsvd9[(0x4780 - ( 0x4000 + sizeof(MC_FIFO) ) )/sizeof(uint32_t)]; //0x4000 ~ 0x477C

  uint32_t pred_line[(0x5680  - 0x4780)/sizeof(uint32_t)]; //0x4780 ~ 0x567C
  uint32_t rsvd10[(0x8000 - 0x5680)/sizeof(uint32_t)];     //0x5680 ~ 0x7FFC ; FIXME: using offset instead 0x74
  uint32_t quant_table[16+ 32];      //0x8000 ~ ? how long
}vmau_reg;

void vmau_reg_init(volatile vmau_reg * vmau);

void dump_vmau_task_chain(const VMAU_TASK_CHAIN * vmau_task_chain_ptr);

void dump_vmau_reg(const vmau_reg * vmau);

void dump_vmau_src(const uint8_t * vmau_src_y, const uint8_t * vmau_src_u, const uint8_t * vmau_src_v);

void dump_vmau_quant_table(const unsigned int *quant_table);

void dump_vmau_cbp_res(const short *vmau_enc_dst);

void dump_vmau_dec_rst(const unsigned char *vmau_dec_rst);

int verify_vmau_enc_rst(const unsigned int sw_cbp, const short *sw_res, 
			 const unsigned int hw_cbp, const short *hw_res);

int verify_vmau_dec_rst(const unsigned char *sw_y, const unsigned char *sw_u, const unsigned char *sw_v, 
			 const unsigned char *hw_y, const unsigned char *hw_u, const unsigned char *hw_v);

void dump_vmau_mc_fifo(const unsigned char *mc_fifo_y, 
		       const unsigned char *mc_fifo_u, const unsigned char *mc_fifo_v);

void dump_nnz(const unsigned char *nnz);

void dump_bs(const uint8_t * bs, int bs_len);

//void dump_entropy_var(const ENTROPY_VAR *entropy_var_ptr);

#endif //__VMAU_H__
