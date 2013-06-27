/*
 * Copyright (c) 2003 The FFmpeg Project
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

/*
 * How to use this decoder:
 * SVQ3 data is transported within Apple Quicktime files. Quicktime files
 * have stsd atoms to describe media trak properties. A stsd atom for a
 * video trak contains 1 or more ImageDescription atoms. These atoms begin
 * with the 4-byte length of the atom followed by the codec fourcc. Some
 * decoders need information in this atom to operate correctly. Such
 * is the case with SVQ3. In order to get the best use out of this decoder,
 * the calling app must make the SVQ3 ImageDescription atom available
 * via the AVCodecContext's extradata[_size] field:
 *
 * AVCodecContext.extradata = pointer to ImageDescription, first characters
 * are expected to be 'S', 'V', 'Q', and '3', NOT the 4-byte atom length
 * AVCodecContext.extradata_size = size of ImageDescription atom memory
 * buffer (which will be the same as the ImageDescription atom size field
 * from the QT file, minus 4 bytes since the length is missing)
 *
 * You will know you have these parameters passed correctly when the decoder
 * correctly decodes this file:
 *  http://samples.mplayerhq.hu/V-codecs/SVQ3/Vertical400kbit.sorenson3.mov
 */
#include "internal.h"
#include "dsputil.h"
#include "avcodec.h"
#include "mpegvideo.h"
#include "h264.h"

#include "h264data.h" //FIXME FIXME FIXME

#include "h264_mvpred.h"
#include "golomb.h"
#include "rectangle.h"
#include "vdpau_internal.h"

#include "utils/Log.h"
#if CONFIG_ZLIB
#include <zlib.h>
#endif

#include "svq1.h"

/**
 * @file
 * svq3 decoder.
 */

#define FULLPEL_MODE  1
#define HALFPEL_MODE  2
#define THIRDPEL_MODE 3
#define PREDICT_MODE  4

/* dual scan (from some older h264 draft)
 o-->o-->o   o
         |  /|
 o   o   o / o
 | / |   |/  |
 o   o   o   o
   /
 o-->o-->o-->o
*/
static const uint8_t svq3_scan[16] = {
    0+0*4, 1+0*4, 2+0*4, 2+1*4,
    2+2*4, 3+0*4, 3+1*4, 3+2*4,
    0+1*4, 0+2*4, 1+1*4, 1+2*4,
    0+3*4, 1+3*4, 2+3*4, 3+3*4,
};

static const uint8_t svq3_pred_0[25][2] = {
    { 0, 0 },
    { 1, 0 }, { 0, 1 },
    { 0, 2 }, { 1, 1 }, { 2, 0 },
    { 3, 0 }, { 2, 1 }, { 1, 2 }, { 0, 3 },
    { 0, 4 }, { 1, 3 }, { 2, 2 }, { 3, 1 }, { 4, 0 },
    { 4, 1 }, { 3, 2 }, { 2, 3 }, { 1, 4 },
    { 2, 4 }, { 3, 3 }, { 4, 2 },
    { 4, 3 }, { 3, 4 },
    { 4, 4 }
};

static const int8_t svq3_pred_1[6][6][5] = {
    { { 2,-1,-1,-1,-1 }, { 2, 1,-1,-1,-1 }, { 1, 2,-1,-1,-1 },
      { 2, 1,-1,-1,-1 }, { 1, 2,-1,-1,-1 }, { 1, 2,-1,-1,-1 } },
    { { 0, 2,-1,-1,-1 }, { 0, 2, 1, 4, 3 }, { 0, 1, 2, 4, 3 },
      { 0, 2, 1, 4, 3 }, { 2, 0, 1, 3, 4 }, { 0, 4, 2, 1, 3 } },
    { { 2, 0,-1,-1,-1 }, { 2, 1, 0, 4, 3 }, { 1, 2, 4, 0, 3 },
      { 2, 1, 0, 4, 3 }, { 2, 1, 4, 3, 0 }, { 1, 2, 4, 0, 3 } },
    { { 2, 0,-1,-1,-1 }, { 2, 0, 1, 4, 3 }, { 1, 2, 0, 4, 3 },
      { 2, 1, 0, 4, 3 }, { 2, 1, 3, 4, 0 }, { 2, 4, 1, 0, 3 } },
    { { 0, 2,-1,-1,-1 }, { 0, 2, 1, 3, 4 }, { 1, 2, 3, 0, 4 },
      { 2, 0, 1, 3, 4 }, { 2, 1, 3, 0, 4 }, { 2, 0, 4, 3, 1 } },
    { { 0, 2,-1,-1,-1 }, { 0, 2, 4, 1, 3 }, { 1, 4, 2, 0, 3 },
      { 4, 2, 0, 1, 3 }, { 2, 0, 1, 4, 3 }, { 4, 2, 1, 0, 3 } },
};

static const struct { uint8_t run; uint8_t level; } svq3_dct_tables[2][16] = {
    { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 }, { 0, 2 }, { 3, 1 }, { 4, 1 }, { 5, 1 },
      { 0, 3 }, { 1, 2 }, { 2, 2 }, { 6, 1 }, { 7, 1 }, { 8, 1 }, { 9, 1 }, { 0, 4 } },
    { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 }, { 2, 1 }, { 0, 3 }, { 0, 4 }, { 0, 5 },
      { 3, 1 }, { 4, 1 }, { 1, 2 }, { 1, 3 }, { 0, 6 }, { 0, 7 }, { 0, 8 }, { 0, 9 } }
};

static const uint32_t svq3_dequant_coeff[32] = {
     3881,  4351,  4890,  5481,  6154,  6914,  7761,  8718,
     9781, 10987, 12339, 13828, 15523, 17435, 19561, 21873,
    24552, 27656, 30847, 34870, 38807, 43747, 49103, 54683,
    61694, 68745, 77615, 89113,100253,109366,126635,141533
};

/* ********************************** H264 Func Start *************************************** */
static const uint8_t rem6[52]={
0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3,
};

static const uint8_t div6[52]={
0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
};

static void ff_svq3_write_back_intra_pred_mode(H264Context *h){
    int8_t *mode= h->intra4x4_pred_mode + h->mb2br_xy[h->mb_xy];

    AV_COPY32(mode, h->intra4x4_pred_mode_cache + 4 + 8*4);
    mode[4]= h->intra4x4_pred_mode_cache[7+8*3];
    mode[5]= h->intra4x4_pred_mode_cache[7+8*2];
    mode[6]= h->intra4x4_pred_mode_cache[7+8*1];
}

/**
 * checks if the top & left blocks are available if needed & changes the dc mode so it only uses the available blocks.
 */
static int ff_svq3_check_intra4x4_pred_mode(H264Context *h){
    MpegEncContext * const s = &h->s;
    static const int8_t top [12]= {-1, 0,LEFT_DC_PRED,-1,-1,-1,-1,-1, 0};
    static const int8_t left[12]= { 0,-1, TOP_DC_PRED, 0,-1,-1,-1, 0,-1,DC_128_PRED};
    int i;

    if(!(h->top_samples_available&0x8000)){
        for(i=0; i<4; i++){
            int status= top[ h->intra4x4_pred_mode_cache[scan8[0] + i] ];
            if(status<0){
                av_log(h->s.avctx, AV_LOG_ERROR, "top block unavailable for requested intra4x4 mode %d at %d %d\n", status, s->mb_x, s->mb_y);
                return -1;
            } else if(status){
                h->intra4x4_pred_mode_cache[scan8[0] + i]= status;
            }
        }
    }

    if((h->left_samples_available&0x8888)!=0x8888){
        static const int mask[4]={0x8000,0x2000,0x80,0x20};
        for(i=0; i<4; i++){
            if(!(h->left_samples_available&mask[i])){
                int status= left[ h->intra4x4_pred_mode_cache[scan8[0] + 8*i] ];
                if(status<0){
                    av_log(h->s.avctx, AV_LOG_ERROR, "left block unavailable for requested intra4x4 mode %d at %d %d\n", status, s->mb_x, s->mb_y);
                    return -1;
                } else if(status){
                    h->intra4x4_pred_mode_cache[scan8[0] + 8*i]= status;
                }
            }
        }
    }

    return 0;
} //FIXME cleanup like ff_h264_check_intra_pred_mode

/**
 * checks if the top & left blocks are available if needed & changes the dc mode so it only uses the available blocks.
 */
