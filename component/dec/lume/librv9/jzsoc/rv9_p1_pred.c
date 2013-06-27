#include "../../libjzcommon/jzasm.h"
#include "../../libjzcommon/jzmedia.h"
#include "rv9_dcore.h"
#include "rv9_p1_type.h"

#define VERT_PRED             0
#define HOR_PRED              1
#define DC_PRED               2
#define DIAG_DOWN_LEFT_PRED   3
#define DIAG_DOWN_RIGHT_PRED  4
#define VERT_RIGHT_PRED       5
#define HOR_DOWN_PRED         6
#define VERT_LEFT_PRED        7
#define HOR_UP_PRED           8

#define LEFT_DC_PRED          9
#define TOP_DC_PRED           10
#define DC_128_PRED           11

#define DIAG_DOWN_LEFT_PRED_RV40_NODOWN   12
#define HOR_UP_PRED_RV40_NODOWN           13
#define VERT_LEFT_PRED_RV40_NODOWN        14

#define DC_PRED8x8            0
#define HOR_PRED8x8           1
#define VERT_PRED8x8          2
#define PLANE_PRED8x8         3

#define LEFT_DC_PRED8x8       4
#define TOP_DC_PRED8x8        5
#define DC_128_PRED8x8        6
int offset_y[16] ={ 0, 4, 8, 12, 80, 84, 88, 92, 160, 164, 168, 172, 240, 244, 248, 252};
int offset_c[4]  ={ 0, 4, 48, 52};
uint8_t left_top_table_div[24] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3,3,3,3,0, 0,1,1};
uint8_t left_top_table_res[24] = {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0,1,2,3,0, 1,0,1};

