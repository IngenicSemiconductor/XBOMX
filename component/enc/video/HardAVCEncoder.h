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

#ifndef HARD_AVC_ENCODER_H_
#define HARD_AVC_ENCODER_H_

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/foundation/ABase.h>
#include <utils/Vector.h>
#include "binder/MemoryHeapBase.h"
#include "dmmu.h"
#include "x264/x264.h"

#include "SimpleHardOMXComponent.h"

namespace android {

typedef enum
{
    AVC_BASELINE = 66,
    AVC_MAIN = 77,
    AVC_EXTENDED = 88,
    AVC_HIGH = 100,
    AVC_HIGH10 = 110,
    AVC_HIGH422 = 122,
    AVC_HIGH444 = 144
} AVCProfile;

typedef enum
{
    AVC_LEVEL_AUTO = 0,
    AVC_LEVEL1_B = 9,
    AVC_LEVEL1 = 10,
    AVC_LEVEL1_1 = 11,
    AVC_LEVEL1_2 = 12,
    AVC_LEVEL1_3 = 13,
    AVC_LEVEL2 = 20,
    AVC_LEVEL2_1 = 21,
    AVC_LEVEL2_2 = 22,
    AVC_LEVEL3 = 30,
    AVC_LEVEL3_1 = 31,
    AVC_LEVEL3_2 = 32,
    AVC_LEVEL4 = 40,
    AVC_LEVEL4_1 = 41,
    AVC_LEVEL4_2 = 42,
    AVC_LEVEL5 = 50,
    AVC_LEVEL5_1 = 51
} AVCLevel;

struct MediaBuffer;
class VpuMem{
 public:
  VpuMem(){};
  ~VpuMem();
  Vector<MemoryHeapBase * > mDevBuffers;
  void* vpu_mem_alloc(int size);
};
 
struct HardAVCEncoder : public MediaBufferObserver,
                        public SimpleHardOMXComponent {
    HardAVCEncoder(
            const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

    // Override SimpleHardOMXComponent methods
    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);

    // Override HardOMXComponent methods

    virtual OMX_ERRORTYPE getExtensionIndex(
            const char *name, OMX_INDEXTYPE *index);

    // Implement MediaBufferObserver
    virtual void signalBufferReturned(MediaBuffer *buffer);


    // Callbacks required by PV's encoder
    int32_t allocOutputBuffers(unsigned int sizeInMbs, unsigned int numBuffers);
    void    unbindOutputBuffer(int32_t index);
    int32_t bindOutputBuffer(int32_t index, uint8_t **yuv);
    VpuMem mVpuMem;

protected:
    virtual ~HardAVCEncoder();

private:
    enum {
        kNumBuffers = 2,
    };

    enum {
        kStoreMetaDataExtensionIndex = OMX_IndexVendorStartUnused + 1
    };

    // OMX input buffer's timestamp and flags
    typedef struct {
        int64_t mTimeUs;
        int32_t mFlags;
    } InputBufferInfo;

    int32_t  mVideoWidth;
    int32_t  mVideoHeight;
    int32_t  mVideoFrameRate;
    int32_t  mVideoBitRate;
    int32_t  mVideoColorFormat;
    bool     mStoreMetaDataInBuffers;
    int32_t  mIDRFrameRefreshIntervalInSec;
    AVCProfile mAVCEncProfile;
    AVCLevel   mAVCEncLevel;

    int64_t  mNumInputFrames;
    int64_t  mPrevTimestampUs;
    bool     mStarted;
    bool     mSpsPpsHeaderReceived;
    bool     mReadyForNextFrame;
    bool     mSawInputEOS;
    bool     mSignalledError;
    bool     mIsIDRFrame;

    //    tagAVCHandle          *mHandle;
    //tagAVCEncParam        *mEncParams;
    uint8_t               *mInputFrameData;
    uint32_t              *mSliceGroup;
    Vector<MediaBuffer *> mOutputBuffers;
    Vector<InputBufferInfo> mInputBufferInfoVec;

       uint8_t * mCsdData;
       size_t mCsdSize;

       x264_param_t mParam;
       x264_t *h;
       int i_frame;

       x264_nal_t* mNALHolder;
       int32_t mNALUCount;
       int32_t mNALUIndex;
       int64_t mLastInputBufferTime;
       int32_t mNALUHasStartCode;
       int32_t mEncArgQP;

    void initPorts();
    OMX_ERRORTYPE initEncParams();
    OMX_ERRORTYPE initEncoder();
    OMX_ERRORTYPE releaseEncoder();
    void releaseOutputBuffers();

    uint8_t* extractGrallocData(void *data, buffer_handle_t *buffer);
    void releaseGrallocData(buffer_handle_t buffer);

    DISALLOW_EVIL_CONSTRUCTORS(HardAVCEncoder);
};

}  // namespace android

#endif  // HARD_AVC_ENCODER_H_