static int ff_svq3_check_intra_pred_mode(H264Context *h, int mode){
    MpegEncContext * const s = &h->s;
    static const int8_t top [7]= {LEFT_DC_PRED8x8, 1,-1,-1};
    static const int8_t left[7]= { TOP_DC_PRED8x8,-1, 2,-1,DC_128_PRED8x8};

    if(mode > 6U) {
        av_log(h->s.avctx, AV_LOG_ERROR, "out of range intra chroma pred mode at %d %d\n", s->mb_x, s->mb_y);
        return -1;
    }

    if(!(h->top_samples_available&0x8000)){
        mode= top[ mode ];
        if(mode<0){
            av_log(h->s.avctx, AV_LOG_ERROR, "top block unavailable for requested intra mode at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
    }

    if((h->left_samples_available&0x8080) != 0x8080){
        mode= left[ mode ];
        if(h->left_samples_available&0x8080){ //mad cow disease mode, aka MBAFF + constrained_intra_pred
            mode= ALZHEIMER_DC_L0T_PRED8x8 + (!(h->left_samples_available&0x8000)) + 2*(mode == DC_128_PRED8x8);
        }
        if(mode<0){
            av_log(h->s.avctx, AV_LOG_ERROR, "left block unavailable for requested intra mode at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
    }

    return mode;
}
/* ********************************** H264 Func End *************************************** */

void ff_svq3_luma_dc_dequant_idct_c(DCTELEM *block, int qp)
{
    const int qmul = svq3_dequant_coeff[qp];
#define stride 16
    int i;
    int temp[16];
    static const int x_offset[4] = {0, 1*stride, 4* stride,  5*stride};
    static const int y_offset[4] = {0, 2*stride, 8* stride, 10*stride};

    for (i = 0; i < 4; i++){
        const int offset = y_offset[i];
        const int z0 = 13*(block[offset+stride*0] +    block[offset+stride*4]);
        const int z1 = 13*(block[offset+stride*0] -    block[offset+stride*4]);
        const int z2 =  7* block[offset+stride*1] - 17*block[offset+stride*5];
        const int z3 = 17* block[offset+stride*1] +  7*block[offset+stride*5];

        temp[4*i+0] = z0+z3;
        temp[4*i+1] = z1+z2;
        temp[4*i+2] = z1-z2;
        temp[4*i+3] = z0-z3;
    }

    for (i = 0; i < 4; i++){
        const int offset = x_offset[i];
        const int z0 = 13*(temp[4*0+i] +    temp[4*2+i]);
        const int z1 = 13*(temp[4*0+i] -    temp[4*2+i]);
        const int z2 =  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3 = 17* temp[4*1+i] +  7*temp[4*3+i];

        block[stride*0 +offset] = ((z0 + z3)*qmul + 0x80000) >> 20;
        block[stride*2 +offset] = ((z1 + z2)*qmul + 0x80000) >> 20;
        block[stride*8 +offset] = ((z1 - z2)*qmul + 0x80000) >> 20;
        block[stride*10+offset] = ((z0 - z3)*qmul + 0x80000) >> 20;
    }
}
#undef stride

void ff_svq3_add_idct_c(uint8_t *dst, DCTELEM *block, int stride, int qp,
                            int dc)
{
    const int qmul = svq3_dequant_coeff[qp];
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    if (dc) {
        dc = 13*13*((dc == 1) ? 1538*block[0] : ((qmul*(block[0] >> 3)) / 2));
        block[0] = 0;
    }

    for (i = 0; i < 4; i++) {
        const int z0 = 13*(block[0 + 4*i] +    block[2 + 4*i]);
        const int z1 = 13*(block[0 + 4*i] -    block[2 + 4*i]);
        const int z2 =  7* block[1 + 4*i] - 17*block[3 + 4*i];
        const int z3 = 17* block[1 + 4*i] +  7*block[3 + 4*i];

        block[0 + 4*i] = z0 + z3;
        block[1 + 4*i] = z1 + z2;
        block[2 + 4*i] = z1 - z2;
        block[3 + 4*i] = z0 - z3;
    }

    for (i = 0; i < 4; i++) {
        const int z0 = 13*(block[i + 4*0] +    block[i + 4*2]);
        const int z1 = 13*(block[i + 4*0] -    block[i + 4*2]);
        const int z2 =  7* block[i + 4*1] - 17*block[i + 4*3];
        const int z3 = 17* block[i + 4*1] +  7*block[i + 4*3];
        const int rr = (dc + 0x80000);

        dst[i + stride*0] = cm[ dst[i + stride*0] + (((z0 + z3)*qmul + rr) >> 20) ];
        dst[i + stride*1] = cm[ dst[i + stride*1] + (((z1 + z2)*qmul + rr) >> 20) ];
        dst[i + stride*2] = cm[ dst[i + stride*2] + (((z1 - z2)*qmul + rr) >> 20) ];
        dst[i + stride*3] = cm[ dst[i + stride*3] + (((z0 - z3)*qmul + rr) >> 20) ];
    }
}

static inline int svq3_decode_block(GetBitContext *gb, DCTELEM *block,
                                    int index, const int type)
{
    static const uint8_t *const scan_patterns[4] =
    { luma_dc_zigzag_scan, zigzag_scan, svq3_scan, chroma_dc_scan };

    int run, level, sign, vlc, limit;
    const int intra = (3 * type) >> 2;
    const uint8_t *const scan = scan_patterns[type];

    for (limit = (16 >> intra); index < 16; index = limit, limit += 8) {
        for (; (vlc = svq3_get_ue_golomb(gb)) != 0; index++) {

          if (vlc == INVALID_VLC)
              return -1;

          sign = (vlc & 0x1) - 1;
          vlc  = (vlc + 1) >> 1;

          if (type == 3) {
              if (vlc < 3) {
                  run   = 0;
                  level = vlc;
              } else if (vlc < 4) {
                  run   = 1;
                  level = 1;
              } else {
                  run   = (vlc & 0x3);
                  level = ((vlc + 9) >> 2) - run;
              }
          } else {
              if (vlc < 16) {
                  run   = svq3_dct_tables[intra][vlc].run;
                  level = svq3_dct_tables[intra][vlc].level;
              } else if (intra) {
                  run   = (vlc & 0x7);
                  level = (vlc >> 3) + ((run == 0) ? 8 : ((run < 2) ? 2 : ((run < 5) ? 0 : -1)));
              } else {
                  run   = (vlc & 0xF);
                  level = (vlc >> 4) + ((run == 0) ? 4 : ((run < 3) ? 2 : ((run < 10) ? 1 : 0)));
              }
          }

          if ((index += run) >= limit)
              return -1;

          block[scan[index]] = (level ^ sign) - sign;
        }

        if (type != 2) {
            break;
        }
    }

    return 0;
}

static inline void svq3_mc_dir_part(MpegEncContext *s,
                                    int x, int y, int width, int height,
                                    int mx, int my, int dxy,
                                    int thirdpel, int dir, int avg)
{
    const Picture *pic = (dir == 0) ? &s->last_picture : &s->next_picture;
    uint8_t *src, *dest;
    int i, emu = 0;
    int blocksize = 2 - (width>>3); //16->0, 8->1, 4->2

    mx += x;
    my += y;

    if (mx < 0 || mx >= (s->h_edge_pos - width  - 1) ||
        my < 0 || my >= (s->v_edge_pos - height - 1)) {

        if ((s->flags & CODEC_FLAG_EMU_EDGE)) {
            emu = 1;
        }

        mx = av_clip (mx, -16, (s->h_edge_pos - width  + 15));
        my = av_clip (my, -16, (s->v_edge_pos - height + 15));
    }

    /* form component predictions */
    dest = s->current_picture.data[0] + x + y*s->linesize;
    src  = pic->data[0] + mx + my*s->linesize;

    if (emu) {
        ff_emulated_edge_mc(s->edge_emu_buffer, src, s->linesize, (width + 1), (height + 1),
                            mx, my, s->h_edge_pos, s->v_edge_pos);
        src = s->edge_emu_buffer;
    }
    if (thirdpel)
        (avg ? s->dsp.avg_tpel_pixels_tab : s->dsp.put_tpel_pixels_tab)[dxy](dest, src, s->linesize, width, height);
    else
        (avg ? s->dsp.avg_pixels_tab : s->dsp.put_pixels_tab)[blocksize][dxy](dest, src, s->linesize, height);

    if (!(s->flags & CODEC_FLAG_GRAY)) {
        mx     = (mx + (mx < (int) x)) >> 1;
        my     = (my + (my < (int) y)) >> 1;
        width  = (width  >> 1);
        height = (height >> 1);
        blocksize++;

        for (i = 1; i < 3; i++) {
            dest = s->current_picture.data[i] + (x >> 1) + (y >> 1)*s->uvlinesize;
            src  = pic->data[i] + mx + my*s->uvlinesize;

            if (emu) {
                ff_emulated_edge_mc(s->edge_emu_buffer, src, s->uvlinesize, (width + 1), (height + 1),
                                    mx, my, (s->h_edge_pos >> 1), (s->v_edge_pos >> 1));
                src = s->edge_emu_buffer;
            }
            if (thirdpel)
                (avg ? s->dsp.avg_tpel_pixels_tab : s->dsp.put_tpel_pixels_tab)[dxy](dest, src, s->uvlinesize, width, height);
            else
                (avg ? s->dsp.avg_pixels_tab : s->dsp.put_pixels_tab)[blocksize][dxy](dest, src, s->uvlinesize, height);
        }
    }
}

static inline int svq3_mc_dir(H264Context *h, int size, int mode, int dir,
                              int avg)
{
    int i, j, k, mx, my, dx, dy, x, y;
    MpegEncContext *const s = (MpegEncContext *) h;
    const int part_width  = ((size & 5) == 4) ? 4 : 16 >> (size & 1);
    const int part_height = 16 >> ((unsigned) (size + 1) / 3);
    const int extra_width = (mode == PREDICT_MODE) ? -16*6 : 0;
    const int h_edge_pos  = 6*(s->h_edge_pos - part_width ) - extra_width;
    const int v_edge_pos  = 6*(s->v_edge_pos - part_height) - extra_width;

    for (i = 0; i < 16; i += part_height) {
        for (j = 0; j < 16; j += part_width) {
            const int b_xy = (4*s->mb_x + (j >> 2)) + (4*s->mb_y + (i >> 2))*h->b_stride;
            int dxy;
            x = 16*s->mb_x + j;
            y = 16*s->mb_y + i;
            k = ((j >> 2) & 1) + ((i >> 1) & 2) + ((j >> 1) & 4) + (i & 8);

            if (mode != PREDICT_MODE) {
                pred_motion(h, k, (part_width >> 2), dir, 1, &mx, &my);
            } else {
                mx = s->next_picture.motion_val[0][b_xy][0]<<1;
                my = s->next_picture.motion_val[0][b_xy][1]<<1;

                if (dir == 0) {
                    mx = ((mx * h->frame_num_offset) / h->prev_frame_num_offset + 1) >> 1;
                    my = ((my * h->frame_num_offset) / h->prev_frame_num_offset + 1) >> 1;
                } else {
                    mx = ((mx * (h->frame_num_offset - h->prev_frame_num_offset)) / h->prev_frame_num_offset + 1) >> 1;
                    my = ((my * (h->frame_num_offset - h->prev_frame_num_offset)) / h->prev_frame_num_offset + 1) >> 1;
                }
            }

            /* clip motion vector prediction to frame border */
            mx = av_clip(mx, extra_width - 6*x, h_edge_pos - 6*x);
            my = av_clip(my, extra_width - 6*y, v_edge_pos - 6*y);

            /* get (optional) motion vector differential */
            if (mode == PREDICT_MODE) {
                dx = dy = 0;
            } else {
                dy = svq3_get_se_golomb(&s->gb);
                dx = svq3_get_se_golomb(&s->gb);

                if (dx == INVALID_VLC || dy == INVALID_VLC) {
                    av_log(h->s.avctx, AV_LOG_ERROR, "invalid MV vlc\n");
                    return -1;
                }
            }

            /* compute motion vector */
            if (mode == THIRDPEL_MODE) {
                int fx, fy;
                mx  = ((mx + 1)>>1) + dx;
                my  = ((my + 1)>>1) + dy;
                fx  = ((unsigned)(mx + 0x3000))/3 - 0x1000;
                fy  = ((unsigned)(my + 0x3000))/3 - 0x1000;
                dxy = (mx - 3*fx) + 4*(my - 3*fy);

                svq3_mc_dir_part(s, x, y, part_width, part_height, fx, fy, dxy, 1, dir, avg);
                mx += mx;
                my += my;
            } else if (mode == HALFPEL_MODE || mode == PREDICT_MODE) {
                mx  = ((unsigned)(mx + 1 + 0x3000))/3 + dx - 0x1000;
                my  = ((unsigned)(my + 1 + 0x3000))/3 + dy - 0x1000;
                dxy = (mx&1) + 2*(my&1);

                svq3_mc_dir_part(s, x, y, part_width, part_height, mx>>1, my>>1, dxy, 0, dir, avg);
                mx *= 3;
                my *= 3;
            } else {
                mx = ((unsigned)(mx + 3 + 0x6000))/6 + dx - 0x1000;
                my = ((unsigned)(my + 3 + 0x6000))/6 + dy - 0x1000;

                svq3_mc_dir_part(s, x, y, part_width, part_height, mx, my, 0, 0, dir, avg);
                mx *= 6;
                my *= 6;
            }

            /* update mv_cache */
            if (mode != PREDICT_MODE) {
                int32_t mv = pack16to32(mx,my);

                if (part_height == 8 && i < 8) {
                    *(int32_t *) h->mv_cache[dir][scan8[k] + 1*8] = mv;

                    if (part_width == 8 && j < 8) {
                        *(int32_t *) h->mv_cache[dir][scan8[k] + 1 + 1*8] = mv;
                    }
                }
                if (part_width == 8 && j < 8) {
                    *(int32_t *) h->mv_cache[dir][scan8[k] + 1] = mv;
                }
                if (part_width == 4 || part_height == 4) {
                    *(int32_t *) h->mv_cache[dir][scan8[k]] = mv;
                }
            }

            /* write back motion vectors */
            fill_rectangle(s->current_picture.motion_val[dir][b_xy], part_width>>2, part_height>>2, h->b_stride, pack16to32(mx,my), 4);
        }
    }

    return 0;
}

static int svq3_decode_mb(H264Context *h, unsigned int mb_type)
{
    int i, j, k, m, dir, mode;
    int cbp = 0;
    uint32_t vlc;
    int8_t *top, *left;
    MpegEncContext *const s = (MpegEncContext *) h;
    const int mb_xy = h->mb_xy;
    const int b_xy  = 4*s->mb_x + 4*s->mb_y*h->b_stride;

    h->top_samples_available      = (s->mb_y == 0) ? 0x33FF : 0xFFFF;
    h->left_samples_available     = (s->mb_x == 0) ? 0x5F5F : 0xFFFF;
    h->topright_samples_available = 0xFFFF;

    if (mb_type == 0) {           /* SKIP */
        if (s->pict_type == FF_P_TYPE || s->next_picture.mb_type[mb_xy] == -1) {
            svq3_mc_dir_part(s, 16*s->mb_x, 16*s->mb_y, 16, 16, 0, 0, 0, 0, 0, 0);

            if (s->pict_type == FF_B_TYPE) {
                svq3_mc_dir_part(s, 16*s->mb_x, 16*s->mb_y, 16, 16, 0, 0, 0, 0, 1, 1);
            }

            mb_type = MB_TYPE_SKIP;
        } else {
            mb_type = FFMIN(s->next_picture.mb_type[mb_xy], 6);
            if (svq3_mc_dir(h, mb_type, PREDICT_MODE, 0, 0) < 0)
                return -1;
            if (svq3_mc_dir(h, mb_type, PREDICT_MODE, 1, 1) < 0)
                return -1;

            mb_type = MB_TYPE_16x16;
        }
    } else if (mb_type < 8) {     /* INTER */
        if (h->thirdpel_flag && h->halfpel_flag == !get_bits1 (&s->gb)) {
            mode = THIRDPEL_MODE;
        } else if (h->halfpel_flag && h->thirdpel_flag == !get_bits1 (&s->gb)) {
            mode = HALFPEL_MODE;
        } else {
            mode = FULLPEL_MODE;
        }

        /* fill caches */
        /* note ref_cache should contain here:
            ????????
            ???11111
            N??11111
            N??11111
            N??11111
        */

        for (m = 0; m < 2; m++) {
            if (s->mb_x > 0 && h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - 1]+6] != -1) {
                for (i = 0; i < 4; i++) {
                    *(uint32_t *) h->mv_cache[m][scan8[0] - 1 + i*8] = *(uint32_t *) s->current_picture.motion_val[m][b_xy - 1 + i*h->b_stride];
                }
            } else {
                for (i = 0; i < 4; i++) {
                    *(uint32_t *) h->mv_cache[m][scan8[0] - 1 + i*8] = 0;
                }
            }
            if (s->mb_y > 0) {
                memcpy(h->mv_cache[m][scan8[0] - 1*8], s->current_picture.motion_val[m][b_xy - h->b_stride], 4*2*sizeof(int16_t));
                memset(&h->ref_cache[m][scan8[0] - 1*8], (h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride]] == -1) ? PART_NOT_AVAILABLE : 1, 4);

                if (s->mb_x < (s->mb_width - 1)) {
                    *(uint32_t *) h->mv_cache[m][scan8[0] + 4 - 1*8] = *(uint32_t *) s->current_picture.motion_val[m][b_xy - h->b_stride + 4];
                    h->ref_cache[m][scan8[0] + 4 - 1*8] =
                        (h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride + 1]+6] == -1 ||
                         h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride    ]  ] == -1) ? PART_NOT_AVAILABLE : 1;
                }else
                    h->ref_cache[m][scan8[0] + 4 - 1*8] = PART_NOT_AVAILABLE;
                if (s->mb_x > 0) {
                    *(uint32_t *) h->mv_cache[m][scan8[0] - 1 - 1*8] = *(uint32_t *) s->current_picture.motion_val[m][b_xy - h->b_stride - 1];
                    h->ref_cache[m][scan8[0] - 1 - 1*8] = (h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride - 1]+3] == -1) ? PART_NOT_AVAILABLE : 1;
                }else
                    h->ref_cache[m][scan8[0] - 1 - 1*8] = PART_NOT_AVAILABLE;
            }else
                memset(&h->ref_cache[m][scan8[0] - 1*8 - 1], PART_NOT_AVAILABLE, 8);

            if (s->pict_type != FF_B_TYPE)
                break;
        }

        /* decode motion vector(s) and form prediction(s) */
        if (s->pict_type == FF_P_TYPE) {
            if (svq3_mc_dir(h, (mb_type - 1), mode, 0, 0) < 0)
                return -1;
        } else {        /* FF_B_TYPE */
            if (mb_type != 2) {
                if (svq3_mc_dir(h, 0, mode, 0, 0) < 0)
                    return -1;
            } else {
                for (i = 0; i < 4; i++) {
                    memset(s->current_picture.motion_val[0][b_xy + i*h->b_stride], 0, 4*2*sizeof(int16_t));
                }
            }
            if (mb_type != 1) {
                if (svq3_mc_dir(h, 0, mode, 1, (mb_type == 3)) < 0)
                    return -1;
            } else {
                for (i = 0; i < 4; i++) {
                    memset(s->current_picture.motion_val[1][b_xy + i*h->b_stride], 0, 4*2*sizeof(int16_t));
                }
            }
        }

        mb_type = MB_TYPE_16x16;
    } else if (mb_type == 8 || mb_type == 33) {   /* INTRA4x4 */
        memset(h->intra4x4_pred_mode_cache, -1, 8*5*sizeof(int8_t));

        if (mb_type == 8) {
            if (s->mb_x > 0) {
                for (i = 0; i < 4; i++) {
                    h->intra4x4_pred_mode_cache[scan8[0] - 1 + i*8] = h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - 1]+6-i];
                }
                if (h->intra4x4_pred_mode_cache[scan8[0] - 1] == -1) {
                    h->left_samples_available = 0x5F5F;
                }
            }
            if (s->mb_y > 0) {
                h->intra4x4_pred_mode_cache[4+8*0] = h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride]+0];
                h->intra4x4_pred_mode_cache[5+8*0] = h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride]+1];
                h->intra4x4_pred_mode_cache[6+8*0] = h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride]+2];
                h->intra4x4_pred_mode_cache[7+8*0] = h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride]+3];

                if (h->intra4x4_pred_mode_cache[4+8*0] == -1) {
                    h->top_samples_available = 0x33FF;
                }
            }

            /* decode prediction codes for luma blocks */
            for (i = 0; i < 16; i+=2) {
                vlc = svq3_get_ue_golomb(&s->gb);

                if (vlc >= 25){
                    av_log(h->s.avctx, AV_LOG_ERROR, "luma prediction:%d\n", vlc);
                    return -1;
                }

                left    = &h->intra4x4_pred_mode_cache[scan8[i] - 1];
                top     = &h->intra4x4_pred_mode_cache[scan8[i] - 8];

                left[1] = svq3_pred_1[top[0] + 1][left[0] + 1][svq3_pred_0[vlc][0]];
                left[2] = svq3_pred_1[top[1] + 1][left[1] + 1][svq3_pred_0[vlc][1]];

                if (left[1] == -1 || left[2] == -1){
                    av_log(h->s.avctx, AV_LOG_ERROR, "weird prediction\n");
                    return -1;
                }
            }
        } else {    /* mb_type == 33, DC_128_PRED block type */
            for (i = 0; i < 4; i++) {
                memset(&h->intra4x4_pred_mode_cache[scan8[0] + 8*i], DC_PRED, 4);
            }
        }

        ff_svq3_write_back_intra_pred_mode(h);

        if (mb_type == 8) {
            ff_svq3_check_intra4x4_pred_mode(h);

            h->top_samples_available  = (s->mb_y == 0) ? 0x33FF : 0xFFFF;
            h->left_samples_available = (s->mb_x == 0) ? 0x5F5F : 0xFFFF;
        } else {
            for (i = 0; i < 4; i++) {
                memset(&h->intra4x4_pred_mode_cache[scan8[0] + 8*i], DC_128_PRED, 4);
            }

            h->top_samples_available  = 0x33FF;
            h->left_samples_available = 0x5F5F;
        }

        mb_type = MB_TYPE_INTRA4x4;
    } else {                      /* INTRA16x16 */
        dir = i_mb_type_info[mb_type - 8].pred_mode;
        dir = (dir >> 1) ^ 3*(dir & 1) ^ 1;

        if ((h->intra16x16_pred_mode = ff_svq3_check_intra_pred_mode(h, dir)) == -1){
            av_log(h->s.avctx, AV_LOG_ERROR, "check_intra_pred_mode = -1\n");
            return -1;
        }

        cbp = i_mb_type_info[mb_type - 8].cbp;
        mb_type = MB_TYPE_INTRA16x16;
    }

    if (!IS_INTER(mb_type) && s->pict_type != FF_I_TYPE) {
        for (i = 0; i < 4; i++) {
            memset(s->current_picture.motion_val[0][b_xy + i*h->b_stride], 0, 4*2*sizeof(int16_t));
        }
        if (s->pict_type == FF_B_TYPE) {
            for (i = 0; i < 4; i++) {
                memset(s->current_picture.motion_val[1][b_xy + i*h->b_stride], 0, 4*2*sizeof(int16_t));
            }
        }
    }
    if (!IS_INTRA4x4(mb_type)) {
        memset(h->intra4x4_pred_mode+h->mb2br_xy[mb_xy], DC_PRED, 8);
    }
    if (!IS_SKIP(mb_type) || s->pict_type == FF_B_TYPE) {
        memset(h->non_zero_count_cache + 8, 0, 4*9*sizeof(uint8_t));
        s->dsp.clear_blocks(h->mb);
    }

    if (!IS_INTRA16x16(mb_type) && (!IS_SKIP(mb_type) || s->pict_type == FF_B_TYPE)) {
        if ((vlc = svq3_get_ue_golomb(&s->gb)) >= 48){
            av_log(h->s.avctx, AV_LOG_ERROR, "cbp_vlc=%d\n", vlc);
            return -1;
        }

        cbp = IS_INTRA(mb_type) ? golomb_to_intra4x4_cbp[vlc] : golomb_to_inter_cbp[vlc];
    }
    if (IS_INTRA16x16(mb_type) || (s->pict_type != FF_I_TYPE && s->adaptive_quant && cbp)) {
        s->qscale += svq3_get_se_golomb(&s->gb);

        if (s->qscale > 31){
            av_log(h->s.avctx, AV_LOG_ERROR, "qscale:%d\n", s->qscale);
            return -1;
        }
    }
    if (IS_INTRA16x16(mb_type)) {
        if (svq3_decode_block(&s->gb, h->mb, 0, 0)){
            av_log(h->s.avctx, AV_LOG_ERROR, "error while decoding intra luma dc\n");
            return -1;
        }
    }

    if (cbp) {
        const int index = IS_INTRA16x16(mb_type) ? 1 : 0;
        const int type = ((s->qscale < 24 && IS_INTRA4x4(mb_type)) ? 2 : 1);

        for (i = 0; i < 4; i++) {
            if ((cbp & (1 << i))) {
                for (j = 0; j < 4; j++) {
                    k = index ? ((j&1) + 2*(i&1) + 2*(j&2) + 4*(i&2)) : (4*i + j);
                    h->non_zero_count_cache[ scan8[k] ] = 1;

                    if (svq3_decode_block(&s->gb, &h->mb[16*k], index, type)){
                        av_log(h->s.avctx, AV_LOG_ERROR, "error while decoding block\n");
                        return -1;
                    }
                }
            }
        }

        if ((cbp & 0x30)) {
            for (i = 0; i < 2; ++i) {
              if (svq3_decode_block(&s->gb, &h->mb[16*(16 + 4*i)], 0, 3)){
                av_log(h->s.avctx, AV_LOG_ERROR, "error while decoding chroma dc block\n");
                return -1;
              }
            }

            if ((cbp & 0x20)) {
                for (i = 0; i < 8; i++) {
                    h->non_zero_count_cache[ scan8[16+i] ] = 1;

                    if (svq3_decode_block(&s->gb, &h->mb[16*(16 + i)], 1, 1)){
                        av_log(h->s.avctx, AV_LOG_ERROR, "error while decoding chroma ac block\n");
                        return -1;
                    }
                }
            }
        }
    }

    h->cbp= cbp;
    s->current_picture.mb_type[mb_xy] = mb_type;

    if (IS_INTRA(mb_type)) {
        h->chroma_pred_mode = ff_svq3_check_intra_pred_mode(h, DC_PRED8x8);
    }

    return 0;
}

