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
#ifndef LUME_AUDIO_DEC_H_INCLUDED
#define LUME_AUDIO_DEC_H_INCLUDED


#include <OMX_Component.h>

#include "mp_decoder.h"
#include "lume_decoder.h"

namespace android{

class DecFactor
{
public:
    DecFactor();
    virtual ~DecFactor();
	mpDecorder* CreateAudioDecoder(char *drv);
private:
	mpDecorder * m_dec;
};

class AudioDecoder{
public:
	AudioDecoder();
	virtual ~AudioDecoder();
	OMX_BOOL ALumeDecInit(OMX_AUDIO_CONFIG_EQUALIZERTYPE* aEqualizerType);
	void ALumeDecDeinit();
	int DecodeAudio(OMX_S16* aOutBuff,
                           OMX_U32* aOutputLength, OMX_U8** aInputBuf,
                           OMX_U32* aInBufSize,
                           OMX_S32* aFrameCount,
                           OMX_AUDIO_PARAM_PCMMODETYPE* aAudioPcmParam,
                           OMX_BOOL aMarkerFlag,
                           OMX_BOOL* aResizeFlag);
	
	void ResetDecoder(); // for repositioning
	OMX_S32 iInputUsedLength;
	OMX_S32 FindAudioCodec(sh_audio_t *sh_audio);
	OMX_BOOL AudioDecSetConext(sh_audio_t *sh);
	
	sh_audio_t* GetAudiosh(){
	    return shContext;
	}

private:
	int init_audio_codec(sh_audio_t *sh_audio);
	void uninit_audio(sh_audio_t *sh_audio);
	DecFactor decFactor;
	int iInitFlag;
	unsigned int iSaveOutputLen;
	sh_audio_t *shContext;
	
};

}
#endif  //#ifndef LUME_AUDIO_DEC_H_INCLUDED

