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
#include "tmactivitymanager.h"

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
        EOpen, EOldOpen, EError, EClosed
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
* MThumbnailStoreDiskSpaceNotifierObserver
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
* CThumbnailStoreDiskSpaceNotifierAO.
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
class CThumbnailStore: public CBase, 
                       public MThumbnailStoreDiskSpaceNotifierObserver,
                       public MTMActivityManagerObserver
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
	 * @param aReadOnly flag is store write protected
     * @return New CThumbnailStore instance.
     */
    static CThumbnailStore* NewL( RFs& aFs, TInt aDrive, TDesC& aImei, CThumbnailServer* aServer, const TBool aReadOnly );

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
        const TInt64 aModified, const TBool aThumbFromPath, TBool aBlackListed);

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
     * Delete thumbnails.
     *
     * @since S60 v5.0
     * @param aPath           Path of the media object whose thumbnail is
     *                        to be deleted.
     * @param aForce          Force to delete instantly 
     * @param aTransaction    Create a transaction 
     */
    void DeleteThumbnailsL( const TDesC& aPath, TBool aForce = EFalse,
                            TBool aTransaction = ETrue);
    
    /**
     * Rename thumbnails.
     *
     * @since S60 v5.0
     * @param aCurrentPath     Current path of the Thumbnail
     * @param aNewPath         New path for the Thumbnail
     */
    void RenameThumbnailsL( const TDesC& aCurrentPath, const TDesC& aNewPath );    
    
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
     * @param aCheckGridSizeOnly check only is grid size missing
     */
    void GetMissingSizesL( const TDesC& aPath, TInt aSourceType, RArray <
            TThumbnailPersistentSize > & aMissingSizes, TBool aCheckGridSizeOnly  );
        
    /**
     * Check IMEI (owner) of db
     *
     * @since S60 v5.0
     */  
    TInt CheckImei();
    
    /**
     * Check version of db
     *
     * @since S60 v5.0
     */  
    TInt CheckVersion();
    
    /**
     * Check mediaid of store
     *
     * @since S60 v5.0
     */  
    TInt CheckMediaIDL();
    
    /**
     * Add version and IMEI to db
     *
     * @since S60 v5.0
     */  
    void AddVersionAndImeiL();
    
    /**
     * Update IMEI to db
     *
     * @since S60 v5.0
     */
    void UpdateImeiL();
    
    /**
     * Checks that database rowids match.
     *
     * @since S60 v5.0
     */
    
    TInt CheckRowIDs();
    
    /**
     * Check is disk full
     *
     * @since S60 v5.0
     */
    TBool IsDiskFull();
    
    /**
     * Checks timestamp of blacklisted entry to timestamp of file, from
     * which thumbnail entry was created, in filesystem
     *
     * @param aPath Path from which thumbnail created
     * @param aModified current timestampt
     * @contains indication whether file modified
     */
    TBool CheckModifiedByPathL( const TDesC& aPath, const TInt64 aModified, TBool& modifiedChanged);
    
    /**
     *  @return TBool is store write protected
     */
    TBool IsReadOnly();

private:
    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aFs File server.
     * @param aDrive Drive the store used for
	 * @param aReadOnly set flag if store is write protected
     * @return New CThumbnailStore instance.
     */
    CThumbnailStore( RFs& aFs, TInt aDrive, TDesC& aImei, CThumbnailServer* aServer, const TBool aReadOnly);

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();
    
    /**
     * Starts constructing database
     *
     * @since S60 v5.0
     */
    void PrepareDbL();
    
    /**
     * Open database
     *
     * @since S60 v5.0
     * @param aNewDatabase Delete existing before creating new
     */
    TInt OpenDatabaseL( TBool aNewDatabase = EFalse);
    
    /**
     * Open database
     *
     * @since S60 v5.0
     */
    TInt OpenDatabaseFileL();

    /**
     * Construct database tables
     *
     * @since S60 v5.0
     */
    void CreateTablesL();
    void CreateTempTablesL();
    
    /**
     * Delete and create database
     *
     * @since S60 v5.0
     * @param aDelete Delete old db
     */
    void RecreateDatabaseL( const TBool aDelete );

    /**
     * Prepare, reset & close statements
     *
     * @since S60 v5.0
     */
    void PrepareStatementsL();    
    static void ResetStatement( TAny* aStmt );
    void CloseStatements();
    
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
        const TThumbnailSize& aThumbnailSize, const TInt64 aModified,
        const TBool aThumbFromPath);

    /**
     * Finds possible existing duplicate thumbnail.
     *
     * @since S60 v5.0
     * @param aPath Path of the image from which thumbnail was created.
     * @param aThumbnailId ID of the thumbnail
     * @param aThumbnailSize Associated size of the thumbnail to be deleted
     */
    TBool FindDuplicateL( const TDesC& aPath, const TThumbnailSize& aThumbnailSize );    
    
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
    
    void StartMaintenance();
    
    /**
     * Callback for harvesting complete timer
     *
     * @since S60 v5.0
     */
    static TInt AutoFlushTimerCallBack(TAny* aAny);
    
    /**
     * Callback for maintenance timer
     *
     * @since S60 v5.0
     */
    static TInt MaintenanceTimerCallBack(TAny* aAny);
    
    /**
    * Touches blacklisted items
    *
    */
    void PrepareBlacklistedItemsForRetryL();
    
    /**
    * Deletes thumbs added to Deleted table
    *
    */
    TInt DeleteMarkedL();
    
    /**
    * Checks if thumbnail source files are still in the file system.
    * If not, delete corresponding thumbs.
    *
    * @return ETrue, if finished.
    */
    TBool FileExistenceCheckL();
    
    /**
    * Checks if thumbnail database is usable
    *
    * @return KErrNone, if no problems
    */
    TInt CheckDbState();
    
    /**
     * Strips drive letter from URI.
     *
     * @since S60 v5.0
     * @param aPath Path.
     */
    void StripDriveLetterL( TDes& aPath );
    
