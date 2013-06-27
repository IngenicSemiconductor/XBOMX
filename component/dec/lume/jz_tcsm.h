
#ifndef __JZ_TCSM_H
#define __JZ_TCSM_H

#define TCSM_TOCTL_SET_MMAP (0x99 + 0x1)

struct tcsm_mmap {
	unsigned int start;
	unsigned int len;
};

#endif

