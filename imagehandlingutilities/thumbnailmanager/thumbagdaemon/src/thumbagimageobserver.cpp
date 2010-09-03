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

#include "thumbagimageobserver.h"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbagimageobserverTraces.h"
#endif


// ---------------------------------------------------------------------------
// NewLC
// ---------------------------------------------------------------------------
//
CThumbAGImageObserver* CThumbAGImageObserver::NewLC(CThumbAGProcessor* aProcessor)
    {
    TN_DEBUG1( "CThumbAGImageObserver::NewLC() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_NEWLC, "CThumbAGImageObserver::NewLC - begin" );
    
	CThumbAGImageObserver* self = new (ELeave) CThumbAGImageObserver(aProcessor);
	CleanupStack::PushL( self );
	self->ConstructL();
	return self;
	}
	
// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CThumbAGImageObserver* CThumbAGImageObserver::NewL(CThumbAGProcessor* aProcessor)
	{
	TN_DEBUG1( "CThumbAGImageObserver::NewL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_NEWL, "CThumbAGImageObserver::NewL - begin" );
    
	CThumbAGImageObserver* self = CThumbAGImageObserver::NewLC(aProcessor);
	CleanupStack::Pop( self );
	return self;
	}

// ---------------------------------------------------------------------------
// CThumbAGImageObserver
// ---------------------------------------------------------------------------
//
CThumbAGImageObserver::CThumbAGImageObserver(CThumbAGProcessor* aProcessor)
 	: iShutdownObserver(NULL), iMdESession(NULL), iProcessor(aProcessor)
 	{
 	// No implementation required
 	}

// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CThumbAGImageObserver::ConstructL()
	{
	TN_DEBUG1( "CThumbAGImageObserver::ConstructL() - begin" );
	OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_CONSTRUCTL, "CThumbAGImageObserver::ConstructL - begin" );
	
#ifdef _DEBUG
    iAddCounter = 0;
    iModCounter = 0;
#endif
    
    InitializeL();
    	
	TN_DEBUG1( "CThumbAGImageObserver::ConstructL() - end" );
	OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_CONSTRUCTL, "CThumbAGImageObserver::ConstructL - end" );
	}

// ---------------------------------------------------------------------------
// ~CThumbAGImageObserver
// ---------------------------------------------------------------------------
//
void CThumbAGImageObserver::InitializeL()
    {
    TN_DEBUG1( "CThumbAGImageObserver::InitializeL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_INITIALIZEL, "CThumbAGImageObserver::InitializeL - begin" );
   
        TN_DEBUG1( "CThumbAGImageObserver::InitializeL() - create observers" );
        OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_INITIALIZEL, "CThumbAGImageObserver::InitializeL - create observers" );
        
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
        
        TN_DEBUG1( "CThumbAGImageObserver::InitializeL() - connect to MDS" );
        OstTrace0( TRACE_FATAL, DUP2_CTHUMBAGIMAGEOBSERVER_INITIALIZEL, "CThumbAGImageObserver::InitializeL - connect to MDS" );
        
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
      
        TN_DEBUG1( "CThumbAGImageObserver::InitializeL() - end" );
        OstTrace0( TRACE_FATAL, DUP3_CTHUMBAGIMAGEOBSERVER_INITIALIZEL, "CThumbAGImageObserver::InitializeL - end" );
    }

// ---------------------------------------------------------------------------
// ~CThumbAGImageObserver
// ---------------------------------------------------------------------------
//
CThumbAGImageObserver::~CThumbAGImageObserver()
    {
    TN_DEBUG1( "CThumbAGImageObserver::~CThumbAGImageObserver() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_CTHUMBAGIMAGEOBSERVER, "CThumbAGImageObserver::~CThumbAGImageObserver - begin" );
    
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
    
    TN_DEBUG1( "CThumbAGImageObserver::~CThumbAGImageObserver() - end" );
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_CTHUMBAGIMAGEOBSERVER, "CThumbAGImageObserver::~CThumbAGImageObserver - end" );
    }

