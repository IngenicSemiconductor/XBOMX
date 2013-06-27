#include "jzm_mpeg2_dec.h"
#include "../libjzcommon/jzasm.h"
#include <utils/Log.h>
__place_k0_data__ unsigned int MBA_5_HW[8] = {
  0x55650000, 0x34344444, 0x23232323, 0x13131313, 0x01010101, 0x01010101, 0x01010101, 0x01010101, // 0x0 
};
__place_k0_data__ unsigned int MBA_11_HW[64] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 0x0 
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1f0b200b, 0x1d0b1e0b, 0x1b0b1c0b, 0x190b1a0b, // 0x1 
  0x170b180b, 0x150b160b, 0x140a140a, 0x130a130a, 0x120a120a, 0x110a110a, 0x100a100a, 0x0f0a0f0a, // 0x2 
  0x0e080e08, 0x0e080e08, 0x0e080e08, 0x0e080e08, 0x0d080d08, 0x0d080d08, 0x0d080d08, 0x0d080d08, // 0x3 
  0x0c080c08, 0x0c080c08, 0x0c080c08, 0x0c080c08, 0x0b080b08, 0x0b080b08, 0x0b080b08, 0x0b080b08, // 0x4 
  0x0a080a08, 0x0a080a08, 0x0a080a08, 0x0a080a08, 0x09080908, 0x09080908, 0x09080908, 0x09080908, // 0x5 
  0x08070807, 0x08070807, 0x08070807, 0x08070807, 0x08070807, 0x08070807, 0x08070807, 0x08070807, // 0x6 
  0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x07070707, // 0x7 
};
__place_k0_data__ unsigned int MB_P_HW[16] = {
  0x12051106, 0x01051a05, 0x08030803, 0x08030803, 0x02020202, 0x02020202, 0x02020202, 0x02020202, // 0x0 
  0x0a010a01, 0x0a010a01, 0x0a010a01, 0x0a010a01, 0x0a010a01, 0x0a010a01, 0x0a010a01, 0x0a010a01, // 0x1 
};
__place_k0_data__ unsigned int MB_B_HW[32] = {
  0x11060006, 0x1a061606, 0x1e051e05, 0x01050105, 0x08040804, 0x08040804, 0x0a040a04, 0x0a040a04, // 0x0 
  0x04030403, 0x04030403, 0x04030403, 0x04030403, 0x06030603, 0x06030603, 0x06030603, 0x06030603, // 0x1 
  0x0c020c02, 0x0c020c02, 0x0c020c02, 0x0c020c02, 0x0c020c02, 0x0c020c02, 0x0c020c02, 0x0c020c02, // 0x2 
  0x0e020e02, 0x0e020e02, 0x0e020e02, 0x0e020e02, 0x0e020e02, 0x0e020e02, 0x0e020e02, 0x0e020e02, // 0x3 
};
__place_k0_data__ unsigned int CBP_7_HW[64] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 0x0 
  0x12071107, 0x18071407, 0x22072107, 0x28072407, 0x3f063f06, 0x30063006, 0x09060906, 0x06060606, // 0x1 
  0x1f051f05, 0x1f051f05, 0x10051005, 0x10051005, 0x2f052f05, 0x2f052f05, 0x20052005, 0x20052005, // 0x2 
  0x07050705, 0x07050705, 0x0b050b05, 0x0b050b05, 0x0d050d05, 0x0d050d05, 0x0e050e05, 0x0e050e05, // 0x3 
  0x05050505, 0x05050505, 0x0a050a05, 0x0a050a05, 0x03050305, 0x03050305, 0x0c050c05, 0x0c050c05, // 0x4 
  0x01040104, 0x01040104, 0x01040104, 0x01040104, 0x02040204, 0x02040204, 0x02040204, 0x02040204, // 0x5 
  0x04040404, 0x04040404, 0x04040404, 0x04040404, 0x08040804, 0x08040804, 0x08040804, 0x08040804, // 0x6 
  0x0f030f03, 0x0f030f03, 0x0f030f03, 0x0f030f03, 0x0f030f03, 0x0f030f03, 0x0f030f03, 0x0f030f03, // 0x7 
};
__place_k0_data__ unsigned int CBP_9_HW[32] = {
  0x00090009, 0x36093909, 0x3b093709, 0x3e093d09, 0x17081708, 0x1b081b08, 0x1d081d08, 0x1e081e08, // 0x0 
  0x27082708, 0x2b082b08, 0x2d082d08, 0x2e082e08, 0x19081908, 0x16081608, 0x29082908, 0x26082608, // 0x1 
  0x35083508, 0x3a083a08, 0x33083308, 0x3c083c08, 0x15081508, 0x1a081a08, 0x13081308, 0x1c081c08, // 0x2 
  0x25082508, 0x2a082a08, 0x23082308, 0x2c082c08, 0x31083108, 0x32083208, 0x34083408, 0x38083808, // 0x3 
};
__place_k0_data__ unsigned int DC_lum_5_HW[8] = {
  0x12121212, 0x12121212, 0x22222222, 0x22222222, 0x03030303, 0x33333333, 0x43434343, 0x00655454, // 0x0 
};
__place_k0_data__ unsigned int DC_chrom_5_HW[8] = {
  0x02020202, 0x02020202, 0x12121212, 0x12121212, 0x22222222, 0x22222222, 0x33333333, 0x00554444, // 0x0 
};
__place_k0_data__ unsigned int DC_long_HW[8] = {
  0x65656565, 0x65656565, 0x65656565, 0x65656565, 0x76767676, 0x76767676, 0x87878787, 0xb9a99898, // 0x0 
};
__place_k0_data__ unsigned int DCT_B15_8_HW[128] = {
  0x00000000, 0x00000000, 0x41004100, 0x41004100, 0x08170817, 0x09170917, 0x07170717, 0x03270327, // 0x0 
  0x01760176, 0x01760176, 0x01660166, 0x01660166, 0x05160516, 0x05160516, 0x06160616, 0x06160616, // 0x1 
  0x0c180258, 0x01a801b8, 0x0d180e18, 0x02480428, 0x03150315, 0x03150315, 0x03150315, 0x03150315, // 0x2 
  0x02250225, 0x02250225, 0x02250225, 0x02250225, 0x04150415, 0x04150415, 0x04150415, 0x04150415, // 0x3 
  0x02130213, 0x02130213, 0x02130213, 0x02130213, 0x02130213, 0x02130213, 0x02130213, 0x02130213, // 0x4 
  0x02130213, 0x02130213, 0x02130213, 0x02130213, 0x02130213, 0x02130213, 0x02130213, 0x02130213, // 0x5 
  0x81048104, 0x81048104, 0x81048104, 0x81048104, 0x81048104, 0x81048104, 0x81048104, 0x81048104, // 0x6 
  0x01340134, 0x01340134, 0x01340134, 0x01340134, 0x01340134, 0x01340134, 0x01340134, 0x01340134, // 0x7 
  0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, // 0x8 
  0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, // 0x9 
  0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, // 0xa 
  0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, 0x01120112, // 0xb 
  0x01230123, 0x01230123, 0x01230123, 0x01230123, 0x01230123, 0x01230123, 0x01230123, 0x01230123, // 0xc 
  0x01230123, 0x01230123, 0x01230123, 0x01230123, 0x01230123, 0x01230123, 0x01230123, 0x01230123, // 0xd 
  0x01450145, 0x01450145, 0x01450145, 0x01450145, 0x01550155, 0x01550155, 0x01550155, 0x01550155, // 0xe 
  0x0a170a17, 0x02370237, 0x0b170b17, 0x01870187, 0x01970197, 0x01d801c8, 0x05280338, 0x01f801e8, // 0xf 
};
__place_k0_data__ unsigned int DCT_B15_10_HW[8] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x06290629, 0x0f190f19, 0x111a034a, 0x10191019, // 0x0 
};
__place_k0_data__ unsigned int DCT_13_HW[32] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 0x0 
  0x0a2d0b2d, 0x044d063d, 0x027d035d, 0x01fd026d, 0x01dd01ed, 0x1b1d01cd, 0x191d1a1d, 0x171d181d, // 0x1 
  0x01bc01bc, 0x092c092c, 0x053c053c, 0x01ac01ac, 0x034c034c, 0x082c082c, 0x161c161c, 0x151c151c, // 0x2 
  0x019c019c, 0x141c141c, 0x131c131c, 0x025c025c, 0x043c043c, 0x018c018c, 0x072c072c, 0x121c121c, // 0x3 
};
__place_k0_data__ unsigned int DCT_15_HW[32] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, // 0x0 
  0x127f128f, 0x125f126f, 0x123f124f, 0x121f122f, 0x20ef120f, 0x20cf20df, 0x20af20bf, 0x208f209f, // 0x1 
  0x11fe11fe, 0x11ee11ee, 0x11de11de, 0x11ce11ce, 0x11be11be, 0x11ae11ae, 0x119e119e, 0x118e118e, // 0x2 
  0x117e117e, 0x116e116e, 0x115e115e, 0x114e114e, 0x113e113e, 0x112e112e, 0x111e111e, 0x110e110e, // 0x3 
};
__place_k0_data__ unsigned int DCT_16_HW[16] = {
  0x81008100, 0x81008100, 0x81008100, 0x81008100, 0x81008100, 0x81008100, 0x81008100, 0x81008100, // 0x0 
  0x02110212, 0x020f0210, 0x11020703, 0x0f021002, 0x0d020e02, 0x20010c02, 0x1e011f01, 0x1c011d01, // 0x1 
};
__place_k0_data__ unsigned int DCT_B14AC_5_HW[16] = {
  0x00000000, 0x00000000, 0x01350000, 0x04150515, 0x01240124, 0x03140314, 0x02130213, 0x02130213, // 0x0 
  0x81028102, 0x81028102, 0x81028102, 0x81028102, 0x01120112, 0x01120112, 0x01120112, 0x01120112, // 0x1 
};
__place_k0_data__ unsigned int DCT_B14DC_5_HW[16] = {
  0x00000000, 0x00000000, 0x01350000, 0x04150515, 0x01240124, 0x03140314, 0x02130213, 0x02130213, // 0x0 
  0x01110111, 0x01110111, 0x01110111, 0x01110111, 0x01110111, 0x01110111, 0x01110111, 0x01110111, // 0x1 
};
__place_k0_data__ unsigned int DCT_B14_8_HW[20] = {
  0x00000000, 0x00000000, 0x41004100, 0x41004100, 0x03270327, 0x0a170a17, 0x01470147, 0x09170917, // 0x0 
  0x08160816, 0x08160816, 0x07160716, 0x07160716, 0x02260226, 0x02260226, 0x06160616, 0x06160616, // 0x1 
  0x01680e18, 0x0c180d18, 0x02380428, 0x0b180158, };
