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
* Description:  Thumbnail server
 *
*/


#ifndef THUMBNAILSERVER_H
#define THUMBNAILSERVER_H

#include <e32base.h>
#include <fbs.h>
#include <w32std.h>
#include <apmrec.h>
#include <apgcli.h>
#include <e32hashtab.h>

#include <mdesession.h>

#include "thumbnailmanager.h"
#include "thumbnailmanagerconstants.h" // TThumbnailServerRequestId
#include "thumbnailstore.h"     // TThumbnailPersistentSize
#include "tmshutdownobserver.h"
#include "tmformatobserver.h"

// Needed for creating server thread.
const TUint KDefaultHeapSize = 160 * 1024;

class CThumbnailTaskProcessor;
class CThumbnailProvider;
class CThumbnailTask;
class CThumbnailStore;
class MIHLScaler;
class CThumbnailServerSession;
class CThumbnailDiskUnmountObserver;
class CThumbnailMemoryCardObserver;
class CThumbnailFetchedChecker;

/**
 * Helper function to destroy all objects which are referred to
 * by pointers in a hash table.
 * @since S60 v5.0
 * @param aHashMap Reference to hash map
 */
template <class K, class V> void ResetAndDestroyHashMap( RHashMap<K, V* > &
    aHashMap )
    {
    THashMapIter<K, V* > iter( aHashMap );

    // const pointer to a non-const object
    V* const * item = iter.NextValue();

    while ( item )
        {
        delete *item;
        item = iter.NextValue();
        }
    aHashMap.Close();
    }

/**
 *  Thumbnail server.
 *
 *  @since S60 v5.0
 */
