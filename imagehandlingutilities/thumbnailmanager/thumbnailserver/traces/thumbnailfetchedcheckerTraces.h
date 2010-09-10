// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILFETCHEDCHECKERTRACES_H__
#define __THUMBNAILFETCHEDCHECKERTRACES_H__

#define KOstTraceComponentID 0x102830ab

#define CTHUMBNAILFETCHEDCHECKER_LASTFETCHRESULT 0x86000f
#define DUP1_CTHUMBNAILFETCHEDCHECKER_LASTFETCHRESULT 0x860010
#define CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT 0x860011
#define DUP1_CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT 0x860012
#define DUP4_CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT 0x860013
#define DUP2_CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT 0x860014
#define DUP3_CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT 0x860015
#define CTHUMBNAILFETCHEDCHECKER_DELETEFETCHRESULT 0x860016
#define DUP1_CTHUMBNAILFETCHEDCHECKER_DELETEFETCHRESULT 0x860017
#define CTHUMBNAILFETCHEDCHECKER_RENAMEFETCHRESULTL 0x860018
#define DUP1_CTHUMBNAILFETCHEDCHECKER_RENAMEFETCHRESULTL 0x860019
#define CTHUMBNAILFETCHEDCHECKER_RESET 0x86001a
#define CENTRY_NEWL 0x86001b
#define CENTRY_FINDCB 0x86001c
#define CENTRY_FINDCBURI 0x86001d
#define CENTRY_INSERTCB 0x86001e


#ifndef __KERNEL_MODE__
#ifndef __OSTTRACEGEN2_TUINT32_CONST_TDESC16REF_TUINT16__
#define __OSTTRACEGEN2_TUINT32_CONST_TDESC16REF_TUINT16__

inline TBool OstTraceGen2( TUint32 aTraceID, const TDesC16& aParam1, TUint16 aParam2 )
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
        if ((length + sizeof ( TUint16 )) <= KOstMaxDataLength)
            {
            *( ( TUint16* )ptr ) = aParam2;
            ptr += sizeof ( TUint16 );
            length += sizeof ( TUint16 );
            }
        *( ( TUint8* )ptr ) = 0;
        ptr += sizeof ( TUint8 );
        *( ( TUint8* )ptr ) = 0;
        ptr += sizeof ( TUint8 );
        ptr -= length;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, length );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_CONST_TDESC16REF_TUINT16__

#endif


#ifndef __KERNEL_MODE__
#ifndef __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TUINT_TINT__
#define __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TUINT_TINT__

inline TBool OstTraceGen3( TUint32 aTraceID, const TDesC16& aParam1, TUint aParam2, TInt aParam3 )
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
        if ((length + sizeof ( TUint )) <= KOstMaxDataLength)
            {
            *( ( TUint* )ptr ) = aParam2;
            ptr += sizeof ( TUint );
            length += sizeof ( TUint );
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

#endif // __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TUINT_TINT__


#ifndef __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TUINT32_TINT32__
#define __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TUINT32_TINT32__

inline TBool OstTraceGen3( TUint32 aTraceID, const TDesC16& aParam1, TUint32 aParam2, TInt32 aParam3 )
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
        if ((length + sizeof ( TUint )) <= KOstMaxDataLength)
            {
            *( ( TUint* )ptr ) = aParam2;
            ptr += sizeof ( TUint );
            length += sizeof ( TUint );
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

#endif // __OSTTRACEGEN3_TUINT32_CONST_TDESC16REF_TUINT32_TINT32__

#endif


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


#ifndef __KERNEL_MODE__
#ifndef __OSTTRACEGEN2_TUINT32_CONST_TDESC16REF_CONST_TDESC16REF__
#define __OSTTRACEGEN2_TUINT32_CONST_TDESC16REF_CONST_TDESC16REF__

inline TBool OstTraceGen2( TUint32 aTraceID, const TDesC16& aParam1, const TDesC16& aParam2 )
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
        // Check that parameter lenght is not too long
        TInt length2 = aParam2.Size();
        if ((length + length2 + sizeof ( TUint32 )) > KOstMaxDataLength)
            {
            length2 = KOstMaxDataLength - (length + sizeof ( TUint32 ));
            }
        TInt lengthAligned2 = ( length2 + 3 ) & ~3;
        if (lengthAligned2 > 0)
            {
            length = length + sizeof ( TUint32 ) + lengthAligned2;
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
        if (length2 > 0)
            {
            // Number of elements is written before data
            // In case of Unicode string, number of elements is half of length
            *( ( TUint32* )ptr ) = length2 / (aParam2.Size() / aParam2.Length());
            ptr += sizeof ( TUint32 );
            memcpy( ptr, aParam2.Ptr(), length2 );
            ptr += length2;
            // Fillers are written to get 32-bit alignment
            while ( length2++ < lengthAligned2 )
                {
                *ptr++ = 0;
                }
            length += sizeof ( TUint32 ) + lengthAligned2;
            }
        else if (length + sizeof ( TUint32 ) <= KOstMaxDataLength)
            {
            *( ( TUint32* )ptr ) = 0;
            ptr += sizeof ( TUint32 );
            length += sizeof ( TUint32 );
            }
        ptr -= length;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, length );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_CONST_TDESC16REF_CONST_TDESC16REF__

#endif


#endif

// End of file

