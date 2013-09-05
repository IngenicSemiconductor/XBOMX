/*
 * Copyright (C) 2011 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "HWDec"
#include <utils/Log.h>

#include "HWDec.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>
#include <dlfcn.h>
#include "lume_dec.h"
#include "PlanarImage.h"

#include <LUMEDefs.h>
#include <HardwareAPI.h>
#include <ui/GraphicBufferMapper.h>
#include "HardwareRenderer_FrameBuffer.h"
extern "C"{
#include "stream.h"
#include "demuxer.h"
#include "stheader.h"
}


using namespace android;

extern "C" {
/*
 *exposed interface for libstagefright_soft_lume.so.
 */
  VideoDecorder* CreateLUMESoftVideoDecoder();
  OMX_ERRORTYPE DecInit(VideoDecorder*videoD);
  OMX_ERRORTYPE DecDeinit(VideoDecorder*videoD);
  OMX_BOOL VideoDecSetConext(VideoDecorder*videoD,sh_video_t *sh);
  OMX_BOOL DecodeVideo(VideoDecorder*videoD,
		       OMX_U8* aOutBuffer, OMX_U32* aOutputLength,
		       OMX_U8** aInputBuf, OMX_U32* aInBufSize,
		       OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
		       OMX_S32* aFrameCount, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag);
}

namespace android {


static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
};

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

HWDec::HWDec(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : SimpleHardOMXComponent(name, callbacks, appData, component),
      mHandle(NULL),
      mInputBufferCount(0),
      mWidth(320),
      mHeight(240),
      mPictureSize(mWidth * mHeight * 3 / 2),
      mCropLeft(0),
      mCropTop(0),
      mCropWidth(mWidth),
      mCropHeight(mHeight),
      mFirstPicture(NULL),
      mFirstPictureId(-1),
      mPicId(0),
      mHeadersDecoded(false),
      mEOSStatus(INPUT_DATA_AVAILABLE),
      mVideoDecoder(NULL),
      vContext(NULL),
      mDecFrame(NULL),
      mNumSamplesOutput(0),
      mVideoFormat(VF_INVAL),
      mOutputPortSettingsChange(NONE),
      mRenderer(NULL),
      mVContextNeedFree(false),
      mDecInited(false){
  ALOGV("HWDec construct");
    initPorts();
    mOutputBuf = (PlanarImage *)malloc(sizeof(PlanarImage));
    //CHECK_EQ(initDecoder(), (status_t)OK);
  ALOGV("HWDec construct out");
}

HWDec::~HWDec() {
  ALOGV("~HWDec ");

  /**/
  if(mVideoDecoder){
    delete mVideoDecoder;
  }

    while (mPicToHeaderMap.size() != 0) {
        OMX_BUFFERHEADERTYPE *header = mPicToHeaderMap.editValueAt(0);
        mPicToHeaderMap.removeItemsAt(0);
        delete header;
        header = NULL;
    }
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());

    delete[] mFirstPicture;
    if(mHandle){
      dlclose(mHandle);
      mHandle = NULL;
      ALOGE("libstagefright_soft_vlume.so freed");
    }

    if(vContext && mVContextNeedFree){
      delete vContext;
      vContext = NULL;
    }

    if(mOutputBuf){
      free(mOutputBuf);
      mOutputBuf = NULL;
    }
  ALOGV("~HWDec out");
}

void HWDec::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = kInputPortIndex;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = MAX_VIDEO_EXTRACTOR_BUFFER_RANGE;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    if (mVideoFormat == VF_MPEG4){
      def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_MPEG4);
      def.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
    } else if (mVideoFormat == VF_H264) {
      def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_AVC);
      def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    }else if (mVideoFormat == VF_WMV3) {
      def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_WMV3);
      def.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;      
    }else if (mVideoFormat == VF_RV40) {
      def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RV40);
      def.format.video.eCompressionFormat = OMX_VIDEO_CodingRV;      
    }
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.pNativeWindow = NULL;

    addPort(def);

    def.nPortIndex = kOutputPortIndex;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumOutputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RAW);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    def.format.video.pNativeWindow = NULL;

    def.nBufferSize =
        (def.format.video.nFrameWidth * def.format.video.nFrameHeight * 3) / 2;

    addPort(def);
}

