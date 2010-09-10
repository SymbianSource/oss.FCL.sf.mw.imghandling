// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILMANAGERIMPLTRACES_H__
#define __THUMBNAILMANAGERIMPLTRACES_H__

#define KOstTraceComponentID 0x102830aa

#define CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL 0x860007
#define DUP1_CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL 0x860008
#define DUP2_CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL 0x860009
#define DUP3_CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL 0x86000a
#define DUP4_CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL 0x86000b
#define CTHUMBNAILMANAGERIMPL_CONSTRUCTL 0x86000c
#define DUP1_CTHUMBNAILMANAGERIMPL_CONSTRUCTL 0x86000d
#define DUP2_CTHUMBNAILMANAGERIMPL_CONSTRUCTL 0x86000e
#define DUP3_CTHUMBNAILMANAGERIMPL_CONSTRUCTL 0x86000f
#define DUP4_CTHUMBNAILMANAGERIMPL_CONSTRUCTL 0x860010
#define CTHUMBNAILMANAGERIMPL_GETTHUMBNAILL 0x860011
#define DUP1_CTHUMBNAILMANAGERIMPL_GETTHUMBNAILL 0x860012
#define DUP2_CTHUMBNAILMANAGERIMPL_GETTHUMBNAILL 0x860013
#define DUP3_CTHUMBNAILMANAGERIMPL_GETTHUMBNAILL 0x860014
#define CTHUMBNAILMANAGERIMPL_IMPORTTHUMBNAILL 0x860015
#define CTHUMBNAILMANAGERIMPL_SETTHUMBNAILL 0x860016
#define CTHUMBNAILMANAGERIMPL_CREATETHUMBNAILS 0x860017
#define DUP1_CTHUMBNAILMANAGERIMPL_CREATETHUMBNAILS 0x860018
#define CTHUMBNAILMANAGERIMPL_DELETETHUMBNAILS 0x860019
#define DUP1_CTHUMBNAILMANAGERIMPL_DELETETHUMBNAILS 0x86001a
#define CTHUMBNAILMANAGERIMPL_CANCELREQUEST 0x86001b
#define CTHUMBNAILMANAGERIMPL_CHANGEPRIORITY 0x86001c
#define CTHUMBNAILMANAGERIMPL_UPDATETHUMBNAILSL 0x86001d
#define CTHUMBNAILMANAGERIMPL_RENAMETHUMBNAILSL 0x86001e
#define CTHUMBNAILMANAGERIMPL_VALIDATEPRIORITY 0x86001f
#define DUP1_CTHUMBNAILMANAGERIMPL_VALIDATEPRIORITY 0x860020


#ifndef __KERNEL_MODE__
#ifndef __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TINT_TINT__
#define __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TINT_TINT__

inline TBool OstTraceGen3( TUint32 aTraceID, const TDesC16& aParam1, TInt aParam2, TInt aParam3 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TInt length = 0;
        // Check that parameter lenght is not too long
        TInt length1 = aParam1.Size();
        if ((length + length1 + sizeof ( TUint32 )) > KOstMaxDataLength)
            {
            length1 = KOstMaxDataLength - (length + sizeof ( TUint32 ));
            }
        TInt lengthAligned1 = ( length1 + 3 ) & ~3;
        if (lengthAligned1 > 0)
            {
            length = length + sizeof ( TUint32 ) + lengthAligned1;
            }
        TUint8 data[ KOstMaxDataLength ];
        TUint8* ptr = data;
        // Set length to zero and calculate it againg
        // when adding parameters
        length = 0;
        if (length1 > 0)
            {
            // Number of elements is written before data
            // In case of Unicode string, number of elements is half of length
            *( ( TUint32* )ptr ) = length1 / (aParam1.Size() / aParam1.Length());
            ptr += sizeof ( TUint32 );
            memcpy( ptr, aParam1.Ptr(), length1 );
            ptr += length1;
            // Fillers are written to get 32-bit alignment
            while ( length1++ < lengthAligned1 )
                {
                *ptr++ = 0;
                }
            length += sizeof ( TUint32 ) + lengthAligned1;
            }
        else if (length + sizeof ( TUint32 ) <= KOstMaxDataLength)
            {
            *( ( TUint32* )ptr ) = 0;
            ptr += sizeof ( TUint32 );
            length += sizeof ( TUint32 );
            }
        // Check that there are enough space to next parameter
        if ((length + sizeof ( TInt )) <= KOstMaxDataLength)
            {
            *( ( TInt* )ptr ) = aParam2;
            ptr += sizeof ( TInt );
            length += sizeof ( TInt );
            }
        // Check that there are enough space to next parameter
        if ((length + sizeof ( TInt )) <= KOstMaxDataLength)
            {
            *( ( TInt* )ptr ) = aParam3;
            ptr += sizeof ( TInt );
            length += sizeof ( TInt );
            }
        ptr -= length;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, length );
        }
    return retval;
    }

