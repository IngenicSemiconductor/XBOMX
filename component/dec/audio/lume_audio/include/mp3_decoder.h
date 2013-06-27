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
#ifndef MP3_DECODER_H_INCLUDED
#define MP3_DECODER_H_INCLUDED

#include "mp_decoder.h"

namespace android{

#define ADCTRL_RESYNC_STREAM 1
#define CONTROL_TRUE		1
#define ADCTRL_SKIP_FRAME 2         /* skip block/frame, called while seeking! */
#define CONTROL_UNKNOWN -1


class DecFactor;

class Mp3Decoder: public mpDecorder
{
public:
	
	Mp3Decoder();
	virtual ~Mp3Decoder();	
	int preinit(sh_audio_t *sh);
	int init(sh_audio_t *sh);
	void uninit(sh_audio_t *sh);
	int control(sh_audio_t *sh,int cmd,void* arg, ...);
	int decode_audio(sh_audio_t *sh_audio,unsigned char **inbuf,int *inslen,unsigned char* outbuf,int *outlen);
	static ad_info_t m_info;
private:
    int decode_frame(sh_audio_t *sh,unsigned char *output,int *outlen);
    
	friend class DecFactor;
	int audio_output_channels;
    unsigned char * buffer;
    int a_in_buffer_len;
    int buffer_pos;
    
    int fakemono;
    
};

}
#endif  //#ifndef MP3_DECODER_H_INCLUDED

