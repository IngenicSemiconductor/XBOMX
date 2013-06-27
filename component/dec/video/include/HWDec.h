/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, hardware
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HARD_AVC_H_

#define HARD_AVC_H_

#include "SimpleHardOMXComponent.h"
#include <utils/KeyedVector.h>

#include "basetype.h"
#include "HardwareRenderer.h"
#include "lume_dec.h"
#include "PlanarImage.h"
namespace android {

  typedef struct
  {
    u32 cropLeftOffset;
    u32 cropOutWidth;
    u32 cropTopOffset;
    u32 cropOutHeight;
  } CropParams;

enum VideoFormat {
  VF_INVAL,
  VF_MPEG1,
  VF_H261,
  VF_H263,
  VF_MPEG4,
  VF_MJPEG,
  VF_H264,
  VF_WMV3,
  VF_RV40,
};
  
struct HWDec : public SimpleHardOMXComponent {
    HWDec(const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

protected:
    virtual ~HWDec();

    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual OMX_ERRORTYPE getConfig(OMX_INDEXTYPE index, OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);

private:
  typedef VideoDecorder* (*createLUMEVideoDecoder)();
  typedef OMX_ERRORTYPE (*decinit)(VideoDecorder*);
  typedef OMX_ERRORTYPE (*decdeinit)(VideoDecorder*);
  typedef OMX_BOOL (*setconext)(VideoDecorder*,sh_video_t *);

  typedef OMX_BOOL (*decodeframe)(VideoDecorder*,
				  OMX_U8* , OMX_U32* ,
				  OMX_U8** , OMX_U32* ,
				  OMX_PARAM_PORTDEFINITIONTYPE* ,
				  OMX_S32* , OMX_BOOL , OMX_BOOL *);
  decodeframe mDecFrame;
    VideoDecorder* mVideoDecoder;
    sh_video_t * vContext;
    enum {
        kInputPortIndex   = 0,
        kOutputPortIndex  = 1,
        kNumInputBuffers  = 8,
        //kNumOutputBuffers = 2,
	//kNumInputBuffers = 4,
	kNumOutputBuffers = 3,
    };

    enum EOSStatus {
        INPUT_DATA_AVAILABLE,
        INPUT_EOS_SEEN,
        OUTPUT_FRAMES_FLUSHED,
    };

    void *mHandle;

    size_t mInputBufferCount;

    uint32_t mWidth, mHeight, mPictureSize;
    uint32_t mCropLeft, mCropTop;
    uint32_t mCropWidth, mCropHeight;

    uint8_t *mFirstPicture;
    int32_t mFirstPictureId;

    int32_t mPicId;  // Which output picture is for which input buffer?

    // OMX_BUFFERHEADERTYPE may be overkill, but it is convenient
    // for tracking the following fields: nFlags, nTimeStamp, etc.
    KeyedVector<int32_t, OMX_BUFFERHEADERTYPE *> mPicToHeaderMap;
    bool mHeadersDecoded;

    EOSStatus mEOSStatus;

    enum OutputPortSettingChange {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    };
    OutputPortSettingChange mOutputPortSettingsChange;

    void initPorts();
    status_t initDecoder();
    void updatePortDefinitions();
    bool drainAllOutputBuffers();
    void drainOneOutputBuffer(int32_t picId, uint8_t *data);
    void saveFirstOutputBuffer(int32_t pidId, uint8_t *data);
    bool handleCropRectEvent(const CropParams* crop);
    //    bool handlePortSettingChangeEvent(const H264SwDecInfo *info);

    bool mDecInited;
    uint32_t mNumSamplesOutput;
    VideoFormat mVideoFormat;
    DISALLOW_EVIL_CONSTRUCTORS(HWDec);
    sp<HardwareRenderer> mRenderer;
    PlanarImage *mOutputBuf;
    bool mVContextNeedFree;
};

}  // namespace android

#endif  // HARD_AVC_H_

