/*
 * RV30/40 decoder common data
 * Copyright (c) 2007 Mike Melanson, Konstantin Shishkov
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
 * RV30/40 decoder common data
 */
#include "libavcore/imgutils.h"
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "golomb.h"
#include "rv34.h"
#include "rv34vlc.h"
#include "rv34data.h"
#include "rv40vlc2.h"
#include "rv40data.h"

#include "dsputil.h"
#include "mpegvideo.h"
#include "mathops.h"
#include "rectangle.h"

#define IPU_BUG
#ifdef JZC_MXU_OPT
#include "../libjzcommon/com_config.h"
#include "../libjzcommon/jzmedia.h"
#include "../libjzcommon/jzasm.h"
#include "../libjzcommon/jz4760e_dcsc.h"
#include "../libjzcommon/jz4760e_tcsm_init.c"
#include "../libjzcommon/jz4760e_2ddma_hw.h"
#include "rv40mxu.h"
#endif

#undef printf
extern volatile unsigned char * cpm_base;
extern volatile unsigned char * vpu_base;
extern volatile unsigned char * gp0_base;
extern volatile unsigned char * mc_base;
extern volatile unsigned char * vmau_base;
extern volatile unsigned char * tcsm1_base;

#define CPM_VPU_SWRST    (cpm_base + 0xC4)
#define CPM_VPU_SR     	 (0x1<<31)
#define CPM_VPU_STP    	 (0x1<<30)
#define CPM_VPU_ACK    	 (0x1<<29)

#define write_cpm_reg(a)    (*(volatile unsigned int *)(CPM_VPU_SWRST) = a)
#define read_cpm_reg()      (*(volatile unsigned int *)(CPM_VPU_SWRST))

#define RST_VPU()							\
    {									\
	write_cpm_reg(read_cpm_reg() | CPM_VPU_STP);			\
	while( !(read_cpm_reg() & CPM_VPU_ACK) );			\
	write_cpm_reg( (read_cpm_reg() | CPM_VPU_SR) & (~CPM_VPU_STP) ); \
	write_cpm_reg( read_cpm_reg() & (~CPM_VPU_SR) & (~CPM_VPU_STP) ); \
    }

#include <sched.h>

#ifdef JZC_MC_OPT
#include "jzsoc/rv9_p0_mc.c"
#endif

#ifdef JZC_DBLK_OPT
unsigned int dblk_gp_slice_end;
#include "jzsoc/t_dblk.h"
//#include "jzsoc/rv9_sram.h"
#endif

#ifdef JZC_DCORE_OPT
#include "jzsoc/rv9_dcore.h"
#include "jzsoc/rv9_tcsm.h"
RV9_MB_DecARGs *dMB;
#endif //JZC_DCORE_OPT

#ifdef JZC_VLC_HW_OPT
volatile char * rv9_hw_bs_buffer;
#include "vlc_tables.h"
#include "vlc_bs.c"
#endif

/* ******************************* DBG INFO ************************** */
#include "utils/Log.h"
#define EL(x,y...)  //{ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y);}

//#define CRC_CHECK
#ifdef CRC_CHECK
#include "../libjzcommon/crc.c"
static int mpframe;
static int frame_crc;
#undef printf
#undef sprintf
#undef fprintf
#endif

#define DEBUG_FRAME_NUM 0
//#define DEBUG_MB_Y  0
//#define DEBUG_MB_X  0
//#define REMOVE_IDCT
//#define WRITE_MBINFO
//#define DEBUG_AUX_WAITING

//#define DEBUG_MC
//#define DEBUG_DBLK

//#define TEST_TIME
#ifdef TEST_TIME
static unsigned int slice_init_time;
static unsigned int dec_time;
static unsigned int wait_time;
static unsigned int frame_time;
static int frame_num;
#endif

#define XCHG2(a,b,t)   t=a;a=b;b=t

