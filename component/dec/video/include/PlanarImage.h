#ifndef PLANNAR_IAMGE_H
#define PLANNAR_IAMGE_H

typedef struct{
    uint32_t phy_planar[4];
    uint32_t planar[4];
    uint32_t stride[4];
    uint32_t isvalid;
    int64_t  pts;
    int      is_dechw;
    void* memheapbase[4];
    uint32_t memheapbase_offset[4];
}PlanarImage;

#endif