status_t HWDec::initDecoder() {
  ALOGV("initDecoder in");

  mVideoDecoder = CreateLUMESoftVideoDecoder();//(VideoDecorder*)fnc();
  CHECK(mVideoDecoder);

  /**/
  if(DecInit(mVideoDecoder) != OMX_ErrorNone)
    return OMX_ErrorUndefined;

  if (!vContext) {
    vContext = new sh_video_t;
    memset(vContext,0x0,sizeof(sh_video_t));

    vContext->bih = (BITMAPINFOHEADER *)malloc(sizeof(BITMAPINFOHEADER) + 0);
    memset(vContext->bih,0,sizeof(BITMAPINFOHEADER) + 0);
    
    vContext->bih->biSize = sizeof(BITMAPINFOHEADER)  + 0;
        
    if (mVideoFormat == VF_MPEG4){
      vContext->bih->biCompression = mmioFOURCC('F', 'M', 'P', '4');
      vContext->format = mmioFOURCC('F', 'M', 'P', '4');
    }else if (mVideoFormat == VF_H264){
      vContext->bih->biCompression = mmioFOURCC('A','V','C','1');
      vContext->format = mmioFOURCC('A', 'V', 'C', '1');
    }else if (mVideoFormat == VF_WMV3){
      vContext->bih->biCompression = mmioFOURCC('V','C','-','1');
      vContext->format = mmioFOURCC('V','C','-','1');
    }else if (mVideoFormat == VF_RV40){
      vContext->bih->biCompression = mmioFOURCC('R','V','4','0');
      vContext->format = mmioFOURCC('R','V','4','0');
    }
    vContext->is_rtsp = 1; //no extradata

    vContext->disp_w = vContext->bih->biWidth = mWidth;
    vContext->disp_h = vContext->bih->biHeight = mHeight;
    
    vContext->bih->biBitCount = 16; //YUV
    vContext->bih->biSizeImage = mWidth * mHeight * vContext->bih->biBitCount/8;
    vContext->bih->biCompression = vContext->format;

    mVContextNeedFree = true;
  }
  if(VideoDecSetConext(mVideoDecoder,vContext)==OMX_FALSE){
    ALOGE("VideoDecSetConext failed!");
    return OMX_FALSE;
  }  

  return OK;   
}