static unsigned int GetTimer(void){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
/* ******************************* DBG INFO ************************** */

static VLC aic_top_vlc;
static VLC aic_mode1_vlc[AIC_MODE1_NUM], aic_mode2_vlc[AIC_MODE2_NUM];
static VLC ptype_vlc[NUM_PTYPE_VLCS], btype_vlc[NUM_BTYPE_VLCS];

#if 0
static const int16_t mode2_offs[] = {
    0,  614, 1222, 1794, 2410,  3014,  3586,  4202,  4792, 5382, 5966, 6542,
    7138, 7716, 8292, 8864, 9444, 10030, 10642, 11212, 11814
};
#else
static const int16_t mode2_offs[] = {
    0,  426 , 856, 1208, 1646, 2064, 2414, 2856, 3224, 3588, 3950, 4310,
    4662, 5024, 5376, 5732, 6086, 6454, 6890, 7256, 7682
};
#endif

static av_always_inline void ZERO8x2(void* dst, int stride)
{
    fill_rectangle(dst,                 1, 2, stride, 0, 4);
    fill_rectangle(((uint8_t*)(dst))+4, 1, 2, stride, 0, 4);
}

/** translation of RV30/40 macroblock types to lavc ones */
static const int rv40_mb_type_to_lavc[12] = {
    MB_TYPE_INTRA,
    MB_TYPE_INTRA16x16              | MB_TYPE_SEPARATE_DC,
    MB_TYPE_16x16   | MB_TYPE_L0,
    MB_TYPE_8x8     | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L1,
    MB_TYPE_SKIP,
    MB_TYPE_DIRECT2 | MB_TYPE_16x16,
    MB_TYPE_16x8    | MB_TYPE_L0,
    MB_TYPE_8x16    | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L0L1,
    MB_TYPE_16x16   | MB_TYPE_L0    | MB_TYPE_SEPARATE_DC
};


static RV34VLC intra_vlcs[NUM_INTRA_TABLES], inter_vlcs[NUM_INTER_TABLES];

/**
 * @defgroup vlc RV30/40 VLC generating functions
 * @{
 */

#if 0
static const int table_offs[] = {
    0,   1818,   3622,   4144,   4698,   5234,   5804,   5868,   5900,   5932,
    5996,   6252,   6316,   6348,   6380,   7674,   8944,  10274,  11668,  12250,
    14060,  15846,  16372,  16962,  17512,  18148,  18180,  18212,  18244,  18308,
    18564,  18628,  18660,  18692,  20036,  21314,  22648,  23968,  24614,  26384,
    28190,  28736,  29366,  29938,  30608,  30640,  30672,  30704,  30768,  31024,
    31088,  31120,  31184,  32570,  33898,  35236,  36644,  37286,  39020,  40802,
    41368,  42052,  42692,  43348,  43380,  43412,  43444,  43476,  43604,  43668,
    43700,  43732,  45100,  46430,  47778,  49160,  49802,  51550,  53340,  53972,
    54648,  55348,  55994,  56122,  56154,  56186,  56218,  56346,  56410,  56442,
    56474,  57878,  59290,  60636,  62036,  62682,  64460,  64524,  64588,  64716,
    64844,  66076,  67466,  67978,  68542,  69064,  69648,  70296,  72010,  72074,
    72138,  72202,  72330,  73572,  74936,  75454,  76030,  76566,  77176,  77822,
    79582,  79646,  79678,  79742,  79870,  81180,  82536,  83064,  83672,  84242,
    84934,  85576,  87384,  87448,  87480,  87544,  87672,  88982,  90340,  90902,
    91598,  92182,  92846,  93488,  95246,  95278,  95310,  95374,  95502,  96878,
    98266,  98848,  99542, 100234, 100884, 101524, 103320, 103352, 103384, 103416,
    103480, 104874, 106222, 106910, 107584, 108258, 108902, 109544, 111366, 111398,
    111430, 111462, 111494, 112878, 114320, 114988, 115660, 116310, 116950, 117592
};
#else
static const int table_offs[] = {
    0,  1634,  3298,  3586,  3914,  4218,  4558,  4622,  4654,  4686, 
    4750,  5006,  5070,  5102,  5134,  6320,  7546,  8644,  9884, 10276, 
    11920, 13566, 13860, 14220, 14538, 14974, 15006, 15038, 15070, 15134,
    15390, 15454, 15486, 15518, 16696, 17904, 19014, 20222, 20738, 22324,
    23964, 24276, 24712, 25048, 25584, 25616, 25648, 25680, 25744, 26000,
    26064, 26096, 26160, 27328, 28534, 29650, 30878, 31392, 33000, 34570,
    34904, 35456, 35892, 36416, 36448, 36480, 36512, 36544, 36672, 36736,
    36768, 36800, 38028, 39208, 40328, 41522, 42036, 43698, 45284, 45718,
    46260, 46822, 47338, 47466, 47498, 47530, 47562, 47690, 47754, 47786,
    47818, 48984, 50232, 51342, 52624, 53140, 54744, 54808, 54872, 55000,
    55128, 56306, 57510, 57786, 58116, 58426, 58768, 59286, 60898, 60962,
    61026, 61090, 61218, 62408, 63604, 63888, 64226, 64550, 64932, 65450,
    67082, 67146, 67178, 67242, 67370, 68570, 69762, 70076, 70454, 70796,
    71352, 71866, 73456, 73520, 73552, 73616, 73744, 74906, 76138, 76470,
    77034, 77394, 77930, 78444, 79982, 80014, 80046, 80110, 80238, 81452,
    82598, 82954, 83516, 84078, 84596, 85108, 86788, 86820, 86852, 86884,
    86948, 88180, 89284, 89842, 90380, 90922, 91438, 91950, 93558, 93590,
    93622, 93654, 93686, 94940, 96256, 96800, 97338, 97862, 98374, 98886,
};
#endif

static VLC_TYPE table_data[98886][2];

/**
 * Generate VLC from codeword lengths.
 * @param bits   codeword lengths (zeroes are accepted)
 * @param size   length of input data
 * @param vlc    output VLC
 * @param insyms symbols for input codes (NULL for default ones)
 * @param num    VLC table number (for static initialization)
 */
static void rv40_gen_vlc(const uint8_t *bits, int size, VLC *vlc, const uint8_t *insyms,
                         const int num)
{
    int i;
    int counts[17] = {0}, codes[17];
    uint16_t cw[MAX_VLC_SIZE], syms[MAX_VLC_SIZE];
    uint8_t bits2[MAX_VLC_SIZE];
    int maxbits = 0, realsize = 0;

    for(i = 0; i < size; i++){
	if(bits[i]){
	    bits2[realsize] = bits[i];
	    syms[realsize] = insyms ? insyms[i] : i;
	    realsize++;
	    maxbits = FFMAX(maxbits, bits[i]);
	    counts[bits[i]]++;
	}
    }

    codes[0] = 0;
    for(i = 0; i < 16; i++)
	codes[i+1] = (codes[i] + counts[i]) << 1;
    for(i = 0; i < realsize; i++)
	cw[i] = codes[bits2[i]]++;

    vlc->table = &table_data[table_offs[num]];
    vlc->table_allocated = table_offs[num + 1] - table_offs[num];
    init_vlc_sparse(vlc, FFMIN(maxbits, 8), realsize,
		    bits2, 1, 1,
		    cw,    2, 2,
		    syms,  2, 2, INIT_VLC_USE_NEW_STATIC);
}

/**
 * Initialize all tables.
 */
static av_cold void rv34_init_tables(void)
{
    int i, j, k;

    for(i = 0; i < NUM_INTRA_TABLES; i++){
	for(j = 0; j < 2; j++){
	    rv40_gen_vlc(rv34_table_intra_cbppat   [i][j], CBPPAT_VLC_SIZE,   &intra_vlcs[i].cbppattern[j],     NULL, 19*i + 0 + j);
	    rv40_gen_vlc(rv34_table_intra_secondpat[i][j], OTHERBLK_VLC_SIZE, &intra_vlcs[i].second_pattern[j], NULL, 19*i + 2 + j);
	    rv40_gen_vlc(rv34_table_intra_thirdpat [i][j], OTHERBLK_VLC_SIZE, &intra_vlcs[i].third_pattern[j],  NULL, 19*i + 4 + j);
	    for(k = 0; k < 4; k++){
		rv40_gen_vlc(rv34_table_intra_cbp[i][j+k*2],  CBP_VLC_SIZE,   &intra_vlcs[i].cbp[j][k],         rv34_cbp_code, 19*i + 6 + j*4 + k);
	    }
	}
	for(j = 0; j < 4; j++){
	    rv40_gen_vlc(rv34_table_intra_firstpat[i][j], FIRSTBLK_VLC_SIZE, &intra_vlcs[i].first_pattern[j], NULL, 19*i + 14 + j);
	}
	rv40_gen_vlc(rv34_intra_coeff[i], COEFF_VLC_SIZE, &intra_vlcs[i].coefficient, NULL, 19*i + 18);
    }

    for(i = 0; i < NUM_INTER_TABLES; i++){
	rv40_gen_vlc(rv34_inter_cbppat[i], CBPPAT_VLC_SIZE, &inter_vlcs[i].cbppattern[0], NULL, i*12 + 95);
	for(j = 0; j < 4; j++){
	    rv40_gen_vlc(rv34_inter_cbp[i][j], CBP_VLC_SIZE, &inter_vlcs[i].cbp[0][j], rv34_cbp_code, i*12 + 96 + j);
	}
	for(j = 0; j < 2; j++){
	    rv40_gen_vlc(rv34_table_inter_firstpat [i][j], FIRSTBLK_VLC_SIZE, &inter_vlcs[i].first_pattern[j],  NULL, i*12 + 100 + j);
	    rv40_gen_vlc(rv34_table_inter_secondpat[i][j], OTHERBLK_VLC_SIZE, &inter_vlcs[i].second_pattern[j], NULL, i*12 + 102 + j);
	    rv40_gen_vlc(rv34_table_inter_thirdpat [i][j], OTHERBLK_VLC_SIZE, &inter_vlcs[i].third_pattern[j],  NULL, i*12 + 104 + j);
	}
	rv40_gen_vlc(rv34_inter_coeff[i], COEFF_VLC_SIZE, &inter_vlcs[i].coefficient, NULL, i*12 + 106);
    }
}

/** @} */ // vlc group
/**
 * Initialize all tables.
 */
static av_cold void rv40_init_tables(void)
{
    int i;
#ifdef JZC_VLC_HW_OPT
    VLC_TYPE_8B (*cp_ptr)[2];
#endif
    static VLC_TYPE aic_table[1 << AIC_TOP_BITS][2];
    static VLC_TYPE aic_mode1_table[AIC_MODE1_NUM << AIC_MODE1_BITS][2];
    static VLC_TYPE aic_mode2_table[7682][2];
    static VLC_TYPE ptype_table[NUM_PTYPE_VLCS << PTYPE_VLC_BITS][2];
    static VLC_TYPE btype_table[NUM_BTYPE_VLCS << BTYPE_VLC_BITS][2];

    aic_top_vlc.table = aic_table;
    aic_top_vlc.table_allocated = 1 << AIC_TOP_BITS;
    init_vlc(&aic_top_vlc, AIC_TOP_BITS, AIC_TOP_SIZE,
	     rv40_aic_top_vlc_bits,  1, 1,
	     rv40_aic_top_vlc_codes, 1, 1, INIT_VLC_USE_NEW_STATIC);
    for(i = 0; i < AIC_MODE1_NUM; i++){
	  // Every tenth VLC table is empty
	if((i % 10) == 9) continue;
	aic_mode1_vlc[i].table = &aic_mode1_table[i << AIC_MODE1_BITS];
	aic_mode1_vlc[i].table_allocated = 1 << AIC_MODE1_BITS;
	init_vlc(&aic_mode1_vlc[i], AIC_MODE1_BITS, AIC_MODE1_SIZE,
		 aic_mode1_vlc_bits[i],  1, 1,
		 aic_mode1_vlc_codes[i], 1, 1, INIT_VLC_USE_NEW_STATIC);
#ifdef JZC_VLC_HW_OPT
	cp_ptr=aic_mode1_vlc[i].table;
	for(j=0;j<(1 << AIC_MODE1_BITS);j++){
	    cp_ptr[j][0]=aic_mode1_vlc[i].table[j][0];
	    cp_ptr[j][1]=(aic_mode1_vlc[i].table[j][1]-1<< 4) | 0xd;
	}
#endif
    }
    for(i = 0; i < AIC_MODE2_NUM; i++){
	aic_mode2_vlc[i].table = &aic_mode2_table[mode2_offs[i]];
	aic_mode2_vlc[i].table_allocated = mode2_offs[i + 1] - mode2_offs[i];
	init_vlc(&aic_mode2_vlc[i], AIC_MODE2_BITS, AIC_MODE2_SIZE,
		 aic_mode2_vlc_bits[i],  1, 1,
		 aic_mode2_vlc_codes[i], 2, 2, INIT_VLC_USE_NEW_STATIC);
    }
    for(i = 0; i < NUM_PTYPE_VLCS; i++){
	ptype_vlc[i].table = &ptype_table[i << PTYPE_VLC_BITS];
	ptype_vlc[i].table_allocated = 1 << PTYPE_VLC_BITS;
	init_vlc_sparse(&ptype_vlc[i], PTYPE_VLC_BITS, PTYPE_VLC_SIZE,
			ptype_vlc_bits[i],  1, 1,
			ptype_vlc_codes[i], 1, 1,
			ptype_vlc_syms,     1, 1, INIT_VLC_USE_NEW_STATIC);
#ifdef JZC_VLC_HW_OPT
	cp_ptr=ptype_vlc[i].table;
	for(j=0;j<(1 << PTYPE_VLC_BITS);j++){
	    cp_ptr[j][0]=ptype_vlc[i].table[j][0];
	    cp_ptr[j][1]=(ptype_vlc[i].table[j][1]-1<< 4) | 0xd;
	}
#endif
    }
    for(i = 0; i < NUM_BTYPE_VLCS; i++){
	btype_vlc[i].table = &btype_table[i << BTYPE_VLC_BITS];
	btype_vlc[i].table_allocated = 1 << BTYPE_VLC_BITS;
	init_vlc_sparse(&btype_vlc[i], BTYPE_VLC_BITS, BTYPE_VLC_SIZE,
			btype_vlc_bits[i],  1, 1,
			btype_vlc_codes[i], 1, 1,
			btype_vlc_syms,     1, 1, INIT_VLC_USE_NEW_STATIC);
#ifdef JZC_VLC_HW_OPT
	cp_ptr=btype_vlc[i].table;
	for(j=0;j<(1 << BTYPE_VLC_BITS);j++){
	    cp_ptr[j][0]=btype_vlc[i].table[j][0];
	    cp_ptr[j][1]=(btype_vlc[i].table[j][1]-1<< 4) | 0xd;
	}
#endif
    }
}

/**
 * Get stored dimension from bitstream.
 *
 * If the width/height is the standard one then it's coded as a 3-bit index.
 * Otherwise it is coded as escaped 8-bit portions.
 */
static int get_dimension(GetBitContext *gb, const int *dim)
{
    int t   = get_bits(gb, 3);
    int val = dim[t];
    if(val < 0)
	val = dim[get_bits1(gb) - val];
    if(!val){
	do{
	    t = get_bits(gb, 8);
	    val += t << 2;
	}while(t == 0xFF);
    }
    return val;
}

/**
 * Get encoded picture size - usually this is called from rv40_parse_slice_header.
 */
static void rv40_parse_picture_size(GetBitContext *gb, int *w, int *h)
{
    *w = get_dimension(gb, rv40_standard_widths);
    *h = get_dimension(gb, rv40_standard_heights);
}

int inline ff_rv40_get_start_offset(GetBitContext *gb, int mb_size);

static int rv40_parse_slice_header(RV34DecContext *r, GetBitContext *gb, SliceInfo *si)
{
    int mb_bits;
    int w = r->s.width, h = r->s.height;
    int mb_size;

    memset(si, 0, sizeof(SliceInfo));
    if(get_bits1(gb))
	return -1;
    si->type = get_bits(gb, 2);
    if(si->type == 1) si->type = 0;
    si->quant = get_bits(gb, 5);
    if(get_bits(gb, 2))
	return -1;
    si->vlc_set = get_bits(gb, 2);
    skip_bits1(gb);
    si->pts = get_bits(gb, 13);
    if(!si->type || !get_bits1(gb))
	rv40_parse_picture_size(gb, &w, &h);
    if(av_image_check_size(w, h, 0, r->s.avctx) < 0)
	return -1;
    si->width  = w;
    si->height = h;
    mb_size = ((w + 15) >> 4) * ((h + 15) >> 4);
    mb_bits = ff_rv40_get_start_offset(gb, mb_size);
    si->start = get_bits(gb, mb_bits);

    return 0;
}

/**
 * Decode 4x4 intra types array.
 */
#ifdef JZC_VLC_HW_OPT
#define get_bits(s,n) get_bits_hw(n)
#define get_bits1(s) get_bits1_hw()
#define skip_bits(s,n) skip_bits_hw(n)
#define show_bits(s,n) show_ubits_hw(n)
#define get_vlc2(s,table,bits,max_depth) get_vlc2_hw(table,bits,max_depth)
#define svq3_get_ue_golomb svq3_get_ue_golombl
#define svq3_get_se_golomb svq3_get_se_golombl
#endif

#ifdef DIV9_TABLE_OPT
uint8_t div9[81]={
    0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,
    0x01,0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81,
    0x02,0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82,
    0x03,0x13,0x23,0x33,0x43,0x53,0x63,0x73,0x83,
    0x04,0x14,0x24,0x34,0x44,0x54,0x64,0x74,0x84,
    0x05,0x15,0x25,0x35,0x45,0x55,0x65,0x75,0x85,
    0x06,0x16,0x26,0x36,0x46,0x56,0x66,0x76,0x86,
    0x07,0x17,0x27,0x37,0x47,0x57,0x67,0x77,0x87,
    0x08,0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x88,
};
#endif

static av_always_inline int rv40_decode_intra_types(RV34DecContext *r, GetBitContext *gb, int8_t *dst)
{
    MpegEncContext *s = &r->s;
    int i, j, k, v;
    int A, B, C;
    int pattern;
    int8_t *ptr;

    for(i = 0; i < 4; i++, dst += r->intra_types_stride){
	if(!i && s->first_slice_line){
	    pattern = get_vlc2(gb, aic_top_vlc.table, AIC_TOP_BITS, 1);
	    dst[0] = (pattern >> 2) & 2;
	    dst[1] = (pattern >> 1) & 2;
	    dst[2] =  pattern       & 2;
	    dst[3] = (pattern << 1) & 2;
	    continue;
	}
	ptr = dst;
	for(j = 0; j < 4; j++){
	      /* Coefficients are read using VLC chosen by the prediction pattern
	       * The first one (used for retrieving a pair of coefficients) is
	       * constructed from the top, top right and left coefficients
	       * The second one (used for retrieving only one coefficient) is
	       * top + 10 * left.
	       */
	    A = ptr[-r->intra_types_stride + 1]; // it won't be used for the last coefficient in a row
	    B = ptr[-r->intra_types_stride];
	    C = ptr[-1];
	    pattern = A + (B << 4) + (C << 8);

	    if(A!=B&&B!=C&&A!=C)
		k=MODE2_PATTERNS_NUM;
	    else
		for(k = 0; k < MODE2_PATTERNS_NUM; k++)
		    if(pattern == rv40_aic_table_index[k])
			break;

	    if(k < MODE2_PATTERNS_NUM && j < 3){ //pattern is found, decoding 2 coefficients
		v = get_vlc2(gb, aic_mode2_vlc[k].table, AIC_MODE2_BITS, 2);
#ifdef DIV9_TABLE_OPT
		uint32_t a=div9[v];
		*ptr++ = a&0xf;
		*ptr++ = a>>4;
#else
		*ptr++ = v/9;
		*ptr++ = v%9;
#endif
		j++;
	    }else{
#ifdef JZC_VLC_HW_OPT
		CPU_SET_CTRL((AIC_MODE1_BITS-1 << 4) | (1 << 3) | (3 << 1) | 1);
		CPU_GET_RSLT();
#endif
		if((B|C)>=0){
#ifdef JZC_VLC_HW_OPT
		    VLC_TYPE_8B (*table)[2]=aic_mode1_vlc[B + C*10].table;
		    int index=CPU_GET_RSLT();
		    v = table[index][0];
		    int ctrl = table[index][1];
		    CPU_SET_CTRL(ctrl);
		    CPU_GET_RSLT();
		    CPU_GET_RSLT();
#else
		    v = get_vlc2(gb, aic_mode1_vlc[B + C*10].table, AIC_MODE1_BITS, 1);
#endif
		}
		else{ // tricky decoding
		    v = 0;
		    switch(C){
		    case -1: // code 0 -> 1, 1 -> 0
			if(B < 2)
			    v = get_bits1(gb) ^ 1;
			break;
		    case  0:
		    case  2: // code 0 -> 2, 1 -> 0
			v = (get_bits1(gb) ^ 1) << 1;
			break;
		    }
		}
		*ptr++ = v;
	    }

	}
    }

    return 0;
}

/**
 * Decode macroblock information.
 */
static av_always_inline int svq3_get_ue_golombl(GetBitContext *gb){
    uint32_t buf,t;

    t=get_bits1(gb);
    buf=show_bits(gb,8);
    buf|=(t<<8);
    if(buf&0x155){
	buf >>= 1;

	if(ff_interleaved_golomb_vlc_len[buf]-1) skip_bits(gb, ff_interleaved_golomb_vlc_len[buf]-1);
	return ff_interleaved_ue_golomb_vlc_code[buf];
    }else{
	int ret = 1;
	buf >>= 1;
	if(ff_interleaved_golomb_vlc_len[buf]-1) skip_bits(gb,FFMIN(ff_interleaved_golomb_vlc_len[buf], 8)-1);

	while (1) {
	    if (ff_interleaved_golomb_vlc_len[buf] != 9){
		ret <<= (ff_interleaved_golomb_vlc_len[buf] - 1) >> 1;
		ret |= ff_interleaved_dirac_golomb_vlc_code[buf];
		break;
	    }
	    ret = (ret << 4) | ff_interleaved_dirac_golomb_vlc_code[buf];
	    buf=show_bits(gb,8);
	    skip_bits(gb,FFMIN(ff_interleaved_golomb_vlc_len[buf], 8));
	}
	return ret - 1;
    }
}
static av_always_inline int rv40_decode_mb_info(RV34DecContext *r,int bk_mb_type)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int q, i;
    int prev_type = 0;
    int mb_pos = s->mb_x ;
    int blocks[RV34_MB_TYPES] = {0};
    int count = 0;

    if(!r->s.mb_skip_run)
	r->s.mb_skip_run = svq3_get_ue_golomb(gb) + 1;

    if(--r->s.mb_skip_run)
	return RV34_MB_SKIP;

    if(r->avail_cache[6-1])
	blocks[r->mb_type[mb_pos - 1]]++;
    if(r->avail_cache[6-4]){
	blocks[r->mb_type[mb_pos ]]++;
	if(r->avail_cache[6-2])
	    blocks[r->mb_type[mb_pos + 1]]++;
	if(r->avail_cache[6-5])
	    blocks[bk_mb_type]++;
    }

    for(i = 0; i < RV34_MB_TYPES; i++){
	if(blocks[i] > count){
	    count = blocks[i];
	    prev_type = i;
	}
    }
#ifdef JZC_VLC_HW_OPT
    uint32_t ctrl_val;
    int index,ctrl;
    VLC_TYPE_8B (*table)[2];
    ctrl_val = ((PTYPE_VLC_BITS-1 << 4) | (1 << 3) | (3 << 1) | 1 );
    CPU_SET_CTRL(ctrl_val);
    CPU_GET_RSLT();
#endif
    if(s->pict_type == FF_P_TYPE){
#ifdef JZC_VLC_HW_OPT
	prev_type = block_num_to_ptype_vlc_num[prev_type];
	table=ptype_vlc[prev_type].table;
	index= CPU_GET_RSLT();
	q = table[index][0];
	ctrl = table[index][1];
	CPU_SET_CTRL(ctrl);
	CPU_GET_RSLT();
	CPU_GET_RSLT();
#else
	prev_type = block_num_to_ptype_vlc_num[prev_type];
	q = get_vlc2(gb, ptype_vlc[prev_type].table, PTYPE_VLC_BITS, 1);
#endif
	if(q < PBTYPE_ESCAPE)
	    return q;
#ifdef JZC_VLC_HW_OPT
	CPU_SET_CTRL(ctrl_val);
	index= CPU_GET_RSLT();
	index= CPU_GET_RSLT();
	q = table[index][0];
	ctrl = table[index][1];
	CPU_SET_CTRL(ctrl);
	CPU_GET_RSLT();
	CPU_GET_RSLT();
#else
	q = get_vlc2(gb, ptype_vlc[prev_type].table, PTYPE_VLC_BITS, 1);
#endif
	av_log(s->avctx, AV_LOG_ERROR, "Dquant for P-frame\n");
    }else{
#ifdef JZC_VLC_HW_OPT
	prev_type = block_num_to_btype_vlc_num[prev_type];
	table=btype_vlc[prev_type].table;
	index= CPU_GET_RSLT()>>1;
	q = table[index][0];
	ctrl = table[index][1];
	CPU_SET_CTRL(ctrl);
	CPU_GET_RSLT();
	CPU_GET_RSLT();
#else
	prev_type = block_num_to_btype_vlc_num[prev_type];
	q = get_vlc2(gb, btype_vlc[prev_type].table, BTYPE_VLC_BITS, 1);
#endif
	if(q < PBTYPE_ESCAPE)
	    return q;
#ifdef JZC_VLC_HW_OPT
	CPU_SET_CTRL((BTYPE_VLC_BITS-1 << 4) | (1 << 3) | (3 << 1) | 1);
	index= CPU_GET_RSLT();
	index= CPU_GET_RSLT();
	q = table[index][0];
	ctrl = table[index][1];
	CPU_SET_CTRL(ctrl);
	CPU_GET_RSLT();
	CPU_GET_RSLT();
#else
	q = get_vlc2(gb, btype_vlc[prev_type].table, BTYPE_VLC_BITS, 1);
#endif
	av_log(s->avctx, AV_LOG_ERROR, "Dquant for B-frame\n");
    }
    return 0;
}

/**
 * @defgroup block RV30/40 4x4 block decoding functions
 * @{
 */

/**
 * Decode coded block pattern.
 */

#ifdef JZC_CCBP_TBL_OPT
typedef struct
{
    unsigned char dsc_to_ccbp1;
    unsigned char dsc_to_cmask1;
    unsigned char dsc_to_clen1;
} CCBP_TBL1;

CCBP_TBL1 dsc_lookup[81];
unsigned char getb_lookup[256];
#endif//JZC_CCBP_TBL_OPT

static av_always_inline int rv40_decode_cbp(GetBitContext *gb, RV34VLC *vlc, int table)
{

    int pattern, code, cbp=0;
    int ones;
    static const int cbp_masks[3] = {0x100000, 0x010000, 0x110000};
    static const int shifts[4] = { 0, 2, 8, 10 };
    const int *curshift = shifts;
    int i, t, mask;

    code = get_vlc2(gb, vlc->cbppattern[table].table, 8, 2);
    pattern = code & 0xF;
    code >>= 4;

    ones = rv34_count_ones[pattern];
    for(mask = 8; mask; mask >>= 1, curshift++){
	if(pattern & mask)
	    cbp |= get_vlc2(gb, vlc->cbp[table][ones].table, vlc->cbp[table][ones].bits, 1) << curshift[0];
    }

#ifdef JZC_CCBP_TBL_OPT
    uint32_t ccbp_part1=dsc_lookup[code].dsc_to_ccbp1;
    uint32_t getb_low4=dsc_lookup[code].dsc_to_cmask1;
    uint32_t getb_len=dsc_lookup[code].dsc_to_clen1;
      /*
	fprintf(ccbp_tab,"d=%d\nccbp_part1=0x%x, getb_low4=0x%x, getb_len=0x%x\n",
	d, ccbp_part1, getb_low4, getb_len);
      */
    uint32_t cc_bs=getb_len?get_bits(gb,getb_len):0;
    
    getb_low4|=cc_bs<<4;
    ccbp_part1|=getb_lookup[getb_low4];
    cbp|=ccbp_part1<<16;
      /*    fprintf(ccbp_tab,"cbp=0x%x\nc=0x%x\t getb_low4=0x%x\t ccbp_part1=0x%x\n",
	    cbp,c, getb_low4, ccbp_part1);*/
#else
    for(i = 0; i < 4; i++){
	t = modulo_three_table[code][i];
	if(t == 1)
	    cbp |= cbp_masks[get_bits1(gb)] << i;
	if(t == 2)
	    cbp |= cbp_masks[2] << i;
    }
#endif//#ifdef JZC_CCBP_TBL_OPT

    return cbp;
}

/**
 * Get one coefficient value from the bistream and store it.
 */
