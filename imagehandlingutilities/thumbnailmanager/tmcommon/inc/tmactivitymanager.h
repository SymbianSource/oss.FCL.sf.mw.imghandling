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

#include <e32base.h>
#include <hwrmlight.h>

#ifndef TMACTIVITYMANAGER_H
#define TMACTIVITYMANAGER_H


class MTMActivityManagerObserver

{
public :
    virtual void ActivityChanged(const TBool aActive) = 0;
};


class CTMActivityManager : public CActive,
                           public MHWRMLightObserver

{
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return Instance of CThumbAGProcessor.
     */
    static CTMActivityManager* NewL(MTMActivityManagerObserver* aObserver, TInt aTimeout = 60);

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CTMActivityManager();
    void SetTimeout(TInt aTimeout);
    void Start();
    void Reset();
    void Stop();
    TBool IsInactive();


protected: // from CActive
    void DoCancel();
    void RunL();
    TInt RunError(TInt aError);

protected:
    CTMActivityManager(MTMActivityManagerObserver* aObserver, TInt aTimeout);
    void ConstructL();
    void NotifyObserver();
    
private: //From MHWRMLightObserver
    void LightStatusChanged(TInt aTarget, CHWRMLight::TLightStatus aStatus);
    
protected:
    enum TWatch { ENone = 0, EWaitingForInactivity, EWaitingForActivity };

protected:
    RTimer iTimer;
    TWatch iWatch;
    MTMActivityManagerObserver* iObserver; ///The observer of activity status
    TInt iTimeout; ///Current inactivity period
    
    //Backlight control 
    CHWRMLight* iLight;
    //backlight status
    TBool iLights;

    //previous status
    TInt iPreviousStatus;
    TBool iFirstRound;
};

#endif // TMACTIVITYMANAGER_H
