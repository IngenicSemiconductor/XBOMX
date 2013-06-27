/*
 * H.26L/H.264/AVC/JVT/14496-10/... encoder/decoder
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * H.264 / AVC / MPEG4 part10 codec.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include "internal.h"
#include "dsputil.h"
#include "avcodec.h"
#include "mpegvideo.h"
#include "h264.h"
#include "h264data.h"
#include "h264_mvpred.h"
#include "h264_parser.h"
#include "golomb.h"
#include "mathops.h"
#include "rectangle.h"
#include "vdpau_internal.h"
#include "libmpdemux/stheader.h"

#include "utils/Log.h"
#define EL(x,y...) //{ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y);}
#include <sched.h>

#include "cabac.h"
extern volatile unsigned char *sde_base;
extern volatile unsigned char *tcsm0_base;
extern volatile unsigned char *tcsm1_base;
extern volatile unsigned char *sram_base;
extern volatile unsigned char *gp0_base;
extern volatile unsigned char *vpu_base;
extern volatile unsigned char *cpm_base;

//#undef NDEBUG
#include <assert.h>
#include"../libjzcommon/jzm_vpu.h"       
#include"jzm_h264_dec.h"  
#include"jzm_h264_dec.c"  


#ifdef JZC_CRC_VER       // need crc?
# undef   fprintf
# undef   printf
# include "../libjzcommon/crc.c"
short crc_code;
short mpFrame;
#endif

#include "../libjzcommon/jzasm.h"
#include "../libjzcommon/jzmedia.h"

//#define JZC_PMON_P0
#define STA_CCLK
#ifdef JZC_PMON_P0
#include "../libjzcommon/jz4760e_pmon.h"
FILE *pmon_p0_fp;
extern char* filename;
int pmon_p0_frmcnt = 0;
PMON_CREAT(fslice);
PMON_CREAT(fwait);
PMON_CREAT(fedge);
#endif //JZC_PMON_P0

#define ENABLE_GRAY 1
static const uint8_t rem6[52]={
0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3,
};

static const uint8_t div6[52]={
0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
};

#if 0
const uint8_t *ff_h264_decode_nal(H264Context *h, uint8_t *src, int *dst_length, int *consumed, int length){
    int i, si, di;
    uint8_t *dst;
    int bufidx;

//    src[0]&0x80;                //forbidden bit
    h->nal_ref_idc= src[0]>>5;
    h->nal_unit_type= src[0]&0x1F;

    src++; length--;
#if 0
    for(i=0; i<length; i++)
        printf("%2X ", src[i]);
#endif

#if HAVE_FAST_UNALIGNED
# if HAVE_FAST_64BIT
#   define RS 7
    for(i=0; i+1<length; i+=9){
        if(!((~AV_RN64A(src+i) & (AV_RN64A(src+i) - 0x0100010001000101ULL)) & 0x8000800080008080ULL))
# else
#   define RS 3
    for(i=0; i+1<length; i+=5){
        if(!((~AV_RN32A(src+i) & (AV_RN32A(src+i) - 0x01000101U)) & 0x80008080U))
# endif
            continue;
        if(i>0 && !src[i]) i--;
        while(src[i]) i++;
#else
#   define RS 0
    for(i=0; i+1<length; i+=2){
        if(src[i]) continue;
        if(i>0 && src[i-1]==0) i--;
#endif
        if(i+2<length && src[i+1]==0 && src[i+2]<=3){
            if(src[i+2]!=3){
                /* startcode, so we must be past the end */
                length=i;
            }
            break;
        }
        i-= RS;
    }

    if(i>=length-1){ //no escaped 0
        *dst_length= length;
        *consumed= length+1; //+1 for the header
        return src;
    }

    bufidx = h->nal_unit_type == NAL_DPC ? 1 : 0; // use second escape buffer for inter data
    av_fast_malloc(&h->rbsp_buffer[bufidx], &h->rbsp_buffer_size[bufidx], length+FF_INPUT_BUFFER_PADDING_SIZE);
    dst= h->rbsp_buffer[bufidx];

    if (dst == NULL){
        return NULL;
    }

//printf("decoding esc\n");
    memcpy(dst, src, i);
    si=di=i;
    while(si+2<length){
        //remove escapes (very rare 1:2^22)
        if(src[si+2]>3){
            dst[di++]= src[si++];
            dst[di++]= src[si++];
        }else if(src[si]==0 && src[si+1]==0){
            if(src[si+2]==3){ //escape
                dst[di++]= 0;
                dst[di++]= 0;
                si+=3;
                continue;
            }else //next start code
                goto nsc;
        }

        dst[di++]= src[si++];
    }
    while(si<length)
        dst[di++]= src[si++];
nsc:

    memset(dst+di, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    *dst_length= di;
    *consumed= si + 1;//+1 for the header
//FIXME store exact number of bits in the getbitcontext (it is needed for decoding)
    return dst;
}
#else
const uint8_t *ff_h264_decode_nal(H264Context *h, uint8_t *src, int *dst_length, int *consumed, int length){
    int i, si, di;
    uint8_t *dst;
    int bufidx;

//    src[0]&0x80;                //forbidden bit
    h->nal_ref_idc= src[0]>>5;
    h->nal_unit_type= src[0]&0x1F;

    src++; length--;

#define ESC_BUFFER_NUM 500
    int esc_index[ESC_BUFFER_NUM];
    int esc_len[ESC_BUFFER_NUM];    
    int esc_count = 0;
    
    //si = i;
    si = i = 0;

    int j = 0;
    int len = 0;

    //int max_len = i + 2;
    int max_len = 0;
    int low_num = 0;
    int high_num = 0;
    
    while(si+2<length){
      if(src[si+2]>3){
        si += 3;
	//continue;
      }else if(src[si]==0 && src[si+1]==0){
          if(src[si+2]==3){ //escape
            si+=2;
	    if(0 == esc_count){
	      esc_len[0] = si;
	      max_len = si;
	    }
            esc_index[esc_count++] = si++;    
	    if(esc_count == ESC_BUFFER_NUM){
	      *dst_length = -1;
	      printf("ERROR: ESC_BUFFER_NUM OVERFLOW! escapes this frame\n");
	      return NULL;
	    }
            continue;
          }else{ //next start code
    	      esc_index[esc_count] = si + 2;    
	      length=si;
              goto next_start_code;
	  }
      }else{
          si += 1;
      }
    }

    esc_index[esc_count] = length + 1;    
    si = length;

next_start_code:
      
    for(j = 0; j < esc_count; ++j){
      len = esc_index[j+1] - esc_index[j] - 1;
      esc_len[j + 1] = len;

      if(max_len < len){
     	max_len = len;
        low_num = j + 1;
      }
    }    

    //printf("length: %d, max_len: %d, low_num: %d, esc_count: %d\n", length, max_len, low_num, esc_count);
    
    if(low_num > 0){
      int ii = 0;
      uint8_t *source, *dest;

      di = esc_index[low_num -1];
      for(j = low_num -1; j >=0; --j){	
        ii++;
        len = esc_len[j];
	di = esc_index[low_num - ii] - len;
	source = src + di;
	dest = src + di + ii;	
	memmove(dest, source, len);
      }
    }
      
    high_num = esc_count - low_num;
    if(high_num > 0){
      di = esc_index[low_num];
      for(j = 0; j < high_num; ++j){
        len = esc_len[low_num + j + 1];
	memcpy(&src[di], &src[di + 1 + j], len);
	di += len;
      }
    }

    *dst_length = si - esc_count;
    *consumed = si + 1;//+1 for the header
    return src += low_num;    
}
#endif

int ff_h264_decode_rbsp_trailing(H264Context *h, const uint8_t *src){
    int v= *src;
    int r;

    tprintf(h->s.avctx, "rbsp trailing %X\n", v);

    for(r=1; r<9; r++){
        if(v&1) return r;
        v>>=1;
    }
    return 0;
}

static void free_tables(H264Context *h){
    int i;
    H264Context *hx;
    av_freep(&h->intra4x4_pred_mode);
    av_freep(&h->intra4x4_pred_mode_hw);
    av_freep(&h->chroma_pred_mode_table);
    av_freep(&h->cbp_table);
    av_freep(&h->mvd_table[0]);
    av_freep(&h->mvd_table[1]);
    av_freep(&h->direct_table);
    av_freep(&h->non_zero_count);
    av_freep(&h->slice_table_base);
    h->slice_table= NULL;
    av_freep(&h->hw_slice_table_base);
    h->hw_slice_table= NULL;
    av_freep(&h->list_counts);

    av_freep(&h->mb2b_xy);
    av_freep(&h->mb2br_xy);
    av_freep(&h->mb2b8_xy);

    for(i = 0; i < MAX_THREADS; i++) {
        hx = h->thread_context[i];
        if(!hx) continue;
        av_freep(&hx->top_borders[1]);
        av_freep(&hx->top_borders[0]);
        av_freep(&hx->s.obmc_scratchpad);
        av_freep(&hx->rbsp_buffer[1]);
        av_freep(&hx->rbsp_buffer[0]);
        hx->rbsp_buffer_size[0] = 0;
        hx->rbsp_buffer_size[1] = 0;
        if (i) av_freep(&h->thread_context[i]);
    }
}

static void init_dequant8_coeff_table(H264Context *h){
    int i,q,x,i_list;
    const int transpose = (h->h264dsp.h264_idct8_add != ff_h264_idct8_add_c); //FIXME ugly
    jzm_h264 *st_h264 = (jzm_h264 *)(h->st_h264);

    //const int transpose = 1; //FIXME ugly
    h->dequant8_coeff[0] = h->dequant8_buffer[0];
    h->dequant8_coeff[1] = h->dequant8_buffer[1];
    for( i_list = 0; i_list < 2; i_list++ ) {
      for( i=0; i<64;i++) {
	st_h264->scaling_matrix8[i_list][i] = h->pps.scaling_matrix8[i_list][transpose ? (i>>3)|((i&7)<<3) : i];
      }
    }

    for(i=0; i<2; i++ ){
        if(i && !memcmp(h->pps.scaling_matrix8[0], h->pps.scaling_matrix8[1], 64*sizeof(uint8_t))){
            h->dequant8_coeff[1] = h->dequant8_buffer[0];
            break;
        }

        for(q=0; q<52; q++){
	  int shift = div6[q];
	  int idx = rem6[q];
	  for(x=0; x<64; x++)
	    h->dequant8_coeff[i][q][transpose ? (x>>3)|((x&7)<<3) : x] =
	      ((uint32_t)dequant8_coeff_init[idx][ dequant8_coeff_init_scan[((x>>1)&12) | (x&3)] ] *
	       h->pps.scaling_matrix8[i][x]) << shift;
        }
    }
}

static void init_dequant4_coeff_table(H264Context *h){
    int i,j,q,x;
    const int transpose = (h->h264dsp.h264_idct_add != ff_h264_idct_add_c); //FIXME ugly
    jzm_h264 *st_h264 = (jzm_h264 *)(h->st_h264);
    //const int transpose = 1; //FIXME ugly
    int ii;
    int i_list;
    int tab_tmp[6] = {0, 3, 1, 4, 2, 5};
    for( ii = 0; ii < 6; ii++ ) {
      int i_list = tab_tmp[ii];
      for( i=0; i<16;i++) {
	st_h264->scaling_matrix4[ii][i] = h->pps.scaling_matrix4[i_list][transpose ? (i>>2)|((i<<2)&0xF) : i];		  
      }
    }

    for(i=0; i<6; i++ ){
        h->dequant4_coeff[i] = h->dequant4_buffer[i];
        for(j=0; j<i; j++){
            if(!memcmp(h->pps.scaling_matrix4[j], h->pps.scaling_matrix4[i], 16*sizeof(uint8_t))){
                h->dequant4_coeff[i] = h->dequant4_buffer[j];
                break;
            }
        }
        if(j<i)
            continue;

        for(q=0; q<52; q++){
            int shift = div6[q] + 2;
            int idx = rem6[q];
            for(x=0; x<16; x++)
                h->dequant4_coeff[i][q][transpose ? (x>>2)|((x<<2)&0xF) : x] =
                    ((uint32_t)dequant4_coeff_init[idx][(x&1) + ((x>>2)&1)] *
                    h->pps.scaling_matrix4[i][x]) << shift;
        }
    }
}

