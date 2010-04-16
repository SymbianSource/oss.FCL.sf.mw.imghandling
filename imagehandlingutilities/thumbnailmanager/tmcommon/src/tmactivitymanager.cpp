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
    TN_DEBUG1( "CTMActivityManager::NewL()");
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
: CActive(CActive::EPriorityHigh), iObserver(aObserver), iTimeout(aTimeout), iPreviousStatus(KErrNotFound)
    {
    TN_DEBUG1( "CTMActivityManager::CTMActivityManager()");
    CActiveScheduler::Add(this);
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::~CTMActivityManager()
// ---------------------------------------------------------------------------
//
CTMActivityManager::~CTMActivityManager()
    {
    TN_DEBUG1( "CTMActivityManager::~CTMActivityManager()");
#ifdef MONITOR_LIGHTS
    delete iLight;
    iLight = NULL;
#endif
    Cancel();
    iTimer.Close();
    }
 
// ---------------------------------------------------------------------------
// CTMActivityManager::ConstructL()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::ConstructL()
    {
    TN_DEBUG1( "CTMActivityManager::ConstructL()");
    iTimer.CreateLocal();
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::SetTimeout()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::SetTimeout(TInt aTimeout)
    {
    TN_DEBUG1( "CTMActivityManager::SetTimeout()");
    iTimeout = aTimeout;
    Reset();
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::Reset()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::Reset()
    {
    TN_DEBUG1( "CTMActivityManager::Reset()");
#ifdef MONITOR_LIGHTS
    delete iLight;
    iLight = NULL;
#endif
    Cancel();
    Start();
    }
 
// ---------------------------------------------------------------------------
// CTMActivityManager::DoCancel()
// ---------------------------------------------------------------------------
void CTMActivityManager::DoCancel()
    {
    TN_DEBUG1( "CTMActivityManager::DoCancel()");
    iTimer.Cancel();
    iWatch = ENone;
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::Start()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::Start()
    {
    TN_DEBUG1( "CTMActivityManager::Start()");
    
    iFirstRound = ETrue;
    
#ifdef MONITOR_LIGHTS
    if(!iLight)
        {
        TRAP_IGNORE(iLight = CHWRMLight::NewL(this));
        }
#endif
    
    if( !IsActive() )
        {
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::RunL()
// ---------------------------------------------------------------------------
//
void CTMActivityManager::RunL()
    {
    TN_DEBUG4( "CTMActivityManager::RunL(0x%08x) %d, observer = 0x%08x", this, iStatus.Int(), iObserver);
    
    if(iFirstRound)
        {
        TN_DEBUG1( "CTMActivityManager::RunL() iFirstRound");
        iFirstRound = EFalse;
        if (!IsActive())
             {
             iWatch = EWaitingForInactivity;
             iTimer.Inactivity(iStatus, iTimeout);
             SetActive();
             }
        NotifyObserver();
        return;
        }
    
    if (iStatus == KErrNone)
        {
        if (iWatch == EWaitingForInactivity)
            {
            TInt inactivity = User::InactivityTime().Int();
            if (inactivity >= iTimeout)
                {
                TN_DEBUG1( "CTMActivityManager::RunL() inactive");
                NotifyObserver();

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
                TN_DEBUG1( "CTMActivityManager::RunL() active");
                NotifyObserver();
             
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
        TN_DEBUG1( "CTMActivityManager::RunL() end");
    }

// ---------------------------------------------------------------------------
// CTMActivityManager::RunError()
// ---------------------------------------------------------------------------
//
TInt CTMActivityManager::RunError(TInt aError)
    {
    TN_DEBUG2( "CTMActivityManager::RunError() %d", aError);
    
    if (aError != KErrNone)
        {
        Reset();
        }
    
    // nothing to do
    return KErrNone;
    }

// -----------------------------------------------------------------------------
// IsInactive()
// -----------------------------------------------------------------------------
//
TBool CTMActivityManager::IsInactive()
    {
#ifdef MONITOR_LIGHTS
#ifdef _DEBUG
TN_DEBUG3( "CTMActivityManager::IsInactive()= %d, iLights = %d", User::InactivityTime().Int(), iLights);
#endif
#else
TN_DEBUG2( "CTMActivityManager::IsInactive()= %d", User::InactivityTime().Int());
#endif

    //if lights are off or inactivity timer is less that target the device is not idle
    if( User::InactivityTime() >= TTimeIntervalSeconds(iTimeout)
#ifdef MONITOR_LIGHTS
            || !iLights
#endif
            )
      {
      TN_DEBUG1( "CTMActivityManager::IsInactive() ETrue");
      return ETrue;
      }
    TN_DEBUG1( "CTMActivityManager::IsInactive() EFalse");
    return EFalse;
    }

#ifdef MONITOR_LIGHTS
// -----------------------------------------------------------------------------
// LightStatusChanged()
// -----------------------------------------------------------------------------
//
void CTMActivityManager::LightStatusChanged(TInt aTarget, CHWRMLight::TLightStatus aStatus)
    {
    TN_DEBUG3( "CTMActivityManager::LightStatusChanged() aTarget = %d, aStatus == %d", aTarget, aStatus);
    
    if(aTarget & CHWRMLight::EPrimaryDisplay)
        {
         if( aStatus == CHWRMLight::ELightOff )
            {
            TN_DEBUG1( "CTMActivityManager::LightStatusChanged() -- OFF");
            iLights = EFalse;
            }
        else
            {
            TN_DEBUG1( "CTMActivityManager::LightStatusChanged() -- ON");
            iLights = ETrue;
            }
     
        NotifyObserver();
        }
    }
#endif

// -----------------------------------------------------------------------------
// NotifyObserver()
// -----------------------------------------------------------------------------
//
void CTMActivityManager::NotifyObserver()
    {
    TN_DEBUG1( "void CTMAActivityManager::NotifyObserver()");
    TBool status = IsInactive();
    
    if( iPreviousStatus != status )
        {
        iPreviousStatus = status; 
        if (iObserver)
            {
            iObserver->ActivityChanged(!status);
            }
        }
    }
