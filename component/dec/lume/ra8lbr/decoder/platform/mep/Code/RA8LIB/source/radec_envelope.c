/* ***** BEGIN LICENSE BLOCK *****   
 * Source last modified: $Id: radec_envelope.c,v 1.1.1.1 2007/12/07 08:11:48 zpxu Exp $ 
 * 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM   
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc.   
 * All Rights Reserved.   
 *   
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Community 
 * Source License (the "RCSL"), including Attachment G and any 
 * applicable attachments, all available at 
 * http://www.helixcommunity.org/content/rcsl.  You may also obtain 
 * the license terms directly from RealNetworks.  You may not use this 
 * file except in compliance with the RCSL and its Attachments. There 
 * are no redistribution rights for the source code of this 
 * file. Please see the applicable RCSL for the rights, obligations 
 * and limitations governing use of the contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *   
 * Technology Compatibility Kit Test Suite(s) Location:   
 * https://rarvcode-tck.helixcommunity.org   
 *   
 * Contributor(s):   
 *   
 * ***** END LICENSE BLOCK ***** */  

/**************************************************************************** */
/*    Socrates Software Ltd : Toshiba Group Company */
/*    DESCRIPTION OF CHANGES: */
/*    
/*		1. Debug trace features added		
/*
 *    CONTRIBUTORS : Vinayak Bhat,Deepak,Sudeendra,Naveen
 *   */
/**************************************************************************** */

#include "radec_defines.h"

/* coding params for rms[0] */
#define RMS0BITS	 6	/* bits for env[0] */
#define RMS0MIN		-6	/* arbitrary! */
#define CODE2RMS(i)	((i)+(RMS0MIN))

#ifdef RA8_DEBUG
extern struct SRA8_Debug sRA8_Debug;
#endif
extern int *dmem_rmsIndex;
/**************************************************************************************
 * Function:    DecodeEnvelope
 *
 * Description: decode the power envelope
 *
 * Inputs:      pointer to initialized Gecko2Info struct
 *              number of bits remaining in bitstream for this frame
 *              index of current channel
 *
 * Outputs:     rmsIndex[0, ..., cregsions-1] has power index for each region
 *              updated rmsMax with largest value of rmsImdex for this channel
 *
 * Return:      number of bits remaining in bitstream, -1 if out-of-bits
 **************************************************************************************/
int DecodeEnvelope(Gecko2Info *gi,SBitStreamInfo *bsi, int availbits, int ch)
{
	int r, code, nbits, rprime, cache, rmsMax;
	int *rmsIndex; 

	int cntRgns = gi->sHdInfo.cRegions;
	int cntCplStart = gi->sHdInfo.cplStart;
	unsigned char FuncDetect = 1;

#ifdef __DMA_ENVELOPE__
	rmsIndex = dmem_rmsIndex;
#else
	rmsIndex = gi->sDecBuf.rmsIndex;
#endif

	if (availbits < RMS0BITS)
		return -1;

	/* unpack first index */
	code = GetBits(bsi, RMS0BITS, 1); 
	availbits -= RMS0BITS;
	rmsIndex[0] = CODE2RMS(code);

	/* check for escape code */
	/* ASSERT(rmsIndex[0] != 0); */

	rmsMax = rmsIndex[0];
#ifdef RA8_DEBUG
	sRA8_Debug.iRmsIndex[ch][0]=rmsIndex[0];
#endif
	for (r = 1; r < cntRgns; r++) {

		/* for interleaved regions, choose a reasonable table */
		if (r < 2 * cntCplStart) {
			rprime = r >> 1;
			if (rprime < 1) 
				rprime = 1;
		} else {
			rprime = r - cntCplStart;
		}
		
		/* above NUM_POWTABLES, always use the same Huffman table */
		if (rprime > NUM_POWTABLES) 
			rprime = NUM_POWTABLES;

		cache = GetBits(bsi, MAX_HUFF_BITS, 0);
		nbits = DecodeHuffmanScalar(huffTabPower, &huffTabPowerInfo[rprime-1], cache, &code,&FuncDetect);	
        if (nbits == -1)
			return -1;
		/* ran out of bits coding power envelope - should not happen (encoder spec) */
		if (nbits > availbits)
			return -1;

		availbits -= nbits;
		AdvanceBitstream(bsi, nbits);

		/* encoder uses differential coding with differences constrained to the range [-12, 11] */
		rmsIndex[r] = rmsIndex[r-1] + (code - 12);
		if (rmsIndex[r] > rmsMax)
			rmsMax = rmsIndex[r];
#ifdef RA8_DEBUG
		sRA8_Debug.iRmsIndex[ch][r]=rmsIndex[r];
#endif
	}
	gi->sBufferInfo.rmsMax[ch] = rmsMax;
#ifdef RA8_DEBUG
	sRA8_Debug.iRmsMax[ch]=gi->sBufferInfo.rmsMax[ch];
#endif
	return availbits;
}