OMX_ERRORTYPE HWDec::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
  ALOGV("internalGetParameter in index = 0x%x",index);
    switch (index) {
#if 1
        case 0x7F000012:
	  {
	    GetAndroidNativeBufferUsageParams *Usageparams = (GetAndroidNativeBufferUsageParams *)params;
	    Usageparams->nUsage |= (
				   GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP);
	    return OMX_ErrorNone;
	  }
#endif
        case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

            if (formatParams->nPortIndex > kOutputPortIndex) {
                return OMX_ErrorUndefined;
            }

            if (formatParams->nIndex != 0) {
                return OMX_ErrorNoMore;
            }

            if (formatParams->nPortIndex == kInputPortIndex) {
	      if (mVideoFormat == VF_MPEG4){
                formatParams->eCompressionFormat = OMX_VIDEO_CodingMPEG4;
	      }else if (mVideoFormat == VF_H264){
		formatParams->eCompressionFormat = OMX_VIDEO_CodingAVC;
	      }else if (mVideoFormat == VF_WMV3){
		formatParams->eCompressionFormat = OMX_VIDEO_CodingWMV;
	      }else if (mVideoFormat == VF_RV40){
		formatParams->eCompressionFormat = OMX_VIDEO_CodingRV;
	      }
	      formatParams->eColorFormat = OMX_COLOR_FormatUnused;
	      formatParams->xFramerate = 0;
            } else {
                CHECK(formatParams->nPortIndex == kOutputPortIndex);

                formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
                formatParams->eColorFormat = OMX_COLOR_FormatYUV420Planar;
                formatParams->xFramerate = 0;

            }
            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
            OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
                    (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) params;

            if (profileLevel->nPortIndex != kInputPortIndex) {
                ALOGE("Invalid port index: %ld", profileLevel->nPortIndex);
                return OMX_ErrorUnsupportedIndex;
            }

            size_t index = profileLevel->nProfileIndex;
            size_t nProfileLevels =
                    sizeof(kProfileLevels) / sizeof(kProfileLevels[0]);
            if (index >= nProfileLevels) {
                return OMX_ErrorNoMore;
            }

            profileLevel->eProfile = kProfileLevels[index].mProfile;
            profileLevel->eLevel = kProfileLevels[index].mLevel;
            return OMX_ErrorNone;
        }

        default:
            return SimpleHardOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE HWDec::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
  ALOGV("internalSetParameter in index = %x, %x ",index, OMX_IndexParamStandardComponentRole);
    switch (index) {
        case 0x7F000011:
	{
	    //OMX.google.android.index.enableAndroidNativeBuffers
	    EnableAndroidNativeBuffersParams *pANBParams = (EnableAndroidNativeBuffersParams *) params;
	    if(pANBParams->nPortIndex == kOutputPortIndex 
	             && pANBParams->enable == OMX_TRUE) {
	      OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(pANBParams->nPortIndex)->mDef;
	      def->format.video.eColorFormat = (OMX_COLOR_FORMATTYPE) HAL_PIXEL_FORMAT_RGBA_8888;
	      if(mRenderer == NULL)
		mRenderer = new HardwareRenderer_FrameBuffer(def->format.video);
	    }
	    return OMX_ErrorNone;
	}

        case 0x7F000014:
	{
	    if (!vContext) {
	        vContext = (sh_video_t *)params;
		mVContextNeedFree = false;
		ALOGV("vContext=%x vContext->format=%x",vContext,vContext->format);
	    }
	    return OMX_ErrorNone;
	}

        case OMX_IndexParamStandardComponentRole:
        {
	    const OMX_PARAM_COMPONENTROLETYPE *roleParams =
	     (const OMX_PARAM_COMPONENTROLETYPE *)params;

	    ALOGV("roleParams->cRole =  %s", (const char *)roleParams->cRole);
	    if (strncmp((const char *)roleParams->cRole, "video_decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE - 1) == 0){
	      mVideoFormat = VF_MPEG4;
	    }else if (strncmp((const char *)roleParams->cRole, "video_decoder.avc", OMX_MAX_STRINGNAME_SIZE - 1) == 0){
	      mVideoFormat = VF_H264;
	    }else if (strncmp((const char *)roleParams->cRole, "video_decoder.wmv3", OMX_MAX_STRINGNAME_SIZE - 1) == 0){
	      mVideoFormat = VF_WMV3;
	    }else if (strncmp((const char *)roleParams->cRole, "video_decoder.rv40", OMX_MAX_STRINGNAME_SIZE - 1) == 0){
	      mVideoFormat = VF_RV40;
	    }else{
	      //return OMX_ErrorUndefined;
	      mVideoFormat = VF_H264;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

            if (formatParams->nPortIndex > kOutputPortIndex) {
                return OMX_ErrorUndefined;
            }

            if (formatParams->nIndex != 0) {
                return OMX_ErrorNoMore;
            }

            return OMX_ErrorNone;
        }
#if 1
	/*add by gysun : get w*h from ACodec.cpp*/
        case OMX_IndexParamPortDefinition:
	{
	  OMX_PARAM_PORTDEFINITIONTYPE *def = (OMX_PARAM_PORTDEFINITIONTYPE *)params;
	  mWidth = def->format.video.nFrameWidth;
	  mHeight = def->format.video.nFrameHeight;
	  ALOGE("change w*h=(%d*%d)",mWidth,mHeight);
	}
#endif
        default:
            return SimpleHardOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE HWDec::getConfig(
        OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
        case OMX_IndexConfigCommonOutputCrop:
        {
            OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)params;

            if (rectParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            rectParams->nLeft = mCropLeft;
            rectParams->nTop = mCropTop;
            rectParams->nWidth = mCropWidth;
            rectParams->nHeight = mCropHeight;

            return OMX_ErrorNone;
        }

        default:
            return OMX_ErrorUnsupportedIndex;
    }
}
#if 1
void HWDec::onQueueFilled(OMX_U32 portIndex) {
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

  if (mOutputPortSettingsChange != NONE) {
    return;
  }

  if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
    return;
  }

  List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
  List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
  //    H264SwDecRet ret = H264SWDEC_PIC_RDY;
  status_t err = OK;
  bool portSettingsChanged = false;
  while ((mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty())
	 && !outQueue.empty()) {

    if (mEOSStatus == INPUT_EOS_SEEN) {
      drainAllOutputBuffers();
      return;
    }

    BufferInfo *inInfo = *inQueue.begin();
    OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
    ++mPicId;
    if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
      inQueue.erase(inQueue.begin());
      inInfo->mOwnedByUs = false;
      notifyEmptyBufferDone(inHeader);
      mEOSStatus = INPUT_EOS_SEEN;
      continue;
    }

    OMX_BUFFERHEADERTYPE *header = new OMX_BUFFERHEADERTYPE;
    memset(header, 0, sizeof(OMX_BUFFERHEADERTYPE));
    header->nTimeStamp = inHeader->nTimeStamp;
    header->nFlags = inHeader->nFlags;
    
    inQueue.erase(inQueue.begin());
    //	OMX_U8 outBuf[1280*1280];
    OMX_U32 outLength = 0;
    OMX_U8 * pStream = inHeader->pBuffer + inHeader->nOffset;
    OMX_U32 inLength = inHeader->nFilledLen;
    OMX_BOOL drop_frame = OMX_FALSE;
    OMX_S32 frameCount = 0;
    OMX_PARAM_PORTDEFINITIONTYPE PortParam;
    PortParam.format.video.nFrameWidth = mCropWidth;
    PortParam.format.video.nFrameHeight = mCropHeight;

    /**/
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    BufferInfo *outInfo = *outQueue.begin();
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
    OMX_U8 *outBuf = outHeader->pBuffer + outHeader->nOffset;
    ////
    uint64_t mPts = inHeader->nTimeStamp; 
    if(mRenderer != NULL)
      outBuf = (OMX_U8*)mOutputBuf;
    mVideoDecoder->shContext->pts=((double)inHeader->nTimeStamp)/1000000.0;
    if(header->nFlags & OMX_BUFFERFLAG_SEEKFLAG)
      mVideoDecoder->shContext->seekFlag = 1;
    else
      mVideoDecoder->shContext->seekFlag = 0;
    OMX_BOOL ret = DecodeVideo(mVideoDecoder,
      			       (OMX_U8*)outBuf,
      			       (OMX_U32*)&outLength,		 
      			       (OMX_U8**)(&pStream),
      			       &inLength,
      			       &PortParam,
      			       &frameCount,
      			       (OMX_BOOL)1,
      			       &drop_frame);
    mPts=((PlanarImage*)outBuf)->pts;
    //ALOGE("mPts:%lld", mPts);
    if(ret == OMX_TRUE){
      if (((int)PortParam.format.video.nFrameWidth != mCropWidth )
	  || ((int)PortParam.format.video.nFrameHeight != mCropHeight)) {
	ALOGE("w h changed (%d * %d)!!",
	     (int)PortParam.format.video.nFrameWidth,(int)PortParam.format.video.nFrameHeight);

	mWidth  = (int)PortParam.format.video.nFrameWidth;
	mHeight = (int)PortParam.format.video.nFrameHeight;
	mPictureSize = mWidth * mHeight * 3 / 2;
	mCropWidth = mWidth;
	mCropHeight = mHeight;
	updatePortDefinitions();
	notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
	mOutputPortSettingsChange = AWAITING_DISABLED;		  
	portSettingsChanged = true;

	//
	notify(OMX_EventPortSettingsChanged, 1,
	       OMX_IndexConfigCommonOutputCrop, NULL);

	if(mRenderer != NULL){
	  mRenderer.clear();
	  mRenderer = new HardwareRenderer_FrameBuffer(PortParam.format.video);
	}
      }else if (outLength == 0){
	ALOGV("decode failed ,try next mpts = %lld",mPts);
	inInfo->mOwnedByUs = false;
	notifyEmptyBufferDone(inHeader);
	return;
      }else if (mRenderer != NULL){
	RenderData rdata;
	//rdata.input = const_cast<void*>(mOutputBuf);//damn ugly.
	rdata.input = mOutputBuf;//damn ugly.
	rdata.inputSize = outLength;
	rdata.platformPrivate = NULL;
	//rdata.state = state;
	rdata.needReinit = false;
	rdata.bufferHandle = (buffer_handle_t) outHeader->pBuffer;
	  
	mRenderer->render(&rdata);
      }
    }else{
      ALOGV("H264 video decode failed !!");
      err = ERROR_MALFORMED;
    }
	  
    inInfo->mOwnedByUs = false;
    notifyEmptyBufferDone(inHeader);

    if (portSettingsChanged) {
      portSettingsChanged = false;

      return;
    }

    if (mFirstPicture && !outQueue.empty()) {
      drainOneOutputBuffer(mFirstPictureId, mFirstPicture);
      delete[] mFirstPicture;
      mFirstPicture = NULL;
      mFirstPictureId = -1;
    }else{
      //            drainOneOutputBuffer(mPicId, (uint8_t*)outBuf);
      /*drain a frame */
      outQueue.erase(outQueue.begin());
#if 0
      OMX_BUFFERHEADERTYPE *header = mPicToHeaderMap.valueFor(mPicId);
      uint64_t mPts = inHeader->nTimeStamp; 
      int index=0;
      for(int i = 0; i< mPicToHeaderMap.size();i++){
	if((mPicToHeaderMap.valueAt(i))->nTimeStamp <= mPts){
	  header = mPicToHeaderMap.valueAt(i);
	  mPts = header->nTimeStamp;
	  index = i;
	}
      }
#endif
      //ALOGE("mPts=%lld",mPts);
      outHeader->nTimeStamp = mPts;
      outHeader->nFlags = header->nFlags;
      outHeader->nFilledLen = mPictureSize;

      outInfo->mOwnedByUs = false;
      notifyFillBufferDone(outHeader);
    }

    if (err != OK) {
      notify(OMX_EventError, OMX_ErrorUndefined, err, NULL);
    }
  }
}
#else
#endif

