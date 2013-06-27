#ifndef __UCFSDEF__
#define __UCFSDEF__

#include <fcntl.h>
#define FS_size_t       unsigned long       /* 32 bit unsigned */
#define FS_u32          unsigned long       /* 32 bit unsigned */
#define FS_i32          signed long         /* 32 bit signed */
#define FS_u16          unsigned short      /* 16 bit unsigned */
#define FS_i16          signed short        /* 16 bit signed */
typedef struct {
  FS_u32 fileid_lo;          /* unique id for file (lo)      */
  FS_u32 fileid_hi;          /* unique id for file (hi)      */
  FS_u32 fileid_ex;          /* unique id for file (ex)      */
  FS_i32 EOFClust;           /* used by FAT FSL only         */
  FS_u32 CurClust;           /* used by FAT FSL only         */
  FS_i32 filepos;            /* current position in file     */
  FS_i32 size;               /* size of file                 */
  int dev_index;             /* index in _FS_devinfo[]       */
  FS_i16 error;              /* error code                   */
  unsigned char inuse;       /* handle in use mark           */
  unsigned char mode_r;      /* mode READ                    */
  unsigned char mode_w;      /* mode WRITE                   */
  unsigned char mode_a;      /* mode APPEND                  */
  unsigned char mode_c;      /* mode CREATE                  */
  unsigned char mode_b;      /* mode BINARY                  */
} FS_FILE;



/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

/*********************************************************************
*
*             STD file I/O functions
*/

FS_FILE             *FS_FOpen(const char *pFileName, const char *pMode);
void                FS_FClose(FS_FILE *pFile);
FS_size_t           FS_FRead(void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);
FS_size_t           FS_FWrite(const void *pData, FS_size_t Size, FS_size_t N, FS_FILE *pFile);


/*********************************************************************
*
*             file pointer handling
*/

int                 FS_FSeek(FS_FILE *pFile, FS_i32 Offset, int Whence);
FS_i32              FS_FTell(FS_FILE *pFile);


/*********************************************************************
*
*             I/O error handling
*/

FS_i16              FS_FError(FS_FILE *pFile);
void                FS_ClearErr(FS_FILE *pFile);


/*********************************************************************
*
*             file functions
*/

int                 FS_Remove(const char *pFileName);


/*********************************************************************
*
*             IOCTL
*/

int                 FS_IoCtl(const char *pDevName, FS_i32 Cmd, FS_i32 Aux, void *pBuffer);
/* Maximum size of a directory name */
#define FS_DIRNAME_MAX            255
#define FS_ino_t  int

struct FS_DIRENT {
  FS_ino_t  d_ino;                      /* to be POSIX conform */
  char      d_name[FS_DIRNAME_MAX];
  char      FAT_DirAttr;                /* FAT only. Contains the "DIR_Attr" field of an entry. */
};

typedef struct {
  struct FS_DIRENT  dirent;  /* cunrrent directory entry     */
  FS_u32 dirid_lo;           /* unique id for file (lo)      */
  FS_u32 dirid_hi;           /* unique id for file (hi)      */
  FS_u32 dirid_ex;           /* unique id for file (ex)      */
  FS_i32 dirpos;             /* current position in file     */
  FS_i32 size;               /* size of file                 */
  int dev_index;             /* index in _FS_devinfo[]       */
  FS_i16 error;              /* error code                   */
  unsigned char inuse;       /* handle in use mark           */
} FS_DIR;

FS_DIR              *FS_OpenDir(const char *pDirName);
int                 FS_CloseDir(FS_DIR *pDir);
struct FS_DIRENT    *FS_ReadDir(FS_DIR *pDir);
void                FS_RewindDir(FS_DIR *pDir);
int                 FS_MkDir(const char *pDirName);
int                 FS_RmDir(const char *pDirName);

int UCFS_FClose(FS_FILE *fp);
#define FILE   FS_FILE
#define fopen  FS_FOpen
#define fclose UCFS_FClose
#define fread  FS_FRead
#define fwrite FS_FWrite
#define fseek  FS_FSeek
#define ftell  FS_FTell
#define ferror  FS_FError

#undef fgets
char *fgets(char *line,int size,FILE *fp);
#undef fprintf
#define fprintf(x,y,c...) ({printf("%s %d",__FILE__,__LINE__); printf(y,##c);})
#undef stderr
#define stderr 1
#undef stdout
#define stdout 2
#undef vfprintf
#define vfprintf fprintf
//int fprintf(FILE * stream, const char * format,...);
#define EOF 0 
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0
#undef mkdir
#define mkdir(x,y) FS_MkDir(x)
#undef rmdir
#define rmdir FS_RmDir 

#undef open
#define open uc_fopen
#undef close
#define close uc_fclose
#define read 
#define read uc_fread
#undef lseek
#define lseek uc_fseek

#undef sync
#define sync uc_fsync


// open
extern FS_FILE *  BUFF_Open ( const char *pFileName, const char *pMode );
// close
extern void   BUFF_Close( FS_FILE * PORT  );
// read
extern FS_size_t  BUFF_Read ( void *pData, FS_size_t Size, FS_size_t N, FS_FILE *MY_FS  );
// seek
extern int   BUFF_Seek ( FS_FILE *MY_FS, FS_i32 Offset, int Whence );


inline static int uc_fopen( const char * pathname, int flags,mode_t mode)
{
	//printf("uc_fopen\n");
	return   (int)BUFF_Open( pathname, "rb" );
	//return (int)FS_FOpen(pathname,"rb");
}
inline static ssize_t uc_fread(int fd,void * buf ,size_t count)
{
	return (ssize_t)BUFF_Read( buf, 1,count,(FS_FILE *)fd );
	// return (ssize_t)FS_FRead(buf,1,count,(FS_FILE *)fd);
} 

inline static int uc_fsync(void)
{
	return 1;
} 

inline static ssize_t uc_fwrite (int fd,const void * buf,size_t count)
{
} 
inline static loff_t uc_fseek (int fd, loff_t offset, int whence)
{
	return BUFF_Seek((FS_FILE *)fd,offset, whence  );
}
inline static int uc_fclose (int fd)
{
  BUFF_Close( (FS_FILE *)fd );
	return 1;
}
inline static int UCFS_FClose(FS_FILE *fp)
{
	FS_FClose(fp);
	return 1;
}

/*
int uc_fopen( const char * pathname, int flags,mode_t mode);
ssize_t uc_fread(int fd,void * buf ,size_t count); 
int uc_fsync(void); 

ssize_t uc_fwrite (int fd,const void * buf,size_t count); 
loff_t uc_flseek (int fd, loff_t offset, int whence);
int uc_fclose (int fd);
*/

#endif //__UCFSDEF__
