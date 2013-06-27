#if 1
void tile_stuff(uint8_t *tile_y, uint8_t *tile_c, 
		uint8_t *frame_y, uint8_t *frame_u, uint8_t *frame_v,
		int linesize, int uvlinesize, 
		int mb_height, int mb_width, int expand)
{
    volatile uint8_t *pxl, *edge, *tile, *dest;
    int tile_linesize; 
    int mb_i, mb_j, i, j;

    if(expand)
      tile_linesize = mb_width*16 + 32*2;
    else
      tile_linesize = mb_width*16;

    fprintf(stderr, "linesize: %d, width: %d, height: %d, %p, %p\n", tile_linesize, mb_width, mb_height, tile_y, tile_c);
  
    for(mb_j=0; mb_j<mb_height; mb_j++){
      for(mb_i=0; mb_i<mb_width; mb_i++){
        tile = tile_y + mb_j*16*tile_linesize + mb_i*16*16;
        dest = frame_y + mb_j*16*linesize + mb_i*16;
        for(j=0; j<16; j++){
          for(i=0; i<16; i++){
            *tile++ = dest[i];
          }
          dest += linesize;
        }
        tile = tile_c + mb_j*8*tile_linesize + mb_i*16*8;
        dest = frame_u + mb_j*8*uvlinesize + mb_i*8;
        for(j=0; j<8; j++){
          for(i=0; i<8; i++){
            tile[i] = dest[i];
          }
          tile += 16;
          dest += uvlinesize;
        }
        tile = tile_c + mb_j*8*tile_linesize + mb_i*16*8 + 8;
        dest = frame_v + mb_j*8*uvlinesize + mb_i*8;
        for(j=0; j<8; j++){
          for(i=0; i<8; i++){
            tile[i] = dest[i];
          }
          tile += 16;
          dest += uvlinesize;
        }
      }
    }
    
    if(expand){
      /********************* top ***********************/
      for(mb_i=0; mb_i<mb_width; mb_i++){
        // Y
        pxl = tile_y + mb_i*16*16;
        edge = pxl - tile_linesize*32;
        for(j=0; j<16; j++){
          memcpy(edge, pxl, 16);
          memcpy(edge+tile_linesize*16, pxl, 16);
          edge += 16;
        }
        // C
        pxl = tile_c + mb_i*16*8;
        edge = pxl - tile_linesize*16;
        for(j=0; j<8; j++){
          memcpy(edge, pxl, 16);
          memcpy(edge+tile_linesize*8, pxl, 16);
          edge += 16;
        }
      }
      
      /********************* bottom ***********************/
      for(mb_i=0; mb_i<mb_width; mb_i++){
        // Y
        edge = tile_y + mb_height*16*tile_linesize + mb_i*16*16;
        pxl = edge - tile_linesize*16 + 16*15;
        for(j=0; j<16; j++){
          memcpy(edge, pxl, 16);
          memcpy(edge+tile_linesize*16, pxl, 16);
          edge += 16;
        }
        // C
        edge = tile_c + mb_height*8*tile_linesize + mb_i*16*8;
        pxl = edge - tile_linesize*8 + 16*7;
        for(j=0; j<8; j++){
          memcpy(edge, pxl, 16);
          memcpy(edge+tile_linesize*8, pxl, 16);
          edge += 16;
        }
      }
      /********************* left ***********************/
      for(mb_i=-2; mb_i<mb_height+2; mb_i++){
        // Y
        for(j=0; j<16; j++){
          pxl = tile_y + mb_i*16*tile_linesize + 16*j;
          edge = pxl - 16*32;
          memset(edge, pxl[0], 16);
          memset(edge+16*16, pxl[0], 16);
        }
        // C
        for(j=0; j<8; j++){
          pxl = tile_c + mb_i*8*tile_linesize + 16*j;
          edge = pxl - 8*32;
          memset(edge, pxl[0], 8);
          memset(edge+8, pxl[8], 8);
          memset(edge+8*16, pxl[0], 8);
          memset(edge+8*16+8, pxl[8], 8);
        }
      }
      
      /********************* right ***********************/
      for(mb_i=-2; mb_i<mb_height+2; mb_i++){
        // Y
        for(j=0; j<16; j++){
          pxl = tile_y + (mb_width-1)*16*16 + 15 + mb_i*16*tile_linesize + 16*j;
          edge = pxl - 15 + 16*16;
          memset(edge, pxl[0], 16);
          memset(edge+16*16, pxl[0], 16);
        }
        // C
        for(j=0; j<8; j++){
          pxl = tile_c + (mb_width-1)*16*8 + 7 + mb_i*8*tile_linesize + 16*j;
          edge = pxl - 7 + 16*8;
          memset(edge, pxl[0], 8);
          memset(edge+8, pxl[8], 8);
          memset(edge+8*16, pxl[0], 8);
          memset(edge+8*16+8, pxl[8], 8);
        }
      }
    }
}

