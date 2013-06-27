#include "mp_decoder.h"
extern "C" {
#include "libmpeg2/mpeg2.h"
}
#include "libmpeg2/attributes.h"
#include "libmpeg2/mpeg2_internal.h"

#include "mpeg2_decoder.h"

#include "jzasm.h"

#include <utils/Log.h>

//#define EL(x,y...) {ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y);}
#define EL(x,y...)
//#define EL(x,y...){ALOGE(x,##y);}

extern "C"{
  //void *jz4740_alloc_frame (int align, int size);
  //unsigned int get_phy_addr (unsigned int vaddr);
}
extern int use_jz_buf;

namespace android{

  //namespace lumevideo{
    
mpeg2Decoder::mpeg2Decoder()
    :dropped_frames(0){
    avcodec_initialized = 0;
    mFrame_Mem = 0;
}

mpeg2Decoder::~mpeg2Decoder(){
    
}

int mpeg2Decoder::preinit(sh_video_t *sh){
    return 1;
}

int mpeg2Decoder::init(sh_video_t *sh){
    vd_libmpeg2_ctx_t *context;
    mpeg2dec_t * mpeg2dec;
    int accel;

    if(sh->disp_w && sh->disp_h && (mFrame_Mem == NULL))
        mFrame_Mem = new LumeMemory(sh->disp_w,sh->disp_h);

    accel = 0;
    #if HAVE_MVI
       accel |= MPEG2_ACCEL_ALPHA_MVI;
    #elif HAVE_VIS
       accel |= MPEG2_ACCEL_SPARC_VIS;
    #endif
    mpeg2_accel(accel);

    mpeg2dec = mpeg2_init ();

    if(!mpeg2dec) return 0;

    mpeg2_custom_fbuf(mpeg2dec,1); // enable DR1

    context = (vd_libmpeg2_ctx_t*)calloc(1, sizeof(vd_libmpeg2_ctx_t));
    context->mpeg2dec = mpeg2dec;
    sh->context = context;

    //use_jz_buf = 0;
    mFrame_Mem->muse_jz_buf = mpeg2dec->use_jz_buf = 1;
    mpeg2dec->VpuMem_ptr = mVpuMem_ptr;
    sh->drvmpeg2 = 1;

    mpeg2dec->decoder.slice_info_hw.des_va = (unsigned int*)jz4740_alloc_frame(mpeg2dec->VpuMem_ptr, 32, 0x5000);
    mpeg2dec->decoder.slice_info_hw.des_pa = mpeg2dec->decoder.slice_info_hw.des_va;

    return 1;
}
void mpeg2Decoder::uninit(sh_video_t *sh){
    int i;
    vd_libmpeg2_ctx_t *context = (vd_libmpeg2_ctx_t*)sh->context;
    mpeg2dec_t * mpeg2dec = context->mpeg2dec;
    if (context->pending_buffer) free(context->pending_buffer);
    mpeg2dec->decoder.convert=NULL;
    mpeg2dec->decoder.convert_id=NULL;
    mpeg2_close (mpeg2dec);
    for (i=0; i < 3; i++)
        free(context->quant_store[i]);
    free(sh->context);

	if(mFrame_Mem)
		delete mFrame_Mem;
	mFrame_Mem = NULL;
}

static void dump_file(unsigned char *data,int len,unsigned int framenum){
    int i,j;
    FILE *fp;
    if((fp=fopen("/data/err.log","a+")) == NULL)
	fp = fopen("/data/err.log","w+");
    
    fprintf(fp,"framenum is %d\n",framenum);
    for(i = 0;i < len / 16;i++)
    {
	fprintf(fp,"%08x:",i);
	for(j = 0;j < 16;j++)
	{
	    fprintf(fp,"%02x ",data[i * 16 +j]);
	}
	fprintf(fp,"\n");
    }
    if((len % 16))
    {
	fprintf(fp,"%08x:",i);
	for(j = 0;j < (len % 16);j++)
	{
	    fprintf(fp,"%02x ",data[i * 16 +j]);
	}
	fprintf(fp,"\n");
    }
    fclose(fp);
}

static char * copy_bs=NULL;

int mpeg2Decoder::decode_video(sh_video_t *sh,unsigned char **inbuf,int *inslen,unsigned char* outbuf,int *outlen, int dropframe){

    *outlen = 0;
    if(*inslen<=0) return NULL; // skipped frame

#define DUMP_DATA 0
#if  DUMP_DATA==1
    unsigned char *dump_duf = (unsigned char *)*inbuf;
    static int framenum = 0;
    framenum++;
    unsigned int dump_len = *inslen;
    if(framenum > 100)
    {
	dump_file(dump_duf,dump_len,framenum);
	EL("ddddddddddddddddddddddddddddddddddddddddddddd");
    }
#endif

    vd_libmpeg2_ctx_t *context = (vd_libmpeg2_ctx_t*)sh->context;
    mpeg2dec_t * mpeg2dec = context->mpeg2dec;
    const mpeg2_info_t * info = mpeg2_info (mpeg2dec);
    int drop_frame, framedrop=dropframe;
    int len = *inslen;

    uint8_t *p = (uint8_t*)(*inbuf);//(uint8_t*)(*((int *)*inbuf)?? hardly believe it could work in opencore...
    //ALOGE("aInputBuf addr is 0x%x",p);

    // MPlayer registers its own draw_slice callback, prevent libmpeg2 from freeing the context
    mpeg2dec->decoder.convert=NULL;
    mpeg2dec->decoder.convert_id=NULL;
    
    sh->ds->need_free += 1;
    
    if(len<=0){
        return NULL; // skipped null frame
    }

    // append extra 'end of frame' code:
    ((char*)p+len)[0]=0;
    ((char*)p+len)[1]=0;
    ((char*)p+len)[2]=1;
    ((char*)p+len)[3]=0xff;
    len+=4;

    if (sh->ds->seek_flag){
      mpeg2dec->seek_flag = 1;
      EL("mpeg2dec->seek_flag set to 1");
      context->pending_length = 0;
    }

    if (mpeg2dec->seek_flag == 1){
      mpeg2dec->fbuf[0]->buf[1] = NULL;
      mpeg2dec->fbuf[1]->buf[1] = NULL;
      mpeg2dec->fbuf[2]->buf[1] = NULL;
    }

    if (context->pending_length && !sh->ds->seek_flag) {
        mpeg2_buffer (mpeg2dec, context->pending_buffer, context->pending_buffer + context->pending_length);
    } else {
      mpeg2_buffer (mpeg2dec, (uint8_t *)p, (uint8_t *)p+len);
    }
    if (sh->ds->seek_flag){
      sh->ds->seek_flag = 0;
    }

    while(1){
        int state=mpeg2_parse (mpeg2dec);
        int type;
	
        mp_image_t* mpi_new;
        unsigned long pw, ph;
        int imgfmt;

        switch(state){
        case STATE_BUFFER:
            if (context->pending_length) {
                // just finished the pending data, continue with processing of the passed buffer
                context->pending_length = 0;
		mpeg2_buffer (mpeg2dec, (uint8_t *)p, (uint8_t *)p+len);
            } else {
                // parsing of the passed buffer finished, return.
                *inbuf += *inslen;
                *inslen = 0;
                return 0;
            }
            break;
        case STATE_SEQUENCE:
            pw = info->sequence->display_width * info->sequence->pixel_width;
            ph = info->sequence->display_height * info->sequence->pixel_height;
            if(ph) sh->aspect = (float) pw / (float) ph;
            // video parameters initialized/changed, (re)init libvo:
            if (info->sequence->width >> 1 == info->sequence->chroma_width &&
                info->sequence->height >> 1 == info->sequence->chroma_height) {
                imgfmt = IMGFMT_YV12;
            } else if (info->sequence->width >> 1 == info->sequence->chroma_width &&
                       info->sequence->height == info->sequence->chroma_height) {
                imgfmt = IMGFMT_422P;
            } else{
                *inbuf += *inslen;
                *inslen = 0;
                return 0;
            }
            if (imgfmt == context->imgfmt &&
                info->sequence->picture_width == context->width &&
                info->sequence->picture_height == context->height &&
                sh->aspect == context->aspect)
                break;
#if 0
            if(!mpcodecs_config_vo(sh,
                                   info->sequence->picture_width,
                                   info->sequence->picture_height, imgfmt))
                return 0;
#endif
            context->imgfmt = imgfmt;
            context->width = info->sequence->picture_width;
            context->height = info->sequence->picture_height;
            context->aspect = sh->aspect;
            break;
        case STATE_PICTURE:
            type=info->current_picture->flags&PIC_MASK_CODING_TYPE;
            
            drop_frame = framedrop && (mpeg2dec->decoder.coding_type == B_TYPE);
            drop_frame |= framedrop>=2; // hard drop
            if (drop_frame) {
                mpeg2_skip(mpeg2dec, 1);
                break;
            }
            mpeg2_skip(mpeg2dec, 0); //mpeg2skip skips frames until set again to 0


            // get_buffer "callback":
#if 0
            mpi_new=mpcodecs_get_image(sh,MP_IMGTYPE_IPB,
                                       (type==PIC_FLAG_CODING_TYPE_B) ?
                                       use_callback : (MP_IMGFLAG_PRESERVE|MP_IMGFLAG_READABLE),
                                       info->sequence->width,
                                       info->sequence->height);
#else
            mpi_new = mFrame_Mem->get_image(mpeg2dec->VpuMem_ptr, sh->codec->outfmt[sh->outfmtidx], MP_IMGTYPE_IPB,
                                            (MP_IMGFLAG_PRESERVE|MP_IMGFLAG_READABLE),
                                            info->sequence->width,
                                            info->sequence->height);
#endif
            if(!mpi_new){
                *inbuf += *inslen;
                *inslen = 0;
                return 0; // VO ERROR!!!!!!!!
            }
            mpeg2_set_buf(mpeg2dec, mpi_new->planes, mpi_new);
            //mpi_new->stride[0] = info->sequence->width*16;
            //mpi_new->stride[1] = info->sequence->chroma_width;
            //mpi_new->stride[2] = info->sequence->chroma_width;
	    mpeg2dec->decoder.y_stride = mpi_new->stride[0];
	    mpeg2dec->decoder.c_stride = mpi_new->stride[1];
            if (info->current_picture->flags&PIC_FLAG_TOP_FIELD_FIRST)
                mpi_new->fields |= MP_IMGFIELD_TOP_FIRST;
            else mpi_new->fields &= ~MP_IMGFIELD_TOP_FIRST;
            if (info->current_picture->flags&PIC_FLAG_REPEAT_FIRST_FIELD)
                mpi_new->fields |= MP_IMGFIELD_REPEAT_FIRST;
            else mpi_new->fields &= ~MP_IMGFIELD_REPEAT_FIRST;
            mpi_new->fields |= MP_IMGFIELD_ORDERED;
            if (!(info->current_picture->flags&PIC_FLAG_PROGRESSIVE_FRAME))
                mpi_new->fields |= MP_IMGFIELD_INTERLACED;
/*
 * internal libmpeg2 does export quantization values per slice
 * we let postproc know them to fine tune it's strength
 */
#if defined(MPEG12_POSTPROC) && defined(CONFIG_LIBMPEG2_INTERNAL)
            mpi_new->qstride=info->sequence->width>>4;
            {
                char **p = &context->quant_store[type==PIC_FLAG_CODING_TYPE_B ?
                                                 2 : (context->quant_store_idx ^= 1)];
                *p = (char*)realloc(*p, mpi_new->qstride*(info->sequence->height>>4));
                mpi_new->qscale = *p;
            }
            mpeg2dec->decoder.quant_store=mpi_new->qscale;
            mpeg2dec->decoder.quant_stride=mpi_new->qstride;
            mpi_new->pict_type=type; // 1->I, 2->P, 3->B
            mpi_new->qscale_type= 1;
#endif

            if (mpi_new->flags&MP_IMGFLAG_DRAW_CALLBACK
                && !(mpi_new->flags&MP_IMGFLAG_DIRECT)) {
                // nice, filter/vo likes draw_callback :)
                mpeg2dec->decoder.convert=NULL;//draw_slice;
                mpeg2dec->decoder.convert_id=sh;
            } else {
                mpeg2dec->decoder.convert=NULL;
                mpeg2dec->decoder.convert_id=NULL;
            }

            break;
        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
            // decoding done:
            if(info->display_fbuf) {
                mp_image_t* mpi = (mp_image_t*)info->display_fbuf->id;
		EL("mpi is %x dis:%x fbuf:%x %x %x", mpi, info->display_fbuf, info->display_fbuf->buf[0], info->display_fbuf->buf[1], info->display_fbuf->buf[2]);

                if (context->pending_length == 0) {
                    context->pending_length = mpeg2dec->buf_end - mpeg2dec->buf_start;
                    context->pending_buffer = (uint8_t*)realloc(context->pending_buffer, context->pending_length);
                    memcpy(context->pending_buffer, mpeg2dec->buf_start, context->pending_length);
                } else {
                    // still some data in the pending buffer, shouldn't happen
                    context->pending_length = mpeg2dec->buf_end - mpeg2dec->buf_start;
                    context->pending_buffer = (uint8_t*)realloc(context->pending_buffer, context->pending_length + len);
		    memmove(context->pending_buffer, mpeg2dec->buf_start, context->pending_length);
		    memcpy(context->pending_buffer+context->pending_length, p, len);
                    context->pending_length += len;
                }

                *inbuf += *inslen;
                *inslen = 0;

		if (mpeg2dec->seek_flag == 2 && (/*mpeg2dec->decoder.coding_type == 1 || */mpeg2dec->decoder.coding_type == 2)){ //seek process end
		  EL("mpeg2dec->seek_flag set to 0");
		  mpeg2dec->seek_flag = 0;
		}

		if (mpeg2dec->seek_flag == 1 && mpeg2dec->decoder.coding_type == 1 && mpeg2dec->fbuf[0]->buf[1] != NULL){
		  EL("mpeg2dec->seek_flag set to 2");
		  mpeg2dec->seek_flag = 2;//decode a I frame but out put frame invalid, so seek_flag to 2
		}
#if 1
		if ((/*mpeg2dec->decoder.coding_type == 3 
		       && */(mpeg2dec->fbuf[1]->buf[1] == NULL || mpeg2dec->fbuf[2]->buf[1] == NULL))// I and B frame
		    || (mpeg2dec->decoder.coding_type == 2 && mpeg2dec->fbuf[1]->buf[1] == NULL)// P frame
		    || mpeg2dec->seek_flag == 2 //seek_flag == 2 no right out put frame
		    || (mpeg2dec->fbuf[0]->buf[1] == NULL)){ //fbuf[0] is NULL, decode process wrong.
		  EL("return last_mpi %x", mpeg2dec->last_mpi);
		  return (int)(mpeg2dec->last_mpi);
		}
#endif
		mpeg2dec->last_mpi = mpi;
		EL("set last_mpi is %x", mpeg2dec->last_mpi);
                return (int)mpi;
            }
        }
    }
}


int mpeg2Decoder::control(sh_video_t *sh,int cmd,void* arg, ...){
    vd_libmpeg2_ctx_t *context = (vd_libmpeg2_ctx_t*)sh->context;
    mpeg2dec_t * mpeg2dec = context->mpeg2dec;
    const mpeg2_info_t * info = mpeg2_info (mpeg2dec);

    switch(cmd) {
    case VDCTRL_QUERY_FORMAT:
        if (info->sequence->width >> 1 == info->sequence->chroma_width &&
	    info->sequence->height >> 1 == info->sequence->chroma_height &&
	    (*((int*)arg)) == IMGFMT_YV12)
	    return CONTROL_TRUE;
	if (info->sequence->width >> 1 == info->sequence->chroma_width &&
		info->sequence->height == info->sequence->chroma_height &&
	    (*((int*)arg)) == IMGFMT_422P)
	    return CONTROL_TRUE;
	return CONTROL_FALSE;
    }

    return CONTROL_UNKNOWN;
}

vd_info_t mpeg2Decoder::m_info = {
	"libmpeg2 MPEG 1/2 Video decoder",
	"libmpeg2",
	"A'rpi & Fabian Franz",
	"Aaron & Walken",
	"native"
};
  //}
}
