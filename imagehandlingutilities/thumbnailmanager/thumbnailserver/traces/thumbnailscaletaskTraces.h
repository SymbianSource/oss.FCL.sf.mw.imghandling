/*
* Copyright (c) 2006-2007 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  Task for scaling thumbnails.
 *
*/
// Created by TraceCompiler 2.3.0
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILSCALETASKTRACES_H__
#define __THUMBNAILSCALETASKTRACES_H__

#define KOstTraceComponentID 0x102830ab

#define CTHUMBNAILSCALETASK_CTHUMBNAILSCALETASK 0x86004a
#define DUP1_CTHUMBNAILSCALETASK_CTHUMBNAILSCALETASK 0x86004b
#define CTHUMBNAILSCALETASK_STARTL 0x86004c
#define DUP1_CTHUMBNAILSCALETASK_STARTL 0x86004d
#define DUP2_CTHUMBNAILSCALETASK_STARTL 0x86004e
#define DUP3_CTHUMBNAILSCALETASK_STARTL 0x86004f
#define DUP4_CTHUMBNAILSCALETASK_STARTL 0x860050
#define DUP5_CTHUMBNAILSCALETASK_STARTL 0x860051
#define DUP6_CTHUMBNAILSCALETASK_STARTL 0x860052
#define CTHUMBNAILSCALETASK_RUNL 0x860053
#define DUP1_CTHUMBNAILSCALETASK_RUNL 0x860054
#define DUP2_CTHUMBNAILSCALETASK_RUNL 0x860055
#define CTHUMBNAILSCALETASK_DOCANCEL 0x860056
#define CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x860057
#define DUP1_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x860058
#define DUP2_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x860059
#define DUP3_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x86005a
#define DUP4_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x86005b
#define DUP5_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x86005c
#define DUP6_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x86005d
#define DUP7_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x86005e
#define DUP8_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x86005f
#define DUP9_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x860060
#define CTHUMBNAILSCALETASK_ENCODEL 0x8601e3
#define DUP1_CTHUMBNAILSCALETASK_ENCODEL 0x8601e4
#define DUP2_CTHUMBNAILSCALETASK_ENCODEL 0x8601e5
#define DUP3_CTHUMBNAILSCALETASK_ENCODEL 0x8601e6
#define DUP4_CTHUMBNAILSCALETASK_ENCODEL 0x8601e7
#define DUP5_CTHUMBNAILSCALETASK_ENCODEL 0x8601e8
#define DUP6_CTHUMBNAILSCALETASK_ENCODEL 0x8601e9
#define DUP7_CTHUMBNAILSCALETASK_ENCODEL 0x8601ea
#define DUP8_CTHUMBNAILSCALETASK_ENCODEL 0x8601eb
#define DUP10_CTHUMBNAILSCALETASK_STOREANDCOMPLETEL 0x8601ec


#ifndef __KERNEL_MODE__
#ifndef __OSTTRACEGEN1_TUINT32_CONST_TDESC16REF__
#define __OSTTRACEGEN1_TUINT32_CONST_TDESC16REF__

inline TBool OstTraceGen1( TUint32 aTraceID, const TDesC16& aParam1 )
    {
    TBool retval;
    TInt size = aParam1.Size();
    // BTrace assumes that parameter size is atleast 4 bytes
    if (size % 4 == 0)
        {
        TUint8* ptr = ( TUint8* )aParam1.Ptr();
        // Data is written directly and length is determined from trace message length
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, size );
        }
    else
        {
        TUint8 data[ KOstMaxDataLength ];
        TUint8* ptr = data;
        if (size > KOstMaxDataLength)
            {
            size = KOstMaxDataLength;
            }
        TInt sizeAligned = ( size + 3 ) & ~3;
        memcpy( ptr, aParam1.Ptr(), size );
        ptr += size;
        // Fillers are written to get 32-bit alignment
        while ( size++ < sizeAligned )
            {
            *ptr++ = 0;
            }
        ptr -= sizeAligned;
        size = sizeAligned;
        // Data is written directly and length is determined from trace message length
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, size );
        }
    return retval;
    }

#endif // __OSTTRACEGEN1_TUINT32_CONST_TDESC16REF__

#endif


#endif

// End of file

