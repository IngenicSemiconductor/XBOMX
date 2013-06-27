#include "lume_dec.h"
#include "lume_video_decoder.h"
#include "jzasm.h"

#include <utils/Log.h>

#define EL(x,y...) //{ALOGE("%s %d",__FILE__,__LINE__); LOGE(x,##y);}

extern "C"{
  void *jz4740_alloc_frame (int *VpuMem_ptr, int align, int size);
  unsigned int get_phy_addr (unsigned int vaddr);
  void ff_mpeg_flush(AVCodecContext *avctx);
}

namespace android{

  //namespace lumevideo{
    
int lumeDecoder::get_buffer(AVCodecContext *avctx, AVFrame *pic){
    sh_video_t *sh = (sh_video_t *)avctx->opaque;
    vd_lume_ctx *ctx = (vd_lume_ctx *)sh->context;
    lumeDecoder *vdec = (lumeDecoder *)ctx->vdec; 
    mp_image_t *mpi=NULL;
    int flags= MP_IMGFLAG_ACCEPT_STRIDE | MP_IMGFLAG_PREFER_ALIGNED_STRIDE;
    int type= MP_IMGTYPE_IPB;
    int width= avctx->width;
    int height= avctx->height;
    avcodec_align_dimensions(avctx, &width, &height);
    //printf("get_buffer %d %d %d\n", pic->reference, ctx->ip_count, ctx->b_count);
    if (pic->buffer_hints) {
        mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "Buffer hints: %u\n", pic->buffer_hints);
        type = MP_IMGTYPE_TEMP;
        if (pic->buffer_hints & FF_BUFFER_HINTS_READABLE)
            flags |= MP_IMGFLAG_READABLE;
        if (pic->buffer_hints & FF_BUFFER_HINTS_PRESERVE) {
            type = MP_IMGTYPE_STATIC;
            flags |= MP_IMGFLAG_PRESERVE;
        }
        if (pic->buffer_hints & FF_BUFFER_HINTS_REUSABLE) {
            type = MP_IMGTYPE_STATIC;
            flags |= MP_IMGFLAG_PRESERVE;
        }
        flags|=(!avctx->hurry_up && ctx->do_slices) ?
	    MP_IMGFLAG_DRAW_CALLBACK:0;
        mp_msg(MSGT_DECVIDEO, MSGL_DBG2, type == MP_IMGTYPE_STATIC ? "using STATIC\n" : "using TEMP\n");
    } else {
        if(!pic->reference){
            ctx->b_count++;
            flags|=(!avctx->hurry_up && ctx->do_slices) ?
		MP_IMGFLAG_DRAW_CALLBACK:0;
        }else{
            ctx->ip_count++;
            flags|= MP_IMGFLAG_PRESERVE|MP_IMGFLAG_READABLE
		| (ctx->do_slices ? MP_IMGFLAG_DRAW_CALLBACK : 0);
        }
    }
    
    if(vdec->init_vo((sh_video_t *)sh,(enum PixelFormat)avctx->pix_fmt) < 0){
        avctx->release_buffer= avcodec_default_release_buffer;
        avctx->get_buffer= avcodec_default_get_buffer;
        return avctx->get_buffer(avctx, pic);
    }
    
    if (IMGFMT_IS_XVMC(ctx->best_csp) || IMGFMT_IS_VDPAU(ctx->best_csp)) {
        type =  MP_IMGTYPE_NUMBERED | (0xffff << 16);
    } else if (!pic->buffer_hints) {
	if(ctx->b_count>1 || ctx->ip_count>2){
	     if(avctx->codec_id != CODEC_ID_MPEG1VIDEO && avctx->codec_id != CODEC_ID_MPEG2VIDEO){
            ctx->do_dr1=0; //FIXME
            avctx->get_buffer= avcodec_default_get_buffer;
            return avctx->get_buffer(avctx, pic);
	    }
	}
	
	if(avctx->has_b_frames){
	    type= MP_IMGTYPE_IPB;
	}else{
	    type= MP_IMGTYPE_IP;
	}
	mp_msg(MSGT_DECVIDEO, MSGL_DBG2, type== MP_IMGTYPE_IPB ? "using IPB\n" : "using IP\n");
    }
    
