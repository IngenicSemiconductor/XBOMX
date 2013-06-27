
#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mp_msg.h"
#include "stream.h"
#include "help_mp.h"
#include "m_option.h"
#include "m_struct.h"
#include "pvfile.h"
#include "impfile.h"

#include "mpfile_parser.h"

static struct stream_priv_s {
  char* filename;
  char *filename2;
} stream_priv_dflts = {
  NULL, NULL
};

#define ST_OFF(f) M_ST_OFF(struct stream_priv_s,f)
/// URL definition
static m_option_t stream_opts_fields[] = {
  {"string", ST_OFF(filename), CONF_TYPE_STRING, 0, 0 ,0, NULL},
  {"filename", ST_OFF(filename2), CONF_TYPE_STRING, 0, 0 ,0, NULL},
  { NULL, NULL, 0, 0, 0, 0,  NULL }
};
static struct m_struct_st stream_opts = {
  "file",
  sizeof(struct stream_priv_s),
  &stream_priv_dflts,
  stream_opts_fields
};  
#include "oscl_mutex.h"


#include "utils/Log.h"
static int fill_buffer(stream_t *s, char* buffer, int max_len){
  PVFile*    ipFilePtr = (PVFile*)(s->fd);    
  int size;
  MPFileParser *parser = (MPFileParser *)s->Context;
  AVSyncLock *pParserLock = parser->GetParserLock();
#if 1
  if(pParserLock)
  {
	  AVUNLOCK_REQ(pParserLock);
	  //ALOGE("pParserLock lock isLock = %d",pParserLock->isLock);
  }
#endif

  size = ipFilePtr->Read(buffer, 1, max_len);
#if 1
  if(pParserLock)
  {
	  // ALOGE("pParserLock unlock isLock = %d",pParserLock->isLock);
	  AVLOCK_REQ(pParserLock);
  }
#endif
  return size;

}

static int write_buffer(stream_t *s, char* buffer, int len) {
   /* unsurport tempply */

  return -1;
}

static int seek(stream_t *s,off_t newpos) {
  s->pos = newpos;
  PVFile*    ipFilePtr = (PVFile*)(s->fd);

  /* return 0 means success ,-1 means failed */
  if(ipFilePtr->Seek(newpos, Oscl_File::SEEKSET)){
    s->eof=1;
    return 0;
  }
  return 1;
}

static int seek_forward(stream_t *s,off_t newpos) {
  if(newpos<s->pos){
//    mp_msg(MSGT_STREAM,MSGL_INFO,"Cannot seek backward in linear streams!\n");
    return 0;
  }
  while(s->pos<newpos){
      int len=s->fill_buffer(s,(char*)(s->buffer),STREAM_BUFFER_SIZE);
    if(len<=0){ s->eof=1; s->buf_pos=s->buf_len=0; break; } // EOF
    s->buf_pos=0;
    s->buf_len=len;
    s->pos+=len;
  }
  return 1;
}

static int control(stream_t *s, int cmd, void *arg) {
  int res;
  off_t size;
  PVFile*    ipFilePtr = (PVFile*)(s->fd);    

  switch(cmd) {
    case STREAM_CTRL_GET_SIZE: {
      res=ipFilePtr->Seek(0, Oscl_File::SEEKEND);

      if(res == -1)
  	  return STREAM_UNSUPPORTED;
      else
	  size = ipFilePtr->Tell();
	
      ipFilePtr->Seek(s->pos, Oscl_File::SEEKSET);

      if(size != (off_t)-1) {
        *((off_t*)arg) = size;
        return 1;
      }
	  break;
    }
    case STREAM_CTRL_RESET:{
        //ipFilePtr->Seek(0, Oscl_File::SEEKSET);
    }
  case STREAM_CTRL_SET_AVSYNC:{
	  break;
  }
	  
		
  }
  return STREAM_UNSUPPORTED;
}

static int open_f(stream_t *stream,int mode, void* opts, int* file_format) {

    int res;
  off_t len;
  PVFile*    ipFilePtr = (PVFile*)(stream->fd);    
  res=ipFilePtr->Seek(0, Oscl_File::SEEKEND);
  if(res ==-1)
      return STREAM_ERROR;
  else
      len = ipFilePtr->Tell();
  
  ipFilePtr->Seek(0, Oscl_File::SEEKSET);

  if(len == -1) {
    if(mode == STREAM_READ) stream->seek = seek_forward;
    stream->type = STREAMTYPE_STREAM; // Must be move to STREAMTYPE_FILE
    stream->flags |= STREAM_SEEK_FW;
  } else if(len >= 0) {
    stream->seek = seek;
    stream->end_pos = len;
    stream->type = STREAMTYPE_FILE;
  }
  stream->fill_buffer = fill_buffer;
  stream->write_buffer = write_buffer;
  stream->control = control;

  m_struct_free(&stream_opts,opts);
  return STREAM_OK;
}

stream_info_t stream_info_opencore = {
  "Opencore",
  "opencore",
  "Albeu",
  "based on the code from ??? (probably Arpi)",
  open_f,
  { "file", "", NULL },
  &stream_opts,
  1 // Urls are an option string
};
