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
* Description:  Processor
 *
*/


#ifndef THUMBAGPROCESSOR_H
#define THUMBAGPROCESSOR_H

#include <thumbnailmanager.h>
#include <thumbnailmanagerobserver.h>
#include <thumbnaildata.h>
#include <mdesession.h>
#include <mdccommon.h>
#include <mdeobjectquery.h>
#include "thumbnaillog.h"
#include <harvesterclient.h>
#include <e32property.h>
#include <mpxcollectionobserver.h>
#include "tmactivitymanager.h"
#include "tmformatobserver.h"
#include "tmrpropertyobserver.h"
#include "thumbnailmanagerconstants.h"

//FORWARD DECLARATIONS
class MMPXCollectionUtility;


enum TThumbnailGenerationItemType
      {
      EGenerationItemTypeUnknown,
      EGenerationItemTypeNotFound,
      EGenerationItemTypeCamera,
      EGenerationItemTypeAny,
      EGenerationItemTypeAudio,
      EGenerationItemTypeVideo,
      EGenerationItemTypeImage,
      EGenerationItemTypeItemCount
      };

  enum TThumbnailGenerationItemAction
      {
      EGenerationItemActionAdd,
      EGenerationItemActionDelete,
      EGenerationItemActionModify,
      EGenerationItemAction2ndAdd,
      EGenerationItemActionResolveType,
      EGenerationItemActionCount
      };

  struct TThumbnailGenerationItem
      {
      inline TThumbnailGenerationItem(): iItemId( KErrNotFound ), iItemType(EGenerationItemTypeUnknown), 
              iItemAction(EGenerationItemActionAdd), iPlaceholder(EFalse), iUri(NULL){}
      inline ~TThumbnailGenerationItem(){delete iUri; iUri = NULL;}
      
      TItemId iItemId;
      TThumbnailGenerationItemType iItemType;
      TThumbnailGenerationItemAction iItemAction;
      TBool iPlaceholder;
      HBufC* iUri;
      };

/**
 *  Processor object for handling thumb generation
 *
 *  @since S60 v5.0
 */