#else
# include "soc/jzmedia.h"

void tile_stuff(uint8_t *tile_y, uint8_t *tile_c, 
		uint8_t *frame_y, uint8_t *frame_u, uint8_t *frame_v,
		int linesize, int uvlinesize, 
		int mb_height, int mb_width, int expand)
{
    //volatile uint8_t *pxl, *edge, *tile, *dest;
    uint8_t *pxl, *edge, *edge1, *tile, *dest, *dest_c;
    int tile_linesize; 
    int mb_i, mb_j, i;
    
    if(expand)
        tile_linesize = mb_width*16 + 32*2;
    else
        tile_linesize = mb_width*16;

  //fprintf(stderr, "linesize: %d, width: %d, height: %d, %08x, %08x\n", tile_linesize, mb_width, mb_height, tile_y, tile_c);
  
    for(mb_j = 0; mb_j < mb_height; mb_j++){
        for(mb_i = 0; mb_i < mb_width; mb_i++){
            tile = tile_y + mb_j*16*tile_linesize + mb_i*16*16;
            dest = frame_y + mb_j*16*linesize + mb_i*16;
            for(i = 0; i < 16; i++){
/*                 for(i=0; i<16; i++){ */
/*                     *tile++ = dest[i]; */
/*                 } */
                S32LDD(xr1, dest, 0x0);
                S32LDD(xr2, dest, 0x4);
                S32LDD(xr3, dest, 0x8);
                S32LDD(xr4, dest, 0xC);

                S32STD(xr1, tile, 0x0);
                S32STD(xr2, tile, 0x4);
                S32STD(xr3, tile, 0x8);
                S32STD(xr4, tile, 0xC);

                tile += 16;
                dest += linesize;
            }
            tile = tile_c + mb_j*8*tile_linesize + mb_i*16*8;
            dest = frame_u + mb_j*8*uvlinesize + mb_i*8;
            dest_c = frame_v + mb_j*8*uvlinesize + mb_i*8;
            for(i = 0; i < 8; i++){
/*                 for(i=0; i<8; i++){ */
/*                     tile[i] = dest[i]; */
/*                 } */
                S32LDD(xr1, dest, 0x0);
                S32LDD(xr2, dest, 0x4);
                S32LDD(xr3, dest_c, 0x0);
                S32LDD(xr4, dest_c, 0x4);

                S32STD(xr1, tile, 0x0);
                S32STD(xr2, tile, 0x4);
                S32STD(xr3, tile, 0x8);
                S32STD(xr4, tile, 0xC);

                tile += 16;
                dest += uvlinesize;
                dest_c += uvlinesize;
            }
        }
    }

    if(expand){
        /********************* top ***********************/
        for(mb_i = 0; mb_i < mb_width; mb_i++){
            // Y
            pxl = tile_y + mb_i*16*16;
            edge = pxl - tile_linesize*32;
            edge1 = pxl - tile_linesize*16;
            
            S32LDD(xr1, pxl, 0x0);
            S32LDD(xr2, pxl, 0x4);
            S32LDD(xr3, pxl, 0x8);
            S32LDD(xr4, pxl, 0xC);
          
            for(i = 0; i < 16; i++){
                //memcpy(edge, pxl, 16);
                //memcpy(edge+tile_linesize*16, pxl, 16);
                S32STD(xr1, edge, 0x0);
                S32STD(xr2, edge, 0x4);
                S32STD(xr3, edge, 0x8);
                S32STD(xr4, edge, 0xC);

                S32STD(xr1, edge1, 0x0);
                S32STD(xr2, edge1, 0x4);
                S32STD(xr3, edge1, 0x8);
                S32STD(xr4, edge1, 0xC);

                edge += 16;
                edge1 += 16;
            }
            // C

            pxl = tile_c + mb_i*16*8;
            edge = pxl - tile_linesize * 16;
            edge1 = pxl - tile_linesize * 8;

            S32LDD(xr1, pxl, 0x0);
            S32LDD(xr2, pxl, 0x4);
            S32LDD(xr3, pxl, 0x8);
            S32LDD(xr4, pxl, 0xC);

            for(i = 0; i < 8; i++){
                //memcpy(edge, pxl, 16);
                //memcpy(edge+tile_linesize*8, pxl, 16);
                S32STD(xr1, edge, 0x0);
                S32STD(xr2, edge, 0x4);
                S32STD(xr3, edge, 0x8);
                S32STD(xr4, edge, 0xC);
                
                S32STD(xr1, edge1, 0x0);
                S32STD(xr2, edge1, 0x4);
                S32STD(xr3, edge1, 0x8);
                S32STD(xr4, edge1, 0xC);

                edge += 16;
                edge1 += 16;
            }
        }
      
        /********************* bottom ***********************/
        for(mb_i = 0; mb_i < mb_width; mb_i++){
            // Y
            edge = tile_y + mb_height*16*tile_linesize + mb_i*16*16;
            edge1 = edge + 16*tile_linesize;
            pxl = edge - tile_linesize*16 + 16*15;

            S32LDD(xr1, pxl, 0x0);
            S32LDD(xr2, pxl, 0x4);
            S32LDD(xr3, pxl, 0x8);
            S32LDD(xr4, pxl, 0xC);

            for(i = 0; i < 16; i++){
                //memcpy(edge, pxl, 16);
                //memcpy(edge+tile_linesize*16, pxl, 16);
                S32STD(xr1, edge, 0x0);
                S32STD(xr2, edge, 0x4);
                S32STD(xr3, edge, 0x8);
                S32STD(xr4, edge, 0xC);
                
                S32STD(xr1, edge1, 0x0);
                S32STD(xr2, edge1, 0x4);
                S32STD(xr3, edge1, 0x8);
                S32STD(xr4, edge1, 0xC);

                edge += 16;
                edge1 += 16;
            }
            // C
            edge = tile_c + mb_height*8*tile_linesize + mb_i*16*8;
            edge1 = tile_c + 8*tile_linesize;
            pxl = edge - tile_linesize*8 + 16*7;

            S32LDD(xr1, pxl, 0x0);
            S32LDD(xr2, pxl, 0x4);
            S32LDD(xr3, pxl, 0x8);
            S32LDD(xr4, pxl, 0xC);

            for(i = 0; i < 8; i++){
                //memcpy(edge, pxl, 16);
                //memcpy(edge+tile_linesize*8, pxl, 16);
                S32STD(xr1, edge, 0x0);
                S32STD(xr2, edge, 0x4);
                S32STD(xr3, edge, 0x8);
                S32STD(xr4, edge, 0xC);
                
                S32STD(xr1, edge1, 0x0);
                S32STD(xr2, edge1, 0x4);
                S32STD(xr3, edge1, 0x8);
                S32STD(xr4, edge1, 0xC);

                edge += 16;
                edge1 += 16;
            }
        }
        /********************* left ***********************/
        for(mb_i = -2; mb_i < mb_height + 2; mb_i++){
            // Y
            for(i = 0; i < 16; i++){
                pxl = tile_y + mb_i*16*tile_linesize + 16*i;
                edge = pxl - 16*32;
                edge1 = pxl - 16*16;
                //memset(edge, pxl[0], 16);
                //memset(edge+16*16, pxl[0], 16);
                
                S8LDD(xr1, pxl, 0x0, ptn7);
                
                S32STD(xr1, edge, 0x0);
                S32STD(xr1, edge, 0x4);
                S32STD(xr1, edge, 0x8);
                S32STD(xr1, edge, 0xC);

                S32STD(xr1, edge1, 0x0);
                S32STD(xr1, edge1, 0x4);
                S32STD(xr1, edge1, 0x8);
                S32STD(xr1, edge1, 0xC);
            }
            // C
            for(i = 0; i < 8; i++){
                pxl = tile_c + mb_i*8*tile_linesize + 16*i;
                edge = pxl - 8*32;
                edge = pxl - 8*16;
                //memset(edge, pxl[0], 8);
                //memset(edge+8, pxl[8], 8);
                //memset(edge+8*16, pxl[0], 8);
                //memset(edge+8*16+8, pxl[8], 8);

                S8LDD(xr1, pxl, 0x0, ptn7);
                
                S32STD(xr1, edge, 0x0);
                S32STD(xr1, edge, 0x4);
                S32STD(xr1, edge1, 0x0);
                S32STD(xr1, edge1, 0x4);

                S8LDD(xr1, pxl, 0x8, ptn7);

                S32STD(xr1, edge, 0x8);
                S32STD(xr1, edge, 0xC);
                S32STD(xr1, edge1, 0x8);
                S32STD(xr1, edge1, 0xC);
            }
        }
        
        /********************* right ***********************/
        for(mb_i = -2; mb_i < mb_height + 2; mb_i++){
            // Y
            for(i = 0; i < 16; i++){
                pxl = tile_y + (mb_width-1)*16*16 + 15 + mb_i*16*tile_linesize + 16*i;
                edge = pxl - 15 + 16*16;
                edge1 = pxl - 15 + 16*32;
                //memset(edge, pxl[0], 16);
                //memset(edge+16*16, pxl[0], 16);
                
                S8LDD(xr1, pxl, 0x0, ptn7);

                S32STD(xr1, edge, 0x0);
                S32STD(xr1, edge, 0x4);
                S32STD(xr1, edge, 0x8);
                S32STD(xr1, edge, 0xC);

                S32STD(xr1, edge1, 0x0);
                S32STD(xr1, edge1, 0x4);
                S32STD(xr1, edge1, 0x8);
                S32STD(xr1, edge1, 0xC);
            }
            // C
            for(i = 0; i < 8; i++){
                pxl = tile_c + (mb_width-1)*16*8 + 7 + mb_i*8*tile_linesize + 16*i;
                edge = pxl - 7 + 16*8;
                edge = pxl - 7 + 16*16;
                //memset(edge, pxl[0], 8);
                //memset(edge+8, pxl[8], 8);
                //memset(edge+8*16, pxl[0], 8);
                //memset(edge+8*16+8, pxl[8], 8);

                S8LDD(xr1, pxl, 0x0, ptn7);
                
                S32STD(xr1, edge, 0x0);
                S32STD(xr1, edge, 0x4);
                S32STD(xr1, edge1, 0x0);
                S32STD(xr1, edge1, 0x4);

                S8LDD(xr1, pxl, 0x8, ptn7);

                S32STD(xr1, edge, 0x8);
                S32STD(xr1, edge, 0xC);
                S32STD(xr1, edge1, 0x8);
                S32STD(xr1, edge1, 0xC);
            }
        }
    }
}
#endif


