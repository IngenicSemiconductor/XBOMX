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

/*/////////////////////////////////////////////////////////////////////////// */
/*    RealNetworks, Inc. Confidential and Proprietary Information. */
/* */
/*    Copyright (c) 1995-2002 RealNetworks, Inc. */
/*    All Rights Reserved. */
/* */
/*    Do not redistribute. */
/* */
/*/////////////////////////////////////////////////////////////////////////// */
/*////////////////////////////////////////////////////////// */
/* */
/*    INTEL Corporation Proprietary Information */
/* */
/*    This listing is supplied under the terms of a license */
/*    agreement with INTEL Corporation and may not be copied */
/*    nor disclosed except in accordance with the terms of */
/*    that agreement. */
/* */
/*    Copyright (c) 1997-1998 Intel Corporation. */
/*    All Rights Reserved. */
/* */
/*////////////////////////////////////////////////////////// */
/* $Header: /icdev/cvsroot/jz4760/mplayer.10rc2/librv9/common/realvdo.h,v 1.3 2010/07/23 09:38:22 dqliu Exp $ */
/*////////////////////////////////////////////////////////// */

#ifndef REALVDO_H__
#define REALVDO_H__

/* The file defines the base RealVideo class. */

#include "hlxclib/stdio.h"
#include "rvtypes.h"
#include "rvstruct.h"
#include "rvcpu.h"
#include "rvdebug.h"
#include "rvstatus.h" 
#include "rvfid.h"
#include "rvmsg.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
extern FILE* g_debug_fp;

extern RV_Trace_Level g_debug_trace_level;

extern char* g_debug_module_name;
#endif


extern U32          g_number_of_loads;
            /* This counter allows Load() and Unload() to perform */
            /* initialization only once, and cleanup only after all */
            /* loaded instances have terminated. */

/* The RealVideo class represents an arbitrary host-independent video */
/* environment.  This is the view of the world from the underlying codec */
/* looking up.  Its implementation mediates communication between a */
/* specific video environment and the codec's RV interface. */
/* Typically only one RealVideo object is allocated, and is used by all */
/* codec instances. */

/* */
/* Define the information that gets passed into, and returned from, */
/* each call to decompress a frame. */
/* */

typedef U32     RV_Decoder_Decompression_Flags;
    /* This set of flags gets passed into each Decode() invocation. */

#define     RV_DDF_DONT_DRAW         0x1
    /* This flag indicates that the input image should be decoded but */
    /* not displayed.  It is enabled when the video environment sees */
    /* various indications such as preroll requests or hurry up requests. */

#define     RV_DDF_KEY_FRAME         0x2
    /* This flag is set if the video environment has signalled that */
    /* the frame is a key frame. */

#define     RV_DDF_MORE_FRAMES       0x4
    /* This flag indicates that the RV_Decoder's Decode method is being */
    /* called to retrieve the second (or subsequent) output image */
    /* corresponding to a single input image. */

#define     RV_DDF_LAST_FRAME        0x200

typedef U32     RV_Decoder_Decompression_Notes;
    /* This set of notes gets returned from each Decode() invocation. */

#define     RV_DDN_MORE_FRAMES       0x1
    /* Indicates that the decoder has more frames to emit.  This note is */
    /* used by a decoder to inform the RealVideo layer that multiple output */
    /* frames are to be emitted for a given input frame.  The RealVideo layer */
    /* will loop, repeatedly calling Decode() with the same input image and */
    /* with the RV_DDF_MORE_FRAMES flag set, until this note is no longer set. */
    /* This note should only be returned when operating in video environments */
    /* that support multiple output frames per input frame. */

#define     RV_DDN_DONT_DRAW         0x2
    /* This note is returned by a decoder to indicate that the output */
    /* image should not be passed back to the environment.  It should */
    /* be set by a decoder when the RV_DDF_DONT_DRAW flag is set, or */
    /* in situations where the decoder has nothing to display. */

#define     RV_DDN_KEY_FRAME         0x4
    /* Indicates that the decompressed image is a key frame. */
    /* Note that enhancement layer EI frames are not key frames, in the */
    /* traditional sense, because they have dependencies on lower layer */
    /* frames. */

