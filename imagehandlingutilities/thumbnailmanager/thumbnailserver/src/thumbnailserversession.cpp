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
* Description:  Server side session for Thumbnail Manager Server
 *
*/

#include <imageconversion.h> 

#include "thumbnailserversession.h"
#include "thumbnailserver.h"
#include "thumbnailtaskprocessor.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailgeneratetask.h"
#include "thumbnailscaletask.h"
#include "thumbnaildecodetask.h"
#ifdef RD_MDS_2_5
#include "thumbnailmdsquerytask.h"
#endif // RD_MDS_2_5
#include "thumbnaillog.h"
#include "thumbnailpanic.h"

#include "thumbnailcenrep.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailserversessionTraces.h"
#endif


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailServerSession::CThumbnailServerSession()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailServerSession::CThumbnailServerSession(): CSession2()
    {
    iBitmapHandle = 0;
    iBitmap = NULL;
    iBuffer = NULL;
    }


// ---------------------------------------------------------------------------
// CThumbnailServerSession::~CThumbnailServerSession()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailServerSession::~CThumbnailServerSession()
    {
    Server()->DropSession(this);
    delete iBitmap;
    iBitmap = NULL;
    delete iBuffer;
    iBuffer = NULL;
    }


// -----------------------------------------------------------------------------
// CThumbnailServerSession::DispatchMessageL()
// Message dispatcher.
// -----------------------------------------------------------------------------
//
TInt CThumbnailServerSession::DispatchMessageL( const RMessage2& aMessage )
    {
    TInt err( KErrNone );

    switch ( aMessage.Function())
        {
        case ERequestThumbByFileHandleAsync:
                {
                RequestThumbByFileHandleAsyncL( aMessage );
                break;
                }
        case ERequestThumbByPathAsync:
                {
                RequestThumbByPathAsyncL( aMessage );
                break;
                }
        case ERequestSetThumbnailByBuffer:
                {
                RequestSetThumbnailByBufferL( aMessage );
                break;
                }
        case ERequestSetThumbnailByBitmap:
                {
                RequestSetThumbnailByBitmapL( aMessage );
                break;
                }                
        case EReleaseBitmap:
                {
                ReleaseBitmap( aMessage );
                break;
                }
        case ECancelRequest:
                {
                err = CancelRequest( aMessage );
                break;
                }
        case EChangePriority:
                {
                err = ChangePriority( aMessage );
                break;
                }
        case EDeleteThumbnails:
                {
                DeleteThumbnailsL( aMessage );
                break;
                }
        case EDeleteThumbnailsById:
                {
                DeleteThumbnailsByIdL( aMessage );
                break;
                }                
        case EGetMimeTypeBufferSize:
                {
                GetMimeTypeBufferSizeL( aMessage );
                break;
                }
        case EGetMimeTypeList:
                {
                GetMimeTypeListL( aMessage );
                break;
                }
        case ERequestThumbByIdAsync:
                {
                RequestThumbByIdAsyncL( aMessage );
                break;
                }
        case EUpdateThumbnails:
                {
                UpdateThumbnailsL( aMessage );
                break;
                }       
        case ERenameThumbnails:
                {
                RenameThumbnailsL( aMessage );
                break;
                } 
        default:
                {
                err = KErrUnknown;
                break;
                }
        }

    return err;
    }


// ---------------------------------------------------------------------------
// CThumbnailServerSession::CreateL()
// ---------------------------------------------------------------------------
//
void CThumbnailServerSession::CreateL()
    {
    TN_DEBUG2( "CThumbnailServerSession::AddSession() = 0x%08x", this );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_CREATEL, "CThumbnailServerSession::CreateL;this=%o", this );
    
    Server()->AddSession();
    }


// -----------------------------------------------------------------------------
// CThumbnailServerSession::ServiceL()
// Handles service request messages from clients.
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::ServiceL( const RMessage2& aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::ServiceL() - begin" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_SERVICEL, "CThumbnailServerSession::ServiceL - begin" );
    
    __ASSERT_DEBUG( !iMessage.Handle(), ThumbnailPanic(EThumbnailMessageNotCompleted));
    if ( iMessage.Handle())
        {
        iMessage.Complete( KErrUnknown );
        iMessage = RMessage2();
        }
    
    iMessage = aMessage;

    if ( iMessage.Handle())
        {
        // get client thread
        TInt err = iMessage.Client( iClientThread );
        if (err != KErrNone)
            {
            TN_DEBUG1( "CThumbnailServerSession::ServiceL() - client thread not found");
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_SERVICEL, "CThumbnailServerSession::ServiceL - client thread not found" );
    
            iMessage = RMessage2();
            }       
        }
    else
        {
        TN_DEBUG1( "CThumbnailServerSession::ServiceL() - message null");
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_SERVICEL, "CThumbnailServerSession::ServiceL - message null" );
        }
    
    // clean up possible trash
    if (iBitmapHandle)
        {
        Server()->DeleteBitmapFromPool( iBitmapHandle );
        iBitmapHandle = 0;
        }
    delete iBitmap;
    iBitmap = NULL;
    delete iBuffer;
    iBuffer = NULL;
    iOriginalSize = TSize();
    
    TInt ret = KErrNone;

    TRAPD( err, ret = DispatchMessageL( aMessage ) );
    
    // if message was not completed, or Leave occurred
    if ( iMessage.Handle())
        {
        if ( ClientThreadAlive() )
            {
            iMessage.Complete( ConvertSqlErrToE32Err( err != KErrNone ? err : ret ));        
            }
        
        iMessage = RMessage2();
        }
    
    // close thread handle
    iClientThread.Close();
    
    TN_DEBUG1( "CThumbnailServerSession::ServiceL() - end" );
    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILSERVERSESSION_SERVICEL, "CThumbnailServerSession::ServiceL - end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailServerSession::Server()
// Returns the server pointer.
// -----------------------------------------------------------------------------
//
CThumbnailServer* CThumbnailServerSession::Server()
    {
    return ( CThumbnailServer* )( CSession2::Server());
    }


// -----------------------------------------------------------------------------
// CThumbnailServerSession::Cancel()
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::Cancel()
    {
    }