__place_k0_data__ unsigned int DCT_B14_10_HW[8] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x062a111a, 0x033a017a, 0x101a024a, 0x052a0f1a, // 0x0 
};
__place_k0_data__ unsigned int MV_4_HW[2] = {
  0x13132436, 0x02020202};
__place_k0_data__ unsigned int MV_10_HW[12] = {
  0x0a0a0a0a, 0x0a0a0a0a, 0x0a0a0a0a, 0xcadaeafa, 0x9999aaba, 0x79798989, 0x67676767, 0x67676767, // 0x0 
  0x57575757, 0x57575757, 0x47474747, 0x47474747, };
__place_k0_data__ unsigned int MB_I_HW[1] = {
  //0x01020101,
  0x00000546,
};
__place_k0_data__ int DMV_2_HW[1] = {
  0x0000e611,
};

extern volatile unsigned char *gp0_base;
extern volatile unsigned char *vpu_base;
int M2D_SliceInit(_M2D_SliceInfo *s)
{
  int i, j;
  _M2D_BitStream *bs= &s->bs_buf;

  volatile unsigned int *chn = (volatile unsigned int *)s->des_va;
  /*-------------------------sch init begin-------------------------------*/
  { /*configure sch*/
    /*GEN_VDMA_ACFG(chn, REG_SCH_GLBC, 0, 
      (SCH_INTE_ACFGERR | SCH_INTE_BSERR | SCH_INTE_ENDF) );*/ /*open interrupt*/
    //ALOGE("REG_SCH_GLBC %x", (SCH_INTE_ACFGERR | SCH_INTE_BSERR | SCH_INTE_ENDF));
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH3_HID(HID_DBLK) |
					SCH_CH2_HID(HID_VMAU) |
					SCH_CH1_HID(HID_MCE) |
					SCH_DEPTH(MPEG2_FIFO_DEP) 
					)
		  );
    //ALOGE("REG_SCH_BND %x", (SCH_CH3_HID(HID_DBLK) | SCH_CH2_HID(HID_VMAU) | SCH_CH1_HID(HID_MCE) | SCH_DEPTH(MPEG2_FIFO_DEP)));
					
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG0, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG1, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE1, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE2, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE3, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE4, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, (((s->coding_type != I_TYPE)<<2) | 
					 SCH_CH2_PE| 
					 SCH_CH3_PE					 
					 )
		  );
    //ALOGE("REG_SCH_SCHC %x", (((s->coding_type != I_TYPE)<<2) | SCH_CH2_PE|SCH_CH3_PE));
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH3_HID(HID_DBLK) |
					SCH_CH2_HID(HID_VMAU) |
					SCH_CH1_HID(HID_MCE) |
					SCH_DEPTH(MPEG2_FIFO_DEP) |
					((s->coding_type != I_TYPE)<<0) | 
					SCH_BND_G0F2 | 
					SCH_BND_G0F3 					
					) 		  
		  );
  }
  /*-------------------------sch init end-------------------------------*/  

  /*-------------------------mc init begin-------------------------------*/
  { /*configure motion*/
    int intpid= MPEG_HPEL; 
    int cintpid= MPEG_HPEL;
    for(i=0; i<16; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_ILUT+i*8, 0, 
		  MCE_CH1_IINFO(IntpFMT[intpid][i].intp[0],/*intp1*/
				IntpFMT[intpid][i].tap,/*tap*/
				IntpFMT[intpid][i].intp_pkg[0],/*intp1_pkg*/
				IntpFMT[intpid][i].hldgl,/*hldgl*/
				IntpFMT[intpid][i].avsdgl,/*avsdgl*/
				IntpFMT[intpid][i].intp_dir[0],/*intp0_dir*/
				IntpFMT[intpid][i].intp_rnd[0],/*intp0_rnd*/
				IntpFMT[intpid][i].intp_sft[0],/*intp0_sft*/
				IntpFMT[intpid][i].intp_sintp[0],/*sintp0*/
				IntpFMT[intpid][i].intp_srnd[0],/*sintp0_rnd*/
				IntpFMT[intpid][i].intp_sbias[0]/*sintp0_bias*/
				));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_ILUT+i*8+4, 0, 
		  MCE_CH1_IINFO(IntpFMT[intpid][i].intp[1],/*intp1*/
				0,/*tap*/
				IntpFMT[intpid][i].intp_pkg[1],/*intp1_pkg*/
				IntpFMT[intpid][i].hldgl,/*hldgl*/
				IntpFMT[intpid][i].avsdgl,/*avsdgl*/
				IntpFMT[intpid][i].intp_dir[1],/*intp1_dir*/
				IntpFMT[intpid][i].intp_rnd[1],/*intp1_rnd*/
				IntpFMT[intpid][i].intp_sft[1],/*intp1_sft*/
				IntpFMT[intpid][i].intp_sintp[1],/*sintp1*/
				IntpFMT[intpid][i].intp_srnd[1],/*sintp1_rnd*/
				IntpFMT[intpid][i].intp_sbias[1]/*sintp1_bias*/
				));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+i*8+4, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[0][7],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[0][6],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[0][5],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[0][4]/*coef5*/
			      ));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+i*8, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[0][3],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[0][2],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[0][1],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[0][0]/*coef5*/
			      ));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+(i+16)*8+4, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[1][7],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[1][6],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[1][5],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[1][4]/*coef5*/
			      ));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+(i+16)*8, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[1][3],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[1][2],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[1][1],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[1][0]/*coef5*/
			      ));
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_ILUT+i*8, 0, 
		  MCE_CH2_IINFO(IntpFMT[cintpid][i].intp[0],
				IntpFMT[cintpid][i].intp_dir[0],
				IntpFMT[cintpid][i].intp_sft[0],
				IntpFMT[cintpid][i].intp_coef[0][0],
				IntpFMT[cintpid][i].intp_coef[0][1],
				IntpFMT[cintpid][i].intp_rnd[0]) );
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_ILUT+i*8+4, 0, 
		  MCE_CH2_IINFO(IntpFMT[cintpid][i].intp[1],
				IntpFMT[cintpid][i].intp_dir[1],
				IntpFMT[cintpid][i].intp_sft[1],
				IntpFMT[cintpid][i].intp_coef[1][0],
				IntpFMT[cintpid][i].intp_coef[1][1],
				IntpFMT[cintpid][i].intp_rnd[1]) );
    }
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_BINFO, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_BINFO, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CTRL, 0, ( MCE_COMP_AUTO_EXPD |
					  MCE_CH2_EN |
					  MCE_CLKG_EN |
					  MCE_OFA_EN |
					  MCE_CACHE_FLUSH |
					  MCE_EN 
					  ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_PINFO, 0, MCE_PINFO(0, 0, 6, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_PINFO, 0, MCE_PINFO(0, 0, 6, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_WINFO, 0, MCE_WINFO(0, 0, 0, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_WTRND, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO1, 0, MCE_WINFO(0, 0, 0, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO2, 0, 0);
    
    // configure ref frame.
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+ i*8+ 4, 0, s->pic_ref[0][i]); 
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+ i*8+ 128 + 4, 0, s->pic_ref[0][i]);
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+ i*8+ 4, 0, s->pic_ref[1][i]); 
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+ i*8+ 128 + 4, 0, s->pic_ref[1][i]); 
    }

    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WTRND, 0, 0);
    //GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(s->mb_width*16, 0, 16) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(s->y_stride / 16, 0, 16) );
    GEN_VDMA_ACFG(chn, REG_MCE_GEOM, 0, MCE_GEOM(s->mb_height*16, s->mb_width*16) );
    //GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(s->mb_width*16, 0, DOUT_C_STRD) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(s->c_stride / 8, 0, DOUT_C_STRD) );
    GEN_VDMA_ACFG(chn, REG_MCE_DSA, 0, SCH1_DSA);
    GEN_VDMA_ACFG(chn, REG_MCE_DDC, 0, TCSM1_MOTION_DHA);
  }
  /*-------------------------mc init end-------------------------------*/
  
  /*-------------------------vmau init begin-------------------------------*/
  { /*configure vmau*/
    for ( i = 0 ; i < 4; i++){ /*write qt table*/
      for ( j = 0 ; j < 16; j++){
	unsigned int idx= REG_VMAU_QT+ i*64 + (j<<2);
	GEN_VDMA_ACFG(chn, idx, 0, s->coef_qt[i][j]);
      }
    }
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_RUN, 0, VMAU_RESET);
    GEN_VDMA_ACFG(chn, REG_VMAU_VIDEO_TYPE, 0, VMAU_FMT_MPEG2);
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_STR, 0, (16<<16)|16);  
    //ALOGE("REG_VMAU_DEC_STR %x", (16<<16)|16);
    GEN_VDMA_ACFG(chn, REG_VMAU_Y_GS, 0, s->mb_width*16);
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_CTR, 0, (VMAU_CTRL_TO_DBLK | VMAU_CTRL_FIFO_M) );
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_DONE, 0, SCH2_DSA);
    GEN_VDMA_ACFG(chn, REG_VMAU_NCCHN_ADDR, 0, VMAU_CHN_BASE);      
  }
  /*-------------------------vmau init end-------------------------------*/
  
  /*-------------------------dblk init begin-------------------------------*/
  { /*configure dblk*/
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_RESET);
    //GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, DBLK_GPIC_STR(s->mb_width*128, s->mb_width*256) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, DBLK_GPIC_STR(s->c_stride, s->y_stride) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_YA, 0, s->pic_cur[0]);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_CA, 0, s->pic_cur[1]);
    GEN_VDMA_ACFG(chn, REG_DBLK_GSIZE, 0, DBLK_GSIZE(s->mb_height, s->mb_width) );
    GEN_VDMA_ACFG(chn, REG_DBLK_VTR, 0, DBLK_VTR(0, 0, 0, 0,
					       0, DBLK_FMT_MPEG2) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GP_ENDA, 0, DBLK_END_FLAG);
    GEN_VDMA_ACFG(chn, REG_DBLK_GENDA, 0, SCH3_DSA);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPOS, 0, DBLK_GPOS(s->mb_pos_y, s->mb_pos_x) );
    GEN_VDMA_ACFG(chn, REG_DBLK_DHA, 0, DBLK_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_SLICE_RUN);
  }
  /*-------------------------dblk init end-------------------------------*/

  /*-------------------------SDE init begin-------------------------------*/
  { /*configure sde*/
    GEN_VDMA_ACFG(chn, REG_SDE_CODEC_ID, 0, SDE_FMT_MPEG2_DEC);
    //ALOGE("REG_SDE_CODEC_ID %x", SDE_FMT_MPEG2_DEC);
    { /*fill vlc_ram*/
      // mb
      if (s->coding_type == I_TYPE) {
	for (i= 0; i< MB_I_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((MB_I_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_I_HW[i]);
	  //ALOGE("MB_I_HW: %x  %x", idx, MB_I_HW[i]);
	} 
      }
      if (s->coding_type == P_TYPE) {
	for (i= 0; i< MB_P_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((MB_P_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_P_HW[i]);
	} 
      }
      if (s->coding_type == B_TYPE) {
	for (i= 0; i< MB_B_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((MB_B_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_B_HW[i]);
	} 
      }
      // mba, dct
      for (i= 0; i< MBA_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((MBA_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MBA_5_HW[i]);
	//ALOGE("MBA_5_HW: %x %x", idx, MBA_5_HW[i]);
      } 
      for (i= 0; i< MBA_11_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((MBA_11_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MBA_11_HW[i]);
	//ALOGE("MBA_11_HW %x %x", idx, MBA_11_HW[i]);
      } 
      for (i= 0; i< DC_lum_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DC_lum_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_lum_5_HW[i]);
	//ALOGE("DC_lum_5_HW %x %x", idx, DC_lum_5_HW[i]);
      } 
      for (i= 0; i< DC_long_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DC_long_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_long_HW[i]);
	//ALOGE("DC_long_HW %x %x", idx, DC_long_HW[i]);
      } 
      for (i= 0; i< DC_chrom_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DC_chrom_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_chrom_5_HW[i]);
	//ALOGE("DC_chrom_5_HW %x %x", idx, DC_chrom_5_HW[i]);
      } 
      for (i= 0; i< DCT_B15_8_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B15_8_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B15_8_HW[i]);
	//ALOGE("DCT_B15_8_HW %x %x", idx, DCT_B15_8_HW[i]);
      } 
      for (i= 0; i< DCT_B15_10_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B15_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B15_10_HW[i]);
	//ALOGE("DCT_B15_10_HW %x %x", idx, DCT_B15_10_HW[i]);
      } 
      for (i= 0; i< DCT_13_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_13_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_13_HW[i]);
	//ALOGE("DCT_13_HW %x %x", idx, DCT_13_HW[i]);
      } 
      for (i= 0; i< DCT_15_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_15_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_15_HW[i]);
	//ALOGE("DCT_15_HW %x %x", idx, DCT_15_HW[i]);
      } 
      for (i= 0; i< DCT_16_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_16_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_16_HW[i]);
	//ALOGE("DCT_16_HW %x %x", idx, DCT_16_HW[i]);
      } 
      for (i= 0; i< DCT_B14AC_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B14AC_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14AC_5_HW[i]);
	//ALOGE("DCT_B14AC_5_HW %x %x", idx, DCT_B14AC_5_HW[i]);
      } 
      for (i= 0; i< DCT_B14DC_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B14DC_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14DC_5_HW[i]);
	//ALOGE("DCT_B14DC_5_HW %x %x", idx, DCT_B14DC_5_HW[i]);
      } 
      for (i= 0; i< DCT_B14_8_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B14_8_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14_8_HW[i]);
	//ALOGE("DCT_B14_8_HW %x %x", idx, DCT_B14_8_HW[i]);
      } 
      for (i= 0; i< DCT_B14_10_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B14_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14_10_HW[i]);
	//ALOGE("DCT_B14_10_HW %x %x", idx, DCT_B14_10_HW[i]);
      } 
      
      // cbp
      if (s->coding_type != I_TYPE) {
	for (i= 0; i< CBP_7_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((CBP_7_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, CBP_7_HW[i]);
	  //ALOGE("CBP_7_HW %x %x", idx, CBP_7_HW[i]);
	} 
	for (i= 0; i< CBP_9_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((CBP_9_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, CBP_9_HW[i]);
	  //ALOGE("CBP_9_HW %x %x", idx, CBP_9_HW[i]);
	}       
      }
      // mv
      for (i= 0; i< MV_4_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((MV_4_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MV_4_HW[i]);
	//ALOGE("MV_4_HW %x %x", idx, MV_4_HW[i]);
      }       
      for (i= 0; i< MV_10_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((MV_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MV_10_HW[i]);
	//ALOGE("MV_10_HW %x %x", idx, MV_10_HW[i]);
      }       
      for (i= 0; i< DMV_2_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DMV_2_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DMV_2_HW[i]);
	//ALOGE("DMV_2_HW %x %x", idx, DMV_2_HW[i]);
      }       
    }
    
    for (i= 0; i< 16; i++){ /*fill scan_ram*/
      unsigned int idx= REG_SDE_CTX_TBL+ 0x1000+ (i<<2);
      GEN_VDMA_ACFG(chn, idx, 0, s->scan[i]);
      //ALOGE("s->scan %x %x", idx, s->scan[i]);
    }
     
    GEN_VDMA_ACFG(chn, REG_SDE_CFG2, 0, bs->buffer);
    int *tbsb = (int *)bs->buffer;
    //ALOGE("REG_SDE_CFG2 %x %x %x %x %x", bs->buffer, tbsb[0], tbsb[1], tbsb[2], tbsb[3]);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG1, 0, bs->bit_ofst);
    //ALOGE("REG_SDE_CFG1 %x", bs->bit_ofst);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG0, 0, GET_SL_INFO(s->coding_type, /*coding_type*/ 
						    s->fr_pred_fr_dct, /*fr_pred_fr_dct*/
						    s->pic_type, /*pic_type*/
						    s->conceal_mv, /*conceal_mv*/
						    s->intra_dc_pre, /*intra_dc_pre*/
						    s->intra_vlc_format, /*intra_vlc_format*/
						    s->mpeg1, /*mpeg1*/
						    s->top_fi_first, /*top_fi_first*/ 
						    s->q_scale_type, /*q_scale_type*/
						    s->sec_fld,
						    s->f_code[0][0], /*f_code_f0*/
						    s->f_code[0][1], /*f_code_f1*/
						    s->f_code[1][0], /*f_code_b0*/
						    s->f_code[1][1] /*f_code_b1*/));
    GEN_VDMA_ACFG(chn, REG_SDE_CFG3, 0, s->qs_code);
    //ALOGE("REG_SDE_CFG3 %x", s->qs_code);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG8, 0, TCSM1_MOTION_DHA);
    //ALOGE("REG_SDE_CFG8 %x", TCSM1_MOTION_DHA);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG11, 0, TCSM1_MSRC_BUF);
    //ALOGE("REG_SDE_CFG11 %x", TCSM1_MSRC_BUF);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG9, 0, VMAU_CHN_BASE);
    //ALOGE("REG_SDE_CFG9 %x", VMAU_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG10, 0, DBLK_CHN_BASE);
    //ALOGE("REG_SDE_CFG10 %x", DBLK_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, 0, SDE_SLICE_INIT);
    
    GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, (SDE_MODE_AUTO | SDE_EN) );
    //GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, (SDE_MODE_DEBUG | SDE_EN) );
    GEN_VDMA_ACFG(chn, REG_SDE_SL_GEOM, 0, SDE_SL_GEOM(s->mb_height, s->mb_width, 
						       s->mb_pos_y, s->mb_pos_x) );
    GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, VDMA_ACFG_TERM, SDE_MB_RUN);
  }
  /*-------------------------SDE init end-------------------------------*/
  jz_dcache_wb();
  return chn;
}

