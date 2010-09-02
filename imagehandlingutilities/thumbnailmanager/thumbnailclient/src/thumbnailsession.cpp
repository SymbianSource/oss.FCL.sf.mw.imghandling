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
* Description:  Thumbnail server client-side session
 *
*/


// INCLUDE FILES
#include "thumbnailsession.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailsessionTraces.h"
#endif

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// RThumbnailSession::RThumbnailSession()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
RThumbnailSession::RThumbnailSession(): RSessionBase()
    {
    // No implementation required
    TN_DEBUG1( "RThumbnailSession::RThumbnailSession");
    OstTrace0( TRACE_NORMAL, RTHUMBNAILSESSION_RTHUMBNAILSESSION, "RThumbnailSession::RThumbnailSession" );
    }


// ---------------------------------------------------------------------------
// RThumbnailSession::Connect()
// ---------------------------------------------------------------------------
//
TInt RThumbnailSession::Connect()
    { 
    TN_DEBUG1( "RThumbnailSession::Connect - start");
    OstTrace0( TRACE_NORMAL, RTHUMBNAILSESSION_CONNECT, "RThumbnailSession::Connect - start" );
    
    StartServer();
    
    // special case
    // wait possibly needed here to give an old server process
    // time to enter shutdown state
    User::After(1000);
    
    TInt err = CreateSession( KThumbnailServerName, Version(), KMessageSlots );
    TInt retry = 1;
 
    // special case
    // old server still alive, wait and try again
    while (retry <= 10 && err != KErrNone)
        {
        TN_DEBUG1( "RThumbnailSession::Connect - retry");
        OstTrace0( TRACE_NORMAL, DUP1_RTHUMBNAILSESSION_CONNECT, "RThumbnailSession::Connect - retry" );
    
        User::After(retry * 50000);
        StartServer();
        err = CreateSession( KThumbnailServerName, Version(), KMessageSlots );
        retry++;
        }
    
    TN_DEBUG1( "RThumbnailSession::Connect - end");
    OstTrace0( TRACE_NORMAL, DUP2_RTHUMBNAILSESSION_CONNECT, "RThumbnailSession::Connect - end" );
    
    return err;
    }


// ---------------------------------------------------------------------------
// RThumbnailSession::Close()
// Closes session
// ---------------------------------------------------------------------------
//
void RThumbnailSession::Close()
    {
    TN_DEBUG1( "RThumbnailSession::Close");
    OstTrace0( TRACE_NORMAL, RTHUMBNAILSESSION_CLOSE, "RThumbnailSession::Close" );
    
    RSessionBase::Close();
    }


// ---------------------------------------------------------------------------
// RThumbnailSession::Version()
// Closes session
// ---------------------------------------------------------------------------
//
TVersion RThumbnailSession::Version()
    {
    return TVersion( KThumbnailServerMajorVersionNumber,
        KThumbnailServerMinorVersionNumber, KThumbnailServerBuildVersionNumber )
        ;
    }


// ---------------------------------------------------------------------------
// RThumbnailSession::StartServer()
// ---------------------------------------------------------------------------
//
TInt RThumbnailSession::StartServer()
    {
    TN_DEBUG1( "RThumbnailSession::StartServer - start");
    OstTrace0( TRACE_NORMAL, RTHUMBNAILSESSION_STARTSERVER, "RThumbnailSession::StartServer - start" );
    
    TInt res( KErrNone );
    // create server - if one of this name does not already exist

    TFindServer findServer( KThumbnailServerName );
    TFullName name;
    if ( findServer.Next( name ) != KErrNone )
        // we don't exist already
        {
        TN_DEBUG1( "RThumbnailSession::StartServer - server process doesn't exist yet");
        OstTrace0( TRACE_NORMAL, DUP1_RTHUMBNAILSESSION_STARTSERVER, "RThumbnailSession::StartServer - server process doesn't exist yet" );
    
        RProcess server;
        // Create the server process
        // KNullDesC param causes server's E32Main() to be run
        res = server.Create( KThumbnailServerExe, KNullDesC );
        if ( res != KErrNone )
            {
            TN_DEBUG2( "RThumbnailSession::StartServer - error creating process: %d", res);
            OstTrace1( TRACE_NORMAL, DUP2_RTHUMBNAILSESSION_STARTSERVER, "RThumbnailSession::StartServer;res=%d", res );
            return res;
            }
        
        TN_DEBUG1( "RThumbnailSession::StartServer - process created");
        OstTrace0( TRACE_NORMAL, DUP3_RTHUMBNAILSESSION_STARTSERVER, "RThumbnailSession::StartServer - process created" );

        // Process created successfully
        TRequestStatus status;
        server.Rendezvous( status );
        server.Resume(); // start it going

        // Wait until the completion of the server creation
        User::WaitForRequest( status );

        if ( status != KErrNone )
            {
            TN_DEBUG2( "RThumbnailSession::StartServer - status: %d, closing", status.Int() );
            OstTrace1( TRACE_NORMAL, DUP4_RTHUMBNAILSESSION_STARTSERVER, "RThumbnailSession::StartServer;status.Int()=%d", status.Int() );
        
            server.Close();
            return status.Int();
            }
        
        // Server created successfully
        server.Close(); // we're no longer interested in the other process
        }
    
    TN_DEBUG1( "RThumbnailSession::StartServer - end");
    OstTrace0( TRACE_NORMAL, DUP5_RTHUMBNAILSESSION_STARTSERVER, "RThumbnailSession::StartServer - end" );
    
    return res;
    }


