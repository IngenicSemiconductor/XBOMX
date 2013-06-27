/*
 * H.26L/H.264/AVC/JVT/14496-10/... cavlc bitstream decoding
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
 * H.264 / AVC / MPEG4 part10 cavlc bitstream decoding.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#define CABAC 0

#include "internal.h"
#include "avcodec.h"
#include "mpegvideo.h"
#include "h264.h"
#include "h264data.h" // FIXME FIXME FIXME
#include "h264_mvpred.h"
#include "golomb.h"

//#undef NDEBUG
#include <assert.h>

static const uint8_t golomb_to_inter_cbp_gray[16]={
 0, 1, 2, 4, 8, 3, 5,10,12,15, 7,11,13,14, 6, 9,
};

static const uint8_t golomb_to_intra4x4_cbp_gray[16]={
15, 0, 7,11,13,14, 3, 5,10,12, 1, 2, 4, 8, 6, 9,
};

static const uint8_t chroma_dc_coeff_token_len[4*5]={
 2, 0, 0, 0,
 6, 1, 0, 0,
 6, 6, 3, 0,
 6, 7, 7, 6,
 6, 8, 8, 7,
};

static const uint8_t chroma_dc_coeff_token_bits[4*5]={
 1, 0, 0, 0,
 7, 1, 0, 0,
 4, 6, 1, 0,
 3, 3, 2, 5,
 2, 3, 2, 0,
};

static const uint8_t coeff_token_len[4][4*17]={
{
     1, 0, 0, 0,
     6, 2, 0, 0,     8, 6, 3, 0,     9, 8, 7, 5,    10, 9, 8, 6,
    11,10, 9, 7,    13,11,10, 8,    13,13,11, 9,    13,13,13,10,
    14,14,13,11,    14,14,14,13,    15,15,14,14,    15,15,15,14,
    16,15,15,15,    16,16,16,15,    16,16,16,16,    16,16,16,16,
},
{
     2, 0, 0, 0,
     6, 2, 0, 0,     6, 5, 3, 0,     7, 6, 6, 4,     8, 6, 6, 4,
     8, 7, 7, 5,     9, 8, 8, 6,    11, 9, 9, 6,    11,11,11, 7,
    12,11,11, 9,    12,12,12,11,    12,12,12,11,    13,13,13,12,
    13,13,13,13,    13,14,13,13,    14,14,14,13,    14,14,14,14,
},
{
     4, 0, 0, 0,
     6, 4, 0, 0,     6, 5, 4, 0,     6, 5, 5, 4,     7, 5, 5, 4,
     7, 5, 5, 4,     7, 6, 6, 4,     7, 6, 6, 4,     8, 7, 7, 5,
     8, 8, 7, 6,     9, 8, 8, 7,     9, 9, 8, 8,     9, 9, 9, 8,
    10, 9, 9, 9,    10,10,10,10,    10,10,10,10,    10,10,10,10,
},
{
     6, 0, 0, 0,
     6, 6, 0, 0,     6, 6, 6, 0,     6, 6, 6, 6,     6, 6, 6, 6,
     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,
     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,
     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,     6, 6, 6, 6,
}
};

static const uint8_t coeff_token_bits[4][4*17]={
{
     1, 0, 0, 0,
     5, 1, 0, 0,     7, 4, 1, 0,     7, 6, 5, 3,     7, 6, 5, 3,
     7, 6, 5, 4,    15, 6, 5, 4,    11,14, 5, 4,     8,10,13, 4,
    15,14, 9, 4,    11,10,13,12,    15,14, 9,12,    11,10,13, 8,
    15, 1, 9,12,    11,14,13, 8,     7,10, 9,12,     4, 6, 5, 8,
},
{
     3, 0, 0, 0,
    11, 2, 0, 0,     7, 7, 3, 0,     7,10, 9, 5,     7, 6, 5, 4,
     4, 6, 5, 6,     7, 6, 5, 8,    15, 6, 5, 4,    11,14,13, 4,
    15,10, 9, 4,    11,14,13,12,     8,10, 9, 8,    15,14,13,12,
    11,10, 9,12,     7,11, 6, 8,     9, 8,10, 1,     7, 6, 5, 4,
},
{
    15, 0, 0, 0,
    15,14, 0, 0,    11,15,13, 0,     8,12,14,12,    15,10,11,11,
    11, 8, 9,10,     9,14,13, 9,     8,10, 9, 8,    15,14,13,13,
    11,14,10,12,    15,10,13,12,    11,14, 9,12,     8,10,13, 8,
    13, 7, 9,12,     9,12,11,10,     5, 8, 7, 6,     1, 4, 3, 2,
},
{
     3, 0, 0, 0,
     0, 1, 0, 0,     4, 5, 6, 0,     8, 9,10,11,    12,13,14,15,
    16,17,18,19,    20,21,22,23,    24,25,26,27,    28,29,30,31,
    32,33,34,35,    36,37,38,39,    40,41,42,43,    44,45,46,47,
    48,49,50,51,    52,53,54,55,    56,57,58,59,    60,61,62,63,
}
};

static const uint8_t total_zeros_len[16][16]= {
    {1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9},
    {3,3,3,3,3,4,4,4,4,5,5,6,6,6,6},
    {4,3,3,3,4,4,3,3,4,5,5,6,5,6},
    {5,3,4,4,3,3,3,4,3,4,5,5,5},
    {4,4,4,3,3,3,3,3,4,5,4,5},
    {6,5,3,3,3,3,3,3,4,3,6},
    {6,5,3,3,3,2,3,4,3,6},
    {6,4,5,3,2,2,3,3,6},
    {6,6,4,2,2,3,2,5},
    {5,5,3,2,2,2,4},
    {4,4,3,3,1,3},
    {4,4,2,1,3},
    {3,3,1,2},
    {2,2,1},
    {1,1},
};

static const uint8_t total_zeros_bits[16][16]= {
    {1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1},
    {7,6,5,4,3,5,4,3,2,3,2,3,2,1,0},
    {5,7,6,5,4,3,4,3,2,3,2,1,1,0},
    {3,7,5,4,6,5,4,3,3,2,2,1,0},
    {5,4,3,7,6,5,4,3,2,1,1,0},
    {1,1,7,6,5,4,3,2,1,1,0},
    {1,1,5,4,3,3,2,1,1,0},
    {1,1,1,3,3,2,2,1,0},
    {1,0,1,3,2,1,1,1},
    {1,0,1,3,2,1,1},
    {0,1,1,2,1,3},
    {0,1,1,1,1},
    {0,1,1,1},
    {0,1,1},
    {0,1},
};

static const uint8_t chroma_dc_total_zeros_len[3][4]= {
    { 1, 2, 3, 3,},
    { 1, 2, 2, 0,},
    { 1, 1, 0, 0,},
};

static const uint8_t chroma_dc_total_zeros_bits[3][4]= {
    { 1, 1, 1, 0,},
    { 1, 1, 0, 0,},
    { 1, 0, 0, 0,},
};

static const uint8_t run_len[7][16]={
    {1,1},
    {1,2,2},
    {2,2,2,2},
    {2,2,2,3,3},
    {2,2,3,3,3,3},
    {2,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,4,5,6,7,8,9,10,11},
};

static const uint8_t run_bits[7][16]={
    {1,0},
    {1,1,0},
    {3,2,1,0},
    {3,2,1,1,0},
    {3,2,3,2,1,0},
    {3,0,1,3,2,5,4},
    {7,6,5,4,3,2,1,1,1,1,1,1,1,1,1},
};

static VLC coeff_token_vlc[4];
static VLC_TYPE coeff_token_vlc_tables[520+332+280+256][2];
static const int coeff_token_vlc_tables_size[4]={520,332,280,256};

static VLC chroma_dc_coeff_token_vlc;
static VLC_TYPE chroma_dc_coeff_token_vlc_table[256][2];
static const int chroma_dc_coeff_token_vlc_table_size = 256;

static VLC total_zeros_vlc[15];
static VLC_TYPE total_zeros_vlc_tables[15][512][2];
static const int total_zeros_vlc_tables_size = 512;

static VLC chroma_dc_total_zeros_vlc[3];
static VLC_TYPE chroma_dc_total_zeros_vlc_tables[3][8][2];
static const int chroma_dc_total_zeros_vlc_tables_size = 8;

static VLC run_vlc[6];
static VLC_TYPE run_vlc_tables[6][8][2];
static const int run_vlc_tables_size = 8;

static VLC run7_vlc;
static VLC_TYPE run7_vlc_table[96][2];
static const int run7_vlc_table_size = 96;

#define LEVEL_TAB_BITS 8
static int8_t cavlc_level_tab[7][1<<LEVEL_TAB_BITS][2];

static av_cold void init_cavlc_level_tab(void){
    int suffix_length, mask;
    unsigned int i;

    for(suffix_length=0; suffix_length<7; suffix_length++){
        for(i=0; i<(1<<LEVEL_TAB_BITS); i++){
            int prefix= LEVEL_TAB_BITS - av_log2(2*i);
            int level_code= (prefix<<suffix_length) + (i>>(LEVEL_TAB_BITS-prefix-1-suffix_length)) - (1<<suffix_length);

            mask= -(level_code&1);
            level_code= (((2+level_code)>>1) ^ mask) - mask;
            if(prefix + 1 + suffix_length <= LEVEL_TAB_BITS){
                cavlc_level_tab[suffix_length][i][0]= level_code;
                cavlc_level_tab[suffix_length][i][1]= prefix + 1 + suffix_length;
            }else if(prefix + 1 <= LEVEL_TAB_BITS){
                cavlc_level_tab[suffix_length][i][0]= prefix+100;
                cavlc_level_tab[suffix_length][i][1]= prefix + 1;
            }else{
                cavlc_level_tab[suffix_length][i][0]= LEVEL_TAB_BITS+100;
                cavlc_level_tab[suffix_length][i][1]= LEVEL_TAB_BITS;
            }
        }
    }
}

av_cold void ff_h264_decode_init_vlc(void){
    static int done = 0;

    if (!done) {
        int i;
        int offset;
        done = 1;

        chroma_dc_coeff_token_vlc.table = chroma_dc_coeff_token_vlc_table;
        chroma_dc_coeff_token_vlc.table_allocated = chroma_dc_coeff_token_vlc_table_size;
        init_vlc(&chroma_dc_coeff_token_vlc, CHROMA_DC_COEFF_TOKEN_VLC_BITS, 4*5,
                 &chroma_dc_coeff_token_len [0], 1, 1,
                 &chroma_dc_coeff_token_bits[0], 1, 1,
                 INIT_VLC_USE_NEW_STATIC);

        offset = 0;
        for(i=0; i<4; i++){
            coeff_token_vlc[i].table = coeff_token_vlc_tables+offset;
            coeff_token_vlc[i].table_allocated = coeff_token_vlc_tables_size[i];
            init_vlc(&coeff_token_vlc[i], COEFF_TOKEN_VLC_BITS, 4*17,
                     &coeff_token_len [i][0], 1, 1,
                     &coeff_token_bits[i][0], 1, 1,
                     INIT_VLC_USE_NEW_STATIC);
            offset += coeff_token_vlc_tables_size[i];
        }
        /*
         * This is a one time safety check to make sure that
         * the packed static coeff_token_vlc table sizes
         * were initialized correctly.
         */
        assert(offset == FF_ARRAY_ELEMS(coeff_token_vlc_tables));

        for(i=0; i<3; i++){
            chroma_dc_total_zeros_vlc[i].table = chroma_dc_total_zeros_vlc_tables[i];
            chroma_dc_total_zeros_vlc[i].table_allocated = chroma_dc_total_zeros_vlc_tables_size;
            init_vlc(&chroma_dc_total_zeros_vlc[i],
                     CHROMA_DC_TOTAL_ZEROS_VLC_BITS, 4,
                     &chroma_dc_total_zeros_len [i][0], 1, 1,
                     &chroma_dc_total_zeros_bits[i][0], 1, 1,
                     INIT_VLC_USE_NEW_STATIC);
        }
        for(i=0; i<15; i++){
            total_zeros_vlc[i].table = total_zeros_vlc_tables[i];
            total_zeros_vlc[i].table_allocated = total_zeros_vlc_tables_size;
            init_vlc(&total_zeros_vlc[i],
                     TOTAL_ZEROS_VLC_BITS, 16,
                     &total_zeros_len [i][0], 1, 1,
                     &total_zeros_bits[i][0], 1, 1,
                     INIT_VLC_USE_NEW_STATIC);
        }

        for(i=0; i<6; i++){
            run_vlc[i].table = run_vlc_tables[i];
            run_vlc[i].table_allocated = run_vlc_tables_size;
            init_vlc(&run_vlc[i],
                     RUN_VLC_BITS, 7,
                     &run_len [i][0], 1, 1,
                     &run_bits[i][0], 1, 1,
                     INIT_VLC_USE_NEW_STATIC);
        }
        run7_vlc.table = run7_vlc_table,
        run7_vlc.table_allocated = run7_vlc_table_size;
        init_vlc(&run7_vlc, RUN7_VLC_BITS, 16,
                 &run_len [6][0], 1, 1,
                 &run_bits[6][0], 1, 1,
                 INIT_VLC_USE_NEW_STATIC);

        init_cavlc_level_tab();
    }
}

