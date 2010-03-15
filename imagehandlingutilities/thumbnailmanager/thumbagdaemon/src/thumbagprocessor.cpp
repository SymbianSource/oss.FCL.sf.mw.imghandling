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

// ---------------------------------------------------------------------------
// CThumbAGProcessor::NewL()
// ---------------------------------------------------------------------------
//
CThumbAGProcessor* CThumbAGProcessor::NewL()
    {
    TN_DEBUG1( "CThumbAGProcessor::NewL() - begin" );
    
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
CThumbAGProcessor::CThumbAGProcessor(): CActive( CActive::EPriorityStandard )
    {
    TN_DEBUG1( "CThumbAGProcessor::CThumbAGProcessor() - begin" );
    
    CActiveScheduler::Add( this );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::ConstructL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::ConstructL()
    {
    TN_DEBUG1( "CThumbAGProcessor::ConstructL() - begin" );
    
    iTMSession = CThumbnailManager::NewL( *this );
    iQueryAllItems = NULL;
    iQueryPlaceholders = NULL;
    iQuery = NULL;
    iQueryActive = EFalse;
    iModify = EFalse;
    iProcessingCount = 0;
    
    iActiveCount = 0;
    
    // set auto create values from cenrep
    CheckAutoCreateValuesL();
    
    iPeriodicTimer = CPeriodic::NewL(CActive::EPriorityIdle);
    
    SetForceRun( EFalse );    
    iActive = EFalse;
    
    iFormatObserver = CTMFormatObserver::NewL( *this );
       
    iFormatting = EFalse;     
    iSessionDied = EFalse;
    
    iCollectionUtility = NULL;
    
    iActivityManager = CTMActivityManager::NewL( this, KBackgroundGenerationIdle);

    UpdatePSValues(ETrue);

    if(iForegroundGenerationObserver)
      {
      delete iForegroundGenerationObserver;
      iForegroundGenerationObserver = NULL;
      }
    
    RProperty::Define(KTAGDPSNotification, KMPXHarvesting, RProperty::EInt);
    
	//start foreground generation observer
    iForegroundGenerationObserver = CTMRPropertyObserver::NewL( *this, KTAGDPSNotification, KForceBackgroundGeneration, ETrue );  
    
    TN_DEBUG1( "CThumbAGProcessor::ConstructL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::~CThumbAGProcessor()
// ---------------------------------------------------------------------------
//
CThumbAGProcessor::~CThumbAGProcessor()
    {
    TN_DEBUG1( "CThumbAGProcessor::~CThumbAGProcessor() - begin" );
    
    if(iForegroundGenerationObserver)
      {
      delete iForegroundGenerationObserver;
      iForegroundGenerationObserver = NULL;
      }
    
    if(iActivityManager)
        {
        delete iActivityManager;
        iActivityManager = NULL;
        }
    
    if (iInactivityTimer)
        {
        iInactivityTimer->Cancel();
        delete iInactivityTimer;
        iInactivityTimer = NULL;
        }
    
    if(iPeriodicTimer)
        {
        iPeriodicTimer->Cancel();
        delete iPeriodicTimer;
        }
    
    if (!iInit)
        {
#ifdef MDS_MODIFY_OBSERVER
        iHarvesterClient.RemoveHarvesterEventObserver(*this);
        iHarvesterClient.Close();
#endif
        }
    
    if ( iCollectionUtility )
        {
        iCollectionUtility->Close();
        iCollectionUtility = NULL;
        }

    Cancel();
    
    if(iQueryPlaceholders)
        {
        iQueryPlaceholders->Cancel();
        delete iQueryPlaceholders;
        iQueryPlaceholders = NULL;
        }
    
    if (iQuery)
        {
        iQuery->Cancel();
        delete iQuery;
        iQuery = NULL;
        }
    
    if (iQueryAllItems)
       {
       iQueryAllItems->Cancel();
       delete iQueryAllItems;
       iQueryAllItems = NULL;
       }

    iAddQueue.Close();
    iModifyQueue.Close();
    iRemoveQueue.ResetAndDestroy();
    iQueryQueue.Close();
    iPlaceholderQueue.Close();
	  
    i2ndRoundGenerateQueue.Close();
    
    if (iTMSession)
        {
        delete iTMSession;
        iTMSession = NULL;
        }
    
    delete iFormatObserver;
    
    TN_DEBUG1( "CThumbAGProcessor::~CThumbAGProcessor() - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleQueryNewResults()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::HandleQueryNewResults( CMdEQuery& /*aQuery*/,
                                               const TInt /*aFirstNewItemIndex*/,
                                               const TInt /*aNewItemCount*/ )
    {
    // No implementation required
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleQueryCompleted()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::HandleQueryCompleted( CMdEQuery& aQuery, const TInt aError )
    {
    TN_DEBUG3( "CThumbAGProcessor::HandleQueryCompleted, aError == %d Count== %d", aError, aQuery.Count());
    
    if(&aQuery == iQueryPlaceholders)
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted - iQueryPlaceholders completed");
        
        iPlaceholderQueue.Reset();
        // if no errors in query
        if (aError == KErrNone )
            {
            for(TInt i = 0; i < iQueryPlaceholders->Count(); i++)
               {    
               const CMdEObject* object = &iQueryPlaceholders->Result(i);
              
               if(!object)
                   {
                   continue;
                   }
              
               if(!object->Placeholder())
                   {
                   TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted %d not placeholder", object->Id());
                   continue;
                   }
              
               /*if (iPlaceholderQueue.Find( object->Id() ) == KErrNotFound)
                   {
                   TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted %d added to placeholder queue", object->Id());*/
                   TRAP_IGNORE( iPlaceholderQueue.AppendL( object->Id() )); 
                 //}
               }
           }
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
        // if no errors in query
        if (aError == KErrNone )
            {
            for(TInt i = 0; i < iQueryAllItems->Count(); i++)
                {    
                const CMdEObject* object = &iQueryAllItems->Result(i);
               
                if(!object)
                    {
                    continue;
                    }
               
                if (iAddQueue.Find( object->Id() ) == KErrNotFound && iModifyQueue.Find( object->Id()) == KErrNotFound  )
                    {
                    TRAP_IGNORE( iAddQueue.AppendL( object->Id() ));
                    }
                }
#ifdef _DEBUG
TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted IN-COUNTERS---------- Amount: %d, Add",iQueryAllItems->Count());
#endif
            }
            //free query
            delete iQueryAllItems;
            iQueryAllItems = NULL;
        }
    else if(&aQuery == iQuery )
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted - Query completed");
        
        if(iQueryActive)
            {
            iQueryReady = ETrue;
            iQueryActive = EFalse;
            }
    
        // if no errors in query
        if (aError == KErrNone && iQuery)
            {
            iProcessingCount = iQuery->Count();
			
            if(iProcessingCount != iQueryQueue.Count())
                {
                TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() some result items missing");
                
                RArray<TItemId> iQueryQueueDelta;
                
                TInt itemIndex(KErrNotFound);
                
                //search delta items
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
                         TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() missing from results item %d", iQueryQueue[queryItem] );
                         iQueryQueueDelta.Append( iQueryQueue[queryItem] );
                         }
                     }
                 
                 TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() missing items total count %d", iQueryQueueDelta.Count()); 
                 //cleanup from previous queue it item is not found from MDS
                 while(iQueryQueueDelta.Count())
                     {
                     itemIndex = iLastQueue->Find(iQueryQueueDelta[0]);
                     if(itemIndex >= 0)
                         {
                         TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompleted() remove items %d", iQueryQueue[0]);
                         iLastQueue->Remove( itemIndex );
                         }
                     iQueryQueueDelta.Remove(0);
                     }
                 iQueryQueueDelta.Close();
                }
            
            // no results, reset query
            if( !iProcessingCount)
                {
                delete iQuery;
                iQuery = NULL;
                iProcessingCount = 0;
                iModify = EFalse;
                }
            }
        else
            {
            TInt itemIndex(KErrNotFound);
            
            //cleanup current queue
            while(iQueryQueue.Count())
                {
                itemIndex = iLastQueue->Find(iQueryQueue[0]);
                if(itemIndex >= 0)
                    {
                    iLastQueue->Remove( itemIndex );
                    }
                iQueryQueue.Remove(0);
                }
        
            delete iQuery;
            iQuery = NULL;
            iProcessingCount = 0;
            TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() Query FAILED!");   
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() - NO QUERY ACTIVE"); 
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
    // No implementation required
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::ThumbnailReady()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::ThumbnailReady( TInt aError, MThumbnailData& /*aThumbnail*/,
                                        TThumbnailRequestId /*aId*/ )
    {
    TN_DEBUG2( "CThumbAGProcessor::ThumbnailReady() aError == %d", aError );
    
    iActiveCount--;
    
    if(iActiveCount <= 0)
        {
        iActiveCount = 0;
        iActive = EFalse;
        }
    
    // TNM server died, delete session
    if( aError == KErrServerTerminated )
        {
        TN_DEBUG1( "CThumbAGProcessor::ThumbnailReady() - **** THUMBNAIL SERVER DIED ****" );
		
        iSessionDied = ETrue;
        
        if( !iPeriodicTimer->IsActive())
            {
            StartTimeout();
			}
			
        //reset PS idle so that RunL() can startup reopen TNM session and proceed
        TInt ret = RProperty::Set(KServerIdle, KIdle, ETrue);
        TN_DEBUG2( "CThumbAGProcessor::ThumbnailReady() set Idle ret = %d", ret );
        
        return;
        }
    
    ActivateAO();
	
    TN_DEBUG1( "CThumbAGProcessor::ThumbnailReady() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::SetMdESession()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::SetMdESession( CMdESession* aMdESession )
    {
    TN_DEBUG1( "CThumbAGProcessor::SetMdESession() - begin" );
    
    iMdESession = aMdESession;
    
    __ASSERT_DEBUG((iMdESession), User::Panic(_L("CThumbAGProcessor::SetMdESession() !iMdESession "), KErrBadHandle));
    
    TRAPD( err, iDefNamespace = &iMdESession->GetDefaultNamespaceDefL() );
    if (err != KErrNone)
        {
        TN_DEBUG2( "CThumbAGProcessor::SetMdESession() GetDefaultNamespaceDefL() err = %d", err );
        }
    
    __ASSERT_DEBUG((iDefNamespace), User::Panic(_L("CThumbAGProcessor::SetMdESession() !iDefNamespace "), KErrBadHandle));
       
    //do async init
    iInit = ETrue;
    
	ActivateAO();
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::AddToQueue()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::AddToQueueL( TObserverNotificationType aType, 
                                    const RArray<TItemId>& aIDArray, 
                                    const RPointerArray<HBufC>& aObjectUriArray,
                                    TBool /*aPresent*/ )
    {
    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - begin" );

    // update queues
    if (aType == ENotifyAdd)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyAdd" );
        
        for (int i=0; i<aIDArray.Count(); i++)
            {
            // do not to append to Add queue if exist already in Add or 2nd Add queue (just processed)     
            if (iAddQueue.Find( aIDArray[i] ) == KErrNotFound && i2ndRoundGenerateQueue.Find( aIDArray[i] ) == KErrNotFound)
                {
                iAddQueue.AppendL(aIDArray[i]);
                }
            }
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyModify" );
        
        if(iPHHarvesting)
            {
        
            TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - PH  harvesting active, treat like add" );
            for (int i=0; i<aIDArray.Count(); i++)
                {
                TInt itemIndex = iPlaceholderQueue.Find( aIDArray[i] );
                                
                if (itemIndex >= 0)
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - remove from placeholder queue");
                    iPlaceholderQueue.Remove( itemIndex );
                    }
                
                if(iAddQueue.Find( aIDArray[i]) == KErrNotFound && i2ndRoundGenerateQueue.Find( aIDArray[i]))
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - append to add queue");
                    iAddQueue.Append( aIDArray[i]);
                    }
                }
            }
        else
            {
            TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - PH  harvesting finished, check is real modify!" );
            
            TInt itemIndex(KErrNotFound);
            
            for (int i=0; i<aIDArray.Count(); i++)
                {
                itemIndex = iPlaceholderQueue.Find( aIDArray[i] );
                
                if (itemIndex >= 0)
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - placeholder modify, remove from placeholder queue");
                    iPlaceholderQueue.Remove( itemIndex );
                    }
                else
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - real modify");
                    itemIndex = iAddQueue.Find( aIDArray[i] );
                                    
                    if (itemIndex >= 0)
                        {
                        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - remove from add queue");
                        iAddQueue.Remove( itemIndex );
                        }
					else
						{
						
						itemIndex = i2ndRoundGenerateQueue.Find( aIDArray[i] );
                                    
	                    if (itemIndex >= 0)
	                        {
	                        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - remove from 2nd round add queue");
	                        i2ndRoundGenerateQueue.Remove( itemIndex );
	                        }
					}
                    
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - append to modify queue");
                    iModifyQueue.AppendL(aIDArray[i]);
                    
                    SetForceRun( ETrue );
                    } 
                }
            }
        }
        else if (aType == ENotifyRemove)
            {
            TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyRemove, remove IDs from all queues");
            
            for (int i=0; i<aIDArray.Count(); i++)
                {
                // can be removed from Add queue
                TInt itemIndex = iAddQueue.Find( aIDArray[i] );
                if(itemIndex >= 0)
                    {
                    iAddQueue.Remove(itemIndex);
                    }
    
                // ..and Modify Queue
                itemIndex = iModifyQueue.Find( aIDArray[i] );
                if(itemIndex >= 0)
                    {
                    iModifyQueue.Remove(itemIndex);
                    }
                }
            
            TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyRemove append URIs to remove queue");
            for (int i=0; i<aObjectUriArray.Count(); i++)
                {
                HBufC* temp = aObjectUriArray[i]->AllocL();
                iRemoveQueue.Append( temp );
                TN_DEBUG2( "CThumbAGProcessor::AddToQueueL() - %S", temp); 
                }
            }
#ifdef _DEBUG
        else
            {
	        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() -  should not come here" );
	        User::Leave( KErrArgument );
            }
#endif
    
    ActivateAO(); 
    
    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::CreateThumbnailsL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::CreateThumbnailsL( const CMdEObject* aObject )
    {
    TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() - begin" );
    
    __ASSERT_DEBUG((iTMSession), User::Panic(_L("CThumbAGProcessor::CreateThumbnailsL() !iTMSession "), KErrBadHandle));
    __ASSERT_DEBUG((iDefNamespace), User::Panic(_L("CThumbAGProcessor::CreateThumbnailsL() !iDefNamespace "), KErrBadHandle));
    
    if(!iTMSession || !iDefNamespace)
        {
        return;
        }
    
    TInt orientationVal = 0;
    TInt64 modifiedVal = 0;
    
    CMdEProperty* orientation = NULL;
    CMdEObjectDef& imageObjectDef = iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );       
    TInt orientErr = aObject->Property( imageObjectDef.GetPropertyDefL( MdeConstants::Image::KOrientationProperty ), orientation, 0 );
    
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
            iTMSession->SetQualityPreferenceL( CThumbnailManager::EOptimizeForQuality );
            }
		// 1st roung generation
        else
            {
            //1st round
            TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() EOptimizeForQualityWithPreview");
            iTMSession->SetQualityPreferenceL( CThumbnailManager::EOptimizeForQualityWithPreview );
            
            CMdEObjectDef& videoObjectDef = iDefNamespace->GetObjectDefL( MdeConstants::Video::KVideoObject );
            
            // add item to 2nd round queue 
            if(iLastQueue == &iAddQueue || iLastQueue == &iModifyQueue)
                {
                TN_DEBUG2( "CThumbAGProcessor::CreateThumbnailsL() - 1st round add/modify, append to 2nd round queue", aObject->Id() );
                if(i2ndRoundGenerateQueue.Find(aObject->Id()) == KErrNotFound)
                    {
                    i2ndRoundGenerateQueue.Append( aObject->Id() );
                    }
                }
            
           if( !(imageObjectDef.Id() == aObject->Def().Id() || videoObjectDef.Id() == aObject->Def().Id()) )
                {
                TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() 1st round and not image or video, skip");
                ActivateAO();
                return;
                }
            }

        // run as lower priority than getting but higher that creating thumbnails
        TRAPD(err, iTMSession->UpdateThumbnailsL(KNoId, aObject->Uri(), orientationVal, modifiedVal, CActive::EPriorityIdle ));
      
        if ( err != KErrNone )
            {
            TN_DEBUG2( "CThumbAGProcessor::CreateThumbnailsL, iTMSession error == %d", err );
            
            iSessionDied = ETrue;
            iActive = EFalse;
            ActivateAO();
            } 
        else
            {
            iActiveCount++;
            iActive = ETrue;
            }
        }
    else
        {
        ActivateAO();
        }
        
    TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::QueryL( RArray<TItemId>& aIDArray )
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryL() - begin" );
    
    __ASSERT_DEBUG((iMdESession), User::Panic(_L("CThumbAGProcessor::QueryL() !iMdeSession "), KErrBadHandle));
    __ASSERT_DEBUG((iDefNamespace), User::Panic(_L("CThumbAGProcessor::QueryL() !iDefNamespace "), KErrBadHandle));
    
    if(!iMdESession  || !iDefNamespace)
        {
        return;
        }
    
	//reset query queue
    iQueryQueue.Reset();
	//set reference to current pprocessing queue
    iLastQueue = &aIDArray;
    
    iQueryReady = EFalse;

    // delete old query
    if (iQuery)
        {
        delete iQuery;
        iQuery = NULL;
        }
    
    //move ID from source queue to Query queue
    TInt maxCount = aIDArray.Count();
        
    TN_DEBUG3( "CThumbAGProcessor::QueryL() - fill begin aIDArray == %d, iQueryQueue == %d", aIDArray.Count(), iQueryQueue.Count() );
    
    for(TInt i=0;i < KMaxQueryItems && i < maxCount; i++)
        {
        TN_DEBUG2( "CThumbAGProcessor::QueryL() - fill %d", aIDArray[i] );
        iQueryQueue.Append( aIDArray[i] );
        }
    
    TN_DEBUG3( "CThumbAGProcessor::QueryL() - fill end aIDArray == %d, iQueryQueue == %d", aIDArray.Count(), iQueryQueue.Count() );
    
    CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject );
    iQuery = iMdESession->NewObjectQueryL( *iDefNamespace, objDef, this );
    iQuery->SetResultMode( EQueryResultModeItem );

    CMdELogicCondition& rootCondition = iQuery->Conditions();
    rootCondition.SetOperator( ELogicConditionOperatorAnd );
    
    // add IDs
    CleanupClosePushL( iQueryQueue );
    rootCondition.AddObjectConditionL( iQueryQueue );
    CleanupStack::Pop( &iQueryQueue );
    
    // add object type conditions 
    if (!iModify)
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
    
    TN_DEBUG1( "CThumbAGProcessor::QueryL() - end" );
    }


// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryForPlaceholders()
// ---------------------------------------------------------------------------
//

void CThumbAGProcessor::QueryPlaceholdersL()
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL" );
    
    __ASSERT_DEBUG((iMdESession), User::Panic(_L("CThumbAGProcessor::QueryPlaceholdersL() !iMdeSession "), KErrBadHandle));
    __ASSERT_DEBUG((iDefNamespace), User::Panic(_L("CThumbAGProcessor::QueryPlaceholdersL() !iDefNamespace "), KErrBadHandle));
    
    if(!iMdESession  || !iDefNamespace)
         {
         return;
         }
    
    if( iQueryPlaceholders )
        {
        if( !iQueryPlaceholders->IsComplete() )
            {
            TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL active- skip" );
            return;
            }
        
        // delete old query
        iQueryPlaceholders->Cancel();
        delete iQueryPlaceholders;
        iQueryPlaceholders = NULL;
        }
   
    TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL - start" );

    CMdEObjectDef& imageObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );
    CMdEObjectDef& videoObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Video::KVideoObject );
    CMdEObjectDef& audioObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Audio::KAudioObject );
    
    CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject);
    iQueryPlaceholders = iMdESession->NewObjectQueryL( *iDefNamespace, objDef, this );
        
    iQueryPlaceholders->SetResultMode( EQueryResultModeItem );
    
    CMdELogicCondition& rootCondition = iQueryPlaceholders->Conditions();
    rootCondition.SetOperator( ELogicConditionOperatorOr );
    
    CMdEObjectCondition& imagePHObjectCondition = rootCondition.AddObjectConditionL(imageObjDef);
    imagePHObjectCondition.SetPlaceholderOnly( ETrue );
    imagePHObjectCondition.SetNotPresent( ETrue );
    
    CMdEObjectCondition& videoPHObjectCondition = rootCondition.AddObjectConditionL(videoObjDef);
    videoPHObjectCondition.SetPlaceholderOnly( ETrue );
    videoPHObjectCondition.SetNotPresent( ETrue );
    
    CMdEObjectCondition& audioPHObjectCondition = rootCondition.AddObjectConditionL(audioObjDef);
    audioPHObjectCondition.SetPlaceholderOnly( ETrue );
    audioPHObjectCondition.SetNotPresent( ETrue );
    
    iQueryPlaceholders->FindL();  
   
    TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL - end" );
    }


