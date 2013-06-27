/**************************************************
Deblock
**************************************************/
#include "jzm_x264_enc.h"
extern volatile unsigned char * sde_base;
extern volatile unsigned char * vpu_base;
#define write_vpu_reg(off, value) (*((volatile unsigned int *)(off)) = (value))
#define read_vpu_reg(off, ofst)   (*((volatile unsigned int *)(off + ofst)))

int comp_block_c(uint8_t *ref_ptr, uint8_t *dut_ptr, int w, int h, int ref_str, int dut_str)
{
    int i, j;
    for ( i = 0 ; i<h; i++)
        for ( j = 0 ; j<w; j++){
            if ( ref_ptr[i*ref_str+j] != dut_ptr[i*dut_str+j] )
                return 0;
        }
    return 1;
}
void show_block_c(uint8_t *ref_ptr, uint8_t *dut_ptr, int w, int h, int ref_str, int dut_str)
{
    int i, j;
    for ( i = 0 ; i<h; i++){
        for ( j = 0 ; j<w; j++){
            if ( ref_ptr[i*ref_str+j] != dut_ptr[i*dut_str+j] )
                printf("%02x+%02x,", ref_ptr[i*ref_str+j], dut_ptr[i*dut_str+j]);
            else
                printf("%02x-%02x,", ref_ptr[i*ref_str+j], dut_ptr[i*dut_str+j]);
        }
        printf("\n");
    }
}

void comp_mb_c(x264_t *h, int mb_x, int mb_y){
    int stridey   = h->fdec->i_stride[0];
    int strideuv  = h->fdec->i_stride[1];
    unsigned char * ref_y  = h->fdec->plane[0] + (mb_y * 16* stridey  ) + mb_x * 16;
    unsigned char * ref_cb = h->fdec->plane[1] + (mb_y * 8 * strideuv) + mb_x * 8;
    unsigned char * ref_cr = h->fdec->plane[2] + (mb_y * 8 * strideuv) + mb_x * 8;
    
    int mb_y_1 = mb_y -1;
    unsigned char * up_ref_y  = h->fdec->plane[0] + (mb_y_1 * 16* stridey  ) + mb_x * 16;
    unsigned char * up_ref_cb = h->fdec->plane[1] + (mb_y_1 * 8 * strideuv) + mb_x * 8;
    unsigned char * up_ref_cr = h->fdec->plane[2] + (mb_y_1 * 8 * strideuv) + mb_x * 8;
    
    int dut_str_y = (h->sps->i_mb_width*256 + 2*256);
    int dut_str_uv = (h->sps->i_mb_width*128 + 2*128);
    
    int jstic_mbx = mb_x;
    int jstic_mby = mb_y;
    int lp_x = mb_x;
    int lp_y = mb_y-1;

    HwInfo_t * hwinfo = h->hwinfo;
    uint8_t * dec_dst_y = (uint8_t *)hwinfo->fb_ptr[0][0];
    uint8_t * dec_dst_uv = (uint8_t *)hwinfo->fb_ptr[0][1];
    
    uint8_t * dut_y  = dec_dst_y + (jstic_mby * dut_str_y ) + jstic_mbx * 256;
    uint8_t * dut_cb = dec_dst_uv + (jstic_mby * dut_str_uv ) + jstic_mbx * 128;
    uint8_t * dut_cr = dec_dst_uv + (jstic_mby * dut_str_uv ) + jstic_mbx * 128 + 8;
    
    uint8_t * up_dut_y  = dec_dst_y + (lp_y * dut_str_y ) + lp_x * 256;
    uint8_t * up_dut_cb = dec_dst_uv + (lp_y * dut_str_uv ) + lp_x * 128; 
    uint8_t * up_dut_cr = dec_dst_uv + (lp_y * dut_str_uv ) + lp_x * 128 + 8;
  
    int body_err = 0;
    int ret = comp_block_c(ref_y , dut_y , 16 ,12 , stridey, 16);
    if(!ret){
        printf("[DBLK] Y body error! <%08x>\n", (int)dut_y);
        show_block_c(ref_y , dut_y , 16 ,12 , stridey, 16);
    }
    body_err |= !ret;
    if(mb_y!=0){
        ret = comp_block_c(up_ref_y , up_dut_y , 16 ,16 , stridey, 16);
        if(!ret){
            printf("[DBLK] Y UP error! <%08x>\n", (int)(up_dut_y+12*16) );
            show_block_c(up_ref_y , up_dut_y , 16 ,16 , stridey, 16);
        }
    }
    
    body_err |= !ret;
    ret = comp_block_c(ref_cb , dut_cb , 8 ,4 , strideuv, 16);
    if(!ret){
        printf("[DBLK] U body error! <%08x>\n", (int)dut_cb);
        show_block_c(ref_cb , dut_cb , 8 ,4 , strideuv, 16);
    }
    body_err |= !ret;
    if(mb_y!=0){
        ret = comp_block_c(up_ref_cb , up_dut_cb , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] U UP error! <%08x>\n", (int)(up_dut_cb+4*16) );
            show_block_c(up_ref_cb , up_dut_cb , 8 ,8 , strideuv, 16);
        }
    }
    body_err |= !ret;
    
    ret = comp_block_c(ref_cr , dut_cr , 8 ,4 , strideuv, 16);
    if(!ret){
        printf("[DBLK] V body error! <%08x>\n", (int)dut_cr);
        show_block_c(ref_cr , dut_cr , 8 ,4 , strideuv, 16);
    }
    body_err |= !ret;
    if(mb_y!=0){
        ret = comp_block_c(up_ref_cr , up_dut_cr , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] V UP error! <%08x>\n", (int)(up_dut_cr+4*16));
            show_block_c(up_ref_cr , up_dut_cr , 8 ,8 , strideuv, 16);
        }
    }
    body_err |= !ret;
    
    if(body_err){
        printf("frame check error @ (MBX: %d, MBY: %d)\n", mb_x, mb_y);
        exit(1);
    }
}