#ifdef JZC_VLC_HW_OPT
static uint32_t vlc_ctrl;
#endif
static av_always_inline void decode_coeff(DCTELEM *dst, int coef, int esc, GetBitContext *gb, VLC* vlc)
{
    if(coef){
	if(coef == esc){
#ifdef JZC_VLC_HW_OPT
	    CPU_SET_CTRL(vlc_ctrl);
	    coef=CPU_GET_RSLT();
	    coef=CPU_GET_RSLT();
#else
	    coef = get_vlc2(gb, vlc->table, 8, 2);
#endif
	    if(coef > 23){
		coef -= 23;
		coef = 22 + ((1 << coef) | get_bits(gb, coef));
	    }
	    coef += esc;
	}
	if(get_bits1(gb))
	    coef = -coef;
	*dst = coef;
    }
}

/**
 * Decode 2x2 subblock of coefficients.
 */
static av_always_inline void decode_subblock(DCTELEM *dst, int code, const int is_block2, GetBitContext *gb, VLC *vlc)
{
    int coeffs[4];

    coeffs[0] = modulo_three_table[code][0];
    coeffs[1] = modulo_three_table[code][1];
    coeffs[2] = modulo_three_table[code][2];
    coeffs[3] = modulo_three_table[code][3];
    decode_coeff(dst  , coeffs[0], 3, gb, vlc);
    if(is_block2){
	decode_coeff(dst+4, coeffs[1], 2, gb, vlc);
	decode_coeff(dst+1, coeffs[2], 2, gb, vlc);
    }else{
	decode_coeff(dst+1, coeffs[1], 2, gb, vlc);
	decode_coeff(dst+4, coeffs[2], 2, gb, vlc);
    }
    decode_coeff(dst+5, coeffs[3], 2, gb, vlc);
}

/**
 * Decode coefficients for 4x4 block.
 *
 * This is done by filling 2x2 subblocks with decoded coefficients
 * in this order (the same for subblocks and subblock coefficients):
 *  o--o
 *    /
 *   /
 *  o--o
 */
#ifdef JZC_VLC_HW_OPT
uint32_t *sec_pat_ctrl;
uint32_t sec_pat_ctrl_intra[2],sec_pat_ctrl_inter[2];
#endif
static av_always_inline void rv40_decode_block(DCTELEM *dst, GetBitContext *gb, RV34VLC *rvlc, int fc, int sc, int n)
{
    int code, pattern;

    code = get_vlc2(gb, rvlc->first_pattern[fc].table, 8, 2);

    pattern = code & 0x7;
    code >>= 3;
    decode_subblock(dst, code, 0, gb, &rvlc->coefficient);

    if(pattern & 4){
#ifdef JZC_VLC_HW_OPT
	CPU_SET_CTRL(sec_pat_ctrl[sc]);
	code=CPU_GET_RSLT();
	code=CPU_GET_RSLT();
#else
	code = get_vlc2(gb, rvlc->second_pattern[sc].table, 8, 2);
#endif
	decode_subblock(dst + 2, code, 0, gb, &rvlc->coefficient);
    }
    if(pattern & 2){ // Looks like coefficients 1 and 2 are swapped for this block
#ifdef JZC_VLC_HW_OPT
	CPU_SET_CTRL(sec_pat_ctrl[sc]);
	code=CPU_GET_RSLT();
	code=CPU_GET_RSLT();
#else
	code = get_vlc2(gb, rvlc->second_pattern[sc].table, 8, 2);
#endif
	decode_subblock(dst + 4*2, code, 1, gb, &rvlc->coefficient);
    }
    if(pattern & 1){
	code = get_vlc2(gb, rvlc->third_pattern[sc].table, 8, 2);
	decode_subblock(dst + 4*2+2, code, 0, gb, &rvlc->coefficient);
    }

}

/**
 * Decode starting slice position.
 * @todo Maybe replace with ff_h263_decode_mba() ?
 */
int inline ff_rv40_get_start_offset(GetBitContext *gb, int mb_size)
{
    int i;
    for(i = 0; i < 5; i++)
	if(rv34_mb_max_sizes[i] >= mb_size - 1)
	    break;
    return rv34_mb_bits_sizes[i];
}

/**
 * Select VLC set for decoding from current quantizer, modifier and frame type.
 */
#ifdef JZC_VLC_HW_OPT
#else
static inline RV34VLC* choose_vlc_set(int quant, int mod, int type)
{
    if(mod == 2 && quant < 19) quant += 10;
    else if(mod && quant < 26) quant += 5;
    return type ? &inter_vlcs[rv34_quant_to_vlc_set[1][av_clip(quant, 0, 30)]]
	: &intra_vlcs[rv34_quant_to_vlc_set[0][av_clip(quant, 0, 30)]];
}
/**
 * Decode quantizer difference and return modified quantizer.
 */
static inline int rv40_decode_dquant(GetBitContext *gb, int quant)
{
    if(get_bits1(gb))
	return rv34_dquant_tab[get_bits1(gb)][quant];
    else
	return get_bits(gb, 5);
}
#endif

/** macroblock partition width in 8x8 blocks */
static const uint8_t part_sizes_w[RV34_MB_TYPES] = { 2, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2 };

/** macroblock partition height in 8x8 blocks */
static const uint8_t part_sizes_h[RV34_MB_TYPES] = { 2, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 2 };

/** availability index for subblocks */
static const uint8_t avail_indexes[4] = { 6, 7, 10, 11 };

/**
 * motion vector prediction
 *
 * Motion prediction performed for the block by using median prediction of
 * motion vectors from the left, top and right top blocks but in corner cases
 * some other vectors may be used instead.
 */
static av_always_inline void rv40_pred_mv(RV34DecContext *r, int block_type, int subblock_no, int dmv_no)
{
    MpegEncContext *s = &r->s;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
#if 0
    int A[2] = {0}, B[2], C[2];
    int i, j;
    int mx, my;
#endif
    int avail_index = avail_indexes[subblock_no];
    int c_off = part_sizes_w[block_type];

    mv_pos += (subblock_no & 1) + (subblock_no >> 1)*s->b8_stride;
    if(subblock_no == 3)
	c_off = -1;

    int *cur_mv=s->current_picture_ptr->motion_val[0][mv_pos];
#if 0
    if(r->avail_cache[avail_index - 1]){
	A[0] = cur_mv[-1][0];//s->current_picture_ptr->motion_val[0][mv_pos-1][0];
	A[1] = cur_mv[-1][1];//s->current_picture_ptr->motion_val[0][mv_pos-1][1];
    }
    if(r->avail_cache[avail_index - 4]){
	B[0] = cur_mv[-s->b8_stride][0];//s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][0];
	B[1] = cur_mv[-s->b8_stride][1];//s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][1];
    }else{
	B[0] = A[0];
	B[1] = A[1];
    }
    if(!r->avail_cache[avail_index - 4 + c_off]){
	if(r->avail_cache[avail_index - 4] && (r->avail_cache[avail_index - 1])){
	    C[0] = cur_mv[-s->b8_stride-1][0];//s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][0];
	    C[1] = cur_mv[-s->b8_stride-1][1];//s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][1];
	}else{
	    C[0] = A[0];
	    C[1] = A[1];
	}
    }else{
	C[0] = cur_mv[-s->b8_stride+c_off][0];//s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+c_off][0];
	C[1] = cur_mv[-s->b8_stride+c_off][1];//s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+c_off][1];
    }
    mx = mid_pred(A[0], B[0], C[0]);
    my = mid_pred(A[1], B[1], C[1]);
    mx += r->dmv[dmv_no][0];
    my += r->dmv[dmv_no][1];
    int aa=mx;
    int bb=my;
    for(j = 0; j < part_sizes_h[block_type]; j++){
	for(i = 0; i < part_sizes_w[block_type]; i++){
	      //      s->current_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][0] = mx;
	      //      s->current_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][1] = my;
	    cur_mv[i + j*s->b8_stride][0]=mx;
	    cur_mv[i + j*s->b8_stride][1]=my;
	}
    }
#else
    D32SLL(xr1,xr0,xr0,xr0,1);//xr1=A=0;
    if(r->avail_cache[avail_index - 1]){
	S32LDD(xr1,cur_mv,-4);//xr1=mv_left
    }
    S32OR(xr2,xr1,xr0);
    S32OR(xr3,xr1,xr0);
    if(r->avail_cache[avail_index - 4]){
	S32LDDV(xr2,cur_mv,-s->b8_stride,2);//xr2=mv_above
    }else{
    }
    if(!r->avail_cache[avail_index - 4 + c_off]){
	if(r->avail_cache[avail_index - 4] && (r->avail_cache[avail_index - 1])){
	    S32LDDV(xr3,cur_mv,-s->b8_stride-1,2);//xr3=mv_above
	}else{
	}
    }else{
	S32LDDV(xr3,cur_mv,-s->b8_stride+c_off,2);//xr3=mv_above
    }

    D16MAX(xr4,xr1,xr2);//xr4=max of (A,B)
    D16MIN(xr5,xr1,xr2);//xr5=min of (A,B)
    D16MIN(xr4,xr4,xr3);//xr4:comp max(A,B) and C, get rid of the MAX of (A,B,C)
    D16MAX(xr4,xr4,xr5);//xr4:get rid of the MIN of (A,B,C), xr4=mid

    S32LDDV(xr6,r->dmv,dmv_no,2);
    Q16ADD_AA_WW(xr9,xr4,xr6,xr0);

#if 0
    if(((aa&0xffff)|((bb&0xffff)<<16)) != S32M2I(xr9)){
	printf("r->dmv[dmv_no][0]=0x%x, r->dmv[dmv_no][1]=0x%x, xr7=0x%x,xr8=0x%x,xr9=0x%x,xr6=0x%x\n",
	       r->dmv[dmv_no][0],r->dmv[dmv_no][1],S32M2I(xr7),S32M2I(xr8),S32M2I(xr9),S32M2I(xr6));

	printf("((aa&0xffff)|((bb&0xffff)<<16))=0x%x, aa=0x%x, bb=0x%x, xr4=0x%x, xr5=0x%x\n",
	       ((aa&0xffff)|((bb&0xffff)<<16)),aa,bb,S32M2I(xr4),S32M2I(xr5));
    }
#endif
      /*
	for(j = 0; j < part_sizes_h[block_type]; j++){
	for(i = 0; i < part_sizes_w[block_type]; i++){
	S32STDV(xr9,cur_mv,i + j*s->b8_stride,2);
	}
	}
      */
#endif
}

/**
 * Calculate motion vector component that should be added for direct blocks.
 */

#define GET_PTS_DIFF(a, b) ((a - b + 8192) & 0x1FFF)
/**
 * Predict motion vector for B-frame macroblock.
 */
static av_always_inline void rv40_pred_b_vector(int A[2], int B[2], int C[2],
						int A_avail, int B_avail, int C_avail,
						int *mx, int *my)
{
    if(A_avail + B_avail + C_avail != 3){
	*mx = A[0] + B[0] + C[0];
	*my = A[1] + B[1] + C[1];
	if(A_avail + B_avail + C_avail == 2){
	    *mx /= 2;
	    *my /= 2;
	}
    }else{
	*mx = mid_pred(A[0], B[0], C[0]);
	*my = mid_pred(A[1], B[1], C[1]);
    }
}

/**
 * motion vector prediction for B-frames
 */
static inline void rv40_pred_mv_b(RV34DecContext *r, int block_type, int dir)
{
    MpegEncContext *s = &r->s;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int has_A = 0, has_B = 0, has_C = 0;
      //int i, j;
    Picture *cur_pic = s->current_picture_ptr;
    const int mask = dir ? MB_TYPE_L1 : MB_TYPE_L0;
    int type = cur_pic->mb_type[mb_pos];

    int *cur_mv=cur_pic->motion_val[dir][mv_pos];

    D32SLL(xr1,xr0,xr0,xr2,1);//xr1=A,xr2=B
    D32SLL(xr3,xr0,xr0,xr0,1);//xr3=C
    if((r->avail_cache[6-1] & type) & mask){
	S32LDD(xr1,cur_mv,-4);//xr1=mv_left
	has_A = 1;
    }
    if((r->avail_cache[6-4] & type) & mask){
	S32LDDV(xr2,cur_mv,-s->b8_stride,2);//xr2=mv_above
	has_B = 1;
    }
    if(r->avail_cache[6-4] && (r->avail_cache[6-2] & type) & mask){
	S32LDDV(xr3,cur_mv,-s->b8_stride+2,2);//xr3=mv_dia
	has_C = 1;
    }else if((s->mb_x+1) == s->mb_width && (r->avail_cache[6-5] & type) & mask){
	S32LDDV(xr3,cur_mv,-s->b8_stride-1,2);//xr3=mv_dia
	has_C = 1;
    }

    if(has_A + has_B + has_C != 3){
	Q16ACC_AA(xr1,xr2,xr3,xr0);//xr1=A+B+C
	if(has_A + has_B + has_C == 2){
	    Q16SLR(xr4,xr1,xr0,xr0,15);//xr4=sign
	    S32AND(xr5,xr1,xr4);//xr5=minus&odds
	    D16AVG(xr1,xr5,xr1);//xr1=xr1/2
	}
    }else{
	D16MAX(xr4,xr1,xr2);//xr4=max of (A,B)
	D16MIN(xr5,xr1,xr2);//xr5=min of (A,B)
	D16MIN(xr4,xr4,xr3);//xr4:comp max(A,B) and C, get rid of the MAX of (A,B,C)
	D16MAX(xr1,xr4,xr5);//xr1:get rid of the MIN of (A,B,C), xr1=mid
    }

    S32LDDV(xr6,r->dmv,dir,2);
    Q16ADD_AA_WW(xr9,xr1,xr6,xr0);

    S32STD(xr9,cur_mv,0);
    S32STD(xr9,cur_mv,4);
    S32STDV(xr9,cur_mv,s->b8_stride,2);
    S32STDV(xr9,cur_mv,1 + s->b8_stride,2);
  
}

static const int chroma_coeffs[3] = { 0, 3, 5 };
/**
 * generic motion compensation function
 *
 * @param r decoder context
 * @param block_type type of the current block
 * @param xoff horizontal offset from the start of the current block
 * @param yoff vertical offset from the start of the current block
 * @param mv_off offset to the motion vector information
 * @param width width of the current partition in 8x8 blocks
 * @param height height of the current partition in 8x8 blocks
 * @param dir motion compensation direction (i.e. from the last or the next reference frame)
 * @param thirdpel motion vectors are specified in 1/3 of pixel
 * @param qpel_mc a set of functions used to perform luma motion compensation
 * @param chroma_mc a set of functions used to perform chroma motion compensation
 */
/** number of motion vectors in each macroblock type */
static const int num_mvs[RV34_MB_TYPES] = { 0, 0, 1, 4, 1, 1, 0, 0, 2, 2, 2, 1 };

static av_always_inline int svq3_get_se_golombl(GetBitContext *gb){
    unsigned int buf,t;
    int log;

    t=get_bits1(gb);
    buf=show_bits(gb,8);
    buf|=(t<<8);

    if(buf&0x155){
	buf >>= 1;
	if(ff_interleaved_golomb_vlc_len[buf]-1) skip_bits(gb,ff_interleaved_golomb_vlc_len[buf]-1);

	return ff_interleaved_se_golomb_vlc_code[buf];
    }else{
	skip_bits(gb,7);
	buf=(buf<<23)| 1 ;

	for(log=31; (buf & 0x80000000) == 0; log--){
	    buf = (buf << 2) - ((buf << log) >> (log - 1)) + (buf >> 30);
	    log--;
	    if(buf & 0x80000000) break;
	    buf = (buf << 2) - ((buf << log) >> (log - 1)) + (buf >> 30);
	    log--;
	    if(buf & 0x80000000) break;
	    buf = (buf << 2) - ((buf << log) >> (log - 1)) + (buf >> 30);
	    log--;
	    if(buf & 0x80000000) break;
	    if(log==16) return INVALID_VLC;
	    buf = (buf << 2) - ((buf << log) >> (log - 1)) + (buf >> 30);
	    if(log<28) skip_bits(gb,8);
	    buf|=(show_bits(gb,8)<<24);
	}

	if((63 - 2*log)%8) {skip_bits(gb,(63 - 2*log)%8);}

	return (signed) (((((buf << log) >> log) - 1) ^ -(buf & 0x1)) + 1) >> 1;
    }
}

