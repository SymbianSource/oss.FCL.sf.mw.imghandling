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
#include "thumbnailimagedecoderv2.h"
#include "thumbnaillog.h"
#include "thumbnailpanic.h"
#include "thumbnailmanagerconstants.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailimagedecoderv2Traces.h"
#endif


// ============================ MEMBER FUNCTIONS ===============================
//------------------------------------------------------------------------
// CThumbnailImageDecoder::CThumbnailImageDecoder()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailImageDecoderv2::CThumbnailImageDecoderv2( RFs& aFs): 
    CActive(EPriorityStandard ), iBitmap( NULL ), iFs( aFs ), iBuffer( NULL )
    {
    CActiveScheduler::Add( this );
    }


// ---------------------------------------------------------------------------
// CThumbnailImageDecoder::~CThumbnailImageDecoder()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailImageDecoderv2::~CThumbnailImageDecoderv2()
    {
    Release();
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::CreateL()
// Creates thumbnail of image
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv2::CreateL(TDesC8& aBuffer, MThumbnailProviderObserver& aObserver)
    {
    TN_DEBUG1( "CThumbnailImageDecoderv2::CreateL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODERV2_CREATEL, "CThumbnailImageDecoderv2::CreateL - start" );

    iBuffer = &aBuffer;
    iObserver = &aObserver;

    CreateDecoderL();
    
    TN_DEBUG1( "CThumbnailImageDecoderv2::CreateL() end" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODERV2_CREATEL, "CThumbnailImageDecoderv2::CreateL - end" );
    }

// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::DecodeL()
// Decode the thumbnail image
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv2::DecodeL( )
    {
    TN_DEBUG1( "CThumbnailImageDecoderv2::DecodeL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODERV2_DECODEL, "CThumbnailImageDecoderv2::DecodeL - start" );
    
    // Create the bitmap
    if ( !iBitmap )
        {
        iBitmap = new( ELeave )CFbsBitmap();
        }
    
	//set displaymode from global constants
    TInt err = iBitmap->Create( iDecoder->FrameInfo().iOverallSizeInPixels, iDecoder->FrameInfo().iFrameDisplayMode);
    if (err != KErrNone)
        {
        delete iBitmap;
        iBitmap = NULL;
        User::Leave(err);
        }
    
    iDecoder->Convert( &iStatus, * iBitmap );

    SetActive();  
    
    TN_DEBUG1( "CThumbnailImageDecoderv2::DecodeL() end" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODERV2_DECODEL, "CThumbnailImageDecoderv2::DecodeL - end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::Release()
// Releases resources
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv2::Release()
    {
    Cancel();
    
    delete iDecoder;
    iDecoder = NULL;
    
    delete iBitmap;
    iBitmap = NULL;
    
    iBuffer = NULL; // we don't own the buffer
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::DoCancel()
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv2::DoCancel()
    {
    if ( iDecoder )
        {
        iDecoder->Cancel();
        delete iDecoder;
        iDecoder = NULL;
        }
    
    delete iBitmap;
    iBitmap = NULL;
    
    iBuffer = NULL; // we don't own the buffer
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::RunL()
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv2::RunL()
    {
    // This call takes ownership of iBitmap
    // EXIF always false
    // Rotated always false
    iObserver->ThumbnailProviderReady( iStatus.Int(), iBitmap, iOriginalSize, EFalse, EFalse);

    iBitmap = NULL; // owned by server now
    iBuffer = NULL; // we don't own the buffer
    
    Release();
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::CreateDecoderL
// Creates image decoder
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv2::CreateDecoderL()
    {
    TN_DEBUG1( "CThumbnailImageDecoderv2::CreateDecoderL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - start" );
    
    delete iDecoder;
    iDecoder = NULL;
    
    CImageDecoder::TOptions options = ( CImageDecoder::TOptions )( 
            CImageDecoder::EOptionNoDither );

    TRAPD( decErr, iDecoder = CExtJpegDecoder::DataNewL( 
            CExtJpegDecoder::EHwImplementation, iFs, *iBuffer, options ));
                    
    if ( decErr != KErrNone )
        {
        TN_DEBUG2( "CThumbnailImageDecoderv2::CreateDecoderL() - HW CExtJpegDecoder failed = %d", decErr );
        OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - HW CExtJpegDecoder failed;decErr=%d", decErr );
        TRAP( decErr, iDecoder = CExtJpegDecoder::DataNewL(
                CExtJpegDecoder::ESwImplementation, iFs, *iBuffer, options ));
        
        if ( decErr != KErrNone )
            {
            TN_DEBUG2( "CThumbnailImageDecoderv2::CreateDecoderL() - SW CExtJpegDecoder failed %d", decErr);
            OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - SW CExtJpegDecoder failed;decErr=%d", decErr );
            LeaveIfCorruptL( decErr );
            
            TRAP( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, KJpegMime(), options ) );
            
            if ( decErr != KErrNone )
                {                  
                TN_DEBUG2( "CThumbnailImageDecoderv2::CreateDecoderL() - CImageDecoder failed %d", decErr);
                OstTrace1( TRACE_NORMAL, DUP3_CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - CImageDecoder failed;decErr=%d", decErr );
                LeaveIfCorruptL( decErr );
            
                // don't force any mime type
                TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, options ) );
                if ( decErr != KErrNone )
                    {                    
                    TN_DEBUG2( "CThumbnailImageDecoderv2::CImageDecoder() - CImageDecoder no mime error %d", decErr  );
                    OstTrace1( TRACE_NORMAL, DUP4_CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - CImageDecoder no mime error;decErr=%d", decErr );
                    User::Leave( decErr );
                    }
                }
            
            TN_DEBUG1( "CThumbnailImageDecoderv2::CreateDecoderL() - CImageDecoder created" );
            OstTrace0( TRACE_NORMAL, DUP5_CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - CImageDecoder created" );
            }
        else
            {
            TN_DEBUG1( "CThumbnailImageDecoderv2:CreateDecoderL() - SW CExtJpegDecoder created" );
            OstTrace0( TRACE_NORMAL, DUP6_CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - SW CExtJpegDecoder created" );
            }               
        }
    else
        {
        TN_DEBUG1( "CThumbnailImageDecoderv2::CreateDecoderL() - HW CExtJpegDecoder created" );
        OstTrace0( TRACE_NORMAL, DUP7_CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - HW CExtJpegDecoder created" );
        }
     
    TN_DEBUG1( "CThumbnailImageDecoderv2::CreateDecoderL() end" );
    OstTrace0( TRACE_NORMAL, DUP8_CTHUMBNAILIMAGEDECODERV2_CREATEDECODERL, "CThumbnailImageDecoderv2::CreateDecoderL - end" );
    }

// -----------------------------------------------------------------------------
// CThumbnailImageDecoderv2::LeaveIfCorruptL()
// Leave if image is corrupted
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoderv2::LeaveIfCorruptL(const TInt aError )
    {
    //no sense to try other codecs if image is corrupted
    if( aError == KErrCorrupt || aError == KErrUnderflow)
        {
        User::Leave( aError );
        }
    }

//End of file
