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

#include <utils/Log.h>
#include <cutils/properties.h> // for property_get
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBuffer.h>
#include <gui/Surface.h>
#include <ui/ANativeObjectBase.h>
#include <ui/GraphicBufferMapper.h>
#include <gui/ISurfaceTexture.h>
#include <system/window.h>
#include <utils/Singleton.h>
#include "HardwareRenderer_FrameBuffer.h"
#include "PlanarImage.h"

#include "jzasm.h"

#include <media/stagefright/MediaDefs.h>
#include <LUMEDefs.h>

#define EL(x,y...) //ALOGE(x,##y);

#define LOG_TAG "HardwareRenderer_FrameBuffer"


namespace android {

static int mFormatUseBlockMode = 0;
HardwareRenderer_FrameBuffer::HardwareRenderer_FrameBuffer(const OMX_VIDEO_PORTDEFINITIONTYPE meta)
    : mIPU_inited(false),
      mUseJzBuf(-1),
      mDstStride(-1),
      mDstWidth(-1),
      mRegionChanged(false),
      mBytesPerDstPixel(4),
      mCropWidth(0),
      mCropHeight(0),
      mIsLUMEDec(false)
{  
    int32_t tmp;
    
    mColorFormat = (OMX_COLOR_FORMATTYPE)meta.eColorFormat;
    mWidth = meta.nFrameWidth;
    mHeight = meta.nFrameHeight;

    EL("Before : mWidth=%d,mHeight=%d",mWidth,mHeight);
    if(mWidth > 1280) {
      mBuffer_Height = mHeight * 1280 / mWidth;
      mBuffer_Width = 1280;
    }else{
      mBuffer_Width = mWidth;
      mBuffer_Height = mHeight;
    }
    EL("After : mBuffer_Width=%d,mBuffer_Height=%d",mBuffer_Width,mBuffer_Height);

    mUseJzBuf = 1; 
    mFormatUseBlockMode = 1;

    mIsLUMEDec = true;

    mCropLeft = mCropTop = 0;
    mCropRight = mBuffer_Width - 1;
    mCropBottom = mBuffer_Height - 1;

    mCropWidth = mCropRight - mCropLeft + 1;
    mCropHeight = mCropBottom - mCropTop + 1;

    switch (mColorFormat) {
    default:
        //mhalFormat = HAL_PIXEL_FORMAT_RGB_565;//HAL_PIXEL_FORMAT_RGBX_8888; //TODO:jim:temporary as the same format with softrender.
      mhalFormat = HAL_PIXEL_FORMAT_RGBX_8888;//HAL_PIXEL_FORMAT_RGBX_8888; //TODO:jim:temporary as the same format with softrender.
      break;
    }

    switch (mhalFormat) {
        case HAL_PIXEL_FORMAT_RGBX_8888:
            mBytesPerDstPixel = 4;
            break;
        default:
            mBytesPerDstPixel = 2;
            break;
    }
//	LOGD("mColorFormat= 0x%x, rotationDegrees= 0x%x. mCropLeft=%d, mCropTop=%d, mCropRight=%d, mCropBottom=%d", mColorFormat, rotationDegrees, mCropLeft, mCropTop, mCropRight, mCropBottom);

    CHECK(mWidth > 0);
    CHECK(mHeight > 0);

    mIPUHandler = NULL;
    if (ipu_open(&mIPUHandler) < 0) {
      ALOGE("ERROR: ipu_open() failed mIPUHandler=%p", mIPUHandler);
      ipu_close(&mIPUHandler);
      mIPUHandler = NULL;
      return;
    }
    
    //memset(&mOldState, 0, sizeof(mOldState));
    
    mRegionChanged = true;

    int ret = dmmu_get_page_table_base_phys(&tlb_base_phys);
    if (ret < 0) {
      ALOGE("ERROR: dmmu_get_page_table_base_phys failed!\n");
      return;
    }

    memset(&src_mem_info, 0, sizeof(dmmu_mem_info));
    memset(&dst_mem_info, 0, sizeof(dmmu_mem_info));
}
  
HardwareRenderer_FrameBuffer::~HardwareRenderer_FrameBuffer() 
{  
  if (mIPUHandler) {
    mIPU_inited = false;
    ipu_close(&mIPUHandler);
    mIPUHandler = NULL;
  }    
}
  
void HardwareRenderer_FrameBuffer::initIPUDestBuffer(void* data/*, struct VideoWindowState *state*/)

{
  {//demmu map the dst.
    memset(&dst_mem_info, 0, sizeof(dmmu_mem_info));
    dst_mem_info.vaddr = data;
    dst_mem_info.size = mBuffer_Height * mBuffer_Width * mBytesPerDstPixel;
    
    int ret = dmmu_map_user_memory(&dst_mem_info);
    if (ret < 0) {
      ALOGE("ERROR: !!!!dst dmmu_map_user_memory failed!\n");
      memset(&dst_mem_info, 0, sizeof(dmmu_mem_info));
      return;
    }
  }

  struct dest_data_info *dst = &mIPUHandler->dst_info;
  unsigned int output_mode;
  struct ipu_data_buffer *dstBuf = &dst->dstBuf;
  
  //ALOGE("dst_addr = %p state->TvOut = %d dstWidth = %d dstHeight = %d \n dstCropLeft = %d dstCropTop = %d dstCropRight = %d dstCropBottom = %d", dst_addr, state->TvOut, state->Right - state->Left, state->Bottom - state->Top, state->Left, state->Top, state->Right, state->Bottom);
  
  memset(dst, 0, sizeof(dest_data_info));
  memset(dstBuf, 0, sizeof(ipu_data_buffer));
  
  dst->dst_mode = IPU_OUTPUT_TO_FRAMEBUFFER | IPU_OUTPUT_BLOCK_MODE;

  dst->fmt = mhalFormat;

  dst->left = 0;
  dst->top = 0;

  dst->width = mBuffer_Width;
  dst->height = mBuffer_Height;
  dstBuf->y_buf_phys = 0;
  dst->dtlb_base = tlb_base_phys;

  dst->out_buf_v = data;
  dstBuf->y_stride = mDstStride * mBytesPerDstPixel;

  //ALOGE("dst->width:%d, dst->height:%d, dst->left:%d, dst->top:%d, dst->dtlb_base:%d, dst->out_buf_v:%d, dstBuf->y_stride:%d", dst->width, dst->height, dst->left, dst->top, dst->dtlb_base, dst->out_buf_v, dstBuf->y_stride);
}
  
void HardwareRenderer_FrameBuffer::initIPUSourceBuffer(void *data, size_t srcWidth, size_t srcHeight, 
						       size_t srcCropLeft,size_t srcCropTop, size_t srcCropRight, size_t srcCropBottom)
{
  {
    if(!mUseJzBuf && !mIsLUMEDec){//no need for jzbuf, which has already been demmu mapped.
      memset(&src_mem_info, 0, sizeof(dmmu_mem_info));
      src_mem_info.vaddr = data;
      src_mem_info.size = srcWidth * srcHeight * 3 / 2;//nonjzmedia default as 420P.
    
      int ret = dmmu_map_user_memory(&src_mem_info);
      if (ret < 0) {
	ALOGE("ERROR: src dmmu_map_user_memory pimg0->planar[0] failed!\n");
	memset(&src_mem_info, 0, sizeof(dmmu_mem_info));
	return;
      }    
    }
  }

  struct source_data_info *src = &mIPUHandler->src_info;;
  struct ipu_data_buffer *srcBuf = &mIPUHandler->src_info.srcBuf;
  PlanarImage *pimg = (PlanarImage *)data;
  
  //ALOGE("srcWidth = %d srcHeight = %d \n srcCropLeft = %d srcCropTop = %d srcCropRight = %d srcCropBottom = %d", srcWidth, srcHeight, srcCropLeft, srcCropTop, srcCropRight, srcCropBottom);
  
  //ALOGE("%p %p %p", ((PlanarImage *)data)->phy_planar[0], ((PlanarImage *)data)->phy_planar[1], ((PlanarImage *)data)->phy_planar[2]);
  
  switch (mColorFormat) {
  case OMX_COLOR_FormatYUV420Planar:
  case OMX_COLOR_FormatCbYCrY:
  case OMX_COLOR_FormatYUV420SemiPlanar:
  default:
    if (mUseJzBuf) {
      src->fmt = HAL_PIXEL_FORMAT_JZ_YUV_420_B;
    } else {
      src->fmt = HAL_PIXEL_FORMAT_JZ_YUV_420_P;	    
    }
  }
    
  if ((data == NULL) || (srcWidth == 0))
    return;
  
  src->is_virt_buf = 1;
  src->stlb_base = tlb_base_phys;

  src->width = srcWidth;
  src->height = srcHeight;

  srcBuf->y_buf_phys = 0; 
  srcBuf->u_buf_phys = 0; 
  srcBuf->v_buf_phys = 0; 
  
  if (!mUseJzBuf) {
    if(mIsLUMEDec){
      srcBuf->y_buf_v = reinterpret_cast<void*>(pimg->planar[0]);

      srcBuf->u_buf_v = reinterpret_cast<void*>(pimg->planar[1]);
      srcBuf->v_buf_v = reinterpret_cast<void*>(pimg->planar[2]);

      srcBuf->y_stride = pimg->stride[0];
      srcBuf->u_stride = pimg->stride[1];
      srcBuf->v_stride = pimg->stride[1];
    }else{
      srcBuf->y_buf_v = data;
      srcBuf->u_buf_v = data + srcWidth * srcHeight;
      srcBuf->v_buf_v = data + srcWidth * srcHeight + srcWidth * srcHeight / 4;
      
      srcBuf->y_stride = srcWidth;
      srcBuf->u_stride = srcWidth / 2;
      srcBuf->v_stride = srcWidth / 2;
    }
  } else {
    srcBuf->y_buf_v = reinterpret_cast<void*>(pimg->planar[0]);	    /* virtual address of y buffer or base address */
    srcBuf->u_buf_v = reinterpret_cast<void*>(pimg->planar[1]);
    
    if (src->fmt == HAL_PIXEL_FORMAT_JZ_YUV_420_P) {
      srcBuf->v_buf_v = reinterpret_cast<void*>(pimg->planar[2]);
    } else {
      srcBuf->v_buf_v = reinterpret_cast<void*>(pimg->planar[1]);
    }
    
    srcBuf->y_stride = pimg->stride[0];
    srcBuf->u_stride = pimg->stride[1];
    srcBuf->v_stride = pimg->stride[1];
  }
}


void HardwareRenderer_FrameBuffer::render(RenderData* data)
{
  android_native_buffer_t *buf;
  int err;
  GraphicBufferMapper &mapper = GraphicBufferMapper::get();
  
  Rect bounds(mCropWidth, mCropHeight);
  void *dst;
  buffer_handle_t bufferHandle = data->bufferHandle;
  CHECK_EQ(0, mapper.lock(bufferHandle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));

  if(mUseJzBuf != 1)//when non 420B.
    jz_dcache_wb();
  
  //mDstStride = buf->stride;
  mDstStride = (mBuffer_Width + 31) & 0xffffffe0;

  initIPUSourceBuffer(data->input, mWidth, mHeight, mCropLeft, mCropTop, mCropRight, mCropBottom);
  
  initIPUDestBuffer(dst/*, (data->state)*/);
    
  if (mIPU_inited == false) {
    if ((err = ipu_init(mIPUHandler)) < 0) {
      ALOGE("ERROR: ipu_init() failed mIPUHandler=%p", mIPUHandler);
      return;
    } else {
      mIPU_inited = true;
    }
  }
  
  ipu_postBuffer(mIPUHandler);

  {//clean up the mapped dmmu mem. but still it looks ok even without it...
    int ret;
    
    if(dst_mem_info.vaddr != NULL){
      ret = dmmu_unmap_user_memory(&dst_mem_info);//memset 0 at init is the best to cover all the other data members of dst_mem_info.
      if (ret < 0) {
	ALOGE("ERROR: !!!!dst dmmu_unmap_user_memory failed!\n");
	return;
      }
    }

    if(!mUseJzBuf && src_mem_info.vaddr != NULL){
      ret = dmmu_unmap_user_memory(&src_mem_info);
      if (ret < 0) {
	ALOGE("ERROR: !!!!src dmmu_unmap_user_memory failed!\n");
	return;
      }
    }
  }  

  CHECK_EQ(0, mapper.unlock(bufferHandle));
  buf = NULL;
}
}  // namespace android
