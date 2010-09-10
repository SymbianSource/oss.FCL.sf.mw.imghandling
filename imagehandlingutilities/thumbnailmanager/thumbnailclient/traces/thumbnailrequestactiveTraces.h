// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILREQUESTACTIVETRACES_H__
#define __THUMBNAILREQUESTACTIVETRACES_H__

#define KOstTraceComponentID 0x102830aa

#define CTHUMBNAILREQUESTACTIVE_CTHUMBNAILREQUESTACTIVE 0x860021
#define CTHUMBNAILREQUESTACTIVE_STARTL 0x860022
#define DUP1_CTHUMBNAILREQUESTACTIVE_STARTL 0x860023
#define DUP2_CTHUMBNAILREQUESTACTIVE_STARTL 0x860024
#define DUP3_CTHUMBNAILREQUESTACTIVE_STARTL 0x860025
#define DUP4_CTHUMBNAILREQUESTACTIVE_STARTL 0x860026
#define DUP5_CTHUMBNAILREQUESTACTIVE_STARTL 0x860027
#define CTHUMBNAILREQUESTACTIVE_RUNL 0x860028
#define DUP1_CTHUMBNAILREQUESTACTIVE_RUNL 0x860029
#define DUP2_CTHUMBNAILREQUESTACTIVE_RUNL 0x86002a
#define DUP3_CTHUMBNAILREQUESTACTIVE_RUNL 0x86002b
#define DUP4_CTHUMBNAILREQUESTACTIVE_RUNL 0x86002c
#define DUP5_CTHUMBNAILREQUESTACTIVE_RUNL 0x86002d
#define DUP6_CTHUMBNAILREQUESTACTIVE_RUNL 0x86002e
#define DUP7_CTHUMBNAILREQUESTACTIVE_RUNL 0x86002f
#define DUP8_CTHUMBNAILREQUESTACTIVE_RUNL 0x860030
#define DUP9_CTHUMBNAILREQUESTACTIVE_RUNL 0x860031
#define DUP10_CTHUMBNAILREQUESTACTIVE_RUNL 0x860032
#define DUP11_CTHUMBNAILREQUESTACTIVE_RUNL 0x860033
#define DUP12_CTHUMBNAILREQUESTACTIVE_RUNL 0x860034
#define DUP13_CTHUMBNAILREQUESTACTIVE_RUNL 0x860035
#define DUP14_CTHUMBNAILREQUESTACTIVE_RUNL 0x860036
#define DUP15_CTHUMBNAILREQUESTACTIVE_RUNL 0x860037
#define DUP16_CTHUMBNAILREQUESTACTIVE_RUNL 0x860038
#define DUP17_CTHUMBNAILREQUESTACTIVE_RUNL 0x860039
#define CTHUMBNAILREQUESTACTIVE_RUNERROR 0x86003a
#define CTHUMBNAILREQUESTACTIVE_DOCANCEL 0x86003b
#define CTHUMBNAILREQUESTACTIVE_ASYNCCANCEL 0x86003c
#define CTHUMBNAILREQUESTACTIVE_RELEASESERVERBITMAP 0x86003d
#define CTHUMBNAILREQUESTACTIVE_HANDLEERROR 0x86003e
#define DUP1_CTHUMBNAILREQUESTACTIVE_HANDLEERROR 0x86003f
#define DUP2_CTHUMBNAILREQUESTACTIVE_HANDLEERROR 0x860040
#define DUP3_CTHUMBNAILREQUESTACTIVE_HANDLEERROR 0x860041
#define DUP4_CTHUMBNAILREQUESTACTIVE_HANDLEERROR 0x860042
#define DUP5_CTHUMBNAILREQUESTACTIVE_HANDLEERROR 0x860043
#define CTHUMBNAILREQUESTACTIVE_SETTHUMBNAILL 0x860044
#define CTHUMBNAILREQUESTACTIVE_GET2NDPHASETHUMBNAILL 0x860045
#define CTHUMBNAILREQUESTACTIVE_CHANGEPRIORITY 0x860046
#define CTHUMBNAILREQUESTACTIVE_STARTERROR 0x860047
#define CTHUMBNAILREQUESTACTIVE_TIMERCALLBACK 0x860048
#define DUP1_CTHUMBNAILREQUESTACTIVE_TIMERCALLBACK 0x860049
#define CTHUMBNAILREQUESTACTIVE_ISVIRTUALURI 0x86004a


#ifndef __OSTTRACEGEN2_TUINT32_TINT_TINT__
#define __OSTTRACEGEN2_TUINT32_TINT_TINT__

inline TBool OstTraceGen2( TUint32 aTraceID, TInt aParam1, TInt aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TInt* )ptr ) = aParam1;
        ptr += sizeof ( TInt );
        *( ( TInt* )ptr ) = aParam2;
        ptr += sizeof ( TInt );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TINT_TINT__


#ifndef __OSTTRACEGEN2_TUINT32_TINT32_TINT32__
#define __OSTTRACEGEN2_TUINT32_TINT32_TINT32__

inline TBool OstTraceGen2( TUint32 aTraceID, TInt32 aParam1, TInt32 aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TInt* )ptr ) = aParam1;
        ptr += sizeof ( TInt );
        *( ( TInt* )ptr ) = aParam2;
        ptr += sizeof ( TInt );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TINT32_TINT32__



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

