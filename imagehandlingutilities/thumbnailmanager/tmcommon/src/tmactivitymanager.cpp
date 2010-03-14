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
* Description:  Activity Manager 
*
*/

#include "tmactivitymanager.h"
#include "thumbnaillog.h"

// ---------------------------------------------------------------------------
// CTMActivityManager::NewL()
// ---------------------------------------------------------------------------
//
CTMActivityManager* CTMActivityManager::NewL(MTMActivityManagerObserver* aObserver, TInt aTimeout)
    {
    CTMActivityManager* self = new (ELeave) CTMActivityManager(aObserver, aTimeout);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }
 
// ---------------------------------------------------------------------------
// CTMActivityManager::CTMActivityManager()
// ---------------------------------------------------------------------------
//
CTMActivityManager::CTMActivityManager(MTMActivityManagerObserver* aObserver, TInt aTimeout)
: CActive(CActive::EPriorityHigh), iObserver(aObserver), iTimeout(aTimeout)
    {   
    CActiveScheduler::Add(this);
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::~CTMActivityManager()
// ---------------------------------------------------------------------------
//
CTMActivityManager::~CTMActivityManager()
    {
    Cancel();
    iTimer.Close();
    }
 
// ---------------------------------------------------------------------------
// CTMActivityManager::ConstructL()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::ConstructL()
    {
    iTimer.CreateLocal();
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::SetTimeout()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::SetTimeout(TInt aTimeout)
    {
    iTimeout = aTimeout;
    Reset();
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::Reset()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::Reset()
    {
    Cancel();
    Start();
    }
 
// ---------------------------------------------------------------------------
// CTMActivityManager::DoCancel()
// ---------------------------------------------------------------------------
void CTMActivityManager::DoCancel()
    {
    iTimer.Cancel();
    iWatch = ENone;
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::Start()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::Start()
    {
    if (!IsActive())
        {
        iWatch = EWaitingForInactivity;
        iTimer.Inactivity(iStatus, iTimeout);
        SetActive();
        }
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::RunL()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::RunL()
    {
    if (iStatus == KErrNone)
        {
        if (iWatch == EWaitingForInactivity)
            {
            TInt inactivity = User::InactivityTime().Int();
            if (inactivity >= iTimeout)
                {
                if (iObserver)
                    {
                    iObserver->InactivityDetected();
                    }
            if (!IsActive()) //observer might have called a Reset()
                {
                iTimer.Inactivity(iStatus,0);
                iWatch = EWaitingForActivity;
                }
            }
            else
                {
                iTimer.Inactivity(iStatus,iTimeout);
                }
            }
        else if (iWatch == EWaitingForActivity)
            {
            if (iObserver)
                {
                iObserver->ActivityDetected();
                }
            
            if (!IsActive()) //observer might have called a Reset()
                {
                iTimer.Inactivity(iStatus,iTimeout);
                iWatch = EWaitingForInactivity;
                }
        }
    
        if (!IsActive()) //observer might have called a Reset()
            {
            SetActive();
            }
        }
    else
        {
        iWatch = ENone;
        }
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::RunError()
// ---------------------------------------------------------------------------
//
TInt CTMActivityManager::RunError(TInt aError)
    {
    TN_DEBUG1( "CTMActivityManager::RunError()");
    
    if (aError != KErrNone)
        {
        TN_DEBUG2( "CTMActivityManager::RunError = %d", aError );
        Reset();
        }
    
    // nothing to do
    return KErrNone;
    }

