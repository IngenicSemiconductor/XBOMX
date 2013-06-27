/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include "lume_decoder.h"

#define CONVERT_TO_CLASS(x) ((mpDecorder*)(x))
#include <utils/Log.h>
#define EL(x,y...) //{ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y); }
//#define EL(x,y...)

extern "C"{
    int avcodec_initialized = 0;
    int cooknplaying = 0;
    void audio_avcodec_register_all(int);
    void init_avcodec(void);
    int ds_parse_audio(demux_stream_t *sh, uint8_t **buffer, int *len, double pts, loff_t pos);
    void ds_clear_parser_audio(sh_audio_t *sh);
}

namespace android{

lumeDecoder::lumeDecoder()
{
	audio_output_channels = 2;
	aframe_cnt=0;
	avcodec_initialized=0;
}

lumeDecoder::~lumeDecoder()
{
}

int lumeDecoder::preinit(sh_audio_t *sh)
{
	sh->audio_out_minsize=AVCODEC_MAX_AUDIO_FRAME_SIZE;

	return 1;
}

static int setup_format(sh_audio_t *sh_audio, const AVCodecContext *lavc_context)
{
    int samplerate = lavc_context->sample_rate;
    int sample_format = sh_audio->sample_format;
    switch (lavc_context->sample_fmt) {
        case SAMPLE_FMT_U8:  sample_format = AF_FORMAT_U8;       break;
        case SAMPLE_FMT_S16: sample_format = AF_FORMAT_S16_NE;   break;
        case SAMPLE_FMT_S32: sample_format = AF_FORMAT_S32_NE;   break;
        case SAMPLE_FMT_FLT: sample_format = AF_FORMAT_FLOAT_NE; break;
        default:
            mp_msg(MSGT_DECAUDIO, MSGL_FATAL, "Unsupported sample format\n");
    }
    if(sh_audio->wf){
        // If the decoder uses the wrong number of channels all is lost anyway.
        // sh_audio->channels=sh_audio->wf->nChannels;

	//AAC SBR sample could be settled by the passing out para of aac_real_samplerate.
	/*
        if (lavc_context->codec_id == CODEC_ID_AAC &&
            samplerate == 2*sh_audio->wf->nSamplesPerSec) {
            mp_msg(MSGT_DECAUDIO, MSGL_WARN,
                   "Ignoring broken container sample rate for ACC with SBR\n");
        } else
	*/
	if ((lavc_context->codec_id != CODEC_ID_AAC) && (sh_audio->wf->nSamplesPerSec))
	    samplerate=sh_audio->wf->nSamplesPerSec;
    }
    if (lavc_context->channels != sh_audio->channels ||
        samplerate != sh_audio->samplerate ||
        sample_format != sh_audio->sample_format) {
        sh_audio->channels=lavc_context->channels;
        sh_audio->samplerate=samplerate;
        sh_audio->sample_format = sample_format;
        sh_audio->samplesize=af_fmt2bits(sh_audio->sample_format)/ 8;

        return 1;
    }

    return 0;
}

void init_avcodec(void)
{
    if (!avcodec_initialized) {
        avcodec_init();
        audio_avcodec_register_all(cooknplaying);
        avcodec_initialized = 1;
    }
}

int lumeDecoder::init(sh_audio_t *sh_audio)
{
    int tries = 0;
    int x;
    AVCodecContext *lavc_context;
    AVCodec *lavc_codec;

    mp_msg(MSGT_DECAUDIO,MSGL_V,"Lume's libavcodec audio codec\n");
    if(sh_audio->mCookNplaying)
      cooknplaying = 1;
    else
      cooknplaying = 0;
    init_avcodec();

    lavc_codec = avcodec_find_decoder_by_name(sh_audio->codec->dll);
    if(!lavc_codec){
        ALOGE("Error: Cannot find codec '%s' in libavcodec...",sh_audio->codec->dll);

        return 0;
    }

    /*
#ifdef LUME_VIDEO_REAL_SUPPORTED
#else
    if(lavc_codec->id==CODEC_ID_COOK)
      return 0;
#endif
    */

    lavc_context = avcodec_alloc_context();
    sh_audio->context=lavc_context;

    lavc_context->drc_scale = 1.0;
    lavc_context->sample_rate = sh_audio->samplerate;
    lavc_context->bit_rate = sh_audio->i_bps * 8;
    if(sh_audio->wf){
        lavc_context->channels = sh_audio->wf->nChannels;
        lavc_context->sample_rate = sh_audio->wf->nSamplesPerSec;
        lavc_context->bit_rate = sh_audio->wf->nAvgBytesPerSec * 8;
        lavc_context->block_align = sh_audio->wf->nBlockAlign;
        lavc_context->bits_per_coded_sample = sh_audio->wf->wBitsPerSample;
    }
    lavc_context->request_channels = audio_output_channels;
    lavc_context->codec_tag = sh_audio->format; //FOURCC
    lavc_context->codec_type = CODEC_TYPE_AUDIO;
    lavc_context->codec_id = lavc_codec->id; // not sure if required, imho not --A'rpi

    /* alloc extra data */
    if (sh_audio->wf && sh_audio->wf->cbSize > 0) {
        lavc_context->extradata = (uint8_t*)av_mallocz(sh_audio->wf->cbSize + FF_INPUT_BUFFER_PADDING_SIZE);
        lavc_context->extradata_size = sh_audio->wf->cbSize;
        memcpy(lavc_context->extradata, (char *)sh_audio->wf + sizeof(WAVEFORMATEX),
               lavc_context->extradata_size);
    }

    // for QDM2
    if (sh_audio->codecdata_len && sh_audio->codecdata && !lavc_context->extradata)
    {
        lavc_context->extradata = (uint8_t*)av_malloc(sh_audio->codecdata_len);
        lavc_context->extradata_size = sh_audio->codecdata_len;
        memcpy(lavc_context->extradata, (char *)sh_audio->codecdata,
               lavc_context->extradata_size);
    }

    /* open it */
    if (avcodec_open(lavc_context, lavc_codec) < 0) {
        ALOGE("Error: Could not open codec.");

        return 0;
    }
    mp_msg(MSGT_DECAUDIO,MSGL_V,"INFO: libavcodec \"%s\" init OK!\n", lavc_codec->name);

    if(sh_audio->format==0x3343414D){
        // MACE 3:1
        sh_audio->ds->ss_div = 2*3; // 1 samples/packet
        sh_audio->ds->ss_mul = 2*sh_audio->wf->nChannels; // 1 byte*ch/packet
    } else if(sh_audio->format==0x3643414D){
	// MACE 6:1
	sh_audio->ds->ss_div = 2*6; // 1 samples/packet
	sh_audio->ds->ss_mul = 2*sh_audio->wf->nChannels; // 1 byte*ch/packet
    }

#if 0
    // Decode at least 1 byte:  (to get header filled)
    do {
        x=decode_audio(sh_audio,sh_audio->a_buffer,1,sh_audio->a_buffer_size);
    } while (x <= 0 && tries++ < 5);
    if(x>0) sh_audio->a_buffer_len=x;
#endif
   
    sh_audio->i_bps=lavc_context->bit_rate/8;
    if (sh_audio->wf && sh_audio->wf->nAvgBytesPerSec)
        sh_audio->i_bps=sh_audio->wf->nAvgBytesPerSec;

    switch (lavc_context->sample_fmt) {
    case SAMPLE_FMT_U8:
    case SAMPLE_FMT_S16:
    case SAMPLE_FMT_S32:
    case SAMPLE_FMT_FLT:
        break;
    default:
        return 0;
    }

    return 1;
}

void lumeDecoder::uninit(sh_audio_t *sh)
{
    AVCodecContext *lavc_context = (AVCodecContext *)sh->context;
    
    if (lavc_context) 
    {
        if (avcodec_close(lavc_context) < 0)
            ALOGE("Error: Could not close codec.");

        av_freep(&lavc_context->extradata);
        av_freep(&lavc_context);
    }
}

int lumeDecoder::control(sh_audio_t *sh,int cmd,void* arg, ...)
{
    AVCodecContext *lavc_context = (AVCodecContext *)sh->context;
    
    switch(cmd){
    case ADCTRL_RESYNC_STREAM:
        avcodec_flush_buffers(lavc_context);
	return CONTROL_TRUE;
    }

    return CONTROL_UNKNOWN;
}


int lumeDecoder::decode_audio(sh_audio_t *sh_audio,unsigned char **inbuf,int *inlen,unsigned char* outbuf,int *outlen)
{
  //int len2 = *outlen * 2; // 16bit
    double pts;
    int x = *inlen;

    /*when some audio no do  parsing in demux_processing,
      then here will tell decodec to do paring before decodeced data*/
    ((AVCodecContext*)(sh_audio->context))->audio_needparing = sh_audio->need_parsing; //add by gysun	

#define DUMP_DATA 0
#if  DUMP_DATA==1
    unsigned char *dump_duf = (unsigned char *)*inbuf;
    static int framenum = 0;
    framenum++;
    unsigned int dump_len = x;
    if(framenum < 3)
    {
	dump_file(dump_duf,dump_len,framenum);
	EL11("ddddddddddddddddddddddddddddddddddddddddddddd");
    }
#endif

    //if(len2 < 192000)
	//len2 = 192000;
    unsigned char *out = (unsigned char *)outbuf;
    uint8_t *in = (uint8_t *)*inbuf;
    int ilen = *inlen;
    int len2 = 0;
    *outlen = 0;

    int y,len=-1;
    const int minlen = 96000;
    //int maxlen = 192000;
    int maxlen = 285000;
    unsigned char *start=NULL;
    while(len<minlen && ilen > 0){
	AVPacket pkt;
	len2=maxlen;
	
	av_init_packet(&pkt);
	pkt.data = *inbuf;
	pkt.size = ilen;

	if(sh_audio->ds && sh_audio->ds->seek_flag > 0){
	    avcodec_flush_buffers((AVCodecContext *)sh_audio->context);
	    sh_audio->ds->seek_flag = 0;
	}

	y=avcodec_decode_audio3((AVCodecContext *)sh_audio->context,(int16_t*)out,&len2,&pkt);
	EL("y=%d,len2=%d",y,len2);    
    
	if(y<0){
	  if(ilen==*inlen){
	    ALOGE("Error: decode audio failed!!!");
	    *inbuf += *inlen;
	    *inlen = 0;
	    return -1;
	  }else{
	    *inlen=0;
	    *outlen = len / 2;
	    aframe_cnt++;
	    return len;
	  }
	}
	
	ilen -= y;
	*inbuf += y;
	if(len2>0){

#if 0
	    if (((AVCodecContext *)sh_audio->context)->channels >= 5) {
		    int samplesize = av_get_bits_per_sample_format(((AVCodecContext *)
								    sh_audio->context)->sample_fmt) / 8;
		    
		    reorder_channel_nch(out, AF_CHANNEL_LAYOUT_LAVC_DEFAULT,
					AF_CHANNEL_LAYOUT_MPLAYER_DEFAULT,
					((AVCodecContext *)sh_audio->context)->channels,
					len2 / samplesize, samplesize);
	    }
#endif
	    if(len<0) 
		len=len2; 
	    else 
		len+=len2;
		
	    out+=len2;
	    maxlen -= len2;    
	}

	if (setup_format(sh_audio, (AVCodecContext *)sh_audio->context))
	    break;
	if(sh_audio->format == 0x20455041) //ape packet too large
	  break;
    }

    if(len2)
	aframe_cnt++;
    
    if(ilen >= 0)
	*inlen = ilen;	
    else
	*inlen = 0;
    
    if(len >= 0)
	*outlen = len / 2;//(av_get_bits_per_sample_format(((AVCodecContext *)sh_audio->context)->sample_fmt) / 8);
    else 
	return -1;
	
    return len;
}

ad_info_t lumeDecoder::m_info = {
    "FFmpeg/libavcodec audio decoders",
    "ffmpeg",
    "Nick Kurshev",
    "ffmpeg.sf.net",
    ""
};

}