// Current we only used one forward predict picture, so we just exchange the pointer Ok
// In future, we may take more pred pic, then we need followed func
#ifdef CHECK_FDEC
void yuv_stuff(uint8_t *frame_y, uint8_t *frame_u, uint8_t *frame_v, 
		uint8_t *tile_y, uint8_t *tile_c,
		int linesize, int uvlinesize, 
		int mb_height, int mb_width)
{
    volatile uint8_t *tile, *dest;
    int mb_i, mb_j, i, j;

    for(mb_j = 0; mb_j < mb_height; mb_j++){
        for(mb_i = 0; mb_i < mb_width; mb_i++){
            tile = tile_y + mb_j*16*linesize + mb_i*16*16;
            dest = frame_y + mb_j*16*linesize + mb_i*16;
            for(j = 0; j < 16; j++){
                for(i = 0; i < 16; i++){
                    dest[i] = *tile++;
                }
                dest += linesize;
            }
            tile = tile_c + mb_j*8*linesize + mb_i*16*8;
            dest = frame_u + mb_j*8*uvlinesize + mb_i*8;
            for(j = 0; j < 8; j++){
                for(i = 0; i < 8; i++){
                    dest[i] = tile[i];
                }
                tile += 16;
                dest += uvlinesize;
            }
            tile = tile_c + mb_j*8*linesize + mb_i*16*8 + 8;
            dest = frame_v + mb_j*8*uvlinesize + mb_i*8;
            for(j = 0; j < 8; j++){
                for(i = 0; i < 8; i++){
                    dest[i] = tile[i];
                }
                tile += 16;
                dest += uvlinesize;
            }
        }
    }
}
#endif