static void init_dequant_tables(H264Context *h){
    int i,x;
    init_dequant4_coeff_table(h);
    if(h->pps.transform_8x8_mode)
        init_dequant8_coeff_table(h);
    if(h->sps.transform_bypass){
        for(i=0; i<6; i++)
            for(x=0; x<16; x++)
                h->dequant4_coeff[i][0][x] = 1<<6;
        if(h->pps.transform_8x8_mode)
            for(i=0; i<2; i++)
                for(x=0; x<64; x++)
                    h->dequant8_coeff[i][0][x] = 1<<6;
    }
}


int ff_h264_alloc_tables(H264Context *h){
    MpegEncContext * const s = &h->s;
    const int big_mb_num= s->mb_stride * (s->mb_height+1);
    const int row_mb_num= 2*s->mb_stride*s->avctx->thread_count;
    int x,y;

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->intra4x4_pred_mode, row_mb_num * 8  * sizeof(uint8_t), fail)
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->intra4x4_pred_mode_hw, big_mb_num * 8  * sizeof(uint8_t), fail)

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->non_zero_count    , big_mb_num * 32 * sizeof(uint8_t), fail)
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->slice_table_base  , (big_mb_num+s->mb_stride) * sizeof(*h->slice_table_base), fail)
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->cbp_table, big_mb_num * sizeof(uint16_t), fail)

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->chroma_pred_mode_table, big_mb_num * sizeof(uint8_t), fail)
#if 0
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mvd_table[0], 16*row_mb_num * sizeof(uint8_t), fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mvd_table[1], 16*row_mb_num * sizeof(uint8_t), fail);
#else
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mvd_table[0], 32*big_mb_num * sizeof(uint16_t), fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mvd_table[1], 32*big_mb_num * sizeof(uint16_t), fail);
#endif
    
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->hw_slice_table_base, (big_mb_num+s->mb_stride) * sizeof(*h->slice_table_base), fail)

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->direct_table, 4*big_mb_num * sizeof(uint8_t) , fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->list_counts, big_mb_num * sizeof(uint8_t), fail)

    memset(h->slice_table_base, -1, (big_mb_num+s->mb_stride)  * sizeof(*h->slice_table_base));
    h->slice_table= h->slice_table_base + s->mb_stride*2 + 1;

    memset(h->hw_slice_table_base, -1, (big_mb_num+s->mb_stride)  * sizeof(uint8_t));
    h->hw_slice_table= h->slice_table_base + s->mb_stride*2 + 1;

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mb2b_xy  , big_mb_num * sizeof(uint32_t), fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mb2br_xy , big_mb_num * sizeof(uint32_t), fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mb2b8_xy , big_mb_num * sizeof(uint32_t), fail);
    for(y=0; y<s->mb_height; y++){
        for(x=0; x<s->mb_width; x++){
            const int mb_xy= x + y*s->mb_stride;
            const int b_xy = 4*x + 4*y*h->b_stride;
            const int b8_xy= 2*x + 2*y*h->b8_stride;
            h->mb2b_xy [mb_xy]= b_xy;
            h->mb2br_xy[mb_xy]= 8*(FMO ? mb_xy : (mb_xy % (2*s->mb_stride)));
            h->mb2b8_xy[mb_xy]= b8_xy;
        }
    }

    s->obmc_scratchpad = NULL;

    if(!h->dequant4_coeff[0])
        init_dequant_tables(h);

    return 0;
fail:
    free_tables(h);
    return -1;
}

/**
 * Mimic alloc_tables(), but for every context thread.
 */
static void clone_tables(H264Context *dst, H264Context *src, int i){
    MpegEncContext * const s = &src->s;
    dst->intra4x4_pred_mode       = src->intra4x4_pred_mode + i*8*2*s->mb_stride;
    dst->intra4x4_pred_mode_hw    = src->intra4x4_pred_mode_hw;
    dst->non_zero_count           = src->non_zero_count;
    dst->slice_table              = src->slice_table;
    dst->hw_slice_table           = src->hw_slice_table;
    dst->cbp_table                = src->cbp_table;
    dst->mb2b_xy                  = src->mb2b_xy;
    dst->mb2br_xy                 = src->mb2br_xy;
    dst->mb2b8_xy                 = src->mb2b8_xy;
    dst->chroma_pred_mode_table   = src->chroma_pred_mode_table;
    dst->mvd_table[0]             = src->mvd_table[0];
    dst->mvd_table[1]             = src->mvd_table[1];
    dst->direct_table             = src->direct_table;
    dst->list_counts              = src->list_counts;

    dst->s.obmc_scratchpad = NULL;
    ff_h264_pred_init(&dst->hpc, src->s.codec_id);
}

/**
 * Init context
 * Allocate buffers which are not shared amongst multiple threads.
 */
static int context_init(H264Context *h){
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->top_borders[0], h->s.mb_width * (16+8+8) * sizeof(uint8_t), fail)
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->top_borders[1], h->s.mb_width * (16+8+8) * sizeof(uint8_t), fail)
    return 0;
fail:
    return -1; // free_tables will clean up for us
}

 static int decode_nal_units(H264Context *h, const uint8_t *buf, int buf_size, int extra);

static av_cold void common_init(H264Context *h){

   
    MpegEncContext * const s = &h->s;

    s->width = s->avctx->width;
    s->height = s->avctx->height;
    s->codec_id= s->avctx->codec->id;

    ff_h264dsp_init(&h->h264dsp);
    ff_h264_pred_init(&h->hpc, s->codec_id);

    h->dequant_coeff_pps= -1;
    s->unrestricted_mv=1;
    s->decode=1; //FIXME

    dsputil_init(&s->dsp, s->avctx); // needed so that idct permutation is known early

    memset(h->pps.scaling_matrix4, 16, 6*16*sizeof(uint8_t));
    memset(h->pps.scaling_matrix8, 16, 2*64*sizeof(uint8_t));
}


 av_cold int ff_h264_decode_init(AVCodecContext *avctx){
   H264Context *h= avctx->priv_data;
   MpegEncContext * const s = &h->s;
   
   EL("[%s] --------> ff_h264_decode_init <-------", __FUNCTION__);
  
   avctx->is_dechw = 1;
#ifdef JZC_CRC_VER
   crc_code = 0;
   mpFrame = 0;
#endif //JZC_CRC_VER

   S32I2M(xr16,0x3);
 
   h->st_h264 = jz4740_alloc_frame(avctx->VpuMem_ptr, 32, sizeof(jzm_h264));
   jzm_h264 *st_h264 = (jzm_h264 *)(h->st_h264);
   st_h264->des_va = jz4740_alloc_frame(avctx->VpuMem_ptr, 128, 0x5000); 
   st_h264->des_pa = st_h264->des_va;    
  
   avctx->use_jz_buf = 1;
   MPV_decode_defaults(s);

   s->avctx = avctx;
   common_init(h);

   s->out_format = FMT_H264;
   s->workaround_bugs= avctx->workaround_bugs;
   // set defaults
   // s->decode_mb= ff_h263_decode_mb;
   s->quarter_sample = 1;
   if(!avctx->has_b_frames)
     s->low_delay= 1;

   avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;

   ff_h264_decode_init_vlc();

   h->thread_context[0] = h;
   h->outputed_poc = INT_MIN;
   h->prev_poc_msb= 1<<16;
   //h->x264_build = -1;
   h->x264_build = 0;
   ff_h264_reset_sei(h);
   if(avctx->codec_id == CODEC_ID_H264){
     if(avctx->ticks_per_frame == 1){
       s->avctx->time_base.den *=2;
     }
     avctx->ticks_per_frame = 2;
   }

   ////for android PREFETCHER_DEPACK_NAL
   if(avctx->extradata_size > 0 && avctx->extradata){
       if(*(char *)avctx->extradata == 1){
	   int i, cnt, nalsize;
	   unsigned char *p = avctx->extradata;
	   h->is_avc = 1;
	   if(avctx->extradata_size < 7) {
	       av_log(avctx, AV_LOG_ERROR, "avcC too short\n");
	       return -1;
	   }
	   /* sps and pps in the avcC always have length coded with 2 bytes,
	      so put a fake nal_length_size = 2 while parsing them */
	   h->nal_length_size = 2;
	   // Decode sps from avcC
	   cnt = *(p+5) & 0x1f; // Number of sps
	   p += 6;
	   for (i = 0; i < cnt; i++) {
	       nalsize = AV_RB16(p) + 2;
	       if(decode_nal_units(h, p, nalsize,1) < 0) {
		   av_log(avctx, AV_LOG_ERROR, "Decoding sps %d from avcC failed\n", i);
		   return -1;
	       }
	       p += nalsize;
	   }
	   // Decode pps from avcC
	   cnt = *(p++); // Number of pps
	   for (i = 0; i < cnt; i++) {
	       nalsize = AV_RB16(p) + 2;
	       if(decode_nal_units(h, p, nalsize,1)  != nalsize) {
		   av_log(avctx, AV_LOG_ERROR, "Decoding pps %d from avcC failed\n", i);
		   return -1;
	       }
	       p += nalsize;
	   }
	   // Now store right nal length size, that will be use to parse all other nals
	   h->nal_length_size = ((*(((char*)(avctx->extradata))+4))&0x03)+1;
	   avctx->nal_length_size = h->nal_length_size;
	   avctx->is_avc = 1;

	   /*for rtsp : no nalsize bytes in nal units though there have extradata in file header*/
	   if(((sh_video_t *)(avctx->opaque))->is_rtsp == 1)
	       h->is_avc = 0;

       } else {
	   h->is_avc = 0;
	   h->nal_length_size=4;
	   avctx->nal_length_size = 4;
	   avctx->is_avc = 0;
	   if(decode_nal_units(h, s->avctx->extradata, s->avctx->extradata_size,1) < 0)
	       return -1;

	   if(avctx->is_flash == 0){

#ifdef PREFETCHER_DEPACK_NAL
	       if(((sh_video_t *)(avctx->opaque))->is_rtsp != 1)
		   h->is_avc = 1;//prefetcher depack nal to is_avc format
#endif
	   }
       }
   }
   else{
       avctx->nal_length_size = 4;
       avctx->is_avc = 0;
       h->nal_length_size=4;

       if(avctx->is_flash == 0){

#ifdef PREFETCHER_DEPACK_NAL
	   if(((sh_video_t *)(avctx->opaque))->is_rtsp != 1)
	       h->is_avc = 1;//prefetcher depack nal to is_avc format
#endif
       }
   }
   ////for android PREFETCHER_DEPACK_NAL...

   if(h->sps.bitstream_restriction_flag && s->avctx->has_b_frames < h->sps.num_reorder_frames){
     s->avctx->has_b_frames = h->sps.num_reorder_frames;
     s->low_delay = 0;
   }

   EL("[%s] --------> ff_h264_decode_init end <-------", __FUNCTION__);
   return 0;
 }

int ff_h264_frame_start(H264Context *h){
    MpegEncContext * const s = &h->s;
    int i;

    
    if(MPV_frame_start(s, s->avctx) < 0)
        return -1;
  
    ff_er_frame_start(s);
  
    /*
     * MPV_frame_start uses pict_type to derive key_frame.
     * This is incorrect for H.264; IDR markings must be used.
     * Zero here; IDR markings per slice in frame or fields are ORed in later.
     * See decode_nal_units().
     */
    s->current_picture_ptr->key_frame= 0;
    s->current_picture_ptr->mmco_reset= 0;

    assert(s->linesize && s->uvlinesize);

    for(i=0; i<16; i++){
        h->block_offset[i]= 4*((scan8[i] - scan8[0])&7) + 4*s->linesize*((scan8[i] - scan8[0])>>3);
        h->block_offset[24+i]= 4*((scan8[i] - scan8[0])&7) + 8*s->linesize*((scan8[i] - scan8[0])>>3);
    }
    for(i=0; i<4; i++){
        h->block_offset[16+i]=
        h->block_offset[20+i]= 4*((scan8[i] - scan8[0])&7) + 4*s->uvlinesize*((scan8[i] - scan8[0])>>3);
        h->block_offset[24+16+i]=
        h->block_offset[24+20+i]= 4*((scan8[i] - scan8[0])&7) + 8*s->uvlinesize*((scan8[i] - scan8[0])>>3);
    }

    /* can't be in alloc_tables because linesize isn't known there.
     * FIXME: redo bipred weight to not require extra buffer? */
    for(i = 0; i < s->avctx->thread_count; i++)
        if(!h->thread_context[i]->s.obmc_scratchpad)
            h->thread_context[i]->s.obmc_scratchpad = av_malloc(16*2*s->linesize + 8*2*s->uvlinesize);

    /* some macroblocks can be accessed before they're available in case of lost slices, mbaff or threading*/
    memset(h->slice_table, -1, (s->mb_height*s->mb_stride-1) * sizeof(*h->slice_table));

    memset(h->hw_slice_table, -1, (s->mb_height*s->mb_stride-1) * sizeof(uint8_t));

//    s->decode= (s->flags&CODEC_FLAG_PSNR) || !s->encoding || s->current_picture.reference /*|| h->contains_intra*/ || 1;

    // We mark the current picture as non-reference after allocating it, so
    // that if we break out due to an error it can be released automatically
    // in the next MPV_frame_start().
    // SVQ3 as well as most other codecs have only last/next/current and thus
    // get released even with set reference, besides SVQ3 and others do not
    // mark frames as reference later "naturally".
    if(s->codec_id != CODEC_ID_SVQ3)
        s->current_picture_ptr->reference= 0;

    s->current_picture_ptr->field_poc[0]=
    s->current_picture_ptr->field_poc[1]= INT_MAX;
    assert(s->current_picture_ptr->long_ref==0);

    return 0;
}

