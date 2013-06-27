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

#ifndef HARD_AAC_2_H_
#define HARD_AAC_2_H_

#include "SimpleHardOMXComponent.h"
#include "lume_audio_dec.h"
#include "aacdecoder_lib.h"

namespace android {

struct HWAudioDec : public SimpleHardOMXComponent {
    HWAudioDec(const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

protected:
    virtual ~HWAudioDec();

    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);

private:
    enum {
        kNumInputBuffers        = 4,
        kNumOutputBuffers       = 4,
    };

    enum AudioFormat {
        AF_INVAL,
	AF_AAC,
	AF_MPEG3,
    };
    
    AudioDecoder *mAudioDecoder;
    sh_audio_t *aContext;
    bool mDecInited;
    AudioFormat mAudioFormat;
    bool mAContextNeedFree;

    int32_t mNumChannels;
    int32_t mSamplingRate;

    bool mIsADTS;
    bool mIsFirst;
    bool mSignalledError;
    int64_t mAnchorTimeUs;
    int64_t mNumSamplesOutput;

    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;

    void initPorts();
    status_t initDecoder();

    DISALLOW_EVIL_CONSTRUCTORS(HWAudioDec);
};

}  // namespace android

#endif  // HARD_AAC_2_H_
