#ifndef __JZ47_VAE_MAP__
#define __JZ47_VAE_MAP__

typedef volatile struct{
    void *ipu_base;
    void *mc_base;
    void *vpu_base;
    void *cpm_base;
    void *lcd_base;
    void *gp0_base;
    void *gp1_base;
    void *gp2_base;
    void *vmau_base;
    void *dblk0_base;
    void *dblk1_base;
    void *sde_base;

    void *tcsm0_base;
    void *tcsm1_base;
    void *sram_base;
    void *ahb1_base;
    void *ddr_base;
    void *aux_base;
}VAE_ADDR;


void VAE_map();
void VAE_unmap();

#endif
