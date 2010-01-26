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

#include "thumbagprocessor.h"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"
#include "thumbagformatobserver.h"
#include <mpxcollectionutility.h>
#include <mpxmessagegeneraldefs.h>
#include <mpxcollectionmessage.h>

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
    
    iFormatObserver = CThumbAGFormatObserver::NewL( this );
       
    iFormatting = EFalse;     
    iSessionDied = EFalse;
    
    iCollectionUtility = NULL;
        
    TN_DEBUG1( "CThumbAGProcessor::ConstructL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::~CThumbAGProcessor()
// ---------------------------------------------------------------------------
//
CThumbAGProcessor::~CThumbAGProcessor()
    {
    TN_DEBUG1( "CThumbAGProcessor::~CThumbAGProcessor() - begin" );
    
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
    
    if (iQuery)
        {
        iQuery->Cancel();
        delete iQuery;
        iQuery = NULL;
        }
    
    if (iQueryForPlaceholders)
       {
       iQueryForPlaceholders->Cancel();
       delete iQueryForPlaceholders;
       iQueryForPlaceholders = NULL;
       }
    
    iAddQueue.Close();
    iModifyQueue.Close();
    iRemoveQueue.Close();
    iQueryQueue.Close();
    iPresentQueue.Close();
    iTempModifyQueue.Close();
    iTempAddQueue.Close();
    iPlaceholderIDs.Close();
    
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
void CThumbAGProcessor::HandleQueryCompleted( CMdEQuery& /*aQuery*/, const TInt aError )
    {

    if( iQueryForPlaceholdersActive && iQueryForPlaceholders)
        {
        TN_DEBUG3( "CThumbAGProcessor::HandleQueryCompletedv2, aError == %d Count== %d", aError, iQueryForPlaceholders->Count());
        
        // if no errors in query
        if (aError == KErrNone )
            {
            
            for(TInt i = 0; i < iQueryForPlaceholders->Count(); i++)
                {    
                const CMdEObject* object = &iQueryForPlaceholders->Result(i);
                
                if(!object)
                    continue;
                
                if(!object->Placeholder())
                    {
                    TN_DEBUG2( "CThumbAGProcessor::HandleQueryCompletedv2 %d not placeholder",object->Id());
                    continue;
                    }
                
                if (iPlaceholderIDs.Find( object->Id() ) == KErrNotFound)
                   {
                   TRAP_IGNORE( iPlaceholderIDs.AppendL( object->Id() )); 
                   }
                }
                
            }
        else
            {
            TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() FAILED!");   
            }
        
        iQueryForPlaceholdersActive = EFalse;
        }
    
    else if(iQueryActive)
        {
        TN_DEBUG3( "CThumbAGProcessor::HandleQueryCompleted, aError == %d Count== %d", aError, iQuery->Count());
        iQueryReady = ETrue;
        iQueryActive = EFalse;
    
        // if no errors in query
        if (aError == KErrNone && iQuery)
            {
            iProcessingCount = iQuery->Count();
            }
        else
            {
            iProcessingCount = 0;
            TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() FAILED!");   
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGProcessor::HandleQueryCompleted() - NO QUERY ACTIVE"); 
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
        
        if( !iTimerActive)
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
	ActivateAO();
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::AddToQueue()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::AddToQueueL( TObserverNotificationType aType, 
                                    const RArray<TItemId>& aIDArray, TBool aPresent )
    {
    TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - begin" );
    
    if(aPresent)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - Add to SetPresentQueue" );
        for (int i=0; i<aIDArray.Count(); i++)
             {
             // only add to Present queue if not already in Add or Present queue        
              if (iAddQueue.Find( aIDArray[i] ) == KErrNotFound)
                  {
                  if (iPresentQueue.Find( aIDArray[i] ) == KErrNotFound)
                      {
                      iPresentQueue.AppendL(aIDArray[i]);
                      }
                  }
             }     
        }
    // update queues
    else if (aType == ENotifyAdd)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyAdd" );
        
        for (int i=0; i<aIDArray.Count(); i++)
            {
            // only add to Add queue if not already in Add queue        
            if (iAddQueue.Find( aIDArray[i] ) == KErrNotFound)
                {
                //if in Present Queue remove from there and and put to add queue
                TInt index = iPresentQueue.Find( aIDArray[i] );
                if( index != KErrNotFound)
                    {
                    iPresentQueue.Remove( index );
                    }
                iAddQueue.AppendL(aIDArray[i]); 
                iTempAddQueue.AppendL(aIDArray[i]);
                }
            }
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyModify" );
            
        for (int i=0; i<aIDArray.Count(); i++)
             {
             TInt itemIndex = iPlaceholderIDs.Find( aIDArray[i] );
             if(itemIndex >= 0 )
                 {
                 TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - harvesting now ended for this placeholder - not Real ENotifyModify" );
                 iPlaceholderIDs.Remove(itemIndex);
                 TN_DEBUG2( "CThumbAGProcessor::AddToQueueL() - placeholders left %d", iPlaceholderIDs.Count() );
                 if (iAddQueue.Find( aIDArray[i] ) == KErrNotFound)
                     {
                     iAddQueue.AppendL(aIDArray[i]);
                     }
                 if (iTempModifyQueue.Find( aIDArray[i] ) == KErrNotFound)
                     {
                     iTempModifyQueue.AppendL( aIDArray[i] );
                     }
                 }
             else if ( iTempAddQueue.Find( aIDArray[i] ) == KErrNotFound)
                 {
                 TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - Real ENotifyModify, force run" );
                 iModifyQueue.InsertL( aIDArray[i], 0 );
                  
                 TInt itemIndex = iAddQueue.Find( aIDArray[i] );
                 if(itemIndex >= 0)
                    {
                    iAddQueue.Remove(itemIndex);
                    }
                 itemIndex = iPresentQueue.Find( aIDArray[i] );
                 if(itemIndex >= 0)
                    {
                    iPresentQueue.Remove(itemIndex);
                    }
                 SetForceRun( ETrue );
                 }
             else
                 {
                 if (iTempModifyQueue.Find( aIDArray[i] ) == KErrNotFound)
                     {
                     TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - harvesting now ended for this file - not Real ENotifyModify" );
                     iTempModifyQueue.AppendL( aIDArray[i] );
                     }
                else
                     {
                     TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - harvesting ended allready for this file - Real ENotifyModify, force run" );
                     iModifyQueue.InsertL( aIDArray[i], 0 );
                     TInt itemIndex = iAddQueue.Find( aIDArray[i] );
                     if(itemIndex >= 0)
                        {
                        iAddQueue.Remove(itemIndex);
                        }
                     itemIndex = iPresentQueue.Find( aIDArray[i] );
                     if(itemIndex >= 0)
                        {
                        iPresentQueue.Remove(itemIndex);
                        }
                     SetForceRun( ETrue );
                     }
                 } 
             }        
        }
    else if (aType == ENotifyRemove)
        {
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() - ENotifyRemove" );
        
        for (int i=0; i<aIDArray.Count(); i++)
            {
            // only add to Remove queue if not already in Remove queue        
            if (iRemoveQueue.Find( aIDArray[i] ) == KErrNotFound)
                {
                iRemoveQueue.AppendL(aIDArray[i]);
                }

            // can be removed from Add queue
            TInt itemIndex = iAddQueue.Find( aIDArray[i] );
            if(itemIndex >= 0)
                {
                iAddQueue.Remove(itemIndex);
                }
            // ..and Present Queue
            itemIndex = iPresentQueue.Find( aIDArray[i] );
            if(itemIndex >= 0)
                {
                iPresentQueue.Remove(itemIndex);
                }
            // ..and Modify Queue
            itemIndex = iModifyQueue.Find( aIDArray[i] );
            if(itemIndex >= 0)
                {
                iModifyQueue.Remove(itemIndex);
                }
            }  
        }
    else
        {
        // should not come here
        TN_DEBUG1( "CThumbAGProcessor::AddToQueueL() -  should not come here" );
        return;
        }

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
    
    if( iModify )
        {
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
        iModCounter++;
#endif     
        }
    else
        {
        CThumbnailObjectSource* source = NULL;
        CMdEProperty* mimeType = NULL;
        CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject );       
        TInt mime = aObject->Property( objDef.GetPropertyDefL( MdeConstants::Object::KItemTypeProperty ), mimeType, 0 );
        
        // create new thumbs
        if (mime != KErrNotFound)
            {
            source = CThumbnailObjectSource::NewLC(aObject->Uri(), aObject->Id(), mimeType->TextValueL());
            }
        else
            {
            source = CThumbnailObjectSource::NewLC(aObject->Uri(), aObject->Id());
            }
        
        if (iTMSession)
            {
            // run as very low priority task
            TInt id = iTMSession->CreateThumbnails(*source, CActive::EPriorityIdle );
            if ( id < 0 )
                {
                TN_DEBUG2( "CThumbAGProcessor::CreateThumbnailsL, iTMSession error == %d", id );
                           
                iSessionDied = ETrue;
                iActive = EFalse;
                CleanupStack::PopAndDestroy( source );
                ActivateAO();
                } 
           else
                {
                iActive = ETrue;
                CleanupStack::PopAndDestroy( source );
                }
            
            }
        else
            {
            ActivateAO();
            }
        
#ifdef _DEBUG
        iAddCounter++;
#endif
        }
    
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
            TN_DEBUG1( "iHarvesterClient AddHarvesterEventObserver added");
            }
        
        TN_DEBUG1( "create MMPXCollectionUtility");
        iCollectionUtility = MMPXCollectionUtility::NewL( this, KMcModeIsolated );
        TN_DEBUG1( "CThumbAGProcessor::RunL() - Initialisation done" );
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
            TN_DEBUG2( "CThumbAGProcessor::RunL() - Session restart failed, error == %d", err );
            }        
        else {
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
    
    //Iforce run can proceed from this point
    if( !iForceRun )
        {
        //check if harvesting or waiting timeout
        if( iHarvesting || iTimerActive || iMPXHarvesting )
            {
            TN_DEBUG1( "void CThumbAGProcessor::RunL() Harvester or timer active, abort");
            return;
            }
        else
            {
            //check is server idle
	        TInt idle(-1);

            TInt ret = RProperty::Get(KServerIdle, KIdle, idle);
	        
	        if(ret == KErrNone )
	            {
	            if(!idle)
	                {
					//start wait timer and retry on after callback
	                TN_DEBUG1( "CThumbAGProcessor::RunL() server not idle, wait... " );
	                if( !iTimerActive)
	                    {
	                    StartTimeout();
	                    }
	                return;
	                }
	            }
	        else
	            {
	            TN_DEBUG2( "CThumbAGProcessor::RunL() get KServerIdle failed %d, continue...", ret );
	            }
            }
        }
    else
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() forced run");
        }
    
    
    //Handle completed MDS Query
    if( iQueryReady && iProcessingCount)
        {
        TInt err(KErrNone);
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
    else if( iQueryActive || iQueryForPlaceholdersActive )
        {
        if(iForceRun && !iModify && iQueryActive)
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
        TN_DEBUG1( "void CThumbAGProcessor::RunL() add thumbnails");
        
        // query for object info
        iQueryActive = ETrue;
        
        QueryL( iAddQueue );     
        }
    else if ( iPresentQueue.Count() > 0 )
        {
        TN_DEBUG1( "void CThumbAGProcessor::RunL() add thumbnails for present thumbnails" );
                
        // query for object info
        iQueryActive = ETrue;
                
        QueryL( iPresentQueue );  
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
  
    if( aHEObserverType == EHEObserverTypePlaceholder)
        {
        TRAP_IGNORE( QueryForPlaceholdersL() );
        return;
        }    
    
    if( aHEObserverType != EHEObserverTypeOverall)
        {
        return;
        }
    
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
    
    if(iHarvestingTemp == iHarvesting)
        {
        TN_DEBUG2( "CThumbAGProcessor::HarvestingUpdated -- no change %d", iHarvesting);
        }
    else
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
                 
             iTempModifyQueue.Reset();
             iTempAddQueue.Reset();
             iPlaceholderIDs.Reset();
            }
        }

    TN_DEBUG3( "CThumbAGProcessor::HarvestingUpdated -- end() iHarvesting == %d, iMPXHarvesting == %d", iHarvesting, iMPXHarvesting);
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::StartTimeout()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::StartTimeout()
    {
    CancelTimeout();
    
    if(!iHarvesting && !iMPXHarvesting && !iPeriodicTimer->IsActive())
        {
        iPeriodicTimer->Start( KHarvestingCompleteTimeout, KHarvestingCompleteTimeout, 
                TCallBack(PeriodicTimerCallBack, this));
        iTimerActive = ETrue;
        }
    else
        {
        iTimerActive = EFalse;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::StopTimeout()
// ---------------------------------------------------------------------------
//
void CThumbAGProcessor::CancelTimeout()
    {
    if(iTimerActive)
       {
       iPeriodicTimer->Cancel();
       }
    iTimerActive = EFalse;
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::RunError()
// ---------------------------------------------------------------------------
//
TInt CThumbAGProcessor::RunError(TInt aError)
    {
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
    if(iFormatting)
        {
        TN_DEBUG1( "CThumbAGProcessor::ActivateAO() - FORMATTING - DAEMON ON PAUSE");
        return;
        }
        
    if( !IsActive() && (!iHarvesting || iForceRun ))
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
    rep->Get( KAutoCreateImageGrid, imageGrid );
    rep->Get( KAutoCreateImageList, imageList );
    rep->Get( KAutoCreateImageFullscreen, imageFull );
    rep->Get( KAutoCreateVideoGrid, videoGrid );
    rep->Get( KAutoCreateVideoList, videoList );
    rep->Get( KAutoCreateVideoFullscreen, videoFull );
    rep->Get( KAutoCreateAudioGrid, audioGrid );
    rep->Get( KAutoCreateAudioList, audioList );
    rep->Get( KAutoCreateAudioFullscreen, audioFull );
    
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
                
        itemIndex = iAddQueue.Find( aIDArray[i] );
        
        if(itemIndex >= 0)
            {
            iAddQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iAddQueue" );
            continue;
            }
        
        itemIndex = iPresentQueue.Find( aIDArray[i] );
                
        if(itemIndex >= 0)
            {
            iPresentQueue.Remove(itemIndex);
            TN_DEBUG1( "CThumbAGProcessor::RemoveFromQueues() - iPresentQueue" );
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
// CThumbAGProcessor::SetFormat()
// ---------------------------------------------------------------------------
//

void CThumbAGProcessor::SetFormat(TBool aStatus)
    {
    TN_DEBUG2( "CThumbAGProcessor::SetFormat(%d) - end", aStatus );
    
    iFormatting = aStatus;
    if(!aStatus)
        {
        ActivateAO();
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGProcessor::QueryForPlaceholders()
// ---------------------------------------------------------------------------
//

void CThumbAGProcessor::QueryForPlaceholdersL()
    {
    TN_DEBUG1( "CThumbAGProcessor::QueryForPlaceholders" );
    if(iQueryForPlaceholdersActive)
        {
        TN_DEBUG1( "CThumbAGProcessor::QueryForPlaceholders - skip" );
        return;
        }
    
    TN_DEBUG1( "CThumbAGProcessor::QueryForPlaceholders - start" );

    // delete old query
    if (iQueryForPlaceholders)
       {
       iQueryForPlaceholdersActive = EFalse;
       iQueryForPlaceholders->Cancel();
       delete iQueryForPlaceholders;
       iQueryForPlaceholders = NULL;
       }
    
    CMdEObjectDef& imageObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Image::KImageObject );
    CMdEObjectDef& videoObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Video::KVideoObject );
    CMdEObjectDef& audioObjDef = iDefNamespace->GetObjectDefL( MdeConstants::Audio::KAudioObject );
    
    CMdEObjectDef& objDef = iDefNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject);
    iQueryForPlaceholders = iMdESession->NewObjectQueryL( *iDefNamespace, objDef, this );
        
    iQueryForPlaceholders->SetResultMode( EQueryResultModeItem );
    
    CMdELogicCondition& rootCondition = iQueryForPlaceholders->Conditions();
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
    
    iQueryForPlaceholders->FindL();  
    
    iQueryForPlaceholdersActive = ETrue;
    
    TN_DEBUG1( "CThumbAGProcessor::QueryForPlaceholders - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGProcessor::HandleCollectionMessage
// From MMPXCollectionObserver
// Handle collection message.
// -----------------------------------------------------------------------------
//

void CThumbAGProcessor::HandleCollectionMessage( CMPXMessage* aMessage,
                                                        TInt aError )
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
void CThumbAGProcessor::HandleOpenL( const CMPXMedia& /*aEntries*/,
                                            TInt /*aIndex*/,
                                            TBool /*aComplete*/,
                                            TInt /*aError*/ )
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

// End of file
