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
    // define P&S property types
    if (iDefineKey)
        {
        RProperty::Define(iKeyCategory,iPropertyKey,
                          RProperty::EInt,KAllowAllPolicy,KPowerMgmtPolicy);
        }
    
    // attach to the property
    TInt err = iProperty.Attach(iKeyCategory,iPropertyKey,EOwnerThread);
    User::LeaveIfError(err);
    
    // wait for the previously attached property to be updated
    iProperty.Subscribe(iStatus);
    SetActive();
    }

// ---------------------------------------------------------------------------
// CTMShutdownObserver::~CTMShutdownObserver()
// ---------------------------------------------------------------------------
//
CTMShutdownObserver::~CTMShutdownObserver()
    {
    Cancel();
    iProperty.Close();
    }

// ---------------------------------------------------------------------------
// CTMShutdownObserver::RunL()
// ---------------------------------------------------------------------------
//
void CTMShutdownObserver::RunL()
    {
    // resubscribe before processing new value to prevent missing updates
    iProperty.Subscribe(iStatus);
    SetActive();
    
    // retrieve the value
    TInt value = 0;
    iProperty.Get(value);

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
    iProperty.Cancel();
    }

// End of file
