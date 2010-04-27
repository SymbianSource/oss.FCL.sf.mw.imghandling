/*
* Copyright (c) 2007 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Panic codes for Thumbnail Manager
 *
*/


#ifndef THUMBNAILPANIC_H
#define THUMBNAILPANIC_H

#include <e32std.h>

_LIT( KThumbnailPanicCategory, "ThumbnailManager" );

enum TThumbnailPanicCode
    {
    // Numerical values used to help finding the reason based on
    // panic code.
    EThumbnailNullPointer = 0, EThumbnailBadSize = 1, EThumbnailBadBitmapHandle
        = 2, EThumbnailBadPath = 3, EThumbnailUnknownMessage = 4,
        EThumbnailMessageNotCompleted = 5, EThumbnailBitmapNotReleased = 6,
        EThumbnailEmptyDescriptor = 7, EThumbnailWrongId = 8, EThumbnailAlreadyRunning =
        9, EThumbnailDatabaseUnrecoverable = 10, EThumbnailSQLTransaction = 11
    };

/**
 * Call User::Panic() with Thumbnail Manager panic category using
 * one of the panic codes above
 * @param aCode Panic code
 */
inline void ThumbnailPanic( TThumbnailPanicCode aCode );

/**
 * Panic if a pointer is NULL
 */
#define TN_ASSERT_NOT_NULL( ptr ) \
__ASSERT_ALWAYS( ptr, ThumbnailPanic( EThumbnailNullPointer ) );

#include "thumbnailpanic.inl"


#endif // THUMBNAILPANIC_H
