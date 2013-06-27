#ifndef __UCLIB_H__
#define __UCLIB_H__
#define	RAND_MAX	2147483647

//extern "C"
//{
//	
//	unsigned int alloc(unsigned int nbytes);
//	unsigned int Drv_realloc(unsigned int address,unsigned int nbytes);
//	void deAlloc(unsigned int address);
//  unsigned int Drv_calloc(unsigned int size,unsigned int n);
//}


#include <stdlib.h>
#include <string.h>
#include <sys/types.h> /* size_t */
#include <linux/errno.h>
#include <sys/unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include "jz_mxu.h"

//typedef struct
//{
//  int quot;			// Quotient.  
//  int rem;			// Remainder.  
//} div_t;
//
#undef div
  
#define div(a, b)                       \
({                                      \
  div_t result;                         \
  result.quot = a / b;                  \
  result.rem = a - (result.quot * b);   \
  result;                               \
})

#define off_t int 

#define abs(a)	   (((a) < 0) ? -(a) : (a))

//#define LocalAlloc(x,size) (void *)alloc(size)
//#define LocalFree(x) deAlloc(()unsigned int)x)
/*

*/
//-------------------------------------------------------------------


extern unsigned alloc(unsigned int nbytes);
extern unsigned int alignAlloc(unsigned int align,unsigned int size);
extern void deAlloc(unsigned int address);
extern unsigned int Drv_realloc(unsigned int address,unsigned int nbytes);
extern unsigned int Drv_calloc(unsigned int size,unsigned int n);
//-------------------------------------------------------------------

//---------------------------------------------------------------------------------
extern void *Module_Alloc(unsigned int nbyte);
extern void *Module_Realloc(void* address,unsigned int nbytes);
extern void *Module_CAlloc(unsigned int size,unsigned int n);
extern void *Module_alignAlloc(unsigned int align,unsigned int n);
//----------------------------------------------------------------
extern void GM_Dealloc(void* addr);
//-------------------------------------------------------------------
static inline void *uc_malloc_static(unsigned int addr)
{
	return (void *)alloc((unsigned int)addr);
}
static inline void uc_free_static(void *addr)
{
	if(addr != NULL)
		deAlloc((unsigned int)addr);
}
static inline void *uc_calloc_static(unsigned int x,unsigned int n)
{
	return (void *)Drv_calloc(x,n);
}
static inline void *uc_realloc_static(void *addr,unsigned int size)
{
	return (void *)Drv_realloc((unsigned int)addr,(unsigned int)size);
}
static inline void *uc_memalign_static(unsigned int x,unsigned int size)
{
	return (void *)alignAlloc(x,(unsigned int)size);
}
//-------------------------------------------------------------------

#define MP_USER_DRIVER_MEM 0
#if MP_USER_DRIVER_MEM
static inline void *uc_malloc(unsigned int addr)
{
	return (void *)static_uc_malloc((unsigned int)addr);	;
}
static inline void uc_free(void *addr)
{
		static_uc_free((unsigned int)addr);
}
static inline void *uc_calloc(unsigned int x,unsigned int n)
{
	return (void *)static_uc_calloc(x,n);
}
static inline void *uc_realloc(void *addr,unsigned int size)
{
	return static_uc_realloc((unsigned int)addr,(unsigned int)size);
}
static inline void *uc_memalign(unsigned int x,unsigned int size)
{
	return (void *)static_uc_memalign(x,(unsigned int)size);
}
#else
#if 0
static inline void *uc_malloc(unsigned int addr)
{
	return (void *)Module_Alloc((unsigned int)addr);	;
}
static inline void uc_free(void *addr)
{
	if(addr != NULL)
		GM_Dealloc(addr);
}
static inline void *uc_calloc(unsigned int x,unsigned int n)
{
	return (void *)Module_CAlloc(x,n);
}
static inline void *uc_realloc(void *addr,unsigned int size)
{
	return (void *)Module_Realloc(addr,(unsigned int)size);
}

static inline void *uc_memalign(unsigned int x,unsigned int size)
{
	return (void *)Module_alignAlloc(x,(unsigned int)size);
}
#endif
#endif
static inline char * uc_strdup(const char *str)
{
   char *p;
   if (!str)
      return(NULL);
   if ((p = (char *)uc_malloc(strlen(str) + 1)) != 0)
      return(strcpy(p,str));
   return(NULL);
}


#undef malloc
//#define MEMDEBUG 1
#ifdef MEMDEBUG
#define malloc(x) ({void * mem;printf("malloc:%s %d",__FILE__,__LINE__);mem = uc_malloc(x);printf(" addr = %08x len = %d\n",mem,x);mem;})
#else
#define malloc(x) uc_malloc(x)
#endif

#undef free

#ifdef MEMDEBUG
#define free(x) ({printf("free:%s %d addr = %08x\n",__FILE__,__LINE__,x);uc_free(x);})
#else
#define free(x) uc_free(x)
#endif

#undef callloc

