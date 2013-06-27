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

#include "pcm_decoder.h"

#include "config.h"
extern "C"{
//#include "ad_internal.h"
#include "libaf/af_format.h"
#include "libaf/reorder_ch.h"
}

#define CONVERT_TO_CLASS(x) ((mpDecorder*)(x))
#include <utils/Log.h>
#define EL(x,y...) //{ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y); }
#define mp_msg(w,x,y,z...) ALOGE(y,##z)

namespace android{

PcmDecoder::PcmDecoder()
{
	audio_output_channels = 2;
    buffer = (unsigned char *)malloc(81920);
	a_in_buffer_len = 0;
}

PcmDecoder::~PcmDecoder()
{
	EL("");
    free(buffer);
    
}

int PcmDecoder::preinit(sh_audio_t *sh)
{
    sh->audio_out_minsize=2048;

    return 1;
}


int PcmDecoder::init(sh_audio_t *sh_audio)
{
    WAVEFORMATEX *h=sh_audio->wf;
    if (!h)
        return 0;
    sh_audio->i_bps=h->nAvgBytesPerSec;
    sh_audio->channels=h->nChannels;
    sh_audio->samplerate=h->nSamplesPerSec;
    sh_audio->samplesize=(h->wBitsPerSample+7)/8;
    sh_audio->sample_format=AF_FORMAT_S16_LE; // default
    switch(sh_audio->format){ /* hardware formats: */
    case 0x0:
    case 0x1: // Microsoft PCM
    case 0xfffe: // Extended
        switch (sh_audio->samplesize) {
        case 1: sh_audio->sample_format=AF_FORMAT_U8; break;
        case 2: sh_audio->sample_format=AF_FORMAT_S16_LE; break;
        case 3: sh_audio->sample_format=AF_FORMAT_S24_LE; break;
        case 4: sh_audio->sample_format=AF_FORMAT_S32_LE; break;
        }
        break;
    case 0x3: // IEEE float
        sh_audio->sample_format=AF_FORMAT_FLOAT_LE;
        break;
    case 0x6:  sh_audio->sample_format=AF_FORMAT_A_LAW;break;
    case 0x7:  sh_audio->sample_format=AF_FORMAT_MU_LAW;break;
    case 0x11: sh_audio->sample_format=AF_FORMAT_IMA_ADPCM;break;
    case 0x50: sh_audio->sample_format=AF_FORMAT_MPEG2;break;
/*    case 0x2000: sh_audio->sample_format=AFMT_AC3; */
    case 0x20776172: // 'raw '
        sh_audio->sample_format=AF_FORMAT_S16_BE;
        if(sh_audio->samplesize==1) sh_audio->sample_format=AF_FORMAT_U8;
        break;
    case 0x736F7774: // 'twos'
        sh_audio->sample_format=AF_FORMAT_S16_BE;
        // intended fall-through
    case 0x74776F73: // 'sowt'
        if(sh_audio->samplesize==1) sh_audio->sample_format=AF_FORMAT_S8;
        break;
    case 0x32336c66: // 'fl32', bigendian float32
        sh_audio->sample_format=AF_FORMAT_FLOAT_BE;
        sh_audio->samplesize=4;
        break;
    case 0x666c3332: // '23lf', little endian float32, MPlayer internal fourCC
        sh_audio->sample_format=AF_FORMAT_FLOAT_LE;
        sh_audio->samplesize=4;
        break;
/*    case 0x34366c66: // 'fl64', bigendian float64
       sh_audio->sample_format=AF_FORMAT_FLOAT_BE;
       sh_audio->samplesize=8;
       break;
    case 0x666c3634: // '46lf', little endian float64, MPlayer internal fourCC
       sh_audio->sample_format=AF_FORMAT_FLOAT_LE;
       sh_audio->samplesize=8;
       break;*/
    case 0x34326e69: // 'in24', bigendian int24
        sh_audio->sample_format=AF_FORMAT_S24_BE;
        sh_audio->samplesize=3;
        break;
    case 0x696e3234: // '42ni', little endian int24, MPlayer internal fourCC
        sh_audio->sample_format=AF_FORMAT_S24_LE;
        sh_audio->samplesize=3;
        break;
    case 0x32336e69: // 'in32', bigendian int32
        sh_audio->sample_format=AF_FORMAT_S32_BE;
        sh_audio->samplesize=4;
        break;
    case 0x696e3332: // '23ni', little endian int32, MPlayer internal fourCC
        sh_audio->sample_format=AF_FORMAT_S32_LE;
        sh_audio->samplesize=4;
        break;
    default: if(sh_audio->samplesize!=2) sh_audio->sample_format=AF_FORMAT_U8;
    }
    if (!sh_audio->samplesize) // this would cause MPlayer to hang later
        sh_audio->samplesize = 2;
    return 1;
}

void PcmDecoder::uninit(sh_audio_t *sh)
{
}

int PcmDecoder::control(sh_audio_t *sh,int cmd,void* arg, ...)
{
    int skip;
    switch(cmd)
    {
      case ADCTRL_SKIP_FRAME:
	skip=sh->i_bps/16;
	skip=skip&(~3);
//	demux_read_data(sh->ds,NULL,skip);
	return CONTROL_TRUE;
    }
    return CONTROL_UNKNOWN;
}


int PcmDecoder::decode_audio(sh_audio_t *sh_audio,unsigned char **inbuf,int *inlen,unsigned char* outbuf,int *outlen)
{
    *outlen = 0;
    
    if (*inlen > 0 && sh_audio->channels >= 5) {
        reorder_channel_nch(*inbuf, AF_CHANNEL_LAYOUT_WAVEEX_DEFAULT,
                            AF_CHANNEL_LAYOUT_MPLAYER_DEFAULT,
                            sh_audio->channels,
                            *inlen / sh_audio->samplesize, sh_audio->samplesize);
    }
//    *outlen = *inlen/sh_audio->samplesize/sh_audio->channels*2;

    //  memcpy(outbuf,*inbuf,*outlen*sh_audio->samplesize);
#if 1
    if (sh_audio->sample_format == AF_FORMAT_S16_BE){
      unsigned char *tbuf = outbuf;
      int i;
      for (i = 0; i < *inlen; i+=2){
	tbuf[0] = (*inbuf)[i+1];
	tbuf[1] = (*inbuf)[i];
	tbuf+=2;
      }
    }else if(sh_audio->sample_format == AF_FORMAT_S24_LE){
      int i,n=*inlen/3;
      uint8_t *in=*inbuf;
      for(i=0;i<n;i++){
	in++;
	*outbuf++=*in++;
	*outbuf++=*in++;
      }
    }else{
      memcpy(outbuf,*inbuf,*inlen);
    }
#else
    memcpy(outbuf,*inbuf,*inlen);
#endif
    /*outlen is out sample num*/
    *outlen = *inlen/sh_audio->samplesize;
    *inbuf += *inlen;
    *inlen = 0;
    return *outlen;
}

ad_info_t PcmDecoder::m_info = {
	"Uncompressed PCM audio decoder",
	"pcm",
	"Nick Kurshev",
	"A'rpi",
	""
};

}
