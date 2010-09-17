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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbagvideoobserverTraces.h"
#endif



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
    OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_NEWL, "CThumbAGVideoObserver::NewL" );
    
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
	OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_CONSTRUCTL, "CThumbAGVideoObserver::ConstructL - begin" );
	
#ifdef _DEBUG
    iAddCounter = 0;
    iModCounter = 0;
#endif
    
    InitializeL();
    	
	TN_DEBUG1( "CThumbAGVideoObserver::ConstructL() - end" );
	OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_CONSTRUCTL, "CThumbAGVideoObserver::ConstructL - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGVideoObserver
// ---------------------------------------------------------------------------
//
void CThumbAGVideoObserver::InitializeL()
    {
    TN_DEBUG1( "CThumbAGVideoObserver::InitializeL() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_INITIALIZEL, "CThumbAGVideoObserver::InitializeL - begin" );
    
   
        TN_DEBUG1( "CThumbAGVideoObserver::InitializeL() - create observers" );
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_INITIALIZEL, "CThumbAGVideoObserver::InitializeL - create observers" );
        
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
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBAGVIDEOOBSERVER_INITIALIZEL, "CThumbAGVideoObserver::InitializeL - connect to MDS" );
        
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
        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGVIDEOOBSERVER_INITIALIZEL, "CThumbAGVideoObserver::InitializeL - end" );
    }

