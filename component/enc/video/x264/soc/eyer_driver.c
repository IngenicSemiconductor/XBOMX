#include "eyer_driver.h"
#include "errno.h"
#undef printf
//#define AUX_EYER_SIM
//unsigned int do_get_phy_addr(unsigned int vaddr);
int write_cnt = 0 ;
int read_cnt = 0 ;


void write_fifo(unsigned int *wptr, unsigned int *rptr)
{
  write_reg((wptr), (read_reg((wptr), 0)+1));
}

void wait_fifo_notfull(unsigned int *wptr, unsigned int *rptr, int fifo_depth)
{
  while (read_reg((wptr), 0) >=(read_reg((rptr), 0)+fifo_depth))
    {
    }
}

void aux_clr()
{
}

void wait_fifo_notempty(unsigned int *wptr, unsigned int *rptr)
{
  while(read_reg((wptr), 0) == read_reg((rptr), 0))
    {
      sleep(1);
    }
}
//#define AUX_RESPOR

void wait_aux_end(volatile unsigned int *end_ptr, int endvalue)
{
  while(*end_ptr != endvalue)
    {
    }
  write_reg((end_ptr), 0);
  //printf("aux end!!\n");
}

void consume_fifo(unsigned int *wptr, unsigned int *rptr)
{
  write_reg((rptr), (read_reg((rptr), 0)+1));
}

void load_aux_pro_bin(char * name, unsigned int addr)
{
  FILE *fp;
  printf("load aux pro bin %s to %x-%p\n", name, (addr), do_get_phy_addr(addr));
  fp = fopen(name, "r+b");
  if (!fp)
    printf(" error while open %s \n", name);
  unsigned int but_ptr[1024];
  int len;

  while ( ( len = fread(but_ptr, 4, 1024, fp) ) ){
    printf(" len of aux task(word) = %d\n",len);
    int i;
    for ( i = 0; i<len; i++)
      {
	write_reg((addr), but_ptr[i]);
	addr = addr + 4;
      }
  }
  printf("pro end address: %x-%p\n", (addr), do_get_phy_addr(addr));
  fclose(fp);
}

void eyer_stop()
{
  fprintf(stderr, "--------eyer stop-------\n");
}

unsigned int get_phy_addr (unsigned int vaddr);

void* do_get_phy_addr(unsigned int vaddr){
  if ( vaddr >= (unsigned int)tcsm0_base && vaddr < (unsigned int)tcsm0_base + TCSM0__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)tcsm0_base;
      return (void *)(ofst+TCSM0__OFFSET);
    }
  else if ( vaddr >= (unsigned int)tcsm1_base && vaddr < (unsigned int)tcsm1_base + TCSM1__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)tcsm1_base;
      return (void *)(ofst+TCSM1__OFFSET);
    }
  else if ( vaddr >= (unsigned int)sram_base && vaddr < (unsigned int)sram_base + SRAM__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)sram_base;
      return (void *)(ofst+SRAM__OFFSET);
    }
  else if ( vaddr >= (unsigned int)vpu_base && vaddr < (unsigned int)vpu_base + VPU__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)vpu_base;
      return (void *)(ofst+VPU__OFFSET);
    }
  else if ( vaddr >= (unsigned int)gp0_base && vaddr < (unsigned int)gp0_base + GP0__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)gp0_base;
      return (void *)(ofst+GP0__OFFSET);
    }
  else if ( vaddr >= (unsigned int)gp1_base && vaddr < (unsigned int)gp1_base + GP1__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)gp1_base;
      return (void *)(ofst+GP1__OFFSET);
    }
  else if ( vaddr >= (unsigned int)gp2_base && vaddr < (unsigned int)gp2_base + GP2__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)gp2_base;
      return (void *)(ofst+GP2__OFFSET);
    }
  else if ( vaddr >= (unsigned int)mc_base && vaddr < (unsigned int)mc_base + MC__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)mc_base;
      return (void *)(ofst+MC__OFFSET);
    }
  else if ( vaddr >= (unsigned int)dblk0_base && vaddr < (unsigned int)dblk0_base + DBLK0__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)dblk0_base;
      return (void *)(ofst+DBLK0__OFFSET);
    }
  else if ( vaddr >= (unsigned int)dblk1_base && vaddr < (unsigned int)dblk1_base + DBLK1__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)dblk1_base;
      return (void *)(ofst+DBLK1__OFFSET);
    }
  else if ( vaddr >= (unsigned int)vmau_base && vaddr < (unsigned int)vmau_base + VMAU__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)vmau_base;
      return (void *)(ofst+VMAU__OFFSET);
    }
  else if ( vaddr >= (unsigned int)aux_base && vaddr < (unsigned int)aux_base + AUX__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)aux_base;
      return (void *)(ofst+AUX__OFFSET);
    }
  else if ( vaddr >= (unsigned int)sde_base && vaddr < (unsigned int)sde_base + SDE__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)sde_base;
      return (void *)(ofst+SDE__OFFSET);
    }
  else 
    return (void *)get_phy_addr(vaddr);
}


