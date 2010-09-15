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
* Description:  Thumbnail Auto Generate Daemon 
*
*/


#include <e32svr.h>
#include <centralrepository.h>

#include <mdesession.h>
#include <mdeconstants.h>
#include <mdequery.h>
#include <mdeobject.h>

#include "thumbagaudioobserver.h"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"


// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CThumbAGAudioObserver* CThumbAGAudioObserver::NewLC(CThumbAGProcessor* aProcessor)
    {
    TN_DEBUG1( "CThumbAGAudioObserver::NewLC() - begin" );
    
	CThumbAGAudioObserver* self = new (ELeave) CThumbAGAudioObserver(aProcessor);
	CleanupStack::PushL( self );
	self->ConstructL();
	return self;
	}
	
// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CThumbAGAudioObserver* CThumbAGAudioObserver::NewL(CThumbAGProcessor* aProcessor)
	{
	TN_DEBUG1( "CThumbAGAudioObserver::NewL() - begin" );
    
	CThumbAGAudioObserver* self = CThumbAGAudioObserver::NewLC(aProcessor);
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// CThumbAGAudioObserver
// ---------------------------------------------------------------------------
//
CThumbAGAudioObserver::CThumbAGAudioObserver(CThumbAGProcessor* aProcessor)
 	: iShutdownObserver(NULL), iMDSShutdownObserver(NULL), iMdESession(NULL), iProcessor(aProcessor)
 	{
 	// No implementation required
 	}

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CThumbAGAudioObserver::ConstructL()
	{
	TN_DEBUG1( "CThumbAGAudioObserver::ConstructL() - begin" );
	
#ifdef _DEBUG
    iAddCounter = 0;
    iModCounter = 0;
#endif
    
    InitializeL();
    	
	TN_DEBUG1( "CThumbAGAudioObserver::ConstructL() - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGAudioObserver
// ---------------------------------------------------------------------------
//
void CThumbAGAudioObserver::InitializeL()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::InitializeL() - begin" );
    
   
        TN_DEBUG1( "CThumbAGAudioObserver::InitializeL() - create observers" );
        
        // create shutdown observer
        if(iMDSShutdownObserver)
            {
            delete iMDSShutdownObserver;
            iMDSShutdownObserver = NULL;
            }     
        iMDSShutdownObserver = CTMShutdownObserver::NewL( *this, KMdSPSShutdown, KMdSShutdown, EFalse );

        if(iShutdownObserver)
            {
            delete iShutdownObserver;
            iShutdownObserver = NULL;
            }
        iShutdownObserver = CTMShutdownObserver::NewL( *this, KTAGDPSNotification, KShutdown, ETrue );  
        iShutdown = EFalse;
        
        // MDS session reconnect timer
        if (!iReconnect)
            {
            iReconnect = CPeriodic::NewL(CActive::EPriorityIdle);
            }
        
        TN_DEBUG1( "CThumbAGAudioObserver::InitializeL() - connect to MDS" );
        
        if(iMdESession)
            {
            TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
            TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
        
            // connect to MDS
            delete iMdESession;
            iMdESession = NULL;
            }

        iMdESession = CMdESession::NewL( *this );
        iSessionError = EFalse;
      
        TN_DEBUG1( "CThumbAGAudioObserver::InitializeL() - end" );
    }

// ---------------------------------------------------------------------------
// ~CThumbAGAudioObserver
// ---------------------------------------------------------------------------
//
CThumbAGAudioObserver::~CThumbAGAudioObserver()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::~CThumbAGAudioObserver() - begin" );
    
    iShutdown = ETrue;    
    
    Shutdown();
    
    TN_DEBUG1( "CThumbAGAudioObserver::~CThumbAGAudioObserver() - end" );
    }

void CThumbAGAudioObserver::Shutdown()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::Shutdown()" );
    
    delete iMDSShutdownObserver;
    iMDSShutdownObserver = NULL;
    delete iShutdownObserver;
    iShutdownObserver = NULL;
    
    if(iReconnect)
        {
        iReconnect->Cancel();
        delete iReconnect;
        iReconnect = NULL;
        }
    
    if (iMdESession)
        {
        // 2 observers
        TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
        TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
                
        delete iMdESession;
        iMdESession = NULL;
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGAudioObserver::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbAGAudioObserver::HandleSessionOpened( CMdESession& /* aSession */, TInt aError )
    {
    TN_DEBUG1( "CThumbAGAudioObserver::HandleSessionOpened");
    
    if (aError == KErrNone)
        {
        TRAPD( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGAudioObserver::HandleSessionOpened, AddObserversL error == %d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGAudioObserver::HandleSessionOpened error == %d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGAudioObserver::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGAudioObserver::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbAGAudioObserver::HandleSessionError == %d", aError );
    if (aError != KErrNone && !iSessionError)
        {
        iSessionError = ETrue;
    
        if (!iShutdown)
            {
            if (!iReconnect->IsActive())
                {
                iReconnect->Start( KMdEReconnect, KMdEReconnect, 
                                   TCallBack(ReconnectCallBack, this));
                
                TN_DEBUG1( "CThumbAGAudioObserver::HandleSessionError() - reconnect timer started" );
                }
            }

        }   
    }

// -----------------------------------------------------------------------------
// CThumbAGAudioObserver::HandleObjectNotification
// -----------------------------------------------------------------------------
//
void CThumbAGAudioObserver::HandleObjectNotification( CMdESession& /*aSession*/, 
                                               TObserverNotificationType aType,
                                               const RArray<TItemId>& aObjectIdArray )
    {
    TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - begin" );

    // no processor or shutting down
    if ( iShutdown || !iProcessor)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyAdd)
        {
        TN_DEBUG2( "CThumbAGAudioObserver::HandleObjectNotification() - ENotifyAdd %d", aObjectIdArray.Count() );
        iAddCounter = aObjectIdArray.Count();
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG2( "CThumbAGAudioObserver::HandleObjectNotification() - ENotifyModify %d", aObjectIdArray.Count() );
        iModCounter = aObjectIdArray.Count();
        }
#endif
    
    if ( (aType == ENotifyAdd || aType == ENotifyModify ) && (aObjectIdArray.Count() > 0) )
        {
        TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - AddToQueueL" );

        // Add event to processing queue by type and enable force run        
        RPointerArray<HBufC> dummyArray;
        TRAPD(err, iProcessor->AddToQueueL(aType, EGenerationItemTypeAudio, aObjectIdArray, dummyArray, EFalse));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG3( "CThumbAGAudioObserver::IN-COUNTERS---------- Add = %d Modify = %d", iAddCounter, iModCounter );
    iModCounter = 0;
    iAddCounter = 0;
#endif

    TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGAudioObserver::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGAudioObserver::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::ShutdownNotification()" );
    
    if (!iShutdown)
        {
        TN_DEBUG1( "CThumbAGAudioObserver::ShutdownNotification() shutdown" );
        iShutdown = ETrue;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGAudioObserver::AddObserversL
// ---------------------------------------------------------------------------
//
void CThumbAGAudioObserver::AddObserversL()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::AddObserversL() - begin" );
    
    CMdENamespaceDef& defaultNamespace = iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& audioDef = defaultNamespace.GetObjectDefL( MdeConstants::Audio::KAudioObject );
    
    // set observing conditions
    CMdELogicCondition* addCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
    addCondition->AddObjectConditionL( audioDef );
    
    CMdELogicCondition* modifyCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
    modifyCondition->AddObjectConditionL( audioDef );
    
    // add observer
    iMdESession->AddObjectObserverL( *this, addCondition, ENotifyAdd ); 

   // modify observer
   iMdESession->AddObjectObserverL( *this, modifyCondition, ENotifyModify );
   
   CleanupStack::Pop( 2, addCondition );
     
    TN_DEBUG1( "CThumbAGAudioObserver::AddObserversL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGAudioObserver::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGAudioObserver::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGAudioObserver::ReconnectCallBack() - reinitialize");
    
    CThumbAGAudioObserver* self = static_cast<CThumbAGAudioObserver*>( aAny );
    
    self->iReconnect->Cancel();
    
    // reconnect to MDS
    TRAP_IGNORE( self->InitializeL() );
    
    TN_DEBUG1( "CThumbAGAudioObserver::ReconnectCallBack() - done");
    
    return KErrNone;
    }


// End of file
