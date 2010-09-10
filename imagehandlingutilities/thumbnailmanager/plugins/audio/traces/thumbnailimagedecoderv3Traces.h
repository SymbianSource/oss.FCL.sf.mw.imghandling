// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILIMAGEDECODERV3TRACES_H__
#define __THUMBNAILIMAGEDECODERV3TRACES_H__

#define KOstTraceComponentID 0x20022d5d

#define CTHUMBNAILIMAGEDECODERV3_CREATEL 0x860005
#define CTHUMBNAILIMAGEDECODERV3_DECODEL 0x860006
#define DUP1_CTHUMBNAILIMAGEDECODERV3_DECODEL 0x860007
#define DUP2_CTHUMBNAILIMAGEDECODERV3_DECODEL 0x860008
#define CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL 0x860009
#define DUP1_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL 0x86000a
#define DUP2_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL 0x86000b
#define DUP3_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL 0x86000c
#define DUP4_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL 0x86000d
#define DUP5_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL 0x86000e


#ifndef __OSTTRACEGEN3_TUINT32_TINT_TINT_TINT__
#define __OSTTRACEGEN3_TUINT32_TINT_TINT_TINT__

inline TBool OstTraceGen3( TUint32 aTraceID, TInt aParam1, TInt aParam2, TInt aParam3 )
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
        *( ( TInt* )ptr ) = aParam3;
        ptr += sizeof ( TInt );
        ptr -= 12;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 12 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN3_TUINT32_TINT_TINT_TINT__


#ifndef __OSTTRACEGEN3_TUINT32_TINT32_TINT32_TINT32__
#define __OSTTRACEGEN3_TUINT32_TINT32_TINT32_TINT32__

inline TBool OstTraceGen3( TUint32 aTraceID, TInt32 aParam1, TInt32 aParam2, TInt32 aParam3 )
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
        *( ( TInt* )ptr ) = aParam3;
        ptr += sizeof ( TInt );
        ptr -= 12;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 12 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN3_TUINT32_TINT32_TINT32_TINT32__



#endif

// End of file