void comp_last_4line_c(x264_t *h){
    int i;
    int stridey   = h->fdec->i_stride[0];
    int strideuv  = h->fdec->i_stride[1];
    HwInfo_t * hwinfo = h->hwinfo;
    uint8_t *dec_dst_y = (uint8_t *)hwinfo->fb_ptr[0][0];
    uint8_t *dec_dst_uv = (uint8_t *)hwinfo->fb_ptr[0][1];
    int dut_str_y = (h->sps->i_mb_width*256 + 2*256);
    int dut_str_uv = (h->sps->i_mb_width*128 + 2*128);
    int ret = 0, error = 0;
    
    for(i=0; i<h->sps->i_mb_width; i++){
        uint8_t * ref_y  = h->fdec->plane[0] + ( ((h->sps->i_mb_height-1)*16+12)* stridey) + i*16;
        uint8_t * ref_cb = h->fdec->plane[1] + ( ((h->sps->i_mb_height-1)*8+4) * strideuv) + i*8;
        uint8_t * ref_cr = h->fdec->plane[2] + ( ((h->sps->i_mb_height-1)*8+4) * strideuv) + i*8;

        uint8_t * dut_y  = dec_dst_y + (h->sps->i_mb_height-1)*dut_str_y + i*256 + 12*16;
        uint8_t * dut_cb = dec_dst_uv + (h->sps->i_mb_height-1)*dut_str_uv + i*128 + 4*16;
        uint8_t * dut_cr = dec_dst_uv + (h->sps->i_mb_height-1)*dut_str_uv + i*128 + 4*16 + 8;

        ret = comp_block_c(ref_y, dut_y, 16, 4, stridey, 16);
        if(!ret) {
            printf("[DBLK] Y MB%d(addr: %08x) last 4 line error!\n", i, (unsigned int)dut_y);
            show_block_c(ref_y, dut_y, 16, 4, stridey, 16);
            error=1;
        }

        ret = comp_block_c(ref_cb, dut_cb, 8, 4, strideuv, 16);
        if(!ret) {
            printf("[DBLK] U MB%d(addr: %08x) last 4 line error!\n", i, (unsigned int)dut_cb);
            show_block_c(ref_cb, dut_cb, 8, 4, strideuv, 16);
            error=1;
        }
        
        ret = comp_block_c(ref_cr, dut_cr, 8, 4, strideuv, 16);
        if(!ret) {
            printf("[DBLK] V MB%d(addr: %08x) last 4 line error!\n", i, (unsigned int)dut_cr);
            show_block_c(ref_cr, dut_cr, 8, 4, strideuv, 16);
            error=1;
        }

        if(error){
            exit(1);
        }
    }
}

