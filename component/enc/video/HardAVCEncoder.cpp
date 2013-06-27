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

#define LOG_NDEBUG 0
#define LOG_TAG "HardAVCEncoder"
#include <utils/Log.h>
#define EL(x,y...) //{ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y);}

#include "OMX_Video.h"

#include <HardwareAPI.h>
#include <MetadataBufferType.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>

#include "HardAVCEncoder.h"

typedef struct {
    int b_progress;
    int i_seek;
    void * hin;
    void * hout;
    FILE *qpfile;
} cli_opt_t;

int  VAE_map();
void VAE_unmap();
void Lock_Vpu();
void UnLock_Vpu();

using namespace android;
extern "C" {
  void    x264_param_default( x264_param_t * );
  int     x264_encoder_headers( x264_t *h, x264_nal_t **pp_nal, int *pi_nal );
  int     Parse( int argc, char **argv, x264_param_t *param, cli_opt_t *opt );
  x264_t *x264_encoder_open( x264_param_t * );
  int     x264_encoder_encode ( x264_t *, x264_nal_t **, int *, x264_picture_t *, x264_picture_t * );
  void    x264_encoder_close  ( x264_t * );
  short   crc(unsigned char * data_buf, int byte_num, short test);
  void *  jz4740_alloc_frame (int *VpuMem_ptr, int align, int size){
    VpuMem * vmem=(VpuMem*)VpuMem_ptr;
    int vaddr=(int)(vmem->vpu_mem_alloc(align+size));
    vaddr=(vaddr+align-1)&(~(align-1));
    return (void*)vaddr;
  }

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

typedef struct LevelConversion {
    OMX_U32 omxLevel;
    AVCLevel avcLevel;
} LevelConcersion;

static LevelConversion ConversionTable[] = {
    { OMX_VIDEO_AVCLevel1,  AVC_LEVEL1_B },
    { OMX_VIDEO_AVCLevel1b, AVC_LEVEL1   },
    { OMX_VIDEO_AVCLevel11, AVC_LEVEL1_1 },
    { OMX_VIDEO_AVCLevel12, AVC_LEVEL1_2 },
    { OMX_VIDEO_AVCLevel13, AVC_LEVEL1_3 },
    { OMX_VIDEO_AVCLevel2,  AVC_LEVEL2 },
#if 1
    // encoding speed is very poor if video
    // resolution is higher than CIF
    { OMX_VIDEO_AVCLevel21, AVC_LEVEL2_1 },
    { OMX_VIDEO_AVCLevel22, AVC_LEVEL2_2 },
    { OMX_VIDEO_AVCLevel3,  AVC_LEVEL3   },
    { OMX_VIDEO_AVCLevel31, AVC_LEVEL3_1 },
    { OMX_VIDEO_AVCLevel32, AVC_LEVEL3_2 },
    { OMX_VIDEO_AVCLevel4,  AVC_LEVEL4   },
    { OMX_VIDEO_AVCLevel41, AVC_LEVEL4_1 },
    { OMX_VIDEO_AVCLevel42, AVC_LEVEL4_2 },
    { OMX_VIDEO_AVCLevel5,  AVC_LEVEL5   },
    { OMX_VIDEO_AVCLevel51, AVC_LEVEL5_1 },
#endif
};

static status_t ConvertOmxAvcLevelToAvcSpecLevel(
        OMX_U32 omxLevel, AVCLevel *avcLevel) {
    for (size_t i = 0, n = sizeof(ConversionTable)/sizeof(ConversionTable[0]);
        i < n; ++i) {
        if (omxLevel == ConversionTable[i].omxLevel) {
            *avcLevel = ConversionTable[i].avcLevel;
            return OK;
        }
    }

    ALOGE("ConvertOmxAvcLevelToAvcSpecLevel: %d level not supported",
            (int32_t)omxLevel);

    return BAD_VALUE;
}

static status_t ConvertAvcSpecLevelToOmxAvcLevel(
    AVCLevel avcLevel, OMX_U32 *omxLevel) {
    for (size_t i = 0, n = sizeof(ConversionTable)/sizeof(ConversionTable[0]);
        i < n; ++i) {
        if (avcLevel == ConversionTable[i].avcLevel) {
            *omxLevel = ConversionTable[i].omxLevel;
            return OK;
        }
    }

    ALOGE("ConvertAvcSpecLevelToOmxAvcLevel: %d level not supported",
            (int32_t) avcLevel);

    return BAD_VALUE;
}

inline static void ConvertYUV420SemiPlanarToYUV420Planar(
        uint8_t *inyuv, uint8_t* outyuv,
        int32_t width, int32_t height) {

    int32_t outYsize = width * height;
    uint32_t *outy =  (uint32_t *) outyuv;
    uint16_t *outcb = (uint16_t *) (outyuv + outYsize);
    uint16_t *outcr = (uint16_t *) (outyuv + outYsize + (outYsize >> 2));

    /* Y copying */
    memcpy(outy, inyuv, outYsize);

    /* U & V copying */
    uint32_t *inyuv_4 = (uint32_t *) (inyuv + outYsize);
    for (int32_t i = height >> 1; i > 0; --i) {
        for (int32_t j = width >> 2; j > 0; --j) {
            uint32_t temp = *inyuv_4++;
            uint32_t tempU = temp & 0xFF;
            tempU = tempU | ((temp >> 8) & 0xFF00);

            uint32_t tempV = (temp >> 8) & 0xFF;
            tempV = tempV | ((temp >> 16) & 0xFF00);

            // Flip U and V
            *outcb++ = tempV;
            *outcr++ = tempU;
        }
    }
}

static void* MallocWrapper(
        void *userData, int32_t size, int32_t attrs) {
    void *ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

static void FreeWrapper(void *userData, void* ptr) {
    free(ptr);
}

static int32_t DpbAllocWrapper(void *userData,
        unsigned int sizeInMbs, unsigned int numBuffers) {
    HardAVCEncoder *encoder = static_cast<HardAVCEncoder *>(userData);
    CHECK(encoder != NULL);
    return encoder->allocOutputBuffers(sizeInMbs, numBuffers);
}

static int32_t BindFrameWrapper(
        void *userData, int32_t index, uint8_t **yuv) {
    HardAVCEncoder *encoder = static_cast<HardAVCEncoder *>(userData);
    CHECK(encoder != NULL);
    return encoder->bindOutputBuffer(index, yuv);
}

static void UnbindFrameWrapper(void *userData, int32_t index) {
    HardAVCEncoder *encoder = static_cast<HardAVCEncoder *>(userData);
    CHECK(encoder != NULL);
    return encoder->unbindOutputBuffer(index);
}

HardAVCEncoder::HardAVCEncoder(
            const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component)
    : SimpleHardOMXComponent(name, callbacks, appData, component),
      mVideoWidth(176),
      mVideoHeight(144),
      mVideoFrameRate(30),
      mVideoBitRate(512000),
      mVideoColorFormat(OMX_COLOR_FormatYUV420Planar),
      mStoreMetaDataInBuffers(false),
      mIDRFrameRefreshIntervalInSec(1),
      mAVCEncProfile(AVC_MAIN),
      mAVCEncLevel(AVC_LEVEL3),
      mNumInputFrames(-1),
      mPrevTimestampUs(-1),
      mStarted(false),
      mSawInputEOS(false),
      mSignalledError(false),
      mInputFrameData(NULL),
      mSliceGroup(NULL) {

    initPorts();
    ALOGI("Construct HardAVCEncoder================");
}

HardAVCEncoder::~HardAVCEncoder() {
    ALOGV("Destruct HardAVCEncoder");
    releaseEncoder();
    List<BufferInfo *> &outQueue = getPortQueue(1);
    List<BufferInfo *> &inQueue = getPortQueue(0);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());
}

OMX_ERRORTYPE HardAVCEncoder::initEncParams() {
  
    mCsdData = NULL;
    mCsdSize = 0;
    mNALHolder = NULL;
    mNALUCount = 0;
    mNALUIndex = 0;
    mNALUHasStartCode = 0;
    mEncArgQP = 0;

    if (mVideoWidth % 16 != 0 || mVideoHeight % 16 != 0) {
        ALOGE("Video frame size %dx%d must be a multiple of 16",
	      mVideoWidth, mVideoHeight);
        return OMX_ErrorBadParameter;
    }

    i_frame = 0;
    x264_param_default( &mParam );
    mParam.i_csp = X264_CSP_YUYV;
    mParam.VpuMem_ptr=(int*)(&mVpuMem);

    mParam.i_width=mVideoWidth;
    mParam.i_height=mVideoHeight;
    mParam.i_fps_num=mVideoFrameRate;
    mParam.rc.i_fbr_bitrate=mVideoBitRate;

    cli_opt_t opt;
    char * argv[] = {
      "x264",
      "--bframes",        "0",
      "--me",             "dia",
      "--subme",          "1",
      "--trellis",        "0",
      "--weightp",        "0",
      "--ref",            "1",
      "--partition",      "none",
      "--qp",             "26",
      "--sync-lookahead", "0",
      "--rc-lookahead",   "0",
      "--aq-mod",         "0",
      "--no-8x8dct",
      "--ratetol",        "1.0",
      "--keyint",         "25"
    };
    
    int argc = sizeof(argv)/sizeof(argv[0]);

    if( Parse( argc, argv, &mParam, &opt ) < 0 ){
      ALOGE("Parse failed!");
      return OMX_ErrorBadParameter;
    }

    mParam.i_frame_total = 0;

    VAE_map();

    if( ( h = x264_encoder_open( &mParam ) ) == NULL ){
      ALOGE( "x264 [error]: x264_encoder_open failed\n" );
      return OMX_ErrorBadParameter;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE HardAVCEncoder::initEncoder() {
    CHECK(!mStarted);

    OMX_ERRORTYPE errType = OMX_ErrorNone;
    if (OMX_ErrorNone != (errType = initEncParams())) {
        ALOGE("Failed to initialized encoder params");
        mSignalledError = true;
        notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
        return errType;
    }

    mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS
    mSpsPpsHeaderReceived = false;
    mReadyForNextFrame = true;
    mIsIDRFrame = false;
    mStarted = true;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE HardAVCEncoder::releaseEncoder() {
    if (!mStarted) {
        return OMX_ErrorNone;
    }

    if(mCsdData)
      delete mCsdData;

    x264_encoder_close( h );
    
    VAE_unmap();

    releaseOutputBuffers();

    mStarted = false;

    return OMX_ErrorNone;
}

void HardAVCEncoder::releaseOutputBuffers() {
    for (size_t i = 0; i < mOutputBuffers.size(); ++i) {
        MediaBuffer *buffer = mOutputBuffers.editItemAt(i);
        buffer->setObserver(NULL);
        buffer->release();
    }
    mOutputBuffers.clear();
}

void HardAVCEncoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    //    ALOGE("%d+++++++++++%d        %d", __LINE__, mVideoWidth, mVideoHeight);

    const size_t kInputBufferSize = (mVideoWidth * mVideoHeight * 3) >> 1;//yuv420 tile from camera

    const size_t kOutputBufferSize = 512*1024;

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kInputBufferSize;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    //    ALOGE("%d+++++++++++%d        %d", __LINE__, mVideoWidth, mVideoHeight);

    def.format.video.cMIMEType = const_cast<char *>("video/raw");
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    def.format.video.xFramerate = (mVideoFrameRate << 16);  // Q16 format
    def.format.video.nBitrate = mVideoBitRate;
    def.format.video.nFrameWidth = mVideoWidth;
    def.format.video.nFrameHeight = mVideoHeight;
    def.format.video.nStride = mVideoWidth;
    def.format.video.nSliceHeight = mVideoHeight;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kOutputBufferSize;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>("video/avc");
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.xFramerate = (0 << 16);  // Q16 format
    def.format.video.nBitrate = mVideoBitRate;
    def.format.video.nFrameWidth = mVideoWidth;
    def.format.video.nFrameHeight = mVideoHeight;
    def.format.video.nStride = mVideoWidth;
    def.format.video.nSliceHeight = mVideoHeight;

    addPort(def);
}

OMX_ERRORTYPE HardAVCEncoder::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamVideoErrorCorrection:
        {
            return OMX_ErrorNotImplemented;
        }

        case OMX_IndexParamVideoBitrate:
        {
            OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
                (OMX_VIDEO_PARAM_BITRATETYPE *) params;

            if (bitRate->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            bitRate->eControlRate = OMX_Video_ControlRateVariable;
            bitRate->nTargetBitrate = mVideoBitRate;
            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

            if (formatParams->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            if (formatParams->nIndex > 2) {
                return OMX_ErrorNoMore;
            }

            if (formatParams->nPortIndex == 0) {
                formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
                if (formatParams->nIndex == 0) {
                    formatParams->eColorFormat = OMX_COLOR_FormatYUV420Planar;
                } else if (formatParams->nIndex == 1) {
                    formatParams->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
                } else {
                    formatParams->eColorFormat = OMX_COLOR_FormatAndroidOpaque;
                }
            } else {
                formatParams->eCompressionFormat = OMX_VIDEO_CodingAVC;
                formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoAvc:
        {
            OMX_VIDEO_PARAM_AVCTYPE *avcParams =
                (OMX_VIDEO_PARAM_AVCTYPE *)params;

            if (avcParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            avcParams->eProfile = OMX_VIDEO_AVCProfileMain;
            OMX_U32 omxLevel = AVC_LEVEL3;
            if (OMX_ErrorNone !=
                ConvertAvcSpecLevelToOmxAvcLevel(mAVCEncLevel, &omxLevel)) {
                return OMX_ErrorUndefined;
            }

            avcParams->eLevel = (OMX_VIDEO_AVCLEVELTYPE) omxLevel;
            avcParams->nRefFrames = 1;
            avcParams->nBFrames = 0;
            avcParams->bUseHadamard = OMX_TRUE;
            avcParams->nAllowedPictureTypes =
                    (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP);
            avcParams->nRefIdx10ActiveMinus1 = 0;
            avcParams->nRefIdx11ActiveMinus1 = 0;
            avcParams->bWeightedPPrediction = OMX_FALSE;
            avcParams->bEntropyCodingCABAC = OMX_TRUE;
            avcParams->bconstIpred = OMX_FALSE;
            avcParams->bDirect8x8Inference = OMX_FALSE;
            avcParams->bDirectSpatialTemporal = OMX_FALSE;
            avcParams->nCabacInitIdc = 0;
            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
            OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
                (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)params;

            if (profileLevel->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            const size_t size =
                    sizeof(ConversionTable) / sizeof(ConversionTable[0]);

            if (profileLevel->nProfileIndex >= size) {
                return OMX_ErrorNoMore;
            }

            profileLevel->eProfile = OMX_VIDEO_AVCProfileMain;
            profileLevel->eLevel = ConversionTable[profileLevel->nProfileIndex].omxLevel;

            return OMX_ErrorNone;
        }

        default:
            return SimpleHardOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE HardAVCEncoder::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    int32_t indexFull = index;

    switch (indexFull) {
        case OMX_IndexParamVideoErrorCorrection:
        {
            return OMX_ErrorNotImplemented;
        }

        case OMX_IndexParamVideoBitrate:
        {
            OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
                (OMX_VIDEO_PARAM_BITRATETYPE *) params;

            if (bitRate->nPortIndex != 1 ||
                bitRate->eControlRate != OMX_Video_ControlRateVariable) {
                return OMX_ErrorUndefined;
            }

            mVideoBitRate = bitRate->nTargetBitrate;
            return OMX_ErrorNone;
        }

        case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *def =
                (OMX_PARAM_PORTDEFINITIONTYPE *)params;
            if (def->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            if (def->nPortIndex == 0) {
                if (def->format.video.eCompressionFormat != OMX_VIDEO_CodingUnused ||
                    (def->format.video.eColorFormat != OMX_COLOR_FormatYUV420Planar &&
                     def->format.video.eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar &&
                     def->format.video.eColorFormat != OMX_COLOR_FormatAndroidOpaque)) {
                    return OMX_ErrorUndefined;
                }
            } else {
                if (def->format.video.eCompressionFormat != OMX_VIDEO_CodingAVC ||
                    (def->format.video.eColorFormat != OMX_COLOR_FormatUnused)) {
                    return OMX_ErrorUndefined;
                }
            }

            OMX_ERRORTYPE err = SimpleHardOMXComponent::internalSetParameter(index, params);
            if (OMX_ErrorNone != err) {
                return err;
            }

            if (def->nPortIndex == 0) {
                mVideoWidth = def->format.video.nFrameWidth;
                mVideoHeight = def->format.video.nFrameHeight;
                mVideoFrameRate = def->format.video.xFramerate >> 16;
                mVideoColorFormat = def->format.video.eColorFormat;
            } else {
                mVideoBitRate = def->format.video.nBitrate;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;

            if (strncmp((const char *)roleParams->cRole,
                        "video_encoder.avc",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoPortFormat:
        {
            const OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
                (const OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

            if (formatParams->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            if (formatParams->nIndex > 2) {
                return OMX_ErrorNoMore;
            }

            if (formatParams->nPortIndex == 0) {
                if (formatParams->eCompressionFormat != OMX_VIDEO_CodingUnused ||
                    ((formatParams->nIndex == 0 &&
                      formatParams->eColorFormat != OMX_COLOR_FormatYUV420Planar) ||
                    (formatParams->nIndex == 1 &&
                     formatParams->eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar) ||
                    (formatParams->nIndex == 2 &&
                     formatParams->eColorFormat != OMX_COLOR_FormatAndroidOpaque) )) {
                    return OMX_ErrorUndefined;
                }
                mVideoColorFormat = formatParams->eColorFormat;
            } else {
                if (formatParams->eCompressionFormat != OMX_VIDEO_CodingAVC ||
                    formatParams->eColorFormat != OMX_COLOR_FormatUnused) {
                    return OMX_ErrorUndefined;
                }
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoAvc:
        {
            OMX_VIDEO_PARAM_AVCTYPE *avcType =
                (OMX_VIDEO_PARAM_AVCTYPE *)params;

            if (avcType->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            // PV's AVC encoder only supports baseline profile
            if (avcType->eProfile != OMX_VIDEO_AVCProfileMain ||
                avcType->nRefFrames != 1 ||
                avcType->nBFrames != 0 ||
                avcType->bUseHadamard != OMX_TRUE ||
                (avcType->nAllowedPictureTypes & OMX_VIDEO_PictureTypeB) != 0 ||
                avcType->nRefIdx10ActiveMinus1 != 0 ||
                avcType->nRefIdx11ActiveMinus1 != 0 ||
                avcType->bWeightedPPrediction != OMX_FALSE ||
                avcType->bEntropyCodingCABAC != OMX_TRUE ||
                avcType->bconstIpred != OMX_FALSE ||
                avcType->bDirect8x8Inference != OMX_FALSE ||
                avcType->bDirectSpatialTemporal != OMX_FALSE ||
                avcType->nCabacInitIdc != 0) {
	      // we support any paras because we ignore them now
	      //                return OMX_ErrorUndefined;
            }

            if (OK != ConvertOmxAvcLevelToAvcSpecLevel(avcType->eLevel, &mAVCEncLevel)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case kStoreMetaDataExtensionIndex:
        {
            StoreMetaDataInBuffersParams *storeParams =
                    (StoreMetaDataInBuffersParams*)params;
            if (storeParams->nPortIndex != 0) {
                ALOGV("%s: StoreMetadataInBuffersParams.nPortIndex not zero!",
                        __FUNCTION__);
                return OMX_ErrorUndefined;
            }

            mStoreMetaDataInBuffers = storeParams->bStoreMetaData;
            ALOGV("StoreMetaDataInBuffers set to: %s",
                    mStoreMetaDataInBuffers ? " true" : "false");

            if (mStoreMetaDataInBuffers) {
                mVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar;
                if (mInputFrameData == NULL) {
                    mInputFrameData =
                            (uint8_t *) malloc((mVideoWidth * mVideoHeight * 3 ) >> 1);
                }
            }

            return OMX_ErrorNone;
        }

        default:
            return SimpleHardOMXComponent::internalSetParameter(index, params);
    }
}

void HardAVCEncoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mSawInputEOS) {
        return;
    }

    if (!mStarted) {
        if (OMX_ErrorNone != initEncoder()) {
            return;
        }
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!mSawInputEOS && !inQueue.empty() && !outQueue.empty()) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

        outHeader->nTimeStamp = 0;
        outHeader->nFlags = 0;
        outHeader->nOffset = 0;
        outHeader->nFilledLen = 0;
        outHeader->nOffset = 0;

        uint8_t *outPtr = (uint8_t *) outHeader->pBuffer;
        uint32_t dataLength = outHeader->nAllocLen;
	uint8_t *inputData = NULL;

        int32_t type;
        buffer_handle_t srcBuffer; // for MetaDataMode only

	/*encode a frame*/
	x264_picture_t pic_out;
	x264_nal_t *nal;
	int i_nal, i, i_nalu_size;

	if(!mSpsPpsHeaderReceived){
	  x264_encoder_headers( h, &nal, &i_nal );
	  
	  for(i=0;i<i_nal;i++){
	    uint8_t type = (nal[i].p_payload)[4] & 0x1f;
	    switch ( type ) {
	    case 0x07:
	      memcpy(outPtr, nal[i].p_payload, nal[i].i_payload);
	      outHeader->nFilledLen = nal[i].i_payload;
	      outPtr += nal[i].i_payload;
	      break;
	    case 0x08:
	      memcpy(outPtr, nal[i].p_payload, nal[i].i_payload);
	      outHeader->nFilledLen += nal[i].i_payload;
	      outPtr += nal[i].i_payload;
	      break;
	    }
	  }
	  ++mNumInputFrames;
	  EL("outHeader->nFilledLen=%d",outHeader->nFilledLen);
	  mSpsPpsHeaderReceived = true;
	  CHECK_EQ(0, mNumInputFrames);  // 1st video frame is 0
	  outHeader->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
	  outQueue.erase(outQueue.begin());
	  outInfo->mOwnedByUs = false;
	  notifyFillBufferDone(outHeader);
	  return;
	}

        // Get next input video frame
        if (mReadyForNextFrame) {
            // Save the input buffer info so that it can be
            // passed to an output buffer
            InputBufferInfo info;
            info.mTimeUs = inHeader->nTimeStamp;
            info.mFlags = inHeader->nFlags;
            mInputBufferInfoVec.push(info);
            mPrevTimestampUs = inHeader->nTimeStamp;

            if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                mSawInputEOS = true;
            }

            if (inHeader->nFilledLen > 0) {
	      inputData = (uint8_t *)inHeader->pBuffer + inHeader->nOffset;
	      CHECK(inputData != NULL);
	      ++mNumInputFrames;
            }
        }

	x264_picture_t pic;
	memset(&pic, 0, sizeof(x264_picture_t));
    
	pic.img.plane[0] = NULL;
	pic.img.plane[1] = NULL;
	pic.img.plane[2] = NULL;

	pic.i_pts = (int64_t)i_frame * 1;
    
	/* Do not force any parameters */
	pic.i_type = X264_TYPE_AUTO;
	pic.i_qpplus1 = 0;
    
	pic.img.i_plane = 0;
	pic.param = NULL;

	/*encode a frame*/
	{
	  dmmu_mem_info meminfo;
	  meminfo.size=inHeader->nFilledLen - inHeader->nOffset;
          // ALOGI("dmmu_map_user_memory %d", meminfo.size);
	  meminfo.vaddr=inputData;
	  meminfo.pages_phys_addr_table=NULL;
	  int err=dmmu_map_user_memory(&meminfo);
	}
	//pic.img.raw_yuv422_ptr = (uint32_t *)mInputBuffer->data();
	pic.img.raw_yuv422_ptr = (uint32_t *)inputData;
	//time0 = GetTimer();
	    
	Lock_Vpu();
	if( x264_encoder_encode( h, &nal, &i_nal, &pic, &pic_out ) < 0 ){
	  UnLock_Vpu();
	  ALOGE("x264 [error]: x264_encoder_encode failed\n" );
	  mSignalledError = true;
	  releaseGrallocData(srcBuffer);
	  notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
	  
	  return;
	}
	UnLock_Vpu();
	// time1 = GetTimer();

	i_frame++;
	dataLength = 0;

	for(i=0;i<i_nal;i++){
#ifdef WRITE_H264RAW_STREAM
	  fwrite(nal[i].p_payload, 1, nal[i].i_payload, rawh264_f);   
#endif
	  uint8_t type = (nal[i].p_payload)[4] & 0x1f;
	  switch ( type ) {
	  case 0x5://I nal
	    outHeader->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
	  case 0x1://P nal
	    int size = nal[i].i_payload - 4;
	    if((dataLength + size) > outHeader->nAllocLen){
	      ALOGE("x264 frame outputBuffer is underflow!!");
	      mSignalledError = true;
	      releaseGrallocData(srcBuffer);
	      notify(OMX_EventError, OMX_ErrorOverflow, 0, 0);
	      return;
	    };

	    memcpy(outPtr+dataLength, nal[i].p_payload + 4, size);
	    dataLength += size;	    
	    break;
	  }
	}
	//time1 = GetTimer();
#ifdef WRITE_H264RAW_STREAM
	fclose(rawh264_f);
#endif

        inQueue.erase(inQueue.begin());
        inInfo->mOwnedByUs = false;
        releaseGrallocData(srcBuffer);
        notifyEmptyBufferDone(inHeader);

        outQueue.erase(outQueue.begin());
        CHECK(!mInputBufferInfoVec.empty());
        InputBufferInfo *inputBufInfo = mInputBufferInfoVec.begin();
        outHeader->nTimeStamp = inputBufInfo->mTimeUs;
        outHeader->nFlags |= (inputBufInfo->mFlags | OMX_BUFFERFLAG_ENDOFFRAME);
        if (mSawInputEOS) {
            outHeader->nFlags |= OMX_BUFFERFLAG_EOS;
        }
        outHeader->nFilledLen = dataLength;
        outInfo->mOwnedByUs = false;
        notifyFillBufferDone(outHeader);
        mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
    }
}

int32_t HardAVCEncoder::allocOutputBuffers(
        unsigned int sizeInMbs, unsigned int numBuffers) {
    CHECK(mOutputBuffers.isEmpty());
    size_t frameSize = (sizeInMbs << 7) * 3;
    for (unsigned int i = 0; i <  numBuffers; ++i) {
        MediaBuffer *buffer = new MediaBuffer(frameSize);
        buffer->setObserver(this);
        mOutputBuffers.push(buffer);
    }

    return 1;
}

void HardAVCEncoder::unbindOutputBuffer(int32_t index) {
    CHECK(index >= 0);
}

int32_t HardAVCEncoder::bindOutputBuffer(int32_t index, uint8_t **yuv) {
    CHECK(index >= 0);
    CHECK(index < (int32_t) mOutputBuffers.size());
    *yuv = (uint8_t *) mOutputBuffers[index]->data();

    return 1;
}

void HardAVCEncoder::signalBufferReturned(MediaBuffer *buffer) {
    ALOGV("signalBufferReturned: %p", buffer);
}

OMX_ERRORTYPE HardAVCEncoder::getExtensionIndex(
        const char *name, OMX_INDEXTYPE *index) {
    if (!strcmp(name, "OMX.google.android.index.storeMetaDataInBuffers")) {
        *(int32_t*)index = kStoreMetaDataExtensionIndex;
        return OMX_ErrorNone;
    }
    return OMX_ErrorUndefined;
}

uint8_t *HardAVCEncoder::extractGrallocData(void *data, buffer_handle_t *buffer) {
    OMX_U32 type = *(OMX_U32*)data;
    status_t res;
    if (type != kMetadataBufferTypeGrallocSource) {
        ALOGE("Data passed in with metadata mode does not have type "
                "kMetadataBufferTypeGrallocSource (%d), has type %ld instead",
                kMetadataBufferTypeGrallocSource, type);
        return NULL;
    }
    buffer_handle_t imgBuffer = *(buffer_handle_t*)((uint8_t*)data + 4);

    const Rect rect(mVideoWidth, mVideoHeight);
    uint8_t *img;
    res = GraphicBufferMapper::get().lock(imgBuffer,
            GRALLOC_USAGE_HW_VIDEO_ENCODER,
            rect, (void**)&img);
    if (res != OK) {
        ALOGE("%s: Unable to lock image buffer %p for access", __FUNCTION__,
                imgBuffer);
        return NULL;
    }

    *buffer = imgBuffer;
    return img;
}

void HardAVCEncoder::releaseGrallocData(buffer_handle_t buffer) {
    if (mStoreMetaDataInBuffers) {
        GraphicBufferMapper::get().unlock(buffer);
    }
}

  void* VpuMem::vpu_mem_alloc(int size){
    mDevBuffers.push();
    MemoryHeapBase** devbuf = &mDevBuffers.editItemAt(mDevBuffers.size() - 1);
    EL("devbuf=0x%x,*devbuf=0x%x",devbuf,*devbuf);
    *devbuf = new MemoryHeapBase(size);
    EL("MemoryHeapBase base=0x%x,size=0x%x",(*devbuf)->getBase(),(*devbuf)->getSize());
    int ret_size=(*devbuf)->getSize();
    void * vaddr=(*devbuf)->getBase();
    //replace for memset, get all tlb entry
    {
      unsigned char * head = (unsigned char *)vaddr;
      int i;
      for(i=0;i<ret_size;i+=0x1000)
	head[i]=0;
      head[ret_size-1]=0;
    }
    dmmu_mem_info meminfo;
    meminfo.size=ret_size;
    meminfo.vaddr=vaddr;
    meminfo.pages_phys_addr_table=NULL;
    int err=dmmu_map_user_memory(&meminfo);
    if(err==-1){
      ALOGE("dmmu map error, media will crash");
      *(volatile int *)0x80000001 = 0;    
    }
    EL("dmmu pages_phys_addr_table=0x%x, vaddr=0x%x, paddr=0x%x",
       meminfo.pages_phys_addr_table,meminfo.vaddr,meminfo.paddr); 
    return vaddr;
  }

  VpuMem::~VpuMem(){
    dmmu_mem_info meminfo;
    for (int i = 0; i < mDevBuffers.size(); i++) {
      MemoryHeapBase* mptr = mDevBuffers.editItemAt(i);
      meminfo.size=mptr->getSize();
      meminfo.vaddr=mptr->getBase();
      meminfo.pages_phys_addr_table=NULL;
      dmmu_unmap_user_memory(&meminfo);
      delete mptr;
    }
  }

}  // namespace android

OMX_ERRORTYPE createHardOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    sp<HardOMXComponent> codec = new android::HardAVCEncoder(name, callbacks, appData, component);
    if (codec == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    
    OMX_ERRORTYPE err = codec->initCheck();
    if (err != OMX_ErrorNone)
      return err;
    
    codec->incStrong(NULL);

    return OMX_ErrorNone;
}
