/*****************************************************************************
 *
 * JZC MC HardWare Core Accelerate(H.264)
 *
* $Id: mpeg4_mc_hw.c,v 1.1 2010/10/25 12:07:05 yyang Exp $
 *
 ****************************************************************************/
#include "vp6_dcore.h"  
#include "jzmedia.h"
//#include "vp6_tcsm0.h"
//#include "vp6_tcsm1.h"
//#include "../vp6data.h"
extern VP6_Frame_GlbARGs *dFRM;
extern VP6_MB_DecARGs *dMB;

#define JZC_MC_DBG
#undef printf
#ifdef JZC_MC_DBG
extern short  mpFrame;
#define RECON_BUF_STRD 60
#define TCSM_MC_CHAIN0 0xF4000000
#define TCSM_MC_CHAIN1 0xF4001000
#define TCSM_YUVRECON_BUF 0xF4003000
#endif 
int mc_tag;

static int vp6_adjust(int v, int t)
{
    int V = v, s = v >> 31;
    V ^= s;
    V -= s;
    if (V-t-1 >= (unsigned)(t-1))
        return v;
    V = 2*t - V;
    V += s;
    V ^= s;
    return V;
}

static void vp56_edge_filter(uint8_t *yuv,
                             int pix_inc, int line_inc, int t)
{
    int pix2_inc = 2 * pix_inc;
    int i, v;

    for (i=0; i<12; i++) {
        v = (yuv[-pix2_inc] + 3*(yuv[0]-yuv[-pix_inc]) - yuv[pix_inc] + 4) >>3;
        v = vp6_adjust(v, t);
        yuv[-pix_inc] = av_clip_uint8(yuv[-pix_inc] + v);
        yuv[0] = av_clip_uint8(yuv[0] - v);
        yuv += line_inc;
    }
}
static void vp56_deblock_filter(uint8_t *yuv,
                                int stride, int dx, int dy)
{
    int t = vp56_filter_threshold[dFRM->quantizer];
    if (dx)  vp56_edge_filter(yuv +         10-dx ,      1, stride, t);
    if (dy)  vp56_edge_filter(yuv + stride*(10-dy), stride,      1, t);
}

int vp6_block_variance_aux(uint8_t *src, int stride)
{
    int sum = 0, square_sum = 0;
    int y, x;

    for (y=0; y<8; y+=2) {
        for (x=0; x<8; x+=2) {
            sum += src[x];
            square_sum += src[x]*src[x];
        }
        src += 2*stride;
    }
    return (16*square_sum - sum*sum) >> 8;
}

void vp6_filter_hv4_hw(uint8_t *dst, uint8_t *src, int stride, int src_stride,
                           int delta, const int16_t *weights)
{
  int x, y;
  for (y=0; y<8; y++) {
        for (x=0; x<8; x++) {
	  dst[x] = av_clip_uint8((  src[x-delta  ] * (weights[0])
				    + src[x        ] * (weights[1])
				    + src[x+delta  ] * (weights[2])
				    + src[x+2*delta] * (weights[3]) + 64) >> 7);
        }
	src += src_stride;
        dst += stride;
    }
}

void put_h264_chroma_mc8_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int src_stride, int h, int x, int y){
    const int A=(8-x)*(8-y);
    const int B=(  x)*(8-y);
    const int C=(8-x)*(  y);
    const int D=(  x)*(  y);
    int i;

    //assert(x<8 && y<8 && x>=0 && y>=0);

    for(i=0; i<h; i++)
    {
        dst[0] = (A*src[0] + B*src[1] + C*src[src_stride+0] + D*src[src_stride+1] + 32) >> 6;
        dst[1] = (A*src[1] + B*src[2] + C*src[src_stride+1] + D*src[src_stride+2] + 32) >> 6;
        dst[2] = (A*src[2] + B*src[3] + C*src[src_stride+2] + D*src[src_stride+3] + 32) >> 6;
        dst[3] = (A*src[3] + B*src[4] + C*src[src_stride+3] + D*src[src_stride+4] + 32) >> 6;
        dst[4] = (A*src[4] + B*src[5] + C*src[src_stride+4] + D*src[src_stride+5] + 32) >> 6;
        dst[5] = (A*src[5] + B*src[6] + C*src[src_stride+5] + D*src[src_stride+6] + 32) >> 6;
        dst[6] = (A*src[6] + B*src[7] + C*src[src_stride+6] + D*src[src_stride+7] + 32) >> 6;
        dst[7] = (A*src[7] + B*src[8] + C*src[src_stride+7] + D*src[src_stride+8] + 32) >> 6;
        dst+= stride;
        src+= src_stride;
    }
}