    if (ctx->best_csp == IMGFMT_RGB8 || ctx->best_csp == IMGFMT_BGR8)
        flags |= MP_IMGFLAG_RGB_PALETTE;
    
    //mpi= mpcodecs_get_image(sh, type, flags, width, height);
    
    mpi = vdec->mFrame_Mem->get_image(avctx->VpuMem_ptr,sh->codec->outfmt[sh->outfmtidx],type,flags,width,height);
    if (!mpi) return -1;
    
    avctx->draw_horiz_band= NULL;
    if(IMGFMT_IS_VDPAU(mpi->imgfmt)) {
        //avctx->draw_horiz_band = draw_slice;
	printf("draw_slice is null!");
    }
    pic->data[0]= mpi->planes[0];
    pic->data[1]= mpi->planes[1];
    pic->data[2]= mpi->planes[2];
    pic->data[3]= mpi->planes[3];
    
    
    /* Note, some (many) codecs in libavcodec must have stride1==stride2 && no changes between frames
     * lavc will check that and die with an error message, if its not true
     */
    
    pic->linesize[0]= mpi->stride[0];
    pic->linesize[1]= mpi->stride[1];
    pic->linesize[2]= mpi->stride[2];
    pic->linesize[3]= mpi->stride[3];
    
    pic->opaque = mpi;
    if(pic->reference){
        pic->age= ctx->ip_age[0];
	
        ctx->ip_age[0]= ctx->ip_age[1]+1;
        ctx->ip_age[1]= 1;
        ctx->b_age++;
    }else{
        pic->age= ctx->b_age;
	
        ctx->ip_age[0]++;
        ctx->ip_age[1]++;
        ctx->b_age=1;
    }
    pic->type= FF_BUFFER_TYPE_USER;
    return 0;
}
    
    
void lumeDecoder::release_buffer(struct AVCodecContext *avctx, AVFrame *pic){
    mp_image_t *mpi= (mp_image_t *)pic->opaque;
    sh_video_t *sh = (sh_video_t *)avctx->opaque;
    vd_lume_ctx *ctx = (vd_lume_ctx *)sh->context;
    int i;
      
    if(ctx->ip_count <= 2 && ctx->b_count<=1){
        if(mpi->flags&MP_IMGFLAG_PRESERVE)
            ctx->ip_count--;
        else
            ctx->b_count--;
    }
    
    if (mpi) {
        // Palette support: free palette buffer allocated in get_buffer
        if (mpi->bpp == 8)
            av_freep(&mpi->planes[1]);
#if CONFIG_XVMC
        if (IMGFMT_IS_XVMC(mpi->imgfmt)) {
            struct xvmc_pix_fmt *render = (struct xvmc_pix_fmt*)pic->data[2]; //same as mpi->priv
            if(mp_msg_test(MSGT_DECVIDEO, MSGL_DBG5))
                mp_msg(MSGT_DECVIDEO, MSGL_DBG5, "vd_lume::release_buffer (xvmc render=%p)\n", render);
            assert(render!=NULL);
            assert(render->xvmc_id == AV_XVMC_ID);
            render->state&=~AV_XVMC_STATE_PREDICTION;
        }
#endif
        // release mpi (in case MPI_IMGTYPE_NUMBERED is used, e.g. for VDPAU)
        mpi->usage_count--;
    }

    if(pic->type!=FF_BUFFER_TYPE_USER){
        avcodec_default_release_buffer(avctx, pic);
        return;
    }
    
    for(i=0; i<4; i++){
        pic->data[i]= NULL;
    }

}
    
typedef struct dp_hdr_s {
    uint32_t chunks;        // number of chunks
    uint32_t timestamp; // timestamp from packet header
    uint32_t len;        // length of actual data
    uint32_t chunktab;        // offset to chunk offset array
} dp_hdr_t;