// ---------------------------------------------------------------------------
// CThumbAGProcessor::RunL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::RunL()
    {
    TN_DEBUG1( "CThumbAGProcessor::RunL() - begin" );
    
    if (iSessionDied)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - iSessionDied" );
        delete iTMSession;
        iTMSession = NULL;
        }
    
    if (iInit)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Do Initialisation 1" );
        
        iInit = EFalse;
        iInit2 = ETrue;

        iAddQueue.Reset();
        iModifyQueue.Reset();
        iRemoveQueue.ResetAndDestroy();
        iQueryQueue.Reset();
        iPlaceholderQueue.Reset();
        
        TRAP_IGNORE(QueryPlaceholdersL());
		//query all items after PH query
        iDoQueryAllItems = ETrue;
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Initialisation 1 done" );
        ActivateAO();
        return;
        }
    
    if(iInit2)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Do Initialisation 2" );
		
        iInit2 = EFalse;
        TInt err(KErrNone);
        
#ifdef  MDS_MODIFY_OBSERVER        
        TN_DEBUG1( "CThumbAGProcessor::RunL() do iHarvesterClient connect");
        err = iHarvesterClient.Connect();
        TN_DEBUG2( "CThumbAGProcessor::RunL() iHarvesterClient connect err = %d", err);
        
        __ASSERT_DEBUG((err==KErrNone), User::Panic(_L("CThumbAGProcessor::RunL(), !iHarvesterClient "), err));
        
        if(  err == KErrNone )
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() add iHarvesterClient observer");
            err = iHarvesterClient.AddHarvesterEventObserver( *this, EHEObserverTypeOverall | EHEObserverTypePlaceholder, KMaxTInt );
            TN_DEBUG2( "CThumbAGProcessor::RunL() iHarvesterClient observer err = %d", err);
            __ASSERT_DEBUG((err==KErrNone), User::Panic(_L("CThumbAGProcessor::RunL(), !iHarvesterClient "), err));
            }
