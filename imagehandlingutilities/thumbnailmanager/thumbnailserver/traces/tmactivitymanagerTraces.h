// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __TMACTIVITYMANAGERTRACES_H__
#define __TMACTIVITYMANAGERTRACES_H__

#define KOstTraceComponentID 0x102830ab

#define CTMACTIVITYMANAGER_CTMACTIVITYMANAGER 0x8601c0
#define DUP1_CTMACTIVITYMANAGER_CTMACTIVITYMANAGER 0x8601c1
#define CTMACTIVITYMANAGER_CONSTRUCTL 0x8601c2
#define CTMACTIVITYMANAGER_SETTIMEOUT 0x8601c3
#define CTMACTIVITYMANAGER_RESET 0x8601c4
#define CTMACTIVITYMANAGER_DOCANCEL 0x8601c5
#define CTMACTIVITYMANAGER_START 0x8601c6
#define CTMACTIVITYMANAGER_RUNL 0x8601c7
#define DUP5_CTMACTIVITYMANAGER_RUNL 0x8601c8
#define DUP1_CTMACTIVITYMANAGER_RUNL 0x8601c9
#define DUP2_CTMACTIVITYMANAGER_RUNL 0x8601ca
#define DUP3_CTMACTIVITYMANAGER_RUNL 0x8601cb
#define DUP4_CTMACTIVITYMANAGER_RUNL 0x8601cc
#define CTMACTIVITYMANAGER_RUNERROR 0x8601cd
#define DUP6_CTMACTIVITYMANAGER_RUNL 0x8601ce
#define DUP7_CTMACTIVITYMANAGER_RUNL 0x8601cf
#define CTMACTIVITYMANAGER_ISINACTIVE 0x8601d0
#define DUP1_CTMACTIVITYMANAGER_ISINACTIVE 0x8601d1
#define CTMACTIVITYMANAGER_LIGHTSTATUSCHANGED 0x8601d2
#define DUP1_CTMACTIVITYMANAGER_LIGHTSTATUSCHANGED 0x8601d3
#define DUP2_CTMACTIVITYMANAGER_LIGHTSTATUSCHANGED 0x8601d4
#define CTMACTIVITYMANAGER_NOTIFYOBSERVER 0x8601d5
#define CTMACTIVITYMANAGER_NEWL 0x8601e2


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

