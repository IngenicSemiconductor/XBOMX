#ifndef __T_INTPID_H__
#define __T_INTPID_H__

#define __place_k0_data__

typedef char I8;
typedef unsigned char U8;
typedef short I16;
typedef unsigned short U16;

typedef int I32;
typedef unsigned int U32;

enum BlkTYP {
    BLK16X16 = 0,
    BLK16X8,
    BLK8X16,
    BLK8X8,
};
enum SBlkTYP {
    NOSUB = 0,
    SBLK8X4,
    SBLK4X8,
    SBLK4X4,
};

typedef struct CompVector_t{
      /*MB information*/
    U8 mbx, mby;
    U8 mbtype;
    U8 rst;
    U8 rote, rotdir;
    U8 automv;
    U8 mbpar, subpar[4];
    U8 field[4];
    U8 list[2][4], refn[2][4];
    I16 mvx[2][4][4][2], mvy[2][4][4][2];
      /*Ext. information*/
    U8 intpid;
    U8 cintpid;
    U8 rgr;
    U8 its;
    I8 its_scale;
    I16 its_rnd;
    U8 its_sft;
    U8 wt_mode;
    U8 wt_denom;
    I8 wt_lcoef, wt_rcoef;
    I8 wt_rnd;
    U8 wt_sft;
      /*Golden*/
    U8 std_y[256];
    U8 std_u[64];
    U8 std_v[64];
}CompVector_t;

typedef struct EstiVector_t{
      /*MB information*/
    U8 mbx, mby;
    U8 rst;
    U8 mbpar, subpar[4];
    U8 list[4];
    U8 expd;
    I8 pmvx, pmvy;
    U8 intpid;
    U8 cintpid;
      /*RAW*/
    U32 dummy;
    U8 raw[256];
      /*Golden*/
    U16 cost[4][4];
    I8 mvx[4][4], mvy[4][4];
    U8 bstep[4][4], sstep[4][4];
    U8 met_y[256];
    U8 met_u[64];
    U8 met_v[64];
}EstiVector_t;

typedef struct WTpara_t{
    I8 wcoef_y;
    I8 wofst_y;
    I8 wcoef_u;
    I8 wofst_u;
    I8 wcoef_v;
    I8 wofst_v;
} WTpara_t;

#if 0
__place_k0_data__
static char ParNum[] = {1, 2, 2, 4};

__place_k0_data__
static char BlkOfst[4][4][2] = 
    { {0},
      {{0},{0,8}},
      {{0},{8,0}},
      {{0},{8,0},{0,8},{8,8}}
    };

__place_k0_data__
static char SblkOfst[4][4][2] = 
    { {0},
      {{0},{0,4}},
      {{0},{4,0}},
      {{0},{4,0},{0,4},{4,4}}
    };

__place_k0_data__
static char BlkWidth[] = {0x3, 0x3, 0x2, 0x2};

__place_k0_data__
static char BlkHeight[] = {0x3, 0x2, 0x3, 0x2};

__place_k0_data__
static char SblkWidth[] = {0x2, 0x2, 0x1, 0x1};

__place_k0_data__
static char SblkHeight[] = {0x2, 0x1, 0x2, 0x1};
#endif

#endif /*__T_INTPID_H__*/
