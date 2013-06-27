#include "t_motion.h"

#include "jz4760e_2ddma_hw.h"
#include "tcsm1.h"
#include "sram.h"


#define FENC_STRIDE 16
//#define FDEC_STRIDE 32

#define UPDATE_FIFO_INDEX(index, fifo_end) do{    \
 	    index++;                              \
	    if(index == fifo_end)             \
	    {                                     \
	        index = 0;                        \
	    }                                     \
        }while(0)


void x264_macroblock_init_index_and_neighbour(TCSM1 *tcsm1_ptr, int i_mb_x, int i_mb_y )
{
    int i_mb_stride = tcsm1_ptr->slice_var.i_mb_stride;

    int i_mb_width = tcsm1_ptr->slice_var.i_mb_width;
    int i_first_mb = tcsm1_ptr->slice_var.i_first_mb;

    int i_mb_xy = i_mb_y * i_mb_stride + i_mb_x;
    int i_top_y = i_mb_y - 1 ;
    int i_top_xy = i_top_y * i_mb_stride + i_mb_x;

    X264_MB_VAR *mb = &tcsm1_ptr->mb_array[tcsm1_ptr->aux_fifo_index];
    UPDATE_FIFO_INDEX(tcsm1_ptr->aux_fifo_index, AUX_FIFO_DEP);

    mb->i_mb_x = i_mb_x;
    mb->i_mb_y = i_mb_y;
    mb->i_mb_xy = i_mb_xy;

    unsigned int i_neighbour = 0;

    if( i_top_xy >= i_first_mb )
    {
        i_neighbour |= MB_TOP;
    }

    if( i_mb_x > 0 && i_mb_xy > i_first_mb )
    {
        i_neighbour |= MB_LEFT;

    }

    if( i_mb_x < i_mb_width - 1 && i_top_xy + 1 >= i_first_mb )
    {
        if(i_mb_x + 1 == tcsm1_ptr->slice_var.i_mb_width - 1)
	{
	  CP32(tcsm1_ptr->topright_mv, &tcsm1_ptr->top_line_mv[mb->i_mb_x]);
        }
 
        i_neighbour |= MB_TOPRIGHT;
    }

    if( i_mb_x > 0 && i_top_xy - 1 >= i_first_mb )
    {
        i_neighbour |= MB_TOPLEFT;
    }

    mb->i_neighbour = i_neighbour;
}

