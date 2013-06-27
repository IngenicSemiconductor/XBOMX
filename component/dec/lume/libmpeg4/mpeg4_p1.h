#ifndef _MPEG4_P1_H_
#define _MPEG4_P1_H_

#ifndef JZC_P1_OPT

#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "mjpegenc.h"
#include "msmpeg4.h"
#include "faandct.h"
#include <limits.h>
#include "mpeg4.h"
#include "mpeg4_dcore.h"

#undef printf

static inline void gmc1_c(uint8_t *dst, uint8_t *src, int stride, int h, int x16, int y16, int rounder)
{
    const int A=(16-x16)*(16-y16);
    const int B=(   x16)*(16-y16);
    const int C=(16-x16)*(   y16);
    const int D=(   x16)*(   y16);
    int i;

    for(i=0; i<h; i++)
    {
        dst[0]= (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1] + rounder)>>8;
        dst[1]= (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2] + rounder)>>8;
        dst[2]= (A*src[2] + B*src[3] + C*src[stride+2] + D*src[stride+3] + rounder)>>8;
        dst[3]= (A*src[3] + B*src[4] + C*src[stride+3] + D*src[stride+4] + rounder)>>8;
        dst[4]= (A*src[4] + B*src[5] + C*src[stride+4] + D*src[stride+5] + rounder)>>8;
        dst[5]= (A*src[5] + B*src[6] + C*src[stride+5] + D*src[stride+6] + rounder)>>8;
        dst[6]= (A*src[6] + B*src[7] + C*src[stride+6] + D*src[stride+7] + rounder)>>8;
        dst[7]= (A*src[7] + B*src[8] + C*src[stride+7] + D*src[stride+8] + rounder)>>8;
        dst+= stride;
        src+= stride;
    }
}

