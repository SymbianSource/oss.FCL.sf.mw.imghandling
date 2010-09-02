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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnaildataimplTraces.h"
#endif

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailDataImpl::CThumbnailDataImpl()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailDataImpl::CThumbnailDataImpl()
    {
    OstTrace0( TRACE_NORMAL, CTHUMBNAILDATAIMPL_CTHUMBNAILDATAIMPL, "CThumbnailDataImpl::CThumbnailDataImpl" );
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::~CThumbnailDataImpl()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailDataImpl::~CThumbnailDataImpl()
    {
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILDATAIMPL_CTHUMBNAILDATAIMPL, "CThumbnailDataImpl::~CThumbnailDataImpl" );
    delete iBitmap;
    iBitmap = NULL;
    iClientData = NULL;
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::Bitmap()
// Get a pointer to a CFbsBitmap containing the thumbnail image.
// ---------------------------------------------------------------------------
//
CFbsBitmap* CThumbnailDataImpl::Bitmap()
    {
    OstTrace0( TRACE_NORMAL, CTHUMBNAILDATAIMPL_BITMAP, "CThumbnailDataImpl::Bitmap" );
    return iBitmap;
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::DetachBitmap()
// Get a pointer to a CFbsBitmap containing the thumbnail image.
// ---------------------------------------------------------------------------
//
CFbsBitmap* CThumbnailDataImpl::DetachBitmap()
    {
    OstTrace0( TRACE_NORMAL, CTHUMBNAILDATAIMPL_DETACHBITMAP, "CThumbnailDataImpl::DetachBitmap" );
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
    OstTrace0( TRACE_NORMAL, CTHUMBNAILDATAIMPL_CLIENTDATA, "CThumbnailDataImpl::ClientData" );
    return iClientData;
    }


// ---------------------------------------------------------------------------
// CThumbnailDataImpl::Set
// Sets the thumbnail object.
// ---------------------------------------------------------------------------
//
void CThumbnailDataImpl::Set( CFbsBitmap* aBitmap, TAny* aClientData )
    {
    OstTrace0( TRACE_NORMAL, CTHUMBNAILDATAIMPL_SET, "CThumbnailDataImpl::Set" );
    delete iBitmap;
    iBitmap = aBitmap;
    iClientData = aClientData;
    }

// End of file