#endif // __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TINT_TINT__


#ifndef __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TINT32_TINT32__
#define __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TINT32_TINT32__

inline TBool OstTraceGen3( TUint32 aTraceID, const TDesC16& aParam1, TInt32 aParam2, TInt32 aParam3 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TInt length = 0;
        // Check that parameter lenght is not too long
        TInt length1 = aParam1.Size();
        if ((length + length1 + sizeof ( TUint32 )) > KOstMaxDataLength)
            {
            length1 = KOstMaxDataLength - (length + sizeof ( TUint32 ));
            }
        TInt lengthAligned1 = ( length1 + 3 ) & ~3;
        if (lengthAligned1 > 0)
            {
            length = length + sizeof ( TUint32 ) + lengthAligned1;
            }
        TUint8 data[ KOstMaxDataLength ];
        TUint8* ptr = data;
        // Set length to zero and calculate it againg
        // when adding parameters
        length = 0;
        if (length1 > 0)
            {
            // Number of elements is written before data
            // In case of Unicode string, number of elements is half of length
            *( ( TUint32* )ptr ) = length1 / (aParam1.Size() / aParam1.Length());
            ptr += sizeof ( TUint32 );
            memcpy( ptr, aParam1.Ptr(), length1 );
            ptr += length1;
            // Fillers are written to get 32-bit alignment
            while ( length1++ < lengthAligned1 )
                {
                *ptr++ = 0;
                }
            length += sizeof ( TUint32 ) + lengthAligned1;
            }
        else if (length + sizeof ( TUint32 ) <= KOstMaxDataLength)
            {
            *( ( TUint32* )ptr ) = 0;
            ptr += sizeof ( TUint32 );
            length += sizeof ( TUint32 );
            }
        // Check that there are enough space to next parameter
        if ((length + sizeof ( TInt )) <= KOstMaxDataLength)
            {
            *( ( TInt* )ptr ) = aParam2;
            ptr += sizeof ( TInt );
            length += sizeof ( TInt );
            }
        // Check that there are enough space to next parameter
        if ((length + sizeof ( TInt )) <= KOstMaxDataLength)
            {
            *( ( TInt* )ptr ) = aParam3;
            ptr += sizeof ( TInt );
            length += sizeof ( TInt );
            }
        ptr -= length;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, length );
        }
    return retval;
    }

#endif // __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TINT32_TINT32__

#endif


#ifndef __OSTTRACEGEN3_TUINT32_TUINT_TUINT_TUINT__
#define __OSTTRACEGEN3_TUINT32_TUINT_TUINT_TUINT__

inline TBool OstTraceGen3( TUint32 aTraceID, TUint aParam1, TUint aParam2, TUint aParam3 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 12 ];
        TUint8* ptr = data;
        *( ( TUint* )ptr ) = aParam1;
        ptr += sizeof ( TUint );
        *( ( TUint* )ptr ) = aParam2;
        ptr += sizeof ( TUint );
        *( ( TUint* )ptr ) = aParam3;
        ptr += sizeof ( TUint );
        ptr -= 12;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 12 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN3_TUINT32_TUINT_TUINT_TUINT__


#ifndef __OSTTRACEGEN3_TUINT32_TUINT32_TUINT32_TUINT32__
#define __OSTTRACEGEN3_TUINT32_TUINT32_TUINT32_TUINT32__

inline TBool OstTraceGen3( TUint32 aTraceID, TUint32 aParam1, TUint32 aParam2, TUint32 aParam3 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 12 ];
        TUint8* ptr = data;
        *( ( TUint* )ptr ) = aParam1;
        ptr += sizeof ( TUint );
        *( ( TUint* )ptr ) = aParam2;
        ptr += sizeof ( TUint );
        *( ( TUint* )ptr ) = aParam3;
        ptr += sizeof ( TUint );
        ptr -= 12;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 12 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN3_TUINT32_TUINT32_TUINT32_TUINT32__



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