// -----------------------------------------------------------------------------
// CThumbnailServerSession::UpdateThumbnailsL()
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::UpdateThumbnailsL( const RMessage2& aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::UpdateThumbnailsL()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL, "CThumbnailServerSession::UpdateThumbnailsL" );
    
    if(aMessage.Int1() != KCheckValue)
       {
       TN_DEBUG1( "CThumbnailServerSession::UpdateThumbnailsL() - error in aMessage - leaving" );
       User::Leave(KErrArgument);
       }
    
    // read message params
    aMessage.ReadL( 0, iRequestParams );
    const TThumbnailRequestParams& params = iRequestParams();
    
    TBool finished = Server()->UpdateThumbnailsL( params.iFileName, params.iOrientation, params.iModified );
    
    RArray < TThumbnailPersistentSize >* missingSizes = NULL;
    
    TBool gridSizeOnly(EFalse);
            
    if ( params.iQualityPreference == CThumbnailManager
                    ::EOptimizeForQualityWithPreview )
        {
        gridSizeOnly = ETrue;
        }
    
    if (finished)
        {
        // source type
        TDataType mimeType;
        TInt sourceType = 0;
        TInt err = Server()->MimeTypeFromFileExt( params.iFileName, mimeType );
        
        // need to use recognizer
        if (err == KErrNotFound)
            {
            Server()->Fs().ShareProtected();
            RFile64 file;
            CleanupClosePushL( file );
          
            User::LeaveIfError( file.Open( Server()->Fs(), params.iFileName, EFileShareReadersOrWriters )); 
            TN_DEBUG2( "CThumbnailServerSession::UpdateThumbnailsL - file handle opened for %S", &params.iFileName );
          
            mimeType = Server()->ResolveMimeTypeL(file);
          
            file.Close();
            TN_DEBUG1("CThumbnailServerSession::UpdateThumbnailsL - file handle closed");
          
            CleanupStack::Pop( &file );    
            }
        
        // get missing sizes
        if ( ( params.iControlFlags & EThumbnailGeneratePersistentSizesOnly ) != 0 )
            {
            sourceType = Server()->SourceTypeFromMimeType( mimeType );
            
            missingSizes = new (ELeave) RArray < TThumbnailPersistentSize >;
            CleanupClosePushL( *missingSizes );
        
            Server()->GetMissingSizesL( params.iFileName, sourceType, *missingSizes, gridSizeOnly );
                
            if ( missingSizes->Count() == 0)
                {
                // all thumbs already exist
                CleanupStack::PopAndDestroy( missingSizes );
                delete missingSizes;
                missingSizes = NULL;
                }            
            }
        
        if(!missingSizes)
            {
            TN_DEBUG1( "CThumbnailServerSession::UpdateThumbnailsL() - finished part 1" );
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL, "CThumbnailServerSession::UpdateThumbnailsL - finished part 1" );
            aMessage.Complete( KErrNone );
            }
        else
            {
            OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL, "CThumbnailServerSession::UpdateThumbnailsL  - some sizes missing..." );
            TN_DEBUG1( "CThumbnailServerSession::UpdateThumbnailsL() - some sizes missing..." ); 
            }
        }

    if (missingSizes || !finished)
        {
        TN_DEBUG1( "CThumbnailServerSession::UpdateThumbnailsL() - need to create (some) thumbs" );
        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL, "CThumbnailServerSession::UpdateThumbnailsL - need to create (some) thumbs" );
        
        if(missingSizes)
            {
            CleanupStack::PopAndDestroy( missingSizes );
            delete missingSizes;
            missingSizes = NULL;
            }
        
        if(Server()->StoreForPathL(params.iFileName)->IsDiskFull())
            {
            User::Leave( KErrDiskFull );
            }
        
        // need to create new thumbs
        aMessage.Complete( KThumbnailErrThumbnailNotFound );
        }
    else
        {
        TN_DEBUG1( "CThumbnailServerSession::UpdateThumbnailsL() - finished part 2" );
        OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILSERVERSESSION_UPDATETHUMBNAILSL, "CThumbnailServerSession::UpdateThumbnailsL - finished part 2" );
        }
    
    iMessage = RMessage2();
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::RenameThumbnailsL()
// Rename thumbnails.
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::RenameThumbnailsL( const RMessage2& aMessage )
    {
    if(aMessage.Int1() != KCheckValue)
       {
       TN_DEBUG1( "CThumbnailServerSession::RenameThumbnailsL() - error in aMessage - leaving" );
       OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_RENAMETHUMBNAILSL, "CThumbnailServerSession::RenameThumbnailsL - error in aMessage - leaving" );
       User::Leave(KErrArgument);
       }
    
    // read message params
    aMessage.ReadL( 0, iRequestParams );
    const TThumbnailRequestParams& params = iRequestParams();
    
    // renaming only inside one store
    if (params.iFileName.Left(1).CompareF( params.iTargetUri.Left(1) ) == 0)
        {
        Server()->RenameThumbnailsL( params.iFileName, params.iTargetUri );
        
        aMessage.Complete( KErrNone );
        }
    else
        {
        aMessage.Complete( KErrNotSupported );
        }
    
    iMessage = RMessage2();
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::RequestThumbByIdAsyncL()
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::RequestThumbByIdAsyncL( const RMessage2&
    aMessage )
    {
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    TN_DEBUG1( "CThumbnailServerSession::RequestThumbByIdAsyncL() - begin" );
    
    if(aMessage.Int1() != KCheckValue)
       {
       TN_DEBUG1( "CThumbnailServerSession::RequestThumbByIdAsync() - error in aMessage - leaving" );
       OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYIDASYNCL, "CThumbnailServerSession::RequestThumbByIdAsyncL - leaving" );
       User::Leave(KErrArgument);
       }

    aMessage.ReadL( 0, iRequestParams );
    const TThumbnailRequestParams& params = iRequestParams();
    
#ifdef RD_MDS_2_5        
        // try to query path from MDS
        CThumbnailMDSQueryTask* task = new( ELeave )CThumbnailMDSQueryTask(
                Server()->Processor(), params.iPriority + 1, Server()->GetMdESession(), *Server());
        
        CleanupStack::PushL( task );
        task->QueryPathByIdL(params.iThumbnailId, EFalse);
        task->SetMessageData( TThumbnailServerRequestId( this, params.iRequestId ), iMessage, iClientThread );
        Server()->QueueTaskL( task );
        CleanupStack::Pop( task ); // owned by processor now
        
        // query task is now responsible for completing the message
        iMessage = RMessage2();
#else
        User::Leave(KErrNotSupported);