static av_always_inline int rv40_decode_mv(RV34DecContext *r, int block_type)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int i, l;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int next_bt;
    int pbdir;

      //int midx = mv_pos;
    memset(r->dmv, 0, sizeof(r->dmv));

    for(i = 0; i < num_mvs[block_type]; i++){
	r->dmv[i][0] = svq3_get_se_golomb(gb);
	r->dmv[i][1] = svq3_get_se_golomb(gb);
    }

    int *dmb_mv_fw=(int*)dMB->motion_val[0];
    int *dmb_mv_bw=(int*)dMB->motion_val[1];
    int *mv_fw=(int*)(s->current_picture_ptr->motion_val[0][mv_pos]);
    int *mv_bw=(int*)(s->current_picture_ptr->motion_val[1][mv_pos]);
    short (*ref_mv)[2]=s->next_picture_ptr->motion_val[0][mv_pos];
    switch(block_type){
    case RV34_MB_TYPE_INTRA:
    case RV34_MB_TYPE_INTRA16x16:
	mv_fw[0]=mv_fw[1]=0;
	mv_fw[s->b8_stride]=mv_fw[s->b8_stride+1]=0;
	dmb_mv_fw[0]=0;
	return 0;
    case RV34_MB_SKIP:
	if(s->pict_type == FF_P_TYPE){
	    mv_fw[0]=mv_fw[1]=0;
	    mv_fw[s->b8_stride]=mv_fw[s->b8_stride+1]=0;
	    dmb_mv_fw[0]=0;
	    break;
	}
    case RV34_MB_B_DIRECT:
	  //surprisingly, it uses motion scheme from next reference frame
	next_bt = s->next_picture_ptr->mb_type[s->mb_x + s->mb_y * s->mb_stride];
	dMB->next_bt = next_bt;
	int refdist = GET_PTS_DIFF(r->next_pts, r->last_pts);

	if(IS_INTRA(next_bt) || IS_SKIP(next_bt) || !refdist){
	    mv_bw[0]=mv_bw[1]=0;
	    mv_bw[s->b8_stride]=mv_bw[s->b8_stride+1]=0;
	    dmb_mv_fw[0] = 0;
	    dmb_mv_bw[0] = 0;
	}else{
	    for(l = 0; l < 2; l++){
		int dist = l ? -GET_PTS_DIFF(r->next_pts, r->cur_pts) : GET_PTS_DIFF(r->cur_pts, r->last_pts);
		int mul=((dist << 15) / refdist);

		S32I2M(xr1,mul);
		S32LDD(xr2,ref_mv,0);//xr2=refmv[0]
		D16MULF_LW(xr3,xr1,xr2);//xr3=((refmv[0] * mul + 0x4000) >> 15)
		S32LDD(xr4,ref_mv,4);//xr4=refmv[1]

		S32STD(xr3,dmb_mv_fw,0);//store to dmb_mv[l][0]
		D16MULF_LW(xr5,xr1,xr4);//xr5=((refmv[1] * mul + 0x4000) >> 15)
		S32LDDV(xr6,ref_mv,s->b8_stride,2);//xr6=refmv[s->b8_stride]

		S32STD(xr5,dmb_mv_fw,4);//store to dmb_mv[l][1]
		D16MULF_LW(xr7,xr1,xr6);//xr7=((refmv[s->b8_stride] * mul + 0x4000) >> 15)
		S32LDDV(xr8,ref_mv,s->b8_stride+1,2);//xr8=refmv[s->b8_stride+1]

		S32STD(xr7,dmb_mv_fw,8);//store to dmb_mv[l][2]
		D16MULF_LW(xr9,xr1,xr8);//xr9=((refmv[s->b8_stride+1] * mul + 0x4000) >> 15)
		S32STD(xr9,dmb_mv_fw,12);//store to dmb_mv[l][3]

		dmb_mv_fw+=4;
	    }
	    S32STD(xr3,mv_bw,0);//store to mv_bw[l][0]
	    S32STD(xr5,mv_bw,4);//store to mv_bw[l][1]
	    S32STDV(xr7,mv_bw,s->b8_stride,2);//store to mv_bw[l][s->b8_stride]
	    S32STDV(xr9,mv_bw,s->b8_stride+1,2);//store to mv_bw[l][s->b8_stride+1]
	}
	mv_fw[0]=mv_fw[1]=0;
	mv_fw[s->b8_stride]=mv_fw[s->b8_stride+1]=0;
	break;
    case RV34_MB_P_16x16:
    case RV34_MB_P_MIX16x16:
	rv40_pred_mv(r, block_type, 0, 0);
	S32STD(xr9,mv_fw,0);
	S32STD(xr9,mv_fw,4);
	S32STDV(xr9,mv_fw,s->b8_stride,2);
	S32STDV(xr9,mv_fw,s->b8_stride+1,2);
	S32STD(xr9,dmb_mv_fw,0);
	break;
    case RV34_MB_B_FORWARD:
    case RV34_MB_B_BACKWARD:
	((int*)r->dmv[1])[0] = ((int*)r->dmv[0])[0];
	pbdir = (block_type == RV34_MB_B_BACKWARD);
	rv40_pred_mv_b  (r, block_type, pbdir);
	dmb_mv_fw[0]=mv_fw[0];
	dmb_mv_bw[0]=mv_bw[0];
	i_movz(mv_fw,mv_bw,pbdir);
	mv_fw[0]=mv_fw[1]=0;
	mv_fw[s->b8_stride]=mv_fw[s->b8_stride+1]=0;
	break;
    case RV34_MB_P_16x8:
    case RV34_MB_P_8x16:
	rv40_pred_mv(r, block_type, 0, 0);
	S32STD(xr9,mv_fw,0);
	S32STD(xr9,dmb_mv_fw,0);
	rv40_pred_mv(r, block_type, 1 + (block_type == RV34_MB_P_16x8), 1);
	S32STDV(xr9,mv_fw,s->b8_stride+1,2);
	if(block_type == RV34_MB_P_16x8){
	    mv_fw[1]=mv_fw[0];
	    dmb_mv_fw[2]=mv_fw[s->b8_stride]=mv_fw[s->b8_stride+1];
	}
	if(block_type == RV34_MB_P_8x16){
	    dmb_mv_fw[1]=mv_fw[1]=mv_fw[s->b8_stride+1];
	    mv_fw[s->b8_stride]=mv_fw[0];
	}	
	break;
    case RV34_MB_B_BIDIR:
	rv40_pred_mv_b  (r, block_type, 0);
	rv40_pred_mv_b  (r, block_type, 1);
	dmb_mv_fw[0]=mv_fw[0];
	dmb_mv_bw[0]=mv_bw[0];
	break;
    case RV34_MB_P_8x8:
	for(i=0;i< 4;i++){
	    rv40_pred_mv(r, block_type, i, i);
	    S32STDV(xr9,mv_fw,s->b8_stride*(i>>1)+(i&1),2);
	    S32STDV(xr9,dmb_mv_fw,i,2);
	}
	break;
    }
      // rv34_decode_mv_aux(r,block_type);
    return 0;
}


/** @} */ // mv group

/**
 * @defgroup recons Macroblock reconstruction functions
 * @{
 */
/** mapping of RV30/40 intra prediction types to standard H.264 types */
#ifdef JZC_VMAU_OPT
static const uint8_t ittrans_mau[8][9] = {
    {
	DC_128_PRED,DC_128_PRED,DC_128_PRED,DC_128_PRED,DC_128_PRED,
	DC_128_PRED,DC_128_PRED,DC_128_PRED,DC_128_PRED,
    },
    {
	LEFT_DC_PRED, HOR_PRED, HOR_PRED, DIAG_DOWN_RIGHT_PRED, DIAG_DOWN_LEFT_PRED,
	VERT_RIGHT_PRED, VERT_LEFT_PRED, HOR_UP_PRED, HOR_DOWN_PRED,
    },
    {
	TOP_DC_PRED, VERT_PRED, VERT_PRED, DIAG_DOWN_RIGHT_PRED, DIAG_DOWN_LEFT_PRED,
	VERT_RIGHT_PRED, VERT_LEFT_PRED, HOR_UP_PRED, HOR_DOWN_PRED,
    },
    {
	DC_PRED, VERT_PRED, HOR_PRED, DIAG_DOWN_RIGHT_PRED, DIAG_DOWN_LEFT_PRED,
	VERT_RIGHT_PRED, VERT_LEFT_PRED, HOR_UP_PRED, HOR_DOWN_PRED,
    },
    {
	DC_128_PRED,DC_128_PRED,DC_128_PRED,DC_128_PRED,DC_128_PRED,
	DC_128_PRED,DC_128_PRED,DC_128_PRED,DC_128_PRED,
    },
    {
	LEFT_DC_PRED, HOR_PRED, HOR_PRED, DIAG_DOWN_RIGHT_PRED, DIAG_DOWN_LEFT_PRED,
	VERT_RIGHT_PRED, VERT_LEFT_PRED, HOR_UP_PRED, HOR_DOWN_PRED,
    },
    {
	TOP_DC_PRED, VERT_PRED, VERT_PRED, DIAG_DOWN_RIGHT_PRED, DIAG_DOWN_LEFT_PRED,
	VERT_RIGHT_PRED, VERT_LEFT_PRED, HOR_UP_PRED, HOR_DOWN_PRED,
    },
    {
	DC_PRED, VERT_PRED, HOR_PRED, DIAG_DOWN_RIGHT_PRED, DIAG_DOWN_LEFT_PRED,
	VERT_RIGHT_PRED, VERT_LEFT_PRED, HOR_UP_PRED, HOR_DOWN_PRED,
    },

};
static const uint8_t ittrans16mau[4][4] = {
    {DC_PRED8x8, DC_PRED8x8, DC_PRED8x8, DC_PRED8x8},
    {DC_PRED8x8, HOR_PRED8x8, HOR_PRED8x8, HOR_PRED8x8},

    {DC_PRED8x8, VERT_PRED8x8, VERT_PRED8x8, VERT_PRED8x8},
    {DC_PRED8x8, VERT_PRED8x8, HOR_PRED8x8, PLANE_PRED8x8},
};
static const uint8_t ittrans16mau_2[4][4] = {
    {DC_PRED8x8, DC_PRED8x8, DC_PRED8x8, DC_PRED8x8},
    {DC_PRED8x8, HOR_PRED8x8, HOR_PRED8x8, DC_PRED8x8},

    {DC_PRED8x8, VERT_PRED8x8, VERT_PRED8x8, DC_PRED8x8},
    {DC_PRED8x8, VERT_PRED8x8, HOR_PRED8x8, DC_PRED8x8},
};
#endif
/**
 * Perform 4x4 intra prediction.
 */
static av_always_inline void rv40_output_macroblock(RV34DecContext *r, int8_t *intra_types, int cbp, int is16)
{
      //MpegEncContext *s = &r->s;
      //DSPContext *dsp = &s->dsp;
    int itype;
    int idx;
    int i,j,num;

    dMB->mau_ypm[1] = 0;
    if(!is16){
	uint8_t avail[6*8] = {0};
	  // Set neighbour information.
	if(r->avail_cache[1])
	    avail[0] = 1;
	if(r->avail_cache[2])
	    avail[1] = avail[2] = 1;
	if(r->avail_cache[3])
	    avail[3] = avail[4] = 1;
	if(r->avail_cache[4])
	    avail[5] = 1;
	if(r->avail_cache[5])
	    avail[8] = avail[16] = 1;
	if(r->avail_cache[9])
	    avail[24] = avail[32] = 1;

	fill_rectangle(r->avail_cache + 6, 2, 2, 4, 0, 4);
	dMB->mau_ypm[0] = 0;
	dMB->mau_cpm = 0;
	dMB->mau_na = 0;

	idx=5;
	for(j = 0; j < 4; j++){
	    idx+=4;
	      //idx = 9 + j*8;
	    for(i = 0; i < 4; i++,idx++){
		num = (j<<2) + i;
#ifdef JZC_VMAU_OPT
		itype=ittrans_mau[(avail[idx+7]<<2)|(avail[idx-8]<<1)|(avail[idx-1])][intra_types[i]];
		dMB->mau_ypm[j>>1] |= itype<<((num&7)<<2);
#endif
		avail[idx] = 1;
	    }
	    intra_types += r->intra_types_stride;
	}
	intra_types -= r->intra_types_stride<<2;
	idx=4;
	for(j = 0; j < 2; j++){
	    idx+=2;
	      //idx = 6 + j*4;
	    for(i = 0; i < 2; i++, idx++){
		num = 16 + (j<<1) + i;
		int up=!!r->avail_cache[idx-4];
		  //int right=!!r->avail_cache[idx-3];
		int left=!!r->avail_cache[idx-1];
#ifdef JZC_VMAU_OPT
		itype=ittrans_mau[(!(i|j)<<2)|(up<<1)|(left)][intra_types[(i+(j?r->intra_types_stride:0))<<1]];
		dMB->mau_cpm |= itype<<(((j<<1) + i)<<2);
#endif
		r->avail_cache[idx] = 1;
	    }
	}
    }else{
	dMB->mau_mcbp |= 0x2;//mbtype 16x16intra
	int tbl_i=(!!r->avail_cache[6-4]<<1)|!!r->avail_cache[6-1];
#ifdef JZC_VMAU_OPT
	dMB->mau_na = (!r->avail_cache[6-4]<<2)|!r->avail_cache[6-1];
#endif
	itype = intra_types[0];
	if(itype!=3){
#ifdef JZC_VMAU_OPT
	    dMB->mau_ypm[0] = dMB->mau_cpm = ittrans16mau[tbl_i][itype];
#endif
	}else{
#ifdef JZC_VMAU_OPT
	    dMB->mau_ypm[0] = ittrans16mau[tbl_i][itype];
	    dMB->mau_cpm = DC_PRED8x8;
#endif
	}
    }
}

/**
 * @addtogroup bitstream
 * Decode macroblock header and return CBP in case of success, -1 otherwise.
 */
#ifdef JZC_VLC_HW_OPT
uint32_t vlc_intra_tbls,vlc_inter_tbls;
#endif
static av_always_inline int rv40_decode_mb_header(RV34DecContext *r, int8_t *intra_types)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int i, t;
    int cur_mb_type;
    static int bk_mb_type;
    uint32_t mb_type;
    if(!r->si.type){
	dMB->is16 = get_bits1(gb);
	if(!dMB->is16){
	    if(!get_bits1(gb))
		av_log(s->avctx, AV_LOG_ERROR, "Need DQUANT\n");
	}
	mb_type = s->current_picture_ptr->mb_type[mb_pos]=dMB->is16 ? MB_TYPE_INTRA16x16 : MB_TYPE_INTRA;
	cur_mb_type = r->block_type = dMB->is16 ? RV34_MB_TYPE_INTRA16x16 : RV34_MB_TYPE_INTRA;
#ifdef JZC_DBLK_OPT
	r->mbtype[s->mb_x] = cur_mb_type;
#endif
    }else{
	r->block_type = rv40_decode_mb_info(r,bk_mb_type);
	cur_mb_type = r->block_type;
#ifdef JZC_DBLK_OPT
	r->mbtype[s->mb_x] = cur_mb_type;
#endif
	if(cur_mb_type == -1)
	    return -1;
	mb_type=s->current_picture_ptr->mb_type[mb_pos] = rv40_mb_type_to_lavc[cur_mb_type];

	if(cur_mb_type == RV34_MB_SKIP){
	    if(s->pict_type == FF_P_TYPE)
		cur_mb_type = RV34_MB_P_16x16;
	    if(s->pict_type == FF_B_TYPE)
		cur_mb_type = RV34_MB_B_DIRECT;
	}
	bk_mb_type=r->mb_type[s->mb_x];
	r->mb_type[s->mb_x]=cur_mb_type;

	dMB->is16 = !!IS_INTRA16x16(mb_type);
	rv40_decode_mv(r, r->block_type);

	if(r->block_type == RV34_MB_SKIP){
	      //fill_rectangle(intra_types, 4, 4, r->intra_types_stride, 0, sizeof(intra_types[0]));
	    uint8_t * p = intra_types;
	    S32STD(xr0, p, 0);
	    S32SDIV(xr0, p, r->intra_types_stride, 0);
	    S32SDIV(xr0, p, r->intra_types_stride, 0);
	    S32SDIV(xr0, p, r->intra_types_stride, 0);
	    return 0;
	}
	r->chroma_vlc = 1;
	r->luma_vlc   = 0;
    }

    if(IS_INTRA(mb_type)){
	if(dMB->is16){
	    t = get_bits(gb, 2);
	      //fill_rectangle(intra_types, 4, 4, r->intra_types_stride, t, sizeof(intra_types[0]));
	    t = t * 0x01010101;
	    S32I2M(xr1, t);
	    uint8_t * p = intra_types;
	    S32STD(xr1, p, 0);
	    S32SDIV(xr1, p, r->intra_types_stride, 0);
	    S32SDIV(xr1, p, r->intra_types_stride, 0);
	    S32SDIV(xr1, p, r->intra_types_stride, 0);
	    r->luma_vlc   = 2;
	}else{
	    if(rv40_decode_intra_types(r, gb, intra_types) < 0)
		return -1;
	    r->luma_vlc   = 1;
	}
	r->chroma_vlc = 0;
	r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 0);
    }else{
	/* for(i = 0; i < 16; i++) */
	/*     intra_types[(i & 3) + (i>>2) * r->intra_types_stride] = 0; */
	uint8_t * p = intra_types;
	S32STD(xr0, p, 0);
	S32SDIV(xr0, p, r->intra_types_stride, 0);
	S32SDIV(xr0, p, r->intra_types_stride, 0);
	S32SDIV(xr0, p, r->intra_types_stride, 0);
	r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);
	if(cur_mb_type == RV34_MB_P_MIX16x16){
	    dMB->is16 = 1;
	    r->chroma_vlc = 1;
	    r->luma_vlc   = 2;
	    r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 0);
	}
    }
    return rv40_decode_cbp(gb, r->cur_vlcs, dMB->is16);
}

