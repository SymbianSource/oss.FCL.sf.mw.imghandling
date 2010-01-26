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


#ifndef THUMBNAILPROVIDER_H
#define THUMBNAILPROVIDER_H

#include <ecom/ecom.h>
#include <apmstd.h>

#include "thumbnailmanager.h" // TThumbnailFlags
#include "thumbnaillog.h"

/**
 *  Thumbnail provider observer class
 *
 *  @since S60 v5.0
 */
class MThumbnailProviderObserver
    {
public:
    /**
     * Thumbnail provider observer callback to notify the server when
     * thumbnail has been generated.
     *
     * @since S60 5.0
     * @param aError Error code
     * @param aBitmap Thumbnail bitmap. The observer implementation will
     *                assume ownership of the bitmap.
     * @param aOriginalSize Original size of the object in pixels.
     * @param aEXIF ETrue if thumbnail origin is EXIF.
     */
    virtual void ThumbnailProviderReady( TInt aError, CFbsBitmap* aBitmap,
        const TSize& aOriginalSize, const TBool aEXIF, const TBool aPortrait  ) = 0;
};


/**
 *  Base class for thumbnail provider plug-ins.
 *
 *  @since S60 v5.0
 */
class CThumbnailProvider: public CBase
    {
public:
    /**
     * Create a provider instance. The instance is identified
     * by the implementation UID.
     */
    static CThumbnailProvider* NewL( TUid aUid );

    /**
     * Get the UID of this provider.
     *
     * @since S60 5.0
     * @return Provider implementation UID
     */
    TUid Uid();

    /**
     * Set the observer to receive notification
     * about thumbnail generation completion.
     *
     * @since S60 5.0
     * @param aObserver Reference to the observing object. Ownership not
     *                  transferred.
     */
    void SetObserver( MThumbnailProviderObserver& aObserver );

    /**
     * Set the target size for uncropped thumbnails. The actual thumbnail
     * at least this wide or high, depending on which is the limiting
     * factor.
     *
     * @since S60 5.0
     * @param aSize Target size in pixels for uncropped thumbnails.
     */
    void SetTargetSize( const TSize& aSize );

    /**
     * Set the target size for cropped thumbnails. The actual thumbnail
     * at least this large and it will include the whole thumbnail,
     * not just the cropped portion.
     *
     * @since S60 5.0
     * @param aCroppedSize Target size in pixels for cropped thumbnails.
     */
    void SetCroppedTargetSize( const TSize& aCroppedSize );

    /**
     * Reset thumbnail parameters to defaults.
     */
    void Reset();

    /**
     * Get thumbnail bitmap. Must be implemented by provider plug-ins.
     *
     * @since S60 5.0
     * @param aFs File server session that can be used
     * @param aFile File handle to the object file
     * @param aMimeType MIME-type of the object file.
     * @param aFlags Thumbnail generation flags
     * @param aDisplayMode Display mode
     */
    virtual void GetThumbnailL( RFs& aFs, RFile64& aFile, const TDataType&
        aMimeType, const CThumbnailManager::TThumbnailFlags aFlags, const
        TDisplayMode aDisplayMode, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference ) = 0;
  
    /**
     * Get thumbnail bitmap. Must be implemented by provider plug-ins.
     *
     * @since S60 5.0
     * @param aFs File server session that can be used
     * @param aBuffer object buffer
     * @param aMimeType MIME-type of the object file.
     * @param aFlags Thumbnail generation flags
     * @param aDisplayMode Display mode
     */    
    virtual void GetThumbnailL( RFs& aFs, TDesC8* aBuffer, const
        TDataType& aMimeType, const CThumbnailManager::TThumbnailFlags aFlags,
        const TDisplayMode aDisplayMode, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference  ) = 0;  
    
    /**
     * Get thumbnail bitmap. Must be implemented by provider plug-ins.
     *
     * @since S60 5.0
     * @param aFs File server session that can be used
     * @param aBuffer object buffer
     */
    
    virtual void GetThumbnailL( RFs& aFs, TDesC8& aBuffer) = 0;

    /**
     * Cancel a previous thumbnail request, if any.
     *
     * @since S60 5.0
     */
    virtual void CancelGetThumbnail() = 0;

    virtual ~CThumbnailProvider(){}

protected:
    /**
     * Get the actual target size for the thumbnail.
     * This is the maximum of the non-cropped and cropped
     * target sizes.
     *
     * @since S60 5.0
     */
    inline void ResolveSize();

    /**
     * Calculate the scaled down size for the thumbnail.
     * Aspect ratio is always preserved. If cropping is disabled
     * the resulting size at most as wide or tall as requested.
     * If cropping is enabled, the result is at least as wide
     * and as tall as requested (unless original size is smaller
     * than requested).
     *
     * @since S60 5.0
     * @param aOriginalSize Original size in pixels
     * @param aTargetSize Target size in pixels
     * @param aCrop Enable cropping
     * @return Result size
     */
    inline TSize FitToSize( const TSize& aOriginalSize, const TSize&
        aTargetSize, TBool aCrop );

protected:
    /**
     * Provider observer. Implemented by thumbnail generation task.
     * Not own.
     */
    MThumbnailProviderObserver* iObserver;

    /**
     * Thumbnail generation flags.
     */
    CThumbnailManager::TThumbnailFlags iFlags;

    /**
     * Thumbnail generation flags.
     */
    CThumbnailManager::TThumbnailQualityPreference iQualityPreference;

    /**
     * Size of the original object.
     */
    TSize iOriginalSize;

    /**
     * Target size for uncropped thumbnails.
     */
    TSize iTargetSize;

    /**
     * Target size for cropped thumbnails.
     */
    TSize iCroppedTargetSize;

    /**
     * Display mode for thumbnail bitmap
     */
    TDisplayMode iDisplayMode;

    /**
     * ECOM plug-in destructor key.
     */
    TUid iDtor_ID_Key;

    /**
     * UID of this implementation.
     */
    TUid iUid;
};


#endif // THUMBNAILPROVIDER_H