class CThumbAGProcessor: public CActive,
                         public MThumbnailManagerObserver,
                         public MThumbnailManagerRequestObserver,
                         public MMdEQueryObserver,
                         public MHarvesterEventObserver,
                         public MMPXCollectionObserver,
                         public MTMActivityManagerObserver,
                         public MTMFormatObserver,
                         public MTMRPropertyObserver
    {
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return Instance of CThumbAGProcessor.
     */
    static CThumbAGProcessor* NewL();

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbAGProcessor();
    
public:
    
    // From MMdEQueryObserver
    void HandleQueryNewResults( CMdEQuery& aQuery,
                                TInt aFirstNewItemIndex,
                                TInt aNewItemCount );
    void HandleQueryCompleted( CMdEQuery& aQuery, TInt aError );
    
    // from MThumbnailManagerObserver
    void ThumbnailPreviewReady( MThumbnailData& aThumbnail, TThumbnailRequestId aId );
    void ThumbnailReady( TInt aError, MThumbnailData& aThumbnail, TThumbnailRequestId aId );
    // from MThumbnailManagerRequestObserver
    void ThumbnailRequestReady( TInt aError, TThumbnailRequestType aRequestType, TThumbnailRequestId aId  );

    // from MHarvesterEventObserver
    void HarvestingUpdated( 
             HarvesterEventObserverType aHEObserverType, 
             HarvesterEventState aHarvesterEventState,
             TInt aItemsLeft );
    
    //From MTMFormatObserver
    void FormatNotification(TBool aFormat);
    
private: 
    // From MMPXCollectionObserver
    /// See @ref MMPXCollectionObserver::HandleCollectionMessageL
    void HandleCollectionMessage( CMPXMessage* aMessage,  TInt aError );

    /// See @ref MMPXCollectionObserver::HandleOpenL
    void HandleOpenL(const CMPXMedia& aEntries, TInt aIndex, TBool aComplete, TInt aError);

    /// See @ref MMPXCollectionObserver::HandleOpenL
    void HandleOpenL(const CMPXCollectionPlaylist& aPlaylist, TInt aError);  
    
    /// See @ref MMPXCollectionObserver::HandleCollectionMediaL
    void HandleCollectionMediaL( const CMPXMedia& aMedia, TInt aError );
    
private: //From MTMActivityManagerObserver
    void ActivityChanged(const TBool aActive);
    
private: //From MTMRPropertyObserver
    void RPropertyNotification(const TInt aError, const TUid aKeyCategory, const TUint aPropertyKey, const TInt aValue);
    
public:     
    
    /**
     * Sets MdE Session
     *
     * @since S60 v5.0
     * @param aMdESession MdE Session
     */
    void SetMdESessionL( CMdESession* aMdESession );    
    
    /**
     * Adds new IDs to queue
     *
     * @since S60 v5.0
     * @param aType TObserverNotificationType
	 * @param TThumbnailGenerationItemType
     * @param aIDArray IDs for thumbnail creation
     * @param aForce pass ETrue if processor is forced to run without waiting harvesting complete
     */
    void AddToQueueL( TObserverNotificationType aType, TThumbnailGenerationItemType aItemType, 
            const RArray<TItemId>& aIDArray, const RPointerArray<HBufC>& aObjectUriArray, 
            TBool aPresent );
    
    /**
     * Calls Thumbnail Manager to create thumbnails
     *
     * @since S60 v5.0
     * @param aObject MdEObject
     */
    void CreateThumbnailsL( const CMdEObject* aObject ); 
    
    /**
     * Remove IDs from queue
     *
     * @since S60 v5.0
     * @param aIDArray IDs for thumbnail creation
     */
    void RemoveFromQueues( const RArray<TItemId>& aIDArray, const TBool aRemoveFromDelete = EFalse);
    
    void SetForceRun( const TBool aForceRun );
	
	//prepare processor for shutdown
	void Shutdown();
    
protected:
    
    void QueryAllItemsL();
    
    /**
     * QueryL
     *
     * @since S60 v5.0
     * @param TThumbnailGenerationItemAction
     */
    void QueryL( TThumbnailGenerationItemAction aType);
    
	 /**
     * QueryPlaceholdersL
     *
     * @since S60 v5.0
	 * @param aPresent item should not be present item
     */
    void QueryPlaceholdersL(const TBool aPresent);

   	 /**
     * DeleteAndCancelQuery
     *
     * @since Symbian^3
     * @param aRemoveItems restore IDs back to original queue, default = ETrue
     */
    void DeleteAndCancelQuery(TBool aRestoreItems);
    
protected:

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
     * Implements RunL error handling.
     *
     * @since S60 v5.0
     */
    TInt RunError(TInt aError);

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return Instance of CThumbAGProcessor.
     */
    CThumbAGProcessor();

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();
    
    /**
     * Activate AO
     *
     * @since S60 v5.0
     */
    void ActivateAO();
    
    /**
     * Callback for harvesting complete timer
     *
     * @since S60 v5.0
     */
    static TInt PeriodicTimerCallBack(TAny* aAny);
    
    /**
     * Check auto creation values from cenrep
     *
     * @since S60 v5.0
     */
    void CheckAutoCreateValuesL();
    
    /**
     * Start timeout timer
     *
     * @since S60 v5.0
     */
    void StartTimeout();
    
    /**
     * Cancel timeout timer
     *
     * @since S60 v5.0
     */
    void CancelTimeout();
   
    /**
     * Update KItemsLeft PS value
     * 
     * @since S60 v5.0
 	 * @param aDefine (re)define PS key before setting value
     */
    void UpdatePSValues(const TBool aDefine, const TBool aForce);
       
    /**
     * Comparison function for logaritmic use of queue arrays
     * 
     * @since S60 S^3
     */
    static TInt Compare(const TThumbnailGenerationItem& aLeft, const TThumbnailGenerationItem& aRight);
	
	 /**
     * Comparison function for logaritmic use of queue arrays
     * 
     * @since S60 S^3
     */
    static TInt CompareId(const TItemId& aLeft, const TItemId& aRight);
    
	/**
     * Recalculate item counts
     * 
     * @since S60 S^3
     */
    void UpdateItemCounts();
	
    /**
     * Callback for mount timeout
     *
     * @since S60 v5.0
     */
    static TInt MountTimerCallBack(TAny* aAny);
    
	 /**
     * Set item's action from type
     *
     * @since S^3
     */
    void SetGenerationItemAction( TThumbnailGenerationItem& aGenerationItem, const TThumbnailGenerationItemType aItemType );
	
     /**
     * Set item type property from TDedIf
     * @param aGenerationItem TThumbnailGenerationItem
     * @param aItemType TThumbnailGenerationItemType
     * @since S^3
     */
    void SetGenerationItemType( TThumbnailGenerationItem& aGenerationItem, const TDefId aDefId );
 
     /**
     * Append item to processing queue or update existing
     * @param aGenerationItem modified TThumbnailGenerationItem
     * @param aDefId items MDS object definition ID
     * @since S^3
     */
    void AppendProcessingQueue(TThumbnailGenerationItem& item );
        
private:
    
    // not own
    CMdESession* iMdESession;
    CMdENamespaceDef* iDefNamespace;
    
    // own
    CThumbnailManager* iTMSession;
    CMdEObjectQuery* iQuery;
    CMdEObjectQuery* iQueryAllItems;
    CMdEObjectQuery* iQueryPlaceholders;
    
    RArray<TThumbnailGenerationItem> iGenerationQueue;
    //RPointerArray<HBufC> iRemoveQueue;
    RArray<TItemId> iQueryQueue;
    
	//background generation state
	// EFalse = 1st round, create only grid size thumbnail for images and videos
	// ETrue = 2nds round, create all missing sizes for all media items
    TBool i2ndRound;    
	
	// query state
	// EFalse = normal mode
	// ETrue = querying unknown items
    TBool iUnknown;
    
	//MDS query issues
    TBool iQueryActive;
	//MDS query complete
    TBool iQueryReady;
    
	//Processing MDS itens which are modified
    TBool iModify;
    TUint iProcessingCount;

    //MDS harvester's overall state
    TBool iHarvesting;
    TBool iHarvestingTemp;

    //MDS Harvester's placeholder harvesting state
    TBool iPHHarvesting;
    TBool iPHHarvestingTemp;
    
    //MDS Harvester's SD card harvesting state
    TBool iMMCHarvesting;
    TBool iMMCHarvestingTemp;
    
    CPeriodic* iPeriodicTimer;
    CPeriodic* iMountTimer;

	//MDS Harvester client
    RHarvesterClient iHarvesterClient;

    //Set when running RunL() first time
    TBool iInit;
    
    //2nd phase init after MDE session is open
    TBool iInit2;
    
    // auto create values from centrep
    TBool iAutoImage;
    TBool iAutoVideo;
    TBool iAutoAudio;
    
    // in case of modified files force TN update
    TBool iForceRun; 
    // controlled by Photos application to run TN generation on foreground
    TBool iForegroundRun;
	//request pending in TNM side
   
    CTMFormatObserver* iFormatObserver;
   
   //formating started
    TBool iFormatting;
	//TNM server session died
    TBool iSessionDied;
   
   //pending request count
    TUint iActiveCount;
    
    MMPXCollectionUtility* iCollectionUtility; // own
    
	//Flag is MPX harvesting or MTP synchronisation in progress
    TBool iMPXHarvesting;

    //overall status of device
    TBool iIdle;
    
	//monitors device activity
    CTMActivityManager* iActivityManager;
    
	//Observer foreground generation 
    CTMRPropertyObserver* iForegroundGenerationObserver;
    
	//Previously notified amount of items in processing queues (add/modify)
    TInt iPreviousItemsLeft;
    TBool iPreviousDaemonProcessing; 
	//set ETrue when QueryAllItems needs to be run after placeholder query
    TBool iDoQueryAllItems;
    
    TBool iShutdown;
	
	//item counts
    TUint32 iModifyItemCount;
    TUint32 iImageItemCount;
    TUint32 iVideoItemCount;
    TUint32 iAudioItemCount;
    TUint32 iDeleteItemCount;
    TUint32 iAddItemCount;
    TUint32 iCameraItemCount;
    TUint32 iUnknownItemCount;
    TUint32 i2ndAddItemCount;
    TUint32 iPlaceholderItemCount;
    
    //for book keeping previous items left count got from MDS harvester
    TUint32 iMMCHarvestingItemsLeftTemp;
    TUint32 iPHHarvestingItemsLeftTemp;
    
	//MdE object definitons used in query
    CMdEObjectDef* iImageObjectDef;
    CMdEObjectDef* iVideoObjectDef;
    CMdEObjectDef* iAudioObjectDef;
    TBool iHarvesterActivated;
  };

#endif // THUMBAGPROCESSOR_H
