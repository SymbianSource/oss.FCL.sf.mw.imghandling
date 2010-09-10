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
* Description:  Task for generating new thumbnails
 *
*/
// Created by TraceCompiler 2.3.0
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILGENERATETASKTRACES_H__
#define __THUMBNAILGENERATETASKTRACES_H__

#define KOstTraceComponentID 0x102830ab

#define CTHUMBNAILGENERATETASK_CTHUMBNAILGENERATETASK 0x86001f
#define DUP1_CTHUMBNAILGENERATETASK_CTHUMBNAILGENERATETASK 0x860020
#define DUP2_CTHUMBNAILGENERATETASK_CTHUMBNAILGENERATETASK 0x860021
#define CTHUMBNAILGENERATETASK_STARTL 0x860022
#define DUP1_CTHUMBNAILGENERATETASK_STARTL 0x860023
#define DUP2_CTHUMBNAILGENERATETASK_STARTL 0x860024
#define CTHUMBNAILGENERATETASK_DOCANCEL 0x860025
#define CTHUMBNAILGENERATETASK_THUMBNAILPROVIDERREADY 0x860026
#define DUP1_CTHUMBNAILGENERATETASK_THUMBNAILPROVIDERREADY 0x860027
#define DUP2_CTHUMBNAILGENERATETASK_THUMBNAILPROVIDERREADY 0x860028
#define DUP3_CTHUMBNAILGENERATETASK_THUMBNAILPROVIDERREADY 0x860029
#define CTHUMBNAILGENERATETASK_CREATESCALETASKSL 0x86002a
#define DUP1_CTHUMBNAILGENERATETASK_CREATESCALETASKSL 0x86002b
#define DUP2_CTHUMBNAILGENERATETASK_CREATESCALETASKSL 0x86002c
#define DUP3_CTHUMBNAILGENERATETASK_CREATESCALETASKSL 0x86002d
#define DUP4_CTHUMBNAILGENERATETASK_CREATESCALETASKSL 0x86002e
#define DUP5_CTHUMBNAILGENERATETASK_CREATESCALETASKSL 0x86002f
#define DUP6_CTHUMBNAILGENERATETASK_CREATESCALETASKSL 0x860030
#define DUP7_CTHUMBNAILGENERATETASK_CREATESCALETASKSL 0x860031
#define CTHUMBNAILGENERATETASK_CREATEBLACKLISTEDL 0x860032
#define CTHUMBNAILGENERATETASK_DOBLACKLISTING 0x860033
#define DUP1_CTHUMBNAILGENERATETASK_DOBLACKLISTING 0x860034
#define DUP2_CTHUMBNAILGENERATETASK_DOBLACKLISTING 0x860035
#define DUP3_CTHUMBNAILGENERATETASK_DOBLACKLISTING 0x860036
#define DUP4_CTHUMBNAILGENERATETASK_DOBLACKLISTING 0x860037
#define DUP5_CTHUMBNAILGENERATETASK_DOBLACKLISTING 0x860038


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