/**
 * @addtogroup recons
 * @{
 */
/**
 * mask for retrieving all bits in coded block pattern
 * corresponding to one 8x8 block
 */
#define LUMA_CBP_BLOCK_MASK 0x33

#define U_CBP_MASK 0x0F0000
#define V_CBP_MASK 0xF00000

#ifdef JZC_DBLK_OPT
static av_always_inline int rv40_set_deblock_coef(RV34DecContext *r)
{
    MpegEncContext *s = &r->s;
    int hmvmask = 0, vmvmask = 0;
    int midx = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int16_t (*motion_val)[2];  
    int tmp1, tmp2, tmp3, tmp4;

#if 0
    if (s->pict_type == FF_B_TYPE && ((r->block_type == RV34_MB_SKIP) || (r->block_type == RV34_MB_B_DIRECT)))
	motion_val = &s->next_picture_ptr->motion_val[0][midx];
    else
#endif
	motion_val = &s->current_picture_ptr->motion_val[0][midx];

    int * mval = motion_val + s->b8_stride; 
    S32LUI(xr15,3,ptn4);         
    S32LDD(xr1,motion_val,-0x4); 
    S32LDD(xr2,motion_val,0x0);  
    S32LDD(xr3,motion_val,0x4);  

    S32LDD(xr7,mval,-0x4); 
    S32LDD(xr8,mval,0x0);  
    S32LDD(xr9,mval,0x4);  
  
    Q16ADD_SS_WW(xr5,xr2,xr1,xr0);
    Q16ADD_SS_WW(xr6,xr3,xr2,xr0);
    Q16ADD_SS_WW(xr12,xr8,xr7,xr0);
    Q16ADD_SS_WW(xr13,xr9,xr8,xr0);

    D16CPS(xr5,xr5,xr5);   
    D16CPS(xr6,xr6,xr6);   
    D16SLT(xr5,xr15,xr5);
    D16SLT(xr6,xr15,xr6);
  
    D16CPS(xr12,xr12,xr12);   
    D16CPS(xr13,xr13,xr13);   
    D16SLT(xr12,xr15,xr12);
    D16SLT(xr13,xr15,xr13);

    tmp1 = S32M2I(xr5);
    tmp2 = S32M2I(xr6);  
    tmp3 = S32M2I(xr12);
    tmp4 = S32M2I(xr13);  

    if (tmp1) vmvmask |= 0x11;
    if (tmp2) vmvmask |= 0x44;
    if (tmp3) vmvmask |= 0x1100;
    if (tmp4) vmvmask |= 0x4400;

    if (s->mb_y){
	int *mval1 = motion_val - s->b8_stride;    
	S32LDD(xr4,mval1,0x0);
	S32LDD(xr10,mval1,0x4);
	Q16ADD_SS_WW(xr5,xr2,xr4,xr0); 
	Q16ADD_SS_WW(xr6,xr3,xr10,xr0); 
    
	D16CPS(xr5,xr5,xr5);   
	D16CPS(xr6,xr6,xr6);   
	D16SLT(xr5,xr15,xr5);
	D16SLT(xr6,xr15,xr6);
	tmp1 = S32M2I(xr5);
	tmp2 = S32M2I(xr6);  
	if (tmp1) hmvmask |= 0x03;
	if (tmp2) hmvmask |= 0x0c;
    }

    Q16ADD_SS_WW(xr5,xr8,xr2,xr0); 
    Q16ADD_SS_WW(xr6,xr9,xr3,xr0); 

    D16CPS(xr5,xr5,xr5);   
    D16CPS(xr6,xr6,xr6);   
    D16SLT(xr5,xr15,xr5);
    D16SLT(xr6,xr15,xr6);
    tmp2 = S32M2I(xr5);
    tmp3 = S32M2I(xr6);  
    if (tmp2) hmvmask |= 0x300;
    if (tmp3) hmvmask |= 0xc00;

    if(s->first_slice_line)
	hmvmask &= ~0x000F;
    if(!s->mb_x)
	vmvmask &= ~0x1111;
    return hmvmask | vmvmask;
}
#else
static av_always_inline int is_mv_diff_gt_3(int16_t (*motion_val)[2], int step)
{
    int d;
    d = motion_val[0][0] - motion_val[-step][0];
    if(d < -3 || d > 3)
	return 1;
    d = motion_val[0][1] - motion_val[-step][1];
    if(d < -3 || d > 3)
	return 1;
    return 0;
}

static inline int rv40_set_deblock_coef(RV34DecContext *r)
{
    MpegEncContext *s = &r->s;
    int hmvmask = 0, vmvmask = 0, i, j;
    int midx = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int16_t (*motion_val)[2] = &s->current_picture_ptr->motion_val[0][midx];
    for(j = 0; j < 16; j += 8){
	for(i = 0; i < 2; i++){
	    if(is_mv_diff_gt_3(motion_val + i, 1))
		vmvmask |= 0x11 << (j + i*2);
	    if((j || s->mb_y) && is_mv_diff_gt_3(motion_val + i, s->b8_stride))
		hmvmask |= 0x03 << (j + i*2);
	}
	motion_val += s->b8_stride;
    }

    if(s->first_slice_line)
	hmvmask &= ~0x000F;
    if(!s->mb_x)
	vmvmask &= ~0x1111;
    return hmvmask | vmvmask;
}
#endif

#ifdef  JZC_DCORE_OPT
static av_always_inline int rv40_decode_macroblock(RV34DecContext *r, int8_t *intra_types)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int cbp, cbp2;
    int i;

      //DCTELEM block16[16];
    int luma_dc_quant;
    int dist, pos;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
      // Calculate which neighbours are available. Maybe it's worth optimizing too.
    uint32_t *pavail = r->avail_cache - 1;
    for(i=0; i<3; i++){
	S32SDI(xr0,pavail,0x4);
	S32SDI(xr0,pavail,0x4);
	S32SDI(xr0,pavail,0x4);
	S32SDI(xr0,pavail,0x4);
    }

    S32I2M(xr11,1);
    pavail = r->avail_cache;
    S32STD(xr11,pavail,24);
    S32STD(xr11,pavail,28);
    S32STD(xr11,pavail,40);
    S32STD(xr11,pavail,44);

    uint32_t *mtype = s->current_picture_ptr->mb_type + mb_pos;
    dist = (s->mb_x - s->resync_mb_x) + (s->mb_y - s->resync_mb_y) * s->mb_width;
    S32LDD(xr1,mtype,-4);
    mtype-=s->mb_stride;
    S32LDD(xr2,mtype,-4);
    S32LDD(xr3,mtype,0);
    S32LDD(xr4,mtype,4);
    if(s->mb_x && dist){
	S32STD(xr1,pavail,20);
	S32STD(xr1,pavail,36);
    }
    if(dist >= s->mb_width){
	S32STD(xr3,pavail,8);
	S32STD(xr3,pavail,12);
    }
    if(((s->mb_x+1) < s->mb_width) && dist >= s->mb_width - 1)
	S32STD(xr4,pavail,16);
    if(s->mb_x && dist > s->mb_width)
	S32STD(xr2,pavail,4);

    s->qscale = r->si.quant;
    dMB->qscale = s->qscale;
    dist=r->mbtype[s->mb_x];
    cbp = cbp2 = rv40_decode_mb_header(r, intra_types);

    r->cbp_luma  [mb_pos] = cbp;
    r->cbp_chroma[mb_pos] = cbp >> 16;
    if(s->pict_type == FF_I_TYPE)
	r->deblock_coefs[mb_pos] = 0xFFFF;
    else
	r->deblock_coefs[mb_pos] = rv40_set_deblock_coef(r) | r->cbp_luma[mb_pos];

    dMB->cbp = cbp; // used for VMAU

#ifdef JZC_DBLK_OPT
    dMB->mbtype = r->mbtype[s->mb_x];

    dMB->mbtype_cur = rv40_mb_type_to_lavc[ dMB->mbtype ];
    dMB->cur_coef = r->deblock_coefs[mb_pos];
    if(IS_INTRA(dMB->mbtype_cur) || IS_SEPARATE_DC(dMB->mbtype_cur))
	r->cbp_luma  [mb_pos] = r->deblock_coefs[mb_pos] = 0xFFFF;
    if(IS_INTRA(dMB->mbtype_cur))
	r->cbp_chroma[mb_pos] = 0xFF;
    dMB->cur_cbp = r->cbp_luma[mb_pos] | (r->cbp_chroma[mb_pos] << 16);

    if( s->mb_y == 0 ) {
	dMB->mbtype_above = 0;
	dMB->up_coef = 0;
	dMB->up_cbp = 0;
    } else {
	dMB->mbtype_above = rv40_mb_type_to_lavc[ dist ];
	dMB->up_coef = r->deblock_coefs[mb_pos - s->mb_stride];
	dMB->up_cbp = r->cbp_luma[mb_pos - s->mb_stride] | (r->cbp_chroma[mb_pos - s->mb_stride] << 16);
    }
#endif

    s->current_picture_ptr->qscale_table[mb_pos] = s->qscale;

    if(cbp == -1)
	return -1;

    dMB->mau_mcbp = (0x9<<2);//rv9 alway add error
    luma_dc_quant = r->block_type == RV34_MB_P_MIX16x16 ? r->luma_dc_quant_p[s->qscale] : r->luma_dc_quant_i[s->qscale];
    int main_len = 0;
    uint8_t * addr = dMB->block16; // reduce bus press
    if(dMB->is16){
	  //memset(dMB->block16, 0, 32);
	i_pref(30, addr, 0);
	addr += 32;
	MXU_SETZERO(dMB->block16,1);
	rv40_decode_block(dMB->block16, gb, r->cur_vlcs, 3, 0, 0);
#ifdef JZC_VMAU_OPT
	main_len += 16;
	dMB->mau_mcbp |= 1<<4;//need dc transfrom
#endif
    }

    for(i = 0; i < 16; i++, cbp >>= 1){
	if(cbp & 1){
#ifdef JZC_VMAU_OPT
	    i_pref(30, addr, 0);
	    addr += 32;
	    MXU_SETZERO(dMB->block16 + main_len,1);
	    rv40_decode_block(dMB->block16 + main_len, gb, r->cur_vlcs, r->luma_vlc, 0, i);
	    main_len += 16;
#endif
	}
    }

    if(r->block_type == RV34_MB_P_MIX16x16){
#ifdef JZC_VLC_HW_OPT
	vlc_ctrl=CALC_VLC2_CTRL_INTER();
	sec_pat_ctrl=sec_pat_ctrl_inter;
	r->cur_vlcs = vlc_inter_tbls;
#else
	r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);
#endif
    }
    for(; i < 24; i++, cbp >>= 1){
	if(cbp & 1){
#ifdef JZC_VMAU_OPT
	    i_pref(30, addr, 0);
	    addr += 32;
	    MXU_SETZERO(dMB->block16 + main_len,1);
	    rv40_decode_block(dMB->block16 + main_len, gb, r->cur_vlcs, r->chroma_vlc, 1,i);
	    main_len += 16;
#endif
	}
    }

    dMB->mau_mlen = main_len;
    if(IS_INTRA(s->current_picture_ptr->mb_type[mb_pos])){
	dMB->mau_mcbp |= 1<<3;//prediction en
	rv40_output_macroblock(r, intra_types, cbp2, dMB->is16);
    }
    
    return 0;
}
#else
static int rv34_decode_macroblock(RV34DecContext *r, int8_t *intra_types)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int cbp, cbp2;
    int i, blknum, blkoff;
    DCTELEM block16[64];
    int luma_dc_quant;
    int dist;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;

      // Calculate which neighbours are available. Maybe it's worth optimizing too.
    memset(r->avail_cache, 0, sizeof(r->avail_cache));
    fill_rectangle(r->avail_cache + 6, 2, 2, 4, 1, 4);
    dist = (s->mb_x - s->resync_mb_x) + (s->mb_y - s->resync_mb_y) * s->mb_width;
    if(s->mb_x && dist)
	r->avail_cache[5] =
	    r->avail_cache[9] = s->current_picture_ptr->mb_type[mb_pos - 1];
    if(dist >= s->mb_width)
	r->avail_cache[2] =
	    r->avail_cache[3] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride];
    if(((s->mb_x+1) < s->mb_width) && dist >= s->mb_width - 1)
	r->avail_cache[4] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride + 1];
    if(s->mb_x && dist > s->mb_width)
	r->avail_cache[1] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride - 1];

    s->qscale = r->si.quant;
    cbp = cbp2 = rv34_decode_mb_header(r, intra_types);
    r->cbp_luma  [mb_pos] = cbp;
    r->cbp_chroma[mb_pos] = cbp >> 16;
    if(s->pict_type == FF_I_TYPE)
	r->deblock_coefs[mb_pos] = 0xFFFF;
    else
	r->deblock_coefs[mb_pos] = rv34_set_deblock_coef(r) | r->cbp_luma[mb_pos];
    s->current_picture_ptr->qscale_table[mb_pos] = s->qscale;

    if(cbp == -1)
	return -1;

    luma_dc_quant = r->block_type == RV34_MB_P_MIX16x16 ? r->luma_dc_quant_p[s->qscale] : r->luma_dc_quant_i[s->qscale];
    if(dMB->is16){
	memset(block16, 0, sizeof(block16));
	rv34_decode_block(block16, gb, r->cur_vlcs, 3, 0);
	rv34_dequant4x4_16x16(block16, rv34_qscale_tab[luma_dc_quant],rv34_qscale_tab[s->qscale]);
	rv34_inv_transform_noround(block16);
    }

    for(i = 0; i < 16; i++, cbp >>= 1){
	if(!dMB->is16 && !(cbp & 1)) continue;
	blknum = ((i & 2) >> 1) + ((i & 8) >> 2);
	blkoff = ((i & 1) << 2) + ((i & 4) << 3);
	if(cbp & 1)
	    rv34_decode_block(s->block[blknum] + blkoff, gb, r->cur_vlcs, r->luma_vlc, 0);
	rv34_dequant4x4(s->block[blknum] + blkoff, rv34_qscale_tab[s->qscale],rv34_qscale_tab[s->qscale]);
	if(dMB->is16) //FIXME: optimize
	    s->block[blknum][blkoff] = block16[(i & 3) | ((i & 0xC) << 1)];
	rv34_inv_transform(s->block[blknum] + blkoff);
    }

    int m,n,l;
    for(m = 0; m < 4; m++)
	{
	    int16_t *src = s->block[m];
	    int16_t *dst = s->block[m];
	    for(n = 0; n < 8; n++)
		{
		    for(l = 0; l < 8; l++){
			  //dst[n*8+l] = src[n*8+l];	    
			printf("%x ",dst[n*8+l]);
		    }
		    printf("\n");
		}
	    printf("\n");
	}
    printf("\n");

    if(r->block_type == RV34_MB_P_MIX16x16)
	r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);
    for(; i < 24; i++, cbp >>= 1){
	if(!(cbp & 1)) continue;
	blknum = ((i & 4) >> 2) + 4;
	blkoff = ((i & 1) << 2) + ((i & 2) << 4);
	rv34_decode_block(s->block[blknum] + blkoff, gb, r->cur_vlcs, r->chroma_vlc, 1);
	rv34_dequant4x4(s->block[blknum] + blkoff, rv34_qscale_tab[rv34_chroma_quant[1][s->qscale]],rv34_qscale_tab[rv34_chroma_quant[0][s->qscale]]);
	rv34_inv_transform(s->block[blknum] + blkoff);
    }
    if(IS_INTRA(s->current_picture_ptr->mb_type[mb_pos]))
	rv34_output_macroblock(r, intra_types, cbp2, dMB->is16);
    else
	rv34_apply_differences(r, cbp2);

    return 0;
}
#endif
static av_always_inline int check_slice_end(RV34DecContext *r, MpegEncContext *s)
{
    int bits;
    if(s->mb_y >= s->mb_height){
	return 1;
    }
    if(!s->mb_num_left){
	return 1;
    }
    if(r->s.mb_skip_run > 1){
	return 0;
    }
#ifdef JZC_VLC_HW_OPT
    bits = r->bits - get_bs_index();
#else
    bits = r->bits - get_bits_count(&s->gb);
#endif
#ifdef JZC_VLC_HW_OPT
    if(bits < 0 )
	return 1;
    if(bits < 8)
	if(!show_bits(&s->gb,bits))
	    return 1;
#else
    if(bits < 0 || (bits < 8 && !show_bits(&s->gb, bits)))
	{
	    return 1;
	}
#endif
    return 0;
}
#ifdef JZC_VLC_HW_OPT
#undef get_bits
#undef get_bits1
#undef show_bits
#undef skip_bits
#undef get_vlc2
#undef svq3_get_ue_golomb
#undef svq3_get_se_golomb
#endif