// ---------------------------------------------------------------------------
// ~CThumbAGVideoObserver
// ---------------------------------------------------------------------------
//
CThumbAGVideoObserver::~CThumbAGVideoObserver()
    {
    TN_DEBUG1( "CThumbAGVideoObserver::~CThumbAGVideoObserver() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_CTHUMBAGVIDEOOBSERVER, "CThumbAGVideoObserver::~CThumbAGVideoObserver - begin" );
    
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
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_CTHUMBAGVIDEOOBSERVER, "CThumbAGVideoObserver::~CThumbAGVideoObserver - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGVideoObserver::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbAGVideoObserver::HandleSessionOpened( CMdESession& /* aSession */, TInt aError )
    {
    TN_DEBUG1( "CThumbAGVideoObserver::HandleSessionOpened");
    OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_HANDLESESSIONOPENED, "CThumbAGVideoObserver::HandleSessionOpened" );
    
    if (aError == KErrNone)
        {
        TRAPD( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGVideoObserver::HandleSessionOpened, AddObserversL error == %d", err );
            OstTrace1( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_HANDLESESSIONOPENED, "CThumbAGVideoObserver::HandleSessionOpened - AddObserversL;err=%d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGVideoObserver::HandleSessionOpened error == %d", aError );
        OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGVIDEOOBSERVER_HANDLESESSIONOPENED, "CThumbAGVideoObserver::HandleSessionOpened;aError=%d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGVideoObserver::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGVideoObserver::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbAGVideoObserver::HandleSessionError == %d", aError );
    OstTrace1( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_HANDLESESSIONERROR, "CThumbAGVideoObserver::HandleSessionError;aError=%d", aError );
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
                OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_HANDLESESSIONERROR, "CThumbAGVideoObserver::HandleSessionError - reconnect timer started" );
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
    OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGVideoObserver::HandleObjectNotification - begin" );

    // no processor or shutting down
    if ( iShutdown || !iProcessor)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyAdd)
        {
        TN_DEBUG2( "CThumbAGVideoObserver::HandleObjectNotification() - ENotifyAdd %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGVideoObserver::HandleObjectNotification - ENotifyAdd;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
        iAddCounter = aObjectIdArray.Count();
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG2( "CThumbAGVideoObserver::HandleObjectNotification() - ENotifyModify %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_NORMAL, DUP2_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGVideoObserver::HandleObjectNotification - ENotifyModify;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
        iModCounter = aObjectIdArray.Count();
        }
#endif
    
    if ( (aType == ENotifyAdd || aType == ENotifyModify ) && (aObjectIdArray.Count() > 0) )
        {
        TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - AddToQueueL" );
        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGVideoObserver::HandleObjectNotification - AddToQueueL" );

        // Add event to processing queue by type and enable force run        
        RPointerArray<HBufC> dummyArray;
        TRAPD(err, iProcessor->AddToQueueL(aType, EGenerationItemTypeVideo, aObjectIdArray, dummyArray, EFalse));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - error adding to queue" );
            OstTrace0( TRACE_NORMAL, DUP4_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGVideoObserver::HandleObjectNotification - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - bad notification" );
        OstTrace0( TRACE_NORMAL, DUP5_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGVideoObserver::HandleObjectNotification - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG3( "CThumbAGVideoObserver::IN-COUNTERS---------- Add = %d Modify = %d", iAddCounter, iModCounter );
    OstTraceExt2( TRACE_NORMAL, DUP6_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGVideoObserver::IN-COUNTERS---------- Add = %u Modify = %u", iAddCounter, iModCounter );
    iModCounter = 0;
    iAddCounter = 0;
#endif

    TN_DEBUG1( "CThumbAGVideoObserver::HandleObjectNotification() - end" );
    OstTrace0( TRACE_NORMAL, DUP7_CTHUMBAGVIDEOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGVideoObserver::HandleObjectNotification - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGVideoObserver::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGVideoObserver::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGVideoObserver::ShutdownNotification()" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_SHUTDOWNNOTIFICATION, "CThumbAGVideoObserver::ShutdownNotification" );
    
    if (!iShutdown)
        {
        TN_DEBUG1( "CThumbAGVideoObserver::ShutdownNotification() shutdown" );
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_SHUTDOWNNOTIFICATION, "CThumbAGVideoObserver::ShutdownNotification - shutdown" );
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
    OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_ADDOBSERVERSL, "CThumbAGVideoObserver::AddObserversL - begin" );
    
    CMdENamespaceDef& defaultNamespace = iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& objectDef = defaultNamespace.GetObjectDefL( MdeConstants::Object::KBaseObject );
    CMdEPropertyDef& originPropDef = objectDef.GetPropertyDefL( MdeConstants::Object::KOriginProperty );
    CMdEObjectDef& videoDef = defaultNamespace.GetObjectDefL( MdeConstants::Video::KVideoObject );
    
    // set observing conditions
    CMdELogicCondition* addCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
	addCondition->AddObjectConditionL( videoDef );
	
	addCondition->AddPropertyConditionL( originPropDef, TMdEUintNotEqual(MdeConstants::Object::ECamera));
    
    CMdELogicCondition* modifyCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
	modifyCondition->AddObjectConditionL( videoDef );

	modifyCondition->AddPropertyConditionL( originPropDef, TMdEUintNotEqual(MdeConstants::Object::ECamera));
    
    // add observer
    iMdESession->AddObjectObserverL( *this, addCondition, ENotifyAdd ); 

    // modify observer
    iMdESession->AddObjectObserverL( *this, modifyCondition, ENotifyModify );
	
	CleanupStack::Pop( 2, addCondition );
     
    TN_DEBUG1( "CThumbAGVideoObserver::AddObserversL() - end" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_ADDOBSERVERSL, "CThumbAGVideoObserver::AddObserversL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGVideoObserver::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGVideoObserver::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGVideoObserver::ReconnectCallBack() - reinitialize");
    OstTrace0( TRACE_NORMAL, CTHUMBAGVIDEOOBSERVER_RECONNECTCALLBACK, "CThumbAGVideoObserver::ReconnectCallBack - reinitialize" );
    
    CThumbAGVideoObserver* self = static_cast<CThumbAGVideoObserver*>( aAny );
    
    self->iReconnect->Cancel();
    
    // reconnect to MDS
    TRAP_IGNORE( self->InitializeL() );
    
    TN_DEBUG1( "CThumbAGVideoObserver::ReconnectCallBack() - done");
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBAGVIDEOOBSERVER_RECONNECTCALLBACK, "CThumbAGVideoObserver::ReconnectCallBack - done" );
    
    return KErrNone;
    }


// End of file
