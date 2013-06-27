/*****************************************************************************
 * me.c: h264 encoder library (Motion Estimation)
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#include "common/common.h"
#include "macroblock.h"
#include "me.h"

#define KENTRO_FSSE       1
#define KENTRO_FSST       10
#define KENTRO_FSCE       0
#define KENTRO_FSCT       0x4000
#define KENTRO_EDGE       12
int hazard_edge;
int hit_fsst;

/* presets selected from good points on the speed-vs-quality curve of several test videos
 * subpel_iters[i_subpel_refine] = { refine_hpel, refine_qpel, me_hpel, me_qpel }
 * where me_* are the number of EPZS iterations run on all candidate block types,
 * and refine_* are run only on the winner.
 * the subme=8,9 values are much higher because any amount of satd search makes
 * up its time by reducing the number of qpel-rd iterations. */
static const int subpel_iterations[][4] =
   {{0,0,0,0},
    {1,1,0,0},
    {0,1,1,0},
    {0,2,1,0},
    {0,2,1,1},
    {0,2,1,2},
    {0,0,2,2},
    {0,0,2,2},
    {0,0,4,10},
    {0,0,4,10},
    {0,0,4,10}};

/* (x-1)%6 */
static const int mod6m1[8] = {5,0,1,2,3,4,5,0};
/* radius 2 hexagon. repeated entries are to avoid having to compute mod6 every time. */
static const int hex2[8][2] = {{-1,-2}, {-2,0}, {-1,2}, {1,2}, {2,0}, {1,-2}, {-1,-2}, {-2,0}};
static const int square1[9][2] = {{0,0}, {0,-1}, {0,1}, {-1,0}, {1,0}, {-1,-1}, {-1,1}, {1,-1}, {1,1}};

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters, int *p_halfpel_thresh, int b_refine_qpel );

#define BITS_MVD( mx, my )\
    (p_cost_mvx[(mx)<<2] + p_cost_mvy[(my)<<2])

#define COST_MV( mx, my )\
{\
    int cost = h->pixf.fpelcmp[i_pixel]( p_fenc, FENC_STRIDE,\
                   &p_fref_w[(my)*stride+(mx)], stride )\
             + BITS_MVD(mx,my);\
    COPY3_IF_LT( bcost, cost, bmx, mx, bmy, my );\
}

