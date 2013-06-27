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
#include "symbol_map.h"
#include "gecko2_codec_bld.h"

static const sysLibrarySymbolEntry z_pSymbolMap[] = {
    {"npGecko2CodecBuilder", "create_npGecko2CodecBuilder"}
};

extern "C" {
    __declspec(dllexport) int npGetSymbolMap(const sysLibrarySymbolEntry*& pMap);
    __declspec(dllexport) npGecko2CodecBuilder* create_npGecko2CodecBuilder();
}

int npGetSymbolMap(const sysLibrarySymbolEntry*& pMap)
{
    pMap = z_pSymbolMap;
    return sizeof(z_pSymbolMap) / sizeof(sysLibrarySymbolEntry);
}

npGecko2CodecBuilder* create_npGecko2CodecBuilder()
{
    return new npGecko2CodecBuilder(true);
}

