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
* Description:  Image thumbnail provider plugin
 *
*/


#include <ecom/ecom.h>
#include <implementationproxy.h>

#include "thumbnailimageprovider.h"
#include "thumbnailimagedecoder.h"
#include "thumbnailimagedecoderv2.h"
#include "thumbnailmanageruids.hrh"
#include "thumbnaillog.h"
#include "thumbnailmanagerconstants.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailimageproviderTraces.h"
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
CThumbnailImageProvider* CThumbnailImageProvider::NewL()
    {
    CThumbnailImageProvider* self = new( ELeave )CThumbnailImageProvider();
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailImageProvider::CThumbnailImageProvider()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailImageProvider::CThumbnailImageProvider()
    {
    TN_DEBUG1( "CThumbnailImageProvider::CThumbnailImageProvider()" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEPROVIDER_CTHUMBNAILIMAGEPROVIDER, "CThumbnailImageProvider::CThumbnailImageProvider" );
    }


// ---------------------------------------------------------------------------
// CThumbnailImageProvider::~CThumbnailImageProvider()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailImageProvider::~CThumbnailImageProvider()
    {
    TN_DEBUG1( "CThumbnailImageProvider::~CThumbnailImageProvider()" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEPROVIDER_CTHUMBNAILIMAGEPROVIDER, "CThumbnailImageProvider::~CThumbnailImageProvider" );
    delete iImageDecoder;
    iImageDecoder = NULL;
    delete iImageDecoderv2;
    iImageDecoderv2 = NULL;
    REComSession::DestroyedImplementation( iDtor_ID_Key );
    }


// ---------------------------------------------------------------------------
// CThumbnailImageProvider::GetThumbnailL()
// Provides the thumbnail image
// ---------------------------------------------------------------------------
//
void CThumbnailImageProvider::GetThumbnailL( RFs& aFs, RFile64& aFile, const
    TDataType& aMimeType, const CThumbnailManager::TThumbnailFlags aFlags,
    const TDisplayMode /*aDisplayMode*/, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference )
    {
    TN_DEBUG1( "CThumbnailImageProvider::GetThumbnailL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEPROVIDER_GETTHUMBNAILL, "CThumbnailImageProvider::GetThumbnailL - start" );

    if ( !iImageDecoder )
        {
        iImageDecoder = new( ELeave )CThumbnailImageDecoder( aFs );
        }

    iMimeType = aMimeType;
    iFlags = aFlags;
    iQualityPreference = aQualityPreference;
	//set default mode displaymode from global constants
    iDisplayMode = KStoreDisplayMode;

//Symbian^4 specific
    if ( KJpegMime() != iMimeType.Des8() ) 
        {
        iDisplayMode = EColor16M;
        }
		
//TODO currently only ARM platforms supports MAP mode
#if !(defined(__CC_ARM) || defined(__ARMCC__))
    iDisplayMode = EColor16M;
#endif	
	
    iImageDecoder->CreateL( aFile, *iObserver, iQualityPreference, iMimeType, iTargetSize );
    iOriginalSize = iImageDecoder->OriginalSize();
    iImageDecoder->DecodeL( iDisplayMode, iFlags );
    
    TN_DEBUG1( "CThumbnailImageProvider::GetThumbnailL() end" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEPROVIDER_GETTHUMBNAILL, "CThumbnailImageProvider::GetThumbnailL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbnailImageProvider::GetThumbnailL()
// Provides the thumbnail image
// ---------------------------------------------------------------------------
//
void CThumbnailImageProvider::GetThumbnailL( RFs& aFs, TDesC8* aBuffer, const
    TDataType& aMimeType, const CThumbnailManager::TThumbnailFlags aFlags,
    const TDisplayMode /*aDisplayMode*/, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference )
    {
    TN_DEBUG1( "CThumbnailImageProvider::GetThumbnailL() start" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILIMAGEPROVIDER_GETTHUMBNAILL, "CThumbnailImageProvider::GetThumbnailL - start" );
    
    if ( !iImageDecoder )
        {
        iImageDecoder = new( ELeave )CThumbnailImageDecoder( aFs );
        }

    iMimeType = aMimeType;
    iFlags = aFlags;
    iQualityPreference = aQualityPreference;
	//set default mode displaymode from global constants
    iDisplayMode = KStoreDisplayMode;
	
//Symbian^4 specific
    if ( KJpegMime() != iMimeType.Des8() ) 
        {
        iDisplayMode = EColor16M;
        }

//TODO currently only ARM platforms supports MAP mode
#if !(defined(__CC_ARM) || defined(__ARMCC__))
    iDisplayMode = EColor16M;
#endif	
		
    iImageDecoder->CreateL( aBuffer, *iObserver, iQualityPreference, iMimeType, iTargetSize );
    iOriginalSize = iImageDecoder->OriginalSize();
    iImageDecoder->DecodeL( iDisplayMode, iFlags );
    
    TN_DEBUG1( "CThumbnailImageProvider::GetThumbnailL() end" );
    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILIMAGEPROVIDER_GETTHUMBNAILL, "CThumbnailImageProvider::GetThumbnailL - end" );
    }

// ---------------------------------------------------------------------------
// CThumbnailImageProvider::GetThumbnailL()
// Provides the thumbnail image
// ---------------------------------------------------------------------------
//
void CThumbnailImageProvider::GetThumbnailL( RFs& aFs, TDesC8& aBuffer)
    {
    TN_DEBUG1( "CThumbnailImageProvider::GetThumbnailL() start" );
    OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILIMAGEPROVIDER_GETTHUMBNAILL, "CThumbnailImageProvider::GetThumbnailL - start" );

    if ( !iImageDecoderv2 )
        {     
        iImageDecoderv2 = new( ELeave )CThumbnailImageDecoderv2( aFs);
        }
    
    iImageDecoderv2->CreateL(aBuffer, *iObserver);
       
    iImageDecoderv2->DecodeL();
    
    TN_DEBUG1( "CThumbnailImageProvider::GetThumbnailL() end" );
    OstTrace0( TRACE_NORMAL, DUP5_CTHUMBNAILIMAGEPROVIDER_GETTHUMBNAILL, "CThumbnailImageProvider::GetThumbnailL - end" );
    }

// ---------------------------------------------------------------------------
// Cancel thumbnail request
// ---------------------------------------------------------------------------
//
void CThumbnailImageProvider::CancelGetThumbnail()
    {
    if ( iImageDecoder )
        {
        iImageDecoder->Cancel();
        }
    if ( iImageDecoderv2)
        {
        iImageDecoderv2->Cancel();
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
    IMPLEMENTATION_PROXY_ENTRY( THUMBNAIL_IMAGE_PROVIDER_IMP_UID,
        CThumbnailImageProvider::NewL )
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
