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
#include <CoreApplicationUIsDomainPSKeys.h> 

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
    
#ifdef _DEBUG
    iAddCounter = 0;
    iModCounter = 0;
    iDelCounter = 0;
#endif
    
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
    
    //do some initializing async in RunL()
    iInit = ETrue;
    iForceRun = EFalse;    
    iActive = EFalse;
    
    iFormatObserver = CTMFormatObserver::NewL( *this );
       
    iFormatting = EFalse;     
    iSessionDied = EFalse;
    
    iCollectionUtility = NULL;
    
    iLight = CHWRMLight::NewL(this);

#ifdef _DEBUG 
    if(iLight)
        {
        iLightMask = iLight->SupportedTargets();
        TN_DEBUG2( "CThumbAGProcessor::ConstructL() - iLightMask == %d", iLightMask );
        }
#endif
        
    iActivityManager = CTMActivityManager::NewL( this, KBackgroundGenerationIdle);
    
    ActivateAO();
    
    TN_DEBUG1( "CThumbAGProcessor::ConstructL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::~CThumbAGProcessor()
// ---------------------------------------------------------------------------
//
CThumbAGProcessor::~CThumbAGProcessor()
    {
    TN_DEBUG1( "CThumbAGProcessor::~CThumbAGProcessor() - begin" );
    
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
        iHarvesterClient.RemoveHarvesterEventObserver(*this);
        iHarvesterClient.Close();
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
    iRemoveQueue.Close();
    iQueryQueue.Close();
    iPlaceholderQueue.Close();
    
    if (iTMSession)
        {
        delete iTMSession;
        iTMSession = NULL;
        }
    
    delete iFormatObserver;
    
    delete iLight;
    iLight = NULL;
    
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
               
                if (iAddQueue.Find( object->Id() ) == KErrNotFound)
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
            }
        else
            {
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
 
    iActive = EFalse; 
    
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
    
    TRAPD( err, iDefNamespace = &iMdESession->GetDefaultNamespaceDefL() );
    if (err != KErrNone)
        {
        TN_DEBUG1( "CThumbAGProcessor::SetMdESession - Error: GetDefaultNamespaceDefL leave" );
        }
    
    TRAP_IGNORE(QueryPlaceholdersL());
    TRAP_IGNORE(QueryAllItemsL());
    
	ActivateAO();
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::AddToQueue()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::AddToQueueL( TObserverNotificationType aType, 
                                    const RArray<TItemId>& aIDArray, TBool /*aPresent*/ )
    {
    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - begin" );

    // update queues
    if (aType == ENotifyAdd)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyAdd" );
        
        for (int i=0; i<aIDArray.Count(); i++)
            {
            // only add to Add queue if not already in Add queue        
            if (iAddQueue.Find( aIDArray[i] ) == KErrNotFound)
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
                // only add to Add queue if not already in Add queue        
                if (iAddQueue.Find( aIDArray[i] ) == KErrNotFound)
                    {
                    iAddQueue.AppendL(aIDArray[i]); 
                    }
                
                TInt itemIndex = iPlaceholderQueue.Find( aIDArray[i] );
                                
                if (itemIndex >= 0)
                    {
                    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - remove from placeholder queue");
                    iPlaceholderQueue.Remove( itemIndex );
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
                    iAddQueue.Append( aIDArray[i] );
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
                    
                    if( iPlaceholderQueue.Find( aIDArray[i] ) == KErrNotFound )
                        {
                        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - append to modify queue");
                        iModifyQueue.AppendL(aIDArray[i]);
                        }
                    
                    SetForceRun( ETrue );
                    } 
                }
            }
        }
        else if (aType == ENotifyRemove)
            {
            TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyRemove, remove IDs from all queues, append to Delete" );
            
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
                
                //add to remove queue
                if (iRemoveQueue.Find( aIDArray[i] ) == KErrNotFound)
                    {
                    iRemoveQueue.AppendL(aIDArray[i]);
                    }
                }
            }
