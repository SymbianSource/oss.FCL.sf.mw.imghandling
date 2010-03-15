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
* Description:  Thumbnail server
 *
*/


#include <e32svr.h>
#include <MIHLScaler.h>
#include <driveinfo.h>
#include <caf/data.h>
#include <Oma2Agent.h>
#include <bautils.h>  
#include <mdesession.h>

#include "thumbnailserver.h"
#include "thumbnailtaskprocessor.h"
#include "thumbnailserversession.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanageruids.hrh"
#include "thumbnaillog.h"
#include "thumbnailstore.h"
#include "thumbnaildiskunmountobserver.h"
#include "thumbnailpanic.h"
#include "thumbnailcenrep.h"
#include "thumbnailmemorycardobserver.h"
#include "tmgetimei.h"
//#include "thumbnailfetchedchecker.h"


_LIT8( KThumbnailMimeWildCard, "*" );
_LIT8( KThumbnailMimeImage, "image" );
_LIT8( KThumbnailMimeVideo, "video" );
_LIT8( KThumbnailMimeAudio, "audio" );

const TChar KThumbnailMimeSeparatorChar = '/';
const TChar KThumbnailMimeWildCardChar = '*';
const TChar KThumbnailMimeTypeSeparatorChar = ' ';

// ----------------------------------------------------------------------------------------
// Server's policy here
// ----------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
// Total number of ranges
// ----------------------------------------------------------------------------------------
const TUint KThumbnailServerRangeCount = 16;
 
// ----------------------------------------------------------------------------------------
// Definition of the ranges
// ----------------------------------------------------------------------------------------
const TInt KThumbnailServerRanges[KThumbnailServerRangeCount] = 
{
    ERequestThumbByPathAsync,      
    ERequestThumbByFileHandleAsync,          
    EReleaseBitmap,
    ECancelRequest,
    EChangePriority,
    EDeleteThumbnails,
    EGetMimeTypeBufferSize,
    EGetMimeTypeList,
    ERequestThumbByIdAsync,
    ERequestThumbByBufferAsync,
    ERequestSetThumbnailByBuffer,
    EDeleteThumbnailsById,
    EReserved1,
    EUpdateThumbnails,
    ERequestSetThumbnailByBitmap,
    EThumbnailServerRequestCount,
};

// ----------------------------------------------------------------------------------------
// Policy to implement for each of the above ranges 
// ----------------------------------------------------------------------------------------      
const TUint8 KThumbnailServerElementsIndex[KThumbnailServerRangeCount] = 
    {
    CPolicyServer::ECustomCheck,    // ERequestThumbByPathAsync
    CPolicyServer::ECustomCheck,    // ERequestThumbByFileHandleAsync
    CPolicyServer::ECustomCheck,    // EReleaseBitmap
    CPolicyServer::ECustomCheck,    // ECancelRequest
    CPolicyServer::ECustomCheck,    // EChangePriority
    CPolicyServer::ECustomCheck,    // EDeleteThumbnails
    CPolicyServer::ECustomCheck,    // EGetMimeTypeBufferSize
    CPolicyServer::ECustomCheck,    // EGetMimeTypeList
    CPolicyServer::ECustomCheck,    // ERequestThumbByIdAsync
    CPolicyServer::ECustomCheck,    // ERequestThumbByBufferAsync
    CPolicyServer::ECustomCheck,    // ERequestSetThumbnailByBuffer
    CPolicyServer::ECustomCheck,    // EDeleteThumbnailsById
    CPolicyServer::ECustomCheck,    
    CPolicyServer::ECustomCheck,    // EUpdateThumbnails
    CPolicyServer::ECustomCheck,    // ERequestSetThumbnailByBitmap
    CPolicyServer::ECustomCheck,    // EThumbnailServerRequestCount
    };

// ----------------------------------------------------------------------------------------
// Package all the above together into a policy 
// ---------------------------------------------------------------------------------------- 
const CPolicyServer::TPolicy KThumbnailServerPolicy =
    {
    CPolicyServer::EAlwaysPass,
    KThumbnailServerRangeCount,      // number of ranges
    KThumbnailServerRanges,          // ranges array
    KThumbnailServerElementsIndex,   // elements<->ranges index
    NULL 
                               // array of elements
    };

// ---------------------------------------------------------------------------
// CustomSecurityCheckL
// ---------------------------------------------------------------------------
//
CPolicyServer::TCustomResult CThumbnailServer::CustomSecurityCheckL(
        const RMessage2& aMsg, TInt& /*aAction*/, TSecurityInfo& /*aMissing*/ )
    {
    CPolicyServer::TCustomResult securityCheckResult = EFail;
    
    switch ( aMsg.Function() )
        {
        case ERequestThumbByPathAsync:
        case ERequestThumbByFileHandleAsync:
        case ERequestThumbByIdAsync:
        case ERequestThumbByBufferAsync:
            {
            securityCheckResult = EPass;
            break;
            }
        case EReleaseBitmap:
        case ECancelRequest:
        case EChangePriority:
        case EDeleteThumbnails:
        case EGetMimeTypeBufferSize:
        case EGetMimeTypeList:
        case ERequestSetThumbnailByBuffer:
        case EDeleteThumbnailsById:
        case EUpdateThumbnails:   
        case ERequestSetThumbnailByBitmap:
            {
            if( aMsg.HasCapability( ECapabilityReadDeviceData ) && 
                aMsg.HasCapability( ECapabilityWriteDeviceData ) )
                {
                securityCheckResult = EPass;
                }
            break;
            }

        case EReserved1:
        case EThumbnailServerRequestCount:
        default:
            {
            securityCheckResult = EFail;
            }
        }
    
    return securityCheckResult;
    }
// ---------------------------------------------------------------------------
// CustomFailureActionL
// ---------------------------------------------------------------------------
//
CPolicyServer::TCustomResult CThumbnailServer::CustomFailureActionL(
        const RMessage2& /*aMsg*/, TInt /*aAction*/, const TSecurityInfo& /*aMissing*/ )
    {
    // Not used
    return EFail;
    }

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailServer::CThumbnailServer()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailServer::CThumbnailServer(): CPolicyServer( CActive::EPriorityStandard,
    KThumbnailServerPolicy, EUnsharableSessions )
    {
    // No implementation required
    }

