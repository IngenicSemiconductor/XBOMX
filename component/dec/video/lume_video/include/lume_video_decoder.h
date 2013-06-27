
#include "vd_lume_ctx.h"

namespace android{

  //namespace lumevideo{
        
class lumeDecoder: public mpDecorder
{
public:
	
	lumeDecoder();
	virtual ~lumeDecoder();	
	virtual int preinit(sh_video_t *sh);
	virtual int init(sh_video_t *sh);
	virtual void uninit(sh_video_t *sh);
	virtual int control(sh_video_t *sh,int cmd,void* arg, ...);
	virtual int decode_video(sh_video_t *sh_audio,unsigned char **inbuf,int *inslen,unsigned char* outbuf,int *outlen, int drop_frame);
	static vd_info_t m_info;
private:
	int avcodec_initialized;
	friend class DecFactor;
	static int get_buffer(AVCodecContext *avctx, AVFrame *pic);
	static void release_buffer(struct AVCodecContext *avctx, AVFrame *pic);
	//void set_format_params(struct AVCodecContext *avctx, enum PixelFormat fmt);
	int init_vo(sh_video_t *sh, enum PixelFormat pix_fmt);
	void init_avcodec(int isvp);
	LumeMemory *mFrame_Mem;
	int dropped_frames;
	char * copy_bs;
};
  //}
}
