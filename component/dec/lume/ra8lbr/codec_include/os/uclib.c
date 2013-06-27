#include "uclib.h"
/*
inline void *uc_malloc(unsigned int addr)
{
	unsigned int d;
		
	return (void *)alloc((unsigned int)addr);	;
}
inline void uc_free(void *addr)
{
	if(addr != NULL)
		deAlloc((unsigned int)addr);
}
inline void *uc_calloc(unsigned int x,unsigned int n)
{
	return (void *)Drv_calloc(x,n);
}
inline void *uc_realloc(void *addr,unsigned int size)
{
	return Drv_realloc((unsigned int)addr,(unsigned int)size);
}
char * uc_strdup(const char *str)
{
   char *p;
   if (!str)
      return(NULL);
   if (p = uc_malloc(strlen(str) + 1))
      return(strcpy(p,str));
   return(NULL);
}
inline void *uc_memalign(unsigned int x,unsigned int size)
{
	return (void *)alignAlloc(x,(unsigned int)size);
}
*/
void dumpdatabuf(unsigned char *buf,unsigned int len)
{
	int i,j;
	printf("\n");
	for(i = 0;i < len / 16;i++)
	{
		printf("%04x: ",i);
		for(j = 0; j < 16; j++)
			printf("%02x ",buf[j + i * 16]);
		printf("\n");	
	}
	if((i & 15) > 0)
	{
			printf("%02x: ",j);
		for(j = 0; j < (i & 15); j++)
			printf("%02x ",buf[j + i * 16]);
		printf("\n");	
	}
}