int M2D_SliceInit_ext(_M2D_SliceInfo *s)
{
  int i, j;
  _M2D_BitStream *bs= &s->bs_buf;
  int va = 0;

  volatile unsigned int *chn = (volatile unsigned int *)s->des_va;
  /*-------------------------sch init begin-------------------------------*/
  { /*configure sch*/
#if 0
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH3_HID(HID_DBLK) |
					SCH_CH2_HID(HID_VMAU) |
					SCH_CH1_HID(HID_MCE) |
					SCH_DEPTH(MPEG2_FIFO_DEP) 
					)
		  );
					
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG0, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG1, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE1, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE2, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE3, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE4, 0, 0);
#else
    chn += 16;
#endif
    va = chn;
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, (((s->coding_type != I_TYPE)<<2) | 
					 SCH_CH2_PE| 
					 SCH_CH3_PE					 
					 )
		  );
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH3_HID(HID_DBLK) |
					SCH_CH2_HID(HID_VMAU) |
					SCH_CH1_HID(HID_MCE) |
					SCH_DEPTH(MPEG2_FIFO_DEP) |
					((s->coding_type != I_TYPE)<<0) | 
					SCH_BND_G0F2 | 
					SCH_BND_G0F3 					
					) 		  
		  );
  }
  i_cache(0x19, va, 0);
  /*-------------------------sch init end-------------------------------*/  

  /*-------------------------mc init begin-------------------------------*/
  { /*configure motion*/
    int intpid= MPEG_HPEL; 
    int cintpid= MPEG_HPEL;
#if 0
    for(i=0; i<16; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_ILUT+i*8, 0, 
		  MCE_CH1_IINFO(IntpFMT[intpid][i].intp[0],/*intp1*/
				IntpFMT[intpid][i].tap,/*tap*/
				IntpFMT[intpid][i].intp_pkg[0],/*intp1_pkg*/
				IntpFMT[intpid][i].hldgl,/*hldgl*/
				IntpFMT[intpid][i].avsdgl,/*avsdgl*/
				IntpFMT[intpid][i].intp_dir[0],/*intp0_dir*/
				IntpFMT[intpid][i].intp_rnd[0],/*intp0_rnd*/
				IntpFMT[intpid][i].intp_sft[0],/*intp0_sft*/
				IntpFMT[intpid][i].intp_sintp[0],/*sintp0*/
				IntpFMT[intpid][i].intp_srnd[0],/*sintp0_rnd*/
				IntpFMT[intpid][i].intp_sbias[0]/*sintp0_bias*/
				));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_ILUT+i*8+4, 0, 
		  MCE_CH1_IINFO(IntpFMT[intpid][i].intp[1],/*intp1*/
				0,/*tap*/
				IntpFMT[intpid][i].intp_pkg[1],/*intp1_pkg*/
				IntpFMT[intpid][i].hldgl,/*hldgl*/
				IntpFMT[intpid][i].avsdgl,/*avsdgl*/
				IntpFMT[intpid][i].intp_dir[1],/*intp1_dir*/
				IntpFMT[intpid][i].intp_rnd[1],/*intp1_rnd*/
				IntpFMT[intpid][i].intp_sft[1],/*intp1_sft*/
				IntpFMT[intpid][i].intp_sintp[1],/*sintp1*/
				IntpFMT[intpid][i].intp_srnd[1],/*sintp1_rnd*/
				IntpFMT[intpid][i].intp_sbias[1]/*sintp1_bias*/
				));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+i*8+4, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[0][7],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[0][6],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[0][5],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[0][4]/*coef5*/
			      ));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+i*8, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[0][3],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[0][2],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[0][1],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[0][0]/*coef5*/
			      ));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+(i+16)*8+4, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[1][7],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[1][6],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[1][5],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[1][4]/*coef5*/
			      ));
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+(i+16)*8, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[1][3],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[1][2],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[1][1],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[1][0]/*coef5*/
			      ));
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_ILUT+i*8, 0, 
		  MCE_CH2_IINFO(IntpFMT[cintpid][i].intp[0],
				IntpFMT[cintpid][i].intp_dir[0],
				IntpFMT[cintpid][i].intp_sft[0],
				IntpFMT[cintpid][i].intp_coef[0][0],
				IntpFMT[cintpid][i].intp_coef[0][1],
				IntpFMT[cintpid][i].intp_rnd[0]) );
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_ILUT+i*8+4, 0, 
		  MCE_CH2_IINFO(IntpFMT[cintpid][i].intp[1],
				IntpFMT[cintpid][i].intp_dir[1],
				IntpFMT[cintpid][i].intp_sft[1],
				IntpFMT[cintpid][i].intp_coef[1][0],
				IntpFMT[cintpid][i].intp_coef[1][1],
				IntpFMT[cintpid][i].intp_rnd[1]) );
    }
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_BINFO, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_BINFO, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CTRL, 0, ( MCE_COMP_AUTO_EXPD |
					  MCE_CH2_EN |
					  MCE_CLKG_EN |
					  MCE_OFA_EN |
					  MCE_CACHE_FLUSH |
					  MCE_EN 
					  ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_PINFO, 0, MCE_PINFO(0, 0, 6, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_PINFO, 0, MCE_PINFO(0, 0, 6, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_WINFO, 0, MCE_WINFO(0, 0, 0, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_WTRND, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO1, 0, MCE_WINFO(0, 0, 0, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO2, 0, 0);
#else
    chn += 0x116;
#endif
    va = chn;
    // configure ref frame.
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+ i*8+ 4, 0, s->pic_ref[0][i]); 
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+ i*8+ 128 + 4, 0, s->pic_ref[0][i]);
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+ i*8+ 4, 0, s->pic_ref[1][i]); 
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+ i*8+ 128 + 4, 0, s->pic_ref[1][i]); 
    }
    for (i = 0; i < 8; i++){
      i_cache(0x19, va, 0);
      va+=32;
    }