#ifdef JZC_VLC_HW_OPT
int last_quant,last_vlc_set;
uint8_t last_intra_tbl_sel,last_inter_tbl_sel;
#endif

#ifdef WRITE_MBINFO
static void print_mbinfo(MpegEncContext * s, RV9_MB_DecARGs * dmb, uint8_t * mode)
{
    FILE * fp = fopen("/data/real_dmb_info.log", mode);

    int i, j;
    fprintf(fp, "--------- > frame : %d, mb : %d %d < --------\n", mpframe, s->mb_y, s->mb_x);
    fprintf(fp, "new_slice = 0x%x\n", dmb->new_slice);
    fprintf(fp, "er_slice = 0x%x\n", dmb->er_slice);
    fprintf(fp, "mb_x = 0x%x\n", dmb->mb_x);
    fprintf(fp, "mb_y = 0x%x\n\n", dmb->mb_y);

    fprintf(fp, "mbtype = 0x%x\n", dmb->mbtype);
    fprintf(fp, "next_bt = 0x%x\n\n", dmb->next_bt);

    fprintf(fp, "cbp = 0x%x\n", dmb->cbp);
    fprintf(fp, "qscale = 0x%x\n", dmb->qscale);
    fprintf(fp, "mau_na = 0x%x\n", dmb->mau_na); //neighbor avail
    fprintf(fp, "mau_mcbp = 0x%x\n", dmb->mau_mcbp);//main cbp high 8 bits
    fprintf(fp, "mau_mlen = 0x%x\n", dmb->mau_mlen);//main len
    fprintf(fp, "mau_ypm[0] = 0x%x\n", dmb->mau_ypm[0]);//y_pred_mode
    fprintf(fp, "mau_ypm[1] = 0x%x\n", dmb->mau_ypm[1]);//y_pred_mode
    fprintf(fp, "mau_cpm = 0x%x\n\n", dmb->mau_cpm);//c_pred_mode

    fprintf(fp, "mbtype_cur = 0x%x\n", dmb->mbtype_cur);
    fprintf(fp, "mbtype_above = 0x%x\n", dmb->mbtype_above);
    fprintf(fp, "cur_cbp = 0x%x\n", dmb->cur_cbp);
    fprintf(fp, "up_cbp = 0x%x\n", dmb->up_cbp);
    fprintf(fp, "cur_coef = 0x%x\n", dmb->cur_coef);
    fprintf(fp, "up_coef = 0x%x\n", dmb->up_coef);
    fprintf(fp, "is16 = 0x%x\n\n", dmb->is16);

    for(i = 0; i < 2; i++)
	for(j = 0; j < 4; j++){
	    fprintf(fp, "motion_val[%d][%d][0] = 0x%x\n", i, j, dmb->motion_val[i][j][0]);
	    fprintf(fp, "motion_val[%d][%d][1] = 0x%x\n", i, j, dmb->motion_val[i][j][1]);
	}

    fprintf(fp, "\n");
      /* DCTELEM block16[16]; */
      /* DCTELEM block[6][64]; // 144+192=336 words */
    volatile short * src = dmb->block16;
    if( dmb->is16) {
	fprintf(fp, "--------------->  block16 <---------------\n");
	for(i = 0; i < 4; i++) {
	    fprintf(fp, "%04d %04d %04d %04d\n", src[0], src[1], src[2], src[3]);
	    src += 4;
	}
    }
    fprintf(fp, "---------------->  block <----------------\n");
    for(j = 0; j < 24; j++) {
	fprintf(fp, "block %d :\n", j);
	for(i = 0; i < 4; i++) {
	    fprintf(fp, "%04d %04d %04d %04d\n", src[0], src[1], src[2], src[3]);
	    src += 4;
	}
    }

    fclose(fp);
}
#endif

static int rv40_decode_slice(RV34DecContext *r, int end, const uint8_t* buf, int buf_size)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int mb_pos;
    int res;

    EL("A new slice (%d, %d)", s->mb_x, s->mb_y);

#ifdef TEST_TIME
    unsigned int time = GetTimer();
#endif
#ifdef JZC_DCORE_OPT
    RV9_Slice_GlbARGs *dSlice = (int *) TCSM1_VCADDR(TCSM1_SLICE_BUF);
    volatile int * tcsm1_fifo_wp = (int *)TCSM1_VCADDR(TCSM1_MBNUM_WP);
    int *task_fifo_wp;

    int tcsm1_fifo_value = 0;
    *tcsm1_fifo_wp = 0; // write by P0, used by P1
    *(volatile int *)TCSM1_VCADDR(FIFO_ADDR_READY) = 1;

    AUX_RESET();
    *((volatile int *)TCSM1_VCADDR(TCSM1_P1_TASK_DONE)) = 0;
    AUX_START();

    *(volatile unsigned int *)(vpu_base + 0x0) =  (0x1<<9) | (0x1<<30) | (0x1<<29);
    unsigned int tlb_addr;
    dmmu_get_page_table_base_phys(&tlb_addr);
      //EL("[%s] Get tlb phy addr : 0x%08x", __FUNCTION__, tlb_addr);
    *(volatile unsigned int *)(vpu_base + 0x30) = tlb_addr;
#endif

    init_get_bits(&r->s.gb, buf, buf_size*8);
    res = rv40_parse_slice_header(r, gb, &r->si);
    if(res < 0){
	av_log(s->avctx, AV_LOG_ERROR, "Incorrect or unknown slice header\n");
	return -1;
    }

    if ((s->mb_x == 0 && s->mb_y == 0) || s->current_picture_ptr==NULL) {
	if(s->width != r->si.width || s->height != r->si.height){
	    ALOGE("Changing dimensions to %dx%d, s->width = %d, s->height = %d", r->si.width,r->si.height, s->width, s->height);
	    av_log(s->avctx, AV_LOG_DEBUG, "Changing dimensions to %dx%d\n", r->si.width,r->si.height);
	    MPV_common_end(s);
	    s->width  = r->si.width;
	    s->height = r->si.height;
	    avcodec_set_dimensions(s->avctx, s->width, s->height);
	    if(MPV_common_init(s) < 0)
		return -1;
	    r->intra_types_stride = s->mb_width*4 + 4;
	    r->intra_types_hist = av_realloc(r->intra_types_hist, r->intra_types_stride * 5 * sizeof(*r->intra_types_hist));
	    r->mb_type = av_realloc(r->mb_type, r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));
	    r->intra_types = r->intra_types_hist + r->intra_types_stride;
      
	    r->cbp_luma   = av_realloc(r->cbp_luma,   r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_luma));
	    r->cbp_chroma = av_realloc(r->cbp_chroma, r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_chroma));
	    r->deblock_coefs = av_realloc(r->deblock_coefs, r->s.mb_stride * r->s.mb_height * sizeof(*r->deblock_coefs));
#ifdef JZC_DBLK_OPT
	    r->mbtype = av_realloc(r->mbtype, r->s.mb_stride * sizeof(*r->mbtype));
#endif
	    r->task_fifo = jz4740_alloc_frame(r->s.avctx->VpuMem_ptr, 128, (s->mb_width * s->mb_height + 5) * CIRCLE_BUF_LEN);  //FIXME: maybe we should use (s->width+15) / 16
	}
	EL("A new frame : Y : 0x%08x", s->current_picture.data[0]);
	s->pict_type = r->si.type ? r->si.type : FF_I_TYPE;
	if(MPV_frame_start(s, s->avctx) < 0)
	    return -1;
	ff_er_frame_start(s);
	r->cur_pts = r->si.pts;
	if(s->pict_type != FF_B_TYPE){
	    r->last_pts = r->next_pts;
	    r->next_pts = r->cur_pts;
	}
	s->mb_x = s->mb_y = 0;
	jz_dcache_wb();
    }

    if(!r->task_fifo)
	r->task_fifo = jz4740_alloc_frame(r->s.avctx->VpuMem_ptr, 128, (s->mb_width * s->mb_height + 5) * CIRCLE_BUF_LEN);
      //memset(r->task_fifo, 0x0, (s->mb_width * s->mb_height + 5) * CIRCLE_BUF_LEN);
    task_fifo_wp = r->task_fifo;
    *(volatile unsigned int *)TCSM1_VCADDR(TASK_FIFO_ADDR) = r->task_fifo;
    *(volatile int *)TCSM1_VCADDR(FIFO_ADDR_READY) = 0;

    r->si.end = end;
    s->qscale = r->si.quant;

    r->bits = buf_size*8;
    s->mb_num_left = r->si.end - r->si.start;
    r->s.mb_skip_run = 0;

    mb_pos = s->mb_x + s->mb_y * s->mb_width;
    if(r->si.start != mb_pos){
	av_log(s->avctx, AV_LOG_ERROR, "Slice indicates MB offset %d, got %d\n", r->si.start, mb_pos);
	s->mb_x = r->si.start % s->mb_width;
	s->mb_y = r->si.start / s->mb_width;
    }

    memset(r->intra_types_hist, -1, r->intra_types_stride * 5 * sizeof(*r->intra_types_hist));
    s->first_slice_line = 1;
    s->resync_mb_x= s->mb_x;
    s->resync_mb_y= s->mb_y;
    
      /* ***********  get slice info  *********** */
    EL("get slice info");
    RV9_Slice_GlbARGs sdSlice;
    if (s->pict_type != FF_B_TYPE){
	sdSlice.refqp = r->si.quant;
    }else
	sdSlice.refqp = dSlice->refqp;

    sdSlice.pict_type = s->pict_type;
    sdSlice.si_type = r->si.type;
    sdSlice.mb_width = s->mb_width;
    sdSlice.mb_height = s->mb_height;
    sdSlice.linesize = s->linesize;
    sdSlice.uvlinesize = s->uvlinesize;

    sdSlice.last_data[0] = s->last_picture.data[0];
    sdSlice.last_data[1] = s->last_picture.data[1];
    sdSlice.last_data[2] = s->last_picture.data[2];
    
    sdSlice.next_data[0] = s->next_picture.data[0];
    sdSlice.next_data[1] = s->next_picture.data[1];
    sdSlice.next_data[2] = s->next_picture.data[2];

    memcpy(dSlice,&sdSlice,sizeof(RV9_Slice_GlbARGs));

      /* ***********  init hardware  *********** */
#ifdef JZC_MC_OPT
    motion_config_rv9(s);
#endif

