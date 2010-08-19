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

#include "thumbagvideoobserver.h"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"


// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CThumbAGVideoObserver* CThumbAGVideoObserver::NewLC(CThumbAGProcessor* aProcessor)
    {
    TN_DEBUG1( "CThumbAGVideoObserver::NewLC() - begin" );
    
	CThumbAGVideoObserver* self = new (ELeave) CThumbAGVideoObserver(aProcessor);
	CleanupStack::PushL( self );
	self->ConstructL();
	return self;
	}
	
// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CThumbAGVideoObserver* CThumbAGVideoObserver::NewL(CThumbAGProcessor* aProcessor)
	{
	TN_DEBUG1( "CThumbAGVideoObserver::NewL() - begin" );
    
	CThumbAGVideoObserver* self = CThumbAGVideoObserver::NewLC(aProcessor);
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// CThumbAGVideoObserver
// ---------------------------------------------------------------------------
//
CThumbAGVideoObserver::CThumbAGVideoObserver(CThumbAGProcessor* aProcessor)
 	: iShutdownObserver(NULL), iMDSShutdownObserver(NULL), iMdESession(NULL), iProcessor(aProcessor)
 	{
 	// No implementation required
 	}

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CThumbAGVideoObserver::ConstructL()
	{
	TN_DEBUG1( "CThumbAGVideoObserver::ConstructL() - begin" );
	
#ifdef _DEBUG
    iAddCounter = 0;
    iModCounter = 0;
#endif
    
    InitializeL();
    	
	TN_DEBUG1( "CThumbAGVideoObserver::ConstructL() - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGVideoObserver
// ---------------------------------------------------------------------------
//
void CThumbAGVideoObserver::InitializeL()
    {
    TN_DEBUG1( "CThumbAGVideoObserver::InitializeL() - begin" );
    
   
        TN_DEBUG1( "CThumbAGVideoObserver::InitializeL() - create observers" );
        
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
        
        TN_DEBUG1( "CThumbAGVideoObserver::InitializeL() - connect to MDS" );
        
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
      
        TN_DEBUG1( "CThumbAGVideoObserver::InitializeL() - end" );
    }

// ---------------------------------------------------------------------------
// ~CThumbAGVideoObserver
// ---------------------------------------------------------------------------
//
CThumbAGVideoObserver::~CThumbAGVideoObserver()
    {
    TN_DEBUG1( "CThumbAGVideoObserver::~CThumbAGVideoObserver() - begin" );
    
    iShutdown = ETrue;    
    
    delete iMDSShutdownObserver;
    delete iShutdownObserver;
    
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
    
    TN_DEBUG1( "CThumbAGVideoObserver::~CThumbAGVideoObserver() - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGVideoObserver::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbAGVideoObserver::HandleSessionOpened( CMdESession& /* aSession */, TInt aError )
    {
    TN_DEBUG1( "CThumbAGVideoObserver::HandleSessionOpened");
    
    if (aError == KErrNone)
        {
        TRAPD( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGVideoObserver::HandleSessionOpened, AddObserversL error == %d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGVideoObserver::HandleSessionOpened error == %d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGVideoObserver::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGVideoObserver::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbAGVideoObserver::HandleSessionError == %d", aError );
    if (aError != KErrNone && !iSessionError)
        {
        iSessionError = ETrue;
    
        if (!iShutdown)
            {
            if (!iReconnect->IsActive())
                {
                iReconnect->Start( KMdEReconnect, KMdEReconnect, 
                                   TCallBack(ReconnectCallBack, this));
                
                TN_DEBUG1( "CThumbAGVideoObserver::HandleSessionError() - reconnect timer started" );
                }
            }

        }   
    }

// -----------------------------------------------------------------------------
// CThumbAGVideoObserver::HandleObjectNotification
// -----------------------------------------------------------------------------
//
void CThumbAGVideoObserver::HandleObjectNotification( CMdESession& /*aSession*/, 
                                               TObserverNotificationType aType,
                                               const RArray<TItemId>& aObjectIdArray )
    {
    TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - begin" );

    // no processor or shutting down
    if ( iShutdown || !iProcessor)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyAdd)
        {
        TN_DEBUG2( "CThumbAGVideoObserver::HandleObjectNotification() - ENotifyAdd %d", aObjectIdArray.Count() );
        iAddCounter = aObjectIdArray.Count();
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG2( "CThumbAGVideoObserver::HandleObjectNotification() - ENotifyModify %d", aObjectIdArray.Count() );
        iModCounter = aObjectIdArray.Count();
        }
#endif
    
    if ( (aType == ENotifyAdd || aType == ENotifyModify ) && (aObjectIdArray.Count() > 0) )
        {
        TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - AddToQueueL" );

        // Add event to processing queue by type and enable force run        
        RPointerArray<HBufC> dummyArray;
        TRAPD(err, iProcessor->AddToQueueL(aType, EGenerationItemTypeVideo, aObjectIdArray, dummyArray, EFalse));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG3( "CThumbAGVideoObserver::IN-COUNTERS---------- Add = %d Modify = %d", iAddCounter, iModCounter );
    iModCounter = 0;
    iAddCounter = 0;
#endif

    TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGVideoObserver::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGVideoObserver::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGVideoObserver::ShutdownNotification()" );
    
    if (!iShutdown)
        {
        TN_DEBUG1( "CThumbAGVideoObserver::ShutdownNotification() shutdown" );
        iShutdown = ETrue;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGVideoObserver::AddObserversL
// ---------------------------------------------------------------------------
//
void CThumbAGVideoObserver::AddObserversL()
    {
    TN_DEBUG1( "CThumbAGVideoObserver::AddObserversL() - begin" );
    
    CMdENamespaceDef& defaultNamespace = iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& objectDef = defaultNamespace.GetObjectDefL( MdeConstants::Object::KBaseObject );
    CMdEPropertyDef& originPropDef = objectDef.GetPropertyDefL( MdeConstants::Object::KOriginProperty );
    CMdEObjectDef& videoDef = defaultNamespace.GetObjectDefL( MdeConstants::Video::KVideoObject );
    
    // set observing conditions
    CMdELogicCondition* addCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
	
	CMdEObjectCondition& addObjectCondition =  addCondition->AddObjectConditionL( videoDef );
	CleanupStack::PushL( &addObjectCondition );
	
	CMdEPropertyCondition& addPropertyCondition  = addCondition->AddPropertyConditionL( originPropDef, TMdEUintNotEqual(MdeConstants::Object::ECamera));
	CleanupStack::PushL( &addPropertyCondition );
    
    CMdELogicCondition* modifyCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
	CMdEObjectCondition& modifyObjectCondition =  modifyCondition->AddObjectConditionL( videoDef );
	CleanupStack::PushL( &modifyObjectCondition );
	
	CMdEPropertyCondition& modifyPropertyCondition =  modifyCondition->AddPropertyConditionL( originPropDef, TMdEUintNotEqual(MdeConstants::Object::ECamera));
	CleanupStack::PushL( &modifyPropertyCondition );
    
    // add observer
    iMdESession->AddObjectObserverL( *this, addCondition, ENotifyAdd ); 

    // modify observer
    iMdESession->AddObjectObserverL( *this, modifyCondition, ENotifyModify );
	
	CleanupStack::Pop( 6, addCondition );
     
    TN_DEBUG1( "CThumbAGVideoObserver::AddObserversL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGVideoObserver::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGVideoObserver::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGVideoObserver::ReconnectCallBack() - reinitialize");
    
    CThumbAGVideoObserver* self = static_cast<CThumbAGVideoObserver*>( aAny );
    
    self->iReconnect->Cancel();
    
    // reconnect to MDS
    TRAP_IGNORE( self->InitializeL() );
    
    TN_DEBUG1( "CThumbAGVideoObserver::ReconnectCallBack() - done");
    
    return KErrNone;
    }


// End of file
