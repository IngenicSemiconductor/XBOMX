/*******************************************************
 Motion Test Center
 ******************************************************/
#define __place_k0_data__
#undef printf
#undef fprintf
#include "t_motion_p0.h"
#include "rv9_tcsm.h" 
#include "t_intpid.h"

void motion_init_rv9(int intpid, int cintpid)
{
  int i;
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
  SET_REG1_CTRL(0,/*esms*/
		0,/*erms*/
		0,/*earm*/
		0,/*pmve*/
		0,/*emet*/
		0,/*esa*/
		1,/*cae*/
		0,/*csf*/
		0xF,/*pgc*/
		1,/*ch2en*/
		3,/*pri*/
		1,/*ckge*/
		1,/*ofa*/
		0,/*rote*/
		0,/*rotdir*/
		0,/*wm*/
		1,/*ccf*/
		0,/*irqe*/
		0,/*rst*/
		1 /*en*/);

  SET_REG1_BINFO(AryFMT[intpid],/*ary*/
		 0,/*doe*/
		 0,/*expdy*/
		 0,/*expdx*/
		 0,/*ilmd*/
		 SubPel[intpid]-1,/*pel*/
		 0,/*fld*/
		 0,/*fldsel*/
		 0,/*boy*/
		 0,/*box*/
		 0,/*bh*/
		 0,/*bw*/
		 0/*pos*/);
  SET_REG2_BINFO(AryFMT[cintpid],/*ary*/
		 0,/*doe*/
		 0,/*expdy*/
		 0,/*expdx*/
		 0,/*ilmd*/
		 SubPel[cintpid]-1,/*pel*/
		 0,/*fld*/
		 0,/*fldsel*/
		 0,/*boy*/
		 0,/*box*/
		 0,/*bh*/
		 0,/*bw*/
		 0/*pos*/);
#if 1
  SET_REG1_PINFO(0,/*rgr*/
		 0,/*its*/
		 6,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);
  SET_REG2_PINFO(0,/*rgr*/
		 0,/*its*/
		 6,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);

  SET_REG1_WINFO(0,/*wt*/
		 0, /*wtpd*/
		 IS_BIAVG,/*wtmd*/
		 1,/*biavg_rnd*/
		 0,/*wt_denom*/
		 0,/*wt_sft*/
		 0,/*wt_lcoef*/
		 0/*wt_rcoef*/);
  SET_REG1_WTRND(0);

  SET_REG2_WINFO1(0,/*wt*/
		  0, /*wtpd*/
		  IS_BIAVG,/*wtmd*/
		  1,/*biavg_rnd*/
		  0,/*wt_denom*/
		  0,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WINFO2(0,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WTRND(0, 0);
#endif
}

void motion_config_rv9(MpegEncContext *s)
{
#ifdef IPU_BUG
    int y_strd=(s->mb_width*256 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
    int c_strd=(s->mb_width*128 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
#endif
    
    SET_REG1_CTRL(0,/*esms*/
		  0,/*erms*/
		  0,/*earm*/
		  0,/*pmve*/
		  0,/*emet*/
		  0,/*esa*/
		  1,/*cae*/
		  0,/*csf*/
		  0xF,/*pgc*/
		  1,/*ch2en*/
		  3,/*pri*/
		  1,/*ckge*/
		  1,/*ofa*/
		  0,/*rote*/
		  0,/*rotdir*/
		  0,/*wm*/
		  1,/*ccf*/
		  0,/*irqe*/
		  0,/*rst*/
		  1 /*en*/);

    SET_TAB1_RLUT(0, (int)(s->last_picture.data[0]), 0, 0); 
    SET_TAB1_RLUT(16, (int)(s->next_picture.data[0]), 0, 0); 
    SET_TAB2_RLUT(0, (int)(s->last_picture.data[1]), 0, 0, 0, 0); 
    SET_TAB2_RLUT(16, (int)(s->next_picture.data[1]), 0, 0, 0, 0);  

#ifdef IPU_BUG
    SET_REG1_STRD(y_strd/16,0,PREVIOUS_LUMA_STRIDE);
    SET_REG2_STRD(c_strd/8,0,PREVIOUS_CHROMA_STRIDE);
#else
    SET_REG1_STRD(s->linesize/16,0,PREVIOUS_LUMA_STRIDE);
    SET_REG2_STRD(s->linesize/16,0,PREVIOUS_CHROMA_STRIDE);
#endif
    SET_REG1_GEOM(s->mb_height*16,s->mb_width*16);
}