// ---------------------------------------------------------------------------
// CThumbnailServer::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailServer* CThumbnailServer::NewL()
    {
    CThumbnailServer* self = new( ELeave )CThumbnailServer();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailServer::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailServer::ConstructL()
    {
    TN_DEBUG1( "CThumbnailServer::ConstructL()" );
    
#ifdef _DEBUG
    iPlaceholderCounter = 0;
#endif
    
    // create shutdown observer
    iShutdownObserver = CTMShutdownObserver::NewL( *this, KTMPSNotification, KShutdown, ETrue );  
    iShutdown = EFalse;
    
    // connect to MDS
    iMdESession = CMdESession::NewL( *this );
    
    User::LeaveIfError( iFbsSession.Connect());
    User::LeaveIfError( Start( KThumbnailServerName ));
    User::LeaveIfError( iFs.Connect());
    iProcessor = CThumbnailTaskProcessor::NewL();
    REComSession::FinalClose();
    REComSession::ListImplementationsL( TUid::Uid( THUMBNAIL_PROVIDER_IF_UID ),
        iPluginInfoArray );
    
    CTMGetImei * imeiGetter = CTMGetImei::NewLC();
   
    iImei = imeiGetter->GetIMEI();
    CleanupStack::PopAndDestroy(imeiGetter);
    
    iFs.CreatePrivatePath(EDriveC);
    iFs.SetSessionToPrivate(EDriveC);
    
    iCenrep = CThumbnailCenRep::NewL();
               
    iPersistentSizes = iCenrep->GetPersistentSizes();
            
    iMMCObserver = CThumbnailMemoryCardObserver::NewL( this, iFs );
    
    iFormatObserver = CTMFormatObserver::NewL( *this );
    
    iFormatting = EFalse;
    
    //OpenStoresL();
    
    AddUnmountObserversL();
    
    iReconnect = CPeriodic::NewL(CActive::EPriorityIdle);
	
    //iFetchedChecker = CThumbnailFetchedChecker::NewL();
    }


// ---------------------------------------------------------------------------
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailServer::~CThumbnailServer()
    {
    TN_DEBUG1( "CThumbnailServer::~CThumbnailServer()" );

    iShutdown = ETrue;
    
    //delete iFetchedChecker;
    delete iShutdownObserver;
    delete iProcessor;
    
    if(iReconnect)
        {
        iReconnect->Cancel();
        delete iReconnect;
        iReconnect = NULL;
        }
    
    if (iMdESession)
        {
        delete iMdESession;
        iMdESession = NULL;
        }

    ResetAndDestroyHashMap < TInt, CThumbnailStore > ( iStores );
    ResetAndDestroyHashMap < TInt32, CThumbnailProvider > ( iProviders );
    
    iUnmountObservers.ResetAndDestroy();
    delete iMMCObserver;
    delete iFormatObserver;
    
    THashMapIter < TInt, TThumbnailBitmapRef > bpiter( iBitmapPool );

    // const pointer to a non-const object
    const TThumbnailBitmapRef* ref = bpiter.NextValue();

    while ( ref )
        {
        delete ref->iBitmap;
        ref = bpiter.NextValue();
        }
    
    delete iScaler;
    iBitmapPool.Close();
    iFbsSession.Disconnect();
    iRecognizer.Close();
    iPluginInfoArray.ResetAndDestroy();
    delete iCenrep;
    iFs.Close();
    REComSession::FinalClose();
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::HandleSessionOpened
// -----------------------------------------------------------------------------
//
void CThumbnailServer::HandleSessionOpened( CMdESession& /* aSession */, TInt /*aError*/ )
    {
    TN_DEBUG1( "CThumbnailServer::HandleSessionOpened");
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::HandleSessionError
// -----------------------------------------------------------------------------
//
void CThumbnailServer::HandleSessionError( CMdESession& /*aSession*/, TInt aError )
    {
    TN_DEBUG2( "CThumbnailServer::HandleSessionError == %d", aError );
    if (aError != KErrNone && !iShutdown)
        {
        if (!iReconnect->IsActive())
            {
            iReconnect->Start( KMdEReconnect, KMdEReconnect, 
                               TCallBack(ReconnectCallBack, this));
            
            TN_DEBUG1( "CThumbnailServer::HandleSessionError() - reconnect timer started" );
            }
        } 
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::NewSessionL()
// Creates new server session.
// -----------------------------------------------------------------------------
//
CSession2* CThumbnailServer::NewSessionL( const TVersion& aVersion, const
    RMessage2&  /*aMessage*/ )const
    {
    const TVersion v( KThumbnailServerMajorVersionNumber,
        KThumbnailServerMinorVersionNumber, KThumbnailServerBuildVersionNumber )
        ;
    if ( !User::QueryVersionSupported( v, aVersion ))
        {
        User::Leave( KErrNotSupported );
        }
    return new( ELeave )CThumbnailServerSession();
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::ThreadFunctionL()
// -----------------------------------------------------------------------------
//
void CThumbnailServer::ThreadFunctionL()
    {
    // Rename own thread
    User::LeaveIfError( User::RenameThread( KThumbnailServerName ));

    CThumbnailServer* server = NULL;
    CActiveScheduler* scheduler = new( ELeave )CActiveScheduler();

    if ( scheduler )
        {
        CActiveScheduler::Install( scheduler );
        CleanupStack::PushL( scheduler );
        server = CThumbnailServer::NewL(); // Adds server in scheduler
        // No need to CleanupStack::PushL(server) since no leaves can happen
        RProcess::Rendezvous( KErrNone );
        TN_DEBUG1( 
            "CThumbnailServer::ThreadFunctionL() -- CActiveScheduler::Start() in" );
        CActiveScheduler::Start();
        TN_DEBUG1( 
            "CThumbnailServer::ThreadFunctionL() -- CActiveScheduler::Start() out" );
        // Comes here if server gets shut down
        delete server;
        CleanupStack::PopAndDestroy( scheduler );
        }
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::AddSession()
// -----------------------------------------------------------------------------
//
void CThumbnailServer::AddSession()
    {
    TN_DEBUG2( "CThumbnailServer::AddSession() iSessionCount was %d",
        iSessionCount );
    iSessionCount++;
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::DropSession()
// -----------------------------------------------------------------------------
//
void CThumbnailServer::DropSession(CThumbnailServerSession* aSession)
    {
    TN_DEBUG2( "CThumbnailServer::DropSession() iSessionCount was %d",
        iSessionCount );
    iSessionCount--;
    
    iProcessor->RemoveTasks(aSession);
    
    TN_DEBUG2( "CThumbnailServer::DropSession() aSession = 0x%08x", aSession );        
    
    // clean-up bitmap pool
    
    THashMapIter < TInt, TThumbnailBitmapRef > bpiter( iBitmapPool );

    // const pointer to a non-const object
    const TThumbnailBitmapRef* ref = bpiter.NextValue();

    while ( ref )
        {
        
        TN_DEBUG2( "CThumbnailServer::DropSession() - ref->iSession = 0x%08x", ref->iSession );
        
        if ( ref->iSession == aSession )
            {            
            delete ref->iBitmap;            
            bpiter.RemoveCurrent();
                        
            TN_DEBUG2( "CThumbnailServer::DropSession() - deleted bitmap, left=%d", 
                                iBitmapPool.Count());
            }
        ref = bpiter.NextValue();
        
        }

    if ( iSessionCount <= 0 )
        {
        // rename thread
        User::RenameThread( KThumbnailServerShutdown );
        
        // server shutdown
        if (!iShutdown)
            {
            CActiveScheduler::Stop();
            iShutdown = ETrue;
            }
        }
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::ShutdownNotification
// -----------------------------------------------------------------------------
//
void CThumbnailServer::ShutdownNotification()
    {
    if (!iShutdown)
        {
        CActiveScheduler::Stop();
        iShutdown = ETrue;
        }
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::AddBitmapToPoolL()
// Add bitmap to bitmap pool.
// -----------------------------------------------------------------------------
//
void CThumbnailServer::AddBitmapToPoolL( CThumbnailServerSession* aSession,
    CFbsBitmap* aBitmap, TThumbnailServerRequestId aRequestId )
    {
    if( !aBitmap )
        {
        TN_DEBUG1( "CThumbnailServer::AddBitmapToPoolL() - KErrArgument");
        User::Leave( KErrArgument );
        }
    TN_DEBUG4( 
        "CThumbnailServer::AddBitmapToPoolL(aSession=0x%08x, aBitmap=0x%08x), handle=%d", aSession, aBitmap, aBitmap->Handle());

    TThumbnailBitmapRef* ptr = iBitmapPool.Find( aBitmap->Handle());

    TN_DEBUG2( "CThumbnailServer::AddBitmapToPoolL() - req id = %d", aRequestId.iRequestId );
    
    if ( ptr )
        {
        ptr->iRefCount++;
        }
    else
        {
        TThumbnailBitmapRef ref;
        ref.iBitmap = aBitmap;
        ref.iSession = aSession;
        ref.iRefCount = 1; // magic: first reference        
        ref.iRequestId = aRequestId.iRequestId;               
        
        iBitmapPool.InsertL( aBitmap->Handle(), ref );
        }
    
#ifdef _DEBUG
    TN_DEBUG2( "CThumbnailServer::BITMAP-POOL-COUNTER----------, Bitmaps = %d", iBitmapPool.Count() );
#endif
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::StoreThumbnailL()
// -----------------------------------------------------------------------------
//
void CThumbnailServer::StoreThumbnailL( const TDesC& aPath, CFbsBitmap* aBitmap,
    const TSize& aOriginalSize, const TBool aCropped, const TThumbnailSize aThumbnailSize,
    const TInt64 aModified, const TBool aThumbFromPath, const TBool aCheckExist )
    {
    TN_DEBUG6( 
        "CThumbnailServer::StoreBitmapL(aPath=%S, aBitmap=0x%08x, aOriginalSize=%dx%d, aCropped=%d)", &aPath, aBitmap, aOriginalSize.iWidth, aOriginalSize.iHeight, aCropped );
#ifdef _DEBUG
    TN_DEBUG2( "CThumbnailServer::StoreThumbnailL() - iScaledBitmap displaymode is %d", aBitmap->DisplayMode());
#endif
    
    if (!aCheckExist)
        {
        StoreForPathL( aPath )->StoreThumbnailL( aPath, aBitmap, aOriginalSize,
                       aCropped, aThumbnailSize, aModified, aThumbFromPath );
        }    
    else if(BaflUtils::FileExists( iFs, aPath))
        {
        StoreForPathL( aPath )->StoreThumbnailL( aPath, aBitmap, aOriginalSize,
                       aCropped, aThumbnailSize, aModified, aThumbFromPath );
        }
    else
        {
        TN_DEBUG1( "CThumbnailServer::StoreThumbnailL() - file doesn't exists anymore, skip store!");
        }
	/*if( iFetchedChecker )
		{	
    	iFetchedChecker->SetFetchResult( aPath, KErrNone );
		}*/
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::FetchThumbnailL()
// -----------------------------------------------------------------------------
//
void CThumbnailServer::FetchThumbnailL( const TDesC& aPath, CFbsBitmap* &
    aThumbnail, TDesC8* & aData, const TThumbnailSize aThumbnailSize, TSize &aOriginalSize )
    {
    TN_DEBUG3( "CThumbnailServer::FetchThumbnailL(aPath=%S aThumbnailSize=%d)", &aPath, aThumbnailSize );
    StoreForPathL( aPath )->FetchThumbnailL( aPath, aThumbnail, aData, aThumbnailSize, aOriginalSize);
/*    TInt err( iFetchedChecker->LastFetchResult( aPath ) );
    if ( err == KErrNone ) // To avoid useless sql gets that fails for sure
        {
        TRAP( err, StoreForPathL( aPath )->FetchThumbnailL( aPath, aThumbnail, aData, aThumbnailSize, aOriginalSize) );
        if ( err != KErrNone )
            {
            iFetchedChecker->SetFetchResult( aPath, err );
            }
        }
    User::LeaveIfError( err );*/
    }



// -----------------------------------------------------------------------------
// CThumbnailServer::DeleteBitmapFromPool()
// Removes bitmap from bitmap pool
// -----------------------------------------------------------------------------
//
void CThumbnailServer::DeleteBitmapFromPool( TInt aHandle )
    {
    TN_DEBUG2( "CThumbnailServer::DeleteBitmapFromPool(%d)", aHandle );

    TThumbnailBitmapRef* ptr = iBitmapPool.Find( aHandle );
    if ( ptr )
        {
        ptr->iRefCount--;
        if ( !ptr->iRefCount )
            {
            TN_DEBUG3( 
                "CThumbnailServer::DeleteBitmapFromPool(%d) -- deleting 0x%08x)", aHandle, ptr );
            delete ptr->iBitmap;
            ptr->iBitmap = NULL;
            iBitmapPool.Remove( aHandle );
            TN_DEBUG2( "CThumbnailServer::DeleteBitmapFromPool -- items left %d", iBitmapPool.Count() );
            }
        else
            {
            TN_DEBUG3( 
                "CThumbnailServer::DeleteBitmapFromPool(%d) -- refcount now %d",
                aHandle, ptr->iRefCount );
            }
        }
    else
        {
        //__ASSERT_DEBUG(( EFalse ), ThumbnailPanic( EThumbnailBitmapNotReleased ));
        TN_DEBUG2( "CThumbnailServer::DeleteBitmapFromPool(%d) -- not found!",
            aHandle );
        }
    }


// -----------------------------------------------------------------------------
// Delete thumbnails for given object file
// -----------------------------------------------------------------------------
//
void CThumbnailServer::DeleteThumbnailsL( const TDesC& aPath )
    {
    TN_DEBUG2( "CThumbnailServer::DeleteThumbnailsL(%S)", &aPath);
    
    StoreForPathL( aPath )->DeleteThumbnailsL( aPath );
	/*
	if( iFetchedChecker )
		{
    	iFetchedChecker->SetFetchResult( aPath, KErrNone );
		}*/
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::ResolveMimeTypeL()
// -----------------------------------------------------------------------------
//
TDataType CThumbnailServer::ResolveMimeTypeL( RFile& aFile )
    {
    TN_DEBUG1( "CThumbnailStore::ResolveMimeTypeL()");
    RFile tmp = aFile;
    
    // check if DRM
    ContentAccess::CData* data = ContentAccess::CData::NewLC( 
            tmp, ContentAccess::KDefaultContentObject, ContentAccess::EPeek );

    TInt filetype( 0 );
    TInt drm( 0 );
    User::LeaveIfError( data->GetAttribute( ContentAccess::EIsProtected, drm ) );
    data->GetAttribute( ContentAccess::EFileType, filetype );
    CleanupStack::PopAndDestroy();    

	//close aFile on leave	
    CleanupClosePushL( aFile );    
    
    if ( drm && filetype != ContentAccess::EOma1Dcf )
        {
        // cannot handle other than Oma DRM 1.x files
        TN_DEBUG1( "CThumbnailStore::ResolveMimeTypeL()- only OMA DRM 1.0 supported");
        User::Leave(KErrNotSupported);
        }    
    
    TDataRecognitionResult res;
    if ( iRecognizer.Handle() == KNullHandle )
        {
        // error using recognizer, (re)connect
        User::LeaveIfError( iRecognizer.Connect());
        }
    
    User::LeaveIfError( iRecognizer.RecognizeData( aFile, res ));
    
    if ( res.iConfidence == CApaDataRecognizerType::ENotRecognized )
        {
        // file type not supported
        User::Leave( KErrNotSupported );
        }

    CleanupStack::Pop( &aFile );
    return res.iDataType;
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::ResolveProviderL()
// Resolves plugin to be used in thumbnail creation.
// -----------------------------------------------------------------------------
//
CThumbnailProvider* CThumbnailServer::ResolveProviderL( const TDesC8& aMimeType
    )
    {
#ifdef _DEBUG
    TBuf < KMaxDataTypeLength > buf; // 16-bit descriptor for debug prints
    buf.Copy( aMimeType );
    TN_DEBUG2( "CThumbnailServer::ResolveProviderL(%S)", &buf );
#endif 

    CThumbnailProvider* ret = NULL;

    TInt separatorPos = aMimeType.Locate( KThumbnailMimeSeparatorChar );
    TPtrC8 mediaType( aMimeType.Left( separatorPos ));
    TPtrC8 subType( aMimeType.Mid( separatorPos + 1 )); // skip slash

    const TInt count = iPluginInfoArray.Count();
    for ( TInt i( 0 ); i < count && !ret; i++ )
        {
        const TDesC8& opaqueData = iPluginInfoArray[i]->OpaqueData();
        TInt pSeparatorPos = opaqueData.Locate( KThumbnailMimeSeparatorChar );
        TPtrC8 pMediaType( opaqueData.Left( pSeparatorPos ));
        TPtrC8 pSubType( opaqueData.Mid( pSeparatorPos + 1 )); // skip slash
        
        if ( !pMediaType.CompareF( mediaType ))
            {
            if ( !pSubType.CompareF( KThumbnailMimeWildCard ) ||
                !pSubType.CompareF( subType ))
                {
#ifdef _DEBUG
                TN_DEBUG3( 
                    "CThumbnailServer::ResolveProviderL(%S) -- using provider 0x%08x", &buf, iPluginInfoArray[i]->ImplementationUid().iUid );
#endif 
                ret = GetProviderL( iPluginInfoArray[i]->ImplementationUid());
                }
            }
        }
    if ( !ret )
        {
#ifdef _DEBUG
        TN_DEBUG2( 
            "CThumbnailServer::ResolveProviderL(%S) -- provider not found",
            &buf );
#endif 
        User::Leave( KErrNotSupported );
        }
    return ret;
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::GetProviderL()
// -----------------------------------------------------------------------------
//
CThumbnailProvider* CThumbnailServer::GetProviderL( const TUid& aImplUid )
    {
    CThumbnailProvider** resPtr = iProviders.Find( aImplUid.iUid );
    CThumbnailProvider* res = NULL;
    if ( resPtr )
        {
        // Use existing instance
        res = * resPtr;
        }
    else
        {
        // Plug-in needs to be loaded
        TN_DEBUG2( 
            "CThumbnailServer::GetProviderL() -- loading plug-in, UID 0x%08x",
            aImplUid );
        res = CThumbnailProvider::NewL( aImplUid );
        TN_DEBUG1( "CThumbnailServer::GetProviderL() -- loading complete" );
        CleanupStack::PushL( res );
        iProviders.InsertL( aImplUid.iUid, res );
        CleanupStack::Pop( res );
        }

    return res;
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::QueueTaskL()
// Adds thumbnailtask to processor queue.
// -----------------------------------------------------------------------------
//
void CThumbnailServer::QueueTaskL( CThumbnailTask* aTask )
    {
    __ASSERT_DEBUG(( aTask ), ThumbnailPanic( EThumbnailNullPointer ));
    iProcessor->AddTaskL( aTask );
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::DequeTask()
// Removes thumbnailtask from processor queue.
// -----------------------------------------------------------------------------
//
TInt CThumbnailServer::DequeTask( const TThumbnailServerRequestId& aRequestId )
    {
    
    TInt error = iProcessor->RemoveTask( aRequestId );
        
    // clean-up bitmap pool        
        
    THashMapIter < TInt, TThumbnailBitmapRef > bpiter( iBitmapPool );

    // const pointer to a non-const object
    const TThumbnailBitmapRef* ref = bpiter.NextValue();

    while ( ref )
        {
        
        TN_DEBUG2( "CThumbnailServer::DequeTask() - ref->iRequestId = %d", ref->iRequestId );

        if ( ref->iSession == aRequestId.iSession && 
             ref->iRequestId == aRequestId.iRequestId )
            {            
            delete ref->iBitmap;            
            bpiter.RemoveCurrent();                        
                        
            TN_DEBUG2( "CThumbnailServer::DequeTask() - deleted bitmap, left=%d", 
                                iBitmapPool.Count());
            }
        ref = bpiter.NextValue();
        
        }

    return error;
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::ChangeTaskPriority()
// Changes priority of specific task.
// -----------------------------------------------------------------------------
//
TInt CThumbnailServer::ChangeTaskPriority( const TThumbnailServerRequestId&
    aRequestId, TInt aNewPriority )
    {
    return iProcessor->ChangeTaskPriority( aRequestId, aNewPriority );
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::ScaleBitmapL()
// Used to scale image.
// -----------------------------------------------------------------------------
//
void CThumbnailServer::ScaleBitmapL( TRequestStatus& aStatus, const CFbsBitmap&
    aSource, CFbsBitmap& aDest, const TRect& aSourceRect )
    {
    if ( !iScaler )
        {
        iScaler = IHLScaler::CreateL();
        }
    TRect destRect( TPoint(), aDest.SizeInPixels());
    User::LeaveIfError( iScaler->Scale( aStatus, aSource, aSourceRect, aDest,
        destRect ));
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::CancelScale()
// Cancels scaling task.
// -----------------------------------------------------------------------------
//
void CThumbnailServer::CancelScale()
    {
    if ( iScaler )
        {
        iScaler->CancelProcess();
        }
    }


// -----------------------------------------------------------------------------
// CThumbnailServer::Processor()
// Returns processor.
// -----------------------------------------------------------------------------
//
CThumbnailTaskProcessor& CThumbnailServer::Processor()
    {
    __ASSERT_DEBUG(( iProcessor ), ThumbnailPanic( EThumbnailNullPointer ));
    return * iProcessor;
    }

// -----------------------------------------------------------------------------
// Get the thumbnail store instance, which is responsible for this drive
// -----------------------------------------------------------------------------
//
CThumbnailStore* CThumbnailServer::StoreForDriveL( const TInt aDrive )
    {
    TN_DEBUG2( "CThumbnailServer::StoreForDriveL() drive=%d", aDrive );
    CThumbnailStore** resPtr = iStores.Find( aDrive );
    CThumbnailStore* res = NULL;


    if ( resPtr )
        {
        res = * resPtr;
        }
    else
        {
        if(iFormatting)
           {
           TN_DEBUG1( "CThumbnailServer::StoreForDriveL() - FORMATTING! - ABORT");
           User::Leave( KErrNotSupported );
           } 
        TVolumeInfo volumeInfo;
        TInt err = iFs.Volume( volumeInfo, aDrive );
        if ( err || volumeInfo.iDrive.iDriveAtt& KDriveAttRom ||
            volumeInfo.iDrive.iDriveAtt& KDriveAttRemote ||
            volumeInfo.iDrive.iMediaAtt& KMediaAttWriteProtected ||
            volumeInfo.iDrive.iMediaAtt& KMediaAttLocked )
            {
            // We don't support ROM disks or remote mounts. Media
            // must be read-write and not locked.
            User::Leave( KErrAccessDenied);
            }

        res = CThumbnailStore::NewL( iFs, aDrive, iImei, this );
        CleanupStack::PushL( res );
        iStores.InsertL( aDrive, res );
        res->SetPersistentSizes(iPersistentSizes);
        CleanupStack::Pop( res );
        
        for(TInt i = 0; i<iUnmountObservers.Count(); i++)
            {
            iUnmountObservers[i]->StartNotify();
            }
        }

    return res;
    }

// -----------------------------------------------------------------------------
// Get the thumbnail store instance, which is responsible for the drive
// identified by given path
// -----------------------------------------------------------------------------
//
CThumbnailStore* CThumbnailServer::StoreForPathL( const TDesC& aPath )
    {
    if(aPath.Length() < 3 || aPath.Length() > KMaxPath)
        {
        TN_DEBUG1( "CThumbnailServer::StoreForPathL() - KErrArgument");
        User::Leave(KErrArgument);
        }
    TInt drive = 0;
    User::LeaveIfError( RFs::CharToDrive( aPath[0], drive ));
    return StoreForDriveL( drive );
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::PersistentSizeL()
// ---------------------------------------------------------------------------
//
TThumbnailPersistentSize & CThumbnailServer::PersistentSizeL( TThumbnailSize
        aThumbnailSize )
    {
    if ( !iCenrep )
           {
           iCenrep = CThumbnailCenRep::NewL();
           }
    
    return iCenrep->PersistentSizeL( aThumbnailSize );
    
    }

// -----------------------------------------------------------------------------
// Open store for each mounted drive
// -----------------------------------------------------------------------------
//
void CThumbnailServer::OpenStoresL()
    {      
    // get list of mounted drives and open stores
    TDriveList driveListInt;
    TInt driveCountInt(0);
    User::LeaveIfError(DriveInfo::GetUserVisibleDrives(
           iFs, driveListInt, driveCountInt, KDriveAttInternal | KDriveAttRemovable ));

    for( TInt i = EDriveA; i <= EDriveZ && driveCountInt; i++ )
        {
        if (driveListInt[i])
            {
            TVolumeInfo volumeInfo;
            TInt err = iFs.Volume( volumeInfo, i );
            
            if (!err)
                {
                TN_DEBUG2( "CThumbnailServer::OpenStoresL() StoreForDriveL %d", i);
                
                // ignore errors
                TRAP_IGNORE( StoreForDriveL( i ));
                
                driveCountInt--;
                }
            }            
        }
    
    }

// -----------------------------------------------------------------------------
// Close the thumbnail store instance, which is responsible for this drive
// -----------------------------------------------------------------------------
//
void CThumbnailServer::CloseStoreForDriveL( const TInt aDrive )
    {
    TN_DEBUG2( "CThumbnailServer::CloseStoreForDriveL drive=%d", aDrive);
    CThumbnailStore** store = iStores.Find( aDrive );
    
    if (store)
        {
        delete *store;
        iStores.Remove( aDrive );
        }
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::PersistentSizes()
// ---------------------------------------------------------------------------
//
RArray < TThumbnailPersistentSize > CThumbnailServer::PersistentSizesL()
    {
    return iPersistentSizes;
    }

void CThumbnailServer::GetMissingSizesL( const TDesC& aPath, TInt aSourceType, RArray <
    TThumbnailPersistentSize > & aMissingSizes, TBool aCheckGridSizeOnly  )
    {
    StoreForPathL( aPath )->GetMissingSizesL( aPath, aSourceType, aMissingSizes, aCheckGridSizeOnly );
    }

// ---------------------------------------------------------------------------
// CThumbnailServer::Fs()
// ---------------------------------------------------------------------------
//
RFs& CThumbnailServer::Fs()
    {
    return iFs;
    }

// ---------------------------------------------------------------------------
// CThumbnailServer::AddUnmountObserversL()
// ---------------------------------------------------------------------------
//
void CThumbnailServer::AddUnmountObserversL()
    {
    TDriveList driveList;
    TInt drive; 
    TDriveInfo driveInfo;
    
    iUnmountObservers.ResetAndDestroy();
    
    User::LeaveIfError( iFs.DriveList(driveList) );
   
    // search all drives
    for( drive = EDriveA; drive <= EDriveZ; drive++ )
        {
        if( !driveList[drive] ) 
            {
            // If drive-list entry is zero, drive is not available
            continue;
            }
        
        TInt err = iFs.Drive(driveInfo, drive);
        
        // if removable drive, add observer
        if (!err && driveInfo.iDriveAtt& KDriveAttRemovable)
            {
            TN_DEBUG2( "CThumbnailServer::AddOnMountObserver drive=%d", drive);
            CThumbnailDiskUnmountObserver* obs = CThumbnailDiskUnmountObserver::NewL( iFs, drive, this );
            CleanupStack::PushL( obs );
            iUnmountObservers.AppendL( obs );
            CleanupStack::Pop( obs );
            }
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailServer::MemoryCardStatusChangedL()
// ---------------------------------------------------------------------------
//
void CThumbnailServer::MemoryCardStatusChangedL()
    {
    TN_DEBUG1( "CThumbnailServer::MemoryCardStatusChangedL in()" );
    TDriveList driveList;
    TInt drive; 
    TVolumeInfo volumeInfo;
    TDriveInfo driveInfo;
        
    User::LeaveIfError( iFs.DriveList(driveList) );
       
    // search all drives
    for( drive = EDriveA; drive <= EDriveZ; drive++ )
        {
        if( !driveList[drive] ) 
           {
          // If drive-list entry is zero, drive is not available
            continue;
           }
            
        TInt err = iFs.Volume(volumeInfo, drive);
        TInt err_drive = iFs.Drive(driveInfo, drive);    
        
        // mount -- if removable drive, add new store
        if (!err && !err_drive && driveInfo.iDriveAtt& KDriveAttRemovable)
            {
            // ignore errors
            //TRAP_IGNORE( StoreForDriveL( drive ));
            }
        
        //dismount -- if removable drive, close store
        else if(err && !err_drive && driveInfo.iDriveAtt& KDriveAttRemovable)
            {
            CloseStoreForDriveL( drive);
            }
        }

    TN_DEBUG1( "CThumbnailServer::MemoryCardStatusChangedL out()" );
    }


// -----------------------------------------------------------------------------
// Get the required size (in characters) for a buffer that contains the
// list of supported MIME types
// -----------------------------------------------------------------------------
//
TInt CThumbnailServer::GetMimeTypeBufferSize()const
    {
    TInt size = 0;
    for ( TInt i = iPluginInfoArray.Count(); --i >= 0; )
        {
        const TDesC8& opaqueData = iPluginInfoArray[i]->OpaqueData();
        size += opaqueData.Length();
        size++; // space for separator character
        }
    if ( size )
        {
        size--; // no need for a separator character at the end
        }

    return size;
    }

// -----------------------------------------------------------------------------
// Get the list of supported MIME types and store them in the buffer
// allocated by the client.
// -----------------------------------------------------------------------------
//
void CThumbnailServer::GetMimeTypeList( TDes& aBuffer )const
    {
    TBuf < KMaxDataTypeLength > buf; // needed for convert from TBuf8 to TBuf
    aBuffer.Zero();
    const TInt count = iPluginInfoArray.Count();
    for ( TInt i = 0; i < count; i++ )
        {
        const TDesC8& opaqueData = iPluginInfoArray[i]->OpaqueData();
        buf.Copy( opaqueData );
        aBuffer.Append( buf );
        aBuffer.Append( KThumbnailMimeTypeSeparatorChar );
        }
    if ( count )
        {
        // remove last separator char
        aBuffer.SetLength( aBuffer.Length() - 1 );
        }
    }


// -----------------------------------------------------------------------------
// Updates thumbnails by given Id.
// -----------------------------------------------------------------------------
//
TBool CThumbnailServer::UpdateThumbnailsL( const TDesC& aPath,
                                           const TInt /*aOrientation*/, const TInt64 aModified )
    {
    TN_DEBUG1( "CThumbnailServer::UpdateThumbnailsL()");
    
    // 1. check path change
    // 2. check orientation change
    // 3. check timestamp change

    TBool orientationChanged = EFalse;
    TBool modifiedChanged = EFalse;
    
    CThumbnailStore* store = StoreForPathL( aPath );
   
    // placeholder for orientation check
    orientationChanged = EFalse;
    
    if (orientationChanged)
        {
        TN_DEBUG1( "CThumbnailServer::UpdateThumbnailsL() - orientation updated");
        
        // orientation updated, no need to check further
        return ETrue;
        }
    else
        {
        TN_DEBUG1( "CThumbnailServer::UpdateThumbnailsL() - exist");
        
        TBool exists = store->CheckModifiedByPathL(aPath, aModified, modifiedChanged);
       
        if(!exists)
            {
            TN_DEBUG1( "CThumbnailServer::UpdateThumbnailsL() - exists NO");
            //not found, needs to be generated
            return EFalse;
            }
        
        TN_DEBUG1( "CThumbnailServer::UpdateThumbnailsL() - modified ?");
        
        if (modifiedChanged)
            {
            TN_DEBUG1( "CThumbnailServer::UpdateThumbnailsL() - modified YES");
            
            // delete old thumbs
            store->DeleteThumbnailsL(aPath, ETrue);
            
            // need to create new thumbs
            }
        else
            {
            TN_DEBUG1( "CThumbnailServer::UpdateThumbnailsL() - modified NO");
            
            // not modified
            return ETrue;
            }
        }
  
    
    TN_DEBUG1( "CThumbnailServer::UpdateThumbnailsL() - no thumbs found, create new");
    
    return EFalse;
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::MimeTypeFromFileExt()
// -----------------------------------------------------------------------------
//
TInt CThumbnailServer::MimeTypeFromFileExt( const TDesC& aFileName, TDataType& aMimeType )
    {
    TBool found = ETrue;
    TParsePtrC parse( aFileName );
    TPtrC ext( parse.Ext() );
    
    if ( ext.CompareF( KJpegExt ) == 0 || ext.CompareF( KJpgExt ) == 0)
        {
        aMimeType = TDataType( KJpegMime );
        }
    else if ( ext.CompareF( KJpeg2000Ext ) == 0 )
        {
        aMimeType = TDataType( KJpeg2000Mime );
        }
    else if ( ext.CompareF( KSvgExt ) == 0 )
        {
        aMimeType = TDataType( KSvgMime );
        }    
    else if ( ext.CompareF( KGifExt ) == 0 )
        {
        aMimeType = TDataType( KGifMime );
        } 
    else if ( ext.CompareF( KPngExt ) == 0 )
        {
        aMimeType = TDataType( KPngMime );
        }
    else if ( ext.CompareF( KMpgExt1 ) == 0 )
        {
        aMimeType = TDataType( KMpgMime1 );
        } 
    else if ( ext.CompareF( KMpeg4Ext ) == 0 )
        {
        aMimeType = TDataType( KMpeg4Mime );
        } 
    else if ( ext.CompareF( KMp4Ext ) == 0 )
        {
        aMimeType = TDataType( KMp4Mime );
        } 
    else if ( ext.CompareF( KAviExt ) == 0 )
        {
        aMimeType = TDataType( KAviMime );
        }
    else if ( ext.CompareF( KMp3Ext ) == 0 )
        {
        aMimeType = TDataType( KMp3Mime );
        } 
    else if ( ext.CompareF( KNonEmbeddArtExt ) == 0 )
        {
        aMimeType = TDataType( KNonEmbeddArtMime );
        }
    else if ( ext.CompareF( KAacExt ) == 0 )
        {
        aMimeType = TDataType( KAacMime );
        }   
    else if ( ext.CompareF( KWmaExt ) == 0 )
        {
        aMimeType = TDataType( KWmaMime );
        } 
    else if ( ext.CompareF( KBmpExt ) == 0 )
        {
        aMimeType = TDataType( KBmpMime );
        }
    else if ( ext.CompareF( K3gpExt ) == 0 )
        {
        aMimeType = TDataType( KVideo3gppMime );
        } 
    else if ( ext.CompareF( KAmrExt ) == 0 )
        {
        aMimeType = TDataType( KAudioAmrMime );
        }
    else if ( ext.CompareF( KWmvExt ) == 0 )
        {
        aMimeType = TDataType( KVideoWmvMime );
        } 
    else if ( ext.CompareF( KRealAudioExt ) == 0 )
        {
        aMimeType = TDataType( KRealAudioMime );
        }
    else if ( ext.CompareF( KPmRealAudioPluginExt ) == 0 )
        {
        aMimeType = TDataType( KPmRealAudioPluginMime );
        } 
    else if ( ext.CompareF( KRealVideoExt ) == 0 )
        {
        aMimeType = TDataType( KRealVideoMime );
        }
    else if ( ext.CompareF( KM4aExt ) == 0 )
        {
        aMimeType = TDataType( KM4aMime);
        }
    else if ( ext.CompareF( KM4vExt ) == 0 )
        {
        aMimeType = TDataType( KMp4Mime);
        }
    else if ( ext.CompareF( KPmRealVideoPluginExt ) == 0 )
        {
        aMimeType = TDataType( KPmRealVideoPluginMime );
        }
    else if ( ext.CompareF( KPmRealVbVideoPluginExt ) == 0 )
        {
        aMimeType = TDataType( KPmRealVbVideoPluginMime );
        }
    else if ( ext.CompareF( KFlashVideoExt ) == 0 )
        {
        aMimeType = TDataType( KFlashVideoMime );
        } 
    else if ( ext.CompareF( KMatroskaVideoExt ) == 0 )
        {
        aMimeType = TDataType( KMatroskaVideoMime );
        } 
    else
        {
        aMimeType = TDataType( KNullDesC8 );
        found = EFalse;
        }
    
    if (found)
        {
        return KErrNone;
        }
    
    return KErrNotFound;
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::SourceTypeFromMimeType()
// -----------------------------------------------------------------------------
//
TInt CThumbnailServer::SourceTypeFromMimeType( const TDataType& aMimeType )
    {
    const TPtrC8 mimeType = aMimeType.Des8();
  
    TInt separatorPos = mimeType.Locate( KThumbnailMimeSeparatorChar );
    TPtrC8 mediaType( mimeType.Left( separatorPos ));

    if (mediaType.Compare(KThumbnailMimeImage) == 0)
        {
        return TThumbnailPersistentSize::EImage;
        }
    else if (mediaType.Compare(KThumbnailMimeVideo) == 0)
        {
        return TThumbnailPersistentSize::EVideo;
        }
    else if (mediaType.Compare(KThumbnailMimeAudio) == 0)
        {
        return TThumbnailPersistentSize::EAudio;
        }

    return TThumbnailPersistentSize::EUnknownSourceType;        
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::SourceTypeFromSizeType()
// -----------------------------------------------------------------------------
//
TInt CThumbnailServer::SourceTypeFromSizeType( const TInt aSizeType )
    {
    TInt sourceType = 0;
    
    switch (aSizeType)
        {
        case EImageGridThumbnailSize:
        case EImageListThumbnailSize:
        case EImageFullScreenThumbnailSize:
            sourceType = TThumbnailPersistentSize::EImage;
            break;
        case EVideoGridThumbnailSize:
        case EVideoListThumbnailSize:
        case EVideoFullScreenThumbnailSize:  
            sourceType = TThumbnailPersistentSize::EVideo;
            break;
        case EAudioGridThumbnailSize:
        case EAudioListThumbnailSize:
        case EAudioFullScreenThumbnailSize:
            sourceType = TThumbnailPersistentSize::EAudio;
            break;
        default:
            sourceType = TThumbnailPersistentSize::EUnknownSourceType;  
        }
    
    return sourceType;
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::SupportedMimeType()
// -----------------------------------------------------------------------------
//
TBool CThumbnailServer::SupportedMimeType( const TDataType& aMimeType )
    {
    const TPtrC8 mimeType = aMimeType.Des8();
    
    if ( mimeType.CompareF( KJpegMime ) == 0 || 
         mimeType.CompareF( KJpeg2000Mime ) == 0 ||
         mimeType.CompareF( KGifMime ) == 0 ||
         mimeType.CompareF( KPngMime ) == 0 ||
         mimeType.CompareF( KBmpMime ) == 0 ||
         mimeType.CompareF( KMpgMime1 ) == 0 ||
         mimeType.CompareF( KMpeg4Mime ) == 0 ||
         mimeType.CompareF( KMp4Mime ) == 0 ||
         mimeType.CompareF( KAviMime ) == 0 ||
         mimeType.CompareF( KVideo3gppMime ) == 0 ||
         mimeType.CompareF( KVideoWmvMime ) == 0 ||
         mimeType.CompareF( KRealVideoMime ) == 0 ||
         mimeType.CompareF( KMp3Mime ) == 0 ||
         mimeType.CompareF( KAacMime ) == 0 ||
         mimeType.CompareF( KWmaMime ) == 0 ||
         mimeType.CompareF( KAudioAmrMime ) == 0 ||
         mimeType.CompareF( KRealAudioMime ) == 0 ||
         mimeType.CompareF( KM4aMime ) == 0  ||
         mimeType.CompareF( KFlashVideoMime ) == 0 ||
         mimeType.CompareF( KPmRealVideoPluginMime ) == 0 ||
         mimeType.CompareF( KPmRealVbVideoPluginMime ) == 0 ||
         mimeType.CompareF( KPmRealAudioPluginMime ) == 0 )
        {
        return ETrue;
        }
    
    return EFalse;
    }

// -----------------------------------------------------------------------------
// CThumbnailServer::GetMdESession()
// -----------------------------------------------------------------------------
//
CMdESession* CThumbnailServer::GetMdESession()
    {
    return iMdESession;
    }


// -----------------------------------------------------------------------------
// E32Main()
// -----------------------------------------------------------------------------
//
TInt E32Main()
    {
    __UHEAP_MARK;
    CTrapCleanup* cleanup = CTrapCleanup::New();
    TInt result = KErrNoMemory;
    if ( cleanup )
        {
        TRAP( result, CThumbnailServer::ThreadFunctionL());
        TN_DEBUG2( 
            "CThumbnailServer::E32Main() -- thread function out, result=%d",
            result );
        delete cleanup;
        }
    if ( result != KErrNone )
        {
        // Signal the client that server creation failed
        TN_DEBUG1( "CThumbnailServer::E32Main() -- Rendezvous() in" );
        RProcess::Rendezvous( result );
        TN_DEBUG1( "CThumbnailServer::E32Main() -- Rendezvous() out" );
        }

    __UHEAP_MARKEND;
    return result;
    }

// -----------------------------------------------------------------------------
// Closes stores for removable drives
// -----------------------------------------------------------------------------
//
void CThumbnailServer::CloseRemovableDrivesL()
    {
    TDriveList driveList;
    TInt drive; 
    TDriveInfo driveInfo;
    iFormatting = ETrue;    
        
    User::LeaveIfError( iFs.DriveList(driveList) );
       
    // search all drives
    for( drive = EDriveA; drive <= EDriveZ; drive++ )
        {
        if( !driveList[drive] ) 
           {
           // If drive-list entry is zero, drive is not available
           continue;
           }
            
        TInt err = iFs.Drive(driveInfo, drive);
            
        // if removable drive, close store
        if (!err && driveInfo.iDriveAtt& KDriveAttRemovable)
            {
            TN_DEBUG2( "CThumbnailServer::CloseRemovableDrive drive=%d", drive);
            CloseStoreForDriveL(drive);
            }
        }
    iProcessor->RemoveAllTasks();
    }

// -----------------------------------------------------------------------------
// Open Stores for removable drives
// -----------------------------------------------------------------------------
//
void CThumbnailServer::OpenRemovableDrivesL()
    {
    TDriveList driveList;
    TInt drive; 
    TDriveInfo driveInfo;
    iFormatting = EFalse;    
        
    User::LeaveIfError( iFs.DriveList(driveList) );
       
    // search all drives
    for( drive = EDriveA; drive <= EDriveZ; drive++ )
        {
        if( !driveList[drive] ) 
           {
           // If drive-list entry is zero, drive is not available
           continue;
           }
            
        TInt err = iFs.Drive(driveInfo, drive);
            
        // if removable drive, open store
        if (!err && driveInfo.iDriveAtt& KDriveAttRemovable)
            {
            TN_DEBUG2( "CThumbnailServer::OpenRemovableDrive drive=%d", drive);
            StoreForDriveL(drive);
            }
        }
    }

// -----------------------------------------------------------------------------
// Is formatting ongoing
// -----------------------------------------------------------------------------
//
TBool CThumbnailServer::IsFormatting()
    {
    return iFormatting;
    }

// ---------------------------------------------------------------------------
// CThumbnailServer::FormatNotification
// Handles a format operation
// ---------------------------------------------------------------------------
//
void CThumbnailServer::FormatNotification( TBool aFormat )
    {
    TN_DEBUG2( "CThumbnailServer::FormatNotification(%d)", aFormat );
    
    if(aFormat)
        {
        TRAP_IGNORE( CloseRemovableDrivesL() );
        }
    else 
        {
        TRAP_IGNORE( OpenRemovableDrivesL() );
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailServer::ReconnectCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbnailServer::ReconnectCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbnailServer::ReconnectCallBack() - reconnect");
    
    CThumbnailServer* self = static_cast<CThumbnailServer*>( aAny );
    
    self->iReconnect->Cancel();
    
    if (self->iMdESession)
        {
        delete self->iMdESession;
        self->iMdESession = NULL;
        }
    
    // reconnect to MDS
    TRAP_IGNORE( self->iMdESession = CMdESession::NewL( *self ) );
    
    TN_DEBUG1( "CThumbAGDaemon::ReconnectCallBack() - done");
    
    return KErrNone;
    }