#ifdef MEMDEBUG
#define calloc(x,y) ({void *mem;printf("calloc:%s %d",__FILE__,__LINE__);mem = uc_calloc(x,y);printf(" addr = %08x len = %d\n",mem,x,y);mem;})
#else
#define calloc(x,y) uc_calloc(x,y)
#endif

#undef realloc
#ifdef MEMDEBUG
#define realloc(x,y) ({void *mem;printf("realloc:%s %d",__FILE__,__LINE__);mem = uc_realloc(x,y);printf(" addr = %08x %d\n",mem,y);mem;})
#else
#define realloc(x,y) uc_realloc(x,y)
#endif

#undef memalign

#ifdef MEMDEBUG
 #define memalign(x,y) ({void *mem;printf("memalign:%s %d",__FILE__,__LINE__); mem = uc_memalign(x,y);printf(" addr = %08x %d\n",mem,y);mem;})
#else
 #define memalign(x,y) uc_memalign(x,y)
#endif
#undef strdup

#ifdef MEMDEBUG
#define strdup(x) ({char * mem;printf("strdup:%s %d",__FILE__,__LINE__);mem = uc_strdup(x);printf(" addr = %08x\n",mem);mem;})
#else
#define strdup(x) uc_strdup(x)
#endif


#define F(x,s...) {printf("%s:%d  ",__FILE__,__LINE__); printf(x,##s);}
#undef stderr
#define stderr 1
#undef stdout
#define stdout 2
#undef fprintf
#define fprintf(x,y,c...) ({printf("%s %d",__FILE__,__LINE__); printf(y,##c);})

#undef vfprintf
#define vfprintf fprintf

#define MY_THROW() asm("syscall")
#undef exit
#define exit(x) do{ printf("=======================%s %d exit\n",__FILE__,__LINE__); return x;}while(0)
#undef perror
#define perror printf
#undef fflush
#define fflush(x) 

/*
inline void * mxu_memset_32 (void *BLOCK, int C) {
  int32_t tmp; 
  S32I2M(xr1, C);
  tmp = (int32_t)BLOCK - 4;
  S32SDI(xr1, tmp, 4);
  S32SDI(xr1, tmp, 4);
  S32SDI(xr1, tmp, 4);
  S32SDI(xr1, tmp, 4);
  S32SDI(xr1, tmp, 4);
  S32SDI(xr1, tmp, 4);
  S32SDI(xr1, tmp, 4);
  S32SDI(xr1, tmp, 4);
}
*/




static inline void *uc_memcpy_tmp(void *to,const void *from,unsigned int size)
{
	return memcpy(to,from,size);
}

