#include <linux/types.h>

#include <stdio.h>
#include "tcsm1.h"
#include "vmau.h"
#include "sram.h"
#include "bits_mask_def.h"
  
#include "instructions.h"

#define offsetof(type, member)  __builtin_offsetof (type, member)
//#define offsetof(s, m)   (size_t)&(((s *)0)->m)

unsigned char raw_y[256] = {
0x2c,0x2e,0x31,0x23,0x20,0x24,0x25,0x24,0x22,0x22,0x24,0x23,0x23,0x1f,0x1d,0x21,
0x2c,0x2b,0x2b,0x29,0x21,0x22,0x25,0x26,0x25,0x23,0x24,0x25,0x24,0x21,0x1d,0x20,
0x2d,0x2c,0x2b,0x2b,0x25,0x20,0x24,0x25,0x25,0x23,0x23,0x2b,0x25,0x20,0x24,0x2f,
0x27,0x2b,0x2b,0x29,0x24,0x20,0x21,0x24,0x24,0x22,0x24,0x2b,0x23,0x20,0x27,0x25,
0x25,0x27,0x28,0x2d,0x28,0x1f,0x20,0x22,0x22,0x23,0x26,0x28,0x23,0x22,0x29,0x1e,
0x25,0x23,0x25,0x2c,0x27,0x1f,0x22,0x23,0x22,0x23,0x26,0x28,0x23,0x22,0x28,0x1f,
0x25,0x26,0x25,0x28,0x2b,0x25,0x23,0x25,0x25,0x21,0x25,0x27,0x22,0x20,0x22,0x23,
0x2c,0x2a,0x24,0x1e,0x25,0x27,0x22,0x23,0x22,0x22,0x28,0x26,0x1f,0x1f,0x23,0x25,
0x2e,0x28,0x29,0x25,0x23,0x26,0x20,0x1f,0x23,0x25,0x27,0x23,0x1d,0x1f,0x25,0x23,
0x28,0x2a,0x2e,0x38,0x36,0x25,0x20,0x23,0x26,0x27,0x28,0x29,0x25,0x24,0x24,0x21,
0x27,0x28,0x27,0x30,0x35,0x25,0x26,0x27,0x22,0x22,0x26,0x2a,0x2a,0x28,0x24,0x25,
0x23,0x27,0x23,0x27,0x2c,0x24,0x24,0x26,0x26,0x23,0x28,0x28,0x22,0x21,0x24,0x28,
0x1b,0x23,0x2c,0x29,0x28,0x1f,0x1d,0x25,0x2b,0x29,0x27,0x21,0x1d,0x21,0x25,0x26,
0x1d,0x21,0x32,0x2d,0x28,0x20,0x1f,0x27,0x2e,0x28,0x1b,0x1a,0x25,0x22,0x23,0x25,
0x2c,0x20,0x1e,0x28,0x29,0x21,0x20,0x24,0x2e,0x29,0x2c,0x2c,0x2d,0x27,0x2d,0x42,
0x29,0x21,0x1a,0x20,0x27,0x25,0x21,0x1e,0x21,0x1d,0x33,0x45,0x39,0x45,0x3d,0x3e,
};

unsigned char raw_u[64] = {
0x7b, 0x7d, 0x81, 0x80, 0x81, 0x81, 0x81, 0x80,
0x7c, 0x7c, 0x7f, 0x81, 0x81, 0x80, 0x80, 0x7f,
0x7d, 0x7d, 0x7f, 0x81, 0x81, 0x80, 0x80, 0x80,
0x7c, 0x7d, 0x7f, 0x80, 0x80, 0x80, 0x81, 0x80,
0x7d, 0x7d, 0x7e, 0x80, 0x80, 0x7f, 0x80, 0x80,
0x7f, 0x7d, 0x7e, 0x7f, 0x7e, 0x7e, 0x7f, 0x7f,
0x80, 0x7e, 0x7f, 0x7f, 0x7c, 0x7e, 0x7e, 0x7d,
0x7e, 0x80, 0x7f, 0x80, 0x7e, 0x7b, 0x78, 0x78,
};

unsigned char raw_v[64] = {
0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80,
0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80,
0x7f, 0x7e, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80,
0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80,
0x7f, 0x80, 0x81, 0x7f, 0x7f, 0x7f, 0x7f, 0x80,
0x7f, 0x80, 0x81, 0x80, 0x80, 0x80, 0x7f, 0x7f,
0x7f, 0x7f, 0x80, 0x80, 0x81, 0x7f, 0x7e, 0x7e,
0x80, 0x7f, 0x80, 0x80, 0x80, 0x7e, 0x7e, 0x7e,
};


void vmau_reg_init(volatile vmau_reg * vmau){
  //memset(vmau, 0, sizeof(vmau_reg));
  vmau->glb_trigger    = BIT2;        //RESET VMAU FIXME: can NOT do reset              
  vmau->video_type     = BIT11 | BIT0;//encode enable | H264               
  vmau->glb_enable     = 1;    //1: fifo mode, 0: single mode
  vmau->dec_stride     = 0x100010;               

  //vmau->main_cbp       = 0;                 
  //vmau->quant_param    = 0;

  //vmau->src_phy_addr   = 0;         //contain in task_chain            

  //vmau->acbp           = do_get_phy_addr(&vmau_var->enc_dst.cbp);

  //vmau->next_task_chain_phy_addr = do_get_phy_addr(&vmau_var->task_chain); 

  //vmau->src_len        = 0;                  
  
  //vmau->acbp           = do_get_phy_addr(&vmau_var->enc_dst.cbp); //contain in task_chain

  //vmau->top_left_and_c_pred      = 0;      
  //vmau->y_pred0        = 0;    //contain in task_chain              
  //vmau->y_pred1        = 0;                  
  //vmau->enc_addr_out   = 0;    //contain in acbp              


  //vmau->status         = 0;                   

  //vmau->task_chain_phy_addr      = 0;      

  //vmau->video_type     = BIT11 | BIT0;//encode enable | H264               

  //vmau->frame_size.all     = 0;               

  //vmau->dec_end_flag_phy_addr    = do_get_phy_addr(&vmau_var->enc_end_flag);    
  //vmau->enc_end_flag_phy_addr    = do_get_phy_addr(&vmau_var->dec_end_flag);    

  //vmau->mb_pos         = 0 ;                   
  //vmau->mcf_status     = 0;               
  //vmau->dec_y_phy_addr = do_get_phy_addr(vmau_var->dec_dst_y);           
  //vmau->dec_u_phy_addr = do_get_phy_addr(vmau_var->dec_dst_uv);           
  //vmau->dec_v_phy_addr = do_get_phy_addr(vmau_var->dec_dst_uv + 8);           

}

