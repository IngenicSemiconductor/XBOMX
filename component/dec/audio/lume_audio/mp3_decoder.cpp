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

#include "mp3_decoder.h"

#include "config.h"
extern "C"{
#include "mad.h"
}

#define CONVERT_TO_CLASS(x) ((mpDecorder*)(x))
#include <utils/Log.h>
#define EL(x,y...) //{ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y); }
#define mp_msg(w,x,y,z...) ALOGE(y,##z)

namespace android{

typedef struct mad_decoder_s {

  struct mad_synth  synth;
  struct mad_stream stream;
  struct mad_frame  frame;

  int have_frame;

  int               output_sampling_rate;
  int               output_open;
  int               output_mode;

} mad_decoder_t;


Mp3Decoder::Mp3Decoder()
{
	audio_output_channels = 2;
    buffer = (unsigned char *)malloc(81920);
	a_in_buffer_len = 0;
}

Mp3Decoder::~Mp3Decoder()
{
	EL("");
    free(buffer);
    
}

int Mp3Decoder::preinit(sh_audio_t *sh)
{
    mad_decoder_t *mad_dec = (mad_decoder_t *)malloc(sizeof(mad_decoder_t));
    memset(mad_dec,0,sizeof(mad_decoder_t));
    sh->context = mad_dec;

    mad_synth_init  (&mad_dec->synth);
    mad_stream_init (&mad_dec->stream);
    mad_frame_init  (&mad_dec->frame);

    sh->audio_out_minsize=2*4608;
    sh->audio_in_minsize=4096;

    return 1;
}

/* utility to scale and round samples to 16 bits */
static inline signed int scale(mad_fixed_t sample) {
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

int Mp3Decoder::decode_frame(sh_audio_t *sh,unsigned char *outdata,int *outlen){
    mad_decoder_t *mad_dec = (mad_decoder_t *) sh->context;
    int len;
	unsigned char *inbuf = buffer;
	int inbuff_len = a_in_buffer_len;
	*outlen = 0;
	uint16_t *output = (uint16_t *)outdata; 
	int nChannels = 0;
	while(1){
		int ret;
//		EL("sh->a_in_buffer: %p, inbuff_len: %d",inbuf,inbuff_len);
		mad_dec->stream.error = MAD_ERROR_NONE;
		mad_stream_buffer (&mad_dec->stream, (unsigned char *)inbuf, inbuff_len);
//		EL("");
		ret=mad_frame_decode (&mad_dec->frame, &mad_dec->stream);
        	if (sh->samplerate == 0)//hpwang 2011-08-03 add for samplerate is zero while mpeg2 audio
		  sh->samplerate = mad_dec->frame.header.samplerate;
		nChannels = mad_dec->frame.header.mode ? 2 : 1;
		if (nChannels != sh->channels)
		  sh->channels = mad_dec->synth.pcm.channels;
//		ALOGE("RET: %d, stream error: %d",ret,mad_dec->stream.error);
//		EL("ret: %d, next_frame: %p",ret,mad_dec->stream.next_frame);

		//ALOGE("sh->channels = %d,sh->samplerate = %d",sh->channels,sh->samplerate);
		inbuff_len -= (mad_dec->stream.next_frame - inbuf); 
		inbuf = (unsigned char *)mad_dec->stream.next_frame;
		if(inbuff_len <= 0)
		{
			a_in_buffer_len = 0;
			return 0;
		}
		a_in_buffer_len = inbuff_len;
		if(a_in_buffer_len == 0)
			return (inbuf - buffer);

		if (ret == 0){
			mad_synth_frame (&mad_dec->synth, &mad_dec->frame);
			{
				unsigned int         nchannels, nsamples;
				mad_fixed_t const   *left_ch, *right_ch;
				struct mad_pcm      *pcm = &mad_dec->synth.pcm;
				
				nchannels = pcm->channels;
				nsamples  = pcm->length;
				left_ch   = pcm->samples[0];
				right_ch  = pcm->samples[1];
				*outlen += nchannels*nsamples;
				
				while (nsamples--) {
					/* output sample(s) in 16-bit signed little-endian PCM */
					*output++ = scale(*left_ch++);
					if (nchannels == 2)
						*output++ = scale(*right_ch++);
						
				}
			}
		}else if(!MAD_RECOVERABLE(mad_dec->stream.error))
		{
			if(mad_dec->stream.error == MAD_ERROR_BUFLEN)
			{
				memcpy(buffer, inbuf, a_in_buffer_len);
				return (inbuf - buffer);
			}
			else
			{
				//memcpy(buffer, inbuf, a_in_buffer_len);
				return 0;
			}
		}
	}
    mp_msg(MSGT_DECAUDIO,MSGL_INFO,"Cannot sync MAD frame\n");
    return 0;
}

int Mp3Decoder::init(sh_audio_t *sh_audio)
{
    mad_decoder_t *mad_dec = (mad_decoder_t *) sh_audio->context;

    mad_dec->have_frame = 0;
    
    //mad_dec->have_frame=read_frame(sh_audio);
    //if(!mad_dec->have_frame) return 0; // failed to sync...

    //sh_audio->channels=(mad_dec->frame.header.mode == MAD_MODE_SINGLE_CHANNEL) ? 1 : 2;
    //sh_audio->samplerate=mad_dec->frame.header.samplerate;
    //sh_audio->i_bps=mad_dec->frame.header.bitrate/8;
    sh_audio->samplesize=2;

    EL("channels: %d, bitrate: %d",sh_audio->channels,sh_audio->i_bps);
    
    return 1;
}

void Mp3Decoder::uninit(sh_audio_t *sh)
{
    mad_decoder_t *mad_dec = (mad_decoder_t *) sh->context;
    mad_synth_finish (&mad_dec->synth);
    mad_frame_finish (&mad_dec->frame);
    mad_stream_finish(&mad_dec->stream);
    free(sh->context);
}

int Mp3Decoder::control(sh_audio_t *sh,int cmd,void* arg, ...)
{
    mad_decoder_t *mad_dec = (mad_decoder_t *) sh->context;
    // various optional functions you MAY implement:
    switch(cmd){
    case ADCTRL_RESYNC_STREAM:
        mad_dec->have_frame=0;
        mad_synth_init  (&mad_dec->synth);
        mad_stream_init (&mad_dec->stream);
        mad_frame_init  (&mad_dec->frame);
        return CONTROL_TRUE;
    case ADCTRL_SKIP_FRAME:
        //mad_dec->have_frame=decode_frame(sh);
        //return CONTROL_TRUE;
		break;
    }
    return CONTROL_UNKNOWN;
}


int Mp3Decoder::decode_audio(sh_audio_t *sh_audio,unsigned char **inbuf,int *inlen,unsigned char* outbuf,int *outlen)
{
    mad_decoder_t *mad_dec = (mad_decoder_t *) sh_audio->context;
	int consume;
    *outlen=0;
    if(sh_audio->ds && sh_audio->ds->seek_flag > 0){
      mad_stream_init (&mad_dec->stream);
      a_in_buffer_len = 0;
      sh_audio->ds->seek_flag = 0;
    }
	//if(*inlen == 0)
	//	return 0;
	memcpy(buffer + a_in_buffer_len,*inbuf,*inlen);
	a_in_buffer_len += *inlen;
    *inbuf += *inlen;
	*inlen = 0;
    EL("a_in_buffer_len: %d",a_in_buffer_len);

	consume = decode_frame(sh_audio,outbuf,outlen);


    return *outlen;
}

ad_info_t Mp3Decoder::m_info = {
	"libmad mpeg audio decoder",
	"libmad",
	"A'rpi",
	"libmad...",
	"based on Xine's libmad/xine_decoder.c"
};

}