#endif
 
        TN_DEBUG1( "CThumbAGProcessor::RunL() MMPXCollectionUtility");
        TRAP( err, iCollectionUtility = MMPXCollectionUtility::NewL( this, KMcModeIsolated ));
        TN_DEBUG2( "CThumbAGProcessor::RunL() create MMPXCollectionUtility err = %d", err);
        __ASSERT_DEBUG((iCollectionUtility), User::Panic(_L("CThumbAGProcessor::RunL(), !iCollectionUtility "), err));
        
        __ASSERT_DEBUG((iActivityManager), User::Panic(_L("CThumbAGProcessor::RunL(), !iActivityManager "), KErrBadHandle));
        if(iActivityManager)
            {
            iActivityManager->Start();
            }
        
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Initialisation 2 done" );
        return;
        }
    
    // restart session if died
    if (!iTMSession)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - open TNM session");
        iActive = EFalse;
        TRAPD( err, iTMSession = CThumbnailManager::NewL( *this ) );
		
        if (err != KErrNone)
            {
            iTMSession = NULL;
            ActivateAO();
            TN_DEBUG2( "CThumbAGProcessor::RunL() - Session restart failed, error == %d", err );
            }        
        else 
            {
            iSessionDied = EFalse;
            }
        }    
   
    // do not run if request is already issued to TNM server even if forced
    if( iActive)
        {
        if(iActiveCount >= KMaxDaemonRequests)
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - waiting for previous to complete, abort..." );
            return;
            }
        }
    else
        {
        iActiveCount = 0;   
        }
    
    
    //force run can proceed from this point
