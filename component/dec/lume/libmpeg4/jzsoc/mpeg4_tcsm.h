/*****************************************************************************
 *
 * JZ4760 TCSM Space Seperate
 *
 * $Id: mpeg4_tcsm.h,v 1.1 2010/10/25 12:06:34 yyang Exp $
 *
 ****************************************************************************/

#ifndef __VP6_TCSM_H__
#define __VP6_TCSM_H__

#define TCSM0_BANK0 0xF4000000
#define TCSM0_BANK1 0xF4001000
#define TCSM0_BANK2 0xF4002000
#define TCSM0_BANK3 0xF4003000

#define TCSM1_BANK0 0xF4000000
#define TCSM1_BANK1 0xF4001000
#define TCSM1_BANK2 0xF4002000
#define TCSM1_BANK3 0xF4003000

#define SRAM_BANK0  0x132D0000
#define SRAM_BANK1  0x132D1000
#define SRAM_BANK2  0x132D2000
#define SRAM_BANK3  0x132D3000

/*
  XXXX_PADDR:       physical address
  XXXX_VCADDR:      virtual cache-able address
  XXXX_VUCADDR:     virtual un-cache-able address 
*/
#define TCSM0_PADDR(a)        (((a) & 0xFFFF) | 0x132B0000) 
#define TCSM0_VCADDR(a)       (((a) & 0xFFFF) | 0x932B0000) 
#define TCSM0_VUCADDR(a)      (((a) & 0xFFFF) | 0xB32B0000) 

#define TCSM1_PADDR(a)        (((a) & 0xFFFF) | 0x132B0000) 
#define TCSM1_VCADDR(a)       (((a) & 0xFFFF) | 0x932B0000) 
#define TCSM1_VUCADDR(a)      (((a) & 0xFFFF) | 0xB32B0000) 

#define SRAM_PADDR(a)         (((a) & 0xFFFF) | 0x132D0000) 
#define SRAM_VCADDR(a)        (((a) & 0xFFFF) | 0x932D0000) 
#define SRAM_VUCADDR(a)       (((a) & 0xFFFF) | 0xB32D0000) 

#define VP6_YUNIT             (12*11*4)
#define VP6_CUNIT             (4*3*4)
#define VP6_RYBUF_LEN         (VP6_YUNIT*4)
#define VP6_RCBUF_LEN         (VP6_CUNIT*4)
#define RECON_BUF_STRD   (80)
/*Reference Buf: previous(frontward) direction*/
#define TCSM1_RYPBUF          (TCSM1_BANK0)
#define TCSM1_RUPBUF          (TCSM1_RYPBUF+VP6_RYBUF_LEN)
#define TCSM1_RVPBUF          (TCSM1_RUPBUF+VP6_RCBUF_LEN)
/*Reference Buf: future(backward) direction*/
#define TCSM1_RYFBUF          (TCSM1_RVPBUF+VP6_RCBUF_LEN)
#define TCSM1_RUFBUF          (TCSM1_RYFBUF+VP6_RYBUF_LEN)
#define TCSM1_RVFBUF          (TCSM1_RUFBUF+VP6_RCBUF_LEN)

#if 1
#define VP6_YCSTRD            (20)
#define VP6_CCSTRD            (12)
#define VP6_DYBUF_LEN         (VP6_YCSTRD*16)
#define VP6_DCBUF_LEN         (VP6_CCSTRD* 8)
/*Destination Buf: previous(frontward) direction*/
/*Destination Buf: future(backward) direction*/
#define TCSM1_DYPBUF           (TCSM1_RVFBUF+VP6_RCBUF_LEN)
#define TCSM1_DYFBUF           (TCSM1_DYPBUF+VP6_DYBUF_LEN)
#define TCSM1_DUPBUF           (TCSM1_DYFBUF+VP6_DYBUF_LEN)
#define TCSM1_DUFBUF           (TCSM1_DUPBUF+VP6_DCBUF_LEN)
#define TCSM1_DVPBUF           (TCSM1_DUFBUF+VP6_DCBUF_LEN)
#define TCSM1_DVFBUF           (TCSM1_DVPBUF+VP6_DCBUF_LEN)
#endif

