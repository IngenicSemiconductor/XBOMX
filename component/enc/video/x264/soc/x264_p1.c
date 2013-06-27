#define __p1_main __attribute__ ((__section__ (".p1_main")))

#define P1_USE_PADDR

#include "config_jz_soc.h"

#include "jz47xx_aux.h"
#include "jzmedia.h"
#include "jzasm.h"
#include "tcsm1.h"


#include "jz4760e_2ddma_hw.h"

#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#define UNUSED __attribute__((unused))
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define NOINLINE __attribute__((noinline))
#define MAY_ALIAS __attribute__((may_alias))
#define x264_constant_p(x) __builtin_constant_p(x)
#else
#define UNUSED
#define ALWAYS_INLINE inline
#define NOINLINE
#define MAY_ALIAS
#define x264_constant_p(x) 0
#endif

typedef union { uint16_t i; uint8_t  c[2]; } MAY_ALIAS x264_union16_t;
typedef union { uint32_t i; uint16_t b[2]; uint8_t  c[4]; } MAY_ALIAS x264_union32_t;
typedef union { uint64_t i; uint32_t a[2]; uint16_t b[4]; uint8_t c[8]; } MAY_ALIAS x264_union64_t;
#define M16(src) (((x264_union16_t*)(src))->i)
#define M32(src) (((x264_union32_t*)(src))->i)
#define M64(src) (((x264_union64_t*)(src))->i)
#define CP16(dst,src) M16(dst) = M16(src)
#define CP32(dst,src) M32(dst) = M32(src)
#define CP64(dst,src) M64(dst) = M64(src)

enum slice_type_e
{
    SLICE_TYPE_P  = 0,
    SLICE_TYPE_B  = 1,
    SLICE_TYPE_I  = 2,
    SLICE_TYPE_SP = 3,
    SLICE_TYPE_SI = 4
};

enum macroblock_position_e
{
    MB_LEFT     = 0x01,
    MB_TOP      = 0x02,
    MB_TOPRIGHT = 0x04,
    MB_TOPLEFT  = 0x08,

    MB_PRIVATE  = 0x10,

    ALL_NEIGHBORS = 0xf,
};

enum intra16x16_pred_e
{
    I_PRED_16x16_V  = 0,
    I_PRED_16x16_H  = 1,
    I_PRED_16x16_DC = 2,
    I_PRED_16x16_P  = 3,

    I_PRED_16x16_DC_LEFT = 4,
    I_PRED_16x16_DC_TOP  = 5,
    I_PRED_16x16_DC_128  = 6,
};

enum intra_chroma_pred_e
{
    I_PRED_CHROMA_DC = 0,
    I_PRED_CHROMA_H  = 1,
    I_PRED_CHROMA_V  = 2,
    I_PRED_CHROMA_P  = 3,

    I_PRED_CHROMA_DC_LEFT = 4,
    I_PRED_CHROMA_DC_TOP  = 5,
    I_PRED_CHROMA_DC_128  = 6
};

/* Colorspace type
 * legacy only; nothing other than I420 is really supported. */
#define X264_CSP_MASK           0x00ff  /* */
#define X264_CSP_NONE           0x0000  /* Invalid mode     */
#define X264_CSP_I420           0x0001  /* yuv 4:2:0 planar */
#define X264_CSP_I422           0x0002  /* yuv 4:2:2 planar */
#define X264_CSP_I444           0x0003  /* yuv 4:4:4 planar */
#define X264_CSP_YV12           0x0004  /* yuv 4:2:0 planar */
#define X264_CSP_YUYV           0x0005  /* yuv 4:2:2 packed */
#define X264_CSP_RGB            0x0006  /* rgb 24bits       */
#define X264_CSP_BGR            0x0007  /* bgr 24bits       */
#define X264_CSP_BGRA           0x0008  /* bgr 32bits       */
#define X264_CSP_MAX            0x0009  /* end of list */
#define X264_CSP_VFLIP          0x1000  /* */


#include "x264_soc.c"

