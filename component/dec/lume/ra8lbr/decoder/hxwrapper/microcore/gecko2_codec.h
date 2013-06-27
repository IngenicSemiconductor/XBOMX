/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
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
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef GECKO2_CODEC_H
#define GECKO2_CODEC_H

#include "audio_codec.h"

//#ifdef GECKO2_RPCODEC
#include "../gecko2decshim.h"
//#endif

//#ifdef GECKO2_ARM
//#include "arm/gecko2codecshim.h"
//#endif

class npGecko2Codec : public npAudioCodec
{
public:
    npGecko2Codec();
    virtual ~npGecko2Codec();

	int InitDecoder(int nSamples,
			int sampleRate,
			int nChannels,
			int nFrameBits,
			BYTE *pOpaqueData,
			int opaqueDataLength) ;

    // npAudioCodec interface functions
    virtual void Init(int type, const char* pConfig, int configSize);

    virtual bool Initialized() const;
    virtual int MaxSamples() const;
    virtual int SampleRate() const;
    virtual int Channels() const;
    virtual int Delay() const;

    virtual void Reset();
    virtual int Decode(const char* pData, int dataSize,
		       int& consumed,
		       signed short* pOut,
		       bool eof);

    virtual void Conceal(int samples);

private:
    bool m_initialized;
    int m_maxSamples;
    int m_sampleRate;
    int m_channels;
    int m_delay;
    int m_concealFrames;
    int m_frameSize;
    int m_delayFrames;
    CNewGeckoDecoderShim* m_pCodec;
};

#endif //_GECKO2_CODEC_H_
