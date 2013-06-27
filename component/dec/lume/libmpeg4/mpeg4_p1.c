/*
 * The simplest mpeg encoder (well, it was the simplest!)
 * Copyright (c) 2000,2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * 4MV & hq & B-frame encoding stuff by Michael Niedermayer <michaelni@gmx.at>
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
 * The simplest mpeg encoder (well, it was the simplest!).
 */

#ifndef JZC_P1_OPT

#include "libavutil/intmath.h"
#include "libavcore/imgutils.h"
#include "avcodec.h"
#include "dsputil.h"
#include "internal.h"
#include "mpegvideo.h"
//#include "mpegvideo_common.h"
#include "mpeg4_p1.h"
#include "mjpegenc.h"
#include "msmpeg4.h"
#include "faandct.h"
#include "xvmc_internal.h"
#include <limits.h>
#include "mpeg4.h"

#undef printf

extern MPEG4_Frame_GlbARGs *gnpFRM;
extern MPEG4_MB_DecARGs *gnpMMD;

MPEG4_Frame_GlbARGs *dFRM = NULL;
MPEG4_MB_DecARGs *dMB = NULL;

void MPV_get_buf(int num){
  dFRM = gnpFRM;
  dMB = &gnpMMD[num];
}

static void dct_unquantize_mpeg2_intra_c_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                                   DCTELEM *block, int n, int qscale)
{
  int i, level, nCoeffs;
  const uint16_t *quant_matrix;

  if(tdFRM->alternate_scan) nCoeffs= 63;
  else nCoeffs= tdMB->block_last_index[n];

  if (n < 4)
    block[0] = block[0] * tdMB->y_dc_scale;
  else
    block[0] = block[0] * tdMB->c_dc_scale;
  quant_matrix = tdFRM->intra_matrix;

  for(i=1;i<=nCoeffs;i++) {
    int j= tdFRM->permutated[i];
    level = block[j];
    if (level) {
      //printf("i=%d, j=%d, block[j]=%d, quant_matrix[j]=%d, qscale %d\n", i, j, block[j], quant_matrix[j], qscale);
      if (level < 0) {
	level = -level;
	level = (int)(level * qscale * quant_matrix[j]) >> 3;
	level = -level;
      } else {
	level = (int)(level * qscale * quant_matrix[j]) >> 3;
      }
      //printf("block[%d]=%d\n", j, level);
      block[j] = level;
    }
  }
}

static void dct_unquantize_mpeg2_inter_c_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                                   DCTELEM *block, int n, int qscale)
{               
    int i, level, nCoeffs;
    const uint16_t *quant_matrix;
    int sum=-1; 
            
    if(tdFRM->alternate_scan) nCoeffs= 63;
    else nCoeffs= tdMB->block_last_index[n];
        
    quant_matrix = tdFRM->inter_matrix;
    for(i=0; i<=nCoeffs; i++) {
        int j= tdFRM->permutated[i];
        level = block[j];
        if (level) {
            if (level < 0) {       
                level = -level;
                level = (((level << 1) + 1) * qscale *
                         ((int) (quant_matrix[j]))) >> 4;
                level = -level;
            } else {
                level = (((level << 1) + 1) * qscale *
                         ((int) (quant_matrix[j]))) >> 4;
            }
            block[j] = level;
            sum+=level;
        }
    }   
    block[63]^=sum&1;
}

static void dct_unquantize_h263_intra_c_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                                  DCTELEM *block, int n, int qscale)
{
    int i, level, qmul, qadd;
    int nCoeffs;
    //uint8_t raster_end[64];
    assert(tdMB->block_last_index[n]>=0);

    qmul = qscale << 1;

    if (!tdFRM->h263_aic) {
        if (n < 4)
            block[0] = block[0] * tdMB->y_dc_scale;
        else
            block[0] = block[0] * tdMB->c_dc_scale;
        qadd = (qscale - 1) | 1;
    }else{
        qadd = 0;
    }
    if(tdMB->ac_pred)
        nCoeffs=63;
    else
      nCoeffs= tdFRM->raster_end[ tdMB->block_last_index[n] ];
      //nCoeffs= s->inter_scantable.raster_end[ s->block_last_index[n] ];
    //printf("dct_unquantize_h263_intra_c_opt, nCoeffs %d qmul %d qadd %d\n", nCoeffs, qmul, qadd);
    for(i=1; i<=nCoeffs; i++) {
        level = block[i];
        if (level) {
            if (level < 0) {
                level = level * qmul - qadd;
            } else {
                level = level * qmul + qadd;
            }
            block[i] = level;
        }
    }
}

