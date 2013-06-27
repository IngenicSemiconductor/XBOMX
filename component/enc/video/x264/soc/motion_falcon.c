/*******************************************************
 Motion For Falcon
 ******************************************************/

#include "config_jz_soc.h"

#include "t_vpu.h"
#include "t_motion.h"
#include "t_intpid.h"

#include "../soc/jz4760e_2ddma_hw.h"
#include "jz477x_mce.h"
#include "tcsm1.h"
#include "sram.h"

motion_config_t mcfg;


#define DOUT_Y_STRD 16
#define DOUT_C_STRD 8

volatile int pmon1_intp;
volatile int pmon2_intp;
volatile int pmon1_pref;
volatile int pmon2_pref;
volatile int pmon1_imiss;
volatile int pmon2_imiss;
volatile int pmon1_pmiss;
volatile int pmon2_pmiss;
volatile int pmon1_tran;
volatile int pmon2_tran;
volatile int pmon1_work;
volatile int pmon2_work;

#undef printf
 
void motion_init(int intpid, int cintpid)
{
  int i;

  //memset(&mcfg, 0, sizeof(motion_config_t));

  /************* motion DOUT buffer alloc ***************/
  SET_VPU_GLBC(0, 0, 0, 0, 0);

  for(i=0; i<16; i++){
    SET_TAB1_ILUT(i,/*idx*/
		  IntpFMT[intpid][i].intp[1],/*intp2*/
		  IntpFMT[intpid][i].intp_pkg[1],/*intp2_pkg*/
		  IntpFMT[intpid][i].hldgl,/*hldgl*/
		  IntpFMT[intpid][i].avsdgl,/*avsdgl*/
		  IntpFMT[intpid][i].intp_dir[1],/*intp2_dir*/
		  IntpFMT[intpid][i].intp_rnd[1],/*intp2_rnd*/
		  IntpFMT[intpid][i].intp_sft[1],/*intp2_sft*/
		  IntpFMT[intpid][i].intp_sintp[1],/*sintp2*/
		  IntpFMT[intpid][i].intp_srnd[1],/*sintp2_rnd*/
		  IntpFMT[intpid][i].intp_sbias[1],/*sintp2_bias*/
		  IntpFMT[intpid][i].intp[0],/*intp1*/
		  IntpFMT[intpid][i].tap,/*tap*/
		  IntpFMT[intpid][i].intp_pkg[0],/*intp1_pkg*/
		  IntpFMT[intpid][i].intp_dir[0],/*intp1_dir*/
		  IntpFMT[intpid][i].intp_rnd[0],/*intp1_rnd*/
		  IntpFMT[intpid][i].intp_sft[0],/*intp1_sft*/
		  IntpFMT[intpid][i].intp_sintp[0],/*sintp1*/
		  IntpFMT[intpid][i].intp_srnd[0],/*sintp1_rnd*/
		  IntpFMT[intpid][i].intp_sbias[0]/*sintp1_bias*/
		  );
    SET_TAB1_CLUT(i,/*idx*/
		  IntpFMT[intpid][i].intp_coef[0][7],/*coef8*/
		  IntpFMT[intpid][i].intp_coef[0][6],/*coef7*/
		  IntpFMT[intpid][i].intp_coef[0][5],/*coef6*/
		  IntpFMT[intpid][i].intp_coef[0][4],/*coef5*/
		  IntpFMT[intpid][i].intp_coef[0][3],/*coef4*/
		  IntpFMT[intpid][i].intp_coef[0][2],/*coef3*/
		  IntpFMT[intpid][i].intp_coef[0][1],/*coef2*/
		  IntpFMT[intpid][i].intp_coef[0][0] /*coef1*/
		  );
    SET_TAB1_CLUT(16+i,/*idx*/
		  IntpFMT[intpid][i].intp_coef[1][7],/*coef8*/
		  IntpFMT[intpid][i].intp_coef[1][6],/*coef7*/
		  IntpFMT[intpid][i].intp_coef[1][5],/*coef6*/
		  IntpFMT[intpid][i].intp_coef[1][4],/*coef5*/
		  IntpFMT[intpid][i].intp_coef[1][3],/*coef4*/
		  IntpFMT[intpid][i].intp_coef[1][2],/*coef3*/
		  IntpFMT[intpid][i].intp_coef[1][1],/*coef2*/
		  IntpFMT[intpid][i].intp_coef[1][0] /*coef1*/
		  );

    SET_TAB2_ILUT(i,/*idx*/
		  IntpFMT[cintpid][i].intp[1],/*intp2*/
		  IntpFMT[cintpid][i].intp_dir[1],/*intp2_dir*/
		  IntpFMT[cintpid][i].intp_sft[1],/*intp2_sft*/
		  IntpFMT[cintpid][i].intp_coef[1][0],/*intp2_lcoef*/
		  IntpFMT[cintpid][i].intp_coef[1][1],/*intp2_rcoef*/
		  IntpFMT[cintpid][i].intp_rnd[1],/*intp2_rnd*/
		  IntpFMT[cintpid][i].intp[0],/*intp1*/
		  IntpFMT[cintpid][i].intp_dir[0],/*intp1_dir*/
		  IntpFMT[cintpid][i].intp_sft[0],/*intp1_sft*/
		  IntpFMT[cintpid][i].intp_coef[0][0],/*intp1_lcoef*/
		  IntpFMT[cintpid][i].intp_coef[0][1],/*intp1_rcoef*/
		  IntpFMT[cintpid][i].intp_rnd[0]/*intp1_rnd*/
		  );
  }

  SET_REG1_STAT(1,/*pfe*/
		1,/*lke*/
		1 /*tke*/);
  SET_REG2_STAT(1,/*pfe*/
		1,/*lke*/
		1 /*tke*/);

  SET_REG1_BINFO(AryFMT[intpid],/*ary*/
		 0,/*doe*/
		 15,/*expdy*/
		 15,/*expdx*/
		 0,/*ilmd*/
		 SubPel[intpid]-1,/*pel*/
		 0,/*fld*/
		 0,/*fldsel*/
		 0,/*boy*/
		 0,/*box*/
		 3,/*bh*/
		 3,/*bw*/
		 0/*pos*/);
  SET_REG2_BINFO(0,/*ary*/
		 0,/*doe*/
		 0,/*expdy*/
		 0,/*expdx*/
		 (cintpid == H264_EPEL),/*ilmd*/
		 SubPel[cintpid],/*pel*/
		 0,/*fld*/
		 0,/*fldsel*/
		 0,/*boy*/
		 0,/*box*/
		 3,/*bh*/
		 3,/*bw*/
		 0/*pos*/);

  SET_REG1_IINFO1(IntpFMT[intpid][H2V0].intp[0],/*intp*/
		  IntpFMT[intpid][H2V0].tap,/*tap*/
		  1,/*intp_pkg*/
		  IntpFMT[intpid][H2V0].intp_dir[0],/*intp_dir*/
		  IntpFMT[intpid][H2V0].intp_rnd[0],/*intp_rnd*/
		  IntpFMT[intpid][H2V0].intp_sft[0],/*intp_sft*/
		  0,/*sintp*/
		  0,/*sintp_rnd*/
		  0 /*sintp_bias*/);
  SET_REG1_IINFO2(IntpFMT[intpid][H0V2].intp[0],/*intp*/
		  0,/*intp_pkg*/
		  0,/*hldgl*/
		  0,/*avsdgl*/
		  IntpFMT[intpid][H0V2].intp_dir[0],/*intp_dir*/
		  IntpFMT[intpid][H0V2].intp_rnd[0],/*intp_rnd*/
		  IntpFMT[intpid][H0V2].intp_sft[0],/*intp_sft*/
		  0,/*sintp*/
		  0,/*sintp_rnd*/
		  0 /*sintp_bias*/);

  SET_REG1_TAP1(IntpFMT[intpid][H2V0].intp_coef[0][7],
		IntpFMT[intpid][H2V0].intp_coef[0][6],
		IntpFMT[intpid][H2V0].intp_coef[0][5],
		IntpFMT[intpid][H2V0].intp_coef[0][4],
		IntpFMT[intpid][H2V0].intp_coef[0][3],
		IntpFMT[intpid][H2V0].intp_coef[0][2],
		IntpFMT[intpid][H2V0].intp_coef[0][1],
		IntpFMT[intpid][H2V0].intp_coef[0][0]);
  SET_REG1_TAP2(IntpFMT[intpid][H0V2].intp_coef[0][7],
		IntpFMT[intpid][H0V2].intp_coef[0][6],
		IntpFMT[intpid][H0V2].intp_coef[0][5],
		IntpFMT[intpid][H0V2].intp_coef[0][4],
		IntpFMT[intpid][H0V2].intp_coef[0][3],
		IntpFMT[intpid][H0V2].intp_coef[0][2],
		IntpFMT[intpid][H0V2].intp_coef[0][1],
		IntpFMT[intpid][H0V2].intp_coef[0][0]);
}

