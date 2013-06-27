/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rn_frumv.h,v 1.1.1.1 2009/12/14 15:59:40 rliu Exp $
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc.
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


#ifndef HX_FRUMV_H__
#define HX_FRUMV_H__

#define FRAME_TYPE_I	0
#define FRAME_TYPE_P	1
#define FRAME_TYPE_B	2

#define FRU_MV_TYPE_INTRA 0
#define FRU_MV_TYPE_INTER 1

#include "hxtypes.h"
#include "hxresult.h"  // for HX_RESULT

#ifdef __cplusplus
extern "C" {
#endif


typedef struct tag_FRU_MV
{
	LONG32		iMVx2;
	LONG32		iMVy2;
} FRU_MV;

typedef struct tag_FRU_MV_INFO
{
	ULONG32		ulVersion;
	ULONG32		ulNumberOfElementsX;
	ULONG32		ulNumberOfElementsY;
	UCHAR		*pMBType;
	FRU_MV		*pMV;
} FRU_MV_INFO;

// different struct versions
#define FRU_VERSION_16x16_BLOCKS 0

/////////////////////////////////////////////////////////
//
// Exported FRU functions
// TBD: these should not be needed for Tromso
//
/////////////////////////////////////////////////////////
#if defined(_WIN32)
// On Windows rv90 is compiled with __stdcall by default
// We just changed rv2000 and rn_fru2 to be compiled with __cdecl
// and this is why we now explicitly need to define the calling
// convention for these functions.
extern HX_RESULT __cdecl HX_FRU_Init(
	void **HX_FRU_ref, 
	ULONG32 pels, ULONG32 lines,
	ULONG32 luma_pitch, ULONG32 chroma_pitch);

extern HX_RESULT __cdecl HX_FRU_Setup(
	void *HX_FRU_ref,
	ULONG32 mode,
	UCHAR* A[3],
	UCHAR* B[3],
	ULONG32 quant,
	ULONG32 pict_type,
	ULONG32 dt,
	FRU_MV_INFO* fru_mv_info);

extern HX_RESULT __cdecl HX_FRU_GetFrame(
	void *HX_FRU_ref,ULONG32 timePlacement,UCHAR* dst[3]);

extern HX_RESULT __cdecl HX_FRU_Free (void *HX_FRU_ref);

#else

extern HX_RESULT HX_FRU_Init(
	void **HX_FRU_ref, 
	ULONG32 pels, ULONG32 lines,
	ULONG32 luma_pitch, ULONG32 chroma_pitch);

extern HX_RESULT HX_FRU_Setup(
	void *HX_FRU_ref,
	ULONG32 mode,
	UCHAR* A[3],
	UCHAR* B[3],
	ULONG32 quant,
	ULONG32 pict_type,
	ULONG32 dt,
	FRU_MV_INFO* fru_mv_info);

extern HX_RESULT HX_FRU_GetFrame (
	void *HX_FRU_ref,ULONG32 timePlacement,UCHAR* dst[3]);

extern HX_RESULT HX_FRU_Free (void *HX_FRU_ref);
#endif

#ifdef __cplusplus
}
#endif


#endif
