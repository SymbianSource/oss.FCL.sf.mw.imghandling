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
#include <exifread.h>
#include <e32math.h>

#include <iclextjpegapi.h>
#include "thumbnailimagedecoder.h"
#include "thumbnaillog.h"
#include "thumbnailpanic.h"

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
CThumbnailImageDecoder::CThumbnailImageDecoder( RFs& aFs ): CActive(
    EPriorityStandard ), iFs( aFs )
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
    }

// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::DecodeL()
// Decode the thumbnail image
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::DecodeL( const TDisplayMode aDisplayMode, const CThumbnailManager::TThumbnailFlags aFlags)
    {
    TN_DEBUG1( "CThumbnailImageDecoder::DecodeL() start" );
    
    // Create the bitmap
    if ( !iBitmap )
        {
        iBitmap = new( ELeave )CFbsBitmap();
        }
    
    TN_DEBUG3( "CThumbnailImageDecoder::DecodeL() %d x %d", iSize.iWidth, iSize.iHeight );
    if( iOriginalSize.iWidth < iOriginalSize.iHeight )
        {
        TInt height = iSize.iHeight;
        iSize.iHeight = iSize.iWidth;
        iSize.iWidth = height;
        iPortrait = ETrue;
        TN_DEBUG3( "CThumbnailImageDecoder::DecodeL() %d x %d", iSize.iWidth, iSize.iHeight );
        }
    else
        {
        iPortrait = EFalse;
        }
    
    TN_DEBUG3( "CThumbnailImageDecoder::DecodeL() iOriginalSize = %d x %d", iOriginalSize.iWidth, iOriginalSize.iHeight );

    //Size in both x and y dimension must be non-zero, positive value
    TSize loadSize( iOriginalSize) ;
    
    
    if(iOriginalSize.iHeight < iSize.iHeight || iOriginalSize.iWidth < iSize.iWidth )
        {
        loadSize = iOriginalSize;
        TN_DEBUG1( "CThumbnailImageDecoder::DecodeL() LoadSize is OriginalSize" );
        }
    else if((iFrameInfoFlags& TFrameInfo::EFullyScaleable || IsSvg()) && aFlags == !CThumbnailManager::ECropToAspectRatio)
        {
        loadSize = iSize;
        TN_DEBUG1( "CThumbnailImageDecoder::DecodeL() EFullyScaleable start" );
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
        }

    User::LeaveIfError( iBitmap->Create( loadSize, aDisplayMode ));

    iDecoder->Convert( &iStatus, * iBitmap );
    while ( iStatus == KErrUnderflow )
        {
        iDecoder->ContinueConvert( &iStatus );
        }
    
    SetActive();
    
    TN_DEBUG1( "CThumbnailImageDecoder::DecodeL() end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::Release()
// Releases resources
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::Release()
    {
    Cancel();
    delete iJpegReadBuffer;
    iJpegReadBuffer = NULL;
    delete iExifThumbImage;
    iExifThumbImage = NULL;
    delete iDecoder;
    iDecoder = NULL;
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
        delete iJpegReadBuffer;
        iJpegReadBuffer = NULL;
        delete iExifThumbImage;
        iExifThumbImage = NULL;
        delete iDecoder;
        iDecoder = NULL;
        }
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
    
    TBool thumbFound( EFalse );
    
    // If the image is in jpeg format, try to get thumbnail from EXIF data (if EOptimizeForQuality not set)
    if ( IsJpeg() && !( aFlags == CThumbnailManager::EOptimizeForQuality ))
        {
        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() crete exif decoder" );
        TRAPD( err, CreateExifDecoderL( aFlags ));
        thumbFound = ( err == KErrNone );
        iEXIF = ETrue;
        }

    if ( !thumbFound )
        {
        iEXIF = EFalse;
        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() crete normal decoder" );
        
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
            options = ( CImageDecoder::TOptions )( CImageDecoder
                ::EOptionNoDither | CImageDecoder::EPreferFastDecode );
            }

        if ( IsSvg())
            {
            if ( !iBuffer )
                {
                iDecoder = CImageDecoder::FileNewL( iFile, ContentAccess::EPeek, 
                        options, KImageTypeSVGUid, KNullUid, KNullUid );
                
                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                }
            else
                {
                TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, options, KImageTypeSVGUid ) );
                
                if ( decErr != KErrNone )
                    {
                    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - error 1" );
                    
                    User::Leave( decErr );
                    }
                
                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                }
            }
        else if ( !IsJpeg())
            {
            if ( !iBuffer )
                {
                iDecoder = CImageDecoder::FileNewL( iFile, ContentAccess::EPeek, options );
                
                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                }
            else
                {
                TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, iMimeType.Des8(), options) );
                
                if ( decErr != KErrNone )
                    {                        
                    // don't force any mime type
                    TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, options ) );
                    if ( decErr != KErrNone )
                        {                        
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - error 2" );
                        
                        User::Leave( decErr );
                        }
                    }
                
                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
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
                    TRAP( decErr, iDecoder = CExtJpegDecoder::FileNewL(
                            CExtJpegDecoder::ESwImplementation, iFs, fullName, options) );
                    
                    if ( decErr != KErrNone )
                        {
                        iDecoder = CImageDecoder::FileNewL( iFile, ContentAccess::EPeek, options );
                        
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                        }
                    else
                        {
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - SW CExtJpegDecoder created" );
                        }
                    }
                else 
                    {
                    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - HW CExtJpegDecoder created" );
                    }
                }
            else
                {
                TRAPD( decErr, iDecoder = CExtJpegDecoder::DataNewL(
                        CExtJpegDecoder::EHwImplementation, iFs, *iBuffer, options ));
                
                if ( decErr != KErrNone )
                    {
                    TRAP( decErr, iDecoder = CExtJpegDecoder::DataNewL(
                            CExtJpegDecoder::ESwImplementation, iFs, *iBuffer, options ));
                    
                    if ( decErr != KErrNone )
                        {                        
                        TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, iMimeType.Des8(), options) );
                        
                        if ( decErr != KErrNone )
                            {                        
                            // don't force any mime type
                            TRAPD( decErr, iDecoder = CImageDecoder::DataNewL( iFs, *iBuffer, options ) );
                            if ( decErr != KErrNone )
                                {                                
                                TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - error 3" );
                                
                                User::Leave( decErr );
                                }
                            }
                        
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - CImageDecoder created" );
                        }
                    else
                        {
                        TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - SW CExtJpegDecoder created" );
                        }               
                    }
                else
                    {
                    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() - HW CExtJpegDecoder created" );
                    }               
                }
            }
        }
    
    TN_DEBUG1( "CThumbnailImageDecoder::CreateDecoderL() end" );
    }


// -----------------------------------------------------------------------------
// CThumbnailImageDecoder::CreateExifDecoderL()
// -----------------------------------------------------------------------------
//
void CThumbnailImageDecoder::CreateExifDecoderL( CThumbnailManager
    ::TThumbnailQualityPreference aFlags )
    {
    TN_DEBUG1( "CThumbnailImageDecoder::CreateExifDecoderL() start" );
    
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
            CImageDecoder::EPreferFastDecode );
        }

    TRAPD( err, iDecoder = CExtJpegDecoder::DataNewL( iFs, * iExifThumbImage,
        options ));

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

/*
    // If the Exif thumbnail is smaller than requested it will not be used
    TFrameInfo frame = iDecoder->FrameInfo( 0 );
    
    if ( frame.iOverallSizeInPixels.iWidth < iSize.iWidth ||
        frame.iOverallSizeInPixels.iHeight < iSize.iHeight ) 
        {
        User::Leave( KErrGeneral );
        }
    */
    TN_DEBUG1( "CThumbnailImageDecoder::CreateExifDecoderL() end" );
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

//End of file