#define     RV_DDN_B_FRAME           0x8
    /* Indicates that the decompressed image is a B frame. */
    /* At most one of RV_DDN_KEY_FRAME and RV_DDN_B_FRAME will be set. */

#define		RV_DDN_DEBLOCKING_FILTER	 0x10
	/* Indicates that the returned frame has gone through the */
	/* deblocking filter. */

#define     RV_DDN_FRU_FRAME            0x20
    /* Indicates that the decompressed image is a B frame. */
    /* At most one of RV_DDN_KEY_FRAME and RV_DDN_B_FRAME will be set. */




#define                 RV_SIGNATURE    0xAEBFC0D1


struct RVDecoder
{

        RV_CPU_Information         m_cpu;
            /* m_cpu describes the CPU that we are currently executing on. */
            /* This is initialized during RealVideo's constructor, and never */
            /* updated thereafter. */


        U32             m_signature;
            /* m_signature is used to guard against a bogus RealVideo */
            /* pointer.  It is initialized with a magic value during */
            /* construction, and cleared by the destructor. */

        RV_Boolean                 m_is_big_endian;
            /* Indicates whether the current system has little */
            /* or big-endian memory. */

        F64                         m_picture_clock_frequency;
            /* The default is 29.97 frames per second.  The underlying */
            /* codec can reassign this if needed. */

		struct RV_Image_Format		m_in_format;
		struct RV_Image_Format		m_out_format;
			/* input and output formats */

        void    *m_coredecoder;
            /* m_coredecoder is never NULL.  It is instantiated by our */
            /* constructor, and deallocated by our destructor. */
};

/* Construct the specific Decoder object. */
void * Decoder_CreateDecoder(RV_FID fid, struct RVDecoder* rvdecoder, RV_Status *status);

/* Static functions declared in RV_Decoder class. */
RV_Status  Decoder_Startup();

RV_Status 
Decoder_Shutdown();


RV_Status      Decoder_Load(void);
            /* Performs one-time load-time initialization. */

void            Decoder_Delete(void *coredecoder);



RV_Status      Decoder_Custom_Message(void *t, RV_Custom_Message_ID*);

RV_Status      Decoder_Decode
                        (
			 void *coredecoder,
			 const struct RV_Image                       *src,
			 struct RV_Image                       *dst,
			 const RV_Decoder_Decompression_Flags  flags,
			 RV_Decoder_Decompression_Notes *notes,
			 I32                             temporal_offset,
             I32      drop_frame
			 );
  
RV_Status      Decoder_Start_Sequence(void *coredecoder,
							   const struct RV_Image_Format*,
                                       const struct RV_Image_Format*);


RV_Status      Decoder_Transfer_State(void *dest, void *source);
	/* Transfer decoder status from one core decoder to another */
	/* after transfer, the new decoder can decode any sub-sequent
	   P and I frame, but will skip B frames until another P or I
	   frame has been decoded */

RV_Status
Decoder_Change_Output_Format(void *coredecoder,
							 const struct RV_Image_Format *dst_fmt);





void RVDecoder_Init(struct RVDecoder *rvdecoder, RV_FID fid, RV_Status *ps);
void RVDecoder_Delete(struct RVDecoder *rvdecoder);

RV_Boolean RVDecoder_is_active(struct RVDecoder *rvdecoder);
            /* is_active simply tests that a given RealVideo pointer is valid. */

        /* Use static (not virtual) methods for most operating system services. */
        /* Many services must be available before a RealVideo object is */
        /* instantiated, so non-static methods will not work for them. */

RV_Boolean    RVDecoder_Load();
RV_Boolean    RVDecoder_Unload();
            /* Load and Unload are called by the environment-specific RealVideo */
            /* to perform one-time initialization and cleanup, */
            /* "one-time" in the sense of loading an application or DLL. */
            /* These implementations call the underlying RV_Encoder and */
            /* RV_Decoder's Startup and Shutdown methods.  Reference */
            /* counting is performed here, so that the RV_Encoder and */
            /* RV_Decoder's implementations are called only once. */


#ifdef __cplusplus
}
#endif


#endif /* REALVDO_H__ */
