/*******************************************************
 Motion Test Center
 ******************************************************/
#define __place_k0_data__
#undef printf
#undef fprintf
#include "t_motion.h"
#include "rv9_dcore.h"

#define MPEG_HPEL  0
#define MPEG_QPEL  1
#define H264_QPEL  2
#define H264_EPEL  3
#define RV8_TPEL   4
#define RV9_QPEL   5
#define RV9_CPEL   6 
#define WMV2_QPEL  7
#define VC1_QPEL   8
#define AVS_QPEL   9
#define VP6_QPEL   10
#define VP8_QPEL   11

enum SPelSFT {
    HPEL = 1,
    QPEL,
    EPEL,
};

#define IS_EC     1
#define IS_ILUT0  0
#define IS_ILUT1  2

#define BLK3300     0x0F0
#define BLK3200     0x0E0
#define BLK3202     0x2E0
#define BLK2300     0x0B0
#define BLK2320     0x8B0
#define BLK2200     0x0A0
#define BLK2202     0x2A0
#define BLK2220     0x8A0
#define BLK2222     0xAA0

// is_bidir  dir   doe  cflo
static int sort0[] = {0x0,        0x40000000,  0x3000000, 0x43000000, 0x2000000,  0x42000000};
static int sort1[] = {0x80000000, 0xC0000000, 0x80000000, 0xC3000000, 0x80000000, 0xC2000000};

