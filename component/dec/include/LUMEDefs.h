/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LUMEDEFS_H_
#define LUMEDEFS_H_

#include <media/stagefright/MediaSource.h>

#define OMX_BUFFERFLAG_SEEKFLAG 0x00000100

namespace android{

//useless for now
#define FIFO_VALUE 20
#define MULTIPLE_S_TO_US 1000000.0

#define MAX_AUDIO_EXTRACTOR_FRAME_OFFSET_WITH_LEN_NUM 1024 // suppose lumeextractor will never compose more than 1024 numed audio frames.

#define MAX_AUDIO_EXTRACTOR_BUFFER_RANGE 0x80000 
#define MAX_VIDEO_EXTRACTOR_BUFFER_RANGE 0x100000
#define MAX_SUB_EXTRACTOR_BUFFER_RANGE 1048576
#define MAX_UNKNOWN_PACKET_SIZE //
#define MAX_AUDIO_EXTRACTOR_PACKETING_PTS_GAP 50000 //us
//#define MAX_AUDIO_DECODER_BUFFER_SIZE 192000
#define MAX_AUDIO_DECODER_BUFFER_SIZE 0x100000
#define MAX_SYSTEM_VORBIS_SIZE 202000

//More extractor buffers could previent decoder being hungry.
#define AUDIO_EXTRACTOR_BUFFER_COUNT 20
#define VIDEO_EXTRACTOR_BUFFER_COUNT 20
#define SUB_EXTRACTOR_BUFFER_COUNT 20

///////////Dis/Enable the below macro will make the extractor/decoder... running in a different mode.

//Register lume extractor to DataSource.
#define SNIFFER_LUME_REGISTED

//Register audio/video decoder to OMXCodec.
#define OMXCODEC_LUME_VIDEODEC_REGISTED
#define OMXCODEC_LUME_AUDIODEC_REGISTED

//Give the ability of using lume extractor or stagefright decoder, and debugging when wants to disable video|audio.
#define MEDIA_MIMETYPE_VIDEO_GENERAL MEDIA_MIMETYPE_VIDEO_LUME //MEDIA_MIMETYPE_UNKNOWN;MEDIA_MIMETYPE_VIDEO_LUME
#define MEDIA_MIMETYPE_AUDIO_GENERAL MEDIA_MIMETYPE_AUDIO_LUME //MEDIA_MIMETYPE_UNKNOWN;MEDIA_MIMETYPE_AUDIO_LUME

//Makes Extractor running in a independent thread which decoupled from decoders.
#define SINGLE_THREAD_FOR_EXTRACTOR

//System ringtones are frequently/shortly played. Disable prefetcher for effect. 
//#define OPTIMIZE_FOR_SYSTEM_RINGTONE //TODO:Checking isSystemRingtone by fileLen is not approriate.should by path.

//Feeds the audio decoder with only 1 frame at a time of decoder.read(). It is actually optimized for vorbis to previent the waste of packing only 1 packet at extractor.read(), but also seems works well for other audio formats.
//#define AUDIODEC_IN_UNIT_OF_1_FRAME

//Shows the specific pos decoded pic in gallery. The default shown is the first pic, most of it is black. user exprience. 
#define THUMBNAIL_TIME_PERCENTAGE 0.1 // because some video seek index need create,0.5 too slow 

///////////


///////////Dis/Enable the below macro will affect how retriever running.
//#define RETRIEVER_GETFRAME_TRY_TIMES 4

///////////


///////////Dis/Enable the below macro will enable printf some key debugging info.

//#define DEBUG_VIDEODEC_COST_TIME
//#define DEBUG_AUDIODEC_COST_TIME

//Printf 6(modifiable) in&out buffer values for video decoder.
//#define DEBUG_VIDEODEC_COUNTED_BUFVALUE 6
//#define DEBUG_AUDIODEC_COUNTED_BUFVALUE 6

#define DEBUG_AWESOMEPLAYER_CODE_PATH
//#define DEBUG_LUMEEXTRACTOR_CODE_PATH
//#define DEBUG_LUMEVIDEODECODER_CODE_PATH
//#define DEBUG_LUMEAUDIODECODER_CODE_PATH
#define DEBUG_METADATARETRIEVER_CODE_PATH
///////////


//////////MIME type strings

extern const char *MEDIA_MIMETYPE_VIDEO_LUME;
extern const char *MEDIA_MIMETYPE_AUDIO_LUME;
extern const char *MEDIA_MIMETYPE_CONTAINER_LUME;
extern const char *MEDIA_MIMETYPE_VIDEO_X264;
extern const char *MEDIA_MIMETYPE_SUBTL_LUME;
extern const char *MEDIA_MIMETYPE_UNKNOWN;
extern const char *MEDIA_MIMETYPE_CONTAINER_OGV;
extern const char *MEDIA_MIMETYPE_AUDIO_MP4A;
extern const char *MEDIA_MIMETYPE_VIDEO_WMV3;
extern const char *MEDIA_MIMETYPE_VIDEO_RV40;
/////////

enum META_DATA_EXTENDED {
    kKeyBitsPerSample     = 'btps',
    kKeyAudioSH           = 'aush',  //sh_audio_t*
    kKeyVideoSH           = 'vish',  //sh_video_t*
    kKeyAVSyncLock        = 'avlk',  //AVSyncLock*
    kKeyUseJzBuf          = 'jzbf',  //use_jz_buf
    kKeyStartTime         = 'sttm',  //add starttime
    kKeyPmemPhyAddr       = 'pmpa',  //use camera pmem phy addr buffer for encording
    kKeyIsVp              = 'isVp',  //used for open cooker codec
    kKeyAudioSeek         = 'adsk',
    kKeyNALUHasStartCode  = 'nlco',   //int, 1 means x264enc output NALU with start code; 0 means with len bytes.
    kKeyX264EncQP         = 'enqp',   //int, for controlling x264 bitrate.
    kKeyIsWVM             = 'iwvm',   //int32_t (bool)
    kKeyStreamType        = 'stTP',    //nuplayer streamtype
    kKeyExtracterBufCount = 'ebct', //MediaBuffer count of LUMEExtractor mediasource.
    kKeyComposedPacketOffsetwithLen = 'cpow', //void*, the offset and len of a frame in a mediabuffer.
    kKeyComposedPacketOffsetwithLenItemCount = 'cpic', //int32, the item count of the frame offset_len pairs.
    kKeyComposedPacketOffsetwithLenItemConsumed = 'cpis', //int32, consumed count of frame offset_len pairs.
    kKeySpts              = 'spts', //int64, decoupled from mediabuffer->getspts.
    kKeyEpts              = 'epts', //int64, decoupled from mediabuffer->getepts.
};

enum READ_OPTIONS_SEEK_MODE_EXTENDED {
    SEEK_WITH_IS_THUMB_MODE = MediaSource::ReadOptions::SEEK_CLOSEST + 8,//8 can be 1 at least.
};

}

#endif //LUMEDEFS_H_
