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

#include "dvdpcm_decoder.h"

#include "config.h"
extern "C"{
//#include "ad_internal.h"
//#include "libaf/af_format.h"
//#include "libaf/reorder_ch.h"
}

#define CONVERT_TO_CLASS(x) ((mpDecorder*)(x))
#include <utils/Log.h>
#define EL(x,y...) //{ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y); }
#define mp_msg(w,x,y,z...) ALOGE(y,##z)

namespace android{

DvdpcmDecoder::DvdpcmDecoder()
{
  //audio_output_channels = 2;
  // buffer = (unsigned char *)malloc(81920);
  //	a_in_buffer_len = 0;
}

DvdpcmDecoder::~DvdpcmDecoder()
{
  //	EL("");
  // free(buffer);
    
}

int DvdpcmDecoder::preinit(sh_audio_t *sh)
{
    sh->audio_out_minsize=2048;

    return 1;
}


int DvdpcmDecoder::init(sh_audio_t *sh)
{
    sh->i_bps = 0;
    if(sh->codecdata_len==3){
	// we have LPCM header:
	unsigned char h=sh->codecdata[1];
	sh->channels=1+(h&7);
	switch((h>>4)&3){
	case 0: sh->samplerate=48000;break;
	case 1: sh->samplerate=96000;break;
	case 2: sh->samplerate=44100;break;
	case 3: sh->samplerate=32000;break;
	}
	switch ((h >> 6) & 3) {
	  case 0:
	    ALOGE("AF_FORMAT_S16_BE");
	    sh->sample_format = AF_FORMAT_S16_BE;
	    sh->samplesize = 2;
	    break;
	  case 1:
	    // mp_msg(MSGT_DECAUDIO, MSGL_INFO, MSGTR_SamplesWanted);
	    sh->i_bps = sh->channels * sh->samplerate * 5 / 2;
	  case 2:
	    sh->sample_format = AF_FORMAT_S24_BE;
	    sh->samplesize = 3;
 ALOGE("AF_FORMAT_S24_BE");
	    break;
	  default:
	    sh->sample_format = AF_FORMAT_S16_BE;
	    sh->samplesize = 2;
 ALOGE("AF_FORMAT_S16_BE2");
	}
    } else {
	// use defaults:
	sh->channels=2;
	sh->samplerate=48000;
	sh->sample_format = AF_FORMAT_S16_BE;
 ALOGE("AF_FORMAT_S16_BE3");
	sh->samplesize = 2;

    }
    if (!sh->i_bps)
    sh->i_bps = sh->samplesize * sh->channels * sh->samplerate;
    return 1;  
}

void DvdpcmDecoder::uninit(sh_audio_t *sh)
{
}

int DvdpcmDecoder::control(sh_audio_t *sh,int cmd,void* arg, ...)
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


int DvdpcmDecoder::decode_audio(sh_audio_t *sh_audio,unsigned char **inputbuf,int *inlen,unsigned char* buf,int *outlen)
{
  int j,len;
  int consume_len = 0;
  unsigned char *inbuf = *inputbuf;
  if (sh_audio->samplesize == 3) {
    if (((sh_audio->codecdata[1] >> 6) & 3) == 1) {
      // 20 bit
      // not sure if the "& 0xf0" and "<< 4" are the right way around
      // can somebody clarify?
      for (j = 0; j < *inlen; j += 12) {
        char tmp[10];
        //len = demux_read_data(sh_audio->ds, tmp, 10);
        //if (len < 10) break;
	memcpy(tmp, inbuf, 10);
	inbuf += 10;
	consume_len += 10;
	if(consume_len > *inlen) break;
        // first sample
        buf[j + 0] = tmp[0];
        buf[j + 1] = tmp[1];
        buf[j + 2] = tmp[8] & 0xf0;
        // second sample
        buf[j + 3] = tmp[2];
        buf[j + 4] = tmp[3];
        buf[j + 5] = tmp[8] << 4;
        // third sample
        buf[j + 6] = tmp[4];
        buf[j + 7] = tmp[5];
        buf[j + 8] = tmp[9] & 0xf0;
        // fourth sample
        buf[j + 9] = tmp[6];
        buf[j + 10] = tmp[7];
        buf[j + 11] = tmp[9] << 4;
      }
      len = j;
    } else {
      // 24 bit
      for (j = 0; j < *inlen; j += 12) {
        char tmp[12];
	//len = demux_read_data(sh_audio->ds, tmp, 12);
        //if (len < 12) break;
	memcpy(tmp, inbuf, 12);
	inbuf += 12;
	consume_len += 12;
	if(consume_len > *inlen) break;
        // first sample
        buf[j + 0] = tmp[0];
        buf[j + 1] = tmp[1];
        buf[j + 2] = tmp[8];
        // second sample
        buf[j + 3] = tmp[2];
        buf[j + 4] = tmp[3];
        buf[j + 5] = tmp[9];
        // third sample
        buf[j + 6] = tmp[4];
        buf[j + 7] = tmp[5];
        buf[j + 8] = tmp[10];
        // fourth sample
        buf[j + 9] = tmp[6];
        buf[j + 10] = tmp[7];
        buf[j + 11] = tmp[11];
      }
      len = j;
    }
  } else
    {
      if (sh_audio->sample_format == AF_FORMAT_S16_BE){
	unsigned char *tbuf = buf;
	int i;
	for (i = 0; i < *inlen; i+=2){
	  tbuf[0] = (*inputbuf)[i+1];
	  tbuf[1] = (*inputbuf)[i];
	  tbuf+=2;
	}
      }else if(sh_audio->sample_format == AF_FORMAT_S24_BE){
        int i,n=*inlen/3;
	unsigned char *in=*inputbuf;
	unsigned char *tbuf = buf;	
	for(i=0;i<n;i++){
	  in++;
	  tbuf[1]=*in++;
	  tbuf[0]=*in++;
	  tbuf+=2;
	}
      }
      else{
	memcpy(buf, *inputbuf, *inlen);
	}
      len = *inlen;
    }

  *inputbuf += *inlen;
  *inlen = 0;
  if(len >= 0)
    *outlen = len / sh_audio->samplesize;
  else 
    return -1;
  return *outlen;
}

ad_info_t DvdpcmDecoder::m_info = {
	"Uncompressed DVD/VOB LPCM audio decoder",
	"dvdpcm",
	"Nick Kurshev",
	"A'rpi",
	""
};

}