#if 0
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WTRND, 0, 0);
    //GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(s->mb_width*16, 0, 16) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(s->y_stride / 16, 0, 16) );
    GEN_VDMA_ACFG(chn, REG_MCE_GEOM, 0, MCE_GEOM(s->mb_height*16, s->mb_width*16) );
    //GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(s->mb_width*16, 0, DOUT_C_STRD) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(s->c_stride / 8, 0, DOUT_C_STRD) );
    GEN_VDMA_ACFG(chn, REG_MCE_DSA, 0, SCH1_DSA);
    GEN_VDMA_ACFG(chn, REG_MCE_DDC, 0, TCSM1_MOTION_DHA);
  }
  /*-------------------------mc init end-------------------------------*/
  
  /*-------------------------vmau init begin-------------------------------*/
  { /*configure vmau*/
    for ( i = 0 ; i < 4; i++){ /*write qt table*/
      for ( j = 0 ; j < 16; j++){
	unsigned int idx= REG_VMAU_QT+ i*64 + (j<<2);
	GEN_VDMA_ACFG(chn, idx, 0, s->coef_qt[i][j]);
      }
    }
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_RUN, 0, VMAU_RESET);
    GEN_VDMA_ACFG(chn, REG_VMAU_VIDEO_TYPE, 0, VMAU_FMT_MPEG2);
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_STR, 0, (16<<16)|16);  
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_CTR, 0, (VMAU_CTRL_TO_DBLK | VMAU_CTRL_FIFO_M) );
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_DONE, 0, SCH2_DSA);
    GEN_VDMA_ACFG(chn, REG_VMAU_Y_GS, 0, s->mb_width*16);
    GEN_VDMA_ACFG(chn, REG_VMAU_NCCHN_ADDR, 0, VMAU_CHN_BASE);      
  }
  /*-------------------------vmau init end-------------------------------*/
  
  /*-------------------------dblk init begin-------------------------------*/
  { /*configure dblk*/
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_RESET);
    //GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, DBLK_GPIC_STR(s->mb_width*128, s->mb_width*256) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, DBLK_GPIC_STR(s->c_stride, s->y_stride) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GSIZE, 0, DBLK_GSIZE(s->mb_height, s->mb_width) );
    GEN_VDMA_ACFG(chn, REG_DBLK_VTR, 0, DBLK_VTR(0, 0, 0, 0,
					       0, DBLK_FMT_MPEG2) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GP_ENDA, 0, DBLK_END_FLAG);
    GEN_VDMA_ACFG(chn, REG_DBLK_GENDA, 0, SCH3_DSA);
