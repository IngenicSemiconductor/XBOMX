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

#include "faad_decoder.h"

#define FAAD_MAX_CHANNELS 8
#define FAAD_BUFFLEN (FAAD_MIN_STREAMSIZE*FAAD_MAX_CHANNELS)

#define ADCTRL_RESYNC_STREAM 1       /* resync, called after seeking! */
#define CONTROL_TRUE 1
#define CONTROL_UNKNOWN -1

#define CONVERT_TO_CLASS(x) ((mpDecorder*)(x))
#include <utils/Log.h>

//#define EL(x,y...) printf("%s %d",__FILE__,__LINE__);printf(x,##y);printf("\n");//{ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y); }
#define EL(x,y...)
#define mp_msg(w,x,y,z...) ALOGE(y,##z)
#define mp_dbg(w,x,y,z...) ALOGE(y,##z)

namespace android{

faadDecoder::faadDecoder()
{
	audio_output_channels = 2;
	faac_hdec = 0;
	initfaad = 0;
}

faadDecoder::~faadDecoder()
{
	EL("");
}

int faadDecoder::preinit(sh_audio_t *sh)
{
	sh->audio_out_minsize=8192*FAAD_MAX_CHANNELS;
	sh->audio_in_minsize=FAAD_BUFFLEN;
	return 1;
}

int faadDecoder::aac_probe(unsigned char *buffer, int len)
{

	int i = 0, pos = 0;
	mp_msg(MSGT_DECAUDIO,MSGL_V, "\nAAC_PROBE: %d bytes\n", len);
	while(i <= len-4) {
		if(
			((buffer[i] == 0xff) && ((buffer[i+1] & 0xf6) == 0xf0)) ||
			(buffer[i] == 'A' && buffer[i+1] == 'D' && buffer[i+2] == 'I' && buffer[i+3] == 'F')
			) {
			pos = i;
			break;
		}
		mp_msg(MSGT_DECAUDIO,MSGL_V, "AUDIO PAYLOAD: %x %x %x %x\n", buffer[i], buffer[i+1], buffer[i+2], buffer[i+3]);
		i++;
	}
	mp_msg(MSGT_DECAUDIO,MSGL_V, "\nAAC_PROBE: ret %d\n", pos);
	return pos;
}

int  faadDecoder::aac_sync(sh_audio_t *sh)
{

	int pos = 0;
#if 0
	if(!sh->codecdata_len) {
		if(sh->a_in_buffer_len < sh->a_in_buffer_size){
			sh->a_in_buffer_len +=
				demux_read_data(sh->ds,(unsigned char *)&sh->a_in_buffer[sh->a_in_buffer_len],
								sh->a_in_buffer_size - sh->a_in_buffer_len);
		}
		pos = aac_probe((unsigned char *)sh->a_in_buffer, sh->a_in_buffer_len);
		if(pos) {
			sh->a_in_buffer_len -= pos;
			memmove(sh->a_in_buffer, &(sh->a_in_buffer[pos]), sh->a_in_buffer_len);
			mp_msg(MSGT_DECAUDIO,MSGL_V, "\nAAC SYNC AFTER %d bytes\n", pos);
		}
	}
#endif
	return pos;
}

int faadDecoder::init(sh_audio_t *sh)
{

	unsigned long faac_samplerate;
	unsigned char faac_channels;
	int faac_init, pos = 0;

	faac_hdec = faacDecOpen();

	// If we don't get the ES descriptor, try manual config
	if(!sh->codecdata_len && sh->wf) {
		sh->codecdata_len = sh->wf->cbSize;
		sh->codecdata = (unsigned char *)malloc(sh->codecdata_len);
		memcpy(sh->codecdata, sh->wf+1, sh->codecdata_len);
		mp_msg(MSGT_DECAUDIO,MSGL_DBG2,"FAAD: codecdata extracted from WAVEFORMATEX\n");
	}
	if(sh->codecdata_len){ // We have ES DS in codecdata
        faacDecConfigurationPtr faac_conf = faacDecGetCurrentConfiguration(faac_hdec);
        if (audio_output_channels <= 2) {
            faac_conf->downMatrix = 1;
            faacDecSetConfiguration(faac_hdec, faac_conf);
        }

        /*int i;
          for(i = 0; i < sh_audio->codecdata_len; i++)
          printf("codecdata_dump %d: 0x%02X\n", i, sh_audio->codecdata[i]);*/
        
        faac_init = faacDecInit2(faac_hdec, sh->codecdata,
                                 sh->codecdata_len,	&faac_samplerate, &faac_channels);
    }
    if(faac_init < 0) {
        mp_msg(MSGT_DECAUDIO,MSGL_WARN,"FAAD: Failed to initialize the decoder!\n"); // XXX: deal with cleanup!
        faacDecClose(faac_hdec);
        // XXX: free a_in_buffer here or in uninit?
        return 0;
    } else {
        mp_msg(MSGT_DECAUDIO,MSGL_V,"FAAD: Decoder init done (%dBytes)!\n", sh->a_in_buffer_len); // XXX: remove or move to debug!
        mp_msg(MSGT_DECAUDIO,MSGL_V,"FAAD: Negotiated samplerate: %ldHz  channels: %d\n", faac_samplerate, faac_channels);
        // 8 channels is aac channel order #7.
        sh->channels = faac_channels == 7 ? 8 : faac_channels;
        if (audio_output_channels <= 2) sh->channels = faac_channels > 1 ? 2 : 1;
        sh->samplerate = faac_samplerate;
        sh->samplesize=2;
        //sh->o_bps = sh->samplesize*faac_channels*faac_samplerate;
        if(!sh->i_bps) {
            mp_msg(MSGT_DECAUDIO,MSGL_WARN,"FAAD: compressed input bitrate missing, assuming 128kbit/s!\n");
            sh->i_bps = 128*1000/8; // XXX: HACK!!! ::atmos
        } else
            mp_msg(MSGT_DECAUDIO,MSGL_V,"FAAD: got %dkbit/s bitrate from MP4 header!\n",sh->i_bps*8/1000);
		initfaad = 1;
	}
	
	return 1;
}

void faadDecoder::uninit(sh_audio_t *sh)
{
	mp_msg(MSGT_DECAUDIO,MSGL_V,"FAAD: Closing decoder!\n");
	if(faac_hdec)
		faacDecClose(faac_hdec);
}

int faadDecoder::control(sh_audio_t *sh,int cmd,void* arg, ...)
{

    switch(cmd)
    {
	case ADCTRL_RESYNC_STREAM:
		aac_sync(sh);
		return CONTROL_TRUE;
    }
	return CONTROL_UNKNOWN;
}

int faadDecoder::init_firstDecoder(sh_audio_t *sh_audio,unsigned char **inbuf,int *inlen,unsigned char* outbuf,int *outlen)
{

	unsigned long faac_samplerate;
	unsigned char faac_channels;
	int faac_init, pos = 0;
	if(!faac_hdec)
		faac_hdec = faacDecOpen();
	if(!sh_audio->codecdata_len)
	{
        faacDecConfigurationPtr faac_conf;
        /* Set the default object type and samplerate */
        /* This is useful for RAW AAC files */
        faac_conf = faacDecGetCurrentConfiguration(faac_hdec);
        if(sh_audio->samplerate)
            faac_conf->defSampleRate = sh_audio->samplerate;
        /* XXX: FAAD support FLOAT output, how do we handle
         * that (FAAD_FMT_FLOAT)? ::atmos
         */
        if (audio_output_channels <= 2) faac_conf->downMatrix = 1;
        switch(sh_audio->samplesize){
        case 1: // 8Bit
            mp_msg(MSGT_DECAUDIO,MSGL_WARN,"FAAD: 8Bit samplesize not supported by FAAD, assuming 16Bit!\n");
        default:
            sh_audio->samplesize=2;
        case 2: // 16Bit
            faac_conf->outputFormat = FAAD_FMT_16BIT;
            break;
        case 3: // 24Bit
            faac_conf->outputFormat = FAAD_FMT_24BIT;
            break;
        case 4: // 32Bit
            faac_conf->outputFormat = FAAD_FMT_32BIT;
            break;
        }
        //faac_conf->defObjectType = LTP; // => MAIN, LC, SSR, LTP available.

        faacDecSetConfiguration(faac_hdec, faac_conf);

#if CONFIG_FAAD_INTERNAL
        /* init the codec, look for LATM */
        faac_init = faacDecInit(faac_hdec, *inbuf,
                                *inlen, &faac_samplerate, &faac_channels,1);
        if (faac_init < 0 && *inlen >= 3 && sh_audio->format == mmioFOURCC('M', 'P', '4', 'L')) {
            // working LATM not found at first try, look further on in stream
            int i;
#if 0
            for (i = 0; i < 5; i++) {
                pos = *inlen-3;
                memmove(*inbuf, &(*inbuf[pos]), 3);
                *inlen  = 3;
                *inlen += demux_read_data(sh_audio->ds,&*inbuf[*inlen],
                                                       *inbuf_size - *inlen);
                faac_init = faacDecInit(faac_hdec, *inbuf,
                                        *inlen, &faac_samplerate, &faac_channels,1);
                if (faac_init >= 0) break;
            }
#endif
        }
#else
        /* external faad does not have latm lookup support */
        faac_init = faacDecInit(faac_hdec, *inbuf,
                                *inlen, &faac_samplerate, &faac_channels);
#endif
        
        if (faac_init < 0) {
            pos = aac_probe(*inbuf, *inlen);
            if(pos) {
                *inlen -= pos;
                *inbuf += pos;
                pos = 0;
            }

            /* init the codec */
#if CONFIG_FAAD_INTERNAL
            faac_init = faacDecInit(faac_hdec, *inbuf,
                                    *inlen, &faac_samplerate, &faac_channels,0);
#else
            faac_init = faacDecInit(faac_hdec, *inbuf,
                                    *inlen, &faac_samplerate, &faac_channels);
#endif
        }

        *inlen -= (faac_init > 0)?faac_init:0; // how many bytes init consumed
        // XXX FIXME: shouldn't we memcpy() here in a_in_buffer ?? --A'rpi

	}
	if(faac_init < 0) {
		mp_msg(MSGT_DECAUDIO,MSGL_WARN,"FAAD: Failed to initialize the decoder!\n"); // XXX: deal with cleanup!
		faacDecClose(faac_hdec);
		faac_hdec = 0;
		// XXX: free a_in_buffer here or in uninit?
		return 0;
	} else {
		mp_msg(MSGT_DECAUDIO,MSGL_V,"FAAD: Decoder init done (%dBytes)!\n", sh_audio->a_in_buffer_len); // XXX: remove or move to debug!
		mp_msg(MSGT_DECAUDIO,MSGL_V,"FAAD: Negotiated samplerate: %ldHz  channels: %d\n", faac_samplerate, faac_channels);
		sh_audio->channels = faac_channels;
		if (audio_output_channels <= 2) sh_audio->channels = faac_channels > 1 ? 2 : 1;
		sh_audio->samplerate = faac_samplerate;
		sh_audio->samplesize=2;
		//sh_audio->o_bps = sh_audio->samplesize*faac_channels*faac_samplerate;
		if(!sh_audio->i_bps) {
			mp_msg(MSGT_DECAUDIO,MSGL_WARN,"FAAD: compressed input bitrate missing, assuming 128kbit/s!\n");
			sh_audio->i_bps = 128*1000/8; // XXX: HACK!!! ::atmos
		} else 
			mp_msg(MSGT_DECAUDIO,MSGL_V,"FAAD: got %dkbit/s bitrate from MP4 header!\n",sh_audio->i_bps*8/1000);
	}  
  return 1;
}
#define MAX_FAAD_ERRORS 10
int faadDecoder::decode_audio(sh_audio_t *sh_audio,unsigned char **inbuf,int *inlen,unsigned char* outbuf,int *outlen)
{

	int len = 0, last_dec_len = 0, errors = 0;
	//  int j = 0;
	void *faac_sample_buffer;
    
	if(*inlen <= 0)
		return 0;
	if(initfaad == 0)
	{
		ALOGE("init faad &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&");
		initfaad = init_firstDecoder(sh_audio,inbuf,inlen,outbuf,outlen);
		if(initfaad == 0)
			return -1;
		if(*inlen <= 0)
			return 0;		

	}
	int ilen = *inlen;
	last_dec_len = 0;
	int bytesconsumed;	
	int olen = 0;
	unsigned char *in = *inbuf;
	
	in = *inbuf;
	do
	{
		//ALOGE("%x  %x",in,outbuf);
		//ALOGE("outsize = %d",obuflen);
		faac_sample_buffer = faacDecDecode2(faac_hdec, &faac_finfo, in, ilen,(void **)&outbuf,*outlen);
//		faac_sample_buffer = faacDecDecode(faac_hdec, &faac_finfo, in, ilen);
#if 0
		faac_finfo.bytesconsumed = ilen;
		faac_finfo.samples = 12280;
		faac_finfo.error = 0;
#endif
		bytesconsumed = faac_finfo.bytesconsumed;
		
		//ALOGE("bytesconsumed = %d ilen = %d",bytesconsumed,ilen);
		ilen -= bytesconsumed;
		in += bytesconsumed;

		if(faac_finfo.error > 0) {
			//ALOGE("FAAD: Failed to decode frame: %s \n",faacDecGetErrorMessage(faac_finfo.error));
			faacDecGetErrorMessage(faac_finfo.error);
			
		} else if (faac_finfo.samples == 0) {
			//break;
			ALOGE("FAAD: Decoded zero samples!\n");
			//break;
		} else {
			/* XXX: samples already multiplied by channels! */
			if (sh_audio->channels >= 5)
				mp_msg(MSGT_DECAUDIO,MSGL_DBG2,"FAAD: 5 channel not support now!\n");
			//else if(faac_sample_buffer)
			//	memcpy(outbuf,faac_sample_buffer, sh_audio->samplesize*faac_finfo.samples);
			//ALOGE("sh_audio->samplesize*faac_finfo.samples %d",sh_audio->samplesize*faac_finfo.samples);
			//ALOGE("faac_sample_buffer = %x",faac_sample_buffer);
			olen += sh_audio->samplesize*faac_finfo.samples; 
			outbuf += sh_audio->samplesize*faac_finfo.samples;

		}
        
	}while((ilen > 0) && bytesconsumed);
    
	if(ilen < 0) ilen = 0;
	*inbuf += *inlen - ilen;
	*inlen = ilen;

    if(olen == 0){
        *outlen = 0;
        olen = -1;
    }
    else
        *outlen = olen / sh_audio->samplesize;
    
	return olen;
}

ad_info_t faadDecoder::m_info = {
	"AAC (MPEG2/4 Advanced Audio Coding)",
	"faad",
	"Felix Buenemann",
	"faad2",
	"uses libfaad2"
};

}
