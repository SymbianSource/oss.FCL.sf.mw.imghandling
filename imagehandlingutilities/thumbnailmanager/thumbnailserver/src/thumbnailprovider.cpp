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
* Description:  Base class for thumbnail provider plug-ins
 *
*/


#include "thumbnailprovider.h"
#include "thumbnaillog.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// Construct new CThumbnailProvider implementation instance via ECom
// ---------------------------------------------------------------------------
//
CThumbnailProvider* CThumbnailProvider::NewL( TUid aUid )
    {
    TAny* interface = REComSession::CreateImplementationL( aUid, _FOFF(
        CThumbnailProvider, iDtor_ID_Key ));
    CThumbnailProvider* ret = reinterpret_cast < CThumbnailProvider*  > (
        interface );
    ret->iUid = aUid;
    return ret;
    }


// ---------------------------------------------------------------------------
// Get the UID of the provider
// ---------------------------------------------------------------------------
//
TUid CThumbnailProvider::Uid()
    {
    return iUid;
    }


// ---------------------------------------------------------------------------
// CThumbnailProvider::SetObserver()
// ---------------------------------------------------------------------------
//
void CThumbnailProvider::SetObserver( MThumbnailProviderObserver& aObserver )
    {
    iObserver = &aObserver;
    }


// ---------------------------------------------------------------------------
// CThumbnailProvider::Reset()
// ---------------------------------------------------------------------------
//
void CThumbnailProvider::Reset()
    {
    iTargetSize = TSize();
    iCroppedTargetSize = TSize();
    }


// ---------------------------------------------------------------------------
// CThumbnailProvider::SetTargetSize()
// ---------------------------------------------------------------------------
//
void CThumbnailProvider::SetTargetSize( const TSize& aSize )
    {
    iTargetSize = aSize;
    }


// ---------------------------------------------------------------------------
// CThumbnailProvider::SetCroppedTargetSize()
// ---------------------------------------------------------------------------
//
void CThumbnailProvider::SetCroppedTargetSize( const TSize& aCroppedSize )
    {
    iCroppedTargetSize = aCroppedSize;
    }

// End of file
