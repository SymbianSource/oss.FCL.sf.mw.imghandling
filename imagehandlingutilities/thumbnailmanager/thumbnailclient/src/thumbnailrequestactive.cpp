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
* Description:  Active object for an asynchronous thumbnail request
 *
*/


#include <e32base.h>
#include <hal.h>
#include <hal_data.h>
#include <apmstd.h>

#include "thumbnailrequestactive.h"
#include "thumbnailrequestqueue.h"
#include "thumbnailsession.h"
#include "thumbnailmanagerobserver.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaildataimpl.h"
#include "thumbnailpanic.h"
#include "thumbnaillog.h"

#include "thumbnailmanagerimpl.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::~CThumbnailRequestActive()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailRequestActive::~CThumbnailRequestActive()
    {
    Cancel();
    
    if(iTimer)
        {
        iTimer->Cancel();
        }
    delete iTimer;
    
    ReleaseServerBitmap();
    delete iCallbackThumbnail;
    delete iParams.iBuffer;
    delete iBitmap;
    iFile.Close();
    iMyFileHandle.Close();
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::NewL
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailRequestActive* CThumbnailRequestActive::NewL( RFs& aFs,
    RThumbnailSession& aThumbnailSession, MThumbnailManagerObserver& aObserver,
    MThumbnailManagerRequestObserver* aRequestObserver,
    TThumbnailRequestId aId, TInt aPriority, CThumbnailRequestQueue* aQueue )
    {
    CThumbnailRequestActive* self = new( ELeave )CThumbnailRequestActive( aFs,
        aThumbnailSession, aObserver, aRequestObserver, aId, aPriority, aQueue );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::CThumbnailRequestActive()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailRequestActive::CThumbnailRequestActive( RFs& aFs, RThumbnailSession&
    aThumbnailSession, MThumbnailManagerObserver& aObserver,
    MThumbnailManagerRequestObserver* aRequestObserver,
    TThumbnailRequestId aId, TInt aPriority, CThumbnailRequestQueue* aQueue ):
    CActive( aPriority ), iSession( aThumbnailSession ), iParamsPckg( iParams ),
    iObserver( aObserver ), iRequestObserver( aRequestObserver ), iFs( aFs ), iBitmapHandle( 0 ), 
    iRequestId( aId ), iRequestQueue( aQueue ), iCanceled( EFalse )
    {
    CActiveScheduler::Add( this );
    TN_DEBUG2( "CThumbnaiRequestActive::CThumbnailRequestActive() AO's priority = %d", Priority());
    
    iBitmap = NULL;
    iRequestCompleted = EFalse;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::ConstructL()
    {
    iCallbackThumbnail = new( ELeave )CThumbnailDataImpl();

    iTimer = CPeriodic::NewL(CActive::EPriorityIdle);
    iStartError = KErrNone;
    
#ifdef _DEBUG
    iStartExecTime.UniversalTime();
#endif
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::StartL()
// Start active request.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::StartL()
    {
    TN_DEBUG3( "CThumbnailRequestActive::StartL(0x%08x) reqid = %d", this, iRequestId );
    
    iRequestActive = ETrue;

#ifdef _DEBUG
    TTime stop;
    stop.UniversalTime();
    TN_DEBUG3( "CThumbnailRequestActive::StartL() req id = %d, queuing time %d ms",
               iParams.iRequestId, (TInt)stop.MicroSecondsFrom(iStartExecTime).Int64()/1000 );
#endif
    
    switch (iRequestType)
        {
        case EReqGetThumbnailHandle:
            {
            iSession.RequestThumbnailL( iFile, iTargetUri, iParamsPckg, iStatus );
            break;
            }        
        case EReqGetThumbnailPath:
            {
            // for custom size requests file handle can be opened here already, because
            // custom thumbnails are never stored in db  
            if (iParams.iThumbnailSize == ECustomThumbnailSize &&
                iParams.iOriginalControlFlags != EThumbnailGeneratePersistentSizesOnly)
                {
                TN_DEBUG1( "CThumbnaiRequestActive::StartL()- custom size request" );
                  
                iFile.Close();
                User::LeaveIfError( iFile.Open( iFs, iPath, EFileShareReadersOrWriters ) );  
                
                TN_DEBUG2( "CThumbnailRequestActive::StartL() - file handle opened for %S", &iTargetUri );
                
                CleanupClosePushL( iFile );
                iSession.RequestThumbnailL( iFile, iPath, iParamsPckg, iStatus );
                CleanupStack::PopAndDestroy( &iFile );   
                }
            else
                {
                iSession.RequestThumbnailL( iPath, iTargetUri, iParams.iThumbnailId, 
                                            iParamsPckg, iStatus );
                }            
            break;
            }  
        case EReqSetThumbnailBuffer:
            {
            iSession.RequestSetThumbnailL( iParams.iBuffer, iTargetUri, iParamsPckg, iStatus );
            break;
            }  
        case EReqSetThumbnailBitmap:
            {
            iSession.RequestSetThumbnailL( iBitmap->Handle(), iTargetUri, iParamsPckg, iStatus );
            break;
            }  
        case EReqUpdateThumbnails:
            {
            iSession.UpdateThumbnails( iPath, iOrientation, iModified, iParamsPckg, iStatus );
            break;
            }       
        case EReqGetThumbnailHandleLater:
            {
            // open file handle
            iFile.Close();
            User::LeaveIfError( iFile.Open( iFs, iTargetUri, EFileShareReadersOrWriters ) );  
            
            TN_DEBUG2( "CThumbnailRequestActive::StartL() - file handle opened for %S", &iTargetUri );
            
            CleanupClosePushL( iFile );
            iSession.RequestThumbnailL( iFile, iTargetUri, iParamsPckg, iStatus );
            CleanupStack::PopAndDestroy( &iFile );
            break;
            }  
        case EReqDeleteThumbnails:
            {
            // by path
            if (iPath != KNullDesC)
                {
                iSession.DeleteThumbnails( iPath, iParamsPckg, iStatus );
                }
            // by id
            else
                {
                iSession.DeleteThumbnails( iParams.iThumbnailId, iParamsPckg, iStatus );
                }
            break;
            }
        case EReqRenameThumbnails:
            {
            iSession.RenameThumbnails( iParamsPckg, iStatus );
            break;
            }  
        default:
            {
            break;
            }
        }
    
    iTimer->Cancel();
    iTimer->Start( KClientRequestTimeout, KClientRequestTimeout, 
                   TCallBack(TimerCallBack, this));
    SetActive();
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::RunL()
    {
    TN_DEBUG2( "CThumbnailRequestActive::RunL() - request ID: %d", iParams.iRequestId );
 
    if ( iParams.iControlFlags == EThumbnailPreviewThumbnail )
        {
        iRequestCompleted = EFalse;
		iProcessingPreview = ETrue;
        }
    else
        {
        iRequestCompleted = ETrue;
        }
    
    iTimer->Cancel();
    
    if (iRequestType == EReqDeleteThumbnails)
        {
        TN_DEBUG1( "CThumbnailRequestActive::RunL() - delete" );
    
        if (iRequestObserver)
            {
            iRequestObserver->ThumbnailRequestReady(iStatus.Int(), ERequestDeleteThumbnails ,iParams.iRequestId);
            }
        
        iFile.Close();
        iMyFileHandle.Close();
    
        // no action for delete/rename or canceled request
        iRequestQueue->RequestComplete(this);
        
#ifdef _DEBUG
    TTime stop;
    stop.UniversalTime();
    TN_DEBUG3( "CThumbnailRequestActive::RunL() total execution time of req %d is %d ms",
                iParams.iRequestId, (TInt)stop.MicroSecondsFrom(iStartExecTime).Int64()/1000 );
#endif
        }
    else if (iCanceled || iRequestType == EReqRenameThumbnails)
        {
        if (iCanceled)
            {
            TN_DEBUG1( "CThumbnailRequestActive::RunL() - canceled" );
            }
        else if (iRequestType == EReqRenameThumbnails)
            {
            TN_DEBUG1( "CThumbnailRequestActive::RunL() - rename" );
            }
    
        iFile.Close();
        iMyFileHandle.Close();
    
        // no action for delete/rename or canceled request
        iRequestQueue->RequestComplete(this);
        
#ifdef _DEBUG
    TTime stop;
    stop.UniversalTime();
    TN_DEBUG3( "CThumbnailRequestActive::RunL() total execution time of req %d is %d ms",
                iParams.iRequestId, (TInt)stop.MicroSecondsFrom(iStartExecTime).Int64()/1000 );
#endif
        }
    else if ( iStatus.Int() == KThumbnailErrThumbnailNotFound && iParams.iFileName.Length() && 
         !( iParams.iFlags& CThumbnailManager::EDoNotCreate ))
        {
        TN_DEBUG2( "CThumbnaiRequestActive::RunL() - no thumbnail found - lets try with file handle %S", &iParams.iFileName );
        iRequestCompleted = EFalse;
        
        // We tried to get thumbnail using file path, but it was not found in
        // the database. We need to open the file now (on the client side) and
        // use file handle.     
        iFile.Close();
        TInt err = iFile.Open( iFs, iParams.iFileName, EFileShareReadersOrWriters );
        TN_DEBUG2( "CThumbnaiRequestActive::RunL() - file handle open err = %d", err );
        User::LeaveIfError( err );
        
        CleanupClosePushL( iFile );
        
        TN_DEBUG2( "CThumbnaiRequestActive::RunL() - file handle opened for %S", &iParams.iFileName );
        
        iSession.RequestThumbnailL( iFile, iParams.iFileName, iParamsPckg, iStatus );
        CleanupStack::PopAndDestroy( &iFile );
        
        iTimer->Start( KClientRequestTimeout, KClientRequestTimeout, 
                           TCallBack(TimerCallBack, this));
        SetActive();
        }
    else if ( iStatus.Int() )
        {
        TN_DEBUG2( "CThumbnailRequestActive::RunL() - error (%d) occured", iStatus.Int() );
        // An error occurred
        iError = iStatus.Int();
        HandleError();
        }
    //1st round thumb ready from exif (stored)
    else if (iParams.iOriginalControlFlags == EThumbnailGeneratePersistentSizesOnly && !iProcessingPreview)
	    {
	    TN_DEBUG1( "CThumbnaiRequestActive::RunL()- generate persistent sizes thumbnailready" );
	    iBitmapHandle = iParams.iBitmapHandle;
	    
	    TN_DEBUG2( "CThumbnaiRequestActive::RunL() - iObserver.ThumbnailReady %d", iParams.iRequestId );
	    iObserver.ThumbnailReady( iStatus.Int(), *iCallbackThumbnail, iParams.iRequestId );
	  
	    ReleaseServerBitmap();
	    
	    iFile.Close();
	    iMyFileHandle.Close();
	    
	    iRequestQueue->RequestComplete(this);
	    
        #ifdef _DEBUG
            TTime stop;
            stop.UniversalTime();
            TN_DEBUG3( "CThumbnailRequestActive::RunL() total execution time %d, %d ms",
                       iParams.iRequestId, (TInt)stop.MicroSecondsFrom(iStartExecTime).Int64()/1000 );
        #endif
	    }
    //1st round preview ready from too low quality exif/exif not found (not-stored)
    //relauch using normal decoding
    else if (iParams.iOriginalControlFlags == EThumbnailGeneratePersistentSizesOnly && iProcessingPreview)
        {
        TN_DEBUG1( "CThumbnaiRequestActive::RunL()- generate persistent sizes thumbnailpreviewready" );
        iBitmapHandle = iParams.iBitmapHandle;
        
        TN_DEBUG2( "CThumbnaiRequestActive::RunL() - iObserver.ThumbnailPreviewReady %d", iParams.iRequestId );
        iObserver.ThumbnailPreviewReady( *iCallbackThumbnail, iParams.iRequestId );
        
        iProcessingPreview = EFalse;
      
        ReleaseServerBitmap();
        
        iFile.Close();
        
        //set flags so that EThumbnailGeneratePersistentSizesOnly is done aka check all missing sizes 
        iParams.iQualityPreference = CThumbnailManager::EOptimizeForQuality;
        iParams.iControlFlags = EThumbnailGeneratePersistentSizesOnly;
        iRequestType = EReqUpdateThumbnails;
        
        //increase priority
        this->SetPriority(this->Priority() + 1);
        iParams.iPriority++;
        
        iSession.UpdateThumbnails( iPath, iOrientation, iModified, iParamsPckg, iStatus );
        
        SetActive();
           
    	#ifdef _DEBUG
        TTime stop;
        stop.UniversalTime();
        TN_DEBUG3( "CThumbnailRequestActive::RunL() total execution time %d, %d ms",
                   iParams.iRequestId, (TInt)stop.MicroSecondsFrom(iStartExecTime).Int64()/1000 );
    	#endif
        }
    else
        {
        TN_DEBUG1( "CThumbnailRequestActive::RunL() - succesful" );
        
        // Success
        iBitmapHandle = iParams.iBitmapHandle;
        
        if( iBitmapHandle )
            {
            CFbsBitmap* bitmap = new( ELeave )CFbsBitmap();
            CleanupStack::PushL( bitmap );
            User::LeaveIfError( bitmap->Duplicate( iBitmapHandle ));
            CleanupStack::Pop( bitmap );
            
            // reduce bpp value (displaymode to match reqested bits per pixel)
            #ifdef _DEBUG
            TN_DEBUG2( "CThumbnailRequestActive::RunL() - displaymode is %d", bitmap->DisplayMode());
            #endif
            
            if( bitmap->DisplayMode() > iParams.iDisplayMode )
                {
                bitmap->SetDisplayMode( iParams.iDisplayMode );
                #ifdef _DEBUG
                TN_DEBUG2( "CThumbnailRequestActive::RunL() - displaymode is now %d", bitmap->DisplayMode());
                #endif
                }
            
            iCallbackThumbnail->Set( bitmap, iClientData );
            bitmap = NULL; // Owned by iCallbackThumbnail or client now
            }

        if ( iProcessingPreview )
            {
            TN_DEBUG2( "CThumbnailRequestActive::RunL() - iObserver.ThumbnailPreviewReady %d", iParams.iRequestId );
			
            //increase priority of 2nd round (both, AO and request itself)
            this->SetPriority(this->Priority() + 1);
            iParams.iPriority++;
            iObserver.ThumbnailPreviewReady( *iCallbackThumbnail, iParams.iRequestId );
            iProcessingPreview = EFalse;
            
            ReleaseServerBitmap();
            
            iFile.Close();
            
            Get2ndPhaseThumbnailL();
            }
        else
            {
            TN_DEBUG2( "CThumbnailRequestActive::RunL() - iObserver.ThumbnailReady %d", iParams.iRequestId );
            
            iObserver.ThumbnailReady( iStatus.Int(), * iCallbackThumbnail, iParams.iRequestId );
            ReleaseServerBitmap();    
            
            iFile.Close();
            iMyFileHandle.Close();
            
            iRequestQueue->RequestComplete(this);
            
#ifdef _DEBUG
        TTime stop;
        stop.UniversalTime();
        TN_DEBUG4( "CThumbnailRequestActive::RunL(0x%08x) total execution time of req %d is %d ms",
                this, iParams.iRequestId, (TInt)stop.MicroSecondsFrom(iStartExecTime).Int64()/1000 );
#endif
            }
        }
    
    delete iBitmap;
    iBitmap = NULL;
    
    //if request is complete set it also not active -> really finished
    if(iRequestCompleted)
        {
        iRequestActive = EFalse;
        }
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::RunError()
// ---------------------------------------------------------------------------
//
TInt CThumbnailRequestActive::RunError( TInt aError )
    {
    TN_DEBUG1( "CThumbnailRequestActive::RunError");
    
    iTimer->Cancel();
    
    // If RunL() left, the client did not get any callbacks yet.
    // We need to notify it of the error now.
    iError = aError;
    HandleError();
    
    return KErrNone;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::DoCancel()
    {
    TN_DEBUG1( "CThumbnailRequestActive::DoCancel");
    iTimer->Cancel();
    
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

    iCanceled = ETrue;
    iSession.CancelRequest( iRequestId );
    ReleaseServerBitmap();
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::AsyncCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::AsyncCancel()
    {
    TN_DEBUG1( "CThumbnailRequestActive::AsyncCancel");

    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
    
    iCanceled = ETrue;
    iSession.CancelRequest( iRequestId );
    ReleaseServerBitmap();

    // AO stays active until request is complete or timeout is reached
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::ReleaseServerBitmap()
// Releases reserved bitmap.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::ReleaseServerBitmap()
    {
    if ( iBitmapHandle && iSession.Handle())
        {
    	TN_DEBUG1( "CThumbnailRequestActive::ReleaseServerBitmap");
    
        iSession.ReleaseBitmap( iBitmapHandle );
        iBitmapHandle = 0;
        }
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::HandleError()
// Error handling function.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::HandleError()
    {
    TN_DEBUG1( "CThumbnailRequestActive::HandleError");
    
    if ( iError )
        {
        if( iError == KErrServerTerminated)
            {
            iSession.Close();
            TInt connErr = iSession.Connect();
            if (connErr != KErrNone)
                {
                TN_DEBUG2( "CThumbnaiRequestActive::HandleError() - session reconnect err %d", connErr );
                }
            else
                {
                TN_DEBUG1( "CThumbnailRequestActive::HandleError() - session reconnected");
                }
            }

        if (iRequestObserver && iRequestType == EReqDeleteThumbnails)
             {
             TN_DEBUG2( "CThumbnaiRequestActive::HandleError() - iRequestObserver->ThumbnailRequestReady %d", iParams.iRequestId );
             iRequestObserver->ThumbnailRequestReady(iError, ERequestDeleteThumbnails ,iParams.iRequestId);
             }
        else
            {			
			 iCallbackThumbnail->Set( NULL, iClientData );
        
	        // don't leak internal TNM codes
	        if (iError == KThumbnailErrThumbnailNotFound)
	            {
	            iError = KErrNotFound;
	            }
			
            TN_DEBUG2( "CThumbnaiRequestActive::HandleError() - iObserver.ThumbnailReady %d", iParams.iRequestId );
            iObserver.ThumbnailReady( iError, *iCallbackThumbnail, iParams.iRequestId );
            }
        
        iError = KErrNone;
        }
    
    ReleaseServerBitmap();
    
    iFile.Close();
    iMyFileHandle.Close();
    
    iRequestCompleted = ETrue;
    iRequestQueue->RequestComplete(this);
    iRequestActive = EFalse;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::GetThumbnailL()
// Get a thumbnail for an object file.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::GetThumbnailL( const RFile64& aFile, TThumbnailId aThumbnailId,
    CThumbnailManager::TThumbnailFlags aFlags, CThumbnailManager
    ::TThumbnailQualityPreference aQualityPreference, const TSize& aSize, const
    TDisplayMode aDisplayMode, const TInt aPriority, TAny* aClientData, TBool aGeneratePersistentSizesOnly,
    const TDesC& aTargetUri, TThumbnailSize aThumbnailSize)
    {
    iRequestType = EReqGetThumbnailHandle;

    if ( aQualityPreference == CThumbnailManager
        ::EOptimizeForQualityWithPreview )
        {
        // We may need the file handle later for the 2nd phase
        // thumbnail
        iMyFileHandle.Close();
        User::LeaveIfError( iMyFileHandle.Duplicate( aFile ));
        }

    iClientData = aClientData;
    iParams.iControlFlags = (aGeneratePersistentSizesOnly ?
    						EThumbnailGeneratePersistentSizesOnly :
    						EThumbnailNoControlFlags);    
    iParams.iOriginalControlFlags = iParams.iControlFlags;
    iParams.iBitmapHandle = 0;
    iParams.iSize = aSize;
    iParams.iDisplayMode = aDisplayMode;
    User::LeaveIfError( aFile.FullName( iParams.iFileName ));
    iParams.iRequestId = iRequestId;
    iParams.iPriority = aPriority;
    iParams.iFlags = aFlags;
    iParams.iQualityPreference = aQualityPreference;
    iParams.iThumbnailSize = aThumbnailSize;
    iParams.iThumbnailId = aThumbnailId;
    iParams.iOverwrite = EFalse;
    
    User::LeaveIfError( iFile.Duplicate( aFile ));
    
    iTargetUri = aTargetUri;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::GetThumbnailL()
// Get a thumbnail opening an object file later.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::GetThumbnailL( TThumbnailId aThumbnailId,
    const TDesC& aPath, CThumbnailManager::TThumbnailFlags aFlags, CThumbnailManager
    ::TThumbnailQualityPreference aQualityPreference, const TSize& aSize, const
    TDisplayMode aDisplayMode, const TInt aPriority, TAny* aClientData, TBool aGeneratePersistentSizesOnly,
    const TDesC& aTargetUri, TThumbnailSize aThumbnailSize)
    {
    iRequestType = EReqGetThumbnailHandleLater;

    iClientData = aClientData;
    iParams.iControlFlags = (aGeneratePersistentSizesOnly ?
                            EThumbnailGeneratePersistentSizesOnly :
                            EThumbnailNoControlFlags);
    iParams.iOriginalControlFlags = iParams.iControlFlags;
    iParams.iBitmapHandle = 0;
    iParams.iSize = aSize;
    iParams.iDisplayMode = aDisplayMode;
    iParams.iFileName = aPath;
    iParams.iRequestId = iRequestId;
    iParams.iPriority = aPriority;
    iParams.iFlags = aFlags;
    iParams.iQualityPreference = aQualityPreference;
    iParams.iThumbnailSize = aThumbnailSize;
    iParams.iThumbnailId = aThumbnailId;
    iParams.iOverwrite = EFalse;
    
    iTargetUri = aTargetUri;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestActive::GetThumbnailL()
// Get a thumbnail for an object file.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::GetThumbnailL( const TDesC& aPath, TThumbnailId aThumbnailId,
    CThumbnailManager::TThumbnailFlags aFlags, CThumbnailManager
    ::TThumbnailQualityPreference aQualityPreference, const TSize& aSize, const
    TDisplayMode aDisplayMode, const TInt aPriority, TAny* aClientData, TBool aGeneratePersistentSizesOnly,
    const TDesC& aTargetUri, TThumbnailSize aThumbnailSize)
    {
    iRequestType = EReqGetThumbnailPath;

    iClientData = aClientData;
    iParams.iControlFlags = (aGeneratePersistentSizesOnly ?
    						EThumbnailGeneratePersistentSizesOnly :
    						EThumbnailNoControlFlags);
    iParams.iOriginalControlFlags = iParams.iControlFlags;
    iParams.iBitmapHandle = 0;
    iParams.iSize = aSize;
    iParams.iDisplayMode = aDisplayMode;
    iParams.iRequestId = iRequestId;
    iParams.iPriority = aPriority;
    iParams.iFlags = aFlags;
    iParams.iQualityPreference = aQualityPreference;
    iParams.iThumbnailSize = aThumbnailSize;
    iParams.iThumbnailId = aThumbnailId;
    iParams.iFileName = aPath;
    iParams.iOverwrite = EFalse;
    
    iPath = aPath;
    iTargetUri = aTargetUri;
    }

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::SetThumbnailL()
// Get a thumbnail for an object file.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::SetThumbnailL( TDesC8* aBuffer, TThumbnailId aThumbnailId,
    const TDesC8& aMimeType, CThumbnailManager::TThumbnailFlags aFlags,
    CThumbnailManager::TThumbnailQualityPreference aQualityPreference, const TSize& aSize,
    const TDisplayMode aDisplayMode, const TInt aPriority, TAny* aClientData,
    TBool aGeneratePersistentSizesOnly, const TDesC& aTargetUri, TThumbnailSize aThumbnailSize,
    TBool aOverwrite)
    {
    iRequestType = EReqSetThumbnailBuffer;

    iClientData = aClientData;
    iParams.iControlFlags = (aGeneratePersistentSizesOnly ?
                            EThumbnailGeneratePersistentSizesOnly :
                            EThumbnailNoControlFlags);
    iParams.iOriginalControlFlags = iParams.iControlFlags;  
    iParams.iMimeType = TDataType( aMimeType );
    iParams.iBitmapHandle = 0;
    iParams.iSize = aSize;
    iParams.iBuffer = aBuffer; // save pointer for deletion (~CThumbnailRequestActive())
    iParams.iThumbnailSize = aThumbnailSize;
    iParams.iDisplayMode = aDisplayMode;
    iParams.iRequestId = iRequestId;
    iParams.iPriority = aPriority;
    iParams.iFlags = aFlags;
    iParams.iQualityPreference = aQualityPreference;
    iParams.iThumbnailId = aThumbnailId;
    iParams.iOverwrite = aOverwrite;
    
    iTargetUri = aTargetUri;
    }

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::SetThumbnailL()
// Get a thumbnail for an object file.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::SetThumbnailL( CFbsBitmap* aBitmap, TThumbnailId aThumbnailId,
    const TDesC8& aMimeType, CThumbnailManager::TThumbnailFlags aFlags,
    CThumbnailManager::TThumbnailQualityPreference aQualityPreference, const TSize& aSize,
    const TDisplayMode aDisplayMode, const TInt aPriority, TAny* aClientData,
    TBool aGeneratePersistentSizesOnly, const TDesC& aTargetUri, TThumbnailSize aThumbnailSize,
    TBool aOverwrite)
    {    
    iClientData = aClientData;
    iParams.iControlFlags = (aGeneratePersistentSizesOnly ?
                            EThumbnailGeneratePersistentSizesOnly :
                            EThumbnailNoControlFlags);
    iParams.iOriginalControlFlags = iParams.iControlFlags;  
    iParams.iBitmapHandle = 0;
    iParams.iSize = aSize;
    iParams.iThumbnailSize = aThumbnailSize;
    iParams.iDisplayMode = aDisplayMode;
    iParams.iRequestId = iRequestId;
    iParams.iPriority = aPriority;
    iParams.iFlags = aFlags;
    iParams.iQualityPreference = aQualityPreference;
    iParams.iThumbnailId = aThumbnailId;
    iParams.iFileName = aTargetUri;
    iParams.iOverwrite = aOverwrite;
    
    iTargetUri = aTargetUri;
    
    TInt memoryFree( 0 );
    HAL::Get( HALData::EMemoryRAMFree, memoryFree );
    
    TN_DEBUG2( "CThumbnaiRequestActive::SetThumbnailbyBitmap() - memoryFree %d", memoryFree );
    
    if(memoryFree > KMemoryNeed)
        {
        // save parameter bitmap, it will be deleted when request is complete
        iBitmap = aBitmap;                   
        iParams.iMimeType = TDataType( aMimeType );
        iRequestType = EReqSetThumbnailBitmap;
        }
    else
        {
        // memory low, create thumbs using filehandle
        TN_DEBUG1( "CThumbnaiRequestActive::SetThumbnailbyBitmap() - memory low, create thumbs using filehandle!" );
        delete aBitmap;
        aBitmap = NULL;
        iParams.iPriority = aPriority - 1;
        iRequestType = EReqGetThumbnailHandleLater;
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::UpdateThumbnailsL()
// Update thumbnails by Id.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::UpdateThumbnailsL( const TDesC& aPath, 
    const TThumbnailId aThumbnailId, CThumbnailManager::TThumbnailFlags aFlags, 
    CThumbnailManager::TThumbnailQualityPreference aQualityPreference, 
    const TDisplayMode aDisplayMode, const TInt aPriority, const TInt aOrientation,
    const TInt64 aModified )
    {
    iRequestType = EReqUpdateThumbnails;
    
    iParams.iControlFlags = EThumbnailGeneratePersistentSizesOnly;   
    iParams.iOriginalControlFlags = iParams.iControlFlags;
    iParams.iBitmapHandle = 0;
    iParams.iDisplayMode = aDisplayMode;
    iParams.iRequestId = iRequestId;
    iParams.iPriority = aPriority;
    iParams.iFlags = aFlags;
    iParams.iQualityPreference = aQualityPreference;
    iParams.iThumbnailId = aThumbnailId;
    iParams.iOverwrite = EFalse;
    
    iPath = aPath;
    iOrientation = aOrientation;
    iModified = aModified;
    }

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::DeleteThumbnails()
// Delete thumbnails.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::DeleteThumbnails( const TDesC& aPath, 
    const TThumbnailId aThumbnailId, const TInt aPriority )
    {
    iRequestType = EReqDeleteThumbnails;
    
    iParams.iRequestId = iRequestId;
    iParams.iPriority = aPriority;
    iParams.iThumbnailId = aThumbnailId;
    
    iPath = aPath;
    }

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::RenameThumbnails()
// Rename thumbnails.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::RenameThumbnails( const TDesC& aCurrentPath, 
        const TDesC& aNewPath, const TInt aPriority )
    {
    iRequestType = EReqRenameThumbnails;
    
    iParams.iRequestId = iRequestId;
    iParams.iPriority = aPriority;
    iParams.iFileName = aCurrentPath;
    iParams.iTargetUri = aNewPath;
    }

// ---------------------------------------------------------------------------
// Request 2nd phase thumbnail
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::Get2ndPhaseThumbnailL()
    {
    TN_DEBUG2( "CThumbnailRequestActive::Get2ndPhaseThumbnailL() %d", iParams.iRequestId );
    
    iParams.iQualityPreference = CThumbnailManager::EOptimizeForQuality;
    iParams.iControlFlags = EThumbnailNoControlFlags;
    
    if ( iMyFileHandle.SubSessionHandle())
        {
        User::LeaveIfError( iMyFileHandle.FullName( iParams.iFileName ));
        iSession.RequestThumbnailL( iMyFileHandle, iParams.iTargetUri, iParamsPckg, iStatus );
        SetActive();
        }
    else
        {
        iSession.RequestThumbnailL( iParams.iFileName, iParams.iTargetUri, iParams.iThumbnailId, iParamsPckg, iStatus );
        SetActive();
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::ChangePriority()
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::ChangePriority( const TInt aNewPriority )
    {
    TN_DEBUG1( "CThumbnailRequestActive::ChangePriority");
    
    iParams.iPriority = aNewPriority;
    
    if (!IsActive())
        {
        this->SetPriority(aNewPriority);
        }
    else
        {
        iSession.ChangePriority(iRequestId, aNewPriority);
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::StartError()
// Error handling function.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestActive::StartError( const TInt aErr )
    {
    TN_DEBUG1( "CThumbnailRequestActive::StartError");
    
    iStartError = aErr;
    iRequestActive = ETrue;
    
    if (!iTimer->IsActive())
        {
        iTimer->Start( KClientRequestStartErrorTimeout, KClientRequestStartErrorTimeout, 
                       TCallBack(TimerCallBack, this));
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailRequestActive::TimerCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbnailRequestActive::TimerCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbnailRequestActive::TimerCallBack() - request timeout");
    
    CThumbnailRequestActive* self = static_cast<CThumbnailRequestActive*>( aAny );
    
    self->iTimer->Cancel();
    
    self->Cancel();
    
    if (self->iStartError != KErrNone)
        {
        self->iError = self->iStartError;
        }
    else
        {
        self->iError = KErrTimedOut;
        }
    
    self->HandleError();
    
    TN_DEBUG1( "CThumbnailRequestActive::TimerCallBack() - end");
    
    return KErrNone;
    }

// End of file
