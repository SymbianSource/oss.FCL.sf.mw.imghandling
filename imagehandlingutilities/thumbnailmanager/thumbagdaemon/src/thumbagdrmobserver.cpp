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

#include "thumbagdrmobserver.h"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"


// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CThumbAGDrmObserver* CThumbAGDrmObserver::NewLC(CThumbAGProcessor* aProcessor)
    {
    TN_DEBUG1( "CThumbAGDrmObserver::NewLC() - begin" );
    
    CThumbAGDrmObserver* self = new (ELeave) CThumbAGDrmObserver(aProcessor);
	CleanupStack::PushL( self );
	self->ConstructL();
	return self;
	}
	
// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CThumbAGDrmObserver* CThumbAGDrmObserver::NewL(CThumbAGProcessor* aProcessor)
	{
	TN_DEBUG1( "CThumbAGDrmObserver::NewL() - begin" );
    
	CThumbAGDrmObserver* self = CThumbAGDrmObserver::NewLC(aProcessor);
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// CThumbAGDrmObserver
// ---------------------------------------------------------------------------
//
CThumbAGDrmObserver::CThumbAGDrmObserver(CThumbAGProcessor* aProcessor)
 	: iShutdownObserver(NULL), iMDSShutdownObserver(NULL), iMdESession(NULL), iProcessor(aProcessor)
 	{
 	// No implementation required
 	}

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CThumbAGDrmObserver::ConstructL()
	{
	TN_DEBUG1( "CThumbAGDrmObserver::ConstructL() - begin" );
	
#ifdef _DEBUG
    iModCounter = 0;
#endif
    
    InitializeL();
    	
	TN_DEBUG1( "CThumbAGDrmObserver::ConstructL() - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGDrmObserver
// ---------------------------------------------------------------------------
//
void CThumbAGDrmObserver::InitializeL()
    {
    TN_DEBUG1( "CThumbAGDrmObserver::InitializeL() - begin" );
    
   
        TN_DEBUG1( "CThumbAGDrmObserver::InitializeL() - create observers" );
        
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
        
        TN_DEBUG1( "CThumbAGDrmObserver::InitializeL() - connect to MDS" );
        
        if(iMdESession)
            {
            // observer
            TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
        
            // connect to MDS
            delete iMdESession;
            iMdESession = NULL;
            }

        iMdESession = CMdESession::NewL( *this );
        iSessionError = EFalse;
      
        TN_DEBUG1( "CThumbAGDrmObserver::InitializeL() - end" );
    }

// ---------------------------------------------------------------------------
// ~CThumbAGDrmObserver
// ---------------------------------------------------------------------------
//
CThumbAGDrmObserver::~CThumbAGDrmObserver()
    {
    TN_DEBUG1( "CThumbAGDrmObserver::~CThumbAGDrmObserver() - begin" );
    
    iShutdown = ETrue;    
    
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
        // observer
        TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
                
        delete iMdESession;
        iMdESession = NULL;
        }
    
    TN_DEBUG1( "CThumbAGDrmObserver::~CThumbAGCameraObserver() - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGDrmObserver::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbAGDrmObserver::HandleSessionOpened( CMdESession& /* aSession */, TInt aError )
    {
    TN_DEBUG1( "CThumbAGDrmObserver::HandleSessionOpened");
    
    if (aError == KErrNone)
        {
        TRAPD( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGDrmObserver::HandleSessionOpened, AddObserversL error == %d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGDrmObserver::HandleSessionOpened error == %d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGDrmObserver::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGDrmObserver::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbAGDrmObserver::HandleSessionError == %d", aError );
    if (aError != KErrNone && !iSessionError)
        {
        iSessionError = ETrue;
    
        if (!iShutdown)
            {
            if (!iReconnect->IsActive())
                {
                iReconnect->Start( KMdEReconnect, KMdEReconnect, 
                                   TCallBack(ReconnectCallBack, this));
                
                TN_DEBUG1( "CThumbAGDrmObserver::HandleSessionError() - reconnect timer started" );
                }
            }

        }   
    }

// -----------------------------------------------------------------------------
// CThumbAGDrmObserver::HandleObjectNotification
// -----------------------------------------------------------------------------
//
void CThumbAGDrmObserver::HandleObjectNotification( CMdESession& /*aSession*/, 
                                               TObserverNotificationType aType,
                                               const RArray<TItemId>& aObjectIdArray )
    {
    TN_DEBUG1( "CThumbAGDrmObserver::HandleObjectNotification() - begin" );

    // no processor or shutting down
    if ( iShutdown || !iProcessor)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyModify)
        {
        TN_DEBUG2( "CThumbAGDrmObserver::HandleObjectNotification() - ENotifyModify %d", aObjectIdArray.Count() );
        iModCounter = aObjectIdArray.Count();
        }
#endif
    
    if ( (aType == ENotifyModify ) && (aObjectIdArray.Count() > 0) )
        {
        TN_DEBUG1( "CThumbAGDrmObserver::HandleObjectNotification() - AddToQueueL" );

        RPointerArray<HBufC> dummyArray1;
        TRAPD(err, iProcessor->AddToQueueL(ENotifyModify, EGenerationItemTypeVideo, aObjectIdArray, dummyArray1, EFalse, ETrue));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGDrmObserver::HandleObjectNotification() - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGDrmObserver::HandleObjectNotification() - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG2( "CThumbAGDrmObserver::IN-COUNTERS---------- Modify = %d", iModCounter );
    iModCounter = 0;
#endif

    TN_DEBUG1( "CThumbAGDrmObserver::HandleObjectNotification() - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGDrmObserver::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGDrmObserver::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGDrmObserver::ShutdownNotification()" );
    
    if (!iShutdown)
        {
        TN_DEBUG1( "CThumbAGDrmObserver::ShutdownNotification() shutdown" );
        iShutdown = ETrue;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGDrmObserver::AddObserversL
// ---------------------------------------------------------------------------
//
void CThumbAGDrmObserver::AddObserversL()
    {
    TN_DEBUG1( "CThumbAGDrmObserver::AddObserversL() - begin" );
    
    CMdENamespaceDef& defaultNamespace = iMdESession->GetDefaultNamespaceDefL();
    
    CMdEObjectDef& mediaObjDef = defaultNamespace.GetObjectDefL( 
        MdeConstants::MediaObject::KMediaObject );
    CMdEObjectDef& videoObjDef = defaultNamespace.GetObjectDefL( 
        MdeConstants::Video::KVideoObject );
    CMdEPropertyDef& rightsPropDef = mediaObjDef.GetPropertyDefL( 
        MdeConstants::MediaObject::KRightsStatus );


    // set observing conditions
    CMdELogicCondition* condition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
    condition->AddObjectConditionL( videoObjDef );
    condition->AddPropertyConditionL( rightsPropDef, TMdEUintEqual(MdeConstants::MediaObject::ERightsReceived));
            
    // add observers
    iMdESession->AddObjectObserverL( *this, condition, ENotifyModify ); 
   
    CleanupStack::Pop( condition );    
    
    TN_DEBUG1( "CThumbAGDrmObserver::AddObserversL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGDrmObserver::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGDrmObserver::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGDrmObserver::ReconnectCallBack() - reinitialize");
    
    CThumbAGDrmObserver* self = static_cast<CThumbAGDrmObserver*>( aAny );
    
    self->iReconnect->Cancel();
    
    // reconnect to MDS
    TRAP_IGNORE( self->InitializeL() );
    
    TN_DEBUG1( "CThumbAGDrmObserver::ReconnectCallBack() - done");
    
    return KErrNone;
    }


// End of file