#if 0
bool HWDec::handlePortSettingChangeEvent(const H264SwDecInfo *info) {
  ALOGV("handlePortSettingChangeEvent in");
  ALOGV("info w*h = (%d * %d)",info->picWidth,info->picHeight);
    if (mWidth != info->picWidth || mHeight != info->picHeight) {
        mWidth  = info->picWidth;
        mHeight = info->picHeight;
        mPictureSize = mWidth * mHeight * 3 / 2;
        mCropWidth = mWidth;
        mCropHeight = mHeight;
        updatePortDefinitions();
        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        return true;
    }

    return false;
}
#endif
bool HWDec::handleCropRectEvent(const CropParams *crop) {
  ALOGV("handleCropRectEvent in crop w*h = (%d *%d)",crop->cropOutWidth,crop->cropOutHeight);
    if (mCropLeft != crop->cropLeftOffset ||
        mCropTop != crop->cropTopOffset ||
        mCropWidth != crop->cropOutWidth ||
        mCropHeight != crop->cropOutHeight) {
        mCropLeft = crop->cropLeftOffset;
        mCropTop = crop->cropTopOffset;
        mCropWidth = crop->cropOutWidth;
        mCropHeight = crop->cropOutHeight;

        notify(OMX_EventPortSettingsChanged, 1,
                OMX_IndexConfigCommonOutputCrop, NULL);

        return true;
    }
    return false;
}