// ---------------------------------------------------------------------------
// Request a thumbnail for an object file using file handle
// ---------------------------------------------------------------------------
//
void RThumbnailSession::RequestThumbnailL( const RFile64& aFile, const TDesC& aTargetUri,
    TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus )
    {
    TIpcArgs args( &aParams, KCheckValue ); // 1st and 2nd argument
    User::LeaveIfError( aFile.TransferToServer( args, 2, 3 )); // 3th and 4th argument
    aParams().iTargetUri = aTargetUri;
    SendReceive( ERequestThumbByFileHandleAsync, args, aStatus );
    }


// ---------------------------------------------------------------------------
// Request a thumbnail for an object file using file path
// ---------------------------------------------------------------------------
//
void RThumbnailSession::RequestThumbnailL( const TDesC& aPath, const TDesC& aTargetUri, const TThumbnailId /*aThumbnailId*/,
    TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus )
    {
    TIpcArgs args( &aParams, KCheckValue);
    aParams().iFileName = aPath;
    aParams().iTargetUri = aTargetUri;
    
    if(aPath.Length()== 0)
        {
        SendReceive( ERequestThumbByIdAsync, args, aStatus );
        }
    else
        {        
        SendReceive( ERequestThumbByPathAsync, args, aStatus );
        }
    }

// ---------------------------------------------------------------------------
// Request a thumbnail for an object file using file path
// ---------------------------------------------------------------------------
//
void RThumbnailSession::RequestThumbnailL( const TThumbnailId aThumbnailId,
        const TDesC& /*aTargetUri*/,
        TThumbnailRequestParamsPckg& aParams, 
        TRequestStatus& aStatus )
    {
    TIpcArgs args( &aParams, KCheckValue );
    aParams().iThumbnailId = aThumbnailId;
    SendReceive( ERequestThumbByIdAsync, args, aStatus );
    }

#if 0
// ---------------------------------------------------------------------------
// Request a thumbnail for an object file using file path
// ---------------------------------------------------------------------------
//
void RThumbnailSession::RequestThumbnailL( const TDesC& aPath, const TDesC& aTargetUri,
    TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus )
    {
    TIpcArgs args( &aParams );
    aParams().iFileName = aPath;
    aParams().iTargetUri = aTargetUri;
    SendReceive( ERequestThumbByPathAsync, args, aStatus );
    }
#endif

void RThumbnailSession::RequestSetThumbnailL( 
        TDesC8* aBuffer, const TDesC& aTargetUri,         
        TThumbnailRequestParamsPckg& aParams, 
        TRequestStatus& aStatus  )
    {
    if( !aBuffer )
        {
        TN_DEBUG1( "RThumbnailSession::RequestSetThumbnailL() - !aBuffer KErrArgument");
        OstTrace0( TRACE_NORMAL, RTHUMBNAILSESSION_REQUESTSETTHUMBNAILL, "RThumbnailSession::RequestSetThumbnailL - !aBuffer KErrArgument" );
        User::Leave( KErrArgument );
        }
    
    TIpcArgs args( &aParams, aBuffer, aBuffer->Length(), KCheckValue );
    aParams().iTargetUri = aTargetUri;
    SendReceive( ERequestSetThumbnailByBuffer, args, aStatus );   
    }

void RThumbnailSession::RequestSetThumbnailL( 
        TInt aBitmapHandle, const TDesC& aTargetUri,         
        TThumbnailRequestParamsPckg& aParams, 
        TRequestStatus& aStatus  )
    {
    if( !aBitmapHandle )
        {
        TN_DEBUG1( "RThumbnailSession::RequestSetThumbnailL() - !aBitmapHandle KErrArgument");
        OstTrace0( TRACE_NORMAL, DUP1_RTHUMBNAILSESSION_REQUESTSETTHUMBNAILL, "RThumbnailSession::RequestSetThumbnailL - !aBitmapHandle KErrArgument" );
        User::Leave( KErrArgument );
        }
    
    TIpcArgs args( &aParams, aBitmapHandle, KCheckValue );
    aParams().iTargetUri = aTargetUri;
    SendReceive( ERequestSetThumbnailByBitmap, args, aStatus );   
    }