#define TCSM1_YUVRECON_BUF  (TCSM1_RVFBUF+VP6_RCBUF_LEN)
/*------ MC Descriptor Chain -------*/
/*
  +--------------------------------+<-- DHA - 12
  | FID: MC processed SBLK ID(Fdir)|
  +--------------------------------+<-- DHA - 8
  | PID: MC processed SBLK ID(Pdir)|
  +--------------------------------+<-- DHA - 4
  | SF: SYNC Status Flag           |
  +--------------------------------+<-- DHA
  | Y MB Start Addr (3 wrod)       |
  |   -- FDD Data Node Head        |
  |   -- Y Reference Start Addr    |
  |   -- Y Destination Start Addr  |
  +--------------------------------+
  | SBLK CMD (1 wrod)              |
  |   -- FDD Auto mode configure   |
  +--------------------------------+
  | SBLK CMD (1 wrod)              |
  |   -- FDD Auto mode configure   |
  +--------------------------------+
   .
   .
   .
  +--------------------------------+
  | JP: JUMP Node (1 wrod)         |
  +--------------------------------+
  | U MB Start Addr (3 wrod)       |
  |   -- FDD Data Node Head        |
  |   -- U Reference Start Addr    |
  |   -- U Destination Start Addr  |
  +--------------------------------+
  | SBLK CMD (1 wrod)              |
  |   -- FDD Auto mode configure   |
  +--------------------------------+
   .
   .
   .
  +--------------------------------+
  | Last SBLK CMD (1 wrod)         |
  |   -- FDD Auto mode configure   |
  +--------------------------------+
  | JP: JUMP Node (1 wrod)         |
  +--------------------------------+
  | FDD SYNC Node (1 wrod)         |
  +--------------------------------+
  Note:
       1. PID: MC FDD auto-write MC's current processed ID into this Address
               CPU can read it to know current MC's progress.
       2. SF:  In FDD SYNC node, FDD auto write the flag into this place.
  VP6_NLEN:   Single(Y/U/V) Chain length(3word Start Addr + 16SBLK Node)  
 */
#define VP6_NLEN_Y             (2+4*4+1)
#define VP6_NLEN_C             (2+2+1)

#define TCSM1_MCC_YFID         (TCSM1_DVFBUF+VP6_DCBUF_LEN) 
#define TCSM1_MCC_YPID         (TCSM1_MCC_YFID+4)
#define TCSM1_MCC_YSF          (TCSM1_MCC_YPID+4)
/*MC chain: previous(frontward) direction start address*/
/*MC chain: future(backward) direction start address*/
#define TCSM1_MCC_YPS          (TCSM1_MCC_YSF +4)
#define TCSM1_MCC_YFS          (TCSM1_MCC_YPS+VP6_NLEN_Y*4)
#define TCSM1_MCC_YSN          (TCSM1_MCC_YFS+VP6_NLEN_Y*4)

#define TCSM1_MCC_CFID         (TCSM1_MCC_YSN +4) 
#define TCSM1_MCC_CPID         (TCSM1_MCC_CFID+4)
#define TCSM1_MCC_CSF          (TCSM1_MCC_CPID+4)

#define TCSM1_MCC_UPS          (TCSM1_MCC_CSF+4)
#define TCSM1_MCC_UFS          (TCSM1_MCC_UPS+VP6_NLEN_C*4)
#define TCSM1_MCC_VPS          (TCSM1_MCC_UFS+VP6_NLEN_C*4)
#define TCSM1_MCC_VFS          (TCSM1_MCC_VPS+VP6_NLEN_C*4)
#define TCSM1_MCC_CSN          (TCSM1_MCC_VFS+VP6_NLEN_C*4) 

#define EDGE_WIDTH 24
#define EMU_BUF_W 24
#define TCSM1_EMU_LEN  (6*EMU_BUF_W * 12)  
#define TCSM1_EMU_BUF  (TCSM1_MCC_CSN + 4)

#define TCSM0_PHY_ADDR(addr) (((unsigned int)addr & 0xFFFF) | 0x132b0000)
#define TCSM1_PHY_ADDR(addr) (((unsigned int)addr & 0xFFFF) | 0x132c0000)

#endif /*__VP6_TCSM_H__*/