#ifdef _DEBUG
        else
            {
	        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() -  should not come here" );
            __ASSERT_DEBUG((EFalse), User::Panic(_L("CThumbAGProcessor::AddToQueueL()"), -2));
			return;
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
    
    TInt orientationVal = 0;
    TInt64 modifiedVal = 0;
    
    CMdEProperty* orientation = NULL;
    CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );       
    TInt orientErr = aObject->Property( objDef.GetPropertyDefL( MdeConstants::Image::KOrientationProperty ), orientation, 0 );
    
    if (orientErr == KErrNone)
        {
        orientationVal = orientation->Uint16ValueL();
        }
        
    CMdEProperty* modified = NULL;
    CMdEObjectDef& objDef2 = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject );       
    TInt modifyErr = aObject->Property( objDef2.GetPropertyDefL( MdeConstants::Object::KLastModifiedDateProperty ), modified, 0 );
    
    if (modifyErr >= 0)
        {
        modifiedVal = modified->TimeValueL().Int64();
        }
    
    // modify existing thumbs
    if (iTMSession)
        {
        // run as lower priority than getting but higher that creating thumbnails
        TRAPD(err, iTMSession->UpdateThumbnailsL(aObject->Id(), aObject->Uri(), orientationVal, modifiedVal, CActive::EPriorityIdle ));
        
        if ( err != KErrNone )
            {
            TN_DEBUG2( "CThumbAGProcessor::UpdateThumbnailsL, iTMSession error == %d", err );
            
            iSessionDied = ETrue;
            iActive = EFalse;
            ActivateAO();
            } 
        else
            {
            iActive = ETrue;
            }
        }
    else
        {
        ActivateAO();
        }
        
#ifdef _DEBUG
    if(iModify)
        {
        iModCounter++;
        }
    else
        {
        iAddCounter++;
        }
#endif     
        
#ifdef _DEBUG
    TN_DEBUG3( "CThumbAGProcessor::OUT-COUNTERS----------, Add = %d Modify = %d", 
               iAddCounter, iModCounter );
#endif
   
    TN_DEBUG1( "CThumbAGProcessor::CreateThumbnailsL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryL()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::QueryL( RArray<TItemId>& aIDArray )
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryL() - begin" );
    
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
        TN_DEBUG2( "CThumbAGProcessor::QueryL() - fill %d", aIDArray[0] );
        iQueryQueue.Append( aIDArray[0] );
        aIDArray.Remove( 0 );
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
    
    iQueryQueue.Reset();
    
    TN_DEBUG1( "CThumbAGProcessor::QueryL() - end" );
    }


// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryForPlaceholders()
// ---------------------------------------------------------------------------
//

