// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBAGDAEMONTRACES_H__
#define __THUMBAGDAEMONTRACES_H__

#define KOstTraceComponentID 0x2001fd51

#define CTHUMBAGDAEMON_NEWLC 0x81003b
#define CTHUMBAGDAEMON_CONSTRUCTL 0x81003c
#define DUP1_CTHUMBAGDAEMON_CONSTRUCTL 0x81003d
#define CTHUMBAGDAEMON_INITIALIZEL 0x81003e
#define DUP1_CTHUMBAGDAEMON_INITIALIZEL 0x81003f
#define DUP2_CTHUMBAGDAEMON_INITIALIZEL 0x810040
#define CTHUMBAGDAEMON_CTHUMBAGDAEMON 0x810041
#define DUP1_CTHUMBAGDAEMON_CTHUMBAGDAEMON 0x810042
#define CTHUMBAGDAEMON_THREADFUNCTIONL 0x810043
#define DUP1_CTHUMBAGDAEMON_THREADFUNCTIONL 0x810044
#define CTHUMBAGDAEMON_HANDLESESSIONOPENED 0x810045
#define DUP1_CTHUMBAGDAEMON_HANDLESESSIONOPENED 0x810046
#define DUP2_CTHUMBAGDAEMON_HANDLESESSIONOPENED 0x810047
#define CTHUMBAGDAEMON_HANDLESESSIONERROR 0x810048
#define DUP1_CTHUMBAGDAEMON_HANDLESESSIONERROR 0x810049
#define CTHUMBAGDAEMON_HANDLEURIOBJECTNOTIFICATION 0x81004a
#define DUP1_CTHUMBAGDAEMON_HANDLEURIOBJECTNOTIFICATION 0x81004b
#define DUP2_CTHUMBAGDAEMON_HANDLEURIOBJECTNOTIFICATION 0x81004c
#define CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION 0x81004d
#define DUP1_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION 0x81004e
#define DUP2_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION 0x81004f
#define DUP3_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION 0x810050
#define DUP4_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION 0x810051
#define DUP5_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION 0x810052
#define DUP6_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION 0x810053
#define CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION 0x810054
#define DUP1_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION 0x810055
#define DUP2_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION 0x810056
#define DUP3_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION 0x810057
#define DUP4_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION 0x810058
#define DUP5_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION 0x810059
#define CTHUMBAGDAEMON_SHUTDOWNNOTIFICATION 0x81005a
#define CTHUMBAGDAEMON_ADDOBSERVERSL 0x81005b
#define DUP1_CTHUMBAGDAEMON_ADDOBSERVERSL 0x81005c
#define CTHUMBAGDAEMON_DAEMONENABLEDL 0x81005d
#define DUP1_CTHUMBAGDAEMON_DAEMONENABLEDL 0x81005e
#define CTHUMBAGDAEMON_RECONNECTCALLBACK 0x81005f
#define DUP1_CTHUMBAGDAEMON_RECONNECTCALLBACK 0x810060
#define _E32MAIN 0x810061
#define DUP1__E32MAIN 0x810062
#define DUP2__E32MAIN 0x810063
#define CTHUMBAGDAEMON_NEWL 0x860001


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



#ifndef __OSTTRACEGEN2_TUINT32_TUINT_TUINT__
#define __OSTTRACEGEN2_TUINT32_TUINT_TUINT__

inline TBool OstTraceGen2( TUint32 aTraceID, TUint aParam1, TUint aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TUint* )ptr ) = aParam1;
        ptr += sizeof ( TUint );
        *( ( TUint* )ptr ) = aParam2;
        ptr += sizeof ( TUint );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TUINT_TUINT__


#ifndef __OSTTRACEGEN2_TUINT32_TUINT32_TUINT32__
#define __OSTTRACEGEN2_TUINT32_TUINT32_TUINT32__

inline TBool OstTraceGen2( TUint32 aTraceID, TUint32 aParam1, TUint32 aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TUint* )ptr ) = aParam1;
        ptr += sizeof ( TUint );
        *( ( TUint* )ptr ) = aParam2;
        ptr += sizeof ( TUint );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TUINT32_TUINT32__



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

