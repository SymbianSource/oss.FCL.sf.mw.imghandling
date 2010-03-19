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
* Description:  TM RProperty Observer
 *
*/


#ifndef TMRPROPERTYDOBSERVER_H
#define TMRPROPERTYDOBSERVER_H

#include <e32base.h>
#include <w32std.h>
#include <e32property.h> 


/**
 * Observer interface for signaling the RProperty change
 */
class MTMRPropertyObserver
    {
public:

    virtual void RPropertyNotification(const TInt aError, const TUid aKeyCategory, const TUint aPropertyKey, const TInt aValue) = 0;
    };

/**
 *  Active object for observing P&S keys
 *
 *  @since S60 v5.0
 */
class CTMRPropertyObserver: public CActive
    {
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return Instance of CTMRPropertyObserver.
     */
    static CTMRPropertyObserver* NewL( MTMRPropertyObserver& aObserver, const TUid& aKeyCategory,
                                      const TInt aPropertyKey, TBool aDefineKey);

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CTMRPropertyObserver();
    
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
     * @return Instance of CTMRPropertyObserver.
     */
    CTMRPropertyObserver( MTMRPropertyObserver& aObserver, const TUid& aKeyCategory,
                         const TInt iPropertyKey, TBool aDefineKey);

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:
    
    // not own
    MTMRPropertyObserver& iObserver;
    
    const TUid& iKeyCategory;
    RProperty iProperty;
    TInt iPropertyKey;
    
    TBool iDefineKey;
};

#endif // TMRPROPERTYDOBSERVER_H