public : // From MThumbnailStoreDiskSpaceNotifierObserver
    void HandleDiskSpaceNotificationL(TBool aDiskFull);

    void HandleDiskSpaceError(TInt aError);
    
private: //From MTMActivityManagerObserver
    void ActivityChanged(const TBool aActive);
    
private:
    // data

    /**
     * Fileserver.
     */
    RFs& iFs;

    /**
     * Drive number & char
     */
    TInt iDrive;
    TChar iDriveChar;

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
     * Periodic timer handling db maintenance
     */
    CPeriodic* iMaintenanceTimer; 
	   
    /**
    * Notifier for situations where free disk space runs out.
    */
    CThumbnailStoreDiskSpaceNotifierAO* iDiskFullNotifier;
    
    TBool iDiskFull;
    
    CTMActivityManager* iActivityManager;
    
    // device idle
    TBool iIdle;
    
    // delete thumbs
    TBool iDeleteThumbs;
    
    // check if thumb source files still exist
    TBool iCheckFilesExist;
    TInt64 iLastCheckedRowID;
    
    // store is in a state in which db can't be used
    TBool iUnrecoverable;
    
    // prepared statements
    RSqlStatement iStmt_KThumbnailSelectInfoByPath;
    RSqlStatement iStmt_KThumbnailSelectTempInfoByPath;
    RSqlStatement iStmt_KThumbnailInsertTempThumbnailInfo;
    RSqlStatement iStmt_KThumbnailInsertTempThumbnailInfoData;
    RSqlStatement iStmt_KThumbnailSelectModifiedByPath;
    RSqlStatement iStmt_KThumbnailSelectTempModifiedByPath;
    RSqlStatement iStmt_KThumbnailFindDuplicate;
    RSqlStatement iStmt_KThumbnailTempFindDuplicate;
    RSqlStatement iStmt_KThumbnailSqlFindDeleted;
    RSqlStatement iStmt_KThumbnailSelectSizeByPath;
    RSqlStatement iStmt_KThumbnailSelectTempSizeByPath;
    RSqlStatement iStmt_KThumbnailSqlSelectRowIDInfoByPath;
    RSqlStatement iStmt_KThumbnailSqlDeleteInfoByPath;
    RSqlStatement iStmt_KThumbnailSqlDeleteInfoDataByPath;
    RSqlStatement iStmt_KTempThumbnailSqlSelectRowIDInfoByPath;
    RSqlStatement iStmt_KTempThumbnailSqlDeleteInfoByPath;
    RSqlStatement iStmt_KTempThumbnailSqlDeleteInfoDataByPath;
    RSqlStatement iStmt_KThumbnailSqlInsertDeleted;
    RSqlStatement iStmt_KThumbnailSqlSelectMarked;
    RSqlStatement iStmt_KThumbnailSqlDeleteInfoByRowID;
    RSqlStatement iStmt_KThumbnailSqlDeleteInfoDataByRowID;
    RSqlStatement iStmt_KThumbnailSelectAllPaths;
    RSqlStatement iStmt_KThumbnailRename;
    RSqlStatement iStmt_KThumbnailTempRename;
    
    /**
     * Dynamic batch size
     */
    TInt iBatchFlushItemCount;

    /**
     * Measure time spend in flush
     */
    TTime iStartFlush, iStopFlush;
	/**
     * How long previous flush took ms
     */
    TInt iPreviousFlushDelay;
    
    /**
     * is store write protected
     */
    TBool iReadOnly;

};
// End of File


#endif // THUMBNAILSTORE_H