#ifdef _DEBUG
	if( iForegroundRun )
		{
      	TN_DEBUG1( "void CThumbAGProcessor::RunL() KForceBackgroundGeneration enabled");
	  	}
	
    if( iForceRun )
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - *** FORCED RUN ***");
        }
#endif
	
  	if( iForceRun || iForegroundRun )
      	{
        TN_DEBUG1( "void CThumbAGProcessor::RunL() skip idle detection!");
      	CancelTimeout();
     	 }
  	else
	    {
        if(iActivityManager)
            {
            iIdle = iActivityManager->IsInactive();
            }
	    
        if( !iIdle || iHarvesting || iMPXHarvesting || iPeriodicTimer->IsActive() )
            {
            TN_DEBUG1( "void CThumbAGProcessor::RunL() device not idle");
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
                    StartTimeout();
                    return;
                }
            TN_DEBUG1( "void CThumbAGProcessor::RunL() device and server idle, process");
            }
	    }
    
    //Handle completed MDS Query
    if( iQueryReady && iProcessingCount)
        {
        TInt err(KErrNone);
        //if force or non forced
        if((iForceRun && iModify ) || (!iForceRun && !iModify ))
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - iQueryReady START" );
            
            const CMdEObject* object = &iQuery->Result( iProcessingCount-1 );
            iProcessingCount--;
            
            TInt itemIndex = iLastQueue->Find( object->Id());
            if(itemIndex >= 0)
                {
                iLastQueue->Remove(itemIndex);
                }
				
            // process one item at once
            if ( object )
                {
                //remove item from queryQueue when request is issued 
                itemIndex = iQueryQueue.Find( object->Id());
                if(itemIndex >= 0)
                    {
                    iQueryQueue.Remove(itemIndex);
                    }
            
                TRAP( err, CreateThumbnailsL(object) );
                TN_DEBUG2( "CThumbAGProcessor::RunL(), CreateThumbnailsL error == %d", err );
                __ASSERT_DEBUG((err==KErrNone), User::Panic(_L("CThumbAGProcessor::RunL(), CreateThumbnailsL() "), err));
                }
            }
        //force is coming, but executing non-forced query complete-> cancel old
        else
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - deleting query 1" );
            delete iQuery;
            iQuery = NULL;
            iQueryReady = EFalse;
            iProcessingCount = 0;
            
            //move remainig IDs in query queue back to original queue
            while(iQueryQueue.Count())
                {
                if(iLastQueue)
                    {
                    if(iLastQueue->Find( iQueryQueue[0]) == KErrNotFound)
                        {
                        iLastQueue->Append(iQueryQueue[0]);
                        }
                    }
                iQueryQueue.Remove(0);
                }
            iLastQueue = NULL;
            ActivateAO();
            return;    
            }
        
        //is last query item
        if( iProcessingCount <= 0 )
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - iQueryReady FINISH" );
            iQueryReady = EFalse;
            iQueryActive = EFalse;
            }
            
        ActivateAO();
        }
    //waiting for MDS query to complete
    else if( iQueryActive )
        {
        if(iForceRun && !iModify)
            {
            if(iQuery)
                {
                TN_DEBUG1( "CThumbAGProcessor::RunL() - deleting query 2" );
                iQuery->Cancel();
                delete iQuery;
                iQuery = NULL;
                }

            iQueryReady = EFalse;
            iQueryActive = EFalse;
            
            //move remainig IDs in query queue back to original queue
            while(iQueryQueue.Count())
                {
                if(iLastQueue)
                    {
                    if(iLastQueue->Find( iQueryQueue[0]) == KErrNotFound)
                        {
                        iLastQueue->Append(iQueryQueue[0]);
                        }
                    }
                iQueryQueue.Remove(0);
                }
            iLastQueue = NULL;
            
            ActivateAO();
            }
        else  
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - waiting for query to complete, abort..." );
            }    
        }

    // no items in query queue, start new
    // select queue to process, priority by type
    else if ( iModifyQueue.Count() > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() update thumbnails");
        
        i2ndRound = EFalse;
        
        // query for object info
        iQueryActive = ETrue;
        iModify = ETrue;
        QueryL( iModifyQueue );
       }
    else if ( iAddQueue.Count() > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() update 1st round thumbnails");
        
        i2ndRound = EFalse;
        
        // query for object info
        iQueryActive = ETrue;
        
        QueryL( iAddQueue );     
        }
    else if ( iRemoveQueue.Count() > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() delete thumbnails");

        i2ndRound = EFalse;
        
        // delete thumbs by URI
        __ASSERT_DEBUG((iTMSession), User::Panic(_L("CThumbAGProcessor::RunL() !iTMSession "), KErrBadHandle));
        if(iTMSession)
            {
            HBufC* uri = iRemoveQueue[0];
            TN_DEBUG2( "void CThumbAGProcessor::RunL() delete %S",  uri);
            CThumbnailObjectSource* source = NULL;
            TRAPD(err,  source = CThumbnailObjectSource::NewL( *uri, KNullDesC));
               
        	if(err == KErrNone)
            	{
                iTMSession->DeleteThumbnails( *source );
                }
            iRemoveQueue.Remove( 0 );
            delete source;
            delete uri;
            }
            
        ActivateAO();
        }
    else if( i2ndRoundGenerateQueue.Count() > 0)
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() update 2nd round thumbnails");
            
        // query for object info
        iQueryActive = ETrue;
        i2ndRound = ETrue;
        QueryL( i2ndRoundGenerateQueue );     
        }
        
    TN_DEBUG1( "CThumbAGProcessor::RunL() - end" );
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
         TInt /*aItemsLeft*/ )
    {
    TN_DEBUG3( "CThumbAGProcessor::HarvestingUpdated -- start() aHEObserverType = %d, aHarvesterEventState = %d", aHEObserverType, aHarvesterEventState );

    #ifdef _DEBUG
    if( aHEObserverType == EHEObserverTypePlaceholder)
        {
        TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypePlaceholder");
        }
    else if( aHEObserverType == EHEObserverTypeOverall)
        {
        TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypeOverall");
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
                }
            else
                {
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS placeholder harvesting finished");
                TRAP_IGNORE(QueryPlaceholdersL());
                }
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
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS harvesterin started");
                CancelTimeout();
                }
            else
                {
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MDS harvesting finished ");
                // continue processing if needed
                StartTimeout();
                }
            }
        }
   
    TN_DEBUG3( "CThumbAGProcessor::HarvestingUpdated -- end() iHarvesting == %d, iPHHarvesting == %d ", iHarvesting, iPHHarvesting);
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::StartTimeout()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::StartTimeout()
    {
    TN_DEBUG1( "CThumbAGProcessor::StartTimeout()");
    CancelTimeout();
    
    if(!iHarvesting && !iMPXHarvesting && !iPeriodicTimer->IsActive())
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
    
    UpdatePSValues();
        
    iActiveCount--;
    
    if(iActiveCount <= 0)
        {
        iActiveCount = 0;
        iActive = EFalse;
        }
    
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
#ifdef _DEBUG
    TN_DEBUG6( "CThumbAGProcessor::ActivateAO() items in queue Add = %d, Mod = %d, Del = %d, Query = %d, iPlaceholder = %d", iAddQueue.Count(),  iModifyQueue.Count(), iRemoveQueue.Count(), iQueryQueue.Count(), iPlaceholderQueue.Count());
    TN_DEBUG2( "CThumbAGProcessor::ActivateAO() items in queue 2nd Add = %d", i2ndRoundGenerateQueue.Count());
    TN_DEBUG3( "CThumbAGProcessor::ActivateAO() iActive = %d, iActiveCount = %d", iActive, iActiveCount);
    TN_DEBUG3( "CThumbAGProcessor::ActivateAO() iHarvesting == %d, iMPXHarvesting == %d", iHarvesting, iMPXHarvesting);
    TN_DEBUG4( "CThumbAGProcessor::ActivateAO() iIdle = %d, timer = %d, iForceRun = %d", iIdle, iPeriodicTimer->IsActive(), iForceRun);
    TN_DEBUG4( "CThumbAGProcessor::ActivateAO() iModify = %d, iQueryReady = %d, iProcessingCount = %d", iModify, iQueryReady, iProcessingCount);
#endif
    
    if(iFormatting)
        {
        TN_DEBUG1( "CThumbAGProcessor::ActivateAO() - FORMATTING - DAEMON ON PAUSE");
        return;
        }
        
    if( !IsActive() )
        {
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
    
    //check if forced run needs to continue
    if (iModifyQueue.Count())
        {
        SetForceRun( ETrue );
        }
    else
        {
        iModify = EFalse;
        SetForceRun( EFalse );
        }

    UpdatePSValues();
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::PeriodicTimerCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGProcessor::PeriodicTimerCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGProcessor::PeriodicTimerCallBack()");
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
    ret = rep->Get( KAutoCreateImageList, imageList );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateImageList %d", ret);
    ret = rep->Get( KAutoCreateImageFullscreen, imageFull );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateImageFullscreen %d", ret);
    ret = rep->Get( KAutoCreateVideoGrid, videoGrid );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateVideoGrid %d", ret);
    ret = rep->Get( KAutoCreateVideoList, videoList );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateVideoList %d", ret);
    ret = rep->Get( KAutoCreateVideoFullscreen, videoFull );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateVideoFullscreen %d", ret);
    ret = rep->Get( KAutoCreateAudioGrid, audioGrid );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateAudioGrid %d", ret);
    ret = rep->Get( KAutoCreateAudioList, audioList );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateAudioList %d", ret);
    ret = rep->Get( KAutoCreateAudioFullscreen, audioFull );
    TN_DEBUG2( "CThumbAGProcessor::CheckAutoCreateValuesL() KAutoCreateAudioFullscreen %d", ret);
    
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
    
    TInt itemIndex = KErrNotFound;
    
    for (int i=0; i< aIDArray.Count(); i++)
        {
        TN_DEBUG2( "CThumbAGProcessor::RemoveFromQueues() - %d", aIDArray[i]);

        itemIndex = iPlaceholderQueue.Find( aIDArray[i] );
                         
        if(itemIndex >= 0)
            {
            iPlaceholderQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iPlaceholderQueue" );
            }
                
        itemIndex = iAddQueue.Find( aIDArray[i] );
        
        if(itemIndex >= 0)
            {
            iAddQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iAddQueue" );
            }

        itemIndex = i2ndRoundGenerateQueue.Find( aIDArray[i] );
                
        if(itemIndex >= 0)
            {
            i2ndRoundGenerateQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - i2ndRoundGenerateQueue" );
            }
        
        itemIndex = iModifyQueue.Find( aIDArray[i] );
         
        if(itemIndex >= 0)
            {
            iModifyQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iModifyQueue" );
			 
            if( iModifyQueue.Count() == 0)
			    {
			    SetForceRun( EFalse );
		        }
			 
            continue;
            }
    
        /*if( aRemoveFromDelete )
            {
            itemIndex = iRemoveQueue.Find( aIDArray[i] );
             
            if(itemIndex >= 0)
                {
                iRemoveQueue.Remove(itemIndex);
                TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iRemoveQueue" );
                continue;
                }
            }*/
        }
    
    TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - end" );
    }
	
// ---------------------------------------------------------------------------
// CThumbAGProcessor::SetForceRun()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::SetForceRun( const TBool aForceRun)
    {
    TN_DEBUG2( "CThumbAGProcessor::SetForceRun(%d) - end", aForceRun ); 

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
            return;
            }
        
        // delete old query
        iQueryAllItems->Cancel();
        delete iQueryAllItems;
        iQueryAllItems = NULL;
        }
    
    TN_DEBUG1( "CThumbAGProcessor::QueryAllItemsL - start" );
    
    CMdEObjectDef& imageObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );
    CMdEObjectDef& videoObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Video::KVideoObject );
    CMdEObjectDef& audioObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Audio::KAudioObject );
    
    CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject);
    iQueryAllItems = iMdESession->NewObjectQueryL( *iDefNamespace, objDef, this );
        
    iQueryAllItems->SetResultMode( EQueryResultModeItem );
    
    CMdELogicCondition& rootCondition = iQueryAllItems->Conditions();
    rootCondition.SetOperator( ELogicConditionOperatorOr );
    
    CMdEObjectCondition& imagePHObjectCondition = rootCondition.AddObjectConditionL(imageObjDef);
    
    CMdEObjectCondition& videoPHObjectCondition = rootCondition.AddObjectConditionL(videoObjDef);
    
    CMdEObjectCondition& audioPHObjectCondition = rootCondition.AddObjectConditionL(audioObjDef);
    
    iQueryAllItems->FindL();  
    
    TN_DEBUG1( "CThumbAGProcessor::QueryAllItemsL - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleCollectionMessage
// From MMPXCollectionObserver
// Handle collection message.
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::HandleCollectionMessage( CMPXMessage* aMessage, TInt aError )
    {
    if ( aError != KErrNone || !aMessage )
        {
        return;
        }
    
    TMPXMessageId generalId( *aMessage->Value<TMPXMessageId>( KMPXMessageGeneralId ) );
    
    TN_DEBUG2( "CThumbAGProcessor::HandleCollectionMessage KMPXMessageGeneralId=%d", generalId);

	//we are interestead of only general system events
    if ( generalId == KMPXMessageGeneral )
        {
        TInt event( *aMessage->Value<TInt>( KMPXMessageGeneralEvent ) );
        TInt op( *aMessage->Value<TInt>( KMPXMessageGeneralType ) );
        TN_DEBUG3( "CThumbAGProcessor::HandleCollectionMessage KMPXMessageGeneralEvent=%d", event, op);
        if ( event == TMPXCollectionMessage::EBroadcastEvent )
            {
            switch( op )
                {
			    //when MTP sync or music collection is started then pause processing
                case EMcMsgRefreshStart:
                case EMcMsgUSBMTPStart:
                    TN_DEBUG1("CThumbAGProcessor::HandleCollectionMessage MPX refresh started" );
                    iMPXHarvesting = ETrue;
                    CancelTimeout();
                    break;
			    //when MTP sync or music collection refresh is complete then resume processing
                case EMcMsgRefreshEnd:
                case EMcMsgUSBMTPEnd:
                case EMcMsgUSBMTPNotActive:
                    TN_DEBUG1("CThumbAGProcessor::HandleCollectionMessage MPX refresh finished/not active" );
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
    if(aActive)
        {
        iIdle = EFalse;
        }
    else
        {
        iIdle = ETrue; 
        
        if(iAddQueue.Count() + iModifyQueue.Count() + iRemoveQueue.Count() + i2ndRoundGenerateQueue.Count() > 0 )
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
    
    iFormatting = aFormat;
    if(!aFormat)
        {
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
void CThumbAGProcessor::UpdatePSValues(const TBool aDefine)
    {
    TInt itemsLeft = iModifyQueue.Count() + iAddQueue.Count();
    TBool daemonProcessing = EFalse;
    
    if(itemsLeft + i2ndRoundGenerateQueue.Count() + iRemoveQueue.Count() > 0 )
        {
        daemonProcessing = ETrue;
        }
    
    if(aDefine)
        {
        TN_DEBUG1( "CThumbAGProcessor::UpdatePSValues() define");
        RProperty::Define(KTAGDPSNotification, KDaemonProcessing, RProperty::EInt);
        RProperty::Set(KTAGDPSNotification, KDaemonProcessing, 0);
        daemonProcessing = EFalse;
        RProperty::Define(KTAGDPSNotification, KItemsleft, RProperty::EInt);
        RProperty::Set(KTAGDPSNotification, KItemsleft, 0);
        iPreviousItemsLeft = 0;
        }
    
    if( daemonProcessing != iPreviousDaemonProcessing)
        {
        TN_DEBUG2( "CThumbAGProcessor::UpdatePSValues() update KDaemonProcessing == %d", daemonProcessing);
        iPreviousDaemonProcessing = daemonProcessing;
        RProperty::Set(KTAGDPSNotification, KDaemonProcessing, daemonProcessing);
        }
    
    if( itemsLeft != iPreviousItemsLeft)
        {
        TN_DEBUG2( "CThumbAGProcessor::UpdatePSValues() update KItemsleft == %d", itemsLeft);
        iPreviousItemsLeft = itemsLeft;
        RProperty::Set(KTAGDPSNotification, KItemsleft, itemsLeft );
        }
    }

// End of file
