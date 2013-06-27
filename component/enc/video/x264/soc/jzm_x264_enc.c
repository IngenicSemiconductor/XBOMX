#include "jzm_x264_enc.h"

void H264E_SliceInit(_H264E_SliceInfo *s)
{
  unsigned int i, j, tmp = 0;
  volatile unsigned int *chn = (volatile unsigned int *)s->des_va;


#if 0
  GEN_VDMA_ACFG(chn, REG_SCH_GLBC, 0, SCH_GLBC_HIAXI
#ifdef x264_TLB_OPT
		| SCH_GLBC_TLBE
#endif
#ifdef X264_IRQ_OPT		
		| SCH_INTE_ACFGERR | SCH_INTE_BSERR | SCH_INTE_ENDF | SCH_INTE_TLBERR
#endif
		);
#endif

  GEN_VDMA_ACFG(chn, TCSM_FLUSH, 0, 0x0);

  /**************************************************
   EFE configuration
   *************************************************/
  GEN_VDMA_ACFG(chn, REG_EFE_GEOM, 0, (EFE_FST_MBY(s->first_mby) | 
				       EFE_LST_MBY(s->last_mby) |
				       EFE_LST_MBX(s->mb_width-1)) );

  GEN_VDMA_ACFG(chn, REG_EFE_COEF_BA, 0, VRAM_MAU_RESA);
  GEN_VDMA_ACFG(chn, REG_EFE_RAWY_SBA, 0, s->fb[2][0]);
  GEN_VDMA_ACFG(chn, REG_EFE_RAWC_SBA, 0, s->fb[2][1]);
  GEN_VDMA_ACFG(chn, REG_EFE_TOPMV_BA, 0, VRAM_TOPMV_BA);
  GEN_VDMA_ACFG(chn, REG_EFE_TOPPA_BA, 0, VRAM_TOPPA_BA);
  GEN_VDMA_ACFG(chn, REG_EFE_MECHN_BA, 0, VRAM_ME_CHN_BASE);
  GEN_VDMA_ACFG(chn, REG_EFE_MAUCHN_BA, 0, VRAM_MAU_CHN_BASE);
  GEN_VDMA_ACFG(chn, REG_EFE_DBLKCHN_BA, 0, VRAM_DBLK_CHN_BASE);
  GEN_VDMA_ACFG(chn, REG_EFE_SDECHN_BA, 0, VRAM_SDE_CHN_BASE);
  GEN_VDMA_ACFG(chn, REG_EFE_RAW_DBA, 0, VRAM_RAWY_BA);

  /**************************************************
   Motion configuration
   *************************************************/
  GEN_VDMA_ACFG(chn, REG_MCE_CH1_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );

  GEN_VDMA_ACFG(chn, REG_MCE_CH2_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );

  GEN_VDMA_ACFG(chn, REG_MCE_CTRL, 0, ( MCE_ESTI_MAX_SDIA(15) |
					MCE_ESTI_USE_PMV |
					MCE_ESTI_QPEL |
					MCE_ESTI_PUT_MET |
					MCE_CH2_EN |
					MCE_CLKG_EN |
					MCE_OFA_EN |
					MCE_MODE_ESTI |
					MCE_CACHE_FLUSH |
					MCE_EN ) );

  GEN_VDMA_ACFG(chn, REG_MCE_ESTIC, 0, MCE_ESTIC(0, 10, 0, 1) );

  for(i=0; i<16; i++){
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_ILUT+i*8, 0, 
		  MCE_CH2_IINFO(IntpFMT[H264_EPEL][i].intp[0],
				IntpFMT[H264_EPEL][i].intp_dir[0],
				IntpFMT[H264_EPEL][i].intp_sft[0],
				IntpFMT[H264_EPEL][i].intp_coef[0][0],
				IntpFMT[H264_EPEL][i].intp_coef[0][1],
				IntpFMT[H264_EPEL][i].intp_rnd[0]) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_ILUT+i*8+4, 0, 
		  MCE_CH2_IINFO(IntpFMT[H264_EPEL][i].intp[1],
				IntpFMT[H264_EPEL][i].intp_dir[1],
				IntpFMT[H264_EPEL][i].intp_sft[1],
				IntpFMT[H264_EPEL][i].intp_coef[1][0],
				IntpFMT[H264_EPEL][i].intp_coef[1][1],
				IntpFMT[H264_EPEL][i].intp_rnd[1]) );
  }

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_BINFO, 0, 
		MCE_BINFO(IS_SKIRT, 12, 12, 0, SubPel[H264_QPEL]-1) );

  GEN_VDMA_ACFG(chn, REG_MCE_CH2_BINFO, 0, 
		MCE_BINFO(IS_SKIRT, 0, 0, 1, SubPel[H264_EPEL]) );

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_IINFO1, 0, 
		MCE_CH1_IINFO(IntpFMT[H264_QPEL][H2V0].intp[0], 
			      IntpFMT[H264_QPEL][H2V0].tap,
			      1, 0, 0,
			      IntpFMT[H264_QPEL][H2V0].intp_dir[0],
			      IntpFMT[H264_QPEL][H2V0].intp_rnd[0],
			      IntpFMT[H264_QPEL][H2V0].intp_sft[0],
			      0, 0, 0) );

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_IINFO2, 0, 
		MCE_CH1_IINFO(IntpFMT[H264_QPEL][H0V2].intp[0], 
			      IntpFMT[H264_QPEL][H0V2].tap,
			      0, 0, 0,
			      IntpFMT[H264_QPEL][H0V2].intp_dir[0],
			      IntpFMT[H264_QPEL][H0V2].intp_rnd[0],
			      IntpFMT[H264_QPEL][H0V2].intp_sft[0],
			      0, 0, 0) );
  
  GEN_VDMA_ACFG(chn, REG_MCE_CH1_TAP1L, 0, 
		MCE_CH1_TAP(IntpFMT[H264_QPEL][H2V0].intp_coef[0][0],
			    IntpFMT[H264_QPEL][H2V0].intp_coef[0][1],
			    IntpFMT[H264_QPEL][H2V0].intp_coef[0][2],
			    IntpFMT[H264_QPEL][H2V0].intp_coef[0][3]) );

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_TAP1M, 0, 
		MCE_CH1_TAP(IntpFMT[H264_QPEL][H2V0].intp_coef[0][4],
			    IntpFMT[H264_QPEL][H2V0].intp_coef[0][5],
			    IntpFMT[H264_QPEL][H2V0].intp_coef[0][6],
			    IntpFMT[H264_QPEL][H2V0].intp_coef[0][7]) );
  
  GEN_VDMA_ACFG(chn, REG_MCE_CH1_TAP2L, 0, 
		MCE_CH1_TAP(IntpFMT[H264_QPEL][H0V2].intp_coef[0][0],
			    IntpFMT[H264_QPEL][H0V2].intp_coef[0][1],
			    IntpFMT[H264_QPEL][H0V2].intp_coef[0][2],
			    IntpFMT[H264_QPEL][H0V2].intp_coef[0][3]) );

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_TAP2M, 0, 
		MCE_CH1_TAP(IntpFMT[H264_QPEL][H0V2].intp_coef[0][4],
			    IntpFMT[H264_QPEL][H0V2].intp_coef[0][5],
			    IntpFMT[H264_QPEL][H0V2].intp_coef[0][6],
			    IntpFMT[H264_QPEL][H0V2].intp_coef[0][7]) );
  
  GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+4, 0, s->fb[1][0]); 
  GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+4, 0, s->fb[1][1]); 

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_PINFO, 0, 0);
  GEN_VDMA_ACFG(chn, REG_MCE_CH2_PINFO, 0, 0);

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_WINFO, 0, 0);
  GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO1, 0, 0);
  GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO2, 0, 0);
  
  GEN_VDMA_ACFG(chn, REG_MCE_CH1_WTRND, 0, 0);
  GEN_VDMA_ACFG(chn, REG_MCE_CH2_WTRND, 0, 0);

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(s->mb_width*16+16*2, 16, 0) );
  GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(s->mb_width*16+16*2, 0, 0) );

  GEN_VDMA_ACFG(chn, REG_MCE_GEOM, 0, MCE_GEOM(s->mb_height*16, s->mb_width*16) );

  GEN_VDMA_ACFG(chn, REG_MCE_DSA, 0, VRAM_ME_DSA);
  GEN_VDMA_ACFG(chn, REG_MCE_MVPA, 0, VRAM_ME_MVPA);
  GEN_VDMA_ACFG(chn, REG_MCE_DDC, 0, VRAM_ME_CHN_BASE);

  /**************************************************
   VMAU configuration
   *************************************************/
  GEN_VDMA_ACFG(chn, REG_VMAU_GBL_RUN, 0, VMAU_RESET);
  GEN_VDMA_ACFG(chn, REG_VMAU_VIDEO_TYPE, 0, (VMAU_MODE_ENC | VMAU_FMT_H264 | 6<<7) );
  GEN_VDMA_ACFG(chn, REG_VMAU_NCCHN_ADDR, 0, VRAM_MAU_CHN_BASE);
  GEN_VDMA_ACFG(chn, REG_VMAU_ENC_DONE, 0, VRAM_MAU_ENC_SYNA);
  GEN_VDMA_ACFG(chn, REG_VMAU_DEC_DONE, 0, VRAM_MAU_DEC_SYNA);
  GEN_VDMA_ACFG(chn, REG_VMAU_Y_GS, 0, s->mb_width*16);
  GEN_VDMA_ACFG(chn, REG_VMAU_GBL_CTR, 0, (VMAU_CTRL_TO_DBLK | VMAU_CTRL_FIFO_M) );

  for(j=0; j<4; j++){
    for(i=0; i<4; i++){
      GEN_VDMA_ACFG(chn, REG_VMAU_QT+tmp, 0, *(int *)(&s->scaling_list[j][i*4]) );
      tmp += 4;
    }
  }
  for(j=0; j<4; j++){
    for (i=0; i<8; i++){
      int tmp0 = (1ULL << 8) / s->scaling_list[j][2*i];
      int tmp1 = (1ULL << 8) / s->scaling_list[j][2*i+1];
      GEN_VDMA_ACFG(chn, REG_VMAU_QT+tmp, 0, ((tmp0 & 0xFFFF) | (tmp1<<16)) );
      tmp += 4;
    }
  }

  /**************************************************
   DBLK configuration
   *************************************************/
  GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_RESET);
  GEN_VDMA_ACFG(chn, REG_DBLK_DHA, 0, VRAM_DBLK_CHN_BASE);
  GEN_VDMA_ACFG(chn, REG_DBLK_GENDA, 0, VRAM_DBLK_CHN_SYNA);
  GEN_VDMA_ACFG(chn, REG_DBLK_GSIZE, 0, DBLK_GSIZE(s->mb_height, s->mb_width) );
  GEN_VDMA_ACFG(chn, REG_DBLK_GPOS, 0, DBLK_GPOS(s->first_mby, 0) );
  GEN_VDMA_ACFG(chn, REG_DBLK_CTRL, 0, DBLK_CTRL(1, s->rotate, s->deblock) );
  GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_YA, 0, s->fb[0][0] - (s->mb_width+3)*256);
  GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_CA, 0, s->fb[0][1] - (s->mb_width+3)*128);
  GEN_VDMA_ACFG(chn, REG_DBLK_GP_ENDA, 0, VRAM_DBLK_DOUT_SYNA);
  GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, DBLK_GPIC_STR(s->mb_width*128, s->mb_width*256) );
  GEN_VDMA_ACFG(chn, REG_DBLK_VTR, 0, DBLK_VTR(s->beta_offset, s->alpha_c0_offset, 0, 0,
					       s->frame_type, DBLK_FMT_H264) );
  GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_SLICE_RUN);

  /**************************************************
   SDE configuration
   *************************************************/
  GEN_VDMA_ACFG(chn, REG_SDE_STAT, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, (SDE_MODE_STEP | SDE_EN) );
  GEN_VDMA_ACFG(chn, REG_SDE_SL_GEOM, 0, SDE_SL_GEOM(s->mb_height, s->mb_width, 
						     s->first_mby, 0) );
  GEN_VDMA_ACFG(chn, REG_SDE_CODEC_ID, 0, SDE_FMT_H264_ENC);
  //slice_info0 {desp_link_en[1], auto_syn_en[0]}
  GEN_VDMA_ACFG(chn, REG_SDE_CFG0, 0, 0x3);
  //slice_info1 {qp[8], slice_type[0]}
  GEN_VDMA_ACFG(chn, REG_SDE_CFG1, 0, (s->qp<<8 | (s->frame_type + 1)) );
  //desp_addr
  GEN_VDMA_ACFG(chn, REG_SDE_CFG2, 0, VRAM_SDE_CHN_BASE);
  //sync_addr
  GEN_VDMA_ACFG(chn, REG_SDE_CFG3, 0, VRAM_SDE_SYNA);
  //bs_addr
  GEN_VDMA_ACFG(chn, REG_SDE_CFG4, 0, s->bs);
  //context table
  for(i=0; i<460; i++){
    int idx = s->state[i];
    if(idx <= 63)
      idx = 63 - idx;
    else
      idx -= 64;
    GEN_VDMA_ACFG(chn, REG_SDE_CTX_TBL+i*4, 0, 
		  (lps_range[idx] | ((s->state[i]>>6) & 0x1)) );
  }
  //init sync
  GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, 0, SDE_SLICE_INIT);

  /**************************************************
   TOPMV/TOPPA init
   *************************************************/
