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


#ifndef THUMBNAILDATAIMPL_H
#define THUMBNAILDATAIMPL_H

#include "thumbnaildata.h"

/**
 *  Thumbnail object implementation.
 *
 *  @since S60 v5.0
 */
NONSHARABLE_CLASS( CThumbnailDataImpl ): public CBase, public MThumbnailData
    {
public:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New thumbnail instance.
     */
    CThumbnailDataImpl();

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailDataImpl();

public:

    /**
     * Get a pointer to a CFbsBitmap containing the thumbnail image. Ownership
     * of the object is not transferred (i.e. client must not delete the
     * pointer).
     * @since S60 v5.0
     * @return Pointer to a bitmap representing the thumbnail or NULL if
     *         thumbnail pointer is not available or it has been detached.
     */
    CFbsBitmap* Bitmap();

    /**
     * Get a pointer to a CFbsBitmap containing the thumbnail image. Ownership
     * of the object is transferred to the caller. Client must delete the
     * bitmap after it is done processing it.
     * @since S60 v5.0
     * @return Pointer to a bitmap representing the thumbnail or NULL if
     *         thumbnail pointer is not available or it has been detached.
     *         Caller assumes ownership of the bitmap.
     */
    CFbsBitmap* DetachBitmap();

    /**
     * Get client data structure.
     *
     * @since S60 v5.0
     * @return A pointer for client data specified as a parameter for
     *         GetThumbnailL() or NULL if not specified.
     */
    TAny* ClientData();

    /**
     * Sets the thumbnail object data.
     *
     * @since S60 v5.0
     * @param aBitmap Bitmab for the object.
     * @param aClientData Pointer to arbitrary client data.
     *                    This pointer is not used by the API for
     *                    anything other than returning it in the
     *                    ThumbnailReady callback.
     */
    void Set( CFbsBitmap* aBitmap, TAny* aClientData );

private:
    // data

    /**
     * Bitmap.
     */
    CFbsBitmap* iBitmap;

    /**
     * Arbitrary client data.
     */
    TAny* iClientData;
};

#endif // THUMBNAILDATAIMPL_H