#ifdef JZC_DBLK_OPT
#ifdef IPU_BUG
    int y_strd=(s->mb_width*256 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
    int c_strd=(s->mb_width*128 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
#else
    int y_strd=s->mb_width*256;
    int c_strd=s->mb_width*128;
#endif

    hw_dblk_real_init(TCSM1_PADDR(DBLK_CHAIN), //dha
                      TCSM1_PADDR(DBLK_ENDA), //chn_end_base, 
                      s->mb_width,//mb_width, 
                      s->mb_height, // height
                      s->mb_x,//mb_x, 
                      s->mb_y,//mb_y, 
                      s->current_picture.data[0],  //dst_y
                      s->current_picture.data[1],  // dst_uv
                      TCSM1_PADDR(DBLK_DMA_ENDA), // dma_end_base
                      &dblk_gp_slice_end, //slice_end_base
		      y_strd,   // y stride
		      c_strd,   // uv stride
                      (s->mb_width*16 * s->mb_height*16) < (176*144)//mid_mode
                      );
    dblk_gp_slice_end = -1;
    *((volatile int*)TCSM1_VCADDR(DBLK_ENDA)) = -1;
    *((volatile int*)TCSM1_VCADDR(DBLK_DMA_ENDA)) = -1;
#endif
       
#ifdef DEBUG_MC
    if( mpframe == DEBUG_FRAME_NUM && s->mb_y == 12 && s->mb_x == 22 )
	{
	    int i;
	    FILE * fp = fopen("/data/real_config.c", "a+");
	      //motion_config_rv9
	    fprintf(fp, "\n");
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// CTRL\n\n", REG1_CTRL, GET_REG1_CTRL() | (1<<3));
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// RLUT1\n", TAB1_RLUT, GET_TAB1_RLUT(0));
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// RLUT1\n", (TAB1_RLUT + 1 * 4), GET_TAB1_RLUT(1));
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// RLUT1\n", (TAB1_RLUT + 32 * 4), GET_TAB1_RLUT(32));
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// RLUT1\n\n", (TAB1_RLUT + 33 * 4), GET_TAB1_RLUT(33));
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// RLUT2\n", TAB2_RLUT, GET_TAB2_RLUT(0));
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// RLUT2\n", (TAB2_RLUT + 1 * 4), GET_TAB2_RLUT(1));
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// RLUT2\n", (TAB2_RLUT + 32 * 4), GET_TAB2_RLUT(32));
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// RLUT2\n\n", (TAB2_RLUT + 33 * 4), GET_TAB2_RLUT(33));
	    unsigned int strd = (GET_REG1_STRD() & ~0xFFF0000) | ((s->linesize/16)<<16);
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// STRD1\n", REG1_STRD, strd);
	    strd = (GET_REG2_STRD() & ~0xFFF0000) | ((s->linesize/16)<<16);
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// STRD2\n", REG2_STRD, strd);
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// GEOM\n", REG1_GEOM, GET_REG1_GEOM());
	    fclose(fp);
	}
#endif

#ifdef JZC_VLC_HW_OPT
    int quant=r->si.quant;
    int mod=r->si.vlc_set;
    if(quant!=last_quant||mod!=last_vlc_set)
	{ // set table data
	    int i;
	    uint8_t tbl_sel;
	    uint32_t * vlc_table_base;
	    uint32_t * hw_table_base;
	    last_quant=quant;
	    last_vlc_set=mod;
	    if(mod == 2 && quant < 19) quant += 10;
	    else if(mod && quant < 26) quant += 5;
	      // set 1st table
	    tbl_sel = rv34_quant_to_vlc_set[0][av_clip(quant, 0, 30)];
	    if(tbl_sel!=last_intra_tbl_sel){
		last_intra_tbl_sel=tbl_sel;
		vlc_intra_tbls=&intra_vlcs[tbl_sel];
		hw_table_base = (vlc_base + VLC_TBL_OFST);
		vlc_table_base  = &hw_coef_table[tbl_sel][0];
		for (i=0; i<(cfg_coef_table[tbl_sel].ram_size + 1)/2; i++)
		    hw_table_base[i] = vlc_table_base[i];

		hw_table_base = (vlc_base + VLC_TBL_OFST+0x400);
		vlc_table_base  = &second_pattern_intra_table[tbl_sel<<1][0];
		for (i=0; i<(cfg_sp_intra_table[tbl_sel<<1].ram_size + 1)/2; i++)
		    hw_table_base[i] = vlc_table_base[i];
		sec_pat_ctrl_intra[0]=CALC_SEC_PAT_CTRL(cfg_sp_intra_table[tbl_sel<<1].lvl0_len,0x200);

		hw_table_base = (vlc_base + VLC_TBL_OFST+0x600);
		vlc_table_base  = &second_pattern_intra_table[(tbl_sel<<1)+1][0];
		for (i=0; i<(cfg_sp_intra_table[(tbl_sel<<1)+1].ram_size + 1)/2; i++)
		    hw_table_base[i] = vlc_table_base[i];
		sec_pat_ctrl_intra[1]=CALC_SEC_PAT_CTRL(cfg_sp_intra_table[(tbl_sel<<1)+1].lvl0_len,0x300);
	    }
	      // set 2nd table
	    tbl_sel = rv34_quant_to_vlc_set[1][av_clip(quant, 0, 30)];
	    if(tbl_sel!=last_inter_tbl_sel){
		last_inter_tbl_sel=tbl_sel;
		vlc_inter_tbls=&inter_vlcs[tbl_sel];
		hw_table_base = (vlc_base + VLC_TBL_OFST + 0x200);
		vlc_table_base  = &hw_coef_table[tbl_sel+5][0];
		for (i=0; i<(cfg_coef_table[tbl_sel+5].ram_size + 1)/2; i++)
		    hw_table_base[i] = vlc_table_base[i];

		hw_table_base = (vlc_base + VLC_TBL_OFST+0x800);
		vlc_table_base  = &second_pattern_inter_table[tbl_sel<<1][0];
		for (i=0; i<(cfg_sp_inter_table[tbl_sel<<1].ram_size + 1)/2; i++)
		    hw_table_base[i] = vlc_table_base[i];
		sec_pat_ctrl_inter[0]=CALC_SEC_PAT_CTRL(cfg_sp_inter_table[tbl_sel<<1].lvl0_len,0x400);

		hw_table_base = (vlc_base + VLC_TBL_OFST+0xC00);
		vlc_table_base  = &second_pattern_inter_table[(tbl_sel<<1)+1][0];
		for (i=0; i<(cfg_sp_inter_table[(tbl_sel<<1)+1].ram_size + 1)/2; i++)
		    hw_table_base[i] = vlc_table_base[i];
		sec_pat_ctrl_inter[1]=CALC_SEC_PAT_CTRL(cfg_sp_inter_table[(tbl_sel<<1)+1].lvl0_len,0x600);
	    }
	}
    sync_hw_bits(gb);
#endif

#ifdef JZC_VLC_HW_OPT
    jz_dcache_wb();
#endif
      /* ************ decode MB info  ************** */
    EL("decode MB info");
#ifdef TEST_TIME
    time = GetTimer() - time;
    slice_init_time += time;
    time = GetTimer();
#endif
    while(!check_slice_end(r, s)) {

	dMB = task_fifo_wp;
	dMB->mb_x = s->mb_x;
	dMB->mb_y = s->mb_y;
	dMB->new_slice = 0;

	dMB->er_slice = rv40_decode_macroblock(r, r->intra_types + s->mb_x * 4 + 4);
	if(dMB->er_slice < 0){
	    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_ERROR|DC_ERROR|MV_ERROR);
	    return -1;
	}

#ifdef REMOVE_IDCT
	  //if ( mpframe == DEBUG_FRAME_NUM && s->mb_y == DEBUG_MB_Y && s->mb_x == DEBUG_MB_X )
	    {
		  /* VMAU work with no IDCT */
		dMB->cbp = 0; // clear residuals indicator
		dMB->mau_mlen = 0; // set residuals data length 0
	    }
#endif

#ifdef WRITE_MBINFO
	if ( mpframe == DEBUG_FRAME_NUM && s->mb_y == DEBUG_MB_Y && s->mb_x == DEBUG_MB_X )
	    print_mbinfo(s, task_fifo_wp, "w+");
#endif

	if (++s->mb_x == s->mb_width) {
	    s->mb_x = 0;
	    s->mb_y++;
	    memcpy(r->intra_types_hist, r->intra_types+r->intra_types_stride * 3, r->intra_types_stride * sizeof(*r->intra_types_hist));
	      //memset(r->intra_types, -1, r->intra_types_stride * 4 * sizeof(*r->intra_types_hist));
	}

#ifdef JZC_DCORE_OPT
	  //jz_dcache_wb();
	{
	    int i, va;
	      // dMB->block16 - dMB =  3 * 32
	    int length = 3 + dMB->mau_mlen;
	    va = (int)task_fifo_wp;
	    for(i = 0; i < length; i++){
		i_cache(0x19, va, 0);
		va += 32;
	    }
	    i_sync();
	}
	task_fifo_wp += CIRCLE_BUF_LEN >> 2;
	*tcsm1_fifo_wp=++tcsm1_fifo_value;
#endif
	if(s->mb_x == s->resync_mb_x)
	    s->first_slice_line=0;
	s->mb_num_left--;
    }
#ifdef JZC_VLC_HW_OPT
    sync_sw_bits(gb);
#endif

#ifdef TEST_TIME
	time = GetTimer() - time;
	dec_time += time;
	time = GetTimer();
#endif

#ifdef JZC_DCORE_OPT
    {
	int ai;
	int add_num = 1;
	for (ai=0; ai<add_num + 2; ai++) {
	    RV9_MB_DecARGs * dMB = task_fifo_wp;
	    if (ai == add_num)
		dMB->new_slice = -1;
	    else
		dMB->new_slice = 2; // 0 : not last mb, 1 : slice last mb, 2 : frame last mb, -1 : pseduo block

	    dMB->er_slice = 0;
	    dMB->mb_y = s->mb_y;
	    dMB->mb_x = s->mb_x;

	    dMB->mbtype = -1;

	    dMB->mau_mcbp = 0xb<<2;
	    dMB->mau_mlen = 0;
	    dMB->qscale = s->qscale;

	    dMB->mbtype_above = 0;
	    dMB->cbp = 0;
	    dMB->cur_cbp = 0;
	    dMB->up_cbp = 0;
	    dMB->cur_coef = 0;
	    dMB->up_coef = 0;
	    dMB->is16 = 0;

	      //jz_dcache_wb();
	    {
		int i;
		int va = (int)task_fifo_wp;
		for(i = 0; i < 3; i++){
		    i_cache(0x19, va, 0);
		    va += 32;
		}
		i_sync();
	    }
	    task_fifo_wp += CIRCLE_BUF_LEN >> 2;
	      //*(uint16_t *)task_fifo_wp_d2 = (sizeof(RV9_MB_DecARGs)) & 0xFFFC;

	    (*tcsm1_fifo_wp)++;
	}

	(*tcsm1_fifo_wp) += 5;

	EL("waiting for P1 task done!\n");
	int tmp;

#ifdef DEBUG_AUX_WAITING
	volatile unsigned int * debug = TCSM1_VCADDR(DEBUG);
#endif
	do {
	    tmp = *((volatile int *)TCSM1_VCADDR(TCSM1_P1_TASK_DONE));
#ifdef DEBUG_AUX_WAITING
	    ALOGE("debug : %d %d %d %d %d %d %d %d %d\n",
		 debug[0], debug[1], debug[2], debug[3], debug[4], debug[5], debug[6], debug[7], debug[8]);

#ifdef DEBUG_MC
	    EL("[ MC STAT ] CH1 : 0x%08x, CH2 : 0x%08x\n",
	       *(volatile unsigned int *)(mc_base + 0x4), *(volatile unsigned int *)(mc_base + 0x804));
	    EL("[ MC FSTA ] 0x%08x\n", *(volatile unsigned int *)(vmau_base + 0x64));
	    EL("[ VMAU STA ] 0x%08x\n", *(volatile unsigned int *)(vmau_base + 0x48));
#endif
	    usleep(100000);
#endif
	} while (tmp == 0);

#ifdef TEST_TIME
	time = GetTimer() - time;
	wait_time += time;
#endif
	AUX_RESET();
    }
#endif // JZC_DCORE_OPT

    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

    EL("decode slice over!\n");
    return s->mb_y == s->mb_height;
}

unsigned int abc_table[32]=
    {
	0x800000,//0
	0x800000,//1
	0x800000,//2
	0x800000,//3
	0x800000,//4
	0x800000,//5
	0x800000,//6
	0x800000,//7
	0x3800000,//8
	0x3800100,//9
	0x37a0100,//10
	0x4600101,//11
	0x44b0101,//12
	0x43b0101,//13
	0x62f0101,//14
	0x6250101,//15
	0x61d0101,//16
	0x7170201,//17
	0x8120201,//18
	0x80f0201,//19
	0x90d0201,//20
	0x90b0302,//21
	0xa0a0302,//22
	0xa090302,//23
	0xb080402,//24
	0xb070403,//25
	0xc060503,//26
	0xd050503,//27
	0xe040503,//28
	0xf030704,//29
	0x10020805,//30
	0x11010905,//31
    };


/** @} */ // recons group end

/**
 * Initialize decoder.
 */
static av_cold int rv40_decode_init(AVCodecContext *avctx)
{
 avctx->is_dechw = 1;
#ifdef JZC_CCBP_TBL_OPT
    {int i;
	for(i=0; i<81; i++) {
	    int cc[4];
	    int bn;
	    dsc_lookup[i].dsc_to_cmask1=0;
	    dsc_lookup[i].dsc_to_clen1=0;
	    cc[0]=i/27;
	    cc[1]=(i%27)/9;
	    cc[2]=(i%9)/3;
	    cc[3]=i%3;
	    dsc_lookup[i].dsc_to_ccbp1=
		((cc[0]==2)?0x11:0)
		|((cc[1]==2)?0x22:0)
		|((cc[2]==2)?0x44:0)
		|((cc[3]==2)?0x88:0);
	    for(bn=0;bn<4;bn++){
		if(cc[bn]==1){
		    dsc_lookup[i].dsc_to_cmask1 |= 0x8>>bn;
		    dsc_lookup[i].dsc_to_clen1++;
		}
	    }
	      /*      fprintf(ccbp_tab,"i=%d\ndsc_to_ccbp1=0x%x, dsc_to_cmask1=0x%x, dsc_to_clen1=0x%x\n",
		      i,
		      dsc_lookup[i].dsc_to_ccbp1,
		      dsc_lookup[i].dsc_to_cmask1,
		      dsc_lookup[i].dsc_to_clen1);
	      */
	}

	printf("dsc_lookup[0].dsc_to_ccbp1=0x%x, dsc_to_cmask1=0x%x, dsc_to_clen1=0x%x\n",
	       dsc_lookup[i].dsc_to_ccbp1, dsc_lookup[i].dsc_to_cmask1,dsc_lookup[i].dsc_to_clen1);

	for(i=0; i<256; i++){
	    U8 bof;//bit offset
	    U8 low4=i&0xf;
	    U8 high4=(i>>4)&0xf;
	    U8 new4=0;
	    U8 sof=0;
	    getb_lookup[i]=0;
	    for(bof=0;bof<4;bof++){
		if(low4&(1<<bof)){
		    U8 raw=((high4 & (1<<sof))<<(bof-sof)) | ((~high4 & (1<<sof))<<(4+bof-sof));
		    getb_lookup[i] |= (bof==0?raw<<3:(bof==1?raw<<1:(bof==2?raw>>1:(bof==3?raw>>3:0))));
		    sof++;
		}
	    }
	      //      fprintf(ccbp_tab,"getb_lookup[i]=0x%x\n",getb_lookup[i]);
	}
    }

#endif//#ifdef JZC_CCBP_TBL_OPT

    RV34DecContext *r = avctx->priv_data;

    r->rv30 = 0;
    MpegEncContext *s = &r->s;

#ifdef JZC_MXU_OPT
    S32I2M(xr16, 0x7);
#endif

#ifdef JZC_DCORE_OPT
    {
#if 1   // Android
#define TCSM_BINLEN 0x4000
	r->loadbin = av_malloc(TCSM_BINLEN);
	if (r->loadbin == NULL) {
	    ALOGE("malloc loadbin error");
	    return -1;
	} else {
	    memset(r->loadbin, 0, TCSM_BINLEN);
	}

	if( (r->binlen = loadfile("rv9_p1.bin", r->loadbin, TCSM_BINLEN, 1)) == -1 ){
	    ALOGE("LOAD RV9_P1_BIN ERROR.....................");
	    return -1;
	}
#undef TCSM_BINLEN
#else   // Linux
	int * tmp_hm_buf = jz4740_alloc_frame(avctx->VpuMem_ptr, 32, SPACE_HALF_MILLION_BYTE);
	if (tmp_hm_buf < 0)
	    printf("JZ4740 ALLOC tmp_hm_buf ERROR !! \n");

	int i;
	  // load p1 insn and data to reserved mem
	FILE *fp_text;
	int len, *reserved_mem;
	int *load_buf;	
	fp_text = fopen("./rv9_p1.bin", "r+b");
	if (!fp_text){
	    printf(" error while open rv9_p1.bin \n");
	    exit_player_with_rc();
	}
	load_buf = tmp_hm_buf;
	len = fread(load_buf, 4, SPACE_HALF_MILLION_BYTE, fp_text);
	reserved_mem = (int *)TCSM1_VCADDR(P1_MAIN_ADDR);      
	  //printf("----------------------rv9_p1.bin len = %d\n", len);
	for(i=0; i<len; i++)
	    reserved_mem[i] = load_buf[i];
	fclose(fp_text);
#endif
    }
#endif

#ifdef JZC_TCSM_OPT
    r->task_fifo = NULL;
#endif

#ifdef JZC_PMON_P0
    i_mtc0_2(0, 16, 7);
#endif

    MPV_decode_defaults(s);
    s->avctx= avctx;
    s->out_format = FMT_H263;
    s->codec_id= avctx->codec_id;

    s->width = avctx->width;
    s->height = avctx->height;

    r->s.avctx = avctx;
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
    r->s.flags |= CODEC_FLAG_EMU_EDGE;
    avctx->pix_fmt = PIX_FMT_YUV420P;
    avctx->has_b_frames = 1;
    s->low_delay = 0;

    if (MPV_common_init(s) < 0)
	return -1;

    ff_h264_pred_init(&r->h, CODEC_ID_RV40);

    r->intra_types_stride = 4*s->mb_stride + 4;
    r->intra_types_hist = av_malloc(r->intra_types_stride * 5 * sizeof(*r->intra_types_hist));
    r->mb_type = av_mallocz(r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));
    r->intra_types = r->intra_types_hist + r->intra_types_stride;
  
    r->cbp_luma   = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_luma));
    r->cbp_chroma = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_chroma));
    r->deblock_coefs = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->deblock_coefs));

#ifdef JZC_DBLK_OPT
    r->mbtype = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->mbtype));
#endif

    if(!intra_vlcs[0].cbppattern[0].bits)
	rv34_init_tables();

    if(!aic_top_vlc.bits)
	rv40_init_tables();
#ifdef JZC_VLC_HW_OPT
    SET_GLOBAL_CTRL(0x101);
      //SET_GLOBAL_CTRL(0x300);   /* --- get be ready for test 4780 --- */
    last_quant=last_vlc_set=0xffabcdef;
    last_intra_tbl_sel=last_inter_tbl_sel=0xfe;
#endif
      //r->parse_slice_header = rv40_parse_slice_header;
      //r->decode_intra_types = rv40_decode_intra_types;
      //r->decode_mb_info     = rv40_decode_mb_info;
      //r->loop_filter        = rv40_loop_filter;
    r->luma_dc_quant_i = rv40_luma_dc_quant[0];
    r->luma_dc_quant_p = rv40_luma_dc_quant[1];
    avctx->use_jz_buf=1;

#ifdef CRC_CHECK
    mpframe = 0;
    frame_crc = 0;
#endif

#ifdef TEST_TIME
    slice_init_time = dec_time = wait_time = 0;
    frame_time = 0;
    frame_num = 0;
#endif
    return 0;
}

static int get_slice_offset(AVCodecContext *avctx, const uint8_t *buf, int n)
{
    if(avctx->slice_count) return avctx->slice_offset[n];
    else                   return AV_RL32(buf + n*8 - 4) == 1 ? AV_RL32(buf + n*8) :  AV_RB32(buf + n*8);
}
#undef printf


#if 1
void prt_square_rv(void * start_ptr,int size,int h,int w, int stride){
    unsigned int* start_int=(int*)start_ptr;
    unsigned short* start_short=(short*)start_ptr;
    unsigned char* start_byte=(char*)start_ptr;
    int i, j;
    if(size==4){
	for(i=0;i<h;i++){
	    for(j=0;j<w;j++){
		printf("0x%8x,",start_int[i*stride+j]);
	    }
	    printf("\n");
	}
    }
    if(size==2){
	for(i=0;i<h;i++){
	    for(j=0;j<w;j++){
		printf("0x%4x,",start_short[i*stride+j]);
	    }
	    printf("\n");
	}
    }
    if(size==1){
	for(i=0;i<h;i++){
	    for(j=0;j<w;j++){
		printf("0x%2x,",start_byte[i*stride+j]);
	    }
	    printf("\n");
	}
    }
}
#endif