#endif // RD_MDS_2_5
        
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TInt tookTime = (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000;
    TN_DEBUG2( "CThumbnailServerSession::RequestThumbByIdAsyncL() request took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000 );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYIDASYNCL, "CThumbnailServerSession::RequestThumbByIdAsyncL;tookTime=%d", tookTime );
#endif 
    
    TN_DEBUG1("CThumbnailServerSession::RequestThumbByIdAsyncL() - end" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYIDASYNCL, "CThumbnailServerSession::RequestThumbByIdAsyncL - end" );
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::RequestThumbByFileHandleAsync()
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::RequestThumbByFileHandleAsyncL( const RMessage2&
    aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::RequestThumbByFileHandleAsyncL()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL" );
    
    if(aMessage.Int1() != KCheckValue)
       {
       TN_DEBUG1( "CThumbnailServerSession::RequestThumbByFileHandleAsync() - error in aMessage - leaving" );
       OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - error in aMessage - leaving" );
       User::Leave(KErrArgument);
       }
    
    aMessage.ReadL( 0, iRequestParams );
    const TThumbnailRequestParams& params = iRequestParams();

    RFile64 file;
    CleanupClosePushL(file);
    User::LeaveIfError( file.AdoptFromClient( aMessage, 2, 3 ));
    
    ResolveMimeTypeL(&file);
    
    if(params.iThumbnailSize == EFullScreenThumbnailSize ||
       params.iThumbnailSize == EGridThumbnailSize ||
       params.iThumbnailSize == EListThumbnailSize )
        {
        TInt sourceType = TThumbnailPersistentSize::EUnknownSourceType;
        sourceType = Server()->SourceTypeFromMimeType( params.iMimeType );
        ModifyThumbnailSize(sourceType);
        }
    
    // delete existing
    if(params.iImport && params.iOverwrite)
        {
        Server()->DeleteThumbnailsL( params.iTargetUri);
        }
    
    // CreateThumbnails
    if (params.iControlFlags == EThumbnailGeneratePersistentSizesOnly)
        {
        TN_DEBUG1( "CThumbnailServerSession::RequestThumbByFileHandleAsyncL() - EThumbnailGeneratePersistentSizesOnly" );
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - EThumbnailGeneratePersistentSizesOnly" );
        CreateGenerateTaskFromFileHandleL( &file );
        
        // ownership of the file transferred
        CleanupStack::Pop(&file);
        }
    // single thumbnail request
    else
        {
        TRAPD( err, FetchThumbnailL());
        
        if( err == KErrCompletion )
            {
            // If thumbnail of requested size is blacklisted, fetching is left with KErrCompletion
            TN_DEBUG1( "CThumbnailServerSession::RequestThumbByFileHandleAsyncL() - thumbnail blacklisted" );
            OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - thumbnail blacklisted" );
            
            aMessage.Complete( err );
            iMessage = RMessage2();
            
            // close file
            CleanupStack::PopAndDestroy(&file);
            }
        else if ( !err && iBitmap )
            {
            TN_DEBUG1( "CThumbnailServerSession::RequestThumbByFileHandleAsyncL() - found existing thumbnail - bitmap " );
            OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - found existing thumbnail - bitmap " );

            // Thumbnail already stored
            CleanupStack::PopAndDestroy(&file);
            TN_DEBUG1("CThumbnailServerSession::RequestThumbByFileHandleAsyncL - file handle closed");
            OstTrace0( TRACE_NORMAL, DUP5_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - file handle closed" );

            ProcessBitmapL();
            }
        else if ( (err == KErrNotFound || err == KErrAccessDenied) && 
                 !(params.iFlags& CThumbnailManager::EDoNotCreate) )
            {
            TN_DEBUG1( "CThumbnailServerSession::RequestThumbByFileHandleAsyncL() - KErrNotFound & !EDoNotCreate" );
            OstTrace0( TRACE_NORMAL, DUP6_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - KErrNotFound & !EDoNotCreate" );
            CreateGenerateTaskFromFileHandleL( &file);
            
            // ownership of the file transferred
        	CleanupStack::Pop(&file);
            }
        else if (!err && iBuffer)
            {
            TN_DEBUG1( "CThumbnailServerSession::RequestThumbByFileHandleAsyncL() - found existing thumbnail - jpeg " );
            OstTrace0( TRACE_NORMAL, DUP7_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - found existing thumbnail - jpeg" );
            
            CThumbnailDecodeTask* task = new( ELeave )CThumbnailDecodeTask( Server()
                        ->Processor(), * Server(), iBuffer, params.iPriority, params.iDisplayMode );
            
            CleanupStack::PushL( task );
            task->SetMessageData( TThumbnailServerRequestId( this, params.iRequestId ), iMessage, iClientThread );
            Server()->QueueTaskL( task );
            CleanupStack::Pop( task ); // owned by processor now
            
            // Decode task is now responsible for completing the message
            iMessage = RMessage2();
            
            //CThumbnailDecodeTask is responsible freeing
            iBuffer = NULL;
            
            // close file
            CleanupStack::PopAndDestroy(&file);
            TN_DEBUG1("CThumbnailServerSession::RequestThumbByFileHandleAsyncL - file handle closed");
            OstTrace0( TRACE_NORMAL, DUP8_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - file handle closed" );
            }
        else
            {
            TN_DEBUG2( "CThumbnailServerSession::RequestThumbByFileHandleAsyncL() - thumbnail not found, err=%d", err );
            OstTrace1( TRACE_NORMAL, DUP9_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYFILEHANDLEASYNCL, "CThumbnailServerSession::RequestThumbByFileHandleAsyncL - thumbnail not found;err=%d", err );
            
            aMessage.Complete( ConvertSqlErrToE32Err( err ));
            iMessage = RMessage2();
            
            // close file
            CleanupStack::PopAndDestroy(&file);
            }     
        }
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::RequestThumbByPathAsync()
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::RequestThumbByPathAsyncL( const RMessage2&
    aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::RequestThumbByPathAsyncL()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL, "CThumbnailServerSession::RequestThumbByPathAsyncL" );
    
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    if(aMessage.Int1() != KCheckValue)
       {
       TN_DEBUG1( "CThumbnailServerSession::RequestThumbByPathAsync() - error in aMessage - leaving" );
       OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL, "CThumbnailServerSession::RequestThumbByPathAsyncL - error in aMessage - leaving" );
       User::Leave(KErrArgument);
       }

    aMessage.ReadL( 0, iRequestParams );
    const TThumbnailRequestParams& params = iRequestParams();
    
    ResolveMimeTypeL(NULL);
    
    if(params.iThumbnailSize == EFullScreenThumbnailSize ||
       params.iThumbnailSize == EGridThumbnailSize ||
       params.iThumbnailSize == EListThumbnailSize )
        {
        TInt sourceType = TThumbnailPersistentSize::EUnknownSourceType;
        sourceType = Server()->SourceTypeFromMimeType( params.iMimeType );
        ModifyThumbnailSize(sourceType);
        }
    
    // should always be true
    if (params.iControlFlags != EThumbnailGeneratePersistentSizesOnly)
		{
	    TRAPD( err, FetchThumbnailL());
	    
	    if ( !err && iBitmap )
	        {
	        TN_DEBUG1( 
	            "CThumbnailServerSession::RequestThumbByPathAsyncL() - found existing thumbnail- bitmap" );
	        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL, "CThumbnailServerSession::RequestThumbByPathAsyncL - found existing thumbnail- bitmap" );

	        ProcessBitmapL();
	        }
	    else if ( !err && iBuffer)
	        {
	        TN_DEBUG1( 
	            "CThumbnailServerSession::RequestThumbByPathAsyncL() - found existing thumbnail- jpeg" );
	        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL, "CThumbnailServerSession::RequestThumbByPathAsyncL - found existing thumbnail- jpeg" );
	        
	        CThumbnailDecodeTask* task = new( ELeave )CThumbnailDecodeTask( Server()
	               ->Processor(), * Server(), iBuffer, params.iPriority, params.iDisplayMode );
	        
	        CleanupStack::PushL( task );
	        task->SetMessageData( TThumbnailServerRequestId( this, params.iRequestId ), iMessage, iClientThread );
	        Server()->QueueTaskL( task );
	        CleanupStack::Pop( task ); // owned by processor now
	        
	        // Decode task is now responsible for completing the message
	        iMessage = RMessage2();
	        
	        //CThumbnailDecodeTask is responsible freeing
	        iBuffer = NULL;
	        }
	    else if( err == KErrCompletion )
	        {
            // If thumbnail of requested size is blacklisted, fetching is left with KErrCompletion
            TN_DEBUG1( "CThumbnailServerSession::RequestThumbByPathAsyncL() - thumbnail blacklisted" );
            OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL, "CThumbnailServerSession::RequestThumbByPathAsyncL - thumbnail blacklisted" );
            
            aMessage.Complete( err );
            iMessage = RMessage2();
	        }
	    else 
	        {
	        TN_DEBUG2( 
	            "CThumbnailServerSession::RequestThumbByPathAsyncL() - thumbnail not found, err = %d", err );
	        OstTrace1( TRACE_NORMAL, DUP5_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL, "CThumbnailServerSession::RequestThumbByPathAsyncL  - thumbnail not found;err=%d", err );
	        
	        // don't try to create from virtual URI
	        if ( params.iVirtualUri )
	            {
                User::Leave(err);
	            }
	        
            // disk space check only for stored sizes
            if ( params.iThumbnailSize != ECustomThumbnailSize && 
                 Server()->StoreForPathL(params.iFileName)->IsDiskFull() )
                {
                User::Leave( KErrDiskFull );
                }
	        
	        if ( (err == KErrNotFound || err == KErrAccessDenied) && 
	            !(params.iFlags& CThumbnailManager::EDoNotCreate) )
	            {
	            // Special error code so that the client side can open the file
	            // and retry the request using file handle
	            err = KThumbnailErrThumbnailNotFound;
	            }

            User::Leave(err);
	        }   
	   }
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TInt tookTime = (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000;
    TN_DEBUG2( "CThumbnailStore::RequestThumbByPathAsyncL() request took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000 );
    OstTrace1( TRACE_NORMAL, DUP6_CTHUMBNAILSERVERSESSION_REQUESTTHUMBBYPATHASYNCL, "CThumbnailServerSession::RequestThumbByPathAsyncL;tookTime=%d", tookTime );
#endif
    }   
    
void CThumbnailServerSession::RequestSetThumbnailByBufferL( const RMessage2& aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::RequestSetThumbnailByBufferL()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBUFFERL, "CThumbnailServerSession::RequestSetThumbnailByBufferL" );
    
    if(aMessage.Int3() != KCheckValue)
        {
        TN_DEBUG1( "CThumbnailServerSession::RequestSetThumbnailByBufferL() - error in aMessage - leaving" );
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBUFFERL, "CThumbnailServerSession::RequestSetThumbnailByBufferL - error in aMessage - leaving" );
        User::Leave(KErrArgument);
        }
      
    aMessage.ReadL( 0, iRequestParams );
    const TThumbnailRequestParams& params = iRequestParams();
    
    // delete existing
    if(params.iOverwrite)
        {
        Server()->DeleteThumbnailsL( params.iTargetUri);
        }
    
    // if only one size
    if ((params.iControlFlags & EThumbnailGeneratePersistentSizesOnly) == 0)
        {
        if(params.iThumbnailSize == EFullScreenThumbnailSize ||
           params.iThumbnailSize == EGridThumbnailSize ||
           params.iThumbnailSize == EListThumbnailSize )
           {
           TInt sourceType = TThumbnailPersistentSize::EUnknownSourceType;
           TDataType mimetype;
           TInt ret = Server()->MimeTypeFromFileExt( params.iTargetUri, mimetype );
           
           if( ret == KErrNone )
               {
               sourceType = Server()->SourceTypeFromMimeType( mimetype );   
               ModifyThumbnailSize(sourceType);
               }
           User::LeaveIfError( ret );
           }
        }
    
    TInt bufferSize = aMessage.Int2();
    HBufC8* buffer = HBufC8::NewMaxLC( bufferSize );
    TPtr8 ptr = buffer->Des();
    aMessage.ReadL( 1 /* buffer pointer */, ptr );
    
    CreateGenerateTaskFromBufferL( buffer );
    CleanupStack::Pop( buffer );
    }

void CThumbnailServerSession::RequestSetThumbnailByBitmapL( const RMessage2& aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::RequestSetThumbnailByBitmapL()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBITMAPL, "CThumbnailServerSession::RequestSetThumbnailByBitmapL" );

    if(aMessage.Int2() != KCheckValue)
        {
        TN_DEBUG1( "CThumbnailServerSession::RequestSetThumbnailByBitmapL() - error in aMessage - leaving" );
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBITMAPL, "CThumbnailServerSession::RequestSetThumbnailByBitmapL - error in aMessage - leaving" );
        User::Leave(KErrArgument);
        }
        
    aMessage.ReadL( 0, iRequestParams );
    const TThumbnailRequestParams& params = iRequestParams();
    
    const TThumbnailServerRequestId reqId( this, params.iRequestId );
    
    // delete existing
    if(params.iOverwrite)
        {
        Server()->DeleteThumbnailsL( params.iTargetUri);
        }
    
    // if only one size
    if ((params.iControlFlags & EThumbnailGeneratePersistentSizesOnly) == 0)
        {
        if(params.iThumbnailSize == EFullScreenThumbnailSize ||
           params.iThumbnailSize == EGridThumbnailSize ||
           params.iThumbnailSize == EListThumbnailSize )
           {
           TInt sourceType = TThumbnailPersistentSize::EUnknownSourceType;
           TDataType mimetype;
           TInt ret = Server()->MimeTypeFromFileExt( params.iTargetUri, mimetype );
           
           if( ret == KErrNone )
               {
               sourceType = Server()->SourceTypeFromMimeType( mimetype );   
               ModifyThumbnailSize(sourceType);
               }
           User::LeaveIfError( ret );
           }
        }
    
    TInt bitmapHandle = aMessage.Int1();
    
    // get bitmap
    CFbsBitmap* bitmap = new( ELeave )CFbsBitmap();
    CleanupStack::PushL( bitmap );
    User::LeaveIfError( bitmap->Duplicate( bitmapHandle ) );

    // use pool to prevent bitmap leak
    // this bitmap is shared to several scale tasks, one of which can Leave
    Server()->AddBitmapToPoolL( this, bitmap, reqId );
    
    CleanupStack::Pop( bitmap );
    iBitmapHandle = bitmap->Handle();
    
    RArray < TThumbnailPersistentSize >* missingSizes = NULL;
    
    // source type
    TDataType mimeType;
    TInt sourceType = 0;
    TInt err = Server()->MimeTypeFromFileExt( params.iTargetUri, mimeType );
    
    // get missing sizes
    if ( err == KErrNone && ( params.iControlFlags & EThumbnailGeneratePersistentSizesOnly ) != 0 )
        {
        sourceType = Server()->SourceTypeFromMimeType( mimeType );
        
        missingSizes = new (ELeave) RArray < TThumbnailPersistentSize >;
        CleanupClosePushL( *missingSizes );
    
        Server()->GetMissingSizesL( params.iTargetUri, sourceType, *missingSizes, EFalse);
            
        if ( missingSizes->Count() == 0)
            {
            // all thumbs already exist
            CleanupStack::PopAndDestroy( missingSizes );
            delete missingSizes;
            missingSizes = NULL;
            }            
        }
    
    // if missing sizes, create scale tasks
    if ( missingSizes )
        {
        const TInt count = missingSizes->Count();
        
        TSize bitmapSize = bitmap->SizeInPixels();
        
        // scale small thumbs first, because fullscreen encoding takes longer
        for ( TInt i( count-1 ); i >= 0; i-- )
            {           
            if( bitmapSize.iWidth < bitmapSize.iHeight )
               {
               TThumbnailSize size = (*missingSizes)[ i ].iType;
        
               if ( size == EFullScreenThumbnailSize ||
                    size == EVideoFullScreenThumbnailSize ||
                    size == EAudioFullScreenThumbnailSize ||
                    size == EImageFullScreenThumbnailSize ||
                    size == EContactFullScreenThumbnailSize )
                   {
                   TInt height = (*missingSizes)[i].iSize.iHeight;
                   (*missingSizes)[i].iSize.iHeight = (*missingSizes)[i].iSize.iWidth;
                   (*missingSizes)[i].iSize.iWidth = height;
                    
                   TN_DEBUG1( "CThumbnailServerSession::RequestSetThumbnailByBitmapL() - portrait");
                   OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_REQUESTSETTHUMBNAILBYBITMAPL, "CThumbnailServerSession::RequestSetThumbnailByBitmapL - portrait" );
                   }
               }
        
            CThumbnailScaleTask* scaleTask = CThumbnailScaleTask::NewL( Server()->Processor(),
                    *Server(), params.iTargetUri, bitmap, bitmapSize,
                    (*missingSizes)[i].iSize, (*missingSizes)[i].iCrop, params.iDisplayMode,
                    KMaxPriority, KNullDesC, (*missingSizes)[i].iType, params.iModified, EFalse, EFalse,
                    reqId, params.iVirtualUri);
            
            CleanupStack::PushL( scaleTask );
            scaleTask->SetDoStore( ETrue );
            Server()->Processor().AddTaskL( scaleTask );
            CleanupStack::Pop( scaleTask );
        
            // completion to first task, because task processor works like stack
            if( i == 0 )
                {            
                // scaleTask is now responsible for completing the RMessage
                scaleTask->SetMessageData( reqId, iMessage, iClientThread );
                iMessage = RMessage2();
                }
            }
        }
    else
        {
        // complete message
        aMessage.Complete( KErrNone );
        iMessage = RMessage2();
        }
    
    if ( missingSizes )
        {
        CleanupStack::PopAndDestroy( missingSizes );
        delete missingSizes;
        missingSizes = NULL;
        }
    
    // Scale tasks now reference the bitmap in the pool
    Server()->DeleteBitmapFromPool( iBitmapHandle );
    iBitmapHandle = 0;
    bitmap = NULL;
    }
 