void comp_border_c(x264_t *h){
    int stridey   = h->fdec->i_stride[0];
    int strideuv  = h->fdec->i_stride[1];
    
    unsigned char * ref_y  = h->fdec->plane[0] - 16* stridey -16;
    unsigned char * ref_cb = h->fdec->plane[1] - 8 * strideuv -8;
    unsigned char * ref_cr = h->fdec->plane[2] - 8 * strideuv -8;

    int dut_str_y = (h->sps->i_mb_width*256 + 2*256);
    int dut_str_uv = (h->sps->i_mb_width*128 + 2*128);

    HwInfo_t * hwinfo = h->hwinfo;
    uint8_t *dec_dst_y = (uint8_t *)hwinfo->fb_ptr[0][0] - (h->sps->i_mb_width+3)*256;
    uint8_t *dec_dst_uv = (uint8_t *)hwinfo->fb_ptr[0][1] - (h->sps->i_mb_width+3)*128;

    unsigned char * dut_y  = dec_dst_y;
    unsigned char * dut_cb = dec_dst_uv;
    unsigned char * dut_cr = dec_dst_uv +8;

    int mb_x;
    int ret;
    int error=0;
    for(mb_x =0 ;mb_x < (h->sps->i_mb_width+2) ;mb_x++){
        ret = comp_block_c(ref_y , dut_y , 16 ,16 , stridey, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- Y UP --BORDER ERROR !\n", mb_x);
            show_block_c(ref_y , dut_y , 16 ,16 , stridey, 16);
            error=1;
        }
    
        ret = comp_block_c(ref_cb , dut_cb , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- U UP --BORDER ERROR !\n", mb_x);
            show_block_c(ref_cb , dut_cb , 8 ,8 , strideuv, 16);
            error=1;
        }
    
        ret = comp_block_c(ref_cr , dut_cr , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- V UP --BORDER ERROR !\n", mb_x);
            show_block_c(ref_cr , dut_cr , 8 ,8 , strideuv, 16);
            error=1;
        }
    
        ref_y += 16;
        ref_cb += 8;
        ref_cr += 8;

        dut_y += 256;
        dut_cb += 128;
        dut_cr += 128;
    }
    //botton 
    ref_y  = h->fdec->plane[0] + (h->sps->i_mb_height) * stridey *16 - 16;
    ref_cb = h->fdec->plane[1] + (h->sps->i_mb_height) * strideuv *8 -8;
    ref_cr = h->fdec->plane[2] + (h->sps->i_mb_height) * strideuv *8 -8;
    dut_y  = dec_dst_y + (h->sps->i_mb_height +1) * dut_str_y;
    dut_cb = dec_dst_uv + (h->sps->i_mb_height +1) * dut_str_uv;
    dut_cr = dec_dst_uv + (h->sps->i_mb_height +1) * dut_str_uv +8;

    for(mb_x =0 ;mb_x < (h->sps->i_mb_width+2) ;mb_x++){
        ret = comp_block_c(ref_y , dut_y , 16 ,16 , stridey, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- Y BOTTOM --BORDER ERROR !\n", mb_x);
            show_block_c(ref_y , dut_y , 16 ,16 , stridey, 16);
            error=1;
        }
    
        ret = comp_block_c(ref_cb , dut_cb , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- U BOTTOM --BORDER ERROR !\n", mb_x);
            show_block_c(ref_cb , dut_cb , 8 ,8 , strideuv, 16);
            error=1;
        }
    
        ret = comp_block_c(ref_cr , dut_cr , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- V BOTTOM --BORDER ERROR !\n", mb_x);
            show_block_c(ref_cr , dut_cr , 8 ,8 , strideuv, 16);
            error=1;
        }
    
        ref_y += 16;
        ref_cb += 8;
        ref_cr += 8;
        
        dut_y += 256;
        dut_cb += 128;
        dut_cr += 128;
    }

    //left
    ref_y  = h->fdec->plane[0] -16;
    ref_cb = h->fdec->plane[1] -8;
    ref_cr = h->fdec->plane[2] -8;
    dut_y  = dec_dst_y + dut_str_y;
    dut_cb = dec_dst_uv + dut_str_uv;
    dut_cr = dec_dst_uv + dut_str_uv +8;

    for(mb_x =0 ;mb_x < (h->sps->i_mb_height+1) ;mb_x++){
        ret = comp_block_c(ref_y , dut_y , 16 ,16 , stridey, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- Y LEFT --BORDER ERROR !\n", mb_x);
            show_block_c(ref_y , dut_y , 16 ,16 , stridey, 16);
            error=1;
        }
    
        ret = comp_block_c(ref_cb , dut_cb , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- U LEFT --BORDER ERROR !\n", mb_x);
            show_block_c(ref_cb , dut_cb , 8 ,8 , strideuv, 16);
            error=1;
        }
    
        ret = comp_block_c(ref_cr , dut_cr , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- V LEFT --BORDER ERROR !\n", mb_x);
            show_block_c(ref_cr , dut_cr , 8 ,8 , strideuv, 16);
            error=1;
        }
    
        ref_y += stridey *16;
        ref_cb += strideuv *8;
        ref_cr += strideuv *8;

        dut_y += dut_str_y;
        dut_cb += dut_str_uv;
        dut_cr += dut_str_uv;
    }

    //right
    ref_y  = h->fdec->plane[0] +h->sps->i_mb_width*16 ;
    ref_cb = h->fdec->plane[1] +h->sps->i_mb_width*8  ;
    ref_cr = h->fdec->plane[2] +h->sps->i_mb_width*8  ; 
    dut_y  = dec_dst_y + dut_str_y + (h->sps->i_mb_width +1 )*256;
    dut_cb = dec_dst_uv + dut_str_uv + (h->sps->i_mb_width +1 )*128;
    dut_cr = dec_dst_uv + dut_str_uv + (h->sps->i_mb_width +1 )*128 +8;

    for(mb_x =0 ;mb_x < (h->sps->i_mb_height+1) ;mb_x++){
        ret = comp_block_c(ref_y , dut_y , 16 ,16 , stridey, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- Y RIGHT --BORDER ERROR !\n", mb_x);
            show_block_c(ref_y , dut_y , 16 ,16 , stridey, 16);
            error=1;
        }
    
        ret = comp_block_c(ref_cb , dut_cb , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- U RIGHT --BORDER ERROR !\n", mb_x);
            show_block_c(ref_cb , dut_cb , 8 ,8 , strideuv, 16);
            error=1;
        }
    
        ret = comp_block_c(ref_cr , dut_cr , 8 ,8 , strideuv, 16);
        if(!ret){
            printf("[DBLK] MB (%d) -- V RIGHT --BORDER ERROR !\n", mb_x);
            show_block_c(ref_cr , dut_cr , 8 ,8 , strideuv, 16);
            error=1;
        }
    
        ref_y += stridey *16;
        ref_cb += strideuv *8;
        ref_cr += strideuv *8;

        dut_y += dut_str_y;
        dut_cb += dut_str_uv;
        dut_cr += dut_str_uv;
    }
    
    if(error){
        exit(1);
    }
}