#ifdef DEBUG_MC
static void mc_dump_frame_data(MpegEncContext * s)
{
    int i, j;
    int mb_w, mb_h;
    unsigned char * src_y, * src_c;
    char filename[100];

#ifdef IPU_BUG
    int y_strd=(s->mb_width*256 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
    int c_strd=(s->mb_width*128 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
#else
    src_y = s->last_picture.data[0];
    src_c = s->last_picture.data[1];
#endif

    sprintf(filename, "/data/ref_frame%d.c", mpframe);
    FILE * fp = fopen(filename, "w+");
    fprintf(fp, "char fref_frame_y [] = {\n");
    for(mb_h = 0; mb_h < s->mb_height; mb_h++){
#ifdef IPU_BUG
	src_y = s->last_picture.data[0] + mb_h * y_strd;
#endif
	for(mb_w = 0; mb_w < s->mb_width; mb_w++) {
	    fprintf(fp, "\t//mb : (%d, %d)\n", mb_h, mb_w);
	    for(i = 0; i < 16; i++){
		fprintf(fp, "\t0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", src_y[0], src_y[1], src_y[2], src_y[3], src_y[4], src_y[5], src_y[6], src_y[7], src_y[8], src_y[9], src_y[10], src_y[11], src_y[12], src_y[13], src_y[14], src_y[15]);
		src_y += 16;
	    }
	}
    }
    fprintf(fp, "}\n\n");
    fprintf(fp, "char fref_frame_c [] = {\n");
    for(mb_h = 0; mb_h < s->mb_height; mb_h++){
#ifdef IPU_BUG
	src_c = s->last_picture.data[1] + mb_h * c_strd;
#endif
	for(mb_w = 0; mb_w < s->mb_width; mb_w++) {
	    fprintf(fp, "\t//mb : (%d, %d)\n", mb_h, mb_w);
	    for(j = 0; j < 8; j++) {
		fprintf(fp, "\t0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", src_c[0], src_c[1], src_c[2], src_c[3], src_c[4], src_c[5], src_c[6], src_c[7], src_c[8], src_c[9], src_c[10], src_c[11], src_c[12], src_c[13], src_c[14], src_c[15]);
		src_c += 16;
	    }
	}
    }
    fprintf(fp, "}\n\n");

    fprintf(fp, "char bref_frame_y [] = {\n");
    for(mb_h = 0; mb_h < s->mb_height; mb_h++){
#ifdef IPU_BUG
	src_y = s->next_picture.data[0] + mb_h * y_strd;
#endif
	for(mb_w = 0; mb_w < s->mb_width; mb_w++) {
	    fprintf(fp, "\t//mb : (%d, %d)\n", mb_h, mb_w);
	    for(i = 0; i < 16; i++){
		fprintf(fp, "\t0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", src_y[0], src_y[1], src_y[2], src_y[3], src_y[4], src_y[5], src_y[6], src_y[7], src_y[8], src_y[9], src_y[10], src_y[11], src_y[12], src_y[13], src_y[14], src_y[15]);
		src_y += 16;
	    }
	}
    }
    fprintf(fp, "}\n\n");
    fprintf(fp, "char bref_frame_c [] = {\n");
    for(mb_h = 0; mb_h < s->mb_height; mb_h++){
#ifdef IPU_BUG
	src_c = s->next_picture.data[1] + mb_h * c_strd;
#endif
	for(mb_w = 0; mb_w < s->mb_width; mb_w++) {
	    fprintf(fp, "\t//mb : (%d, %d)\n", mb_h, mb_w);
	    for(j = 0; j < 8; j++) {
		fprintf(fp, "\t0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", src_c[0], src_c[1], src_c[2], src_c[3], src_c[4], src_c[5], src_c[6], src_c[7], src_c[8], src_c[9], src_c[10], src_c[11], src_c[12], src_c[13], src_c[14], src_c[15]);
		src_c += 16;
	    }
	}
    }
    fprintf(fp, "}\n\n");

    fclose(fp);

    ALOGE("MC dump frame data done ...\n");

}
#endif

#ifdef CRC_CHECK
static void dump_frame_data(MpegEncContext *s)
{
    int i, j;
    int mb_w, mb_h;
    unsigned char * src_y, * src_c;
    char filename[100];

#ifdef IPU_BUG
    int y_strd=(s->mb_width*256 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
    int c_strd=(s->mb_width*128 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
#else
    src_y = s->current_picture.data[0];
    src_c = s->current_picture.data[1];
#endif

    sprintf(filename, "/data/real_frame%d.c", mpframe);
    FILE * fp = fopen(filename, "w+");
    for(mb_h = 0; mb_h < s->mb_height; mb_h++){
#ifdef IPU_BUG
	src_y = s->current_picture.data[0] + mb_h * y_strd;
	src_c = s->current_picture.data[1] + mb_h * c_strd;
#endif
	for(mb_w = 0; mb_w < s->mb_width; mb_w++) {
	    fprintf(fp, "mb : (%d, %d)\n************************ Y ***********************\n", mb_h, mb_w);
	    for(i = 0; i < 16; i++){
		fprintf(fp, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", src_y[0], src_y[1], src_y[2], src_y[3], src_y[4], src_y[5], src_y[6], src_y[7], src_y[8], src_y[9], src_y[10], src_y[11], src_y[12], src_y[13], src_y[14], src_y[15]);
		src_y += 16;
	    }
	    fprintf(fp, "************************ C ***********************\n");
	    for(j = 0; j < 8; j++) {
		fprintf(fp, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ** 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", src_c[0], src_c[1], src_c[2], src_c[3], src_c[4], src_c[5], src_c[6], src_c[7], src_c[8], src_c[9], src_c[10], src_c[11], src_c[12], src_c[13], src_c[14], src_c[15]);
		src_c += 16;
	    }
	}
    }
    fclose(fp);

    ALOGE("dump frame data done ...\n");

    return 0;
}
#endif

int ff_rv40_decode_frame(AVCodecContext *avctx,
			 void *data, int *data_size,
			 AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    RV34DecContext *r = avctx->priv_data;
    MpegEncContext *s = &r->s;
    AVFrame *pict = data;
    SliceInfo si;
    int i;
    int slice_count;
    const uint8_t *slices_hdr = NULL;
    int last = 0;

    EL("decode a frame, buf_size = 0x%08x\n", buf_size);

#ifdef TEST_TIME
    int time = GetTimer();
    /* slice_init_time = dec_time = wait_time = 0; */
    /* frame_time = 0; */
    /* frame_num = 0; */
#endif

      /* no supplementary picture */
    if (buf_size == 0) {
	  /* special case for last picture */
	if (s->low_delay==0 && s->next_picture_ptr) {
	    *pict= *(AVFrame*)s->next_picture_ptr;
	    s->next_picture_ptr= NULL;

	    *data_size = sizeof(AVFrame);
	}
	return 0;
    }

    if(!avctx->slice_count){
	slice_count = (*buf++) + 1;
	slices_hdr = buf + 4;
	buf += 8 * slice_count;
    }else
	slice_count = avctx->slice_count;

      //parse first slice header to check whether this frame can be decoded
    if(get_slice_offset(avctx, slices_hdr, 0) > buf_size){
	av_log(avctx, AV_LOG_ERROR, "Slice offset is greater than frame size\n");
	return -1;
    }
    init_get_bits(&s->gb, buf+get_slice_offset(avctx, slices_hdr, 0), buf_size-get_slice_offset(avctx, slices_hdr, 0));
    if(rv40_parse_slice_header(r, &r->s.gb, &si) < 0 || si.start){
	av_log(avctx, AV_LOG_ERROR, "First slice header is incorrect\n");
	return -1;
    }

    if((!s->next_picture_ptr || !s->next_picture_ptr->data[0]) && si.type) // P or B frame
	return -1;
    if((!s->last_picture_ptr || !s->last_picture_ptr->data[0]) && si.type == FF_B_TYPE)
	return -1;

      /* skip b frames if we are in a hurry */
    if(avctx->hurry_up && si.type==FF_B_TYPE) return buf_size;
    if(   (avctx->skip_frame >= AVDISCARD_NONREF && si.type==FF_B_TYPE)
	  || (avctx->skip_frame >= AVDISCARD_NONKEY && si.type!=FF_I_TYPE)
	  ||  avctx->skip_frame >= AVDISCARD_ALL)
	return buf_size;
      /* skip everything if we are in a hurry>=5 */
    if(avctx->hurry_up>=5)
	return buf_size;

#ifdef JZC_VLC_HW_OPT
    jz_dcache_wb();
#endif

      /* *******************  init hardware info  **********************  */
    RST_VPU(); // we must add this, error maybe occurs
#ifdef JZC_DCORE_OPT
    memcpy(TCSM1_VCADDR(P1_MAIN_ADDR), r->loadbin, r->binlen);
    jz_dcache_wb(); /* flush cache into reserved mem */
#endif

#ifdef JZC_MC_OPT
    motion_init_rv9(RV9_QPEL, RV9_EPEL);
    *(volatile int *)TCSM1_VCADDR(TCSM1_MOTION_DSA) = 0x8000FFFF;

#ifdef DEBUG_MC
    {  
	int i, idx, value;
	FILE * fn = fopen("/data/real_config.c", "w+");
	fclose(fn);
	FILE * fp = fopen("/data/real_config.c", "a+");
	  //fprintf(fp, "// motion_init_rv9\n");
	fprintf(fp, "volatile unsigned int * mc_base = 0x%x;\n", 0x13250000);
	fprintf(fp, "// ILUT1\n");
	for(i = 0; i < 32; i++) {
	    value = MOTION_V_BASE + TAB1_ILUT + i * 4;
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);\n", (TAB1_ILUT + i * 4), read_reg(value, 0));
	}
	fprintf(fp, "// ILUT2\n");
	for(i = 0; i < 32; i++) {
	    value = MOTION_V_BASE + TAB2_ILUT + i * 4;
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);\n", (TAB2_ILUT + i * 4), read_reg(value, 0));
	}
	fprintf(fp, "// CLUT\n");
	for(i = 0; i < 64; i++) {
	    value = MOTION_V_BASE + TAB1_CLUT + i * 4;
	    fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);\n", (TAB1_CLUT + i * 4), read_reg(value, 0));
	}

	fprintf(fp, "\n");
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// STAT1\n", REG1_STAT, GET_REG1_STAT());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// STAT2\n", REG2_STAT, GET_REG2_STAT());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// CTRL\n", REG1_CTRL, GET_REG1_CTRL() | (1<<3));
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// BINFO1\n", REG1_BINFO, GET_REG1_BINFO());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// BINFO2\n", REG2_BINFO, GET_REG2_BINFO());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// PINFO1\n", REG1_PINFO, GET_REG1_PINFO());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// PINFO2\n", REG2_PINFO, GET_REG2_PINFO());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// WINFO1\n", REG1_WINFO, GET_REG1_WINFO());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// WTRND1\n", REG1_WTRND, GET_REG1_WTRND());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// REG2_WINFO1\n", REG2_WINFO1, GET_REG2_WINFO1());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// REG2_WINFO2\n", REG2_WINFO2, GET_REG2_WINFO2());
	fprintf(fp, "write_reg((mc_base + 0x%x), 0x%x);// WTRND2\n", REG2_WTRND, GET_REG2_WTRND());
	fclose(fp);
    }
#endif

#endif

      /* ********************************* decode slice  *********************************** */
    for(i=0; i<slice_count; i++){
	int offset= get_slice_offset(avctx, slices_hdr, i);
	int size;
	if(i+1 == slice_count)
	    size= buf_size - offset;
	else
	    size= get_slice_offset(avctx, slices_hdr, i+1) - offset;

	if(offset > buf_size){
	    av_log(avctx, AV_LOG_ERROR, "Slice offset is greater than frame size\n");
	    break;
	}

	r->si.end = s->mb_width * s->mb_height;
	if(i+1 < slice_count){
	    init_get_bits(&s->gb, buf+get_slice_offset(avctx, slices_hdr, i+1), (buf_size-get_slice_offset(avctx, slices_hdr, i+1))*8);
	    if(rv40_parse_slice_header(r, &r->s.gb, &si) < 0){
		if(i+2 < slice_count)
		    size = get_slice_offset(avctx, slices_hdr, i+2) - offset;
		else
		    size = buf_size - offset;
	    }else
		r->si.end = si.start;
	}

	last = rv40_decode_slice(r, r->si.end, buf + offset, size);
	s->mb_num_left = r->s.mb_x + r->s.mb_y*r->s.mb_width - r->si.start;
	if(last < 0)
	    {
		((volatile int *)TCSM1_VCADDR(TCSM1_P1_FIFO_RP))[0]=0;
		*((volatile int *)TCSM1_VCADDR(TCSM1_P1_TASK_DONE)) = 0x1;	
		AUX_RESET();       
		return -1;
	    }
	else if(last)
	    break;
    }

    if(last){
	  //if(r->loop_filter)
	  //    r->loop_filter(r, s->mb_height - 1);
    
	ff_er_frame_end(s);
	MPV_frame_end(s);

	if (s->pict_type == FF_B_TYPE || s->low_delay) {
	    *pict= *(AVFrame*)s->current_picture_ptr;
	} else if (s->last_picture_ptr != NULL) {
	    *pict= *(AVFrame*)s->last_picture_ptr;
	}

	if(s->last_picture_ptr || s->low_delay){
	    *data_size = sizeof(AVFrame);
	    ff_print_debug_info(s, pict);
	}
	s->current_picture_ptr= NULL; //so we can detect if frame_end wasnt called (find some nicer solution...)
    }

#ifdef DEBUG_MC
    if( mpframe == DEBUG_FRAME_NUM ) //get MC chain
	{
	    volatile unsigned int * motion_dha = TCSM1_VCADDR(VDMA_CHECK);
	    FILE * fp = fopen("/data/real_config.c", "a+");

	    fprintf(fp, "//motion_dha\n");
	    fprintf(fp, "volatile unsigned int * motion_dha = 0x%x;\n", TCSM1_PADDR(VDMA_CHECK));
	    int count = 0;
	    while( *motion_dha != 0xa000FFFF ) {
		fprintf(fp, "write_reg((motion_dha + 0x%x), 0x%08x);\n", count, *motion_dha);
		motion_dha++;
		count += 4;
	    }
	    fprintf(fp, "write_reg((motion_dha + 0x%x), 0x%08x);\n", count, *motion_dha);
	    fclose(fp);
	}

    if( mpframe == DEBUG_FRAME_NUM ) {
    	mc_dump_frame_data(s);
	dump_frame_data(s);
    }
#endif

#ifdef DEBUG_DBLK
    if( mpframe == DEBUG_FRAME_NUM ) {
	volatile unsigned int * dblk_chain = TCSM1_VCADDR(VDMA_CHECK);
	ALOGE("[ DBLK ] id_dha : 0x%08x\n", *dblk_chain++);
	ALOGE("[ DBLK ] coef_nei_info : 0x%08x\n", *dblk_chain++);
	ALOGE("[ DBLK ] mb_info : 0x%08x\n", *dblk_chain++);
    }
#endif

#ifdef CRC_CHECK
      //if ( s->pict_type != FF_B_TYPE )
    {
	int crc_i;
	int linelen = (s->mb_width*256 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
	for (crc_i = 0; crc_i < s->mb_height; crc_i++ ) {
	    frame_crc = crc(s->current_picture.data[0] + crc_i * linelen, s->mb_width * 256, frame_crc);
	}

	linelen = (s->mb_width*128 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
	for (crc_i = 0; crc_i < s->mb_height; crc_i++ ) {
	    frame_crc = crc(s->current_picture.data[1] + crc_i * linelen, s->mb_width * 128, frame_crc);
	}

	ALOGE("[%04d %s frame]  crc : 0x%08x\n", mpframe,
	     s->pict_type == FF_I_TYPE ? "I" : s->pict_type == FF_P_TYPE ? "P" : "B", frame_crc);  

	  //dump_frame_data(s);
    }

    mpframe++;
#endif

#ifdef TEST_TIME
    frame_time += GetTimer() - time;
    frame_num++;
    ALOGE("%d frame is %s frame",
	 frame_num, s->pict_type == FF_I_TYPE ? "I" : (s->pict_type == FF_P_TYPE ? "P" : "B"));
    ALOGE("[ avg time ] slice_init = %d, dec = %d, wait = %d, frame = %d",
    	 slice_init_time / frame_num, dec_time / frame_num, wait_time / frame_num, frame_time / frame_num);
#endif

    return buf_size;
}

av_cold int ff_rv40_decode_end(AVCodecContext *avctx)
{
    RV34DecContext *r = avctx->priv_data;

    MPV_common_end(&r->s);

    av_freep(&r->intra_types_hist);
    av_freep(&r->mb_type);
    r->intra_types = NULL;
    av_freep(&r->cbp_luma);
    av_freep(&r->cbp_chroma);
    av_freep(&r->deblock_coefs);

#ifdef JZC_DCORE_OPT
    av_freep(&r->loadbin);
#endif

#ifdef JZC_DBLK_OPT
    av_freep(&r->mbtype);
#endif
    return 0;
}

AVCodec rv40_decoder = {
    "rv40",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_RV40,
    sizeof(RV34DecContext),
    rv40_decode_init,
    NULL,
    ff_rv40_decode_end,
    ff_rv40_decode_frame,
    CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush = ff_mpeg_flush,
    .long_name = NULL_IF_CONFIG_SMALL("RealVideo 4.0"),
    .pix_fmts= ff_pixfmt_list_420,
};
