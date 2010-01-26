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

//FORWARD DECLARATIONS
class CThumbAGFormatObserver;
class MMPXCollectionUtility;

/**
 *  Processor object for handling thumb generation
 *
 *  @since S60 v5.0
 */
class CThumbAGProcessor: public CActive,
                         public MThumbnailManagerObserver,
                         public MMdEQueryObserver,
                         public MHarvesterEventObserver,
                         public MMPXCollectionObserver
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

    // from MHarvesterEventObserver
    void HarvestingUpdated( 
             HarvesterEventObserverType aHEObserverType, 
             HarvesterEventState aHarvesterEventState,
             TInt aItemsLeft );
    
private: // From MMPXCollectionObserver
    /// See @ref MMPXCollectionObserver::HandleCollectionMessageL
    void HandleCollectionMessage( CMPXMessage* aMessage,  TInt aError );

    /// See @ref MMPXCollectionObserver::HandleOpenL
    void HandleOpenL(const CMPXMedia& aEntries, TInt aIndex, TBool aComplete, TInt aError);

    /// See @ref MMPXCollectionObserver::HandleOpenL
    void HandleOpenL(const CMPXCollectionPlaylist& aPlaylist, TInt aError);  
    
    /// See @ref MMPXCollectionObserver::HandleCollectionMediaL
    void HandleCollectionMediaL( const CMPXMedia& aMedia, TInt aError );

public:     
    
    /**
     * Sets MdE Session
     *
     * @since S60 v5.0
     * @param aMdESession MdE Session
     */
    void SetMdESession( CMdESession* aMdESession );    
    
    /**
     * Adds new IDs to queue
     *
     * @since S60 v5.0
     * @param aType TObserverNotificationType
     * @param aIDArray IDs for thumbnail creation
     * @param aForce pass ETrue if processor is forced to run without waiting harvesting complete
     */
    void AddToQueueL( TObserverNotificationType aType, const RArray<TItemId>& aIDArray, TBool aPresent );
    
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
    
    void SetFormat(TBool aStatus);
    
    void QueryForPlaceholdersL();

protected:
    
    /**
     * QueryL
     *
     * @since S60 v5.0
     * @param aIDArray Item IDs to query
     */
    void QueryL( RArray<TItemId>& aIDArray );
    
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

private:
    
    // not own
    CMdESession* iMdESession;
    CMdENamespaceDef* iDefNamespace;
    
    // own
    CThumbnailManager* iTMSession;
    CMdEObjectQuery* iQuery;
    CMdEObjectQuery* iQueryForPlaceholders;
    
    RArray<TItemId> iAddQueue;
    RArray<TItemId> iModifyQueue;
    RArray<TItemId> iRemoveQueue;
    RArray<TItemId> iPresentQueue;
    RArray<TItemId> iQueryQueue;
    
    RArray<TItemId> iTempModifyQueue;
    RArray<TItemId> iTempAddQueue;
    
    RArray<TItemId> iPlaceholderIDs;
    
    TBool iQueryActive;
    TBool iQueryReady;
    
    TBool iQueryForPlaceholdersActive;
    
    TBool iModify;
    TInt iProcessingCount;

    //Flag is MDS Harvester harevsting
    TBool iHarvesting;
    TBool iHarvestingTemp;
    
    CPeriodic* iPeriodicTimer;
    TBool iTimerActive;

	//MDS Harvester client
    RHarvesterClient iHarvesterClient;

    //Set when running RunL() first time
    TBool iInit;
    
    // auto create
    TBool iAutoImage;
    TBool iAutoVideo;
    TBool iAutoAudio;
    
#ifdef _DEBUG
    TUint32 iAddCounter;
    TUint32 iModCounter;
    TUint32 iDelCounter;
#endif

    TBool iForceRun; 
	//request pending in TNM side
    TBool iActive;
    //PS key to get info server's idle status
    RProperty iProperty;
    
    CThumbAGFormatObserver* iFormatObserver;
   
    TBool iFormatting;
    TBool iSessionDied;
   
    TInt iActiveCount;
    
    MMPXCollectionUtility* iCollectionUtility; // own
    
	//Flag is MPX harvesting or MTP synchronisation in progress
    TBool iMPXHarvesting;
};

#endif // THUMBAGPROCESSOR_H