__p1_main void main()
{
    TCSM1 *tcsm1_ptr = (TCSM1 *)TCSM1_CPUBASE;//0xF4000000
    vmau_reg *vmau_reg_ptr = tcsm1_ptr->slice_var.vmau_reg_phy;

    S32I2M(xr16, 0x7);

    tcsm1_ptr->gp1_raw_data_index = 0;
    tcsm1_ptr->gp2_raw_data_index = 0;
    tcsm1_ptr->mce_raw_data_index = 0;
    tcsm1_ptr->vmau_raw_data_index = 0;

    tcsm1_ptr->mce_aux_fifo_index = 0;
    tcsm1_ptr->last_mce_aux_fifo_index = 0;
    tcsm1_ptr->vmau_aux_fifo_index = 0;

    tcsm1_ptr->gp0_aux_fifo_index = 0;
    tcsm1_ptr->gp1_aux_fifo_index = 0;

    tcsm1_ptr->i_gp1_mb_x = 0;
    tcsm1_ptr->i_gp1_mb_y = 0;

    int i_mb_x = 0;
    int i_mb_y = 0;
    int mb_xy = 0;
      
    if(X264_CSP_YUYV == tcsm1_ptr->slice_var.i_csp)
    {
        x264_macroblock_load_raw_yuv422_data( tcsm1_ptr );
        x264_macroblock_load_raw_yuv422_data( tcsm1_ptr );
	copy_raw_yuv420_data_from_tcsm1_to_sram(tcsm1_ptr);
    }else
    {
        x264_macroblock_load_raw_data( tcsm1_ptr, 0, 0 );
    }

    while( tcsm1_ptr->aux_fifo_write_num <= tcsm1_ptr->slice_var.i_last_mb )
    {
      //minus 1 because mce pred 1
#define AUX_FIFO_IS_WRITABLE while(tcsm1_ptr->aux_fifo_write_num - tcsm1_ptr->aux_fifo_read_num >= AUX_FIFO_DEP -2)
        AUX_FIFO_IS_WRITABLE;


	x264_macroblock_init_index_and_neighbour( tcsm1_ptr, i_mb_x, i_mb_y );
        mb_xy = i_mb_x + i_mb_y * tcsm1_ptr->slice_var.i_mb_width;

        i_mb_x++;
        if( i_mb_x == tcsm1_ptr->slice_var.i_mb_width )
        {
            i_mb_y++;
            i_mb_x = 0;
        }
  

	if(mb_xy < tcsm1_ptr->slice_var.i_last_mb){
	  if(X264_CSP_YUYV == tcsm1_ptr->slice_var.i_csp)
	  {
	      x264_macroblock_load_raw_yuv422_data( tcsm1_ptr );//the last one is not necessary
	      copy_raw_yuv420_data_from_tcsm1_to_sram( tcsm1_ptr );
	  }else
	  {
	      tcsm1_ptr->i_next_mb_x = i_mb_x;
	      tcsm1_ptr->i_next_mb_y = i_mb_y;

	      x264_macroblock_load_raw_data( tcsm1_ptr, tcsm1_ptr->i_next_mb_x, tcsm1_ptr->i_next_mb_y);
	  }

	}

	if( tcsm1_ptr->slice_var.i_slice_type == SLICE_TYPE_P){

	    ALIGNED_4(int16_t pskip_mv[2]);
	    ALIGNED_4(int16_t mvp[2]);
	    ALIGNED_4(int16_t mvd[2]);


	  if(mb_xy != 0 ){
	    while(tcsm1_ptr->mce_end_flag != MCE_END_FLAG );
	    tcsm1_ptr->mce_end_flag = 0;

	    ALIGNED_4( int16_t mv[2]);

	    int mv_tmp = tcsm1_ptr->mv_cost.all;
	    mv[0] = (int16_t)(int8_t)(mv_tmp & 0xff);
	    mv[1] = (int16_t)(int8_t)((mv_tmp >> 8) & 0xff);

	    X264_MB_VAR *last_mb = &tcsm1_ptr->mb_array[tcsm1_ptr->last_mce_aux_fifo_index];
	    UPDATE_FIFO_INDEX(tcsm1_ptr->last_mce_aux_fifo_index, AUX_FIFO_DEP);
	    	    
	    tcsm1_ptr->top_line_mv[last_mb->i_mb_x] = M32(mv);//the (most_topright - 1) process specially 

	    CP32( mvp, tcsm1_ptr->mv_var.mvp );

	    mvd[0] = mv[0] - mvp[0];
	    mvd[1] = mv[1] - mvp[1];

	    CP32( last_mb->mvd, mvd );

	    M32(tcsm1_ptr->left_mv) = M32(mv);

	  }


	    x264_mb_predict_mv_16x16_hw( tcsm1_ptr, pskip_mv );
	    CP32(tcsm1_ptr->mv_var.mvp, pskip_mv);

	    X264_MB_VAR *mb = &tcsm1_ptr->mb_array[tcsm1_ptr->mce_aux_fifo_index];
	    int i_neighbour = mb->i_neighbour;	    
	    if( !(i_neighbour & MB_LEFT) || !(i_neighbour & MB_TOP) )
	    {
		M32( pskip_mv ) = 0;
	    }
	    CP32( mb->pskip_mv, pskip_mv );

	    x264_mb_mce_config( tcsm1_ptr );
	    UPDATE_FIFO_INDEX(tcsm1_ptr->mce_aux_fifo_index, AUX_FIFO_DEP);

#ifndef DEBUG_MCE_PIPE
	  if(mb_xy == tcsm1_ptr->slice_var.i_last_mb ){
	    while(tcsm1_ptr->mce_end_flag != MCE_END_FLAG );
	    tcsm1_ptr->mce_end_flag = 0;

	    ALIGNED_4( int16_t mv[2]);

	    int mv_tmp = tcsm1_ptr->mv_cost.all;
	    mv[0] = (int16_t)(int8_t)(mv_tmp & 0xff);
	    mv[1] = (int16_t)(int8_t)((mv_tmp >> 8) & 0xff);

	    X264_MB_VAR *last_mb = &tcsm1_ptr->mb_array[tcsm1_ptr->last_mce_aux_fifo_index];
	    UPDATE_FIFO_INDEX(tcsm1_ptr->last_mce_aux_fifo_index, AUX_FIFO_DEP);
	    	    
	    tcsm1_ptr->top_line_mv[last_mb->i_mb_x] = M32(mv);//the (most_topright - 1) process specially 

	    CP32( mvp, tcsm1_ptr->mv_var.mvp );

	    mvd[0] = mv[0] - mvp[0];
	    mvd[1] = mv[1] - mvp[1];

	    CP32( last_mb->mvd, mvd );

	    M32(tcsm1_ptr->left_mv) = M32(mv);

	  }
#endif


#ifdef DEBUG_MCE_PIPE
	    if(mb->i_mb_xy == 0){
	        continue;
	    }
#endif
	}


	x264_macroblock_config_vmau( tcsm1_ptr, vmau_reg_ptr);

    }



    i_nop;  
    i_nop;    
    i_nop;      
    i_nop;  
    i_wait();
}
