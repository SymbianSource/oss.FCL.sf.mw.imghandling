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

#include "thumbagdaemon.h"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbagdaemonTraces.h"
#endif


// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CThumbAGDaemon* CThumbAGDaemon::NewLC()
    {
    TN_DEBUG1( "CThumbAGDaemon::NewLC() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_NEWLC, "CThumbAGDaemon::NewLC - begin" );
    
	CThumbAGDaemon* self = new (ELeave) CThumbAGDaemon();
	CleanupStack::PushL( self );
	self->ConstructL();
	return self;
	}
	
// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CThumbAGDaemon* CThumbAGDaemon::NewL()
	{
	TN_DEBUG1( "CThumbAGDaemon::NewL() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBAGDAEMON_NEWL, "CThumbAGDaemon::NewL" );
    
	CThumbAGDaemon* self = CThumbAGDaemon::NewLC();
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// CThumbAGDaemon
// ---------------------------------------------------------------------------
//
CThumbAGDaemon::CThumbAGDaemon()
 	: CServer2( CActive::EPriorityStandard, CServer2::EUnsharableSessions ), iShutdownObserver(NULL), 
 	       iMDSShutdownObserver(NULL), iMdESession(NULL), iProcessor(NULL)
 	{
 	// No implementation required
 	}

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CThumbAGDaemon::ConstructL()
	{
	TN_DEBUG1( "CThumbAGDaemon::ConstructL() - begin" );
	OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_CONSTRUCTL, "CThumbAGDaemon::ConstructL - begin" );
	
	StartL( KTAGDaemonName );
	
#ifdef _DEBUG
    iAddCounter = 0;
    iDelCounter = 0;
#endif
    
    InitializeL();
    	
	TN_DEBUG1( "CThumbAGDaemon::ConstructL() - end" );
	OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_CONSTRUCTL, "CThumbAGDaemon::ConstructL - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGDaemon
// ---------------------------------------------------------------------------
//
void CThumbAGDaemon::InitializeL()
    {
    TN_DEBUG1( "CThumbAGDaemon::InitializeL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_INITIALIZEL, "CThumbAGDaemon::InitializeL - begin" );
    
    if (DaemonEnabledL())
        {
        TN_DEBUG1( "CThumbAGDaemon::InitializeL() - create observers" );
        OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_INITIALIZEL, "CThumbAGDaemon::InitializeL - create observers" );
        
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
        
        // create processor
        if(iProcessor)
            {
            iProcessor->Shutdown();
            delete iProcessor;
            iProcessor = NULL; 
            }
        
        iProcessor = CThumbAGProcessor::NewL(); 
        
        // MDS session reconnect timer
        if (!iReconnect)
            {
            iReconnect = CPeriodic::NewL(CActive::EPriorityIdle);
            }
        
        TN_DEBUG1( "CThumbAGDaemon::InitializeL() - connect to MDS" );
        
        if(iMdESession)
            {
            TRAP_IGNORE( iMdESession->RemoveObjectObserverWithUriL( *this ) );
            TRAP_IGNORE( iMdESession->RemoveObjectPresentObserverL( * this  ));
        
            // connect to MDS
            delete iMdESession;
            iMdESession = NULL;
            }

        iMdESession = CMdESession::NewL( *this );
        iSessionError = EFalse;
        
        delete iImageObserver;
        iImageObserver = NULL;
        iImageObserver = CThumbAGImageObserver::NewL(iProcessor);
       
        delete iCameraObserver;
        iCameraObserver = NULL;
        iCameraObserver = CThumbAGCameraObserver::NewL(iProcessor);
        
        delete iAudioObserver;
        iAudioObserver = NULL;
        iAudioObserver = CThumbAGAudioObserver::NewL(iProcessor);
        
        delete iVideoObserver;
        iVideoObserver = NULL;
        iVideoObserver = CThumbAGVideoObserver::NewL(iProcessor);
        }
    else
        {
        // no error here, but need to shutdown daemon neatly
        User::Leave(KErrNone);
        }
        TN_DEBUG1( "CThumbAGDaemon::InitializeL() - end" );
        OstTrace0( TRACE_FATAL, DUP2_CTHUMBAGDAEMON_INITIALIZEL, "CThumbAGDaemon::InitializeL - end" );
    }

// ---------------------------------------------------------------------------
// ~CThumbAGDaemon
// ---------------------------------------------------------------------------
//
CThumbAGDaemon::~CThumbAGDaemon()
    {
    TN_DEBUG1( "CThumbAGDaemon::~CThumbAGDaemon() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_CTHUMBAGDAEMON, "CThumbAGDaemon::~CThumbAGDaemon - begin" );
    
    iShutdown = ETrue;
    
    delete iImageObserver;
    iImageObserver = NULL;
    
    delete iCameraObserver;
    iCameraObserver = NULL;
    
    delete iVideoObserver;
    iVideoObserver = NULL;
    
    delete iAudioObserver;
    iAudioObserver = NULL;
    
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
    
    if (iProcessor)
        {
        iProcessor->Shutdown();
        delete iProcessor;
        iProcessor = NULL;
        }
    
    if (iMdESession)
        {
#ifdef MDS_URI_OBSERVER
        // remove observer with uri
        TRAP_IGNORE( iMdESession->RemoveObjectObserverWithUriL( *this ) );
#endif
        
        //present observer
        TRAP_IGNORE( iMdESession->RemoveObjectPresentObserverL( *this ) );
        
        delete iMdESession;
        iMdESession = NULL;
        }
    
    TN_DEBUG1( "CThumbAGDaemon::~CThumbAGDaemon() - end" );
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_CTHUMBAGDAEMON, "CThumbAGDaemon::~CThumbAGDaemon - end" );
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::NewSessionL()
// -----------------------------------------------------------------------------
//
CSession2* CThumbAGDaemon::NewSessionL( const TVersion& /*aVersion*/,
                                        const RMessage2& /*aMessage*/ )const
    {
    // no services, no clients, no sessions
    User::Leave(KErrNotSupported);
    
    //just for getting rid of compiler warning about missing return value
    return NULL;
    }

// ---------------------------------------------------------------------------
// CThumbAGDaemon::ThreadFunctionL
// ---------------------------------------------------------------------------
//
void CThumbAGDaemon::ThreadFunctionL()
    {
	TN_DEBUG1( "CThumbAGDaemon::ThreadFunctionL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_THREADFUNCTIONL, "CThumbAGDaemon::ThreadFunctionL - begin" );
    
    User::LeaveIfError( User::RenameThread( KTAGDaemonName ) );

    CThumbAGDaemon* server = NULL;
    CActiveScheduler* scheduler = new( ELeave )CActiveScheduler();

    if ( scheduler )
        {
        CActiveScheduler::Install( scheduler );
        
        CleanupStack::PushL( scheduler );
        server = CThumbAGDaemon::NewL(); // Adds server in scheduler

        RProcess::Rendezvous( KErrNone );

        CActiveScheduler::Start();
       
        // comes here if server gets shut down
        delete server;
        server = NULL;
        
        CleanupStack::PopAndDestroy( scheduler );
        }
    
    TN_DEBUG1( "CThumbAGDaemon::ThreadFunctionL() - end" );
	OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_THREADFUNCTIONL, "CThumbAGDaemon::ThreadFunctionL - end" );
	}

// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::HandleSessionOpened( CMdESession& /* aSession */, TInt aError )
    {
    TN_DEBUG1( "CThumbAGDaemon::HandleSessionOpened");
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_HANDLESESSIONOPENED, "CThumbAGDaemon::HandleSessionOpened" );
    
    if (aError == KErrNone)
        {
        TRAPD(err, iProcessor->SetMdESessionL(iMdESession));
        
        TRAP( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGDaemon::HandleSessionOpened, AddObserversL error == %d", err );
            OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_HANDLESESSIONOPENED, "CThumbAGDaemon::HandleSessionOpened;err=%d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGDaemon::HandleSessionOpened error == %d", aError );
        OstTrace1( TRACE_FATAL, DUP2_CTHUMBAGDAEMON_HANDLESESSIONOPENED, "CThumbAGDaemon::HandleSessionOpened;aError=%d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbAGDaemon::HandleSessionError == %d", aError );
    OstTrace1( TRACE_FATAL, CTHUMBAGDAEMON_HANDLESESSIONERROR, "CThumbAGDaemon::HandleSessionError;aError=%d", aError );
    if (aError != KErrNone && !iShutdown && !iSessionError)
        {
        iSessionError = ETrue;
    
	    delete iImageObserver;
	    iImageObserver = NULL;
    
	    delete iCameraObserver;
	    iCameraObserver = NULL;
    
	    delete iVideoObserver;
	    iVideoObserver = NULL;
    
	    delete iAudioObserver;
	    iAudioObserver = NULL;
		
        // kill processor right away, because it also has MdESession
        if(iProcessor)
            {
			iProcessor->Shutdown();
            delete iProcessor;
            iProcessor = NULL; 
            }
    
        if (!iReconnect->IsActive())
            {
            iReconnect->Start( KMdEReconnect, KMdEReconnect, 
                               TCallBack(ReconnectCallBack, this));
            
            TN_DEBUG1( "CThumbAGDaemon::HandleSessionError() - reconnect timer started" );
            OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_HANDLESESSIONERROR, "CThumbAGDaemon::HandleSessionError - reconnect timer started" );
            }

        }   
    }

#ifdef MDS_URI_OBSERVER
// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleObjectNotification
// -----------------------------------------------------------------------------
//

void CThumbAGDaemon::HandleUriObjectNotification(CMdESession& /*aSession*/, 
        TObserverNotificationType aType,
        const RArray<TItemId>& aObjectIdArray,
        const RPointerArray<HBufC>& aObjectUriArray)
    {
    TN_DEBUG1( "CThumbAGDaemon::HandleUriObjectNotification() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_HANDLEURIOBJECTNOTIFICATION, "CThumbAGDaemon::HandleUriObjectNotification - begin" );
    
    if(!iProcessor || iShutdown)
        {
        return;
        }
    
    if(aType == ENotifyRemove)
        {
        TN_DEBUG1( "CThumbAGDaemon::HandleUriObjectNotification() - removed");
        OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_HANDLEURIOBJECTNOTIFICATION, "CThumbAGDaemon::HandleUriObjectNotification - removed" );
        TInt err(0);
        TRAP(err, iProcessor->AddToQueueL(aType, EGenerationItemTypeAny, aObjectIdArray, aObjectUriArray, EFalse));
        __ASSERT_DEBUG((err==KErrNone), User::Panic(_L("CThumbAGDaemon::HandleUriObjectNotification()"), err));

        err = KErrNone;
        }
    TN_DEBUG1( "CThumbAGDaemon::HandleUriObjectNotification() - end" );
    OstTrace0( TRACE_FATAL, DUP2_CTHUMBAGDAEMON_HANDLEURIOBJECTNOTIFICATION, "CThumbAGDaemon::HandleUriObjectNotification - end" );
    }
#endif

// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleObjectNotification
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::HandleObjectNotification( CMdESession& /*aSession*/, 
                                               TObserverNotificationType aType,
                                               const RArray<TItemId>& aObjectIdArray )
    {
    TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION, "CThumbAGDaemon::HandleObjectNotification - begin" );

    // no processor or shutting down
    if (!iProcessor || iShutdown)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyRemove)
        {
        TN_DEBUG2( "CThumbAGDaemon::HandleObjectNotification() - ENotifyRemove %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION, "CThumbAGDaemon::HandleObjectNotification - ENotifyRemove;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
        iDelCounter = aObjectIdArray.Count();
        }
#endif
    
    if ( aType == ENotifyRemove && aObjectIdArray.Count() > 0 )
        {
        TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - AddToQueueL" );
		OstTrace0( TRACE_FATAL, DUP2_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION, "CThumbAGDaemon::HandleObjectNotification - AddToQueueL" );
		
        // If delete event, remove IDs from Modify and Add queues
        iProcessor->RemoveFromQueues( aObjectIdArray, EFalse);
        
        // Add event to processing queue by type and enable force run        
        RPointerArray<HBufC> dummyArray;
        TRAPD(err, iProcessor->AddToQueueL(aType, EGenerationItemTypeAny, aObjectIdArray, dummyArray, EFalse));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - error adding to queue" );
            OstTrace0( TRACE_FATAL, DUP3_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION, "CThumbAGDaemon::HandleObjectNotification - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - bad notification" );
        OstTrace0( TRACE_FATAL, DUP4_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION, "CThumbAGDaemon::HandleObjectNotification - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG2( "CThumbAGDaemon::IN-COUNTERS---------- Delete = %d", iDelCounter );
    OstTrace1( TRACE_FATAL, DUP5_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION, "CThumbAGDaemon::IN-COUNTERS---------- Delete;iDelCounter=%u", iDelCounter );
    iDelCounter = 0;
#endif

    TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - end" );
    OstTrace0( TRACE_FATAL, DUP6_CTHUMBAGDAEMON_HANDLEOBJECTNOTIFICATION, "CThumbAGDaemon::HandleObjectNotification - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleObjectPresentNotification
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::HandleObjectPresentNotification(CMdESession& /*aSession*/, 
               TBool aPresent, const RArray<TItemId>& aObjectIdArray)
    {
    TN_DEBUG3( "CThumbAGDaemon::HandleObjectPresentNotification() - aPresent == %d count == %d", aPresent, aObjectIdArray.Count() );
    OstTraceExt2( TRACE_FATAL, CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION, "CThumbAGDaemon::HandleObjectPresentNotification;aPresent=%d;aObjectIdArray.Count()=%d", aPresent, aObjectIdArray.Count() );
    
    // no processor or shutting down
    if (!iProcessor || iShutdown)
        {
        return;
        }
    
    TInt err = 0;
    
    //tread present objects as added
    if(aPresent)
        {
        if ( aObjectIdArray.Count() > 0) 
            {
		    // do not force run of these items
            RPointerArray<HBufC> dummyArray;
            TRAP(err, iProcessor->AddToQueueL(ENotifyAdd, EGenerationItemTypeUnknown, aObjectIdArray, dummyArray, ETrue));
           
            TN_DEBUG2( "CThumbAGDaemon::HandleObjectPresentNotification() - ENotifyAdd unknown items %d", aObjectIdArray.Count() );     
            OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION, "CThumbAGDaemon::HandleObjectPresentNotification - ENotifyAdd unknown items;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
           #ifdef _DEBUG
           iAddCounter = aObjectIdArray.Count();
           if (err != KErrNone)
               {
               TN_DEBUG1( "CThumbAGDaemon::HandleObjectPresentNotification() - error adding to queue" );
               OstTrace0( TRACE_FATAL, DUP2_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION, "CThumbAGDaemon::HandleObjectPresentNotification - error adding to queue" );
               }
           #endif
           }
        }
    else
        {
        TN_DEBUG1( "CThumbAGDaemon::HandleObjectPresentNotification() - handle not present" );
        OstTrace0( TRACE_FATAL, DUP3_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION, "CThumbAGDaemon::HandleObjectPresentNotification - handle not present" );

        #ifdef _DEBUG    
        if( iAddCounter < aObjectIdArray.Count() )
            {
            iAddCounter = 0;
            }
        else
            {
            iDelCounter = aObjectIdArray.Count();
            }
        #endif
           
        if ( aObjectIdArray.Count() > 0) 
            {
            iProcessor->RemoveFromQueues( aObjectIdArray, ETrue );
            }
        }
    
    #ifdef _DEBUG
    TN_DEBUG3( "CThumbAGDaemon::IN-COUNTERS---------- Add = %d Delete = %d", iAddCounter, iDelCounter );
    OstTraceExt2( TRACE_FATAL, DUP4_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION, "CThumbAGDaemon::::IN-COUNTERS---------- Add, Delete;iAddCounter=%u;iDelCounter=%u", iAddCounter, iDelCounter );
    iDelCounter = 0;
	iAddCounter = 0;
    #endif
    
    TN_DEBUG1( "CThumbAGDaemon::HandleObjectPresentNotification() - end" );
    OstTrace0( TRACE_FATAL, DUP5_CTHUMBAGDAEMON_HANDLEOBJECTPRESENTNOTIFICATION, "CThumbAGDaemon::HandleObjectPresentNotification - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGDaemon::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGDaemon::ShutdownNotification()" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_SHUTDOWNNOTIFICATION, "CThumbAGDaemon::ShutdownNotification" );
    
    if (!iShutdown)
        {
        CActiveScheduler::Stop();
        iShutdown = ETrue;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGDaemon::AddObserversL
// ---------------------------------------------------------------------------
//
void CThumbAGDaemon::AddObserversL()
    {
    TN_DEBUG1( "CThumbAGDaemon::AddObserversL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_ADDOBSERVERSL, "CThumbAGDaemon::AddObserversL - begin" );
 
#ifdef MDS_URI_OBSERVER
    // remove observer with uri
    iMdESession->AddObjectObserverWithUriL( *this, NULL, ENotifyRemove );
#endif
    
    // object present observer
    iMdESession->AddObjectPresentObserverL( *this );
    
    TN_DEBUG1( "CThumbAGDaemon::AddObserversL() - end" );
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_ADDOBSERVERSL, "CThumbAGDaemon::AddObserversL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGDaemon::DaemonEnabledL
// ---------------------------------------------------------------------------
//
TBool CThumbAGDaemon::DaemonEnabledL()
    {
    TN_DEBUG1( "CThumbAGDaemon::DaemonEnabledL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_DAEMONENABLEDL, "CThumbAGDaemon::DaemonEnabledL - begin" );
    CRepository* rep = CRepository::NewL( TUid::Uid( THUMBNAIL_CENREP_UID ));
    
    // get value
    TBool val( EFalse );
    TInt ret = rep->Get( KEnableDaemon, val );
    
    delete rep;
    rep = NULL;
    
    TN_DEBUG3( "CThumbAGDaemon::DaemonEnabledL() - val == %d, ret == %d", val, ret );
    OstTraceExt2( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_DAEMONENABLEDL, "CThumbAGDaemon::DaemonEnabledL;val=%u;ret=%d", val, ret );
    return val;
    }

// ---------------------------------------------------------------------------
// CThumbAGDaemon::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGDaemon::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGDaemon::ReconnectCallBack() - reinitialize");
    OstTrace0( TRACE_FATAL, CTHUMBAGDAEMON_RECONNECTCALLBACK, "CThumbAGDaemon::ReconnectCallBack - reinitialize" );
    
    CThumbAGDaemon* self = static_cast<CThumbAGDaemon*>( aAny );
    
    self->iReconnect->Cancel();
    
    // reconnect to MDS
    TRAP_IGNORE( self->InitializeL() );
    
    TN_DEBUG1( "CThumbAGDaemon::ReconnectCallBack() - done");
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGDAEMON_RECONNECTCALLBACK, "CThumbAGDaemon::ReconnectCallBack - done" );
    
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// E32Main
// ---------------------------------------------------------------------------
//
TInt E32Main()
    {    
    TN_DEBUG1( "CThumbAGDaemon::E32Main() - begin" );
    OstTrace0( TRACE_FATAL, _E32MAIN, "::E32Main - begin" );

    __UHEAP_MARK;

    CTrapCleanup* cleanup = CTrapCleanup::New();
    TInt result = KErrNoMemory;
    
    if ( cleanup )
        {
        TRAP( result, CThumbAGDaemon::ThreadFunctionL());
        delete cleanup;
        cleanup = NULL;
        }
    
    if ( result != KErrNone )
        {
        TN_DEBUG1( "CThumbAGDaemon::E32Main() - error" );
        OstTrace0( TRACE_FATAL, DUP1__E32MAIN, "::E32Main - error" );
        
        // Signal the client that server creation failed
        RProcess::Rendezvous( result );
        }    
    
    __UHEAP_MARKEND;

    TN_DEBUG1( "CThumbAGDaemon::E32Main() - end" );
    OstTrace0( TRACE_FATAL, DUP2__E32MAIN, "::E32Main - end" );
    
    return result;
    }

// End of file
