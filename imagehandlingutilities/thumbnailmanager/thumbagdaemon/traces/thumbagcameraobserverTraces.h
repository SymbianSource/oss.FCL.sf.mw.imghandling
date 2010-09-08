/*
* Copyright (c) 2006-2007 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  Thumbnail Auto Generate Daemon 
*
*/
// Created by TraceCompiler 2.3.0
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBAGCAMERAOBSERVERTRACES_H__
#define __THUMBAGCAMERAOBSERVERTRACES_H__

#define KOstTraceComponentID 0x2001fd51

#define CTHUMBAGCAMERAOBSERVER_NEWLC 0x81001e
#define CTHUMBAGCAMERAOBSERVER_NEWL 0x81001f
#define CTHUMBAGCAMERAOBSERVER_CONSTRUCTL 0x810020
#define DUP1_CTHUMBAGCAMERAOBSERVER_CONSTRUCTL 0x810021
#define CTHUMBAGCAMERAOBSERVER_INITIALIZEL 0x810022
#define DUP1_CTHUMBAGCAMERAOBSERVER_INITIALIZEL 0x810023
#define DUP2_CTHUMBAGCAMERAOBSERVER_INITIALIZEL 0x810024
#define DUP3_CTHUMBAGCAMERAOBSERVER_INITIALIZEL 0x810025
#define CTHUMBAGCAMERAOBSERVER_CTHUMBAGCAMERAOBSERVER 0x810026
#define DUP1_CTHUMBAGCAMERAOBSERVER_CTHUMBAGCAMERAOBSERVER 0x810027
#define CTHUMBAGCAMERAOBSERVER_HANDLESESSIONOPENED 0x810028
#define DUP1_CTHUMBAGCAMERAOBSERVER_HANDLESESSIONOPENED 0x810029
#define DUP2_CTHUMBAGCAMERAOBSERVER_HANDLESESSIONOPENED 0x81002a
#define CTHUMBAGCAMERAOBSERVER_HANDLESESSIONERROR 0x81002b
#define DUP1_CTHUMBAGCAMERAOBSERVER_HANDLESESSIONERROR 0x81002c
#define CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION 0x81002d
#define DUP1_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION 0x81002e
#define DUP2_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION 0x81002f
#define DUP3_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION 0x810030
#define DUP4_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION 0x810031
#define DUP5_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION 0x810032
#define DUP6_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION 0x810033
#define DUP7_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION 0x810034
#define CTHUMBAGCAMERAOBSERVER_SHUTDOWNNOTIFICATION 0x810035
#define DUP1_CTHUMBAGCAMERAOBSERVER_SHUTDOWNNOTIFICATION 0x810036
#define CTHUMBAGCAMERAOBSERVER_ADDOBSERVERSL 0x810037
#define DUP1_CTHUMBAGCAMERAOBSERVER_ADDOBSERVERSL 0x810038
#define CTHUMBAGCAMERAOBSERVER_RECONNECTCALLBACK 0x810039
#define DUP1_CTHUMBAGCAMERAOBSERVER_RECONNECTCALLBACK 0x81003a


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