static void rv40_pred_4x4_block_aux(RV9_MB_DecARGs *dMB, uint8_t *dst, int stride,int num, uint8_t *intra_top, uint8_t *recon, uint8_t topleft)
{
  int i, k, l;
  int intra4x4_left[3];
  uint8_t *top, topl;
  uint8_t *src_left = intra4x4_left;
  int div, res, tag;
  uint32_t dc,dcl,dct,t7;
  uint8_t *recon_tmp;

  div = left_top_table_div[num];
  res = left_top_table_res[num];
  if (!div){
    if (!res){
      topl = topleft;
      recon_tmp = recon+(div*4)*stride+((num<16)?15:7);
    }
    else{
      topl = intra_top[res * 4 - 1];
      recon_tmp = dst-1;
    }
    top = intra_top + res * 4;
  }else{
    if (!res){
      tag = ((num<16)?15:7);
      topl = recon[(div*4-1)*stride+tag];
      recon_tmp = recon+(div*4)*stride+tag;
    }
    else{
      topl = dst[-1*stride-1];
      recon_tmp = dst-1;
    }
    top = dst - stride;
  } 
  S32LDD(xr1, top, 0x0);     //xr1: t3 t2 t1 t0  
  if (!dMB->rup[num]){
    t7= top[7];
    S32LDD(xr2, top, 0x4);     //xr2: t7 t6 t5 t4
  }else{
    t7= top[3];
    S8LDD(xr2, top, 3, 7);
  }
  switch(dMB->itype4[num]){
  case VERT_PRED:
    S32LDD(xr15, top,0x0); //[4] [5] [6] [7]   
    S32STD (xr15, dst, 0);
    S32STDV(xr15, dst, stride,0);
    S32STDV(xr15, dst, stride*2,0);
    S32STDV(xr15, dst, stride*3,0);
    break;
    //dc=((uint32_t*)src_top)[1];
    //pred4x4_vertical_test(dst,stride,src_top+4);
  case LEFT_DC_PRED:
    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
    }
    //S32STD(xr15,src_left,0x4);
    //S32LDD(xr15,src_left,0x4);
    D8SUMC(xr14,xr15,xr15);  
    Q16SAR(xr14,xr14,xr14,xr13,2);
    Q16SAT(xr14,xr14,xr13);
    S32STD (xr14, dst, 0);
    S32STDV(xr14, dst, stride,0);
    S32STDV(xr14, dst, stride*2,0);
    S32STDV(xr14, dst, stride*3,0); 
    break;

    //dc=(dcl>>2)*0x01010101;
    //pred4x4_left_dc_test(dst,stride,src_left+4);
  case TOP_DC_PRED:
    S32LDD(xr15,top,0x0);
    D8SUMC(xr14,xr15,xr15);  
    Q16SAR(xr14,xr14,xr14,xr13,2);
    Q16SAT(xr14,xr14,xr13);
    S32STD (xr14, dst, 0);
    S32STDV(xr14, dst, stride,0);
    S32STDV(xr14, dst, stride*2,0);
    S32STDV(xr14, dst, stride*3,0); 
    break;
    //dc=(dct>>2)*0x01010101;
    //pred4x4_top_dc_test(dst,stride,src_top+4); 
  case DC_128_PRED:
    //    dc = 128;
    //S8LDD(xr15,&dc,0,7);
    S32I2M(xr15,0x80808080);
    S32STD (xr15, dst, 0);
    S32STDV(xr15, dst, stride,0);
    S32STDV(xr15, dst, stride*2,0);
    S32STDV(xr15, dst, stride*3,0);	      
    break;
    //dc=128U*0x01010101U;
    //pred4x4_128_dc_test(dst,stride);
  case DC_PRED:
    //dcl= (src_left[4] + src_left[5] + src_left[6] + src_left[7] + 2);
    //dct= (src_top[4]  + src_top[5]  + src_top[6]  + src_top[7]  + 2);
    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
    }
    //S32STD(xr15,src_left,0x4);

    //S32LDD(xr15,src_left,0x4);
    S32LDD(xr14,top ,0x0);
    D8SUMC(xr15,xr15,xr14);
    Q16ADD_AA_XW(xr13,xr15,xr15,xr12);
    Q16SAR(xr13,xr13,xr12,xr12,3);
    Q16SAT(xr13,xr13,xr12);
    S32STD (xr13, dst, 0);
    S32STDV(xr13, dst, stride,0);
    S32STDV(xr13, dst, stride*2,0);
    S32STDV(xr13, dst, stride*3,0);     
    break;    
    //pred4x4_dc_test(dst,stride,src_top+4,src_left+4);
    break;
  case HOR_PRED:

    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
    }
    S32STD(xr15,src_left,0x4);
    S8LDD (xr15,src_left, 4, 7);
    S8LDD (xr14,src_left, 5, 7);
    S8LDD (xr13,src_left, 6, 7);
    S8LDD (xr12,src_left, 7, 7);

    S32STD (xr15, dst, 0);
    S32STDV(xr14, dst, stride,0);
    S32STDV(xr13, dst, stride*2,0);
    S32STDV(xr12, dst, stride*3,0);	      
    break;
  case DIAG_DOWN_RIGHT_PRED:
    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
    }
    S32STD(xr15,src_left,0x4);
    S32LDDR(xr11,src_left,0x4);     //xr11:l0 l1 l2 l3
    S8LDD  (xr12,&topl, 0,7);     //xr12:lt lt lt lt
    S32ALNI(xr13,xr11,xr11,ptn1);   //xr13:l1 l2 l3 l0
    S32ALNI(xr14,xr11,xr13,ptn2);   //xr14:12 l3 l1 l2  //0
    S32SFL (xr15,xr11,xr13,xr0,ptn3);//xr15:l0 l1 l1 l2 //1
    S32SFL (xr10,xr11,xr15,xr0,ptn3);//xr10:10 l1 l0 l1
    S32ALNI(xr9 ,xr12,xr10,ptn3);    //xr 9:lt 10 l1 l0 //2
    S32ALNI(xr8 ,xr1 ,xr12,ptn3);    //xr 8:t0 lt lt lt 
    S32ALNI(xr7 ,xr9 ,xr8 ,ptn3);    //xr 7:l0 t0 lt lt //3

    D8SUMC (xr6,xr7,xr9);
    D8SUMC (xr10,xr15,xr14);
    Q16SAR (xr6,xr6,xr10,xr10,2);
    Q16SAT (xr6,xr6,xr10);
    S32STDV(xr6,dst,3*stride,0);

    S32ALNI(xr6,xr1,xr8,ptn2);     //xr6:t1 t0 t0 lt   //3
    D8SUMC (xr5,xr6,xr7);
    D8SUMC (xr4,xr9,xr15);
    Q16SAR (xr5,xr5,xr4 ,xr4 ,2);
    Q16SAT (xr5,xr5,xr4 );
    S32STDV(xr5,dst,2*stride,0);

    S32ALNI(xr5,xr1,xr6,ptn1);   //xr5:t2 t1 t0 t1
    D8SUMC (xr4,xr5,xr6);       
    D8SUMC (xr3,xr7,xr9);
    Q16SAR (xr4,xr4,xr3 ,xr3 ,2);
    Q16SAT (xr4,xr4,xr3 );
    S32STDV(xr4,dst,stride,0);

    S32SFL (xr4,xr1,xr5,xr0,ptn3);//xr4:t3 t2 t2 t1
    D8SUMC (xr4,xr4,xr5);       
    D8SUMC (xr6,xr6,xr7);
    Q16SAR (xr4,xr4,xr6 ,xr6 ,2);
    Q16SAT (xr4,xr4,xr6 );
    S32STD (xr4,dst,0);    
    //pred4x4_down_right_test(dst,stride,src_top+4,src_left+4);
    break;
  case VERT_RIGHT_PRED:
    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
    }
    //S32STD(xr15,src_left,0x4);
    S8LDD  (xr11,&topl, 0,7);     //xr11:lt lt lt lt    
    S32ALNI(xr12,xr1,xr11,ptn1);    //xr12:t2 t1 t0 lt
    Q8AVGR (xr2,xr12,xr1);         
    S32STD (xr2,dst,0);    

    //S32LDD (xr15,src_left,0x4);    //xr15:l3 l2 l1 l0    
    S32ALNI(xr13,xr1,xr12,ptn2);   //xr13:t1 t0 t2 t1
    S32ALNI(xr14,xr12,xr13,ptn2);  //xr14:t0 lt t1 t0

    S32ALNI(xr10,xr15,xr11,ptn2);  //xr10:l1 l0 lt lt
    S32ALNI(xr9 ,xr10,xr14,ptn1);  //xr9 :l0 lt lt t0
    S32ALNI(xr8 ,xr15,xr15,ptn3);  //xr8 :l0 l3 l2 l1
    S32ALNI(xr7 ,xr8 ,xr10,ptn2);  //xr7 :l2 l1 l1 l0
    
    D8SUMC(xr6 ,xr13,xr14);        //xr6:
    D8SUMC(xr5 ,xr9 ,xr7 );        //xr5:
    Q16SAR(xr6,xr6,xr5,xr5,2);
    Q16SAT(xr6,xr6,xr5);           //xr6:
    S32STDV(xr6,dst,3*stride,0);
    
    S32ALNI(xr5,xr15,xr9,ptn2); //l1 l0 l0 lt
    D8SUMC (xr4,xr5,xr0);       //
    Q16SAR (xr4,xr4,xr0,xr0,2);
    Q16SAT (xr4,xr4,xr0);
    S32ALNI(xr4,xr2,xr4, ptn1);      
    S32STDV(xr4,dst,2*stride,0);    
    
    S32ALNI(xr4,xr13,xr1,ptn2);  //xr4:t2 t1 t3 t2
    D8SUMC (xr4,xr4,xr13);    
    D8SUMC (xr5,xr14,xr9);//
    Q16SAR (xr4,xr4,xr5,xr5,2);
    Q16SAT (xr4,xr4,xr5);
    S32STDV(xr4,dst,stride,0);    
    //pred4x4_vertical_right_test(dst,stride,src_top+4,src_left+4);
    break;
  case HOR_DOWN_PRED:
    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
    }
    S32STD(xr15,src_left,0x4);
    S32LDDR (xr15,src_left,0x4);   //xr15:l0 l1 l2 l3        
    S8LDD  (xr11,&topl, 0,7);    //xr11:lt lt lt lt  
    S32ALNI(xr14,xr11,xr15,ptn3);  //xr14:lt l0 l1 l2
    Q8AVGR (xr2,xr15,xr14);        //xr2 :dst[0] dst[stride] dst[2*stride] dst[3*stride] 
    S32ALNI(xr13,xr1,xr11,ptn1);   //xr13:t2 t1 t0 lt
    S32ALNI(xr12,xr1,xr13,ptn2);   //xr12:t1 t0 t2 t1
    S32ALNI(xr11,xr13,xr12,ptn2);  //xr11:t0 lt t1 t0
    S32ALNI(xr10,xr13,xr14,ptn2);  //xr10:t0 lt lt l0
    S32ALNI(xr9 ,xr10,xr15,ptn2);  //xr9 :lt l0 l0 l1

    D8SUMC(xr14, xr12,xr11);        
    D8SUMC(xr8, xr10,xr9); 
    Q16SAR(xr14,xr14,xr8,xr8,2);
    Q16SAT(xr14,xr14,xr8);           //xr14:dst[3] dst[2] dst[1] dst[1+1*stride]
 
    S32ALNI(xr8,xr14,xr14,ptn3);     //xr8:dst[1+1*stride] dst[3] dst[2] dst[1]
    S32ALNI(xr7,xr8,xr2,ptn1);     //xr7:dst[3] dst[2] dst[1] dst[0]
    S32STD(xr7,dst,0);
    
    S32ALNI(xr6,xr2,xr2,ptn1);     //xr6:dst[std] dst[2*std] dst[3*std] dst[0]
    S32ALNI(xr5,xr14,xr6,ptn3);     //xr5:dst[1+1*stride] dst[std] dst[2*std] dst[3*std]
  
    S32ALNI(xr4,xr7,xr5,ptn2);     //xr4:dst[1] dst[0] dst[1+1*stride] dst[std]
    S32STDV(xr4,dst,stride,0);     

    S32ALNI(xr3,xr9,xr15,ptn3);    //xr3:l1 l0 l1 l2
    S32ALNI(xr1,xr3,xr3 ,ptn3);    //xr1:l2 l1 l0 l1
    S32ALNI(xr1,xr15,xr1,ptn1);    //xr1:l1 l2 l3 l2
    
    D8SUMC(xr1,xr3,xr1);           
    Q16SAR(xr1,xr1,xr1,xr0,2);
    Q16SAT(xr1,xr1,xr0);           //xr1:dst[1+2*stride] dst[1+3*stride] 0 0
    S32ALNI(xr7,xr6,xr1,ptn1);     //xr7:dst[2*std] dst[3*std] dst[0] dst[1+2*stride]
    S32SFL(xr1,xr1,xr7,xr0,ptn0);  //xr1:dst[1+2*stride] dst[2*std] dst[1+3*stride] dst[3*std]
    S32ALNI(xr11,xr4,xr1,ptn2);  
    S32STDV(xr11,dst,2*stride,0);

    S32SFL(xr0,xr11,xr1,xr11,ptn3);
    S32STDV(xr11,dst,3*stride,0);   
    //pred4x4_horizontal_down_test(dst,stride,src_top+4,src_left+4);
    break;
  case DIAG_DOWN_LEFT_PRED_RV40_NODOWN:
    //l4= l5= l6= l7= l3;
    //pred4x4_down_left_rv40_nodown_test(dst,stride,src_top+4,src_left+4,dMB->rup[num]);
  case DIAG_DOWN_LEFT_PRED:
    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
      
      S8LDI(xr14,recon_tmp,20,0);
      S8LDI(xr14,recon_tmp,20,1);
      S8LDI(xr14,recon_tmp,20,2);
      S8LDI(xr14,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
      
      S8LDI(xr14,recon_tmp,12,0);
      S8LDI(xr14,recon_tmp,12,1);
      S8LDI(xr14,recon_tmp,12,2);
      S8LDI(xr14,recon_tmp,12,3);      
    }
    S32STD(xr15,src_left,0x4);
    S32STD(xr14,src_left,0x8);
    S32LDD(xr15,src_left,0x4);      //xr15:l3 l2 l1 l0        
    if(dMB->itype4[num] == DIAG_DOWN_LEFT_PRED_RV40_NODOWN)
      S8LDD (xr14,src_left,7,7);      
    else
      S32LDD(xr14,src_left,0x8);      //xr14:l7 l6 l5 l4
    S32ALNI(xr13,xr2,xr1,ptn2);     //xr13:t5 t4 t3 t2
    S32ALNI(xr12,xr2,xr1,ptn3);     //xr12:t4 t3 t2 t1
    S32ALNI(xr11,xr14,xr15,ptn2);   //xr11:l5 l4 l3 l2
    S32ALNI(xr10,xr14,xr15,ptn3);   //xr10:l4 l3 l2 l1

    S32SFL(xr9,xr13,xr12,xr8,ptn3); //xr9 :t5 t4 t4 t3 xr8:t3 t2 t2 t1
    S32SFL(xr7,xr12,xr1 ,xr6,ptn3); //xr7 :t4 t3 t3 t2 xr6:t2 t1 t1 t0

    S32SFL(xr5,xr11,xr10,xr4,ptn3); //xr5 :l5 l4 l4 l3 xr4 :l3 l2 l2 l1
    S32SFL(xr3,xr10,xr15,xr13,ptn3);//xr3 :l4 l3 l3 l2 xr13:l2 l1 l1 l0

    D8SUMC(xr12,xr9,xr7); //xr12:t5+t4+t4+t3+2  t4+t3+t3+t2+2
    D8SUMC(xr11,xr8,xr6); //xr11:t3+t2+t2+t1+2  t2+t1+t1+t0+2
    D8SUMC(xr10,xr5,xr3); //xr10:l5+l4+l4+l3+2  l4+l3+l3+l2+2
    D8SUMC(xr13 ,xr4,xr13);//xr9 :l3+l2+l2+l1+2  l2+l1+l1+l0+2

    Q16ADD_AA_WW(xr12,xr12,xr10,xr0); //
    Q16ADD_AA_WW(xr11,xr11,xr13 ,xr0); //

    Q16SAR(xr12,xr12,xr11,xr11,3);
    Q16SAT(xr12,xr12,xr11);
    S32STD(xr12,dst,0x0);
    
    S32ALNI(xr13,xr2,xr9,ptn1);    //xr13:t6 t5 t4 t5
    S32ALNI(xr11,xr14,xr5,ptn1);   //xr11:l6 l5 l4 l5
    S32SFL(xr10,xr2,xr13,xr0,ptn3);   //xr10:t7 t6 t6 t5
    S32SFL(xr6 ,xr14,xr11,xr0,ptn3);  //xr6 :l7 l6 l6 l5
    
    D8SUMC(xr13,xr13,xr10);
    D8SUMC(xr10,xr11,xr6);

    Q16ADD_AA_WW(xr11,xr13,xr10,xr0);
    Q16SAR(xr11,xr11,xr0,xr0,3);
    Q16SAT(xr11,xr11,xr0);           //xr11:dst[3+1*strd] dst[3+2*strd] 0 0
    S32ALNI(xr9,xr11,xr11,ptn1);     //xr9 :dst[3+2*strd] 0 0 dst[3+1*strd]
    S32ALNI(xr12,xr9, xr12,ptn3);     //
    S32STDV(xr12,dst,stride,0);

    S32ALNI(xr11,xr9,xr9,ptn1);      //xr11:0 0 dst[3+1*strd] dst[3+2*strd]
    S32ALNI(xr12,xr11,xr12,ptn3);
    S32STDV(xr12,dst,2*stride,0);

    S32SFL(xr13,xr14,xr2,xr0,ptn3); //xr13:l7 l6 t7 t6
    D8SUMC(xr13,xr13,xr0);
    Q16SAR(xr13,xr13,xr0,xr0,2);
    Q16SAT(xr13,xr13,xr0);
    S32ALNI(xr13,xr13,xr13,ptn1);
    S32ALNI(xr12,xr13,xr12,ptn3);
    S32STDV(xr12,dst,3*stride,0);
    //pred4x4_down_left_rv40_test(dst,stride,src_top+4,src_left+4,dMB->rup[num]);
    break;
  case HOR_UP_PRED_RV40_NODOWN:
    //l4= l5= l6= l7= l3;
    //pred4x4_horizontal_up_rv40_nodown_test(dst,stride,src_top+4,src_left+4,dMB->rup[num]);
  case HOR_UP_PRED:
    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
      
      S8LDI(xr14,recon_tmp,20,0);
      S8LDI(xr14,recon_tmp,20,1);
      S8LDI(xr14,recon_tmp,20,2);
      S8LDI(xr14,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
      
      S8LDI(xr14,recon_tmp,12,0);
      S8LDI(xr14,recon_tmp,12,1);
      S8LDI(xr14,recon_tmp,12,2);
      S8LDI(xr14,recon_tmp,12,3);      
    }
    S32STD(xr15,src_left,0x4);
    S32STD(xr14,src_left,0x8);
    S32LDD(xr15,src_left,0x4);      //xr15:l3 l2 l1 l0        
    if(dMB->itype4[num] == HOR_UP_PRED_RV40_NODOWN)
      S8LDD (xr14,src_left,7,7);      
    else
      S32LDD(xr14,src_left,0x8);      //xr14:l7 l6 l5 l4
    S32ALNI(xr13,xr2,xr1,ptn1);     //xr13:t6 t5 t4 t3
    S32ALNI(xr4,xr2,xr1,ptn2);     //xr12:t5 t4 t3 t2
    S32ALNI(xr11,xr2,xr1,ptn3);     //xr11:t4 t3 t2 t1

    S32SFL(xr13,xr13,xr4,xr12,ptn3); //xr13 :t6 t5 t5 t4 xr12:t4 t3 t3 t2
    S32SFL(xr11,xr4,xr11,xr4,ptn3);  //xr11 :t5 t4 t4 t3 xr4: t3 t2 t2 t1

    S32ALNI(xr10,xr15,xr15,ptn1);   //xr10:l2 l1 l0 l3
    S32SFL (xr9,xr10,xr15,xr8,ptn3);     //xr9 :l2 l1 l3 l2// xr8:l0 l3 l1 l0
    S32SFL (xr7,xr10,xr9,xr0,ptn3); //xr7 :l2 l1 l2 l1//
    S32ALNI(xr6,xr8,xr7,ptn2);      //xr6 :l1 l0 l2 l1//
    S32ALNI(xr5,xr8,xr6,ptn2);      //xr5 :l1 l0 l1 l0//
    D8SUMC(xr10,xr13,xr11);  //xr10:t6+t5+t5+t4+2 t5+t4+t4+t3+2
    D8SUMC(xr8 ,xr9 ,xr7);   //xr8 :l2+l1+l3+l2+2 l2+l1+l2+l1+2

    D8SUMC(xr3 ,xr12,xr4);   //xr3 :t4+t3+t3+t2+2 t3+t2+t2+t1+2
    D8SUMC(xr4 ,xr6 ,xr5);   //xr4 :l1+l0+l2+l1+2 l1+l0+l1+l0+2
    
    Q16ACCM_AA(xr10,xr8,xr3,xr4);
    Q16SAR(xr10,xr10,xr4,xr4,3);
    Q16SAT(xr10,xr10,xr4);
    S32STD(xr10,dst,0x0);

    S8LDD  (xr8 ,src_left,7,7);   //xr8 :l3 l3 l3 l3
    S32ALNI(xr4,xr9,xr15,ptn2);   //xr4 :l3 l2 l3 l2
    S32ALNI(xr5,xr4,xr8,ptn3);    //xr5 :l2 l3 l3 l3
    S32ALNI(xr6,xr2,xr2,ptn2);    //xr6 :t5 t4 t7 t6
    S32ALNI(xr8,xr6,xr13,ptn2);   //xr8 :t7 t6 t6 t5

    S8LDD  (xr3, &t7,0,7);        //xr3 :t7 t7 t7 t7
    S32ALNI(xr3,xr6,xr3,ptn3);    //xr3 :t6 t7 t7 t7
    D8SUMC (xr3,xr3,xr8);
    D8SUMC (xr5,xr5,xr4);
    Q16ADD_AA_WW(xr3,xr3,xr5,xr0);
    Q16SAR(xr3,xr3,xr0,xr0,3);
    Q16SAT(xr3,xr0,xr3);
    S32ALNI(xr10,xr3,xr10,ptn2);
    S32STDV(xr10,dst,stride,0x0);    

    S32ALNI(xr8,xr14,xr15,ptn1);  //xr8:l6 l5 l4 l3
    S32ALNI(xr4,xr14,xr8,ptn2);   //xr4:l5 l4 l6 l5
    S32ALNI(xr5,xr8 ,xr4,ptn2);   //xr5:l4 l3 l5 l4
    S32ALNI(xr9,xr8 ,xr2,ptn2);   //xr9:l4 l3 t7 t6
    Q8AVGR(xr13,xr4,xr5);         //xr13:dst[2+3*strd] 1 1 1

    D8SUMC(xr5,xr5,xr9);
    D8SUMC(xr4,xr4,xr0);
    Q16SAR(xr5,xr5,xr4,xr4,2);
    Q16SAT(xr5,xr4,xr5);

    S32ALNI(xr10,xr5,xr10,ptn2);
    S32STDV(xr10,dst,2*stride,0x0); 
    S32SFL(xr8,xr5,xr13,xr0,ptn0);     
    S32SFL (xr10,xr8,xr10,xr0,ptn3);
    S32STDV(xr10,dst,3*stride,0x0); 
    //pred4x4_horizontal_up_rv40_test(dst,stride,src_top+4,src_left+4,dMB->rup[num]);
    break;
  case VERT_LEFT_PRED_RV40_NODOWN:
    //l4=l3;
    //pred4x4_vertical_left_rv40_nodown_test(dst,stride,src_top+4,src_left+4,dMB->rup[num]);
  case VERT_LEFT_PRED:
    if(num<16){    
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,20,1);
      S8LDI(xr15,recon_tmp,20,2);
      S8LDI(xr15,recon_tmp,20,3);
      
      S8LDI(xr14,recon_tmp,20,0);
      S8LDI(xr14,recon_tmp,20,1);
      S8LDI(xr14,recon_tmp,20,2);
      S8LDI(xr14,recon_tmp,20,3);
    }
    else{
      S8LDD(xr15,recon_tmp, 0,0);
      S8LDI(xr15,recon_tmp,12,1);
      S8LDI(xr15,recon_tmp,12,2);
      S8LDI(xr15,recon_tmp,12,3);
      
      S8LDI(xr14,recon_tmp,12,0);
      S8LDI(xr14,recon_tmp,12,1);
      S8LDI(xr14,recon_tmp,12,2);
      S8LDI(xr14,recon_tmp,12,3);      
    }
    S32STD(xr15,src_left,0x4);
    S32STD(xr14,src_left,0x8);
    S32ALNI(xr3,xr1,xr1,ptn1); //xr3: t2 t1 t0 t3
    S32ALNI(xr4,xr2,xr2,ptn3); //xr4: t4 t7 t6 t5
    
    S32ALNI(xr5,xr2,xr1,ptn3); //xr5: t4 t3 t2 t1
    S32ALNI(xr6,xr4,xr5,ptn3); //xr6: t5 t4 t3 t2
        
    Q8AVGR(xr7,xr5,xr6);
    S32STDV(xr7,dst,2*stride,0);

    S32ALNI(xr8,xr4,xr4,ptn3); //xr8: t5 t4 t7 t6
    S32ALNI(xr9,xr8,xr6,ptn3); //xr9: t6 t5 t4 t3           

    S32SFL(xr11,xr9,xr6,xr13,ptn3);//xr11:t6 t5 t5 t4 //xr13:t4 t3 t3 t2
    S32SFL(xr12,xr6,xr5,xr10,ptn3);//xr12:t5 t4 t4 t3 //xr10:t3 t2 t2 t1
    D8SUMC(xr14,xr13,xr10);        //xr14:
    D8SUMC(xr15,xr11,xr12);        //xr15:
    Q16SAR(xr15,xr15,xr14,xr14,2);
    Q16SAT(xr15,xr15,xr14);        //xr15:
    S32STDV(xr15,dst,3*stride,0);

    S32SFL(xr0, xr1,xr1,xr1,ptn3);  //xr1 :t1 t0 t1 t0//
    S32SFL(xr0, xr10,xr1,xr10,ptn3);//xr10:t2 t1 t1 t0//
  
    S32LDD(xr11,src_left,0x4);      //xr11:l3 l2 l1 l0
    S32LDD(xr12,src_left,0x8);      //xr12:l7 l6 l5 l4
    if(dMB->itype4[num] == VERT_LEFT_PRED_RV40_NODOWN)
      {
	S32ALNI(xr12,xr12,xr12,ptn3);    //xr3 :l4 l7 l6 l5
	S32ALNI(xr12,xr12,xr11,ptn1);     //xr3 :l7 l6 l5 l3
      }
    S32ALNI(xr13,xr12,xr11,ptn3);   //xr13:l4 l3 l2 l1
    S32SFL(xr14,xr13,xr11,xr0,ptn3);//xr14:l4 l3 l3 l2 //
    S32SFL(xr0 ,xr14,xr13,xr13,ptn3);//xr13:l3 l2 l2 l1 //
    D8SUMC(xr11,xr1,xr10);           //xr11:2*t0+2*t1+2 t0+2*t1+t2+2 
    D8SUMC(xr12,xr13,xr14);          //xr12:l1+2*l2+l3+2 l2+2*l3+l4+2 
    Q16ADD_AA_WW(xr11,xr11,xr12,xr0);//xr11:src[0] src[1*stride]
    Q16SAR(xr11,xr11,xr0,xr0,3);     
    Q16SAT(xr11,xr11,xr0);           //xr11:src[0] src[1*stride] 0 0
     
    S32ALNI(xr10,xr7,xr11,ptn1);     
    S32STD(xr10,dst,0);

    S32ALNI(xr12,xr11,xr11,ptn1);   //xr12: src[1*stride] 0 0 src[0]  
    S32ALNI(xr10,xr15,xr12,ptn1);   //xr10:    
    S32STDV(xr10,dst,stride,0);
    //pred4x4_vertical_left_rv40_test(dst,stride,src_top+4,src_left+4,dMB->rup[num]);
    break;
  }
}

