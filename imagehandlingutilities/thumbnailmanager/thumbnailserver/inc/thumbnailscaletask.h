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
* Description:  Task for scaling thumbnails
 *
*/


#ifndef THUMBNAILSCALETASK_H
#define THUMBNAILSCALETASK_H

#include "thumbnailtask.h"
#include "thumbnailprovider.h"
#include "thumbnailstore.h" // TThumbnailPersistentSize

class CThumbnailServer;

/**
 *  Task for scaling thumbnails.
 *
 *  @since S60 v5.0
 */
class CThumbnailScaleTask: public CThumbnailTask
    {
public:

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @param aProcessor Processor object for running tasks.
     * @param aServer Server to use.
     * @param aFilename Filename of file from which the thumbnail is to be
     *                  created.
     * @param aBitmap Bitmap image. Scale task assumes ownership.
     * @param aOriginalSize Original size of the image.
     * @param aTargetSize Target size of thumbnail image.
     * @param aCrop Enable or disable cropping.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aThumbnailSize Prededined size of thumbnail.
     * @return New CThumbnailScaleTask object.
     * @param aEXIF is origin EXIF.
     * @param aRequestId Request ID.
     */
    static CThumbnailScaleTask* NewL( CThumbnailTaskProcessor& aProcessor,
        CThumbnailServer& aServer, const TDesC& aFilename, CFbsBitmap* aBitmap,
        const TSize& aOriginalSize, const TSize& aTargetSize, TBool aCrop,
        TDisplayMode aDisplayMode, TInt aPriority, const TDesC& aTargetUri,
        const TThumbnailSize aThumbnailSize, const TInt64 aModified,
        const TBool aBitmapToPool, const TBool aEXIF, 
        const TThumbnailServerRequestId aRequestId, const TBool aImportVirtual);

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailScaleTask();

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
     * Changes priority of the task.
     *
     * @since S60 v5.0
     */
    void ChangeTaskPriority( TInt aNewPriority );

    /**
     * Set whether the scaled image is to be stored or not.
     *
     * @since S60 v5.0
     * @param aDoStore Boolean value stating should the image be stored.
     */
    void SetDoStore( TBool aDoStore );

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aProcessor Processor object for running tasks.
     * @param aServer Server to use.
     * @param aFilename Filename of file from which the thumbnail is to be
     *                  created.
     * @param aBitmap Bitmap image. Scale task assumes ownership.
     * @param aOriginalSize Original size of the image.
     * @param aTargetSize Target size of thumbnail image.
     * @param aCrop Enable or disable cropping.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aThumbnailSize Prededined size of thumbnail.
     * @return New CThumbnailScaleTask object.
     * @param aEXIF is origin EXIF.
     * @param aRequestId Request ID.
     */
    CThumbnailScaleTask( CThumbnailTaskProcessor& aProcessor, CThumbnailServer&
        aServer, const TDesC& aFilename, CFbsBitmap* aBitmap, const TSize&
        aOriginalSize, const TSize& aTargetSize, TBool aCrop, TDisplayMode
        aDisplayMode, TInt aPriority, const TDesC& aTargetUri,
        const TThumbnailSize aThumbnailSize, const TInt64 aModified,
        const TBool aBitmapToPool, const TBool aEXIF, 
        const TThumbnailServerRequestId aRequestId, const TBool aImportVirtual);

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

    /**
     * Calculates target size to be used for the thumbnail when cropping is
     * disabled. The target size may be smaller than requested if aspect
     * ratios do not match. Updates iTargetSize.
     *
     * @since S60 v5.0
     */
    void CalculateTargetSize();

    /**
     * Calculates cropping rectangle to be used for the thumbnail when
     * cropping is enabled. Target size will always be the same as requested
     * size and the cropping rectangle defines which area of the image will
     * be visible. Updates iCropRectangle.
     *
     * @since S60 v5.0
     */
    void CalculateCropRectangle();

    /**
     * StoreAndCompleteL
     *
     * @since S60 v5.0
     */
    void StoreAndCompleteL();

private:
    // Data

    /**
     * Server.
     */
    CThumbnailServer& iServer;

    /**
     * Resides in bitmap pool.
     * Not own. But we do are responsible for decreasing the reference count
     * if iBitmapInPool is set.
     */
    CFbsBitmap* iBitmap;

    /**
     * Temporary place to store the original bitmap. This is set to NULL
     * after the bitmap has been successfully added to the bitmap pool.
     * Own.
     */
    CFbsBitmap* iOwnBitmap;

    /**
     * Bitmap pool.
     */
    TBool iBitmapInPool;

    /**
     * Original size of media object.
     */
    TSize iOriginalSize;

    /**
     * Target size of scaled thubnail (not real of preview)
     */
    TSize iTargetSize;
    
    /**
     * Target size of the thumbnail.
     */
    TSize iTargetSizeTN;

    /**
     * Incidates if cropping is enabled.
     */
    TBool iCrop;

    /**
     * Defines the visible area of iBitmap which is to be included
     * in the thumbnail if cropping is enabled. Not used if cropping
     * is disabled.
     */
    TRect iCropRectangle;

    /**
     * Display mode.
     */
    TDisplayMode iDisplayMode;

    /**
     * Filename of the image to be used.
     */
    TFileName iFilename;
    
    /**
     * Uri of the target object
     */
    TFileName iTargetUri;

    /**
     * Scaled image.
     */
    CFbsBitmap* iScaledBitmap; // own

    /**
     * If set, scaled bitmap must be released from pool.
     */
    TInt iScaledBitmapHandle;

    /**
     * If set, thumbnail will be stored for later use.
     */
    TBool iDoStore;

    /**
     * Temporary buffer for client/server parameters
     */
    TThumbnailRequestParamsPckgBuf iParamsBuf;

     /**
     * Size of requested Thumbnail
     */
    TThumbnailSize iThumbnailSize;
    
	 /**
     * timestamp
     */
    TInt64 iModified;
    
	/**
     * Add bitmap to server's pool.
     */
    TBool iBitmapToPool;
 
#ifdef _DEBUG
    TTime aStart, aStop;
#endif
    
     /**
     * Is origin EXIF.
     */
    TBool iEXIF;
    
    // virtual uri
    TBool iVirtualUri;
};

#endif // THUMBNAILSCALETASK_H
