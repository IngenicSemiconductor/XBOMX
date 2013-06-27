/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hwpfdefs.h,v 1.1.1.1 2009/12/14 15:59:40 rliu Exp $
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the Real Format Source Code
 * Porting and Optimization License, available at
 * https://helixcommunity.org/2005/license/realformatsource (unless
 * RealNetworks otherwise expressly agrees in writing that you are
 * subject to a different license).  You may also obtain the license
 * terms directly from RealNetworks.  You may not use this file except
 * in compliance with the Real Format Source Code Porting and
 * Optimization License. There are no redistribution rights for the
 * source code of this file. Please see the Real Format Source Code
 * Porting and Optimization License for the rights, obligations and
 * limitations governing use of the contents of the file.
 * 
 * RealNetworks is the developer of the Original Code and owns the
 * copyrights in the portions it created.
 * 
 * This file, and the files included with this file, is distributed and
 * made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL
 * SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT
 * OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 * https://rarvcode-tck.helixcommunity.org
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef _HWPFDEFS_H_
#define _HWPFDEFS_H_

// Flags for DVPFHeader

// import video source flags
#define HX_SYSTEM_BUFFER    0x00000001  // Standard video buffer
#define HX_DMA_BUFFER       0x00000002  // DMA ready video buffer
#define HX_COPY_VIDEO       0x00000004  // Manually copy video data to hw surface

// import alpha flags
#define HX_COPY_ALPHA       0x00000008  // Manually copy alpha data to hw surface
#define HX_DISABLE_ALPHA    0x00000010  // Disable hw alpha blending

typedef struct _tagDVPFHeader
{
    INT32   lRV8Strengths[4];

    UINT32  ulFlags;

    void*   lpSurface;                  // device specific surface
    
} DVPFHeader;

#endif //_HWPFDEFS_H_

