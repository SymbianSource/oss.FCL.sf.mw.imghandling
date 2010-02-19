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
* Description:  Store for thumbnails.
 *
*/


#ifndef THUMBNAILSTORE_H
#define THUMBNAILSTORE_H

#include <sqldb.h>
#include <e32base.h>
#include <f32file.h>
#include "thumbnailcenrep.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"

class RFs;
class CFbsBitmap;
class CThumbnailServer;

/**
 *  Database transaction
 *
 *  @since S60 v5.0
 */
class RThumbnailTransaction
    {
    enum TState
        {
        EOpen, EError, EClosed
    };
public:
    RThumbnailTransaction( RSqlDatabase& aDatabase );

    void BeginL();
    void Close();
    void CommitL();
    TInt Rollback();

private:
    RSqlDatabase& iDatabase;
    TState iState;
};


/**
* MMdSDiskSpaceNotifierObserver
* Observer interface for a disk space notifier.
*/
class MThumbnailStoreDiskSpaceNotifierObserver
    {
    public :
        /**
         * Called to notify the observer that disk space has crossed the specified threshold value.
         *
         * @param aDiskFull is disk full (freespace under specified threshold level)
         */
        virtual void HandleDiskSpaceNotificationL(TBool aDiskFull) = 0;
        
        /**
         * Called to if disk space notifier has an error situation.
         *
         * @param aError error code
         */
        virtual void HandleDiskSpaceError(TInt aError) = 0;

    };

/**
* CMSDiskSpaceNotifierAO.
* A disk space notifier class
*/
class CThumbnailStoreDiskSpaceNotifierAO : public CActive
    {
    public:
        enum TDiskSpaceNotifierState
            {
            ENormal,
            EIterate
            };

    public : // Constructors and destructors
        /**
         * Constructs a disk space notifier implementation.
         *
         * @param aThreshold minimum free disk space threshold level in bytes
         * @param aFilename filename which defines monitored drive's number
         * @return  implementation
         */
        static CThumbnailStoreDiskSpaceNotifierAO* NewL(
                MThumbnailStoreDiskSpaceNotifierObserver& aObserver, 
            TInt64 aThreshold, const TDesC& aFilename);
        
        /**
         * Constructs a disk space notifier implementation and leaves it
         * in the cleanup stack.
         *
         * @param aThreshold minimum free disk space threshold level in bytes
         * @return implementation
         */
        static CThumbnailStoreDiskSpaceNotifierAO* NewLC(        
                MThumbnailStoreDiskSpaceNotifierObserver& aObserver, 
            TInt64 aThreshold, const TDesC& aFilename );

        /**
        * Destructor.
        */
        virtual ~CThumbnailStoreDiskSpaceNotifierAO();

        TBool DiskFull() const;

    protected: // Functions from base classes
        /**
         * From CActive
         * Callback function.
         * Invoked to handle responses from the server.
         */
        void RunL();

        /**
         * From CActive
         * Handles errors that occur during notifying the observer.
         */
        TInt RunError(TInt aError);

        /**
         * From CActive
         * Cancels any outstanding operation.
         */
        void DoCancel();

    private: // Constructors and destructors

        /**
         * constructor
         */
        CThumbnailStoreDiskSpaceNotifierAO(
                MThumbnailStoreDiskSpaceNotifierObserver& aObserver,
            TInt64 aThreshold, TDriveNumber aDrive );

        /**
         * 2nd phase constructor
         * @param aThreshold minimum free disk space threshold level in bytes
         * @param aDrive monitored drive's number
         */
        void ConstructL();

    private: // New methods

        void StartNotifier();

        static TDriveNumber GetDriveNumberL( const TDesC& aFilename );

    private: // Data

        MThumbnailStoreDiskSpaceNotifierObserver& iObserver;
        
        RFs iFileServerSession;
        
        const TInt64 iThreshold;
        
        const TDriveNumber iDrive;
        
        TDiskSpaceNotifierState iState;
        
        TInt iIterationCount;
        
        TBool iDiskFull;
    };


/**
 *  Store for thumbnails.
 *
 *  @since S60 v5.0
 */
