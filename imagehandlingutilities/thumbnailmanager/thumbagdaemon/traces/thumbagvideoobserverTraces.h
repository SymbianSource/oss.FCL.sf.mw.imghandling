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

#ifndef __THUMBAGVIDEOOBSERVERTRACES_H__
#define __THUMBAGVIDEOOBSERVERTRACES_H__

#define KOstTraceComponentID 0x2001fd51

#define CTHUMBAGVIDEOOBSERVER_NEWL 0x8600b1
#define CTHUMBAGVIDEOOBSERVER_CONSTRUCTL 0x8600b2
#define DUP1_CTHUMBAGVIDEOOBSERVER_CONSTRUCTL 0x8600b3
#define CTHUMBAGVIDEOOBSERVER_INITIALIZEL 0x8600b4
#define DUP1_CTHUMBAGVIDEOOBSERVER_INITIALIZEL 0x8600b5
#define DUP2_CTHUMBAGVIDEOOBSERVER_INITIALIZEL 0x8600b6
#define DUP3_CTHUMBAGVIDEOOBSERVER_INITIALIZEL 0x8600b7
#define CTHUMBAGVIDEOOBSERVER_CTHUMBAGVIDEOOBSERVER 0x8600b8
#define DUP1_CTHUMBAGVIDEOOBSERVER_CTHUMBAGVIDEOOBSERVER 0x8600b9
#define CTHUMBAGVIDEOOBSERVER_HANDLESESSIONOPENED 0x8600ba
#define DUP1_CTHUMBAGVIDEOOBSERVER_HANDLESESSIONOPENED 0x8600bb
#define DUP2_CTHUMBAGVIDEOOBSERVER_HANDLESESSIONOPENED 0x8600bc
#define CTHUMBAGVIDEOOBSERVER_HANDLESESSIONERROR 0x8600bd
#define DUP1_CTHUMBAGVIDEOOBSERVER_HANDLESESSIONERROR 0x8600be
#define CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION 0x8600bf
#define DUP1_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION 0x8600c0
#define DUP2_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION 0x8600c1
#define DUP3_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION 0x8600c2
#define DUP4_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION 0x8600c3
#define DUP5_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION 0x8600c4
#define DUP6_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION 0x8600c5
#define DUP7_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION 0x8600c6
#define CTHUMBAGVIDEOOBSERVER_SHUTDOWNNOTIFICATION 0x8600c7
#define DUP1_CTHUMBAGVIDEOOBSERVER_SHUTDOWNNOTIFICATION 0x8600c8
#define CTHUMBAGVIDEOOBSERVER_ADDOBSERVERSL 0x8600c9
#define DUP1_CTHUMBAGVIDEOOBSERVER_ADDOBSERVERSL 0x8600ca
#define CTHUMBAGVIDEOOBSERVER_RECONNECTCALLBACK 0x8600cb
#define DUP1_CTHUMBAGVIDEOOBSERVER_RECONNECTCALLBACK 0x8600cc


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

