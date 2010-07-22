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


#ifndef THUMBAGIMAGEOBSERVER_H
#define THUMBAGIMAGEOBSERVER_H

#include <e32base.h>
#include <w32std.h>

#include <mdesession.h>

#include "thumbagprocessor.h"
#include "thumbnaillog.h"
#include "tmshutdownobserver.h"

/**
 *  ThumbAG daemon.
 *
 *  @since S60 v5.0
 */
NONSHARABLE_CLASS( CThumbAGImageObserver ): public CBase, 
                                     public MMdESessionObserver,
                                     public MMdEObjectObserver,
                                     public MTMShutdownObserver
    {
public:

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @return New CThumbAGImageObserver server.
     */
    static CThumbAGImageObserver* NewLC(CThumbAGProcessor* aProcessor);

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @return New CThumbAGImageObserver server.
     */
    static CThumbAGImageObserver* NewL(CThumbAGProcessor* aProcessor);
    
    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbAGImageObserver();


public:
    
    // from MMdESessionObserver
    void HandleSessionOpened( CMdESession& aSession, TInt aError );
    void HandleSessionError( CMdESession& aSession, TInt aError );
    
    // from MMdEObjectObserver
    void HandleObjectNotification(CMdESession& aSession, 
                                  TObserverNotificationType aType,
                                  const RArray<TItemId>& aObjectIdArray);
    
    // from MTMShutdownObserver
    void ShutdownNotification();
    
protected:
    
    /**
     * AddObserversL
     *
     * @since S60 v5.0
     */
    void AddObserversL();    
    
    
private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New CThumbAGImageObserver instance.
     */
    CThumbAGImageObserver(CThumbAGProcessor* aProcessor);

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();
    
	 /**
     * Initilization helper
     *
     * @since S60 v5.2
     */
    void InitializeL();
    
    /**
     * Callback for reconnect timer
     *
     * @since S60 v5.0
     */
    static TInt ReconnectCallBack(TAny* aAny);

private:
	
    // own
    CTMShutdownObserver* iShutdownObserver;
    CTMShutdownObserver* iMDSShutdownObserver;
    CMdESession* iMdESession;
    //not owned
    CThumbAGProcessor* iProcessor;
    
    TBool iShutdown;
 
    // reconnect timer
    CPeriodic* iReconnect;
    
    TBool iSessionError;
    
#ifdef _DEBUG
    TUint32 iAddCounter;
    TUint32 iModCounter;
#endif
};

#endif // THUMBAGDAEMON_H
