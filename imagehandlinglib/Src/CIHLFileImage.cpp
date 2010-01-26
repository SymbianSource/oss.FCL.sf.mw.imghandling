/*
* Copyright (c) 2004 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Implementation of Image class.
*
*/


// INCLUDE FILES
#include "CIHLFileImage.h"

#include "CIHLBitmap.h"
#include "IHLImplementationIds.h"
#include "IHLDebugPrint.h" // Debug print
#include <IHLInterfaceIds.h>

// Private namespace for constants and functions
namespace
	{
	// Fixed scale factors
	enum TScaleFactors
		{
		EFull		= 1,
		EHalf		= 2,
		EQuarter	= 4,
		EEighth		= 8,
		};

	// Panic function
    _LIT( KIHLPanicString, "IHLImage" );
    void Panic( TInt aPanicCode ) { User::Panic( KIHLPanicString, aPanicCode ); }
	}

// ============================ MEMBER FUNCTIONS ===============================
// -----------------------------------------------------------------------------
//
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
CIHLFileImage::CIHLFileImage( TInt aImageIndex )
:CActive( CActive::EPriorityStandard ),
iImageIndex( aImageIndex )
    {
	CActiveScheduler::Add( this );
    }

// -----------------------------------------------------------------------------
//
// Two-phased constructor.
// -----------------------------------------------------------------------------
CIHLFileImage* CIHLFileImage::NewL( RFile& aFile, TInt aImageIndex, const TUint32 aOptions )
	{
    CIHLFileImage* self = new (ELeave) CIHLFileImage( aImageIndex );
	CleanupStack::PushL( self );
	self->ConstructL( aFile, aOptions );
	CleanupStack::Pop(); // self
    return self;
	}

CIHLFileImage* CIHLFileImage::NewL( RFs& aFs, const TDesC8& aDataBuf,
                                    TInt aImageIndex, const TUint32 aOptions )
    {
    CIHLFileImage* self = new (ELeave) CIHLFileImage( aImageIndex );
	CleanupStack::PushL( self );
	self->ConstructL( aFs, aDataBuf, aOptions );
	CleanupStack::Pop(); // self
    return self;
    }

// -----------------------------------------------------------------------------
//
// Symbian constructor can leave.
// -----------------------------------------------------------------------------
void CIHLFileImage::ConstructL( RFile& aFile, const TUint32 aOptions )
	{
    TInt decoderOptions( CImageDecoder::EOptionNoDither | CImageDecoder::EOptionUseFrameSizeInPixels );

	// Open decoder
    IHL_DEBUG1( KIHLDebug1, "IHL - CIHLFileImage - Start create ICL image decoder" );
	if( aOptions & MIHLFileImage::EOptionNoDRMConsume )
	    {
	    iDecoder = CImageDecoder::FileNewL( aFile, ContentAccess::EPeek,
	                             (CImageDecoder::TOptions)decoderOptions );
	    }
	else
	    {
	    iDecoder = CImageDecoder::FileNewL( aFile, ContentAccess::EView,
	                             (CImageDecoder::TOptions)decoderOptions );
	    }
	ConstructCommonL();
    IHL_DEBUG1( KIHLDebug2, "IHL - CIHLFileImage - ICL image decoder ready!" );
	}

void CIHLFileImage::ConstructL( RFs& aFs, const TDesC8& aDataBuf, const TUint32 /*aOptions*/ )
    {
    TInt decoderOptions( CImageDecoder::EOptionNoDither | CImageDecoder::EOptionUseFrameSizeInPixels );

    IHL_DEBUG1( KIHLDebug1, "IHL - CIHLFileImage - Start create buffered ICL image decoder" );

    iDecoder = CImageDecoder::DataNewL( aFs, aDataBuf, (CImageDecoder::TOptions)decoderOptions );
	ConstructCommonL();

    IHL_DEBUG1( KIHLDebug2, "IHL - CIHLFileImage - Buffered ICL image decoder ready!" );
    }


// -----------------------------------------------------------------------------
// CIHLFileImage::ConstructCommonL
// -----------------------------------------------------------------------------