// -----------------------------------------------------------------------------
// CThumbnailServerSession::CreateGenerateTaskL()
// Create a task to generate a new thumbnail
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::CreateGenerateTaskFromFileHandleL( RFile64* aFile)
    {
    TThumbnailRequestParams& params = iRequestParams();

    TN_DEBUG2( 
        "CThumbnailServerSession::CreateGenerateTaskFromFileHandleL() -- create thumbnail generation task for %S", &params.iFileName );
    OstTraceExt1( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMFILEHANDLEL, "CThumbnailServerSession::CreateGenerateTaskFromFileHandleL -- create thumbnail generation task for;params.iFileName=%S", params.iFileName );
    
    // disk space check only for stored sizes
    if ( params.iImport && 
         Server()->StoreForPathL(params.iTargetUri)->IsDiskFull() )
        {
        User::Leave( KErrDiskFull );
        }
    else if ( params.iThumbnailSize != ECustomThumbnailSize && 
         Server()->StoreForPathL(params.iFileName)->IsDiskFull() )
        {
        User::Leave( KErrDiskFull );
        }
    
    RArray < TThumbnailPersistentSize >* missingSizes = NULL;
    
    // get missing sizes
    if ( ( params.iControlFlags & EThumbnailGeneratePersistentSizesOnly ) != 0 )
        {
        TInt sourceType = TThumbnailPersistentSize::EUnknownSourceType;
        sourceType = Server()->SourceTypeFromMimeType( params.iMimeType );
        missingSizes = new (ELeave) RArray < TThumbnailPersistentSize >;
        CleanupClosePushL( *missingSizes );
		
	    TBool gridSizeOnly(EFalse);
            
	    if ( params.iQualityPreference == CThumbnailManager::EOptimizeForQualityWithPreview )
	        {
	        gridSizeOnly = ETrue;
	        }
        
	    // import vs. normal
	    if(params.iImport)
	        {
            Server()->GetMissingSizesL( params.iTargetUri, sourceType, *missingSizes, gridSizeOnly);
	        }
	    else
	        {
            Server()->GetMissingSizesL( params.iFileName, sourceType, *missingSizes, gridSizeOnly);
	        }     
        
        if ( missingSizes->Count() == 0)
            {
            // all thumbs already exist
            CleanupStack::PopAndDestroy( missingSizes );
            delete missingSizes;
            missingSizes = NULL;
            
            if( aFile )
               {
               aFile->Close();
               }
            return;
            }            
        }
    // creating single TN on demand
    else if( params.iThumbnailSize > ECustomThumbnailSize && params.iThumbnailSize  < EThumbnailSizeCount)
        {
        TThumbnailPersistentSize persistentSize = Server()->PersistentSizeL(params.iThumbnailSize);
        
        if(persistentSize.iCrop)
            {
            params.iFlags = ( CThumbnailManager::TThumbnailFlags ) (params.iFlags | CThumbnailManager::ECropToAspectRatio);
            }
        else
            {
            params.iFlags = ( CThumbnailManager::TThumbnailFlags ) (params.iFlags & CThumbnailManager::ECropToAspectRatio);
            }
        
        if( ClientThreadAlive() )
            {
            iMessage.Write( 0, iRequestParams );
            }
        }
    
    // priority
    TInt priority = params.iPriority;
    if ( priority > KMaxGeneratePriority )
        {
        priority = KMaxGeneratePriority;
        }
    
    // create new task
    if( !aFile)
        {
        TN_DEBUG1("CThumbnailServerSession::CreateGenerateTaskFromFileHandleL() - KErrArgument");
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMFILEHANDLEL, "CThumbnailServerSession::CreateGenerateTaskFromFileHandleL - KErrArgument" );
        User::Leave( KErrArgument );
        }
    CleanupClosePushL( *aFile );
    CThumbnailGenerateTask* task = new( ELeave )CThumbnailGenerateTask( Server()
        ->Processor(), * Server(), aFile, NULL, &params.iMimeType, params.iFlags,
        params.iSize, params.iDisplayMode, priority, missingSizes, params.iTargetUri,
        params.iThumbnailSize, params.iModified, params.iQualityPreference,
        params.iVirtualUri);

    // do not store bitmaps to server pool when generating only
    if( params.iControlFlags & EThumbnailGeneratePersistentSizesOnly )
        {
        task->ScaledBitmapToPool( EFalse );
        }
        
    CleanupStack::Pop( aFile );
    
    CleanupStack::PushL( task );
    task->SetMessageData( TThumbnailServerRequestId( this, params.iRequestId ),iMessage, iClientThread );
    Server()->QueueTaskL( task );
    CleanupStack::Pop( task ); // owned by processor now
    
    if ( missingSizes )
        {
        CleanupStack::Pop( missingSizes );
        }

    // Generate task is now responsible for completing the message
    iMessage = RMessage2();
    } 

