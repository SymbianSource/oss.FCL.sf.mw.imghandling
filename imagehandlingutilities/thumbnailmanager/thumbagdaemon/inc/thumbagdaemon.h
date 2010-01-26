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


#ifndef THUMBAGDAEMON_H
#define THUMBAGDAEMON_H

#include <e32base.h>
#include <w32std.h>

#include <mdesession.h>

#include "thumbagprocessor.h"
#include "tmshutdownobserver.h"
#include "thumbnaillog.h"


/**
 *  ThumbAG daemon.
 *
 *  @since S60 v5.0
 */
NONSHARABLE_CLASS( CThumbAGDaemon ): public CServer2, 
                                     public MMdESessionObserver,
                                     public MMdEObjectObserver,
                                     public MTMShutdownObserver,
                                     public MMdEObjectPresentObserver
                                     
    {
public:

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @return New CThumbAGDaemon server.
     */
    static CThumbAGDaemon* NewLC();

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @return New CThumbAGDaemon server.
     */
    static CThumbAGDaemon* NewL();
    
    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbAGDaemon();

public:

    /**
     * Creates new server session.
     *
     * @since S60 v5.0
     * @param aVersion Version info.
     * @param aMessage Message to be passed.
     * @return New session.
     */
    CSession2* NewSessionL( const TVersion& aVersion,
                            const RMessage2& aMessage ) const;    
    
    /**
     * ThreadFunctionL
     *
     * @since S60 v5.0
     */
    static void ThreadFunctionL();    

public:
    
    // from MMdESessionObserver
    void HandleSessionOpened( CMdESession& aSession, TInt aError );
    void HandleSessionError( CMdESession& aSession, TInt aError );
    
    // from MMdEObjectObserver
    void HandleObjectNotification(CMdESession& aSession, 
                                  TObserverNotificationType aType,
                                  const RArray<TItemId>& aObjectIdArray);
    
    void HandleObjectPresentNotification(CMdESession& aSession, 
                TBool aPresent, const RArray<TItemId>& aObjectIdArray);
    
    // from MTMShutdownObserver
    void ShutdownNotification();
    
protected:
    
    /**
     * AddObserversL
     *
     * @since S60 v5.0
     */
    void AddObserversL();    
    
    /**
     * Check if daemon needs to run
     *
     * @since S60 v5.0
     */
    TBool DaemonEnabledL();
    
private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New CThumbAGDaemon instance.
     */
    CThumbAGDaemon();

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:
	
    // own
    CTMShutdownObserver* iShutdownObserver;
    CTMShutdownObserver* iMDSShutdownObserver;
    CMdESession* iMdESession;
    CThumbAGProcessor* iProcessor;
    
    TBool iShutdown;
 
#ifdef _DEBUG
    TUint32 iAddCounter;
    TUint32 iModCounter;
    TUint32 iDelCounter;
#endif
};

#endif // THUMBAGDAEMON_H