// -----------------------------------------------------------------------------
// CThumbAGImageObserver::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbAGImageObserver::HandleSessionOpened( CMdESession& /* aSession */, TInt aError )
    {
    TN_DEBUG1( "CThumbAGImageObserver::HandleSessionOpened");
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_HANDLESESSIONOPENED, "CThumbAGImageObserver::HandleSessionOpened" );
    
    if (aError == KErrNone)
        {
        TRAPD( err, AddObserversL() );
        if (err != KErrNone)
            {
            TN_DEBUG2( "CThumbAGImageObserver::HandleSessionOpened, AddObserversL error == %d", err );
            OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_HANDLESESSIONOPENED, "CThumbAGImageObserver::HandleSessionOpened, AddObserversL error;err=%d", err );
            }
        }
    else
        {
        TN_DEBUG2( "CThumbAGImageObserver::HandleSessionOpened error == %d", aError );
        OstTrace1( TRACE_FATAL, DUP2_CTHUMBAGIMAGEOBSERVER_HANDLESESSIONOPENED, "CThumbAGImageObserver::HandleSessionOpened;aError=%d", aError );
        }
    }

// -----------------------------------------------------------------------------
// CThumbAGImageObserver::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbAGImageObserver::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbAGImageObserver::HandleSessionError == %d", aError );
    OstTrace1( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_HANDLESESSIONERROR, "CThumbAGImageObserver::HandleSessionError;aError=%d", aError );
    if (aError != KErrNone && !iSessionError)
        {
        iSessionError = ETrue;
    
        if (!iShutdown)
            {
            if (!iReconnect->IsActive())
                {
                iReconnect->Start( KMdEReconnect, KMdEReconnect, 
                                   TCallBack(ReconnectCallBack, this));
                
                TN_DEBUG1( "CThumbAGImageObserver::HandleSessionError() - reconnect timer started" );
                OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_HANDLESESSIONERROR, "CThumbAGImageObserver::HandleSessionError - reconnect timer started" );
                }
            }

        }   
    }

