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

#ifndef TMACTIVITYMANAGER_H
#define TMACTIVITYMANAGER_H


class MTMActivityManagerObserver

{
public :
    virtual void ActivityDetected() = 0;
    virtual void InactivityDetected() = 0;
};


class CTMActivityManager : public CActive

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


protected: // from CActive
    void DoCancel();
    void RunL();
    TInt RunError(TInt aError);

protected:
    CTMActivityManager(MTMActivityManagerObserver* aObserver, TInt aTimeout);
    void ConstructL();


protected:
    enum TWatch { ENone = 0, EWaitingForInactivity, EWaitingForActivity };

protected:
    RTimer iTimer;
    TWatch iWatch;
    MTMActivityManagerObserver* iObserver; ///The observer of activity status
    TInt iTimeout; ///Current inactivity period

};

#endif // TMACTIVITYMANAGER_H
