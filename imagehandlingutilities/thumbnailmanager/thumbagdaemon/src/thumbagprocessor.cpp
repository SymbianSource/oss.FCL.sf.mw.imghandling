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


#include <e32base.h>

#include <mdeconstants.h>
#include <centralrepository.h>

#include <mpxcollectionutility.h>
#include <mpxmessagegeneraldefs.h>
#include <mpxcollectionmessage.h>
#include <coreapplicationuisdomainpskeys.h> 

#include "thumbagprocessor.h"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbagprocessorTraces.h"
#endif

// ---------------------------------------------------------------------------
// CThumbAGProcessor::NewL()
// ---------------------------------------------------------------------------
//
CThumbAGProcessor* CThumbAGProcessor::NewL()
    {
    TN_DEBUG1( "CThumbAGProcessor::NewL() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_NEWL, "CThumbAGProcessor::NewL - begin" );
    
    CThumbAGProcessor* self = new( ELeave )CThumbAGProcessor();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::CThumbAGProcessor()
// ---------------------------------------------------------------------------
//
CThumbAGProcessor::CThumbAGProcessor(): CActive( CActive::EPriorityStandard ), iMMCHarvestingItemsLeftTemp(0), 
        iPHHarvestingItemsLeftTemp(0)
    {
    TN_DEBUG1( "CThumbAGProcessor::CThumbAGProcessor() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_CTHUMBAGPROCESSOR, "CThumbAGProcessor::CThumbAGProcessor- begin" );
    
    CActiveScheduler::Add( this );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::ConstructL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::ConstructL()
    {
    TN_DEBUG1( "CThumbAGProcessor::ConstructL() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_CONSTRUCTL, "CThumbAGProcessor::ConstructL - begin" );
    
    iShutdown = EFalse;
    
    iTMSession = CThumbnailManager::NewL( *this );
    iTMSession->SetRequestObserver(*this);
    
    iQueryAllItems = NULL;
    iQueryPlaceholders = NULL;
    iQuery = NULL;
    iQueryActive = EFalse;
    iModify = EFalse;
    iProcessingCount = 0;
    
    iActiveCount = 0;
    
    // set auto create values from cenrep
    CheckAutoCreateValuesL();
    
    iPeriodicTimer = CPeriodic::NewL(CActive::EPriorityStandard);
    
    iMountTimer = CPeriodic::NewL(CActive::EPriorityUserInput);
    
    SetForceRun( EFalse );    
    
    iFormatObserver = CTMFormatObserver::NewL( *this );
       
    iFormatting = EFalse;     
    iSessionDied = EFalse;
    
    iCollectionUtility = NULL;
    
    iActivityManager = CTMActivityManager::NewL( this, KBackgroundGenerationIdle);

    UpdatePSValues(ETrue, ETrue);
    
    RProperty::Define(KTAGDPSNotification, KMPXHarvesting, RProperty::EInt);
    
	//start foreground generation observer
    iForegroundGenerationObserver = CTMRPropertyObserver::NewL( *this, KTAGDPSNotification, KForceBackgroundGeneration, ETrue );  
    
    TN_DEBUG1( "CThumbAGProcessor::ConstructL() - end" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_CONSTRUCTL, "CThumbAGProcessor::ConstructL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::~CThumbAGProcessor()
// ---------------------------------------------------------------------------
//
CThumbAGProcessor::~CThumbAGProcessor()
    {
    TN_DEBUG1( "CThumbAGProcessor::~CThumbAGProcessor() - begin" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_CTHUMBAGPROCESSOR, "CThumbAGProcessor::~CThumbAGProcessor - begin" );
    
    Shutdown();
    
    Cancel();
    
    if (iTMSession)
        {
        iTMSession->RemoveRequestObserver();
        delete iTMSession;
        iTMSession = NULL;
        }
    
    if(iMountTimer)
        {
        iMountTimer->Cancel();
        delete iMountTimer;
        iMountTimer = NULL;
        }
    
    if(iPeriodicTimer)
        {
        iPeriodicTimer->Cancel();
        delete iPeriodicTimer;
        iPeriodicTimer = NULL;
        }
    
    if(iActivityManager)
        {
        delete iActivityManager;
        iActivityManager = NULL;
        }
    
    if (iQuery)
        {
        iQuery->Cancel();
        delete iQuery;
        iQuery = NULL;
        }
    
    if(iQueryPlaceholders)
        {
        iQueryPlaceholders->Cancel();
        delete iQueryPlaceholders;
        iQueryPlaceholders = NULL;
        }
    
    if (iQueryAllItems)
       {
       iQueryAllItems->Cancel();
       delete iQueryAllItems;
       iQueryAllItems = NULL;
       }
    
    if (!iInit)
        {
#ifdef MDS_MODIFY_OBSERVER
        iHarvesterClient.RemoveHarvesterEventObserver(*this);
        iHarvesterClient.Close();
#endif
        }
    
    if(iForegroundGenerationObserver)
        {
        delete iForegroundGenerationObserver;
        iForegroundGenerationObserver = NULL;
        }
    
    if(iFormatObserver)
        {
        delete iFormatObserver;
        iFormatObserver = NULL;
        }
    
    if ( iCollectionUtility )
        {
        iCollectionUtility->Close();
        iCollectionUtility = NULL;
        }
    
    for(TInt i=0;i<iGenerationQueue.Count();i++)
        {
        delete iGenerationQueue[i].iUri;
        iGenerationQueue[i].iUri = NULL;
        }
    
    iGenerationQueue.Reset();
    iGenerationQueue.Close();
    
    iQueryQueue.Close();
    
    TN_DEBUG1( "CThumbAGProcessor::~CThumbAGProcessor() - end" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_CTHUMBAGPROCESSOR, "CThumbAGProcessor::~CThumbAGProcessor - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::Shutdown()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::Shutdown()
    {
    TN_DEBUG1( "CThumbAGProcessor::Shutdown()" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_SHUTDOWN, "CThumbAGProcessor::Shutdown" );
    iShutdown = ETrue;
    UpdatePSValues(EFalse, EFalse);
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleQueryNewResults()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::HandleQueryNewResults( CMdEQuery& aQuery,
                                               const TInt aFirstNewItemIndex,
                                               const TInt aNewItemCount )
    {
    // PH & AllItems query results are handled here
    
    if(iShutdown)
        {
        return;
        }
    
    if (aNewItemCount > 0)
        {
        if(&aQuery == iQueryPlaceholders)
            {
            TN_DEBUG2( "CThumbAGProcessor::HandleQueryNewResults - iQueryPlaceholders, %d new", aNewItemCount);
            OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_HANDLEQUERYNEWRESULTS, "CThumbAGProcessor::HandleQueryNewResults - iQueryPlaceholders;aNewItemCount=%d", aNewItemCount );
            
            for(TInt i = aFirstNewItemIndex; i < iQueryPlaceholders->Count(); i++)
                {    
                const CMdEObject* object = &iQueryPlaceholders->Result(i);
              
                if(!object)
                    {
                    continue;
                    }
              
                if(!object->Placeholder())
                    {
                    TN_DEBUG2( "CThumbAGProcessor::HandleQueryNewResults %d not placeholder", object->Id());
                    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_HANDLEQUERYNEWRESULTS, "CThumbAGProcessor::HandleQueryNewResults - not placeholder;object->Id()=%u", object->Id() );
                    continue;
                    }
               
                // ignore if fails
                TThumbnailGenerationItem item;
                item.iItemId = object->Id();
                item.iPlaceholder = ETrue;
                
                SetGenerationItemType( item, object->Def().Id());
                
                AppendProcessingQueue( item );
                }  
            }
        else if(&aQuery == iQueryAllItems)
            {
            TN_DEBUG2( "CThumbAGProcessor::HandleQueryNewResults - QueryAllItems, %d new", aNewItemCount);
            OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_HANDLEQUERYNEWRESULTS, "CThumbAGProcessor::HandleQueryNewResults - QueryAllItems;aNewItemCount=%d", aNewItemCount );
            
            for(TInt i = aFirstNewItemIndex; i < iQueryAllItems->Count(); i++)
                {    
                const CMdEObject* object = &iQueryAllItems->Result(i);
               
                if(!object)
                    {
                    continue;
                    }

                TThumbnailGenerationItem item;
                item.iItemId = object->Id();
                
                SetGenerationItemType(item, object->Def().Id());
                
                TInt itemIndex = iGenerationQueue.FindInOrder(item, Compare);
                
                item.iPlaceholder = object->Placeholder(); 
                
                AppendProcessingQueue( item );
                }
            }    
        }
    else
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryNewResults - error, no new items");
        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_HANDLEQUERYNEWRESULTS, "CThumbAGProcessor::HandleQueryNewResults - error, no new items" );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleQueryCompleted()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::HandleQueryCompleted( CMdEQuery& aQuery, const TInt aError )
    {
    TN_DEBUG3( "CThumbAGProcessor::HandleQueryCompleted, aError == %d Count== %d", aError, aQuery.Count());
    OstTraceExt2( TRACE_NORMAL, CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted;aError=%d;aQuery.Count()=%d", aError, aQuery.Count() );
    
    if(iShutdown)
        {
        return;
        }
    
    if(&aQuery == iQueryPlaceholders)
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted - iQueryPlaceholders completed");
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - iQueryPlaceholders completed" );
        
        //free query
        delete iQueryPlaceholders;
        iQueryPlaceholders = NULL;
       
        if(iDoQueryAllItems)
            {
            iDoQueryAllItems = EFalse;
            TRAP_IGNORE(QueryAllItemsL());
            }
        }
    else if(&aQuery == iQueryAllItems)
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted - QueryAllItems completed");
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - QueryAllItems completed" );

#ifdef _DEBUG
TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted IN-COUNTERS---------- Amount: %d, Add",iQueryAllItems->Count());
OstTrace1( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted IN-COUNTERS---------- Amount;iQueryAllItems->Count()=%d", iQueryAllItems->Count() );
#endif
       
        //free query
        delete iQueryAllItems;
        iQueryAllItems = NULL;
        }
    else if(&aQuery == iQuery ) 
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted - Query completed");
        OstTrace0( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - Query completed" );
        
        if(iQueryActive)
            {
            iQueryReady = ETrue;
            iQueryActive = EFalse;
            }
    
        // if no errors in query
        if (aError == KErrNone && iQuery)
            {
            iProcessingCount = iQuery->Count();
			
            if(iProcessingCount != iQueryQueue.Count() )
                {
                TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() some result items missing");
                OstTrace0( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - some result items missing" );
                
                RArray<TItemId> queryQueueDelta;
                
                    //search delta items which were queried, but not found
                     for(TInt queryItem =0; queryItem < iQueryQueue.Count();queryItem++)
                         {
                         TBool found(EFalse);
                         for(TInt queryResult = 0; queryResult < iQuery->Count(); queryResult++)
                            {    
                            const CMdEObject* object = &iQuery->Result(queryResult);
                            
                            if( iQueryQueue[queryItem] == object->Id())
                                {
                                found = ETrue;
                                break;
                                }
                            }
                             
                         if(!found)
                             {
                             TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() %d missing from query results", iQueryQueue[queryItem] );
                             OstTrace1( TRACE_NORMAL, DUP6_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted;iQueryQueue[queryItem]=%d", iQueryQueue[queryItem] );
                             
                             // ignore if fails
                             queryQueueDelta.InsertInOrder(iQueryQueue[queryItem], CompareId);
                             }
                         }
                     
                     TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() missing items found %d", queryQueueDelta.Count()); 
                     OstTrace1( TRACE_NORMAL, DUP7_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - missing items found;queryQueueDelta.Count()=%d", queryQueueDelta.Count() );
                     //cleanup from previous queue it item is not found from MDS
                     while(queryQueueDelta.Count())
                         {
                         TThumbnailGenerationItem item;
                         item.iItemId = queryQueueDelta[0];
                         TInt itemIndex = iQueryQueue.FindInOrder(item.iItemId, CompareId);
                         
                         if(itemIndex >= 0)
                             {
                             TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() remove %d from iQueryQueue", queryQueueDelta[0]);
                             OstTrace1( TRACE_NORMAL, DUP8_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - remove from iQueryQueue;queryQueueDelta[0]=%d", queryQueueDelta[0] );
                             iQueryQueue.Remove( itemIndex );
                             
                             //remove from procesing queue
                             itemIndex = iGenerationQueue.FindInOrder(item, Compare);
                             
                             if(itemIndex >= 0)
                                 {
                                 
                                 if( iUnknown )
                                     {
                                     TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() mark %d as EGenerationItemTypeNotFound in iGenerationQueue", queryQueueDelta[0]);
                                     OstTrace1( TRACE_NORMAL, DUP9_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - mark as EGenerationItemTypeNotFound in iGenerationQueue;queryQueueDelta[0]=%d", queryQueueDelta[0] );
                                     //mark to be removed, cleanup is done below
                                     iGenerationQueue[itemIndex].iItemType = EGenerationItemTypeNotFound;
                                     }
                                 else
                                     {
                                     TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() remove %d from iGenerationQueue", queryQueueDelta[0]);
                                     OstTrace1( TRACE_NORMAL, DUP10_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - remove from iGenerationQueue;queryQueueDelta[0]=%d", queryQueueDelta[0] );
                                     iGenerationQueue.Remove( itemIndex );
                                     }
                                 }
                             }
                         queryQueueDelta.Remove(0);
                         }
                     queryQueueDelta.Close();
                    }

                if(iUnknown)
                    {
                    for(TInt i = 0; i < iQuery->Count(); i++)
                      {    
                      const CMdEObject* object = &iQuery->Result(i);
                     
                      if(!object)
                          {
                          continue;
                          }

                          TThumbnailGenerationItem tempItem;
                          tempItem.iItemId = object->Id();
                          TInt itemIndex = iGenerationQueue.FindInOrder(tempItem, Compare);
                                           
                        if(itemIndex >= 0)
                            {
                            TThumbnailGenerationItem& item = iGenerationQueue[itemIndex]; 
                            
                            if(iGenerationQueue[itemIndex].iItemType == EGenerationItemTypeNotFound)
                                {
                                TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() remove EGenerationItemTypeNotFound %d item from iGenerationQueue", item.iItemId);
                                OstTrace1( TRACE_NORMAL, DUP11_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - remove EGenerationItemTypeNotFound item from iGenerationQueue;item.iItemId=%u", item.iItemId );
                                iGenerationQueue.Remove(itemIndex);
                                continue;
                                }
                            
                                SetGenerationItemType(item, object->Def().Id());
                                    
                                if(item.iItemType == EGenerationItemTypeUnknown )
                                  {
                                  TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() remove unknown item %d", item.iItemId);
                                  OstTrace1( TRACE_NORMAL, DUP12_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - remove unknown item;item.iItemId=%u", item.iItemId );
                                  iGenerationQueue.Remove(itemIndex);
                                  continue;
                                  }
                              
                              item.iPlaceholder = object->Placeholder(); 
                                      
                            }
                        }
                
                        iQueryQueue.Reset();
                        iProcessingCount = 0;
                        iUnknown = EFalse;
                    }
            
            // no results, reset query
            if( !iProcessingCount )
                {
                delete iQuery;
                iQuery = NULL;
                iModify = EFalse;
                }
            }
        else
            {
            //Delete and cancel query, do not return items back to original queue
            DeleteAndCancelQuery( EFalse );
            TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() Query FAILED!"); 
            OstTrace0( TRACE_NORMAL, DUP13_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - Query FAILED!" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() - NO QUERY ACTIVE"); 
        OstTrace0( TRACE_NORMAL, DUP14_CTHUMBAGPROCESSOR_HANDLEQUERYCOMPLETED, "CThumbAGProcessor::HandleQueryCompleted - NO QUERY ACTIVE" );
        __ASSERT_DEBUG((EFalse), User::Panic(_L("CThumbAGProcessor::HandleQueryCompleted()"), -1));
        }
    
    ActivateAO();
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::ThumbnailPreviewReady()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::ThumbnailPreviewReady( MThumbnailData& /*aThumbnail*/, 
                                               TThumbnailRequestId /*aId*/)
    {
    TN_DEBUG1( "CThumbAGProcessor::ThumbnailPreviewReady()");
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_THUMBNAILPREVIEWREADY, "CThumbAGProcessor::ThumbnailPreviewReady" );
    // No implementation required
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::ThumbnailReady()d
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::ThumbnailReady( TInt aError, MThumbnailData& /*aThumbnail*/,
                                        TThumbnailRequestId /*aId*/ )
    {
    TN_DEBUG2( "CThumbAGProcessor::ThumbnailReady() aError == %d", aError );
    OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_THUMBNAILREADY, "CThumbAGProcessor::ThumbnailReady;aError=%d", aError );
    
    iActiveCount--;
    
    // TNM server died, delete session
    if( aError == KErrServerTerminated )
        {
        TN_DEBUG1( "CThumbAGProcessor::ThumbnailReady() - **** THUMBNAIL SERVER DIED ****" );
		OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_THUMBNAILREADY, "CThumbAGProcessor::ThumbnailReady - **** THUMBNAIL SERVER DIED ****" );
		
        iSessionDied = ETrue;
        
        if( !iPeriodicTimer->IsActive())
            {
            StartTimeout();
			}
			
        //reset PS idle so that RunL() can startup reopen TNM session and proceed
        TInt ret = RProperty::Set(KServerIdle, KIdle, ETrue);
        TN_DEBUG2( "CThumbAGProcessor::ThumbnailReady() set Idle ret = %d", ret );
        OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_THUMBNAILREADY, "CThumbAGProcessor::ThumbnailReady - set Idle ret;ret=%d", ret );
        return;
        }
    
    ActivateAO();
	
    TN_DEBUG1( "CThumbAGProcessor::ThumbnailReady() - end" );
    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_THUMBNAILREADY, "CThumbAGProcessor::ThumbnailReady - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::ThumbnailRequestReady()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::ThumbnailRequestReady( TInt /*aError*/, TThumbnailRequestType aRequestType,
                                               TThumbnailRequestId /*aId*/ )
    {	
    if (aRequestType == ERequestDeleteThumbnails)
        {
        TN_DEBUG1( "CThumbAGProcessor::ThumbnailRequestReady() - delete" );
        OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_THUMBNAILREQUESTREADY, "CThumbAGProcessor::ThumbnailRequestReady - delete" );
    
        iActiveCount--;
        
        ActivateAO();
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::SetMdESession()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::SetMdESessionL( CMdESession* aMdESession )
    {
    TN_DEBUG1( "CThumbAGProcessor::SetMdESession() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_SETMDESESSIONL, "CThumbAGProcessor::SetMdESessionL - begin" );
    
    iMdESession = aMdESession;
    
    __ASSERT_DEBUG((iMdESession), User::Panic(_L("CThumbAGProcessor::SetMdESession() !iMdESession "), KErrBadHandle));
    
    TRAPD( err, iDefNamespace = &iMdESession->GetDefaultNamespaceDefL() );
    if (err != KErrNone)
        {
        TN_DEBUG2( "CThumbAGProcessor::SetMdESession() GetDefaultNamespaceDefL() err = %d", err );
        OstTrace1( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_SETMDESESSIONL, "CThumbAGProcessor::SetMdESessionL;err=%d", err );
        __ASSERT_DEBUG((iDefNamespace), User::Panic(_L("CThumbAGProcessor::SetMdESession() !iDefNamespace "), KErrBadHandle));
        }
    else
        {
        iImageObjectDef = &iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );
        __ASSERT_DEBUG((iImageObjectDef), User::Panic(_L("CThumbAGProcessor::SetMdESession() !iDefNamespace "), KErrBadHandle));
        iVideoObjectDef = &iDefNamespace->GetObjectDefL( MdeConstants::Video::KVideoObject );
        __ASSERT_DEBUG((iVideoObjectDef), User::Panic(_L("CThumbAGProcessor::SetMdESession() !iVideoObjectDef "), KErrBadHandle));
        iAudioObjectDef = &iDefNamespace->GetObjectDefL( MdeConstants::Audio::KAudioObject );
        __ASSERT_DEBUG((iAudioObjectDef), User::Panic(_L("CThumbAGProcessor::SetMdESession() !iAudioObjectDef "), KErrBadHandle));
        
        //do async init
        iInit = ETrue;

        ActivateAO();
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::AddToQueue()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::AddToQueueL( TObserverNotificationType aType, 
                                    TThumbnailGenerationItemType aItemType,
                                    const RArray<TItemId>& aIDArray, 
                                    const RPointerArray<HBufC>& aObjectUriArray,
                                    TBool /*aPresent*/ )
    {
    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - begin" );

    // update queues
    if (aType == ENotifyAdd)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyAdd" );
        
        for (int i=0; i<aIDArray.Count(); i++)
            {
            TThumbnailGenerationItem item;
            item.iItemId = aIDArray[i];      
            item.iItemType = aItemType;
            
            SetGenerationItemAction(item, aItemType);
            
            if(iPHHarvesting)
                {
                item.iPlaceholder = ETrue;
                }

            AppendProcessingQueue( item );
            }
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyModify" );
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - ENotifyModify" );
        
        if(iPHHarvesting)
            {        
            TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - PH  harvesting active, treat like add" );
            OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - PH  harvesting active, treat like add" );
            for (int i=0; i<aIDArray.Count(); i++)
                {
                TThumbnailGenerationItem item;
                item.iItemId = aIDArray[i];
                item.iItemType = aItemType;
                
                TInt itemIndex = iGenerationQueue.FindInOrder(item, Compare);
                                
                if (itemIndex >= 0)
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - set as non-placeholder");
                    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - set as non-placeholder" );
                    iGenerationQueue[itemIndex].iPlaceholder = EFalse;
                    }
                else
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - append");
                    OstTrace0( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - append" );
                     item.iPlaceholder = EFalse;
                     SetGenerationItemAction( item, aItemType );
                     AppendProcessingQueue( item );
                    }
                }
            }
        else
            {
            TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - PH  harvesting finished, check is real modify!" );
            OstTrace0( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - PH  harvesting finished, check is real modify!" );
            
            TInt itemIndex(KErrNotFound);
            
            for (int i=0; i<aIDArray.Count(); i++)
                {
                TThumbnailGenerationItem item;
                item.iItemId = aIDArray[i];
                item.iItemType = aItemType;
                
                itemIndex = iGenerationQueue.FindInOrder(item, Compare);
                
                if (itemIndex >= 0)
                    {
                    if( iGenerationQueue[itemIndex].iPlaceholder )
                        {
                        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - placeholder modify");
                        OstTrace0( TRACE_NORMAL, DUP6_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - placeholder modify" );
                        iGenerationQueue[itemIndex].iPlaceholder = EFalse;
                        }
                    else
                        {
                        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - real modify");
                        OstTrace0( TRACE_NORMAL, DUP7_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - real modify" );
                        iGenerationQueue[itemIndex].iItemAction = EGenerationItemActionModify;
                        SetForceRun( ETrue );
                        }
                    }
                else
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - append");
                    OstTrace0( TRACE_NORMAL, DUP8_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - append" );
                    SetGenerationItemAction( item, aItemType);
                    item.iPlaceholder = EFalse;
                    AppendProcessingQueue( item );
                    }
                }
            }
        }
        else if (aType == ENotifyRemove && aItemType == EGenerationItemTypeAny)
            {
            TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyRemove, remove IDs from all queues");
            OstTrace0( TRACE_NORMAL, DUP9_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - ENotifyRemove, remove IDs from all queues" );
            
            for (int i=0; i<aIDArray.Count(); i++)
                {
                TThumbnailGenerationItem item;
                item.iItemId = aIDArray[i];
                item.iItemType = aItemType;

                TInt itemIndex = iGenerationQueue.FindInOrder(item, Compare);
                
                if(itemIndex >= 0)
                    {
                    iGenerationQueue[itemIndex].iItemAction = EGenerationItemActionDelete;
                    delete iGenerationQueue[itemIndex].iUri;
                    iGenerationQueue[itemIndex].iUri = NULL;
                    
                    if( aObjectUriArray[i])
                        {
                        iGenerationQueue[itemIndex].iUri = aObjectUriArray[i]->AllocL();
                        }
                    else
                        {
                        //invalid URI remove from processing queue
                        iGenerationQueue.Remove(itemIndex);
                        }
                    }
                else
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - append");
                    OstTrace0( TRACE_NORMAL, DUP10_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - append" );
                    item.iItemAction = EGenerationItemActionDelete;
                    delete item.iUri;
                    item.iUri = NULL;
                    
                    if( aObjectUriArray[i])
                        {
                        item.iUri = aObjectUriArray[i]->AllocL();
                        CleanupStack::PushL( item.iUri );
                        User::LeaveIfError( iGenerationQueue.InsertInOrder(item, Compare) );
                        CleanupStack::Pop();
                        }
                    
                    //owned by item
                    item.iUri = NULL;
                    }
                
                TN_DEBUG2( "CThumbAGProcessor::AddToQueueL() - %S", aObjectUriArray[i]); 
                OstTraceExt1( TRACE_NORMAL, DUP11_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL;aObjectUriArray[i]=%S", *aObjectUriArray[i] );
                }
            }
#ifdef _DEBUG
        else
            {
	        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() -  should not come here" );
	        OstTrace0( TRACE_NORMAL, DUP12_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL -  should not come here" );
	        __ASSERT_DEBUG((EFalse), User::Panic(_L("CThumbAGProcessor::AddToQueueL()"), KErrArgument));
	        User::Leave( KErrArgument );
            }
#endif
    
    ActivateAO(); 
    
    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - end" );
    OstTrace0( TRACE_NORMAL, DUP13_CTHUMBAGPROCESSOR_ADDTOQUEUEL, "CThumbAGProcessor::AddToQueueL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::CreateThumbnailsL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::CreateThumbnailsL( const CMdEObject* aObject )
    {
    TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_CREATETHUMBNAILSL, "CThumbAGProcessor::CreateThumbnailsL - begin" );
    
    __ASSERT_DEBUG((iTMSession), User::Panic(_L("CThumbAGProcessor::CreateThumbnailsL() !iTMSession "), KErrBadHandle));
    __ASSERT_DEBUG((iDefNamespace), User::Panic(_L("CThumbAGProcessor::CreateThumbnailsL() !iDefNamespace "), KErrBadHandle));
    
    if(!iTMSession || !iDefNamespace)
        {
        return;
        }
    
    TInt orientationVal = 0;
    TInt64 modifiedVal = 0;
    
    CMdEProperty* orientation = NULL;
       
    TInt orientErr = aObject->Property( iImageObjectDef->GetPropertyDefL( MdeConstants::Image::KOrientationProperty ), orientation, 0 );
    
    if (orientErr == KErrNone)
        {
        orientationVal = orientation->Uint16ValueL();
        }
        
    CMdEProperty* modified = NULL;
    CMdEObjectDef& baseObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject );       
    TInt modifyErr = aObject->Property( baseObjDef.GetPropertyDefL( MdeConstants::Object::KLastModifiedDateProperty ), modified, 0 );

    if (modifyErr >= 0)
        {
        modifiedVal = modified->TimeValueL().Int64();
        }
    
    // update thumbs
    if (iTMSession)
        {
		// 2nd round and modify updates both sizes if needed
        if( i2ndRound )
            {
            //generate both if needed
            TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() EOptimizeForQuality ");
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_CREATETHUMBNAILSL, "CThumbAGProcessor::CreateThumbnailsL - EOptimizeForQuality" );
            iTMSession->SetQualityPreferenceL( CThumbnailManager::EOptimizeForQuality );
            
            TN_DEBUG2( "CThumbAGProcessor::CreateThumbnailsL() - 2nd round add remove from queue", aObject->Id() );
            OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_CREATETHUMBNAILSL, "CThumbAGProcessor::CreateThumbnailsL - 2nd round add remove from queue;aObject->Id()=%u", aObject->Id() );
                        
            TThumbnailGenerationItem item;
            item.iItemId = aObject->Id();
            TInt itemIndex = iGenerationQueue.FindInOrder(item, Compare);
            
            if(itemIndex >=0 )
                {
                if(iGenerationQueue[itemIndex].iItemAction == EGenerationItemAction2ndAdd)
                    {
                    iGenerationQueue.Remove(itemIndex);
                    }
                }
            }
		// 1st roung generation
        else
            {
            //1st round
            TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() EOptimizeForQualityWithPreview");
            OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_CREATETHUMBNAILSL, "CThumbAGProcessor::CreateThumbnailsL - EOptimizeForQualityWithPreview" );
            iTMSession->SetQualityPreferenceL( CThumbnailManager::EOptimizeForQualityWithPreview );
            
            // add item to 2nd round queue 
            TN_DEBUG2( "CThumbAGProcessor::CreateThumbnailsL() - 1st round add/modify, append to 2nd round queue", aObject->Id() );
            OstTrace1( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_CREATETHUMBNAILSL, "CThumbAGProcessor::CreateThumbnailsL - 1st round add/modify, append to 2nd round queue;aObject->Id()=%u", aObject->Id() );
            
            TThumbnailGenerationItem item;
            item.iItemId = aObject->Id();
            TInt itemIndex = iGenerationQueue.FindInOrder(item, Compare);
            
            if(itemIndex >=0 )
                {
                if(iGenerationQueue[itemIndex].iItemAction == EGenerationItemActionModify)
                    {
                    iGenerationQueue.Remove(itemIndex);
                    }
                //change 1st round item for 2nd round processing
                //2nd round item can be deleted
                else if(iGenerationQueue[itemIndex].iItemAction == EGenerationItemActionAdd)
                    {
                    iGenerationQueue[itemIndex].iItemAction = EGenerationItemAction2ndAdd;
                    }
                else
                    {
                    iGenerationQueue.Remove(itemIndex);
                    }
                }
            }

        // run as lower priority than getting but higher that creating thumbnails
        TRAPD(err, iTMSession->UpdateThumbnailsL(KNoId, aObject->Uri(), orientationVal, modifiedVal, CActive::EPriorityIdle ));
      
        if ( err != KErrNone )
            {
            TN_DEBUG2( "CThumbAGProcessor::CreateThumbnailsL, iTMSession error == %d", err );
            OstTrace0( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_CREATETHUMBNAILSL, "CThumbAGProcessor::CreateThumbnailsL" );
            
            iSessionDied = ETrue;
            ActivateAO();
            } 
        else
            {
            iActiveCount++;
            }
        }
    else
        {
        ActivateAO();
        }
        
    TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() - end" );
    OstTrace0( TRACE_NORMAL, DUP6_CTHUMBAGPROCESSOR_CREATETHUMBNAILSL, "CThumbAGProcessor::CreateThumbnailsL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::QueryL(/*RArray<TItemId>& aIDArray*/TThumbnailGenerationItemAction aAction  )
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryL() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_QUERYL, "CThumbAGProcessor::QueryL - begin" );
    
    __ASSERT_DEBUG((iMdESession), User::Panic(_L("CThumbAGProcessor::QueryL() !iMdeSession "), KErrBadHandle));
    __ASSERT_DEBUG((iDefNamespace), User::Panic(_L("CThumbAGProcessor::QueryL() !iDefNamespace "), KErrBadHandle));
    
    if(!iMdESession  || !iDefNamespace || iShutdown)
        {
        return;
        }
    
	//reset query queue
    iQueryQueue.Reset();
	//set reference to current pprocessing queue
    
    iQueryReady = EFalse;

    // delete old query
    if (iQuery)
        {
        delete iQuery;
        iQuery = NULL;
        }
    
    //move ID from source queue to Query queue
    TInt maxCount = iGenerationQueue.Count();
        
    TN_DEBUG3( "CThumbAGProcessor::QueryL() - fill begin iGenerationQueue == %d, iQueryQueue == %d", iGenerationQueue.Count(), iQueryQueue.Count() );
    OstTraceExt2( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_QUERYL, "CThumbAGProcessor::QueryL -  - fill begin iGenerationQueue, iQueryQueue;iGenerationQueue.Count()=%d;iQueryQueue.Count()=%d", iGenerationQueue.Count(), iQueryQueue.Count() );
    
    TInt itemCount(0);
    for(TInt i=0; itemCount < KMaxQueryItems && i < maxCount; i++)
        {
        TInt itemIndex(KErrNotFound);
        switch(aAction)
            {
            //1st round items
            case EGenerationItemActionAdd:
                if(iGenerationQueue[i].iItemAction == aAction )
                    {
                    itemIndex = i;
                    }
                break;
            case EGenerationItemActionModify:
                if( iGenerationQueue[i].iItemAction == aAction )
                    {
                    itemIndex = i;
                    }
                break;
            case EGenerationItemAction2ndAdd:
                if( iGenerationQueue[i].iItemAction == aAction )
                    {
                    itemIndex = i;
                    }
                break;
            //unknown stuff
            case EGenerationItemActionResolveType:
                if( iGenerationQueue[i].iItemType == EGenerationItemTypeUnknown )
                    {
                    itemIndex = i;
                    }
                break;
            default:
                break;
            };
        
        if( itemIndex >= 0 )
            {
            TN_DEBUG2( "CThumbAGProcessor::QueryL() - fill %d", iGenerationQueue[itemIndex].iItemId );        
            OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_QUERYL, "CThumbAGProcessor::QueryL - fill;iGenerationQueue[itemIndex].iItemId=%u", iGenerationQueue[itemIndex].iItemId );
            iQueryQueue.InsertInOrder(iGenerationQueue[itemIndex].iItemId, CompareId);
            itemCount++;
            }
        }
    
    if(!itemCount)
        {
        TN_DEBUG1( "CThumbAGProcessor::QueryL() - empty query, cancel?!");
        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_QUERYL, "CThumbAGProcessor::QueryL - empty query, cancel?!" );
        iQueryActive = EFalse;
        __ASSERT_DEBUG((iMdESession), User::Panic(_L("CThumbAGProcessor::QueryL() empty! "), KErrNotFound));
        return;
        }
    
    TN_DEBUG3( "CThumbAGProcessor::QueryL() - fill end iGenerationQueue == %d, iQueryQueue == %d", iGenerationQueue.Count(), iQueryQueue.Count() );
    OstTraceExt2( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_QUERYL, "CThumbAGProcessor::QueryL;iGenerationQueue.Count()=%d;iQueryQueue.Count()=%d", iGenerationQueue.Count(), iQueryQueue.Count() );
    
    CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject );
    iQuery = iMdESession->NewObjectQueryL( *iDefNamespace, objDef, this );
	
	if(iQuery)
		{
	    iQuery->SetResultMode( EQueryResultModeItem );

	    CMdELogicCondition& rootCondition = iQuery->Conditions();
	    rootCondition.SetOperator( ELogicConditionOperatorAnd );
    
	    // add IDs
	    CleanupClosePushL( iQueryQueue );
	    rootCondition.AddObjectConditionL( iQueryQueue );
	    CleanupStack::Pop( &iQueryQueue );
    
	    // add object type conditions 
	    if (!(iModify || iUnknown))
	        {
	        CMdELogicCondition& objDefCondition = rootCondition.AddLogicConditionL( ELogicConditionOperatorOr );
        
	        if (iAutoImage)
	            {
	            CMdEObjectDef& imageDef = iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );
	            objDefCondition.AddObjectConditionL( imageDef );
	            }
	        if (iAutoVideo)
	            {
	            CMdEObjectDef& videoDef = iDefNamespace->GetObjectDefL( MdeConstants::Video::KVideoObject );
	            objDefCondition.AddObjectConditionL( videoDef );
	            }
	        if (iAutoAudio)
	            {
	            CMdEObjectDef& audioDef = iDefNamespace->GetObjectDefL( MdeConstants::Audio::KAudioObject );
	            objDefCondition.AddObjectConditionL( audioDef );
	            }    
	        }
    
	    iQuery->FindL();
		}
	else
	    {
        iQueryActive = EFalse;
	    }
    
    TN_DEBUG1( "CThumbAGProcessor::QueryL() - end" );
    OstTrace0( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_QUERYL, "CThumbAGProcessor::QueryL - end" );
    }


// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryForPlaceholders()
// ---------------------------------------------------------------------------
//

void CThumbAGProcessor::QueryPlaceholdersL(TBool aPresent)
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_QUERYPLACEHOLDERSL, "CThumbAGProcessor::QueryPlaceholdersL" );
    
    __ASSERT_DEBUG((iMdESession), User::Panic(_L("CThumbAGProcessor::QueryPlaceholdersL() !iMdeSession "), KErrBadHandle));
    __ASSERT_DEBUG((iDefNamespace), User::Panic(_L("CThumbAGProcessor::QueryPlaceholdersL() !iDefNamespace "), KErrBadHandle));
    
    if(!iMdESession  || !iDefNamespace || iShutdown)
         {
         return;
         }
    
    if( iQueryPlaceholders )
        {
        if( !iQueryPlaceholders->IsComplete() )
            {
            TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL active- skip" );
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_QUERYPLACEHOLDERSL, "CThumbAGProcessor::QueryPlaceholdersL - skip" );
            return;
            }
        
        // delete old query
        iQueryPlaceholders->Cancel();
        delete iQueryPlaceholders;
        iQueryPlaceholders = NULL;
        }
   
    TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL - start" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_QUERYPLACEHOLDERSL, "CThumbAGProcessor::QueryPlaceholdersL - start" );

    CMdEObjectDef& imageObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );
    CMdEObjectDef& videoObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Video::KVideoObject );
    CMdEObjectDef& audioObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Audio::KAudioObject );
    
    CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject);
    iQueryPlaceholders = iMdESession->NewObjectQueryL( *iDefNamespace, objDef, this );
        
    if(iQueryPlaceholders)
        {
        iQueryPlaceholders->SetResultMode( EQueryResultModeItem );
        
        CMdELogicCondition& rootCondition = iQueryPlaceholders->Conditions();
        rootCondition.SetOperator( ELogicConditionOperatorOr );
        
        CMdEObjectCondition& imagePHObjectCondition = rootCondition.AddObjectConditionL(imageObjDef);
        CleanupStack::PushL( &imagePHObjectCondition );
        imagePHObjectCondition.SetPlaceholderOnly( ETrue );
        imagePHObjectCondition.SetNotPresent( aPresent );
        
        CMdEObjectCondition& videoPHObjectCondition = rootCondition.AddObjectConditionL(videoObjDef);
        CleanupStack::PushL( &videoPHObjectCondition );
        videoPHObjectCondition.SetPlaceholderOnly( ETrue );
        videoPHObjectCondition.SetNotPresent( aPresent );
        
        CMdEObjectCondition& audioPHObjectCondition = rootCondition.AddObjectConditionL(audioObjDef);
        CleanupStack::PushL( &audioPHObjectCondition );
        audioPHObjectCondition.SetPlaceholderOnly( ETrue );
        audioPHObjectCondition.SetNotPresent( aPresent );
        
        iQueryPlaceholders->FindL(KMdEQueryDefaultMaxCount, KMaxQueryBatchSize);   
       
        CleanupStack::Pop(3, &imagePHObjectCondition );
        }
	
    TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL - end" );
    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_QUERYPLACEHOLDERSL, "CThumbAGProcessor::QueryPlaceholdersL - end" );
    }


// ---------------------------------------------------------------------------
// CThumbAGProcessor::RunL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::RunL()
    {
    TN_DEBUG1( "CThumbAGProcessor::RunL() - begin" );
	OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - begin" );
	
	if(iShutdown)
		{
        TN_DEBUG1( "CThumbAGProcessor::RunL() - shutdown" );
		OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - shutdown" );
		return;
		}
    
    if (iSessionDied)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - iSessionDied" );
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - iSessionDied" );
        delete iTMSession;
        iTMSession = NULL;
        }
    
    if (iInit)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Do Initialisation 1" );
        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - Do Initialisation 1" );
        
        iInit = EFalse;
        iInit2 = ETrue;

        iGenerationQueue.Reset();
        iQueryQueue.Reset();
        
		//query all not present placeholders
        TRAP_IGNORE(QueryPlaceholdersL( ETrue ));
		//query all items after PH query
        iDoQueryAllItems = ETrue;
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Initialisation 1 done" );
        OstTrace0( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - Initialisation 1 done" );
        ActivateAO();
        return;
        }
    
    if(iInit2)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Do Initialisation 2" );
		OstTrace0( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - Do Initialisation 2" );
		
        iInit2 = EFalse;
        TInt err(KErrNone);
        
#ifdef  MDS_MODIFY_OBSERVER        
        TN_DEBUG1( "CThumbAGProcessor::RunL() do iHarvesterClient connect");
        OstTrace0( TRACE_NORMAL, DUP6_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - do iHarvesterClient connect" );
        err = iHarvesterClient.Connect();
        TN_DEBUG2( "CThumbAGProcessor::RunL() iHarvesterClient connect err = %d", err);
        OstTrace1( TRACE_NORMAL, DUP7_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - iHarvesterClient connect;err=%d", err );
        
        __ASSERT_DEBUG((err == KErrNone), User::Panic(_L("CThumbAGProcessor::RunL(), !iHarvesterClient "), err));
        
        if(  err == KErrNone )
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() add iHarvesterClient observer");
            OstTrace0( TRACE_NORMAL, DUP8_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - add iHarvesterClient observer" );
            err = iHarvesterClient.AddHarvesterEventObserver( *this, EHEObserverTypeOverall | EHEObserverTypeMMC | EHEObserverTypePlaceholder, 20 );
            TN_DEBUG2( "CThumbAGProcessor::RunL() iHarvesterClient observer err = %d", err);
            OstTrace1( TRACE_NORMAL, DUP9_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - iHarvesterClient observer;err=%d", err );
            
            if( err != KErrNone )
                {
                TN_DEBUG1( "CThumbAGProcessor::RunL() add iHarvesterClient observer failed");
                OstTrace0( TRACE_NORMAL, DUP10_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - add iHarvesterClient observer failed" );
                // if we fail observer harvester, fake it
                iHarvesterActivated = ETrue;
                }
            __ASSERT_DEBUG((err==KErrNone), User::Panic(_L("CThumbAGProcessor::RunL(), !iHarvesterClient "), err));
            }
#endif
 
        TN_DEBUG1( "CThumbAGProcessor::RunL() MMPXCollectionUtility");
        OstTrace0( TRACE_NORMAL, DUP11_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - MMPXCollectionUtility");
        TRAP( err, iCollectionUtility = MMPXCollectionUtility::NewL( this, KMcModeIsolated ));
        TN_DEBUG2( "CThumbAGProcessor::RunL() create MMPXCollectionUtility err = %d", err);
        OstTrace1( TRACE_NORMAL, DUP12_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - create MMPXCollectionUtility err;err=%d", err );
        __ASSERT_DEBUG((iCollectionUtility), User::Panic(_L("CThumbAGProcessor::RunL(), !iCollectionUtility "), err));
        
        __ASSERT_DEBUG((iActivityManager), User::Panic(_L("CThumbAGProcessor::RunL(), !iActivityManager "), KErrBadHandle));
        if(iActivityManager)
            {
            iActivityManager->Start();
            }
        
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Initialisation 2 done" );
        OstTrace0( TRACE_NORMAL, DUP13_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - Initialisation 2 done" );
        return;
        }
    
    // restart session if died
    if (!iTMSession)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - open TNM session");
        OstTrace0( TRACE_NORMAL, DUP14_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL" );
        TRAPD( err, iTMSession = CThumbnailManager::NewL( *this ) );
		
        if (err != KErrNone)
            {
            iTMSession = NULL;
            ActivateAO();
            TN_DEBUG2( "CThumbAGProcessor::RunL() - Session restart failed, error == %d", err );
            OstTrace1( TRACE_NORMAL, DUP15_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - Session restart failed, error;err=%d", err );
            }        
        else 
            {
            iTMSession->SetRequestObserver(*this);
            iSessionDied = EFalse;
            }
        }    
   
    // do not run if request is already issued to TNM server even if forced
    if(iActiveCount >= KMaxDaemonRequests)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - waiting for previous to complete, abort..." );
        OstTrace0( TRACE_NORMAL, DUP16_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - waiting for previous to complete, abort..." );
        return;
        }
    
    //force run can proceed from this point
#ifdef _DEBUG
	if( iForegroundRun )
		{
      	TN_DEBUG1( "void CThumbAGProcessor::RunL() KForceBackgroundGeneration enabled");
	  	OstTrace0( TRACE_NORMAL, DUP17_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - KForceBackgroundGeneration enabled" );
	  	}
	
    if( iForceRun )
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - *** FORCED RUN ***");
        OstTrace0( TRACE_NORMAL, DUP18_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - *** FORCED RUN ***" );
        }
#endif
	
  	if( (iForceRun || iForegroundRun ) && !iMountTimer->IsActive() )
      	{
        TN_DEBUG1( "void CThumbAGProcessor::RunL() skip idle detection!");
      	OstTrace0( TRACE_NORMAL, DUP19_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - skip idle detection!" );
      	CancelTimeout();
     	}
  	else
	    {
        if( !iIdle || iHarvesting || iMPXHarvesting || iPeriodicTimer->IsActive() )
            {
            TN_DEBUG1( "void CThumbAGProcessor::RunL() device not idle");
            OstTrace0( TRACE_NORMAL, DUP20_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - device not idle" );
            return;
            }
        else
            {
            //check is server idle
            TInt serveIdle(KErrNotFound);
            TInt ret = RProperty::Get(KServerIdle, KIdle, serveIdle);
            
            if(ret != KErrNone || !serveIdle )
                {
            	//start inactivity timer and retry on after callback
            	TN_DEBUG1( "void CThumbAGProcessor::RunL() server not idle");
                OstTrace0( TRACE_NORMAL, DUP21_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - server not idle" );
                StartTimeout();
                return;
                }
            TN_DEBUG1( "void CThumbAGProcessor::RunL() device and server idle, process");
            OstTrace0( TRACE_NORMAL, DUP22_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - device and server idle, process" );
            }
	    }
    
    //Handle completed MDS Query
    if( iQueryReady && iProcessingCount)
        {
        TInt err(KErrNone);
        //if force or non forced
		//if unknown items or mount timer is active, abort processing

        if(((iForceRun && iModify ) || (!iForceRun && !iModify )) && !iUnknownItemCount && !iMountTimer->IsActive())
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - iQueryReady START" );
            OstTrace0( TRACE_NORMAL, DUP23_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - iQueryReady START" );
            
            const CMdEObject* object = &iQuery->Result( iProcessingCount-1 );
            iProcessingCount--;
            
            if ( object )
                {
                //process one item at once
                //remove item from queryQueue when request is issued 
 
                TInt itemIndex = iQueryQueue.FindInOrder(object->Id(), CompareId);
                if(itemIndex >= 0)
                    {
                    iQueryQueue.Remove(itemIndex);
                    }
            
                TRAP( err, CreateThumbnailsL( object ) );
                TN_DEBUG2( "CThumbAGProcessor::RunL(), CreateThumbnailsL error == %d", err );
                OstTrace1( TRACE_NORMAL, DUP24_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - CreateThumbnailsL error;err=%d", err );
                __ASSERT_DEBUG((err==KErrNone), User::Panic(_L("CThumbAGProcessor::RunL(), CreateThumbnailsL() "), err));
                }
            }
        //force is coming, but executing non-forced query complete-> cancel old
        else
            {
			//cancel query
            TN_DEBUG1( "CThumbAGProcessor::RunL() - cancel processing query" );
            OstTrace0( TRACE_NORMAL, DUP25_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - cancel processing query" );
            DeleteAndCancelQuery( ETrue );
	        ActivateAO();
            return;  
            }
        
        //is last query item
        if( iProcessingCount <= 0 )
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - iQueryReady FINISH" );
            OstTrace0( TRACE_NORMAL, DUP26_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - iQueryReady FINISH" );
            iQueryReady = EFalse;
            iQueryActive = EFalse;
            }
            
        ActivateAO();
        }
    //waiting for MDS query to complete
    else if( iQueryActive )
        {
		//state mismatch
        if(iForceRun && !(iModify || iUnknown))
            {
			//cancel query and move items back to original processing queue
            DeleteAndCancelQuery(ETrue);
            ActivateAO();
            }
        else  
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - waiting for query to complete, abort..." );
            OstTrace0( TRACE_NORMAL, DUP27_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - waiting for query to complete, abort..." );
            }    
        }
    else if ( iUnknownItemCount > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() unknown items in queue");
        OstTrace0( TRACE_NORMAL, DUP28_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - unknown items in queue" );
        
        i2ndRound = EFalse;
        iModify = EFalse;
        iUnknown = ETrue;
        iQueryActive = ETrue;

        QueryL( EGenerationItemActionResolveType );
       }
    else if ( iDeleteItemCount > 0 )
       {
       TN_DEBUG1( "void CThumbAGProcessor::RunL() delete thumbnails");
       OstTrace0( TRACE_NORMAL, DUP29_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - delete thumbnails" );
       // delete thumbs by URI
       __ASSERT_DEBUG((iTMSession), User::Panic(_L("CThumbAGProcessor::RunL() !iTMSession "), KErrBadHandle));
       if(iTMSession)
           {
           TInt itemIndex(KErrNotFound);
                               
           for(TInt i=0;i<iGenerationQueue.Count() || itemIndex == KErrNotFound;i++)
               {
               if(iGenerationQueue[i].iItemAction == EGenerationItemActionDelete)
                   {
                   itemIndex = i;
                   }
               }
       
           if(itemIndex >= 0)
               {
               if(!iGenerationQueue[itemIndex].iUri)
                   {
                   //URI is invalid
                   TN_DEBUG1( "void CThumbAGProcessor::RunL() unable to delete URI inbalid");
                   OstTrace0( TRACE_NORMAL, DUP30_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - unable to delete URI inbalid" );
                   iGenerationQueue.Remove( itemIndex );
                   ActivateAO();
                   return;
                   }

               TN_DEBUG2( "void CThumbAGProcessor::RunL() delete %S",  iGenerationQueue[itemIndex].iUri);
               OstTraceExt1( TRACE_NORMAL, DUP31_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL;iGenerationQueue[itemIndex].iUri=%S", *iGenerationQueue[itemIndex].iUri );
               CThumbnailObjectSource* source = NULL;                
               TRAPD(err,  source = CThumbnailObjectSource::NewL( *iGenerationQueue[itemIndex].iUri, KNullDesC));
                  
               if(err == KErrNone)
                   {
                   iTMSession->DeleteThumbnails( *source );
                   }
               delete source;
               
               delete iGenerationQueue[itemIndex].iUri;
               iGenerationQueue[itemIndex].iUri = NULL;
               iGenerationQueue.Remove( itemIndex );
               
               iActiveCount++;
               }
           }
       }
    // no items in query queue, start new
    // select queue to process, priority by type
    else if ( iModifyItemCount > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() update thumbnails");
        OstTrace0( TRACE_NORMAL, DUP32_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - update thumbnails" );
        
        i2ndRound = EFalse;
        
        // query for object info
        iQueryActive = ETrue;
        iModify = ETrue;
        iUnknown = EFalse;
        QueryL( EGenerationItemActionModify );
       }
    else if ( iAddItemCount > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() update 1st round thumbnails");
        OstTrace0( TRACE_NORMAL, DUP33_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - update 1st round thumbnails" );
        
        i2ndRound = EFalse;
        iUnknown = EFalse;
        // query for object info
        iQueryActive = ETrue;
        
        QueryL( EGenerationItemActionAdd );     
        }
    else if( i2ndAddItemCount > 0)
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() update 2nd round thumbnails");
            OstTrace0( TRACE_NORMAL, DUP34_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - update 2nd round thumbnails" );
            
        // query for object info
        iQueryActive = ETrue;
        i2ndRound = ETrue;
        iUnknown = EFalse;
        QueryL( EGenerationItemAction2ndAdd );     
        }
        
    TN_DEBUG1( "CThumbAGProcessor::RunL() - end" );
    OstTrace0( TRACE_NORMAL, DUP35_CTHUMBAGPROCESSOR_RUNL, "CThumbAGProcessor::RunL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::DeleteAndCancelQuery()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::DeleteAndCancelQuery(TBool aRestoreItems)
    {
    TN_DEBUG2( "CThumbAGProcessor::DeleteAndCancelQuery(aRestoreItems = %d) in", aRestoreItems );
    OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_DELETEANDCANCELQUERY, "CThumbAGProcessor::DeleteAndCancelQuery;aRestoreItems=%d", aRestoreItems );
    
    if(iQuery)
        {
        TN_DEBUG1( "CThumbAGProcessor::DeleteAndCancelQuery() - deleting query" );
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_DELETEANDCANCELQUERY, "CThumbAGProcessor::DeleteAndCancelQuery - deleting query" );
        iQuery->Cancel();
        delete iQuery;
        iQuery = NULL;
        }
    
    iQueryReady = EFalse;
    iQueryActive = EFalse;
    iProcessingCount = 0;
    
    //move remainig IDs in query queue back to original queue
    while(iQueryQueue.Count())
        {
        if(!aRestoreItems )
            {
            TThumbnailGenerationItem item;
            item.iItemId = iQueryQueue[0];
            TInt itemIndex = iGenerationQueue.FindInOrder(item, Compare);
                
            if(itemIndex >= 0)
                {
                delete iGenerationQueue[itemIndex].iUri;
                iGenerationQueue[itemIndex].iUri = NULL;
                iGenerationQueue.Remove(itemIndex);
                }
            }
        iQueryQueue.Remove(0);
        }

    TN_DEBUG1( "CThumbAGProcessor::DeleteAndCancelQuery() out" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_DELETEANDCANCELQUERY, "CThumbAGProcessor::DeleteAndCancelQuery - out" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::DoCancel()
    {
    // No implementation required
    }

void CThumbAGProcessor::HarvestingUpdated( 
         HarvesterEventObserverType aHEObserverType, 
         HarvesterEventState aHarvesterEventState,
         TInt aItemsLeft )
    {
    TN_DEBUG4( "CThumbAGProcessor::HarvestingUpdated -- start() aHEObserverType = %d, aHarvesterEventState = %d, aItemsLeft = %d", aHEObserverType, aHarvesterEventState, aItemsLeft );
    OstTraceExt3( TRACE_NORMAL, CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated;aHEObserverType=%u;aHarvesterEventState=%u;aItemsLeft=%d", aHEObserverType, aHarvesterEventState, aItemsLeft );
    
	if(iShutdown)
        {
        return;
        }

    if(!iHarvesterActivated)
        {
        iHarvesterActivated = ETrue;
        }
    
    #ifdef _DEBUG
    if( aHEObserverType == EHEObserverTypePlaceholder)
        {
        TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypePlaceholder");
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypePlaceholder" );
        }
    else if( aHEObserverType == EHEObserverTypeOverall)
        {
        TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypeOverall");
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypeOverall" );
        }
    else if( aHEObserverType == EHEObserverTypeMMC)
        {
        TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypeMMC");
        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypeMMC" );
        }
    #endif
    
    //placeholder harvesting
    if( aHEObserverType == EHEObserverTypePlaceholder)
        {
        switch(aHarvesterEventState)
            {
            case EHEStateStarted:
            case EHEStateHarvesting:
            case EHEStateResumed:
                {
                iPHHarvestingTemp = ETrue;
                break;
                }
            case EHEStatePaused:
            case EHEStateFinished:
            case EHEStateUninitialized:
                {
                iPHHarvestingTemp = EFalse;
                break;
                }
            };
    
        if(iPHHarvestingTemp != iPHHarvesting)
            {
            iPHHarvesting = iPHHarvestingTemp;
           
            if( iPHHarvesting )
                {
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS placeholder harvesterin started");
                OstTrace0( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- MDS placeholder harvesterin starteds" );
                }
            else
                {
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS placeholder harvesting finished");
                OstTrace0( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- MDS placeholder harvesting finished" );
                //query present placeholders
                TRAP_IGNORE(QueryPlaceholdersL( EFalse ));
                iDoQueryAllItems = EFalse;
                iPHHarvestingItemsLeftTemp = 0;
                }
            }
        
        //restart mount timout if PH item count is increasing durin MMC harvesting 
        if(iMMCHarvesting && iPHHarvesting && aItemsLeft > iPHHarvestingItemsLeftTemp)
          {
          //if items count increasing, restart mount timeout 
            TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- PH count increasing, restart mount timeout");
            OstTrace0( TRACE_NORMAL, DUP6_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- PH count increasing, restart mount timeout" );
            
           if(iMountTimer->IsActive())
              {
              iMountTimer->Cancel();
              }

            iMountTimer->Start( KMountTimeout, KMountTimeout, TCallBack(MountTimerCallBack, this));
          }
          
        //we are interestead of only PHs during MMC harvesting
        if( iMMCHarvesting )
            {
            iPHHarvestingItemsLeftTemp = aItemsLeft;
            }
        }
    //overall harvesting
    else if ( aHEObserverType == EHEObserverTypeOverall)
        {
        switch(aHarvesterEventState)
            {
            case EHEStateStarted:
            case EHEStateHarvesting:
            case EHEStatePaused:
            case EHEStateResumed:
                {
                iHarvestingTemp = ETrue;
                break;
                }
            case EHEStateFinished:
            case EHEStateUninitialized:
                {
                iHarvestingTemp = EFalse;
                break;
                }
            };
        
        if(iHarvestingTemp != iHarvesting)
            {
            iHarvesting = iHarvestingTemp;
            
            if( iHarvesting )
                {
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS harvesting started");
                OstTrace0( TRACE_NORMAL, DUP7_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- MDS harvesting started" );
                CancelTimeout();
                }
            else
                {
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS harvesting finished ");
                OstTrace0( TRACE_NORMAL, DUP8_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- MDS harvesting finished" );
                // continue processing if needed
                StartTimeout();
                
                if(iMountTimer->IsActive())
                    {
                    iMountTimer->Cancel();
                    }
                }
            }
        }
    //MMC harvesting
    else if( aHEObserverType == EHEObserverTypeMMC)
        {
        switch(aHarvesterEventState)
            {
            case EHEStateStarted:
            case EHEStateHarvesting:
            case EHEStatePaused:
            case EHEStateResumed:
                {
                iMMCHarvestingTemp = ETrue;
                break;
                }
            case EHEStateFinished:
            case EHEStateUninitialized:
                {
                iMMCHarvestingTemp = EFalse;
                break;
                }
            };
        
        if(iMMCHarvestingTemp != iMMCHarvesting)
            {
            iMMCHarvesting = iMMCHarvestingTemp;
            
            if( iMMCHarvesting )
                {
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS MMC harvesterin started");
                OstTrace0( TRACE_NORMAL, DUP9_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- MDS MMC harvesterin started" );
                UpdatePSValues(EFalse, ETrue);
                iMMCHarvestingItemsLeftTemp = 0;
                }
            else
                {
				//activate timeout if overall harvesting is not active
                if(!iHarvesting)
                    {
                    StartTimeout();
                    }
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS MMC harvesting finished ");
                OstTrace0( TRACE_NORMAL, DUP10_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- MDS MMC harvesting finished" );
                }
            }
        
        //restart mount timout if MMC item count is still increasing 
        if(iMMCHarvesting && aItemsLeft > iMMCHarvestingItemsLeftTemp)
            {
              TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MMC count increasing, restart mount timeout");
              OstTrace0( TRACE_NORMAL, DUP11_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated -- MMC count increasing, restart mount timeout" );
              
             if(iMountTimer->IsActive())
                {
                iMountTimer->Cancel();
                }

              iMountTimer->Start( KMountTimeout, KMountTimeout, TCallBack(MountTimerCallBack, this));
            }
        
            iMMCHarvestingItemsLeftTemp = aItemsLeft;
        }
   
    TN_DEBUG4( "CThumbAGProcessor::HarvestingUpdated -- end() iHarvesting == %d, iPHHarvesting == %d iMMCHarvesting == %d ", iHarvesting, iPHHarvesting, iMMCHarvesting);
    OstTraceExt3( TRACE_NORMAL, DUP12_CTHUMBAGPROCESSOR_HARVESTINGUPDATED, "CThumbAGProcessor::HarvestingUpdated;iHarvesting=%u;iPHHarvesting=%u;iMMCHarvesting=%u", iHarvesting, iPHHarvesting, iMMCHarvesting );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::StartTimeout()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::StartTimeout()
    {
    TN_DEBUG1( "CThumbAGProcessor::StartTimeout()");
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_STARTTIMEOUT, "CThumbAGProcessor::StartTimeout" );
    CancelTimeout();
    
    if(!iHarvesting && !iMPXHarvesting && !iPeriodicTimer->IsActive() && !iShutdown)
        {
        iPeriodicTimer->Start( KHarvestingCompleteTimeout, KHarvestingCompleteTimeout,
                TCallBack(PeriodicTimerCallBack, this));
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::StopTimeout()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::CancelTimeout()
    {
    TN_DEBUG1( "CThumbAGProcessor::CancelTimeout()");
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_CANCELTIMEOUT, "CThumbAGProcessor::CancelTimeout" );
    if(iPeriodicTimer->IsActive())
        {
        iPeriodicTimer->Cancel();
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::RunError()
// ---------------------------------------------------------------------------
//
#ifdef _DEBUG
TInt CThumbAGProcessor::RunError(TInt aError)
#else
TInt CThumbAGProcessor::RunError(TInt /*aError*/)
#endif
    {
    TN_DEBUG2( "CThumbAGrocessor::RunError() %d", aError);
#ifdef _DEBUG
    OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_RUNERROR, "CThumbAGProcessor::RunError;aError=%d", aError );
#else
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_RUNERROR, "CThumbAGProcessor::RunError");
#endif    
    UpdatePSValues(EFalse, EFalse);
        
    iActiveCount--;
    
	ActivateAO();
	
    // nothing to do
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::ActivateAO()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::ActivateAO()
    {
    UpdateItemCounts();
    
    if(iFormatting)
        {
        TN_DEBUG1( "CThumbAGProcessor::ActivateAO() - FORMATTING - DAEMON ON PAUSE");
        OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_ACTIVATEAO, "CThumbAGProcessor::ActivateAO - FORMATTING - DAEMON ON PAUSE" );
        return;
        }
    
    //check if forced run needs to continue
    if ( (iModifyItemCount > 0 || iDeleteItemCount > 0 ||  iUnknownItemCount > 0) && !iMountTimer->IsActive())
        {
        TN_DEBUG1( "CThumbAGProcessor::ActivateAO() -  forced run");
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_ACTIVATEAO, "CThumbAGProcessor::ActivateAO - iModifyItemCount > 0 || iUnknownItemCount > 0 || iDeleteItemCount > 0" );
        SetForceRun( ETrue );
        }
    else
        {
        iModify = EFalse;
        SetForceRun( EFalse );
        }
    
    if( !IsActive() && !iShutdown && ((iActiveCount < KMaxDaemonRequests && !iQueryActive) || iForceRun ))
        {
        TN_DEBUG1( "CThumbAGProcessor::ActivateAO() - Activated");
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_ACTIVATEAO, "CThumbAGProcessor::ActivateAO - Activated" );
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }

    UpdatePSValues(EFalse, EFalse);
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::PeriodicTimerCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGProcessor::PeriodicTimerCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGProcessor::PeriodicTimerCallBack()");
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_PERIODICTIMERCALLBACK, "CThumbAGProcessor::PeriodicTimerCallBack" );
    CThumbAGProcessor* self = static_cast<CThumbAGProcessor*>( aAny );
    
    self->CancelTimeout();
    self->ActivateAO();

    return KErrNone; // Return value ignored by CPeriodic
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::CheckAutoCreateValuesL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::CheckAutoCreateValuesL()
    {
    CRepository* rep = CRepository::NewL( TUid::Uid( THUMBNAIL_CENREP_UID ));
    
    TBool imageGrid( EFalse );
    TBool imageList( EFalse );
    TBool imageFull( EFalse );
    TBool videoGrid( EFalse );
    TBool videoList( EFalse );
    TBool videoFull( EFalse );
    TBool audioGrid( EFalse );
    TBool audioList( EFalse );
    TBool audioFull( EFalse );

    // get cenrep values
    TInt ret = rep->Get( KAutoCreateImageGrid, imageGrid );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateImageGrid %d", ret);
    OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL - KAutoCreateImageGrid;ret=%d", ret );
    ret = rep->Get( KAutoCreateImageList, imageList );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateImageList %d", ret);
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL - KAutoCreateImageList;ret=%d", ret );
    ret = rep->Get( KAutoCreateImageFullscreen, imageFull );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateImageFullscreen %d", ret);
    OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL;ret=%d", ret );
    ret = rep->Get( KAutoCreateVideoGrid, videoGrid );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateVideoGrid %d", ret);
    OstTrace1( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL - KAutoCreateVideoGrid;ret=%d", ret );
    ret = rep->Get( KAutoCreateVideoList, videoList );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateVideoList %d", ret);
    OstTrace1( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL - KAutoCreateVideoList;ret=%d", ret );
    ret = rep->Get( KAutoCreateVideoFullscreen, videoFull );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateVideoFullscreen %d", ret);
    OstTrace1( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL - KAutoCreateVideoFullscreen;ret=%d", ret );
    ret = rep->Get( KAutoCreateAudioGrid, audioGrid );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateAudioGrid %d", ret);
    OstTrace1( TRACE_NORMAL, DUP6_CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL - KAutoCreateAudioGrid;ret=%d", ret );
    ret = rep->Get( KAutoCreateAudioList, audioList );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateAudioList %d", ret);
    OstTrace1( TRACE_NORMAL, DUP7_CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL - KAutoCreateAudioList;ret=%d", ret );
    ret = rep->Get( KAutoCreateAudioFullscreen, audioFull );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateAudioFullscreen %d", ret);
    OstTrace1( TRACE_NORMAL, DUP8_CTHUMBAGPROCESSOR_CHECKAUTOCREATEVALUESL, "CThumbAGProcessor::CheckAutoCreateValuesL - KAutoCreateAudioFullscreen;ret=%d", ret );
    
    iAutoImage = EFalse;
    iAutoVideo = EFalse;
    iAutoAudio = EFalse;
    
    // set processing values
    if (imageGrid || imageList || imageFull)
        {
        iAutoImage = ETrue;
        }
    if (videoGrid || videoList || videoFull)
        {
        iAutoVideo = ETrue;
        }
    if (audioGrid || audioList || audioFull)
        {
        iAutoAudio = ETrue;
        }
    
    delete rep;
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::RemoveFromQueues()
// ---------------------------------------------------------------------------
//
#ifdef _DEBUG
void CThumbAGProcessor::RemoveFromQueues( const RArray<TItemId>& aIDArray, const TBool aRemoveFromDelete )
#else
void CThumbAGProcessor::RemoveFromQueues( const RArray<TItemId>& aIDArray, const TBool /*aRemoveFromDelete*/ )
#endif
    {
    TN_DEBUG2( "CThumbAGProcessor::RemoveFromQueues() aRemoveFromDelete == %d - begin", aRemoveFromDelete );
#ifdef _DEBUG
    OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_REMOVEFROMQUEUES, "CThumbAGProcessor::RemoveFromQueues - aRemoveFromDelete;aRemoveFromDelete=%d", aRemoveFromDelete );
#else
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_REMOVEFROMQUEUES, "CThumbAGProcessor::RemoveFromQueues - aRemoveFromDelete" );
#endif
    
    TInt itemIndex(KErrNotFound);
    
    for (int i=0; i< aIDArray.Count(); i++)
        {
        TThumbnailGenerationItem item;
        item.iItemId = aIDArray[i];
        TN_DEBUG2( "CThumbAGProcessor::RemoveFromQueues() - %d", aIDArray[i]);
        OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_REMOVEFROMQUEUES, "CThumbAGProcessor::RemoveFromQueues;aIDArray[i]=%u", aIDArray[i] );

        itemIndex = iGenerationQueue.FindInOrder(item, Compare);                        
        if(itemIndex >= 0)
            {
            delete iGenerationQueue[itemIndex].iUri;
            iGenerationQueue[itemIndex].iUri = NULL;
            iGenerationQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iGenerationQueue" );
            OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_REMOVEFROMQUEUES, "CThumbAGProcessor::RemoveFromQueues - iGenerationQueue" );
            }
                
        itemIndex = iQueryQueue.FindInOrder(aIDArray[i], CompareId);                    
        if(itemIndex >= 0)
            {
            iQueryQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iQueryQueue" );
            OstTrace0( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_REMOVEFROMQUEUES, "CThumbAGProcessor::RemoveFromQueues - iQueryQueue" );
            }
        }
    
    ActivateAO();
    
    TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - end" );
    OstTrace0( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_REMOVEFROMQUEUES, "CThumbAGProcessor::RemoveFromQueues - end" );
    }
	
// ---------------------------------------------------------------------------
// CThumbAGProcessor::SetForceRun()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::SetForceRun( const TBool aForceRun)
    {
    TN_DEBUG2( "CThumbAGProcessor::SetForceRun(%d) - end", aForceRun ); 
    OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_SETFORCERUN, "CThumbAGProcessor::SetForceRun - end;aForceRun=%u", aForceRun );

    // enable forced run
    iForceRun = aForceRun;
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryForPlaceholders()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::QueryAllItemsL()
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryAllItemsL" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_QUERYALLITEMSL, "CThumbAGProcessor::QueryAllItemsL" );
    
    __ASSERT_DEBUG((iMdESession), User::Panic(_L("CThumbAGProcessor::QueryAllItemsL() !iMdeSession "), KErrBadHandle));
    
    if(!iMdESession)
         {
         return;
         }
    
    if( iQueryAllItems )
        {
        if( !iQueryAllItems->IsComplete() )
            {
            TN_DEBUG1( "CThumbAGProcessor::QueryAllItemsL active- skip" );
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_QUERYALLITEMSL, "CThumbAGProcessor::QueryAllItemsL - active- skip" );
            return;
            }
        
        // delete old query
        iQueryAllItems->Cancel();
        delete iQueryAllItems;
        iQueryAllItems = NULL;
        }
    
    TN_DEBUG1( "CThumbAGProcessor::QueryAllItemsL - start" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_QUERYALLITEMSL, "CThumbAGProcessor::QueryAllItemsL -start" );
    
    CMdEObjectDef& imageObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );
    CMdEObjectDef& videoObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Video::KVideoObject );
    CMdEObjectDef& audioObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Audio::KAudioObject );
    
    CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject);
    iQueryAllItems = iMdESession->NewObjectQueryL( *iDefNamespace, objDef, this );
        
    iQueryAllItems->SetResultMode( EQueryResultModeItem );
    
    CMdELogicCondition& rootCondition = iQueryAllItems->Conditions();
    rootCondition.SetOperator( ELogicConditionOperatorOr );
    
    rootCondition.AddObjectConditionL(imageObjDef);   
    rootCondition.AddObjectConditionL(videoObjDef);   
    rootCondition.AddObjectConditionL(audioObjDef);
    
    iQueryAllItems->FindL(KMdEQueryDefaultMaxCount, KMaxQueryBatchSize);  
    
    TN_DEBUG1( "CThumbAGProcessor::QueryAllItemsL - end" );
    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_QUERYALLITEMSL, "CThumbAGProcessor::QueryAllItemsL" );
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleCollectionMessage
// From MMPXCollectionObserver
// Handle collection message.
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::HandleCollectionMessage( CMPXMessage* aMessage, TInt aError )
    {
    if ( aError != KErrNone || !aMessage || iShutdown )
        {
        return;
        }
    
    TMPXMessageId generalId( *aMessage->Value<TMPXMessageId>( KMPXMessageGeneralId ) );
    
    TN_DEBUG2( "CThumbAGProcessor::HandleCollectionMessage KMPXMessageGeneralId=%d", generalId);
    OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_HANDLECOLLECTIONMESSAGE, "CThumbAGProcessor::HandleCollectionMessage - KMPXMessageGeneralId;generalId=%u", generalId );

	//we are interestead of only general system events
    if ( generalId == KMPXMessageGeneral )
        {
        TInt event( *aMessage->Value<TInt>( KMPXMessageGeneralEvent ) );
        TInt op( *aMessage->Value<TInt>( KMPXMessageGeneralType ) );
        TN_DEBUG3( "CThumbAGProcessor::HandleCollectionMessage KMPXMessageGeneralEvent=%d", event, op);
        OstTraceExt2( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_HANDLECOLLECTIONMESSAGE, "CThumbAGProcessor::HandleCollectionMessage - KMPXMessageGeneralEvent;event=%d;op=%d", event, op );
        if ( event == TMPXCollectionMessage::EBroadcastEvent )
            {
            switch( op )
                {
			    //when MTP sync or music collection is started then pause processing
                case EMcMsgRefreshStart:
                case EMcMsgUSBMTPStart:
                    TN_DEBUG1("CThumbAGProcessor::HandleCollectionMessage MPX refresh started" );
                    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_HANDLECOLLECTIONMESSAGE, "CThumbAGProcessor::HandleCollectionMessage - MPX refresh started" );
                    iMPXHarvesting = ETrue;
                    CancelTimeout();
                    break;
			    //when MTP sync or music collection refresh is complete then resume processing
                case EMcMsgRefreshEnd:
                case EMcMsgUSBMTPEnd:
                case EMcMsgUSBMTPNotActive:
                    TN_DEBUG1("CThumbAGProcessor::HandleCollectionMessage MPX refresh finished/not active" );
                    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_HANDLECOLLECTIONMESSAGE, "CThumbAGProcessor::HandleCollectionMessage - MPX refresh finished/not active" );
                    iMPXHarvesting = EFalse;
                    StartTimeout();
                    break;
                default:
                    break;
                }
            
            //signal Server's stores about MPX harvesting state
            if( iMPXHarvesting )
                {
                RProperty::Set(KTAGDPSNotification, KMPXHarvesting, ETrue);
                }
            else
                {
                RProperty::Set(KTAGDPSNotification, KMPXHarvesting, EFalse);
                }
                
            TN_DEBUG3( "CThumbAGProcessor::HandleCollectionMessage -- end() iHarvesting == %d, iMPXHarvesting == %d", iHarvesting, iMPXHarvesting);
            OstTraceExt2( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_HANDLECOLLECTIONMESSAGE, "CThumbAGProcessor::HandleCollectionMessage - end;iHarvesting=%u;iMPXHarvesting=%u", iHarvesting, iMPXHarvesting );
            }
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleOpenL
// From MMPXCollectionObserver
// Handles the collection entries being opened.
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::HandleOpenL( const CMPXMedia& /*aEntries*/, TInt /*aIndex*/,
                                               TBool /*aComplete*/, TInt /*aError*/ )
     {
     // not needed here
     }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleOpenL
// From MMPXCollectionObserver
// Handles an item being opened.
// -----------------------------------------------------------------------------
void CThumbAGProcessor::HandleOpenL( const CMPXCollectionPlaylist& /*aPlaylist*/, TInt /*aError*/ )
   {
   // not needed here
   }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleCollectionMediaL
// From MMPXCollectionObserver
// Handle media properties.
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::HandleCollectionMediaL( const CMPXMedia& /*aMedia*/,
                                                       TInt /*aError*/ )
    {
    // not needed here
    }

// -----------------------------------------------------------------------------
// ActivityChanged()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::ActivityChanged(const TBool aActive)
    {
    TN_DEBUG2( "void CThumbAGProcessor::ActivityChanged() aActive == %d", aActive);
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_ACTIVITYCHANGED, "CThumbAGProcessor::ActivityChanged" );
    if(aActive)
        {
        iIdle = EFalse;
        }
    else
        {
        iIdle = ETrue; 
        
        if(iGenerationQueue.Count() > 0 )
            {
            ActivateAO();
            }
        }
    }


// ---------------------------------------------------------------------------
// CThumbAGProcessor::FormatNotification
// Handles a format operation
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::FormatNotification( TBool aFormat )
    {
    TN_DEBUG2( "CThumbAGProcessor::FormatNotification(%d)", aFormat );
    OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_FORMATNOTIFICATION, "CThumbAGProcessor::FormatNotification;aFormat=%u", aFormat );
    
    iFormatting = aFormat;
    
    if(!aFormat)
        {
        //force update
        UpdatePSValues(EFalse, ETrue);
        ActivateAO();
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::RPropertyNotification
// Handles a RProperty changed operation
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::RPropertyNotification(const TInt aError, const TUid aKeyCategory, const TUint aPropertyKey, const TInt aValue)
    {
    TN_DEBUG5( "CThumbAGProcessor::RPropertyNotification() aError = %d, aPropertyKey = %d, aKeyCategory = %d, aValue = %d", aError, aPropertyKey, aKeyCategory, aValue );
    OstTraceExt2( TRACE_NORMAL, CTHUMBAGPROCESSOR_RPROPERTYNOTIFICATION, "CThumbAGProcessor::RPropertyNotification;aPropertyKey=%u;aValue=%d", aPropertyKey, aValue );
    
    if(aPropertyKey == KForceBackgroundGeneration && aKeyCategory == KTAGDPSNotification )
        {
        if( aValue == 1 && aError == KErrNone )
            {
            iForegroundRun = ETrue;
            ActivateAO();
            }
        else
            {
            iForegroundRun = EFalse;
            }
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::UpdateItemsLeft
// Update KItemsleft PS value if changed
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::UpdatePSValues(const TBool aDefine, const TBool aForce)
    {
    TInt itemsLeft(KErrNotReady);
    TBool daemonProcessing = ETrue;
    
    if(iShutdown)
        {
        RProperty::Set(KTAGDPSNotification, KDaemonProcessing, EFalse);
        RProperty::Set(KTAGDPSNotification, KItemsleft, 0 );
        return;
        }
   
    if(aDefine)
        {
        TInt ret = RProperty::Define(KTAGDPSNotification, KDaemonProcessing, RProperty::EInt);
        
        if( ret != KErrNone )
            {
            TN_DEBUG2( "CThumbAGProcessor::UpdatePSValues() define KDaemonProcessing ret = %d", ret);
            OstTrace1( TRACE_NORMAL, CTHUMBAGPROCESSOR_UPDATEPSVALUES, "CThumbAGProcessor::UpdatePSValues - define KDaemonProcessing;ret=%d", ret );
            }

        ret = RProperty::Define(KTAGDPSNotification, KItemsleft, RProperty::EInt);
        
        if( ret != KErrNone )
            {
            TN_DEBUG2( "CThumbAGProcessor::UpdatePSValues() define KItemsleft ret = %d", ret);
            OstTrace1( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_UPDATEPSVALUES, "CThumbAGProcessor::UpdatePSValues - define KItemsleft;ret=%d", ret );
            }
        }
    
        // set itemsleft = KErrNotReady (-18) and daemonProcessing = ETrue if
        // - key is initalized
        // - mount timer is pending
        // - harvester observer is not yet activated
        // - single unknown item exists in processing queue
        // - forced
       if( iMountTimer->IsActive() || aForce || aDefine  || iUnknownItemCount
               || !iHarvesterActivated  )
           {
           daemonProcessing = ETrue;
           itemsLeft = KErrNotReady;
           }
       else
           {
           itemsLeft = iAddItemCount + iModifyItemCount;
           }
       
       TN_DEBUG2( "CThumbAGProcessor::UpdatePSValues() KItemsleft == %d", itemsLeft);
       OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_UPDATEPSVALUES, "CThumbAGProcessor::UpdatePSValues;itemsLeft=%d", itemsLeft );
           
       //cancel 2nd round generarion when there is items in 1st round queues
       if(iAddItemCount && i2ndRound)
           {
           DeleteAndCancelQuery(ETrue);
           i2ndRound = EFalse;
           }
    
        if( iGenerationQueue.Count() )
            {
            daemonProcessing = ETrue;
            }
        else
            {
            daemonProcessing = EFalse;
            }
        
        if( daemonProcessing != iPreviousDaemonProcessing)
            {
            TN_DEBUG2( "CThumbAGProcessor::UpdatePSValues() update KDaemonProcessing == %d", daemonProcessing);
            OstTrace1( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_UPDATEPSVALUES, "CThumbAGProcessor::UpdatePSValues - update KDaemonProcessing;daemonProcessing=%u", daemonProcessing );
            iPreviousDaemonProcessing = daemonProcessing;
            TInt ret = RProperty::Set(KTAGDPSNotification, KDaemonProcessing, daemonProcessing);
            
            if(ret != KErrNone )
                {
                TN_DEBUG3( "CThumbAGProcessor::UpdatePSValues() set KDaemonProcessing %d failed %d", daemonProcessing, ret);
                OstTraceExt2( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_UPDATEPSVALUES, "CThumbAGProcessor::UpdatePSValues - set KDaemonProcessing;daemonProcessing=%u;ret=%d", daemonProcessing, ret );
                }
            }
        
        TInt ret = RProperty::Get(KTAGDPSNotification, KItemsleft, iPreviousItemsLeft );
        TN_DEBUG2( "CThumbAGProcessor::UpdatePSValues() iPreviousItemsLeft == %d", iPreviousItemsLeft);
        OstTrace1( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_UPDATEPSVALUES, "CThumbAGProcessor::UpdatePSValues;iPreviousItemsLeft=%d", iPreviousItemsLeft );
        
        if( itemsLeft != iPreviousItemsLeft)
            {
            TN_DEBUG2( "CThumbAGProcessor::UpdatePSValues() Set KItemsleft == %d", itemsLeft);
            OstTrace1( TRACE_NORMAL, DUP6_CTHUMBAGPROCESSOR_UPDATEPSVALUES, "CThumbAGProcessor::UpdatePSValues - Set KItemsleft;itemsLeft=%d", itemsLeft );
            iPreviousItemsLeft = itemsLeft;
            TInt ret = RProperty::Set(KTAGDPSNotification, KItemsleft, itemsLeft );
            
            if(ret != KErrNone )
                {
                TN_DEBUG3( "CThumbAGProcessor::UpdatePSValues() set KItemsleft %d failed %d", itemsLeft, ret);
                OstTraceExt2( TRACE_NORMAL, DUP7_CTHUMBAGPROCESSOR_UPDATEPSVALUES, "CThumbAGProcessor::UpdatePSValues - set KItemsleft failed;itemsLeft=%d;ret=%d", itemsLeft, ret );
                }
            }
        
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::Compare
// Comparison function for logaritmic use of queue arrays
// ---------------------------------------------------------------------------
//

TInt CThumbAGProcessor::Compare(const TThumbnailGenerationItem& aLeft, const TThumbnailGenerationItem& aRight)
    {  
    return (aLeft.iItemId - aRight.iItemId);
    }

TInt CThumbAGProcessor::CompareId(const TItemId& aLeft, const TItemId& aRight)
    {  
    return (aLeft - aRight);
    }

void CThumbAGProcessor::UpdateItemCounts()
    {
    TN_DEBUG1( "CThumbAGProcessor::UpdateItemCounts()");
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts" );
    iModifyItemCount = 0;
    iDeleteItemCount = 0;
    iAddItemCount = 0;
    iUnknownItemCount = 0;
    i2ndAddItemCount = 0;
    iPlaceholderItemCount=0;
    iCameraItemCount =0;
    iImageItemCount=0;
    iVideoItemCount=0;
    iAudioItemCount=0;
    
    for(TInt i=0; i < iGenerationQueue.Count(); i++)
    {
        TThumbnailGenerationItem& item = iGenerationQueue[i];
    
        if(item.iItemAction == EGenerationItemActionModify)
            {
            iModifyItemCount++;
            }
        
        if(item.iItemAction == EGenerationItemActionDelete)
            {
            iDeleteItemCount++;
            }
        
        if(item.iItemType == EGenerationItemTypeUnknown)
            {
            iUnknownItemCount++;
            }
        if(item.iItemAction == EGenerationItemAction2ndAdd)
            {
            i2ndAddItemCount++;
            }
        if(item.iPlaceholder)
            {
            iPlaceholderItemCount++;
            }
        if(item.iItemType == EGenerationItemTypeCamera)
            {
            iCameraItemCount++;
            }
        if(item.iItemAction == EGenerationItemActionAdd )
            {
            iAddItemCount++;
            }
        if(item.iItemType == EGenerationItemTypeAudio)
            {
            iAudioItemCount++;
            }
        if(item.iItemType == EGenerationItemTypeVideo)
            {
            iVideoItemCount++;
            }
        if(item.iItemType == EGenerationItemTypeImage)
            {
            iImageItemCount++;
            }
    }
    
    TN_DEBUG2( "CThumbAGProcessor::UpdateItemCounts() iActiveCount = %d", 
            iActiveCount);
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts - iActiveCount;iActiveCount=%u", iActiveCount );
    TN_DEBUG2( "CThumbAGProcessor::UpdateItemCounts() iPreviousItemsLeft = %d", 
            iPreviousItemsLeft);
    OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iPreviousItemsLeft=%d", iPreviousItemsLeft );
    TN_DEBUG5( "CThumbAGProcessor::UpdateItemCounts() iHarvesting == %d, iMMCHarvesting == %d, iPHHarvesting == %d, iMPXHarvesting == %d", 
            iHarvesting, iMMCHarvesting, iPHHarvesting, iMPXHarvesting);
    OstTraceExt4( TRACE_NORMAL, DUP3_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iHarvesting=%d;iMMCHarvesting=%d;iPHHarvesting=%d;iMPXHarvesting=%d", 
            iHarvesting, iMMCHarvesting, iPHHarvesting, iMPXHarvesting );

    TN_DEBUG5( "CThumbAGProcessor::UpdateItemCounts() iIdle = %d, iForegroundRun = %d, timer = %d, iForceRun = %d", 
            iIdle, iForegroundRun, iPeriodicTimer->IsActive(), iForceRun);
    OstTraceExt4( TRACE_NORMAL, DUP4_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iIdle=%d;iForegroundRun=%d;iPeriodicTimer->IsActive()=%d;iForceRun=%d", 
            iIdle, iForegroundRun, iPeriodicTimer->IsActive(), iForceRun );
			
    TN_DEBUG4( "CThumbAGProcessor::UpdateItemCounts() iModify = %d, iQueryReady = %d, iProcessingCount = %d", 
            iModify, iQueryReady, iProcessingCount);
    OstTraceExt2( TRACE_NORMAL, DUP5_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iModify=%u;iQueryReady=%u", 
            iModify, iQueryReady );
    OstTrace1( TRACE_NORMAL, DUP12_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iProcessingCount=%u", iProcessingCount );
    TN_DEBUG2( "CThumbAGProcessor::UpdateItemCounts() iMountTimer = %d", iMountTimer->IsActive());
    OstTrace1( TRACE_NORMAL, DUP6_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iMountTimer->IsActive()=%u", iMountTimer->IsActive() );
    TN_DEBUG3( "CThumbAGProcessor::UpdateItemCounts() iGenerationQueue = %d, iQueryQueue = %d", 
            iGenerationQueue.Count(), iQueryQueue.Count());
    OstTraceExt2( TRACE_NORMAL, DUP7_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iGenerationQueue.Count()=%d;iQueryQueue.Count()=%d", iGenerationQueue.Count(), iQueryQueue.Count() );
    TN_DEBUG5( "CThumbAGProcessor::UpdateItemCounts() iAddItemCount=%d, i2ndAddItemCount=%d, iModifyItemCount=%d, iDeleteItemCount=%d",
            iAddItemCount, i2ndAddItemCount, iModifyItemCount, iDeleteItemCount );
    OstTrace0( TRACE_NORMAL, DUP8_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts" );
    TN_DEBUG3( "CThumbAGProcessor::UpdateItemCounts() iUnknownItemCount=%d, iPlaceholderItemCount=%d",
            iUnknownItemCount, iPlaceholderItemCount);
    OstTraceExt2( TRACE_NORMAL, DUP9_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iUnknownItemCount=%d;iPlaceholderItemCount=%d", iUnknownItemCount, iPlaceholderItemCount );
    TN_DEBUG4( "CThumbAGProcessor::UpdateItemCounts() iAudioItemCount=%d, iVideoItemCount=%d, iImageItemCount=%d",
            iAudioItemCount, iVideoItemCount, iImageItemCount);
    OstTraceExt3( TRACE_NORMAL, DUP10_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iAudioItemCount=%u;iVideoItemCount=%d;iImageItemCount=%d", 
            iAudioItemCount, iVideoItemCount, iImageItemCount );
			
    TN_DEBUG2( "CThumbAGProcessor::UpdateItemCounts() iCameraItemCount=%d", iCameraItemCount);
    OstTrace1( TRACE_NORMAL, DUP11_CTHUMBAGPROCESSOR_UPDATEITEMCOUNTS, "CThumbAGProcessor::UpdateItemCounts;iCameraItemCount=%d", iCameraItemCount );
    
    //compress queues when empty
    if(!iGenerationQueue.Count())
        {
        iGenerationQueue.Compress();
        }
    
    if(!iQueryQueue.Count())
        {
        iQueryQueue.Compress();
        }
    }


// ---------------------------------------------------------------------------
// CThumbAGProcessor::MountTimerCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGProcessor::MountTimerCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGProcessor::MountTimerCallBack()");
    OstTrace0( TRACE_NORMAL, CTHUMBAGPROCESSOR_MOUNTTIMERCALLBACK, "CThumbAGProcessor::MountTimerCallBack" );
    CThumbAGProcessor* self = static_cast<CThumbAGProcessor*>( aAny );
    
    self->iMountTimer->Cancel();
    
    //activate timeout if overall or mmc harvestig is not active
    if(!self->iHarvesting && !self->iMMCHarvesting )
        {
        self->ActivateAO();
        }

    return KErrNone; // Return value ignored by CPeriodic
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::SetGenerationItemAction()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::SetGenerationItemAction( TThumbnailGenerationItem& aGenerationItem, TThumbnailGenerationItemType aItemType )
    {
    switch( aItemType )
        {
        case EGenerationItemTypeAudio:
            aGenerationItem.iItemAction = EGenerationItemAction2ndAdd;
            break;
        case EGenerationItemTypeCamera:
            aGenerationItem.iItemAction = EGenerationItemAction2ndAdd;
            aGenerationItem.iPlaceholder = ETrue;
            break;
        case EGenerationItemTypeImage:
            aGenerationItem.iItemAction = EGenerationItemActionAdd;
            break;
        case EGenerationItemTypeVideo:
            //S^3 EGenerationItemActionAdd
            //S^4 EGenerationItemAction2ndAdd
            aGenerationItem.iItemAction = EGenerationItemAction2ndAdd;    
            break;
        default:
            aGenerationItem.iItemAction = EGenerationItemActionResolveType;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::SetGenerationItemType()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::SetGenerationItemType( TThumbnailGenerationItem& aGenerationItem, const TDefId aDefId )
    {
        if(aDefId == iImageObjectDef->Id())
          {
            aGenerationItem.iItemType = EGenerationItemTypeImage;
          }
      else if(aDefId == iAudioObjectDef->Id())
          {
          aGenerationItem.iItemType = EGenerationItemTypeAudio;
          }
      else if(aDefId == iVideoObjectDef->Id())
          {
          aGenerationItem.iItemType = EGenerationItemTypeVideo;
          }
      else
          {
          aGenerationItem.iItemType = EGenerationItemTypeUnknown;
          }
        
        SetGenerationItemAction( aGenerationItem, aGenerationItem.iItemType );
    }


// -----------------------------------------------------------------------------
// CThumbAGProcessor::AppendProcessingQueue()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::AppendProcessingQueue( TThumbnailGenerationItem& item )
    {

    TInt itemIndex = iGenerationQueue.FindInOrder( item, Compare );
           
    if(itemIndex >= 0)
       {
       iGenerationQueue[itemIndex].iPlaceholder = item.iPlaceholder;
       iGenerationQueue[itemIndex].iItemType = item.iItemType;
       iGenerationQueue[itemIndex].iItemAction = item.iItemAction;
       }
    else
       {
       iGenerationQueue.InsertInOrder(item, Compare);
       }
    }


// End of file