void motion_config(MCE_VAR *mce, motion_config_t *cfg)
{
  int i;

  SET_REG1_CTRL(0,/*esms*/
		15,/*erms*/
		1,/*earm*/
		1,/*pmve*/
		3,/*esa*/
		1,/*emet*/
		!cfg->work_mode,/*cae*/
		0,/*csf*/
		0xF,/*pgc*/
		1,/*ch2en*/
		3,/*pri*/
		1,/*ckge*/
#ifndef MCE_OFA
		0,/*ofa*/
#else
		1,/*ofa*/
#endif//MCE_OFA
		0,/*rote*/
		0,/*rotdir*/
		cfg->work_mode,/*wm*/
		1,/*ccf*/
		0,/*irqe*/
		0,/*rst*/
		1 /*en*/);

  /******************* ROA setting ******************/
#if 0
  for(i=0; i<16; i++){
    SET_TAB1_RLUT(i,    get_phy_addr(cfg->luma_ref_origin_addr[0][i]), 
        	  cfg->luma_weight[0][i], cfg->luma_offset[0][i]);
    SET_TAB1_RLUT(16+i, get_phy_addr(cfg->luma_ref_origin_addr[1][i]), 
    	          cfg->luma_weight[1][i], cfg->luma_offset[1][i]);
    SET_TAB2_RLUT(i,    get_phy_addr(cfg->chroma_ref_origin_addr[0][i]), 
		  cfg->chroma_weight[0][i][1], cfg->chroma_offset[0][i][1], 
		  cfg->chroma_weight[0][i][0], cfg->chroma_offset[0][i][0]);
    SET_TAB2_RLUT(16+i, get_phy_addr(cfg->chroma_ref_origin_addr[1][i]), 
		  cfg->chroma_weight[1][i][1], cfg->chroma_offset[1][i][1], 
		  cfg->chroma_weight[1][i][0], cfg->chroma_offset[1][i][0]);
  }
#else
  //printf("ref_addr_index = %d\n", cfg->ref_addr_index);
  //for(i=0; i<1; i++){
    i=0;
    SET_TAB1_RLUT(i,    get_phy_addr(cfg->luma_ref_addr[cfg->ref_addr_index][0][i]), 
		  cfg->luma_weight[0][i], cfg->luma_offset[0][i]);

    //SET_TAB1_RLUT(16+i, get_phy_addr(cfg->luma_ref_addr[cfg->ref_addr_index][1][i]), 
    //              cfg->luma_weight[1][i], cfg->luma_offset[1][i]);

    SET_TAB2_RLUT(i,    get_phy_addr(cfg->chroma_ref_addr[cfg->ref_addr_index][0][i]), 
		  cfg->chroma_weight[0][i][1], cfg->chroma_offset[0][i][1], 
		  cfg->chroma_weight[0][i][0], cfg->chroma_offset[0][i][0]);

    //SET_TAB2_RLUT(16+i, get_phy_addr(cfg->chroma_ref_addr[cfg->ref_addr_index][1][i]), 
    //	            cfg->chroma_weight[1][i][1], cfg->chroma_offset[1][i][1], 
    //	            cfg->chroma_weight[1][i][0], cfg->chroma_offset[1][i][0]);
    //}
#endif

  SET_REG1_PINFO(0,/*rgr*/
		 0,/*its*/
		 0,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);
  SET_REG2_PINFO(0,/*rgr*/
		 0,/*its*/
		 0,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);

  SET_REG1_WINFO(0,/*wt*/
		 (cfg->use_weight == IS_WT1), /*wtpd*/
		 cfg->use_weight,/*wtmd*/
		 1,/*biavg_rnd*/
		 cfg->luma_denom,/*wt_denom*/
		 5,/*wt_sft*/
		 0,/*wt_lcoef*/
		 0/*wt_rcoef*/);
  SET_REG1_WTRND(1<<5);

  SET_REG2_WINFO1(0,/*wt*/
		  cfg->use_weight_chroma && (cfg->use_weight == IS_WT1), /*wtpd*/
		  cfg->use_weight,/*wtmd*/
		  1,/*biavg_rnd*/
		  cfg->chroma_denom,/*wt_denom*/
		  5,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WINFO2(5,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WTRND(1<<5, 1<<5);

  //SET_REG1_DSTA(do_get_phy_addr(&mce->pred_out_y[0]));

  SET_REG1_STRD(cfg->linesize,16,DOUT_Y_STRD);
  SET_REG1_GEOM(cfg->mb_height*16,cfg->mb_width*16);

  //SET_REG1_DSA(do_get_phy_addr(&mce->end_flag));

  //SET_REG1_IWTA(do_get_phy_addr(mce->mv_cost_ptr));

  //SET_REG1_RAWA(do_get_phy_addr(mce->raw_data_y));
  
  //SET_REG2_DSTA(do_get_phy_addr(&mce->pred_out_u[0]));//how v?
  SET_REG2_STRD(cfg->linesize,0,DOUT_C_STRD);
}

void ALWAYS_INLINE mce_macroblock_config(MCE_VAR *mce_ptr){
  int pipe_index = v_tcsm1->pipe_index;
  MCE_VAR * const mce_var_phy_ptr = &tcsm1_phy->mce_var[pipe_index];

  SET_REG1_DSTA(&mce_var_phy_ptr->pred_out_y[0]);
  SET_REG1_DSA(&tcsm1_phy->mce_end_flag);

  SET_REG1_IWTA(&tcsm1_phy->mv_cost);   //FIXME: NO need to set each macroblock  
  SET_REG1_RAWA(&sram_phy->raw_data[pipe_index].y);

  SET_REG2_DSTA(&mce_var_phy_ptr->pred_out_u[0]);//how v?
}

#if 0
void esti_execute( MCE_VAR *mce /*, x264_me_t *m*/)
{
  TCSM1 *tcsm1_ptr = v_tcsm1;

  int *tdd = (int *)&mce->task_chain;

  int bmx, bmy;
  int pmx, pmy;


  int i_fmv_range = tcsm1_ptr->slice_var.i_fmv_range;

  // limit motion search to a slightly smaller range than the theoretical limit,
  // since the search may go a few iterations past its given range
  int i_fpel_border = 6; // umh: 1 for diamond, 2 for octagon, 2 for hpel

  /* Calculate max allowed MV range */
#define CLIP_FMV(mv) x264_clip3( mv, -i_fmv_range, i_fmv_range-1 )

  tcsm1_ptr->mb.mv_min[0] = 4*( -16*tcsm1_ptr->mb.i_mb_x - 24 );
  tcsm1_ptr->mb.mv_max[0] = 4*( 16*( tcsm1_ptr->slice_var.i_mb_width - tcsm1_ptr->mb.i_mb_x - 1 ) + 24 );

  tcsm1_ptr->mb.mv_min_spel[0] = CLIP_FMV( tcsm1_ptr->mb.mv_min[0] );
  tcsm1_ptr->mb.mv_max_spel[0] = CLIP_FMV( tcsm1_ptr->mb.mv_max[0] );

  tcsm1_ptr->mb.mv_min_fpel[0] = (tcsm1_ptr->mb.mv_min_spel[0]>>2) + i_fpel_border;
  tcsm1_ptr->mb.mv_max_fpel[0] = (tcsm1_ptr->mb.mv_max_spel[0]>>2) - i_fpel_border;
#undef CLIP_FMV

  int mv_x_min = X264_MAX(tcsm1_ptr->mb.mv_min_fpel[0], -16);
  int mv_x_max = X264_MIN(tcsm1_ptr->mb.mv_max_fpel[0],  16);

  int mv_y_min = X264_MAX(tcsm1_ptr->mb.mv_min_fpel[1], -16);
  int mv_y_max = X264_MIN(tcsm1_ptr->mb.mv_max_fpel[1],  16);


  bmx = x264_clip3( tcsm1_ptr->mb.mvp[0], mv_x_min*4, mv_x_max*4 );
  bmy = x264_clip3( tcsm1_ptr->mb.mvp[1], mv_y_min*4, mv_y_max*4 );

  pmx = (( bmx + 2 ) >> 2)<<2;
  pmy = (( bmy + 2 ) >> 2)<<2;

  tdd[0] = TDD_CFG(1, 1, REG1_MVINFO); 
  tdd[1] = (pmx & 0xFFFF) | ((pmy & 0xFFFF)<<16);
    
  tdd[2] = TDD_ESTI(1,/*vld*/
		    1,/*lk*/
		    0,/*dmy*/
		    1,/*pmc*/
		    0,/*list*/
		    0,/*boy*/
		    0,/*box*/
		    3,/*bh*/
		    3,/*bw*/
		    tcsm1_ptr->mb.i_mb_y,/*mby*/
		    tcsm1_ptr->mb.i_mb_x/*mbx*/);
#if 1
  tdd[3] = TDD_ESTI(1,/*vld*/
		    1,/*lk*/
		    1,/*dmy*/
		    1,/*pmc*/
		    0,/*list*/
		    0,/*boy*/
		    0,/*box*/
		    3,/*bh*/
		    3,/*bw*/
		    tcsm1_ptr->mb.i_mb_y,/*mby*/
		    tcsm1_ptr->mb.i_mb_x/*mbx*/);
  
  tdd[4] = TDD_SYNC(1,/*vld*/
		    0,/*lk*/
		    0xFFFF/*id*/);
#else
  tdd[3] = TDD_SYNC(1,/*vld*/
		    0,/*lk*/
		    0xFFFF/*id*/);

#endif


  tcsm1_ptr->mce_end_flag = 0;  
  SET_REG1_DDC(do_get_phy_addr(&mce->task_chain) + 1);

}
#endif

void motion_verify(const MCE_VAR *mce, const uint8_t *hw_y, const uint8_t *hw_u, const uint8_t *hw_v,
		   int mce_out_stride_c,
		   int mbx, int mby,
		   const uint8_t *dest_y, const uint8_t *dest_cb, const uint8_t *dest_cr,
		   int linesize, int uvlinesize)
{
  int i, j;
  int fail_y=0, fail_c=0;

  for(j=0; j<16; j++)
    for(i=0; i<16; i++)
      //if(mce->inter_pred_data_out_y[j*16+i] != dest_y[j*linesize+i])      
      if(hw_y[j*16+i] != dest_y[j*linesize+i])      
	fail_y++;

  for(j=0; j<8; j++)
    for(i=0; i<8; i++)
      //if((mce->inter_pred_data_out_uv[j*8+i]     != dest_cb[j*uvlinesize+i]) ||
      // (mce->inter_pred_data_out_uv[8*8+j*8+i] != dest_cr[j*uvlinesize+i]) )
      if((hw_u[j*mce_out_stride_c + i] != dest_cb[j*uvlinesize+i]) ||
	 (hw_v[j*mce_out_stride_c + i] != dest_cr[j*uvlinesize+i]) )
	fail_c++;
  
  if(fail_y || fail_c){
#undef printf
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("motion verify failed @(MBY[%d], MBX[%d])\n\n", mby, mbx);
  }

  if(fail_y){
    printf("[std dest_y]:\n");
    for(j=0; j<16; j++){
      for(i=0; i<16; i++)
	printf("  %02x", dest_y[j*linesize+i]);
      printf("\n");
    }

    printf("\n[motion y]:\n");
    for(j=0; j<16; j++){
      for(i=0; i<16; i++)
	printf("  %02x", hw_y[j*16+i]);
      printf("\n");
    }
  }

  if(fail_c){
    printf("[std dest_c]:\n");
    for(j=0; j<8; j++){
      for(i=0; i<8; i++)
	printf("  %02x", dest_cb[j*uvlinesize+i]);
      printf("  |");
      for(i=0; i<8; i++)
	printf("  %02x", dest_cr[j*uvlinesize+i]);
      printf("\n");
    }

    printf("\n[motion c]:\n");
    for(j=0; j<8; j++){
      for(i=0; i<8; i++)
	printf("  %02x", hw_u[j*mce_out_stride_c+i]);
      printf("  |");

      for(i=0; i<8; i++)
	printf("  %02x", hw_v[j*mce_out_stride_c+i]);
      
      printf("\n");
    }
  }

  if(fail_y || fail_c){
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("motion verify failed @(MBY[%d], MBX[%d])\n\n", mby, mbx);

    volatile int *tdd = (int *)&mce->task_chain;
    for(i=0; i<=16; i++)
      printf("TDD[%d]\t%08x\n", i, tdd[i]);

    printf("CTRL: %08x\n", (unsigned int)GET_REG1_CTRL());
    printf("STAT: %08x\n", (unsigned int)GET_REG1_STAT());
    printf("MBPOS: %08x\n", (unsigned int)GET_REG1_MBPOS());
    printf("REFA: %08x\n", (unsigned int)GET_REG1_REFA());

    printf("DSTA: %08x\n", (unsigned int)GET_REG1_DSTA());
    printf("PINFO: %08x\n", (unsigned int)GET_REG1_PINFO());
    printf("BINFO: %08x\n", (unsigned int)GET_REG1_BINFO());
    printf("IINFO1: %08x\n", (unsigned int)GET_REG1_IINFO1());
    printf("IINFO2: %08x\n", (unsigned int)GET_REG1_IINFO2());
    printf("WINFO: %08x\n", (unsigned int)GET_REG1_WINFO());
    printf("WTRND: %08x\n", (unsigned int)GET_REG1_WTRND());
    printf("TAP1L: %08x\n", (unsigned int)GET_REG1_TAP1L());
    printf("TAP1M: %08x\n", (unsigned int)GET_REG1_TAP1M());
    printf("TAP2L: %08x\n", (unsigned int)GET_REG1_TAP2L());
    printf("TAP2M: %08x\n", (unsigned int)GET_REG1_TAP2M());
    printf("STRD: %08x\n", (unsigned int)GET_REG1_STRD());
    printf("GEOM: %08x\n", (unsigned int)GET_REG1_GEOM());
    printf("DDC: %08x\n", (unsigned int)GET_REG1_DDC());
    printf("DSA: %08x\n\n", (unsigned int)GET_REG1_DSA());

    printf("STAT: %08x\n", (unsigned int)GET_REG2_STAT());
    printf("MBPOS: %08x\n", (unsigned int)GET_REG2_MBPOS());
    printf("REFA: %08x\n", (unsigned int)GET_REG2_REFA());
    printf("DSTA: %08x\n", (unsigned int)GET_REG2_DSTA());
    printf("PINFO: %08x\n", (unsigned int)GET_REG2_PINFO());
    printf("BINFO: %08x\n", (unsigned int)GET_REG2_BINFO());
    printf("IINFO1: %08x\n", (unsigned int)GET_REG2_IINFO1());
    printf("IINFO2: %08x\n", (unsigned int)GET_REG2_IINFO2());
    printf("WINFO1: %08x\n", (unsigned int)GET_REG2_WINFO1());
    printf("WINFO2: %08x\n", (unsigned int)GET_REG2_WINFO2());
    printf("WTRND: %08x\n", (unsigned int)GET_REG2_WTRND());
    printf("STRD: %08x\n", (unsigned int)GET_REG2_STRD());
    
    printf("INTP1: %d\n", (int)GET_PMON1_INTP());
    printf("INTP2: %d\n", (int)GET_PMON2_INTP());
    printf("WORK1: %d\n", (int)GET_PMON1_WORK());
    printf("WORK2: %d\n", (int)GET_PMON2_WORK());
    printf("IMISS1: %d\n", (int)GET_PMON1_IMISS());
    printf("IMISS2: %d\n", (int)GET_PMON2_IMISS());
    printf("PMISS1: %d\n", (int)GET_PMON1_PMISS());
    printf("PMISS2: %d\n", (int)GET_PMON2_PMISS());
    printf("TRAN1: %d\n", (int)GET_PMON1_TRAN());
    printf("TRAN2: %d\n", (int)GET_PMON2_TRAN());

    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
#undef exit
    exit(1);
  }

}


int verify_mce_output(){
  return 0;
}

int verify_mce_mv_cost( int8_t hw_mvx, int8_t hw_mvy, int16_t hw_cost, 
		        int8_t sw_mvx, int8_t sw_mvy, int16_t sw_cost, uint32_t *mce_end_flag_ptr){

    int mv_err = 0;

    const int wait_mce_loop_max = 10000000;
    int wait_mce_loop = 0;    

    do{
      wait_mce_loop++;
      if(wait_mce_loop > wait_mce_loop_max){
	printf("mce maybe dead\n");
	printf("mce_end_flag = %d\n", *mce_end_flag_ptr);

	mv_err = -1;
	break;
      }
    }while(*mce_end_flag_ptr != MCE_END_FLAG );

 
    if( hw_mvx != sw_mvx){
        printf("MVX Error  sw : %4d, hw : %4d\n", sw_mvx, hw_mvx);
	mv_err = -1;
    }

    if( hw_mvy != sw_mvy){
        printf("MVY Error  sw : %4d, hw : %4d\n", sw_mvy, hw_mvy);
	mv_err = -11;
    }

    if(hw_cost != sw_cost){
	printf("COST Error sw : %4d, hw : %4d\n", sw_cost, hw_cost);
	mv_err = -1;
    }
    
    return mv_err;
}


void tile_stuff(x264_t *h, uint8_t *tile_y, uint8_t *tile_c, 
		uint8_t *frame_y, uint8_t *frame_u, uint8_t *frame_v,
		int mb_height, int mb_width)
{
  uint8_t *pxl, *edge, *tile, *dest;
  int mb_i, mb_j, i, j;

  int linesize  = h->fdec->i_stride[0];
  int uvlinesize  = h->fdec->i_stride[1];

  for(mb_j=0; mb_j<mb_height; mb_j++){
    for(mb_i=0; mb_i<mb_width; mb_i++){
      tile = tile_y + mb_j*16*linesize + mb_i*16*16;
      dest = frame_y + mb_j*16*linesize + mb_i*16;
      for(j=0; j<16; j++){
	for(i=0; i<16; i++){
	  //printf("%2x, %2x,  ", *tile,  dest[i]);
	  *tile++ = dest[i];
	  //printf("%2x, ", dest[i]);
	}
	dest += linesize;
	printf("\n");
      }

      printf("\n");
      tile = tile_c + mb_j*8*linesize + mb_i*16*8;
      dest = frame_u + mb_j*8*uvlinesize + mb_i*8;
      for(j=0; j<8; j++){
	for(i=0; i<8; i++){
	  tile[i] = dest[i];
  	  //printf("%02x, ", dest[i]);
	}
	tile += 16;
	dest += uvlinesize;
	//printf("\n");
      }

      //printf("\n");
      tile = tile_c + mb_j*8*linesize + mb_i*16*8 + 8;
      dest = frame_v + mb_j*8*uvlinesize + mb_i*8;
      for(j=0; j<8; j++){
	for(i=0; i<8; i++){
	  tile[i] = dest[i];
	  //printf("%02x, ", dest[i]);
	}
	tile += 16;
	dest += uvlinesize;
	//printf("\n");
      }
      //printf("\n");
    }
  }

  /********************* top ***********************/
  for(mb_i=0; mb_i<mb_width; mb_i++){
    // Y
    pxl = tile_y + mb_i*16*16;
    edge = pxl - linesize*32;
    for(j=0; j<16; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+linesize*16, pxl, 16);
      edge += 16;
    }
    // C
    pxl = tile_c + mb_i*16*8;
    edge = pxl - linesize*16;
    for(j=0; j<8; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+linesize*8, pxl, 16);
      edge += 16;
    }
  }

  /********************* bottom ***********************/
  for(mb_i=0; mb_i<mb_width; mb_i++){
    // Y
    edge = tile_y + mb_height*16*linesize + mb_i*16*16;
    pxl = edge - linesize*16 + 16*15;
    for(j=0; j<16; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+linesize*16, pxl, 16);
      edge += 16;
    }
    // C
    edge = tile_c + mb_height*8*linesize + mb_i*16*8;
    pxl = edge - linesize*8 + 16*7;
    for(j=0; j<8; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+linesize*8, pxl, 16);
      edge += 16;
    }
  }
  /********************* left ***********************/
  for(mb_i=-2; mb_i<mb_height+2; mb_i++){
    // Y
    for(j=0; j<16; j++){
      pxl = tile_y + mb_i*16*linesize + 16*j;
      edge = pxl - 16*32;
      memset(edge, pxl[0], 16);
      memset(edge+16*16, pxl[0], 16);
    }
    // C
    for(j=0; j<8; j++){
      pxl = tile_c + mb_i*8*linesize + 16*j;
      edge = pxl - 8*32;
      memset(edge, pxl[0], 8);
      memset(edge+8, pxl[8], 8);
      memset(edge+8*16, pxl[0], 8);
      memset(edge+8*16+8, pxl[8], 8);
    }
  }

  /********************* right ***********************/
  for(mb_i=-2; mb_i<mb_height+2; mb_i++){
    // Y
    for(j=0; j<16; j++){
      pxl = tile_y + (mb_width-1)*16*16 + 15 + mb_i*16*linesize + 16*j;
      edge = pxl - 15 + 16*16;
      memset(edge, pxl[0], 16);
      memset(edge+16*16, pxl[0], 16);
    }
    // C
    for(j=0; j<8; j++){
      pxl = tile_c + (mb_width-1)*16*8 + 7 + mb_i*8*linesize + 16*j;
      edge = pxl - 7 + 16*8;
      memset(edge, pxl[0], 8);
      memset(edge+8, pxl[8], 8);
      memset(edge+8*16, pxl[0], 8);
      memset(edge+8*16+8, pxl[8], 8);
    }
  }
}

