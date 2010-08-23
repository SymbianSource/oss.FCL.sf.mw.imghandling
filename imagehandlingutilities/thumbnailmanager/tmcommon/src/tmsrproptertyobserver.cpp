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
* Description:  RPropery observer 
*
*/


#include <e32base.h>

#include "tmrpropertyobserver.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tmsrproptertyobserverTraces.h"
#endif


// ---------------------------------------------------------------------------
// CTMRPropertyObserver::NewL()
// ---------------------------------------------------------------------------
//
CTMRPropertyObserver* CTMRPropertyObserver::NewL( MTMRPropertyObserver& aObserver,
                                                const TUid& aKeyCategory,
                                                const TInt aPropertyKey,
                                                TBool aDefineKey)
    { 
    CTMRPropertyObserver* self = new( ELeave )CTMRPropertyObserver( aObserver, 
                                                                  aKeyCategory,
                                                                  aPropertyKey,
                                                                  aDefineKey);
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CTMRPropertyObserver::CTMRPropertyObserver()
// ---------------------------------------------------------------------------
//
CTMRPropertyObserver::CTMRPropertyObserver( MTMRPropertyObserver& aObserver,
                                          const TUid& aKeyCategory,
                                          const TInt aPropertyKey,
                                          TBool aDefineKey)
    : CActive( CActive::EPriorityStandard ), iObserver( aObserver ),
      iKeyCategory( aKeyCategory ), iPropertyKey(aPropertyKey), iDefineKey( aDefineKey )
    {   
    CActiveScheduler::Add( this );
    }

// ---------------------------------------------------------------------------
// CTMRPropertyObserver::ConstructL()
// ---------------------------------------------------------------------------
//
void CTMRPropertyObserver::ConstructL()
    { 
    TN_DEBUG1( "CTMRPropertyObserver::ConstructL()");
    OstTrace0( TRACE_NORMAL, CTMRPROPERTYOBSERVER_CONSTRUCTL, "CTMRPropertyObserver::ConstructL" );
    // define P&S property types
    if (iDefineKey)
        {
        RProperty::Define(iKeyCategory,iPropertyKey,
                          RProperty::EInt);
        }
    
    // attach to the property
    TInt err = iProperty.Attach(iKeyCategory,iPropertyKey,EOwnerThread);
    User::LeaveIfError(err);
    
    SetActive();
    TRequestStatus* statusPtr = &iStatus;
    User::RequestComplete( statusPtr, KErrNone );
    }

// ---------------------------------------------------------------------------
// CTMRPropertyObserver::~CTMRPropertyObserver()
// ---------------------------------------------------------------------------
//
CTMRPropertyObserver::~CTMRPropertyObserver()
    {
    TN_DEBUG1( "CTMRPropertyObserver::~CTMRPropertyObserver()");
    OstTrace0( TRACE_NORMAL, CTMRPROPERTYOBSERVER_CTMRPROPERTYOBSERVER, "CTMRPropertyObserver::~CTMRPropertyObserver" );
    Cancel();
    iProperty.Close();
    }

// ---------------------------------------------------------------------------
// CTMRPropertyObserver::RunL()
// ---------------------------------------------------------------------------
//
void CTMRPropertyObserver::RunL()
    {
    TN_DEBUG1( "CTMRPropertyObserver::RunL()");
    OstTrace0( TRACE_NORMAL, CTMRPROPERTYOBSERVER_RUNL, "CTMRPropertyObserver::RunL" );
    // resubscribe before processing new value to prevent missing updates
    iProperty.Subscribe(iStatus);
    SetActive();
    
    // retrieve the value
    TInt value = 0;
    TInt ret = iProperty.Get(value);

    // observer callback
    iObserver.RPropertyNotification(ret, iKeyCategory, iPropertyKey, value);
    }

// ---------------------------------------------------------------------------
// CTMRPropertyObserver::DoCancel()
// ---------------------------------------------------------------------------
//
void CTMRPropertyObserver::DoCancel()
    {
    TN_DEBUG1( "CTMRPropertyObserver::DoCancel()");
    OstTrace0( TRACE_NORMAL, CTMRPROPERTYOBSERVER_DOCANCEL, "CTMRPropertyObserver::DoCancel" );
    iProperty.Cancel();
    }

// End of file
