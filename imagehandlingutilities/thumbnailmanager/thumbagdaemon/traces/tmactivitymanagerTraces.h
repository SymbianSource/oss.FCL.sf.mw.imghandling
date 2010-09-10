// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __TMACTIVITYMANAGERTRACES_H__
#define __TMACTIVITYMANAGERTRACES_H__

#define KOstTraceComponentID 0x2001fd51

#define CTMACTIVITYMANAGER_CTMACTIVITYMANAGER 0x8600cd
#define DUP1_CTMACTIVITYMANAGER_CTMACTIVITYMANAGER 0x8600ce
#define CTMACTIVITYMANAGER_CONSTRUCTL 0x8600cf
#define CTMACTIVITYMANAGER_SETTIMEOUT 0x8600d0
#define CTMACTIVITYMANAGER_RESET 0x8600d1
#define CTMACTIVITYMANAGER_DOCANCEL 0x8600d2
#define CTMACTIVITYMANAGER_START 0x8600d3
#define CTMACTIVITYMANAGER_RUNL 0x8600d4
#define DUP5_CTMACTIVITYMANAGER_RUNL 0x8600d5
#define DUP1_CTMACTIVITYMANAGER_RUNL 0x8600d6
#define DUP2_CTMACTIVITYMANAGER_RUNL 0x8600d7
#define DUP3_CTMACTIVITYMANAGER_RUNL 0x8600d8
#define DUP4_CTMACTIVITYMANAGER_RUNL 0x8600d9
#define CTMACTIVITYMANAGER_RUNERROR 0x8600da
#define DUP6_CTMACTIVITYMANAGER_RUNL 0x8600db
#define DUP7_CTMACTIVITYMANAGER_RUNL 0x8600dc
#define CTMACTIVITYMANAGER_ISINACTIVE 0x8600dd
#define DUP1_CTMACTIVITYMANAGER_ISINACTIVE 0x8600de
#define CTMACTIVITYMANAGER_LIGHTSTATUSCHANGED 0x8600df
#define DUP1_CTMACTIVITYMANAGER_LIGHTSTATUSCHANGED 0x8600e0
#define DUP2_CTMACTIVITYMANAGER_LIGHTSTATUSCHANGED 0x8600e1
#define CTMACTIVITYMANAGER_NOTIFYOBSERVER 0x8600e2
#define CTMACTIVITYMANAGER_NEWL 0x8600f3


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

