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
* Description:  Server side session for thumbnail engine
 *
*/


#ifndef THUMBNAILSERVERSESSION_H
#define THUMBNAILSERVERSESSION_H

#include <e32base.h>
#include <e32capability.h>

#include "thumbnailmanagerconstants.h"

// FORWARD DECLARATIONS
class CThumbnailServer;

/**
 *  Server side session for thumbnail engine.
 *
 *  @since S60 v5.0
 */
NONSHARABLE_CLASS( CThumbnailServerSession ): public CSession2
    {
public:

    /**
     * Constructor.
     *
     * @since S60 v5.0
     * @return New CThumbnailServerSession session.
     */
    CThumbnailServerSession();

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailServerSession();

public:

    /**
     * Message dispatcher.
     *
     * @since S60 v5.0
     * @param aMessage Message to be passed.
     * @return Error code.
     */
    TInt DispatchMessageL( const RMessage2& aMessage );

public:
    // Functions from base classes

    /**
     * CreateL
     *
     * @since S60 v5.0
     */
    void CreateL();

    /**
     * From CSharableSession (pure virtual).
     * Handles service request messages from clients.
     *
     * @since S60 v5.0
     * @param aMessage message from client.
     */
    void ServiceL( const RMessage2& aMessage );

public:
    // New functions

    /**
     * Returns the server pointer.
     *
     * @since S60 v5.0
     * @return Server pointer.
     */
    CThumbnailServer* Server();

    /**
     * Cancellation method.
     *
     * @since S60 v5.0
     */
    void Cancel();

    /**
     * Converts SQL error to E32 error.
     *
     * @since S60 v5.0
     * @param aReason Request message complete value which may be SQL error
     * @return Request message complete value but not SQL error
     */
    static TInt ConvertSqlErrToE32Err( TInt aReason );
    
    void ResolveMimeTypeL( RFile64* aFile );
    
    void ModifyThumbnailSize( TInt aSourceType );

private:

    /**
     * RequestThumbByFileHandleAsyncL
     *
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void RequestThumbByFileHandleAsyncL( const RMessage2& aMessage );

    /**
     * RequestThumbByPathAsyncL
     *
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void RequestThumbByPathAsyncL( const RMessage2& aMessage );
    
    /**
     * RequestThumbByIdAsyncL
     *
     * @since S60 v5.0
     * @param aMessage Message.
     */    
    void RequestThumbByIdAsyncL( const RMessage2& aMessage );
    
    /**
     * RequestSetThumbnailByBufferL
     *
     * @since S60 v5.0
     * @param aMessage Message.
     */        
    void RequestSetThumbnailByBufferL( const RMessage2& aMessage );

    /**
     * RequestSetThumbnailByBitmapL
     *
     * @since S60 v5.0
     * @param aMessage Message.
     */        
    void RequestSetThumbnailByBitmapL( const RMessage2& aMessage );    
    
    /**
     * Fetch thumbnail data from database.
     *
     * @since S60 v5.0
     */
    void FetchThumbnailL();

    /**
     * Create a task to generate a new thumbnail
     *
     * @since S60 v5.0
     * @param aFile File handle
     */
    void CreateGenerateTaskFromFileHandleL( RFile64* aFile);
    
    /**
     * Create a task to generate a new thumbnail
     *
     * @since S60 v5.0
     * @param aBuffer Source image buffer to be set
     */
    void CreateGenerateTaskFromBufferL( TDesC8* aBuffer );

    /**
     * Process a fetched bitmap by creating scale tasks
     * or by returning the bitmap as such.
     *
     * @since S60 v5.0
     */
    void ProcessBitmapL();

    /**
     * ReleaseBitmap()
     *
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void ReleaseBitmap( const RMessage2& aMessage );

    /**
     * Cancels pending request.
     *
     * @since S60 v5.0
     * @param aMessage Message.
     * @return Error code.
     */
    TInt CancelRequest( const RMessage2& aMessage );

    /**
     * Changes priority of pending request.
     *
     * @since S60 v5.0
     * @param aMessage Message.
     * @return Error code.
     */
    TInt ChangePriority( const RMessage2& aMessage );

    /**
     * Delete thumbnails.
     *
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void DeleteThumbnailsL( const RMessage2& aMessage );

    /**
     * Delete thumbnails by Id.
     *
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void DeleteThumbnailsByIdL( const RMessage2& aMessage );    
    
    /**
     * Get the required size (in characters) for a buffer that contains the
     * list of supported MIME types
     * 
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void GetMimeTypeBufferSizeL( const RMessage2& aMessage );

    /**
     * Get the list of supported MIME types and store them in the buffer
     * allocated by the client.
     * 
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void GetMimeTypeListL( const RMessage2& aMessage );
    
    /**
     * Update thumbnails by Thumbnail ID.
     * 
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void UpdateThumbnailsL( const RMessage2& aMessage );
    
    /**
     * Rename thumbnails.
     * 
     * @since S60 v5.0
     * @param aMessage Message.
     */
    void RenameThumbnailsL( const RMessage2& aMessage );
    
    /**
     * Checks if client thread is still alive and RMessage2 handle valid.
     *
     * @since S60 v5.0
     */
    TBool ClientThreadAlive();    
    
private:
    // data
    /**
     * Temporary store for any bitmaps fetched from the store.
     * Own.
     */
    CFbsBitmap* iBitmap;

    /**
     * Temporary store for thumbnail request parameters as received
     * from the client.
     */
    TThumbnailRequestParamsPckgBuf iRequestParams;

    /**
     * Message from client. This is kept in a member variable to make
     * sure the message is completed if a leave occurs while handling
     * the request.
     */
    RMessage2 iMessage;

    /**
     * Size of the original object. Only valid if a stored thumbnail
     * was found when processing a request.
     */
    TSize iOriginalSize;
    
    /**
     * Temporary store for any jpegs fetched from the store.
     * Own.
     */
    TDesC8* iBuffer;
    
    TInt iBitmapHandle;
    
    // client thread from RMessage2
    RThread iClientThread;
};

#endif // THUMBNAILSERVERSESSION_H
