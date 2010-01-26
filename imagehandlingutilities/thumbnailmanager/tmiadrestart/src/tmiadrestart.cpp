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
* Description:  Thumbnail Manager IAD Restart 
*
*/


#include <e32base.h>
#include <e32std.h>  
#include <e32property.h>

#include "thumbnailmanagerconstants.h"

LOCAL_C void MainL()
    {
    // delay so that cenrep has time to read new config
    User::After(5000000);
    
    TInt res( KErrNone );
    RProcess process;
    TFullName name;
    
    // find and terminate Thumb AG Daemon
    TFindProcess findProcess( KTAGDaemonProcess );   
    if ( findProcess.Next(name) == KErrNone )
        {
        res = process.Open(name);
        
        // logon to get termination signal
        TRequestStatus status;
        process.Logon(status);
         
        // shutdown using P&S key
        RProperty::Set(KTAGDPSNotification,KShutdown,1);
        
        // blocks here until thread is terminated
        User::WaitForRequest(status); 
        
        // reset key
        RProperty::Set(KTAGDPSNotification,KShutdown,0);
        
        process.Close();
        }
    
    // find and terminate Thumbnail Server
    TFindProcess findProcess2( KThumbnailServerProcess );
    if ( findProcess2.Next(name) == KErrNone )
        {
        res = process.Open(name);
         
        // logon to get termination signal
        TRequestStatus status;
        process.Logon(status);
          
        // shutdown using P&S key
        RProperty::Set(KTMPSNotification,KShutdown,1);
         
        // blocks here until thread is terminated
        User::WaitForRequest(status); 
         
        // reset key
        RProperty::Set(KTMPSNotification,KShutdown,0);
         
        process.Close();
        }    
    
    // delay just in case old daemon hasn't died yet
    User::After(2500000);
    
    // start new ThumbAGDaemon
    RProcess server;
    TInt retryCount = 0;
    
    // Create the server process
    // KNullDesC param causes server's E32Main() to be run
    res = server.Create( KTAGDaemonExe, KNullDesC );
    
    // try again 3 times if fails
    while ( res != KErrNone)
        {
        if (retryCount > 2)
            {
            return;
            }
        
        User::After(2500000);
        res = server.Create( KTAGDaemonExe, KNullDesC );
        
        retryCount++;
        }
    
    // Process created successfully
    TRequestStatus status;
    server.Rendezvous( status );
    server.Resume(); // start it going
    
    // Wait until the completion of the server creation
    User::WaitForRequest( status );
    
    // Server created successfully
    server.Close(); // we're no longer interested in the other process
    }

GLDEF_C TInt E32Main()
    {
    // Create cleanup stack
    __UHEAP_MARK;
    CTrapCleanup* cleanup = CTrapCleanup::New();

    // Run application code inside TRAP harness
    TInt err = KErrNone;
    TRAP(err, MainL());

    delete cleanup;
    __UHEAP_MARKEND;
    return err;
    }

// End of file