void CIHLFileImage::ConstructCommonL()
    {
	// Check frame count and image index
	iImageCount = iDecoder->FrameCount();
	__ASSERT_ALWAYS( iImageCount > 0, User::Leave( KErrCorrupt ) );

	// Get image types
	iDecoder->ImageType( iImageIndex, iImageType, iImageSubType );

	if( KImageTypeGIFUid == iImageType )
		{
		iGif = ETrue;
		if( iImageCount > 1 )
		    {
    		iAnimation = ETrue;
    		iAnimationFrameCount = iImageCount;
    		iImageCount = 1; // Handled as one animated image
		    }
		}
	__ASSERT_ALWAYS( iImageIndex >= 0 && iImageIndex < iImageCount, User::Leave( KErrArgument ) );

	// cache frame info and set scale sizes
	iFrameInfo = iDecoder->FrameInfo( iImageIndex );
	if( !iAnimation )
		{
        // Animation must always be loaded 1:1
		if( !iGif &&
		    iFrameInfo.iFlags & TFrameInfo::EFullyScaleable )
			{
			// Gif cannot be fully scaleable
			iFullyScaleable = ETrue;
			}
		else
			{
			TSize size( iFrameInfo.iOverallSizeInPixels );
			iLoadSizeArray.AppendL( ScaledLoadSize( size, EEighth ) );
			iLoadSizeArray.AppendL( ScaledLoadSize( size, EQuarter ) );
			iLoadSizeArray.AppendL( ScaledLoadSize( size, EHalf ) );
			}
		}
    }

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
CIHLFileImage::~CIHLFileImage()
    {
	Cancel();
	delete iPrevAnimationFrame;
	delete iSubFrameBitmap;
	delete iDecoder;
	iLoadSizeArray.Reset();
    }