void rv40_pred_8x8_aux(uint8_t *dst,int itype8,uint8_t *src_left,uint8_t *src_top){
  int i,dc,dcl,dct,H=0,V=0;
  int n = 4;
  dst-=PREVIOUS_CHROMA_STRIDE;
  switch(itype8){
  case DC_128_PRED8x8:
    //dc = 128;
    //S8LDD(xr5,&dc,0,7);
    S32I2M(xr5,0x80808080);
    for(i=0; i<8; i++){
      S32SDI(xr5,dst,PREVIOUS_CHROMA_STRIDE);
      S32STD(xr5,dst,0x4);
    }
    break;
    //pred8x8_128_dc_test(dst,PREVIOUS_CHROMA_STRIDE);
  case LEFT_DC_PRED8x8:
    S16LDD(xr5,&n,0,3);
    D32ADD_AA(xr2,xr0,xr0,xr4);
    S32LDD(xr1,src_left,0x4);
    Q8SAD(xr0,xr1,xr0,xr2);
    S32LDD(xr1,src_left,0x8);
    Q8SAD(xr0,xr1,xr0,xr2);
    S32SFL(xr0,xr2,xr2,xr6,ptn3);
    Q16ADD_AA_WW(xr5,xr5,xr6,xr6);
    Q16SAR(xr5,xr5,xr6,xr6,3);
    Q16SAT(xr5,xr5,xr6);
    for(i=0; i<8; i++){
      S32SDI(xr5,dst,PREVIOUS_CHROMA_STRIDE);
      S32STD(xr5,dst,0x4);
    }
    break;
    //pred8x8_left_dc_rv40_test(dst,PREVIOUS_CHROMA_STRIDE,src_left+4);
  case TOP_DC_PRED8x8:
    S16LDD(xr5,&n,0,3);
    D32ADD_AA(xr2,xr0,xr0,xr4);
    S32LDD(xr1,src_top,0x4);
    Q8SAD(xr0,xr1,xr0,xr2);
    S32LDD(xr1,src_top,0x8);
    Q8SAD(xr0,xr1,xr0,xr2);
    S32SFL(xr0,xr2,xr2,xr6,ptn3);
    Q16ADD_AA_WW(xr5,xr5,xr6,xr6);
    Q16SAR(xr5,xr5,xr6,xr6,3);
    Q16SAT(xr5,xr5,xr6);
    for(i=0; i<8; i++){
      S32SDI(xr5,dst,PREVIOUS_CHROMA_STRIDE);
      S32STD(xr5,dst,0x4);
    }
    break;
    //pred8x8_top_dc_rv40_test(dst,PREVIOUS_CHROMA_STRIDE,src_top+4);
  case DC_PRED8x8:
    //dc= 0x01010101*((dcl + dct + 8)>>4);
    n = 8;
    D32ADD_AA(xr2,xr0,xr0,xr4);
    S32LDD(xr1,src_left,0x4);
    Q8SAD(xr0,xr1,xr0,xr2);
    S32LDD(xr1,src_left,0x8);
    Q8SAD(xr0,xr1,xr0,xr2);

    S32LDD(xr1,src_top,0x4);
    Q8SAD(xr0,xr1,xr0,xr2);
    S32LDD(xr1,src_top,0x8);
    Q8SAD(xr0,xr1,xr0,xr2);
    S16LDD(xr5,&n,0,3);

    S32SFL(xr0,xr2,xr2,xr6,ptn3);
    Q16ADD_AA_WW(xr5,xr5,xr6,xr6);
    Q16SAR(xr5,xr5,xr6,xr6,4);
    Q16SAT(xr5,xr5,xr6);
    for(i=0; i<8; i++){
      S32SDI(xr5,dst,PREVIOUS_CHROMA_STRIDE);
      S32STD(xr5,dst,0x4);
    }
    //pred8x8_dc_rv40_test(dst,PREVIOUS_CHROMA_STRIDE,src_top+4, src_left+4);
    break;
  case VERT_PRED8x8:
    S32LDD(xr1,src_top,0x4);
    S32LDD(xr2,src_top,0x8);
    for(i=0; i<8; i++){/*
			 ((uint32_t*)(dst+i*PREVIOUS_CHROMA_STRIDE))[0]=((uint32_t*)(src_top))[1];
			 ((uint32_t*)(dst+i*PREVIOUS_CHROMA_STRIDE))[1]=((uint32_t*)(src_top))[2];
		       */
      S32SDI(xr1,dst,PREVIOUS_CHROMA_STRIDE);
      S32STD(xr2,dst,0x4);
    }
    //pred8x8_vertical_test(dst,PREVIOUS_CHROMA_STRIDE,src_top+4);
    break;
  case HOR_PRED8x8:
    src_left+=3;
    for(i=0; i<8; i++){/*
			 ((uint32_t*)(dst+i*PREVIOUS_CHROMA_STRIDE))[0]=
			 ((uint32_t*)(dst+i*PREVIOUS_CHROMA_STRIDE))[1]= src_left[i+4]*0x01010101;
		       */
      S8LDI(xr1,src_left,1,0x7);
      S32SDI(xr1,dst,PREVIOUS_CHROMA_STRIDE);
      S32STD(xr1,dst,0x4);
    }
    //pred8x8_horizontal_test(dst,PREVIOUS_CHROMA_STRIDE,src_left+4);
    break;
  case PLANE_PRED8x8:
    for(i=1;i<4;i++){
      H+=i*(src_top[7+i] - src_top[7-i]);
      V+=i*(src_left[7+i] - src_left[7-i]);
    }
    H+=i*(src_top[7+i] - src_left[3]);
    V+=i*(src_left[7+i] - src_left[3]);

    H = ( 17*H+16 ) >> 5;
    V = ( 17*V+16 ) >> 5;
    dc=16*(src_left[11] + src_top[11] + 1) - 3*(V+H);
    S32I2M(xr15,H);
    for(i=8; i>0; --i) {
      S32I2M(xr1,dc);
      //S32I2M(xr2,dc+H);
      //S32I2M(xr3,dc+2*H);
      //S32I2M(xr4,dc+3*H);
      //S32I2M(xr5,dc+4*H);
      //S32I2M(xr6,dc+5*H);
      //S32I2M(xr7,dc+6*H);
      //S32I2M(xr8,dc+7*H);
      dst += PREVIOUS_CHROMA_STRIDE;
      D32ADD_AA(xr2,xr1,xr15,xr3);
      D32ADD_AA(xr3,xr3,xr15,xr4);
      D32ADD_AA(xr4,xr4,xr15,xr5);
      D32ADD_AA(xr5,xr5,xr15,xr6);
      D32ADD_AA(xr6,xr6,xr15,xr7);
      D32ADD_AA(xr7,xr7,xr15,xr8);
      D32ADD_AA(xr8,xr8,xr15,xr0);
      D32SARL(xr9,xr2,xr1,5);
      D32SARL(xr10,xr4,xr3,5);
      D32SARL(xr11,xr6,xr5,5);
      D32SARL(xr12,xr8,xr7,5);
      Q16SAT(xr1,xr10,xr9);
      Q16SAT(xr2,xr12,xr11);
      S32STD(xr1,dst,0x0);
      S32STD(xr2,dst,0x4);
      dc+=V;
    }
    //pred8x8_plane_test(dst,PREVIOUS_CHROMA_STRIDE,src_top+4, src_left+4);
    break;
  }
}