int lumeDecoder::init_vo(sh_video_t *sh, enum PixelFormat pix_fmt){
    vd_lume_ctx *ctx = (vd_lume_ctx *)sh->context;
    AVCodecContext *avctx = (AVCodecContext *)ctx->avctx;
    float aspect= av_q2d(avctx->sample_aspect_ratio) * avctx->width / avctx->height;
    int width, height;
    
    width = avctx->width;
    height = avctx->height;
    
    // HACK!
    // if sh->ImageDesc is non-NULL, it means we decode QuickTime(tm) video.
    // use dimensions from BIH to avoid black borders at the right and bottom.
    if (sh->bih && sh->ImageDesc) {
        width = sh->bih->biWidth>>0;
        height = sh->bih->biHeight>>0;
    }

    // it is possible another vo buffers to be used after vo config()
     // lavc reset its buffers on width/heigh change but not on aspect change!!!
    if (av_cmp_q(avctx->sample_aspect_ratio, ctx->last_sample_aspect_ratio) ||
        width != sh->disp_w  ||
        height != sh->disp_h ||
        pix_fmt != ctx->pix_fmt ||
        !ctx->vo_initialized){
        // this is a special-case HACK for MPEG-1/2 VDPAU that uses neither get_format nor
        // sets the value correctly in avcodec_open.
        mp_msg(MSGT_DECVIDEO, MSGL_V, "[lume] aspect_ratio: %f\n", aspect);
        if (sh->aspect == 0 ||
            av_cmp_q(avctx->sample_aspect_ratio,
                     ctx->last_sample_aspect_ratio))
            sh->aspect = aspect;
        ctx->last_sample_aspect_ratio = avctx->sample_aspect_ratio;
        sh->disp_w = width;
        sh->disp_h = height;
        ctx->pix_fmt = pix_fmt;
        ctx->best_csp = pixfmt2imgfmt(pix_fmt);
	
	unsigned int out_fmt = 0;
	for (int i = 0; i < CODECS_MAX_OUTFMT; i++) {
	    out_fmt = sh->codec->outfmt[i];
	    if (out_fmt == (unsigned int) 0xFFFFFFFF)
		continue;
	    
	    if(control(sh, VDCTRL_QUERY_FORMAT, &out_fmt) == CONTROL_TRUE){
		sh->outfmtidx = i;
		break;
	    }
	}
	
        ctx->vo_initialized = 1;
    }
    return 0;
}
    
lumeDecoder::lumeDecoder()
    :dropped_frames(0){
    avcodec_initialized = 0;
    mFrame_Mem = 0;
    copy_bs = NULL;
}

lumeDecoder::~lumeDecoder(){
    ALOGE("ffVideodecoder destructed");
}
    
int lumeDecoder::preinit(sh_video_t *sh){
    return 1;
}    
    
void lumeDecoder::init_avcodec(int isvp){
    if (!avcodec_initialized) {
	avcodec_init();
	video_avcodec_register_all(isvp);
        avcodec_initialized = 1;
    }
}
    
