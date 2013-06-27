/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "HWAudioDec"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include "HWAudioDec.h"

#include <cutils/properties.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaErrors.h>
#include <LUMEDefs.h>

#define FILEREAD_MAX_LAYERS 2

#define DRC_DEFAULT_MOBILE_REF_LEVEL 64  /* 64*-0.25dB = -16 dB below full scale for mobile conf */
#define DRC_DEFAULT_MOBILE_DRC_CUT   127 /* maximum compression of dynamic range for mobile conf */
#define MAX_CHANNEL_COUNT            6  /* maximum number of audio channels that can be decoded */
// names of properties that can be used to override the default DRC settings
#define PROP_DRC_OVERRIDE_REF_LEVEL  "aac_drc_reference_level"
#define PROP_DRC_OVERRIDE_CUT        "aac_drc_cut"
#define PROP_DRC_OVERRIDE_BOOST      "aac_drc_boost"

using namespace android;
extern "C" {
/*
 *exposed interface for libstagefright_soft_lume.so.
 */
AudioDecoder* CreateLUMEAudioDecoder(void);
OMX_BOOL ALumeDecInit(AudioDecoder*audioD);
void ALumeDecDeinit(AudioDecoder*audioD);
OMX_BOOL AudioDecSetConext(AudioDecoder*audioD,sh_audio_t *sh);
int DecodeAudio(AudioDecoder*audioD,
		     OMX_S16* aOutBuff,OMX_U32* aOutputLength,
		     OMX_U8** aInputBuf,OMX_U32* aInBufSize,
		     OMX_S32* aFrameCount,
		     OMX_AUDIO_PARAM_PCMMODETYPE* aAudioPcmParam,
		     OMX_BOOL aMarkerFlag,
		     OMX_BOOL* aResizeFlag);
}
namespace android {

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

HWAudioDec::HWAudioDec(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : SimpleHardOMXComponent(name, callbacks, appData, component),
      mAudioDecoder(NULL),
      aContext(NULL),
      mAContextNeedFree(false),
      mIsADTS(false),
      mSignalledError(false),
      mDecInited(false),
      mAnchorTimeUs(0),
      mNumChannels(2),
      mSamplingRate(44100),
      mNumSamplesOutput(0),
      mAudioFormat(AF_INVAL),
      mOutputPortSettingsChange(NONE) {
    initPorts();
    //CHECK_EQ(initDecoder(), (status_t)OK);
}

HWAudioDec::~HWAudioDec() {
  if(mAudioDecoder){
    delete mAudioDecoder;
  }

  if(aContext && mAContextNeedFree){
    delete aContext;
    aContext = NULL;
  }
}

void HWAudioDec::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = MAX_AUDIO_EXTRACTOR_BUFFER_RANGE;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.audio.cMIMEType = const_cast<char *>("audio/aac");
    def.format.audio.eEncoding = OMX_AUDIO_CodingAAC;
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumOutputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = MAX_AUDIO_DECODER_BUFFER_SIZE;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.audio.cMIMEType = const_cast<char *>("audio/raw");
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    addPort(def);
}

status_t HWAudioDec::initDecoder() {
    status_t status = UNKNOWN_ERROR;
    mIsFirst = true;
    ALOGV("initDecoder in");

    mAudioDecoder = CreateLUMEAudioDecoder();
    CHECK(mAudioDecoder);

    /**/
    if(ALumeDecInit(mAudioDecoder) != OMX_TRUE)
      return OMX_ErrorUndefined;

    if(!aContext) {
      aContext = new sh_audio_t;
      memset(aContext,0x0,sizeof(sh_audio_t));   
      aContext->format = 0x4134504d;
      mAContextNeedFree = true;
    }

    if(AudioDecSetConext(mAudioDecoder,aContext)==OMX_FALSE){
      ALOGE("AudioDecSetConext failed!");
      return OMX_FALSE;
    }  

    return OK;
}

OMX_ERRORTYPE HWAudioDec::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamAudioAac:
        {
            OMX_AUDIO_PARAM_AACPROFILETYPE *aacParams =
                (OMX_AUDIO_PARAM_AACPROFILETYPE *)params;

            if (aacParams->nPortIndex != 0) {
                return OMX_ErrorUndefined;
            }

            aacParams->nBitRate = 0;
            aacParams->nAudioBandWidth = 0;
            aacParams->nAACtools = 0;
            aacParams->nAACERtools = 0;
            aacParams->eAACProfile = OMX_AUDIO_AACObjectMain;

            aacParams->eAACStreamFormat =
                mIsADTS
                    ? OMX_AUDIO_AACStreamFormatMP4ADTS
                    : OMX_AUDIO_AACStreamFormatMP4FF;

            aacParams->eChannelMode = OMX_AUDIO_ChannelModeStereo;

	    aacParams->nChannels = mNumChannels;
	    aacParams->nSampleRate = mSamplingRate;
	    aacParams->nFrameLength = 0;

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)params;

