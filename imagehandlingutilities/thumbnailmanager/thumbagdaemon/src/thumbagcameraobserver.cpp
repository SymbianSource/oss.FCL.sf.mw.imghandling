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

#include "thumbagcameraobserver.h"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbagcameraobserverTraces.h"
#endif


// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CThumbAGCameraObserver* CThumbAGCameraObserver::NewLC(CThumbAGProcessor* aProcessor)
    {
    TN_DEBUG1( "CThumbAGCameraObserver::NewLC() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_NEWLC, "CThumbAGCameraObserver::NewLC - begin" );
    
	CThumbAGCameraObserver* self = new (ELeave) CThumbAGCameraObserver(aProcessor);
	CleanupStack::PushL( self );
	self->ConstructL();
	return self;
	}
	
// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CThumbAGCameraObserver* CThumbAGCameraObserver::NewL(CThumbAGProcessor* aProcessor)
	{
	TN_DEBUG1( "CThumbAGCameraObserver::NewL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_NEWL, "CThumbAGCameraObserver::NewL - begin" );
    
	CThumbAGCameraObserver* self = CThumbAGCameraObserver::NewLC(aProcessor);
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// CThumbAGCameraObserver
// ---------------------------------------------------------------------------
//
CThumbAGCameraObserver::CThumbAGCameraObserver(CThumbAGProcessor* aProcessor)
 	: iShutdownObserver(NULL), iMDSShutdownObserver(NULL), iMdESession(NULL), iProcessor(aProcessor)
 	{
 	// No implementation required
 	}

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CThumbAGCameraObserver::ConstructL()
	{
	TN_DEBUG1( "CThumbAGCameraObserver::ConstructL() - begin" );
	OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_CONSTRUCTL, "CThumbAGCameraObserver::ConstructL - begin" );
	
#ifdef _DEBUG
    iAddCounter = 0;
    iModCounter = 0;
#endif
    
    InitializeL();
    	
	TN_DEBUG1( "CThumbAGCameraObserver::ConstructL() - end" );
	OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_CONSTRUCTL, "CThumbAGCameraObserver::ConstructL - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGCameraObserver
// ---------------------------------------------------------------------------
//
void CThumbAGCameraObserver::InitializeL()
    {
    TN_DEBUG1( "CThumbAGCameraObserver::InitializeL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_INITIALIZEL, "CThumbAGCameraObserver::InitializeL - begin" );
    
   
        TN_DEBUG1( "CThumbAGCameraObserver::InitializeL() - create observers" );
        OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_INITIALIZEL, "CThumbAGCameraObserver::InitializeL - create observers" );
        
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
        
        TN_DEBUG1( "CThumbAGCameraObserver::InitializeL() - connect to MDS" );
        OstTrace0( TRACE_FATAL, DUP2_CTHUMBAGCAMERAOBSERVER_INITIALIZEL, "CThumbAGCameraObserver::InitializeL - connect to MDS" );
        
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
      
        TN_DEBUG1( "CThumbAGCameraObserver::InitializeL() - end" );
        OstTrace0( TRACE_FATAL, DUP3_CTHUMBAGCAMERAOBSERVER_INITIALIZEL, "CThumbAGCameraObserver::InitializeL - end" );
    }

// ---------------------------------------------------------------------------
// ~CThumbAGCameraObserver
// ---------------------------------------------------------------------------
//
CThumbAGCameraObserver::~CThumbAGCameraObserver()
    {
    TN_DEBUG1( "CThumbAGCameraObserver::~CThumbAGCameraObserver() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_CTHUMBAGCAMERAOBSERVER, "CThumbAGCameraObserver::~CThumbAGCameraObserver - begin" );
    
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
        // 2 observers
        TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
        TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
                
        delete iMdESession;
        iMdESession = NULL;
        }
    
    TN_DEBUG1( "CThumbAGCameraObserver::~CThumbAGCameraObserver() - end" );
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_CTHUMBAGCAMERAOBSERVER, "CThumbAGCameraObserver::~CThumbAGCameraObserver - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGCameraObserver::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbAGCameraObserver::HandleSessionOpened( CMdESession& /* aSession */, TInt aError )
    {
    TN_DEBUG1( "CThumbAGCameraObserver::HandleSessionOpened");
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_HANDLESESSIONOPENED, "CThumbAGCameraObserver::HandleSessionOpened" );
    
    if (aError == KErrNone)
        {
        TRAPD( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGCameraObserver::HandleSessionOpened, AddObserversL error == %d", err );
            OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_HANDLESESSIONOPENED, "CThumbAGCameraObserver::HandleSessionOpened - AddObserversL error;err=%d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGCameraObserver::HandleSessionOpened error == %d", aError );
        OstTrace1( TRACE_FATAL, DUP2_CTHUMBAGCAMERAOBSERVER_HANDLESESSIONOPENED, "CThumbAGCameraObserver::HandleSessionOpened;aError=%d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGCameraObserver::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGCameraObserver::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbAGCameraObserver::HandleSessionError == %d", aError );
    OstTrace1( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_HANDLESESSIONERROR, "CThumbAGCameraObserver::HandleSessionError;aError=%d", aError );
    if (aError != KErrNone && !iSessionError)
        {
        iSessionError = ETrue;
    
        if (!iShutdown)
            {
            if (!iReconnect->IsActive())
                {
                iReconnect->Start( KMdEReconnect, KMdEReconnect, 
                                   TCallBack(ReconnectCallBack, this));
                
                TN_DEBUG1( "CThumbAGCameraObserver::HandleSessionError() - reconnect timer started" );
                OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_HANDLESESSIONERROR, "CThumbAGCameraObserver::HandleSessionError - reconnect timer started" );
                }
            }

        }   
    }

// -----------------------------------------------------------------------------
// CThumbAGCameraObserver::HandleObjectNotification
// -----------------------------------------------------------------------------
//
void CThumbAGCameraObserver::HandleObjectNotification( CMdESession& /*aSession*/, 
                                               TObserverNotificationType aType,
                                               const RArray<TItemId>& aObjectIdArray )
    {
    TN_DEBUG1( "CThumbAGCameraObserver::HandleObjectNotification() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGCameraObserver::HandleObjectNotification - begin" );

    // no processor or shutting down
    if ( iShutdown || !iProcessor)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyAdd)
        {
        TN_DEBUG2( "CThumbAGCameraObserver::HandleObjectNotification() - ENotifyAdd %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGCameraObserver::HandleObjectNotification - ENotifyAdd;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
        iAddCounter = aObjectIdArray.Count();
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG2( "CThumbAGCameraObserver::HandleObjectNotification() - ENotifyModify %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_FATAL, DUP2_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGCameraObserver::HandleObjectNotification - ENotifyModify;aObjectIdArray.Count=%d", aObjectIdArray.Count() );
        iModCounter = aObjectIdArray.Count();
        }
#endif
    
    if ( (aType == ENotifyAdd || aType == ENotifyModify ) && (aObjectIdArray.Count() > 0) )
        {
        TN_DEBUG1( "CThumbAGCameraObserver::HandleObjectNotification() - AddToQueueL" );
        OstTrace0( TRACE_FATAL, DUP3_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGCameraObserver::HandleObjectNotification - AddToQueueL" );

        // Add event to processing queue by type and enable force run        
        RPointerArray<HBufC> dummyArray;
        TRAPD(err, iProcessor->AddToQueueL(aType, EGenerationItemTypeCamera, aObjectIdArray, dummyArray, EFalse));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGCameraObserver::HandleObjectNotification() - error adding to queue" );
            OstTrace0( TRACE_FATAL, DUP4_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGCameraObserver::HandleObjectNotification - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGCameraObserver::HandleObjectNotification() - bad notification" );
        OstTrace0( TRACE_FATAL, DUP5_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGCameraObserver::HandleObjectNotification - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG3( "CThumbAGCameraObserver::IN-COUNTERS---------- Add = %d Modify = %d", iAddCounter, iModCounter );
    OstTraceExt2( TRACE_FATAL, DUP6_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGCameraObserver::HandleObjectNotification;iAddCounter=%u;iModCounter=%u", iAddCounter, iModCounter );
    iModCounter = 0;
    iAddCounter = 0;
#endif

    TN_DEBUG1( "CThumbAGCameraObserver::HandleObjectNotification() - end" );
    OstTrace0( TRACE_FATAL, DUP7_CTHUMBAGCAMERAOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGCameraObserver::HandleObjectNotification - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGCameraObserver::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGCameraObserver::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGCameraObserver::ShutdownNotification()" );
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_SHUTDOWNNOTIFICATION, "CThumbAGCameraObserver::ShutdownNotification" );
    
    if (!iShutdown)
        {
        TN_DEBUG1( "CThumbAGCameraObserver::ShutdownNotification() shutdown" );
        OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_SHUTDOWNNOTIFICATION, "CThumbAGCameraObserver::ShutdownNotification - shutdown" );
        iShutdown = ETrue;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGCameraObserver::AddObserversL
// ---------------------------------------------------------------------------
//
void CThumbAGCameraObserver::AddObserversL()
    {
    TN_DEBUG1( "CThumbAGCameraObserver::AddObserversL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_ADDOBSERVERSL, "CThumbAGCameraObserver::AddObserversL - begin" );
    
    CMdENamespaceDef& defaultNamespace = iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& objectDef = defaultNamespace.GetObjectDefL( MdeConstants::Object::KBaseObject );
    CMdEPropertyDef& originPropDef = objectDef.GetPropertyDefL( MdeConstants::Object::KOriginProperty );
    
    // set observing conditions
    CMdELogicCondition* addCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
    addCondition->AddPropertyConditionL( originPropDef, TMdEUintEqual(MdeConstants::Object::ECamera));
    
    CMdELogicCondition* modifyCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
    modifyCondition->AddPropertyConditionL( originPropDef, TMdEUintEqual(MdeConstants::Object::ECamera));
    
    // add observer
    iMdESession->AddObjectObserverL( *this, addCondition, ENotifyAdd ); 

   // modify observer
   iMdESession->AddObjectObserverL( *this, modifyCondition, ENotifyModify );
   
    CleanupStack::Pop( 2, addCondition );  
     
    TN_DEBUG1( "CThumbAGCameraObserver::AddObserversL() - end" );
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_ADDOBSERVERSL, "CThumbAGCameraObserver::AddObserversL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGCameraObserver::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGCameraObserver::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGCameraObserver::ReconnectCallBack() - reinitialize");
    OstTrace0( TRACE_FATAL, CTHUMBAGCAMERAOBSERVER_RECONNECTCALLBACK, "CThumbAGCameraObserver::ReconnectCallBack - reinitialize" );
    
    CThumbAGCameraObserver* self = static_cast<CThumbAGCameraObserver*>( aAny );
    
    self->iReconnect->Cancel();
    
    // reconnect to MDS
    TRAP_IGNORE( self->InitializeL() );
    
    TN_DEBUG1( "CThumbAGCameraObserver::ReconnectCallBack() - done");
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGCAMERAOBSERVER_RECONNECTCALLBACK, "CThumbAGCameraObserver::ReconnectCallBack - done" );
    
    return KErrNone;
    }


// End of file