void check_deblock( x264_t *h ) {
    if(!h->mb.i_mb_x){
        if(h->mb.i_mb_y)
            comp_mb_c( h, h->sps->i_mb_width-1, h->mb.i_mb_y-1);
    } else {
        comp_mb_c( h, h->mb.i_mb_x-1, h->mb.i_mb_y);
    }
}

void check_frame( x264_t *h ) {
    printf("[DBLK] checking frame...\n");
    int mb_x, mb_y;
    for(mb_y=0; mb_y<h->sps->i_mb_height; mb_y++)
        for(mb_x=0; mb_x<h->sps->i_mb_width; mb_x++){
            if(!mb_x){
                if(mb_y)
                    comp_mb_c( h, h->sps->i_mb_width-1, mb_y-1);
            } else {
                comp_mb_c( h, mb_x-1, mb_y);
            }
        }
}

void check_expand( x264_t *h ) {
    //while(!(read_reg(DBLK_V_BASE, DEBLK_REG_GSTA) & 0x1) );
    comp_mb_c( h, (h->sps->i_mb_width-1), (h->sps->i_mb_height-1) );
    comp_last_4line_c(h);
    
    printf("[DBLK] checking expand ...\n");
    comp_border_c(h);
}

/**************************************************
SDE
**************************************************/
void check_parser( x264_t *h ) {
    int sde_err = 0;
    unsigned int hw_low = *(volatile unsigned int *)(sde_base + 0x28);
    unsigned int hw_range_queue = *(volatile unsigned int *)(sde_base + 0x2c);
    unsigned int hw_range = hw_range_queue >> 16;
    unsigned int hw_queue = hw_range_queue & 0xFFFF;
    if (hw_low != h->cabac.i_low) {
        printf("[SDE] low   error %x - %x \n",hw_low ,h->cabac.i_low );
        sde_err++;
    }
    if (hw_range != h->cabac.i_range) {
        printf("[SDE] range error %x - %x \n",hw_range ,h->cabac.i_range );
        sde_err++;
    }
    int que = (h->cabac.i_queue==-1) ? 7:h->cabac.i_queue;
    if (hw_queue != que) {
        printf("[SDE] queue error %x - %x \n",hw_queue ,h->cabac.i_queue );
        sde_err++;
    }
    
    if(sde_err ){
        printf("[SDE] standard:\n low %x \n range %x \n queue %x \n",
               h->cabac.i_low ,h->cabac.i_range ,h->cabac.i_queue );
        
        int id;
        printf("[SDE] descriptor:\n");
        for( id=0 ; id<7 ; id++){
            printf("[%d] %08x\n", id, (unsigned int)read_reg((VRAM_SDE_CHN_BASE + id*4), 0) );
        }
        exit(1);
    }
}

void check_bitstream( x264_t *h , int bs_len, int hw_bs_len) {
    int i;
    uint8_t *std_bs_buf = h->cabac.p_start;
    HwInfo_t * hwinfo = h->hwinfo;
    uint8_t *dut_out = (uint8_t *)hwinfo->bs_ptr;

    printf("[SDE] checking bitstream...\n");
    if( bs_len != hw_bs_len ) {
        printf("[SDE] BS size error: %x - %x\n", bs_len, hw_bs_len);
        //exit(1);
    }

    for (i=0; i<bs_len; i++) {
        if (std_bs_buf[i] != dut_out[i]) {
            printf("[SDE] BS error: %02x - %02x @ addr: %08x [%d/%d]\n",
                   std_bs_buf[i], dut_out[i], (unsigned int)&dut_out[i], i, bs_len);
            exit(1);
        }
    }
} 

