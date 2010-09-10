// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILIMAGEDECODERTRACES_H__
#define __THUMBNAILIMAGEDECODERTRACES_H__

#define KOstTraceComponentID 0x102830ac

#define CTHUMBNAILIMAGEDECODER_CREATEL 0x860001
#define DUP1_CTHUMBNAILIMAGEDECODER_CREATEL 0x860002
#define DUP2_CTHUMBNAILIMAGEDECODER_CREATEL 0x860003
#define DUP3_CTHUMBNAILIMAGEDECODER_CREATEL 0x860004
#define CTHUMBNAILIMAGEDECODER_DECODEL 0x860005
#define DUP1_CTHUMBNAILIMAGEDECODER_DECODEL 0x860006
#define DUP2_CTHUMBNAILIMAGEDECODER_DECODEL 0x860007
#define DUP3_CTHUMBNAILIMAGEDECODER_DECODEL 0x860008
#define DUP4_CTHUMBNAILIMAGEDECODER_DECODEL 0x860009
#define DUP5_CTHUMBNAILIMAGEDECODER_DECODEL 0x86000a
#define DUP6_CTHUMBNAILIMAGEDECODER_DECODEL 0x86000b
#define DUP7_CTHUMBNAILIMAGEDECODER_DECODEL 0x86000c
#define DUP8_CTHUMBNAILIMAGEDECODER_DECODEL 0x86000d
#define CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x86000e
#define DUP1_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x86000f
#define DUP2_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860010
#define DUP3_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860011
#define DUP4_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860012
#define DUP5_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860013
#define DUP6_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860014
#define DUP7_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860015
#define DUP8_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860016
#define DUP9_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860017
#define DUP10_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860018
#define DUP11_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860019
#define DUP12_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x86001a
#define DUP13_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x86001b
#define DUP14_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x86001c
#define DUP15_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x86001d
#define DUP16_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x86001e
#define DUP17_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x86001f
#define DUP18_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860020
#define DUP19_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860021
#define DUP20_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860022
#define DUP21_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860023
#define DUP22_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860024
#define DUP23_CTHUMBNAILIMAGEDECODER_CREATEDECODERL 0x860025
#define CTHUMBNAILIMAGEDECODER_CREATEEXIFDECODERL 0x860026
#define DUP1_CTHUMBNAILIMAGEDECODER_CREATEEXIFDECODERL 0x860027


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

