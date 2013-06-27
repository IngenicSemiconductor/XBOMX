#ifndef _LOADFILE_H_
#define _LOADFILE_H_
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 by dsqiu (dsqiu@ingenic.cn)
 */

#include <stdio.h>
#include "config.h"
int loadfile(char *filename,void *address,int presize,int ischeck);
#endif /* _LOADFILE_H_ */

