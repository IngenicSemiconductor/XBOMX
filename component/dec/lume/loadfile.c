#include "loadfile.h"
#include <utils/Log.h>
#include <stdlib.h>
#define EL(x,y...) {ALOGE("%s %d",__FILE__,__LINE__); ALOGE(x,##y); }
//#define printf EL
//address is 4byte align
int loadfile(char *filename,void *address,int presize,int ischeck)
{
	FILE *fp_text;
	char binpath[256];
	int len,i;
	int *dataspace = NULL,*reserved_mem;
	char *envpath;
	if(!filename) return -1;
	if(ischeck)
	{
		dataspace = malloc(presize);
		if(!dataspace)
		{
			printf("mem space too small\n");
			dataspace = address;
		}
	}else
		dataspace = address;
	printf("%s",filename);
	envpath = getenv("LD_OPENCORE_BIN");

#if 0
	if(envpath)
		snprintf(binpath,256,"%s/%s",envpath,filename);
	else
#endif
		snprintf(binpath,256,"%s/%s",LOADBIN_PATH,filename);
	printf(" open %s %d\n",binpath,presize);
	fp_text = fopen(binpath, "rb");	
	if (!fp_text)
	{
		printf(" error while open %s \n",binpath);
		if(ischeck) free(dataspace);
		return -1;
	}
	
	len = fread(dataspace, 4, (presize / 4), fp_text);
	if(len == 0)
	{
		printf("read %s %x file error %d\n",binpath,fp_text,len);
		if(ischeck) free(dataspace);
		return -1;
	}

	if(dataspace != address)
	{
		reserved_mem = (int *)address;
		for(i = 0;i < len;i++)
			reserved_mem[i] = dataspace[i];
		for(i = 0;i < len;i++)
		{
			if(reserved_mem[i] != dataspace[i])
			{
				printf("address[%d] can not read & write!\n",i);
				if(ischeck) free(dataspace);
				return -1;
			}
		}
	}
	fclose(fp_text);
	if(ischeck) free(dataspace);
	return len * 4;
}