// -----------------------------------------------------------------------------
// CThumbnailServerSession::CreateGenerateTaskL()
// Create a task to generate a new thumbnail
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::CreateGenerateTaskFromBufferL( TDesC8* aBuffer )
    {
    const TThumbnailRequestParams& params = iRequestParams();

    TN_DEBUG2( 
        "CThumbnailServerSession::CreateGenerateTaskFromBufferL() -- create thumbnail generation task for %S", &params.iTargetUri );
    OstTraceExt1( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMBUFFERL, "CThumbnailServerSession::CreateGenerateTaskFromBufferL -  -- create thumbnail generation task for;params.iTargetUri=%S", params.iTargetUri );
  
    // disk space check only for stored sizes
    if ( params.iThumbnailSize != ECustomThumbnailSize && 
         Server()->StoreForPathL(params.iTargetUri)->IsDiskFull() )
        {
        User::Leave( KErrDiskFull );
        }
    
    if(aBuffer && params.iMimeType.Des().Match( KVideoMime ) == 0 )
        {
        User::Leave( KErrNotSupported );
        }
    
    TInt sourceType = TThumbnailPersistentSize::EUnknownSourceType;
    
    TDataType mimetype;
    TInt ret = Server()->MimeTypeFromFileExt( params.iTargetUri, mimetype );
    if(ret == KErrNotFound)
        {
        Server()->Fs().ShareProtected();
        RFile64 file;
        CleanupClosePushL( file );
        
        User::LeaveIfError( file.Open( Server()->Fs(), params.iTargetUri, EFileShareReadersOrWriters )); 
        TN_DEBUG2( "CThumbnailServerSession::CreateGenerateTaskFromBufferL - file handle opened for %S", &params.iFileName );
                OstTraceExt1( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMBUFFERL, "CThumbnailServerSession::CreateGenerateTaskFromBufferL;params.iFileName=%S", params.iFileName );
                
        mimetype = Server()->ResolveMimeTypeL(file);
                
        file.Close();
        TN_DEBUG1("CThumbnailServerSession::CreateGenerateTaskFromBufferL - file handle closed");
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMBUFFERL, "CThumbnailServerSession::CreateGenerateTaskFromBufferL - file handle closed" );
        
        CleanupStack::Pop( &file );    
        }
        
    sourceType = Server()->SourceTypeFromMimeType( mimetype );    
    
    RArray < TThumbnailPersistentSize >* missingSizes = NULL;
    
    // get missing sizes
    if ( ( params.iControlFlags & EThumbnailGeneratePersistentSizesOnly ) != 0 )
        {
        missingSizes = new (ELeave) RArray < TThumbnailPersistentSize >;
        CleanupClosePushL( *missingSizes );
        
        Server()->GetMissingSizesL( params.iTargetUri, sourceType, *missingSizes, EFalse );
            
        if ( missingSizes->Count() == 0)
            {
            // all thumbs already exist
            CleanupStack::PopAndDestroy( missingSizes );
            delete missingSizes;
            missingSizes = NULL;
            
            if ( aBuffer)
               {
               delete aBuffer;
               aBuffer = NULL; 
               }
            return;
            }            
        }
    
    // priority
    TInt priority = params.iPriority;
    if ( priority > KMaxGeneratePriority )
        {
        priority = KMaxGeneratePriority;
        }
    
    // create new task
    if( !aBuffer)
        {
        TN_DEBUG1( "CThumbnailServerSession::UpdateThumbnailsL() - KErrArgument" );
        OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILSERVERSESSION_CREATEGENERATETASKFROMBUFFERL, "CThumbnailServerSession::CreateGenerateTaskFromBufferL - KErrArgument" );
        User::Leave( KErrArgument );
        }
    
    CThumbnailGenerateTask* task = new( ELeave )CThumbnailGenerateTask( Server()
        ->Processor(), * Server(), NULL, aBuffer, &params.iMimeType, params.iFlags,
        params.iSize, params.iDisplayMode, priority, missingSizes, params.iTargetUri,
        params.iThumbnailSize, params.iModified, params.iQualityPreference,
        params.iVirtualUri);

    // do not store bitmaps to server pool when generating only
    if( params.iControlFlags & EThumbnailGeneratePersistentSizesOnly )
        {
        task->ScaledBitmapToPool( EFalse );
        }  
    
    CleanupStack::PushL( task );
    task->SetMessageData( TThumbnailServerRequestId( this, params.iRequestId ),iMessage, iClientThread );
    Server()->QueueTaskL( task );
    CleanupStack::Pop( task ); // owned by processor now
    
    if ( missingSizes )
        {
        CleanupStack::Pop( missingSizes );
        }

    // Generate task is now responsible for completing the message
    iMessage = RMessage2();
    } 
