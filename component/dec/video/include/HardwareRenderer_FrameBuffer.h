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

#ifndef HARDWARE_FRAMEBUFFERRENDERER_H_
#define HARDWARE_FRAMEBUFFERRENDERER_H_

#include <media/stagefright/MediaBuffer.h>
#include <media/IMediaPlayerService.h>
#include <utils/RefBase.h>
#include <ui/ANativeObjectBase.h>
#include <sys/ioctl.h>

#include <android_jz_ipu.h>
#include <utils/StopWatch.h>
#include <OMX_Video.h>

#include "HardwareRenderer.h"
#include "dmmu.h"

//notifyVideoUseIpu flags
#define IPU_DIRECT        1
#define IPU_NODIRECT      2
#define DRAW_COLORKEY     3

/* rotation degree */
#define ClockWiseRot_0      1
#define ClockWiseRot_90     0
#define ClockWiseRot_180    3
#define ClockWiseRot_270    2

#define HAL_PIXEL_FORMAT_JZ_YUV_420_P 0x47700001 // YUV_420_P
 
#define HAL_PIXEL_FORMAT_JZ_YUV_420_B 0x47700002 // YUV_420_P BLOCK MODE

namespace android {

struct MetaData;

class HardwareRenderer_FrameBuffer : public HardwareRenderer{
public:
    HardwareRenderer_FrameBuffer(const OMX_VIDEO_PORTDEFINITIONTYPE meta);

    virtual ~HardwareRenderer_FrameBuffer();

    virtual void render(RenderData* data);
private:
    OMX_COLOR_FORMATTYPE mColorFormat, mDstFormat;
    int32_t mWidth, mHeight;
    int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;
    int mhalFormat;
    bool mRegionChanged, mIPU_inited;
    //struct VideoWindowState mOldState;

    void initIPUDestBuffer(void* dst_addr/* , struct VideoWindowState *state */);
    void initIPUSourceBuffer(void *data, size_t srcWidth, size_t srcHeight, size_t srcCropLeft,
			     size_t srcCropTop, size_t srcCropRight, size_t srcCropBottom); 
    /* bool isStateChanged(struct VideoWindowState *state); */
    /* void saveState(struct VideoWindowState *state); */
    /* void dumpState(struct VideoWindowState *state); */

    struct ipu_image_info*          mIPUHandler;

    int   mUseJzBuf;
    int   mDstStride;
    int   mDstWidth;
    int   mBuffer_Width;
    int   mBuffer_Height;

    dmmu_mem_info src_mem_info;
    dmmu_mem_info dst_mem_info;
    unsigned int tlb_base_phys;
    int mBytesPerDstPixel;
    int mCropWidth;
    int mCropHeight;    

    bool mIsLUMEDec;

    HardwareRenderer_FrameBuffer(const HardwareRenderer_FrameBuffer &);
    HardwareRenderer_FrameBuffer &operator=(const HardwareRenderer_FrameBuffer &);
};

}  // namespace android

#endif  // HARDWARE_RENDERER_H_