void rv40_output_macroblock_aux(RV9_MB_DecARGs *dMB, uint8_t *recon_ptr, uint8_t *intra_top_y, uint8_t *intra_top_u, uint8_t *intra_top_v, uint8_t *recon, DCTELEM *idct, uint8_t *topleft)
{
  int i, j, k,l,dc,dcl,dct;
  uint8_t *Y, *U, *V;
  int cbp = dMB->cbp;
  int num;

  int intra16x16_left[5];
  uint8_t *src_left = intra16x16_left;

  // Set neighboudMB infodMBmation.
  Y = recon_ptr;
  U = recon_ptr + PREVIOUS_OFFSET_U;
  V = recon_ptr + PREVIOUS_OFFSET_V;
  
  if(!dMB->is16){
    for(i = 0; i < 16; i++, cbp >>= 1){  
      rv40_pred_4x4_block_aux(dMB, Y + offset_y[i], PREVIOUS_LUMA_STRIDE,i, intra_top_y, recon, topleft[0]);  
      if(cbp & 1)      
	rv40_add_4x4_block(Y+offset_y[i], PREVIOUS_LUMA_STRIDE, idct+32*i, 0);
    }

    for(i = 0; i < 4; i++, cbp >>= 1){
      rv40_pred_4x4_block_aux(dMB, U + offset_c[i], PREVIOUS_CHROMA_STRIDE,16 + i,intra_top_u,recon+PREVIOUS_OFFSET_U,topleft[1]);
      rv40_pred_4x4_block_aux(dMB, V + offset_c[i], PREVIOUS_CHROMA_STRIDE,16 + i,intra_top_v,recon+PREVIOUS_OFFSET_V,topleft[2]);
      
      if(cbp & 0x01)
	rv40_add_4x4_block(U + offset_c[i], PREVIOUS_CHROMA_STRIDE, idct+512, i*32);
      if(cbp & 0x10)
	rv40_add_4x4_block(V + offset_c[i], PREVIOUS_CHROMA_STRIDE, idct+640, i*32);
    }
  }  
  else{
    int H=0,Ver=0;
    int n = 16;
    uint8_t *dst,*recon_tmp;
#if 0
    for(i=0; i<16; i++){
      src_left[4+i] = recon[i*PREVIOUS_LUMA_STRIDE+15];  //left
    }
#else
    recon_tmp = recon+15;
    S8LDD(xr15,recon_tmp, 0,0);
    S8LDI(xr15,recon_tmp,20,1);
    S8LDI(xr15,recon_tmp,20,2);
    S8LDI(xr15,recon_tmp,20,3);

    S8LDI(xr14,recon_tmp,20,0);
    S8LDI(xr14,recon_tmp,20,1);
    S8LDI(xr14,recon_tmp,20,2);
    S8LDI(xr14,recon_tmp,20,3);

    S8LDI(xr13,recon_tmp,20,0);
    S8LDI(xr13,recon_tmp,20,1);
    S8LDI(xr13,recon_tmp,20,2);
    S8LDI(xr13,recon_tmp,20,3);

    S8LDI(xr12,recon_tmp,20,0);
    S8LDI(xr12,recon_tmp,20,1);
    S8LDI(xr12,recon_tmp,20,2);
    S8LDI(xr12,recon_tmp,20,3);
    S32STD(xr15,src_left,0x4);
    S32STD(xr14,src_left,0x8);    
    S32STD(xr13,src_left,0xc);    
    S32STD(xr12,src_left,0x10);    

#endif
    uint8_t *top_y = intra_top_y - 4;
    dst=Y-PREVIOUS_LUMA_STRIDE;
    switch(dMB->itype4[0]){
    case DC_PRED8x8:
      D32ADD_AA(xr2,xr0,xr0,xr4);
      for(i=0; i<4; i++){
	S32LDI(xr1,src_left,0x4);
	Q8SAD(xr0,xr1,xr0,xr2);
      }
      for(i=0; i<4; i++){
	S32LDI(xr1,top_y,0x4);
	Q8SAD(xr0,xr1,xr0,xr2);
      }      
      S16LDD(xr5,&n,0,3);      
      S32SFL(xr0,xr2,xr2,xr6,ptn3);
      Q16ADD_AA_WW(xr5,xr5,xr6,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,5);
      Q16SAT(xr5,xr5,xr6);      
      for(i=0; i<16; i++){/*
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[0]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[1]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[2]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[3]= dc;
			  */
	S32SDI(xr5,dst,PREVIOUS_LUMA_STRIDE);
	S32STD(xr5,dst,0x4);
	S32STD(xr5,dst,0x8);
	S32STD(xr5,dst,0xc);
      }
      //pred16x16_128_dc_test(Y,dSlice->linesize);//
      break;
      //pred16x16_dc_test(Y,dSlice->linesize,src_top + 4,src_left + 4);  //
    case LEFT_DC_PRED8x8:
      n = 8;
      D32ADD_AA(xr2,xr0,xr0,xr4);
      for(i=0; i<4; i++){
	S32LDI(xr1,src_left,0x4);
	Q8SAD(xr0,xr1,xr0,xr2);
      }
      S16LDD(xr5,&n,0,3);      
      S32SFL(xr0,xr2,xr2,xr6,ptn3);
      Q16ADD_AA_WW(xr5,xr5,xr6,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,4);
      Q16SAT(xr5,xr5,xr6);      
      for(i=0; i<16; i++){/*
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[0]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[1]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[2]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[3]= dc;
			  */
	S32SDI(xr5,dst,PREVIOUS_LUMA_STRIDE);
	S32STD(xr5,dst,0x4);
	S32STD(xr5,dst,0x8);
	S32STD(xr5,dst,0xc);
      }
      break;
      //pred16x16_left_dc_test(Y,dSlice->linesize,src_left+4);//
    case TOP_DC_PRED8x8:
      n = 8;
      D32ADD_AA(xr2,xr0,xr0,xr4);
      for(i=0; i<4; i++){
	S32LDI(xr1,top_y,0x4);
	Q8SAD(xr0,xr1,xr0,xr2);
      }
      S16LDD(xr5,&n,0,3);      
      S32SFL(xr0,xr2,xr2,xr6,ptn3);
      Q16ADD_AA_WW(xr5,xr5,xr6,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,4);
      Q16SAT(xr5,xr5,xr6);      
      for(i=0; i<16; i++){/*
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[0]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[1]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[2]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[3]= dc;
			  */
	S32SDI(xr5,dst,PREVIOUS_LUMA_STRIDE);
	S32STD(xr5,dst,0x4);
	S32STD(xr5,dst,0x8);
	S32STD(xr5,dst,0xc);
      }
      break;
      //pred16x16_top_dc_test(Y,dSlice->linesize,src_top+4);//
    case DC_128_PRED8x8:
      //dc=128U;
      //dc*=0x01010101;
      S32I2M(xr1,0x80808080);
      //S8LDD(xr1,&dc,0,7);
      for(i=0; i<16; i++){/*
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[0]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[1]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[2]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[3]= dc;
			  */
	S32SDI(xr1,dst,PREVIOUS_LUMA_STRIDE);
	S32STD(xr1,dst,0x4);
	S32STD(xr1,dst,0x8);
	S32STD(xr1,dst,0xc);
      }
      //pred16x16_128_dc_test(Y,dSlice->linesize);//
      break;
    case VERT_PRED8x8:
      S32LDD(xr1,top_y,0x4);
      S32LDD(xr2,top_y,0x8);
      S32LDD(xr3,top_y,0xc);
      S32LDD(xr4,top_y,0x10);
      for(i=0; i<16; i++){/*
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[0]=((uint32_t*)(src_top+4))[0];
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[1]=((uint32_t*)(src_top+4))[1];
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[2]=((uint32_t*)(src_top+4))[2];
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[3]=((uint32_t*)(src_top+4))[3];
			  */
	S32SDI(xr1,dst,PREVIOUS_LUMA_STRIDE);
	S32STD(xr2,dst,0x4);
	S32STD(xr3,dst,0x8);
	S32STD(xr4,dst,0xc);
      }
      //pred16x16_vertical_test(Y,dSlice->linesize,src_top + 4); //
      break;
    case HOR_PRED8x8:
      src_left+=3;
      for(i=0; i<16; i++){/*
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[0]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[1]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[2]=
			    ((uint32_t*)(Y+i*PREVIOUS_LUMA_STRIDE))[3]= src_left[i+4]*0x01010101;
			  */
	S8LDI(xr1,src_left,1,0x7);
	S32SDI(xr1,dst,PREVIOUS_LUMA_STRIDE);
	S32STD(xr1,dst,0x4);
	S32STD(xr1,dst,0x8);
	S32STD(xr1,dst,0xc);
      }
      //pred16x16_horizontal_test(Y,dSlice->linesize,src_left+4);//
      break;
    case PLANE_PRED8x8:
#if 1
      for(i=1;i<8;i++){
	H+=i*(top_y[11+i] - top_y[11-i]);
	Ver+=i*(src_left[11+i] - src_left[11-i]);
      }
      H+=i*(top_y[19] - topleft[0]);
      Ver+=i*(src_left[19] - topleft[0]);

      H = ( H + (H>>2) ) >> 4;
      Ver = ( Ver + (Ver>>2) ) >> 4;
      dc = 16*(src_left[19] + top_y[19] + 1) - 7*(Ver+H);
      S32I2M(xr11,H);
      S32I2M(xr13,dc);
      D32SLL(xr14,xr11,xr0,xr0,0x2);
      dst = Y - 4;
      D32ADD_SS(xr13,xr13,xr14,xr15);
      S32I2M(xr12,Ver);
      for(j=16; j>0; --j) {
	for(i=0; i<4; i++){
	  //S32I2M(xr1,dc+(i<<2)*H);
	  //S32I2M(xr2,dc+(i<<2)*H+H);
	  //S32I2M(xr3,dc+(i<<2)*H+2*H);
	  //S32I2M(xr4,dc+(i<<2)*H+3*H);
	  D32ADD_AA(xr1,xr13,xr14,xr13);
	  D32ADD_AA(xr2,xr1,xr11,xr0);
	  D32ADD_AA(xr3,xr2,xr11,xr0);
	  D32ADD_AA(xr4,xr3,xr11,xr0);
	  D32SARL(xr5,xr2,xr1,5);
	  D32SARL(xr6,xr4,xr3,5);
	  Q16SAT(xr1,xr6,xr5);
	  S32SDI(xr1,dst,0x4);
	}
	//dc += Ver;
	D32ADD_AA(xr13,xr15,xr12,xr15);
	dst += PREVIOUS_LUMA_STRIDE-16;
      }
      //pred16x16_plane_compat_test(Y,dSlice->linesize,src_top + 4,src_left + 4);
#endif
      break;
    }    
    for(i = 0; i < 16; i++, cbp >>= 1){    
      // if(cbp & 1)      
      rv40_add_4x4_block(Y+offset_y[i], PREVIOUS_LUMA_STRIDE, idct+32*i, 0);
    }
    src_left = intra16x16_left;
    uint8_t *urecon = recon + PREVIOUS_OFFSET_U;
    uint8_t *vrecon = recon + PREVIOUS_OFFSET_V;
#if 0
    for(i=0; i<8; i++){
      src_left[4+i] = urecon[i*PREVIOUS_CHROMA_STRIDE+7];  //left
    }
#else
    recon_tmp = urecon+7;
    S8LDD(xr15,recon_tmp, 0,0);
    S8LDI(xr15,recon_tmp,12,1);
    S8LDI(xr15,recon_tmp,12,2);
    S8LDI(xr15,recon_tmp,12,3);

    S8LDI(xr14,recon_tmp,12,0);
    S8LDI(xr14,recon_tmp,12,1);
    S8LDI(xr14,recon_tmp,12,2);
    S8LDI(xr14,recon_tmp,12,3);
    S32STD(xr15,src_left,0x4);
    S32STD(xr14,src_left,0x8);    
#endif

    src_left[3] = topleft[1];
    uint8_t *top_u = intra_top_u - 4;

    rv40_pred_8x8_aux(U,dMB->itype4[1],src_left,top_u);
    //add_pixels_clamped_aux(idct+512,32+16, U, PREVIOUS_CHROMA_STRIDE);

    for(i=0; i<8; i++){
      src_left[4+i] = vrecon[i*PREVIOUS_CHROMA_STRIDE+7];  //left
    }
    src_left[3] = topleft[2];
    uint8_t *top_v = intra_top_v - 4;
    rv40_pred_8x8_aux(V,dMB->itype4[1],src_left,top_v);
    //add_pixels_clamped_aux(idct+640,32+16, V, PREVIOUS_CHROMA_STRIDE);  
    for(i = 0; i < 4; i++, cbp >>= 1){
      if(cbp & 0x01)
	rv40_add_4x4_block(U+offset_c[i], PREVIOUS_CHROMA_STRIDE,idct+512,i*32);
      if(cbp & 0x10)
	rv40_add_4x4_block(V+offset_c[i], PREVIOUS_CHROMA_STRIDE,idct+640,i*32);
    }     
  }
}

void rv40_apply_differences(RV9_MB_DecARGs *dMB, DCTELEM *idct, uint8_t *recon_ptr)
{
  int i,j,num;
  uint8_t *Y, *U, *V;
  int cbp = dMB->cbp;
  Y = recon_ptr;
  U = recon_ptr + PREVIOUS_OFFSET_U;
  V = recon_ptr + PREVIOUS_OFFSET_V; 
  if(dMB->is16) cbp|=0xffff;
  for(i = 0; i < 16; i++, cbp >>= 1){    
    if(cbp & 1)      
      rv40_add_4x4_block(Y+offset_y[i], PREVIOUS_LUMA_STRIDE, idct+32*i, 0);
  }
      
  for(i = 0; i < 4; i++, cbp >>= 1){
    if(cbp & 0x01)
      rv40_add_4x4_block(U+offset_c[i], PREVIOUS_CHROMA_STRIDE, idct+512, i*32);
    if(cbp & 0x10)
      rv40_add_4x4_block(V+offset_c[i], PREVIOUS_CHROMA_STRIDE, idct+640, i*32);
  }   
}

