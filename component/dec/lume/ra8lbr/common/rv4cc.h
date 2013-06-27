/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0 and Exhibits. 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM 
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. 
 * All Rights Reserved. 
 * 
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Community Source 
 * License Version 1.0 (the "RCSL"), including Attachments A though H, 
 * all available at http://www.helixcommunity.org/content/rcsl. 
 * You may also obtain the license terms directly from RealNetworks. 
 * You may not use this file except in compliance with the RCSL and 
 * its Attachments. There are no redistribution rights for the source 
 * code of this file. Please see the applicable RCSL for the rights, 
 * obligations and limitations governing use of the contents of the file. 
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 * 
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 * https://rarvcode-tck.helixcommunity.org 
 * 
 * Contributor(s): 
 * 
 * ***** END LICENSE BLOCK ***** */ 

/*////////////////////////////////////////////////////////// */
/* */
/*    INTEL Corporation Proprietary Information */
/* */
/*    This listing is supplied under the terms of a license */
/*    agreement with INTEL Corporation and may not be copied */
/*    nor disclosed except in accordance with the terms of */
/*    that agreement. */
/* */
/*    Copyright (c) 1997-1999 Intel Corporation. */
/*    All Rights Reserved. */
/* */
/*////////////////////////////////////////////////////////// */

#ifndef RV4CC_H__
#define RV4CC_H__

/* $Header: /icdev/cvsroot/jz4760/mplayer.10rc2/librv9/common/rv4cc.h,v 1.1.1.1 2009/12/14 15:59:40 rliu Exp $ */

/* This header defines FOURCC codes of interest, except those such as BI_RGB */
/* that are available from a standard MSVC include file. */
/* All FOURCC codes defined here are in upper case. */

#define RV_CHARS_TO_FOURCC(a,b,c,d) \
    (((U32)((U8)(d) << 24)) | ((U8)(c) << 16) | ((U8)(b) << 8) | ((U8)(a)))

#define FOURCC_UNDEFINED    RV_CHARS_TO_FOURCC('1','D','A','B')
    /* FOURCC_UNDEFINED is used to represent an unknown fourcc, i.e., a BAD 1 ! */

#define FOURCC_H261         RV_CHARS_TO_FOURCC('H','2','6','1')
#define FOURCC_H263         RV_CHARS_TO_FOURCC('H','2','6','3')
#define FOURCC_H263PLUS     RV_CHARS_TO_FOURCC('I','L','V','C')
#define FOURCC_RV89COMBO       RV_CHARS_TO_FOURCC('T','R','O','M')
    /* Note: TROM is a temporary four CC code. */
#define FOURCC_IF09         RV_CHARS_TO_FOURCC('I','F','0','9')
#define FOURCC_ILVR         RV_CHARS_TO_FOURCC('I','L','V','R')
#define FOURCC_IYUV         RV_CHARS_TO_FOURCC('I','Y','U','V')
#define FOURCC_MPEG2V       RV_CHARS_TO_FOURCC('M','P','2','V')
    /* Note: MPEG2V is a temporary four CC code being used to represent */
    /* MPEG-2 video.  It is not an officially registered four CC code. */
#define FOURCC_YUV12        RV_CHARS_TO_FOURCC('I','4','2','0')
#define FOURCC_YUY2         RV_CHARS_TO_FOURCC('Y','U','Y','2')
#define FOURCC_YV12         RV_CHARS_TO_FOURCC('Y','V','1','2')
#define FOURCC_YVU9         RV_CHARS_TO_FOURCC('Y','V','U','9')

#if !defined(BI_BITMAP)
#define BI_BITMAP           RV_CHARS_TO_FOURCC('B', 'I', 'T', 'M')
#endif

#endif /* RV4CC_H__ */