// -----------------------------------------------------------------------------
// CIHLFileImage::Type
// -----------------------------------------------------------------------------
TIHLInterfaceType CIHLFileImage::Type() const
	{
	return TIHLInterfaceType( KIHLInterfaceIdFileImage,
							  KIHLImplementationIdFileImage );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::ImageType
// -----------------------------------------------------------------------------
const TUid& CIHLFileImage::ImageType() const
	{
	return iImageType;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::ImageSubType
// -----------------------------------------------------------------------------
const TUid& CIHLFileImage::ImageSubType() const
	{
	return iImageSubType;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::ImageIndex
// -----------------------------------------------------------------------------
TInt CIHLFileImage::ImageIndex() const
	{
	return iImageIndex;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::ImageCount
// -----------------------------------------------------------------------------
TInt CIHLFileImage::ImageCount() const
	{
	return iImageCount;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::Size
// -----------------------------------------------------------------------------
TSize CIHLFileImage::Size() const
	{
	return iFrameInfo.iOverallSizeInPixels;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::DisplayMode
// -----------------------------------------------------------------------------
TDisplayMode CIHLFileImage::DisplayMode() const
	{
	if( iGif )
		{
		// We cannot trust iFrameDisplayMode for GIF images. It always return EColor256.
		// This is error because palette sure holds only 256 colors but these colors can
		// be still any RGB values and so for cannot be directly put to 8 bit bitmap (EColor256).
		// To decrypt image correctly and without color dithering, we must use 24 bit (EColor16M)
		// destination bitmap. Note that CFbsBitmap has palette methods but they are
		// not supported currently.
		// Return maximum color mode to ensure best image quality.
		return EColor16MU;
		}
	else if( iFrameInfo.iFrameDisplayMode < EColor16MU ||
	         iFrameInfo.iFrameDisplayMode == EColor4K )
		{
		return EColor64K;
		}
	return EColor16MU;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::MaskDisplayMode
// -----------------------------------------------------------------------------
TDisplayMode CIHLFileImage::MaskDisplayMode() const
	{
	if( iFrameInfo.iFlags & TFrameInfo::ETransparencyPossible )
		{
		if( iFrameInfo.iFlags & TFrameInfo::EAlphaChannel )
			{
			return EGray256;
			}
		return EGray2;
		}
	return ENone;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::BackgroundColor
// -----------------------------------------------------------------------------
TRgb CIHLFileImage::BackgroundColor() const
	{
	return iFrameInfo.iBackgroundColor;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::CustomLoadSizeArray
// -----------------------------------------------------------------------------
const RArray<TSize>& CIHLFileImage::CustomLoadSizeArray() const
	{
	return iLoadSizeArray;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::CustomLoadSizeArray
// -----------------------------------------------------------------------------
TBool CIHLFileImage::IsFullyScaleable() const
	{
	return iFullyScaleable;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::IsAnimation
// -----------------------------------------------------------------------------
TBool CIHLFileImage::IsAnimation() const
	{
	return iAnimation;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::AnimationFrameCount
// -----------------------------------------------------------------------------
TInt CIHLFileImage::AnimationFrameCount() const
	{
	return iAnimationFrameCount;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::AnimationFrameDelay
// -----------------------------------------------------------------------------
TTimeIntervalMicroSeconds32 CIHLFileImage::AnimationFrameDelay( TInt aAnimationFrameIndex ) const
	{
	__ASSERT_ALWAYS( aAnimationFrameIndex >= 0 &&
		aAnimationFrameIndex < iAnimationFrameCount, Panic( KErrArgument ) );

	return I64INT( iDecoder->FrameInfo( aAnimationFrameIndex ).iDelay.Int64() );
	}

// ------------------------------------------------------------------------------
// CIHLFileImage::Load
// ------------------------------------------------------------------------------

TInt CIHLFileImage::Load( TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex )
	{
	iImageIndex = aFrameIndex;	
	return LoadRequest( aStatus, aDestination, iImageIndex );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::LoadAnimation
// -----------------------------------------------------------------------------
TInt CIHLFileImage::LoadAnimation( TRequestStatus& aStatus, MIHLBitmap& aDestination,
							  TInt aAnimationFrameIndex )
	{
	__ASSERT_ALWAYS( aAnimationFrameIndex >= 0 &&
		aAnimationFrameIndex < iAnimationFrameCount, Panic( KErrArgument ) );

	return LoadRequest( aStatus, aDestination, aAnimationFrameIndex );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::IsBusy
// -----------------------------------------------------------------------------
TBool CIHLFileImage::IsBusy() const
	{
	return ( iImageState != EInactive );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::CancelLoad
// -----------------------------------------------------------------------------
void CIHLFileImage::CancelLoad()
	{
	Cancel();
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::SetFilter
// -----------------------------------------------------------------------------
void CIHLFileImage::SetFilter( MIHLFilter* /*aFilter*/ )
	{
	// Not in use
	}


// -----------------------------------------------------------------------------
// CIHLFileImage::Decoder
// -----------------------------------------------------------------------------
const CImageDecoder& CIHLFileImage::Decoder() const
	{
	return *iDecoder;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::DoCancel
// -----------------------------------------------------------------------------
void CIHLFileImage::DoCancel()
	{
	iDecoder->Cancel();

	// Delete all processed bitmaps
	ErrorCleanup();

	// Complete with cancel
	iImageState = EInactive;
	RequestComplete( KErrCancel );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::RunL
// -----------------------------------------------------------------------------
void CIHLFileImage::RunL()
	{
	__ASSERT_DEBUG( iDestination, Panic( KErrGeneral ) );
	User::LeaveIfError( iStatus.Int() );

	switch( iImageState )
		{
		case EStartLoad:
			{
			// start loading the bitmaps
			StartLoadL();
			break;
			}
		case ECompleteLoad:
			{
			// complete loading the bitmaps
			CompleteLoadL();
			break;
			}
		default:
			{
			Panic( KErrTotalLossOfPrecision );
			}
		}
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::RunError
// -----------------------------------------------------------------------------
TInt CIHLFileImage::RunError( TInt aError )
	{
    IHL_DEBUG2( KIHLDebug, "IHL - CIHLFileImage - Loading error: %d", aError );

	// Delete all processed bitmaps
	ErrorCleanup();

	// Complete with error
	iImageState = EInactive;
	RequestComplete( aError );
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::LoadRequest
// -----------------------------------------------------------------------------
TInt CIHLFileImage::LoadRequest( TRequestStatus& aStatus,
                                 MIHLBitmap& aDestination,
                                 TInt aFrameIndex )
	{
	if( IsBusy() )
		{
		return KErrNotReady;
		}

	if( aFrameIndex < 0 || aFrameIndex >= iDecoder->FrameCount() )
		{
		return KErrArgument;
		}

	const CFbsBitmap& dstBitmap = aDestination.Bitmap();
	if( !dstBitmap.Handle() )
		{
		return KErrArgument;
		}

	TSize dstSize( dstBitmap.SizeInPixels() );
	if( dstSize != Size() &&
		!iFullyScaleable )
		{
		TBool sizeFound( EFalse );
		const TInt count( iLoadSizeArray.Count() );
		for( TInt i( 0 ); i < count; ++i )
			{
			if( dstSize == iLoadSizeArray[ i ] )
				{
				sizeFound = ETrue;
				}
			}
		if( !sizeFound )
			{
			return KErrArgument;
			}
		}

    IHL_DEBUG1( KIHLDebug, "IHL - CIHLFileImage - Frame loading requested" );

	iImageStatus = &aStatus;
	*iImageStatus = KRequestPending;

	iDestination = static_cast<CIHLBitmap*>( &aDestination ); //lint !e826
	iFrameIndex = aFrameIndex;

	// Start the active object
	iImageState = EStartLoad;
	SelfComplete();
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::StartLoadL
// -----------------------------------------------------------------------------
void CIHLFileImage::StartLoadL()
	{
	__ASSERT_DEBUG( !iSubFrameBitmap, Panic( KErrGeneral ) );

    IHL_DEBUG1( KIHLDebug, "IHL - CIHLFileImage - Start ICL convert" );

	if( iAnimation )
		{
		// Start animation from first frame by default
		iSubFrameIndex = 0;

		// Check is animation can be continued on top of destination bitmap
		if( iDestination->IsCreated() &&
			iDestination->EditorPtr() == this &&
			iDestination->EditorValue() < iFrameIndex )
			{
			// Editor value means frame index
			iSubFrameIndex = iDestination->EditorValue() + 1;
			}

		StartLoadSubFrameL( iSubFrameIndex, ETrue );
		}
    else if( iGif )
        {
		StartLoadSubFrameL( iFrameIndex, EFalse );
        }
	else
		{
        // Frame fills the whole image -> normal load
	    StartLoadNormalFrame( iFrameIndex );
		}

	iImageState = ECompleteLoad;
	SetActive();
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::StartLoadNormalFrame
// -----------------------------------------------------------------------------
void CIHLFileImage::StartLoadNormalFrame( TInt aFrameIndex )
	{
	CFbsBitmap& dstBitmap = iDestination->BitmapModifyable();
	CFbsBitmap& dstMask = iDestination->MaskModifyable();

	if( MaskDisplayMode() && dstMask.Handle() )
		{
		iDecoder->Convert( &iStatus, dstBitmap, dstMask, aFrameIndex );
		}
	else
		{
		dstMask.Reset();
		iDecoder->Convert( &iStatus, dstBitmap, aFrameIndex );
		}
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::StartLoadSubFrameL
// -----------------------------------------------------------------------------
void CIHLFileImage::StartLoadSubFrameL( TInt aFrameIndex, TBool aAnimation )
	{
	__ASSERT_DEBUG( !iSubFrameBitmap, Panic( KErrGeneral ) );

	// Create animation bitmap
	iSubFrameBitmap = CIHLBitmap::NewL();
	CFbsBitmap& subBitmap = iSubFrameBitmap->BitmapModifyable();
	CFbsBitmap& subMask = iSubFrameBitmap->MaskModifyable();

    TSize dstSize( iDestination->Bitmap().SizeInPixels() );
	TFrameInfo subFrameInfo( iDecoder->FrameInfo( aFrameIndex ) );

    // Check is client uses downscaling (not used in animations)
	TSize loadSize( subFrameInfo.iFrameSizeInPixels );   
    iSubFrameScaleFactor = EFull;
    if( !aAnimation &&
        dstSize != iFrameInfo.iOverallSizeInPixels )
        {
        if( dstSize == ScaledLoadSize( iFrameInfo.iOverallSizeInPixels, EHalf ) )
            {
            iSubFrameScaleFactor = EHalf;
            loadSize = ScaledLoadSize( loadSize, EHalf );
            }
        else if( dstSize == ScaledLoadSize( iFrameInfo.iOverallSizeInPixels, EQuarter ) )
            {
            iSubFrameScaleFactor = EQuarter;
            loadSize = ScaledLoadSize( loadSize, EQuarter );
            }
        else if( dstSize == ScaledLoadSize( iFrameInfo.iOverallSizeInPixels, EEighth ) )
            {
            iSubFrameScaleFactor = EEighth;
            loadSize = ScaledLoadSize( loadSize, EEighth );
            }
        }
    User::LeaveIfError( subBitmap.Create( loadSize, EColor16M ) );

	if( subFrameInfo.iFlags & TFrameInfo::ETransparencyPossible )
		{
		User::LeaveIfError( subMask.Create( loadSize,
			subFrameInfo.iFlags & TFrameInfo::EAlphaChannel ? EGray256 : EGray2 ) );
		iDecoder->Convert( &iStatus, subBitmap, subMask, aFrameIndex );
		}
	else
		{
		iDecoder->Convert( &iStatus, subBitmap, aFrameIndex );
		}
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::CompleteLoadL
// -----------------------------------------------------------------------------
void CIHLFileImage::CompleteLoadL()
	{
    IHL_DEBUG1( KIHLDebug1, "IHL - CIHLFileImage - ICL convert complete!" );

	if( iSubFrameBitmap )
		{
        IHL_DEBUG1( KIHLDebug2, "IHL - CIHLFileImage - Start build animation frame" );

		// Copy animation bitmap to destination
		BuildSubFrameL();
		delete iSubFrameBitmap;
		iSubFrameBitmap = NULL;

        IHL_DEBUG1( KIHLDebug3, "IHL - CIHLFileImage - Animation frame complete!" );

		// Save source info destination
		iDestination->SetEditorPtr( this );
		iDestination->SetEditorValue( iSubFrameIndex );

		if( iSubFrameIndex < iFrameIndex )
			{
			// re-start the active object and load next subframe
			iSubFrameIndex++;
			iImageState = EStartLoad;
			SelfComplete();
			}
		else
			{
			// Animation/subframe image ready
			iDestination = NULL;
			iImageState = EInactive;
			RequestComplete( KErrNone );
			}
		}
	else
		{
		// Save source info destination
		iDestination->SetEditorPtr( this );
		iDestination->SetEditorValue( iFrameIndex );

		// Normal image ready
		iDestination = NULL;
		iImageState = EInactive;
		RequestComplete( KErrNone );
		}

    IHL_DEBUG1( KIHLDebug4, "IHL - CIHLFileImage - Frame loading request complete!" );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::BuildSubFrameL
// -----------------------------------------------------------------------------
void CIHLFileImage::BuildSubFrameL()
	{
	__ASSERT_DEBUG( iSubFrameBitmap, Panic( KErrGeneral ) );
	const CFbsBitmap& subBitmap = iSubFrameBitmap->Bitmap();
	const CFbsBitmap& subMask = iSubFrameBitmap->Mask();
	__ASSERT_DEBUG( subBitmap.Handle(), Panic( KErrGeneral ) );

	if( !iAnimation ||
	    ( iAnimation && iSubFrameIndex == 0 ) )
		{
		TFrameInfo frameInfo( iDecoder->FrameInfo( iSubFrameIndex ) );
        if( iDestination->Bitmap().SizeInPixels() == subBitmap.SizeInPixels() &&
            frameInfo.iFrameCoordsInPixels.iTl == TPoint(0,0) )
            {
    		// Sub frame is same size as destination image and has no offset
    		// -> put directly into destination
    		User::LeaveIfError( iDestination->Copy( subBitmap, subMask, ETrue ) );
            }
        else
            {
            // Sub frame size differs from destination image size
    		CFbsBitmap& desBitmap = iDestination->BitmapModifyable();
    		CFbsBitmap& desMask = iDestination->MaskModifyable();

    		// Other frames must be build on top of previous frames
    		__ASSERT_DEBUG( desBitmap.Handle(), Panic( KErrGeneral ) );

            // Fill destination using background color
		    FillL( desBitmap, frameInfo.iBackgroundColor );

    		// Copy loaded frame on top of background
    		CFbsBitGc* bitGc;
    		CFbsBitmapDevice* bitDevice = CFbsBitmapDevice::NewL( &desBitmap );
    		CleanupStack::PushL( bitDevice );
    		User::LeaveIfError( bitDevice->CreateContext( bitGc ) );
    		CleanupStack::PushL( bitGc );

            TPoint framePos( ScaledFramePosition(
                    frameInfo.iFrameCoordsInPixels.iTl, iSubFrameScaleFactor ) );
    		if( subMask.Handle() )
    			{
    			bitGc->BitBltMasked( framePos, &subBitmap,
    								 subBitmap.SizeInPixels(), &subMask, EFalse );
    			}
    		else
    			{
    			bitGc->BitBlt( framePos, &subBitmap, subBitmap.SizeInPixels() );
    			}
    		CleanupStack::PopAndDestroy( 2, bitDevice ); // bitGc, bitDevice

            if( desMask.Handle() )
                {
                // Fill mask to transparent
    		    FillL( desMask, KRgbBlack );

                // Fill bg mask with transparency (=black)
        		CFbsBitmapDevice* maskDev = CFbsBitmapDevice::NewL( &desMask );
        		CleanupStack::PushL( maskDev );
    		    CFbsBitGc* maskGc;
        		User::LeaveIfError( maskDev->CreateContext( maskGc ) );
        		CleanupStack::PushL( maskGc );

                // Combine bg mask with first frame mask
				maskGc->BitBlt( framePos, &subMask, subMask.SizeInPixels() );

    		    CleanupStack::PopAndDestroy( 2, maskDev ); // maskGc, maskDev
                }
            }

		// Create "previous frame" if animation
		if( iAnimation )
		    {
    		if( !iPrevAnimationFrame )
    		    {
    		    iPrevAnimationFrame = CIHLBitmap::NewL();
    		    }
    		CFbsBitmap& desBitmap = iDestination->BitmapModifyable();
    		CFbsBitmap& desMask = iDestination->MaskModifyable();
    		if( iSubFrameBitmap->HasMask() )
    		    {
        		User::LeaveIfError(
        		    iPrevAnimationFrame->Create( desBitmap.SizeInPixels(),
        		        desBitmap.DisplayMode(), desMask.DisplayMode() ) );
        		FillL( iPrevAnimationFrame->BitmapModifyable(), frameInfo.iBackgroundColor );
    		    FillL( iPrevAnimationFrame->MaskModifyable(), KRgbBlack );
    		    }
    		else
    		    {
        		User::LeaveIfError(
        		    iPrevAnimationFrame->Create( desBitmap.SizeInPixels(),
        		                                 desBitmap.DisplayMode() ) );
        		FillL( iPrevAnimationFrame->BitmapModifyable(), frameInfo.iBackgroundColor );
    		    }
		    }
		}
	else // same as iAnimation && iSubFrameIndex > 0
		{
		TFrameInfo prevFrameInfo( iDecoder->FrameInfo( iSubFrameIndex - 1 ) );
	    if ( prevFrameInfo.iFlags & TFrameInfo::ERestoreToPrevious )
	        {
            // Restore destination to "previous frame"
            User::LeaveIfError( iDestination->Copy( *iPrevAnimationFrame, EFalse ) );
	        }

		CFbsBitmap& prevBitmap = iDestination->BitmapModifyable();
		CFbsBitmap& prevMask = iDestination->MaskModifyable();

		// Other frames must be build on top of previous frames
		__ASSERT_DEBUG( prevBitmap.Handle(), Panic( KErrGeneral ) );

		// Restore area in destination bitmap if needed
		TRect restoreRect;
		TBool restoreToBackground( EFalse );
		if( prevFrameInfo.iFlags & TFrameInfo::ERestoreToBackground )
			{
			restoreToBackground = ETrue;
			restoreRect = prevFrameInfo.iFrameCoordsInPixels;
		    FillRectL( prevBitmap, restoreRect, prevFrameInfo.iBackgroundColor );

			// Cache new "previous frame"
			User::LeaveIfError( iPrevAnimationFrame->Copy( *iDestination, EFalse ) );
			}
	    else if( prevFrameInfo.iFlags & TFrameInfo::ELeaveInPlace )
	        {
			// Cache new "previous frame"
			User::LeaveIfError( iPrevAnimationFrame->Copy( *iDestination, EFalse ) );
	        }

		// Copy animation frame to destination bitmap
		TFrameInfo frameInfo( iDecoder->FrameInfo( iSubFrameIndex ) );
		CFbsBitGc* bitGc;
		CFbsBitmapDevice* bitDevice = CFbsBitmapDevice::NewL( &prevBitmap );
		CleanupStack::PushL( bitDevice );
		User::LeaveIfError( bitDevice->CreateContext( bitGc ) );
		CleanupStack::PushL( bitGc );
		if( subMask.Handle() )
			{
			bitGc->BitBltMasked( frameInfo.iFrameCoordsInPixels.iTl, &subBitmap,
									subBitmap.SizeInPixels(), &subMask, EFalse );
			}
		else
			{
			bitGc->BitBlt( frameInfo.iFrameCoordsInPixels.iTl, &subBitmap,
									subBitmap.SizeInPixels() );
			}
		CleanupStack::PopAndDestroy( 2 ); // bitGc, bitDevice

		// Combine masks if any
		if( prevMask.Handle() && subMask.Handle() )
			{
			//////////////////////////////////////////////////////////////////////////
			// ALTERNATIVE WAY TO COMBINE MASKS!
			// Current solution doesn't combine soft masks.
			// Following code could be used if soft masks are enabled in animations.
			// Do not delete!
			//////////////////////////////////////////////////////////////////////////
			/*
			if( restoreToBackground )
				{
				bitDevice = CFbsBitmapDevice::NewL( &prevMask );
				CleanupStack::PushL( bitDevice );
				User::LeaveIfError( bitDevice->CreateContext( bitGc ) );
				CleanupStack::PushL( bitGc );

				bitGc->SetBrushColor( KRgbBlack );
				bitGc->SetBrushStyle( CGraphicsContext::ESolidBrush );
				bitGc->DrawRect( restoreRect );
				bitGc->SetBrushStyle( CGraphicsContext::ENullBrush );

				CleanupStack::PopAndDestroy( 2 ); // bitDevice, bitGc
				}

			prevMask.LockHeap();

			TUint8* srcAddress = reinterpret_cast<TUint8*>( animMask.DataAddress() );
			TSize srcSize( animMask.SizeInPixels() );
			TPoint srcPos( 0,0 );
			TInt srcScanLen8 = CFbsBitmap::ScanLineLength( srcSize.iWidth, EGray256 );

			TUint8* dstAddress = reinterpret_cast<TUint8*>( prevMask.DataAddress() );
			TSize dstSize( prevMask.SizeInPixels() );
			TPoint dstPos( frameInfo.iFrameCoordsInPixels.iTl );
			TPoint dstEndPos( frameInfo.iFrameCoordsInPixels.iBr );
			TInt dstScanLen8 = CFbsBitmap::ScanLineLength( dstSize.iWidth, EGray256 );

			while( dstPos.iY < dstEndPos.iY )
				{
				TUint8* srcAddressCur = srcAddress + ( srcPos.iY * srcScanLen8 );
				TUint8* dstAddressCur = dstAddress + ( dstPos.iY * dstScanLen8 );
				while( dstPos.iX < dstEndPos.iX )
					{
					TUint8& srcPixel = srcAddressCur[ srcPos.iX++ ];
					TUint8& dstPixel = dstAddressCur[ dstPos.iX++ ];
					if( srcPixel > dstPixel )
						{
						dstPixel = srcPixel;
						}
					}

				srcPos.iX = 0;
				srcPos.iY++;
				dstPos.iX = frameInfo.iFrameCoordsInPixels.iTl.iX;
				dstPos.iY++;
				}

			animMask.UnlockHeap();
			*/

			if( restoreToBackground )
				{
		        FillRectL( prevMask, restoreRect, KRgbBlack );
				}

			bitDevice = CFbsBitmapDevice::NewL( &prevMask );
			CleanupStack::PushL( bitDevice );
			User::LeaveIfError( bitDevice->CreateContext( bitGc ) );
			CleanupStack::PushL( bitGc );

			CFbsBitmap* tmpMask = new(ELeave) CFbsBitmap;
			CleanupStack::PushL( tmpMask );
			User::LeaveIfError( tmpMask->Create( prevMask.SizeInPixels(), prevMask.DisplayMode() ) );
			CFbsBitmapDevice* tmpMaskDev = CFbsBitmapDevice::NewL( tmpMask );
			CleanupStack::PushL( tmpMaskDev );
			CFbsBitGc* tmpMaskGc;
			User::LeaveIfError( tmpMaskDev->CreateContext( tmpMaskGc ) );
			CleanupStack::PushL( tmpMaskGc );

			tmpMaskGc->BitBlt( TPoint( 0, 0 ), &prevMask, frameInfo.iFrameCoordsInPixels );

			bitGc->BitBltMasked( frameInfo.iFrameCoordsInPixels.iTl, &subMask,
								 subMask.SizeInPixels(), tmpMask, ETrue );

			CleanupStack::PopAndDestroy( 5 ); // tmpMaskGc,tmpMaskDev,tmpMask,bitGc,bitDevice
			}
		else
			{
			prevMask.Reset(); // Mask not valid anymore -> reset
			}
		}
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::FillL
// -----------------------------------------------------------------------------
void CIHLFileImage::FillL( CFbsBitmap& aBitmap, const TRgb& aColor )
    {
	CFbsBitGc* bitGc;
	CFbsBitmapDevice* bitDevice = CFbsBitmapDevice::NewL( &aBitmap );
	CleanupStack::PushL( bitDevice );
	User::LeaveIfError( bitDevice->CreateContext( bitGc ) );
	CleanupStack::PushL( bitGc );

	bitGc->SetBrushColor( aColor );
	bitGc->SetPenColor( aColor );
	bitGc->SetBrushStyle( CGraphicsContext::ESolidBrush );
	bitGc->Clear();
	bitGc->SetBrushStyle( CGraphicsContext::ENullBrush );

	CleanupStack::PopAndDestroy( 2 ); // bitGc, bitDevice
    }

// -----------------------------------------------------------------------------
// CIHLFileImage::FillRectL
// -----------------------------------------------------------------------------
void CIHLFileImage::FillRectL( CFbsBitmap& aBitmap, const TRect& aRect,
                               const TRgb& aColor )
    {
	CFbsBitGc* bitGc;
	CFbsBitmapDevice* bitDevice = CFbsBitmapDevice::NewL( &aBitmap );
	CleanupStack::PushL( bitDevice );
	User::LeaveIfError( bitDevice->CreateContext( bitGc ) );
	CleanupStack::PushL( bitGc );

	bitGc->SetBrushColor( aColor );
	bitGc->SetPenColor( aColor );
	bitGc->SetBrushStyle( CGraphicsContext::ESolidBrush );
	bitGc->DrawRect( aRect );
	bitGc->SetBrushStyle( CGraphicsContext::ENullBrush );

	CleanupStack::PopAndDestroy( 2 ); // bitGc, bitDevice
    }

// -----------------------------------------------------------------------------
// CIHLFileImage::ErrorCleanup
// -----------------------------------------------------------------------------
void CIHLFileImage::ErrorCleanup()
	{
	if( iSubFrameBitmap )
		{
		delete iSubFrameBitmap;
		iSubFrameBitmap = NULL;
		}

	if( iDestination )
		{
		iDestination = NULL;
		}
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::SelfComplete
// -----------------------------------------------------------------------------
void CIHLFileImage::SelfComplete()
	{
	SetActive();
	iStatus = KRequestPending;
	TRequestStatus* status = &iStatus;
	User::RequestComplete( status, KErrNone );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::RequestComplete
// -----------------------------------------------------------------------------
void CIHLFileImage::RequestComplete( TInt aReason )
	{
	ASSERT( iImageStatus );
	User::RequestComplete( iImageStatus, aReason );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::ScaledLoadSize
// -----------------------------------------------------------------------------
TSize CIHLFileImage::ScaledLoadSize( const TSize& aOriginalSize, TInt aScaleFactor )
	{
	// Divide original size with scalefactor
	// Round always up if result is not integer
	return ( aScaleFactor == EFull ) ? aOriginalSize :
		TSize( aOriginalSize.iWidth / aScaleFactor + ( aOriginalSize.iWidth % aScaleFactor ? 1 : 0 ),
			   aOriginalSize.iHeight / aScaleFactor + ( aOriginalSize.iHeight % aScaleFactor ? 1 : 0 ) );
	}

// -----------------------------------------------------------------------------
// CIHLFileImage::ScaledLoadSize
// -----------------------------------------------------------------------------
TPoint CIHLFileImage::ScaledFramePosition( const TPoint& aOriginalPos, TInt aScaleFactor )
	{
	// Divide original position with scalefactor
	// Round always up if result is not integer
	return ( aScaleFactor == EFull ) ? aOriginalPos :
		TPoint( aOriginalPos.iX / aScaleFactor + ( aOriginalPos.iX % aScaleFactor ? 1 : 0 ),
			    aOriginalPos.iY / aScaleFactor + ( aOriginalPos.iY % aScaleFactor ? 1 : 0 ) );
	}
//  End of File