NONSHARABLE_CLASS( CThumbnailServer ): public CPolicyServer,
                                       public MMdESessionObserver,
                                       public MTMShutdownObserver,
                                       public MTMFormatObserver
    {
public:

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @return New CThumbnailServer server.
     */
    static CThumbnailServer* NewL();

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailServer();

public:

    /**
     * Creates new server session.
     *
     * @since S60 v5.0
     * @param aVersion Version info.
     * @param aMessage Message to be passed.
     * @return New session.
     */
    CSession2* NewSessionL( const TVersion& aVersion, const RMessage2& aMessage
        )const;

    /**
     * ThreadFunctionL
     *
     * @since S60 v5.0
     */
    static void ThreadFunctionL();

    /**
     * AddSession
     *
     * @since S60 v5.0
     */
    void AddSession();

    /**
     * DropSession
     *
     * @since S60 v5.0
     */
    void DropSession(CThumbnailServerSession* aSession);
    
    // from MMdESessionObserver
    void HandleSessionOpened( CMdESession& aSession, TInt aError );
    void HandleSessionError( CMdESession& aSession, TInt aError );    
    
    // from MTMShutdownObserver
    void ShutdownNotification();

    //From MTMFormatObserver
    void FormatNotification(TBool aFormat);    
    
    /**
     * Adds bitmap to bitmap pool. Server assumes ownership of the bitmap and
     * implements reference counting to know when it is safe to delete
     * the object. Reference count is increased if the bitmap already
     * exists in the pool.
     *
     * @since S60 v5.0
     * @param aSession Server side session which owns the bitmap.
     * @param aBitmap Bitmap to be added to pool.
     * @param aRequestId Session specific thumbnail request ID.
     */
    void AddBitmapToPoolL( CThumbnailServerSession* aSession, CFbsBitmap*
        aBitmap, TThumbnailServerRequestId aRequestId );

    /**
     * Store thumbnail.
     *
     * @since S60 v5.0
     * @param aPath Image path.
     * @param aBitmap Thumbnail image.
     * @param aOriginalSize Original size of image.
     * @param aCropped Enabled if the thumbnail was cropped.
     * @param aThumbnailSize Prededined size of requested thumbnail.
     * @param aThumbFromPath Thumbnail created from associated path.
     * @param aThumbFromPath Check first if source uri exists.
     */
    void StoreThumbnailL( const TDesC& aPath, CFbsBitmap* aBitmap, const TSize&
        aOriginalSize, const TBool aCropped, const TThumbnailSize aThumbnailSize,
        const TInt64 aModified, const TBool aThumbFromPath,
        const TBool aCheckExist);

    /**
     * Fetch thumbnail image.
     *
     * @since S60 v5.0
     * @param aPath Path of the media object whose thumbnail is to be
     *              retrieved.
     * @param aThumbnail Pointer to get the fetched thumbnail bitmap.
     *                   Caller assumes ownership.
     * @param aMinimumSize Minimum size of the thumbnail.
     * @param aOriginalSize Original size of the object file in pixels
     * @param aCropped If true, match cropped images only.
     * @param aAllowUpscaling If enabled, allow fetching thumbnails that
     *                        are smaller than requested.
     * @param aThumbnailSize Prededined size of requested thumbnail.
     */
    void FetchThumbnailL( const TDesC& aPath, CFbsBitmap* & aThumbnail, 
             TDesC8* & aData, const TThumbnailSize aThumbnailSize, TSize &aOriginalSize );
    
    /**
     * Delete thumbnails.
     *
     * @since S60 v5.0
     * @param aPath           Path of the media object whose thumbnail is
     *                        to be deleted.
     */
    void DeleteThumbnailsL( const TDesC& aPath );

    /**
     * Delete thumbnails by Id.
     *
     * @since S60 v5.0
     * @param aItemId         Id of item whose thumbnails are to be deleted.
     */
    void DeleteThumbnailsByIdL( TThumbnailId aItemId );    
    
    /**
     * Resolve the MIME-type of given file using recognizer framework.
     *
     * @since S60 v5.0
     * @param aFile File to be recognized.
     * @return MIME-type
     */
    TDataType ResolveMimeTypeL( RFile64& aFile );

    /**
     * Removes bitmap from bitmap pool. Reference count is decremented
     * and if it reaches zero, the bitmap object is deleted.
     *
     * @since S60 v5.0
     * @param aHandle Bitmap handle.
     */
    void DeleteBitmapFromPool( TInt aHandle );

    /**
     * Resolve plugin to be used in thumbnail creation by mime type.
     *
     * @since S60 v5.0
     * @param aMimeType Mime type of the file from which a thumbnail is to be
     *                  created.
     * @return Thumbnail provider.
     */
    CThumbnailProvider* ResolveProviderL( const TDesC8& aMimeType );
    
    /**
     * Preload provider plugins
     *
     * @since S^3
     */
    void PreLoadProviders();
    

    /**
     * Add thumbnailtask to processor queue.
     *
     * @since S60 v5.0
     * @param aTask Task to be added to queue.
     */
    void QueueTaskL( CThumbnailTask* aTask );

    /**
     * Remove thumbnailtask from processor queue.
     *
     * @since S60 v5.0
     * @param aRequestId ID of the task to be removed from queue.
     * @return Error code.
     */
    TInt DequeTask( const TThumbnailServerRequestId& aRequestId );

    /**
     * Change priority of specific task.
     *
     * @since S60 v5.0
     * @param aRequestId ID of the task to be assigned new priority.
     * @param aNewPriority New priority for the task.
     * @return Error code.
     */
    TInt ChangeTaskPriority( const TThumbnailServerRequestId& aRequestId, TInt
        aNewPriority );

    /**
     * Scale bitmap
     *
     * @since S60 v5.0
     * @param aStatus      Request status.
     * @param aSource      Source bitmap.
     * @param aDest        Destination bitmap.
     * @param aSourceRect  Source rectangle for cropping.
     */
    void ScaleBitmapL( TRequestStatus& aStatus, const CFbsBitmap& aSource,
        CFbsBitmap& aDest, const TRect& aSourceRect );

    /**
     * Cancel scaling operation
     *
     * @since S60 v5.0
     */
    void CancelScale();

    /**
     * Returns processor.
     *
     * @since S60 v5.0
     * @return Reference to processor instance.
     */
    CThumbnailTaskProcessor& Processor();

    /**
     * Get persistent size.
     *
     * @since S60 v5.0
     * @param aThumbnailSize Relative thumbnail size 
     * @return Persistent size object
     */    
    TThumbnailPersistentSize & PersistentSizeL( TThumbnailSize
            aThumbnailSize ); 
    
    /**
     * Persistent sizes.
     *
     * @since S60 v5.0
     * @return List of thumbnail sizes (including othe parameters) which
     *         are stored for later access.
     */
    RArray < TThumbnailPersistentSize > PersistentSizesL();

    /**
     * Get missing thumbnail sizes.
     *
     * @since S60 v5.0
     * @param aPath Path associated to missing thumbnails
     * @param aMissingSizes Returns a list of thumbnail sizes not yet create related to the path
	 * @param aCheckGridSizeOnly check only is Grid size missing
     */
    void GetMissingSizesL( const TDesC& aPath, TInt aSourceType, RArray <
        TThumbnailPersistentSize > & aMissingSizes, TBool aCheckGridSizeOnly);

    /**
     * Fileserver
     *
     * @since S60 v5.0
     * @return File server
     */
    RFs& Fs();

    /**
     * Get the required size (in characters) for a buffer that contains the
     * list of supported MIME types
     * 
     * @since S60 v5.0
     */
    TInt GetMimeTypeBufferSize()const;

    /**
     * Get the list of supported MIME types and store them in the buffer.
     *
     * @since S60 v5.0
     * @param aBuffer A buffer to receive the space delimited list of MIME
     *                types. Must be as least as large as the size returned
     *                by GetMimeTypeBufferSizeL()
     */
    void GetMimeTypeList( TDes& aBuffer )const;
    
    /**
     * Close store instance for given drive.
     *
     * @since S60 v5.0
     * @param aDrive Drive for which the store is used, such as EDriveC,
     *               EDriveE, etc.
     */
    void CloseStoreForDriveL( const TInt aDrive );     
    
    /**
     * React to change in memory card status.
     *
     * @since S60 v5.0     
     */
    void MemoryCardStatusChangedL();

    /**
     * Update thumbnails in database
     *
     * @since S60 v5.0
     * @param aPath         (New) path for the Thumbnail
     * @param aOrientation  Thumbnail orientation
     * @param aModified     Last modified
     * @param TBool         EFalse, if new thumbs need to be created
     */  
    TBool UpdateThumbnailsL( const TDesC& aPath,
                             const TInt aOrientation, const TInt64 aModified );
    
    /**
     * Rename thumbnails.
     *
     * @since S60 v5.0
     * @param aCurrentPath     Current path of the Thumbnail
     * @param aNewPath         New path for the Thumbnail
     */
    void RenameThumbnailsL( const TDesC& aCurrentPath, const TDesC& aNewPath );
    
    /** 
     * Resolve mime type from file extension
     *
     * @since S60 v5.0     
     * @param aFileName Image file
     * @param aMimeType Mime type
     * @return TInt KErrNone/KErrNotFound
     */
    TInt MimeTypeFromFileExt( const TDesC& aFileName, TDataType& aMimeType );
    
    /** 
     * Resolve source type from mime type
     *
     * @since S60 v5.0     
     * @param aMimeType Mime type
     * @return TInt TThumbnailPersistentSize enum
     */
    TInt SourceTypeFromMimeType( const TDataType& aMimeType );
    
    /** 
     * Resolve source type from size type
     *
     * @since S60 v5.0     
     * @param aSizeType size type enum
     * @return TInt TThumbnailPersistentSize enum
     */
    TInt SourceTypeFromSizeType( const TInt aSizeType );
    
    /** 
     * Is mime type supported
     *
     * @since S60 v5.0     
     * @param aMimeType Mime type
     * @return TBool supported
     */
    TBool SupportedMimeType( const TDataType& aMimeType );    
    
    /** 
     * Get MDS session
     *
     * @since S60 v5.0     
     * @return CMdESession
     */
    CMdESession* GetMdESession();
    
    /**
     * Get store instance for given path.
     *
     * @since S60 v5.0
     * @param aPath Path to a file
     * @return CThumbnailStore object
     */
    CThumbnailStore* StoreForPathL( const TDesC& aPath );  
    
    /**
     * Close Removable Stores
     *
     * @since S60 v5.0
     */  
    
    void CloseRemovableDrivesL();
    
    /**
     * Open Removable Stores
     *
     * @since S60 v5.o
     */  
    
    void OpenRemovableDrivesL();
    
    /**
     * Checks if formatting is ongoing
     *
     * @since S60 v5.o
     */ 
    
    TBool IsFormatting();
  
    inline CThumbnailFetchedChecker& FetchedChecker() const { return *iFetchedChecker; }
    
    /**
     * Check is path in public folder
     *
     * @param aPath
     * @ret 
     */
    TBool IsPublicPath( const TDesC& aPath );
    
protected: // Functions from base classes

    /**
    * From CPolicyServer.
    * Handles capability checking for certain server requests.
    * @param aMsg  Message which was received from the client side
    * @param aAction  
    * @param aMissing
    * @result return one of TCustomResult set {EPass, EFail, EAsync}.
    */
    CPolicyServer::TCustomResult CustomSecurityCheckL( 
        const RMessage2 &aMsg, TInt &aAction, TSecurityInfo &aMissing );

    /**
    * From CPolicyServer.
    * Handles failure
    * @param aMsg  Message which was received from the client side
    * @param aAction  
    * @param aMissing
    * @result return one of TCustomResult set {EPass, EFail, EAsync}.
    */
    CPolicyServer::TCustomResult CustomFailureActionL(
        const RMessage2 &aMsg, TInt aAction, const TSecurityInfo &aMissing );
    
private:

    /**
     * Get a provider plug-in by the given UID
     *
     * @since S60 v5.0
     * @param aImplUid UID of the provider plug-in.
     * @return Provider instance.
     */
    CThumbnailProvider* GetProviderL( const TUid& aImplUid );

    /**
     * Get store instance for given drive.
     *
     * @since S60 v5.0
     * @param aDrive Drive for which the store is used, such as EDriveC,
     *               EDriveE, etc.
     * @return CThumbnailStore object
     */
    CThumbnailStore* StoreForDriveL( const TInt aDrive );
 
    
    /**
     * Add unmount observer for each unmountable drive.
     *
     * @since S60 v5.0
     */
    void AddUnmountObserversL();
    
    /** 
     * Open store for each mounted drive.
     *
     * @since S60 v5.0     
     */
    void OpenStoresL();
    
    /**
     * Callback for reconnect timer
     *
     * @since S60 v5.0
     */
    static TInt ReconnectCallBack(TAny* aAny);
    
    /**
     * Start unmount timer
     *
     * @since S60 S^3
     */
    void StartUnmountTimeout( const TInt aDrive);
    
    /**
     * Callback for unmount timer
     *
     * @since S60 S^3
     */
    static TInt UnmountCallBack(TAny* aAny);
    
private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New CThumbnailServer instance.
     */
    CThumbnailServer();

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:
    /**
     * Reference counted pointer to a CFbsBitmap object used for the bitmap
     * pool.
     */
    struct TThumbnailBitmapRef
        {
public:
        /**
         * CThumbnailServerSession can be used to identify which client
         * session is responsible for calling ReleaseBitmap() for this
         * bitmap. Not own.
         */
        CThumbnailServerSession* iSession;

        /**
         * Reference count for this bitmap. If reference count reaches zero,
         * the bitmap is deleted by the server.
         */
        TInt iRefCount;

        /**
         * Bitmap object. Not owned directly by TThumbnailBitmapRef, but by
         * the server.
         * Not own.
         */
        CFbsBitmap* iBitmap;
        
        /**
         * Request Id 
         */
        TThumbnailRequestId iRequestId;        
        
        };

private:
    // data

    TBool iShutdown;
    CTMShutdownObserver* iShutdownObserver;
    
    /**
     * Fileserver, own.
     */
    RFs iFs;

    /**
     * Sessioncount.
     */
    TInt iSessionCount;

    /**
     * FbsSession.
     */
    RFbsSession iFbsSession;

    /**
     * Bitmap pool. Reference counted pointers to CFbsBitmaps stored
     * in a hash table with bitmap handle as the key.
     * Own.
     */
    RHashMap < TInt, TThumbnailBitmapRef > iBitmapPool;

    /**
     * Processor for running tasks.
     * Own.
     */
    CThumbnailTaskProcessor* iProcessor;

    /**
     * Central repository data handler
     * Own.
     */
    CThumbnailCenRep* iCenrep;

    /**
     * Recognizer.
     * Own.
     */
    RApaLsSession iRecognizer;

    /**
     * Result of recognition.
     */
    TDataRecognitionResult iResult;

    /**
     * Plug-in info.
     */
    RImplInfoPtrArray iPluginInfoArray;

    /**
     * Thumbnail providers. Stored in a hash table with the implementation
     * UID as the key.
     * Own.
     */
    RHashMap < TInt32, CThumbnailProvider*  > iProviders;

    /**
     * Scaler.
     * Own.
     */
    MIHLScaler* iScaler;

    /**
    * Array of unmount observers.
    */
    RPointerArray<CThumbnailDiskUnmountObserver> iUnmountObservers;
    
    CThumbnailMemoryCardObserver* iMMCObserver;
    
    CTMFormatObserver* iFormatObserver;

    /**
     * Databases for each drive, identified by drive (EDriveC, etc).
     * New databases and created on-the-fly as new requests are received.
     * Databases are closed when drives are dismounted.
     * Own.
     */
    RHashMap < TInt, CThumbnailStore*  > iStores;
    
    // own
    CMdESession* iMdESession;
    
    TBuf<KImeiBufferSize> iImei;
    
    /**
     * Persistent sizes.
     */
    RArray < TThumbnailPersistentSize > iPersistentSizes;
    
    TBool iFormatting;
    
    // reconnect timer
    CPeriodic* iReconnect;

    // unmount timer
    CPeriodic* iUnmount;
    
    // array of unmounte
    RArray < TInt > iUnmountedDrives;
    
#ifdef _DEBUG
    TUint32 iPlaceholderCounter;
#endif
    
    /**
     * Fetched checker.
     * Own.
     */
    CThumbnailFetchedChecker* iFetchedChecker;
    
    TBool iSessionError;
};

#endif // THUMBNAILSERVER_H