/*   for(i=0; i<256; i++){ */
/*     write_reg(VRAM_TOPMV_BA+i*4, 0); */
/*     write_reg(VRAM_TOPPA_BA+i*4, 0); */
/*   } */

  /**************************************************
   SCH configuration
   *************************************************/
  GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SCH_SCHG0, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SCH_SCHG1, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SCH_SCHE1, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SCH_SCHE2, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SCH_SCHE3, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SCH_SCHE4, 0, 0);

  GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, (SCH_CH4_GS1 | SCH_CH4_PE | SCH_CH4_PCH(0) |
				       SCH_CH3_GS1 | SCH_CH3_PE | SCH_CH3_PCH(0) |
				       SCH_CH2_GS0 | SCH_CH2_PE | SCH_CH2_PCH(0) |
				       SCH_CH1_GS0 | SCH_CH1_PCH(0) |
				       (s->frame_type & 0x1)<<2 ) );

  GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH4_HID(HID_SDE) | SCH_CH3_HID(HID_DBLK) |
				      SCH_CH2_HID(HID_VMAU) | SCH_CH1_HID(HID_MCE) |
				      SCH_DEPTH(SCH_FIFO_DEPTH) |
				      SCH_BND_G1F4 | SCH_BND_G1F3 |
				      SCH_BND_G0F4 | SCH_BND_G0F3 | SCH_BND_G0F2 |
				      (s->frame_type & 0x1)<<0 ) );

  GEN_VDMA_ACFG(chn, REG_EFE_CTRL, VDMA_ACFG_TERM, (EFE_X264_QP(s->qp) | EFE_DBLK_EN |
						    EFE_SLICE_TYPE(s->frame_type) |
						    EFE_EN | EFE_RUN) );
}