            if (pcmParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            pcmParams->eNumData = OMX_NumericalDataSigned;
            pcmParams->eEndian = OMX_EndianBig;
            pcmParams->bInterleaved = OMX_TRUE;
            pcmParams->nBitPerSample = 16;
            pcmParams->ePCMMode = OMX_AUDIO_PCMModeLinear;
            pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            pcmParams->eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            // pcmParams->eChannelMapping[2] = OMX_AUDIO_ChannelCF;
            // pcmParams->eChannelMapping[3] = OMX_AUDIO_ChannelLFE;
            // pcmParams->eChannelMapping[4] = OMX_AUDIO_ChannelLS;
            // pcmParams->eChannelMapping[5] = OMX_AUDIO_ChannelRS;

	    pcmParams->nChannels = mNumChannels;
	    pcmParams->nSamplingRate = mSamplingRate;

            return OMX_ErrorNone;
        }

        default:
            return SimpleHardOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE HWAudioDec::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
        case 0x7F000014:
	{
	    if (!aContext) {
	        aContext = (sh_audio_t *)params;
		mAContextNeedFree = false;
		ALOGV("aContext=%x aContext->format=%x",aContext,aContext->format);
	    }
	    return OMX_ErrorNone;
	}

        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;

	    ALOGV("roleParams->cRole =  %s", (const char *)roleParams->cRole);
	    if (strncmp((const char *)roleParams->cRole, 
			"audio_decoder.aac", 
			OMX_MAX_STRINGNAME_SIZE - 1) == 0){
	      //return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioAac:
        {
            const OMX_AUDIO_PARAM_AACPROFILETYPE *aacParams =
                (const OMX_AUDIO_PARAM_AACPROFILETYPE *)params;

            if (aacParams->nPortIndex != 0) {
                return OMX_ErrorUndefined;
            }

            if (aacParams->eAACStreamFormat == OMX_AUDIO_AACStreamFormatMP4FF) {
                mIsADTS = false;
            } else if (aacParams->eAACStreamFormat
                        == OMX_AUDIO_AACStreamFormatMP4ADTS) {
                mIsADTS = true;
            } else {
                return OMX_ErrorUndefined;
            }
	    ALOGV("mIsADTS=%d",mIsADTS);
            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioPcm:
        {
            const OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)params;

            if (pcmParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        default:
            return SimpleHardOMXComponent::internalSetParameter(index, params);
    }
}

void HWAudioDec::onQueueFilled(OMX_U32 portIndex) {
    if(!mDecInited){
      ALOGE("onQueueFilled initDecoder");
      status_t ret = initDecoder();
      if(ret != OK){
	ALOGE("Failed to initdecoder!!!");
	notify(OMX_EventError, OMX_ErrorUndefined, ret, NULL);
	return;
      }
      mDecInited = true;
    }
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    UCHAR* inBuffer[FILEREAD_MAX_LAYERS];
    UINT inBufferLength[FILEREAD_MAX_LAYERS] = {0};
    UINT bytesValid[FILEREAD_MAX_LAYERS] = {0};

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!inQueue.empty() && !outQueue.empty()) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

	OMX_U32 outLength = 0;
	OMX_U8 * pStream = inHeader->pBuffer + inHeader->nOffset;
	OMX_U32 inLength = inHeader->nFilledLen;
	OMX_AUDIO_PARAM_PCMMODETYPE AudioPcmMode;
	OMX_S32 frameCount = 0;
	OMX_BOOL resizeNeeded = OMX_FALSE;

        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            inQueue.erase(inQueue.begin());
            inInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(inHeader);
            if (!mIsFirst) {
                // flush out the decoder's delayed data by calling DecodeFrame
                // one more time, with the AACDEC_FLUSH flag set
                INT_PCM *outBuffer =
                        reinterpret_cast<INT_PCM *>(
                                outHeader->pBuffer + outHeader->nOffset);

		int result = DecodeAudio(mAudioDecoder,
					   (OMX_S16*)outBuffer,
					   (OMX_U32*)&outLength,
					   (OMX_U8**)(&pStream),
					   &inLength,
					   &frameCount,
					   &AudioPcmMode,
					   (OMX_BOOL)1,
					   &resizeNeeded);

		if(outLength <= 0){
		  ALOGE("Error: audio decode failed with outputlen:%d", (int)outLength);
		  outLength = 0;//AudioPlayer could handle a empty buffer. So,it is OK to decode fail and pass a empty frame.
		}else{
		  if(AudioPcmMode.nBitPerSample != 8)
		    outLength *= 2;//The outputlen from decoder has been devided with 2 for a 16 bits sample. recover it to a Byte of 8 bits
		}

                outHeader->nFilledLen = outLength;
            } else {
                // Since we never discarded frames from the start, we won't have
                // to add any padding at the end either.

                outHeader->nFilledLen = 0;
            }

            outHeader->nFlags = OMX_BUFFERFLAG_EOS;

            outQueue.erase(outQueue.begin());
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
            return;
        }

