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


#ifndef THUMBNAILGENERATETASK_H
#define THUMBNAILGENERATETASK_H

#include <apmstd.h>

#include "thumbnailtask.h"
#include "thumbnailprovider.h"
#include "thumbnailstore.h" // TThumbnailPersistentSize

class CThumbnailServer;

/**
 *  Task for generating new thumbnails.
 *
 *  @since S60 v5.0
 */
class CThumbnailGenerateTask: public CThumbnailTask,
                              public MThumbnailProviderObserver
    {
public:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aProcessor Processor object for running tasks.
     * @param aServer Thumbnail server in use.
     * @param aFile File from which the thumbnail is to be created.
     * @param aFlags Flags that control the creation of thumbnail.
     * @param aSize Requested size of thumbnail.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @return New CThumbnailGenerateTask object.
     */
    CThumbnailGenerateTask( CThumbnailTaskProcessor& aProcessor, CThumbnailServer& aServer,
            RFile64* aFile, TDesC8* aBuffer, const TDataType* aMimeType,
            CThumbnailManager::TThumbnailFlags aFlags, const TSize& aSize,
            TDisplayMode aDisplayMode, TInt aPriority,
            RArray < TThumbnailPersistentSize >* aMissingSizes, const TDesC& aTargetUri,
            TThumbnailSize aThumbnailSize, const TInt64 aModified,
            const CThumbnailManager::TThumbnailQualityPreference aQualityPreference,
            TBool aImportVirtual );

    /**
     * Destructor.
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailGenerateTask();

    /**
     * StartL
     *
     * @since S60 v5.0
     */
    void StartL();

    /**
     * Handles an active object's request completion event.
     *
     * @since S60 v5.0
     */
    void RunL();

    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    void DoCancel();

    /**
     * Defines if scaled bitmaps needs to be added to pool.
     *
     * @since S60 v5.0
     * @param aBool boolean
     */
    void ScaledBitmapToPool(TBool aBool);    
    
private:

    /**
     * Calculate the largest non-cropped size and largest cropped size and
     * let the provider know.
     *
     * @since S60 v5.0
     * @param aEffectiveTargetSize Effective target size.
     * @param aCroppedTargetSize Cropped target size.
     */
    void CalculateTargetSizesL( TSize& aEffectiveTargetSize, TSize&
        aCroppedTargetSize );

    /**
     * For notifying when task has been finished.
     *
     * @since S60 v5.0
     * @param aError Error code.
     * @param aBitmap Thumbnail image.
     * @param aOriginalSize Original image size.
     * @param aEXIF is origin EXIF.
     */
    void ThumbnailProviderReady( const TInt aError, CFbsBitmap* aBitmap, const
        TSize& aOriginalSize, const TBool aEXIF, const TBool aPortrait );

    /**
     * For creating scaling task.
     *
     * @since S60 v5.0
     * @param aBitmap Bitmap image.
     * @param aEXIF is origin EXIF.
     */
    void CreateScaleTasksL( CFbsBitmap* aBitmap );
    
    /**
     * For creating blacklisted thumbnail.
     *
     * @param aOriginalSize Original size of bitmap.
     */
    void CreateBlackListedL( const TSize& aOriginalSize );
    
    
    /**
     * Check is blacklisting needed
     *
     * @param aErrorCode verdict is based on this error code
     * @param aOriginalSize Original size of bitmap.
     */
    void DoBlacklisting( const TInt aError, const TSize& aOriginalSize );
    
private:
    // Data

    /**
     * Server.
     */
    CThumbnailServer& iServer;

    /**
     * Imagefile from which the thumbnail is to be created.
     */
    RFile64 iFile;
    
    /**
     * Image buffer
     */    
    TDesC8* iBuffer;

    /**
     * Mime type of the original image.
     */
    TDataType iMimeType;

    /**
     * Filename of the image from which the thumbnail is to be created.
     */
    TFileName iFilename;
    
    /**
     * Flags
     */
    CThumbnailManager::TThumbnailFlags iFlags;
    /**
     * Requested  size of thumbnail.
     */
    TSize iSize;

    /**
     * Display mode.
     */
    TDisplayMode iDisplayMode;

    /**
     * Only missing persistent sized thumbnails will be created
     */
    RArray < TThumbnailPersistentSize >* iMissingSizes;

    /**
     * Uri of the target object
     */
    TFileName iTargetUri;    
    
    /**
     * Relative thumbnail size
     */
    TThumbnailSize iThumbnailSize;    
    
	 /**
     * timestamp
     */
    TInt64 iModified;

    /**
     * Size of the original image.
     */
    TSize iOriginalSize;

    /**
     * Thumbnail provider implementation.
     * Not own.
     */
    CThumbnailProvider* iProvider;
    
    TBool iScaledBitmapToPool;
    
#ifdef _DEBUG
    TTime aStart, aStop;
#endif
    
    CThumbnailManager::TThumbnailQualityPreference iQualityPreference;
    
    TBool iEXIF;
    
    /**
     * Target rect rotated (portrait image)
     */
    TBool iPortrait;
    
    TInt iBitmapHandle;
    
    // virtual uri
    TBool iVirtualUri;
};

#endif // THUMBNAILGENERATETASK_H