// -----------------------------------------------------------------------------
// CThumbnailServerSession::FetchThumbnailL()
// Fetch thumbnail data from database
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::FetchThumbnailL()
    {
    TN_DEBUG1("CThumbnailServerSession::FetchThumbnailL()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_FETCHTHUMBNAILL, "CThumbnailServerSession::FetchThumbnailL" );
    __ASSERT_DEBUG( !iBitmap, ThumbnailPanic( EThumbnailBitmapNotReleased ));
    __ASSERT_DEBUG( !iBuffer, ThumbnailPanic( EThumbnailBitmapNotReleased ));

    delete iBitmap;
    iBitmap = NULL;
    delete iBuffer;
    iBuffer = NULL;

    TThumbnailRequestParams& params = iRequestParams();
    
    if ( params.iThumbnailSize != EUnknownThumbnailSize &&
         params.iThumbnailSize != ECustomThumbnailSize )
        {
        TThumbnailPersistentSize & persistentSize = Server()->PersistentSizeL( params.iThumbnailSize );
        params.iSize = persistentSize.iSize;
        }
    
    if( params.iFileName != KNullDesC )
        {
        TN_DEBUG3( "CThumbnailServerSession::FetchThumbnailL( ThumbnailSize=%d ( Path=%S ))", 
                 params.iThumbnailSize, &params.iFileName );
        OstTraceExt1( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_FETCHTHUMBNAILL, "CThumbnailServerSession::FetchThumbnailL;params.iFileName=%S", params.iFileName );
        Server()->FetchThumbnailL( params.iFileName, iBitmap, iBuffer, params.iThumbnailSize, iOriginalSize);
        }
    else
        {
        User::Leave( KErrNotSupported );
        }
#ifdef _DEBUG
    if( iBitmap)
        {
        TN_DEBUG4( "CThumbnailServerSession::FetchThumbnailL() size %d x %d displaymode %d", iBitmap->SizeInPixels().iWidth, iBitmap->SizeInPixels().iHeight, iBitmap->DisplayMode());
        OstTraceExt3( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_FETCHTHUMBNAILL, "CThumbnailServerSession::FetchThumbnailL;iBitmap->SizeInPixels().iWidth=%d;iBitmap->SizeInPixels().iHeight=%d;iBitmap->DisplayMode()=%u", iBitmap->SizeInPixels().iWidth, iBitmap->SizeInPixels().iHeight, iBitmap->DisplayMode() );
        }
#endif
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::ProcessBitmapL()
// Process a fetched bitmap by creating scale tasks or by returning the
// bitmap as such.
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::ProcessBitmapL()
    {   
    TThumbnailRequestParams& params = iRequestParams();
    
    if ( ClientThreadAlive() )
        {        
        TN_DEBUG2("CThumbnailServerSession::ProcessBitmapL(), iBitmap handle= 0x%08x", iBitmap->Handle());
        OstTrace1( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_PROCESSBITMAPL, "CThumbnailServerSession::ProcessBitmapL;iBitmap->Handle()=%o", iBitmap->Handle() );
        
        params.iBitmapHandle = iBitmap->Handle();
        const TSize bitmapSize = iBitmap->SizeInPixels();
        
        if ( params.iQualityPreference == CThumbnailManager
            ::EOptimizeForQualityWithPreview && bitmapSize.iWidth <
            params.iSize.iWidth && bitmapSize.iHeight < params.iSize.iHeight &&
            bitmapSize.iWidth < iOriginalSize.iWidth && bitmapSize.iHeight <
            iOriginalSize.iHeight )
            {
            // This is a non-scaled preview bitmap
            params.iControlFlags = EThumbnailPreviewThumbnail;
            }

        iMessage.WriteL( 0, iRequestParams );
        
        TN_DEBUG1("CThumbnailServerSession()::ProcessBitmapL() bitmap to pool");
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_PROCESSBITMAPL, "CThumbnailServerSession::ProcessBitmapL - bitmap to pool" );
        
        Server()->AddBitmapToPoolL( this, iBitmap, TThumbnailServerRequestId( this, params.iRequestId ) );
        
        iMessage.Complete( KErrNone );
        iMessage = RMessage2();
        
        iBitmap = NULL; // owned by server now
        }            
    else
        {
        delete iBitmap;
        iBitmap = NULL;
        }
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::ReleaseBitmap()
// Release bitmap from bitmap pool
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::ReleaseBitmap( const RMessage2& aMessage )
    {
    TN_DEBUG2( "CThumbnailServerSession::ReleaseBitmap(%d)", aMessage.Int0());
    OstTrace1( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_RELEASEBITMAP, "CThumbnailServerSession::ReleaseBitmap;aMessage.Int0()=%d", aMessage.Int0() );
    Server()->DeleteBitmapFromPool( aMessage.Int0());
    }


// -----------------------------------------------------------------------------
// CThumbnailServerSession::CancelRequest()
// Cancel pending request.
// -----------------------------------------------------------------------------
//
TInt CThumbnailServerSession::CancelRequest( const RMessage2& aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::CancelRequest()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_CANCELREQUEST, "CThumbnailServerSession::CancelRequest" );
    
    const TThumbnailServerRequestId requestId( this, aMessage.Int0());
    const TInt err = Server()->DequeTask( requestId );
    TN_DEBUG4( 
        "CThumbnailServerSession::CancelRequest(0x%08x/%d) - returning %d",
        requestId.iSession, requestId.iRequestId, err );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_CANCELREQUEST, "CThumbnailServerSession::CancelRequest;requestId.iSession=%o", requestId.iSession );
    OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_CANCELREQUEST, "CThumbnailServerSession::CancelRequest;err=%d", err );
    
    return err;
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::ChangePriority()
// Change priority of pending request.
// -----------------------------------------------------------------------------
//
TInt CThumbnailServerSession::ChangePriority( const RMessage2& aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::ChangePriority()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_CHANGEPRIORITY, "CThumbnailServerSession::ChangePriority" );
    
    const TThumbnailServerRequestId requestId( this, aMessage.Int0());
    const TInt newPriority = aMessage.Int1();

    const TInt err = Server()->ChangeTaskPriority( requestId, newPriority );
    TN_DEBUG5( 
        "CThumbnailServerSession::ChangePriority(0x%08x/%d, %d) - returning %d",
        requestId.iSession, requestId.iRequestId, newPriority, err );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_CHANGEPRIORITY, "CThumbnailServerSession::ChangePriority;requestId.iSession=%o", requestId.iSession );
    OstTraceExt2( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_CHANGEPRIORITY, "CThumbnailServerSession::ChangePriority;requestId.iRequestId=%u;err=%d", requestId.iRequestId, err );
    
    return err;
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::DeleteThumbnailsL()
// Delete thumbnails for given object file
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::DeleteThumbnailsL( const RMessage2& aMessage )
    {
    if(aMessage.Int2() != KCheckValue)
       {
       TN_DEBUG1( "CThumbnailServerSession::DeleteThumbnailsL() - error in aMessage - leaving" );
       OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_DELETETHUMBNAILSL, "CThumbnailServerSession::DeleteThumbnailsL - error in aMessage - leaving" );
       User::Leave(KErrArgument);
       }
    
    HBufC* fileName = HBufC::NewLC( KMaxFileName );
    TPtr ptr = fileName->Des();
    aMessage.ReadL( 1, ptr );
    Server()->DeleteThumbnailsL( ptr );
    CleanupStack::PopAndDestroy( fileName );
    
    aMessage.Complete( KErrNone );
    iMessage = RMessage2();
    }