static inline void gmc1_motion_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                               uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
                               uint8_t **ref_picture)
{
    uint8_t *ptr;
    int offset, src_x, src_y, linesize, uvlinesize;
    int motion_x, motion_y;
    int emu=0;

    motion_x= tdFRM->sprite_offset[0][0];
    motion_y= tdFRM->sprite_offset[0][1];
    src_x = tdMB->mb_x * 16 + (motion_x >> (tdFRM->sprite_warping_accuracy+1));
    src_y = tdMB->mb_y * 16 + (motion_y >> (tdFRM->sprite_warping_accuracy+1));
    motion_x<<=(3-tdFRM->sprite_warping_accuracy);
    motion_y<<=(3-tdFRM->sprite_warping_accuracy);
    src_x = av_clip(src_x, -16, tdFRM->width);
    if (src_x == tdFRM->width)
        motion_x =0;
    src_y = av_clip(src_y, -16, tdFRM->height);
    if (src_y == tdFRM->height)
        motion_y =0;

    linesize = tdFRM->linesize;
    uvlinesize = tdFRM->uvlinesize;

    ptr = ref_picture[0] + (src_y * linesize) + src_x;

    if(tdFRM->flags&CODEC_FLAG_EMU_EDGE){
        if(   (unsigned)src_x >= tdFRM->h_edge_pos - 17
           || (unsigned)src_y >= tdFRM->v_edge_pos - 17){
            ff_emulated_edge_mc(tdFRM->edge_emu_buffer, ptr, linesize, 17, 17, src_x, src_y, tdFRM->h_edge_pos, tdFRM->v_edge_pos);
            ptr= tdFRM->edge_emu_buffer;
        }
    }

    if((motion_x|motion_y)&7){
      //s->dsp.gmc1(dest_y  , ptr  , linesize, 16, motion_x&15, motion_y&15, 128 - s->no_rounding);
      //s->dsp.gmc1(dest_y+8, ptr+8, linesize, 16, motion_x&15, motion_y&15, 128 - s->no_rounding);
	gmc1_c(dest_y  , ptr  , linesize, 16, motion_x&15, motion_y&15, 128 - tdFRM->no_rounding);
        gmc1_c(dest_y+8, ptr+8, linesize, 16, motion_x&15, motion_y&15, 128 - tdFRM->no_rounding);
    }else{
        int dxy;

        dxy= ((motion_x>>3)&1) | ((motion_y>>2)&2);
        if (tdFRM->no_rounding){
            tdFRM->put_no_rnd_pixels_tab[0][dxy](dest_y, ptr, linesize, 16);
        }else{
            tdFRM->put_pixels_tab       [0][dxy](dest_y, ptr, linesize, 16);
        }
    }

    if(CONFIG_GRAY && tdFRM->flags&CODEC_FLAG_GRAY) return;

    motion_x= tdFRM->sprite_offset[1][0];
    motion_y= tdFRM->sprite_offset[1][1];
    src_x = tdMB->mb_x * 8 + (motion_x >> (tdFRM->sprite_warping_accuracy+1));
    src_y = tdMB->mb_y * 8 + (motion_y >> (tdFRM->sprite_warping_accuracy+1));
    motion_x<<=(3-tdFRM->sprite_warping_accuracy);
    motion_y<<=(3-tdFRM->sprite_warping_accuracy);
    src_x = av_clip(src_x, -8, tdFRM->width>>1);
    if (src_x == tdFRM->width>>1)
        motion_x =0;
    src_y = av_clip(src_y, -8, tdFRM->height>>1);
    if (src_y == tdFRM->height>>1)
        motion_y =0;

    offset = (src_y * uvlinesize) + src_x;
    ptr = ref_picture[1] + offset;
    if(tdFRM->flags&CODEC_FLAG_EMU_EDGE){
        if(   (unsigned)src_x >= (tdFRM->h_edge_pos>>1) - 9
           || (unsigned)src_y >= (tdFRM->v_edge_pos>>1) - 9){
            ff_emulated_edge_mc(tdFRM->edge_emu_buffer, ptr, uvlinesize, 9, 9, src_x, src_y, tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
            ptr= tdFRM->edge_emu_buffer;
            emu=1;
        }
    }
    gmc1_c(dest_cb, ptr, uvlinesize, 8, motion_x&15, motion_y&15, 128 - tdFRM->no_rounding);

    ptr = ref_picture[2] + offset;
    if(emu){
        ff_emulated_edge_mc(tdFRM->edge_emu_buffer, ptr, uvlinesize, 9, 9, src_x, src_y, tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
        ptr= tdFRM->edge_emu_buffer;
    }
    gmc1_c(dest_cr, ptr, uvlinesize, 8, motion_x&15, motion_y&15, 128 - tdFRM->no_rounding);

    return;
}

static inline int hpel_motion_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                                  uint8_t *dest, uint8_t *src,
                                  int field_based, int field_select,
                                  int src_x, int src_y,
                                  int width, int height, int stride,
                                  int h_edge_pos, int v_edge_pos,
                                  int w, int h, op_pixels_func *pix_op,
                                  int motion_x, int motion_y)
{
    int dxy;
    int emu=0;

    dxy = ((motion_y & 1) << 1) | (motion_x & 1);
    src_x += motion_x >> 1;
    src_y += motion_y >> 1;

    /* WARNING: do no forget half pels */
    src_x = av_clip(src_x, -16, width); //FIXME unneeded for emu?
    if (src_x == width)
        dxy &= ~1;
    src_y = av_clip(src_y, -16, height);
    if (src_y == height)
        dxy &= ~2;
    src += src_y * stride + src_x;

    if(tdFRM->unrestricted_mv && (tdFRM->flags&CODEC_FLAG_EMU_EDGE)){
        if(   (unsigned)src_x > h_edge_pos - (motion_x&1) - w
           || (unsigned)src_y > v_edge_pos - (motion_y&1) - h){
            ff_emulated_edge_mc(tdFRM->edge_emu_buffer, src, tdFRM->linesize, w+1, (h+1)<<field_based,
                             src_x, src_y<<field_based, h_edge_pos, tdFRM->v_edge_pos);
            src= tdFRM->edge_emu_buffer;
            emu=1;
        }
    }
    if(field_select)
        src += tdFRM->linesize;
    pix_op[dxy](dest, src, stride, h);
    return emu;
}

static inline void chroma_4mv_motion_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                                     uint8_t *dest_cb, uint8_t *dest_cr,
                                     uint8_t **ref_picture,
                                     op_pixels_func *pix_op,
                                     int mx, int my){
    int dxy, emu=0, src_x, src_y, offset;
    uint8_t *ptr;

    /* In case of 8X8, we construct a single chroma motion vector
       with a special rounding */
    mx= ff_h263_round_chroma(mx);
    my= ff_h263_round_chroma(my);

    dxy = ((my & 1) << 1) | (mx & 1);
    mx >>= 1;
    my >>= 1;

    src_x = tdMB->mb_x * 8 + mx;
    src_y = tdMB->mb_y * 8 + my;
    src_x = av_clip(src_x, -8, tdFRM->width/2);
    if (src_x == tdFRM->width/2)
        dxy &= ~1;
    src_y = av_clip(src_y, -8, tdFRM->height/2);
    if (src_y == tdFRM->height/2)
        dxy &= ~2;

    offset = (src_y * (tdFRM->uvlinesize)) + src_x;
    ptr = ref_picture[1] + offset;
    if(tdFRM->flags&CODEC_FLAG_EMU_EDGE){
        if(   (unsigned)src_x > (tdFRM->h_edge_pos>>1) - (dxy &1) - 8
           || (unsigned)src_y > (tdFRM->v_edge_pos>>1) - (dxy>>1) - 8){
            ff_emulated_edge_mc(tdFRM->edge_emu_buffer, ptr, tdFRM->uvlinesize, 9, 9, src_x, src_y, tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
            ptr= tdFRM->edge_emu_buffer;
            emu=1;
        }
    }
    pix_op[dxy](dest_cb, ptr, tdFRM->uvlinesize, 8);

    ptr = ref_picture[2] + offset;
    if(emu){
        ff_emulated_edge_mc(tdFRM->edge_emu_buffer, ptr, tdFRM->uvlinesize, 9, 9, src_x, src_y, tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
        ptr= tdFRM->edge_emu_buffer;
    }
    pix_op[dxy](dest_cr, ptr, tdFRM->uvlinesize, 8);
}

static inline void gmc_motion_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                               uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
                               uint8_t **ref_picture)
{
    uint8_t *ptr;
    int linesize, uvlinesize;
    const int a= tdFRM->sprite_warping_accuracy;
    int ox, oy;

    linesize = tdFRM->linesize;
    uvlinesize = tdFRM->uvlinesize;

    ptr = ref_picture[0];

    ox= tdFRM->sprite_offset[0][0] + tdFRM->sprite_delta[0][0]*tdMB->mb_x*16 + tdFRM->sprite_delta[0][1]*tdMB->mb_y*16;
    oy= tdFRM->sprite_offset[0][1] + tdFRM->sprite_delta[1][0]*tdMB->mb_x*16 + tdFRM->sprite_delta[1][1]*tdMB->mb_y*16;

    ff_gmc_c(dest_y, ptr, linesize, 16,
           ox,
           oy,
           tdFRM->sprite_delta[0][0], tdFRM->sprite_delta[0][1],
           tdFRM->sprite_delta[1][0], tdFRM->sprite_delta[1][1],
           a+1, (1<<(2*a+1)) - tdFRM->no_rounding,
           tdFRM->h_edge_pos, tdFRM->v_edge_pos);
    ff_gmc_c(dest_y+8, ptr, linesize, 16,
           ox + tdFRM->sprite_delta[0][0]*8,
           oy + tdFRM->sprite_delta[1][0]*8,
           tdFRM->sprite_delta[0][0], tdFRM->sprite_delta[0][1],
           tdFRM->sprite_delta[1][0], tdFRM->sprite_delta[1][1],
           a+1, (1<<(2*a+1)) - tdFRM->no_rounding,
           tdFRM->h_edge_pos, tdFRM->v_edge_pos);

    if(CONFIG_GRAY && tdFRM->flags&CODEC_FLAG_GRAY) return;

    ox= tdFRM->sprite_offset[1][0] + tdFRM->sprite_delta[0][0]*tdMB->mb_x*8 + tdFRM->sprite_delta[0][1]*tdMB->mb_y*8;
    oy= tdFRM->sprite_offset[1][1] + tdFRM->sprite_delta[1][0]*tdMB->mb_x*8 + tdFRM->sprite_delta[1][1]*tdMB->mb_y*8;

    ptr = ref_picture[1];
    ff_gmc_c(dest_cb, ptr, uvlinesize, 8,
           ox,
           oy,
           tdFRM->sprite_delta[0][0], tdFRM->sprite_delta[0][1],
           tdFRM->sprite_delta[1][0], tdFRM->sprite_delta[1][1],
           a+1, (1<<(2*a+1)) - tdFRM->no_rounding,
           tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);

    ptr = ref_picture[2];
    ff_gmc_c(dest_cr, ptr, uvlinesize, 8,
           ox,
           oy,
           tdFRM->sprite_delta[0][0], tdFRM->sprite_delta[0][1],
           tdFRM->sprite_delta[1][0], tdFRM->sprite_delta[1][1],
           a+1, (1<<(2*a+1)) - tdFRM->no_rounding,
           tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
}

static inline void qpel_motion_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                               uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
                               int field_based, int bottom_field, int field_select,
                               uint8_t **ref_picture, op_pixels_func (*pix_op)[4],
                               qpel_mc_func (*qpix_op)[16],
                               int motion_x, int motion_y, int h)
{
    uint8_t *ptr_y, *ptr_cb, *ptr_cr;
    int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, v_edge_pos, linesize, uvlinesize;

    dxy = ((motion_y & 3) << 2) | (motion_x & 3);
    src_x = tdMB->mb_x *  16                 + (motion_x >> 2);
    src_y = tdMB->mb_y * (16 >> field_based) + (motion_y >> 2);

    v_edge_pos = tdFRM->v_edge_pos >> field_based;
    linesize = tdFRM->linesize << field_based;
    uvlinesize = tdFRM->uvlinesize << field_based;

    if(field_based){
        mx= motion_x/2;
        my= motion_y>>1;
    }else if(tdFRM->workaround_bugs&FF_BUG_QPEL_CHROMA2){
        static const int rtab[8]= {0,0,1,1,0,0,0,1};
        mx= (motion_x>>1) + rtab[motion_x&7];
        my= (motion_y>>1) + rtab[motion_y&7];
    }else if(tdFRM->workaround_bugs&FF_BUG_QPEL_CHROMA){
        mx= (motion_x>>1)|(motion_x&1);
        my= (motion_y>>1)|(motion_y&1);
    }else{
        mx= motion_x/2;
        my= motion_y/2;
    }
    mx= (mx>>1)|(mx&1);
    my= (my>>1)|(my&1);

    uvdxy= (mx&1) | ((my&1)<<1);
    mx>>=1;
    my>>=1;

    uvsrc_x = tdMB->mb_x *  8                 + mx;
    uvsrc_y = tdMB->mb_y * (8 >> field_based) + my;

    ptr_y  = ref_picture[0] +   src_y *   linesize +   src_x;
    ptr_cb = ref_picture[1] + uvsrc_y * uvlinesize + uvsrc_x;
    ptr_cr = ref_picture[2] + uvsrc_y * uvlinesize + uvsrc_x;

    if(   (unsigned)src_x > tdFRM->h_edge_pos - (motion_x&3) - 16
       || (unsigned)src_y >    v_edge_pos - (motion_y&3) - h  ){
        ff_emulated_edge_mc(tdFRM->edge_emu_buffer, ptr_y, tdFRM->linesize, 17, 17+field_based,
                         src_x, src_y<<field_based, tdFRM->h_edge_pos, tdFRM->v_edge_pos);
        ptr_y= tdFRM->edge_emu_buffer;
        if(!CONFIG_GRAY || !(tdFRM->flags&CODEC_FLAG_GRAY)){
            uint8_t *uvbuf= tdFRM->edge_emu_buffer + 18*tdFRM->linesize;
            ff_emulated_edge_mc(uvbuf, ptr_cb, tdFRM->uvlinesize, 9, 9 + field_based,
                             uvsrc_x, uvsrc_y<<field_based, tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
            ff_emulated_edge_mc(uvbuf + 16, ptr_cr, tdFRM->uvlinesize, 9, 9 + field_based,
                             uvsrc_x, uvsrc_y<<field_based, tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
            ptr_cb= uvbuf;
            ptr_cr= uvbuf + 16;
        }
    }

    if(!field_based)
        qpix_op[0][dxy](dest_y, ptr_y, linesize);
    else{
        if(bottom_field){
            dest_y += tdFRM->linesize;
            dest_cb+= tdFRM->uvlinesize;
            dest_cr+= tdFRM->uvlinesize;
        }

        if(field_select){
            ptr_y  += tdFRM->linesize;
            ptr_cb += tdFRM->uvlinesize;
            ptr_cr += tdFRM->uvlinesize;
        }
        //damn interlaced mode
        //FIXME boundary mirroring is not exactly correct here
        qpix_op[1][dxy](dest_y  , ptr_y  , linesize);
        qpix_op[1][dxy](dest_y+8, ptr_y+8, linesize);
    }
    if(!CONFIG_GRAY || !(tdFRM->flags&CODEC_FLAG_GRAY)){
        pix_op[1][uvdxy](dest_cr, ptr_cr, uvlinesize, h >> 1);
        pix_op[1][uvdxy](dest_cb, ptr_cb, uvlinesize, h >> 1);
    }
}

static av_always_inline void mpeg_motion_opt(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                               uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
                               int field_based, int bottom_field, int field_select,
                               uint8_t **ref_picture, op_pixels_func (*pix_op)[4],
                               int motion_x, int motion_y, int h)
{
    uint8_t *ptr_y, *ptr_cb, *ptr_cr;
    int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, v_edge_pos, uvlinesize, linesize;


    v_edge_pos = tdFRM->v_edge_pos >> field_based;
    linesize   = tdFRM->cp_linesize[0] << field_based;
    uvlinesize = tdFRM->cp_linesize[1] << field_based;

    dxy = ((motion_y & 1) << 1) | (motion_x & 1);

    src_x = tdMB->mb_x* 16               + (motion_x >> 1);
    src_y =(tdMB->mb_y<<(4-field_based)) + (motion_y >> 1);

    if (tdFRM->out_format == FMT_H263) {
        if((tdFRM->workaround_bugs & FF_BUG_HPEL_CHROMA) && field_based){
            mx = (motion_x>>1)|(motion_x&1);
            my = motion_y >>1;
            uvdxy = ((my & 1) << 1) | (mx & 1);
            uvsrc_x = tdMB->mb_x* 8               + (mx >> 1);
            uvsrc_y = (tdMB->mb_y<<(3-field_based)) + (my >> 1);
        }else{
            uvdxy = dxy | (motion_y & 2) | ((motion_x & 2) >> 1);
            uvsrc_x = src_x>>1;
            uvsrc_y = src_y>>1;
        }
    }else if(tdFRM->out_format == FMT_H261){//even chroma mv's are full pel in H261
        mx = motion_x / 4;
        my = motion_y / 4;
        uvdxy = 0;
        uvsrc_x = tdMB->mb_x*8 + mx;
        uvsrc_y = tdMB->mb_y*8 + my;
    } else {
        if(tdFRM->chroma_y_shift){
            mx = motion_x / 2;
            my = motion_y / 2;
            uvdxy = ((my & 1) << 1) | (mx & 1);
            uvsrc_x = tdMB->mb_x* 8               + (mx >> 1);
            uvsrc_y = (tdMB->mb_y<<(3-field_based)) + (my >> 1);
        } else {
            if(tdFRM->chroma_x_shift){
            //Chroma422
                mx = motion_x / 2;
                uvdxy = ((motion_y & 1) << 1) | (mx & 1);
                uvsrc_x = tdMB->mb_x* 8           + (mx >> 1);
                uvsrc_y = src_y;
            } else {
            //Chroma444
                uvdxy = dxy;
                uvsrc_x = src_x;
                uvsrc_y = src_y;
            }
        }
    }

    ptr_y  = ref_picture[0] + src_y * linesize + src_x;
    ptr_cb = ref_picture[1] + uvsrc_y * uvlinesize + uvsrc_x;
    ptr_cr = ref_picture[2] + uvsrc_y * uvlinesize + uvsrc_x;

    if(   (unsigned)src_x > tdFRM->h_edge_pos - (motion_x&1) - 16
       || (unsigned)src_y >    v_edge_pos - (motion_y&1) - h){
            if(tdFRM->codec_id == CODEC_ID_MPEG2VIDEO ||
               tdFRM->codec_id == CODEC_ID_MPEG1VIDEO){
	      //av_log(s->avctx,AV_LOG_DEBUG,"MPEG motion vector out of boundary\n");
                return ;
            }
            ff_emulated_edge_mc(tdFRM->edge_emu_buffer, ptr_y, tdFRM->linesize, 17, 17+field_based,
                             src_x, src_y<<field_based, tdFRM->h_edge_pos, tdFRM->v_edge_pos);
            ptr_y = tdFRM->edge_emu_buffer;
            if(!CONFIG_GRAY || !(tdFRM->flags&CODEC_FLAG_GRAY)){
                uint8_t *uvbuf= tdFRM->edge_emu_buffer+18*tdFRM->linesize;
                ff_emulated_edge_mc(uvbuf  , ptr_cb, tdFRM->uvlinesize, 9, 9+field_based,
                                 uvsrc_x, uvsrc_y<<field_based, tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
                ff_emulated_edge_mc(uvbuf+16, ptr_cr, tdFRM->uvlinesize, 9, 9+field_based,
                                 uvsrc_x, uvsrc_y<<field_based, tdFRM->h_edge_pos>>1, tdFRM->v_edge_pos>>1);
                ptr_cb= uvbuf;
                ptr_cr= uvbuf+16;
            }
    }

    if(bottom_field){ //FIXME use this for field pix too instead of the obnoxious hack which changes picture.data
        dest_y += tdFRM->linesize;
        dest_cb+= tdFRM->uvlinesize;
        dest_cr+= tdFRM->uvlinesize;
    }

    if(field_select){
        ptr_y += tdFRM->linesize;
        ptr_cb+= tdFRM->uvlinesize;
        ptr_cr+= tdFRM->uvlinesize;
    }

    pix_op[0][dxy](dest_y, ptr_y, linesize, h);

    if(!CONFIG_GRAY || !(tdFRM->flags&CODEC_FLAG_GRAY)){
        pix_op[tdFRM->chroma_x_shift][uvdxy](dest_cb, ptr_cb, uvlinesize, h >> tdFRM->chroma_y_shift);
        pix_op[tdFRM->chroma_x_shift][uvdxy](dest_cr, ptr_cr, uvlinesize, h >> tdFRM->chroma_y_shift);
    }
    if((CONFIG_H261_ENCODER || CONFIG_H261_DECODER) && tdFRM->out_format == FMT_H261){
      //ff_h261_loop_filter(s);
      printf("mpeg_motion_opt H261 do not support\n");
    }
}

static inline void MPV_motion_p1(MPEG4_Frame_GlbARGs *tdFRM, MPEG4_MB_DecARGs *tdMB,
                              uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
                              int dir, uint8_t **ref_picture,
                              op_pixels_func (*pix_op)[4], qpel_mc_func (*qpix_op)[16]){

    int dxy, mx, my, src_x, src_y, motion_x, motion_y;
    int mb_x, mb_y, i;
    uint8_t *ptr, *dest;

    mb_x = tdMB->mb_x;
    mb_y = tdMB->mb_y;

    //printf("MPV_motion_opt JZSOC\n");
    //prefetch_motion(s, ref_picture, dir);

#if 0
    if(tdFRM->obmc && tdFRM->pict_type != B_TYPE){
        int16_t mv_cache[4][4][2];
        const int xy= tdMB->mb_x + tdMB->mb_y*tdFRM->mb_stride;
        const int mot_stride= tdFRM->b8_stride;
        const int mot_xy= mb_x*2 + mb_y*2*mot_stride;

        assert(!tdFRM->mb_skipped);

	//printf("MPV_motion_opt in obmc and !B_TYPE\n");
        memcpy(mv_cache[1][1], tdFRM->current_picture.motion_val[0][mot_xy           ], sizeof(int16_t)*4);
        memcpy(mv_cache[2][1], tdFRM->current_picture.motion_val[0][mot_xy+mot_stride], sizeof(int16_t)*4);
        memcpy(mv_cache[3][1], tdFRM->current_picture.motion_val[0][mot_xy+mot_stride], sizeof(int16_t)*4);

        if(mb_y==0 || IS_INTRA(tdFRM->current_picture.mb_type[xy-tdFRM->mb_stride])){
            memcpy(mv_cache[0][1], mv_cache[1][1], sizeof(int16_t)*4);
        }else{
            memcpy(mv_cache[0][1], tdFRM->current_picture.motion_val[0][mot_xy-mot_stride], sizeof(int16_t)*4);
        }

        if(mb_x==0 || IS_INTRA(tdFRM->current_picture.mb_type[xy-1])){
            *(int32_t*)mv_cache[1][0]= *(int32_t*)mv_cache[1][1];
            *(int32_t*)mv_cache[2][0]= *(int32_t*)mv_cache[2][1];
        }else{
            *(int32_t*)mv_cache[1][0]= *(int32_t*)tdFRM->current_picture.motion_val[0][mot_xy-1];
            *(int32_t*)mv_cache[2][0]= *(int32_t*)tdFRM->current_picture.motion_val[0][mot_xy-1+mot_stride];
        }

        if(mb_x+1>=tdFRM->mb_width || IS_INTRA(tdFRM->current_picture.mb_type[xy+1])){
            *(int32_t*)mv_cache[1][3]= *(int32_t*)mv_cache[1][2];
            *(int32_t*)mv_cache[2][3]= *(int32_t*)mv_cache[2][2];
        }else{
            *(int32_t*)mv_cache[1][3]= *(int32_t*)tdFRM->current_picture.motion_val[0][mot_xy+2];
            *(int32_t*)mv_cache[2][3]= *(int32_t*)tdFRM->current_picture.motion_val[0][mot_xy+2+mot_stride];
        }

        mx = 0;
        my = 0;
        for(i=0;i<4;i++) {
            const int x= (i&1)+1;
            const int y= (i>>1)+1;
            int16_t mv[5][2]= {
                {mv_cache[y][x  ][0], mv_cache[y][x  ][1]},
                {mv_cache[y-1][x][0], mv_cache[y-1][x][1]},
                {mv_cache[y][x-1][0], mv_cache[y][x-1][1]},
                {mv_cache[y][x+1][0], mv_cache[y][x+1][1]},
                {mv_cache[y+1][x][0], mv_cache[y+1][x][1]}};
            //FIXME cleanup
            obmc_motion(s, dest_y + ((i & 1) * 8) + (i >> 1) * 8 * tdFRM->linesize,
                        ref_picture[0],
                        mb_x * 16 + (i & 1) * 8, mb_y * 16 + (i >>1) * 8,
                        pix_op[1],
                        mv);

            mx += mv[0][0];
            my += mv[0][1];
        }
        if(!CONFIG_GRAY || !(tdFRM->flags&CODEC_FLAG_GRAY))
            chroma_4mv_motion(s, dest_cb, dest_cr, ref_picture, pix_op[1], mx, my);

        return;
    }
#endif
    switch(tdMB->mv_type) {
    case MV_TYPE_16X16:
      //printf("MPV_motion_opt 16X16\n");
        if(tdMB->mcsel){
            if(tdFRM->real_sprite_warping_points==1){
	      gmc1_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                            ref_picture);
            }else{
	      gmc_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                            ref_picture);
            }
        }else if(tdFRM->quarter_sample){
	  qpel_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                        0, 0, 0,
                        ref_picture, pix_op, qpix_op,
                        tdMB->mv[dir][0][0], tdMB->mv[dir][0][1], 16);
        }else if((CONFIG_WMV2_DECODER || CONFIG_WMV2_ENCODER) && tdFRM->mspel){
	  printf("MPV_motion_p1 WMV2 do not support\n");
        }else
        {
	  mpeg_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                        0, 0, 0,
                        ref_picture, pix_op,
                        tdMB->mv[dir][0][0], tdMB->mv[dir][0][1], 16);
        }
        break;
    case MV_TYPE_8X8:
      //printf("MPV_motion_opt 8X8 quarter_sample %d\n", s->quarter_sample);
        mx = 0;
        my = 0;
        if(tdFRM->quarter_sample){
            for(i=0;i<4;i++) {
                motion_x = tdMB->mv[dir][i][0];
                motion_y = tdMB->mv[dir][i][1];

                dxy = ((motion_y & 3) << 2) | (motion_x & 3);
                src_x = mb_x * 16 + (motion_x >> 2) + (i & 1) * 8;
                src_y = mb_y * 16 + (motion_y >> 2) + (i >>1) * 8;

                /* WARNING: do no forget half pels */
                src_x = av_clip(src_x, -16, tdFRM->width);
                if (src_x == tdFRM->width)
                    dxy &= ~3;
                src_y = av_clip(src_y, -16, tdFRM->height);
                if (src_y == tdFRM->height)
                    dxy &= ~12;

                ptr = ref_picture[0] + (src_y * tdFRM->linesize) + (src_x);
                if(tdFRM->flags&CODEC_FLAG_EMU_EDGE){
                    if(   (unsigned)src_x > tdFRM->h_edge_pos - (motion_x&3) - 8
                       || (unsigned)src_y > tdFRM->v_edge_pos - (motion_y&3) - 8 ){
                        ff_emulated_edge_mc(tdFRM->edge_emu_buffer, ptr, tdFRM->linesize, 9, 9, src_x, src_y, tdFRM->h_edge_pos, tdFRM->v_edge_pos);
                        ptr= tdFRM->edge_emu_buffer;
                    }
                }
                dest = dest_y + ((i & 1) * 8) + (i >> 1) * 8 * tdFRM->linesize;
                qpix_op[1][dxy](dest, ptr, tdFRM->linesize);

                mx += tdMB->mv[dir][i][0]/2;
                my += tdMB->mv[dir][i][1]/2;
            }
        }else{
            for(i=0;i<4;i++) {
	      hpel_motion_opt(tdFRM, tdMB, dest_y + ((i & 1) * 8) + (i >> 1) * 8 * tdFRM->linesize,
                            ref_picture[0], 0, 0,
                            mb_x * 16 + (i & 1) * 8, mb_y * 16 + (i >>1) * 8,
                            tdFRM->width, tdFRM->height, tdFRM->linesize,
                            tdFRM->h_edge_pos, tdFRM->v_edge_pos,
                            8, 8, pix_op[1],
                            tdMB->mv[dir][i][0], tdMB->mv[dir][i][1]);

                mx += tdMB->mv[dir][i][0];
                my += tdMB->mv[dir][i][1];
            }
        }

        if(!CONFIG_GRAY || !(tdFRM->flags&CODEC_FLAG_GRAY)){
	  //printf("MPV_motion_opt 4mv_motion\n");
	  chroma_4mv_motion_opt(tdFRM, tdMB, dest_cb, dest_cr, ref_picture, pix_op[1], mx, my);
        }
	break;
    case MV_TYPE_FIELD:
        if (tdFRM->picture_structure == PICT_FRAME) {
            if(tdFRM->quarter_sample){
                for(i=0; i<2; i++){
		  qpel_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                                1, i, tdMB->field_select[dir][i],
                                ref_picture, pix_op, qpix_op,
                                tdMB->mv[dir][i][0], tdMB->mv[dir][i][1], 8);
                }
            }else{
                /* top field */
	      mpeg_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                            1, 0, tdMB->field_select[dir][0],
                            ref_picture, pix_op,
                            tdMB->mv[dir][0][0], tdMB->mv[dir][0][1], 8);
                /* bottom field */
	      mpeg_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                            1, 1, tdMB->field_select[dir][1],
                            ref_picture, pix_op,
                            tdMB->mv[dir][1][0], tdMB->mv[dir][1][1], 8);
            }
        } else {
            if(tdFRM->picture_structure != tdMB->field_select[dir][0] + 1 && tdFRM->pict_type != B_TYPE /*&& !tdMB->first_field*/){
                ref_picture= tdFRM->current_picture_ptr->data;
            }

            mpeg_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                        0, 0, tdMB->field_select[dir][0],
                        ref_picture, pix_op,
                        tdMB->mv[dir][0][0], tdMB->mv[dir][0][1], 16);
        }
        break;
    case MV_TYPE_16X8:
        for(i=0; i<2; i++){
            uint8_t ** ref2picture;

            if(tdFRM->picture_structure == tdMB->field_select[dir][i] + 1 || tdFRM->pict_type == B_TYPE /*|| tdMB->first_field*/){
                ref2picture= ref_picture;
            }else{
                ref2picture= tdFRM->current_picture_ptr->data;
            }

            mpeg_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                        0, 0, tdMB->field_select[dir][i],
                        ref2picture, pix_op,
                        tdMB->mv[dir][i][0], tdMB->mv[dir][i][1] + 16*i, 8);

            dest_y += 16*tdFRM->linesize;
            dest_cb+= (16>>tdFRM->chroma_y_shift)*tdFRM->uvlinesize;
            dest_cr+= (16>>tdFRM->chroma_y_shift)*tdFRM->uvlinesize;
        }
        break;
    case MV_TYPE_DMV:
        if(tdFRM->picture_structure == PICT_FRAME){
            for(i=0; i<2; i++){
                int j;
                for(j=0; j<2; j++){
		  mpeg_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                                1, j, j^i,
                                ref_picture, pix_op,
                                tdMB->mv[dir][2*i + j][0], tdMB->mv[dir][2*i + j][1], 8);
                }
                pix_op = tdFRM->avg_pixels_tab;
            }
        }else{
            for(i=0; i<2; i++){
	      mpeg_motion_opt(tdFRM, tdMB, dest_y, dest_cb, dest_cr,
                            0, 0, tdFRM->picture_structure != i+1,
                            ref_picture, pix_op,
                            tdMB->mv[dir][2*i][0],tdMB->mv[dir][2*i][1],16);

                // after put we make avg of the same block
                pix_op=tdFRM->avg_pixels_tab;

                //opposite parity is always in the same frame if this is second field
                if(/*!tdMB->first_field*/1){
                    ref_picture = tdFRM->current_picture_ptr->data;
                }
            }
        }
    break;
    default: assert(0);
    }
}

#endif //JZC_P1_OPT
#endif