static void dct_unquantize_h263_inter_c_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                                  DCTELEM *block, int n, int qscale)
{
    int i, level, qmul, qadd;
    int nCoeffs;

    assert(tdMB->block_last_index[n]>=0);

    qadd = (qscale - 1) | 1;
    qmul = qscale << 1;

    nCoeffs= tdFRM->raster_end[ tdMB->block_last_index[n] ];
    //nCoeffs= s->inter_scantable.raster_end[ s->block_last_index[n] ];

    for(i=0; i<=nCoeffs; i++) {
        level = block[i];
        if (level) {
            if (level < 0) {
                level = level * qmul - qadd;
            } else {
                level = level * qmul + qadd;
            }
            block[i] = level;
        }
    }
}

static inline void add_dequant_dct_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale)
{
    if (tdMB->block_last_index[i] >= 0) {
      if (tdFRM->mpeg_quant || tdFRM->codec_id == CODEC_ID_MPEG2VIDEO){
        dct_unquantize_mpeg2_inter_c_opt(tdFRM, tdMB, block, i, qscale);
      }
      else if (tdFRM->out_format == FMT_H263 || tdFRM->out_format == FMT_H261){
        //printf("add_dequant_dct_opt FMT_H263\n");
        dct_unquantize_h263_inter_c_opt(tdFRM, tdMB, block, i, qscale);
      }

        ff_simple_idct_add(dest, line_size, block);
    }
}

static inline void put_dct_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale)
{
  if (tdFRM->mpeg_quant || tdFRM->codec_id == CODEC_ID_MPEG2VIDEO){
    dct_unquantize_mpeg2_intra_c_opt(tdFRM, tdMB, block, i, qscale);
  }
  else if (tdFRM->out_format == FMT_H263 || tdFRM->out_format == FMT_H261){
    dct_unquantize_h263_intra_c_opt(tdFRM, tdMB, block, i, qscale);
  }
  //dct_unquantize_h263_intra_c(s, block, i, qscale);
    ff_simple_idct_put(dest, line_size, block);
}

static inline void add_dct_opt(DCTELEM *block, int i, uint8_t *dest, int line_size)
{
    if (dMB->block_last_index[i] >= 0) {
        ff_simple_idct_add (dest, line_size, block);
    }
}