void vmau_do_a_fake_mb(TCSM1 *tcsm1_ptr, vmau_reg * volatile vmau){
    vmau->glb_trigger    = BIT2;        //RESET VMAU
    vmau->video_type     = BIT11 | BIT0;//encode enable | H264               
    vmau->glb_enable     = 1;           //1: fifo mode, 0: single mode
    vmau->dec_stride     = 0x100010;               
    vmau->frame_size.all = (16 << 16) | 16;

    volatile vmau_reg *vmau_reg_ptr = vmau;

    const int vmau_aux_fifo_index = 0;
    const int pipe_index = 0;

    volatile VMAU_VAR * const vmau_var = &tcsm1_ptr->vmau_var[pipe_index];

    TCSM1 *tcsm1_phy = (TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr;
    VMAU_VAR * const vmau_var_phy = &tcsm1_phy->vmau_var[pipe_index];

    //-----------vmau_task_chain init each macroblock once--------------
    VMAU_TASK_CHAIN * const vmau_task_chain_ptr = &vmau_var->task_chain;
    VMAU_TASK_CHAIN * const vmau_task_chain_phy_ptr = &vmau_var_phy->task_chain;

    const TCSM0 * const tcsm0_phy = tcsm1_ptr->slice_var.tcsm0_phy;
    const VMAU_CBP_RES * const vmau_cbp_res_ptr = &tcsm0_phy->vmau_cbp_res_array[vmau_aux_fifo_index];
    const RECON_MB_PXL * const recon_mb_pxl_phy_ptr = &tcsm0_phy->recon_mb_pxl_array[pipe_index];

    vmau_task_chain_ptr->aux_cbp = (unsigned int *)vmau_cbp_res_ptr;
    vmau_task_chain_ptr->quant_para = tcsm1_ptr->slice_var.vmau_quant_para;
    vmau_task_chain_ptr->main_addr = (unsigned int *)tcsm1_ptr->slice_var.sram_phy->raw_data[tcsm1_ptr->vmau_raw_data_index].y;

    vmau_task_chain_ptr->ncchn_addr = (unsigned int *)vmau_task_chain_phy_ptr;    

    {
        X264_MB_VAR *mb = &tcsm1_ptr->mb_array[vmau_aux_fifo_index];

        //get pred_mode
        mb->i_intra16x16_pred_mode = 6;//I_PRED_16x16_DC_128 = 6
        mb->i_chroma_pred_mode = 6;    //I_PRED_CHROMA_DC_128  = 6

        const int i_mode = mb->i_intra16x16_pred_mode;

	vmau_task_chain_ptr->main_cbp = (MAU_Y_PREDE_MSK << MAU_Y_PREDE_SFT );//enable luma pred
	vmau_task_chain_ptr->main_cbp |= (M16x16  << MAU_MTX_SFT );            //enable luma 16x16
	vmau_task_chain_ptr->y_pred_mode[0] = i_mode;	
	vmau_task_chain_ptr->c_pred_mode_tlr.all = mb->i_chroma_pred_mode;
    }

    tcsm1_ptr->vmau_enc_end_flag = -1;
    tcsm1_ptr->vmau_dec_end_flag = -1;

    //-----------vmau reg init each macroblock once--------------------
    vmau_reg_ptr->dec_end_flag_phy_addr = &tcsm1_phy->vmau_dec_end_flag;    
    vmau_reg_ptr->enc_end_flag_phy_addr = &tcsm1_phy->vmau_enc_end_flag;    
    vmau_reg_ptr->dec_y_phy_addr = &recon_mb_pxl_phy_ptr->y[0];           
    vmau_reg_ptr->dec_u_phy_addr = &recon_mb_pxl_phy_ptr->uv[0];           
    vmau_reg_ptr->dec_v_phy_addr = &recon_mb_pxl_phy_ptr->uv[8];           
    vmau_reg_ptr->next_task_chain_phy_addr = (uint32_t *)vmau_task_chain_phy_ptr; 
    vmau_reg_ptr->glb_trigger = VMAU_TRI_MCCFW_WR;
    vmau_reg_ptr->glb_trigger = VMAU_RUN;

    {
      const int wait_vmau_loop_max = 10000000;
      volatile int wait_vmau_loop = 0;    

      wait_vmau_loop = 0;
      do{
	wait_vmau_loop++;
	if(wait_vmau_loop > wait_vmau_loop_max){
	  printf("vmau maybe dead during do_a_fake_mb!\n");
	  break;
	}
      }while( -1 == tcsm1_ptr->vmau_dec_end_flag);
    }

}

void vmau_task_chain_init(const VMAU_VAR* vmau_var, const unsigned char * vmau_src, const TCSM1 *tcsm1, volatile VMAU_TASK_CHAIN *vmau_task_chain){
  //memset(vmau_task_chain, 0, sizeof(VMAU_TASK_CHAIN));

  vmau_task_chain->main_addr  = (unsigned int *)do_get_phy_addr(vmau_src);
  vmau_task_chain->ncchn_addr = (unsigned int *)do_get_phy_addr(vmau_task_chain);

  //vmau_task_chain->aux_cbp    = do_get_phy_addr( &vmau_var->enc_dst.cbp );

  //unsigned int tmp_cbp = 0;
  //tmp_cbp |= (MAU_Y_PREDE_MSK << MAU_Y_PREDE_SFT ); //BIT27; enable luma pred
  //tmp_cbp |= (M16x16  << MAU_MTX_SFT );             //(BIT24 | BIT25); enable luma16X16
  //tmp_cbp |= (MAU_C_PREDE_MSK << MAU_C_PREDE_SFT ); //enable chroma pred

  //vmau_task_chain->main_cbp = tmp_cbp;

  //vmau_task_chain->quant_para = 0x14e9d3;

  vmau_task_chain->src_len_id_inc.all = 0;

  //vmau_task_chain->aux_cbp = do_get_phy_addr(&vmau_var->enc_dst.cbp);
  //vmau_task_chain->c_pred_mode = 6;
  //vmau_task_chain->top_lr_avalid = 0;
  //vmau_task_chain->y_pred_mode[0] = 6;
  //vmau_task_chain->y_pred_mode[1] = 0;
}

extern volatile unsigned char *vmau_base;
extern volatile unsigned char *tcsm0_base;
extern volatile unsigned char *tcsm1_base;

//#define VMAU_V_BASE vmau_base
#define C_TCSM1_BASE tcsm1_base

#if 0
void test_x264(const unsigned char *vmau_src,  const TCSM1 * tcsm1, volatile vmau_reg * vmau)
{

#define VMAU_V_BASE (int)vmau

  VMAU_VAR *vmau_var = &tcsm1->vmau;

  vmau_reg_init(&tcsm1->vmau_var, vmau);  

  vmau_task_chain_init(vmau_var, vmau_src, tcsm1, &vmau_var->task_chain);
  
  *(volatile int *)(&v_tcsm1->vmau_enc_end_flag) = -1;
  *(volatile int *)(&v_tcsm1->vmau_dec_end_flag) = -1;

  vmau->glb_trigger = BIT0; //run vmau

  while(-1 == vmau_var->enc_end_flag);

  volatile unsigned int *enc_res_ptr = &vmau_var->enc_dst.cbp;
  printf("enc cbp: %x\n",vmau_var->enc_dst.cbp);
  printf("res[0]: %x, %d\n", ((volatile int *)&(vmau_var->enc_dst.res[0])), vmau_var->enc_dst.res[0] );

  int i = 0, j = 0;
  for(i = 0; i < 16; i++){
    for(j = 0; j < 16; j++){
       printf(" %d ", vmau_var->enc_dst.res[i*16 + j] );
    }
    printf("\n");
  }

  while(-1 == v_tcsm1->vmau_dec_end_flag);

  for ( i = 0 ; i < 16; i++){
     for ( j = 0 ; j < 16; j++){
	printf("%2x ", vmau_var->dec_dst_y[i*16+j]);
     }
     printf("\n");
  }

}

#else //for bak

extern volatile unsigned char *sram_base;
#define C_SRAM_BASE sram_base

void test_x264(const unsigned char *vmau_src, volatile const TCSM1 * tcsm1, volatile vmau_reg * vmau)
{
  int tmp;

  VMAU_VAR *vmau_var = &tcsm1->vmau_var[0];

  //volatile unsigned char *chn_base = C_TCSM1_BASE;
  volatile unsigned char *chn_base = &vmau_var->task_chain;  

  //volatile unsigned char *msrc_base = C_SRAM_BASE + 0x1000;
  volatile unsigned char *msrc_base = C_SRAM_BASE;  
  //volatile unsigned char *dst_ybase = C_TCSM1_BASE + 0x2000;
  //volatile unsigned char *dst_ybase = C_TCSM1_BASE + 0x9*4;
  volatile unsigned char *dst_ybase = vmau_var->dec_dst_y;
  //volatile unsigned char *dst_ubase = dst_ybase + 256;
  volatile unsigned char *dst_ubase = vmau_var->dec_dst_uv; 
  volatile unsigned char *dst_vbase = vmau_var->dec_dst_uv + 8;
  //volatile unsigned char *enc_result = C_TCSM1_BASE + 0x4000;
  //volatile unsigned char *enc_result = C_TCSM1_BASE + 0x9*4 + 256 + 64 + 64;
  //volatile unsigned char *enc_result = &vmau_var->enc_dst.cbp;
  volatile unsigned char *enc_flag = &tcsm1->vmau_enc_end_flag;
  volatile unsigned char *dec_flag = &tcsm1->vmau_dec_end_flag;

  printf("chn_base: %x\n", do_get_phy_addr(chn_base));
  printf("enc_flag: %x\n", do_get_phy_addr(enc_flag));
  printf("dec_flag: %x\n", do_get_phy_addr(dec_flag));
  printf("msrc_base: %x\n", do_get_phy_addr(msrc_base));
  printf("dst_ybase: %x\n", do_get_phy_addr(dst_ybase));
  printf("dst_ubase: %x\n", do_get_phy_addr(dst_ubase));
  printf("dst_vbase: %x\n", do_get_phy_addr(dst_vbase));
  //printf("enc_result: %x\n", do_get_phy_addr(enc_result));
  
  tmp = ((H264 & MAU_VIDEO_MSK) << MAU_VIDEO_SFT) |
    ((MAU_ENC_MSK ) << MAU_ENC_SFT );

#define VMAU_V_BASE (int)vmau
  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RESET);
  write_reg(VMAU_V_BASE+VMAU_SLV_VIDEO_TYPE, tmp);
  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, do_get_phy_addr(dec_flag));

  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,do_get_phy_addr(dst_ybase));
  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,do_get_phy_addr(dst_ubase));
  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,do_get_phy_addr(dst_vbase));  
  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_STR,(8<<16)|16);
  write_reg(VMAU_V_BASE+VMAU_SLV_ENC_DONE, do_get_phy_addr(enc_flag));
  write_reg(VMAU_V_BASE+VMAU_SLV_NCCHN_ADDR, do_get_phy_addr(chn_base));
  //write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, VMAU_CTRL_FIFO_M | VMAU_CTRL_IRQ_EN);
  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, VMAU_CTRL_FIFO_M );
  
  
  VMAU_TASK_CHAIN *vmau_tsk_chn_ptr = (VMAU_TASK_CHAIN *)chn_base;
  vmau_tsk_chn_ptr->main_cbp = 0x4b000000;
  vmau_tsk_chn_ptr->quant_para = 0x14e9d3;
  vmau_tsk_chn_ptr->main_addr = do_get_phy_addr(msrc_base);
  vmau_tsk_chn_ptr->ncchn_addr = do_get_phy_addr(chn_base);

  vmau_tsk_chn_ptr->src_len_id_inc.all = 0;

  //vmau_tsk_chn_ptr->aux_cbp = do_get_phy_addr(enc_result);

  vmau_tsk_chn_ptr->c_pred_mode_tlr.all = 6;

  vmau_tsk_chn_ptr->y_pred_mode[0] = 6;
  vmau_tsk_chn_ptr->y_pred_mode[1] = 0;
  
  int i;
  unsigned int base_addr = VMAU_V_BASE + VMAU_QT_BASE;
  for ( i = 0 ; i < 16; i++)
    {
      write_reg(base_addr, 0x10101010);
      base_addr += 4;
    }
  for ( i= 0 ; i<32; i++)
    {
      write_reg(base_addr, 0x100010);
      base_addr += 4;
    }

  base_addr = msrc_base;
  unsigned int *tmp_ptr = raw_y;
  for ( i = 0 ; i< (256/4); i++)
    {
      write_reg(base_addr , *tmp_ptr);
      base_addr += 4;
      tmp_ptr++;
    }

  base_addr = (unsigned int )(msrc_base + 256);
  tmp_ptr = (unsigned int *)raw_u;
  for ( i = 0 ; i< (64/4); i++)
  {
      write_reg(base_addr , *tmp_ptr);
      base_addr += 4;
      tmp_ptr++;
  }


  base_addr = (unsigned int )(msrc_base + 256 + 8);
  tmp_ptr = (unsigned int *)raw_v;
  for ( i = 0 ; i< (64/4); i++)
  {
      write_reg(base_addr , *tmp_ptr);
      base_addr += 4;
      tmp_ptr++;
  }


  volatile unsigned int *endf_ptr = dec_flag;
  volatile unsigned int *enc_endf_ptr = enc_flag;
  *endf_ptr = -1;
  *enc_endf_ptr = -1;
  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RUN);
  do{
  }while(*endf_ptr == -1 );
  printf("end flag: %x\n",*endf_ptr);
  //volatile unsigned int *enc_res_ptr = enc_result;
  //printf("enc cbp: %x\n",*enc_res_ptr);
  //printf("enc cbp: %x\n",enc_res_ptr[0]);
  //printf("enc cbp: %x\n",enc_res_ptr[1]);
  //printf("enc cbp: %x\n",enc_res_ptr[2]);
  //printf("enc cbp: %x\n",enc_res_ptr[3]);

  int j = 0;
  for(i = 0; i < 16; i++){
    for(j = 0; j < 16; j++){
      //printf(" %4d ", ((volatile short *)(enc_res_ptr + 1))[i*16 + j] );
    }
    //printf("\n");
  }


  do{
  }while(*enc_endf_ptr == -1 );

  dump_vmau_dec_rst(dst_ybase);

}

