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
#ifndef FAAD_DECODER_H_INCLUDED
#define FAAD_DECODER_H_INCLUDED

#include "mp_decoder.h"

#ifdef __cplusplus
extern "C"{
#include "faad.h"
}
#endif

namespace android{

class DecFactor;

class faadDecoder: public mpDecorder
{
public:
	
	faadDecoder();
	virtual  ~faadDecoder();	
	int preinit(sh_audio_t *sh);
	int init(sh_audio_t *sh);
	void uninit(sh_audio_t *sh);
	int control(sh_audio_t *sh,int cmd,void* arg, ...);
	int decode_audio(sh_audio_t *sh_audio,unsigned char **inbuf,int *inslen,unsigned char* outbuf,int *outlen);
	static ad_info_t m_info;
private:
	int init_firstDecoder(sh_audio_t *sh_audio,unsigned char **inbuf,int *inlen,unsigned char* outbuf,int *outlen);
	int aac_probe(unsigned char *buffer, int len);
	int aac_sync(sh_audio_t *sh);
	friend class DecFactor;
	int audio_output_channels;
	faacDecHandle faac_hdec;
	int initfaad;
	faacDecFrameInfo faac_finfo;

};

}
#endif  //#ifndef FAAD_DECODER_H_INCLUDED