#define COST_MV_HPEL( mx, my ) \
{ \
    int stride2 = 16; \
    uint8_t *src = h->mc.get_ref( pix, &stride2, m->p_fref, stride, mx, my, bw, bh, &m->weight[0] ); \
    int cost = h->pixf.fpelcmp[i_pixel]( p_fenc, FENC_STRIDE, src, stride2 ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    COPY3_IF_LT( bpred_cost, cost, bpred_mx, mx, bpred_my, my ); \
}

#define COST_MV_X3_DIR( m0x, m0y, m1x, m1y, m2x, m2y, costs )\
{\
    uint8_t *pix_base = p_fref_w + bmx + bmy*stride;\
    h->pixf.fpelcmp_x3[i_pixel]( p_fenc,\
        pix_base + (m0x) + (m0y)*stride,\
        pix_base + (m1x) + (m1y)*stride,\
        pix_base + (m2x) + (m2y)*stride,\
        stride, costs );\
    (costs)[0] += BITS_MVD( bmx+(m0x), bmy+(m0y) );\
    (costs)[1] += BITS_MVD( bmx+(m1x), bmy+(m1y) );\
    (costs)[2] += BITS_MVD( bmx+(m2x), bmy+(m2y) );\
}

#define COST_MV_X4_DIR( m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y, costs )\
{\
    uint8_t *pix_base = p_fref_w + bmx + bmy*stride;\
    h->pixf.fpelcmp_x4[i_pixel]( p_fenc,\
        pix_base + (m0x) + (m0y)*stride,\
        pix_base + (m1x) + (m1y)*stride,\
        pix_base + (m2x) + (m2y)*stride,\
        pix_base + (m3x) + (m3y)*stride,\
        stride, costs );\
    (costs)[0] += BITS_MVD( bmx+(m0x), bmy+(m0y) );\
    (costs)[1] += BITS_MVD( bmx+(m1x), bmy+(m1y) );\
    (costs)[2] += BITS_MVD( bmx+(m2x), bmy+(m2y) );\
    (costs)[3] += BITS_MVD( bmx+(m3x), bmy+(m3y) );\
}

#define COST_MV_X4( m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y )\
{\
    uint8_t *pix_base = p_fref_w + omx + omy*stride;\
    h->pixf.fpelcmp_x4[i_pixel]( p_fenc,\
        pix_base + (m0x) + (m0y)*stride,\
        pix_base + (m1x) + (m1y)*stride,\
        pix_base + (m2x) + (m2y)*stride,\
        pix_base + (m3x) + (m3y)*stride,\
        stride, costs );\
    costs[0] += BITS_MVD( omx+(m0x), omy+(m0y) );\
    costs[1] += BITS_MVD( omx+(m1x), omy+(m1y) );\
    costs[2] += BITS_MVD( omx+(m2x), omy+(m2y) );\
    costs[3] += BITS_MVD( omx+(m3x), omy+(m3y) );\
    COPY3_IF_LT( bcost, costs[0], bmx, omx+(m0x), bmy, omy+(m0y) );\
    COPY3_IF_LT( bcost, costs[1], bmx, omx+(m1x), bmy, omy+(m1y) );\
    COPY3_IF_LT( bcost, costs[2], bmx, omx+(m2x), bmy, omy+(m2y) );\
    COPY3_IF_LT( bcost, costs[3], bmx, omx+(m3x), bmy, omy+(m3y) );\
}

#define COST_MV_X3_ABS( m0x, m0y, m1x, m1y, m2x, m2y )\
{\
    h->pixf.fpelcmp_x3[i_pixel]( p_fenc,\
        p_fref_w + (m0x) + (m0y)*stride,\
        p_fref_w + (m1x) + (m1y)*stride,\
        p_fref_w + (m2x) + (m2y)*stride,\
        stride, costs );\
    costs[0] += p_cost_mvx[(m0x)<<2]; /* no cost_mvy */\
    costs[1] += p_cost_mvx[(m1x)<<2];\
    costs[2] += p_cost_mvx[(m2x)<<2];\
    COPY3_IF_LT( bcost, costs[0], bmx, m0x, bmy, m0y );\
    COPY3_IF_LT( bcost, costs[1], bmx, m1x, bmy, m1y );\
    COPY3_IF_LT( bcost, costs[2], bmx, m2x, bmy, m2y );\
}

/*  1  */
/* 101 */
/*  1  */
#define DIA1_ITER( mx, my )\
{\
    omx = mx; omy = my;\
    COST_MV_X4( 0,-1, 0,1, -1,0, 1,0 );\
}

#define CROSS( start, x_max, y_max )\
{\
    i = start;\
    if( x_max <= X264_MIN(mv_x_max-omx, omx-mv_x_min) )\
        for( ; i < x_max-2; i+=4 )\
            COST_MV_X4( i,0, -i,0, i+2,0, -i-2,0 );\
    for( ; i < x_max; i+=2 )\
    {\
        if( omx+i <= mv_x_max )\
            COST_MV( omx+i, omy );\
        if( omx-i >= mv_x_min )\
            COST_MV( omx-i, omy );\
    }\
    i = start;\
    if( y_max <= X264_MIN(mv_y_max-omy, omy-mv_y_min) )\
        for( ; i < y_max-2; i+=4 )\
            COST_MV_X4( 0,i, 0,-i, 0,i+2, 0,-i-2 );\
    for( ; i < y_max; i+=2 )\
    {\
        if( omy+i <= mv_y_max )\
            COST_MV( omx, omy+i );\
        if( omy-i >= mv_y_min )\
            COST_MV( omx, omy-i );\
    }\
}

void x264_me_search_ref( x264_t *h, x264_me_t *m, int16_t (*mvc)[2], int i_mvc, int *p_halfpel_thresh )
{
    const int bw = x264_pixel_size[m->i_pixel].w;
    const int bh = x264_pixel_size[m->i_pixel].h;
    const int i_pixel = m->i_pixel;
    const int stride = m->i_stride[0];
    int i_me_range = h->param.analyse.i_me_range;
    int bmx, bmy, bcost;
    int bpred_mx = 0, bpred_my = 0, bpred_cost = COST_MAX;
    int omx, omy, pmx, pmy;
    uint8_t *p_fenc = m->p_fenc[0];
    uint8_t *p_fref_w = m->p_fref_w;
    ALIGNED_ARRAY_16( uint8_t, pix,[16*16] );

    int i, j;
    int dir;
    int costs[16];

    //JZVPU
    int step_cnt = 1;
    hazard_edge = 0;
    hit_fsst = 0;

    int mv_x_min = X264_MAX(h->mb.mv_min_fpel[0], -28);
    int mv_y_min = X264_MAX(h->mb.mv_min_fpel[1], -28);
    int mv_x_max = X264_MIN(h->mb.mv_max_fpel[0],  28);
    int mv_y_max = X264_MIN(h->mb.mv_max_fpel[1],  28);

    if(h->mb.i_mb_x == 0)
      mv_x_min = -KENTRO_EDGE;
    if(h->mb.i_mb_x == h->sps->i_mb_width-1) 
      mv_x_max = KENTRO_EDGE;
    if(h->mb.i_mb_y == 0)
      mv_y_min = -KENTRO_EDGE;
    if(h->mb.i_mb_y == h->sps->i_mb_height-1) 
      mv_y_max = KENTRO_EDGE;

#if 0
# define MV_HITMAX(mx, my, u, d, l, r)   \
    if( (u) && (my) <  mv_y_min ) break; \
    if( (d) && (my) >= mv_y_max ) break; \
    if( (l) && (mx) <  mv_x_min ) break; \
    if( (r) && (mx) >= mv_x_max ) break; \
    if( (u) && (h->mb.i_mb_y*16 + (my) <= out_ty) ) break; \
    if( (d) && (h->mb.i_mb_y*16 + 15 + (my) >= out_by) ) break; \
    if( (l) && (h->mb.i_mb_x*16 + (mx) <= out_lx) ) break; \
    if( (r) && (h->mb.i_mb_x*16 + 15 + (mx) >= out_rx) ) break; 
#else
#if 0
# define MV_HITMAX(mx, my, u, d, l, r)   \
    if( (u) && (my) <  mv_y_min ) break; \
    if( (d) && (my) >= mv_y_max ) break; \
    if( (l) && (mx) <  mv_x_min ) break; \
    if( (r) && (mx) >= mv_x_max ) break;
#else
# define MV_HITMAX(mx, my, u, d, l, r)   \
    int tmpv; \
    tmpv = my<<2; if( (u) && !!(tmpv & 0x80) && !(tmpv & 0x40) ) {break;} \
    tmpv = my<<2; if( (d) && !(tmpv & 0x80) && !!(tmpv & 0x40) ) {break;} \
    tmpv = mx<<2; if( (l) && !!(tmpv & 0x80) && !(tmpv & 0x40) ) {break;} \
    tmpv = mx<<2; if( (r) && !(tmpv & 0x80) && !!(tmpv & 0x40) ) {break;}
#endif
#endif

#define CHECK_MVRANGE(mx,my) ( mx >= mv_x_min && mx <= mv_x_max && my >= mv_y_min && my <= mv_y_max )

# define CHECK_EDGE(mx, my, u, d, l, r)  \
    if((u) && ((h->mb.i_mb_y*16 + (my)) <= -KENTRO_EDGE)) {puts("Hazard U"); hazard_edge=1; break;} \
    if((d) && ((h->mb.i_mb_y*16+15 + (my)) >= h->sps->i_mb_height*16+KENTRO_EDGE)) {puts("Hazard D"); hazard_edge=1; break;} \
    if((l) && ((h->mb.i_mb_x*16 + (mx)) <= -KENTRO_EDGE)) {puts("Hazard L"); hazard_edge=1; break;} \
    if((r) && ((h->mb.i_mb_x*16+15 + (mx)) >= h->sps->i_mb_width*16+KENTRO_EDGE)) {puts("Hazard R"); hazard_edge=1; break;} \

    const uint16_t *p_cost_mvx = m->p_cost_mv - m->mvp[0];
    const uint16_t *p_cost_mvy = m->p_cost_mv - m->mvp[1];

    bmx = m->mvp[0] + 2;
    bmy = m->mvp[1] + 2;

    bmx = x264_clip3( bmx, mv_x_min*4, mv_x_max*4 );
    bmy = x264_clip3( bmy, mv_y_min*4, mv_y_max*4 );
    pmx = ( bmx + 0 ) >> 2;
    pmy = ( bmy + 0 ) >> 2;
    bcost = COST_MAX;

#if 0
    if( (h->mb.i_mb_x*16 + pmx - 2) <= -15 )
      pmx += -15 - (h->mb.i_mb_x*16 + pmx -2);
    if( (h->mb.i_mb_y*16 + pmy - 2) <= -15 )
      pmy += -15 - (h->mb.i_mb_y*16 + pmy -2);
    if( (h->mb.i_mb_x*16 + pmx + 15 + 2) >= h->sps->i_mb_width*16 + 15)
      pmx -= (h->mb.i_mb_x*16 + pmx + 15 + 2) - (h->sps->i_mb_width*16 + 15);
    if( (h->mb.i_mb_y*16 + pmy + 15 + 2) >= h->sps->i_mb_height*16 + 15)
      pmy -= (h->mb.i_mb_y*16 + pmy + 15 + 2) - (h->sps->i_mb_height*16 + 15);
#endif

    COST_MV( pmx, pmy );

    {
      /* diamond search, radius 1 */
      i = 1;
      bcost <<= 4;
      do
        {
	  step_cnt++;
	  COST_MV_X4_DIR( 0,-1, 0,1, -1,0, 1,0, costs );

	  if( (costs[0]<<4) < (bcost & ~0xF) )
	    bcost = (costs[0]<<4)+1;
	  if( (costs[2]<<4) < (bcost & ~0xF) )
	    bcost = (costs[2]<<4)+4;
	  if( (costs[3]<<4) < (bcost & ~0xF) )
	    bcost = (costs[3]<<4)+12;
	  if( (costs[1]<<4) < (bcost & ~0xF) )
	    bcost = (costs[1]<<4)+3;
	  if( !(bcost&15) )
	    break;
	  bmx -= (bcost<<28)>>30;
	  bmy -= (bcost<<30)>>30;

	  CHECK_EDGE(bmx, bmy, 
		    ((bcost & 0xF) == 1), 
		    ((bcost & 0xF) == 3), 
		    ((bcost & 0xF) == 4),
		    ((bcost & 0xF) == 12) );

	  MV_HITMAX(bmx, bmy, 
		    ((bcost & 0xF) == 1), 
		    ((bcost & 0xF) == 3), 
		    ((bcost & 0xF) == 4),
		    ((bcost & 0xF) == 12) );

	  bcost &= ~15;

        } while( ++i < i_me_range );
      bcost >>= 4;
    }

    m->mv[0] = bmx << 2;
    m->mv[1] = bmy << 2;
    m->cost = bcost;
    /* subpel refine */

    if(hazard_edge){
      printf("Hazard edge @ MBX: %d, MBY: %d\n", h->mb.i_mb_x, h->mb.i_mb_y);
      return;
    }

    //KENTRO
    if(KENTRO_FSSE){
      if(step_cnt > KENTRO_FSST){
	hit_fsst = 1;
	return;
      }
    }
    if(KENTRO_FSCE){
      if(bcost >= KENTRO_FSCT)
	return;
    }

    refine_subpel( h, m, 1/*hpel*/, 0/*qpel*/, NULL, 0 );
}
#undef COST_MV

void x264_me_refine_qpel( x264_t *h, x264_me_t *m )
{
  refine_subpel( h, m, 0/*hpel*/, 1/*qpel*/, NULL, 1 );
}

#define COST_MV_SAD( mx, my ) \
{ \
    int stride = 16; \
    uint8_t *src = h->mc.get_ref( pix[0], &stride, m->p_fref, m->i_stride[0], mx, my, bw, bh, &m->weight[0] ); \
    int cost = h->pixf.fpelcmp[i_pixel]( m->p_fenc[0], FENC_STRIDE, src, stride ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    COPY3_IF_LT( bcost, cost, bmx, mx, bmy, my ); \
}

//JZVPU
int qpel_cost;
#define COST_MV_SATD( mx, my, dir ) \
if( b_refine_qpel || (dir^1) != odir ) \
{ \
    int stride = 16; \
    uint8_t *src = h->mc.get_ref( pix[0], &stride, m->p_fref, m->i_stride[0], mx, my, bw, bh, &m->weight[0] ); \
    int cost = h->pixf.mbcmp_unaligned[i_pixel]( m->p_fenc[0], FENC_STRIDE, src, stride ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    qpel_cost = cost; \
    if( cost < bcost ) \
    {                  \
        bcost = cost;  \
        bmx = mx;      \
        bmy = my;      \
        bdir = dir;    \
    } \
}

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters, int *p_halfpel_thresh, int b_refine_qpel )
{
    const int bw = x264_pixel_size[m->i_pixel].w;
    const int bh = x264_pixel_size[m->i_pixel].h;
    const uint16_t *p_cost_mvx = m->p_cost_mv - m->mvp[0];
    const uint16_t *p_cost_mvy = m->p_cost_mv - m->mvp[1];
    const int i_pixel = m->i_pixel;
    const int b_chroma_me = h->mb.b_chroma_me && i_pixel <= PIXEL_8x8;
    const int mvy_offset = h->mb.b_interlaced & m->i_ref ? (h->mb.i_mb_y & 1)*4 - 2 : 0;

    ALIGNED_ARRAY_16( uint8_t, pix,[2],[32*18] );   // really 17x17, but round up for alignment
    int omx, omy;
    int i;

    int bmx = m->mv[0];
    int bmy = m->mv[1];
    int bcost = m->cost;
    int odir = -1, bdir;

    //JZVPU
    /* halfpel diamond search */
    int hit_up = 0, hit_left = 0, hit_right = 0, hit_down = 0;
    for( i = hpel_iters; i > 0; i-- )
    {
        int omx = bmx, omy = bmy;
        int costs[4];
        int stride = 32; // candidates are either all hpel or all qpel, so one stride is enough
        uint8_t *src0, *src1, *src2, *src3;
        src0 = h->mc.get_ref( pix[0], &stride, m->p_fref, m->i_stride[0], omx, omy-2, bw, bh+1, &m->weight[0] );
        src2 = h->mc.get_ref( pix[1], &stride, m->p_fref, m->i_stride[0], omx-2, omy, bw+4, bh, &m->weight[0] );
        src1 = src0 + stride;
        src3 = src2 + 1;
        h->pixf.fpelcmp_x4[i_pixel]( m->p_fenc[0], src0, src1, src2, src3, stride, costs );

	if(costs[0] < bcost){
	  hit_up = 1;
	  hit_left = 0;
	  hit_right = 0;
	  hit_down = 0;
	  bcost = costs[0];
	}
	if(costs[2] < bcost){
	  hit_up = 0;
	  hit_left = 1;
	  hit_right = 0;
	  hit_down = 0;
	  bcost = costs[2];
	}
	if(costs[3] < bcost){
	  hit_up = 0;
	  hit_left = 0;
	  hit_right = 1;
	  hit_down = 0;
	  bcost = costs[3];
	}
	if(costs[1] < bcost){
	  hit_up = 0;
	  hit_left = 0;
	  hit_right = 0;
	  hit_down = 1;
	  bcost = costs[1];
	}
	if(hit_up)
	  bmy = bmy - 2;
	if(hit_left)
	  bmx = bmx - 2;
	if(hit_right)
	  bmx = bmx + 2;
	if(hit_down)
	  bmy = bmy + 2;

        if( (bmx == omx) & (bmy == omy) )
            break;
    }

    /* quarterpel diamond search */
    hit_up = hit_left = hit_right = hit_down = 0;
    bdir = -1;
    for( i = qpel_iters; i > 0; i-- )
    {
        odir = bdir;
        omx = bmx;
        omy = bmy;

	if(((omx&3)==0) && ((omy&3)==0)){
	  COST_MV_SATD( omx, omy - 1, 0 );
	  COST_MV_SATD( omx - 1, omy, 2 );
	  COST_MV_SATD( omx + 1, omy, 3 );
	  COST_MV_SATD( omx, omy + 1, 1 );
	} else {
	  COST_MV_SATD( omx-1, omy-1, 0 );
	  COST_MV_SATD( omx-1, omy+1, 1 );
	  COST_MV_SATD( omx+1, omy-1, 2 );
	  COST_MV_SATD( omx+1, omy+1, 3 );
	}

        if( bmx == omx && bmy == omy )
	  break;
    }

    m->cost = bcost;
    m->mv[0] = bmx;
    m->mv[1] = bmy;
}

#define BIME_CACHE( dx, dy, list ) \
{ \
    x264_me_t *m = m##list;\
    int i = 4 + 3*dx + dy; \
    int mvx = om##list##x+dx;\
    int mvy = om##list##y+dy;\
    stride##list[i] = bw;\
    src##list[i] = h->mc.get_ref( pixy_buf[list][i], &stride##list[i], m->p_fref, m->i_stride[0], mvx, mvy, bw, bh, weight_none ); \
    if( rd )\
    {\
        h->mc.mc_chroma( pixu_buf[list][i], 8, m->p_fref[4], m->i_stride[1], mvx, mvy + mv##list##y_offset, bw>>1, bh>>1 );\
        h->mc.mc_chroma( pixv_buf[list][i], 8, m->p_fref[5], m->i_stride[1], mvx, mvy + mv##list##y_offset, bw>>1, bh>>1 );\
    }\
}

#define SATD_THRESH 17/16

/* Don't unroll the BIME_CACHE loop. I couldn't find any way to force this
 * other than making its iteration count not a compile-time constant. */
int x264_iter_kludge = 0;

static void ALWAYS_INLINE x264_me_refine_bidir( x264_t *h, x264_me_t *m0, x264_me_t *m1, int i_weight, int i8, int i_lambda2, int rd )
{
    static const int pixel_mv_offs[] = { 0, 4, 4*8, 0 };
    int16_t *cache0_mv = h->mb.cache.mv[0][x264_scan8[i8*4]];
    int16_t *cache0_mv2 = cache0_mv + pixel_mv_offs[m0->i_pixel];
    int16_t *cache1_mv = h->mb.cache.mv[1][x264_scan8[i8*4]];
    int16_t *cache1_mv2 = cache1_mv + pixel_mv_offs[m0->i_pixel];
    const int i_pixel = m0->i_pixel;
    const int bw = x264_pixel_size[i_pixel].w;
    const int bh = x264_pixel_size[i_pixel].h;
    const uint16_t *p_cost_m0x = m0->p_cost_mv - m0->mvp[0];
    const uint16_t *p_cost_m0y = m0->p_cost_mv - m0->mvp[1];
    const uint16_t *p_cost_m1x = m1->p_cost_mv - m1->mvp[0];
    const uint16_t *p_cost_m1y = m1->p_cost_mv - m1->mvp[1];
    ALIGNED_ARRAY_16( uint8_t, pixy_buf,[2],[9][16*16] );
    ALIGNED_8( uint8_t pixu_buf[2][9][8*8] );
    ALIGNED_8( uint8_t pixv_buf[2][9][8*8] );
    uint8_t *src0[9];
    uint8_t *src1[9];
    uint8_t *pix  = &h->mb.pic.p_fdec[0][(i8>>1)*8*FDEC_STRIDE+(i8&1)*8];
    uint8_t *pixu = &h->mb.pic.p_fdec[1][(i8>>1)*4*FDEC_STRIDE+(i8&1)*4];
    uint8_t *pixv = &h->mb.pic.p_fdec[2][(i8>>1)*4*FDEC_STRIDE+(i8&1)*4];
    const int ref0 = h->mb.cache.ref[0][x264_scan8[i8*4]];
    const int ref1 = h->mb.cache.ref[1][x264_scan8[i8*4]];
    const int mv0y_offset = h->mb.b_interlaced & ref0 ? (h->mb.i_mb_y & 1)*4 - 2 : 0;
    const int mv1y_offset = h->mb.b_interlaced & ref1 ? (h->mb.i_mb_y & 1)*4 - 2 : 0;
    int stride0[9];
    int stride1[9];
    int bm0x = m0->mv[0], om0x = bm0x;
    int bm0y = m0->mv[1], om0y = bm0y;
    int bm1x = m1->mv[0], om1x = bm1x;
    int bm1y = m1->mv[1], om1y = bm1y;
    int bcost = COST_MAX;
    int pass = 0;
    int j;
    int mc_list0 = 1, mc_list1 = 1;
    uint64_t bcostrd = COST_MAX64;
    /* each byte of visited represents 8 possible m1y positions, so a 4D array isn't needed */
    ALIGNED_ARRAY_16( uint8_t, visited,[8],[8][8] );
    /* all permutations of an offset in up to 2 of the dimensions */
    static const int8_t dia4d[33][4] = {
        {0,0,0,0},
        {0,0,0,1}, {0,0,0,-1}, {0,0,1,0}, {0,0,-1,0},
        {0,1,0,0}, {0,-1,0,0}, {1,0,0,0}, {-1,0,0,0},
        {0,0,1,1}, {0,0,-1,-1},{0,1,1,0}, {0,-1,-1,0},
        {1,1,0,0}, {-1,-1,0,0},{1,0,0,1}, {-1,0,0,-1},
        {0,1,0,1}, {0,-1,0,-1},{1,0,1,0}, {-1,0,-1,0},
        {0,0,-1,1},{0,0,1,-1}, {0,-1,1,0},{0,1,-1,0},
        {-1,1,0,0},{1,-1,0,0}, {1,0,0,-1},{-1,0,0,1},
        {0,-1,0,1},{0,1,0,-1}, {-1,0,1,0},{1,0,-1,0},
    };

    if( bm0y < h->mb.mv_min_spel[1] + 8 || bm1y < h->mb.mv_min_spel[1] + 8 ||
        bm0y > h->mb.mv_max_spel[1] - 8 || bm1y > h->mb.mv_max_spel[1] - 8 )
        return;

    h->mc.memzero_aligned( visited, sizeof(uint8_t[8][8][8]) );

    for( pass = 0; pass < 8; pass++ )
    {
        /* check all mv pairs that differ in at most 2 components from the current mvs. */
        /* doesn't do chroma ME. this probably doesn't matter, as the gains
         * from bidir ME are the same with and without chroma ME. */

        if( mc_list0 )
            for( j = x264_iter_kludge; j < 9; j++ )
                BIME_CACHE( square1[j][0], square1[j][1], 0 );

        if( mc_list1 )
            for( j = x264_iter_kludge; j < 9; j++ )
                BIME_CACHE( square1[j][0], square1[j][1], 1 );

        for( j = !!pass; j < 33; j++ )
        {
            int m0x = dia4d[j][0] + om0x;
            int m0y = dia4d[j][1] + om0y;
            int m1x = dia4d[j][2] + om1x;
            int m1y = dia4d[j][3] + om1y;
            if( !pass || !((visited[(m0x)&7][(m0y)&7][(m1x)&7] & (1<<((m1y)&7)))) )
            {
                int i0 = 4 + 3*(m0x-om0x) + (m0y-om0y);
                int i1 = 4 + 3*(m1x-om1x) + (m1y-om1y);
                visited[(m0x)&7][(m0y)&7][(m1x)&7] |= (1<<((m1y)&7));
                h->mc.avg[i_pixel]( pix, FDEC_STRIDE, src0[i0], stride0[i0], src1[i1], stride1[i1], i_weight );
                int cost = h->pixf.mbcmp[i_pixel]( m0->p_fenc[0], FENC_STRIDE, pix, FDEC_STRIDE )
                         + p_cost_m0x[m0x] + p_cost_m0y[m0y] + p_cost_m1x[m1x] + p_cost_m1y[m1y];
                if( rd )
                {
                    if( cost < bcost * SATD_THRESH )
                    {
                        bcost = X264_MIN( cost, bcost );
                        M32( cache0_mv  ) = pack16to32_mask(m0x,m0y);
                        M32( cache0_mv2 ) = pack16to32_mask(m0x,m0y);
                        M32( cache1_mv  ) = pack16to32_mask(m1x,m1y);
                        M32( cache1_mv2 ) = pack16to32_mask(m1x,m1y);
                        h->mc.avg[i_pixel+3]( pixu, FDEC_STRIDE, pixu_buf[0][i0], 8, pixu_buf[1][i1], 8, i_weight );
                        h->mc.avg[i_pixel+3]( pixv, FDEC_STRIDE, pixv_buf[0][i0], 8, pixv_buf[1][i1], 8, i_weight );
                        uint64_t costrd = x264_rd_cost_part( h, i_lambda2, i8*4, m0->i_pixel );
                        COPY5_IF_LT( bcostrd, costrd, bm0x, m0x, bm0y, m0y, bm1x, m1x, bm1y, m1y );
                    }
                }
                else
                    COPY5_IF_LT( bcost, cost, bm0x, m0x, bm0y, m0y, bm1x, m1x, bm1y, m1y );
            }
        }

        mc_list0 = (om0x-bm0x)|(om0y-bm0y);
        mc_list1 = (om1x-bm1x)|(om1y-bm1y);
        if( !mc_list0 && !mc_list1 )
            break;

        om0x = bm0x;
        om0y = bm0y;
        om1x = bm1x;
        om1y = bm1y;
    }

    m0->mv[0] = bm0x;
    m0->mv[1] = bm0y;
    m1->mv[0] = bm1x;
    m1->mv[1] = bm1y;
}

void x264_me_refine_bidir_satd( x264_t *h, x264_me_t *m0, x264_me_t *m1, int i_weight )
{
    x264_me_refine_bidir( h, m0, m1, i_weight, 0, 0, 0 );
}

void x264_me_refine_bidir_rd( x264_t *h, x264_me_t *m0, x264_me_t *m1, int i_weight, int i8, int i_lambda2 )
{
    /* Motion compensation is done as part of bidir_rd; don't repeat
     * it in encoding. */
    h->mb.b_skip_mc = 1;
    x264_me_refine_bidir( h, m0, m1, i_weight, i8, i_lambda2, 1 );
    h->mb.b_skip_mc = 0;
}

#undef COST_MV_SATD
#define COST_MV_SATD( mx, my, dst, avoid_mvp ) \
{ \
    if( !avoid_mvp || !(mx == pmx && my == pmy) ) \
    { \
        h->mc.mc_luma( pix, FDEC_STRIDE, m->p_fref, m->i_stride[0], mx, my, bw, bh, &m->weight[0] ); \
        dst = h->pixf.mbcmp[i_pixel]( m->p_fenc[0], FENC_STRIDE, pix, FDEC_STRIDE ) \
            + p_cost_mvx[mx] + p_cost_mvy[my]; \
        COPY1_IF_LT( bsatd, dst ); \
    } \
    else \
        dst = COST_MAX; \
}

#define COST_MV_RD( mx, my, satd, do_dir, mdir ) \
{ \
    if( satd <= bsatd * SATD_THRESH ) \
    { \
        uint64_t cost; \
        M32( cache_mv  ) = pack16to32_mask(mx,my); \
        M32( cache_mv2 ) = pack16to32_mask(mx,my); \
        if( m->i_pixel <= PIXEL_8x8 )\
        {\
            h->mc.mc_chroma( pixu, FDEC_STRIDE, m->p_fref[4], m->i_stride[1], mx, my + mvy_offset, bw>>1, bh>>1 );\
            h->mc.mc_chroma( pixv, FDEC_STRIDE, m->p_fref[5], m->i_stride[1], mx, my + mvy_offset, bw>>1, bh>>1 );\
        }\
        cost = x264_rd_cost_part( h, i_lambda2, i4, m->i_pixel ); \
        COPY4_IF_LT( bcost, cost, bmx, mx, bmy, my, dir, do_dir?mdir:dir ); \
    } \
}

void x264_me_refine_qpel_rd( x264_t *h, x264_me_t *m, int i_lambda2, int i4, int i_list )
{
    // don't have to fill the whole mv cache rectangle
    static const int pixel_mv_offs[] = { 0, 4, 4*8, 0, 2, 2*8, 0 };
    int16_t *cache_mv = h->mb.cache.mv[i_list][x264_scan8[i4]];
    int16_t *cache_mv2 = cache_mv + pixel_mv_offs[m->i_pixel];
    const uint16_t *p_cost_mvx, *p_cost_mvy;
    const int bw = x264_pixel_size[m->i_pixel].w;
    const int bh = x264_pixel_size[m->i_pixel].h;
    const int i_pixel = m->i_pixel;
    const int mvy_offset = h->mb.b_interlaced & m->i_ref ? (h->mb.i_mb_y & 1)*4 - 2 : 0;

    uint64_t bcost = COST_MAX64;
    int bmx = m->mv[0];
    int bmy = m->mv[1];
    int omx, omy, pmx, pmy, i, j;
    unsigned bsatd;
    int satd;
    int dir = -2;
    int i8 = i4>>2;

    uint8_t *pix  = &h->mb.pic.p_fdec[0][block_idx_xy_fdec[i4]];
    uint8_t *pixu = &h->mb.pic.p_fdec[1][(i8>>1)*4*FDEC_STRIDE+(i8&1)*4];
    uint8_t *pixv = &h->mb.pic.p_fdec[2][(i8>>1)*4*FDEC_STRIDE+(i8&1)*4];

    h->mb.b_skip_mc = 1;

    if( m->i_pixel != PIXEL_16x16 && i4 != 0 )
        x264_mb_predict_mv( h, i_list, i4, bw>>2, m->mvp );
    pmx = m->mvp[0];
    pmy = m->mvp[1];
    p_cost_mvx = m->p_cost_mv - pmx;
    p_cost_mvy = m->p_cost_mv - pmy;
    COST_MV_SATD( bmx, bmy, bsatd, 0 );
    if( m->i_pixel != PIXEL_16x16 )
        COST_MV_RD( bmx, bmy, 0, 0, 0 )
    else
        bcost = m->cost;

    /* check the predicted mv */
    if( (bmx != pmx || bmy != pmy)
        && pmx >= h->mb.mv_min_spel[0] && pmx <= h->mb.mv_max_spel[0]
        && pmy >= h->mb.mv_min_spel[1] && pmy <= h->mb.mv_max_spel[1] )
    {
        COST_MV_SATD( pmx, pmy, satd, 0 );
        COST_MV_RD  ( pmx, pmy, satd, 0, 0 );
        /* The hex motion search is guaranteed to not repeat the center candidate,
         * so if pmv is chosen, set the "MV to avoid checking" to bmv instead. */
        if( bmx == pmx && bmy == pmy )
        {
            pmx = m->mv[0];
            pmy = m->mv[1];
        }
    }

    if( bmy < h->mb.mv_min_spel[1] + 3 ||
        bmy > h->mb.mv_max_spel[1] - 3 )
    {
        h->mb.b_skip_mc = 0;
        return;
    }

    /* subpel hex search, same pattern as ME HEX. */
    dir = -2;
    omx = bmx;
    omy = bmy;
    for( j=0; j<6; j++ )
    {
        COST_MV_SATD( omx + hex2[j+1][0], omy + hex2[j+1][1], satd, 1 );
        COST_MV_RD  ( omx + hex2[j+1][0], omy + hex2[j+1][1], satd, 1, j );
    }

    if( dir != -2 )
    {
        /* half hexagon, not overlapping the previous iteration */
        for( i = 1; i < 10; i++ )
        {
            const int odir = mod6m1[dir+1];
            if( bmy < h->mb.mv_min_spel[1] + 3 ||
                bmy > h->mb.mv_max_spel[1] - 3 )
                break;
            dir = -2;
            omx = bmx;
            omy = bmy;
            for( j=0; j<3; j++ )
            {
                COST_MV_SATD( omx + hex2[odir+j][0], omy + hex2[odir+j][1], satd, 1 );
                COST_MV_RD  ( omx + hex2[odir+j][0], omy + hex2[odir+j][1], satd, 1, odir-1+j );
            }
            if( dir == -2 )
                break;
        }
    }

    /* square refine, same pattern as ME HEX. */
    omx = bmx;
    omy = bmy;
    for( i=0; i<8; i++ )
    {
        COST_MV_SATD( omx + square1[i+1][0], omy + square1[i+1][1], satd, 1 );
        COST_MV_RD  ( omx + square1[i+1][0], omy + square1[i+1][1], satd, 0, 0 );
    }

    m->cost = bcost;
    m->mv[0] = bmx;
    m->mv[1] = bmy;
    x264_macroblock_cache_mv ( h, block_idx_x[i4], block_idx_y[i4], bw>>2, bh>>2, i_list, pack16to32_mask(bmx, bmy) );
    x264_macroblock_cache_mvd( h, block_idx_x[i4], block_idx_y[i4], bw>>2, bh>>2, i_list, pack16to32_mask(bmx - m->mvp[0], bmy - m->mvp[1]) );
    h->mb.b_skip_mc = 0;
}