// -----------------------------------------------------------------------------
// CThumbAGImageObserver::HandleObjectNotification
// -----------------------------------------------------------------------------
//
void CThumbAGImageObserver::HandleObjectNotification( CMdESession& /*aSession*/, 
                                               TObserverNotificationType aType,
                                               const RArray<TItemId>& aObjectIdArray )
    {
    TN_DEBUG1( "CThumbAGImageObserver::HandleObjectNotification() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGImageObserver::HandleObjectNotification - begin" );

    // no processor or shutting down
    if (iShutdown || !iProcessor)
        {
        return;
        }
    
#ifdef _DEBUG
    if (aType == ENotifyAdd)
        {
        TN_DEBUG2( "CThumbAGImageObserver::HandleObjectNotification() - ENotifyAdd %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGImageObserver::HandleObjectNotification - ENotifyAdd;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
        iAddCounter = aObjectIdArray.Count();
        }
    else if (aType == ENotifyModify)
        {
        TN_DEBUG2( "CThumbAGImageObserver::HandleObjectNotification() - ENotifyModify %d", aObjectIdArray.Count() );
        OstTrace1( TRACE_FATAL, DUP2_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGImageObserver::HandleObjectNotification - ENotifyModify;aObjectIdArray.Count()=%d", aObjectIdArray.Count() );
        iModCounter = aObjectIdArray.Count();
        }
#endif
    
    if ( (aType == ENotifyAdd || aType == ENotifyModify) && aObjectIdArray.Count() > 0 )
        {
        TN_DEBUG1( "CThumbAGImageObserver::HandleObjectNotification() - AddToQueueL" );
		OstTrace0( TRACE_FATAL, DUP3_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGImageObserver::HandleObjectNotification - AddToQueueL" );
		
        // Add event to processing queue by type and enable force run        
        RPointerArray<HBufC> dummyArray;
        TRAPD(err, iProcessor->AddToQueueL(aType, EGenerationItemTypeImage, aObjectIdArray, dummyArray, EFalse));
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbAGImageObserver::HandleObjectNotification() - error adding to queue" );
            OstTrace0( TRACE_FATAL, DUP4_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGImageObserver::HandleObjectNotification - error adding to queue" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbAGImageObserver::HandleObjectNotification() - bad notification" );
        OstTrace0( TRACE_FATAL, DUP5_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGImageObserver::HandleObjectNotification - bad notification" );
        }
    
#ifdef _DEBUG
    TN_DEBUG3( "CThumbAGImageObserver::IN-COUNTERS---------- Add = %d Modify = %d", iAddCounter, iModCounter );
    OstTraceExt2( TRACE_FATAL, DUP6_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGImageObserver::HIN-COUNTERS---------- Add, Modify;iAddCounter=%u;iModCounter=%u", iAddCounter, iModCounter );
    iModCounter = 0;
    iAddCounter = 0;
    
#endif

    TN_DEBUG1( "CThumbAGImageObserver::HandleObjectNotification() - end" );
    OstTrace0( TRACE_FATAL, DUP7_CTHUMBAGIMAGEOBSERVER_HANDLEOBJECTNOTIFICATION, "CThumbAGImageObserver::HandleObjectNotification - end" );
    }

// ---------------------------------------------------------------------------
// CThumbAGImageObserver::AddObserversL
// ---------------------------------------------------------------------------
//
void CThumbAGImageObserver::AddObserversL()
    {
    TN_DEBUG1( "CThumbAGImageObserver::AddObserversL() - begin" );
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_ADDOBSERVERSL, "CThumbAGImageObserver::AddObserversL - begin" );
    
    CMdENamespaceDef& defaultNamespace = iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& objectDef = defaultNamespace.GetObjectDefL( MdeConstants::Object::KBaseObject );
    CMdEPropertyDef& originPropDef = objectDef.GetPropertyDefL( MdeConstants::Object::KOriginProperty );
    CMdEObjectDef& imageDef = defaultNamespace.GetObjectDefL( MdeConstants::Image::KImageObject );
    
    // set observing conditions
    CMdELogicCondition* addCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
    addCondition->AddObjectConditionL( imageDef );
	
	addCondition->AddPropertyConditionL( originPropDef, TMdEUintNotEqual(MdeConstants::Object::ECamera));
    
    CMdELogicCondition* modifyCondition = CMdELogicCondition::NewLC( ELogicConditionOperatorAnd );
	modifyCondition->AddObjectConditionL( imageDef );
	
	modifyCondition->AddPropertyConditionL( originPropDef, TMdEUintNotEqual(MdeConstants::Object::ECamera));
	
    // add observers
    iMdESession->AddObjectObserverL( *this, addCondition, ENotifyAdd ); 

    // modify observer
    iMdESession->AddObjectObserverL( *this, modifyCondition, ENotifyModify );
	
    CleanupStack::Pop( 2, addCondition );

    TN_DEBUG1( "CThumbAGImageObserver::AddObserversL() - end" );
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_ADDOBSERVERSL, "CThumbAGImageObserver::AddObserversL - end" );
    }


// -----------------------------------------------------------------------------
// CThumbAGCameraObserver::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbAGImageObserver::ShutdownNotification()
    {
    TN_DEBUG1( "CThumbAGImageObserver::ShutdownNotification()" );
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_SHUTDOWNNOTIFICATION, "CThumbAGImageObserver::ShutdownNotification" );
    
    if (!iShutdown)
        {
        TN_DEBUG1( "CThumbAGImageObserver::ShutdownNotification() shutdown" );
        OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_SHUTDOWNNOTIFICATION, "CThumbAGImageObserver::ShutdownNotification - shutdown" );
        iShutdown = ETrue;
        }
    }

// ---------------------------------------------------------------------------
// CThumbAGImageObserver::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbAGImageObserver::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbAGImageObserver::ReconnectCallBack() - reinitialize");
    OstTrace0( TRACE_FATAL, CTHUMBAGIMAGEOBSERVER_RECONNECTCALLBACK, "CThumbAGImageObserver::ReconnectCallBack - reinitialize" );
    
    CThumbAGImageObserver* self = static_cast<CThumbAGImageObserver*>( aAny );
    
    self->iReconnect->Cancel();
    
    // reconnect to MDS
    TRAP_IGNORE( self->InitializeL() );
    
    TN_DEBUG1( "CThumbAGImageObserver::ReconnectCallBack() - done");
    OstTrace0( TRACE_FATAL, DUP1_CTHUMBAGIMAGEOBSERVER_RECONNECTCALLBACK, "CThumbAGImageObserver::ReconnectCallBack - done" );
    
    return KErrNone;
    }


// End of file
