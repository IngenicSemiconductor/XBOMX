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

#include <utils/threads.h>
#include <utils/String8.h>
#include <LUMEDefs.h>

#include "jzmedia.h"
#include "jzasm.h"

#include "lume_audio_dec.h"
#include "mp3_decoder.h"
#include "lume_decoder.h"
#include "faad_decoder.h"
#include "pcm_decoder.h"
#include "dvdpcm_decoder.h"

#define LOG_TAG "lume_audio_dec"
#include <utils/Log.h>

#define EL_P1(x,y...) //ALOGE(x,##y);
#define EL_P2(x,y...) //ALOGE(x,##y);
using namespace android;
extern "C" {
/*
 *exposed interface for libstagefright_soft_lume.so.
 */
AudioDecoder* CreateLUMEAudioDecoder(void){
  return new AudioDecoder();
}
  
OMX_BOOL ALumeDecInit(AudioDecoder*audioD){
  return audioD->ALumeDecInit(NULL);
}

void ALumeDecDeinit(AudioDecoder*audioD){
  return audioD->ALumeDecDeinit();
}

OMX_BOOL AudioDecSetConext(AudioDecoder*audioD,sh_audio_t *sh){
  return audioD->AudioDecSetConext(sh);
}

int DecodeAudio(AudioDecoder*audioD,
		     OMX_S16* aOutBuff,OMX_U32* aOutputLength,
		     OMX_U8** aInputBuf,OMX_U32* aInBufSize,
		     OMX_S32* aFrameCount,
		     OMX_AUDIO_PARAM_PCMMODETYPE* aAudioPcmParam,
		     OMX_BOOL aMarkerFlag,
		     OMX_BOOL* aResizeFlag){
  return audioD->DecodeAudio(aOutBuff,aOutputLength,
			     aInputBuf,aInBufSize,
			     aFrameCount,
			     aAudioPcmParam,
			     aMarkerFlag,aResizeFlag);
}
}
namespace android{

static int frmcnt = 0;
static int tst,ten;
static int readdata = 0;
static int ttime = 0;
#include <sys/time.h>

// Returns current time in microseconds
unsigned int GetTimer(void){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void consum(){
    if(frmcnt == 0)
    {
	tst = GetTimer();
	ttime = GetTimer() - tst;
    }

    frmcnt++;
    
    if(frmcnt == 100)
    {
	ten = GetTimer();
	ALOGE("frames = 100 time diff = %d audio decode time = %d",ten-tst,readdata);
	frmcnt = 0;
	readdata = 0;
    }
}

void difftime(int s){
    readdata += GetTimer() - s - ttime*2;
}


AudioDecoder::AudioDecoder(){
    shContext = new sh_audio_t;
    memset(shContext,0,sizeof(sh_audio_t));
    iInitFlag = 0;
}

AudioDecoder::~AudioDecoder(){
    uninit_audio(shContext);
    delete shContext;
}

OMX_BOOL AudioDecoder::AudioDecSetConext(sh_audio_t *sh){
    if(shContext)
    {				    
	memcpy(shContext,sh,sizeof(sh_audio_t));
	if(shContext->format == 0)
	    shContext->format = shContext->wf->wFormatTag;
	
	if(shContext->format != 0)
	{
	    if(shContext->format == 0)
		shContext->format = shContext->wf->wFormatTag;
	    
	    if(shContext->format != 0)
	    {
		parse_codec_cfg(NULL);
		
		if(FindAudioCodec(shContext))
		{
		    sh->i_bps=shContext->i_bps;
		    return OMX_TRUE;
		}else{
                    ALOGE("Error: No AudioCodec found!!!");

		    return OMX_FALSE;
		}
	    }else
	    {
		ALOGE("Error: sh format is 0!!!");

		return OMX_FALSE;
	    }
	}
    }

    return OMX_FALSE;
}

OMX_BOOL AudioDecoder::ALumeDecInit(OMX_AUDIO_CONFIG_EQUALIZERTYPE* aEqualizerType){
    S32I2M(xr16,7);
    return OMX_TRUE;
}

void AudioDecoder::ResetDecoder(){
}

void AudioDecoder::ALumeDecDeinit(){
}
    
int AudioDecoder::DecodeAudio(OMX_S16* aOutBuff,OMX_U32* aOutputLength,
			      OMX_U8** aInputBuf,OMX_U32* aInBufSize,
			      OMX_S32* aFrameCount,
			      OMX_AUDIO_PARAM_PCMMODETYPE* aAudioPcmParam,
			      OMX_BOOL aMarkerFlag,
			      OMX_BOOL* aResizeFlag){
    int32_t Status = ALUMEDEC_SUCCESS;
    int len;
    *aResizeFlag = OMX_FALSE;

    if(Status == ALUMEDEC_SUCCESS){
    
#ifdef DEBUG_AUDIODEC_COUNTED_BUFVALUE
    char in_str[128] = "audiodec inbuf:";
    for(int i = 0, j = 15; i < DEBUG_AUDIODEC_COUNTED_BUFVALUE; ++i){
	sprintf(&in_str[j], "%u ", (unsigned int)(*aInputBuf)[i]);
	j += strlen(&in_str[j]);
    }
    ALOGE("%s", in_str);
#endif

#ifdef DEBUG_AUDIODEC_COST_TIME
    unsigned int pre_time = GetTimer();
#endif

    if(shContext && shContext->ad_driver){	    
	len = ((mpDecorder*)(shContext->ad_driver))->decode_audio(shContext,(unsigned char **)aInputBuf,(int *)aInBufSize,(unsigned char *)aOutBuff,(int *)aOutputLength);   
    }
    else{
	ALOGE("Error:There is no shContext or shContext->ad_driver!!!");
	
	*aOutputLength = 0;
	len = -1;
	    
	*aInputBuf += *aInBufSize;
	*aInBufSize = 0;
    }
	
#ifdef DEBUG_AUDIODEC_COST_TIME
    ALOGE("audiodec costTime = %u us", GetTimer() - pre_time);
#endif

#ifdef DEBUG_AUDIODEC_COUNTED_BUFVALUE
    char out_str[128] = "audiodec outbuf:";
    OMX_U8* aOutputBuf = (OMX_U8*)aOutBuff;
    for(int i = 0, j = 16; i < DEBUG_AUDIODEC_COUNTED_BUFVALUE; ++i){
	sprintf(&out_str[j], "%u ", (unsigned int)aOutputBuf[i]);
	j += strlen(&out_str[j]);
    }
    ALOGE("%s", out_str);
#endif
	
    aAudioPcmParam->nSamplingRate = shContext->samplerate;
    aAudioPcmParam->nChannels = shContext->channels > 2 ? 2 : shContext->channels == 0 ? 2 : shContext->channels;
    if(shContext->samplesize > 2)
	aAudioPcmParam->nBitPerSample = 16;
    else
	aAudioPcmParam->nBitPerSample = shContext->samplesize*8;
    
    EL_P2("aAudioPcmParam->nBitPerSample=%d;samplerate:%d;channels:%d", aAudioPcmParam->nBitPerSample, aAudioPcmParam->nSamplingRate, aAudioPcmParam->nChannels);

    if(len < 0){
	Status = ALUMEDEC_INVALID_FRAME;
	*aOutputLength = 0;

	ALOGE("Error: Audio dec failed with negative out len!");
    }else{
	(*aFrameCount)++;

	EL_P2("Audio dec successfully!");
    }
    }
    
    return Status;
}

OMX_S32 AudioDecoder::FindAudioCodec(sh_audio_t *sh_audio){
    unsigned int orig_fourcc = sh_audio->wf ? sh_audio->wf->wFormatTag : 0;
    int force = 0;
    unsigned int i;
    sh_audio->codec = NULL;
    sh_audio->ad_driver = 0;

    // restore original fourcc:
    if (sh_audio->wf)
	sh_audio->wf->wFormatTag = i = orig_fourcc;

    while(1)
      {
	sh_audio->codec = find_audio_codec(sh_audio->format,
					   sh_audio->wf ? (&i) : NULL,
					   sh_audio->codec, force);
	if(!sh_audio->codec){
	  ALOGE("not find codec!");
	  return 0;
	}
	
	sh_audio->ad_driver = (ad_functions *)decFactor.CreateAudioDecoder(sh_audio->codec->drv);

	if(!sh_audio->ad_driver)
	  continue;
	ALOGE("sh_audio->codec->drv=%s dll=%s",sh_audio->codec->drv,sh_audio->codec->dll);	
	if (init_audio_codec(sh_audio)) {
	  break;
	}else
	  decFactor.~DecFactor();	
      }

    if (sh_audio->wf)
	sh_audio->wf->wFormatTag = i;

    return 1;
}

int AudioDecoder::init_audio_codec(sh_audio_t *sh_audio){
    if (!((mpDecorder*)(sh_audio->ad_driver))->preinit(sh_audio)) {
	return 0;
    }
    
    /* allocate audio in buffer: */
    if (sh_audio->audio_in_minsize > 0) {
	sh_audio->a_in_buffer_size = sh_audio->audio_in_minsize;
	sh_audio->a_in_buffer = NULL;//av_mallocz(sh_audio->a_in_buffer_size);
	sh_audio->a_in_buffer_len = 0;
    }
    
    sh_audio->a_buffer_size = sh_audio->audio_out_minsize + MAX_OUTBURST;
    sh_audio->a_buffer = NULL;//av_mallocz(sh_audio->a_buffer_size);
    
    if (!((mpDecorder*)(sh_audio->ad_driver))->init(sh_audio)) {
	uninit_audio(sh_audio);	// free buffers
	return 0;
    }

    if(!sh_audio->wf && !sh_audio->codecdata)
      return 1;
#if 0
    if (!sh_audio->channels || !sh_audio->samplerate ) {
	uninit_audio(sh_audio);	// free buffers
	return 0;
    }
#endif
    if (!sh_audio->o_bps)
	sh_audio->o_bps = sh_audio->channels * sh_audio->samplerate * sh_audio->samplesize;

    mp_msg(MSGT_DECAUDIO, MSGL_INFO,
	   "AUDIO: %d Hz, %d ch, %s, %3.1f kbit/%3.2f%% (ratio: %d->%d)\n",
	   sh_audio->samplerate, sh_audio->channels,
	   af_fmt2str_short(sh_audio->sample_format),
	   sh_audio->i_bps * 8 * 0.001,
	   ((float) sh_audio->i_bps / sh_audio->o_bps) * 100.0,
	   sh_audio->i_bps, sh_audio->o_bps);
    mp_msg(MSGT_IDENTIFY, MSGL_INFO,
	   "ID_AUDIO_BITRATE=%d\nID_AUDIO_RATE=%d\n" "ID_AUDIO_NCH=%d\n",
	   sh_audio->i_bps * 8, sh_audio->samplerate, sh_audio->channels);

    sh_audio->a_out_buffer_size = 0;
    sh_audio->a_out_buffer = NULL;
    sh_audio->a_out_buffer_len = 0;

    return 1;
}

void AudioDecoder::uninit_audio(sh_audio_t *sh_audio){
    if(sh_audio->ad_driver)
	((mpDecorder*)(sh_audio->ad_driver))->uninit(sh_audio);
}

mpDecorder* DecFactor::CreateAudioDecoder(char *drv){
    mpDecorder *newcodec;
    if(strcmp(lumeDecoder::m_info.short_name,drv) == 0){
	EL_P1("Audio dec create lumeDecoder\n");
        m_dec = new lumeDecoder;
    }

    if(strcmp(faadDecoder::m_info.short_name,drv) == 0){
        EL_P1("Audio dec create faadDecoder\n");
        m_dec = new faadDecoder;
    }

    if(strcmp(Mp3Decoder::m_info.short_name,drv) == 0){
        EL_P1("Audio dec create Mp3Decoder\n");
        m_dec = new Mp3Decoder;
    }

    if(strcmp(PcmDecoder::m_info.short_name,drv) == 0){
        EL_P1("Audio dec create PcmDecoder\n");
        m_dec = new PcmDecoder;
    }
    if(strcmp(DvdpcmDecoder::m_info.short_name,drv) == 0){
      EL_P1("Audio dec create DvdpcmDecoder\n");
      m_dec = new DvdpcmDecoder;
    }
    return m_dec;
}

DecFactor::DecFactor(){
    m_dec = NULL;
}

DecFactor:: ~DecFactor(){
  //if(strcmp(lumeDecoder::m_info.short_name,"lume") == 0)
    {
#if 0
	lumeDecoder *m = (lumeDecoder *)m_dec;
	if(m)
	{
	    free(m);
	}
#else
        delete m_dec;
#endif //by xqliang
	
    }
    
}
    
}