int lumeDecoder::init(sh_video_t *sh)
{
    AVCodecContext *avctx;
    vd_lume_ctx *ctx;
    AVCodec *lavc_codec;
    int lowres_w=0;
    int do_vis_debug= 0;

    init_avcodec(sh->mIsVpplayer);
    if(sh->disp_w && sh->disp_h && (mFrame_Mem == NULL)){     
        mFrame_Mem = new LumeMemory(sh->disp_w,sh->disp_h);
    }
    
    ctx = (vd_lume_ctx *)malloc(sizeof(vd_lume_ctx));
	sh->context = (void *)ctx;
    if (!ctx)
        return 0;
    memset(ctx, 0, sizeof(vd_lume_ctx));

    lavc_codec = avcodec_find_decoder_by_name(sh->codec->dll);
    if(!lavc_codec){
        uninit(sh);
        return 0;
    }

    //if( CODEC_ID_MPEG1VIDEO == lavc_codec->id || CODEC_ID_MPEG2VIDEO == lavc_codec->id )
    //  return 0;
    
    ctx->vdec = this;
    
    if(lavc_codec->capabilities&CODEC_CAP_DRAW_HORIZ_BAND)
        ctx->do_slices=1;

    if(lavc_codec->capabilities&CODEC_CAP_DR1 && !do_vis_debug && lavc_codec->id != CODEC_ID_H264 && lavc_codec->id != CODEC_ID_INTERPLAY_VIDEO && lavc_codec->id != CODEC_ID_ROQ && lavc_codec->id != CODEC_ID_VP8)
        ctx->do_dr1=1;
    ctx->b_age= ctx->ip_age[0]= ctx->ip_age[1]= 256*256*256*64;
    ctx->ip_count= ctx->b_count= 0;

    ctx->pic = avcodec_alloc_frame();
    ctx->avctx = avcodec_alloc_context();
    avctx = ctx->avctx;

    avctx->VpuMem_ptr = mVpuMem_ptr;
    avctx->opaque = sh;
    avctx->codec_type = CODEC_TYPE_VIDEO;
    avctx->codec_id = lavc_codec->id;

    if(ctx->do_dr1){
        avctx->flags|= CODEC_FLAG_EMU_EDGE;
        avctx->get_buffer= lumeDecoder::get_buffer;
        avctx->release_buffer= lumeDecoder::release_buffer;
        avctx->reget_buffer= lumeDecoder::get_buffer;
    }

    avctx->flags|= 0;
    avctx->is_dechw = 0;
    avctx->coded_width = sh->disp_w;
    avctx->coded_height= sh->disp_h;
    avctx->workaround_bugs= 0;
    avctx->error_recognition= 0;
    if(0) avctx->flags|= CODEC_FLAG_GRAY;
    avctx->flags2|= 0;
    avctx->codec_tag= sh->format;
    avctx->stream_codec_tag= sh->video.fccHandler;
    avctx->idct_algo= 0;
    avctx->error_concealment= 0;
    avctx->debug= 0;
    avctx->debug_mv= 0;
    avctx->skip_top   = 0;
    avctx->skip_bottom= 0;
    avctx->skip_loop_filter = AVDISCARD_DEFAULT;
    avctx->skip_idct = AVDISCARD_DEFAULT;
    avctx->skip_frame = AVDISCARD_DEFAULT;

    switch (sh->format) {
    case mmioFOURCC('S','V','Q','3'):
    /* SVQ3 extradata can show up as sh->ImageDesc if demux_mov is used, or
       in the phony AVI header if demux_lavf is used. The first case is
       handled here; the second case falls through to the next section. */
        if (sh->ImageDesc) {
            avctx->extradata_size = (*(int *)sh->ImageDesc) - sizeof(int);
            avctx->extradata = (uint8_t*)av_mallocz(avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
            memcpy(avctx->extradata, ((int *)sh->ImageDesc)+1, avctx->extradata_size);
            break;
        }
        /* fallthrough */

    case mmioFOURCC('A','V','R','n'):
    case mmioFOURCC('M','J','P','G'):
    /* AVRn stores huffman table in AVI header */
    /* Pegasus MJPEG stores it also in AVI header, but it uses the common
       MJPG fourcc :( */
        if (!sh->bih || sh->bih->biSize <= sizeof(BITMAPINFOHEADER))
            break;
    avctx->flags |= CODEC_FLAG_EXTERN_HUFF;
    avctx->extradata_size = sh->bih->biSize-sizeof(BITMAPINFOHEADER);
    avctx->extradata = (uint8_t*)av_mallocz(avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(avctx->extradata, sh->bih+1, avctx->extradata_size);

#if 0
    {
	int x;
	uint8_t *p = avctx->extradata;
	
	for (x=0; x<avctx->extradata_size; x++)
	    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "[%x] ", p[x]);
	mp_msg(MSGT_DECVIDEO, MSGL_INFO, "\n");
    }
#endif
    break;

    case mmioFOURCC('R', 'V', '1', '0'):
    case mmioFOURCC('R', 'V', '1', '3'):
    case mmioFOURCC('R', 'V', '2', '0'):
    case mmioFOURCC('R', 'V', '3', '0'):
    case mmioFOURCC('R', 'V', '4', '0'):
        if(sh->bih->biSize<sizeof(*sh->bih)+8){
            /* only 1 packet per frame & sub_id from fourcc */
            avctx->extradata_size= 8;
            avctx->extradata = (uint8_t*)av_mallocz(avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
            ((uint32_t *)avctx->extradata)[0] = 0;
            ((uint32_t *)avctx->extradata)[1] =
                (sh->format == mmioFOURCC('R', 'V', '1', '3')) ? 0x10003001 : 0x10000000;
        } else {
            /* has extra slice header (demux_rm or rm->avi streamcopy) */
            avctx->extradata_size = sh->bih->biSize-sizeof(BITMAPINFOHEADER);
            avctx->extradata = (uint8_t*)av_mallocz(avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
            memcpy(avctx->extradata, sh->bih+1, avctx->extradata_size);
        }
    avctx->sub_id= AV_RB32(avctx->extradata+4);
    
//        printf("%X %X %d %d\n", extrahdr[0], extrahdr[1]);
    break;

    default:
        if (!sh->bih || sh->bih->biSize <= sizeof(BITMAPINFOHEADER))
            break;
        avctx->extradata_size = sh->bih->biSize-sizeof(BITMAPINFOHEADER);
        avctx->extradata = (uint8_t*)av_mallocz(avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
        memcpy(avctx->extradata, sh->bih+1, avctx->extradata_size);
        break;
    }
    /* Pass palette to codec */
    if (sh->bih && (sh->bih->biBitCount <= 8)) {
        avctx->palctrl = (AVPaletteControl*)calloc(1, sizeof(AVPaletteControl));
        avctx->palctrl->palette_changed = 1;
        if (sh->bih->biSize-sizeof(BITMAPINFOHEADER))
            /* Palette size in biSize */
            memcpy(avctx->palctrl->palette, sh->bih+1,
                   FFMIN(sh->bih->biSize-sizeof(BITMAPINFOHEADER), AVPALETTE_SIZE));
        else
            /* Palette size in biClrUsed */
            memcpy(avctx->palctrl->palette, sh->bih+1,
                   FFMIN(sh->bih->biClrUsed * 4, AVPALETTE_SIZE));
    }
    
    if(sh->bih)
        avctx->bits_per_coded_sample= sh->bih->biBitCount;

    /* open it */
    if (avcodec_open(avctx, lavc_codec) < 0) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Can't Open Codec");
        uninit(sh);
        return 0;
    }

    mFrame_Mem->muse_jz_buf=avctx->use_jz_buf;
    ALOGE("avctx->use_jz_buf=%d",avctx->use_jz_buf);
    sh->drvmpeg2 = 0;

    mp_msg(MSGT_DECVIDEO, MSGL_V, "INFO: libavcodec init OK!\n");
    return 1; //mpcodecs_config_vo(sh, sh->disp_w, sh->disp_h, IMGFMT_YV12);
}
    
void lumeDecoder::uninit(sh_video_t *sh){
    vd_lume_ctx *ctx = (vd_lume_ctx *)sh->context;
    AVCodecContext *avctx = ctx->avctx;
    
    if (avctx) {
        if (avctx->codec && avcodec_close(avctx) < 0)
            mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Can't Close Codec!");
	
        av_freep(&avctx->extradata);
        av_freep(&avctx->palctrl);
        av_freep(&avctx->slice_offset);
    }
    
    av_freep(&avctx);
    av_freep(&ctx->pic);
    if (ctx)
        free(ctx);   
    if(mFrame_Mem)
	delete mFrame_Mem;
    mFrame_Mem = NULL;
    avcodec_initialized = 0;
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

//static char * copy_bs=NULL;

int lumeDecoder::decode_video(sh_video_t *sh,unsigned char **inbuf,int *inslen,unsigned char* outbuf,int *outlen, int drop_frame){
    int got_picture=0;
    int ret;
    vd_lume_ctx *ctx = (vd_lume_ctx *)sh->context;
    AVFrame *pic= ctx->pic;
    AVCodecContext *avctx = ctx->avctx;
    mp_image_t *mpi=NULL;
    int dr1= ctx->do_dr1;
    AVPacket pkt;
    
    *outlen = 0;
    if(*inslen<=0) return NULL; // skipped frame
    
    //lume interlace (mpeg2) bug have been fixed. no need of -noslices
    
    avctx->opaque=sh;
    
    if(drop_frame > 0)
    {
        if(avctx)
        {
            if(dropped_frames < 50)
                avctx->skip_frame = AVDISCARD_NONREF;
            else if(dropped_frames < 100)
                avctx->skip_frame = AVDISCARD_NONKEY;
            else
                avctx->skip_frame = AVDISCARD_ALL;
            avctx->hurry_up = 1;
            dropped_frames++;
        }
    }
    else
    {
        if(avctx)
        {
            avctx->skip_frame = AVDISCARD_DEFAULT;
            avctx->hurry_up = 0;
            dropped_frames = 0;
        }
            
    }

    *outlen = 0;

    EL("-------------%x %d",*inbuf,*inslen);
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
    
    if(!copy_bs){
      copy_bs=(char*)jz4740_alloc_frame(avctx->VpuMem_ptr,32,0x100000);
    }
    
    uint8_t *p = (uint8_t*)(/**((int *)*/*inbuf);
       
    memcpy(copy_bs,p,*inslen);
    if(sh->ds){
    sh->ds->need_free += 1;
    if(sh->ds->seek_flag > 0){
      avcodec_flush_buffers(avctx);
      sh->ds->seek_flag = 0;
      sh->mSeek = 1;
      sh->seekFlag = 0;
    }
    }
    if(sh->seekFlag > 0){
      avcodec_flush_buffers(avctx);
      sh->mSeek = 1;
      sh->seekFlag = 0;
    }
    {
      AVPacket avpkt;
      av_init_packet(&avpkt);
      avpkt.data = (uint8_t*)copy_bs;
      avpkt.size = *inslen;
      avpkt.pts = (int64_t)(sh->pts*1000000.0);
      //      ALOGE("avpkt.pts=%lld",avpkt.pts);
      // HACK for CorePNG to decode as normal PNG by default
      avpkt.flags = AV_PKT_FLAG_KEY;
      
      ret = avcodec_decode_video2(avctx, pic,&got_picture, &avpkt);
      if (ret == -99 && memcmp(sh->codec->dll, "h264", 4) == 0){
	ALOGE("avcodec_decode_video return is %d %s", ret, sh->codec->dll);
	sh->is_2648x8 = 1;
      }
    }
    
    if(avctx->use_jz_buf_change)
      mFrame_Mem->muse_jz_buf=avctx->use_jz_buf;

    *inbuf += *inslen;
    *inslen = 0;
    if(ret<0){		
#if  DUMP_DATA==1
    //dump_file(dump_duf,dump_len,framenum);
	
#endif
	ALOGE("Error while decoding frame!\n");
	mp_msg(MSGT_DECVIDEO, MSGL_WARN, "Error while decoding frame!\n");
	return NULL;        // skipped image
    }

    if(!got_picture){
      ALOGE("got_picture=0");
      return NULL;        // skipped image
    }	
    if(dr1 && pic->opaque){
	mpi= (mp_image_t *)pic->opaque;
    }	
#if 1
    if(!mpi)
    {
      mpi = mFrame_Mem->get_image(avctx->VpuMem_ptr,sh->codec->outfmt[sh->outfmtidx], MP_IMGTYPE_EXPORT, 
				  MP_IMGFLAG_PRESERVE, avctx->width, avctx->height);
    }
    if(!mpi){        // temporary!
	ALOGE("no memory!");
        return NULL;
    }
#endif

    if(mpi)
    {	
        mpi->planes[0]=pic->data[0];
        mpi->planes[1]=pic->data[1];
        mpi->planes[2]=pic->data[2];
        mpi->planes[3]=pic->data[3];
        mpi->stride[0]=pic->linesize[0];
        mpi->stride[1]=pic->linesize[1];
        mpi->stride[2]=pic->linesize[2];
        mpi->stride[3]=pic->linesize[3];
	mpi->pts=pic->pts;

	if(pic->memheapbase[0] != NULL){
	  for(int i = 0; i < 4; ++i){
	    mpi->memheapbase[i] = pic->memheapbase[i];
	    mpi->memheapbase_offset[i] = pic->memheapbase_offset[i];
	  }
	}

	//ALOGE("-------------pic base[0]:%p, base[1]:%p,  ->memheapbase[0]:%p, offset[0]:%d; memheapbase[1]:%p, offset[1]:%d;", pic->data[0], pic->data[1], pic->memheapbase[0], pic->memheapbase_offset[0], pic->memheapbase[1], pic->memheapbase_offset[1]);
    }

    if(avctx->pix_fmt==PIX_FMT_YUV422P && mpi->chroma_y_shift==1){
        // we have 422p but user wants 420p
        mpi->stride[1] *= 2;
        mpi->stride[2] *= 2;
    }

    /* to comfirm with newer lavc style */
    mpi->qscale = (char *)pic->qscale_table;
    mpi->qstride = pic->qstride;
    mpi->pict_type = pic->pict_type;
    mpi->qscale_type = pic->qscale_type;
    mpi->fields = MP_IMGFIELD_ORDERED;
    if(pic->interlaced_frame) mpi->fields |= MP_IMGFIELD_INTERLACED;
    if(pic->top_field_first ) mpi->fields |= MP_IMGFIELD_TOP_FIRST;
    if(pic->repeat_pict == 1) mpi->fields |= MP_IMGFIELD_REPEAT_FIRST;
    
    return (int)mpi;
}


int lumeDecoder::control(sh_video_t *sh,int cmd,void* arg, ...){
    vd_lume_ctx *ctx = (vd_lume_ctx *)sh->context;
    AVCodecContext *avctx = (AVCodecContext *)ctx->avctx;
    switch(cmd){
    case VDCTRL_QUERY_FORMAT:
    {
        int format =(*((int *)arg));
        if(format == ctx->best_csp) return CONTROL_TRUE;//supported
        // possible conversions:
        switch(format){
        case IMGFMT_YV12:
        case IMGFMT_IYUV:
        case IMGFMT_I420:
            // "converted" using pointer/stride modification
            if(avctx->pix_fmt==PIX_FMT_YUV420P) return CONTROL_TRUE;// u/v swap
            if(avctx->pix_fmt==PIX_FMT_YUV422P && !ctx->do_dr1) return CONTROL_TRUE;// half stride
            break;
#if CONFIG_XVMC
        case IMGFMT_XVMC_IDCT_MPEG2:
        case IMGFMT_XVMC_MOCO_MPEG2:
            if(avctx->pix_fmt==PIX_FMT_XVMC_MPEG2_IDCT) return CONTROL_TRUE;
#endif
        }
        return CONTROL_FALSE;
    }
    case VDCTRL_RESYNC_STREAM:
        avcodec_flush_buffers(avctx);
        return CONTROL_TRUE;
    case VDCTRL_QUERY_UNSEEN_FRAMES:
        return avctx->has_b_frames + 10;
    }
    return CONTROL_UNKNOWN;

}

vd_info_t lumeDecoder::m_info = {
    "FFmpeg's libavcodec codec family",
    "ffmpeg",
    "A'rpi",
    "A'rpi, Michael, Alex",
    "native codecs"
};
  //}
}
