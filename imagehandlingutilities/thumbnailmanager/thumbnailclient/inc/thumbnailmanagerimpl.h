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
* Description:  Implementation class of Thumbnail Manager
 *
*/


#ifndef THUMBNAILMANAGERIMPL_H
#define THUMBNAILMANAGERIMPL_H

#include <fbs.h>

#include "thumbnailmanager.h"
#include "thumbnailsession.h"
#include "thumbnailrequestqueue.h"
#include "thumbnaildata.h"


class CThumbnailRequestActive;

/**
 *  Implementation class of thumbnail engine.
 *
 *  @since S60 v5.0
 */
NONSHARABLE_CLASS( CThumbnailManagerImpl ): public CThumbnailManager
    {
public:

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailManagerImpl();

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @param  aObserver Observer to receive notifications about completed
     *                   operations.
     * @return           New CThumbnailManagerImpl instance.
     */
    static CThumbnailManagerImpl* NewLC( MThumbnailManagerObserver& aObserver );

    /**
     * Get a thumbnail for an object file. If a thumbnail already exists, it
     * is loaded and if a thumbnail does not exist, it is created
     * transparently. ThumbnailReady() callback will be called when the
     * operation is complete. In addition, ThumbnailPreviewReady()
     * callback may be called if EOptimizeForQualityWithPreview mode was
     * defined.
     *
     * Current values for display mode, thumbnail size, flags and performance
     * preference are used.
     *
     * @since S60 v5.0
     * @param aObjectSource      Source object or file
     * @param aClientData        Pointer to arbitrary client data.
     *                           This pointer is not used by the API for
     *                           anything other than returning it in the
     *                           ThumbnailReady callback.
     * @param aPriority          Priority for this operation
     * @return                   Thumbnail request ID
     */
    TThumbnailRequestId GetThumbnailL( CThumbnailObjectSource& aObjectSource,
        TAny* aClientData = NULL, const TInt aPriority = CActive::EPriorityIdle );
    
    /**
     * Get a thumbnail for an object file. If a thumbnail already exists, it
     * is loaded and if a thumbnail does not exist, it is created
     * transparently. ThumbnailReady() callback will be called when the
     * operation is complete. In addition, ThumbnailPreviewReady()
     * callback may be called if EOptimizeForQualityWithPreview mode was
     * defined.
     *
     * Current values for display mode, thumbnail size, flags and performance
     * preference are used.
     *
     * @since S60 v5.0
     * @param aObjectSource      Source object or file
     * @param aClientData        Pointer to arbitrary client data.
     *                           This pointer is not used by the API for
     *                           anything other than returning it in the
     *                           ThumbnailReady callback.
     * @param aPriority          Priority for this operation
     * @param aGeneratePersistentSizesOnly  Only persitent sizes generated
     * @return                   Thumbnail request ID
     */    
    TThumbnailRequestId GetThumbnailL( CThumbnailObjectSource& aObjectSource,
    	TAny* aClientData, const TInt aPriority, TBool aGeneratePersistentSizesOnly );    
   
    /**
     * Get a persistent thumbnail for an object file. If a thumbnail already
     *  exists, it is loaded and if a thumbnail does not exist, it is created
     * transparently. ThumbnailReady() callback will be called when the
     * operation is complete. In addition, ThumbnailPreviewReady()
     * callback may be called if EOptimizeForQualityWithPreview mode was
     * defined.
     *
     * Current values for display mode, thumbnail size, flags and performance
     * preference are used.
     *
     * @since S60 v5.0
     * @param aThumbnailId       Thumbnail ID
     * @param aThumbnailSizeType Thumbnail size enumeration
     * @param aClientData        Pointer to arbitrary client data.
     *                           This pointer is not used by the API for
     *                           anything other than returning it in the
     *                           ThumbnailReady callback.
     * @param aPriority          Priority for this operation
     * @return                   Thumbnail request ID. This can be used to
     *                           cancel the request or change priority.
     *                           The ID is specific to this CThumbnailManager
     *                           instance and may not be shared with other
     *                           instances.
     */        
    TThumbnailRequestId GetThumbnailL( const TThumbnailId,
        TAny* aClientData = NULL, TInt aPriority = CActive::EPriorityIdle );
    
    /**
     * Import an image to be used as thumbnail for an object. If a
     * thumbnail already exists, it is loaded and if a thumbnail does not
     * exist, it is created transparently. ThumbnailReady() callback will be
     * called when the operation is complete. In addition, ThumbnailPreviewReady()
     * callback may be called if EOptimizeForQualityWithPreview mode was
     * defined.
     * 
     * Current values for display mode, thumbnail size, flags and performance
     * preference are used.
     *
     * @since S60 v5.0
     * @param aObjectSource      Source object or file
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aClientData        Pointer to arbitrary client data.
     *                           This pointer is not used by the API for
     *                           anything other than returning it in the
     *                           ThumbnailReady callback.
     * @param aPriority          Priority for this operation
     * @return                   Thumbnail request ID
     */
    TThumbnailRequestId ImportThumbnailL( CThumbnailObjectSource& aObjectSource,
        const TDesC& aTargetUri, TAny* aClientData = NULL,
        const TInt aPriority = CActive::EPriorityIdle );    

     /**
      * Set persistent size scaled thumbnails for an object. If a
      * thumbnail already exists, it is replaced and if a thumbnail does not
      * exist, it is created transparently.
      * 
      * Current values for display mode, thumbnail size, flags and performance
      * preference are used.
      *
      * @since S60 v5.0
      * @param aObjectSource      Source object or file
      * @param aClientData        Pointer to arbitrary client data.
      *                           This pointer is not used by the API for
      *                           anything other than returning it in the
      *                           ThumbnailReady callback.
      * @param aPriority          Priority for this operation
      * @return                   Thumbnail request ID. This can be used to
      *                           cancel the request or change priority.
      *                           The ID is specific to this CThumbnailManager
      *                           instance and may not be shared with other
      *                           instances.
      */     
    TThumbnailRequestId SetThumbnailL( CThumbnailObjectSource& source,
        TAny* aClientData = NULL, 
        TInt aPriority = CActive::EPriorityIdle );    
    
    /**
     * Get the current display mode for thumbnail bitmaps.
     *
     * @since S60 v5.0
     * @return Default display mode for the thumbnail bitmaps.
     */
    TDisplayMode DisplayMode()const;

    /**
     * Set the current display mode for thumbnail bitmaps.
     *
     * @since S60 v5.0
     * @param aDisplayMode New display mode value for the thumbnail bitmaps.
     */
    void SetDisplayModeL( const TDisplayMode aDisplayMode );

    /**
     * Get the current quality versus performance preference.
     *
     * @since S60 v5.0
     * @return Current quality versus performance preference.
     */
    TThumbnailQualityPreference QualityPreference()const;

    /**
     * Set quality versus performance preference.
     *
     * @since S60 v5.0
     * @param aQualityPreference New quality versus performance preference
     *                           value.
     */
    void SetQualityPreferenceL( const TThumbnailQualityPreference
        aQualityPreference );

    /**
     * Get the current desired size for thumbnail bitmaps.
     *
     * @since S60 v5.0
     * @return Current desired size for thumbnail bitmaps (in pixels).
     */
    const TSize& ThumbnailSize()const;

    /**
     * Set desired size for thumbnail bitmaps.
     *
     * @since S60 v5.0
     * @param aThumbnailSize New quality for the desired thumbnail size.
     */
    void SetThumbnailSizeL( const TSize& aThumbnailSize );

    /**
     * Set desired size for thumbnail bitmaps.
     *
     * @since S60 v5.0
     * @param aThumbnailSize New quality for the desired thumbnail size.
     */
    void SetThumbnailSizeL( const TThumbnailSize aThumbnailSize );

    /**
     * Get current flags for thumbnail generation.
     *
     * @since S60 v5.0
     * @return Current flags.
     */
    TThumbnailFlags Flags()const;

    /**
     * Set flags for thumbnail generation. Several flags may be enabled
     * by combining the values using bitwise or.
     *
     * @since S60 v5.0
     * @param aFlags New flags.
     */
    void SetFlagsL( const TThumbnailFlags aFlags );

    /**
     * Delete all thumbnails for a given object. This is an asynchronous
     * operation, which always returns immediately.
     *
     * @since S60 v5.0
     * @param aObjectSource Source object or file
     */
    void DeleteThumbnails( CThumbnailObjectSource& aObjectSource );

    /**
     * Delete thumbnails by TThumbnailId. This is an asynchronous
     * operation, which always returns immediately.
     *
     * @since S60 v5.0
     * @param aItemId     TThumbnailId
     */
    void DeleteThumbnails( const TThumbnailId aItemId );       
    
    /**
     * Create thumbnail for a given object. This is an asynchronous
     * operation, which always returns immediately. No callbacks are
     * emitted.
     *
     * @since S60 v5.0
     * @param aObjectSource      Source object or file
     * @param aPriority          Priority for this operation
     * @return                   Thumbnail creation request ID
     */
    TThumbnailRequestId CreateThumbnails( CThumbnailObjectSource& aObjectSource,
                                          TInt aPriority = CActive::EPriorityIdle );

    /**
     * Cancel a thumbnail operation.
     *
     * @since S60 v5.0
     * @param aId      Request ID for the operation to be cancelled.
     * @return         Symbian OS error code or KErrNone if cancelling was
     *                 successful.
     */
    TInt CancelRequest( const TThumbnailRequestId aId );

    /**
     * Change the priority of a queued thumbnail operation.
     *
     * @since S60 v5.0
     * @param aId           Request ID for the request which to assign a new
     *                      priority.
     * @param aNewPriority  New priority value
     * @return              Symbian OS error code or KErrNone if change was
     *                      successful.
     */
    TInt ChangePriority( const TThumbnailRequestId aId, const TInt aNewPriority );

    /**
     * Get a list of supported file formats for object files.
     * 
     * The return value is a reference to a list that contains each
     * supported MIME type. There may also be wildcards, such as "image/ *".
     * 
     * The returned reference is valid until CThumbnailManager is
     * destroyed or GetSupportedMimeTypesL() is called again.
     *
     * @since S60 v5.0
     * @return A list of supported MIME types. May contain wildcards.
     *         Ownership not transferred.
     */
    const CDesCArray& GetSupportedMimeTypesL();
    
    /**
     * Update Thumbnails by TThumbnailId. This is an asynchronous
     * operation, which always returns immediately.
     *
     * @since S60 v5.0
     * @param aItemId       TThumbnailId
     * @param aPath         (New) path for the Thumbnail
     * @param aOrientation  Thumbnail orientation
     * @param aModified     Last modified
     * @param aPriority     Priority for this operation
     */
     void UpdateThumbnailsL( const TThumbnailId aItemId, const TDesC& aPath,
                             const TInt aOrientation, const TInt64 aModified, TInt aPriority );

     /**
      * Rename Thumbnails. This is an asynchronous
      * operation, which always returns immediately.
      *
      * @since S60 v5.0
      * @param aCurrentPath     Current path of the Thumbnail
      * @param aNewPath         New path for the Thumbnail
      * @param aPriority        Priority for this operation
      * @return                 Thumbnail request ID. This can be used to
      *                         cancel the request or change priority.
      *                         The ID is specific to this CThumbnailManager
      *                         instance and may not be shared with other
      *                         instances.
      */
     TThumbnailRequestId RenameThumbnailsL( const TDesC& aCurrentPath, 
             const TDesC& aNewPath, TInt aPriority = CActive::EPriorityIdle );

     /**
      * Set optional request observer for getting information about completed 
      * requests that don't include a thumbnail.
      *
      * @since Symbian^3
      * @param aObserver Observer to receive notifications.
      */
     void SetRequestObserver( MThumbnailManagerRequestObserver& aObserver );
     
     /**
      * Remove optional request observer.
      *
      * @since Symbian^3
      */
     void RemoveRequestObserver();
     
private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param  aObserver Observer to receive notifications about completed
     *                   operations.
     * @return           New CThumbnailManagerImpl instance.
     */
    CThumbnailManagerImpl( MThumbnailManagerObserver& aObserver );

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();
    
    /**
     * Check that given priority is in range of CActive::TPriority
     *
     * @since S60 v5.0
     * @param aPriority Priority
     * @return Valid priority.
     */
    TInt ValidatePriority( const TInt aPriority );


private:
    // data

    /**
     * Default width of thumbnail.
     */
    TInt iDefaultWidth;

    /**
     * Default height of thumbnail
     */
    TInt iDefaultHeight;

    /**
     * Observer to receive notifications about completed operations.
     */
    MThumbnailManagerObserver& iObserver;
    
    /**
     * Observer to receive notifications about completed requests.
     */
    MThumbnailManagerRequestObserver* iRequestObserver;

    /**
     * Session.
     */
    RThumbnailSession iSession;

    /**
     * Fileserver.
     */
    RFs iFs;
    
    /**
     * Request queue processor.
     */
    CThumbnailRequestQueue* iRequestQueue;

    /**
     * Display mode.
     */
    TDisplayMode iDisplayMode;

    /**
     * Flags.
     */
    TThumbnailFlags iFlags;

    /**
     * Quality preference.
     */
    TThumbnailQualityPreference iQualityPreference;

    /**
     * Requested size of thumbnail.
     */
    TSize iSize;
    
    TThumbnailSize iThumbnailSize;

    /**
     * Request ID for this thumbnail request, session specific.
     */
    TThumbnailRequestId iRequestId;

    /**
     * Font and Bitmap Server session. Only used unless the client
     * does not already have an open session.
     */
    RFbsSession iFbsSession;

    /**
     * Temporary buffer for filename handling
     */
    TFileName iFileNameBuf;

    /**
     * List of supported MIME-types.
     * Own.
     */
    CDesCArraySeg* iMimeTypeList;
};

#endif