static int rv40_1mv_mc(RV9_MB_DecARGs *dMB, volatile int *tdd, int *tkn, int dir0, int dir1)
{
    int is_bidir = dir0 && dir1;
    int * sort = is_bidir ? sort1 : sort0;
    int mvx, mvy, pos;
    // Y
    if( dir0 ) {
	mvx = dMB->motion_val[0][0][0];
	mvy = dMB->motion_val[0][0][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[2] | BLK3300 | pos ;
	(*tkn)++;
    }

    if( dir1 ) {
	mvx = dMB->motion_val[1][0][0];
	mvy = dMB->motion_val[1][0][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[3] | BLK3300 | pos ;
	(*tkn)++;
    }
    // C
    if( dir0 ) {
	mvx = dMB->motion_val[0][0][0] / 2;
	mvy = dMB->motion_val[0][0][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[4] | BLK3300 | pos ;
	(*tkn)++;
    }

    if( dir1 ) {
	mvx = dMB->motion_val[1][0][0] / 2;
	mvy = dMB->motion_val[1][0][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[5] | BLK3300 | pos ;
	(*tkn)++;
    }

    return 0;
}

static int rv40_2mv_16x8mc(RV9_MB_DecARGs *dMB, volatile int *tdd, int *tkn, int dir0, int dir1)
{
    int is_bidir = dir0 && dir1;
    int * sort = is_bidir ? sort1 : sort0;
    int mvx, mvy, pos;
    // Y
    if( dir0 ) {
	mvx = dMB->motion_val[0][0][0];
	mvy = dMB->motion_val[0][0][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK2300 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][2][0];
	mvy = dMB->motion_val[0][2][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[2] | BLK2320 | pos ;
	(*tkn)++;
    }

    if( dir1 ) {
	mvx = dMB->motion_val[1][0][0];
	mvy = dMB->motion_val[1][0][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK2300 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][2][0];
	mvy = dMB->motion_val[1][2][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[3] | BLK2320 | pos ;
	(*tkn)++;
    }
    // C
    if( dir0 ) {
	mvx = dMB->motion_val[0][0][0] / 2;
	mvy = dMB->motion_val[0][0][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK2300 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][2][0] / 2;
	mvy = dMB->motion_val[0][2][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[4] | BLK2320 | pos ;
	(*tkn)++;
    }

    if( dir1 ) {
	mvx = dMB->motion_val[1][0][0] / 2;
	mvy = dMB->motion_val[1][0][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK2300 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][2][0] / 2;
	mvy = dMB->motion_val[1][2][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[5] | BLK2320 | pos ;
	(*tkn)++;
    }

    return 0;
}

static int rv40_2mv_8x16mc(RV9_MB_DecARGs *dMB, volatile int *tdd, int *tkn, int dir0, int dir1)
{
    int is_bidir = dir0 && dir1;
    int * sort = is_bidir ? sort1 : sort0;
    int mvx, mvy, pos;
    // Y
    if( dir0 ) {
	mvx = dMB->motion_val[0][0][0];
	mvy = dMB->motion_val[0][0][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK3200 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][1][0];
	mvy = dMB->motion_val[0][1][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[2] | BLK3202 | pos ;
	(*tkn)++;
    }

    if( dir1 ) {
	mvx = dMB->motion_val[1][0][0];
	mvy = dMB->motion_val[1][0][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK3200 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][1][0];
	mvy = dMB->motion_val[1][1][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[3] | BLK3202 | pos ;
	(*tkn)++;
    }
    // C
    if( dir0 ) {
	mvx = dMB->motion_val[0][0][0] / 2;
	mvy = dMB->motion_val[0][0][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK3200 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][1][0] / 2;
	mvy = dMB->motion_val[0][1][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[4] | BLK3202 | pos ;
	(*tkn)++;
    }

    if( dir1 ) {
	mvx = dMB->motion_val[1][0][0] / 2;
	mvy = dMB->motion_val[1][0][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK3200 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][1][0] / 2;
	mvy = dMB->motion_val[1][1][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[5] | BLK3202 | pos ;
	(*tkn)++;
    }

    return 0;
}

static int rv40_4mv_mc(RV9_MB_DecARGs *dMB, volatile int *tdd, int *tkn, int dir0, int dir1)
{
    int is_bidir = dir0 && dir1;
    int * sort = is_bidir ? sort1 : sort0;
    int mvx, mvy, pos;
    // Y
    if( dir0 ) {
	mvx = dMB->motion_val[0][0][0];
	mvy = dMB->motion_val[0][0][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK2200 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][1][0];
	mvy = dMB->motion_val[0][1][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK2202 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][2][0];
	mvy = dMB->motion_val[0][2][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK2220 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][3][0];
	mvy = dMB->motion_val[0][3][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[2] | BLK2222 | pos ;
	(*tkn)++;
    }

    if( dir1 ) {
	mvx = dMB->motion_val[1][0][0];
	mvy = dMB->motion_val[1][0][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK2200 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][1][0];
	mvy = dMB->motion_val[1][1][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK2202 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][2][0];
	mvy = dMB->motion_val[1][2][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK2220 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][3][0];
	mvy = dMB->motion_val[1][3][1];
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[3] | BLK2222 | pos ;
	(*tkn)++;
    }
    // C
    if( dir0 ) {
	mvx = dMB->motion_val[0][0][0] / 2;
	mvy = dMB->motion_val[0][0][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK2200 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][1][0] / 2;
	mvy = dMB->motion_val[0][1][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK2202 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][2][0] / 2;
	mvy = dMB->motion_val[0][2][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[0] | BLK2220 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[0][3][0] / 2;
	mvy = dMB->motion_val[0][3][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[4] | BLK2222 | pos ;
	(*tkn)++;
    }

    if( dir1 ) {
	mvx = dMB->motion_val[1][0][0] / 2;
	mvy = dMB->motion_val[1][0][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK2200 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][1][0] / 2;
	mvy = dMB->motion_val[1][1][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK2202 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][2][0] / 2;
	mvy = dMB->motion_val[1][2][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[1] | BLK2220 | pos ;
	(*tkn)++;

	mvx = dMB->motion_val[1][3][0] / 2;
	mvy = dMB->motion_val[1][3][1] / 2;
	pos = ((mvy & 0x3) << 2) | (mvx & 0x3);
	tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
	tdd[ 2*tkn[0]+1 ] = sort[5] | BLK2222 | pos ;
	(*tkn)++;
    }

    return 0;
}

static int rv40_decode_mv_aux(RV9_Slice_GlbARGs *dSlice,RV9_MB_DecARGs *dMB, uint8_t *motion_dha)
{
    int block_type = dMB->mbtype;

    volatile int *tdd = (int *)motion_dha;
    int tkn = 0;
    tdd++;	  	  

    switch(block_type){
    case RV34_MB_SKIP:
	if(dSlice->pict_type == FF_P_TYPE){
	    rv40_1mv_mc(dMB, tdd, &tkn, 1, 0);
	    mc_flag = 1;
	    break;
	}
    case RV34_MB_B_DIRECT:
	//surprisingly, it uses motion scheme from next reference frame
	if(!(IS_16X8(dMB->next_bt) || IS_8X16(dMB->next_bt) || IS_8X8(dMB->next_bt))) {
	    rv40_1mv_mc(dMB, tdd, &tkn, 1, 1);
	} else {
	    rv40_4mv_mc(dMB, tdd, &tkn, 1, 1);
	}
	mc_flag = 1;
	break;
    case RV34_MB_P_16x16:
    case RV34_MB_P_MIX16x16:
	rv40_1mv_mc(dMB, tdd, &tkn, 1, 0);
	mc_flag = 1;
	break;
    case RV34_MB_B_FORWARD:
    case RV34_MB_B_BACKWARD:
	rv40_1mv_mc(dMB, tdd, &tkn, block_type == RV34_MB_B_FORWARD, block_type == RV34_MB_B_BACKWARD);
	mc_flag = 1;
	break;
    case RV34_MB_P_16x8:
    case RV34_MB_P_8x16:
	if(block_type == RV34_MB_P_16x8){
	    rv40_2mv_16x8mc(dMB, tdd, &tkn, 1, 0);
	}
	if(block_type == RV34_MB_P_8x16){
	    rv40_2mv_8x16mc(dMB, tdd, &tkn, 1, 0);
	}
	mc_flag = 1;
	break;
    case RV34_MB_B_BIDIR:
	rv40_1mv_mc(dMB, tdd, &tkn, 1, 1);
	mc_flag = 1;
	break;
    case RV34_MB_P_8x8:
	rv40_4mv_mc(dMB, tdd, &tkn, 1, 0);
	mc_flag = 1;
	break;
    }

    tdd[-1] = TDD_HEAD(1,/*vld*/
		       1,/*lk*/
		       1,/*ch1pel*/
		       1,/*ch2pel*/ 
		       TDD_POS_SPEC,/*posmd*/
		       TDD_MV_SPEC,/*mvmd*/ 
		       tkn,/*tkn*/
		       dMB->mb_y,/*mby*/
		       dMB->mb_x/*mbx*/);

    tdd[2*tkn] = TDD_HEAD(1,/*vld*/
			  1,/*lk*/
			  0,/*ch1pel*/
			  0,/*ch2pel*/ 
			  TDD_POS_SPEC,/*posmd*/
			  TDD_MV_SPEC,/*mvmd*/
			  0,/*tkn*/
			  0,/*mby*/
			  0/*mbx*/);

    tdd[2*tkn+1] = TDD_SYNC(1,/*vld*/
			    0,/*lk*/
			    0, /*crst*/
			    0xFFFF/*id*/);

    return 0;
}
