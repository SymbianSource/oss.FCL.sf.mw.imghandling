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
* Description:  Audio thumbnail provider plugin
 *
*/


#include <ecom/ecom.h>
#include <implementationproxy.h>

#include "thumbnailaudioprovider.h"
#include "thumbnailimagedecoderv3.h"
#include "thumbnailmanageruids.hrh"
#include "thumbnaillog.h"
#include <MetaDataUtility.h>
#include <MetaDataFieldContainer.h>
#include "thumbnailmanagerconstants.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailaudioproviderTraces.h"
#endif



#ifndef IMPLEMENTATION_PROXY_ENTRY
typedef TAny* TProxyNewLPtr;
#define IMPLEMENTATION_PROXY_ENTRY(aUid, aFuncPtr) \
{ {aUid}, static_cast<TProxyNewLPtr>(aFuncPtr) }
#endif 


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailImageProvider::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailAudioProvider* CThumbnailAudioProvider::NewL()
    {
    CThumbnailAudioProvider* self = new( ELeave )CThumbnailAudioProvider();
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailAudioProvider::CThumbnailAudioProvider()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailAudioProvider::CThumbnailAudioProvider()
    {
    TN_DEBUG1( "CThumbnailAudioProvider::CThumbnailAudioProvider()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILAUDIOPROVIDER_CTHUMBNAILAUDIOPROVIDER, "CThumbnailAudioProvider::CThumbnailAudioProvider" );
    }


// ---------------------------------------------------------------------------
// CThumbnailAudioProvider::~CThumbnailAudioProvider()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailAudioProvider::~CThumbnailAudioProvider()
    {
    TN_DEBUG1( "CThumbnailAudioProvider::~CThumbnailAudioProvider()" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILAUDIOPROVIDER_CTHUMBNAILAUDIOPROVIDER, "CThumbnailAudioProvider::~CThumbnailAudioProvider" );
    
    delete iImageDecoderv3;
    iImageDecoderv3 = NULL;
    REComSession::DestroyedImplementation( iDtor_ID_Key );
    }


// ---------------------------------------------------------------------------
// CThumbnailAudioProvider::GetThumbnailL()
// Provides the thumbnail image
// ---------------------------------------------------------------------------
//
void CThumbnailAudioProvider::GetThumbnailL( RFs& aFs, RFile64& aFile, const
    TDataType& aMimeType  , const CThumbnailManager::TThumbnailFlags aFlags,
    const TDisplayMode /*aDisplayMode*/, const CThumbnailManager::TThumbnailQualityPreference /*aQualityPreference*/  )
    {   
    TN_DEBUG1( "CThumbnailAudioProvider::GetThumbnailL() - rfile " );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILAUDIOPROVIDER_GETTHUMBNAILL, "CThumbnailAudioProvider::GetThumbnailL - rfile" );
    CMetaDataUtility* metaDataUtil = CMetaDataUtility::NewL();
    CleanupStack::PushL( metaDataUtil );
    
    RArray<TMetaDataFieldId> wantedFields;
    CleanupClosePushL(wantedFields);
    wantedFields.AppendL(EMetaDataJpeg);
    
    metaDataUtil->OpenFileL(aFile, wantedFields, aMimeType.Des8());
    const CMetaDataFieldContainer& metaCont = metaDataUtil->MetaDataFieldsL();
    TPtrC8 ptr = metaCont.Field8( EMetaDataJpeg );
    HBufC8* data = ptr.AllocL();
    
    CleanupStack::PopAndDestroy(&wantedFields);
    CleanupStack::PopAndDestroy(metaDataUtil);
    CleanupStack::PushL( data );
    
    if(data->Length() == 0)
      {
      User::Leave( KErrNotFound );            
      }
    
    if ( !iImageDecoderv3 )
        {
        iImageDecoderv3 = new( ELeave )CThumbnailImageDecoderv3( aFs );
        }
    
    iMimeType = TDataType(KJpegMime);
    iFlags = aFlags;
	//set default mode displaymode from global constants
    iDisplayMode = KStoreDisplayMode;
    TRAPD( err, iImageDecoderv3->CreateL( data, *iObserver, iFlags, iMimeType, iTargetSize ) );
    if (err == KErrNone)
        {
        CleanupStack::Pop( data );
        }
    else
        {
        // this is because data buffer is already released in CreateDecoderL
        // and we must prevent automatic PopAndDestroy
        CleanupStack::Pop( data );
        User::Leave(err);
        } 
    
    iOriginalSize = iImageDecoderv3->OriginalSize();
    iImageDecoderv3->DecodeL( iDisplayMode );
    }

// ---------------------------------------------------------------------------
// CThumbnailAudioProvider::GetThumbnailL()
// Provides the thumbnail image
// ---------------------------------------------------------------------------
//
void CThumbnailAudioProvider::GetThumbnailL( RFs& /* aFs */, TDesC8* /* aBuffer */, const
    TDataType& /*aMimeType */, const CThumbnailManager::TThumbnailFlags /* aFlags */,
    const TDisplayMode /* aDisplayMode */, const CThumbnailManager::TThumbnailQualityPreference /*aQualityPreference*/ )
    {
    TN_DEBUG1( "CThumbnailAudioProvider::GetThumbnailL() - buffer" );
	OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILAUDIOPROVIDER_GETTHUMBNAILL, "CThumbnailAudioProvider::GetThumbnailL - buffer" );
	__ASSERT_DEBUG((EFalse), User::Panic(_L("CThumbnailAudioProvider::GetThumbnailL"), KErrNotSupported));
    }

// ---------------------------------------------------------------------------
// CThumbnailAudioProvider::GetThumbnailL()
// Provides the thumbnail image
// ---------------------------------------------------------------------------
//
void CThumbnailAudioProvider::GetThumbnailL( RFs& /* aFs */, TDesC8& /*aBuffer */)
    {
    TN_DEBUG1( "CThumbnailAudioProvider::GetThumbnailL() - buffer no mime" );
	OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILAUDIOPROVIDER_GETTHUMBNAILL, "CThumbnailAudioProvider::GetThumbnailL - buffer no mime" );
	__ASSERT_DEBUG((EFalse), User::Panic(_L("CThumbnailAudioProvider::GetThumbnailL"), KErrNotSupported));
	User::Leave( KErrNotSupported );
    }

// ---------------------------------------------------------------------------
// Cancel thumbnail request
// ---------------------------------------------------------------------------
//
void CThumbnailAudioProvider::CancelGetThumbnail()
    {
    if ( iImageDecoderv3)
        {
        iImageDecoderv3->Cancel();
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
    IMPLEMENTATION_PROXY_ENTRY( THUMBNAIL_AUDIO_PROVIDER_IMP_UID,
        CThumbnailAudioProvider::NewL )
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

//End of file