static int svq3_decode_slice_header(H264Context *h)
{
    MpegEncContext *const s = (MpegEncContext *) h;
    const int mb_xy = h->mb_xy;
    int i, header;

    header = get_bits(&s->gb, 8);

    if (((header & 0x9F) != 1 && (header & 0x9F) != 2) || (header & 0x60) == 0) {
        /* TODO: what? */
        av_log(h->s.avctx, AV_LOG_ERROR, "unsupported slice header (%02X)\n", header);
        return -1;
    } else {
        int length = (header >> 5) & 3;

        h->next_slice_index = get_bits_count(&s->gb) + 8*show_bits(&s->gb, 8*length) + 8*length;

        if (h->next_slice_index > s->gb.size_in_bits) {
            av_log(h->s.avctx, AV_LOG_ERROR, "slice after bitstream end\n");
            return -1;
    }

        s->gb.size_in_bits = h->next_slice_index - 8*(length - 1);
        skip_bits(&s->gb, 8);

        if (h->svq3_watermark_key) {
            uint32_t header = AV_RL32(&s->gb.buffer[(get_bits_count(&s->gb)>>3)+1]);
            AV_WL32(&s->gb.buffer[(get_bits_count(&s->gb)>>3)+1], header ^ h->svq3_watermark_key);
        }
        if (length > 0) {
            memcpy((uint8_t *) &s->gb.buffer[get_bits_count(&s->gb) >> 3],
                   &s->gb.buffer[s->gb.size_in_bits >> 3], (length - 1));
        }
        skip_bits_long(&s->gb, 0);
    }

    if ((i = svq3_get_ue_golomb(&s->gb)) == INVALID_VLC || i >= 3){
        av_log(h->s.avctx, AV_LOG_ERROR, "illegal slice type %d \n", i);
        return -1;
    }

    h->slice_type = golomb_to_pict_type[i];

    if ((header & 0x9F) == 2) {
        i = (s->mb_num < 64) ? 6 : (1 + av_log2 (s->mb_num - 1));
        s->mb_skip_run = get_bits(&s->gb, i) - (s->mb_x + (s->mb_y * s->mb_width));
    } else {
        skip_bits1(&s->gb);
        s->mb_skip_run = 0;
    }

    h->slice_num = get_bits(&s->gb, 8);
    s->qscale = get_bits(&s->gb, 5);
    s->adaptive_quant = get_bits1(&s->gb);

    /* unknown fields */
    skip_bits1(&s->gb);

    if (h->unknown_svq3_flag) {
        skip_bits1(&s->gb);
    }

    skip_bits1(&s->gb);
    skip_bits(&s->gb, 2);

    while (get_bits1(&s->gb)) {
        skip_bits(&s->gb, 8);
    }

    /* reset intra predictors and invalidate motion vector references */
    if (s->mb_x > 0) {
        memset(h->intra4x4_pred_mode+h->mb2br_xy[mb_xy - 1      ]+3, -1, 4*sizeof(int8_t));
        memset(h->intra4x4_pred_mode+h->mb2br_xy[mb_xy - s->mb_x]  , -1, 8*sizeof(int8_t)*s->mb_x);
    }
    if (s->mb_y > 0) {
        memset(h->intra4x4_pred_mode+h->mb2br_xy[mb_xy - s->mb_stride], -1, 8*sizeof(int8_t)*(s->mb_width - s->mb_x));

        if (s->mb_x > 0) {
            h->intra4x4_pred_mode[h->mb2br_xy[mb_xy - s->mb_stride - 1]+3] = -1;
        }
    }

    return 0;
}