void ALWAYS_INLINE x264_macroblock_load_raw_data(TCSM1 *tcsm1_ptr, int i_mb_x, int i_mb_y)
{
    int w = 16;
    int i_stride = tcsm1_ptr->slice_var.i_enc_stride[0];
    int i_pix_offset = w * (i_mb_x + i_mb_y * i_stride);

    unsigned char *raw_data_y_phy_ptr = tcsm1_ptr->slice_var.sram_phy->raw_data[tcsm1_ptr->gp2_raw_data_index].y;
    unsigned char *raw_data_uv_phy_ptr = tcsm1_ptr->slice_var.sram_phy->raw_data[tcsm1_ptr->gp2_raw_data_index].uv;

    UPDATE_FIFO_INDEX(tcsm1_ptr->gp2_raw_data_index, RAW_DATA_PIPE_DEP);

    DMA_TASK_CHAIN *gp2_task_chain_ptr = &tcsm1_ptr->gp2_task_chain_array[tcsm1_ptr->gp2_pipe_index];
    DMA_TASK_CHAIN *gp2_task_chain_phy_ptr = &((TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr)->gp2_task_chain_array[tcsm1_ptr->gp2_pipe_index];

    tcsm1_ptr->gp2_pipe_index = !tcsm1_ptr->gp2_pipe_index;

    gp2_task_chain_ptr->node[0].src_phy_addr = tcsm1_ptr->slice_var.raw_plane_y_phy_ptr + i_pix_offset;
    gp2_task_chain_ptr->node[0].dst_phy_addr = raw_data_y_phy_ptr;
    gp2_task_chain_ptr->node[0].stride = GP_STRD(i_stride,GP_FRM_NML,FENC_STRIDE);
    gp2_task_chain_ptr->node[0].link_flag = GP_UNIT(GP_TAG_LK,16,256);


    w = 8;
    i_stride = tcsm1_ptr->slice_var.i_enc_stride[1];
    i_pix_offset = w * (i_mb_x + i_mb_y * i_stride);

    gp2_task_chain_ptr->node[1].src_phy_addr = tcsm1_ptr->slice_var.raw_plane_u_phy_ptr + i_pix_offset;
    gp2_task_chain_ptr->node[1].dst_phy_addr = raw_data_uv_phy_ptr;
    gp2_task_chain_ptr->node[1].stride = GP_STRD(i_stride,GP_FRM_NML,FENC_STRIDE);
    gp2_task_chain_ptr->node[1].link_flag = GP_UNIT(GP_TAG_LK,8,64);

    i_stride = tcsm1_ptr->slice_var.i_enc_stride[2];
    i_pix_offset = w * (i_mb_x + i_mb_y * i_stride);

    gp2_task_chain_ptr->node[2].src_phy_addr = tcsm1_ptr->slice_var.raw_plane_v_phy_ptr + i_pix_offset;
    gp2_task_chain_ptr->node[2].dst_phy_addr = raw_data_uv_phy_ptr + 8;
    gp2_task_chain_ptr->node[2].stride = GP_STRD(i_stride,GP_FRM_NML,FENC_STRIDE);
    gp2_task_chain_ptr->node[2].link_flag = GP_UNIT(GP_TAG_UL,8,64);


    poll_gp2_end();

    set_gp2_dha(&gp2_task_chain_phy_ptr->node[0]);
    set_gp2_dcs();
    //poll_gp2_end();
}

void yuv422_to_yuv420(uint32_t *raw_yuv422_data_ptr, uint8_t *raw_y_ptr, uint8_t *raw_uv_ptr);

#define DEBUG_GP1

#ifdef DEBUG_GP1
void x264_macroblock_load_raw_yuv422_data(TCSM1 *tcsm1_ptr)
{
    TCSM1 *tcsm1_phy_ptr = (TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr;
    int i_mb_x = tcsm1_ptr->i_gp1_mb_x;;
    int i_mb_y = tcsm1_ptr->i_gp1_mb_y;;

    tcsm1_ptr->i_gp1_mb_x++;
    if( tcsm1_ptr->i_gp1_mb_x == tcsm1_ptr->slice_var.i_mb_width )
    {
        tcsm1_ptr->i_gp1_mb_y++;
        tcsm1_ptr->i_gp1_mb_x = 0;
    }

    uint32_t *raw_yuv422_phy_ptr = (uint32_t *)&tcsm1_phy_ptr->raw_yuv422_data[tcsm1_ptr->gp1_raw_data_index];

    UPDATE_FIFO_INDEX(tcsm1_ptr->gp1_raw_data_index, RAW_DATA_PIPE_DEP);

    volatile DMA_TASK_CHAIN *gp1_task_chain_ptr = &tcsm1_ptr->gp1_task_chain_array[tcsm1_ptr->gp1_pipe_index];
    DMA_TASK_CHAIN *gp1_task_chain_phy_ptr = &((TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr)->gp1_task_chain_array[tcsm1_ptr->gp1_pipe_index];

    tcsm1_ptr->gp1_pipe_index = !tcsm1_ptr->gp1_pipe_index;

    int i_stride = 16*tcsm1_ptr->slice_var.i_mb_width*2;
    int i_pix_offset = (8*i_mb_x + 16*i_mb_y * i_stride/sizeof(raw_yuv422_phy_ptr[0]));

    gp1_task_chain_ptr->node[0].src_phy_addr = (uint32_t *)tcsm1_ptr->slice_var.raw_yuv422_phy_ptr + i_pix_offset;
    gp1_task_chain_ptr->node[0].dst_phy_addr = raw_yuv422_phy_ptr;
    gp1_task_chain_ptr->node[0].stride = GP_STRD(i_stride,GP_FRM_NML,32);
    gp1_task_chain_ptr->node[0].link_flag = GP_UNIT(GP_TAG_UL,32,512);

    poll_gp1_end();//let the first polling go
    set_gp1_dha(&gp1_task_chain_phy_ptr->node[0]);
    set_gp1_dcs();
    //poll_gp1_end();

}

void copy_raw_yuv420_data_from_tcsm1_to_sram(TCSM1 *tcsm1_ptr)
{
    TCSM1 *tcsm1_phy_ptr = (TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr;
    unsigned char *raw_data_y_phy_ptr = tcsm1_ptr->slice_var.sram_phy->raw_data[tcsm1_ptr->gp2_raw_data_index].y;
    uint32_t *raw_yuv422_phy_ptr = (uint32_t *)&tcsm1_phy_ptr->raw_yuv422_data[tcsm1_ptr->gp2_raw_data_index];
    UPDATE_FIFO_INDEX(tcsm1_ptr->gp2_raw_data_index, RAW_DATA_PIPE_DEP);

    int gp2_pipe_index = tcsm1_ptr->gp2_pipe_index;
    tcsm1_ptr->gp2_pipe_index = !tcsm1_ptr->gp2_pipe_index;

    yuv422_to_yuv420(raw_yuv422_phy_ptr, tcsm1_ptr->raw_data[gp2_pipe_index].y, tcsm1_ptr->raw_data[gp2_pipe_index].uv);    

    DMA_TASK_CHAIN *gp2_task_chain_ptr = &tcsm1_ptr->gp2_task_chain_array[gp2_pipe_index];
    DMA_TASK_CHAIN *gp2_task_chain_phy_ptr = &((TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr)->gp2_task_chain_array[gp2_pipe_index];


#ifndef RAW_YUV420_DATA_LEN
#define RAW_YUV420_DATA_LEN sizeof(tcsm1_ptr->raw_data)
    gp2_task_chain_ptr->node[0].src_phy_addr = &tcsm1_phy_ptr->raw_data[gp2_pipe_index].y;
    gp2_task_chain_ptr->node[0].dst_phy_addr = raw_data_y_phy_ptr;
    gp2_task_chain_ptr->node[0].stride = GP_STRD(RAW_YUV420_DATA_LEN,GP_FRM_NML,RAW_YUV420_DATA_LEN);
    gp2_task_chain_ptr->node[0].link_flag = GP_UNIT(GP_TAG_UL,RAW_YUV420_DATA_LEN, RAW_YUV420_DATA_LEN);
#undef RAW_YUV420_DATA_LEN
#else
#error RAW_YUV420_DATA_LEN had been defined
#endif

    poll_gp2_end();//let the first polling go

    set_gp2_dha(&gp2_task_chain_phy_ptr->node[0]);
    set_gp2_dcs();
    //poll_gp2_end();

}

#else

void copy_raw_yuv420_data_from_tcsm1_to_sram(TCSM1 *tcsm1_ptr){}

void x264_macroblock_load_raw_yuv422_data(TCSM1 *tcsm1_ptr)
{
    int i_mb_x = tcsm1_ptr->i_gp1_mb_x;;
    int i_mb_y = tcsm1_ptr->i_gp1_mb_y;;

    tcsm1_ptr->i_gp1_mb_x++;
    if( tcsm1_ptr->i_gp1_mb_x == tcsm1_ptr->slice_var.i_mb_width )
    {
        tcsm1_ptr->i_gp1_mb_y++;
        tcsm1_ptr->i_gp1_mb_x = 0;
    }


    unsigned char *raw_data_y_phy_ptr = tcsm1_ptr->slice_var.sram_phy->raw_data[tcsm1_ptr->gp2_raw_data_index].y;
    unsigned char *raw_data_uv_phy_ptr = tcsm1_ptr->slice_var.sram_phy->raw_data[tcsm1_ptr->gp2_raw_data_index].uv;

    uint32_t *raw_yuv422_phy_ptr = (uint32_t *)&tcsm1_ptr->slice_var.sram_phy->raw_yuv422_data[tcsm1_ptr->gp2_raw_data_index];


    UPDATE_FIFO_INDEX(tcsm1_ptr->gp2_raw_data_index, RAW_DATA_PIPE_DEP);

    DMA_TASK_CHAIN *gp2_task_chain_ptr = &tcsm1_ptr->gp2_task_chain_array[tcsm1_ptr->gp2_pipe_index];
    DMA_TASK_CHAIN *gp2_task_chain_phy_ptr = &((TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr)->gp2_task_chain_array[tcsm1_ptr->gp2_pipe_index];

    tcsm1_ptr->gp2_pipe_index = !tcsm1_ptr->gp2_pipe_index;

    int i_stride = 16*tcsm1_ptr->slice_var.i_mb_width*2;
    //int i_stride = 16*40*2;
    int i_pix_offset = (8*i_mb_x + 16*i_mb_y * i_stride/sizeof(raw_yuv422_phy_ptr[0]));

    gp2_task_chain_ptr->node[0].src_phy_addr = (uint32_t *)tcsm1_ptr->slice_var.raw_yuv422_phy_ptr + i_pix_offset;
    gp2_task_chain_ptr->node[0].dst_phy_addr = raw_yuv422_phy_ptr;
    gp2_task_chain_ptr->node[0].stride = GP_STRD(i_stride,GP_FRM_NML,32);
    gp2_task_chain_ptr->node[0].link_flag = GP_UNIT(GP_TAG_UL,32, 512);

    //poll_gp2_end();

    set_gp2_dha(&gp2_task_chain_phy_ptr->node[0]);
    set_gp2_dcs();
    poll_gp2_end();


#if 1    
    int i;

    yuv422_to_yuv420(raw_yuv422_phy_ptr, tcsm1_ptr->raw_data.y, tcsm1_ptr->raw_data.uv);    

    for(i = 0; i < sizeof(tcsm1_ptr->raw_data)/sizeof(int); i++){
        *((uint32_t*)raw_data_y_phy_ptr + i) = *((uint32_t*)&tcsm1_ptr->raw_data + i);
    }
#endif

}
#endif

#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define X264_MAX(a,b) ( (a)>(b) ? (a) : (b) )

static inline int x264_clip3_hw( int v, int i_min, int i_max )
{
    return ( (v < i_min) ? i_min : (v > i_max) ? i_max : v );
}


static inline int x264_median_hw( int a, int b, int c )
{
    int t = (a-b)&((a-b)>>31);
    a -= t;
    b += t;
    b -= (b-c)&((b-c)>>31);
    b += (a-b)&((a-b)>>31);
    return b;
}

static inline void x264_median_mv_hw( int16_t *dst, int16_t *a, int16_t *b, int16_t *c )
{
    dst[0] = x264_median_hw( a[0], b[0], c[0] );
    dst[1] = x264_median_hw( a[1], b[1], c[1] );
}

void x264_mb_predict_mv_16x16_hw( TCSM1 *tcsm1_ptr, int16_t mvp[2] )
{
    int16_t mv_a[2];

    //X264_MB_VAR *mb = &tcsm1_ptr->mb_array[tcsm1_ptr->aux_fifo_index];
    X264_MB_VAR *mb = &tcsm1_ptr->mb_array[tcsm1_ptr->mce_aux_fifo_index];

    int i_neighbour = mb->i_neighbour;

    if(i_neighbour & MB_LEFT)
    {
        M32(mv_a) = M32(tcsm1_ptr->left_mv);
    }else
    {
        M32(mv_a) = 0;
    }

    int16_t mv_b[2];
    if(i_neighbour & MB_TOP)
    {
      CP32( mv_b, &tcsm1_ptr->top_line_mv[mb->i_mb_x]);
    }else
    {
      M32(mv_b) = 0;
    }

    int16_t mv_c[2];
    if(i_neighbour & MB_TOPRIGHT)
    {
        CP32(mv_c, &tcsm1_ptr->top_line_mv[mb->i_mb_x + 1]);
    }else if(i_neighbour & MB_TOPLEFT)
    {
        CP32(mv_c, tcsm1_ptr->topright_mv);
    }else
    {
        M32(mv_c) = 0;
    }

    if( !(i_neighbour & MB_TOP) && !(i_neighbour & MB_TOPLEFT) && i_neighbour & MB_LEFT )
    {
        CP32( mvp, mv_a );
    }
    else
    {
        x264_median_mv_hw( mvp, mv_a, mv_b, mv_c );
    }
}

void esti_execute( MCE_VAR *mce, TCSM1 *tcsm1_ptr)
{
  X264_MV_VAR *mv_var_ptr = &tcsm1_ptr->mv_var;
  //X264_MB_VAR *mb = &tcsm1_ptr->mb_array[tcsm1_ptr->aux_fifo_index];
  X264_MB_VAR *mb = &tcsm1_ptr->mb_array[tcsm1_ptr->mce_aux_fifo_index];

  int *tdd = (int *)&mce->task_chain;

  int bmx, bmy;
  int pmx, pmy;


  int i_fmv_range = tcsm1_ptr->slice_var.i_fmv_range;

  // limit motion search to a slightly smaller range than the theoretical limit,
  // since the search may go a few iterations past its given range
  int i_fpel_border = 6; // umh: 1 for diamond, 2 for octagon, 2 for hpel

  /* Calculate max allowed MV range */
#define CLIP_FMV(mv) x264_clip3_hw( mv, -i_fmv_range, i_fmv_range-1 )

  mv_var_ptr->mv_min[0] = 4*( -16*mb->i_mb_x - 24 );
  mv_var_ptr->mv_max[0] = 4*( 16*( tcsm1_ptr->slice_var.i_mb_width - mb->i_mb_x - 1 ) + 24 );

  mv_var_ptr->mv_min_spel[0] = CLIP_FMV( mv_var_ptr->mv_min[0] );
  mv_var_ptr->mv_max_spel[0] = CLIP_FMV( mv_var_ptr->mv_max[0] );

  mv_var_ptr->mv_min_fpel[0] = (mv_var_ptr->mv_min_spel[0]>>2) + i_fpel_border;
  mv_var_ptr->mv_max_fpel[0] = (mv_var_ptr->mv_max_spel[0]>>2) - i_fpel_border;

#undef CLIP_FMV

  int mv_x_min = X264_MAX(mv_var_ptr->mv_min_fpel[0], -16);
  int mv_x_max = X264_MIN(mv_var_ptr->mv_max_fpel[0],  16);

  int mv_y_min = X264_MAX(mv_var_ptr->mv_min_fpel[1], -16);
  int mv_y_max = X264_MIN(mv_var_ptr->mv_max_fpel[1],  16);

  bmx = x264_clip3_hw( mv_var_ptr->mvp[0], mv_x_min*4, mv_x_max*4 );
  bmy = x264_clip3_hw( mv_var_ptr->mvp[1], mv_y_min*4, mv_y_max*4 );


  pmx = (( bmx + 2 ) >> 2)<<2;
  pmy = (( bmy + 2 ) >> 2)<<2;

  tdd[0] = TDD_CFG(1, 1, REG1_MVINFO); 
  tdd[1] = (pmx & 0xFFFF) | ((pmy & 0xFFFF)<<16);

  //#define DEBUG_MCE_PIPE

#ifdef DEBUG_MCE_PIPE
  if(mb->i_mb_xy <= tcsm1_ptr->slice_var.i_last_mb){
    tdd[2] = TDD_ESTI(1,/*vld*/
		    1,/*lk*/
		    0,/*dmy*/
		    1,/*pmc*/
		    0,/*list*/
		    0,/*boy*/
		    0,/*box*/
		    3,/*bh*/
		    3,/*bw*/
		    mb->i_mb_y,/*mby*/
		    mb->i_mb_x/*mbx*/);
  }else{
    tdd[2] = TDD_ESTI(1,/*vld*/
		    1,/*lk*/
		    1,/*dmy: dump data out*/
		    1,/*pmc*/
		    0,/*list*/
		    0,/*boy*/
		    0,/*box*/
		    3,/*bh*/
		    3,/*bw*/
		    mb->i_mb_y,/*mby*/
		    mb->i_mb_x/*mbx*/);
  
  }

  tdd[3] = TDD_SYNC(1,/*vld*/
		    0,/*lk*/
		    0xFFFF/*id*/);
#else

  tdd[2] = TDD_ESTI(1,/*vld*/
		    1,/*lk*/
		    0,/*dmy*/
		    1,/*pmc*/
		    0,/*list*/
		    0,/*boy*/
		    0,/*box*/
		    3,/*bh*/
		    3,/*bw*/
		    mb->i_mb_y,/*mby*/
		    mb->i_mb_x/*mbx*/);

  tdd[3] = TDD_ESTI(1,/*vld*/
		    1,/*lk*/
		    1,/*dmy: dump data out*/
		    1,/*pmc*/
		    0,/*list*/
		    0,/*boy*/
		    0,/*box*/
		    3,/*bh*/
		    3,/*bw*/
		    mb->i_mb_y,/*mby*/
		    mb->i_mb_x/*mbx*/);
  

  tdd[4] = TDD_SYNC(1,/*vld*/
		    0,/*lk*/
		    0xFFFF/*id*/);

#endif


  tcsm1_ptr->mce_end_flag = 0;  

  TCSM1 *tcsm1_phy = (TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr;
  MCE_VAR * const mce_var_phy_ptr = &tcsm1_phy->mce_var[tcsm1_ptr->pipe_index];
  SET_REG1_DDC( ((int)&mce_var_phy_ptr->task_chain + 1) );

}

void x264_mb_mce_config( TCSM1 *tcsm1_ptr)
{
    const int pipe_index = tcsm1_ptr->pipe_index;
    MCE_VAR * const mce_var_ptr = &tcsm1_ptr->mce_var[pipe_index];

    TCSM1 *tcsm1_phy = (TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr;

    MCE_VAR * const mce_var_phy_ptr = &tcsm1_phy->mce_var[pipe_index];

    SET_REG1_DSTA(&mce_var_phy_ptr->pred_out_y[0]);
    SET_REG1_DSA(&tcsm1_phy->mce_end_flag);

    SET_REG1_IWTA(&tcsm1_phy->mv_cost);   //FIXME: NO need to set each macroblock  
    SET_REG1_RAWA(&tcsm1_ptr->slice_var.sram_phy->raw_data[tcsm1_ptr->mce_raw_data_index].y);

    UPDATE_FIFO_INDEX(tcsm1_ptr->mce_raw_data_index, RAW_DATA_PIPE_DEP);

    SET_REG2_DSTA(&mce_var_phy_ptr->pred_out_u[0]);


    esti_execute(mce_var_ptr, tcsm1_ptr);
}

#define DEBUG_GP0

#ifndef DEBUG_GP0
void x264_macroblock_store_pic_to_mce(TCSM1 *tcsm1_ptr)
{
  TCSM1 *tcsm1_phy = (TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr;
  VMAU_VAR const * const vmau_var_phy = &tcsm1_phy->vmau_var[tcsm1_ptr->gp1_pipe_index];

  volatile DMA_TASK_CHAIN *gp1_task_chain_ptr = &tcsm1_ptr->gp1_task_chain_array[tcsm1_ptr->gp1_pipe_index];
  volatile DMA_TASK_CHAIN *gp1_task_chain_phy_ptr = &tcsm1_phy->gp1_task_chain_array[tcsm1_ptr->gp1_pipe_index];
  UPDATE_FIFO_INDEX(tcsm1_ptr->gp1_pipe_index, DMA_PIPE_DEP);

  X264_MB_VAR *mb = &tcsm1_ptr->mb_array[tcsm1_ptr->gp1_aux_fifo_index];
  UPDATE_FIFO_INDEX(tcsm1_ptr->gp1_aux_fifo_index, AUX_FIFO_DEP);

  uint8_t const * const vmau_dec_y_phy = vmau_var_phy->dec_dst_y;
  uint8_t const * const vmau_dec_uv_phy = vmau_var_phy->dec_dst_uv;

  int i_mb_x = mb->i_mb_x;
  int i_mb_y = mb->i_mb_y;

  int linesize  = tcsm1_ptr->slice_var.i_dec_stride;

  volatile uint32_t *tile_luma = (uint32_t *)(tcsm1_ptr->slice_var.recon_pxl_y_phy_ptr + i_mb_y*16*linesize + i_mb_x*16*16);
  volatile uint32_t *tile_chroma = (uint32_t *)(tcsm1_ptr->slice_var.recon_pxl_uv_phy_ptr + i_mb_y*8*linesize + i_mb_x*16*8);


  gp1_task_chain_ptr->node[0].src_phy_addr = (void *)vmau_dec_y_phy;
  gp1_task_chain_ptr->node[0].dst_phy_addr = (void *)tile_luma;
  gp1_task_chain_ptr->node[0].stride = GP_STRD(256,GP_FRM_NML,256);
  gp1_task_chain_ptr->node[0].link_flag = GP_UNIT(GP_TAG_LK,256,256);

  gp1_task_chain_ptr->node[1].src_phy_addr = (void *)vmau_dec_uv_phy;
  gp1_task_chain_ptr->node[1].dst_phy_addr = (void *)tile_chroma;
  gp1_task_chain_ptr->node[1].stride = GP_STRD(128,GP_FRM_NML,128);
  gp1_task_chain_ptr->node[1].link_flag = GP_UNIT(GP_TAG_UL,128,128);


  poll_gp1_end();//skip the first polling

  set_gp1_dha(&gp1_task_chain_phy_ptr->node[0]);
  set_gp1_dcs();
  //poll_gp1_end();
}

#else

void x264_macroblock_store_pic_to_mce(TCSM1 *tcsm1_ptr)
{
  TCSM1 *tcsm1_phy = (TCSM1 *)tcsm1_ptr->slice_var.tcsm1_phy_addr;

  volatile DMA_TASK_CHAIN *gp0_task_chain_ptr = &tcsm1_ptr->gp0_task_chain_array[tcsm1_ptr->gp0_pipe_index];
  volatile DMA_TASK_CHAIN *gp0_task_chain_phy_ptr = &tcsm1_phy->gp0_task_chain_array[tcsm1_ptr->gp0_pipe_index];
  const TCSM0 * const tcsm0_phy = tcsm1_ptr->slice_var.tcsm0_phy;
  const RECON_MB_PXL * const recon_mb_pxl_phy_ptr = &tcsm0_phy->recon_mb_pxl_array[tcsm1_ptr->gp0_pipe_index];
  UPDATE_FIFO_INDEX(tcsm1_ptr->gp0_pipe_index, DMA_PIPE_DEP);

  X264_MB_VAR *mb = &tcsm1_ptr->mb_array[tcsm1_ptr->gp0_aux_fifo_index];
  UPDATE_FIFO_INDEX(tcsm1_ptr->gp0_aux_fifo_index, AUX_FIFO_DEP);

  const uint8_t * const vmau_dec_y_phy = &recon_mb_pxl_phy_ptr->y[0];
  const uint8_t * const vmau_dec_uv_phy = &recon_mb_pxl_phy_ptr->uv[0];

  int i_mb_x = mb->i_mb_x;
  int i_mb_y = mb->i_mb_y;

  int linesize  = tcsm1_ptr->slice_var.i_dec_stride;

  volatile uint32_t *tile_luma = (uint32_t *)(tcsm1_ptr->slice_var.recon_pxl_y_phy_ptr + i_mb_y*16*linesize + i_mb_x*16*16);
  volatile uint32_t *tile_chroma = (uint32_t *)(tcsm1_ptr->slice_var.recon_pxl_uv_phy_ptr + i_mb_y*8*linesize + i_mb_x*16*8);


  gp0_task_chain_ptr->node[0].src_phy_addr = (void *)vmau_dec_y_phy;
  gp0_task_chain_ptr->node[0].dst_phy_addr = (void *)tile_luma;
  gp0_task_chain_ptr->node[0].stride = GP_STRD(256,GP_FRM_NML,256);
  gp0_task_chain_ptr->node[0].link_flag = GP_UNIT(GP_TAG_LK,256,256);

  gp0_task_chain_ptr->node[1].src_phy_addr = (void *)vmau_dec_uv_phy;
  gp0_task_chain_ptr->node[1].dst_phy_addr = (void *)tile_chroma;
  gp0_task_chain_ptr->node[1].stride = GP_STRD(128,GP_FRM_NML,128);
  gp0_task_chain_ptr->node[1].link_flag = GP_UNIT(GP_TAG_UL,128,128);


  poll_gp0_end();//skip the first polling

  set_gp0_dha(&gp0_task_chain_phy_ptr->node[0]);
  set_gp0_dcs();
  //poll_gp0_end();
}
#endif

void x264_macroblock_config_vmau(TCSM1 *tcsm1_ptr, vmau_reg *vmau_reg_ptr )
{    
    const int vmau_aux_fifo_index = tcsm1_ptr->vmau_aux_fifo_index;
    X264_MB_VAR *mb = &tcsm1_ptr->mb_array[vmau_aux_fifo_index];
    UPDATE_FIFO_INDEX(tcsm1_ptr->vmau_aux_fifo_index, AUX_FIFO_DEP);

    //get pred_mode
    {
          unsigned int i_neighbour = mb->i_neighbour;

	  int b_top = i_neighbour & MB_TOP;
	  int b_left = i_neighbour & MB_LEFT;
	  if( b_top && b_left )
	  {
	      mb->i_intra16x16_pred_mode = I_PRED_16x16_H;
	      mb->i_chroma_pred_mode = I_PRED_CHROMA_H;
	  }
	  else if( b_left )
	  {
	      mb->i_intra16x16_pred_mode = I_PRED_16x16_H;
	      mb->i_chroma_pred_mode = I_PRED_CHROMA_H;
	  }
	  else if( b_top )
	  {
	      mb->i_intra16x16_pred_mode = I_PRED_16x16_DC_TOP;
	      mb->i_chroma_pred_mode = I_PRED_CHROMA_V;	      
	  }
	  else
	  {
	      mb->i_intra16x16_pred_mode = I_PRED_16x16_DC_128;
	      mb->i_chroma_pred_mode = I_PRED_CHROMA_DC_128;
	  }
    }


    const int pipe_index = tcsm1_ptr->vmau_pipe_index;
    UPDATE_FIFO_INDEX(tcsm1_ptr->vmau_pipe_index, VMAU_PIPE_DEP);

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

    UPDATE_FIFO_INDEX(tcsm1_ptr->vmau_raw_data_index, RAW_DATA_PIPE_DEP);


    vmau_task_chain_ptr->ncchn_addr = (unsigned int *)vmau_task_chain_phy_ptr;    


    if( SLICE_TYPE_I == tcsm1_ptr->slice_var.i_slice_type )
    {
        const int i_mode = mb->i_intra16x16_pred_mode;

	vmau_task_chain_ptr->main_cbp = (MAU_Y_PREDE_MSK << MAU_Y_PREDE_SFT );//enable luma pred
	vmau_task_chain_ptr->main_cbp |= (M16x16  << MAU_MTX_SFT );            //enable luma 16x16
	vmau_task_chain_ptr->y_pred_mode[0] = i_mode;

	vmau_task_chain_ptr->c_pred_mode_tlr.all = mb->i_chroma_pred_mode;

    }
    else
    {//P_SLICE
	//enable luma error add
	vmau_task_chain_ptr->main_cbp = (MAU_Y_ERR_MSK << MAU_Y_ERR_SFT );
	//enable chroma error add
	vmau_task_chain_ptr->main_cbp |= (MAU_C_ERR_MSK << MAU_C_ERR_SFT );
    }


#if 1
    //polling the prev vmau end
    if(mb->i_mb_xy > 0 ){
      const int wait_vmau_loop_max = 10000000;
      volatile int wait_vmau_loop = 0;    

      wait_vmau_loop = 0;
      do{
	wait_vmau_loop++;
	if(wait_vmau_loop > wait_vmau_loop_max){
	  break;
	}
      }while( -1 == tcsm1_ptr->vmau_dec_end_flag);

      x264_macroblock_store_pic_to_mce(tcsm1_ptr);


      tcsm1_ptr->aux_fifo_write_num++;        
    }
#endif


    tcsm1_ptr->vmau_enc_end_flag = -1;
    tcsm1_ptr->vmau_dec_end_flag = -1;

    //-----------vmau reg init each macroblock once--------------------
    vmau_reg_ptr->dec_end_flag_phy_addr = &tcsm1_phy->vmau_dec_end_flag;    
    vmau_reg_ptr->enc_end_flag_phy_addr = &tcsm1_phy->vmau_enc_end_flag;    


#ifdef DEBUG_GP0
    vmau_reg_ptr->dec_y_phy_addr = recon_mb_pxl_phy_ptr->y;           
    vmau_reg_ptr->dec_u_phy_addr = recon_mb_pxl_phy_ptr->uv;           
    vmau_reg_ptr->dec_v_phy_addr = recon_mb_pxl_phy_ptr->uv + 8;           
#else
    vmau_reg_ptr->dec_y_phy_addr = vmau_var_phy->dec_dst_y;           
    vmau_reg_ptr->dec_u_phy_addr = vmau_var_phy->dec_dst_uv;           
    vmau_reg_ptr->dec_v_phy_addr = vmau_var_phy->dec_dst_uv + 8;           
#endif

    vmau_reg_ptr->next_task_chain_phy_addr = (uint32_t *)vmau_task_chain_phy_ptr; 

    vmau_reg_ptr->glb_trigger = VMAU_RUN;

#if 1
    if(mb->i_mb_xy == tcsm1_ptr->slice_var.i_last_mb ){
      const int wait_vmau_loop_max = 10000000;
      volatile int wait_vmau_loop = 0;    

      wait_vmau_loop = 0;
      do{
	wait_vmau_loop++;
	if(wait_vmau_loop > wait_vmau_loop_max){
	  break;
	}
      }while( -1 == tcsm1_ptr->vmau_dec_end_flag);

      x264_macroblock_store_pic_to_mce(tcsm1_ptr);

      //poll_gp0_end(); not necessary
      tcsm1_ptr->aux_fifo_write_num++;        
    }
#endif

}

#if 0//Y1V0Y0U0(31~0) 422 format

#if 0
void yuv422_to_yuv420(uint32_t *raw_yuv422_data_ptr, uint8_t *raw_y_ptr, uint8_t *raw_uv_ptr){
    int i, j;

    for(i = 0; i < 16; i += 2){
        for(j = 0; j < 8; j += 4){
	    S32LDD(xr1, raw_yuv422_data_ptr, 0*4 );  
	    S32LDD(xr2, raw_yuv422_data_ptr, 1*4 );  
	    S32LDD(xr3, raw_yuv422_data_ptr, 2*4 );  
	    S32LDD(xr4, raw_yuv422_data_ptr, 3*4 );  

	    S32LDD(xr5, raw_yuv422_data_ptr, (8 + 0)*4 );  
	    S32LDD(xr6, raw_yuv422_data_ptr, (8 + 1)*4 );  
	    S32LDD(xr7, raw_yuv422_data_ptr, (8 + 2)*4 );  
	    S32LDD(xr8, raw_yuv422_data_ptr, (8 + 3)*4 );  

	    S32SFL(xr9,  xr2, xr1, xr10, 1);
	    S32SFL(xr11, xr4, xr3, xr12, 1);

	    S32STD(xr9,  raw_y_ptr, 0*4);
	    S32STD(xr11, raw_y_ptr, 1*4);

	    S32SFL(xr9,  xr6, xr5, xr14, 1);
	    S32SFL(xr11, xr8, xr7, xr15, 1);

	    S32STD(xr9,  raw_y_ptr, (4+0)*4);
	    S32STD(xr11, raw_y_ptr, (4+1)*4);

	    //uv
	    Q8AVGR(xr9,  xr10, xr14);
	    Q8AVGR(xr11, xr12, xr15);

	    S32SFL(xr2,  xr11, xr9, xr1, 1);

	    S32STD(xr1, raw_uv_ptr, 0);
	    S32STD(xr2, raw_uv_ptr, 8);

	    raw_y_ptr += 8;
	    raw_uv_ptr += 4;       
	    raw_yuv422_data_ptr += 4;
        }

	raw_y_ptr += 16;
        raw_uv_ptr += 8;
	raw_yuv422_data_ptr += 8;
    }
}

#else

void yuv422_to_yuv420(uint32_t *raw_yuv422_data_ptr, uint8_t *raw_y_ptr, uint8_t *raw_uv_ptr){
    int i, j;

    uint32_t tmp_u0 = 0;
    uint32_t tmp_u1 = 0;
    uint32_t tmp_v0 = 0;
    uint32_t tmp_v1 = 0;

    uint32_t yuv422 = 0;
    uint32_t yuv422_next_line = 0;

    for(i = 0; i < 16; i += 2){
        for(j = 0; j < 8; j++){
	    yuv422 = raw_yuv422_data_ptr[j];
	    yuv422_next_line = raw_yuv422_data_ptr[8 + j];

            *raw_y_ptr = (uint8_t)((yuv422 >> 8) & 0xFF);
            *(raw_y_ptr + 16) = (uint8_t)((yuv422_next_line >> 8) & 0xFF);
	    
	    raw_y_ptr++;

	    *raw_y_ptr = (uint8_t)(yuv422 >> 24);
	    *(raw_y_ptr + 16) = (uint8_t)(yuv422_next_line >> 24);

	    raw_y_ptr++;

            tmp_u0 = yuv422 & 0xFF;
	    tmp_u1 = yuv422_next_line & 0xFF;

	    tmp_v0  = (yuv422 >> 16) & 0xFF;
	    tmp_v1  = (yuv422_next_line >> 16) & 0xFF;

	    *(raw_uv_ptr) = (uint8_t)( (tmp_u0 + tmp_u1 + 1) >> 1 );
	    *(raw_uv_ptr + 8) = (uint8_t)( (tmp_v0 + tmp_v1 + 1) >>1 );

	    raw_uv_ptr++;       
        }

	raw_y_ptr += 16;
        raw_uv_ptr += 8;
	raw_yuv422_data_ptr += 16;
    }
}
#endif
//end of Y1V0Y0U0(31~0) 422 format

#else //V0Y1U0Y0(31~0) 422 format

#if 1
void yuv422_to_yuv420(uint32_t *raw_yuv422_data_ptr, uint8_t *raw_y_ptr, uint8_t *raw_uv_ptr){
    int i, j;

    for(i = 0; i < 16; i += 2){
        for(j = 0; j < 8; j += 4){
	    S32LDD(xr1, raw_yuv422_data_ptr, 0*4 );  
	    S32LDD(xr2, raw_yuv422_data_ptr, 1*4 );  
	    S32LDD(xr3, raw_yuv422_data_ptr, 2*4 );  
	    S32LDD(xr4, raw_yuv422_data_ptr, 3*4 );  

	    S32LDD(xr5, raw_yuv422_data_ptr, (8 + 0)*4 );  
	    S32LDD(xr6, raw_yuv422_data_ptr, (8 + 1)*4 );  
	    S32LDD(xr7, raw_yuv422_data_ptr, (8 + 2)*4 );  
	    S32LDD(xr8, raw_yuv422_data_ptr, (8 + 3)*4 );  

	    //S32SFL(xr9,  xr2, xr1, xr10, 1);
	    //S32SFL(xr11, xr4, xr3, xr12, 1);
	    S32SFL(xr10, xr2, xr1, xr9, 1);
	    S32SFL(xr12, xr4, xr3, xr11, 1);


	    S32STD(xr9,  raw_y_ptr, 0*4);
	    S32STD(xr11, raw_y_ptr, 1*4);


	    //S32SFL(xr9,  xr6, xr5, xr14, 1);
	    //S32SFL(xr11, xr8, xr7, xr15, 1);
	    S32SFL(xr14, xr6, xr5, xr9, 1);
	    S32SFL(xr15, xr8, xr7, xr11, 1);

	    S32STD(xr9,  raw_y_ptr, (4+0)*4);
	    S32STD(xr11, raw_y_ptr, (4+1)*4);

	    //uv
	    Q8AVGR(xr9,  xr10, xr14);
	    Q8AVGR(xr11, xr12, xr15);

	    S32SFL(xr2,  xr11, xr9, xr1, 1);

	    S32STD(xr1, raw_uv_ptr, 0);
	    S32STD(xr2, raw_uv_ptr, 8);

	    raw_y_ptr += 8;
	    raw_uv_ptr += 4;       
	    raw_yuv422_data_ptr += 4;
        }

	raw_y_ptr += 16;
        raw_uv_ptr += 8;
	raw_yuv422_data_ptr += 8;
    }
}

#else

void yuv422_to_yuv420(uint32_t *raw_yuv422_data_ptr, uint8_t *raw_y_ptr, uint8_t *raw_uv_ptr){
    int i, j;

    uint32_t tmp_u0 = 0;
    uint32_t tmp_u1 = 0;
    uint32_t tmp_v0 = 0;
    uint32_t tmp_v1 = 0;

    uint32_t yuv422 = 0;
    uint32_t yuv422_next_line = 0;

    for(i = 0; i < 16; i += 2){
        for(j = 0; j < 8; j++){
	    yuv422 = raw_yuv422_data_ptr[j];
	    yuv422_next_line = raw_yuv422_data_ptr[8 + j];

            *raw_y_ptr = (uint8_t)yuv422;
            *(raw_y_ptr + 16) = (uint8_t)yuv422_next_line;
	    
	    raw_y_ptr++;

	    *raw_y_ptr = (uint8_t)(yuv422 >> 16);
	    *(raw_y_ptr + 16) = (uint8_t)(yuv422_next_line >> 16);

	    raw_y_ptr++;

            tmp_u0 = (yuv422 >> 8)& 0xFF;
	    tmp_u1 = (yuv422_next_line >> 8)& 0xFF;

	    tmp_v0  = (yuv422 >> 24) & 0xFF;
	    tmp_v1  = (yuv422_next_line >> 24) & 0xFF;

	    *(raw_uv_ptr) = (uint8_t)( (tmp_u0 + tmp_u1 + 1) >> 1 );
	    *(raw_uv_ptr + 8) = (uint8_t)( (tmp_v0 + tmp_v1 + 1) >>1 );

	    raw_uv_ptr++;       
        }

	raw_y_ptr += 16;
        raw_uv_ptr += 8;
	raw_yuv422_data_ptr += 16;
    }
}
#endif
//end of V0Y1U0Y0(31~0) 422 format
#endif 