// ---------------------------------------------------------------------------
// Release bitmap instance kept by server process
// ---------------------------------------------------------------------------
//
void RThumbnailSession::ReleaseBitmap( TInt aBitmapHandle )
    {
    TInt err = Send( EReleaseBitmap, TIpcArgs( aBitmapHandle ));
    while ( err == KErrServerBusy )
        {
        TN_DEBUG1( "RThumbnailSession::ReleaseBitmap() - server slots full");
        OstTrace0( TRACE_NORMAL, RTHUMBNAILSESSION_RELEASEBITMAP, "RThumbnailSession::ReleaseBitmap - server slots full" );
    
        err = Send( EReleaseBitmap, TIpcArgs( aBitmapHandle ));
        }
    }


// ---------------------------------------------------------------------------
// Cancel pending thumbnail request
// ---------------------------------------------------------------------------
//
TInt RThumbnailSession::CancelRequest( TThumbnailRequestId aRequestId )
    {
    TInt err = Send( ECancelRequest, TIpcArgs( aRequestId ));
    while ( err == KErrServerBusy )
        {
        TN_DEBUG1( "RThumbnailSession::CancelRequest() - server slots full");
        OstTrace0( TRACE_NORMAL, RTHUMBNAILSESSION_CANCELREQUEST, "RThumbnailSession::CancelRequest - server slots full" );
    
        err = Send( ECancelRequest, TIpcArgs( aRequestId ));
        }
    return err;
    }


// ---------------------------------------------------------------------------
// Change priority of pending thumbnail request
// ---------------------------------------------------------------------------
//
TInt RThumbnailSession::ChangePriority( TThumbnailRequestId aRequestId, TInt
    aNewPriority )
    {
    TInt err = Send( EChangePriority, TIpcArgs( aRequestId, aNewPriority ));
    while ( err == KErrServerBusy )
        {
        TN_DEBUG1( "RThumbnailSession::ChangePriority() - server slots full");
        OstTrace0( TRACE_NORMAL, RTHUMBNAILSESSION_CHANGEPRIORITY, "RThumbnailSession::ChangePriority - server slots full" );
    
        err = Send( EChangePriority, TIpcArgs( aRequestId, aNewPriority ));
        }
    return err;
    }

// ---------------------------------------------------------------------------
// Delete thumbnails for given object file
// ---------------------------------------------------------------------------
//
void RThumbnailSession::DeleteThumbnails( const TDesC& aPath,
        TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus )
    {
    TIpcArgs args( &aParams, &aPath, KCheckValue);
            
    SendReceive( EDeleteThumbnails, args, aStatus ); 
    }


// ---------------------------------------------------------------------------
// Delete thumbnails by TThumbnailId.
// ---------------------------------------------------------------------------
//
void RThumbnailSession::DeleteThumbnails( const TThumbnailId aItemId,
        TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus )
    {   
    TIpcArgs args( &aParams, aItemId, KCheckValue);
            
    SendReceive( EDeleteThumbnailsById, args, aStatus );  
    }


// ---------------------------------------------------------------------------
//  Get a list of supported MIME types in a HBufC
// ---------------------------------------------------------------------------
//
HBufC* RThumbnailSession::GetMimeTypeListL()
    {
    TInt size = 0;
    TPckg < TInt > pckg( size );
    User::LeaveIfError( SendReceive( EGetMimeTypeBufferSize, TIpcArgs( &pckg )));
    HBufC* res = HBufC::NewLC( size );
    TPtr ptr = res->Des();
    User::LeaveIfError( SendReceive( EGetMimeTypeList, TIpcArgs( &ptr )));
    CleanupStack::Pop( res );
    return res;
    }


// ---------------------------------------------------------------------------
// Update thumbnails.
// ---------------------------------------------------------------------------
//
void RThumbnailSession::UpdateThumbnails( const TDesC& aPath, const TInt aOrientation,
        const TInt64 aModified, TThumbnailRequestParamsPckg& aParams, TRequestStatus& aStatus )
    {
    aParams().iFileName = aPath;
    aParams().iTargetUri = KNullDesC;
    aParams().iOrientation = aOrientation;
    aParams().iModified = aModified;
        
    TIpcArgs args( &aParams, KCheckValue);
            
    SendReceive( EUpdateThumbnails, args, aStatus );  
    }

// ---------------------------------------------------------------------------
// Rename thumbnails.
// ---------------------------------------------------------------------------
//
void RThumbnailSession::RenameThumbnails( TThumbnailRequestParamsPckg& aParams, 
                                          TRequestStatus& aStatus )
    {
    TIpcArgs args( &aParams, KCheckValue);
            
    SendReceive( ERenameThumbnails, args, aStatus ); 
    }
    

// End of file
