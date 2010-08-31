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
#include <e32math.h>

#include <IclExtJpegApi.h>
#include "thumbnailimagedecoder.h"
#include "thumbnaillog.h"
#include "thumbnailpanic.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailimagedecoderTraces.h"
#endif


const TUid KImageTypeSVGUid = 
    {
    0x102073E7
};

// CImageDecoder supports up to 1/8 size reduction if EFullyScaleable is
// not set.
const TInt KMaximumReductionFactor = 8;

// Matchers for recognizing JPEG files
_LIT( KJpegMime, "image/jpeg" );

// Matcher for recognizing SVG files
_LIT( KSvgMime, "image/svg+xml" );



// ============================ MEMBER FUNCTIONS ===============================

// ---------------------------------------------------------------------------
// CThumbnailImageDecoder::CThumbnailImageDecoder()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailImageDecoder::CThumbnailImageDecoder( RFs& aFs ): 
    CActive(EPriorityStandard ), iBitmap( NULL ), iJpegReadBuffer( NULL ),  
    iExifThumbImage( NULL ), iFs( aFs ), iBuffer( NULL )
    {
    CActiveScheduler::Add( this );
    }


// ---------------------------------------------------------------------------
// CThumbnailImageDecoder::~CThumbnailImageDecoder()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailImageDecoder::~CThumbnailImageDecoder()
    {
    Release();
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::CreateL()
// Creates thumbnail of image
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::CreateL( RFile64& aFile, MThumbnailProviderObserver&
    aObserver, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference, const
    TDataType& aMimeType, const TSize& aSize)
    {
    TN_DEBUG1( "CThumbnailImageDecoder::CreateL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODER_CREATEL, "CThumbnailImageDecoder::CreateL - start" );

    iBuffer = NULL;
    iSize = aSize;
    iMimeType = aMimeType;
    iObserver = &aObserver;
    iFile = aFile;

    CreateDecoderL( aQualityPreference );

    const TFrameInfo info( iDecoder->FrameInfo());
    if (( info.iOverallSizeInPixels.iWidth < 1 ) || (
        info.iOverallSizeInPixels.iHeight < 1 ))
        {
        User::Leave( KErrCorrupt );
        }
    iFrameInfoFlags = info.iFlags;
    iOriginalSize = info.iOverallSizeInPixels;
    
    TN_DEBUG1( "CThumbnailImageDecoder::CreateL() end" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODER_CREATEL, "CThumbnailImageDecoder::CreateL - end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::CreateL()
// Creates thumbnail of image
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::CreateL( const TDesC8* aBuffer, MThumbnailProviderObserver&
    aObserver, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference, const
    TDataType& aMimeType, const TSize& aSize)
    {
    TN_DEBUG1( "CThumbnailImageDecoder::CreateL() start" );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILIMAGEDECODER_CREATEL, "CThumbnailImageDecoder::CreateL - start" );

    iSize = aSize;
    iMimeType = aMimeType;
    iObserver = &aObserver;
    iBuffer = aBuffer;
    
    CreateDecoderL( aQualityPreference );

    const TFrameInfo info( iDecoder->FrameInfo());
    if (( info.iOverallSizeInPixels.iWidth < 1 ) || (
        info.iOverallSizeInPixels.iHeight < 1 ))
        {
        User::Leave( KErrCorrupt );
        }
    iFrameInfoFlags = info.iFlags;
    iOriginalSize = info.iOverallSizeInPixels;
    
    TN_DEBUG1( "CThumbnailImageDecoder::CreateL() end" );
    OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILIMAGEDECODER_CREATEL, "CThumbnailImageDecoder::CreateL - end" );
    }

// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::DecodeL()
// Decode the thumbnail image
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::DecodeL( const TDisplayMode aDisplayMode, const CThumbnailManager::TThumbnailFlags aFlags)
    {
    TN_DEBUG1( "CThumbnailImageDecoder::DecodeL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL - start" );
    
    // Create the bitmap
    if ( !iBitmap )
        {
        iBitmap = new( ELeave )CFbsBitmap();
        }
    
    TN_DEBUG3( "CThumbnailImageDecoder::DecodeL() %d x %d", iSize.iWidth, iSize.iHeight );
    OstTraceExt2( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL;iSize.iWidth=%d;iSize.iHeight=%d", iSize.iWidth, iSize.iHeight );
    if( iOriginalSize.iWidth < iOriginalSize.iHeight )
        {
        TInt height = iSize.iHeight;
        iSize.iHeight = iSize.iWidth;
        iSize.iWidth = height;
        iPortrait = ETrue;
        TN_DEBUG3( "CThumbnailImageDecoder::DecodeL() %d x %d", iSize.iWidth, iSize.iHeight );
        OstTraceExt2( TRACE_NORMAL, DUP2_CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL;iSize.iWidth=%d;iSize.iHeight=%d", iSize.iWidth, iSize.iHeight );
        }
    else
        {
        iPortrait = EFalse;
        }
    
    TN_DEBUG3( "CThumbnailImageDecoder::DecodeL() iOriginalSize = %d x %d", iOriginalSize.iWidth, iOriginalSize.iHeight );
    OstTraceExt2( TRACE_NORMAL, DUP3_CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL;iOriginalSize.iWidth=%d;iOriginalSize.iHeight=%d", iOriginalSize.iWidth, iOriginalSize.iHeight );

    //Size in both x and y dimension must be non-zero, positive value
    TSize loadSize( iOriginalSize) ;
    
    if(iOriginalSize.iHeight < iSize.iHeight || iOriginalSize.iWidth < iSize.iWidth )
        {
        loadSize = iOriginalSize;
        TN_DEBUG1( "CThumbnailImageDecoder::DecodeL() LoadSize is OriginalSize" );
        OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL - LoadSize is OriginalSize" );
        }
    else if((iFrameInfoFlags& TFrameInfo::EFullyScaleable || IsSvg()) && aFlags == !CThumbnailManager::ECropToAspectRatio)
        {
        loadSize = iSize;
        TN_DEBUG1( "CThumbnailImageDecoder::DecodeL() EFullyScaleable start" );
        OstTrace0( TRACE_NORMAL, DUP5_CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL - EFullyScaleable start" );
        const TReal32 srcAspect = static_cast < TReal32 > (
              iOriginalSize.iWidth ) / iOriginalSize.iHeight;

          // set loadsize to maximum size within target size 
          if ( (loadSize.iHeight * srcAspect) <= loadSize.iWidth )
              {
              TReal trg = 0;
              TReal src( loadSize.iHeight * srcAspect );
              Math::Round( trg, src, 0 );
              loadSize.SetSize( trg, loadSize.iHeight );
              }
          else
              {
              TReal trg;
              TReal src( loadSize.iWidth / srcAspect );
              Math::Round( trg, src, 0 );
              loadSize.SetSize( loadSize.iWidth, trg );
              }
        
        TN_DEBUG3( "CThumbnailImageDecoder::DecodeL() EFullyScaleable loadSize = %d x %d", loadSize.iWidth, loadSize.iHeight );
        OstTraceExt2( TRACE_NORMAL, DUP6_CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL - EFullyScaleable;loadSize.iWidth=%d;loadSize.iHeight=%d", loadSize.iWidth, loadSize.iHeight );
        }
    else 
        {
        
        // Size reduction factor. 1/1, 1/2, 1/4, and 1/8 are possible values for all
        // plug-ins. SVG graphics can be rendered at any size.
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
            "CThumbnailImageDecoder::DecodeL() - loadSize = (%d,%d) reduction = 1/%d ", loadSize.iWidth, loadSize.iHeight, reductionFactor );
        OstTraceExt3( TRACE_NORMAL, DUP7_CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL;loadSize.iWidth=%d;loadSize.iHeight=%d;reductionFactor=%d", loadSize.iWidth, loadSize.iHeight, reductionFactor );
        }

    TInt err = iBitmap->Create( loadSize, aDisplayMode );
    if (err != KErrNone)
        {
        delete iBitmap;
        iBitmap = NULL;
        User::Leave(err);
        }

    iDecoder->Convert( &iStatus, * iBitmap );
    
    SetActive();
    
    TN_DEBUG1( "CThumbnailImageDecoder::DecodeL() end" );
    OstTrace0( TRACE_NORMAL, DUP8_CTHUMBNAILIMAGEDECODER_DECODEL, "CThumbnailImageDecoder::DecodeL - end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::Release()
// Releases resources
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::Release()
    {
    Cancel();
   
    delete iDecoder;
    iDecoder = NULL;
    
    delete iBitmap;
    iBitmap = NULL;
    delete iJpegReadBuffer;
    iJpegReadBuffer = NULL;
    delete iExifThumbImage;
    iExifThumbImage = NULL;
    
    iBuffer = NULL; // we don't own the buffer
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::DoCancel()
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::DoCancel()
    {
    if ( iDecoder )
        {
        iDecoder->Cancel();
        delete iDecoder;
        iDecoder = NULL;
        }
    
    delete iBitmap;
    iBitmap = NULL;
    delete iJpegReadBuffer;
    iJpegReadBuffer = NULL;
    delete iExifThumbImage;
    iExifThumbImage = NULL;
    
    iBuffer = NULL; // we don't own the buffer
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::RunL()
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::RunL()
    {
    // This call takes ownership of iBitmap
    iObserver->ThumbnailProviderReady( iStatus.Int(), iBitmap, iOriginalSize, iEXIF, iPortrait );

    iBitmap = NULL; // owned by server now
    iBuffer = NULL; // we don't own the buffer
    
    Release();
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::IsJpeg()
// -----------------------------------------------------------------------------
//
TBool CThumbnailImageDecoder::IsJpeg()
    {
    __ASSERT_DEBUG(( iMimeType.Des() != KNullDesC ), ThumbnailPanic(
        EThumbnailEmptyDescriptor ));

    if ( KJpegMime() == iMimeType.Des())
        {
        return ETrue;
        }
    return EFalse;
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::IsSvg()
// -----------------------------------------------------------------------------
//
TBool CThumbnailImageDecoder::IsSvg()
    {
    __ASSERT_DEBUG(( iMimeType.Des() != KNullDesC ), ThumbnailPanic(
        EThumbnailEmptyDescriptor ));

    if ( KSvgMime() == iMimeType.Des())
        {
        return ETrue;
        }
    return EFalse;
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::CreateDecoderL
// Creates image decoder
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::CreateDecoderL( CThumbnailManager::TThumbnailQualityPreference
    aFlags )
    {
    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - start" );
    
    TBool thumbFound( EFalse );
    
    // If the image is in jpeg format, try to get thumbnail from EXIF data (if EOptimizeForQuality not set)
    if ( IsJpeg() && !( aFlags == CThumbnailManager::EOptimizeForQuality ))
        {
        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() create exif decoder" );
        OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - create exif decoder" );
        TRAPD( err, CreateExifDecoderL( aFlags ));
        thumbFound = ( err == KErrNone );
        iEXIF = ETrue;
        }

    if ( !thumbFound )
        {
        iEXIF = EFalse;
        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() create normal decoder" );
        OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - create normal decoder" );
        
        delete iDecoder;
        iDecoder = NULL;
        
        TFileName fullName;
        if ( !iBuffer )
            {
            iFile.FullName( fullName );
            }
        
        CImageDecoder::TOptions options;
        if ( aFlags == CThumbnailManager::EOptimizeForQuality )
            {
            options = ( CImageDecoder::TOptions )( CImageDecoder
                ::EOptionNoDither );
            }
        else
            {
            options  = ( CImageDecoder::TOptions )( CImageDecoder
                ::EOptionNoDither | CImageDecoder::EPreferFastDecode );
            }

        if ( IsSvg())
            {
            if ( !iBuffer )
                {
                iDecoder = CImageDecoder::FileNewL( iFile, ContentAccess::EPeek, 
                        options, KImageTypeSVGUid, KNullUid, KNullUid );
                
                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                OstTrace0( TRACE_NORMAL, DUP3_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder created" );
                }
            else
                {
                TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, options, KImageTypeSVGUid ) );
                
                if ( decErr != KErrNone )
                    {
                    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - error 1" );
                    OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - error 1" );
                    
                    User::Leave( decErr );
                    }
                
                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                OstTrace0( TRACE_NORMAL, DUP5_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder created" );
                }
            }
        else if ( !IsJpeg())
            {
            if ( !iBuffer )
                {
                iDecoder = CImageDecoder::FileNewL( iFile, ContentAccess::EPeek, options );
                
                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                OstTrace0( TRACE_NORMAL, DUP6_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder created" );
                }
            else
                {
                TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, iMimeType.Des8(), options) );
                
                if ( decErr != KErrNone )
                    {                        
                    TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder error %d", decErr );
                    OstTrace1( TRACE_NORMAL, DUP7_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder error;decErr=%d", decErr );
                    LeaveIfCorruptL(decErr);
                    
                    // don't force any mime type
                    TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, options ) );
                    
                    if ( decErr != KErrNone )
                        {                        
                        TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder no mime error %d", decErr );
                        OstTrace1( TRACE_NORMAL, DUP8_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder no mime error;decErr=%d", decErr );
                        
                        User::Leave( decErr );
                        }
                    }
                
                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                OstTrace0( TRACE_NORMAL, DUP9_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder created" );
                }
            }
        else
            {
            if ( !iBuffer )
                {
                TRAPD( decErr, iDecoder = CExtJpegDecoder::FileNewL(
                        CExtJpegDecoder::EHwImplementation, iFs, fullName, options) );
                
                if ( decErr != KErrNone )
                    {
                    TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - HW CExtJpegDecoder failed %d", decErr);
                    OstTrace1( TRACE_NORMAL, DUP10_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL;decErr=%d - HW CExtJpegDecoder failed ", decErr );
                    LeaveIfCorruptL(decErr);
                    
                    TRAP( decErr, iDecoder = CExtJpegDecoder::FileNewL(
                            CExtJpegDecoder::ESwImplementation, iFs, fullName, options) );
                    
                    if ( decErr != KErrNone )
                        {
                        TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - SW CExtJpegDecoder failed %d", decErr);
                        OstTrace1( TRACE_NORMAL, DUP11_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - SW CExtJpegDecoder failed;decErr=%d", decErr );
                        LeaveIfCorruptL(decErr);
                        
                        TRAP( decErr, iDecoder = CImageDecoder::FileNewL( iFile, ContentAccess::EPeek, options ));
                        
                        if( decErr != KErrNone)
                            {
                            TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder failed %d", decErr);
                            OstTrace1( TRACE_NORMAL, DUP12_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder failed ;decErr=%d", decErr );
                            User::Leave( decErr );
                            }
                        
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                        OstTrace0( TRACE_NORMAL, DUP13_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder created" );
                        }
                    else
                        {
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - SW CExtJpegDecoder created" );
                        OstTrace0( TRACE_NORMAL, DUP14_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - SW CExtJpegDecoder created" );
                        }
                    }
                else 
                    {
                    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - HW CExtJpegDecoder created" );
                    OstTrace0( TRACE_NORMAL, DUP15_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL" );
                    }
                }
            else
                {
                TRAPD( decErr, iDecoder = CExtJpegDecoder::DataNewL(
                        CExtJpegDecoder::EHwImplementation, iFs, *iBuffer, options ));
                
                if ( decErr != KErrNone )
                    {
                    TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - HW CExtJpegDecoder failed %d", decErr);
                    OstTrace1( TRACE_NORMAL, DUP16_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - HW CExtJpegDecoder failed;decErr=%d", decErr );
                    LeaveIfCorruptL(decErr);
                    
                    TRAP( decErr, iDecoder = CExtJpegDecoder::DataNewL(
                            CExtJpegDecoder::ESwImplementation, iFs, *iBuffer, options ));
                    
                    if ( decErr != KErrNone )
                        {                       
                        TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - SW CExtJpegDecoder failed %d", decErr);
                        OstTrace1( TRACE_NORMAL, DUP17_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - SW CExtJpegDecoder failed;decErr=%d", decErr );
                        LeaveIfCorruptL(decErr);
                        TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, iMimeType.Des8(), options) );
                        
                        if ( decErr != KErrNone )
                            {                        
                            TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder failed %d", decErr);
                            OstTrace1( TRACE_NORMAL, DUP18_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder failed;decErr=%d", decErr );
                            LeaveIfCorruptL(decErr);
                            // don't force any mime type
                            TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, options ) );

                            if ( decErr != KErrNone )
                                {                                
                                TN_DEBUG2( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder no mime failed %d", decErr);
                                OstTrace1( TRACE_NORMAL, DUP19_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder no mime failed;decErr=%d", decErr );
                                User::Leave( decErr );
                                }
                            }
                        
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                        OstTrace0( TRACE_NORMAL, DUP20_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - CImageDecoder created" );
                        }
                    else
                        {
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - SW CExtJpegDecoder created" );
                        OstTrace0( TRACE_NORMAL, DUP21_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - SW CExtJpegDecoder created" );
                        }               
                    }
                else
                    {
                    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - HW CExtJpegDecoder created" );
                    OstTrace0( TRACE_NORMAL, DUP22_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - HW CExtJpegDecoder created" );
                    }               
                }
            }
        }
    
    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() end" );
    OstTrace0( TRACE_NORMAL, DUP23_CTHUMBNAILIMAGEDECODER_CREATEDECODERL, "CThumbnailImageDecoder::CreateDecoderL - end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::CreateExifDecoderL()
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::CreateExifDecoderL( CThumbnailManager
    ::TThumbnailQualityPreference aFlags )
    {
    TN_DEBUG1( "CThumbnailImageDecoder::CreateExifDecoderL() start" );
    OstTrace0( TRACE_NORMAL, CTHUMBNAILIMAGEDECODER_CREATEEXIFDECODERL, "CThumbnailImageDecoder::CreateExifDecoderL - start" );
    
    // If the image is in jpeg format, try to get thumbnail from EXIF data.
    CExifRead* reader = NULL;
    
    if ( !iBuffer )
        {    
        TInt64 size( 0 );
        User::LeaveIfError( iFile.Size( size ));
    
        TInt readSize = Min( size, KJpegLoadBufferSize );
    
        delete iJpegReadBuffer;
        iJpegReadBuffer = NULL;
        iJpegReadBuffer = HBufC8::NewL( readSize );
        TPtr8 localBuffer = iJpegReadBuffer->Des();
    
        User::LeaveIfError( iFile.Read( localBuffer, readSize ));
        reader = CExifRead::NewL( localBuffer, CExifRead::ENoJpeg );
        }
    else
        {
        reader = CExifRead::NewL( *iBuffer, CExifRead::ENoJpeg );
        }
    
    CleanupStack::PushL( reader );

    iExifThumbImage = reader->GetThumbnailL();
    CleanupStack::PopAndDestroy( reader );

    User::LeaveIfNull( iExifThumbImage );

    delete iDecoder;
    iDecoder = NULL;

    CImageDecoder::TOptions options;
    if ( aFlags == CThumbnailManager::EOptimizeForQuality )
        {
        options = ( CImageDecoder::TOptions )( CImageDecoder::EOptionNoDither );
        }
    else
        {
        options = ( CImageDecoder::TOptions )( CImageDecoder::EOptionNoDither |
            CImageDecoder::EPreferFastDecode  );
        }

    TRAPD( err, iDecoder = CExtJpegDecoder::DataNewL( iFs, * iExifThumbImage, options ));

    if ( err == KErrNotFound || err == KErrNotSupported )
        {
        delete iDecoder;
        iDecoder = NULL;

        iDecoder = CImageDecoder::DataNewL( iFs, * iExifThumbImage, options );
        }
    else
        {
        TN_DEBUG2( "CThumbnailImageDecoder::CreateExifDecoderL() - CExtJpegDecoder err == %d", err );
        User::LeaveIfError( err );
        }

    TN_DEBUG1( "CThumbnailImageDecoder::CreateExifDecoderL() end" );
    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILIMAGEDECODER_CREATEEXIFDECODERL, "CThumbnailImageDecoder::CreateExifDecoderL - end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::CreateExifDecoderL()
// Returns size of original image
// -----------------------------------------------------------------------------
//
const TSize& CThumbnailImageDecoder::OriginalSize()const
    {
    return iOriginalSize;
    }

// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::LeaveIfCorruptL()
// Leave is image is corrupted
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::LeaveIfCorruptL(const TInt aError )
    {
    //no sense to try other codecs if image is corrupted
    if( aError == KErrCorrupt || aError == KErrUnderflow)
        {
        User::Leave( aError );
        }
    }

//End of file
