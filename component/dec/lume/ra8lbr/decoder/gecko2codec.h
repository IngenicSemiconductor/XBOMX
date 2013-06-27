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

/**************************************************************************************
 * Fixed-point RealAudio 8 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * October 2003
 *
 * gecko2codec.h - public C API for Gecko2 decoder
 **************************************************************************************/

#ifndef _GECKO2CODEC_H
#define _GECKO2CODEC_H

#if defined(_WIN32) && !defined(_WIN32_WCE)
#
#elif defined(_WIN32) && defined(_WIN32_WCE) && defined(ARM)
#
#elif defined(_WIN32) && defined(WINCE_EMULATOR)
#
#elif defined(ARM_ADS)
#
#elif defined(_SYMBIAN) && defined(__WINS__)
#
#elif defined(__GNUC__) && defined(ARM)
#
#elif defined(__GNUC__) && defined(__i386__)
#
// added by fanoble
#elif defined(__GNUC__) && defined(__mips__)
#
#elif defined(_OPENWAVE)
#
#else
#error No platform defined. See valid options in gecko2codec.h.
#endif

#ifdef __cplusplus 
extern "C" {
#endif

typedef void *HGecko2Decoder;

enum {
	ERR_GECKO2_NONE =                  0,
	ERR_GECKO2_INVALID_SIDEINFO =     -1,

	ERR_UNKNOWN =                  -9999
};

/* public API */
HGecko2Decoder Gecko2InitDecoder(int nSamples, int nChannels, int nRegions, int nFrameBits, int sampRate, int cplStart, int cplQbits, int *codingDelay);
void Gecko2FreeDecoder(HGecko2Decoder hGecko2Decoder);
int Gecko2Decode(HGecko2Decoder hGecko2Decoder, unsigned char *codebuf, int lostflag, short *outbuf);

#ifdef __cplusplus 
}
#endif

#endif	/* _GECKO2CODEC_H */