// -----------------------------------------------------------------------------
// CThumbnailServerSession::DeleteThumbnailsByIdL()
// Delete thumbnails by TThumbnailId.
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::DeleteThumbnailsByIdL( const RMessage2& aMessage )
    {
    if(aMessage.Int2() != KCheckValue)
       {
       TN_DEBUG1( "CThumbnailServerSession::DeleteThumbnailsByIdL() - error in aMessage - leaving" );
       OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_DELETETHUMBNAILSBYIDL, "CThumbnailServerSession::DeleteThumbnailsByIdL - error in aMessage - leaving" );
       User::Leave(KErrArgument);
       }
    
    // read message params
    aMessage.ReadL( 0, iRequestParams );
    const TThumbnailRequestParams& params = iRequestParams();
    
#ifdef RD_MDS_2_5        
    // try to query path from MDS
    CThumbnailMDSQueryTask* task = new( ELeave )CThumbnailMDSQueryTask(
            Server()->Processor(), params.iPriority + 1, Server()->GetMdESession(), *Server());
    
    CleanupStack::PushL( task );
    task->QueryPathByIdL(params.iThumbnailId, ETrue);
    task->SetMessageData( TThumbnailServerRequestId( this, params.iRequestId ) );
    Server()->QueueTaskL( task );
    CleanupStack::Pop( task ); // owned by processor now
#endif // RD_MDS_2_5
    
    aMessage.Complete( KErrNone );
    iMessage = RMessage2();
    }

// -----------------------------------------------------------------------------
// Get the required size (in characters) for a buffer that contains the
// list of supported MIME types
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::GetMimeTypeBufferSizeL( const RMessage2& aMessage )
    {
    TPckgBuf < TInt > buf;
    buf() = Server()->GetMimeTypeBufferSize();
    aMessage.WriteL( 0, buf );
    }