void vp6_filter_diag2_hw(uint8_t *dst, uint8_t *src,
                             int stride,  int src_stride,int h_weight, int v_weight)
{
  uint8_t *tmp = TCSM1_EMU_BUF + 16;
  put_h264_chroma_mc8_c(tmp, src, EMU_BUF_W, src_stride, 9, h_weight, 0);
  put_h264_chroma_mc8_c(dst, tmp, stride, EMU_BUF_W, 8, 0, v_weight);
}

void vp6_filter_diag4_hw(uint8_t *dst, uint8_t *src, int stride, int src_stride,
                             const int16_t *h_weights,const int16_t *v_weights)
{
    int x, y;
    int tmp[8*11];
    int *t = tmp;

    src -= src_stride;

    for (y=0; y<11; y++) {
        for (x=0; x<8; x++) {
            t[x] = av_clip_uint8((  src[x-1] * h_weights[0]
                               + src[x  ] * h_weights[1]
                               + src[x+1] * h_weights[2]
                               + src[x+2] * h_weights[3] + 64) >> 7);
        }
        src += src_stride;
        t += 8;
    }

    t = tmp + 8;
    for (y=0; y<8; y++) {
        for (x=0; x<8; x++) {
            dst[x] = av_clip_uint8((  t[x-8 ] * v_weights[0]
                                 + t[x   ] * v_weights[1]
                                 + t[x+8 ] * v_weights[2]
                                 + t[x+16] * v_weights[3] + 64) >> 7);
        }
        dst += stride;
        t += 8;
    }
}


void vp6_filter_hw(uint8_t *dst, uint8_t *src,
                       int offset1, int offset2, int stride, int src_stride,
                       int b, int mask, int select, int luma)
{
    int filter4 = 0;
    int x8 = dMB->mv[b][0] & mask;
    int y8 = dMB->mv[b][1] & mask;
    uint8_t *src_yuv;
    int i,j;
    if (luma) {
        x8 *= 2;
        y8 *= 2;
        filter4 = dFRM->filter_mode;
        if (filter4 == 2) {
            if (dFRM->max_vector_length &&
                (FFABS(dMB->mv[b][0]) > dFRM->max_vector_length ||
                 FFABS(dMB->mv[b][1]) > dFRM->max_vector_length)) {
                filter4 = 0;
            } else if (dFRM->sample_variance_threshold
                       && (vp6_block_variance_aux(src+offset1,src_stride)
                           < dFRM->sample_variance_threshold)) {
                filter4 = 0;
            }
        }
    }

    if ((y8 && (offset2-offset1)*dFRM->flip<0) || (!y8 && offset1 > offset2)) {
        offset1 = offset2;
    }

    if (filter4) {
        if (!y8) {                      /* left or right combine */
	  vp6_filter_hv4_hw(dst, src+offset1, stride, src_stride, 1,
	  		    vp6_block_copy_filter[select][x8]);

        } else if (!x8) {               /* above or below combine */
	  vp6_filter_hv4_hw(dst, src+offset1, stride,  src_stride, src_stride,
	                 vp6_block_copy_filter[select][y8]);

        } else {
            vp6_filter_diag4_hw(dst, src+offset1 + ((dMB->mv[b][0]^dMB->mv[b][1])>>31), stride,src_stride,
                             vp6_block_copy_filter[select][x8],
                             vp6_block_copy_filter[select][y8]);
        }
    } else {
        if (!x8 || !y8) {
            put_h264_chroma_mc8_c(dst, src+offset1, stride,src_stride, 8, x8, y8);
        } else {
            vp6_filter_diag2_hw(dst, src+offset1 + ((dMB->mv[b][0]^dMB->mv[b][1])>>31), stride, src_stride, x8, y8);
        }
    }
}  

