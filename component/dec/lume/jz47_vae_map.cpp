#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <utils/Log.h>
#include <utils/threads.h>

#include "config.h"
#include "jz_tcsm.h"
#include <sys/time.h>
#include <sched.h>

#define EFE__OFFSET 0x13240000
#define MC__OFFSET 0x13250000
#define VPU__OFFSET 0x13200000
#define CPM__OFFSET 0x10000000
#define AUX__OFFSET 0x132A0000
#define TCSM0__OFFSET 0x132B0000
#define TCSM1__OFFSET 0x132C0000
#define SRAM__OFFSET 0x132F0000
#define GP0__OFFSET 0x13210000
#define GP1__OFFSET 0x13220000
#define GP2__OFFSET 0x13230000
#define DBLK0__OFFSET 0x13270000
#define DBLK1__OFFSET 0x132D0000
#define SDE__OFFSET 0x13290000
#define JPGC__OFFSET 0x132E0000
#define VMAU__OFFSET 0x13280000


#define EFE__SIZE   0x00001000
#define MC__SIZE   0x00001000
#define VPU__SIZE 0x00001000
#define CPM__SIZE 0x00001000
#define AUX__SIZE 0x00004000
#define TCSM0__SIZE 0x00004000
#define TCSM1__SIZE 0x0000C000
#define SRAM__SIZE 0x00007000

#define GP0__SIZE   0x00001000
#define GP1__SIZE   0x00001000
#define GP2__SIZE   0x00001000
#define DBLK0__SIZE   0x00001000
#define DBLK1__SIZE   0x00001000
#define SDE__SIZE   0x00004000
#define JPGC__SIZE   0x00004000
#define VMAU__SIZE 0x0000F000

int tcsm_fd = -1;
volatile unsigned char *efe_base;
volatile unsigned char *ipu_base;
volatile unsigned char *mc_base;
volatile unsigned char *vpu_base;
volatile unsigned char *cpm_base;
volatile unsigned char *lcd_base;
volatile unsigned char *gp0_base;
volatile unsigned char *gp1_base;
volatile unsigned char *gp2_base;
volatile unsigned char *vmau_base;
volatile unsigned char *dblk0_base;
volatile unsigned char *dblk1_base;
volatile unsigned char *sde_base;
volatile unsigned char *jpgc_base;

volatile unsigned char *tcsm0_base;
volatile unsigned char *tcsm1_base;
volatile unsigned char *sram_base;
volatile unsigned char *ahb1_base;
volatile unsigned char *ddr_base;
volatile unsigned char *aux_base;
#define PMON_ON 887
#define PMON_OFF 889
#ifdef JZC_HW_MEDIA

unsigned char* mmap_tcsm(unsigned int offset, unsigned int len, int fd){
    unsigned char *addr = (unsigned char *)mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if(addr < 0){
	ALOGE("Error: mmap offset %x failed with error:%s", offset, strerror(errno));
	return NULL;
    }else{
	return addr;
    }
}

