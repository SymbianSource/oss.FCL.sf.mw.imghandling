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


const TInt KThumbnailMimeTypeListGranularity = 8;


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::~CThumbnailManagerImpl()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailManagerImpl::~CThumbnailManagerImpl()
    {
    delete iRequestQueue;   
    iSession.Close();
    iFs.Close();

    // Check if we need to disconnect Fbs
    TInt sessionCount = (TInt)Dll::Tls(); 
    if ( sessionCount > 0)
        { 
        if( --sessionCount == 0 )
            {
		    TN_DEBUG1( "CThumbnailManagerImpl::~CThumbnailManagerImpl() - Disconnect FBS" );
            iFbsSession.Disconnect();
            }
	    TN_DEBUG2( "CThumbnailManagerImpl::~CThumbnailManagerImpl() - update sessionCount == %d to TLS", sessionCount );
        Dll::SetTls( (TAny*)sessionCount );
        }

    delete iMimeTypeList;
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
    aObserver ): iObserver( aObserver ), iDisplayMode(
    KThumbnailDefaultDisplayMode ), iFlags( EDefaultFlags ), iQualityPreference
    ( EOptimizeForQuality ), iRequestId( 0 )
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailManagerImpl::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailManagerImpl::ConstructL()
    {
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
        }
    else
        {
        TInt sessionCount = (TInt)Dll::Tls(); 
        if( sessionCount++ > 0 )
            {
            // Increase the reference count in TLS
            Dll::SetTls( (TAny*)sessionCount );
            TN_DEBUG2( "CThumbnailManagerImpl::ConstructL() - update sessionCount == %d to TLS", sessionCount );
            } 
        else
            {
            // Fbs connection was available in the beginning, no need to
            // increase the reference count
            }
        }
    
    // request processor
    iRequestQueue = CThumbnailRequestQueue::NewL();
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
    
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    if(aObjectSource.Id() > 0)
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.Uri(), aObjectSource.Id(), iFlags,
            iQualityPreference, iSize, iDisplayMode, priority, aClientData, aGeneratePersistentSizesOnly,
            KNullDesC, iThumbnailSize);
        }
    else if ( aObjectSource.Uri().Length())
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.Uri(), aObjectSource.Id(), iFlags,
            iQualityPreference, iSize, iDisplayMode, priority, aClientData, aGeneratePersistentSizesOnly,
            KNullDesC, iThumbnailSize );
        }
    else
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.FileHandle(), aObjectSource.Id(), iFlags,
            iQualityPreference, iSize, iDisplayMode, priority, aClientData, aGeneratePersistentSizesOnly,
            KNullDesC, iThumbnailSize );
        }
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    
    TN_DEBUG2( "CThumbnailManagerImpl::GetThumbnailL() - request ID: %d", iRequestId );
    
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

    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    getThumbnailActive->GetThumbnailL( KNullDesC, aThumbnailId, iFlags,
                       iQualityPreference, iSize, iDisplayMode, priority, aClientData,
                       EFalse, KNullDesC, iThumbnailSize );
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    
    TN_DEBUG2( "CThumbnailManagerImpl::GetThumbnailL() - request ID: %d", iRequestId );
    
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
        ( iFs, iSession, iObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );

    if ( aObjectSource.Uri().Length())
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.Uri(), aObjectSource.Id(), iFlags,
            iQualityPreference, iSize, iDisplayMode, priority, aClientData,
            EFalse, aTargetUri, iThumbnailSize );
        }
    else
        {
        getThumbnailActive->GetThumbnailL( aObjectSource.FileHandle(), aObjectSource.Id(), 
            iFlags, iQualityPreference, iSize, iDisplayMode, priority, aClientData,
            EFalse, aTargetUri, iThumbnailSize );
        }
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    
    TN_DEBUG2( "CThumbnailManagerImpl::ImportThumbnailL() - request ID: %d", iRequestId );
    
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
        ( iFs, iSession, iObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    if ( aObjectSource.Uri().Length() &&
         aObjectSource.Buffer() != NULL &&
         aObjectSource.MimeType() != KNullDesC8)
        {
        getThumbnailActive->SetThumbnailL( aObjectSource.GetBufferOwnership(), aObjectSource.Id(),
            aObjectSource.MimeType(), iFlags, iQualityPreference, iSize, iDisplayMode,
            priority, aClientData, EFalse, aObjectSource.Uri(), iThumbnailSize);
        }
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );

    iRequestQueue->Process();
    
    TN_DEBUG2( "CThumbnailManagerImpl::SetThumbnailL() - request ID: %d", iRequestId );
    
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
		iRequestId++;

		__ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));

		TInt priority = ValidatePriority(aPriority);
		
		CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
			( iFs, iSession, iObserver, iRequestId, priority, iRequestQueue );
		
		CleanupStack::PushL( getThumbnailActive );
		
		if (aObjectSource.Bitmap())
			{
			// from bitmap
			getThumbnailActive->SetThumbnailL( aObjectSource.GetBitmapOwnership(),
						 aObjectSource.Id(), KBmpMime, iFlags, iQualityPreference,
						 iSize, iDisplayMode, priority, NULL, ETrue,
						 aObjectSource.Uri(), EUnknownThumbnailSize);
			}
		else if( !aObjectSource.Buffer() )
			{        
			getThumbnailActive->GetThumbnailL( aObjectSource.Id(), 
						 aObjectSource.Uri(), iFlags, iQualityPreference, iSize,
						 iDisplayMode, priority, NULL, ETrue, aObjectSource.Uri(), 
						 EUnknownThumbnailSize);      
			}
		else
			{
			// from buffer
			getThumbnailActive->SetThumbnailL( aObjectSource.GetBufferOwnership(),
						 aObjectSource.Id(), aObjectSource.MimeType(), iFlags,
						 iQualityPreference, iSize, iDisplayMode, priority, NULL,
						 ETrue, aObjectSource.Uri(), EUnknownThumbnailSize);
			}
		
		iRequestQueue->AddRequestL( getThumbnailActive );
		
		CleanupStack::Pop( getThumbnailActive );
		
		iRequestQueue->Process();
		
		TN_DEBUG2( "CThumbnailManagerImpl::CreateThumbnails() - request ID: %d", iRequestId );
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

		__ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
		
		CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
			( iFs, iSession, iObserver, iRequestId, CActive::EPriorityIdle, iRequestQueue );

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
		
		__ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
		
		CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
			( iFs, iSession, iObserver, iRequestId, CActive::EPriorityIdle, iRequestQueue );
		
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
    
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
    
    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestId, priority, iRequestQueue );
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
    
    __ASSERT_DEBUG(( iRequestId > 0 ), ThumbnailPanic( EThumbnailWrongId ));
    
    TInt priority = ValidatePriority(aPriority);
    
    CThumbnailRequestActive* getThumbnailActive = CThumbnailRequestActive::NewL
        ( iFs, iSession, iObserver, iRequestId, priority, iRequestQueue );
    CleanupStack::PushL( getThumbnailActive );
    
    getThumbnailActive->RenameThumbnails( aCurrentPath, aNewPath, priority );
    
    iRequestQueue->AddRequestL( getThumbnailActive );
    CleanupStack::Pop( getThumbnailActive );
    
    iRequestQueue->Process();
    
    return iRequestId;
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
        return CActive::EPriorityIdle;
        }
    else if (aPriority > CActive::EPriorityHigh)
        {
        TN_DEBUG2( "CThumbnailManagerImpl::ValidatePriority() - priority %d too high for CActive", aPriority );
        return CActive::EPriorityHigh;
        }
    else
        {
        return aPriority;
        }
    }

// End of file