void MC_put_o_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height)
 { do { dest[0] = (ref[0]);
        dest[1] = (ref[1]);
        dest[2] = (ref[2]);
        dest[3] = (ref[3]);
        dest[4] = (ref[4]);
        dest[5] = (ref[5]);
        dest[6] = (ref[6]); 
        dest[7] = (ref[7]);
        ref += stride;
        dest += stride;
       } while (--height);
 }

void put_no_rnd_pixels8_l2_c(uint8_t *dst, const uint8_t *src1, const uint8_t *src2,
			      int stride, int src_stride, int h){
    int i;
    for(i=0; i<h; i++){
        uint32_t a,b;
        a= (((const struct unaligned_32 *) (&src1[i*src_stride ]))->l);
        b= (((const struct unaligned_32 *) (&src2[i*src_stride ]))->l);
        *((uint32_t*)&dst[i*stride ]) = no_rnd_avg32(a, b);
        a= (((const struct unaligned_32 *) (&src1[i*src_stride+4]))->l);
        b= (((const struct unaligned_32 *) (&src2[i*src_stride+4]))->l);
        *((uint32_t*)&dst[i*stride+4]) = no_rnd_avg32(a, b);
    }
}

void MC_put_o_8_aux (uint8_t * dest, const uint8_t * ref, const int stride, int ref_stride, int height)
 { do { dest[0] = (ref[0]);
        dest[1] = (ref[1]);
        dest[2] = (ref[2]);
        dest[3] = (ref[3]);
        dest[4] = (ref[4]);
        dest[5] = (ref[5]);
        dest[6] = (ref[6]); 
        dest[7] = (ref[7]);
        ref += ref_stride;
        dest += stride;
       } while (--height);
 }

static void MC_put_o_16_aux (uint8_t * dest, const uint8_t * ref, int dst_stride, const int stride, int height)
 { do { dest[0] = (ref[0]); 
        dest[1] = (ref[1]);
        dest[2] = (ref[2]);
        dest[3] = (ref[3]);
        dest[4] = (ref[4]);
        dest[5] = (ref[5]);
        dest[6] = (ref[6]);
        dest[7] = (ref[7]);
        dest[8] = (ref[8]);
        dest[9] = (ref[9]);
        dest[10] = (ref[10]);
        dest[11] = (ref[11]);
        dest[12] = (ref[12]);
        dest[13] = (ref[13]);
        dest[14] = (ref[14]); 
        dest[15] = (ref[15]);
        ref += stride;
        dest += dst_stride;
      } while (--height); 
  }