using namespace android;
Mutex VaeLock;
int VAECNT = 0;
int VAE_map() {

#if 1
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if ( sched_setaffinity(gettid(), sizeof(mask), &mask) == -1 ){
	ALOGE("[CPU] sched_setaffinity fail");
	return -1;
    }
#endif

    Mutex::Autolock autoLock(VaeLock);
    if (VAECNT == 0){
	ALOGE("[ VAE_map ] VAECNT == 0");
	tcsm_fd = open("/dev/jz-vpu", O_RDWR);
	if (tcsm_fd < 0) {
	    ALOGE("Error:open /dev/jz-vpu error.%s \n", strerror(errno));
	    return -1;
	}
   
	efe_base = mmap_tcsm(EFE__OFFSET, EFE__SIZE, tcsm_fd);
	mc_base = mmap_tcsm(MC__OFFSET, MC__SIZE, tcsm_fd);
	sde_base = mmap_tcsm(SDE__OFFSET, SDE__SIZE, tcsm_fd);
	dblk1_base = mmap_tcsm(DBLK1__OFFSET, DBLK1__SIZE, tcsm_fd);
	dblk0_base = mmap_tcsm(DBLK0__OFFSET, DBLK0__SIZE, tcsm_fd);
	vmau_base = mmap_tcsm(VMAU__OFFSET, VMAU__SIZE, tcsm_fd);
	vpu_base = mmap_tcsm(VPU__OFFSET, VPU__SIZE, tcsm_fd);
	aux_base = mmap_tcsm(AUX__OFFSET, AUX__SIZE, tcsm_fd);
	cpm_base = mmap_tcsm(CPM__OFFSET, CPM__SIZE, tcsm_fd);
	gp0_base = mmap_tcsm(GP0__OFFSET, GP0__SIZE, tcsm_fd);
	gp1_base = mmap_tcsm(GP1__OFFSET, GP1__SIZE, tcsm_fd);
	gp2_base = mmap_tcsm(GP2__OFFSET, GP2__SIZE, tcsm_fd);
	tcsm0_base = mmap_tcsm(TCSM0__OFFSET, TCSM0__SIZE, tcsm_fd);
	tcsm1_base = mmap_tcsm(TCSM1__OFFSET, TCSM1__SIZE, tcsm_fd);
	sram_base = mmap_tcsm(SRAM__OFFSET, SRAM__SIZE, tcsm_fd);
	jpgc_base = mmap_tcsm(JPGC__OFFSET, JPGC__SIZE, tcsm_fd);
    }
    VAECNT++;
    
    ALOGE("VAE map successfully");
    return 0;
}

void VAE_unmap() {
    Mutex::Autolock autoLock(VaeLock);
    VAECNT--;
    if (VAECNT == 0){
	ALOGE("[ VAE_unmap ] VAECNT == 0");
	munmap((void *)efe_base, EFE__SIZE);
	munmap((void *)mc_base, MC__SIZE);
	munmap((void *)vpu_base, VPU__SIZE);
	munmap((void *)cpm_base, CPM__SIZE);
	munmap((void *)sram_base, SRAM__SIZE);
	munmap((void *)sde_base, SDE__SIZE);
	munmap((void *)dblk1_base, DBLK1__SIZE);
	munmap((void *)dblk0_base, DBLK0__SIZE);
	munmap((void *)vmau_base, VMAU__SIZE);
	munmap((void *)aux_base, AUX__SIZE);
	munmap((void *)gp0_base, GP0__SIZE);
	munmap((void *)gp1_base, GP1__SIZE);
	munmap((void *)gp2_base, GP2__SIZE);
	munmap((void *)tcsm0_base, TCSM0__SIZE);
	munmap((void *)tcsm1_base, TCSM1__SIZE);
	munmap((void *)jpgc_base, JPGC__SIZE);
    
	mc_base = (unsigned char *)-1;
	sde_base = (unsigned char *)-1;
	dblk1_base = (unsigned char *)-1;
	dblk0_base = (unsigned char *)-1;
	vmau_base = (unsigned char *)-1;
	vpu_base = (unsigned char *)-1;
	aux_base = (unsigned char *)-1;
	cpm_base = (unsigned char *)-1;
	gp0_base = (unsigned char *)-1;
	gp1_base = (unsigned char *)-1;
	gp2_base = (unsigned char *)-1;
	tcsm0_base = (unsigned char *)-1;
	tcsm1_base = (unsigned char *)-1;
	sram_base = (unsigned char *)-1;
	jpgc_base = (unsigned char *)-1;
	  //asm("break %0"::"i"(PMON_OFF));
  
	int result = close(tcsm_fd);
	if(result < 0)
	    ALOGE("Error:close tcsm failed!!!!!!.%s", strerror(errno));

	tcsm_fd = -1;
    }
    ALOGE("VAE unmap successfully done\n");

}

void Lock_Vpu(){
      //ALOGE("Lock_Vpu");
    ioctl(tcsm_fd, 1, 0);
}

void UnLock_Vpu(){
      //ALOGE("UnLock_Vpu");
    ioctl(tcsm_fd, 2, 0);
}
#endif