void add_edge(x264_t *h, uint8_t *tile_y, uint8_t *tile_c, 
		int mb_height, int mb_width)
{
  uint8_t *pxl, *edge;
  int mb_i, j;

  int linesize  = h->fdec->i_stride[0];

  /********************* top ***********************/
  for(mb_i=0; mb_i<mb_width; mb_i++){
    // Y
    pxl = tile_y + mb_i*16*16;
    edge = pxl - linesize*32;
    for(j=0; j<16; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+linesize*16, pxl, 16);
      edge += 16;
    }
    // C
    pxl = tile_c + mb_i*16*8;
    edge = pxl - linesize*16;
    for(j=0; j<8; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+linesize*8, pxl, 16);
      edge += 16;
    }
  }

  /********************* bottom ***********************/
  for(mb_i=0; mb_i<mb_width; mb_i++){
    // Y
    edge = tile_y + mb_height*16*linesize + mb_i*16*16;
    pxl = edge - linesize*16 + 16*15;
    for(j=0; j<16; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+linesize*16, pxl, 16);
      edge += 16;
    }
    // C
    edge = tile_c + mb_height*8*linesize + mb_i*16*8;
    pxl = edge - linesize*8 + 16*7;
    for(j=0; j<8; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+linesize*8, pxl, 16);
      edge += 16;
    }
  }
  /********************* left ***********************/
  for(mb_i=-2; mb_i<mb_height+2; mb_i++){
    // Y
    for(j=0; j<16; j++){
      pxl = tile_y + mb_i*16*linesize + 16*j;
      edge = pxl - 16*32;
      memset(edge, pxl[0], 16);
      memset(edge+16*16, pxl[0], 16);
    }
    // C
    for(j=0; j<8; j++){
      pxl = tile_c + mb_i*8*linesize + 16*j;
      edge = pxl - 8*32;
      memset(edge, pxl[0], 8);
      memset(edge+8, pxl[8], 8);
      memset(edge+8*16, pxl[0], 8);
      memset(edge+8*16+8, pxl[8], 8);
    }
  }

  /********************* right ***********************/
  for(mb_i=-2; mb_i<mb_height+2; mb_i++){
    // Y
    for(j=0; j<16; j++){
      pxl = tile_y + (mb_width-1)*16*16 + 15 + mb_i*16*linesize + 16*j;
      edge = pxl - 15 + 16*16;
      memset(edge, pxl[0], 16);
      memset(edge+16*16, pxl[0], 16);
    }
    // C
    for(j=0; j<8; j++){
      pxl = tile_c + (mb_width-1)*16*8 + 7 + mb_i*8*linesize + 16*j;
      edge = pxl - 7 + 16*8;
      memset(edge, pxl[0], 8);
      memset(edge+8, pxl[8], 8);
      memset(edge+8*16, pxl[0], 8);
      memset(edge+8*16+8, pxl[8], 8);
    }
  }
}
