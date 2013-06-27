#ifndef __MPEG4_VPU_H__
#define __MPEG4_VPU_H__
#define write_vpu_reg(off, value) (*((volatile unsigned int *)(off)) = (value))
#define read_vpu_reg(off, ofst)   (*((volatile unsigned int *)(off + ofst)))
#define REG_SCH_GLBC         0x00000
#define REG_SCH_TLBA         0x00030
#define SCH_GLBC_TLBE        (0x1<<30)
#define SCH_GLBC_TLBINV      (0x1<<29)

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


#endif
