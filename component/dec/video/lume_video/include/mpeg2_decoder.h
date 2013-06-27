#include "lume_dec.h"
namespace android{

  //namespace lumevideo{

typedef struct {
    mpeg2dec_t *mpeg2dec;
    int quant_store_idx;
    char *quant_store[3];
    int imgfmt;
    int width;
    int height;
    double aspect;
    unsigned char *pending_buffer;
    int pending_length;
} vd_libmpeg2_ctx_t;

class mpeg2Decoder: public mpDecorder
{
public:
	
	mpeg2Decoder();
	virtual ~mpeg2Decoder();	
	virtual int preinit(sh_video_t *sh);
	virtual int init(sh_video_t *sh);
	virtual void uninit(sh_video_t *sh);
	virtual int control(sh_video_t *sh,int cmd,void* arg, ...);
	virtual int decode_video(sh_video_t *sh_audio,unsigned char **inbuf,int *inslen,unsigned char* outbuf,int *outlen, int drop_frame);
	static vd_info_t m_info;
private:
	int avcodec_initialized;
	friend class DecFactor;
	LumeMemory *mFrame_Mem;
    int dropped_frames;
};
//}
}
