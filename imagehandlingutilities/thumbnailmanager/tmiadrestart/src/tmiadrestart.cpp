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
#include "thumbnaillog.h"

#include "thumbnailmanagerconstants.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tmiadrestartTraces.h"
#endif

LOCAL_C void MainL()
    {
    // delay so that cenrep has time to read new config
    TN_DEBUG1( "MainL() after" );
    OstTrace0( TRACE_NORMAL, _MAINL, "::MainL - after" );
    
    User::After(5000000);
    
    TInt res( KErrNone );
    RProcess process;
    TFullName name;
    
    // find and terminate Thumb AG Daemon
    TN_DEBUG1( "MainL() KTAGDaemonProcess find" );
    OstTrace0( TRACE_NORMAL, DUP1__MAINL, "::MainL - find" );
    TFindProcess findProcess( KTAGDaemonProcess );   

    if ( findProcess.Next(name) == KErrNone )
        {
        res = process.Open(name);
        TN_DEBUG2( "MainL() KTAGDaemonProcess open res = %d", res );    
        OstTrace1( TRACE_NORMAL, DUP2__MAINL, "::MainL;res=%d - open res", res );
        
        // logon to get termination signal
        TRequestStatus status;
        TN_DEBUG1( "MainL() KTAGDaemonProcess logon" );
        OstTrace0( TRACE_NORMAL, DUP3__MAINL, "::MainL - logon" );
        process.Logon(status);

        // shutdown using P&S key
        TN_DEBUG2( "MainL() KTAGDaemonProcess Set = %d", res );
        OstTrace1( TRACE_NORMAL, DUP4__MAINL, "::MainL - Set = res;res=%d", res );
        res = RProperty::Set(KTAGDPSNotification,KShutdown,1);
       
        // blocks here until thread is terminated
        TN_DEBUG1( "MainL() KTAGDaemonProcess wait" );
        OstTrace0( TRACE_NORMAL, DUP5__MAINL, "::MainL - KTAGDaemonProcess wait" );
        User::WaitForRequest(status);
        
        // reset key
        TN_DEBUG1( "MainL() KTAGDaemonProcess reset" );
        OstTrace0( TRACE_NORMAL, DUP6__MAINL, "::MainL - KTAGDaemonProcess reset" );
        res = RProperty::Set(KTAGDPSNotification, KShutdown, 0);
        
        TN_DEBUG1( "MainL() KTAGDaemonProcess close" );
        OstTrace0( TRACE_NORMAL, DUP7__MAINL, "::MainL - close" );
        process.Close();
        }
    
    // find and terminate Thumbnail Server
    TFindProcess findProcess2( KThumbnailServerProcess );
    TN_DEBUG1( "MainL() KThumbnailServerProcess find" );
    OstTrace0( TRACE_NORMAL, DUP8__MAINL, "::MainL - find" );
    if ( findProcess2.Next(name) == KErrNone )
        {
        res = process.Open(name);
        TN_DEBUG2( "MainL() KThumbnailServerProcess open res = %d", res );
        OstTrace1( TRACE_NORMAL, DUP9__MAINL, "::MainL - KThumbnailServerProcess open;res=%d", res );
        
        // logon to get termination signal
        TRequestStatus status;
        TN_DEBUG1( "MainL() KThumbnailServerProcess logon" );
        OstTrace0( TRACE_NORMAL, DUP10__MAINL, "::MainL - KThumbnailServerProcess logon" );
        process.Logon(status);
          
        // shutdown using P&S key
        TN_DEBUG2( "MainL() KThumbnailServerProcess Set res = %d", res );
        OstTrace1( TRACE_NORMAL, DUP11__MAINL, "::MainL - KThumbnailServerProcess Set;res=%d", res );
        res = RProperty::Set(KTMPSNotification,KShutdown,1);
         
        // blocks here until thread is terminated
        TN_DEBUG1( "MainL() KThumbnailServerProcess wait" );
        OstTrace0( TRACE_NORMAL, DUP12__MAINL, "::MainL - KThumbnailServerProcess wait" );
        User::WaitForRequest(status);
         
        // reset key
        TN_DEBUG2( "MainL() KThumbnailServerProcess reset res = %d", res );
        OstTrace1( TRACE_NORMAL, DUP13__MAINL, "::MainL;res=%d - KThumbnailServerProcess reset", res );
        res = RProperty::Set(KTMPSNotification, KShutdown, 0);

        TN_DEBUG1( "MainL() KThumbnailServerProcess close" );
        OstTrace0( TRACE_NORMAL, DUP14__MAINL, "::MainL - close" );
        process.Close();
        }    
    
    // delay just in case old daemon hasn't died yet
    TN_DEBUG1( "MainL() after" );
    User::After(2500000);
    
    // start new ThumbAGDaemon
    RProcess server;
    TInt retryCount = 0;
    
    // Create the server process
    // KNullDesC param causes server's E32Main() to be run
    TN_DEBUG2( "MainL() KTAGDaemonExe create res = %d", res );
    OstTrace1( TRACE_NORMAL, DUP15__MAINL, "::MainL - KTAGDaemonExe create res;res=%d", res );
    res = server.Create( KTAGDaemonExe, KNullDesC );
    
    // try again 3 times if fails
    while ( res != KErrNone)
        {
        TN_DEBUG1( "MainL() KTAGDaemonExe retry" );
        OstTrace0( TRACE_NORMAL, DUP16__MAINL, "::MainL- KTAGDaemonExe retry" );
        if (retryCount > 2)
            {
            TN_DEBUG1( "MainL() KTAGDaemonExe retry > 2" );
            OstTrace0( TRACE_NORMAL, DUP17__MAINL, "::MainL - KTAGDaemonExe retry > 2" );
            return;
            }
        
        TN_DEBUG1( "MainL() after" );
        OstTrace0( TRACE_NORMAL, DUP18__MAINL, "::MainL - after" );
        User::After(2500000);
        TN_DEBUG2( "MainL() KTAGDaemonExe create res = %d", res );
        OstTrace1( TRACE_NORMAL, DUP19__MAINL, "::MainL - KTAGDaemonExe create;res=%d", res );
        res = server.Create( KTAGDaemonExe, KNullDesC );
        retryCount++;
        }
    
    // Process created successfully
    TRequestStatus status;
    TN_DEBUG1( "MainL() Rendezvous" );
    OstTrace0( TRACE_NORMAL, DUP20__MAINL, "::MainL - Rendezvous" );
    server.Rendezvous( status );
    TN_DEBUG1( "MainL() Resume" );
    OstTrace0( TRACE_NORMAL, DUP21__MAINL, "::MainL - resume" );
    server.Resume(); // start it going
    
    
    // Wait until the completion of the server creation
    TN_DEBUG1( "MainL() wait" );
    OstTrace0( TRACE_NORMAL, DUP22__MAINL, "::MainL - wait" );
    User::WaitForRequest( status );
    TN_DEBUG1( "MainL() wait done" );
    OstTrace0( TRACE_NORMAL, DUP23__MAINL, "::MainL - wait done"  );
    
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
    cleanup = NULL;
    __UHEAP_MARKEND;
    return err;
    }

// End of file
