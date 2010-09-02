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
* Description:  Video thumbnail provider
 *
*/


#include <e32base.h>
#include <ecom/ecom.h>
#include <implementationproxy.h>
#include <fbs.h>

#include "thumbnailvideoprovider.h"
#include "thumbnailmanageruids.hrh"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailvideoproviderTraces.h"
#endif


#ifndef IMPLEMENTATION_PROXY_ENTRY
typedef TAny* TProxyNewLPtr;
#define IMPLEMENTATION_PROXY_ENTRY(aUid, aFuncPtr) \
{ {aUid}, static_cast<TProxyNewLPtr>(aFuncPtr) }
#endif 

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailVideoProvider::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailVideoProvider* CThumbnailVideoProvider::NewL()
    {
    CThumbnailVideoProvider* self = new( ELeave )CThumbnailVideoProvider();
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailImageProvider::CThumbnailImageProvider()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailVideoProvider::CThumbnailVideoProvider():CActive( EPriorityStandard )
    {
    CActiveScheduler::Add( this );
    
    iTimer.CreateLocal();
    iTimeout = EFalse;
    
    TN_DEBUG1( "CThumbnailVideoProvider::CThumbnailVideoProvider()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILVIDEOPROVIDER_CTHUMBNAILVIDEOPROVIDER, "CThumbnailVideoProvider::CThumbnailVideoProvider" );
    }


// ---------------------------------------------------------------------------
// CThumbnailVideoProvider::~CThumbnailVideoProvider()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailVideoProvider::~CThumbnailVideoProvider()
    {
    // run DoCancel even if not active
    if ( IsActive() )
        {
        Cancel();
        }
    else
        {
        DoCancel();
        }
    
    iTimer.Close();
    
    TN_DEBUG1( "CThumbnailVideoProvider::~CThumbnailVideoProvider()" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILVIDEOPROVIDER_CTHUMBNAILVIDEOPROVIDER, "CThumbnailVideoProvider::~CThumbnailVideoProvider" );
    REComSession::DestroyedImplementation( iDtor_ID_Key );
    }
// -----------------------------------------------------------------------------
// CThumbnailVideoProvider::DoCancel()
// -----------------------------------------------------------------------------
//
void CThumbnailVideoProvider::DoCancel()
    {
    iTimer.Cancel();
    
    if ( iVideoClipInfo )
        {
        iVideoClipInfo->CancelThumb();
        
        delete iVideoClipInfo;
        iVideoClipInfo = NULL;
        }
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::RunL()
// -----------------------------------------------------------------------------
//
void CThumbnailVideoProvider::RunL()
    {
    DoCancel();
    
    // video thumbnail engine doesn't respond
    if (iTimeout)
        {
        iObserver->ThumbnailProviderReady( KErrCompletion, NULL, TSize(), EFalse, EFalse);
        iTimeout = EFalse;
        
        TN_DEBUG1( "CThumbnailVideoProvider::RunL() - timeout" );
        OstTrace0( TRACE_NORMAL, CTHUMBNAILVIDEOPROVIDER_RUNL, "CThumbnailVideoProvider::RunL - timeout" );
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailVideoProvider::GetThumbnailL()
// Provides the thumbnail image of video clip
// ---------------------------------------------------------------------------
//
void CThumbnailVideoProvider::GetThumbnailL( RFs&  /*aFs*/, RFile64& aFile, const
    TDataType&  /*aMimeType*/, const CThumbnailManager::TThumbnailFlags aFlags,
    const TDisplayMode /*aDisplayMode*/, const CThumbnailManager::TThumbnailQualityPreference /*aQualityPreference */)
    {
    TN_DEBUG1( "CThumbnailVideoProvider::GetThumbnailL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILVIDEOPROVIDER_GETTHUMBNAILL, "CThumbnailVideoProvider::GetThumbnailL - start" );

    iFlags = aFlags;
	//set default mode displaymode from global constants
    iDisplayMode = KStoreDisplayMode;

//Symbian^4 specific	
//TODO currently only ARM platforms supports MAP mode
#if !(defined(__CC_ARM) || defined(__ARMCC__))
    iDisplayMode = EColor16M;
#endif	

    TFileName filename;
    User::LeaveIfError( aFile.FullName( filename ));

    TRAPD( err, iVideoClipInfo = CTNEVideoClipInfo::NewL( filename, * this ));

    if ( err != KErrNone )
        {
        User::Leave( KErrCorrupt );
        }
    
    TN_DEBUG1( "CThumbnailVideoProvider::GetThumbnailL() end" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILVIDEOPROVIDER_GETTHUMBNAILL, "CThumbnailVideoProvider::GetThumbnailL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbnailVideoProvider::GetThumbnailL()
// ---------------------------------------------------------------------------
//
void CThumbnailVideoProvider::GetThumbnailL( RFs& /*aFs*/, 
    TDesC8* /* aBuffer */,
    const TDataType& /* aMimeType */, 
    const CThumbnailManager::TThumbnailFlags /*aFlags*/,
    const TDisplayMode /* aDisplayMode */, 
    const CThumbnailManager::TThumbnailQualityPreference /*aQualityPreference */)
    {
    TN_DEBUG1( "CThumbnailVideoProvider::GetThumbnailL() - nothing to do" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILVIDEOPROVIDER_GETTHUMBNAILL, "CThumbnailVideoProvider::GetThumbnailL - nothing to do" );
    }

// ---------------------------------------------------------------------------
// CThumbnailVideoProvider::GetThumbnailL()
// ---------------------------------------------------------------------------
//
void CThumbnailVideoProvider::GetThumbnailL( RFs& /*aFs*/, TDesC8& /*aBuffer*/)
    {
    TN_DEBUG1( "CThumbnailVideoProvider::GetThumbnailL() - nothing to do" );
    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILVIDEOPROVIDER_GETTHUMBNAILL, "CThumbnailVideoProvider::GetThumbnailL - nothing to do" );
    User::Leave( KErrNotSupported );
    }

// ---------------------------------------------------------------------------
// Cancel thumbnail request
// ---------------------------------------------------------------------------
//
void CThumbnailVideoProvider::CancelGetThumbnail()
    {
    // run DoCancel even if not active
    if ( IsActive() )
        {
        Cancel();
        }
    else
        {
        DoCancel();
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailVideoProvider::NotifyVideoClipThumbCompleted()
// Used to notify when thumbnail image of video clip is ready
// ---------------------------------------------------------------------------
//
void CThumbnailVideoProvider::NotifyVideoClipThumbCompleted( CTNEVideoClipInfo&
    /*aInfo*/, TInt aError, CFbsBitmap* aThumb )
    {
    TN_DEBUG2( "CThumbnailVideoProvider::NotifyVideoClipThumbCompleted(aError=%d)", aError );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILVIDEOPROVIDER_NOTIFYVIDEOCLIPTHUMBCOMPLETED, "CThumbnailVideoProvider::NotifyVideoClipThumbCompleted;aError=%d", aError );
    
    iTimer.Cancel();
    iTimeout = EFalse;
    
    if (( aError == KErrNone ) && aThumb )
        {
        iOriginalSize = aThumb->SizeInPixels();
        }
    
    iObserver->ThumbnailProviderReady( aError, aThumb, iOriginalSize, EFalse, EFalse);
    }


// ---------------------------------------------------------------------------
// CThumbnailVideoProvider::NotifyVideoClipInfoReady()
// Used to notify when information of video clip is available
// ---------------------------------------------------------------------------
//
void CThumbnailVideoProvider::NotifyVideoClipInfoReady( CTNEVideoClipInfo&
    aInfo, TInt aError )
    {
    TN_DEBUG2( "CThumbnailVideoProvider::NotifyVideoClipInfoReady(aError=%d)", aError );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILVIDEOPROVIDER_NOTIFYVIDEOCLIPINFOREADY, "CThumbnailVideoProvider::NotifyVideoClipInfoReady;aError=%d", aError );
    
    if ( aError == KErrNone )
        {
        TInt err( KErrNone );

        // cannot determine target size, aspect ratio scaling fails
        TRAP( err, aInfo.GetThumbL( *this, KBestThumbIndex, NULL, iDisplayMode, ETrue ));
  
        if ( err != KErrNone )
            {
            TN_DEBUG2( "CThumbnailVideoProvider::NotifyVideoClipInfoReady() -- GetThumbL() left with %d", err );
            OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILVIDEOPROVIDER_NOTIFYVIDEOCLIPINFOREADY, "CThumbnailVideoProvider::NotifyVideoClipInfoReady - GetThumbL() left with;err=%d", err );
            
            iObserver->ThumbnailProviderReady( err, NULL, TSize(), EFalse, EFalse);
            
            SetActive();
            TRequestStatus* statusPtr = &iStatus;
            User::RequestComplete( statusPtr, KErrNone ); 
            }
        else
            {
            // request timeout
            iTimeout = ETrue;
            iTimer.After(iStatus, KVideoDecoderTimeout);
            SetActive();
            }
        }
    else
        {
        iObserver->ThumbnailProviderReady( aError, NULL, TSize(), EFalse, EFalse);
        
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
    }


// ======== GLOBAL FUNCTIONS ========

// -----------------------------------------------------------------------------
// ImplementationTable
// Define the interface UIDs
// -----------------------------------------------------------------------------
//
const TImplementationProxy ImplementationTable[] = 
    {
    IMPLEMENTATION_PROXY_ENTRY( THUMBNAIL_VIDEO_PROVIDER_IMP_UID,
        CThumbnailVideoProvider::NewL )
};


// -----------------------------------------------------------------------------
// ImplementationGroupProxy
// The one and only exported function that is the ECom entry point
// -----------------------------------------------------------------------------
//
EXPORT_C const TImplementationProxy* ImplementationGroupProxy( TInt&
    aTableCount )
    {
    aTableCount = sizeof( ImplementationTable ) / sizeof( TImplementationProxy );

    return ImplementationTable;
    }

// End of file
