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
* Description:  Server side session for Thumbnail Manager Server
 *
*/
// Created by TraceCompiler 2.3.0
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILSERVERSESSIONTRACES_H__
#define __THUMBNAILSERVERSESSIONTRACES_H__

#define KOstTraceComponentID 0x102830ab

#define CTHUMBNAILSERVERSESSION_CREATEL 0x8600b0
#define CTHUMBNAILSERVERSESSION_SERVICEL 0x8600b1
#define DUP1_CTHUMBNAILSERVERSESSION_SERVICEL 0x8600b2
#define DUP2_CTHUMBNAILSERVERSESSION_SERVICEL 0x8600b3
#define DUP3_CTHUMBNAILSERVERSESSION_SERVICEL 0x8600b4
#define CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL 0x8600b5
#define DUP1_CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL 0x8600b6
#define DUP2_CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL 0x8600b7
#define DUP3_CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL 0x8600b8
#define DUP4_CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL 0x8600b9
#define CTHUMBNAILSERVERSESSION_RENAMETHUMBNAILSL 0x8600ba
#define CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYIDASYNCL 0x8600bb
#define DUP1_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYIDASYNCL 0x8600bc
#define DUP2_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYIDASYNCL 0x8600bd
#define CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600be
#define DUP1_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600bf
#define DUP2_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600c0
#define DUP4_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600c1
#define DUP3_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600c2
#define DUP5_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600c3
#define DUP6_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600c4
#define DUP7_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600c5
#define DUP8_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600c6
#define DUP9_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8600c7
#define CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8600c8
#define DUP1_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8600c9
#define DUP2_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8600ca
#define DUP3_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8600cb
#define DUP4_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8600cc
#define DUP5_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8600cd
#define DUP6_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8600ce
#define CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBUFFERL 0x8600cf
#define DUP1_CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBUFFERL 0x8600d0
#define CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBITMAPL 0x8600d1
#define DUP1_CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBITMAPL 0x8600d2
#define DUP2_CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBITMAPL 0x8600d3
#define CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMFILEHANDLEL 0x8600d4
#define DUP1_CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMFILEHANDLEL 0x8600d5
#define CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMBUFFERL 0x8600d6
#define DUP1_CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMBUFFERL 0x8600d7
#define DUP2_CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMBUFFERL 0x8600d8
#define DUP3_CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMBUFFERL 0x8600d9
#define CTHUMBNAILSERVERSESSION_FETCHTHUMBNAILL 0x8600da
#define DUP1_CTHUMBNAILSERVERSESSION_FETCHTHUMBNAILL 0x8600db
#define DUP2_CTHUMBNAILSERVERSESSION_FETCHTHUMBNAILL 0x8600dc
#define CTHUMBNAILSERVERSESSION_PROCESSBITMAPL 0x8600dd
#define DUP1_CTHUMBNAILSERVERSESSION_PROCESSBITMAPL 0x8600de
#define CTHUMBNAILSERVERSESSION_RELEASEBITMAP 0x8600df
#define CTHUMBNAILSERVERSESSION_CANCELREQUEST 0x8600e0
#define DUP1_CTHUMBNAILSERVERSESSION_CANCELREQUEST 0x8600e1
#define DUP2_CTHUMBNAILSERVERSESSION_CANCELREQUEST 0x8600e2
#define CTHUMBNAILSERVERSESSION_CHANGEPRIORITY 0x8600e3
#define DUP1_CTHUMBNAILSERVERSESSION_CHANGEPRIORITY 0x8600e4
#define DUP2_CTHUMBNAILSERVERSESSION_CHANGEPRIORITY 0x8600e5
#define CTHUMBNAILSERVERSESSION_DELETETHUMBNAILSL 0x8600e6
#define CTHUMBNAILSERVERSESSION_DELETETHUMBNAILSBYIDL 0x8600e7
#define CTHUMBNAILSERVERSESSION_GETMIMETYPELISTL 0x8600e8
#define CTHUMBNAILSERVERSESSION_RESOLVEMIMETYPEL 0x8600e9
#define DUP1_CTHUMBNAILSERVERSESSION_RESOLVEMIMETYPEL 0x8600ea
#define CTHUMBNAILSERVERSESSION_CONVERTSQLERRTOE32ERR 0x8600eb
#define CTHUMBNAILSERVERSESSION_CLIENTTHREADALIVE 0x8600ec
#define DUP1_CTHUMBNAILSERVERSESSION_CLIENTTHREADALIVE 0x8600ed
#define DUP2_CTHUMBNAILSERVERSESSION_CLIENTTHREADALIVE 0x8600ee
#define DUP10_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8601ed
#define DUP11_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8601ee
#define DUP12_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL 0x8601ef
#define DUP7_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8601f0
#define DUP8_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8601f1
#define DUP9_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL 0x8601f2
#define CTHUMBNAILSERVERSESSION_SETJPEGBUFFERL 0x8601f3
#define CTHUMBNAILSERVERSESSION_GETJPEGBUFFERL 0x8601f4


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


#ifndef __OSTTRACEGEN3_TUINT32_TINT_TINT_TUINT__
#define __OSTTRACEGEN3_TUINT32_TINT_TINT_TUINT__

inline TBool OstTraceGen3( TUint32 aTraceID, TInt aParam1, TInt aParam2, TUint aParam3 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 12 ];
        TUint8* ptr = data;
        *( ( TInt* )ptr ) = aParam1;
        ptr += sizeof ( TInt );
        *( ( TInt* )ptr ) = aParam2;
        ptr += sizeof ( TInt );
        *( ( TUint* )ptr ) = aParam3;
        ptr += sizeof ( TUint );
        ptr -= 12;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 12 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN3_TUINT32_TINT_TINT_TUINT__


#ifndef __OSTTRACEGEN3_TUINT32_TINT32_TINT32_TUINT32__
#define __OSTTRACEGEN3_TUINT32_TINT32_TINT32_TUINT32__

inline TBool OstTraceGen3( TUint32 aTraceID, TInt32 aParam1, TInt32 aParam2, TUint32 aParam3 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 12 ];
        TUint8* ptr = data;
        *( ( TInt* )ptr ) = aParam1;
        ptr += sizeof ( TInt );
        *( ( TInt* )ptr ) = aParam2;
        ptr += sizeof ( TInt );
        *( ( TUint* )ptr ) = aParam3;
        ptr += sizeof ( TUint );
        ptr -= 12;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 12 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN3_TUINT32_TINT32_TINT32_TUINT32__



#ifndef __OSTTRACEGEN2_TUINT32_TUINT_TINT__
#define __OSTTRACEGEN2_TUINT32_TUINT_TINT__

inline TBool OstTraceGen2( TUint32 aTraceID, TUint aParam1, TInt aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TUint* )ptr ) = aParam1;
        ptr += sizeof ( TUint );
        *( ( TInt* )ptr ) = aParam2;
        ptr += sizeof ( TInt );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TUINT_TINT__


#ifndef __OSTTRACEGEN2_TUINT32_TUINT32_TINT32__
#define __OSTTRACEGEN2_TUINT32_TUINT32_TINT32__

inline TBool OstTraceGen2( TUint32 aTraceID, TUint32 aParam1, TInt32 aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TUint* )ptr ) = aParam1;
        ptr += sizeof ( TUint );
        *( ( TInt* )ptr ) = aParam2;
        ptr += sizeof ( TInt );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TUINT32_TINT32__



#endif

// End of file

