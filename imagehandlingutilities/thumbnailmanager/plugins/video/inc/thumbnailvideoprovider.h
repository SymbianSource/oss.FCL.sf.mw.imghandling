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
* Description:  Video thumbnail provider.
 *
*/


#ifndef THUMBNAILVIDEOPROVIDER_H
#define THUMBNAILVIDEOPROVIDER_H

#include <ecom.h>
#include <TNEVideoClipInfo.h>

#include "thumbnailprovider.h"

/**
 *  Video thumbnail provider plugin.
 *
 *  @since S60 v5.0
 */
class CThumbnailVideoProvider: public CThumbnailProvider, public CActive, 
    public MTNEVideoClipThumbObserver, public MTNEVideoClipInfoObserver
    {
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return New video thumbnail provider instance.
     */
    static CThumbnailVideoProvider* NewL();

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailVideoProvider();

    /**
     * Function that will provide thumbnail from given parameters.
     *
     * @since S60 v5.0
     * @param aFs File server session
     * @param aFile Source image file.
     * @param aMimeType Mime-type of the clip to be processed.
     * @param aFlags Flags for thumbnail creation.
     * @param aDisplayMode Used display mode.
     */
    void GetThumbnailL( RFs& aFs, RFile64& aFile, const TDataType& aMimeType,
        const CThumbnailManager::TThumbnailFlags aFlags, const TDisplayMode
        aDisplayMode, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference );

    /**
     * Function that will provide thumbnail from given parameters.
     *
     * @since S60 v5.0
     * @param aFs File server session
     * @param aBuffer Source image buffer.
     * @param aMimeType Mime-type of the clip to be processed.
     * @param aFlags Flags for thumbnail creation.
     * @param aDisplayMode Used display mode.
     */    
    void GetThumbnailL( RFs& aFs, TDesC8* aBuffer, const
        TDataType& aMimeType, const CThumbnailManager::TThumbnailFlags aFlags,
        const TDisplayMode aDisplayMode, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference );     
    
    /**
     * Function that will provide thumbnail from given parameters.
     *
     * @since S60 v5.0
     * @param aFs File server session
     * @param aBuffer Source image buffer.
     */ 
    
    void GetThumbnailL( RFs& aFs, TDesC8& aBuffer);
    /**
     * Cancel a previous thumbnail request, if any.
     *
     * @since S60 5.0
     */
    void CancelGetThumbnail();

protected:

    /**
     * Notifies when video clip thumbnail is ready.
     *
     * @since S60 v5.0
     * @param aInfo Video clip info.
     * @param aError Error code.
     * @param aThumb Thumbnail image.
     */
    void NotifyVideoClipThumbCompleted( CTNEVideoClipInfo& aInfo, TInt aError,
        CFbsBitmap* aThumb );

    /**
     * Notifies when info about video clip is ready.
     *
     * @since S60 v5.0
     * @param aInfo Video clip info.
     * @param aError Error code.
     */
    void NotifyVideoClipInfoReady( CTNEVideoClipInfo& aInfo, TInt aError );

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New video thumbnail provider instance.
     */
    CThumbnailVideoProvider();
    
    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    void DoCancel();

    /**
     * Handles an active object's request completion event.
     *
     * @since S60 v5.0
     */
    void RunL();

private:
    // data

    /**
     * Video clip info.
     */
    CTNEVideoClipInfo* iVideoClipInfo;
    
    RTimer iTimer;
    TBool iTimeout;

};

#endif // THUMBNAILVIDEOPROVIDER_H