/* *********************************** H264 Func ************************************* */
static void free_tables(H264Context *h){
    int i;
    H264Context *hx;
    av_freep(&h->intra4x4_pred_mode);
    av_freep(&h->chroma_pred_mode_table);
    av_freep(&h->cbp_table);
    av_freep(&h->mvd_table[0]);
    av_freep(&h->mvd_table[1]);
    av_freep(&h->direct_table);
    av_freep(&h->non_zero_count);
    av_freep(&h->slice_table_base);
    h->slice_table= NULL;
    av_freep(&h->list_counts);

    av_freep(&h->mb2b_xy);
    av_freep(&h->mb2br_xy);

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
    int i,q,x;
    const int transpose = (h->h264dsp.h264_idct8_add != ff_h264_idct8_add_c); //FIXME ugly
    h->dequant8_coeff[0] = h->dequant8_buffer[0];
    h->dequant8_coeff[1] = h->dequant8_buffer[1];

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

static int ff_svq3_alloc_tables(H264Context *h){
    MpegEncContext * const s = &h->s;
    const int big_mb_num= s->mb_stride * (s->mb_height+1);
    const int row_mb_num= 2*s->mb_stride*s->avctx->thread_count;
    int x,y;

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->intra4x4_pred_mode, row_mb_num * 8  * sizeof(uint8_t), fail)

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->non_zero_count    , big_mb_num * 32 * sizeof(uint8_t), fail)
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->slice_table_base  , (big_mb_num+s->mb_stride) * sizeof(*h->slice_table_base), fail)
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->cbp_table, big_mb_num * sizeof(uint16_t), fail)

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->chroma_pred_mode_table, big_mb_num * sizeof(uint8_t), fail)
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mvd_table[0], 16*row_mb_num * sizeof(uint8_t), fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mvd_table[1], 16*row_mb_num * sizeof(uint8_t), fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->direct_table, 4*big_mb_num * sizeof(uint8_t) , fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->list_counts, big_mb_num * sizeof(uint8_t), fail)

    memset(h->slice_table_base, -1, (big_mb_num+s->mb_stride)  * sizeof(*h->slice_table_base));
    h->slice_table= h->slice_table_base + s->mb_stride*2 + 1;

    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mb2b_xy  , big_mb_num * sizeof(uint32_t), fail);
    FF_ALLOCZ_OR_GOTO(h->s.avctx, h->mb2br_xy , big_mb_num * sizeof(uint32_t), fail);
    for(y=0; y<s->mb_height; y++){
        for(x=0; x<s->mb_width; x++){
            const int mb_xy= x + y*s->mb_stride;
            const int b_xy = 4*x + 4*y*h->b_stride;

            h->mb2b_xy [mb_xy]= b_xy;
            h->mb2br_xy[mb_xy]= 8*(FMO ? mb_xy : (mb_xy % (2*s->mb_stride)));
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
//----------------------------
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

/*
 * Func decode_nal_units is cut from h264.c
 */
static int decode_nal_units(H264Context *h, const uint8_t *buf, int buf_size){
    MpegEncContext * const s = &h->s;
    AVCodecContext * const avctx= s->avctx;
    int buf_index=0;
    int next_avc= h->is_avc ? 0 : buf_size;

    h->max_contexts = avctx->thread_count;
    if(!(s->flags2 & CODEC_FLAG2_CHUNKS)){
        h->current_slice = 0;
        if (!s->first_field)
            s->current_picture_ptr= NULL;
        ff_h264_reset_sei(h);
    }

    for(;;){
        const uint8_t *ptr;
        int i, nalsize = 0;

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

    }
    return 0;
}

static int ff_svq3_decode_extradata(H264Context *h)
{
    AVCodecContext *avctx = h->s.avctx;
    
    h->is_avc = 0;
    if(decode_nal_units(h, avctx->extradata, avctx->extradata_size) < 0)
	return -1;
    return 0;
}

static av_cold int ff_svq3_h264_decode_init(AVCodecContext *avctx){
    H264Context *h= avctx->priv_data;
    MpegEncContext * const s = &h->s;

    MPV_decode_defaults(s);

    s->avctx = avctx;
    common_init(h);

    s->out_format = FMT_H264;
    s->workaround_bugs= avctx->workaround_bugs;

    // set defaults
//    s->decode_mb= ff_h263_decode_mb;
    s->quarter_sample = 1;
    if(!avctx->has_b_frames)
    s->low_delay= 1;

    avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;

    ff_h264_decode_init_vlc();

    h->thread_context[0] = h;
    h->outputed_poc = INT_MIN;
    h->prev_poc_msb= 1<<16;
    h->x264_build = -1;
    ff_h264_reset_sei(h);
    if(avctx->codec_id == CODEC_ID_H264){
        if(avctx->ticks_per_frame == 1){
            s->avctx->time_base.den *=2;
        }
        avctx->ticks_per_frame = 2;
    }

    if(avctx->extradata_size > 0 && avctx->extradata &&
        ff_svq3_decode_extradata(h))
        return -1;

    if(h->sps.bitstream_restriction_flag && s->avctx->has_b_frames < h->sps.num_reorder_frames){
        s->avctx->has_b_frames = h->sps.num_reorder_frames;
        s->low_delay = 0;
    }

    return 0;
}

/* *********************************** H264 Func ************************************* */

static av_cold int svq3_decode_init(AVCodecContext *avctx)
{
    MpegEncContext *const s = avctx->priv_data;
    H264Context *const h = avctx->priv_data;
    int m;
    unsigned char *extradata;
    unsigned int size;

    ALOGE("< %s > init start", __FUNCTION__);

    if(avctx->thread_count > 1){
        av_log(avctx, AV_LOG_ERROR, "SVQ3 does not support multithreaded decoding, patch welcome! (check latest SVN too)\n");
        return -1;
    }

    if (ff_svq3_h264_decode_init(avctx) < 0)
        return -1;

    s->flags  = avctx->flags;
    s->flags2 = avctx->flags2;
    s->unrestricted_mv = 1;
    h->is_complex=1;
    avctx->pix_fmt = avctx->codec->pix_fmts[0];

    if (!s->context_initialized) {
        s->width  = avctx->width;
        s->height = avctx->height;
        h->halfpel_flag      = 1;
        h->thirdpel_flag     = 1;
        h->unknown_svq3_flag = 0;
        h->chroma_qp[0]      = h->chroma_qp[1] = 4;

        if (MPV_common_init(s) < 0)
            return -1;

        h->b_stride = 4*s->mb_width;

        ff_svq3_alloc_tables(h);

        /* prowl for the "SEQH" marker in the extradata */
        extradata = (unsigned char *)avctx->extradata;
        for (m = 0; m < avctx->extradata_size; m++) {
            if (!memcmp(extradata, "SEQH", 4))
                break;
            extradata++;
        }

        /* if a match was found, parse the extra data */
        if (extradata && !memcmp(extradata, "SEQH", 4)) {

            GetBitContext gb;
            int frame_size_code;

            size = AV_RB32(&extradata[4]);
            init_get_bits(&gb, extradata + 8, size*8);

            /* 'frame size code' and optional 'width, height' */
            frame_size_code = get_bits(&gb, 3);
            switch (frame_size_code) {
                case 0: avctx->width = 160; avctx->height = 120; break;
                case 1: avctx->width = 128; avctx->height =  96; break;
                case 2: avctx->width = 176; avctx->height = 144; break;
                case 3: avctx->width = 352; avctx->height = 288; break;
                case 4: avctx->width = 704; avctx->height = 576; break;
                case 5: avctx->width = 240; avctx->height = 180; break;
                case 6: avctx->width = 320; avctx->height = 240; break;
                case 7:
                    avctx->width  = get_bits(&gb, 12);
                    avctx->height = get_bits(&gb, 12);
                    break;
            }

            h->halfpel_flag  = get_bits1(&gb);
            h->thirdpel_flag = get_bits1(&gb);

            /* unknown fields */
            skip_bits1(&gb);
            skip_bits1(&gb);
            skip_bits1(&gb);
            skip_bits1(&gb);

            s->low_delay = get_bits1(&gb);

            /* unknown field */
            skip_bits1(&gb);

            while (get_bits1(&gb)) {
                skip_bits(&gb, 8);
            }

            h->unknown_svq3_flag = get_bits1(&gb);
            avctx->has_b_frames = !s->low_delay;
            if (h->unknown_svq3_flag) {
#if CONFIG_ZLIB
                unsigned watermark_width  = svq3_get_ue_golomb(&gb);
                unsigned watermark_height = svq3_get_ue_golomb(&gb);
                int u1 = svq3_get_ue_golomb(&gb);
                int u2 = get_bits(&gb, 8);
                int u3 = get_bits(&gb, 2);
                int u4 = svq3_get_ue_golomb(&gb);
                unsigned long buf_len = watermark_width*watermark_height*4;
                int offset = (get_bits_count(&gb)+7)>>3;
                uint8_t *buf;

                if ((uint64_t)watermark_width*4 > UINT_MAX/watermark_height)
                    return -1;

                buf = av_malloc(buf_len);
                av_log(avctx, AV_LOG_DEBUG, "watermark size: %dx%d\n", watermark_width, watermark_height);
                av_log(avctx, AV_LOG_DEBUG, "u1: %x u2: %x u3: %x compressed data size: %d offset: %d\n", u1, u2, u3, u4, offset);
                if (uncompress(buf, &buf_len, extradata + 8 + offset, size - offset) != Z_OK) {
                    av_log(avctx, AV_LOG_ERROR, "could not uncompress watermark logo\n");
                    av_free(buf);
                    return -1;
                }
                h->svq3_watermark_key = ff_svq1_packet_checksum(buf, buf_len, 0);
                h->svq3_watermark_key = h->svq3_watermark_key << 16 | h->svq3_watermark_key;
                av_log(avctx, AV_LOG_DEBUG, "watermark key %#x\n", h->svq3_watermark_key);
                av_free(buf);
#else
                av_log(avctx, AV_LOG_ERROR, "this svq3 file contains watermark which need zlib support compiled in\n");
                return -1;
#endif
            }
        }
    }

    ALOGE("< %s > init end", __FUNCTION__);
    return 0;
}

/* ********************************** H264 Func Start *************************************** */
static void chroma_dc_dequant_idct_c(DCTELEM *block, int qp, int qmul){
    const int stride= 16*2;
    const int xStride= 16;
    int a,b,c,d,e;

    a= block[stride*0 + xStride*0];
    b= block[stride*0 + xStride*1];
    c= block[stride*1 + xStride*0];
    d= block[stride*1 + xStride*1];

    e= a-b;
    a= a+b;
    b= c-d;
    c= c+d;

    block[stride*0 + xStride*0]= ((a+c)*qmul) >> 7;
    block[stride*0 + xStride*1]= ((e+b)*qmul) >> 7;
    block[stride*1 + xStride*0]= ((a-c)*qmul) >> 7;
    block[stride*1 + xStride*1]= ((e-b)*qmul) >> 7;
}

static inline void xchg_mb_border(H264Context *h, uint8_t *src_y, uint8_t *src_cb, uint8_t *src_cr, int linesize, int uvlinesize, int xchg, int simple){
    MpegEncContext * const s = &h->s;
    int deblock_left;
    int deblock_top;
    int top_idx = 1;
    uint8_t *top_border_m1;
    uint8_t *top_border;

    if(!simple && FRAME_MBAFF){
        if(s->mb_y&1){
            if(!MB_MBAFF)
                return;
        }else{
            top_idx = MB_MBAFF ? 0 : 1;
        }
    }

    if(h->deblocking_filter == 2) {
        deblock_left = h->left_type[0];
        deblock_top  = h->top_type;
    } else {
        deblock_left = (s->mb_x > 0);
        deblock_top =  (s->mb_y > !!MB_FIELD);
    }

    src_y  -=   linesize + 1;
    src_cb -= uvlinesize + 1;
    src_cr -= uvlinesize + 1;

    top_border_m1 = h->top_borders[top_idx][s->mb_x-1];
    top_border    = h->top_borders[top_idx][s->mb_x];

#define XCHG(a,b,xchg)\
if (xchg) AV_SWAP64(b,a);\
else      AV_COPY64(b,a);

    if(deblock_top){
        if(deblock_left){
            XCHG(top_border_m1+8, src_y -7, 1);
        }
        XCHG(top_border+0, src_y +1, xchg);
        XCHG(top_border+8, src_y +9, 1);
        if(s->mb_x+1 < s->mb_width){
            XCHG(h->top_borders[top_idx][s->mb_x+1], src_y +17, 1);
        }
    }

    if(simple || !CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
        if(deblock_top){
            if(deblock_left){
                XCHG(top_border_m1+16, src_cb -7, 1);
                XCHG(top_border_m1+24, src_cr -7, 1);
            }
            XCHG(top_border+16, src_cb+1, 1);
            XCHG(top_border+24, src_cr+1, 1);
        }
    }
}

static av_always_inline void hl_decode_mb_internal(H264Context *h, int simple){
    MpegEncContext * const s = &h->s;
    const int mb_x= s->mb_x;
    const int mb_y= s->mb_y;
    const int mb_xy= h->mb_xy;
    const int mb_type= s->current_picture.mb_type[mb_xy];
    uint8_t  *dest_y, *dest_cb, *dest_cr;
    int linesize, uvlinesize /*dct_offset*/;
    int i;
    int *block_offset = &h->block_offset[0];
    const int transform_bypass = !simple && (s->qscale == 0 && h->sps.transform_bypass);
    void (*idct_add)(uint8_t *dst, DCTELEM *block, int stride);
    void (*idct_dc_add)(uint8_t *dst, DCTELEM *block, int stride);

    dest_y  = s->current_picture.data[0] + (mb_x + mb_y * s->linesize  ) * 16;
    dest_cb = s->current_picture.data[1] + (mb_x + mb_y * s->uvlinesize) * 8;
    dest_cr = s->current_picture.data[2] + (mb_x + mb_y * s->uvlinesize) * 8;

    s->dsp.prefetch(dest_y + (s->mb_x&3)*4*s->linesize + 64, s->linesize, 4);
    s->dsp.prefetch(dest_cb + (s->mb_x&7)*s->uvlinesize + 64, dest_cr - dest_cb, 2);

    h->list_counts[mb_xy]= h->list_count;

    if (!simple && MB_FIELD) {
        linesize   = h->mb_linesize   = s->linesize * 2;
        uvlinesize = h->mb_uvlinesize = s->uvlinesize * 2;
        block_offset = &h->block_offset[24];
        if(mb_y&1){ //FIXME move out of this function?
            dest_y -= s->linesize*15;
            dest_cb-= s->uvlinesize*7;
            dest_cr-= s->uvlinesize*7;
        }
        if(FRAME_MBAFF) {
            int list;
            for(list=0; list<h->list_count; list++){
                if(!USES_LIST(mb_type, list))
                    continue;
                if(IS_16X16(mb_type)){
                    int8_t *ref = &h->ref_cache[list][scan8[0]];
                    fill_rectangle(ref, 4, 4, 8, (16+*ref)^(s->mb_y&1), 1);
                }else{
                    for(i=0; i<16; i+=4){
                        int ref = h->ref_cache[list][scan8[i]];
                        if(ref >= 0)
                            fill_rectangle(&h->ref_cache[list][scan8[i]], 2, 2, 8, (16+ref)^(s->mb_y&1), 1);
                    }
                }
            }
        }
    } else {
        linesize   = h->mb_linesize   = s->linesize;
        uvlinesize = h->mb_uvlinesize = s->uvlinesize;
//        dct_offset = s->linesize * 16;
    }

    if (!simple && IS_INTRA_PCM(mb_type)) {
        for (i=0; i<16; i++) {
            memcpy(dest_y + i*  linesize, h->mb       + i*8, 16);
        }
        for (i=0; i<8; i++) {
            memcpy(dest_cb+ i*uvlinesize, h->mb + 128 + i*4,  8);
            memcpy(dest_cr+ i*uvlinesize, h->mb + 160 + i*4,  8);
        }
    } else {
        if(IS_INTRA(mb_type)){
            if(h->deblocking_filter)
                xchg_mb_border(h, dest_y, dest_cb, dest_cr, linesize, uvlinesize, 1, simple);

            if(simple || !CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                h->hpc.pred8x8[ h->chroma_pred_mode ](dest_cb, uvlinesize);
                h->hpc.pred8x8[ h->chroma_pred_mode ](dest_cr, uvlinesize);
            }

            if(IS_INTRA4x4(mb_type)){
                if(simple || !s->encoding){
                    if(IS_8x8DCT(mb_type)){
                        if(transform_bypass){
                            idct_dc_add =
                            idct_add    = s->dsp.add_pixels8;
                        }else{
                            idct_dc_add = h->h264dsp.h264_idct8_dc_add;
                            idct_add    = h->h264dsp.h264_idct8_add;
                        }
                        for(i=0; i<16; i+=4){
                            uint8_t * const ptr= dest_y + block_offset[i];
                            const int dir= h->intra4x4_pred_mode_cache[ scan8[i] ];
                            if(transform_bypass && h->sps.profile_idc==244 && dir<=1){
                                h->hpc.pred8x8l_add[dir](ptr, h->mb + i*16, linesize);
                            }else{
                                const int nnz = h->non_zero_count_cache[ scan8[i] ];
                                h->hpc.pred8x8l[ dir ](ptr, (h->topleft_samples_available<<i)&0x8000,
                                                            (h->topright_samples_available<<i)&0x4000, linesize);
                                if(nnz){
                                    if(nnz == 1 && h->mb[i*16])
                                        idct_dc_add(ptr, h->mb + i*16, linesize);
                                    else
                                        idct_add   (ptr, h->mb + i*16, linesize);
                                }
                            }
                        }
                    }else{
                        if(transform_bypass){
                            idct_dc_add =
                            idct_add    = s->dsp.add_pixels4;
                        }else{
                            idct_dc_add = h->h264dsp.h264_idct_dc_add;
                            idct_add    = h->h264dsp.h264_idct_add;
                        }
                        for(i=0; i<16; i++){
                            uint8_t * const ptr= dest_y + block_offset[i];
                            const int dir= h->intra4x4_pred_mode_cache[ scan8[i] ];

                            if(transform_bypass && h->sps.profile_idc==244 && dir<=1){
                                h->hpc.pred4x4_add[dir](ptr, h->mb + i*16, linesize);
                            }else{
                                uint8_t *topright;
                                int nnz, tr;
                                if(dir == DIAG_DOWN_LEFT_PRED || dir == VERT_LEFT_PRED){
                                    const int topright_avail= (h->topright_samples_available<<i)&0x8000;
                                    assert(mb_y || linesize <= block_offset[i]);
                                    if(!topright_avail){
                                        tr= ptr[3 - linesize]*0x01010101;
                                        topright= (uint8_t*) &tr;
                                    }else
                                        topright= ptr + 4 - linesize;
                                }else
                                    topright= NULL;

                                h->hpc.pred4x4[ dir ](ptr, topright, linesize);
                                nnz = h->non_zero_count_cache[ scan8[i] ];
                                if(nnz)
				    ff_svq3_add_idct_c(ptr, h->mb + i*16, linesize, s->qscale, 0);
                            }
                        }
                    }
                }
            }else{
                h->hpc.pred16x16[ h->intra16x16_pred_mode ](dest_y , linesize);
		ff_svq3_luma_dc_dequant_idct_c(h->mb, s->qscale);
            }
            if(h->deblocking_filter)
                xchg_mb_border(h, dest_y, dest_cb, dest_cr, linesize, uvlinesize, 0, simple);
        }

        if(!IS_INTRA4x4(mb_type)){
	    for(i=0; i<16; i++){
		if(h->non_zero_count_cache[ scan8[i] ] || h->mb[i*16]){ //FIXME benchmark weird rule, & below
		    uint8_t * const ptr= dest_y + block_offset[i];
		    ff_svq3_add_idct_c(ptr, h->mb + i*16, linesize, s->qscale, IS_INTRA(mb_type) ? 1 : 0);
		}
	    }
        }

        if((simple || !CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)) && (h->cbp&0x30)){
            uint8_t *dest[2] = {dest_cb, dest_cr};
            if(transform_bypass){
                if(IS_INTRA(mb_type) && h->sps.profile_idc==244 && (h->chroma_pred_mode==VERT_PRED8x8 || h->chroma_pred_mode==HOR_PRED8x8)){
                    h->hpc.pred8x8_add[h->chroma_pred_mode](dest[0], block_offset + 16, h->mb + 16*16, uvlinesize);
                    h->hpc.pred8x8_add[h->chroma_pred_mode](dest[1], block_offset + 20, h->mb + 20*16, uvlinesize);
                }else{
                    idct_add = s->dsp.add_pixels4;
                    for(i=16; i<16+8; i++){
                        if(h->non_zero_count_cache[ scan8[i] ] || h->mb[i*16])
                            idct_add   (dest[(i&4)>>2] + block_offset[i], h->mb + i*16, uvlinesize);
                    }
                }
            }else{
                chroma_dc_dequant_idct_c(h->mb + 16*16, h->chroma_qp[0], h->dequant4_coeff[IS_INTRA(mb_type) ? 1:4][h->chroma_qp[0]][0]);
                chroma_dc_dequant_idct_c(h->mb + 16*16+4*16, h->chroma_qp[1], h->dequant4_coeff[IS_INTRA(mb_type) ? 2:5][h->chroma_qp[1]][0]);
		for(i=16; i<16+8; i++){
		    if(h->non_zero_count_cache[ scan8[i] ] || h->mb[i*16]){
			uint8_t * const ptr= dest[(i&4)>>2] + block_offset[i];
			ff_svq3_add_idct_c(ptr, h->mb + i*16, uvlinesize, ff_h264_chroma_qp[s->qscale + 12] - 12, 2);
		    }
		}
	    }
	}
    }
    if(h->cbp || IS_INTRA(mb_type))
        s->dsp.clear_blocks(h->mb);
}

/**
 * Process a macroblock; this case avoids checks for expensive uncommon cases.
 */
static void hl_decode_mb_simple(H264Context *h){
    hl_decode_mb_internal(h, 1);
}

/**
 * Process a macroblock; this handles edge cases, such as interlacing.
 */
static void av_noinline hl_decode_mb_complex(H264Context *h){
    hl_decode_mb_internal(h, 0);
}

static void ff_svq3_hl_decode_mb(H264Context *h){
    MpegEncContext * const s = &h->s;
    const int mb_xy= h->mb_xy;
    const int mb_type= s->current_picture.mb_type[mb_xy];
    int is_complex = CONFIG_SMALL || h->is_complex || IS_INTRA_PCM(mb_type) || s->qscale == 0;

    if (is_complex)
        hl_decode_mb_complex(h);
    else hl_decode_mb_simple(h);
}

static int ff_svq3_frame_start(H264Context *h){
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
/* *********************************** H264 Func End **************************************** */

static int svq3_decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    MpegEncContext *const s = avctx->priv_data;
    H264Context *const h = avctx->priv_data;
    int m, mb_type;
    ALOGE("< svq3_decode_frame > decode an frame");

    /* special case for last picture */
    if (buf_size == 0) {
        if (s->next_picture_ptr && !s->low_delay) {
            *(AVFrame *) data = *(AVFrame *) &s->next_picture;
            s->next_picture_ptr = NULL;
            *data_size = sizeof(AVFrame);
        }
        return 0;
    }

    init_get_bits (&s->gb, buf, 8*buf_size);

    s->mb_x = s->mb_y = h->mb_xy = 0;

    if (svq3_decode_slice_header(h))
        return -1;

    s->pict_type = h->slice_type;
    s->picture_number = h->slice_num;

    if (avctx->debug&FF_DEBUG_PICT_INFO){
        av_log(h->s.avctx, AV_LOG_DEBUG, "%c hpel:%d, tpel:%d aqp:%d qp:%d, slice_num:%02X\n",
               av_get_pict_type_char(s->pict_type), h->halfpel_flag, h->thirdpel_flag,
               s->adaptive_quant, s->qscale, h->slice_num);
    }

    /* for hurry_up == 5 */
    s->current_picture.pict_type = s->pict_type;
    s->current_picture.key_frame = (s->pict_type == FF_I_TYPE);

    /* Skip B-frames if we do not have reference frames. */
    if (s->last_picture_ptr == NULL && s->pict_type == FF_B_TYPE)
        return 0;
    /* Skip B-frames if we are in a hurry. */
    if (avctx->hurry_up && s->pict_type == FF_B_TYPE)
        return 0;
    /* Skip everything if we are in a hurry >= 5. */
    if (avctx->hurry_up >= 5)
        return 0;
    if (  (avctx->skip_frame >= AVDISCARD_NONREF && s->pict_type == FF_B_TYPE)
        ||(avctx->skip_frame >= AVDISCARD_NONKEY && s->pict_type != FF_I_TYPE)
        || avctx->skip_frame >= AVDISCARD_ALL)
        return 0;

    if (s->next_p_frame_damaged) {
        if (s->pict_type == FF_B_TYPE)
            return 0;
        else
            s->next_p_frame_damaged = 0;
    }

    if (ff_svq3_frame_start(h) < 0)
        return -1;

    if (s->pict_type == FF_B_TYPE) {
        h->frame_num_offset = (h->slice_num - h->prev_frame_num);

        if (h->frame_num_offset < 0) {
            h->frame_num_offset += 256;
        }
        if (h->frame_num_offset == 0 || h->frame_num_offset >= h->prev_frame_num_offset) {
            av_log(h->s.avctx, AV_LOG_ERROR, "error in B-frame picture id\n");
            return -1;
        }
    } else {
        h->prev_frame_num = h->frame_num;
        h->frame_num = h->slice_num;
        h->prev_frame_num_offset = (h->frame_num - h->prev_frame_num);

        if (h->prev_frame_num_offset < 0) {
            h->prev_frame_num_offset += 256;
        }
    }

    for (m = 0; m < 2; m++){
        int i;
        for (i = 0; i < 4; i++){
            int j;
            for (j = -1; j < 4; j++)
                h->ref_cache[m][scan8[0] + 8*i + j]= 1;
            if (i < 3)
                h->ref_cache[m][scan8[0] + 8*i + j]= PART_NOT_AVAILABLE;
        }
    }

    for (s->mb_y = 0; s->mb_y < s->mb_height; s->mb_y++) {
        for (s->mb_x = 0; s->mb_x < s->mb_width; s->mb_x++) {
            h->mb_xy = s->mb_x + s->mb_y*s->mb_stride;

            if ( (get_bits_count(&s->gb) + 7) >= s->gb.size_in_bits &&
                ((get_bits_count(&s->gb) & 7) == 0 || show_bits(&s->gb, (-get_bits_count(&s->gb) & 7)) == 0)) {

                skip_bits(&s->gb, h->next_slice_index - get_bits_count(&s->gb));
                s->gb.size_in_bits = 8*buf_size;

                if (svq3_decode_slice_header(h))
                    return -1;

                /* TODO: support s->mb_skip_run */
            }

            mb_type = svq3_get_ue_golomb(&s->gb);

            if (s->pict_type == FF_I_TYPE) {
                mb_type += 8;
            } else if (s->pict_type == FF_B_TYPE && mb_type >= 4) {
                mb_type += 4;
            }
            if (mb_type > 33 || svq3_decode_mb(h, mb_type)) {
                av_log(h->s.avctx, AV_LOG_ERROR, "error while decoding MB %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            if (mb_type != 0) {
                ff_svq3_hl_decode_mb (h);
            }

            if (s->pict_type != FF_B_TYPE && !s->low_delay) {
                s->current_picture.mb_type[s->mb_x + s->mb_y*s->mb_stride] =
                    (s->pict_type == FF_P_TYPE && mb_type < 8) ? (mb_type - 1) : -1;
            }
        }

        ff_draw_horiz_band(s, 16*s->mb_y, 16);
    }

    MPV_frame_end(s);

    if (s->pict_type == FF_B_TYPE || s->low_delay) {
        *(AVFrame *) data = *(AVFrame *) &s->current_picture;
    } else {
        *(AVFrame *) data = *(AVFrame *) &s->last_picture;
    }

    /* Do not output the last pic after seeking. */
    if (s->last_picture_ptr || s->low_delay) {
        *data_size = sizeof(AVFrame);
    }

    return buf_size;
}

static av_cold void ff_svq3_free_context(H264Context *h)
{
    int i;

    free_tables(h); //FIXME cleanup init stuff perhaps

    for(i = 0; i < MAX_SPS_COUNT; i++)
        av_freep(h->sps_buffers + i);

    for(i = 0; i < MAX_PPS_COUNT; i++)
        av_freep(h->pps_buffers + i);
}

static av_cold int ff_svq3_decode_end(AVCodecContext *avctx)
{
    H264Context *h = avctx->priv_data;
    MpegEncContext *s = &h->s;

    ff_svq3_free_context(h);

    MPV_common_end(s);

//    memset(h, 0, sizeof(H264Context));

    return 0;
}

AVCodec svq3_decoder = {
    "svq3",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_SVQ3,
    sizeof(H264Context),
    svq3_decode_init,
    NULL,
    ff_svq3_decode_end,
    svq3_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .long_name = NULL_IF_CONFIG_SMALL("Sorenson Vector Quantizer 3 / Sorenson Video 3 / SVQ3"),
    .pix_fmts= (const enum PixelFormat[]){PIX_FMT_YUVJ420P, PIX_FMT_NONE},
};
