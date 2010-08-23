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
* Description:  Implementation class of Thumbnail Manager.
 *
*/


#include <e32base.h>
#include <akniconconfig.h>
#include <fbs.h>
#include <badesca.h>
#include <centralrepository.h>

#include <thumbnailmanager.h>

#include "thumbnailmanagerimpl.h"
#include "thumbnailrequestactive.h"
#include "thumbnailprovider.h"
#include "thumbnailsession.h"
#include "thumbnailmanageruids.hrh"
#include "thumbnailmanagerprivatecrkeys.h"
#include "thumbnailpanic.h"

#include "thumbnaildata.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailmanagerimplTraces.h"
#endif


const TInt KThumbnailMimeTypeListGranularity = 8;


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::~CThumbnailManagerImpl()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailManagerImpl::~CThumbnailManagerImpl()
    {
    TN_DEBUG1( "CThumbnailManagerImpl::~CThumbnailManagerImpl() - start" );
	OstTrace0( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL, "CThumbnailManagerImpl::~CThumbnailManagerImpl - start" );
	
    delete iRequestQueue;  
    iRequestQueue = NULL;
    
    iSession.Close();
    iFs.Close();

    // Check if we need to disconnect Fbs
    TInt sessionCount = (TInt)Dll::Tls(); 
    if ( sessionCount > 0)
        { 
        if( --sessionCount == 0 )
            {
		    TN_DEBUG1( "CThumbnailManagerImpl::~CThumbnailManagerImpl() - Disconnect FBS" );
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL, "CThumbnailManagerImpl::~CThumbnailManagerImpl - Disconnect FBS" );
            iFbsSession.Disconnect();
            }
	    TN_DEBUG2( "CThumbnailManagerImpl::~CThumbnailManagerImpl() - update sessionCount == %d to TLS", sessionCount );
        OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL, "CThumbnailManagerImpl::~CThumbnailManagerImpl - update sessionCount == %d to TLS", sessionCount );
        Dll::SetTls( (TAny*)sessionCount );
        }

    delete iMimeTypeList;
    iMimeTypeList = NULL;
	
	TN_DEBUG1( "CThumbnailManagerImpl::~CThumbnailManagerImpl() - end" );
    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL, "CThumbnailManagerImpl::~CThumbnailManagerImpl - end" );
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::NewLC()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailManagerImpl* CThumbnailManagerImpl::NewLC( MThumbnailManagerObserver&
    aObserver )
    {
    CThumbnailManagerImpl* self = new( ELeave )CThumbnailManagerImpl( aObserver );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::CThumbnailManagerImpl()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailManagerImpl::CThumbnailManagerImpl( MThumbnailManagerObserver&
    aObserver ): iObserver( aObserver ), iRequestObserver( NULL ), iDisplayMode(
    KThumbnailDefaultDisplayMode ), iFlags( EDefaultFlags ), iQualityPreference
    ( EOptimizeForQuality ), iRequestId( 0 )
    {
    // No implementation required
    TN_DEBUG1( "CThumbnailManagerImpl::CThumbnailManagerImpl()" );
    OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILMANAGERIMPL_CTHUMBNAILMANAGERIMPL, "CThumbnailManagerImpl::CThumbnailManagerImpl" );
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::ConstructL()
    {
    TN_DEBUG1( "CThumbnailManagerImpl::ConstructL - start");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_CONSTRUCTL, "CThumbnailManagerImpl::ConstructL - start" );
    
    User::LeaveIfError( iSession.Connect());
    User::LeaveIfError( iFs.Connect());
    User::LeaveIfError( iFs.ShareProtected());

    if ( !RFbsSession::GetSession() )
        {
        // We need to connect to Fbs (first user in this thread)
        // Maintain a reference count in TLS
        User::LeaveIfError( iFbsSession.Connect()); 
        Dll::SetTls( (TAny*)1 ); 
        TN_DEBUG2( "CThumbnailManagerImpl::ConstructL() - update sessionCount == %d to TLS", 1 );
        OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILMANAGERIMPL_CONSTRUCTL, "CThumbnailManagerImpl::ConstructL - update sessionCount == %d to TLS", 1 );
        }
    else
        {
        TInt sessionCount = (TInt)Dll::Tls(); 
        if( sessionCount++ > 0 )
            {
            // Increase the reference count in TLS
            Dll::SetTls( (TAny*)sessionCount );
            TN_DEBUG2( "CThumbnailManagerImpl::ConstructL() - update sessionCount == %d to TLS", sessionCount );
            OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILMANAGERIMPL_CONSTRUCTL, "CThumbnailManagerImpl::ConstructL - update sessionCount == %d to TLS", sessionCount );
            } 
        else
            {
            // Fbs connection was available in the beginning, no need to
            // increase the reference count
            TN_DEBUG1( "CThumbnailManagerImpl::ConstructL - no need to update sessionCount");
            OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILMANAGERIMPL_CONSTRUCTL, "CThumbnailManagerImpl::ConstructL - no need to update sessionCount" );
            }
        }
    
    // request processor
    iRequestQueue = CThumbnailRequestQueue::NewL();
    
    TN_DEBUG1( "CThumbnailManagerImpl::ConstructL - end");
    OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILMANAGERIMPL_CONSTRUCTL, "CThumbnailManagerImpl::ConstructL - end" );
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::GetThumbnailL
// Get a thumbnail for an object file.
// ---------------------------------------------------------------------------
//
TThumbnailRequestId CThumbnailManagerImpl::GetThumbnailL(
    CThumbnailObjectSource& aObjectSource, TAny* aClientData /*= NULL*/, const
    TInt aPriority, TBool aGeneratePersistentSizesOnly)
    {
    iRequestId++;
    TN_DEBUG4( "CThumbnailManagerImpl::GetThumbnailL() URI==%S, iThumbnailSize==%d, req %d", &aObjectSource.Uri(), iThumbnailSize, iRequestId );
    OstTraceExt3( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_GETTHUMBNAILL, "CThumbnailManagerImpl::GetThumbnailL;aObjectSource.Uri()=%S;iThumbnailSize=%d;iRequestId=%d", aObjectSource.Uri(), iThumbnailSize, iRequestId );
    
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    if(aObjectSource.Id() > 0)
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.Uri(), aObjectSource.Id(), 
            aObjectSource.MimeType(),iFlags, iQualityPreference, iSize, iDisplayMode, 
            priority, aClientData, aGeneratePersistentSizesOnly, KNullDesC, iThumbnailSize);
        }
    else if ( aObjectSource.Uri().Length())
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.Uri(), aObjectSource.Id(),
            aObjectSource.MimeType(), iFlags, iQualityPreference, iSize, iDisplayMode, 
            priority, aClientData, aGeneratePersistentSizesOnly, KNullDesC, iThumbnailSize );
        }
    else
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.FileHandle(), aObjectSource.Id(), 
            aObjectSource.MimeType(), iFlags, iQualityPreference, iSize, iDisplayMode, 
            priority, aClientData, aGeneratePersistentSizesOnly, KNullDesC, iThumbnailSize );
        }
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    
    TN_DEBUG2( "CThumbnailManagerImpl::GetThumbnailL() - request ID: %d", iRequestId );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILMANAGERIMPL_GETTHUMBNAILL, "CThumbnailManagerImpl::GetThumbnailL;iRequestId=%u", iRequestId );
    
    return iRequestId;
    }
    
// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::GetThumbnailL
// Get a thumbnail for an object file.
// ---------------------------------------------------------------------------
//
TThumbnailRequestId CThumbnailManagerImpl::GetThumbnailL(
    CThumbnailObjectSource& aObjectSource, TAny* aClientData /*= NULL*/, const
    TInt aPriority )
    {
    return GetThumbnailL( aObjectSource, aClientData, aPriority, EFalse );
    }    


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::GetThumbnailL
// Get a thumbnail for an object file.
// ---------------------------------------------------------------------------
//
TThumbnailRequestId CThumbnailManagerImpl::GetThumbnailL( const TThumbnailId
    aThumbnailId, TAny* aClientData /*= NULL*/, TInt aPriority)
    {
    iRequestId++;
    TN_DEBUG4( "CThumbnailManagerImpl::GetThumbnailL() aThumbnailId==%d, iThumbnailSize==%d, req %d", aThumbnailId, iThumbnailSize, iRequestId );
    OstTraceExt3( TRACE_NORMAL, DUP2_CTHUMBNAILMANAGERIMPL_GETTHUMBNAILL, "CThumbnailManagerImpl::GetThumbnailL;aThumbnailId=%u;iThumbnailSize=%u;iRequestId=%u", aThumbnailId, iThumbnailSize, iRequestId );

    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    getThumbnailActive->GetThumbnailL( KNullDesC, aThumbnailId, KNullDesC8,
            iFlags, iQualityPreference, iSize, iDisplayMode, priority, aClientData,
            EFalse, KNullDesC, iThumbnailSize );
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    
    TN_DEBUG2( "CThumbnailManagerImpl::GetThumbnailL() - request ID: %d", iRequestId );
    OstTrace1( TRACE_NORMAL, DUP3_CTHUMBNAILMANAGERIMPL_GETTHUMBNAILL, "CThumbnailManagerImpl::GetThumbnailL;iRequestId=%u", iRequestId );
    
    return iRequestId;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::ImportThumbnailL
// Import an image to be used as thumbnail for an object.
// ---------------------------------------------------------------------------
//
TThumbnailRequestId CThumbnailManagerImpl::ImportThumbnailL(
    CThumbnailObjectSource& aObjectSource, const TDesC& aTargetUri,
    TAny* aClientData /*= NULL*/, const TInt aPriority)
    {
    iRequestId++;

    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );

    if ( aObjectSource.Uri().Length())
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.Uri(), aObjectSource.Id(), 
            aObjectSource.MimeType(), iFlags, iQualityPreference, iSize, iDisplayMode, 
            priority, aClientData, EFalse, aTargetUri, iThumbnailSize );
        }
    else
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.FileHandle(), aObjectSource.Id(), 
            aObjectSource.MimeType(), iFlags, iQualityPreference, iSize, iDisplayMode,
            priority, aClientData, EFalse, aTargetUri, iThumbnailSize );
        }
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    
    TN_DEBUG2( "CThumbnailManagerImpl::ImportThumbnailL() - request ID: %d", iRequestId );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_IMPORTTHUMBNAILL, "CThumbnailManagerImpl::ImportThumbnailL;iRequestId=%u", iRequestId );
    
    return iRequestId;
    }

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::SetThumbnailL
// Import an image to be used as thumbnail for an object.
// ---------------------------------------------------------------------------
//
TThumbnailRequestId CThumbnailManagerImpl::SetThumbnailL( CThumbnailObjectSource& aObjectSource,
    TAny* aClientData, TInt aPriority )
    {
    iRequestId++;

    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    if (aObjectSource.Uri().Length() &&
        aObjectSource.TargetUri().Length())
        {
        // from path
        getThumbnailActive->SetThumbnailL( aObjectSource.Uri(),
            aObjectSource.MimeType(), iFlags, iQualityPreference, iSize, iDisplayMode, 
            priority, aClientData, ETrue, aObjectSource.TargetUri(), iThumbnailSize, ETrue);
        } 
    else if (aObjectSource.Uri().Length() &&
             aObjectSource.Bitmap() != NULL)
        {
        // from bitmap
        getThumbnailActive->SetThumbnailL( aObjectSource.GetBitmapOwnership(),
            aObjectSource.Id(), KBmpMime, iFlags, iQualityPreference, iSize, iDisplayMode,
            priority, aClientData, ETrue, aObjectSource.Uri(), iThumbnailSize, ETrue);
        } 
    else if (aObjectSource.Uri().Length() &&
             aObjectSource.Buffer() != NULL &&
             aObjectSource.MimeType() != KNullDesC8)
        {
        // from buffer
        getThumbnailActive->SetThumbnailL( aObjectSource.GetBufferOwnership(), aObjectSource.Id(),
            aObjectSource.MimeType(), iFlags, iQualityPreference, iSize, iDisplayMode,
            priority, aClientData, ETrue, aObjectSource.Uri(), iThumbnailSize, ETrue);
        }
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );

    iRequestQueue->Process();
    
    TN_DEBUG2( "CThumbnailManagerImpl::SetThumbnailL() - request ID: %d", iRequestId );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_SETTHUMBNAILL, "CThumbnailManagerImpl::SetThumbnailL;iRequestId=%u", iRequestId );
    
    return iRequestId;
    }

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::CreateThumbnails
// Create persistent size thumbnails for an object.
// ---------------------------------------------------------------------------
//
TThumbnailRequestId CThumbnailManagerImpl::CreateThumbnails(
	CThumbnailObjectSource& aObjectSource, TInt aPriority )
	{
	TRAPD(err,
		TN_DEBUG2( "CThumbnailManagerImpl::CreateThumbnails() aObjectSource==%S ", &aObjectSource.Uri() );
		OstTraceExt1( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_CREATETHUMBNAILS, "CThumbnailManagerImpl::CreateThumbnails;aObjectSource.Uri()=%S", aObjectSource.Uri() );
		iRequestId++;

		__ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

		TInt priority = ValidatePriority(aPriority);
		
		CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
			( iFs, iSession, iObserver, iRequestObserver, iRequestId, priority, iRequestQueue );
		
		CleanupStack::PushL( getThumbnailActive );
		
		if (aObjectSource.Bitmap())
			{
			// from bitmap
			getThumbnailActive->SetThumbnailL( aObjectSource.GetBitmapOwnership(),
						 aObjectSource.Id(), KBmpMime, iFlags, iQualityPreference,
						 iSize, iDisplayMode, priority, NULL, ETrue,
						 aObjectSource.Uri(), EUnknownThumbnailSize, EFalse);
			}
		else if( !aObjectSource.Buffer() )
			{        
			getThumbnailActive->GetThumbnailL( aObjectSource.Id(), 
						 aObjectSource.Uri(), aObjectSource.MimeType(), iFlags, 
						 iQualityPreference, iSize, iDisplayMode, priority, NULL, 
						 ETrue, aObjectSource.Uri(), EUnknownThumbnailSize);      
			}
		else
			{
			// from buffer
			getThumbnailActive->SetThumbnailL( aObjectSource.GetBufferOwnership(),
						 aObjectSource.Id(), aObjectSource.MimeType(), iFlags,
						 iQualityPreference, iSize, iDisplayMode, priority, NULL,
						 ETrue, aObjectSource.Uri(), EUnknownThumbnailSize, EFalse);
			}
		
		iRequestQueue->AddRequestL( getThumbnailActive );
		
		CleanupStack::Pop( getThumbnailActive );
		
		iRequestQueue->Process();
		
		TN_DEBUG2( "CThumbnailManagerImpl::CreateThumbnails() - request ID: %d", iRequestId );
		OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILMANAGERIMPL_CREATETHUMBNAILS, "CThumbnailManagerImpl::CreateThumbnails;iRequestId=%u", iRequestId );
	);
	
	if( err != KErrNone)
	    {
	    return err;
	    }
    
    return iRequestId;		
	}


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::DisplayMode()
// Get the current display mode for thumbnail bitmaps.
// ---------------------------------------------------------------------------
//
TDisplayMode CThumbnailManagerImpl::DisplayMode()const
    {
    return iDisplayMode;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::SetDisplayModeL()
// Set the current display mode for thumbnail bitmaps.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::SetDisplayModeL( const TDisplayMode aDisplayMode )
    {
    iDisplayMode = aDisplayMode;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::QualityPreference()
// Get the current quality versus performance preference.
// ---------------------------------------------------------------------------
//
CThumbnailManager::TThumbnailQualityPreference CThumbnailManagerImpl
    ::QualityPreference()const
    {
    return iQualityPreference;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::SetQualityPreferenceL()
// Set quality versus performance preference.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::SetQualityPreferenceL( const
    TThumbnailQualityPreference aQualityPreference )
    {
    iQualityPreference = aQualityPreference;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::ThumbnailSize()
// Get the current desired size for thumbnail bitmaps.
// ---------------------------------------------------------------------------
//
const TSize& CThumbnailManagerImpl::ThumbnailSize()const
    {
    return iSize;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::SetThumbnailSizeL()
// Set desired size for thumbnail bitmaps.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::SetThumbnailSizeL( const TSize& aThumbnailSize )
    {
    iSize = aThumbnailSize;
    iThumbnailSize = ECustomThumbnailSize;
    }

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::SetThumbnailSizeL()
// Set desired size for thumbnail bitmaps.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::SetThumbnailSizeL( const TThumbnailSize aThumbnailSize )
    {
    iThumbnailSize = aThumbnailSize;
    }

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::Flags()
// Get current flags for thumbnail generation.
// ---------------------------------------------------------------------------
//
CThumbnailManager::TThumbnailFlags CThumbnailManagerImpl::Flags()const
    {
    return iFlags;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::SetFlagsL()
// Set flags for thumbnail generation. Several flags may be enabled
// by combining the values using bitwise or.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::SetFlagsL( const TThumbnailFlags aFlags )
    {
    iFlags = aFlags;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::DeleteThumbnails()
// Delete all thumbnails for a given object.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::DeleteThumbnails( CThumbnailObjectSource&
    aObjectSource )
    {
	TRAP_IGNORE(
		iRequestId++;
		TN_DEBUG2( "CThumbnailManagerImpl::DeleteThumbnails() URI==%S ", &aObjectSource.Uri() );
		OstTraceExt1( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_DELETETHUMBNAILS, "CThumbnailManagerImpl::DeleteThumbnails;aObjectSource.Uri()=%S", aObjectSource.Uri() );

		__ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
		
		CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
			( iFs, iSession, iObserver, iRequestObserver, iRequestId, CActive::EPriorityIdle, 
			  iRequestQueue );

		CleanupStack::PushL( getThumbnailActive );
		
		const TDesC& uri = aObjectSource.Uri();
		
		if ( uri.Length())
			{
			getThumbnailActive->DeleteThumbnails( uri, 0, CActive::EPriorityIdle );
			}
		else
			{
			TInt err = aObjectSource.FileHandle().FullName( iFileNameBuf );
			if ( !err )
				{
				getThumbnailActive->DeleteThumbnails( iFileNameBuf, 0, CActive::EPriorityIdle );
				}
			}
		
		iRequestQueue->AddRequestL( getThumbnailActive );
		
		CleanupStack::Pop( getThumbnailActive );
		 
		iRequestQueue->Process();   
	);
    }
    

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::DeleteThumbnailsL()
// Delete thumbnails by TThumbnailId.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::DeleteThumbnails( const TThumbnailId aItemId )
    {
	TRAP_IGNORE(
		iRequestId++;
		TN_DEBUG2( "CThumbnailManagerImpl::DeleteThumbnails() aItemId==%d ", aItemId );
		OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILMANAGERIMPL_DELETETHUMBNAILS, "CThumbnailManagerImpl::DeleteThumbnails;aItemId=%u", aItemId );
		
		__ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
		
		CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
			( iFs, iSession, iObserver, iRequestObserver, iRequestId, CActive::EPriorityIdle, 
			  iRequestQueue );
		
		CleanupStack::PushL( getThumbnailActive );
		
		getThumbnailActive->DeleteThumbnails( KNullDesC, aItemId, CActive::EPriorityIdle );
		
		iRequestQueue->AddRequestL( getThumbnailActive );
		
		CleanupStack::Pop( getThumbnailActive );
		
		iRequestQueue->Process();
	);
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::CancelRequest()
// Cancel a thumbnail operation.
// ---------------------------------------------------------------------------
//
TInt CThumbnailManagerImpl::CancelRequest( const TThumbnailRequestId aId )
    {
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
    
    TN_DEBUG2( "CThumbnailManagerImpl::CancelRequest() - request ID: %d", aId );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_CANCELREQUEST, "CThumbnailManagerImpl::CancelRequest;aId=%u", aId );
    
    return iRequestQueue->CancelRequest(aId);
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::ChangePriority()
// Change the priority of a queued thumbnail operation.
// ---------------------------------------------------------------------------
//
TInt CThumbnailManagerImpl::ChangePriority( const TThumbnailRequestId aId,
    const TInt aNewPriority )
    {
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
    
    TInt priority = ValidatePriority(aNewPriority);
    
    TN_DEBUG2( "CThumbnailManagerImpl::ChangePriority() - request ID: %d", aId );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_CHANGEPRIORITY, "CThumbnailManagerImpl::ChangePriority;aId=%u", aId );
    
    return iRequestQueue->ChangePriority(aId, priority);
    }

// ---------------------------------------------------------------------------
// Get the list of supported file systems from server
// ---------------------------------------------------------------------------
//
const CDesCArray& CThumbnailManagerImpl::GetSupportedMimeTypesL()
    {
    if ( !iMimeTypeList )
        {
        iMimeTypeList = new( ELeave )CDesCArraySeg(
            KThumbnailMimeTypeListGranularity );
        HBufC* buf = iSession.GetMimeTypeListL();
        CleanupStack::PushL( buf );
        TLex lex( *buf );
        while ( !lex.Eos())
            {
            iMimeTypeList->AppendL( lex.NextToken());
            }
        CleanupStack::PopAndDestroy( buf );
        }
    return * iMimeTypeList;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::UpdateThumbnails()
// Update thumbnails by given ID
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::UpdateThumbnailsL( const TThumbnailId aItemId, const TDesC& aPath,
                                               const TInt aOrientation, const TInt64 aModified, 
                                               TInt aPriority )
    {
    iRequestId++;
    TN_DEBUG4( "CThumbnailManagerImpl::UpdateThumbnailsL() URI==%S, aItemId==%d, req %d", &aPath, aItemId, iRequestId); 
    OstTraceExt3( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_UPDATETHUMBNAILSL, "CThumbnailManagerImpl::UpdateThumbnailsL;aPath=%S;aItemId=%d;iRequestId=%d", aPath, aItemId, iRequestId );
    
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
    
    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    getThumbnailActive->UpdateThumbnailsL( aPath, aItemId, iFlags, iQualityPreference,
            iDisplayMode, priority, aOrientation, aModified );
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    }

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::RenameThumbnailsL()
// Renames thumbnails by given path
// ---------------------------------------------------------------------------
//
TThumbnailRequestId CThumbnailManagerImpl::RenameThumbnailsL( const TDesC& aCurrentPath, 
        const TDesC& aNewPath, TInt aPriority )
    {
    iRequestId++;
    TN_DEBUG3( "CThumbnailManagerImpl::RenameThumbnailsL() URI==%S, req %d", &aCurrentPath, iRequestId); 
     OstTraceExt1( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_RENAMETHUMBNAILSL, "CThumbnailManagerImpl::RenameThumbnailsL;aCurrentPath=%S", aCurrentPath );
    
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
    
    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    getThumbnailActive->RenameThumbnails( aCurrentPath, aNewPath, priority );
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    
    return iRequestId;
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::SetRequestObserver()
// Adds optional request observer
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::SetRequestObserver( MThumbnailManagerRequestObserver& aObserver )
    {
    iRequestObserver = NULL;
    iRequestObserver = &aObserver;
    }

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::RemoveRequestObserver()
// Removes optional request observer
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::RemoveRequestObserver()
    {
    if (iRequestObserver)
        {
        iRequestObserver = NULL;
        }
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::ValidatePriority()
// Check that given priority is in range of CActive::TPriority 
// ---------------------------------------------------------------------------
//
TInt CThumbnailManagerImpl::ValidatePriority( const TInt aPriority )
    {
    if (aPriority < CActive::EPriorityIdle)
        {
        TN_DEBUG2( "CThumbnailManagerImpl::ValidatePriority() - priority %d too low for CActive", aPriority );
        OstTrace1( TRACE_NORMAL, CTHUMBNAILMANAGERIMPL_VALIDATEPRIORITY, "CThumbnailManagerImpl::ValidatePriority  - priority too low for CActive;aPriority=%d", aPriority );
        return CActive::EPriorityIdle;
        }
    else if (aPriority > CActive::EPriorityHigh)
        {
        TN_DEBUG2( "CThumbnailManagerImpl::ValidatePriority() - priority %d too high for CActive", aPriority );
        OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILMANAGERIMPL_VALIDATEPRIORITY, "CThumbnailManagerImpl::ValidatePriority - priority too high for CActive;aPriority=%d", aPriority );
        return CActive::EPriorityHigh;
        }
    else
        {
        return aPriority;
        }
    }

// End of file
