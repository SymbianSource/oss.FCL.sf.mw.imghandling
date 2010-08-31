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
* Description:  Image thumbnail decoder
 *
*/


//INCLUDE FILES
#include <e32base.h>
#include <imageconversion.h>
#include <ExifRead.h>

#include <IclExtJpegApi.h>
#include "thumbnailimagedecoderv3.h"
#include "thumbnaillog.h"
#include "thumbnailpanic.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailimagedecoderv3Traces.h"
#endif



// CImageDecoder supports up to 1/8 size reduction if EFullyScaleable is
// not set.
const TInt KMaximumReductionFactor = 8;


// ============================ MEMBER FUNCTIONS ===============================

// ---------------------------------------------------------------------------
// CThumbnailImageDecoder::CThumbnailImageDecoder()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailImageDecoderv3::CThumbnailImageDecoderv3( RFs& aFs ): 
    CActive(EPriorityStandard ), iBitmap( NULL ), iFs( aFs ), iBuffer( NULL )
    {
    CActiveScheduler::Add( this );
    }


// ---------------------------------------------------------------------------
// CThumbnailImageDecoderv3::~CThumbnailImageDecoderv3()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailImageDecoderv3::~CThumbnailImageDecoderv3()
    {
    Release();
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoderv3::CreateL()
// Creates thumbnail of image
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv3::CreateL( const TDesC8* aBuffer, MThumbnailProviderObserver&
    aObserver, const CThumbnailManager::TThumbnailFlags /*aFlags*/, const
    TDataType& aMimeType, const TSize& aSize )
    {

    TN_DEBUG1( "CCThumbnailImageDecoderv3::CreateL() called" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODERV3_CREATEL, "CThumbnailImageDecoderv3::CreateL" );

    iSize = aSize;
    iMimeType = aMimeType;
    iObserver = &aObserver;
    iBuffer = aBuffer;

    CreateDecoderL();

    const TFrameInfo info( iDecoder->FrameInfo());
    if (( info.iOverallSizeInPixels.iWidth < 1 ) || (
        info.iOverallSizeInPixels.iHeight < 1 ))
        {
        User::Leave( KErrCorrupt );
        }
    iFrameInfoFlags = info.iFlags;
    iOriginalSize = info.iOverallSizeInPixels; 
    }

// -----------------------------------------------------------------------------
// CThumbnailImageDecoderv3::DecodeL()
// Decode the thumbnail image
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv3::DecodeL( const TDisplayMode aDisplayMode )
    {
    TN_DEBUG1( "CThumbnailImageDecoderv3::DecodeL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODERV3_DECODEL, "CThumbnailImageDecoderv3::DecodeL - start" );
    
    // Create the bitmap
    if ( !iBitmap )
        {
        iBitmap = new( ELeave )CFbsBitmap();
        }

    //Size in both x and y dimension must be non-zero, positive value
    TSize loadSize( iOriginalSize) ;

    // If EFullyScaleable flag is not set, we need to figure out a size
    // reduction factor. 1/1, 1/2, 1/4, and 1/8 are possible values for all
    // plug-ins. SVG graphics can be rendered at any size even though
    // EFullyScaleable is not set.
    if ( !( iFrameInfoFlags& TFrameInfo::EFullyScaleable ) )
        {
        loadSize = iOriginalSize;
        TInt reductionFactor = 1;
        while ( reductionFactor < KMaximumReductionFactor && ( iSize.iWidth <
            loadSize.iWidth / 2 ) && ( iSize.iHeight < loadSize.iHeight / 2 ))
            {
            // magic: use loadSize that is half of previous size
            loadSize.iWidth /= 2;
            loadSize.iHeight /= 2;
            reductionFactor *= 2;
            }
        // If original size is not an exact multiple of reduction factor,
        // we need to round loadSize up
        if ( reductionFactor && iOriginalSize.iWidth % reductionFactor )
            {
            loadSize.iWidth++;
            }
        if ( reductionFactor && iOriginalSize.iHeight % reductionFactor )
            {
            loadSize.iHeight++;
            }
        TN_DEBUG4( 
            "EFullyScaleable not set for image - loadSize=(%d,%d) reduction=1/%d ", loadSize.iWidth, loadSize.iHeight, reductionFactor );
        OstTraceExt3( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODERV3_DECODEL, "CThumbnailImageDecoderv3::DecodeL;loadSize.iWidth=%d;loadSize.iHeight=%d;reductionFactor=%d", loadSize.iWidth, loadSize.iHeight, reductionFactor );
        }

    TInt err = iBitmap->Create( loadSize, aDisplayMode );
    if (err != KErrNone)
        {
        Release();
        User::Leave(err);
        }
    
    iDecoder->Convert( &iStatus, * iBitmap );

    SetActive();
    
    TN_DEBUG1( "CThumbnailImageDecoderv3::DecodeL() end" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILIMAGEDECODERV3_DECODEL, "CThumbnailImageDecoderv3::DecodeL - end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoderv3::Release()
// Releases resources
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv3::Release()
    {
    Cancel();
    
    delete iDecoder;
    iDecoder = NULL;
    
    delete iBitmap;
    iBitmap = NULL;
    delete iBuffer; // we own the buffer
    iBuffer = NULL;
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoderv3::DoCancel()
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv3::DoCancel()
    {
    if ( iDecoder )
        {
        iDecoder->Cancel();
        delete iDecoder;
        iDecoder = NULL;
        }
    
    delete iBitmap;
    iBitmap = NULL;
    delete iBuffer; // we own the buffer
    iBuffer = NULL;
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoderv3::RunL()
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv3::RunL()
    {
    // This call takes ownership of iBitmap
    iObserver->ThumbnailProviderReady( iStatus.Int(), iBitmap, iOriginalSize, EFalse, EFalse );

    iBitmap = NULL; // owned by server now
    delete iBuffer; // we own the buffer
    iBuffer = NULL;
    
    Release();
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoderv3::CreateDecoderL
// Creates image decoder
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv3::CreateDecoderL()
    {
    delete iDecoder;
    iDecoder = NULL;
	
    CImageDecoder::TOptions options = ( CImageDecoder::TOptions )( 
            CImageDecoder::EOptionNoDither );
 
    TRAPD( decErr, iDecoder = CExtJpegDecoder::DataNewL(
            CExtJpegDecoder::EHwImplementation, iFs, *iBuffer, options ));
    
    if ( decErr != KErrNone )
        {
        TN_DEBUG2( "CThumbnailImageDecoderv3::CreateDecoderL() - HW CExtJpegDecoder failed %d", decErr);
        OstTrace1( TRACE_NORMAL, CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL, "CThumbnailImageDecoderv3::CreateDecoderL - HW CExtJpegDecoder failed;decErr=%d", decErr );
        
        LeaveIfCorruptL( decErr );
        
        TRAP( decErr, iDecoder = CExtJpegDecoder::DataNewL(
                CExtJpegDecoder::ESwImplementation, iFs, *iBuffer, options ));
        
        if ( decErr != KErrNone )
            {                             
            TN_DEBUG2( "CThumbnailImageDecoderv3::CreateDecoderL() - SW CExtJpegDecoder failed %d", decErr);
            OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL, "CThumbnailImageDecoderv3::CreateDecoderL - SW CExtJpegDecoder failed;decErr=%d", decErr );
            
            LeaveIfCorruptL( decErr );
            // don't force any mime type
            TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, options ) );
            if ( decErr != KErrNone )
                {
                Release();
                TN_DEBUG2( "CThumbnailImageDecoderv3::CreateDecoderL() - CImageDecoder error %d", decErr );
                OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL, "CThumbnailImageDecoderv3::CreateDecoderL - CImageDecoder error;decErr=%d", decErr );
                
                User::Leave( decErr );
                }     
            
            TN_DEBUG1( "CThumbnailImageDecoderv3::CreateDecoderL() - CImageDecoder created" );
            OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL, "CThumbnailImageDecoderv3::CreateDecoderL - CImageDecoder created" );
            }
        else
            {
            TN_DEBUG1( "CThumbnailImageDecoderv3::CreateDecoderL() - SW CExtJpegDecoder created" );
            OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL, "CThumbnailImageDecoderv3::CreateDecoderL - SW CExtJpegDecoder created" );
            }               
        }
    else
        {
        TN_DEBUG1( "CThumbnailImageDecoderv3::CreateDecoderL() - HW CExtJpegDecoder created" );
        OstTrace0( TRACE_NORMAL, DUP5_CTHUMBNAILIMAGEDECODERV3_CREATEDECODERL, "CThumbnailImageDecoderv3::CreateDecoderL - HW CExtJpegDecoder created" );
        }    
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoderv3::CreateExifDecoderL()
// Returns size of original image
// -----------------------------------------------------------------------------
//
const TSize& CThumbnailImageDecoderv3::OriginalSize()const
    {
    return iOriginalSize;
    }

// -----------------------------------------------------------------------------
// CThumbnailImageDecoder3::LeaveIfCorruptL()
// Leave is image is corrupted
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv3::LeaveIfCorruptL(const TInt aError )
    {
    //no sense to try other codecs if image is corrupted
    if( aError == KErrCorrupt || aError == KErrUnderflow)
        {
        Release();
        User::Leave( aError );
        }
    }

//End of file