class CThumbnailStore: public CBase, public MThumbnailStoreDiskSpaceNotifierObserver
    {
    // Bitmasked Flags
    typedef enum 
    {
    KThumbnailDbFlagCropped = 1,
    KThumbnailDbFlagBlacklisted = 2,
    KThumbnailDbFlagDeleted = 4
    }TThumbnailDbFlags;
    
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @param aFs File server.
     * @param aDrive Drive the store used for
     * @param aCenter Pointer to cenrep data handler
     * @return New CThumbnailStore instance.
     */
    static CThumbnailStore* NewL( RFs& aFs, TInt aDrive, TDesC& aImei, CThumbnailServer* aServer );

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailStore();

    /**
     * Stores thumbnail image.
     *
     * @since S60 v5.0
     * @param aPath Path of the image from which thumbnail was created.
     * @param aThumbnail Thumbnail bitmap.
     * @param aOriginalSize Original size of image.
     * @param aCropped Enabled if image is cropped.
     * @param aThumbnailSize Prededined size of requested thumbnail.
     * @param aThumbFromPath Thumbnail created from associated path.
     */
    void StoreThumbnailL( const TDesC& aPath, CFbsBitmap* aThumbnail, const
        TSize& aOriginalSize, TBool aCropped, const TThumbnailSize aThumbnailSize,
        const TThumbnailId aThumbnailId = 0, const TBool aThumbFromPath = ETrue,
        TBool aBlackListed = EFalse );

    /**
     * Fetches thumbnail image.
     *
     * @since S60 v5.0
     * @param aPath           Path of the media object whose thumbnail is
     *                        to be retrieved.
     * @param aThumbnail      Pointer to get the fetched thumbnail bitmap.
     *                        Caller assumes ownership.
     * @param aData           Pointer to get the fetched thumbnail JPEG.
     *                        Caller assumes ownership.    

     * @param aAllowUpscaling If enabled, allow fetching thumbnails that
     *                        are smaller than requested.
	 * @param aThumbnailSize Prededined size of requested thumbnail.
	 * 
	 * @param aThumbnailSize Reference to real size of TN.
     */
    void FetchThumbnailL( const TDesC& aPath, 
            CFbsBitmap* & aThumbnail, 
            TDesC8* & aData, 
            const TThumbnailSize aThumbnailSize,
            TSize &aThumbnailRealSize 
            );
    
    /**
     * Fetches thumbnail image.
     *
     * @since S60 v5.0
     * @param aThumbnailId           Path of the media object whose thumbnail is
     *                        to be retrieved.
     * @param aThumbnail      Pointer to get the fetched thumbnail bitmap.
     *                        Caller assumes ownership.
     * @param aData           Pointer to get the fetched thumbnail JPEG.
     *                        Caller assumes ownership.                      
     * @param aThumbnailSize    Minimum size of the thumbnail
     * .
     * @param aThumbnailSize    Reference to real size of TN.
     * 
     * @return KErrNone, otherwise KErrNotFound if thumbnail not found from DB
     */    
    TInt FetchThumbnailL( TThumbnailId aThumbnailId, 
            CFbsBitmap*& aThumbnail, 
            TDesC8* & aData, 
            TThumbnailSize aThumbnailSize, 
            TSize &aThumbnailRealSize 
            );
    

    /**
     * Delete thumbnails.
     *
     * @since S60 v5.0
     * @param aPath           Path of the media object whose thumbnail is
     *                        to be deleted.
     */
    void DeleteThumbnailsL( const TDesC& aPath );
    
    /**
      * Delete thumbnails.
      *
      * @since S60 TB9.1
      * @param aTNId           Id of the media object whose thumbnail is
      *                        to be deleted.
      */
     void DeleteThumbnailsL( const TThumbnailId& aTNId );

    /**
     * Persistent sizes.
     *
     * @since S60 v5.0
     * @return List of thumbnail sizes (including othe parameters) which
     *         are stored for later access.
     */
     void SetPersistentSizes(const RArray < TThumbnailPersistentSize > &aSizes);

    /**
     * Get persistent sizes not yet in database
     *
     * @since S60 v5.0
     * @param aPath Path where missing sizes are associated
     * @param aMissingSizes List of missing thumbnail sizes
     */
    void GetMissingSizesAndIDsL( const TDesC& aPath, TInt aSourceType, RArray <
            TThumbnailPersistentSize > & aMissingSizes, TBool& aMissingIDs );
    
    /**
       * Get persistent sizes not yet in database
       *
       * @since S60 TB9.1
       * @param aId Id of TN where missing sizes are associated
       * @param aMissingSizes List of missing thumbnail sizes
       */
      void GetMissingSizesL( const TThumbnailId aId, RArray <
          TThumbnailPersistentSize > & aMissingSizes );
 
    /**
     * Find store for thumbnails.
     *
     * @since S60 TB9.1
     * @param aThumbnailId Id of thumbnails to be updated.
     */
    void FindStoreL(TThumbnailId aThumbnailId);
    
    /**
     * Updates path for thumbnails in current store.
     *
     * @since S60 v5.0
     * @param aItemId Id for thumbnails to be updated.
     * @param aNewPath New path for thumbnails.
     * @return ETrue, if path was updated
     */     
    TBool UpdateStoreL( TThumbnailId aItemId, const TDesC& aNewPath );
    
    /**
     * Updates path for thumbnails in current store.
     *
     * @since S60 v5.0
     * @param aItemId Id for thumbnails to be updated.
     * @param aNewPath New path for thumbnails.
     */  
    void UpdateStoreL( const TDesC& aPath, TThumbnailId aNewId );
    
    /**
     * Check modification timestamp
     *
     * @since S60 v5.0
     * @param aItemId Id for thumbnails to be updated.
     * @param aModified new MDS timestamp
     * @return ETrue, if given timestamp was newer than in DB
     */     
    TBool CheckModifiedL( const TThumbnailId aItemId, const TInt64 aModified );    
    
    /**
     * Fetches thumbnails from store to be moved.
     *
     * @since S60 v5.0
     * @param aItemId Id for thumbnails to be updated.
     * @param aThumbnails Array for thumbnails to be moved.
     */  
    void FetchThumbnailsL(TThumbnailId aItemId, RArray < TThumbnailDatabaseData* >& aThumbnails);
    
    /**
     * Stores thumbnails in to new store.
     *
     * @since S60 v5.0
     * @param aNewPath New path for thumbnails.
     * @param aThumbnails Array for thumbnails to be moved.
     */  
    void StoreThumbnailsL(const TDesC& aNewPath, RArray < TThumbnailDatabaseData* >& aThumbnails);
    
    /**
     * Stores thumbnails in to new store.
     *
     * @since S60 v5.0
     * @param aNewPath New path for thumbnails.
     * @param aThumbnails Array for thumbnails to be moved.
     */  
    TInt CheckImeiL();
    
    /**
     * Stores thumbnails in to new store.
     *
     * @since S60 v5.0
     * @param aNewPath New path for thumbnails.
     * @param aThumbnails Array for thumbnails to be moved.
     */  
    TInt CheckVersionL();
    
    /**
     * Stores thumbnails in to new store.
     *
     * @since S60 v5.0
     * @param aNewPath New path for thumbnails.
     * @param aThumbnails Array for thumbnails to be moved.
     */  
    TInt CheckMediaIDL();
    
    /**
     * Stores thumbnails in to new store.
     *
     * @since S60 v5.0
     * @param aNewPath New path for thumbnails.
     * @param aThumbnails Array for thumbnails to be moved.
     */  
    void AddVersionAndImeiL();
    
    /**
     * Stores thumbnails in to new store.
     *
     * @since S60 v5.0
     * @param aNewPath New path for thumbnails.
     * @param aThumbnails Array for thumbnails to be moved.
     */  
    void ResetThumbnailIDs();
    
    /**
     * Stores thumbnails in to new store.
     *
     * @since S60 v5.0
     * @param aNewPath New path for thumbnails.
     * @param aThumbnails Array for thumbnails to be moved.
     */
    void UpdateImeiL();
    
    /**
     * Checks that database rowids match.
     *
     * @since S60 v5.0
     */
    
    TInt CheckRowIDsL();
    
    TBool IsDiskFull();

private:
    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aFs File server.
     * @param aDrive Drive the store used for
     * @return New CThumbnailStore instance.
     */
    CThumbnailStore( RFs& aFs, TInt aDrive, TDesC& aImei, CThumbnailServer* aServer);

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

    /**
     * Create database tables.
     *
     * @since S60 v5.0
     */
    void CreateTablesL();

    /**
     * Stores thumbnail image.
     *
     * @since S60 v5.0
     * @param aPath Path of the image from which thumbnail was created.
     * @param aData Data.
     * @param aSize Size of thumbnail.
     * @param aOriginalSize Original size of image.
     * @param aFormat Format of the image.
     * @param aFlags Flags.
     * @param aThumbnailSize Associated size of the thumbnail to be deleted
     * @param aThumbFromPath Thumbnail created from associated path.
     */
    void StoreThumbnailL( const TDesC& aPath, const TDes8& aData, const TSize&
        aSize, const TSize& aOriginalSize, const TThumbnailFormat& aFormat, TInt aFlags, 
        const TThumbnailSize& aThumbnailSize, const TThumbnailId aThumbnailId = 0,
        const TBool aThumbFromPath = ETrue);

    /**
     * Finds possible existing duplicate thumbnail.
     *
     * @since S60 v5.0
     * @param aPath Path of the image from which thumbnail was created.
     * @param aThumbnailId ID of the thumbnail
     * @param aThumbnailSize Associated size of the thumbnail to be deleted
     */
    TBool FindDuplicateL( const TDesC& aPath, const TThumbnailId aThumbnailId,
                          const TThumbnailSize& aThumbnailSize );    
    
    /**
     * Flush RAM cache containing generated TNs to persistent storage.
     *
     * @since S60 TB9.1
     * @param aForce which forces logic to flush cache table.
     */
    void FlushCacheTable( TBool aForce = EFalse );
    
    /**
     * Start db auto flush timer 
     *
     * @since S60 TB9.1
     */
    void StartAutoFlush();
    
    /**
     * Stop db auto flush timer 
     *
     * @since S60 TB9.1
     */
    void StopAutoFlush();
    
    /**
     * Callback for harvesting complete timer
     *
     * @since S60 v5.0
     */
    static TInt AutoFlushTimerCallBack(TAny* aAny);
    
    /**
     * Checks timestamp of blacklisted entry to timestamp of file, from
     * which thumbnail entry was created, in filesystem
     *
     * @param aPath Path from which thumbnail created
     * @param aTempTable Indication whether data in temp table
     * @param aModified On return contains indication whether file modified
     */
    void CheckModifiedByPathL( const TDesC& aPath, TBool aTempTable, TBool& aModified  );
    
    /**
     * Checks timestamp of blacklisted entry to timestamp of file, from
     * which thumbnail entry was created, in filesystem
     *
     * @param aId Thumbnail id
     * @param aTempTable Indication whether data in temp table
     * @param aModified On return contains indication whether file modified
     */
    void CheckModifiedByIdL( TUint32 aId, TBool aTempTable, TBool& aModified  );
    
    /**
    * Touches blacklisted items
    *
    */
    void PrepareBlacklistedItemsForRetry();
    
public : // From MThumbnailStoreDiskSpaceNotifierObserver
    void HandleDiskSpaceNotificationL(TBool aDiskFull);

    void HandleDiskSpaceError(TInt aError);
    
private:
    // data

    /**
     * Fileserver.
     */
    RFs& iFs;

    /**
     * Drive number
     */
    TInt iDrive;

    /**
     * Thumbnail database.
     */
    RSqlDatabase iDatabase;

    /**
     * Persistent sizes.
     */
    RArray < TThumbnailPersistentSize > iPersistentSizes;
       
    /**
     * Count of cached TNs not yet committed to db
     */
    TInt iBatchItemCount;
    
    /**
     * Phones IMEI code
     */
    TDesC& iImei;
    
    /**
     * ThumbnailServer
     */
    
    CThumbnailServer* iServer;
  
#ifdef _DEBUG
    TUint32 iThumbCounter;
#endif
    /**
     * Periodic timer handling automatic flushing of db cache
     */
    CPeriodic* iAutoFlushTimer;
    
    /**
    * Notifier for situations where free disk space runs out.
    */
    CThumbnailStoreDiskSpaceNotifierAO* iDiskFullNotifier;
    
    TBool iDiskFull;
};
// End of File


#endif // THUMBNAILSTORE_H
