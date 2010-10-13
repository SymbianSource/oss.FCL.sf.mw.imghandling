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
* Description:  Thumbnail object implementation.
 *
*/


#include <fbs.h>

#include "thumbnaildataimpl.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailDataImpl::CThumbnailDataImpl()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailDataImpl::CThumbnailDataImpl()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::~CThumbnailDataImpl()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailDataImpl::~CThumbnailDataImpl()
    {
    delete iBitmap;
    iClientData = NULL;
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::Bitmap()
// Get a pointer to a CFbsBitmap containing the thumbnail image.
// ---------------------------------------------------------------------------
//
CFbsBitmap* CThumbnailDataImpl::Bitmap()
    {
    return iBitmap;
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::DetachBitmap()
// Get a pointer to a CFbsBitmap containing the thumbnail image.
// ---------------------------------------------------------------------------
//
CFbsBitmap* CThumbnailDataImpl::DetachBitmap()
    {
    CFbsBitmap* ret = iBitmap;
    iBitmap = NULL; // client owns it now
    return ret;
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::ClientData()
// Get client data structure.
// ---------------------------------------------------------------------------
//
TAny* CThumbnailDataImpl::ClientData()
    {
    return iClientData;
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::Set
// Sets the thumbnail object.
// ---------------------------------------------------------------------------
//
void CThumbnailDataImpl::Set( CFbsBitmap* aBitmap, TAny* aClientData )
    {
    delete iBitmap;
    iBitmap = aBitmap;
    iClientData = aClientData;
    }

// End of file