#endif


void dump_vmau_reg(const vmau_reg * vmau){
  printf("\nvmau_reg_paddr: 0x%08x\n", do_get_phy_addr(vmau));
  printf("main_cbp:     offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, main_cbp), vmau->main_cbp);
  printf("quant_param:  offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, quant_param), vmau->quant_param);
  printf("src_phy_addr: offset = 0x%x, val = 0x%x\n",
	 offsetof(vmau_reg, src_phy_addr), vmau->src_phy_addr);
  printf("next_task_chain_phy_addr: offset = 0x%x, val = 0x%x\n", 
	 offsetof(vmau_reg, next_task_chain_phy_addr), vmau->next_task_chain_phy_addr);
  printf("src_len:      offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, src_len), vmau->src_len);
  printf("acbp:         offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, acbp), vmau->acbp);
  printf("top_left_and_c_pred: offset = 0x%x, val = 0x%x\n", 
	 offsetof(vmau_reg, top_left_and_c_pred), vmau->top_left_and_c_pred);
  printf("y_pred0:      offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, y_pred0), vmau->y_pred0);
  printf("y_pred1:      offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, y_pred1), vmau->y_pred1);
  printf("enc_addr_out: offset = 0x%x, val = 0x%x\n", 
	 offsetof(vmau_reg, enc_addr_out), vmau->enc_addr_out);
  printf("glb_trigger:  offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, glb_trigger), vmau->glb_trigger);
  printf("glb_enable:   offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, glb_enable), vmau->glb_enable);
  printf("status:       offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, status), vmau->status);
  printf("task_chain_phy_addr: offset = 0x%x, val = 0x%x\n", 
	 offsetof(vmau_reg, task_chain_phy_addr), vmau->task_chain_phy_addr);
  printf("video_type:   offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, video_type), vmau->video_type);
  printf("frame_size:   offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, frame_size), vmau->frame_size);
  printf("dec_end_flag_phy_addr: offset = 0x%x, val = 0x%x\n",
	 offsetof(vmau_reg, dec_end_flag_phy_addr), vmau->dec_end_flag_phy_addr);
  printf("enc_end_flag_phy_addr: offset = 0x%x, val = 0x%x\n", 
	 offsetof(vmau_reg, enc_end_flag_phy_addr), vmau->enc_end_flag_phy_addr);
  printf("mb_pos:         offset = 0x%x, val = 0x%x\n",
	 offsetof(vmau_reg, mb_pos), vmau->mb_pos);
  printf("dec_y_phy_addr: offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, dec_y_phy_addr), vmau->dec_y_phy_addr);
  printf("dec_u_phy_addr: offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, dec_u_phy_addr), vmau->dec_u_phy_addr);
  printf("dec_v_phy_addr: offset = 0x%x, val = 0x%x\n", 
	 offsetof(vmau_reg, dec_v_phy_addr), vmau->dec_v_phy_addr);
  printf("dec_stride:     offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, dec_stride), vmau->dec_stride);
  printf("mc_fifo:        offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, mc_fifo), vmau->mc_fifo);
  printf("pred_line:      offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, pred_line), vmau->pred_line);
  printf("quant_table:    offset = 0x%x, val = 0x%x\n", offsetof(vmau_reg, quant_table), vmau->quant_table);
  printf("\n");
}

void dump_vmau_task_chain(const VMAU_TASK_CHAIN * vmau_task_chain_ptr){
  printf("\nvmau_task_chain paddr: 0x%08x\n", do_get_phy_addr(vmau_task_chain_ptr));
  printf("main_cbp :   offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN, main_cbp), vmau_task_chain_ptr->main_cbp);

  printf("quant_para : offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN, quant_para), vmau_task_chain_ptr->quant_para);

  printf("src_paddr :  offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN, main_addr), vmau_task_chain_ptr->main_addr);


  printf("ncchn_addr : offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN, ncchn_addr), vmau_task_chain_ptr->ncchn_addr);

  printf("src_len :    offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN,   src_len_id_inc), vmau_task_chain_ptr->src_len_id_inc);

  printf("aux_cbp/enc_dst_paddr : offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN, aux_cbp), vmau_task_chain_ptr->aux_cbp);

  printf("c_pred_mode :    offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN, c_pred_mode_tlr), vmau_task_chain_ptr->c_pred_mode_tlr);

  printf("y_pred_mode[0] : offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN, y_pred_mode[0]), vmau_task_chain_ptr->y_pred_mode[0]);

  printf("y_pred_mode[1] : offset = 0x%x, val = 0x%x\n", 
	 offsetof(VMAU_TASK_CHAIN, y_pred_mode[1]), vmau_task_chain_ptr->y_pred_mode[1]);
  printf("\n");
}

void dump_vmau_src(const uint8_t * vmau_src_y, const uint8_t * vmau_src_u, const uint8_t * vmau_src_v){
    int i, j;
    printf("\ndump vmau_src y raw pxls:\n");
    for ( j = 0 ; j < 16; j++ ){
	for ( i =  0; i < 16; i++)
	{
	    printf("%2x ", vmau_src_y[j*16+i]);
        }	
	printf("\n");
    }
    printf("\n");

    printf("dump vmau_src u raw pxls:\n");
    for ( j = 0 ; j < 8; j++ ){
	for ( i =  0; i < 8; i++)
	{
	    printf("%2x ", vmau_src_u[j*16+i]);
        }	
	printf("\n");
    }
    printf("\n");

    printf("dump vmau_src v raw pxls:\n");
    for ( j = 0 ; j < 8; j++ ){
	for ( i =  0; i < 8; i++)
	{
	    printf("%2x ", vmau_src_v[j*16+i]);
        }	
	printf("\n");
    }
    printf("\n");
}

void dump_vmau_mc_fifo(const unsigned char *mc_fifo_y, 
		       const unsigned char *mc_fifo_u, const unsigned char *mc_fifo_v){

    const int fifo_depth = 5;

    int fifo_index = 0;
    int i = 0, j = 0;

    for(fifo_index = 0; fifo_index < fifo_depth; fifo_index++){

      printf("fifo index = %d\n", fifo_index);
      printf("mc_fifo_y paddr: 0x%08x\n", do_get_phy_addr(mc_fifo_y));
      for(i = 0; i < 16; i++){
	for(j = 0; j < 16; j++){
	  printf("%2x ", mc_fifo_y[i*16 + j]);
	}
	printf("\n");
      }

      printf("mc_fifo_u paddr: 0x%08x\n", do_get_phy_addr(mc_fifo_u));
      for(i = 0; i < 8; i++){
	for(j = 0; j < 8; j++){
	  printf("%2x ", mc_fifo_u[i*16 + j]);
	}
	printf("\n");
      }

      printf("mc_fifo_v paddr: 0x%08x\n", do_get_phy_addr(mc_fifo_v));
      for(i = 0; i < 8; i++){
	for(j = 0; j < 8; j++){
	  printf("%2x ", mc_fifo_v[i*16 + j]);
	}
	printf("\n");
      }

      mc_fifo_y += 256;
      mc_fifo_u += 128;
      mc_fifo_v += 128;

      printf("\n");
    }
}

void dump_vmau_quant_table(const unsigned int *quant_table){
    int i_list;

    printf("\nquant_table_paddr: 0x%08x\n", do_get_phy_addr(quant_table));

    printf("dec quant table:\n");
    for( i_list = 0; i_list < 4; i_list++ )
    {
        int i;
        for ( i = 0 ; i < 4; i++)
        {
	    printf("0x%x, ", quant_table);
	    quant_table++;
        }
	printf("\n");
    }

    printf("enc quant table:\n");
    for( i_list = 0; i_list < 4; i_list++ )
    {
        int i;
        for ( i = 0 ; i < 8; i++)
        {
	    printf("0x%x, ", quant_table);
	    quant_table++;
        }
	printf("\n");
    }
}

void dump_vmau_cbp_res(const short *vmau_enc_dst){
  //printf("enc cbp: %x\n",v_vmau_var->enc_dst.cbp);
  printf("\nenc cbp: 0x%08x\n",*(int *)vmau_enc_dst);
  
  short * res = vmau_enc_dst + 2;
  printf("res:\n");
  int i = 0, j = 0;
    for(i = 0; i < 16; i++){
      for(j = 0; j < 16; j++){
	printf(" %d ", res[i*16 + j] );
      }
      printf("\n");
    }
}

void dump_vmau_dec_rst(const unsigned char *vmau_dec_rst){
  int i = 0, j = 0;

  printf("\nreconstructed y pxls:\n");
  for ( i = 0 ; i < 16; i++){
     for ( j = 0 ; j < 16; j++){
	printf("%2x ", vmau_dec_rst[i*16+j]);
     }
     printf("\n");
  }

  printf("\nreconstructed u pxls:\n");
  vmau_dec_rst += 256;
  for ( i = 0 ; i < 8; i++){
     for ( j = 0 ; j < 8; j++){
	printf("%2x ", vmau_dec_rst[i*16+j]);
     }
     printf("\n");
  }
  
  printf("\nreconstructed v pxls:\n");
  vmau_dec_rst += 8;
  for ( i = 0 ; i < 8; i++){
     for ( j = 0 ; j < 8; j++){
	printf("%2x ", vmau_dec_rst[i*16+j]);
     }
     printf("\n");
  }

}

int verify_vmau_enc_rst(const unsigned int sw_cbp, const short *sw_res, 
			 const unsigned int hw_cbp, const short *hw_res){
#if 1
	// check encoder result
	int i;
	int j;
	const short *mau_ptr = hw_res;
	const short *enc_ref_buf = sw_res;
	int y_err = 0 ;
	int u_err = 0 ;
	int v_err = 0 ;
	int ydc_err = 0;
	int udc_err = 0;
	int vdc_err = 0;
	int cbp_err = 0 ;

	unsigned int enc_ref_cbp = sw_cbp;

	if ( enc_ref_cbp != hw_cbp ){
	  printf("\n\ncbp error  sw:0x%08x, vmau:0x%08x\n", enc_ref_cbp, hw_cbp);
	  cbp_err = 1;
	}

	for ( i= 0 ; i < 16; i++ ){
	  if ( (enc_ref_cbp>>i ) & 0x1 ){
	    //h->zigzagf.scan_4x4( h->dct.luma4x4[i],  mau_ptr);
	    int ii;
	    int jj;
	    for ( ii = 0 ; ii<4; ii++)
	    {
	      for ( jj = 0 ; jj < 4; jj++)
	      {				
		  if ( mau_ptr[ii+jj*4] != enc_ref_buf[i*16+4*ii+jj])
		  {
		    y_err = 1;
		    printf("\nvmau enc_y error:\n");
		    goto dump_enc_y;
		  }	    
	      }
	    }

	    //dump_int16(mau_ptr, 16);

dump_enc_y:	
	    if ( y_err )
	    {
	        y_err = 0;
		printf("cbp_bit: %d\n",i);
		int ii, jj;
		printf("sw: ");
		for ( ii = 0 ; ii<4; ii++)
		{
		  for ( jj = 0 ; jj < 4; jj++)
		  {
		    printf("%2d, ", ((short *)(enc_ref_buf))[i*16+4*ii+jj] );
		  }
	        }

		printf("\nhw: ");
		for ( ii = 0 ; ii<4; ii++)
		{
		  for ( jj = 0 ; jj < 4; jj++)
		  {
		    printf("%2d, ", ((short *)mau_ptr)[ii+4*jj] );
		  }
	        }

		printf("\n\n");
	    }
	    mau_ptr += 16;
	}
      }

      if ( (enc_ref_cbp >> MAU_Y_DC_DCT_SFT) & 0x1 ){
	//h->zigzagf.scan_4x4( h->dct.luma16x16_dc, mau_ptr );
	int ii;
	int jj;
	for ( ii = 0 ; ii<4; ii++)
	  for ( jj = 0 ; jj < 4; jj++)
	  {
	      if ( mau_ptr[ii+4*jj] != enc_ref_buf[24*16+4*ii+jj] )
		{
		  printf("\nVMAU Y DC error:\n");
		  ydc_err = 1;
		  goto dump_y_dc;
		}
	  }

dump_y_dc:
	if ( ydc_err )
	{
	    for ( ii = 0 ; ii<4; ii++)
	      for ( jj = 0 ; jj < 4; jj++)
		{
		  printf("%2d:%2d, ", ((short *)enc_ref_buf)[24*16+4*ii+jj], ((short *)mau_ptr)[ii+4*jj]);
		}
	    printf("\n");
	      //exit(0);
	}
	mau_ptr += 16;
      }

      for ( i=16 ; i < 20; i++ ){
	if ( (enc_ref_cbp>>i ) & 0x1 ){
	  int ii;
	  int jj;
	  for ( ii = 0 ; ii<4; ii++)
	    for ( jj = 0 ; jj < 4; jj++)
	    {
		if ( mau_ptr[4*jj+ii] != enc_ref_buf[i*16+4*ii+jj]){
		  u_err = 1;
		  printf("u error: [%d, %d]\n", i, j);
		}
	    }

	  if ( u_err )
	  {
	    printf("\n--%d--: ",i);
	    int ii;
	    int jj;
	    for ( ii = 0 ; ii<4; ii++)
	      for ( jj = 0 ; jj < 4; jj++)
		{
		  printf("[%x-%x],", enc_ref_buf[i*16+4*ii+jj], mau_ptr[4*jj+ii]);
		}
	  }

	  mau_ptr += 16;
	}
      }


      if ( (enc_ref_cbp >> MAU_U_DC_DCT_SFT) & 0x1 ){
	int ii;
	int jj;
	for ( ii = 0 ;  ii<2; ii++)
	  for ( jj = 0 ;  jj<2; jj++)
	{
	  if ( mau_ptr[ii+2*jj] != enc_ref_buf[25*16+2*ii+jj] )
	    {
	      printf("\n---- VMAU U DC error ---\n");
	      udc_err = 1;
	    }	  
	}
	if ( udc_err )
	{
	    int ii;
	    int jj;
	    for ( ii = 0 ;  ii<2; ii++)
	      for ( jj = 0 ;  jj<2; jj++)
		{
		  printf(" [%x, %x]\n", enc_ref_buf[25*16+2*ii+jj], mau_ptr[ii+2*jj]);
		}	  
	}
	mau_ptr += 4;
      }

      for ( i=20 ; i < 24; i++ ){
	if ( (enc_ref_cbp>>i ) & 0x1 ){
	  int ii;
	  int jj;
	  for ( ii = 0 ; ii<4; ii++)
	    for ( jj = 0 ; jj < 4; jj++)
	    {
	      if ( mau_ptr[4*jj+ii] != enc_ref_buf[i*16+4*ii+jj]){
		v_err = 1;
		printf("v error: [%d, %d]\n", i, j);
	      }
	    }

	  if ( v_err )
	  {
	    int ii;
	    int jj;
	    for ( ii = 0 ; ii<4; ii++)
	      for ( jj = 0 ; jj < 4; jj++)
		{
		  printf("%x-%x", enc_ref_buf[i*16+4*ii+jj], mau_ptr[4*jj+ii]);
		}
	  }
	  mau_ptr += 16;
	}
      }      

      if ( (enc_ref_cbp >> MAU_V_DC_DCT_SFT) & 0x1 ){
	int ii;
	int jj;
	for ( ii = 0 ;  ii<2; ii++)
	  for ( jj = 0 ;  jj<2; jj++)
	  {
	    if ( mau_ptr[ii+2*jj] != enc_ref_buf[25*16+4+2*ii+jj] )
	    {
	      printf("\nVMAU V DC error:\n");
	      vdc_err = 1;
	      goto dump_v_dc;
	    }	  
	  }

dump_v_dc:
	if ( vdc_err )
	{
	    int ii;
	    int jj;
	    for ( ii = 0 ;  ii<2; ii++)
	      for ( jj = 0 ;  jj<2; jj++)
	      {
		  printf(" [%x, %x]\n", enc_ref_buf[25*16+4+2*ii+jj], mau_ptr[ii+2*jj]);
	      }	  
	}

	mau_ptr += 4;
      }

      if (cbp_err || y_err || u_err || v_err || ydc_err || udc_err || vdc_err)
      {
	  printf("\n--VMAU ENC ERROR!!! \n");
	  return -1;
      }
      else 
      {
	  return 0;
      }

#else
      // check encoder result
      int i;
      int j;
      int y_err = 0 ;
      int u_err = 0 ;
      int v_err = 0 ;
      int ydc_err = 0;
      int udc_err = 0;
      int vdc_err = 0;
      int cbp_err = 0 ;

      const short *enc_ref_buf = sw_res;      
      unsigned int enc_ref_cbp = sw_cbp;

      const short *mau_ptr = hw_res -2;
      
      if ( *((unsigned int *)mau_ptr) != enc_ref_cbp ){
	printf("\n---------cbp error: %x-%x\n", enc_ref_cbp, *((unsigned int *)mau_ptr));
	cbp_err = 1;
	}

      mau_ptr = hw_res;

      for ( i= 0 ; i < 16; i++ ){
	if ( (enc_ref_cbp>>i ) & 0x1 ){
	  int ii;
	  int jj;
	  for ( ii = 0 ; ii<4; ii++)
	    for ( jj = 0 ; jj < 4; jj++)
	      {
		if ( mau_ptr[ii+jj*4] != enc_ref_buf[i*16+4*ii+jj]){
		  y_err = 1;
		  printf("vmau y error: [%d, %d,%d ]\n", i, ii, jj);
		}	    
	      }
	
	if ( y_err )
	  {
	    printf("\n--err: %d--: ",i);
	    int ii, jj;
	    for ( ii = 0 ; ii<4; ii++)
	      for ( jj = 0 ; jj < 4; jj++)
	      {
		printf("[%x-%x],", enc_ref_buf[i*16+4*ii+jj], mau_ptr[ii+4*jj]);
	      }
	  }
	mau_ptr += 16;
	}
      }

      if ( (enc_ref_cbp >> MAU_Y_DC_DCT_SFT) & 0x1 ){
	int ii;
	int jj;
	for ( ii = 0 ; ii<4; ii++)
	  for ( jj = 0 ; jj < 4; jj++)
	    {
	      if ( mau_ptr[ii+4*jj] != enc_ref_buf[24*16+4*ii+jj] )
		{
		  printf("\n---- VMAU Y DC error ---\n");
		  ydc_err = 1;
		}
	    }
	if ( ydc_err )
	  {
	    for ( ii = 0 ; ii<4; ii++)
	      for ( jj = 0 ; jj < 4; jj++)
		{
		  printf("%x-%x,", enc_ref_buf[24*16+4*ii+jj], mau_ptr[ii+4*jj]);
		}
	    printf("\n");
	      //exit(0);
	  }
	mau_ptr += 16;
      }

      for ( i=16 ; i < 20; i++ ){
	if ( (enc_ref_cbp>>i ) & 0x1 ){
	  int ii;
	  int jj;
	  for ( ii = 0 ; ii<4; ii++)
	    for ( jj = 0 ; jj < 4; jj++)
	      {
		if ( mau_ptr[4*jj+ii] != enc_ref_buf[i*16+4*ii+jj]){
		  u_err = 1;
		  printf("u error: [%d, %d]\n", i, j);
		}
	      }

	if ( u_err )
	  {
	    printf("\n--%d--: ",i);
	    int ii;
	    int jj;
	    for ( ii = 0 ; ii<4; ii++)
	      for ( jj = 0 ; jj < 4; jj++)
		{
		  printf("[%x-%x],", enc_ref_buf[i*16+4*ii+jj], mau_ptr[4*jj+ii]);
		}
	  }
	mau_ptr += 16;
	}
      }


      if ( (enc_ref_cbp >> MAU_ENC_RESULT_U_DC_MSK) & 0x1 ){
	int ii;
	int jj;
	for ( ii = 0 ;  ii<2; ii++)
	  for ( jj = 0 ;  jj<2; jj++)
	{
	  if ( mau_ptr[ii+2*jj] != enc_ref_buf[25*16+2*ii+jj] )
	    {
	      printf("\n---- VMAU U DC error ---\n");
	      udc_err = 1;
	    }	  
	}
	if ( udc_err )
	  {
	    int ii;
	    int jj;
	    for ( ii = 0 ;  ii<2; ii++)
	      for ( jj = 0 ;  jj<2; jj++)
		{
		  printf(" [%x, %x]\n", enc_ref_buf[25*16+2*ii+jj], mau_ptr[ii+2*jj]);
		}	  
	  }
	mau_ptr += 4;
      }

      for ( i=20 ; i < 24; i++ ){
	if ( (enc_ref_cbp>>i ) & 0x1 ){
	  int ii;
	  int jj;
	  for ( ii = 0 ; ii<4; ii++)
	    for ( jj = 0 ; jj < 4; jj++)
	  {
	    if ( mau_ptr[4*jj+ii] != enc_ref_buf[i*16+4*ii+jj]){
	      v_err = 1;
	      printf("v error: [%d, %d]\n", i, j);
	    }
	  }
	if ( v_err )
	  {
	    int ii;
	    int jj;
	    for ( ii = 0 ; ii<4; ii++)
	      for ( jj = 0 ; jj < 4; jj++)
		{
		  printf("%x-%x", enc_ref_buf[i*16+4*ii+jj], mau_ptr[4*jj+ii]);
		}
	  }
	mau_ptr += 16;
	}
      }      

      if ( (enc_ref_cbp >> MAU_ENC_RESULT_V_DC_MSK) & 0x1 ){
	int ii;
	int jj;
	for ( ii = 0 ;  ii<2; ii++)
	  for ( jj = 0 ;  jj<2; jj++)
	{
	  if ( mau_ptr[ii+2*jj] != enc_ref_buf[25*16+4+2*ii+jj] )
	    {
	      printf("\n---- VMAU V DC error ---\n");
	      vdc_err = 1;
	    }	  
	}
	if ( vdc_err )
	  {
	    int ii;
	    int jj;
	    printf("mau_ptr: %x\n", mau_ptr);
	    for ( ii = 0 ;  ii<2; ii++)
	      for ( jj = 0 ;  jj<2; jj++)
		{
		  printf(" [%x, %x]\n", enc_ref_buf[25*16+4+2*ii+jj], mau_ptr[ii+2*jj]);
		}	  
	  }
	mau_ptr += 4;
      }

      if (y_err || 
	  u_err  ||
	  v_err  ||
	  ydc_err ||
	  udc_err ||
	  vdc_err ||
	  cbp_err
	  )
	{
	  printf("\n--VMAU ENC ERROR!!! \n");
	  return -1;
	}
      else 
	{
	  return 0;
	}


#endif

}

#define FDEC_STRIDE 32

int verify_vmau_dec_rst(const unsigned char *sw_y, const unsigned char *sw_u, const unsigned char *sw_v, 
			 const unsigned char *hw_y, const unsigned char *hw_u, const unsigned char *hw_v){

      int y_dec_erro = 0;
      int u_dec_erro = 0;
      int v_dec_erro = 0;
      unsigned char *ptr;

      int i;
      int j;

      unsigned char *vmau_decdst_buf = hw_y;

      ptr = sw_y;
      for ( i = 0 ; i < 16; i++){
	for ( j = 0 ; j < 16; j++){
	  if ( vmau_decdst_buf[i*16+j] != ptr[i*FDEC_STRIDE + j] )
	  {
	      printf("\n------dec y error--- %d,%d", i, j);
	      y_dec_erro = 1;
	      //break;
	      goto dump_y;
	  }
	}
      }

dump_y:
      if ( y_dec_erro )
      {
	  printf("\n");
	  for ( i = 0 ; i < 16; i++){
	    for ( j = 0 ; j < 16; j++){
	      if ( vmau_decdst_buf[i*16+j] != ptr[i*FDEC_STRIDE + j] )
		printf("%x+%x, ", ptr[i*FDEC_STRIDE + j], vmau_decdst_buf[i*16+j]);
	      else
		printf("%x-%x, ", ptr[i*FDEC_STRIDE + j], vmau_decdst_buf[i*16+j]);
	    }
	    printf("\n");
	  }
      }

      ptr = sw_u;
      unsigned char *mau_dec_ptr = &vmau_decdst_buf[16*16];
      for ( i = 0 ; i < 8; i++){
	for ( j = 0 ; j < 8; j++){
	  if ( mau_dec_ptr[i*16+j] != ptr[i*FDEC_STRIDE + j] )
	    {
	      printf("\n------dec u error--- %d,%d\n", i, j);
	      u_dec_erro = 1;
	      goto dump_u;
	    }
	}
      }

dump_u:
      if ( u_dec_erro )
      {
	  for ( i = 0 ; i < 8; i++){
	    for ( j = 0 ; j < 8; j++){
	      if ( vmau_decdst_buf[i*16+j] != ptr[i*FDEC_STRIDE + j] )
	      {
		printf("%x+%x, ", ptr[i*FDEC_STRIDE + j], mau_dec_ptr[i*16+j]);
	      }else{
		printf("%x-%x, ", ptr[i*FDEC_STRIDE + j], mau_dec_ptr[i*16+j]);
	      }
	    }
	    printf("\n");
	  }
      }

      ptr = sw_v;      
      mau_dec_ptr = mau_dec_ptr + 8;
      for ( i = 0 ; i < 8; i++){
	for ( j = 0 ; j < 8; j++){
	  if ( mau_dec_ptr[i*16+j] != ptr[i*FDEC_STRIDE + j] )
	    {
	      printf("\n------dec v error--- %d,%d", i, j);
	      v_dec_erro = 1;
	      goto dump_v;
	    }
	}
      }

dump_v:
      if ( v_dec_erro )
      {
	  printf("\n");
	  for ( i = 0 ; i < 8; i++){
	    for ( j = 0 ; j < 8; j++){
	      if ( mau_dec_ptr[i*16+j] != ptr[i*FDEC_STRIDE + j] )
	      {
		  printf("%x+%x, ", ptr[i*FDEC_STRIDE + j], mau_dec_ptr[i*16+j]);
	      }
	      else
	      {
		  printf("%x-%x, ", ptr[i*FDEC_STRIDE + j], mau_dec_ptr[i*16+j]);
	      }
	    }
	    printf("\n");
	  }
      }

      if (y_dec_erro || 
	  u_dec_erro  ||
	  v_dec_erro
	  )
	{
	  printf("\n--VMAU DEC ERROR!!! \n");
	  return -1;
	}
      else 
	{
	  return 0;
	}

}

void save_vmau_dec_rst(unsigned char *sw_y, unsigned char *sw_u, unsigned char *sw_v, 
			 const unsigned char *hw_y, const unsigned char *hw_u, const unsigned char *hw_v){

      int i;
      int j;

      unsigned char *ptr;
      unsigned char *vmau_decdst_buf = hw_y;

      ptr = sw_y;
      for ( i = 0 ; i < 16; i++){
	for ( j = 0 ; j < 16; j++){
	  ptr[i*FDEC_STRIDE + j] = vmau_decdst_buf[i*16+j]; 
	}
      }

      ptr = sw_u;
      unsigned char *mau_dec_ptr = &vmau_decdst_buf[16*16];
      for ( i = 0 ; i < 8; i++){
	for ( j = 0 ; j < 8; j++){
	  ptr[i*FDEC_STRIDE + j] = mau_dec_ptr[i*16+j]; 
	}
      }


      ptr = sw_v;      
      mau_dec_ptr = mau_dec_ptr + 8;
      for ( i = 0 ; i < 8; i++){
	for ( j = 0 ; j < 8; j++){
	  ptr[i*FDEC_STRIDE + j] = mau_dec_ptr[i*16+j];
	}
      }
}

void dump_nnz(const unsigned char *nnz){
  int i = 0, j = 0;

  printf("dump nnz:\n");
  for(i = 0; i < 6; i++){
    for(j = 0; j < 8; j++){
      printf("%02x, ", nnz[i*6 + j]);
    }
    printf("\n");
  }
}

void dump_bs(const uint8_t * bs, int bs_len){
  int i, j;

  printf("%d", bs_len);
  for(i = 0; i < bs_len; i++){
      if(i%16 == 0){
	printf("\n");
      }

      printf("%2x, ", bs[i]);
  }

  printf("\n");

}

#if 0
void dump_entropy_var(const ENTROPY_VAR *entropy_var_ptr){
  printf("dump_entropy_var\n");  

  printf("i_mb_x = %d\n", entropy_var_ptr->i_mb_x);
  printf("i_mb_y = %d\n", entropy_var_ptr->i_mb_y);
  printf("i_mb_xy = %d\n", entropy_var_ptr->i_mb_xy);
  //printf("i_type = %d\n", entropy_var_ptr->i_type);

  printf("i_intra16x16_pred_mode = %d\n", entropy_var_ptr->i_intra16x16_pred_mode);
}
#endif

void dump_mem(const void *mem_ptr, int len){
  int i, j;

  printf("%d", len);
  for(i = 0; i < len; i++){
      if(i%16 == 0){
	printf("\n");
      }

      printf("%2x, ", ((uint8_t*)mem_ptr)[i]);
  }

  printf("\n");

}

void dump_int16(const void *mem_ptr, int len){
  int i, j;

  printf("%d", len);
  for(i = 0; i < len; i++){
      if(i%16 == 0){
	printf("\n");
      }

      printf("%2d, ", ((int16_t*)mem_ptr)[i]);
  }

  printf("\n");

}