void HWDec::saveFirstOutputBuffer(int32_t picId, uint8_t *data) {
  ALOGV("saveFirstOutputBuffer in");
    CHECK(mFirstPicture == NULL);
    mFirstPictureId = picId;

    mFirstPicture = new uint8_t[mPictureSize];
    memcpy(mFirstPicture, data, mPictureSize);
}

void HWDec::drainOneOutputBuffer(int32_t picId, uint8_t* data) {
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    BufferInfo *outInfo = *outQueue.begin();
    outQueue.erase(outQueue.begin());
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
    OMX_BUFFERHEADERTYPE *header = mPicToHeaderMap.valueFor(picId);
    //  ALOGV("drainOneOutputBuffer nTimeStamp(%lld) flags(%d),mPictureSize=(%d)",header->nTimeStamp,header->nFlags,mPictureSize);
    outHeader->nTimeStamp = header->nTimeStamp;
    outHeader->nFlags = header->nFlags;
    outHeader->nFilledLen = mPictureSize;
    memcpy(outHeader->pBuffer + outHeader->nOffset,
            data, mPictureSize);
    mPicToHeaderMap.removeItem(picId);
    delete header;
    outInfo->mOwnedByUs = false;
    notifyFillBufferDone(outHeader);
}

bool HWDec::drainAllOutputBuffers() {
  ALOGV("drainAllOutputBuffers in");
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    ALOGV("outQueue.size = %d",outQueue.size());
#if 1 
    for(int i = 0;i< (outQueue.size()-1);i++){
      BufferInfo *outInfo = *outQueue.begin();
      outQueue.erase(outQueue.begin());
    }
    /*last*/
    BufferInfo *outInfo = *outQueue.begin();
    outQueue.erase(outQueue.begin());
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

    outHeader->nTimeStamp = 0;
    outHeader->nFilledLen = 0;
    outHeader->nFlags = OMX_BUFFERFLAG_EOS;
    mEOSStatus = OUTPUT_FRAMES_FLUSHED;

    outInfo->mOwnedByUs = false;
    notifyFillBufferDone(outHeader);
#else
    H264SwDecPicture decodedPicture;
    while (!outQueue.empty()) {
        BufferInfo *outInfo = *outQueue.begin();
        outQueue.erase(outQueue.begin());
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
        if (mHeadersDecoded &&
            H264SWDEC_PIC_RDY ==
                H264SwDecNextPicture(mHandle, &decodedPicture, 1 /* flush */)) {

            int32_t picId = decodedPicture.picId;
            CHECK(mPicToHeaderMap.indexOfKey(picId) >= 0);

            memcpy(outHeader->pBuffer + outHeader->nOffset,
                decodedPicture.pOutputPicture,
                mPictureSize);

            OMX_BUFFERHEADERTYPE *header = mPicToHeaderMap.valueFor(picId);
            outHeader->nTimeStamp = header->nTimeStamp;
            outHeader->nFlags = header->nFlags;
            outHeader->nFilledLen = mPictureSize;
            mPicToHeaderMap.removeItem(picId);
            delete header;
        } else {
            outHeader->nTimeStamp = 0;
            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            mEOSStatus = OUTPUT_FRAMES_FLUSHED;
        }

        outInfo->mOwnedByUs = false;
        notifyFillBufferDone(outHeader);
    }
#endif
    return true;
}

void HWDec::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == kInputPortIndex) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
	mVideoDecoder->shContext->seekFlag = 1;
    }
}

void HWDec::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

void HWDec::updatePortDefinitions() {
  ALOGV("updatePortDefinitions in");
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(0)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def = &editPortInfo(1)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def->nBufferSize =
        (def->format.video.nFrameWidth
            * def->format.video.nFrameHeight * 3) / 2;
}

}  // namespace android

OMX_ERRORTYPE createHardOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    ALOGV("video createHardOMXComponent in");
    sp<HardOMXComponent> codec = new android::HWDec(name, callbacks, appData, component);
    if (codec == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    
    OMX_ERRORTYPE err = codec->initCheck();
    if (err != OMX_ErrorNone)
      return err;
    
    codec->incStrong(NULL);

    return OMX_ErrorNone;
}