void CThumbAGProcessor::QueryPlaceholdersL()
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryPlaceholdersL" );
    
    CMdEObjectQuery::TState state(CMdEObjectQuery::EStateFirst);
    
    if( iQueryPlaceholders )
        {
        state = iQueryPlaceholders->State();
    
        if(state == CMdEObjectQuery::EStateSearching )
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
        delete iTMSession;
        iTMSession = NULL;
        }
    
    if (iInit)
        {
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Do Initialisation" );
        iInit = EFalse;
        TN_DEBUG1( "iHarvesterClient");
        if( iHarvesterClient.Connect() == KErrNone )
            {
            TN_DEBUG1( "iHarvesterClient connected");
            iHarvesterClient.AddHarvesterEventObserver( *this, EHEObserverTypeOverall, KMaxTInt );
            iHarvesterClient.AddHarvesterEventObserver( *this, EHEObserverTypePlaceholder, KMaxTInt );
            iHarvesterClient.AddHarvesterEventObserver( *this, EHEObserverTypeMMC, KMaxTInt );
            TN_DEBUG1( "iHarvesterClient AddHarvesterEventObserver added");
            }
        
        TN_DEBUG1( "create MMPXCollectionUtility");
        iCollectionUtility = MMPXCollectionUtility::NewL( this, KMcModeIsolated );
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Initialisation done" );
        
        iActivityManager->Start();
        
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
        if(iActiveCount <= KMaxDaemonRequests)
            {
            iActiveCount++;
            TN_DEBUG1( "CThumbAGProcessor::RunL() - waiting for previous to complete, abort..." );
            return;
            }
        else
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - iActive jammed - resetted" );
            iActive = EFalse;
            iActiveCount = 0;
            }     
        }
    else
        {
        iActiveCount = 0;   
        }
    
    //force run can proceed from this point
	if( iForceRun )
	    {
	    CancelTimeout();
	    TN_DEBUG1( "void CThumbAGProcessor::RunL() forced run, continue!");
	    }
	else
	    {
	    if( !iLights)
	        {
            iIdle = ETrue;
            }
        else
            {
            iIdle = IsInactive();
            }
        
        if( !iIdle || iHarvesting || iMPXHarvesting || iPeriodicTimer->IsActive() )
            {
            #ifdef _DEBUG
            TN_DEBUG5( "iIdle = %d, iHarvesting = %d, iMPXHarvesting = %d, iPeriodicTimer->IsActive() = %d", 
                    iIdle, iHarvesting, iMPXHarvesting, iPeriodicTimer->IsActive());
            #endif
            TN_DEBUG1( "void CThumbAGProcessor::RunL() device not idle");
            return;
            }
        else
            {
            //check is server idle
            TInt serveIdle(KErrNotFound);
            TInt ret = RProperty::Get(KServerIdle, KIdle, serveIdle);
            
            if(ret == KErrNone )
                {
                if(!serveIdle)
                    {
                    //start inactivity timer and retry on after callback
                    TN_DEBUG1( "void CThumbAGProcessor::RunL() server not idle");
                    StartTimeout();
                    return;
                    }
                }
            TN_DEBUG1( "void CThumbAGProcessor::RunL() device and server idle, process");
            }
	    }
    
    //Handle completed MDS Query
    if( iQueryReady && iProcessingCount)
        {
        TInt err(KErrNone);
        //if force or non forced
        if((iForceRun && iModify) || (!iForceRun && !iModify))
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - iQueryReady START" );
            
            const CMdEObject* object = &iQuery->Result( iProcessingCount-1 );
            iProcessingCount--;
        
            // process one item at once
            if ( object )
                {
                TRAP( err, CreateThumbnailsL(object) );
            
                if ( err != KErrNone )
                    { 
                    TN_DEBUG2( "CThumbAGProcessor::RunL(), CreateThumbnailsL error == %d", err );
                    }
                }
            }
        //force is coming, but executing non-forced query complete-> cancel old
        else
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - deleting query" );
            delete iQuery;
            iQuery = NULL;
            iQueryReady = EFalse;
            iProcessingCount = 0;
            ActivateAO();
            return;    
            }
        
        //is last query item
        if( iProcessingCount <= 0 )
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - iQueryReady FINISH" );
            iQueryReady = EFalse;
            iQueryActive = EFalse;
            iModify = EFalse;
            
            //check if forced run needs to continue
            if (iModifyQueue.Count())
                {
                iForceRun = ETrue;
                }
            else
                {
                iForceRun = EFalse;
                }   
            }
        //keep going if processing Remove items or if Add item fails
        else if( iModify || err )
            {
            ActivateAO();
            }
        }
    //waiting for MDS query to complete
    else if( iQueryActive )
        {
        if(iForceRun && !iModify)
            {
            iQuery->Cancel();
            delete iQuery;
            iQuery = NULL;
            TN_DEBUG1( "CThumbAGProcessor::RunL() - canceling query..." );
            iQueryReady = EFalse;
            iQueryActive = EFalse;
            ActivateAO();
            }
        else  
            {
            TN_DEBUG1( "CThumbAGProcessor::RunL() - waiting for query to complete, abort..." );
            }    
        }

    // select queue to process, priority by type. Process modify events before new images
    else if ( iModifyQueue.Count() > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() update thumbnails");
        
        // query for object info
        iQueryActive = ETrue;
        iModify = ETrue;
        QueryL( iModifyQueue );
       }
    else if ( iRemoveQueue.Count() > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() delete thumbnails");
        
        // delete thumbs by ID
        if (iTMSession)
            {
            iTMSession->DeleteThumbnails( iRemoveQueue[0] );
            }
        iRemoveQueue.Remove( 0 );
            
#ifdef _DEBUG
        iDelCounter++;
        TN_DEBUG2( "CThumbAGProcessor::OUT-COUNTERS----------, Delete = %d", iDelCounter );
#endif
        ActivateAO();
        }
    else if ( iAddQueue.Count() > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() create thumbnails");
        
        // query for object info
        iQueryActive = ETrue;
        
        QueryL( iAddQueue );     
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
    else
        {
        TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- type EHEObserverTypeMMC");
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
    else if( aHEObserverType == EHEObserverTypeMMC)
        {
        switch(aHarvesterEventState)
            {
            case EHEStateStarted:
            case EHEStateHarvesting:
            case EHEStateResumed:
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MMC harvesting started ");
                break;
            case EHEStateFinished:
                TN_DEBUG1( "CThumbAGProcessor::HarvestingUpdated -- MMC harvesting finished ");
                TRAP_IGNORE(QueryAllItemsL());
            break;
            };
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
TInt CThumbAGProcessor::RunError(TInt aError)
    {
    TN_DEBUG1( "CThumbAGProcessor::RunError()");
    if (aError != KErrNone)
        {
        TN_DEBUG2( "CThumbAGProcessor::RunError = %d", aError );
        }
    
    iActive = EFalse;
    
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
    TN_DEBUG6( "CThumbAGProcessor::Items in queue Add = %d, Mod = %d, Del = %d, Query = %d, iPlaceholder = %d", iAddQueue.Count(),  iModifyQueue.Count(), iRemoveQueue.Count(), iQueryQueue.Count(), iPlaceholderQueue.Count());
#endif
    
    if(iFormatting)
        {
        TN_DEBUG1( "CThumbAGProcessor::ActivateAO() - FORMATTING - DAEMON ON PAUSE");
        return;
        }
        
    if( !IsActive() )
        {
#ifdef _DEBUG
        if( iForceRun )
            {
            TN_DEBUG1( "CThumbAGProcessor::ActivateAO() - *** FORCED RUN ***");
            }
#endif
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
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
void CThumbAGProcessor::RemoveFromQueues( const RArray<TItemId>& aIDArray, const TBool aRemoveFromDelete )
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
            continue;
            }
        
        itemIndex = iModifyQueue.Find( aIDArray[i] );
         
        if(itemIndex >= 0)
            {
            iModifyQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iModifyQueue" );
			 
            if( iModifyQueue.Count() == 0)
			    {
			    iForceRun = EFalse;
		        }
			 
            continue;
            }
    
        if( aRemoveFromDelete )
            {
            itemIndex = iRemoveQueue.Find( aIDArray[i] );
             
            if(itemIndex >= 0)
                {
                iRemoveQueue.Remove(itemIndex);
                TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iRemoveQueue" );
                continue;
                }
            }
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
    if (aForceRun)
        {
        iForceRun = aForceRun;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryForPlaceholders()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::QueryAllItemsL()
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryAllItemsL" );
    
    CMdEObjectQuery::TState state(CMdEObjectQuery::EStateFirst);
    
    if( iQueryAllItems )
        {
        state = iQueryAllItems->State();
    
        if(state == CMdEObjectQuery::EStateSearching )
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
    
    TN_DEBUG1( "CThumbAGProcessor::HandleCollectionMessage" );

    TMPXMessageId generalId( *aMessage->Value<TMPXMessageId>( KMPXMessageGeneralId ) );

	//we are interestead of only general system events
    if ( generalId == KMPXMessageGeneral )
        {
        TInt event( *aMessage->Value<TInt>( KMPXMessageGeneralEvent ) );
        if ( event == TMPXCollectionMessage::EBroadcastEvent )
            {
            TInt op( *aMessage->Value<TInt>( KMPXMessageGeneralType ) );
               
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
// LightStatusChanged()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::LightStatusChanged(TInt aTarget, CHWRMLight::TLightStatus aStatus)
    {
    TN_DEBUG3( "void CThumbAGProcessor::LightStatusChanged() aTarget == %d, aStatus == %d", aTarget, aStatus);
    
     if( aStatus == CHWRMLight::ELightOff)
        {
        TN_DEBUG1( "void CThumbAGProcessor::LightStatusChanged() -- OFF");
        iLights = EFalse;
		
        if(iAddQueue.Count() + iModifyQueue.Count() + iRemoveQueue.Count() > 0 )
            {
            ActivateAO();
            }
        }
    else
        {
        TN_DEBUG1( "void CThumbAGProcessor::LightStatusChanged() -- ON");
        iLights = ETrue;
        }
    }

// -----------------------------------------------------------------------------
// IsInactive()
// -----------------------------------------------------------------------------
//
TBool CThumbAGProcessor::IsInactive()
    {
#ifdef _DEBUG
TN_DEBUG2( "CThumbAGProcessor::IsInactive()= %d", User::InactivityTime().Int());
#endif

    if( User::InactivityTime() < TTimeIntervalSeconds(KBackgroundGenerationIdle) )
      {
      return EFalse;
      }
    return ETrue;
    }

// -----------------------------------------------------------------------------
// ActivityDetected()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::ActivityDetected()
    {
    iIdle = EFalse;
    }

// -----------------------------------------------------------------------------
// InactivityDetected()
// -----------------------------------------------------------------------------
//
void CThumbAGProcessor::InactivityDetected()
    {
    iIdle = ETrue; 
    
    if(iAddQueue.Count() + iModifyQueue.Count() + iRemoveQueue.Count() > 0 )
        {
        ActivateAO();
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


// End of file