#else
    chn += 0xa6;
#endif
    va = chn;
    GEN_VDMA_ACFG(chn, REG_DBLK_GPOS, 0, DBLK_GPOS(s->mb_pos_y, s->mb_pos_x) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_YA, 0, s->pic_cur[0]);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_CA, 0, s->pic_cur[1]);
    GEN_VDMA_ACFG(chn, REG_DBLK_DHA, 0, DBLK_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_SLICE_RUN);
  }
  /*-------------------------dblk init end-------------------------------*/

  /*-------------------------SDE init begin-------------------------------*/
  { /*configure sde*/
    GEN_VDMA_ACFG(chn, REG_SDE_CODEC_ID, 0, SDE_FMT_MPEG2_DEC);
    { /*fill vlc_ram*/
      // mb
      if (s->coding_type == I_TYPE) {
	for (i= 0; i< MB_I_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((MB_I_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_I_HW[i]);
	} 
      }
      if (s->coding_type == P_TYPE) {
	for (i= 0; i< MB_P_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((MB_P_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_P_HW[i]);
	} 
      }
      if (s->coding_type == B_TYPE) {
	for (i= 0; i< MB_B_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((MB_B_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_B_HW[i]);
	} 
      }
      for (i = 0; i < ((int)chn - va + 32) / 32; i++){
	i_cache(0x19, va, 0);
	va += 32;	
      }
#if 0
      // mba, dct
      for (i= 0; i< MBA_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((MBA_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MBA_5_HW[i]);
      } 
      for (i= 0; i< MBA_11_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((MBA_11_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MBA_11_HW[i]);
      } 
      for (i= 0; i< DC_lum_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DC_lum_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_lum_5_HW[i]);
      } 
      for (i= 0; i< DC_long_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DC_long_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_long_HW[i]);
      } 
      for (i= 0; i< DC_chrom_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DC_chrom_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_chrom_5_HW[i]);
      } 
      for (i= 0; i< DCT_B15_8_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B15_8_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B15_8_HW[i]);
      } 
      for (i= 0; i< DCT_B15_10_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B15_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B15_10_HW[i]);
      } 
      for (i= 0; i< DCT_13_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_13_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_13_HW[i]);
      } 
      for (i= 0; i< DCT_15_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_15_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_15_HW[i]);
      } 
      for (i= 0; i< DCT_16_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_16_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_16_HW[i]);
      } 
      for (i= 0; i< DCT_B14AC_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B14AC_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14AC_5_HW[i]);
      } 
      for (i= 0; i< DCT_B14DC_5_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B14DC_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14DC_5_HW[i]);
      } 
      for (i= 0; i< DCT_B14_8_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B14_8_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14_8_HW[i]);
      } 
      for (i= 0; i< DCT_B14_10_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DCT_B14_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14_10_HW[i]);
      }