// -----------------------------------------------------------------------------
// Get the list of supported MIME types and store them in the buffer
// allocated by the client.
// -----------------------------------------------------------------------------
//
void CThumbnailServerSession::GetMimeTypeListL( const RMessage2& aMessage )
    {
    TN_DEBUG1( "CThumbnailServerSession::GetMimeTypeListL()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_GETMIMETYPELISTL, "CThumbnailServerSession::GetMimeTypeListL" );
    
    TInt len = aMessage.GetDesMaxLengthL( 0 );
    HBufC* buf = HBufC::NewLC( len );
    TPtr ptr = buf->Des();
    Server()->GetMimeTypeList( ptr );
    aMessage.WriteL( 0, * buf );
    CleanupStack::PopAndDestroy( buf );
    }

// ---------------------------------------------------------------------------
// CThumbnailServerSession::ModifyThumbnailSize
// ---------------------------------------------------------------------------
//
void CThumbnailServerSession::ModifyThumbnailSize( TInt aSourceType )   
    {
    TThumbnailRequestParams& params = iRequestParams();
    if(aSourceType == TThumbnailPersistentSize::EImage)
        {
        if(params.iThumbnailSize == EFullScreenThumbnailSize)
            {
            params.iThumbnailSize = EImageFullScreenThumbnailSize;
            }
        else if(params.iThumbnailSize == EGridThumbnailSize)
            {
            params.iThumbnailSize = EImageGridThumbnailSize;
            }
        else if(params.iThumbnailSize == EListThumbnailSize)
            {
            params.iThumbnailSize = EImageListThumbnailSize;
            }       
        }
    else if(aSourceType == TThumbnailPersistentSize::EVideo)
           {
           if(params.iThumbnailSize == EFullScreenThumbnailSize)
               {
               params.iThumbnailSize = EVideoFullScreenThumbnailSize;
               }
           else if(params.iThumbnailSize == EGridThumbnailSize)
               {
               params.iThumbnailSize = EVideoGridThumbnailSize;
               }
           else if(params.iThumbnailSize == EListThumbnailSize)
               {
               params.iThumbnailSize = EVideoListThumbnailSize;
               }       
           }
    else if(aSourceType == TThumbnailPersistentSize::EAudio)
           {
           if(params.iThumbnailSize == EFullScreenThumbnailSize)
               {
               params.iThumbnailSize = EAudioFullScreenThumbnailSize;
               }
           else if(params.iThumbnailSize == EGridThumbnailSize)
               {
               params.iThumbnailSize = EAudioGridThumbnailSize;
               }
           else if(params.iThumbnailSize == EListThumbnailSize)
               {
               params.iThumbnailSize = EAudioListThumbnailSize;
               }       
           }
    else if(aSourceType == TThumbnailPersistentSize::EContact)
           {
           if(params.iThumbnailSize == EFullScreenThumbnailSize)
               {
               params.iThumbnailSize = EContactFullScreenThumbnailSize;
               }
           else if(params.iThumbnailSize == EGridThumbnailSize)
               {
               params.iThumbnailSize = EContactGridThumbnailSize;
               }
           else if(params.iThumbnailSize == EListThumbnailSize)
               {
               params.iThumbnailSize = EContactListThumbnailSize;
               }       
           }
    }

//------------------------------------------------------------------------
// CThumbnailServerSession::ModifyThumbnailSize
// ---------------------------------------------------------------------------
//
void CThumbnailServerSession::ResolveMimeTypeL( RFile64* aFile )
    { 
    TThumbnailRequestParams& params = iRequestParams();
    TInt res = 0;
        
    // mime type
    if ( params.iMimeType.Des8() == KNullDesC8 && !Server()->SupportedMimeType(params.iMimeType) )
        {
        // try parsing from file extension
        if (params.iImport)
            {
            res = Server()->MimeTypeFromFileExt( params.iTargetUri, params.iMimeType );
            }
        else
            {
            res = Server()->MimeTypeFromFileExt( params.iFileName, params.iMimeType );
            }
        
        if ( res == KErrNotFound )
            {
            if( aFile )
                {
                // parsed type not in the list, resolve from file
                params.iMimeType = Server()->ResolveMimeTypeL(*aFile);
                }
            else
                {
                Server()->Fs().ShareProtected();
                RFile64 file;
                CleanupClosePushL( file );
              
                User::LeaveIfError( file.Open( Server()->Fs(), params.iFileName, EFileShareReadersOrWriters )); 
                TN_DEBUG2( "CThumbnailServerSession::ResolveMimeType - file handle opened for %S", &params.iFileName );
                OstTraceExt1( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_RESOLVEMIMETYPEL, "CThumbnailServerSession::ResolveMimeTypeL - file handle opened;params.iFileName=%S", params.iFileName );
              
                params.iMimeType = Server()->ResolveMimeTypeL(file);
              
                file.Close();
                TN_DEBUG1("CThumbnailServerSession::ResolveMimeType - file handle closed");
                OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_RESOLVEMIMETYPEL, "CThumbnailServerSession::ResolveMimeTypeL - file handle closed" );
              
                CleanupStack::Pop( &file );    
                }    
            }        
        }       
    }


// ---------------------------------------------------------------------------
// RThumbnailMessage::FilterSqlErr
// ---------------------------------------------------------------------------
//
TInt CThumbnailServerSession::ConvertSqlErrToE32Err( TInt aReason )
    {
    TN_DEBUG2("CThumbnailServerSession::ConvertSqlErrToE32Err(%d)", aReason);
    OstTrace1( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_CONVERTSQLERRTOE32ERR, "CThumbnailServerSession::ConvertSqlErrToE32Err;aReason=%d", aReason );
    TInt e32Err(aReason);
	
    if ( aReason >=  - 144 )
    // magic: [-1..-144] is E32 error range 
        {
        // E32 error value or non-negative value
        switch ( aReason )
            {
            case KErrServerTerminated:
                e32Err = KErrCorrupt;
                break;
            default:
                e32Err = aReason;
            }
        }
    else
        {
        switch ( aReason )
            {
            case KSqlErrGeneral:
                e32Err = KErrGeneral;
                break;
            case KSqlErrInternal:
                e32Err = KErrGeneral;
                break;
            case KSqlErrPermission:
                e32Err = KErrPermissionDenied;
                break;
            case KSqlErrAbort:
                e32Err = KErrAbort;
                break;
            case KSqlErrBusy:
                e32Err = KErrServerBusy;
                break;
            case KSqlErrLocked:
                e32Err = KErrLocked;
                break;
            case KSqlErrReadOnly:
                e32Err = KErrAccessDenied;
                break;
            case KSqlErrInterrupt:
                e32Err = KErrAbort;
                break;
            case KSqlErrIO:
                e32Err = KErrGeneral;
                break;
            case KSqlErrCorrupt:
                e32Err = KErrCorrupt;
                break;
            case KSqlErrNotFound:
                e32Err = KErrNotFound;
                break;
            case KSqlErrFull:
                e32Err = KErrOverflow;
                break;
            case KSqlErrCantOpen:
                e32Err = KErrCouldNotConnect;
                break;
            case KSqlErrProtocol:
                e32Err = KErrLocked;
                break;
            case KSqlErrEmpty:
                e32Err = KErrNotFound;
                break;
            case KSqlErrSchema:
                e32Err = KErrAbort;
                break;
            case KSqlErrTooBig:
                e32Err = KErrTooBig;
                break;
            case KSqlErrConstraint:
                e32Err = KErrGeneral;
                break;
            case KSqlErrMismatch:
                e32Err = KErrGeneral;
                break;
            case KSqlErrMisuse:
                e32Err = KErrGeneral;
                break;
            case KSqlErrRange:
                e32Err = KErrOverflow;
                break;
            case KSqlErrNotDb:
                e32Err = KErrCorrupt;
                break;
            case KSqlErrStmtExpired:
                e32Err = KErrAbort;
                break;
            default:
                e32Err = aReason;
            }
        }
    return e32Err;
    }

// ---------------------------------------------------------------------------
// CThumbnailServerSession::ClientThreadAlive()
// Checks if client thread is still alive and RMessage2 handle valid.
// ---------------------------------------------------------------------------
//
TBool CThumbnailServerSession::ClientThreadAlive()
    {
    TN_DEBUG1( "CThumbnailServerSession::ClientThreadAlive()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILSERVERSESSION_CLIENTTHREADALIVE, "CThumbnailServerSession::ClientThreadAlive" );
    
    if ( iMessage.Handle())
        {
        // check if client thread alive
        TExitType exitType = iClientThread.ExitType();
        
        if( exitType != EExitPending )
            {
            TN_DEBUG1( "CThumbnailServerSession::ClientThreadAlive() - client thread died");
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILSERVERSESSION_CLIENTTHREADALIVE, "CThumbnailServerSession::ClientThreadAlive - client thread died" );
        
            iMessage = RMessage2();
            
            return EFalse;
            }
        else
            {
            // all OK
            return ETrue;
            }
        }
    else
        {
        TN_DEBUG1( "CThumbnailServerSession::ClientThreadAlive() - message null");       
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILSERVERSESSION_CLIENTTHREADALIVE, "CThumbnailServerSession::ClientThreadAlive - message null" );
        return EFalse;
        }
    }


// End of file
