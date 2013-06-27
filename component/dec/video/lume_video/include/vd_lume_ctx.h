typedef struct {
  AVCodecContext *avctx;
  AVFrame *pic;
  enum PixelFormat pix_fmt;
  int do_slices;
  int do_dr1;
  int vo_initialized;
  int best_csp;
  int b_age;
  int ip_age[2];
  int qp_stat[32];
  double qp_sum;
  double inv_qp_sum;
  int ip_count;
  int b_count;
  AVRational last_sample_aspect_ratio;
  void *vdec;
} vd_lume_ctx;
