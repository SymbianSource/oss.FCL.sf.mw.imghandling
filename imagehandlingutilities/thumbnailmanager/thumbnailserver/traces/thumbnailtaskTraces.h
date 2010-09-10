// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILTASKTRACES_H__
#define __THUMBNAILTASKTRACES_H__

#define KOstTraceComponentID 0x102830ab

#define CTHUMBNAILTASK_STARTL 0x860194
#define DUP1_CTHUMBNAILTASK_STARTL 0x860195
#define CTHUMBNAILTASK_COMPLETE 0x860196
#define DUP1_CTHUMBNAILTASK_COMPLETE 0x860197
#define CTHUMBNAILTASK_STARTERROR 0x860198
#define DUP1_CTHUMBNAILTASK_STARTERROR 0x860199
#define CTHUMBNAILTASK_SETMESSAGEDATA 0x86019a
#define CTHUMBNAILTASK_CLIENTTHREADALIVE 0x86019b
#define DUP1_CTHUMBNAILTASK_CLIENTTHREADALIVE 0x86019c
#define DUP2_CTHUMBNAILTASK_CLIENTTHREADALIVE 0x86019d


#ifndef __OSTTRACEGEN2_TUINT32_TINT_TUINT__
#define __OSTTRACEGEN2_TUINT32_TINT_TUINT__

inline TBool OstTraceGen2( TUint32 aTraceID, TInt aParam1, TUint aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TInt* )ptr ) = aParam1;
        ptr += sizeof ( TInt );
        *( ( TUint* )ptr ) = aParam2;
        ptr += sizeof ( TUint );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TINT_TUINT__


#ifndef __OSTTRACEGEN2_TUINT32_TINT32_TUINT32__
#define __OSTTRACEGEN2_TUINT32_TINT32_TUINT32__

inline TBool OstTraceGen2( TUint32 aTraceID, TInt32 aParam1, TUint32 aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TInt* )ptr ) = aParam1;
        ptr += sizeof ( TInt );
        *( ( TUint* )ptr ) = aParam2;
        ptr += sizeof ( TUint );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TINT32_TUINT32__



#endif

// End of file

