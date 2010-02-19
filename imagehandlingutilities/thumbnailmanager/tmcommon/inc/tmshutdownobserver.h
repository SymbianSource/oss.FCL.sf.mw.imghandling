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
* Description:  TM Shutdown Observer
 *
*/


#ifndef TMSHUTDOWNOBSERVER_H
#define TMSHUTDOWNOBSERVER_H

#include <e32base.h>
#include <w32std.h>
#include <e32property.h> 


/**
 * Observer interface for signaling the need for shutdown
 */
class MTMShutdownObserver
    {
public:

    virtual void ShutdownNotification() = 0;
    };

/**
 *  Active object for observing P&S keys
 *
 *  @since S60 v5.0
 */
class CTMShutdownObserver: public CActive
    {
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return Instance of CTMShutdownObserver.
     */
    static CTMShutdownObserver* NewL( MTMShutdownObserver& aObserver, const TUid& aKeyCategory,
                                      const TInt aPropertyKey, TBool aDefineKey);

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CTMShutdownObserver();
    
protected:

    /**
     * Handles an active object's request completion event.
     *
     * @since S60 v5.0
     */
    void RunL();

    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    void DoCancel();

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return Instance of CTMShutdownObserver.
     */
    CTMShutdownObserver( MTMShutdownObserver& aObserver, const TUid& aKeyCategory,
                         const TInt iPropertyKey, TBool aDefineKey);

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:
    
    // not own
    MTMShutdownObserver& iObserver;
    
    const TUid& iKeyCategory;
    RProperty iProperty;
    TInt iPropertyKey;
    
    TBool iDefineKey;
};

#endif // TMSHUTDOWNOBSERVER_H