        if (inHeader->nOffset == 0) {
            mAnchorTimeUs = inHeader->nTimeStamp;
            mNumSamplesOutput = 0;
        }

        size_t adtsHeaderSize = 0;
        if (mIsADTS) {
            // skip 30 bits, aac_frame_length follows.
            // ssssssss ssssiiip ppffffPc ccohCCll llllllll lll?????

            const uint8_t *adtsHeader = inHeader->pBuffer + inHeader->nOffset;

            bool signalError = false;
            if (inHeader->nFilledLen < 7) {
                ALOGE("Audio data too short to contain even the ADTS header. "
                      "Got %ld bytes.", inHeader->nFilledLen);
                hexdump(adtsHeader, inHeader->nFilledLen);
                signalError = true;
            } else {
                bool protectionAbsent = (adtsHeader[1] & 1);

                unsigned aac_frame_length =
                    ((adtsHeader[3] & 3) << 11)
                    | (adtsHeader[4] << 3)
                    | (adtsHeader[5] >> 5);

                if (inHeader->nFilledLen < aac_frame_length) {
                    ALOGE("Not enough audio data for the complete frame. "
                          "Got %ld bytes, frame size according to the ADTS "
                          "header is %u bytes.",
                          inHeader->nFilledLen, aac_frame_length);
                    hexdump(adtsHeader, inHeader->nFilledLen);
                    signalError = true;
                } else {
                    adtsHeaderSize = (protectionAbsent ? 7 : 9);

                    pStream = (UCHAR *)adtsHeader + adtsHeaderSize;
                    inLength = aac_frame_length - adtsHeaderSize;

                    inHeader->nOffset += adtsHeaderSize;
                    inHeader->nFilledLen -= adtsHeaderSize;
                }
            }

            if (signalError) {
                mSignalledError = true;

                notify(OMX_EventError,
                       OMX_ErrorStreamCorrupt,
                       ERROR_MALFORMED,
                       NULL);

                return;
            }
        }

	INT_PCM *outBuffer =reinterpret_cast<INT_PCM *>(
			            outHeader->pBuffer + outHeader->nOffset);

	int result = DecodeAudio(mAudioDecoder,
				 (OMX_S16*)outBuffer,
				 (OMX_U32*)&outLength,
				 (OMX_U8**)(&pStream),
				 &inLength,
				 &frameCount,
				 &AudioPcmMode,
				 (OMX_BOOL)1,
				 &resizeNeeded);

