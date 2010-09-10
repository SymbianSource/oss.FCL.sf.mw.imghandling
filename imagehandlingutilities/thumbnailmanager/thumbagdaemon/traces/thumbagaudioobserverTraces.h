// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBAGAUDIOOBSERVERTRACES_H__
#define __THUMBAGAUDIOOBSERVERTRACES_H__

#define KOstTraceComponentID 0x2001fd51

#define CTHUMBAGAUDIOOBSERVER_NEWLC 0x810001
#define CTHUMBAGAUDIOOBSERVER_CONSTRUCTL 0x810002
#define DUP1_CTHUMBAGAUDIOOBSERVER_CONSTRUCTL 0x810003
#define CTHUMBAGAUDIOOBSERVER_INITIALIZEL 0x810004
#define DUP1_CTHUMBAGAUDIOOBSERVER_INITIALIZEL 0x810005
#define DUP2_CTHUMBAGAUDIOOBSERVER_INITIALIZEL 0x810006
#define DUP3_CTHUMBAGAUDIOOBSERVER_INITIALIZEL 0x810007
#define CTHUMBAGAUDIOOBSERVER_CTHUMBAGAUDIOOBSERVER 0x810008
#define DUP1_CTHUMBAGAUDIOOBSERVER_CTHUMBAGAUDIOOBSERVER 0x810009
#define CTHUMBAGAUDIOOBSERVER_SHUTDOWN 0x81000a
#define CTHUMBAGAUDIOOBSERVER_HANDLESESSIONOPENED 0x81000b
#define DUP1_CTHUMBAGAUDIOOBSERVER_HANDLESESSIONOPENED 0x81000c
#define DUP2_CTHUMBAGAUDIOOBSERVER_HANDLESESSIONOPENED 0x81000d
#define CTHUMBAGAUDIOOBSERVER_HANDLESESSIONERROR 0x81000e
#define DUP1_CTHUMBAGAUDIOOBSERVER_HANDLESESSIONERROR 0x81000f
#define CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION 0x810010
#define DUP1_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION 0x810011
#define DUP2_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION 0x810012
#define DUP3_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION 0x810013
#define DUP4_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION 0x810014
#define DUP5_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION 0x810015
#define DUP6_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION 0x810016
#define DUP7_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION 0x810017
#define CTHUMBAGAUDIOOBSERVER_SHUTDOWNNOTIFICATION 0x810018
#define DUP1_CTHUMBAGAUDIOOBSERVER_SHUTDOWNNOTIFICATION 0x810019
#define CTHUMBAGAUDIOOBSERVER_ADDOBSERVERSL 0x81001a
#define DUP1_CTHUMBAGAUDIOOBSERVER_ADDOBSERVERSL 0x81001b
#define CTHUMBAGAUDIOOBSERVER_RECONNECTCALLBACK 0x81001c
#define DUP1_CTHUMBAGAUDIOOBSERVER_RECONNECTCALLBACK 0x81001d


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



#endif

// End of file