static inline void *uc_memcpy(void *to,const void *from,unsigned int size)
{
#if 1
	if((size >= 32) && ((unsigned int)to & 3) == 0 && ((unsigned int)from & 3) == 0)
	{
		register unsigned int tmp_src,tmp_dst,i;
		tmp_src = from - 4;
		tmp_dst = to - 4;
    i =  tmp_dst + (size & (~31)); 
    	while(tmp_dst < i)
    	{
		    S32LDI(xr1, tmp_src, 4);
		    S32LDI(xr2, tmp_src, 4);
		    S32LDI(xr3, tmp_src, 4);
		    S32LDI(xr4, tmp_src, 4);
		    S32LDI(xr5, tmp_src, 4);
		    S32LDI(xr6, tmp_src, 4);
		    S32LDI(xr7, tmp_src, 4);
		    S32LDI(xr8, tmp_src, 4);
		    
		    S32SDI(xr1, tmp_dst, 4);
		    S32SDI(xr2, tmp_dst, 4);
		    S32SDI(xr3, tmp_dst, 4);
		    S32SDI(xr4, tmp_dst, 4);
		    S32SDI(xr5, tmp_dst, 4);
		    S32SDI(xr6, tmp_dst, 4);
		    S32SDI(xr7, tmp_dst, 4);
		    S32SDI(xr8, tmp_dst, 4);      
		    
	  }
   i += ((size & 31) & (~3));
    
    while(tmp_dst < i)
    {
    	S32LDI(xr1, tmp_src, 4);
		  S32SDI(xr1, tmp_dst, 4);
    }
   
    switch((size & 3))
    {
    	case 0:
    		break;
    	case 1:
    		tmp_dst += 4;
    		tmp_src += 4;
    		*((unsigned char *)tmp_dst) = *((unsigned char *)tmp_src);
    		break;
    	case 2:
    		tmp_dst += 4;
    		tmp_src += 4;
    		*((unsigned char *)tmp_dst++) = *((unsigned char *)tmp_src++); 	
    		*((unsigned char *)tmp_dst) = *((unsigned char *)tmp_src); 	
    		break;
    	case 3:
    		tmp_dst += 4;
    		tmp_src += 4;
    		*((unsigned char *)tmp_dst++) = *((unsigned char *)tmp_src++); 	
    		*((unsigned char *)tmp_dst++) = *((unsigned char *)tmp_src++); 	
    		*((unsigned char *)tmp_dst) = *((unsigned char *)tmp_src); 	
    		break;
    }
    
	}else 
		#endif	
		uc_memcpy_tmp(to,from,size);	
}
//void *uc_memcpy(void *to,const void *from,unsigned int size);
static inline void uc_memset_tmp(void *to,unsigned char v,unsigned int size)
{
	memset(to,v,size);
}
static inline void uc_memset(void *to,unsigned char v,unsigned int size)
{
	unsigned int d;
	unsigned char *c_to;
	unsigned int end_to = (unsigned int)to + size;
	unsigned int i_to;
	unsigned int i_tmp;
	#define MEMSET_JZ_MXU 1
	
	if(size < 32 + 8)
	{
		uc_memset_tmp(to,v,size);
		return;
	}	
	//F("size = %d %x\n",size,(unsigned int) to);
	d = 4 - ((unsigned int)to & 3);
	c_to = to;
	//F("d = %d\n",d);
	switch(d)
	{
			case 1:
				*c_to++ = v;
				break;
			case 2:
				*c_to++ = v;
				*c_to++ = v;
				break;
			case 3:
				*c_to++ = v;
				*c_to++ = v;
				*c_to++ = v;
				break;
			default:
				break;
	}
	
	i_to = (unsigned int)c_to - 4;
	d = (unsigned int)v | ((unsigned int)v << 8) | ((unsigned int)v << 16) | ((unsigned int)v << 24);
	S32I2M(xr1,d);
	if((unsigned int)c_to & (31))
	{	
			i_tmp = (i_to & (~31)) + 32;
		
		  //F("i_tmp = %x i_to = %x\n",i_tmp,i_to);
		
			while(i_tmp > i_to){
				
			#if MEMSET_JZ_MXU
			S32SDI(xr1,i_to,4);
			//S32SDIV(xr1,i_to,4,0);
		
			#else
			  i_to += 4;
			  *(unsigned int *)i_to = d;
		  #endif   
		}
  }
	i_tmp = ((end_to - 31) & (~31));
	
	//F("i_tmp = %x i_to = %x\n",i_tmp,i_to);
	
	while(i_tmp > i_to)  
	{
		#if MEMSET_JZ_MXU
		i_pref(30,i_to + 4,0);
		S32SDI(xr1,i_to,4);
		S32SDI(xr1,i_to,4);
		S32SDI(xr1,i_to,4);
		S32SDI(xr1,i_to,4);
		S32SDI(xr1,i_to,4);
		S32SDI(xr1,i_to,4);
		S32SDI(xr1,i_to,4);
		S32SDI(xr1,i_to,4);
		#else
		i_to += 4;
		*(unsigned int *)i_to = d;//S32SDI(xr1,i_to,4);
	  i_to += 4;
	  *(unsigned int *)i_to = d;//S32SDI(xr1,i_to,4);
	  i_to += 4;
	  *(unsigned int *)i_to = d;//S32SDI(xr1,i_to,4);
	  i_to += 4;
	  *(unsigned int *)i_to = d;//S32SDI(xr1,i_to,4);
	  i_to += 4;
		*(unsigned int *)i_to = d;//S32SDI(xr1,i_to,4);
	  i_to += 4;
	  *(unsigned int *)i_to = d;//S32SDI(xr1,i_to,4);
	  i_to += 4;
	  *(unsigned int *)i_to = d;//S32SDI(xr1,i_to,4);
	  i_to += 4;
	  *(unsigned int *)i_to = d;//S32SDI(xr1,i_to,4);
	  #endif
	  
	}
	i_tmp = ((end_to - 4) & (~3));
	//F("i_tmp = %x i_to = %x\n",i_tmp,i_to);
	while(i_tmp > i_to)
	{
	  #if MEMSET_JZ_MXU
	  S32SDI(xr1,i_to,4);
		#else
		i_to		              += 4;
	  *(unsigned int *)i_to = d;
		#endif
	}
	i_tmp = (end_to & 3);
	c_to = i_to + 4;
	//F("i_tmp = %x c_to = %x\n",i_tmp,c_to);
	switch(i_tmp)
	{
		case 1:
			*c_to++ = v;
			break;
		case 2:
			*c_to++ = v;
			*c_to++ = v;
			break;
		case 3:
			
			*c_to++ = v;
			*c_to++ = v;
			*c_to++ = v;
			break;  		
	}
	//if((unsigned int)c_to != end_to)
	//	while(1);	
}
#undef memset
//#define DEBUG_MEMSET
#ifdef DEBUG_MEMSET
#define memset(x,y,z) do{printf("%s:%d->%s\n",__FILE__,__LINE__,__FUNCTION__);uc_memset(x,y,z);}while(0)
#else
#define memset(x,y,z) uc_memset(x,y,z)
#endif

#undef memcpy
#define memcpy(x,y,z) uc_memcpy(x,y,z)

#undef memove
#define memove(x,y,z) uc_memcpy(x,y,z)

#define ShowAddress()          \
do                             \
{			unsigned int dra;        \
			__asm__ __volatile__(    \
			"sw $31,0x00(%0)\n\t"    \
            :                  \
			: "r" (&dra));           \
		F("%x\n",dra);             \
}while(0)

#endif


