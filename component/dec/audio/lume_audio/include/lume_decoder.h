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
#ifndef LUME_DECODER_H_INCLUDED
#define LUME_DECODER_H_INCLUDED

#include "mp_decoder.h"


namespace android{

#define LUME_SUCCESS 1
#define LUME_ERROR   0

typedef enum
{
	ALUMEDEC_SUCCESS                 =  0,
	ALUMEDEC_INVALID_FRAME           =  1,
	ALUMEDEC_INCOMPLETE_FRAME        =  2,
	ALUMEDEC_LOST_FRAME_SYNC         =  4,
	ALUMEDEC_OUTPUT_BUFFER_TOO_SMALL =  8,
	ALUMEDEC_CONTINUE_BUFFER            =  16,
	ALUMEDEC_EXCEPTION_OCCUR            =  32,
} tPVALUMEDecoderErrorCode;

typedef enum
{
	flat       = 0,
	bass_boost = 1,
	rock       = 2,
	pop        = 3,
	jazz       = 4,
	classical  = 5,
	talk       = 6,
	flat_      = 7
	
} e_equalization;

#define CONTROL_OK 1
#define CONTROL_TRUE 1
#define CONTROL_FALSE 0
#define CONTROL_UNKNOWN -1
#define CONTROL_ERROR -2
#define CONTROL_NA -3

// fallback if ADCTRL_RESYNC not implemented: sh_audio->a_in_buffer_len=0;
#define ADCTRL_RESYNC_STREAM 1       /* resync, called after seeking! */

// fallback if ADCTRL_SKIP not implemented: ds_fill_buffer(sh_audio->ds);
#define ADCTRL_SKIP_FRAME 2         /* skip block/frame, called while seeking! */

// fallback if ADCTRL_QUERY_FORMAT not implemented: sh_audio->sample_format
#define ADCTRL_QUERY_FORMAT 3 /* test for availabilty of a format */

// fallback: use hw mixer in libao
#define ADCTRL_SET_VOLUME 4 /* set volume (used for mp3lib and liba52) */


class DecFactor;

class lumeDecoder: public mpDecorder
{
public:
	
	lumeDecoder();
	virtual ~lumeDecoder();	
	int preinit(sh_audio_t *sh);
	int init(sh_audio_t *sh);
	void uninit(sh_audio_t *sh);
	int control(sh_audio_t *sh,int cmd,void* arg, ...);
	int decode_audio(sh_audio_t *sh_audio,unsigned char **inbuf,int *inslen,unsigned char* outbuf,int *outlen);
	static ad_info_t m_info;
private:

	int aframe_cnt;
	friend class DecFactor;
	int audio_output_channels;
};

}
#endif  //#ifndef LUME_AUDIO_DEC_H_INCLUDED