static int pred_weight_table(H264Context *h){
    MpegEncContext * const s = &h->s;
    int list, i;
    int luma_def, chroma_def;

    h->use_weight= 0;
    h->use_weight_chroma= 0;
    h->luma_log2_weight_denom= get_ue_golomb(&s->gb);
    h->chroma_log2_weight_denom= get_ue_golomb(&s->gb);
    luma_def = 1<<h->luma_log2_weight_denom;
    chroma_def = 1<<h->chroma_log2_weight_denom;

    for(list=0; list<2; list++){
        h->luma_weight_flag[list]   = 0;
        h->chroma_weight_flag[list] = 0;
        for(i=0; i<h->ref_count[list]; i++){
            int luma_weight_flag, chroma_weight_flag;

            luma_weight_flag= get_bits1(&s->gb);
	    // printf("luma_weight_flag=%d\n",luma_weight_flag);
            if(luma_weight_flag){
                h->luma_weight[i][list][0]= get_se_golomb(&s->gb);
		//printf("h->luma_weight[%d][%d][0]=0x%x\n",i,list,h->luma_weight[i][list][0]);
                h->luma_weight[i][list][1]= get_se_golomb(&s->gb);
		//	printf("h->luma_weight[%d][%d][1]=0x%x\n",i,list,h->luma_weight[i][list][1]);
                if(   h->luma_weight[i][list][0] != luma_def
                   || h->luma_weight[i][list][1] != 0) {
                    h->use_weight= 1;
                    h->luma_weight_flag[list]= 1;
                }
            }else{
	      //  printf("+++\n");
	      if (luma_def > 127) luma_def = 127;
                h->luma_weight[i][list][0]= luma_def;
		//		printf("h->luma_weight[%d][%d][0]=%x\n",i,list,h->luma_weight[i][list][0]);
                h->luma_weight[i][list][1]= 0;
		//		printf("h->luma_weight[%d][%d][1]=%x\n",i,list,h->luma_weight[i][list][1]);
            }

            if(CHROMA){
                chroma_weight_flag= get_bits1(&s->gb);
                if(chroma_weight_flag){
                    int j;
                    for(j=0; j<2; j++){
                        h->chroma_weight[i][list][j][0]= get_se_golomb(&s->gb);
                        h->chroma_weight[i][list][j][1]= get_se_golomb(&s->gb);
                        if(   h->chroma_weight[i][list][j][0] != chroma_def
                           || h->chroma_weight[i][list][j][1] != 0) {
                            h->use_weight_chroma= 1;
                            h->chroma_weight_flag[list]= 1;
                        }
                    }
                }else{
                    int j;
                    for(j=0; j<2; j++){
                        if (chroma_def > 127) chroma_def = 127;
                        h->chroma_weight[i][list][j][0]= chroma_def;
                        h->chroma_weight[i][list][j][1]= 0;
                    }
                }
            }
        }
        if(h->slice_type_nos != FF_B_TYPE) break;
    }
    h->use_weight= h->use_weight || h->use_weight_chroma;
    return 0;
}

/**
 * Initialize implicit_weight table.
 * @param field, 0/1 initialize the weight for interlaced MBAFF
 *                -1 initializes the rest
 */
static void implicit_weight_table(H264Context *h, int field){
    MpegEncContext * const s = &h->s;
    int ref0, ref1, i, cur_poc, ref_start, ref_count0, ref_count1;

    for (i = 0; i < 2; i++) {
        h->luma_weight_flag[i]   = 0;
        h->chroma_weight_flag[i] = 0;
    }

    if(field < 0){
        cur_poc = s->current_picture_ptr->poc;
    if(   h->ref_count[0] == 1 && h->ref_count[1] == 1 && !FRAME_MBAFF
       && h->ref_list[0][0].poc + h->ref_list[1][0].poc == 2*cur_poc){
        h->use_weight= 0;
        h->use_weight_chroma= 0;
        return;
    }
        ref_start= 0;
        ref_count0= h->ref_count[0];
        ref_count1= h->ref_count[1];
    }else{
        cur_poc = s->current_picture_ptr->field_poc[field];
        ref_start= 16;
        ref_count0= 16+2*h->ref_count[0];
        ref_count1= 16+2*h->ref_count[1];
    }

    h->use_weight= 2;
    h->use_weight_chroma= 2;
    h->luma_log2_weight_denom= 5;
    h->chroma_log2_weight_denom= 5;

    for(ref0=ref_start; ref0 < ref_count0; ref0++){
        int poc0 = h->ref_list[0][ref0].poc;
        for(ref1=ref_start; ref1 < ref_count1; ref1++){
            int poc1 = h->ref_list[1][ref1].poc;
            int td = av_clip(poc1 - poc0, -128, 127);
            int w= 32;
            if(td){
                int tb = av_clip(cur_poc - poc0, -128, 127);
                int tx = (16384 + (FFABS(td) >> 1)) / td;
                int dist_scale_factor = (tb*tx + 32) >> 8;
                if(dist_scale_factor >= -64 && dist_scale_factor <= 128)
                    w = 64 - dist_scale_factor;
            }
            if(field<0){
                h->implicit_weight[ref0][ref1][0]=
                h->implicit_weight[ref0][ref1][1]= w;
            }else{
                h->implicit_weight[ref0][ref1][field]=w;
            }
        }
    }
}

/**
 * instantaneous decoder refresh.
 */
static void idr(H264Context *h){
    ff_h264_remove_all_refs(h);
    h->prev_frame_num= 0;
    h->prev_frame_num_offset= 0;
    h->prev_poc_msb=
    h->prev_poc_lsb= 0;
}

/* forget old pics after a seek */
static void flush_dpb(AVCodecContext *avctx){
    H264Context *h= avctx->priv_data;
    int i;
    for(i=0; i<MAX_DELAYED_PIC_COUNT; i++) {
        if(h->delayed_pic[i])
            h->delayed_pic[i]->reference= 0;
        h->delayed_pic[i]= NULL;
    }
    h->outputed_poc= INT_MIN;
    h->prev_interlaced_frame = 1;
    idr(h);
    if(h->s.current_picture_ptr)
        h->s.current_picture_ptr->reference= 0;
    h->s.first_field= 0;
    ff_h264_reset_sei(h);
    ff_mpeg_flush(avctx);
}

static int init_poc(H264Context *h){
    MpegEncContext * const s = &h->s;
    const int max_frame_num= 1<<h->sps.log2_max_frame_num;
    int field_poc[2];
    Picture *cur = s->current_picture_ptr;

    h->frame_num_offset= h->prev_frame_num_offset;
    if(h->frame_num < h->prev_frame_num)
        h->frame_num_offset += max_frame_num;

    if(h->sps.poc_type==0){
        const int max_poc_lsb= 1<<h->sps.log2_max_poc_lsb;

        if     (h->poc_lsb < h->prev_poc_lsb && h->prev_poc_lsb - h->poc_lsb >= max_poc_lsb/2)
            h->poc_msb = h->prev_poc_msb + max_poc_lsb;
        else if(h->poc_lsb > h->prev_poc_lsb && h->prev_poc_lsb - h->poc_lsb < -max_poc_lsb/2)
            h->poc_msb = h->prev_poc_msb - max_poc_lsb;
        else
            h->poc_msb = h->prev_poc_msb;
//printf("poc: %d %d\n", h->poc_msb, h->poc_lsb);
        field_poc[0] =
        field_poc[1] = h->poc_msb + h->poc_lsb;
        if(s->picture_structure == PICT_FRAME)
            field_poc[1] += h->delta_poc_bottom;
    }else if(h->sps.poc_type==1){
        int abs_frame_num, expected_delta_per_poc_cycle, expectedpoc;
        int i;

        if(h->sps.poc_cycle_length != 0)
            abs_frame_num = h->frame_num_offset + h->frame_num;
        else
            abs_frame_num = 0;

        if(h->nal_ref_idc==0 && abs_frame_num > 0)
            abs_frame_num--;

        expected_delta_per_poc_cycle = 0;
        for(i=0; i < h->sps.poc_cycle_length; i++)
            expected_delta_per_poc_cycle += h->sps.offset_for_ref_frame[ i ]; //FIXME integrate during sps parse

        if(abs_frame_num > 0){
            int poc_cycle_cnt          = (abs_frame_num - 1) / h->sps.poc_cycle_length;
            int frame_num_in_poc_cycle = (abs_frame_num - 1) % h->sps.poc_cycle_length;

            expectedpoc = poc_cycle_cnt * expected_delta_per_poc_cycle;
            for(i = 0; i <= frame_num_in_poc_cycle; i++)
                expectedpoc = expectedpoc + h->sps.offset_for_ref_frame[ i ];
        } else
            expectedpoc = 0;

        if(h->nal_ref_idc == 0)
            expectedpoc = expectedpoc + h->sps.offset_for_non_ref_pic;

        field_poc[0] = expectedpoc + h->delta_poc[0];
        field_poc[1] = field_poc[0] + h->sps.offset_for_top_to_bottom_field;

        if(s->picture_structure == PICT_FRAME)
            field_poc[1] += h->delta_poc[1];
    }else{
        int poc= 2*(h->frame_num_offset + h->frame_num);

        if(!h->nal_ref_idc)
            poc--;

        field_poc[0]= poc;
        field_poc[1]= poc;
    }

    if(s->picture_structure != PICT_BOTTOM_FIELD)
        s->current_picture_ptr->field_poc[0]= field_poc[0];
    if(s->picture_structure != PICT_TOP_FIELD)
        s->current_picture_ptr->field_poc[1]= field_poc[1];
    cur->poc= FFMIN(cur->field_poc[0], cur->field_poc[1]);

    return 0;
}


/**
 * initialize scan tables
 */
static void init_scan_tables(H264Context *h){
    int i;
    if(1){ //FIXME little ugly
        memcpy(h->zigzag_scan, zigzag_scan, 16*sizeof(uint8_t));
        memcpy(h-> field_scan,  field_scan, 16*sizeof(uint8_t));
    }else{
        for(i=0; i<16; i++){
#define T(x) (x>>2) | ((x<<2) & 0xF)
            h->zigzag_scan[i] = T(zigzag_scan[i]);
            h-> field_scan[i] = T( field_scan[i]);
#undef T
        }
    }


    if(1){
        memcpy(h->zigzag_scan8x8,       ff_zigzag_direct,     64*sizeof(uint8_t));
        memcpy(h->zigzag_scan8x8_cavlc, zigzag_scan8x8_cavlc, 64*sizeof(uint8_t));
        memcpy(h->field_scan8x8,        field_scan8x8,        64*sizeof(uint8_t));
        memcpy(h->field_scan8x8_cavlc,  field_scan8x8_cavlc,  64*sizeof(uint8_t));
    }else{
        for(i=0; i<64; i++){
#define T(x) (x>>3) | ((x&7)<<3)
	    h->zigzag_scan8x8[i]       = T(ff_zigzag_direct[i]);
            h->zigzag_scan8x8_cavlc[i] = T(zigzag_scan8x8_cavlc[i]);
            h->field_scan8x8[i]        = T(field_scan8x8[i]);
            h->field_scan8x8_cavlc[i]  = T(field_scan8x8_cavlc[i]);
#undef T
        }
    }

    if(h->sps.transform_bypass){ //FIXME same ugly
        h->zigzag_scan_q0          = zigzag_scan;
        h->zigzag_scan8x8_q0       = ff_zigzag_direct;
        h->zigzag_scan8x8_cavlc_q0 = zigzag_scan8x8_cavlc;
        h->field_scan_q0           = field_scan;
        h->field_scan8x8_q0        = field_scan8x8;
        h->field_scan8x8_cavlc_q0  = field_scan8x8_cavlc;
    }else{
        h->zigzag_scan_q0          = h->zigzag_scan;
        h->zigzag_scan8x8_q0       = h->zigzag_scan8x8;
        h->zigzag_scan8x8_cavlc_q0 = h->zigzag_scan8x8_cavlc;
        h->field_scan_q0           = h->field_scan;
        h->field_scan8x8_q0        = h->field_scan8x8;
        h->field_scan8x8_cavlc_q0  = h->field_scan8x8_cavlc;
    }
}

