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
* Description:  Active object for an asynchronous thumbnail request
 *
*/


#ifndef THUMBNAILREQUESTACTIVE_H
#define THUMBNAILREQUESTACTIVE_H

#include <e32base.h>

#include "thumbnailpanic.h"
#include "thumbnailmanager.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"

class RFs;
class CFbsBitmap;
class RThumbnailSession;
class CThumbnailDataImpl;
class CThumbnailRequestQueue;

enum TThumbnailReqType
    {
    EReqGetThumbnailHandle = 0, 
    EReqGetThumbnailPath = 1,
    EReqSetThumbnailBuffer = 2,
    EReqSetThumbnailBitmap = 3,
    EReqUpdateThumbnails = 4,
    EReqGetThumbnailHandleLater = 5,
    EReqDeleteThumbnails = 6,
    EReqRenameThumbnails = 7,
    EReqSetThumbnailPath = 8
};

/**
 *  Active object for an asynchronous thumbnail request.
 *
 *  @since S60 v5.0
 */
NONSHARABLE_CLASS( CThumbnailRequestActive ): public CActive
    {
public:

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailRequestActive();

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @param aFs Fileserver used.
     * @param aThumbnailSession Session used.
     * @param aObserver Observer to receive notifications about completed operations.
     * @param aRequestObserver Observer to receive notifications about completed requests.
     * @param aId Assigned ID of the request, session specific.
     * @param aPriority assigned processing priority
     * @param aQueue request processor
     * @return New CThumbnailRequestActive object.
     */
    static CThumbnailRequestActive* NewL( RFs& aFs, RThumbnailSession&
        aThumbnailSession, MThumbnailManagerObserver& aObserver, 
        MThumbnailManagerRequestObserver* aRequestObserver, TThumbnailRequestId aId, 
        TInt aPriority, CThumbnailRequestQueue* aQueue );

    /**
     * Get a thumbnail created from file object.
     *
     * @since S60 v5.0
     * @param aFile File from which the thumbnail is to be created.
     * @param aFlags Flags that control the creation of thumbnail.
     * @param aQualityPreference Quality preference value
     * @param aSize Requested size of the thumbnail.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aClientData Pointer to arbitrary client data.
     *                    This pointer is not used by the API for
     *                    anything other than returning it in the
     *                    ThumbnailReady callback.
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     */
    void GetThumbnailL( const RFile64& aFile, TThumbnailId aThumbnailId, const TDesC8& aMimeType,
        CThumbnailManager::TThumbnailFlags aFlags, CThumbnailManager::TThumbnailQualityPreference
        aQualityPreference, const TSize& aSize, const TDisplayMode aDisplayMode,
        const TInt aPriority, TAny* aClientData, TBool aGeneratePersistentSizesOnly, const TDesC& aTargetUri = KNullDesC,
        TThumbnailSize aThumbnailSize = EUnknownThumbnailSize );

    /**
     * Get a thumbnail opening file object later.
     *
     * @since S60 v5.0
     * @param aPath Path to file from which the thumbnail is to be created.
     * @param aFlags Flags that control the creation of thumbnail.
     * @param aQualityPreference Quality preference value
     * @param aSize Requested size of the thumbnail.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aClientData Pointer to arbitrary client data.
     *                    This pointer is not used by the API for
     *                    anything other than returning it in the
     *                    ThumbnailReady callback.
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     */
    void GetThumbnailL( TThumbnailId aThumbnailId, const TDesC& aPath, const TDesC8& aMimeType,
        CThumbnailManager::TThumbnailFlags aFlags, CThumbnailManager::TThumbnailQualityPreference
        aQualityPreference, const TSize& aSize, const TDisplayMode aDisplayMode,
        const TInt aPriority, TAny* aClientData, TBool aGeneratePersistentSizesOnly, const TDesC& aTargetUri = KNullDesC,
        TThumbnailSize aThumbnailSize = EUnknownThumbnailSize );
    
    /**
     * Get a thumbnail created from file path.
     *
     * @since S60 v5.0
     * @param aPath Path to file from which the thumbnail is to be created.
     * @param aFlags Flags that control the creation of thumbnail.
     * @param aQualityPreference Quality preference value
     * @param aSize Requested size of the thumbnail.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aClientData Pointer to arbitrary client data.
     *                    This pointer is not used by the API for
     *                    anything other than returning it in the
     *                    ThumbnailReady callback.
     * @param aGeneratePersistentSizesOnly 
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aThumbnailSize Relative thumbnail size
     */
    void GetThumbnailL( const TDesC& aPath, TThumbnailId aThumbnailId,
        const TDesC8& aMimeType, CThumbnailManager::TThumbnailFlags aFlags, 
        CThumbnailManager::TThumbnailQualityPreference aQualityPreference, 
        const TSize& aSize, const TDisplayMode aDisplayMode, const TInt aPriority, 
        TAny* aClientData, TBool aGeneratePersistentSizesOnly, 
        const TDesC& aTargetUri = KNullDesC, TThumbnailSize aThumbnailSize = EUnknownThumbnailSize );  
    
    /**
     * Set a thumbnail
     *
     * @since S60 v5.0
     * @param aBuffer Buffer containing image where the thumbnail is to be created.
     * @param aFlags Flags that control the creation of thumbnail.
     * @param aQualityPreference Quality preference value
     * @param aSize Requested size of the thumbnail.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aClientData Pointer to arbitrary client data.
     *                    This pointer is not used by the API for
     *                    anything other than returning it in the
     *                    ThumbnailReady callback.
     * @param aGeneratePersistentSizesOnly
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aThumbnailSize Requested thumbnail size 
     * @param aOverwrite Overwrite old existing thumbs
     */    
    void SetThumbnailL( TDesC8* aBuffer, TThumbnailId aThumbnailId, const TDesC8& aMimeType,
        CThumbnailManager::TThumbnailFlags aFlags, CThumbnailManager
        ::TThumbnailQualityPreference aQualityPreference, const TSize& aSize, const
        TDisplayMode aDisplayMode, const TInt aPriority, TAny* aClientData, TBool aGeneratePersistentSizesOnly,
        const TDesC& aTargetUri, TThumbnailSize aThumbnailSize, TBool aOverwrite);
    
    /**
     * Set a thumbnail
     *
     * @since S60 v5.0
     * @param aBitmap Bitmap
     * @param aFlags Flags that control the creation of thumbnail.
     * @param aQualityPreference Quality preference value
     * @param aSize Requested size of the thumbnail.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aClientData Pointer to arbitrary client data.
     *                    This pointer is not used by the API for
     *                    anything other than returning it in the
     *                    ThumbnailReady callback.
     * @param aGeneratePersistentSizesOnly
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aThumbnailSize Requested thumbnail size 
     * @param aOverwrite Overwrite old existing thumbs
     */    
    void SetThumbnailL( CFbsBitmap* aBitmap, TThumbnailId aThumbnailId, const TDesC8& aMimeType,
        CThumbnailManager::TThumbnailFlags aFlags, CThumbnailManager
        ::TThumbnailQualityPreference aQualityPreference, const TSize& aSize, const
        TDisplayMode aDisplayMode, const TInt aPriority, TAny* aClientData, TBool aGeneratePersistentSizesOnly,
        const TDesC& aTargetUri, TThumbnailSize aThumbnailSize, TBool aOverwrite);    
    
    /**
     * Update thumbnails by Id.
     *
     * @since S60 v5.0
     * @param aPath Path to file from which the thumbnail is to be created.
     * @param aFlags Flags that control the creation of thumbnail.
     * @param aQualityPreference Quality preference value
     * @param aSize Requested size of the thumbnail.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aOrientation  Thumbnail orientation
     * @param aModified     Last modified
     */
    void UpdateThumbnailsL( const TDesC& aPath, const TThumbnailId aThumbnailId,
        CThumbnailManager::TThumbnailFlags aFlags, 
        CThumbnailManager::TThumbnailQualityPreference aQualityPreference, 
        const TDisplayMode aDisplayMode, const TInt aPriority, const TInt aOrientation,
        const TInt64 aModified );   
    
    /**
     * Delete thumbnails.
     *
     * @since S60 v5.0
     * @param aPath Path to file from which the thumbnail is to be created.
     * @param aPriority Priority of the request.
     */
    void DeleteThumbnails( const TDesC& aPath, const TThumbnailId aThumbnailId,
        const TInt aPriority );   
    
    /**
     * Rename thumbnails.
     *
     * @since S60 v5.0
     * @param aCurrentPath     Current path of the Thumbnail
     * @param aNewPath         New path for the Thumbnail
     * @param aPriority        Priority for this operation
     */
    void RenameThumbnails( const TDesC& aCurrentPath, const TDesC& aNewPath, 
        const TInt aPriority );    
    
    /**
     * Set thumbnail from file path.
     *
     * @param aPath Path to file from which the thumbnail is to be created.
     * @param aFile File from which the thumbnail is to be created.
     * @param aFlags Flags that control the creation of thumbnail.
     * @param aQualityPreference Quality preference value
     * @param aSize Requested size of the thumbnail.
     * @param aDisplayMode Display mode.
     * @param aPriority Priority of the request.
     * @param aClientData Pointer to arbitrary client data.
     *                    This pointer is not used by the API for
     *                    anything other than returning it in the
     *                    ThumbnailReady callback.
     * @param aGeneratePersistentSizesOnly 
     * @param aOverwrite Overwrite old existing thumbs
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aThumbnailSize Relative thumbnail size
     */
    void SetThumbnailL( const TDesC& aPath, const TDesC8& aMimeType,
        CThumbnailManager::TThumbnailFlags aFlags, 
        CThumbnailManager::TThumbnailQualityPreference aQualityPreference, 
        const TSize& aSize, const TDisplayMode aDisplayMode, const TInt aPriority, 
        TAny* aClientData, TBool aGeneratePersistentSizesOnly, const TDesC& aTargetUri, 
        TThumbnailSize aThumbnailSize, TBool aOverwrite);    
    
    /**
     * Start active request.
     *
     * @since S60 v5.0
     */
    void StartL();
    
    /**
     * Change priority of a request.
     *
     * @since S60 v5.0
     * @param aNewPriority New priority.
     * @return Error code.
     */
    void ChangePriority( const TInt aNewPriority );
    
    /**
     * Returns ID of thumbnail request.
     *
     * @since S60 v5.0
     * @return Request ID
     */
    TThumbnailRequestId RequestId()const;

    /**
     * Is thumbnail request completed
     *
     * @since S60 v5.0
     */
    TBool RequestCompleted()const;
    
	 /**
     * Is thumbnail request active
     *
     * @since S60 v5.0
     */
    TBool IsRequestActive()const;
    
    /**
     * Error handling function.
     *
     * @since S60 v5.0
     */
    void StartError( const TInt aErr );
    
    /**
     * Async cancel
     *
     * @since S60 v5.0
     */
    void AsyncCancel();
    
private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aFs Fileserver used.
     * @param aThumbnailSession Session used.
     * @param aObserver Observer to receive notifications about completed operations.
     * @param aRequestObserver Observer to receive notifications about completed requests.
     * @param aId Assigned ID of the request, session specific.
     * @param aPriority assigned processing priority
     * @param aQueue request processor
     * @return New CThumbnailRequestActive object.
     */
    CThumbnailRequestActive( RFs& aFs, RThumbnailSession& aThumbnailSession,
        MThumbnailManagerObserver& aObserver, MThumbnailManagerRequestObserver* aRequestObserver, 
        TThumbnailRequestId aId, TInt aPriority, CThumbnailRequestQueue* aQueue);

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

    /**
     * Handles an active object's request completion event.
     *
     * @since S60 v5.0
     */
    void RunL();

    /**
     * Handles a leave occurring in the request completion event handler
     * RunL().
     *
     * @since S60 v5.0
     * @param aError The leave code.
     * @return Error code.
     */
    TInt RunError( TInt aError );

    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    void DoCancel();

    /**
     * Release bitmap instance kept by server process.
     *
     * @since S60 v5.0
     */
    void ReleaseServerBitmap();

    /**
     * Error handling function.
     *
     * @since S60 v5.0
     */
    void HandleError();

    /**
     * Request 2nd phase thumbnail
     *
     * @since S60 v5.0
     */
    void Get2ndPhaseThumbnailL();

    /**
     * Callback for timeout timer
     *
     * @since S60 v5.0
     */
    static TInt TimerCallBack(TAny* aAny);
    
    /**
     * Checks if URI is virtual.
     */
    TBool IsVirtualUri( const TDesC& aPath ); 

private:
    // data

    /**
     * Session.
     */
    RThumbnailSession& iSession;
    
    /**
     * Request parameters.
     */
    TThumbnailRequestParams iParams;

    /**
     * Package for request parameters.
     */
    TThumbnailRequestParamsPckg iParamsPckg;

    /**
     * Observer to receive notifications about completed operations.
     */
    MThumbnailManagerObserver& iObserver;
    
    /**
     * Observer to receive notifications about completed requests.
     */
    MThumbnailManagerRequestObserver* iRequestObserver;

    /**
     * Fileserver, not own
     */
    RFs& iFs;

    /**
     * Client data.
     */
    TAny* iClientData;

    /**
     * Callback, own.
     */
    CThumbnailDataImpl* iCallbackThumbnail;

    /**
     * Handle to bitmap, own.
     */
    TInt iBitmapHandle;

    /**
     * Error code.
     */
    TInt iError;

    /**
     * Request id of thumbnail request, session specific.
     */
    TThumbnailRequestId iRequestId;

    /**
     * Duplicated file handle for two phase operation
     */
    RFile64 iMyFileHandle;

    /**
     * If set, we are currently processing the preview thumbnail
     * of a two phase operation. If an error occurs, we can skip
     * directly to 2nd phase.
     */
    TBool iProcessingPreview;

    /**
     * If set, this request object can be deleted.
     */
    TBool iRequestCompleted;
    
	 /**
     * If set, this request object is under processing
     */
    TBool iRequestActive;
    
    /**
     * Bitmap that is passed to server
     */
    CFbsBitmap* iBitmap;
    
    // for activating requests
    RFile64 iFile;
    TFileName iPath;
    TFileName iTargetUri;
    TInt iOrientation;
    TInt64 iModified;
    
    // not own
    CThumbnailRequestQueue* iRequestQueue;
    
    TThumbnailReqType iRequestType;
    
    // request timeout timer
    CPeriodic* iTimer;
    TInt iStartError;
    
    // request already canceled by client
    TBool iCanceled;
    
#ifdef __RETRY_ON_SERVERCRASH
    //request retry count
    TUint iRetry;
#endif
    
#ifdef _DEBUG
    TTime iStartExecTime;
#endif
};

#endif // THUMBNAILREQUESTACTIVE_H



// INLINE FUNCTIONS

inline TBool CThumbnailRequestActive::RequestCompleted()const
    {
    return iRequestCompleted;
    }

inline TBool CThumbnailRequestActive::IsRequestActive()const
    {
    if(IsActive())
        {
        return ETrue;
        }
    return iRequestActive;
    }

inline TThumbnailRequestId CThumbnailRequestActive::RequestId()const
    {
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

    return iRequestId;
    }
