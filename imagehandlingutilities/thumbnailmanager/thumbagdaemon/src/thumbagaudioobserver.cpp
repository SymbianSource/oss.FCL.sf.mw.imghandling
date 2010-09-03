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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbagaudioobserverTraces.h"
#endif


// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CThumbAGAudioObserver* CThumbAGAudioObserver::NewLC(CThumbAGProcessor* aProcessor)
    {
    TN_DEBUG1( "CThumbAGAudioObserver::NewLC() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_NEWLC, "CThumbAGAudioObserver::NewLC" );
    
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
	OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_CONSTRUCTL, "CThumbAGAudioObserver::ConstructL - begin" );
	
#ifdef _DEBUG
    iAddCounter = 0;
    iModCounter = 0;
#endif
    
    InitializeL();
    	
	TN_DEBUG1( "CThumbAGAudioObserver::ConstructL() - end" );
	OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_CONSTRUCTL, "CThumbAGAudioObserver::ConstructL - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGAudioObserver
// ---------------------------------------------------------------------------
//
void CThumbAGAudioObserver::InitializeL()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::InitializeL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_INITIALIZEL, "CThumbAGAudioObserver::InitializeL - begin" );
   
        TN_DEBUG1( "CThumbAGAudioObserver::InitializeL() - create observers" );
        OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_INITIALIZEL, "CThumbAGAudioObserver::InitializeL - create observers" );
        
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
        OstTrace0( TRACE_FATAL, DUP2_CTHUMBAGAUDIOOBSERVER_INITIALIZEL, "CThumbAGAudioObserver::InitializeL - connect to MDS" );
        
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
        OstTrace0( TRACE_FATAL, DUP3_CTHUMBAGAUDIOOBSERVER_INITIALIZEL, "CThumbAGAudioObserver::InitializeL - end" );
    }

// ---------------------------------------------------------------------------
// ~CThumbAGAudioObserver
// ---------------------------------------------------------------------------
//
CThumbAGAudioObserver::~CThumbAGAudioObserver()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::~CThumbAGAudioObserver() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_CTHUMBAGAUDIOOBSERVER, "CThumbAGAudioObserver::~CThumbAGAudioObserver - begin" );
    
    iShutdown = ETrue;    
    
    Shutdown();
    
    TN_DEBUG1( "CThumbAGAudioObserver::~CThumbAGAudioObserver() - end" );
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_CTHUMBAGAUDIOOBSERVER, "CThumbAGAudioObserver::~CThumbAGAudioObserver - end" );
    }

void CThumbAGAudioObserver::Shutdown()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::Shutdown()" );
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_SHUTDOWN, "CThumbAGAudioObserver::Shutdown" );
    
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
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_HANDLESESSIONOPENED, "CThumbAGAudioObserver::HandleSessionOpened" );
    
    if (aError == KErrNone)
        {
        TRAPD( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGAudioObserver::HandleSessionOpened, AddObserversL error == %d", err );
            OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_HANDLESESSIONOPENED, "CThumbAGAudioObserver::HandleSessionOpened;err=%d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGAudioObserver::HandleSessionOpened error == %d", aError );
        OstTrace1( TRACE_FATAL, DUP2_CTHUMBAGAUDIOOBSERVER_HANDLESESSIONOPENED, "CThumbAGAudioObserver::HandleSessionOpened;aError=%d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGAudioObserver::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGAudioObserver::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbAGAudioObserver::HandleSessionError == %d", aError );
    OstTrace1( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_HANDLESESSIONERROR, "CThumbAGAudioObserver::HandleSessionError;aError=%d", aError );
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
                OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_HANDLESESSIONERROR, "CThumbAGAudioObserver::HandleSessionError - reconnect timer started" );
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
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGAudioObserver::HandleObjectNotification - begin" );

    // no processor or shutting down
    if ( iShutdown || !iProcessor)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyAdd)
        {
        TN_DEBUG2( "CThumbAGAudioObserver::HandleObjectNotification() - ENotifyAdd %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGAudioObserver::HandleObjectNotification - ENotifyAdd;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
        iAddCounter = aObjectIdArray.Count();
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG2( "CThumbAGAudioObserver::HandleObjectNotification() - ENotifyModify %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_FATAL, DUP2_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGAudioObserver::HandleObjectNotification - ENotifyModify;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
        iModCounter = aObjectIdArray.Count();
        }
#endif
    
    if ( (aType == ENotifyAdd || aType == ENotifyModify ) && (aObjectIdArray.Count() > 0) )
        {
        TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - AddToQueueL" );
        OstTrace0( TRACE_FATAL, DUP3_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGAudioObserver::HandleObjectNotification - AddToQueueL" );

        // Add event to processing queue by type and enable force run        
        RPointerArray<HBufC> dummyArray;
        TRAPD(err, iProcessor->AddToQueueL(aType, EGenerationItemTypeAudio, aObjectIdArray, dummyArray, EFalse));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - error adding to queue" );
            OstTrace0( TRACE_FATAL, DUP4_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGAudioObserver::HandleObjectNotification - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - bad notification" );
        OstTrace0( TRACE_FATAL, DUP5_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGAudioObserver::HandleObjectNotification - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG3( "CThumbAGAudioObserver::IN-COUNTERS---------- Add = %d Modify = %d", iAddCounter, iModCounter );
    OstTraceExt2( TRACE_FATAL, DUP6_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGAudioObserver::HandleObjectNotification;iAddCounter=%u;iModCounter=%u", iAddCounter, iModCounter );
    iModCounter = 0;
    iAddCounter = 0;
#endif

    TN_DEBUG1( "CThumbAGAudioObserver::HandleObjectNotification() - end" );
    OstTrace0( TRACE_FATAL, DUP7_CTHUMBAGAUDIOOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGAudioObserver::HandleObjectNotification - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGAudioObserver::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGAudioObserver::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGAudioObserver::ShutdownNotification()" );
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_SHUTDOWNNOTIFICATION, "CThumbAGAudioObserver::ShutdownNotification" );
    
    if (!iShutdown)
        {
        TN_DEBUG1( "CThumbAGAudioObserver::ShutdownNotification() shutdown" );
        OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_SHUTDOWNNOTIFICATION, "CThumbAGAudioObserver::ShutdownNotification - shutdown" );
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
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_ADDOBSERVERSL, "CThumbAGAudioObserver::AddObserversL - begin" );
    
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
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_ADDOBSERVERSL, "CThumbAGAudioObserver::AddObserversL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGAudioObserver::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGAudioObserver::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGAudioObserver::ReconnectCallBack() - reinitialize");
    OstTrace0( TRACE_FATAL, CTHUMBAGAUDIOOBSERVER_RECONNECTCALLBACK, "CThumbAGAudioObserver::ReconnectCallBack - reinitialize" );
    
    CThumbAGAudioObserver* self = static_cast<CThumbAGAudioObserver*>( aAny );
    
    self->iReconnect->Cancel();
    
    // reconnect to MDS
    TRAP_IGNORE( self->InitializeL() );
    
    TN_DEBUG1( "CThumbAGAudioObserver::ReconnectCallBack() - done");
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGAUDIOOBSERVER_RECONNECTCALLBACK, "CThumbAGAudioObserver::ReconnectCallBack - done" );
    
    return KErrNone;
    }


// End of file
