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

#include <utils/threads.h>
//#include "lume_phymem.h"
#include "avcodec.h"
#include "lume/libjzcommon/jzmedia.h"
#include "libjzcommon/com_config.h"
#include "libvc1/soc/t_vputlb.h"
#include "PlanarImage.h"

static int frmcnt = 0;
static int tst,ten;
static int readdata = 0;
static int ttime = 0;
#include <sys/time.h>

extern volatile int tlb_i;
#include <sched.h>

// Returns current time in microseconds
static unsigned int GetTimer(void){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

#include "lume_dec.h"
#include "lume_video_decoder.h"
#include "libmpeg2/mpeg2.h"
#include "mpeg2_decoder.h"
#include "media/stagefright/MediaSource.h"
#include <utils/threads.h>

#include <LUMEDefs.h>

#define LOG_TAG "lume_dec"
#include <utils/Log.h>
#define EL(x,y...) //{ALOGE("%s %d",__FILE__,__LINE__); LOGE(x,##y);}

int VAE_map();
void VAE_unmap();
void Lock_Vpu();
void UnLock_Vpu();
using namespace android;
extern "C" {
/*
 *exposed interface for libstagefright_soft_lume.so.
 */
VideoDecorder* CreateLUMESoftVideoDecoder(void){
  return new VideoDecorder();
}
  
OMX_ERRORTYPE DecInit(VideoDecorder*videoD){
  return videoD->DecInit();
}

OMX_ERRORTYPE DecDeinit(VideoDecorder*videoD){
  return videoD->DecDeinit();
}

OMX_BOOL VideoDecSetConext(VideoDecorder*videoD,sh_video_t *sh){
  return videoD->VideoDecSetConext(sh);
}

OMX_BOOL DecodeVideo(VideoDecorder*videoD,
		     OMX_U8* aOutBuffer, OMX_U32* aOutputLength,
		     OMX_U8** aInputBuf, OMX_U32* aInBufSize,
		     OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
		     OMX_S32* aFrameCount, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag){
  return videoD->DecodeVideo(aOutBuffer,aOutputLength,
			     aInputBuf,aInBufSize,
			     aPortParam,
			     aFrameCount,aMarkerFlag,aResizeFlag);
}

unsigned int get_phy_addr (unsigned int vaddr){
    ALOGE("FIXME : get_phy_addr");
    *(volatile int *)0x80000001 = 0;
    return 0;
}

void *jz4740_alloc_frame (int *VpuMem_ptr, int align, int size){
  VpuMem * vmem=(VpuMem*)VpuMem_ptr;
  int vaddr=(int)(vmem->vpu_mem_alloc(align+size));
  vaddr=(vaddr+align-1)&(~(align-1));
  return (void*)vaddr;
}

void *jz4740_alloc_frame2 (int *VpuMem_ptr, int align, int size, void** memHeapBase, int* offset){
  int tmpaddr = 0;

  VpuMem * vmem=(VpuMem*)VpuMem_ptr;
  int vaddr = tmpaddr =(int)(vmem->vpu_mem_alloc(align+size));
  vaddr=(vaddr+align-1)&(~(align-1));
  
  *memHeapBase = vmem->mDevBuffers[vmem->mDevBuffers.size() - 1];
  *offset = vaddr - tmpaddr;

  return (void*)vaddr;
}


void jz4740_free_frame(void *addr){
}

void jz4740_free_devmem (){}

unsigned int get_vaddr (unsigned int vaddr){
  return 0;
}

char *g_strdup(char *s){
    int len;
    char *rets = NULL;
    if(s){
	len = strlen(s);
	if(len){
	    rets = (char *)malloc(len + 1);
	    if(rets)
	      strcpy(rets,s);
	}
	
    }
    return rets;
}

}

#ifdef USE_IPU_THROUGH_MODE
unsigned int disp_buf0 = 0, disp_buf1 = 0, disp_buf2 = 0;

void clear_dispbuf(){
  disp_buf0 = 0;
  disp_buf1 = 0;
  disp_buf2 = 0;
}

unsigned int get_disp_buf0(){
  return disp_buf0;
}

unsigned int get_disp_buf1(){
  return disp_buf1;    
}

unsigned int get_disp_buf2(){
  return disp_buf2;
}
#endif

void swap_palette(void *pal)
{
    int i;
    uint32_t *p = (uint32_t *)pal;
    for (i = 0; i < AVPALETTE_COUNT; i++)
	p[i] = le2me_32(p[i]);
}

namespace android{

  //namespace lumevideo{
    
LumeMemory::LumeMemory(int w,int h){
    iWidth = w;
    iHeight = h;
    memset(&imgctx,0,sizeof(imgctx));
}

#define free_imgmems(x,y)                       \
    do{						\
	int i;					\
	for(i = 0;i < y;i++)			\
	{					\
	  if(x[i])				\
	  {					\
	      free_mp_image(x[i]);		\
	      x[i] = 0;				\
	  }					\
	}					\
    }while(0)

LumeMemory::~LumeMemory(){
    free_imgmems(imgctx.numbered_images,NUM_NUMBERED_MPI);
    free_imgmems(imgctx.static_images,NUM_STATIC_MPI);
    free_imgmems(imgctx.temp_images,NUM_TEMP_MPI);
    free_imgmems(imgctx.export_images,NUM_EXPORT_MPI);
}
    
#undef exit

mp_image_t* LumeMemory::new_mp_image(int w,int h){
    mp_image_t* mpi = (mp_image_t*)malloc(sizeof(mp_image_t));
    EL("mpi: %p",mpi);

    if(!mpi) return NULL; // error!
    memset(mpi,0,sizeof(mp_image_t));
    mpi->width=mpi->w=w;
    mpi->height=mpi->h=h;

    for(int i=0; i< 4; ++i){
      mpi->memheapbase[i] = NULL;
      mpi->memheapbase_offset[i] = -1;
    }

    return mpi;
}
	
void LumeMemory::free_mp_image(mp_image_t* mpi){
      if(!mpi) return;
      if(mpi->flags&MP_IMGFLAG_ALLOCATED){
	/* becouse we allocate the whole image in once */
	  // mpi alloate by MemHeapBase, no need free
	// if(mpi->planes[0]) {
	//   if (!gphy_free(mpi->planes[0]))
	//     g_free(mpi->planes[0]);
        // }
	// if (mpi->flags & MP_IMGFLAG_RGB_PALETTE){
	//   if (!gphy_free(mpi->planes[1]))
	//     g_free(mpi->planes[1]);
        // }
      }
      free(mpi);
    }

void LumeMemory::mp_image_setfmt(mp_image_t* mpi,unsigned int out_fmt){
    mpi->flags &= ~(MP_IMGFLAG_PLANAR|MP_IMGFLAG_YUV|MP_IMGFLAG_SWAPPED);
    mpi->imgfmt=out_fmt;
    // compressed formats
    if(out_fmt == IMGFMT_MPEGPES ||
       out_fmt == IMGFMT_ZRMJPEGNI || out_fmt == IMGFMT_ZRMJPEGIT || out_fmt == IMGFMT_ZRMJPEGIB ||
       IMGFMT_IS_VDPAU(out_fmt) || IMGFMT_IS_XVMC(out_fmt)){
	mpi->bpp=0;
	return;
    }

    mpi->num_planes=1;
    if (IMGFMT_IS_RGB(out_fmt)) {
	if (IMGFMT_RGB_DEPTH(out_fmt) < 8 && !(out_fmt&128))
	    mpi->bpp = IMGFMT_RGB_DEPTH(out_fmt);
	else
	    mpi->bpp=(IMGFMT_RGB_DEPTH(out_fmt)+7)&(~7);
	return;
    }

    if (IMGFMT_IS_BGR(out_fmt)) {
	if (IMGFMT_BGR_DEPTH(out_fmt) < 8 && !(out_fmt&128))
	  mpi->bpp = IMGFMT_BGR_DEPTH(out_fmt);
	else
	    mpi->bpp=(IMGFMT_BGR_DEPTH(out_fmt)+7)&(~7);
	mpi->flags|=MP_IMGFLAG_SWAPPED;
	return;
    }
    mpi->flags|=MP_IMGFLAG_YUV;
    mpi->num_planes=3;

    if (mp_get_chroma_shift(out_fmt, NULL, NULL)) {
	mpi->flags|=MP_IMGFLAG_PLANAR;
	mpi->bpp = mp_get_chroma_shift(out_fmt, &mpi->chroma_x_shift, &mpi->chroma_y_shift);
	mpi->chroma_width  = mpi->width  >> mpi->chroma_x_shift;
	mpi->chroma_height = mpi->height >> mpi->chroma_y_shift;
    }

    switch(out_fmt){
    case IMGFMT_I420:
    case IMGFMT_IYUV:
	mpi->flags|=MP_IMGFLAG_SWAPPED;
    case IMGFMT_YV12:
	return;
    case IMGFMT_420A:
    case IMGFMT_IF09:
	mpi->num_planes=4;
    case IMGFMT_YVU9:
    case IMGFMT_444P:
    case IMGFMT_422P:
    case IMGFMT_411P:
    case IMGFMT_440P:
    case IMGFMT_444P16_LE:
    case IMGFMT_444P16_BE:
    case IMGFMT_422P16_LE:
    case IMGFMT_422P16_BE:
    case IMGFMT_420P16_LE:
    case IMGFMT_420P16_BE:
	return;
    case IMGFMT_Y800:
    case IMGFMT_Y8:
	/* they're planar ones, but for easier handling use them as packed */
	//	mpi->flags|=MP_IMGFLAG_PLANAR;
	mpi->bpp=8;
	mpi->num_planes=1;
	return;
    case IMGFMT_UYVY:
	mpi->flags|=MP_IMGFLAG_SWAPPED;
    case IMGFMT_YUY2:
	mpi->bpp=16;
	mpi->num_planes=1;
	return;
    case IMGFMT_NV12:
	mpi->flags|=MP_IMGFLAG_SWAPPED;
    case IMGFMT_NV21:
	mpi->flags|=MP_IMGFLAG_PLANAR;
	mpi->bpp=12;
	mpi->num_planes=2;
	mpi->chroma_width=(mpi->width>>0);
	mpi->chroma_height=(mpi->height>>1);
	mpi->chroma_x_shift=0;
	mpi->chroma_y_shift=1;
	return;
    }
    mp_msg(MSGT_DECVIDEO,MSGL_WARN,"mp_image: unknown out_fmt: 0x%X\n",out_fmt);
    mpi->bpp=0;
}

mp_image_t* LumeMemory::get_image(int * VpuMem_ptr,unsigned int outfmt, int mp_imgtype, int mp_imgflag, int w, int h){
    mp_image_t* mpi=NULL;
    int w2;
    int number = mp_imgtype >> 16;
    w2 =  (mp_imgflag & MP_IMGFLAG_ACCEPT_ALIGNED_STRIDE) ? ((w + 15) & ( ~15 )) : w;
    // Note: we should call libvo first to check if it supports direct rendering
    // and if not, then fallback to software buffers:
    switch(mp_imgtype & 0xff){
    case MP_IMGTYPE_EXPORT:
	if(!imgctx.export_images[0]){
	    imgctx.export_images[0] = new_mp_image(w2,h);
	}				
	mpi = imgctx.export_images[0];
	break;
    case MP_IMGTYPE_STATIC:
	if(!imgctx.static_images[0]) imgctx.static_images[0] = new_mp_image(w2,h);
	mpi = imgctx.static_images[0];
	break;
    case MP_IMGTYPE_TEMP:
	if(!imgctx.temp_images[0]) imgctx.temp_images[0] = new_mp_image(w2,h);
	mpi = imgctx.temp_images[0];
	break;
#ifdef USE_IPU_THROUGH_MODE
    case MP_IMGTYPE_IPB:
    case MP_IMGTYPE_IP:
	  /* skip (I, P) or (P, P) frame since B Frame referenced */
	  //while (imgctx.static_idx == imgctx.static_fix0 || imgctx.static_idx == imgctx.static_fix1) {
	  if(imgctx.static_images[imgctx.static_idx])
	  {
	      while ((get_disp_buf0() && (uint32_t)imgctx.static_images[imgctx.static_idx]->planes[0] == get_disp_buf0())
		     || (get_disp_buf1() && (uint32_t)imgctx.static_images[imgctx.static_idx]->planes[0] == get_disp_buf1())
		     || (get_disp_buf2() && (uint32_t)imgctx.static_images[imgctx.static_idx]->planes[0] == get_disp_buf2())
		     || (imgctx.static_idx == imgctx.static_fix0 || imgctx.static_idx == imgctx.static_fix1)) {
              imgctx.static_idx++;
              if (imgctx.static_idx >= USE_FBUF_NUM)
		  imgctx.static_idx = 0;
	      }
	  }
	  
	  if(!imgctx.static_images[imgctx.static_idx]) imgctx.static_images[imgctx.static_idx]=new_mp_image(w2,h);
	  mpi=imgctx.static_images[imgctx.static_idx];
	  
	  /* roll fix buf (I,P or P,P) */
	  
	  if (mp_imgtype == MP_IMGTYPE_IP || (mp_imgflag&MP_IMGFLAG_READABLE))
	  {
	      imgctx.static_fix0 = imgctx.static_fix1;
	      imgctx.static_fix1 = imgctx.static_idx;
	  }
	  
	  /* Buf idx + 1  */
	  imgctx.static_idx++;
	  if (imgctx.static_idx >= USE_FBUF_NUM)
	      imgctx.static_idx = 0;
	  break;
#else
    case MP_IMGTYPE_IPB:
	if(!(mp_imgflag&MP_IMGFLAG_READABLE)){ // B frame:
	    if(!imgctx.temp_images[0]) imgctx.temp_images[0] = new_mp_image(w2,h);
	    mpi = imgctx.temp_images[0];
	    break;
	}
    case MP_IMGTYPE_IP:
        EL("w2: %d, h: %d",w2,h);
	if(!imgctx.static_images[imgctx.static_idx]) imgctx.static_images[imgctx.static_idx]=new_mp_image(w2,h);
	mpi=imgctx.static_images[imgctx.static_idx];
	imgctx.static_idx ^= 1;
	break;
#endif
    case MP_IMGTYPE_NUMBERED:
	if (number == -1) {
	    int i;
	    for (i = 0; i < NUM_NUMBERED_MPI; i++)
		if (!imgctx.numbered_images[i] || !imgctx.numbered_images[i]->usage_count)
		    break;
	    number = i;
	}

	if (number < 0 || number >= NUM_NUMBERED_MPI) return NULL;
	if (!imgctx.numbered_images[number]) imgctx.numbered_images[number] = new_mp_image(w2,h);
	mpi = imgctx.numbered_images[number];
	mpi->number = number;
	break;
        
    }
    
    if(mpi){
	mpi->type = mp_imgtype;
	mpi->w = iWidth; 
	mpi->h = iHeight;
	// keep buffer allocation status & color flags only:
	//    mpi->flags&=~(MP_IMGFLAG_PRESERVE|MP_IMGFLAG_READABLE|MP_IMGFLAG_DIRECT);
	mpi->flags &= MP_IMGFLAG_ALLOCATED|MP_IMGFLAG_TYPE_DISPLAYED|MP_IMGFLAGMASK_COLORS;
	// accept restrictions, draw_slice and palette flags only:
	mpi->flags |= mp_imgflag&(MP_IMGFLAGMASK_RESTRICTIONS|MP_IMGFLAG_DRAW_CALLBACK|MP_IMGFLAG_RGB_PALETTE);
	mpi->flags &= ~MP_IMGFLAG_DRAW_CALLBACK;

	if(mpi->width != w2 || mpi->height != h){
	    //	printf("vf.c: MPI parameters changed!  %dx%d -> %dx%d   \n", mpi->width,mpi->height,w2,h);
	    if(mpi->flags & MP_IMGFLAG_ALLOCATED){
		if(mpi->width < w2 || mpi->height < h){
		    // need to re-allocate buffer memory:
                    if (!muse_jz_buf)
		       free(mpi->planes[0]);
		    mpi->flags &= ~MP_IMGFLAG_ALLOCATED;
		    mp_msg(MSGT_VFILTER,MSGL_V,"vf.c: have to REALLOCATE buffer memory :(\n");
		} else { // need to change some vairiable value
		    if(muse_jz_buf){
			int width_ext=(w2+15)&(~15);
			int height_ext=(h+15)&(~15);
			int y_strd=(width_ext*16 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
			int c_strd=(width_ext*8 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));

			mpi->stride[0] = y_strd;
			mpi->stride[1] = c_strd;
		    }
		}
		//	} else {
	    } {
		mpi->width = w2; mpi->chroma_width=(w2 + (1<<mpi->chroma_x_shift) - 1)>>mpi->chroma_x_shift;
		mpi->height = h; mpi->chroma_height=(h + (1<<mpi->chroma_y_shift) - 1)>>mpi->chroma_y_shift;
	    }
	}
	//	  printf("clearing img!\n");
	if(!mpi->bpp) mp_image_setfmt(mpi,outfmt);
	
	if(!(mpi->flags&MP_IMGFLAG_ALLOCATED) && mpi->type > MP_IMGTYPE_EXPORT)
	{	
	  alloc_planes(VpuMem_ptr,mpi);
            mpi_clear(mpi,0,0,mpi->width,mpi->height);
            
	}
	mpi->qscale = NULL;
    }
    
    mpi->usage_count++;
    
    return mpi;
}
    
void LumeMemory::alloc_planes(int * VpuMem_ptr,mp_image_t *mpi) {

    unsigned char* data;
    int w,h,ch,w_aln,size;
    w = mpi->width;
    h = mpi->height;
    ch = mpi->chroma_height;
    w_aln = w*16;

    if(muse_jz_buf){
      int width_ext=(mpi->width+15)&(~15);
      int height_ext=(mpi->height+15)&(~15);
      int y_strd=(width_ext*16 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));
      int c_strd=(width_ext*8 + (IPU_4780BUG_ALIGN-1))&(~(IPU_4780BUG_ALIGN-1));

      void* memheapbase = NULL;
      int offset = 0;
      mpi->planes[0] = (unsigned char *)jz4740_alloc_frame2 (VpuMem_ptr,IPU_4780BUG_ALIGN>256?IPU_4780BUG_ALIGN:256,
					   y_strd*height_ext/16, &memheapbase, &offset);
      mpi->memheapbase[0] = memheapbase;
      mpi->memheapbase_offset[0] = offset;

      mpi->planes[1] = (unsigned char *)jz4740_alloc_frame2 (VpuMem_ptr,IPU_4780BUG_ALIGN>256?IPU_4780BUG_ALIGN:256,
					   c_strd*height_ext/16, &memheapbase, &offset);
      mpi->memheapbase[1] = memheapbase;
      mpi->memheapbase_offset[1] = offset;

      EL("!!!!!!alloc_planes base[0]:%p, base[1]:%p,  memheapbase[0]:%p, offset[0]:%d;memheapbase[1]:%p, offset[1]:%d;", mpi->planes[0], mpi->planes[1], mpi->memheapbase[0], mpi->memheapbase_offset[0], mpi->memheapbase[1], mpi->memheapbase_offset[1]);

      mpi->stride[0] = y_strd;
      mpi->stride[1] = c_strd;
    }else{
      void* memheapbase = NULL;
      int offset = 0;

      if (mpi->imgfmt == IMGFMT_IF09) {
	mpi->planes[0]=(unsigned char *)jz4740_alloc_frame2(VpuMem_ptr,256,mpi->bpp*mpi->width*(mpi->height+2)/8+
							   mpi->chroma_width*mpi->chroma_height, &memheapbase, &offset);
	mpi->memheapbase[0] = memheapbase;
	mpi->memheapbase_offset[0] = offset;
      }else{
	data = (unsigned char *)jz4740_alloc_frame2(VpuMem_ptr,256,mpi->bpp*w*(h+2)/8, &memheapbase, &offset);
	mpi->planes[0]=data; 

	mpi->memheapbase[0] = memheapbase;
	mpi->memheapbase_offset[0] = offset;
      }
    }

    if (mpi->flags&MP_IMGFLAG_PLANAR) {
      if(!muse_jz_buf){
	int bpp = IMGFMT_IS_YUVP16(mpi->imgfmt)? 2 : 1;
	// YV12/I420/YVU9/IF09. feel free to add other planar formats here...
	mpi->stride[0]=mpi->stride[3]=bpp*mpi->width;
	if(mpi->num_planes > 2){
	  mpi->stride[1]=mpi->stride[2]=bpp*mpi->chroma_width;
	  if(mpi->flags&MP_IMGFLAG_SWAPPED){
	    // I420/IYUV  (Y,U,V)
	    mpi->planes[1]=mpi->planes[0]+mpi->stride[0]*mpi->height;
	    mpi->memheapbase_offset[1] = mpi->memheapbase_offset[0] + mpi->stride[0]*mpi->height;

	    mpi->planes[2]=mpi->planes[1]+mpi->stride[1]*mpi->chroma_height;
	    mpi->memheapbase_offset[2] = mpi->memheapbase_offset[1] + mpi->stride[1]*mpi->chroma_height;

	    if (mpi->num_planes > 3){
	      mpi->planes[3]=mpi->planes[2]+mpi->stride[2]*mpi->chroma_height;
	      mpi->memheapbase_offset[3] = mpi->memheapbase_offset[2] + mpi->stride[2]*mpi->chroma_height;
	    }
	  } else {
	    // YV12,YVU9,IF09  (Y,V,U)
	    mpi->planes[2]=mpi->planes[0]+mpi->stride[0]*mpi->height;
	    mpi->memheapbase_offset[2] = mpi->memheapbase_offset[0] + mpi->stride[0]*mpi->height;

	    mpi->planes[1]=mpi->planes[2]+mpi->stride[1]*mpi->chroma_height;
	    mpi->memheapbase_offset[1] = mpi->memheapbase_offset[2] + mpi->stride[1]*mpi->chroma_height;

	    if (mpi->num_planes > 3){
	      mpi->planes[3]=mpi->planes[1]+mpi->stride[1]*mpi->chroma_height;
	      mpi->memheapbase_offset[3] = mpi->memheapbase_offset[1] + mpi->stride[1]*mpi->chroma_height;
	    }
	  }
	} else {
	  // NV12/NV21
	  mpi->stride[1]=mpi->chroma_width;
	  mpi->planes[1]=mpi->planes[0]+mpi->stride[0]*mpi->height;
	}
      }
    } else {
      mpi->stride[0]=mpi->width*mpi->bpp/8;
      if (mpi->flags & MP_IMGFLAG_RGB_PALETTE)
	{
	  mpi->planes[1] = (unsigned char *)jz4740_alloc_frame(VpuMem_ptr,256,1024);
	}
    }
    mpi->flags|=MP_IMGFLAG_ALLOCATED;
}
        
void LumeMemory::mpi_clear(mp_image_t* mpi,int x0,int y0,int w,int h){
    int y;
    if(mpi->flags&MP_IMGFLAG_PLANAR){
	y0&=~1;h+=h&1;
        if(x0==0 && w==mpi->width){
#if 0
	    // full width clear:
	    memset(mpi->planes[0]+mpi->stride[0]*y0,0,mpi->stride[0]*h);
	    memset(mpi->planes[1]+mpi->stride[1]*(y0>>mpi->chroma_y_shift),128,mpi->stride[1]*(h>>mpi->chroma_y_shift));
	    memset(mpi->planes[2]+mpi->stride[2]*(y0>>mpi->chroma_y_shift),128,mpi->stride[2]*(h>>mpi->chroma_y_shift));
#endif
        } else
	    for(y=y0;y<y0+h;y+=2){
		memset(mpi->planes[0]+x0+mpi->stride[0]*y,0,w);
		memset(mpi->planes[0]+x0+mpi->stride[0]*(y+1),0,w);
		memset(mpi->planes[1]+(x0>>mpi->chroma_x_shift)+mpi->stride[1]*(y>>mpi->chroma_y_shift),128,(w>>mpi->chroma_x_shift));
		memset(mpi->planes[2]+(x0>>mpi->chroma_x_shift)+mpi->stride[2]*(y>>mpi->chroma_y_shift),128,(w>>mpi->chroma_x_shift));
	    }
        return;
    }
    // packed:
    for(y=y0;y<y0+h;y++){
	unsigned char* dst=mpi->planes[0]+mpi->stride[0]*y+(mpi->bpp>>3)*x0;
	if(mpi->flags&MP_IMGFLAG_YUV){
	    unsigned int* p=(unsigned int*) dst;
	    int size=(mpi->bpp>>3)*w/4;
	    int i;
#if HAVE_BIGENDIAN
#define CLEAR_PACKEDYUV_PATTERN 0x00800080
#define CLEAR_PACKEDYUV_PATTERN_SWAPPED 0x80008000
#else
#define CLEAR_PACKEDYUV_PATTERN 0x80008000
#define CLEAR_PACKEDYUV_PATTERN_SWAPPED 0x00800080
#endif
	    if(mpi->flags&MP_IMGFLAG_SWAPPED){
		for(i=0;i<size-3;i+=4) p[i]=p[i+1]=p[i+2]=p[i+3]=CLEAR_PACKEDYUV_PATTERN_SWAPPED;
		for(;i<size;i++) p[i]=CLEAR_PACKEDYUV_PATTERN_SWAPPED;
	    } else {
		for(i=0;i<size-3;i+=4) p[i]=p[i+1]=p[i+2]=p[i+3]=CLEAR_PACKEDYUV_PATTERN;
		for(;i<size;i++) p[i]=CLEAR_PACKEDYUV_PATTERN;
	    }
	} else
	    memset(dst,0,(mpi->bpp>>3)*w);
    }
}
    
mpDecorder* DecFactor::CreateVideoDecorder(char *drv){
    mpDecorder *newcodec;
    
    if(strcmp(lumeDecoder::m_info.short_name,drv) == 0)
    {
	m_dec = new lumeDecoder();
    }
    else if(strcmp(mpeg2Decoder::m_info.short_name,drv) == 0)
    {
	m_dec = new mpeg2Decoder();
    }

    return m_dec;
}

VideoDecorder::VideoDecorder(){
    iDisplay_Width = 0;
    iDisplay_Height = 0;
    shContext = 0;
    dec_frame_state = -1;
    startiframe = 1;
    vd_dec = NULL;
}
    
VideoDecorder::~VideoDecorder(){
      if(vd_dec){
	     vd_dec->uninit(shContext);
      }

      if(dec_frame_state != -1)
	  VAE_unmap();
      if(shContext) {
	  free(shContext);
	  shContext = NULL;
      }
}
    
OMX_ERRORTYPE VideoDecorder::DecInit(){
    Mutex::Autolock autoLock(mLock);
    if((dec_frame_state == -1) && VAE_map()){
	ALOGE("VideoDecorder Reload");
	*(volatile int *)0x80000001 = 0;
	return OMX_ErrorUndefined;
    }
    
    dec_frame_state = 0;
    return OMX_ErrorNone;
}
    
OMX_ERRORTYPE VideoDecorder::DecDeinit(){
    Mutex::Autolock autoLock(mLock);
    if(vd_dec)
	vd_dec->uninit(shContext);
    if(dec_frame_state != -1)
	VAE_unmap();
    dec_frame_state = -1;
    return OMX_ErrorNone;
}

int extra_time;    
OMX_BOOL VideoDecorder::DecodeVideo(OMX_U8* aOutBuffer, OMX_U32* aOutputLength,
				    OMX_U8** aInputBuf, OMX_U32* aInBufSize,
				    OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
				    OMX_S32* aFrameCount, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag){

    OMX_BOOL Status = OMX_TRUE;
    OMX_S32 OldWidth, OldHeight, OldFrameSize;
    vd_lume_ctx *ctx = (vd_lume_ctx *)shContext->context;
    AVCodecContext *avctx = ctx->avctx;
    mp_image_t *mpi = NULL;

    vd_libmpeg2_ctx_t *libmpeg2_ctx = (vd_libmpeg2_ctx_t *)shContext->context;
    mpeg2dec_t *libmpeg2dec = (mpeg2dec_t *)libmpeg2_ctx->mpeg2dec;

    OldWidth = aPortParam->format.video.nFrameWidth;
    OldHeight = aPortParam->format.video.nFrameHeight;
    *aOutputLength = 0;
    EL("OldWidth = %d OldHeight = %d *aInBufSize = %d,avctx->width=%d, avctx->height=%d ",OldWidth,OldHeight,*aInBufSize,avctx->width,avctx->height);
    
    int drop_frame = *aResizeFlag;

    *aResizeFlag = OMX_FALSE;
    
#ifdef DEBUG_VIDEODEC_COUNTED_BUFVALUE
    char in_str[128] = "videodec inbuf:";
    for(int i = 0, j = 15; i < DEBUG_VIDEODEC_COUNTED_BUFVALUE; ++i){
	sprintf(&in_str[j], "%u ", (unsigned int)(*aInputBuf)[i]);
	j += strlen(&in_str[j]);
    }
    ALOGE("%s", in_str);
#endif

#ifdef DEBUG_VIDEODEC_COST_TIME
    unsigned int pre_time = GetTimer();
#endif

    if(vd_dec){
        Lock_Vpu();
	mpi = (mp_image_t *)vd_dec->decode_video(shContext,aInputBuf,(int *)aInBufSize,aOutBuffer,(int *)aOutputLength, drop_frame);
	UnLock_Vpu();
    }
    else
	ALOGE("no video decoder!!!!");

    int tFrameWidth,tFrameHeight;
    int t_use_jz_buf, t_is_dechw;
    if (shContext->drvmpeg2){
      tFrameWidth = libmpeg2_ctx->width;
      tFrameHeight = libmpeg2_ctx->height;
      t_use_jz_buf = 1;
      t_is_dechw = 1;
    }else{
      tFrameWidth = avctx->width;
      tFrameHeight = avctx->height;
      t_use_jz_buf = avctx->use_jz_buf;
      t_is_dechw = avctx->is_dechw;
    }

#ifdef DEBUG_VIDEODEC_COST_TIME
    ALOGE("videodec costTime = %u us", GetTimer() - pre_time);
#endif

#ifdef DEBUG_VIDEODEC_COUNTED_BUFVALUE
    char out_str[128] = "videodec outbuf:";
    for(int i = 0, j = 16; i < DEBUG_VIDEODEC_COUNTED_BUFVALUE; ++i){
	sprintf(&out_str[j], "%u ", (unsigned int)aOutBuffer[i]);
	j += strlen(&out_str[j]);
    }
    ALOGE("%s", out_str);
#endif
    
#ifdef USE_IPU_THROUGH_MODE
    if(mpi){				
	disp_buf0 = disp_buf1;

	if(tFrameWidth < 1280){
	    disp_buf1 = disp_buf2;
	    disp_buf2 = (unsigned int)((mp_image_t *)mpi)->planes[0];
        }else{//phy mem not enough
	    disp_buf1 = (unsigned int)((mp_image_t *)mpi)->planes[0];
	}
    }
#endif 
    if(mpi)
    {	
        if(((startiframe && startiframe < 10) || shContext->mSeek)&& mpi->pict_type != 1)
	  {
	    PlanarImage *p = (PlanarImage *)aOutBuffer;
	    p->isvalid = 0;
	    p->is_dechw = t_is_dechw;
	    
	    *aOutputLength = 0;
	    *aInBufSize = 0;
	    startiframe++;
	    return OMX_TRUE;
	  }      
	startiframe = 0;
        shContext->mSeek = 0;

	if((OldWidth != tFrameWidth) || (OldHeight != tFrameHeight))
	  {
	    *aResizeFlag = OMX_TRUE;
	    aPortParam->format.video.nFrameWidth = tFrameWidth;
	    aPortParam->format.video.nFrameHeight = tFrameHeight;
	    aPortParam->nBufferSize = sizeof(PlanarImage);
	    OMX_U32 min_stride = aPortParam->format.video.nFrameWidth;
	    //((aPortParam->format.video.nFrameWidth + 15) & (~15));
	    OMX_U32 min_sliceheight = aPortParam->format.video.nFrameHeight;
	    //((aPortParam->format.video.nFrameHeight + 15) & (~15));
	    aPortParam->format.video.nStride = min_stride;
	    aPortParam->format.video.nSliceHeight = min_sliceheight;
	    ALOGE("resize!!avctx->width=%d,avctx->height=%d\n",tFrameWidth,tFrameHeight);
	  }
	
	PlanarImage *p = (PlanarImage *)aOutBuffer;

	p->phy_planar[0] = (uint32_t)((unsigned int)mpi->planes[0]);
	p->phy_planar[1] = (uint32_t)((unsigned int)mpi->planes[1]);
	p->phy_planar[2] = (uint32_t)((unsigned int)mpi->planes[2]);
	p->phy_planar[3] = (uint32_t)((unsigned int)mpi->planes[3]);
	p->pts=mpi->pts;
	p->planar[0] = (uint32_t)mpi->planes[0];
	p->planar[1] = (uint32_t)mpi->planes[1];
	p->planar[2] = (uint32_t)mpi->planes[2];
	p->planar[3] = (uint32_t)mpi->planes[3];
	p->stride[0] = mpi->stride[0];
	//p->is_dechw = avctx->is_dechw; 
	p->is_dechw = t_is_dechw;
	//ALOGE("planar->pts=%lld",p->pts);	

	if(mpi->memheapbase[0] != NULL){
	  for(int i=0; i<4; ++i){
	    p->memheapbase[i] = mpi->memheapbase[i];
	    p->memheapbase_offset[i] = mpi->memheapbase_offset[i];
	  }
	}

	//ALOGE("#######decoded base[0]:%p, base[1]:%p,  memheapbase[0]:%p, offset[0]:%d;memheapbase[1]:%p, offset[1]:%d;", mpi->planes[0], mpi->planes[1], mpi->memheapbase[0], mpi->memheapbase_offset[0], mpi->memheapbase[1], mpi->memheapbase_offset[1]);

	//ALOGE("===========final base[0]:%p, base[1]:%p,  memheapbase[0]:%p, offset[0]:%d;memheapbase[1]:%p, offset[1]:%d;", p->planar[0], p->planar[1], p->memheapbase[0], p->memheapbase_offset[0], p->memheapbase[1], p->memheapbase_offset[1]);

	if(mpi->imgfmt == IMGFMT_422P){
	    p->stride[1] = mpi->stride[1]*2;
	    p->stride[2] = mpi->stride[2]*2;
	}else{
	    p->stride[1] = mpi->stride[1];
	    p->stride[2] = (mpi->height+15)/16*256;
	}
	
	p->stride[3] = (mpi->height+15)/16*128;      
	p->isvalid = 1;
	
	*aOutputLength = sizeof(PlanarImage);
	//memcpy(aOutBuffer,mpi->planes[0],shContext->disp_w*shContext->disp_h*3/2);
	//*aOutputLength = shContext->disp_w*shContext->disp_h*3/2;
	/*add for gysun*/
	if(*aResizeFlag == OMX_TRUE) *aOutputLength = 0;
	EL("11decoder ok!");
	return OMX_TRUE;
	
    }else
    {
	PlanarImage *p = (PlanarImage *)aOutBuffer;
	p->isvalid = 0;
	
	*aOutputLength = 0;//sizeof(PlanarImage);	
	*aInBufSize = 0;
	//p->is_dechw = avctx->is_dechw; 
	p->is_dechw = t_is_dechw; 
        if (shContext->is_2648x8 == 1){
          shContext->is_2648x8 == 0;
          return OMX_FALSE;
        }
    }
    ALOGE("decoder fail!");
    return OMX_TRUE;
}
    
OMX_S32 VideoDecorder::FindVideoCodec(sh_video_t *sh_video){
    S32I2M(xr16,7);
    int force = 0;
    unsigned int orig_fourcc = sh_video->bih?sh_video->bih->biCompression:0;
    sh_video->codec = NULL;

    while(1)
    {
	sh_video->codec = find_video_codec(sh_video->format,
					   sh_video->bih?((unsigned int*) &sh_video->bih->biCompression):NULL,
					   sh_video->codec,0);
	if(!sh_video->codec ){
	    vd_dec = 0;
	    ALOGE("Can't find video codec!");	    
	    return 0;
	}
	
	vd_dec = decFactor.CreateVideoDecorder(sh_video->codec->drv);
	if(vd_dec){
	    vd_dec->mVpuMem_ptr=(int*)(&mVpuMem);
	    vd_dec->init(sh_video);
	    break;
	}
	ALOGE("video codec init failed");
      
	if(vd_dec)
	    vd_dec->uninit(shContext);	
    }
    ALOGD("vd_dec = %p",vd_dec);
    return 1;
}
    
OMX_BOOL VideoDecorder::VideoDecSetConext(sh_video_t *sh){
    EL("VideoDecorder::VideoDecSetConext in");
    if(DecInit() != OMX_ErrorNone)
    {
	ALOGE("tcsm open fail!\n");
	return OMX_FALSE;
    }

    if(shContext ==  NULL)
	shContext = (sh_video_t *) malloc(sizeof(sh_video_t));

    if(shContext)
    {
	memcpy(shContext,sh,sizeof(sh_video_t));
	if((shContext->format == 0) && shContext->bih)
	    shContext->format = shContext->bih->biCompression;
	
	if(shContext->format != 0)
	{
	    parse_codec_cfg(NULL);
	      if(FindVideoCodec(shContext))
	      {
                  if(strcmp(lumeDecoder::m_info.short_name, shContext->codec->drv) == 0){
		    AVCodecContext *lumectx= ((vd_lume_ctx *)shContext->context)->avctx;
		    sh->context=shContext->context;//just for pass use_jz_buf and use_jz_buf_change
		    if(strcmp(shContext->codec->dll,"h264") == 0){
		      sh->need_depack_nal=1;
		      sh->nal_length_size=lumectx->nal_length_size;
		      sh->is_avc=lumectx->is_avc;
		    }
		    else{
		      sh->need_depack_nal=0;
		    }
		  }else{
		    sh->need_depack_nal=0;
		  }
		  sh->drvmpeg2 = shContext->drvmpeg2; //pass dec_drv
		  EL("find decoder ok!");
		  return OMX_TRUE;
	      }
	      else
		  ALOGE("FindVideoCodec failed");
	}		
    }
    EL("VideoDecorder::VideoDecSetConext out false");    
    return OMX_FALSE;
}


VpuMem::VpuMem()
  :mHeapBasesCount(0){
  for(int i=0; i<MAX_MEMHEAP_NUM; ++i){
    mHeapBases[i] = NULL;
  }
}

void* VpuMem::vpu_mem_alloc(int size){
  mDevBuffers.push();
  MemoryHeapBase** devbuf = &mDevBuffers.editItemAt(mDevBuffers.size() - 1);
  EL("devbuf=0x%x,*devbuf=0x%x, mHeapBasesCount:%d",devbuf,*devbuf, mHeapBasesCount);
  int32_t curIndex = mHeapBasesCount;
  mHeapBases[curIndex] = new MemoryHeapBase(size);
  ++mHeapBasesCount;
  *devbuf = mHeapBases[curIndex].get();
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
  ALOGE("VpuMem::~VpuMem mDevBuffers.size()=%d",mDevBuffers.size());
  for (int i = 0; i < mDevBuffers.size(); i++) {
    MemoryHeapBase* mptr = mDevBuffers.editItemAt(i);
    meminfo.size=mptr->getSize();
    meminfo.vaddr=mptr->getBase();
    meminfo.pages_phys_addr_table=NULL;
    dmmu_unmap_user_memory(&meminfo);
    //delete mptr;
  }
}

  //}
}