#else
      chn += 0x300;
#endif
      va = chn;
      // cbp
      if (s->coding_type != I_TYPE) {
	for (i= 0; i< CBP_7_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((CBP_7_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, CBP_7_HW[i]);
	} 
	for (i= 0; i< CBP_9_LEN; i++){
	  unsigned int idx= REG_SDE_CTX_TBL+ ((CBP_9_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, CBP_9_HW[i]);
	}       
      }
      for (i = 0; i < ((int)chn - va + 32) / 32; i++){
	i_cache(0x19, va, 0);
	va += 32;	
      }
#if 0
      // mv
      for (i= 0; i< MV_4_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((MV_4_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MV_4_HW[i]);
      }       
      for (i= 0; i< MV_10_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((MV_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MV_10_HW[i]);
      }       
      for (i= 0; i< DMV_2_LEN; i++){
	unsigned int idx= REG_SDE_CTX_TBL+ ((DMV_2_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DMV_2_HW[i]);
      }
#endif       
    }
#if 0    
    for (i= 0; i< 16; i++){ /*fill scan_ram*/
      unsigned int idx= REG_SDE_CTX_TBL+ 0x1000+ (i<<2);
      GEN_VDMA_ACFG(chn, idx, 0, s->scan[i]);
    }
#else
    chn += 0x46;
#endif
    va = chn;
    GEN_VDMA_ACFG(chn, REG_SDE_CFG2, 0, bs->buffer);
    int *tbsb = (int *)bs->buffer;
    GEN_VDMA_ACFG(chn, REG_SDE_CFG1, 0, bs->bit_ofst);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG0, 0, GET_SL_INFO(s->coding_type, /*coding_type*/ 
						    s->fr_pred_fr_dct, /*fr_pred_fr_dct*/
						    s->pic_type, /*pic_type*/
						    s->conceal_mv, /*conceal_mv*/
						    s->intra_dc_pre, /*intra_dc_pre*/
						    s->intra_vlc_format, /*intra_vlc_format*/
						    s->mpeg1, /*mpeg1*/
						    s->top_fi_first, /*top_fi_first*/ 
						    s->q_scale_type, /*q_scale_type*/
						    s->sec_fld,
						    s->f_code[0][0], /*f_code_f0*/
						    s->f_code[0][1], /*f_code_f1*/
						    s->f_code[1][0], /*f_code_b0*/
						    s->f_code[1][1] /*f_code_b1*/));
    GEN_VDMA_ACFG(chn, REG_SDE_CFG3, 0, s->qs_code);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG8, 0, TCSM1_MOTION_DHA);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG11, 0, TCSM1_MSRC_BUF);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG9, 0, VMAU_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG10, 0, DBLK_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, 0, SDE_SLICE_INIT);
    
    GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, (SDE_MODE_AUTO | SDE_EN) );
    GEN_VDMA_ACFG(chn, REG_SDE_SL_GEOM, 0, SDE_SL_GEOM(s->mb_height, s->mb_width, 
						       s->mb_pos_y, s->mb_pos_x) );
    GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, VDMA_ACFG_TERM, SDE_MB_RUN);
    for (i = 0; i < 4; i++){
      i_cache(0x19, va, 0);
      va += 32;
    }
  }
  /*-------------------------SDE init end-------------------------------*/
  return chn;
}
