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


// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CThumbAGDaemon* CThumbAGDaemon::NewLC()
    {
    TN_DEBUG1( "CThumbAGDaemon::NewLC() - begin" );
    
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
    
	CThumbAGDaemon* self = CThumbAGDaemon::NewLC();
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// CThumbAGDaemon
// ---------------------------------------------------------------------------
//
CThumbAGDaemon::CThumbAGDaemon()
 	: CServer2( CActive::EPriorityStandard, CServer2::EUnsharableSessions )
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
	
	StartL( KTAGDaemonName );
	
#ifdef _DEBUG
    iAddCounter = 0;
    iModCounter = 0;
    iDelCounter = 0;
#endif
	
    if (DaemonEnabledL())
        {
        TN_DEBUG1( "CThumbAGDaemon::ConstructL() - create observers" );
        
    	// create shutdown observer
        iMDSShutdownObserver = CTMShutdownObserver::NewL( *this, KMdSPSShutdown, KMdSShutdown, EFalse );
    	iShutdownObserver = CTMShutdownObserver::NewL( *this, KTAGDPSNotification, KShutdown, ETrue );  
    	iShutdown = EFalse;
    	
        // create processor
        iProcessor = NULL;
        iProcessor = CThumbAGProcessor::NewL();	
    	
        TN_DEBUG1( "CThumbAGDaemon::ConstructL() - connect to MDS" );
        
    	// connect to MDS
    	iMdESession = NULL;
    	iMdESession = CMdESession::NewL( *this );
        }
    else
        {
        // no error here, but need to shutdown daemon neatly
        User::Leave(KErrNone);
        }
	
	TN_DEBUG1( "CThumbAGDaemon::ConstructL() - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGDaemon
// ---------------------------------------------------------------------------
//
CThumbAGDaemon::~CThumbAGDaemon()
    {
    TN_DEBUG1( "CThumbAGDaemon::~CThumbAGDaemon() - begin" );
    
    iShutdown = ETrue;    
    
    delete iMDSShutdownObserver;
    delete iShutdownObserver;
    
    if (iProcessor)
        {
        delete iProcessor;
        iProcessor = NULL;
        }
    
    if (iMdESession)
        {
        // 3 observers
        TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
        TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
        TRAP_IGNORE( iMdESession->RemoveObjectObserverL( *this ) );
        TRAP_IGNORE(iMdESession->RemoveObjectPresentObserverL( * this  ));
        
        delete iMdESession;
        iMdESession = NULL;
        }
    
    TN_DEBUG1( "CThumbAGDaemon::~CThumbAGDaemon() - end" );
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
        
        CleanupStack::PopAndDestroy( scheduler );
        }
    
    TN_DEBUG1( "CThumbAGDaemon::ThreadFunctionL() - end" );
	}

// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::HandleSessionOpened( CMdESession& /* aSession */, TInt aError )
    {
    TN_DEBUG1( "CThumbAGDaemon::HandleSessionOpened");
    
    if (aError == KErrNone)
        {
        iProcessor->SetMdESession(iMdESession);
        TRAP_IGNORE(iProcessor->QueryForPlaceholdersL());
        
        TRAPD( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGDaemon::HandleSessionOpened, AddObserversL error == %d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGDaemon::HandleSessionOpened error == %d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    if (aError != KErrNone)
        {
        TN_DEBUG2( "CThumbAGDaemon::HandleSessionError == %d", aError );
        }   
    }

// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleObjectNotification
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::HandleObjectNotification( CMdESession& /*aSession*/, 
                                               TObserverNotificationType aType,
                                               const RArray<TItemId>& aObjectIdArray )
    {
    TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - begin" );

    // no processor or shutting down
    if (!iProcessor || iShutdown)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyAdd)
        {
        TN_DEBUG2( "CThumbAGDaemon::HandleObjectNotification() - ENotifyAdd %d", aObjectIdArray.Count() );
        iAddCounter = iAddCounter + aObjectIdArray.Count();
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG2( "CThumbAGDaemon::HandleObjectNotification() - ENotifyModify %d", aObjectIdArray.Count() );
        iModCounter = iModCounter + aObjectIdArray.Count();
        }
    else if (aType == ENotifyRemove)
        {
        TN_DEBUG2( "CThumbAGDaemon::HandleObjectNotification() - ENotifyRemove %d", aObjectIdArray.Count() );
        iDelCounter = iDelCounter + aObjectIdArray.Count();
        }
#endif
    
    if ( (aType == ENotifyAdd || aType == ENotifyModify || aType == ENotifyRemove) &&
         (aObjectIdArray.Count() > 0) )
        {
        TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - AddToQueueL" );
		
        // If delete event, remove IDs from Modify and Add queues
        if ( aType == ENotifyRemove )
            {
            iProcessor->RemoveFromQueues( aObjectIdArray );
            }
        
        // Add event to processing queue by type and enable force run
        TRAPD(err, iProcessor->AddToQueueL(aType, aObjectIdArray, EFalse));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG6( "CThumbAGDaemon::IN-COUNTERS---------- Type: %d Amount: %d, Add = %d Modify = %d Delete = %d", 
               aType, aObjectIdArray.Count(), iAddCounter, iModCounter, iDelCounter );
#endif

    TN_DEBUG1( "CThumbAGDaemon::HandleObjectNotification() - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGDaemon::HandleObjectPresentNotification
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::HandleObjectPresentNotification(CMdESession& /*aSession*/, 
               TBool aPresent, const RArray<TItemId>& aObjectIdArray)
    {
    TN_DEBUG3( "CThumbAGDaemon::HandleObjectPresentNotification() - aPresent == %d count == %d", aPresent, aObjectIdArray.Count() );
    
    // no processor or shutting down
    if (!iProcessor || iShutdown)
        {
        return;
        }
    
    TInt err = 0;
    
    //tread present objects as added
    if(aPresent)
        {
        TRAP_IGNORE( iProcessor->QueryForPlaceholdersL());
        if ( aObjectIdArray.Count() > 0) 
           {
		   // do not force run of these items
           TRAP(err, iProcessor->AddToQueueL(ENotifyModify, aObjectIdArray, ETrue));
           
           TN_DEBUG2( "CThumbAGDaemon::HandleObjectPresentNotification() - ENotifyAdd %d", aObjectIdArray.Count() );
           
           
#ifdef _DEBUG
           iAddCounter = iAddCounter + aObjectIdArray.Count();
           if (err != KErrNone)
               {
               TN_DEBUG1( "CThumbAGDaemon::HandleObjectPresentNotification() - error adding to queue" );
               }
#endif
           }
        }
    else
        {
        TN_DEBUG1( "CThumbAGDaemon::HandleObjectPresentNotification() - handle not present" );
        TRAP_IGNORE( iProcessor->QueryForPlaceholdersL() );
#ifdef _DEBUG    
        if( iModCounter < aObjectIdArray.Count() )
            {
            iModCounter = 0;
            }
        else
            {
            iModCounter = iModCounter - aObjectIdArray.Count();
            }
#endif
           
        if ( aObjectIdArray.Count() > 0) 
           {
           iProcessor->RemoveFromQueues( aObjectIdArray, ETrue );
           }
        }
    
    #ifdef _DEBUG
    TN_DEBUG5( "CThumbAGDaemon::IN-COUNTERS---------- Amount: %d, Add = %d Modify = %d Delete = %d", 
               aObjectIdArray.Count(), iAddCounter, iModCounter, iDelCounter );
    #endif
    
    TN_DEBUG1( "CThumbAGDaemon::HandleObjectPresentNotification() - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGDaemon::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGDaemon::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGDaemon::ShutdownNotification()" );
    
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
    
    CMdENamespaceDef& defaultNamespace = iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& imageDef = defaultNamespace.GetObjectDefL( MdeConstants::Image::KImageObject );
    CMdEObjectDef& videoDef = defaultNamespace.GetObjectDefL( MdeConstants::Video::KVideoObject );
    CMdEObjectDef& audioDef = defaultNamespace.GetObjectDefL( MdeConstants::Audio::KAudioObject );
    
    // set observing conditions
    CMdELogicCondition* addCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorOr );
    addCondition->AddObjectConditionL( imageDef );
    addCondition->AddObjectConditionL( videoDef );
    addCondition->AddObjectConditionL( audioDef );
    CleanupStack::Pop( addCondition );  
    
    CMdELogicCondition* modifyCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorOr );
    modifyCondition->AddObjectConditionL( imageDef );
    modifyCondition->AddObjectConditionL( videoDef );
    modifyCondition->AddObjectConditionL( audioDef );
    CleanupStack::Pop( modifyCondition );
    
    // add observer
    iMdESession->AddObjectObserverL( *this, addCondition, ENotifyAdd ); 
    
    // modify observer
    iMdESession->AddObjectObserverL( *this, modifyCondition, ENotifyModify );
 
    // remove observer
    iMdESession->AddObjectObserverL( *this, NULL, ENotifyRemove );
    
    // object present observer
    iMdESession->AddObjectPresentObserverL( *this );
    
    TN_DEBUG1( "CThumbAGDaemon::AddObserversL() - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGDaemon::DaemonEnabledL
// ---------------------------------------------------------------------------
//
TBool CThumbAGDaemon::DaemonEnabledL()
    {
    TN_DEBUG1( "CThumbAGDaemon::DaemonEnabledL() - begin" );
    CRepository* rep = CRepository::NewL( TUid::Uid( THUMBNAIL_CENREP_UID ));
    
    // get value
    TBool val( EFalse );
    TInt ret = rep->Get( KEnableDaemon, val );
    
    delete rep;
    TN_DEBUG3( "CThumbAGDaemon::DaemonEnabledL() - val == %d, ret == %d", val, ret );
    return val;
    }

// ---------------------------------------------------------------------------
// E32Main
// ---------------------------------------------------------------------------
//
TInt E32Main()
    {    
    TN_DEBUG1( "CThumbAGDaemon::E32Main() - begin" );

    __UHEAP_MARK;

    CTrapCleanup* cleanup = CTrapCleanup::New();
    TInt result = KErrNoMemory;
    
    if ( cleanup )
        {
        TRAP( result, CThumbAGDaemon::ThreadFunctionL());
        delete cleanup;
        }
    
    if ( result != KErrNone )
        {
        TN_DEBUG1( "CThumbAGDaemon::E32Main() - error" );
        
        // Signal the client that server creation failed
        RProcess::Rendezvous( result );
        }    
    
    __UHEAP_MARKEND;

    TN_DEBUG1( "CThumbAGDaemon::E32Main() - end" );
    
    return result;
    }

// End of file
