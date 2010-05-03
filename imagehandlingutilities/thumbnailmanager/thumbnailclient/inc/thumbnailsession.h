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
* Description:  Thumbnail server client-side session
 *
*/


#ifndef THUMBNAILSESSION_H
#define THUMBNAILSESSION_H

#include <e32base.h>

#include "thumbnailmanagerconstants.h"

/**
 *  Thumbnail server client-side session
 *
 *  @since S60 v5.0
 */
NONSHARABLE_CLASS( RThumbnailSession ): public RSessionBase
    {
public:

    /**
     * C++ default constructor.
     *
     * @since S60 v5.0
     * @return New RThumbnailSession session.
     */
    RThumbnailSession();

    /**
     * Connect to server process.
     *
     * @since S60 v5.0
     * @return Error code.
     */
    TInt Connect();

    /**
     * Close session.
     *
     * @since S60 v5.0
     */
    void Close();

    /**
     * Version info.
     *
     * @since S60 v5.0
     * @return Version of client.
     */
    TVersion Version();

    /**
     * Request a thumbnail for an object file.
     *
     * @since S60 v5.0
     * @param aFile File from which the thumbnail is to be created.
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aParams Package for request parameters.
     * @param aStatus Status of the request.
     */
    void RequestThumbnailL( const RFile64& aFile, const TDesC& aTargetUri, TThumbnailRequestParamsPckg&
        aParams, TRequestStatus& aStatus );

    /**
     * Request a thumbnail for an object file.
     *
     * @since S60 v5.0
     * @param aPath Path of image from which the thumbnail is to be created.
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aParams Package for request parameters.
     * @param aStatus Status of the request.
     */
    void RequestThumbnailL( const TDesC& aPath, const TDesC& aTargetUri, const TThumbnailId aThumbnailId, TThumbnailRequestParamsPckg&
        aParams, TRequestStatus& aStatus );

    /**
     * Request a thumbnail for an object file.
     *
     * @since S60 v5.0
     * @param aThumbnailId Thumbnail ID
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aParams Package for request parameters.
     * @param aStatus Status of the request.
     */    
    void RequestThumbnailL( const TThumbnailId aThumbnailId, const TDesC& aTargetUri,
            TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus );  
    
    /**
     * Request set thumbnail for an object file.
     *
     * @since S60 v5.0
     * @param aBuffer buffer from which the thumbnail is to be created.
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aParams Package for request parameters.
     * @param aStatus Status of the request.
     */    
    void RequestSetThumbnailL( 
            TDesC8* aBuffer, const TDesC& aTargetUri,         
            TThumbnailRequestParamsPckg& aParams, 
            TRequestStatus& aStatus  );
    
    /**
     * Request set thumbnail for an object file.
     *
     * @since S60 v5.0
     * @param aBitmapHandle Bitmap handle
     * @param aTargetUri         Target URI to which the imported thumbnail is linked.
     * @param aParams Package for request parameters.
     * @param aStatus Status of the request.
     */    
    void RequestSetThumbnailL( 
            TInt aBitmapHandle, const TDesC& aTargetUri,         
            TThumbnailRequestParamsPckg& aParams, 
            TRequestStatus& aStatus  );    
    
    /**
     * Release bitmap instance kept by server process.
     *
     * @since S60 v5.0
     * @param aBitmapHandle Handle to bitmap.
     */
    void ReleaseBitmap( TInt aBitmapHandle );

    /**
     * Cancel pending thumbnail request.
     *
     * @since S60 v5.0
     * @param aRequestId Session specific thumbnail request ID.
     * @return Error code.
     */
    TInt CancelRequest( TThumbnailRequestId aRequestId );

    /**
     * Change priority of pending thumbnail request.
     *
     * @since S60 v5.0
     * @param aRequestId Session specific thumbnail request ID.
     * @param aNewPriority New priority to be set for the request.
     * @return Error code.
     */
    TInt ChangePriority( TThumbnailRequestId aRequestId, TInt aNewPriority );

    /**
     * Delete thumbnails.
     *
     * @since S60 v5.0
     * @param aPath Path to media object whose thumbnails should be deleted.
     */
    void DeleteThumbnails( const TDesC& aPath, TThumbnailRequestParamsPckg& aParams,
            TRequestStatus& aStatus );

    /**
     * Delete thumbnails.
     *
     * @since S60 v5.0
     * @param aItemId Id of item whose thumbnails should be deleted.
     */
    void DeleteThumbnails( const TThumbnailId aItemId, TThumbnailRequestParamsPckg& aParams,
            TRequestStatus& aStatus );    
    
    /**
     * Get a list of supported MIME types in a HBufC. The list is space
     * delimited.
     *
     * @since S60 v5.0
     * @return MIME type list. Ownership transferred to called.
     */
    HBufC* GetMimeTypeListL();
    
    /**
     * Update thumbnails.
     *
     * @since S60 v5.0
     * @param aPath         (New) path for the Thumbnail
     * @param aOrientation  Thumbnail orientation
     * @param aModified     Last modified
     */
    void UpdateThumbnails( const TDesC& aPath, const TInt aOrientation, const TInt64 aModified,
                           TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus);

    /**
     * Rename thumbnails.
     *
     * @since S60 v5.0
     */
    void RenameThumbnails( TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus );
    
private:

    /**
     * Launch server process.
     *
     * @since S60 v5.0
     * @return Error code.
     */
    TInt StartServer();

};

#endif // THUMBNAILSESSION_H
