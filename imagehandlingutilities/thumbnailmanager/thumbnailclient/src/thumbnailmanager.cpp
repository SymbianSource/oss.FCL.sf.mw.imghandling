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
* Description:  Main interface class to Thumbnail Manager.
 *
*/


#include "thumbnailmanagerimpl.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailManager* CThumbnailManager::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
EXPORT_C CThumbnailManager* CThumbnailManager::NewL( MThumbnailManagerObserver&
    aObserver )
    {
    CThumbnailManager* self = CThumbnailManagerImpl::NewLC( aObserver );
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CThumbnailManager* CThumbnailManager::NewLC()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
EXPORT_C CThumbnailManager* CThumbnailManager::NewLC(
    MThumbnailManagerObserver& aObserver )
    {
    return CThumbnailManagerImpl::NewLC( aObserver );
    }

// ---------------------------------------------------------------------------
// CThumbnailManager::~CThumbnailManager()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailManager::~CThumbnailManager()
    {
    // No implementation required
    }

// End of file
