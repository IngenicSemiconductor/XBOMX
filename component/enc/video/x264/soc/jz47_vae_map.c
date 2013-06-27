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
#include "config.h"
#include "jz47_vae_map.h"
#include "config_jz_soc.h"

#define CPM__OFFSET    0x10000000
#define VPU__OFFSET    0x13200000
#define AUX__OFFSET    0x132A0000
#define TCSM__OFFSET   0x132C0000
#define SRAM__OFFSET   0x132F0000
#define VDMA__OFFSET   0x13210000
#define EFE__OFFSET    0x13240000
#define MC__OFFSET     0x13250000
#define DBLK__OFFSET   0x13270000
#define VMAU__OFFSET   0x13280000
#define SDE__OFFSET    0x13290000
#define JPGC__OFFSET   0x132E0000

#define CPM__SIZE      0x00001000
#define VPU__SIZE      0x00001000
#define AUX__SIZE      0x00004000
#define TCSM__SIZE     0x00008000
#define SRAM__SIZE     0x00004000
#define VDMA__SIZE     0x00001000
#define EFE__SIZE      0x00001000
#define MC__SIZE       0x00001000
#define DBLK__SIZE     0x0000F000
#define VMAU__SIZE     0x0000F000
#define SDE__SIZE      0x00001000
#define JPGC__SIZE     0x00004000

static int tcsm_fd;
volatile unsigned char * cpm_base;
volatile unsigned char * vpu_base;
volatile unsigned char * aux_base;
volatile unsigned char * tcsm_base;
volatile unsigned char * sram_base;
volatile unsigned char * vdma_base;
volatile unsigned char * efe_base;
volatile unsigned char * mc_base;
volatile unsigned char * dblk_base;
volatile unsigned char * vmau_base;
volatile unsigned char * sde_base;
volatile unsigned char * jpgc_base;

volatile unsigned char * ipu_base;
volatile unsigned char * ahb1_base;
volatile unsigned char * ddr_base;
volatile unsigned char * lcd_base;

//#define JZC_PMON_P0_VAE
#ifdef JZC_PMON_P0_VAE
#define PMON_ON_VAE 887
#define PMON_OFF_VAE 889
#endif //JZC_PMON_P0

#define CPM_VPU_SWRST    (cpm_base + 0xC4)
#define CPM_VPU_SR     	 (0x1<<31)
#define CPM_VPU_STP    	 (0x1<<30)
#define CPM_VPU_ACK    	 (0x1<<29)

#define write_cpm_reg(a)    (*(volatile unsigned int *)(CPM_VPU_SWRST) = a)
#define read_cpm_reg()      (*(volatile unsigned int *)(CPM_VPU_SWRST))


#define RST_VPU()            \
{\
     write_cpm_reg(read_cpm_reg() | CPM_VPU_STP); \
     while( !(read_cpm_reg() & CPM_VPU_ACK) );  \
     write_cpm_reg( (read_cpm_reg() | CPM_VPU_SR) & (~CPM_VPU_STP) ); \
     write_cpm_reg( read_cpm_reg() & (~CPM_VPU_SR) & (~CPM_VPU_STP) ); \
}

static unsigned char * mmap_tcsm(unsigned int offset, unsigned int len, int fd) {
    unsigned char * addr = (unsigned char *) mmap( (void *)0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if(addr < 0){
	ALOGE("Error: mmap offset %x failed with error : %s", offset, strerror(errno));
	return NULL;
    }else{
	return addr;
    }
}

void VAE_map() {
#ifdef JZC_PMON_P0_VAE
    asm("break %0"::"i"(PMON_ON_VAE));
#endif //JZC_PMON_P0

      // tricky appoach to use TCSM
    tcsm_fd = open("/dev/jz-vpu", O_RDWR);
    if (tcsm_fd < 0) {
	ALOGE("Error:open /dev/jz-vpu error.%s \n", strerror(errno));
	exit(1);
    }

    cpm_base    = mmap_tcsm(CPM__OFFSET, CPM__SIZE, tcsm_fd);
    vpu_base    = mmap_tcsm(VPU__OFFSET, VPU__SIZE, tcsm_fd);
    aux_base    = mmap_tcsm(AUX__OFFSET, AUX__SIZE, tcsm_fd);
    tcsm_base   = mmap_tcsm(TCSM__OFFSET, TCSM__SIZE, tcsm_fd); 
    sram_base   = mmap_tcsm(SRAM__OFFSET, SRAM__SIZE, tcsm_fd);
    vdma_base   = mmap_tcsm(VDMA__OFFSET, VDMA__SIZE, tcsm_fd); 
    efe_base    = mmap_tcsm(EFE__OFFSET, EFE__SIZE, tcsm_fd);
    mc_base     = mmap_tcsm(MC__OFFSET, MC__SIZE, tcsm_fd);
    dblk_base   = mmap_tcsm(DBLK__OFFSET, DBLK__SIZE, tcsm_fd);
    vmau_base   = mmap_tcsm(VMAU__OFFSET, VMAU__SIZE, tcsm_fd);
    sde_base    = mmap_tcsm(SDE__OFFSET, SDE__SIZE, tcsm_fd);
    jpgc_base   = mmap_tcsm(JPGC__OFFSET, JPGC__SIZE, tcsm_fd);

    *(volatile unsigned int *)(cpm_base + 0x28) &= ~0x4;
    RST_VPU();

    fprintf(stderr, "VAE mmap successfully done!\n");
}

void VAE_unmap() {
#ifdef JZC_PMON_P0_VAE
    asm("break %0"::"i"(PMON_OFF_VAE));
#endif //JZC_PMON_P0

    munmap(cpm_base,   CPM__SIZE);
    munmap(vpu_base,   VPU__SIZE);
    munmap(tcsm_base,  TCSM__SIZE);
    munmap(sram_base,  SRAM__SIZE);
    munmap(mc_base,    MC__SIZE);
    munmap(dblk_base,  DBLK__SIZE);
    munmap(vmau_base,  VMAU__SIZE);
    munmap(sde_base,   SDE__SIZE);
    munmap(jpgc_base,  JPGC__SIZE);
    sram_base = 0 ;  

    printf("VAE unmap successfully done!\n\n");
    close(tcsm_fd);
}