	if (AudioPcmMode.nSamplingRate != mSamplingRate ||
	    AudioPcmMode.nChannels != mNumChannels) {
	  ALOGI("Reconfiguring decoder: %d Hz, %d channels",
		AudioPcmMode.nSamplingRate,
		AudioPcmMode.nChannels);
	  mSamplingRate = AudioPcmMode.nSamplingRate;
	  mNumChannels = AudioPcmMode.nChannels;

	  inHeader->nOffset -= adtsHeaderSize;
	  inHeader->nFilledLen += adtsHeaderSize;

	  notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
	  mOutputPortSettingsChange = AWAITING_DISABLED;
	  return;
        } else if (!AudioPcmMode.nSamplingRate || !AudioPcmMode.nChannels) {
            ALOGW("Invalid stream");
            mSignalledError = true;
            notify(OMX_EventError, OMX_ErrorUndefined, result, NULL);
            return;
        }

        if (result == ALUMEDEC_SUCCESS) {
            inHeader->nOffset += inHeader->nFilledLen - inLength;
            inHeader->nFilledLen = inLength;
	} else {
            ALOGW("decoder returned error %d, substituting silence",
                  result);
            // Discard input buffer.
            inHeader->nFilledLen = 0;
	    inHeader->nOffset = 0;
        }

        if (result == ALUMEDEC_SUCCESS || mNumSamplesOutput > 0) {
            // We'll only output data if we successfully decoded it or
            // we've previously decoded valid data, in the latter case
            // (decode failed) we'll output a silent frame.
            if (mIsFirst) {
                mIsFirst = false;
                // the first decoded frame should be discarded to account
                // for decoder delay
                outLength = 0;
            }

	    if(outLength <= 0){
	      ALOGE("Error: audio decode failed with outputlen:%d", (int)outLength);
	      outLength = 0;//AudioPlayer could handle a empty buffer. So,it is OK to decode fail and pass a empty frame.
	    }else{
	      if(AudioPcmMode.nBitPerSample != 8)
		outLength *= 2;//The outputlen from decoder has been devided with 2 for a 16 bits sample. recover it to a Byte of 8 bits
	    }

            outHeader->nFilledLen = outLength;
            outHeader->nFlags = 0;

            outHeader->nTimeStamp =
                mAnchorTimeUs
                    + (mNumSamplesOutput * 1000000ll) / AudioPcmMode.nSamplingRate;

            mNumSamplesOutput += outLength/ 2 / sizeof(int16_t);

            outInfo->mOwnedByUs = false;
            outQueue.erase(outQueue.begin());
            outInfo = NULL;
            notifyFillBufferDone(outHeader);
            outHeader = NULL;
        }

        if (inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }
    }
}

void HWAudioDec::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == 0) {
        // Make sure that the next buffer output does not still
        // depend on fragments from the last one decoded.
        mIsFirst = true;
	if (mAudioDecoder->GetAudiosh()->ds)
	    mAudioDecoder->GetAudiosh()->ds->seek_flag = 1;
    }
}

void HWAudioDec::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
    if (portIndex != 1) {
        return;
    }

    switch (mOutputPortSettingsChange) {
        case NONE:
            break;

        case AWAITING_DISABLED:
        {
            CHECK(!enabled);
            mOutputPortSettingsChange = AWAITING_ENABLED;
            break;
        }

        default:
        {
            CHECK_EQ((int)mOutputPortSettingsChange, (int)AWAITING_ENABLED);
            CHECK(enabled);
            mOutputPortSettingsChange = NONE;
            break;
        }
    }
}

}  // namespace android

OMX_ERRORTYPE createHardOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    ALOGV("audio createHardOMXComponent in");
    sp<HardOMXComponent> codec = new android::HWAudioDec(name, callbacks, appData, component);
    if (codec == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    
    OMX_ERRORTYPE err = codec->initCheck();
    if (err != OMX_ErrorNone)
      return err;
    
    codec->incStrong(NULL);

    return OMX_ErrorNone;
}
