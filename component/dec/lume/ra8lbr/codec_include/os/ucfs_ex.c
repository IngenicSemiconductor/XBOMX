#include "uclib.h"
#include <ucfsdef.h>

char *fgets(char *line,int size,FILE *fp)
{
	unsigned char s[512];
	unsigned int i;
	for(i = 0;i<size;i++)
	{
		if(fread(&line[i],1,1,fp) == 1)
		{
			if(line[i] == '\r')
				line[i] = 0;
		}else
		{
			line[i] = 0;
		}
	}
	if(i == size)
		line[i - 1] = 0;
	return line;
}
#if 0
int fprintf(FILE * stream, const char * format,...)
{
	va_list args;
	printf("__don't suport__%s:%d: fprintf\n",__FILE__,__LINE__);
	va_start(args,format);
	printf(format,args);
	va_end(args);
}
#endif
/*
// open
extern FS_FILE *  BUFF_Open ( const char *pFileName, const char *pMode );
// close
extern void   BUFF_Close( FS_FILE * PORT  );
// read
extern FS_size_t  BUFF_Read ( void *pData, FS_size_t Size, FS_size_t N, FS_FILE *MY_FS  );
// seek
extern int   BUFF_Seek ( FS_FILE *MY_FS, FS_i32 Offset, int Whence );


int uc_fopen( const char * pathname, int flags,mode_t mode)
{
	//printf("uc_fopen\n");
	return   (int)BUFF_Open( pathname, "rb" );
	//return (int)FS_FOpen(pathname,"rb");
}
ssize_t uc_fread(int fd,void * buf ,size_t count)
{
	return (ssize_t)BUFF_Read( buf, 1,count,(FS_FILE *)fd );
	// return (ssize_t)FS_FRead(buf,1,count,(FS_FILE *)fd);
} 

int uc_fsync(void)
{
	return 1;
} 

ssize_t uc_fwrite (int fd,const void * buf,size_t count)
{
} 
loff_t uc_fseek (int fd, loff_t offset, int whence)
{
	return BUFF_Seek((FS_FILE *)fd,offset, whence  );
}
int uc_fclose (int fd)
{
  BUFF_Close( (FS_FILE *)fd );
	return 1;
}
int UCFS_FClose(FS_FILE *fp)
{
	FS_FClose(fp);
	return 1;
}
*/