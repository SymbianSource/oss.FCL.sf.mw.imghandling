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

#ifndef __THUMBAGIMAGEOBSERVERTRACES_H__
#define __THUMBAGIMAGEOBSERVERTRACES_H__

#define KOstTraceComponentID 0x2001fd51

#define CTHUMBAGIMAGEOBSERVER_NEWLC 0x810064
#define CTHUMBAGIMAGEOBSERVER_NEWL 0x810065
#define CTHUMBAGIMAGEOBSERVER_CONSTRUCTL 0x810066
#define DUP1_CTHUMBAGIMAGEOBSERVER_CONSTRUCTL 0x810067
#define CTHUMBAGIMAGEOBSERVER_INITIALIZEL 0x810068
#define DUP1_CTHUMBAGIMAGEOBSERVER_INITIALIZEL 0x810069
#define DUP2_CTHUMBAGIMAGEOBSERVER_INITIALIZEL 0x81006a
#define DUP3_CTHUMBAGIMAGEOBSERVER_INITIALIZEL 0x81006b
#define CTHUMBAGIMAGEOBSERVER_CTHUMBAGIMAGEOBSERVER 0x81006c
#define DUP1_CTHUMBAGIMAGEOBSERVER_CTHUMBAGIMAGEOBSERVER 0x81006d
#define CTHUMBAGIMAGEOBSERVER_HANDLESESSIONOPENED 0x81006e
#define DUP1_CTHUMBAGIMAGEOBSERVER_HANDLESESSIONOPENED 0x81006f
#define DUP2_CTHUMBAGIMAGEOBSERVER_HANDLESESSIONOPENED 0x810070
#define CTHUMBAGIMAGEOBSERVER_HANDLESESSIONERROR 0x810071
#define DUP1_CTHUMBAGIMAGEOBSERVER_HANDLESESSIONERROR 0x810072
#define CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION 0x810073
#define DUP1_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION 0x810074
#define DUP2_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION 0x810075
#define DUP3_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION 0x810076
#define DUP4_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION 0x810077
#define DUP5_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION 0x810078
#define DUP6_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION 0x810079
#define DUP7_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION 0x81007a
#define CTHUMBAGIMAGEOBSERVER_ADDOBSERVERSL 0x81007b
#define DUP1_CTHUMBAGIMAGEOBSERVER_ADDOBSERVERSL 0x81007c
#define CTHUMBAGIMAGEOBSERVER_SHUTDOWNNOTIFICATION 0x81007d
#define DUP1_CTHUMBAGIMAGEOBSERVER_SHUTDOWNNOTIFICATION 0x81007e
#define CTHUMBAGIMAGEOBSERVER_RECONNECTCALLBACK 0x81007f
#define DUP1_CTHUMBAGIMAGEOBSERVER_RECONNECTCALLBACK 0x810080


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

