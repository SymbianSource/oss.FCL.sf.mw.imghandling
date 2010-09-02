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

#include "tmshutdownobserver.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tmshutdownobserverTraces.h"
#endif


// ---------------------------------------------------------------------------
// CTMShutdownObserver::NewL()
// ---------------------------------------------------------------------------
//
CTMShutdownObserver* CTMShutdownObserver::NewL( MTMShutdownObserver& aObserver,
                                                const TUid& aKeyCategory,
                                                const TInt aPropertyKey,
                                                TBool aDefineKey)
    { 
    CTMShutdownObserver* self = new( ELeave )CTMShutdownObserver( aObserver, 
                                                                  aKeyCategory,
                                                                  aPropertyKey,
                                                                  aDefineKey);
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CTMShutdownObserver::CTMShutdownObserver()
// ---------------------------------------------------------------------------
//
CTMShutdownObserver::CTMShutdownObserver( MTMShutdownObserver& aObserver,
                                          const TUid& aKeyCategory,
                                          const TInt aPropertyKey,
                                          TBool aDefineKey)
    : CActive( CActive::EPriorityStandard ), iObserver( aObserver ),
      iKeyCategory( aKeyCategory ), iPropertyKey(aPropertyKey), iDefineKey( aDefineKey )
    {   
    CActiveScheduler::Add( this );
    }

// ---------------------------------------------------------------------------
// CTMShutdownObserver::ConstructL()
// ---------------------------------------------------------------------------
//
void CTMShutdownObserver::ConstructL()
    { 
    TN_DEBUG1( "CTMShutdownObserver::ConstructL()" );
    OstTrace0( TRACE_NORMAL, CTMSHUTDOWNOBSERVER_CONSTRUCTL, "CTMShutdownObserver::ConstructL" );
    // define P&S property types
    if (iDefineKey)
        {
        TN_DEBUG1( "CTMShutdownObserver::ConstructL() define" );
        OstTrace0( TRACE_NORMAL, DUP1_CTMSHUTDOWNOBSERVER_CONSTRUCTL, "CTMShutdownObserver::ConstructL - define" );
        RProperty::Define(iKeyCategory,iPropertyKey,
                          RProperty::EInt,KAllowAllPolicy,KPowerMgmtPolicy);
        }
    
    // attach to the property
    TInt err = iProperty.Attach(iKeyCategory,iPropertyKey,EOwnerThread);
    TN_DEBUG2( "CTMShutdownObserver::ConstructL() attach err = %d", err );
    OstTrace1( TRACE_NORMAL, DUP2_CTMSHUTDOWNOBSERVER_CONSTRUCTL, "CTMShutdownObserver::ConstructL - attach;err=%d", err );
    User::LeaveIfError(err);
    
    // wait for the previously attached property to be updated
    iProperty.Subscribe(iStatus);
    TN_DEBUG1( "CTMShutdownObserver::ConstructL() subscribe" );
    OstTrace0( TRACE_NORMAL, DUP3_CTMSHUTDOWNOBSERVER_CONSTRUCTL, "CTMShutdownObserver::ConstructL - subscribe" );
    SetActive();
    }

// ---------------------------------------------------------------------------
// CTMShutdownObserver::~CTMShutdownObserver()
// ---------------------------------------------------------------------------
//
CTMShutdownObserver::~CTMShutdownObserver()
    {
    TN_DEBUG1( "CTMShutdownObserver::~CTMShutdownObserver()" );
    OstTrace0( TRACE_NORMAL, CTMSHUTDOWNOBSERVER_CTMSHUTDOWNOBSERVER, "CTMShutdownObserver::~CTMShutdownObserver" );
    Cancel();
    iProperty.Close();
    }

// ---------------------------------------------------------------------------
// CTMShutdownObserver::RunL()
// ---------------------------------------------------------------------------
//
void CTMShutdownObserver::RunL()
    {
    TN_DEBUG2( "CTMShutdownObserver::RunL(%d)", iStatus.Int() );
    OstTrace1( TRACE_NORMAL, CTMSHUTDOWNOBSERVER_RUNL, "CTMShutdownObserver::RunL;iStatus.Int()=%d", iStatus.Int() );
    // resubscribe before processing new value to prevent missing updates
    iProperty.Subscribe(iStatus);
    SetActive();
    
    // retrieve the value
    TInt value = 0;
    TInt err = iProperty.Get(value);
    
    TN_DEBUG2( "CTMShutdownObserver::RunL() Get err = %d", err );
    OstTrace1( TRACE_NORMAL, DUP1_CTMSHUTDOWNOBSERVER_RUNL, "CTMShutdownObserver::RunL - get err;err=%d", err );

    // observer callback
    if (value)
        {
        iObserver.ShutdownNotification();
        }
    }

// ---------------------------------------------------------------------------
// CTMShutdownObserver::DoCancel()
// ---------------------------------------------------------------------------
//
void CTMShutdownObserver::DoCancel()
    {
    TN_DEBUG1( "CTMShutdownObserver::DoCancel()" );
    OstTrace0( TRACE_NORMAL, CTMSHUTDOWNOBSERVER_DOCANCEL, "CTMShutdownObserver::DoCancel" );
    iProperty.Cancel();
    }

// End of file