#define USE_IPU_THROUGH_MODE
static av_always_inline void MPV_decode_mb_internal_opt(DCTELEM block[12][64], int lowres_flag){
    int mb_x, mb_y;
    const int mb_xy = dMB->mb_y * dFRM->mb_stride + dMB->mb_x;
    static int mbnum = 0;
#ifdef HAVE_XVMC // no use
    printf("MPV_d_mb_opt HAVE_XVMC\n");
#endif

    mb_x = dMB->mb_x;
    mb_y = dMB->mb_y;
    
    mbnum++;
    
    // there is some problum
    //s->current_picture.qscale_table[mb_xy]= s->qscale;
    

    if (dFRM->h263_pred || dFRM->h263_aic){
      dFRM->mbintra_table[mb_xy]=1;
    }

    if ((dFRM->flags&CODEC_FLAG_PSNR) || !(dFRM->encoding && (dFRM->intra_only || dFRM->pict_type==B_TYPE) && dFRM->mb_decision != FF_MB_DECISION_RD)) { //FIXME precal

        uint8_t *dest_y, *dest_cb, *dest_cr;
        int dct_linesize, dct_offset;
        op_pixels_func (*op_pix)[4];
        qpel_mc_func (*op_qpix)[16];
        const int linesize= dFRM->cp_linesize[0]; //not s->linesize as this would be wrong for field pics
        const int uvlinesize= dFRM->cp_linesize[1];
        const int readable= dFRM->pict_type != B_TYPE || dFRM->encoding || dFRM->draw_horiz_band || lowres_flag;
        const int block_size= lowres_flag ? 8>>dFRM->lowres : 8;


        /* avoid copy if macroblock skipped in last frame too */
        /* skip only during decoding as we might trash the buffers during encoding a bit */
        if(!dFRM->encoding){
            uint8_t *mbskip_ptr = &dFRM->mbskip_table[mb_xy];
            //const int age= dFRM->cp_age;
            //assert(age);

	    if (dMB->mb_skipped) {
              //printf("MPV_d_mb_opt, mb_skipped\n");
                dMB->mb_skipped= 0;
                //s->mb_skipped = 0;
                assert(tdFRM->pict_type!=I_TYPE);

                (*mbskip_ptr) ++; /* indicate that this time we skipped it */
                if(*mbskip_ptr >99) *mbskip_ptr= 99;

#ifdef USE_IPU_THROUGH_MODE
#else
                /* if previous was skipped too, then nothing to do !  */
                //printf("MPV_d_mb_opt %d cp_reference %d *mbskip_ptr %d age %d\n", mbnum, dFRM->cp_reference, *mbskip_ptr, age);
                //if (*mbskip_ptr >= age && dFRM->cp_reference){
                  //printf("MPV_d_mb_opt ooo age large\n");
                    return;
		    //}
#endif
            } else if(!dFRM->cp_reference){
                (*mbskip_ptr) ++; /* increase counter so the age can be compared cleanly */
                if(*mbskip_ptr >99) *mbskip_ptr= 99;
            } else{
                *mbskip_ptr = 0; /* not skipped */
            }
        }

        dct_linesize = linesize << dMB->interlaced_dct;
        dct_offset =(dMB->interlaced_dct)? linesize : linesize*block_size;

        if(readable){
          //printf("MPV_d_mb_opt readable\n");
            dest_y=  dMB->dest[0];
            dest_cb= dMB->dest[1];
            dest_cr= dMB->dest[2];
        }else{
          //printf("MPV_d_mb_opt readable %d\n", readable);
          //printf("MPV_d_mb_opt, b_scratchpad 0x%08x, 0x%08x\n", s->b_scratchpad, dFRM->b_scratchpad);
          dest_y = dFRM->b_scratchpad;
          dest_cb= dFRM->b_scratchpad+16*linesize;
          dest_cr= dFRM->b_scratchpad+32*linesize;
        }

	//printf("MPV_d_mb_opt mb_intra %d %d\n", dMB->mb_intra, mbnum);
        if (!dMB->mb_intra) {
            /* motion handling */
            /* decoding or more than one mb_type (MC was already done otherwise) */
            if(!dFRM->encoding){
                if(lowres_flag){
                  printf("MPV_d_mb lowres_flag\n");
                }else{
                  //printf("MPV_d_mb_opt mv_dir %d %d\n", dMB->mv_dir, mbnum);
                    op_qpix= dFRM->qpel_put;
                    if ((!dFRM->no_rounding) || dFRM->pict_type==B_TYPE){
                        op_pix = dFRM->put_pixels_tab;
                    }else{
                        op_pix = dFRM->put_no_rnd_pixels_tab;
                    }
                    if (dMB->mv_dir & MV_DIR_FORWARD) {
                        MPV_motion_p1(dFRM, dMB,  dest_y, dest_cb, dest_cr, 0, dFRM->last_picture_data, op_pix, op_qpix);
                        op_pix = dFRM->avg_pixels_tab;
                        op_qpix= dFRM->qpel_avg;
                    }
                    if (dMB->mv_dir & MV_DIR_BACKWARD) {
                        MPV_motion_p1(dFRM, dMB, dest_y, dest_cb, dest_cr, 1, dFRM->next_picture_data, op_pix, op_qpix);
                    }
                }
            }

#if 1
            /* skip dequant / idct if we are really late ;) */
    //printf("MPV_d_mb_opt hurry_up %d %d %d\n", s->hurry_up, dFRM->hurry_up, s->avctx->skip_idct);
            if(dFRM->hurry_up>1) goto skip_idct;
            if(dMB->skip_idct){
                if(  (dMB->skip_idct >= AVDISCARD_NONREF && dFRM->pict_type == B_TYPE)
                   ||(dMB->skip_idct >= AVDISCARD_NONKEY && dFRM->pict_type != I_TYPE)
                   || dMB->skip_idct >= AVDISCARD_ALL)
                    goto skip_idct;
            }
#endif

	    /* add dct residue */
            if(dFRM->encoding || !(   dFRM->h263_msmpeg4 || dFRM->codec_id==CODEC_ID_MPEG1VIDEO || dFRM->codec_id==CODEC_ID_MPEG2VIDEO || (dFRM->codec_id==CODEC_ID_MPEG4 && !dFRM->mpeg_quant))){
              add_dequant_dct_opt(dFRM, dMB, block[0], 0, dest_y                          , dct_linesize, dMB->qscale);
              add_dequant_dct_opt(dFRM, dMB, block[1], 1, dest_y              + block_size, dct_linesize, dMB->qscale);
              add_dequant_dct_opt(dFRM, dMB, block[2], 2, dest_y + dct_offset             , dct_linesize, dMB->qscale);
              add_dequant_dct_opt(dFRM, dMB, block[3], 3, dest_y + dct_offset + block_size, dct_linesize, dMB->qscale);

                if(!CONFIG_GRAY || !(dFRM->flags&CODEC_FLAG_GRAY)){
                    if (dFRM->chroma_y_shift){
                      add_dequant_dct_opt(dFRM, dMB, block[4], 4, dest_cb, uvlinesize, dMB->chroma_qscale);
                      add_dequant_dct_opt(dFRM, dMB, block[5], 5, dest_cr, uvlinesize, dMB->chroma_qscale);
                    }else{
                        dct_linesize >>= 1;
                        dct_offset >>=1;
                        add_dequant_dct_opt(dFRM, dMB, block[4], 4, dest_cb,              dct_linesize, dMB->chroma_qscale);
                        add_dequant_dct_opt(dFRM, dMB, block[5], 5, dest_cr,              dct_linesize, dMB->chroma_qscale);
                        add_dequant_dct_opt(dFRM, dMB, block[6], 6, dest_cb + dct_offset, dct_linesize, dMB->chroma_qscale);
                        add_dequant_dct_opt(dFRM, dMB, block[7], 7, dest_cr + dct_offset, dct_linesize, dMB->chroma_qscale);
                    }
                }
            } else if(dFRM->codec_id != CODEC_ID_WMV2){
                add_dct_opt(block[0], 0, dest_y                          , dct_linesize);
                add_dct_opt(block[1], 1, dest_y              + block_size, dct_linesize);
                add_dct_opt(block[2], 2, dest_y + dct_offset             , dct_linesize);
                add_dct_opt(block[3], 3, dest_y + dct_offset + block_size, dct_linesize);

                if(!CONFIG_GRAY || !(dFRM->flags&CODEC_FLAG_GRAY)){
                    if(dFRM->chroma_y_shift){//Chroma420
                        add_dct_opt(block[4], 4, dest_cb, uvlinesize);
                        add_dct_opt(block[5], 5, dest_cr, uvlinesize);
                    }else{
                        //chroma422
                        dct_linesize = uvlinesize << dMB->interlaced_dct;
                        dct_offset =(dMB->interlaced_dct)? uvlinesize : uvlinesize*8;

                        add_dct_opt(block[4], 4, dest_cb, dct_linesize);
                        add_dct_opt(block[5], 5, dest_cr, dct_linesize);
                        add_dct_opt(block[6], 6, dest_cb+dct_offset, dct_linesize);
                        add_dct_opt(block[7], 7, dest_cr+dct_offset, dct_linesize);
                        if(!dFRM->chroma_x_shift){//Chroma444
                            add_dct_opt(block[8], 8, dest_cb+8, dct_linesize);
                            add_dct_opt(block[9], 9, dest_cr+8, dct_linesize);
                            add_dct_opt(block[10], 10, dest_cb+8+dct_offset, dct_linesize);
                            add_dct_opt(block[11], 11, dest_cr+8+dct_offset, dct_linesize);
                        }
		    }
                }//fi gray
            }
            else if (CONFIG_WMV2_DECODER || CONFIG_WMV2_ENCODER) {
              //ff_wmv2_add_mb(s, block, dest_y, dest_cb, dest_cr);
              printf("MPV_d_mb_p1, WMV2 do not support\n");
            }
        } else {
            /* dct only in intra block */
            if(dFRM->encoding || !(dFRM->codec_id==CODEC_ID_MPEG1VIDEO || dFRM->codec_id==CODEC_ID_MPEG2VIDEO)){
              //put_dct_opt(s, block[0], 0, dest_y                          , dct_linesize, dMB->qscale);
              put_dct_opt(dFRM, dMB, block[0], 0, dest_y                          , dct_linesize, dMB->qscale);
                put_dct_opt(dFRM, dMB, block[1], 1, dest_y              + block_size, dct_linesize, dMB->qscale);
                put_dct_opt(dFRM, dMB, block[2], 2, dest_y + dct_offset             , dct_linesize, dMB->qscale);
                put_dct_opt(dFRM, dMB, block[3], 3, dest_y + dct_offset + block_size, dct_linesize, dMB->qscale);
                if(!CONFIG_GRAY || !(dFRM->flags&CODEC_FLAG_GRAY)){
                    if(dFRM->chroma_y_shift){
                      put_dct_opt(dFRM, dMB, block[4], 4, dest_cb, uvlinesize, dMB->chroma_qscale);
                      put_dct_opt(dFRM, dMB, block[5], 5, dest_cr, uvlinesize, dMB->chroma_qscale);
                    }else{
                        dct_offset >>=1;
                        dct_linesize >>=1;
                        put_dct_opt(dFRM, dMB, block[4], 4, dest_cb,              dct_linesize, dMB->chroma_qscale);
                        put_dct_opt(dFRM, dMB, block[5], 5, dest_cr,              dct_linesize, dMB->chroma_qscale);
                        put_dct_opt(dFRM, dMB, block[6], 6, dest_cb + dct_offset, dct_linesize, dMB->chroma_qscale);
                        put_dct_opt(dFRM, dMB, block[7], 7, dest_cr + dct_offset, dct_linesize, dMB->chroma_qscale);
                    }
                }
            }else{
                ff_simple_idct_put(dest_y                          , dct_linesize, block[0]);
                ff_simple_idct_put(dest_y              + block_size, dct_linesize, block[1]);
                ff_simple_idct_put(dest_y + dct_offset             , dct_linesize, block[2]);
                ff_simple_idct_put(dest_y + dct_offset + block_size, dct_linesize, block[3]);

                if(!CONFIG_GRAY || !(dFRM->flags&CODEC_FLAG_GRAY)){
                    if(dFRM->chroma_y_shift){
                        ff_simple_idct_put(dest_cb, uvlinesize, block[4]);
                        ff_simple_idct_put(dest_cr, uvlinesize, block[5]);
                    }else{
		      dct_linesize = uvlinesize << dMB->interlaced_dct;
                        dct_offset =(dMB->interlaced_dct)? uvlinesize : uvlinesize*8;

                        ff_simple_idct_put(dest_cb,              dct_linesize, block[4]);
                        ff_simple_idct_put(dest_cr,              dct_linesize, block[5]);
                        ff_simple_idct_put(dest_cb + dct_offset, dct_linesize, block[6]);
                        ff_simple_idct_put(dest_cr + dct_offset, dct_linesize, block[7]);
                        if(!dFRM->chroma_x_shift){//Chroma444
                            ff_simple_idct_put(dest_cb + 8,              dct_linesize, block[8]);
                            ff_simple_idct_put(dest_cr + 8,              dct_linesize, block[9]);
                            ff_simple_idct_put(dest_cb + 8 + dct_offset, dct_linesize, block[10]);
                            ff_simple_idct_put(dest_cr + 8 + dct_offset, dct_linesize, block[11]);
                        }
                    }
                }//gray
            }
        }

skip_idct:
        if(!readable){
            dFRM->put_pixels_tab[0][0](dMB->dest[0], dest_y ,   linesize,16);
            dFRM->put_pixels_tab[dFRM->chroma_x_shift][0](dMB->dest[1], dest_cb, uvlinesize,16 >> dFRM->chroma_y_shift);
            dFRM->put_pixels_tab[dFRM->chroma_x_shift][0](dMB->dest[2], dest_cr, uvlinesize,16 >> dFRM->chroma_y_shift);
        }
    }
}

void MPV_decode_mb_p1(void){

  if(dFRM->lowres) 
    MPV_decode_mb_internal_opt(dMB->block, 1);
  else 
    MPV_decode_mb_internal_opt(dMB->block, 0);
}
#endif //JZC_P1_OPT
