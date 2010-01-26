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

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// RThumbnailSession::RThumbnailSession()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
RThumbnailSession::RThumbnailSession(): RSessionBase()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// RThumbnailSession::Connect()
// ---------------------------------------------------------------------------
//
TInt RThumbnailSession::Connect()
    { 
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
        User::After(retry * 50000);
        StartServer();
        err = CreateSession( KThumbnailServerName, Version(), KMessageSlots );
        retry++;
        }
    
    return err;
    }


// ---------------------------------------------------------------------------
// RThumbnailSession::Close()
// Closes session
// ---------------------------------------------------------------------------
//
void RThumbnailSession::Close()
    {
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
    TInt res( KErrNone );
    // create server - if one of this name does not already exist

    TFindServer findServer( KThumbnailServerName );
    TFullName name;
    if ( findServer.Next( name ) != KErrNone )
    // we don't exist already
        {
        RProcess server;
        // Create the server process
        // KNullDesC param causes server's E32Main() to be run
        res = server.Create( KThumbnailServerExe, KNullDesC );
        if ( res != KErrNone )
        // thread created ok - now start it going
            {
            return res;
            }

        // Process created successfully
        TRequestStatus status;
        server.Rendezvous( status );
        server.Resume(); // start it going

        // Wait until the completion of the server creation
        User::WaitForRequest( status );

        if ( status != KErrNone )
            {
            server.Close();
            return status.Int();
            }
        // Server created successfully
        server.Close(); // we're no longer interested in the other process

        }
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
        err = Send( EReleaseBitmap, TIpcArgs( aBitmapHandle ));
        }
    }


// ---------------------------------------------------------------------------
// Cancel pending thumbnail request
// ---------------------------------------------------------------------------
//
TInt RThumbnailSession::CancelRequest( TThumbnailRequestId aRequestId )
    {
    TInt err = SendReceive( ECancelRequest, TIpcArgs( aRequestId ));
    while ( err == KErrServerBusy )
        {
        err = SendReceive( ECancelRequest, TIpcArgs( aRequestId ));
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
    TInt err = SendReceive( EChangePriority, TIpcArgs( aRequestId, aNewPriority ));
    while ( err == KErrServerBusy )
        {
        err = SendReceive( EChangePriority, TIpcArgs( aRequestId, aNewPriority ));
        }
    return err;
    }

// ---------------------------------------------------------------------------
// Delete thumbnails for given object file
// ---------------------------------------------------------------------------
//
TInt RThumbnailSession::CreateThumbnails( const RFile64& aFile, TDisplayMode aDisplayMode )
    {
    TIpcArgs args( aDisplayMode ); // 1st argument
    TInt err = aFile.TransferToServer( args, 1, 2 ); // 2.&3. argument
    err = Send( ECreateThumbnails, args );
    while ( err == KErrServerBusy )
        {
        err = Send( ECreateThumbnails, args );
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
    

// End of file