static void field_end(H264Context *h){
    MpegEncContext * const s = &h->s;
    AVCodecContext * const avctx= s->avctx;
    s->mb_y= 0;

    s->current_picture_ptr->qscale_type= FF_QSCALE_TYPE_H264;
    s->current_picture_ptr->pict_type= s->pict_type;

    if (CONFIG_H264_VDPAU_DECODER && s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
        ff_vdpau_h264_set_reference_frames(s);

    if(!s->dropable) {
        ff_h264_execute_ref_pic_marking(h, h->mmco, h->mmco_index);
        h->prev_poc_msb= h->poc_msb;
        h->prev_poc_lsb= h->poc_lsb;
    }
    h->prev_frame_num_offset= h->frame_num_offset;
    h->prev_frame_num= h->frame_num;

    if (avctx->hwaccel) {
        if (avctx->hwaccel->end_frame(avctx) < 0)
            av_log(avctx, AV_LOG_ERROR, "hardware accelerator failed to decode picture\n");
    }

    if (CONFIG_H264_VDPAU_DECODER && s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
        ff_vdpau_h264_picture_complete(s);

    /*
     * FIXME: Error handling code does not seem to support interlaced
     * when slices span multiple rows
     * The ff_er_add_slice calls don't work right for bottom
     * fields; they cause massive erroneous error concealing
     * Error marking covers both fields (top and bottom).
     * This causes a mismatched s->error_count
     * and a bad error table. Further, the error count goes to
     * INT_MAX when called for bottom field, because mb_y is
     * past end by one (callers fault) and resync_mb_y != 0
     * causes problems for the first MB line, too.
     */
    if (!FIELD_PICTURE)
        ff_er_frame_end(s);

    MPV_frame_end(s);

    h->current_slice=0;
}

/**
 * Replicates H264 "master" context to thread contexts.
 */
static void clone_slice(H264Context *dst, H264Context *src)
{
    memcpy(dst->block_offset,     src->block_offset, sizeof(dst->block_offset));
    dst->s.current_picture_ptr  = src->s.current_picture_ptr;
    dst->s.current_picture      = src->s.current_picture;
    dst->s.linesize             = src->s.linesize;
    dst->s.uvlinesize           = src->s.uvlinesize;
    dst->s.first_field          = src->s.first_field;

    dst->prev_poc_msb           = src->prev_poc_msb;
    dst->prev_poc_lsb           = src->prev_poc_lsb;
    dst->prev_frame_num_offset  = src->prev_frame_num_offset;
    dst->prev_frame_num         = src->prev_frame_num;
    dst->short_ref_count        = src->short_ref_count;

    memcpy(dst->short_ref,        src->short_ref,        sizeof(dst->short_ref));
    memcpy(dst->long_ref,         src->long_ref,         sizeof(dst->long_ref));
    memcpy(dst->default_ref_list, src->default_ref_list, sizeof(dst->default_ref_list));
    memcpy(dst->ref_list,         src->ref_list,         sizeof(dst->ref_list));

    memcpy(dst->dequant4_coeff,   src->dequant4_coeff,   sizeof(src->dequant4_coeff));
    memcpy(dst->dequant8_coeff,   src->dequant8_coeff,   sizeof(src->dequant8_coeff));
}

/**
 * decodes a slice header.
 * This will also call MPV_common_init() and frame_start() as needed.
 *
 * @param h h264context
 * @param h0 h264 master context (differs from 'h' when doing sliced based parallel decoding)
 *
 * @return 0 if okay, <0 if an error occurred, 1 if decoding must not be multithreaded
 */
static int decode_slice_header(H264Context *h, H264Context *h0){
    MpegEncContext * const s = &h->s;
    MpegEncContext * const s0 = &h0->s;
    unsigned int first_mb_in_slice;
    unsigned int pps_id;
    int num_ref_idx_active_override_flag;
    unsigned int slice_type, tmp, i, j;
    int default_ref_list_done = 0;
    int last_pic_structure;

    s->dropable= h->nal_ref_idc == 0;

    //    printf("%s,%s\n",__FILE__,__FUNCTION__);
    if((s->avctx->flags2 & CODEC_FLAG2_FAST) && !h->nal_ref_idc){
        s->me.qpel_put= s->dsp.put_2tap_qpel_pixels_tab;
        s->me.qpel_avg= s->dsp.avg_2tap_qpel_pixels_tab;
    }else{
        s->me.qpel_put= s->dsp.put_h264_qpel_pixels_tab;
        s->me.qpel_avg= s->dsp.avg_h264_qpel_pixels_tab;
    }

    first_mb_in_slice= get_ue_golomb(&s->gb);

    if(first_mb_in_slice == 0){ //FIXME better field boundary detection
        if(h0->current_slice && FIELD_PICTURE){
            field_end(h);
        }

        h0->current_slice = 0;
        if (!s0->first_field)
            s->current_picture_ptr= NULL;
    }

    slice_type= get_ue_golomb_31(&s->gb);
    if(slice_type > 9){
        av_log(h->s.avctx, AV_LOG_ERROR, "slice type too large (%d) at %d %d\n", h->slice_type, s->mb_x, s->mb_y);
        return -1;
    }
    if(slice_type > 4){
        slice_type -= 5;
        h->slice_type_fixed=1;
    }else
        h->slice_type_fixed=0;

    slice_type= golomb_to_pict_type[ slice_type ];
    if (slice_type == FF_I_TYPE
        || (h0->current_slice != 0 && slice_type == h0->last_slice_type) ) {
        default_ref_list_done = 1;
    }
    h->slice_type= slice_type;
    h->slice_type_nos= slice_type & 3;

    s->pict_type= h->slice_type; // to make a few old functions happy, it's wrong though

    pps_id= get_ue_golomb(&s->gb);
    if(pps_id>=MAX_PPS_COUNT){
        av_log(h->s.avctx, AV_LOG_ERROR, "pps_id out of range\n");
        return -1;
    }

    if(!h0->pps_buffers[pps_id]) {
        av_log(h->s.avctx, AV_LOG_ERROR, "non-existing PPS %u referenced\n", pps_id);
        return -1;
    }
    h->pps= *h0->pps_buffers[pps_id];

    if(!h0->sps_buffers[h->pps.sps_id]) {
        av_log(h->s.avctx, AV_LOG_ERROR, "non-existing SPS %u referenced\n", h->pps.sps_id);
        return -1;
    }
    h->sps = *h0->sps_buffers[h->pps.sps_id];

    s->avctx->profile = h->sps.profile_idc;
    s->avctx->level   = h->sps.level_idc;
    s->avctx->refs    = h->sps.ref_frame_count;

    if(h == h0 && h->dequant_coeff_pps != pps_id){
        h->dequant_coeff_pps = pps_id;
        init_dequant_tables(h);
    }

    s->mb_width= h->sps.mb_width;
    s->mb_height= h->sps.mb_height * (2 - h->sps.frame_mbs_only_flag);

    h->b_stride=  s->mb_width*4;
    h->b8_stride= s->mb_width*2;

    s->width = 16*s->mb_width - 2*FFMIN(h->sps.crop_right, 7);
    if(h->sps.frame_mbs_only_flag)
        s->height= 16*s->mb_height - 2*FFMIN(h->sps.crop_bottom, 7);
    else
        s->height= 16*s->mb_height - 4*FFMIN(h->sps.crop_bottom, 3);

    if (s->context_initialized
        && (   s->width != s->avctx->width || s->height != s->avctx->height
            || av_cmp_q(h->sps.sar, s->avctx->sample_aspect_ratio))) {
        if(h != h0)
            return -1;   // width / height changed during parallelized decoding
        free_tables(h);
        flush_dpb(s->avctx);
        MPV_common_end(s);
    }

    if (!s->context_initialized) {
        if(h != h0)
            return -1;  // we cant (re-)initialize context during parallel decoding

        avcodec_set_dimensions(s->avctx, s->width, s->height);
        s->avctx->sample_aspect_ratio= h->sps.sar;
        if(!s->avctx->sample_aspect_ratio.den)
            s->avctx->sample_aspect_ratio.den = 1;

        if(h->sps.video_signal_type_present_flag){
            s->avctx->color_range = h->sps.full_range ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
            if(h->sps.colour_description_present_flag){
                s->avctx->color_primaries = h->sps.color_primaries;
                s->avctx->color_trc       = h->sps.color_trc;
                s->avctx->colorspace      = h->sps.colorspace;
            }
        }

        if(h->sps.timing_info_present_flag){
            int64_t den= h->sps.time_scale;
            if(h->x264_build < 44U)
                den *= 2;
            av_reduce(&s->avctx->time_base.num, &s->avctx->time_base.den,
                      h->sps.num_units_in_tick, den, 1<<30);
        }
        s->avctx->pix_fmt = s->avctx->get_format(s->avctx, s->avctx->codec->pix_fmts);
        s->avctx->hwaccel = ff_find_hwaccel(s->avctx->codec->id, s->avctx->pix_fmt);

        if (MPV_common_init(s) < 0)
            return -1;
        s->first_field = 0;
        h->prev_interlaced_frame = 1;

        init_scan_tables(h);
        ff_h264_alloc_tables(h);
        for(i = 1; i < s->avctx->thread_count; i++) {
            H264Context *c;
            c = h->thread_context[i] = av_malloc(sizeof(H264Context));
            memcpy(c, h->s.thread_context[i], sizeof(MpegEncContext));
            memset(&c->s + 1, 0, sizeof(H264Context) - sizeof(MpegEncContext));
            c->h264dsp = h->h264dsp;
            c->sps = h->sps;
            c->pps = h->pps;
            init_scan_tables(c);
            clone_tables(c, h, i);
        }

        for(i = 0; i < s->avctx->thread_count; i++)
            if(context_init(h->thread_context[i]) < 0)
                return -1;
    }
    
    

    h->frame_num= get_bits(&s->gb, h->sps.log2_max_frame_num);

    h->mb_mbaff = 0;
    h->mb_aff_frame = 0;
    last_pic_structure = s0->picture_structure;
    if(h->sps.frame_mbs_only_flag){
        s->picture_structure= PICT_FRAME;
    }else{
        if(get_bits1(&s->gb)) { //field_pic_flag
            s->picture_structure= PICT_TOP_FIELD + get_bits1(&s->gb); //bottom_field_flag
        } else {
            s->picture_structure= PICT_FRAME;
            h->mb_aff_frame = h->sps.mb_aff;
        }
    }
    h->mb_field_decoding_flag= s->picture_structure != PICT_FRAME;

    if(h0->current_slice == 0){

        while(h->frame_num !=  h->prev_frame_num &&
              h->frame_num != (h->prev_frame_num+1)%(1<<h->sps.log2_max_frame_num)){
            av_log(NULL, AV_LOG_DEBUG, "Frame num gap %d %d\n", h->frame_num, h->prev_frame_num);

            if (ff_h264_frame_start(h) < 0)
                return -1;
	
            h->prev_frame_num++;
            h->prev_frame_num %= 1<<h->sps.log2_max_frame_num;
            s->current_picture_ptr->frame_num= h->prev_frame_num;
	    ff_generate_sliding_window_mmcos(h);
            ff_h264_execute_ref_pic_marking(h, h->mmco, h->mmco_index);
        }

	//	printf("[mpv] 1 ...\n");
        /* See if we have a decoded first field looking for a pair... */
        if (s0->first_field) {
            assert(s0->current_picture_ptr);
            assert(s0->current_picture_ptr->data[0]);
            assert(s0->current_picture_ptr->reference != DELAYED_PIC_REF);

            /* figure out if we have a complementary field pair */
            if (!FIELD_PICTURE || s->picture_structure == last_pic_structure) {
                /*
                 * Previous field is unmatched. Don't display it, but let it
                 * remain for reference if marked as such.
                 */
                s0->current_picture_ptr = NULL;
                s0->first_field = FIELD_PICTURE;

            } else {
                if (h->nal_ref_idc &&
                        s0->current_picture_ptr->reference &&
                        s0->current_picture_ptr->frame_num != h->frame_num) {
                    /*
                     * This and previous field were reference, but had
                     * different frame_nums. Consider this field first in
                     * pair. Throw away previous field except for reference
                     * purposes.
                     */
                    s0->first_field = 1;
                    s0->current_picture_ptr = NULL;

                } else {
                    /* Second field in complementary pair */
                    s0->first_field = 0;
                }
            }

        } else {
            /* Frame or first field in a potentially complementary pair */
            assert(!s0->current_picture_ptr);
            s0->first_field = FIELD_PICTURE;
        }

        if((!FIELD_PICTURE || s0->first_field) && ff_h264_frame_start(h) < 0) {
            s0->first_field = 0;
            return -1;
        }
    }
    if(h != h0)
        clone_slice(h, h0);

    s->current_picture_ptr->frame_num= h->frame_num; //FIXME frame_num cleanup

    assert(s->mb_num == s->mb_width * s->mb_height);
    if(first_mb_in_slice << FIELD_OR_MBAFF_PICTURE >= s->mb_num ||
       first_mb_in_slice                    >= s->mb_num){
        av_log(h->s.avctx, AV_LOG_ERROR, "first_mb_in_slice overflow\n");
        return -1;
    }
    s->resync_mb_x = s->mb_x = first_mb_in_slice % s->mb_width;
    s->resync_mb_y = s->mb_y = (first_mb_in_slice / s->mb_width) << FIELD_OR_MBAFF_PICTURE;
    if (s->picture_structure == PICT_BOTTOM_FIELD)
        s->resync_mb_y = s->mb_y = s->mb_y + 1;
    assert(s->mb_y < s->mb_height);

    if(s->picture_structure==PICT_FRAME){
        h->curr_pic_num=   h->frame_num;
        h->max_pic_num= 1<< h->sps.log2_max_frame_num;
    }else{
        h->curr_pic_num= 2*h->frame_num + 1;
        h->max_pic_num= 1<<(h->sps.log2_max_frame_num + 1);
    }

    if(h->nal_unit_type == NAL_IDR_SLICE){
        get_ue_golomb(&s->gb); /* idr_pic_id */
    }

    if(h->sps.poc_type==0){
        h->poc_lsb= get_bits(&s->gb, h->sps.log2_max_poc_lsb);

        if(h->pps.pic_order_present==1 && s->picture_structure==PICT_FRAME){
            h->delta_poc_bottom= get_se_golomb(&s->gb);
        }
    }

    if(h->sps.poc_type==1 && !h->sps.delta_pic_order_always_zero_flag){
        h->delta_poc[0]= get_se_golomb(&s->gb);

        if(h->pps.pic_order_present==1 && s->picture_structure==PICT_FRAME)
            h->delta_poc[1]= get_se_golomb(&s->gb);
    }

    init_poc(h);

    if(h->pps.redundant_pic_cnt_present){
        h->redundant_pic_count= get_ue_golomb(&s->gb);
    }

    //set defaults, might be overridden a few lines later
    h->ref_count[0]= h->pps.ref_count[0];
    h->ref_count[1]= h->pps.ref_count[1];

    if(h->slice_type_nos != FF_I_TYPE){
        if(h->slice_type_nos == FF_B_TYPE){
            h->direct_spatial_mv_pred= get_bits1(&s->gb);
        }
        num_ref_idx_active_override_flag= get_bits1(&s->gb);

        if(num_ref_idx_active_override_flag){
            h->ref_count[0]= get_ue_golomb(&s->gb) + 1;
            if(h->slice_type_nos==FF_B_TYPE)
                h->ref_count[1]= get_ue_golomb(&s->gb) + 1;

            if(h->ref_count[0]-1 > 32-1 || h->ref_count[1]-1 > 32-1){
                av_log(h->s.avctx, AV_LOG_ERROR, "reference overflow\n");
                h->ref_count[0]= h->ref_count[1]= 1;
                return -1;
            }
        }
        if(h->slice_type_nos == FF_B_TYPE)
            h->list_count= 2;
        else
            h->list_count= 1;
    }else
        h->list_count= 0;

    if(!default_ref_list_done){
        ff_h264_fill_default_ref_list(h);
    }

    if(h->slice_type_nos!=FF_I_TYPE && ff_h264_decode_ref_pic_list_reordering(h) < 0)
        return -1;

    if(h->slice_type_nos!=FF_I_TYPE){
        s->last_picture_ptr= &h->ref_list[0][0];
        ff_copy_picture(&s->last_picture, s->last_picture_ptr);
    }
    if(h->slice_type_nos==FF_B_TYPE){
        s->next_picture_ptr= &h->ref_list[1][0];
        ff_copy_picture(&s->next_picture, s->next_picture_ptr);
    }

    if(   (h->pps.weighted_pred          && h->slice_type_nos == FF_P_TYPE )
       ||  (h->pps.weighted_bipred_idc==1 && h->slice_type_nos== FF_B_TYPE ) )
        pred_weight_table(h);
    else if(h->pps.weighted_bipred_idc==2 && h->slice_type_nos== FF_B_TYPE){
        implicit_weight_table(h, -1);
    }else {
        h->use_weight = 0;
        for (i = 0; i < 2; i++) {
            h->luma_weight_flag[i]   = 0;
            h->chroma_weight_flag[i] = 0;
        }
    }

    if(h->nal_ref_idc)
        ff_h264_decode_ref_pic_marking(h0, &s->gb);

 
    if(FRAME_MBAFF){
        ff_h264_fill_mbaff_ref_list(h);

        if(h->pps.weighted_bipred_idc==2 && h->slice_type_nos== FF_B_TYPE){
            implicit_weight_table(h, 0);
            implicit_weight_table(h, 1);
        }
    }

    if(h->slice_type_nos==FF_B_TYPE && !h->direct_spatial_mv_pred)
        ff_h264_direct_dist_scale_factor(h);
    ff_h264_direct_ref_list_init(h);

    if( h->slice_type_nos != FF_I_TYPE && h->pps.cabac ){
        tmp = get_ue_golomb_31(&s->gb);
        if(tmp > 2){
            av_log(s->avctx, AV_LOG_ERROR, "cabac_init_idc overflow\n");
            return -1;
        }
        h->cabac_init_idc= tmp;
    }

    h->last_qscale_diff = 0;
    tmp = h->pps.init_qp + get_se_golomb(&s->gb);
    if(tmp>51){
        av_log(s->avctx, AV_LOG_ERROR, "QP %u out of range\n", tmp);
        return -1;
    }
    s->qscale= tmp;
    h->chroma_qp[0] = get_chroma_qp(h, 0, s->qscale);
    h->chroma_qp[1] = get_chroma_qp(h, 1, s->qscale);
    //FIXME qscale / qp ... stuff
    if(h->slice_type == FF_SP_TYPE){
        get_bits1(&s->gb); /* sp_for_switch_flag */
    }
    if(h->slice_type==FF_SP_TYPE || h->slice_type == FF_SI_TYPE){
        get_se_golomb(&s->gb); /* slice_qs_delta */
    }

    h->deblocking_filter = 1;

    h->slice_alpha_c0_offset = 0;
    h->slice_beta_offset = 0;
    if( h->pps.deblocking_filter_parameters_present ) {
        tmp= get_ue_golomb_31(&s->gb);
        if(tmp > 2){
            av_log(s->avctx, AV_LOG_ERROR, "deblocking_filter_idc %u out of range\n", tmp);
            return -1;
        }
        h->deblocking_filter= tmp;
        if(h->deblocking_filter < 2)
            h->deblocking_filter^= 1; // 1<->0

        if( h->deblocking_filter ) {
            h->slice_alpha_c0_offset += get_se_golomb(&s->gb) << 1;
            h->slice_beta_offset     += get_se_golomb(&s->gb) << 1;
#if 0
            if(   h->slice_alpha_c0_offset > 104U
               || h->slice_beta_offset     > 104U){
	       av_log(s->avctx, AV_LOG_ERROR, "deblocking filter parameters %d %d out of range\n", h->slice_alpha_c0_offset, h->slice_beta_offset);
               return -1;
            }
#endif
        }
    }

    if(   s->avctx->skip_loop_filter >= AVDISCARD_ALL
       ||(s->avctx->skip_loop_filter >= AVDISCARD_NONKEY && h->slice_type_nos != FF_I_TYPE)
       ||(s->avctx->skip_loop_filter >= AVDISCARD_BIDIR  && h->slice_type_nos == FF_B_TYPE)
       ||(s->avctx->skip_loop_filter >= AVDISCARD_NONREF && h->nal_ref_idc == 0))
        h->deblocking_filter= 0;

    if(h->deblocking_filter == 1 && h0->max_contexts > 1) {
        if(s->avctx->flags2 & CODEC_FLAG2_FAST) {
            /* Cheat slightly for speed:
               Do not bother to deblock across slices. */
            h->deblocking_filter = 2;
        } else {
            h0->max_contexts = 1;
            if(!h0->single_decode_warning) {
                av_log(s->avctx, AV_LOG_INFO, "Cannot parallelize deblocking type 1, decoding such frames in sequential order\n");
                h0->single_decode_warning = 1;
            }
            if(h != h0)
                return 1; // deblocking switched inside frame
        }
    }
    h->qp_thresh= 15 + 52 - FFMIN(h->slice_alpha_c0_offset, h->slice_beta_offset) - FFMAX3(0, h->pps.chroma_qp_index_offset[0], h->pps.chroma_qp_index_offset[1]);

#if 0 //FMO
    if( h->pps.num_slice_groups > 1  && h->pps.mb_slice_group_map_type >= 3 && h->pps.mb_slice_group_map_type <= 5)
        slice_group_change_cycle= get_bits(&s->gb, ?);
#endif

    //  av_log(s->avctx, AV_LOG_WARNING,"##########   h->deblocking_filter=%d\n",h->deblocking_filter);
    h0->last_slice_type = slice_type;
    h->slice_num = ++h0->current_slice;
    if(h->slice_num >= MAX_SLICES){
        av_log(s->avctx, AV_LOG_ERROR, "Too many slices, increase MAX_SLICES and recompile\n");
    }

    for(j=0; j<2; j++){
        int id_list[16];
        int *ref2frm= h->ref2frm[h->slice_num&(MAX_SLICES-1)][j];
        for(i=0; i<16; i++){
            id_list[i]= 60;
            if(h->ref_list[j][i].data[0]){
                int k;
                uint8_t *base= h->ref_list[j][i].base[0];
                for(k=0; k<h->short_ref_count; k++)
                    if(h->short_ref[k]->base[0] == base){
                        id_list[i]= k;
                        break;
                    }
                for(k=0; k<h->long_ref_count; k++)
                    if(h->long_ref[k] && h->long_ref[k]->base[0] == base){
                        id_list[i]= h->short_ref_count + k;
                        break;
                    }
            }
        }

        ref2frm[0]=
        ref2frm[1]= -1;
        for(i=0; i<16; i++)
            ref2frm[i+2]= 4*id_list[i]
                          +(h->ref_list[j][i].reference&3);
        ref2frm[18+0]=
        ref2frm[18+1]= -1;
        for(i=16; i<48; i++)
            ref2frm[i+4]= 4*id_list[(i-16)>>1]
                          +(h->ref_list[j][i].reference&3);
    }

    h->emu_edge_width= (s->flags&CODEC_FLAG_EMU_EDGE) ? 0 : 16;
    h->emu_edge_height= (FRAME_MBAFF || FIELD_PICTURE) ? 0 : h->emu_edge_width;

    if(s->avctx->debug&FF_DEBUG_PICT_INFO){
        av_log(h->s.avctx, AV_LOG_DEBUG, "slice:%d %s mb:%d %c%s%s pps:%u frame:%d poc:%d/%d ref:%d/%d qp:%d loop:%d:%d:%d weight:%d%s %s\n",
               h->slice_num,
               (s->picture_structure==PICT_FRAME ? "F" : s->picture_structure==PICT_TOP_FIELD ? "T" : "B"),
               first_mb_in_slice,
               av_get_pict_type_char(h->slice_type), h->slice_type_fixed ? " fix" : "", h->nal_unit_type == NAL_IDR_SLICE ? " IDR" : "",
               pps_id, h->frame_num,
               s->current_picture_ptr->field_poc[0], s->current_picture_ptr->field_poc[1],
               h->ref_count[0], h->ref_count[1],
               s->qscale,
               h->deblocking_filter, h->slice_alpha_c0_offset/2-26, h->slice_beta_offset/2-26,
               h->use_weight,
               h->use_weight==1 && h->use_weight_chroma ? "c" : "",
               h->slice_type == FF_B_TYPE ? (h->direct_spatial_mv_pred ? "SPAT" : "TEMP") : ""
               );
    }
    return 0;
}

int ff_h264_get_slice_type(const H264Context *h)
{
    switch (h->slice_type) {
    case FF_P_TYPE:  return 0;
    case FF_B_TYPE:  return 1;
    case FF_I_TYPE:  return 2;
    case FF_SP_TYPE: return 3;
    case FF_SI_TYPE: return 4;
    default:         return -1;
    }
}
extern int tcsm_fd;
static int decode_slice(struct AVCodecContext *avctx, void *arg){
  //ALOGE("h264 decode_slice in");
    H264Context *h = *(void**)arg;
    MpegEncContext * const s = &h->s;
    int i,j;
    jzm_h264 *st_h264 = (jzm_h264 *)(h->st_h264);
    const int part_mask= s->partitioned_frame ? (AC_END|AC_ERROR) : 0x7F;
    s->mb_skip_run= -1;
    h->is_complex = FRAME_MBAFF || s->picture_structure != PICT_FRAME || s->codec_id != CODEC_ID_H264 || (CONFIG_GRAY && (s->flags&CODEC_FLAG_GRAY));

    if (h->deblocking_filter == 1)
	  h->deblocking_filter = 2;

      st_h264->slice_type                  = (h->slice_type==FF_B_TYPE) ? JZM_H264_B_TYPE : ((h->slice_type==FF_P_TYPE) ? JZM_H264_P_TYPE : JZM_H264_I_TYPE);      st_h264->slice_num                   = h->slice_num-1; 
      st_h264->start_mb_x                  = h->s.mb_x;    
      st_h264->start_mb_y                  = h->s.mb_y;
      st_h264->mb_width                    = h->s.mb_width; 
      st_h264->mb_height                   = h->s.mb_height; 
      st_h264->field_picture               = h->s.picture_structure != PICT_FRAME;
      st_h264->cabac                       = h->pps.cabac;
      st_h264->qscale                      = s->qscale;        
      st_h264->transform_8x8_mode          = !!h->pps.transform_8x8_mode;
      st_h264->constrained_intra_pred      = !!h->pps.constrained_intra_pred;
      st_h264->direct_8x8_inference_flag   = !!h->sps.direct_8x8_inference_flag;
      st_h264->direct_spatial_mv_pred      = !!h->direct_spatial_mv_pred;
      st_h264->x264_build                  = h->x264_build;
      st_h264->ref_count_0                 = h->ref_count[0]-1; 
      st_h264->ref_count_1                 = h->ref_count[1]-1; 
      st_h264->deblocking_filter           = !!h->deblocking_filter; 
      st_h264->slice_alpha_c0_offset       = h->slice_alpha_c0_offset;
      st_h264->slice_beta_offset           = h->slice_beta_offset;
      st_h264->bs_buffer                   = s->gb.buffer; 
      st_h264->bs_index                    = s->gb.index;
      st_h264->bs_size_in_bits             = s->gb.size_in_bits;
      st_h264->cabac_init_idc              = h->cabac_init_idc;
      st_h264->ref_frm_ctrl                = h->ref_list[1][0].frm_info_ctrl;
      st_h264->ref_frm_mv                  = h->ref_list[1][0].frm_info_mv;
      st_h264->curr_frm_ctrl               = s->current_picture.frm_info_ctrl;
      st_h264->curr_frm_mv                 = s->current_picture.frm_info_mv;
      st_h264->curr_frm_slice_start_mb     = s->current_picture.frm_info_slice_start_mb;
      st_h264->ref_frm_slice_start_mb      = h->ref_list[1][0].frm_info_slice_start_mb;
      st_h264->use_weight                  = h->use_weight;
      st_h264->use_weight_chroma           = h->use_weight_chroma;
      st_h264->luma_log2_weight_denom      = h->luma_log2_weight_denom;
      st_h264->chroma_log2_weight_denom    = h->chroma_log2_weight_denom;
      st_h264->dblk_left_en                = /*h->s.mb_x!=*/0;
      st_h264->dblk_top_en                 = /*h->s.mb_y!=*/0;
      st_h264->dec_result_y                = s->current_picture.data[0];
      st_h264->dec_result_uv               = s->current_picture.data[1];
      for (i=0; i<16; i++) {
	st_h264->dir_scale_table[i] = h->map_col_to_list0[0][i] + (h->map_col_to_list0[1][i] << 5) + (h->dist_scale_factor[i] << 16);
      }

      for (i=0; i<128; i++) {
	unsigned int qp_c0 = h->pps.chroma_qp_table[0][i] & 0xFF;
	unsigned int qp_c1 = h->pps.chroma_qp_table[1][i] & 0xFF;
	unsigned int qp_c0_d6 = qp_c0/6;
	unsigned int qp_c0_m6 = qp_c0%6;
	unsigned int qp_c1_d6 = qp_c1/6;
	unsigned int qp_c1_m6 = qp_c1%6;
	st_h264->chroma_qp_table[i] = ((((((qp_c1_d6 & 0xF) + ((qp_c1_m6 & 0x7) << 4)) << 7) +
					(((qp_c0_d6 & 0xF) + ((qp_c0_m6 & 0x7) << 4)) << 0) ) << 16) +
				      ((qp_c1 & 0x3F) << 6) +
				      ((qp_c0 & 0x3F) << 0)
				      );
      }

      for (i=0; i<16; i++) {
	st_h264->mc_ref_y[0][i] = (unsigned int)(h->ref_list[0][i].data[0]);
	st_h264->mc_ref_c[0][i] = (unsigned int)(h->ref_list[0][i].data[1]);
	st_h264->mc_ref_y[1][i] = (unsigned int)(h->ref_list[1][i].data[0]);
	st_h264->mc_ref_c[1][i] = (unsigned int)(h->ref_list[1][i].data[1]);
	for (j=0; j<2; j++) {
	  st_h264->luma_weight[j][i]      = h->luma_weight[i][j][0];
	  st_h264->chroma_weight[j][i][0] = h->chroma_weight[i][j][0][0];
	  st_h264->chroma_weight[j][i][1] = h->chroma_weight[i][j][1][0];
	  st_h264->luma_offset[j][i]      = h->luma_weight[i][j][1];
	  st_h264->chroma_offset[j][i][0] = h->chroma_weight[i][j][0][1];
	  st_h264->chroma_offset[j][i][1] = h->chroma_weight[i][j][1][1];
	}
	for (j=0; j<16; j++) {
	  st_h264->implicit_weight[i][j] = h->implicit_weight[i][j][0];
	}
      }

#if 0 // print slice info
      unsigned char * p = (unsigned char*)(&st_h264);
      int len = sizeof(st_h264);
      for(i = 0; i < len; i++)
	  ALOGE("0x%x", p[i]);
#endif // print slice info

      jzm_h264_slice_init_vdma(st_h264);
      EL("des_pa=0x%x,data=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",
	 st_h264->des_pa,
	 ((int*)(st_h264->des_pa))[0],
	 ((int*)(st_h264->des_pa))[1],
	 ((int*)(st_h264->des_pa))[2],
	 ((int*)(st_h264->des_pa))[3],
	 ((int*)(st_h264->des_pa))[4],
	 ((int*)(st_h264->des_pa))[5]
	 );
      jz_dcache_wb();
      RST_VPU();

      //clear end flags
      /* *(volatile unsigned int *)(sde_base + 0x0) = 0x0; // clear sde flag */
      /* *(volatile unsigned int *)(dblk0_base + 0x70) = 0x0; //clear dblk flag */
      *((volatile unsigned int *)(vpu_base + REG_SCH_GLBC)) = (SCH_GLBC_HIAXI
							       | SCH_GLBC_TLBE | SCH_GLBC_TLBINV
							       | SCH_INTE_ACFGERR
							       | SCH_INTE_TLBERR
							       | SCH_INTE_BSERR
							       | SCH_INTE_ENDF
							       );

      unsigned int tlb_addr;
      dmmu_get_page_table_base_phys(&tlb_addr);
      *((volatile unsigned int *)(vpu_base + REG_SCH_TLBA)) = tlb_addr;

      *((volatile unsigned int *)(gp0_base + 0x8)) = (((unsigned int)(st_h264->des_pa)& 0xFFFFFF80) | 0x1);

      EL("");

      int a = 0;
      ioctl(tcsm_fd, 0, &a);
      if ((a & 0x1) == 1){
      }else{
	  ALOGE("vpu status=0x%x,vdma status=0x%x,vdma dha=0x%x, sde id=0x%x,sde cfg0=0x%x,sde bsaddr=0x%x,",
	     *(volatile unsigned int *)(vpu_base + 0x34),
	     *(volatile unsigned int *)(gp0_base + 0xC),
	     *(volatile unsigned int *)(gp0_base + 0x8),
	     *(volatile unsigned int *)(sde_base + 0x10),
	     *(volatile unsigned int *)(sde_base + 0x14),
	     *(volatile unsigned int *)(sde_base + 0x1c)
	     );
      }
      EL("");

      //ALOGE("h264 decode_slice out");
    return -1; //not reached
}

/**
 * Call decode_slice() for each context.
 *
 * @param h h264 master context
 * @param context_count number of contexts to execute
 */
static void execute_decode_slices(H264Context *h, int context_count){
    MpegEncContext * const s = &h->s;
    AVCodecContext * const avctx= s->avctx;
    H264Context *hx;
    int i;

    if (s->avctx->hwaccel)
        return;
    if(s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
        return;
    if(context_count == 1) {
        decode_slice(avctx, &h);
    } else {
        for(i = 1; i < context_count; i++) {
            hx = h->thread_context[i];
            hx->s.error_recognition = avctx->error_recognition;
            hx->s.error_count = 0;
        }

        avctx->execute(avctx, (void *)decode_slice,
                       h->thread_context, NULL, context_count, sizeof(void*));

        /* pull back stuff from slices to master context */
        hx = h->thread_context[context_count - 1];
        s->mb_x = hx->s.mb_x;
        s->mb_y = hx->s.mb_y;
        s->dropable = hx->s.dropable;
        s->picture_structure = hx->s.picture_structure;
        for(i = 1; i < context_count; i++)
            h->s.error_count += h->thread_context[i]->s.error_count;
    }
}


static int decode_nal_units(H264Context *h, const uint8_t *buf, int buf_size, int extra){
    MpegEncContext * const s = &h->s;
    AVCodecContext * const avctx= s->avctx;
    int buf_index=0;
    H264Context *hx; ///< thread context
    int context_count = 0;
    int next_avc= h->is_avc ? 0 : buf_size;
    h->max_contexts = avctx->thread_count;
    if(!(s->flags2 & CODEC_FLAG2_CHUNKS)){
      h->current_slice = 0;
      if (!s->first_field)
	s->current_picture_ptr= NULL;
      ff_h264_reset_sei(h);
    }

    for(;;){
        int consumed;
        int dst_length;
        int bit_length;
        const uint8_t *ptr;
        int i, nalsize = 0;
        int err;

        if(buf_index >= next_avc) {
            if(buf_index >= buf_size) break;
            nalsize = 0;
            for(i = 0; i < h->nal_length_size; i++)
                nalsize = (nalsize << 8) | buf[buf_index++];
            if(nalsize <= 1 || nalsize > buf_size - buf_index){
                if(nalsize == 1){
                    buf_index++;
                    continue;
                }else{
                    av_log(h->s.avctx, AV_LOG_ERROR, "AVC: nal size %d\n", nalsize);
                    break;
                }
            }
            next_avc= buf_index + nalsize;
        } else {
            // start code prefix search
            for(; buf_index + 3 < next_avc; buf_index++){
                // This should always succeed in the first iteration.
                if(buf[buf_index] == 0 && buf[buf_index+1] == 0 && buf[buf_index+2] == 1)
                    break;
            }

            if(buf_index+3 >= buf_size) break;

            buf_index+=3;
            if(buf_index >= next_avc) continue;
        }

        hx = h->thread_context[context_count];
	
	////for android PREFETCHER_DEPACK_NAL
	if(avctx->is_flash == 0){
#ifdef PREFETCHER_DEPACK_NAL
	    if((extra==0 ) && (((sh_video_t *)(avctx->opaque))->is_rtsp != 1)){
		h->nal_ref_idc= buf[buf_index]>>5;
		h->nal_unit_type= buf[buf_index]&0x1F;
		ptr=buf + buf_index+1;
		consumed=nalsize;
		dst_length=nalsize-1;
	    }else{
		ptr= ff_h264_decode_nal(hx, buf + buf_index, &dst_length, &consumed, next_avc - buf_index);
	    }
#else
	    ptr= ff_h264_decode_nal(hx, buf + buf_index, &dst_length, &consumed, next_avc - buf_index);
#endif //PREFETCHER_DEPACK_NAL
	}else
	    ptr= ff_h264_decode_nal(hx, buf + buf_index, &dst_length, &consumed, next_avc - buf_index);

        if (ptr==NULL || dst_length < 0){
            return -1;
        }
	////for android PREFETCHER_DEPACK_NAL...

        i= buf_index + consumed;
        if((s->workaround_bugs & FF_BUG_AUTODETECT) && i+3<next_avc &&
           buf[i]==0x00 && buf[i+1]==0x00 && buf[i+2]==0x01 && buf[i+3]==0xE0)
            s->workaround_bugs |= FF_BUG_TRUNCATED;

        if(!(s->workaround_bugs & FF_BUG_TRUNCATED)){
        while(ptr[dst_length - 1] == 0 && dst_length > 0)
            dst_length--;
        }
        bit_length= !dst_length ? 0 : (8*dst_length - ff_h264_decode_rbsp_trailing(h, ptr + dst_length - 1));

        if(s->avctx->debug&FF_DEBUG_STARTCODE){
            av_log(h->s.avctx, AV_LOG_DEBUG, "NAL %d at %d/%d length %d\n", hx->nal_unit_type, buf_index, buf_size, dst_length);
        }

        if (h->is_avc && (nalsize != consumed) && nalsize){
            av_log(h->s.avctx, AV_LOG_DEBUG, "AVC: Consumed only %d bytes instead of %d\n", consumed, nalsize);
        }

        buf_index += consumed;

        if(  (s->hurry_up == 1 && h->nal_ref_idc  == 0) //FIXME do not discard SEI id
           ||(avctx->skip_frame >= AVDISCARD_NONREF && h->nal_ref_idc  == 0))
            continue;

      again:
        err = 0;
        switch(hx->nal_unit_type){
        case NAL_IDR_SLICE:
            if (h->nal_unit_type != NAL_IDR_SLICE) {
                av_log(h->s.avctx, AV_LOG_ERROR, "Invalid mix of idr and non-idr slices");
                return -1;
            }
            idr(h); //FIXME ensure we don't loose some frames if there is reordering
        case NAL_SLICE:
            init_get_bits(&hx->s.gb, ptr, bit_length);
            hx->intra_gb_ptr=
            hx->inter_gb_ptr= &hx->s.gb;
            hx->s.data_partitioning = 0;

            if((err = decode_slice_header(hx, h)))
               break;

	    /*  printf("after slice_header currpic pa=0x%x,0x%x\n",
	   get_phy_addr(s->current_picture.data[0]),
	   get_phy_addr(s->current_picture.data[1]));
	    */

            if (h->current_slice == 1) {
                if (s->avctx->hwaccel && s->avctx->hwaccel->start_frame(s->avctx, NULL, 0) < 0)
                    return -1;
                if(CONFIG_H264_VDPAU_DECODER && s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
                    ff_vdpau_h264_picture_start(s);
            }

            s->current_picture_ptr->key_frame |=
                    (hx->nal_unit_type == NAL_IDR_SLICE) ||
                    (h->sei_recovery_frame_cnt >= 0);
            if(hx->redundant_pic_count==0 && hx->s.hurry_up < 5
               && (avctx->skip_frame < AVDISCARD_NONREF || hx->nal_ref_idc)
               && (avctx->skip_frame < AVDISCARD_BIDIR  || hx->slice_type_nos!=FF_B_TYPE)
               && (avctx->skip_frame < AVDISCARD_NONKEY || hx->slice_type_nos==FF_I_TYPE)
               && avctx->skip_frame < AVDISCARD_ALL){
                if(avctx->hwaccel) {
                    if (avctx->hwaccel->decode_slice(avctx, &buf[buf_index - consumed], consumed) < 0)
                        return -1;
                }else
                if(CONFIG_H264_VDPAU_DECODER && s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU){
                    static const uint8_t start_code[] = {0x00, 0x00, 0x01};
                    ff_vdpau_add_data_chunk(s, start_code, sizeof(start_code));
                    ff_vdpau_add_data_chunk(s, &buf[buf_index - consumed], consumed );
                }else
                    context_count++;
            }
            break;
        case NAL_DPA:
            init_get_bits(&hx->s.gb, ptr, bit_length);
            hx->intra_gb_ptr=
            hx->inter_gb_ptr= NULL;

            if ((err = decode_slice_header(hx, h)) < 0)
                break;

            hx->s.data_partitioning = 1;

            break;
        case NAL_DPB:
            init_get_bits(&hx->intra_gb, ptr, bit_length);
            hx->intra_gb_ptr= &hx->intra_gb;
            break;
        case NAL_DPC:
            init_get_bits(&hx->inter_gb, ptr, bit_length);
            hx->inter_gb_ptr= &hx->inter_gb;

            if(hx->redundant_pic_count==0 && hx->intra_gb_ptr && hx->s.data_partitioning
               && s->context_initialized
               && s->hurry_up < 5
               && (avctx->skip_frame < AVDISCARD_NONREF || hx->nal_ref_idc)
               && (avctx->skip_frame < AVDISCARD_BIDIR  || hx->slice_type_nos!=FF_B_TYPE)
               && (avctx->skip_frame < AVDISCARD_NONKEY || hx->slice_type_nos==FF_I_TYPE)
               && avctx->skip_frame < AVDISCARD_ALL)
                context_count++;
            break;
        case NAL_SEI:
            init_get_bits(&s->gb, ptr, bit_length);
            ff_h264_decode_sei(h);
            break;
        case NAL_SPS:
            init_get_bits(&s->gb, ptr, bit_length);
            ff_h264_decode_seq_parameter_set(h);

            if(s->flags& CODEC_FLAG_LOW_DELAY)
                s->low_delay=1;

            if(avctx->has_b_frames < 2)
                avctx->has_b_frames= !s->low_delay;
            break;
        case NAL_PPS:
            init_get_bits(&s->gb, ptr, bit_length);
            ff_h264_decode_picture_parameter_set(h, bit_length);
            break;
        case NAL_AUD:
        case NAL_END_SEQUENCE:
        case NAL_END_STREAM:
        case NAL_FILLER_DATA:
        case NAL_SPS_EXT:
        case NAL_AUXILIARY_SLICE:
            break;
        default:
            av_log(avctx, AV_LOG_DEBUG, "Unknown NAL code: %d (%d bits)\n", hx->nal_unit_type, bit_length);
        }

        if(context_count == h->max_contexts) {
            execute_decode_slices(h, context_count);
            context_count = 0;
        }

        if (err < 0)
            av_log(h->s.avctx, AV_LOG_ERROR, "decode_slice_header error\n");
        else if(err == 1) {
            /* Slice could not be decoded in parallel mode, copy down
             * NAL unit stuff to context 0 and restart. Note that
             * rbsp_buffer is not transferred, but since we no longer
             * run in parallel mode this should not be an issue. */
            h->nal_unit_type = hx->nal_unit_type;
            h->nal_ref_idc   = hx->nal_ref_idc;
            hx = h;
            goto again;
        }
    }
    if(context_count)
        execute_decode_slices(h, context_count);
    return buf_index;
}

/**
 * returns the number of bytes consumed for building the current frame
 */
static int get_consumed_bytes(MpegEncContext *s, int pos, int buf_size){
        if(pos==0) pos=1; //avoid infinite loops (i doubt that is needed but ...)
        if(pos+10>buf_size) pos=buf_size; // oops ;)

        return pos;
}

static int decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    EL("");
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    H264Context *h = avctx->priv_data;
    MpegEncContext *s = &h->s;
    AVFrame *pict = data;
    int buf_index;
    s->flags= avctx->flags;
    s->flags2= avctx->flags2;
   
   /* end of stream, output what is still in the buffers */
    if (buf_size == 0) {
        Picture *out;
        int i, out_idx;

//FIXME factorize this with the output code below
        out = h->delayed_pic[0];
        out_idx = 0;
        for(i=1; h->delayed_pic[i] && !h->delayed_pic[i]->key_frame && !h->delayed_pic[i]->mmco_reset; i++)
            if(h->delayed_pic[i]->poc < out->poc){
                out = h->delayed_pic[i];
                out_idx = i;
            }

        for(i=out_idx; h->delayed_pic[i]; i++)
            h->delayed_pic[i] = h->delayed_pic[i+1];

        if(out){
            *data_size = sizeof(AVFrame);
            *pict= *(AVFrame*)out;
        }

        return 0;
    }

    EL("");
    buf_index=decode_nal_units(h, buf, buf_size, 0);

    if(buf_index < 0)
        return -1;

    EL("");
    if(!(s->flags2 & CODEC_FLAG2_CHUNKS) && !s->current_picture_ptr){
        if (avctx->skip_frame >= AVDISCARD_NONREF || s->hurry_up) return 0;
        av_log(avctx, AV_LOG_ERROR, "no frame!\n");
        return -1;
    }


#ifdef JZC_CRC_VER
    {
      int crc_i;
      int crc_tlb;
      int linelen = (s->mb_width*256 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
      unsigned char *dest_y  = s->current_picture.data[0];
      unsigned char *dest_uv  = s->current_picture.data[1];
      for (crc_i = 0; crc_i < s->mb_height; crc_i++ ) {
	  crc_code = crc(s->current_picture.data[0] + crc_i * linelen, s->mb_width * 256, crc_code);
      }

      linelen = (s->mb_width*128 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
      for (crc_i = 0; crc_i < s->mb_height; crc_i++ ) {
	  crc_code = crc(s->current_picture.data[1] + crc_i * linelen, s->mb_width * 128, crc_code);
      }

      ALOGE(" + frame: %d, crc_code: 0x%x\n", mpFrame, crc_code);  
      
    }
    mpFrame++;
#endif

#ifdef JZC_PMON_P0
    PMON_ON(fedge);
#endif
#ifdef JZC_PMON_P0
    PMON_OFF(fedge);
#endif

#ifdef JZC_PMON_P0
    {
      pmon_p0_frmcnt ++;
      int mb_num = ((s->width+15)/16)*((s->height+15)/16);

      if (pmon_p0_frmcnt==1)
	printf("PMON nfl:%s\t(size: %d x %d; mb_num: %d) \n",filename,s->width,s->height,mb_num);

      printf("PMON frame num: %d; SLICE_TYPE: %s\n",
	      pmon_p0_frmcnt-1,(h->slice_type == FF_I_TYPE)?"I":
	      ((h->slice_type == FF_P_TYPE)?"P":((h->slice_type == FF_B_TYPE)?"B":"S")));
      printf("PMON PARSE -D: %d; I:%d\n",
	      fslice_pmon_val/mb_num, fslice_pmon_val_ex/mb_num);
      printf("PMON EDGE -D: %d; I:%d\n",
	      fedge_pmon_val/mb_num, fedge_pmon_val_ex/mb_num);
      printf("PMON WAIT -D: %d; I:%d\n",
	      fwait_pmon_val/mb_num, fwait_pmon_val_ex/mb_num);
      fslice_pmon_val=0; fslice_pmon_val_ex=0;
      fwait_pmon_val=0; fwait_pmon_val_ex=0;
      fedge_pmon_val=0; fedge_pmon_val_ex=0;
    }
#endif //JZC_PMON_P0

    if(!(s->flags2 & CODEC_FLAG2_CHUNKS) || (s->mb_y >= s->mb_height && s->mb_height)){
        Picture *out = s->current_picture_ptr;
        Picture *cur = s->current_picture_ptr;
	out->pts=avpkt->pts;
        int i, pics, out_of_order, out_idx;

        field_end(h);

        if (cur->field_poc[0]==INT_MAX || cur->field_poc[1]==INT_MAX) {
            /* Wait for second field. */
            *data_size = 0;

        } else {
            cur->interlaced_frame = 0;
            cur->repeat_pict = 0;

            /* Signal interlacing information externally. */
            /* Prioritize picture timing SEI information over used decoding process if it exists. */

            if(h->sps.pic_struct_present_flag){
                switch (h->sei_pic_struct)
                {
                case SEI_PIC_STRUCT_FRAME:
                    break;
                case SEI_PIC_STRUCT_TOP_FIELD:
                case SEI_PIC_STRUCT_BOTTOM_FIELD:
                    cur->interlaced_frame = 1;
                    break;
                case SEI_PIC_STRUCT_TOP_BOTTOM:
                case SEI_PIC_STRUCT_BOTTOM_TOP:
                    if (FIELD_OR_MBAFF_PICTURE)
                        cur->interlaced_frame = 1;
                    else
                        // try to flag soft telecine progressive
                        cur->interlaced_frame = h->prev_interlaced_frame;
                    break;
                case SEI_PIC_STRUCT_TOP_BOTTOM_TOP:
                case SEI_PIC_STRUCT_BOTTOM_TOP_BOTTOM:
                    // Signal the possibility of telecined film externally (pic_struct 5,6)
                    // From these hints, let the applications decide if they apply deinterlacing.
                    cur->repeat_pict = 1;
                    break;
                case SEI_PIC_STRUCT_FRAME_DOUBLING:
                    // Force progressive here, as doubling interlaced frame is a bad idea.
                    cur->repeat_pict = 2;
                    break;
                case SEI_PIC_STRUCT_FRAME_TRIPLING:
                    cur->repeat_pict = 4;
                    break;
                }

                if ((h->sei_ct_type & 3) && h->sei_pic_struct <= SEI_PIC_STRUCT_BOTTOM_TOP)
                    cur->interlaced_frame = (h->sei_ct_type & (1<<1)) != 0;
            }else{
                /* Derive interlacing flag from used decoding process. */
                cur->interlaced_frame = FIELD_OR_MBAFF_PICTURE;
            }
            h->prev_interlaced_frame = cur->interlaced_frame;

            if (cur->field_poc[0] != cur->field_poc[1]){
                /* Derive top_field_first from field pocs. */
                cur->top_field_first = cur->field_poc[0] < cur->field_poc[1];
            }else{
                if(cur->interlaced_frame || h->sps.pic_struct_present_flag){
                    /* Use picture timing SEI information. Even if it is a information of a past frame, better than nothing. */
                    if(h->sei_pic_struct == SEI_PIC_STRUCT_TOP_BOTTOM
                      || h->sei_pic_struct == SEI_PIC_STRUCT_TOP_BOTTOM_TOP)
                        cur->top_field_first = 1;
                    else
                        cur->top_field_first = 0;
                }else{
                    /* Most likely progressive */
                    cur->top_field_first = 0;
                }
            }

        //FIXME do something with unavailable reference frames

            /* Sort B-frames into display order */

            if(h->sps.bitstream_restriction_flag
               && s->avctx->has_b_frames < h->sps.num_reorder_frames){
                s->avctx->has_b_frames = h->sps.num_reorder_frames;
                s->low_delay = 0;
            }

            if(   s->avctx->strict_std_compliance >= FF_COMPLIANCE_STRICT
               && !h->sps.bitstream_restriction_flag){
                s->avctx->has_b_frames= MAX_DELAYED_PIC_COUNT;
                s->low_delay= 0;
            }

            pics = 0;
            while(h->delayed_pic[pics]) pics++;

            assert(pics <= MAX_DELAYED_PIC_COUNT);

            h->delayed_pic[pics++] = cur;
            if(cur->reference == 0)
                cur->reference = DELAYED_PIC_REF;

            out = h->delayed_pic[0];
            out_idx = 0;
            for(i=1; h->delayed_pic[i] && !h->delayed_pic[i]->key_frame && !h->delayed_pic[i]->mmco_reset; i++)
                if(h->delayed_pic[i]->poc < out->poc){
                    out = h->delayed_pic[i];
                    out_idx = i;
                }
            if(s->avctx->has_b_frames == 0 && (h->delayed_pic[0]->key_frame || h->delayed_pic[0]->mmco_reset))
                h->outputed_poc= INT_MIN;
            out_of_order = out->poc < h->outputed_poc;

            if(h->sps.bitstream_restriction_flag && s->avctx->has_b_frames >= h->sps.num_reorder_frames)
                { }
            else if((out_of_order && pics-1 == s->avctx->has_b_frames && s->avctx->has_b_frames < MAX_DELAYED_PIC_COUNT)
               || (s->low_delay &&
                ((h->outputed_poc != INT_MIN && out->poc > h->outputed_poc + 2)
                 || cur->pict_type == FF_B_TYPE)))
            {
                s->low_delay = 0;
                s->avctx->has_b_frames++;
            }

            if(out_of_order || pics > s->avctx->has_b_frames){
                out->reference &= ~DELAYED_PIC_REF;
                for(i=out_idx; h->delayed_pic[i]; i++)
                    h->delayed_pic[i] = h->delayed_pic[i+1];
            }
            if(!out_of_order && pics > s->avctx->has_b_frames){
                *data_size = sizeof(AVFrame);

                if(out_idx==0 && h->delayed_pic[0] && (h->delayed_pic[0]->key_frame || h->delayed_pic[0]->mmco_reset)) {
                    h->outputed_poc = INT_MIN;
                } else
                    h->outputed_poc = out->poc;
                *pict= *(AVFrame*)out;
            }else{
                av_log(avctx, AV_LOG_DEBUG, "no picture\n");
            }
        }
    }

    assert(pict->data[0] || !*data_size);
    ff_print_debug_info(s, pict);
//printf("out %d\n", (int)pict->data[0]);
   

    return get_consumed_bytes(s, buf_index, buf_size);
}

void ff_h264_hl_decode_mb(H264Context *h){
}

#ifdef TEST
#undef printf
#undef random
#define COUNT 8000
#define SIZE (COUNT*40)
int main(void){
    int i;
    uint8_t temp[SIZE];
    PutBitContext pb;
    GetBitContext gb;
//    int int_temp[10000];
    DSPContext dsp;
    AVCodecContext avctx;

    dsputil_init(&dsp, &avctx);

    init_put_bits(&pb, temp, SIZE);
    printf("testing unsigned exp golomb\n");
    for(i=0; i<COUNT; i++){
        START_TIMER
        set_ue_golomb(&pb, i);
        STOP_TIMER("set_ue_golomb");
    }
    flush_put_bits(&pb);

    init_get_bits(&gb, temp, 8*SIZE);
    for(i=0; i<COUNT; i++){
        int j, s;

        s= show_bits(&gb, 24);

        START_TIMER
        j= get_ue_golomb(&gb);
        if(j != i){
            printf("mismatch! at %d (%d should be %d) bits:%6X\n", i, j, i, s);
//            return -1;
        }
        STOP_TIMER("get_ue_golomb");
    }


    init_put_bits(&pb, temp, SIZE);
    printf("testing signed exp golomb\n");
    for(i=0; i<COUNT; i++){
        START_TIMER
        set_se_golomb(&pb, i - COUNT/2);
        STOP_TIMER("set_se_golomb");
    }
    flush_put_bits(&pb);

    init_get_bits(&gb, temp, 8*SIZE);
    for(i=0; i<COUNT; i++){
        int j, s;

        s= show_bits(&gb, 24);

        START_TIMER
        j= get_se_golomb(&gb);
        if(j != i - COUNT/2){
            printf("mismatch! at %d (%d should be %d) bits:%6X\n", i, j, i, s);
//            return -1;
        }
        STOP_TIMER("get_se_golomb");
    }

#if 0
    printf("testing 4x4 (I)DCT\n");

    DCTELEM block[16];
    uint8_t src[16], ref[16];
    uint64_t error= 0, max_error=0;

    for(i=0; i<COUNT; i++){
        int j;
//        printf("%d %d %d\n", r1, r2, (r2-r1)*16);
        for(j=0; j<16; j++){
            ref[j]= random()%255;
            src[j]= random()%255;
        }

        h264_diff_dct_c(block, src, ref, 4);

        //normalize
        for(j=0; j<16; j++){
//            printf("%d ", block[j]);
            block[j]= block[j]*4;
            if(j&1) block[j]= (block[j]*4 + 2)/5;
            if(j&4) block[j]= (block[j]*4 + 2)/5;
        }
//        printf("\n");

        h->h264dsp.h264_idct_add(ref, block, 4);
/*        for(j=0; j<16; j++){
            printf("%d ", ref[j]);
        }
        printf("\n");*/

        for(j=0; j<16; j++){
            int diff= FFABS(src[j] - ref[j]);

            error+= diff*diff;
            max_error= FFMAX(max_error, diff);
        }
    }
    printf("error=%f max_error=%d\n", ((float)error)/COUNT/16, (int)max_error );
    printf("testing quantizer\n");
    for(qp=0; qp<52; qp++){
        for(i=0; i<16; i++)
            src1_block[i]= src2_block[i]= random()%255;

    }
    printf("Testing NAL layer\n");

    uint8_t bitstream[COUNT];
    uint8_t nal[COUNT*2];
    H264Context h;
    memset(&h, 0, sizeof(H264Context));

    for(i=0; i<COUNT; i++){
        int zeros= i;
        int nal_length;
        int consumed;
        int out_length;
        uint8_t *out;
        int j;

        for(j=0; j<COUNT; j++){
            bitstream[j]= (random() % 255) + 1;
        }

        for(j=0; j<zeros; j++){
            int pos= random() % COUNT;
            while(bitstream[pos] == 0){
                pos++;
                pos %= COUNT;
            }
            bitstream[pos]=0;
        }

        START_TIMER

        nal_length= encode_nal(&h, nal, bitstream, COUNT, COUNT*2);
        if(nal_length<0){
            printf("encoding failed\n");
            return -1;
        }

        out= ff_h264_decode_nal(&h, nal, &out_length, &consumed, nal_length);

        STOP_TIMER("NAL")

        if(out_length != COUNT){
            printf("incorrect length %d %d\n", out_length, COUNT);
            return -1;
        }

        if(consumed != nal_length){
            printf("incorrect consumed length %d %d\n", nal_length, consumed);
            return -1;
        }

        if(memcmp(bitstream, out, COUNT)){
            printf("mismatch\n");
            return -1;
        }
    }
#endif

    printf("Testing RBSP\n");


    return 0;
}
#endif /* TEST */


av_cold void ff_h264_free_context(H264Context *h)
{
    int i;

    free_tables(h); //FIXME cleanup init stuff perhaps

    for(i = 0; i < MAX_SPS_COUNT; i++)
        av_freep(h->sps_buffers + i);

    for(i = 0; i < MAX_PPS_COUNT; i++)
        av_freep(h->pps_buffers + i);
}

av_cold int ff_h264_decode_end(AVCodecContext *avctx)
{
    H264Context *h = avctx->priv_data;
    MpegEncContext *s = &h->s;

#ifdef JZC_PMON_P0
    printf("PMON ============================\n");
#endif
    ff_h264_free_context(h);
    MPV_common_end(s);
//    memset(h, 0, sizeof(H264Context));
    return 0;
}


AVCodec h264_decoder = {
    "h264",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_H264,
    sizeof(H264Context),
    ff_h264_decode_init,
    NULL,
    ff_h264_decode_end,
    decode_frame,
    /*CODEC_CAP_DRAW_HORIZ_BAND |*/ CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush= flush_dpb,
    .long_name = NULL_IF_CONFIG_SMALL("H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"),
    .pix_fmts= ff_hwaccel_pixfmt_list_420,
};

#if CONFIG_H264_VDPAU_DECODER
AVCodec h264_vdpau_decoder = {
    "h264_vdpau",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_H264,
    sizeof(H264Context),
    ff_h264_decode_init,
    NULL,
    ff_h264_decode_end,
    decode_frame,
    CODEC_CAP_DR1 | CODEC_CAP_DELAY | CODEC_CAP_HWACCEL_VDPAU,
    .flush= flush_dpb,
    .long_name = NULL_IF_CONFIG_SMALL("H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (VDPAU acceleration)"),
    .pix_fmts = (const enum PixelFormat[]){PIX_FMT_VDPAU_H264, PIX_FMT_NONE},
};
#endif

