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
#ifndef LUME_DEC_H_INCLUDED
#define LUME_DEC_H_INCLUDED

#ifndef OMX_Component_h
#include "OMX_Component.h"
#endif
#include "binder/MemoryHeapBase.h"
#include "dmmu.h"

#ifdef __cplusplus
extern "C"{
#include "stream.h"
#include "demuxer.h"
#include "stheader.h"

#include "codec-cfg.h"
#include "avcodec.h"

#include "mp_image.h"
#include "libavutil/mem.h"
#include "img_format.h"
#include "intreadwrite.h"
#include "fmt-conversion.h"
#include "codec-cfg.h"
  extern void	video_avcodec_register_all(int isvp);
}
#endif
#include <utils/threads.h>

#define NUM_NUMBERED_MPI 50
#define NUM_STATIC_MPI 2
#define NUM_TEMP_MPI 1
#define NUM_EXPORT_MPI 1

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

#define VDCTRL_QUERY_FORMAT 3 /* test for availabilty of a format */
#define VDCTRL_QUERY_MAX_PP_LEVEL 4 /* test for postprocessing support (max level) */
#define VDCTRL_SET_PP_LEVEL 5 /* set postprocessing level */
#define VDCTRL_SET_EQUALIZER 6 /* set color options (brightness,contrast etc) */
#define VDCTRL_GET_EQUALIZER 7 /* get color options (brightness,contrast etc) */
#define VDCTRL_RESYNC_STREAM 8 /* seeking */
#define VDCTRL_QUERY_UNSEEN_FRAMES 9 /* current decoder lag */

#define MAX_MEMHEAP_NUM 100

namespace android
{
  //namespace lumevideo{
	typedef struct mp_codec_info_s
	{
	    /* codec long name ("Autodesk FLI/FLC Animation decoder" */
	    const char *name;
	    /* short name (same as driver name in codecs.conf) ("dshow") */
	    const char *short_name;
	    /* interface author/maintainer */
	    const char *maintainer;
	    /* codec author ("Aaron Holtzman <aholtzma@ess.engr.uvic.ca>") */
	    const char *author;
	    /* any additional comments */
	    const char *comment;
	} mp_codec_info_t;

	typedef mp_codec_info_t vd_info_t;

	class mpDecorder{
	public:
	    mpDecorder(){}
	    virtual ~mpDecorder(){}
	    int * mVpuMem_ptr;
	    virtual int preinit(sh_video_t *sh){return 0;}
	    virtual int init(sh_video_t *sh){return 0;}
	    virtual void uninit(sh_video_t *sh){};
	    virtual int control(sh_video_t *sh,int cmd,void* arg, ...){return 0;}
	    virtual int decode_video(sh_video_t *sh_audio,unsigned char **inbuf,int *inlen,unsigned char* outbuf,int *outlen, int drop_frame){
		return 0;
	    }
	};

    class VpuMem{
    public:
      VpuMem();
      ~VpuMem();
      Vector<MemoryHeapBase * > mDevBuffers;
      sp<MemoryHeapBase> mHeapBases[MAX_MEMHEAP_NUM];
      int32_t mHeapBasesCount;

      void* vpu_mem_alloc(int size);
    };

    class LumeMemory
    {
    public:
      LumeMemory(int w,int h);
      ~LumeMemory();
      int muse_jz_buf;
      mp_image_t* get_image(int * VpuMem_ptr,unsigned int outfmt, int mp_imgtype, int mp_imgflag, int w, int h);
    private:
      mp_image_t* new_mp_image(int w,int h);
      void free_mp_image(mp_image_t* mpi);

      void mp_image_setfmt(mp_image_t* mpi,unsigned int out_fmt);
	
      typedef struct vf_image_context_s {
	mp_image_t* static_images[NUM_STATIC_MPI];
	mp_image_t* temp_images[NUM_TEMP_MPI];
	mp_image_t* export_images[1];
	mp_image_t* numbered_images[NUM_NUMBERED_MPI];
	int static_idx;
        int static_fix0, static_fix1;
      } vf_image_context_t;
      int iWidth,iHeight;
      vf_image_context_t imgctx;
      void alloc_planes(int * VpuMem_ptr,mp_image_t *mpi);
      void mpi_clear(mp_image_t* mpi,int x0,int y0,int w,int h);
    };


    class DecFactor
    {
    public:
      DecFactor()
	{
	  m_dec = NULL;
	}
      virtual ~DecFactor()
	{
	  if(m_dec)
	    free(m_dec);

	}
      mpDecorder * CreateVideoDecorder(char *drv);
    private:
      mpDecorder * m_dec;
    };


    class OpenmaxLumeAO;
    class VideoDecorder{
    public:
      VideoDecorder();
      virtual ~VideoDecorder();
      OMX_ERRORTYPE DecInit();
	
      OMX_BOOL DecodeVideo(OMX_U8* aOutBuffer, OMX_U32* aOutputLength,
			   OMX_U8** aInputBuf, OMX_U32* aInBufSize,
			   OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
			   OMX_S32* aFrameCount, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag);
	
      OMX_ERRORTYPE DecDeinit();
      OMX_BOOL VideoDecSetConext(sh_video_t *sh);
      sh_video_t *shContext;

    private:
      OMX_S32 iDisplay_Width, iDisplay_Height;
      //OMX_S32 avcodec_initialized;
      OMX_S32 dec_frame_state;
      mpDecorder *vd_dec;
      int startiframe;
      int dropped_frames;
    
      static int get_buffer(AVCodecContext *avctx, AVFrame *pic);
      static void release_buffer(struct AVCodecContext *avctx, AVFrame *pic);
      int init_vo(sh_video_t *sh, enum PixelFormat pix_fmt);
      OMX_S32 FindVideoCodec(sh_video_t *sh_video);
      DecFactor decFactor; 
      VpuMem mVpuMem;
      Mutex mLock;
    };
    //}
}
#endif ///#ifndef LUME_DEC_H_INCLUDED