void ff_emulated_edge_mc_vp6(uint8_t *buf, uint8_t *src,
			    int buf_linesize, int src_linesize, int block_w, int block_h,
			    int src_x, int src_y, int w, int h){
  int x, y;
  int start_y, start_x, end_y, end_x;

  if(src_y>= h){
    src+= (h-1-src_y)*src_linesize;
    src_y=h-1;
  }else if(src_y<=-block_h){
    src+= (1-block_h-src_y)*src_linesize;
    src_y=1-block_h;
  }
  if(src_x>= w){
    src+= (w-1-src_x);
    src_x=w-1;
  }else if(src_x<=-block_w){
    src+= (1-block_w-src_x);
    src_x=1-block_w;
  }

  start_y= FFMAX(0, -src_y);
  start_x= FFMAX(0, -src_x);
  end_y= FFMIN(block_h, h-src_y);
  end_x= FFMIN(block_w, w-src_x); 

  // copy existing part
  for(y=start_y; y<end_y; y++){
    uint32_t start = ((uint32_t)&src[start_x + y*src_linesize]) & (~31);
    for(x=start_x; x<end_x; x++){
      buf[x + y*buf_linesize]= src[x + y*src_linesize];
    }
  }

  //top
  for(y=0; y<start_y; y++){
    for(x=start_x; x<end_x; x++){
      buf[x + y*buf_linesize]= buf[x + start_y*buf_linesize];
    }
  }

  //bottom
  for(y=end_y; y<block_h; y++){
    for(x=start_x; x<end_x; x++){
      buf[x + y*buf_linesize]= buf[x + (end_y-1)*buf_linesize];
    }
  }

  for(y=0; y<block_h; y++){
    //left
    for(x=0; x<start_x; x++){
      buf[x + y*buf_linesize]= buf[start_x + y*buf_linesize];
    }

    //right
    for(x=end_x; x<block_w; x++){
      buf[x + y*buf_linesize]= buf[end_x - 1 + y*buf_linesize];
    }
  }
}
static void vp56_mc_hw(int b, int plane, uint8_t *src,
                    int stride, int x, int y)
{
    uint8_t *dst=dFRM->current_picture.ptr[plane]+dMB->block_offset[b];
    uint8_t *src_block;
    int src_offset;
    int overlap_offset = 0;
    int mask = dFRM->vp56_coord_div[b] - 1;
    int deblock_filtering = dFRM->deblock_filtering;
    int dx;
    int dy;
    uint8_t *src_yuv;
    int i,j;
    
    mc_tag = 0;
    uint8_t *emu_edge_buf;
    emu_edge_buf = TCSM1_EMU_BUF + b * EMU_BUF_W * 12;

    if (dFRM->skip_loop_filter >= AVDISCARD_ALL ||
      (dFRM->skip_loop_filter >= AVDISCARD_NONKEY
       && !dFRM->current_picture.key_frame))
        deblock_filtering = 0;

    dx = dMB->mv[b][0] / dFRM->vp56_coord_div[b];
    dy = dMB->mv[b][1] / dFRM->vp56_coord_div[b];

    if (b >= 4) {
        x /= 2;
        y /= 2;
    }
    x += dx - 2;
    y += dy - 2;

    if (x<0 || x+12>=dFRM->plane_width[plane] ||
        y<0 || y+12>=dFRM->plane_height[plane]) {
        ff_emulated_edge_mc_vp6(emu_edge_buf,
				src + dMB->block_offset[b] + (dy-2)*stride + (dx-2),
				EMU_BUF_W,stride, 12, 12, x, y,
				dFRM->plane_width[plane],
				dFRM->plane_height[plane]);
        src_block = emu_edge_buf;
        src_offset = 2 + 2*EMU_BUF_W;
        mc_tag = 1;
    } else if (deblock_filtering) {
        /* only need a 12x12 block, but there is no such dsp function, */
        /* so copy a 16x12 block */
      MC_put_o_16_aux(emu_edge_buf, src + dMB->block_offset[b] + (dy-2)*stride + (dx-2), EMU_BUF_W, stride, 12);
      src_block = emu_edge_buf;
      src_offset = 2 + 2*EMU_BUF_W;
      mc_tag = 1;
    } else {
      src_block = src;
      src_offset = dMB->block_offset[b] + dy*stride + dx;
      mc_tag = 0;
    }

    if (deblock_filtering)
      vp56_deblock_filter(src_block,  ((mc_tag)? EMU_BUF_W :stride),  dx&7, dy&7);
    
    if (dMB->mv[b][0] & mask)
      overlap_offset += (dMB->mv[b][0] > 0) ? 1 : -1;
    if (dMB->mv[b][1] & mask)
      overlap_offset += (dMB->mv[b][1] > 0) ?((mc_tag)? EMU_BUF_W :stride) : (-((mc_tag)? EMU_BUF_W :stride));
    
    if (overlap_offset) {
      if (dFRM->filter_flag){
	vp6_filter_hw(dst, src_block, src_offset, src_offset+overlap_offset,
		      stride,((mc_tag)? EMU_BUF_W :stride), b, mask, dFRM->filter_selection, b<4);
      }
      else
	put_no_rnd_pixels8_l2_c(dst, src_block+src_offset,
				src_block+src_offset+overlap_offset,
				stride,((mc_tag)? EMU_BUF_W :stride),8);
    } else {
      MC_put_o_8_aux(dst, src_block+src_offset, stride, ((mc_tag)? EMU_BUF_W :stride), 8);
    }
}

