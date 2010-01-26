/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Implementation of FileImageExtJpg class.
*
*/


// INCLUDE FILES
#include "CIHLFileImageExtJpg.h"

#include "CIHLBitmap.h"
#include "IHLImplementationIds.h"
#include <IHLInterfaceIds.h>
#include <IclExtJpegApi.h>

// Private namespace for constants and functions
namespace
	{
	// Fixed scale factors
	enum TScaleFactors
		{
		EHalf		= 2,
		EQuarter	= 4,
		EEighth		= 8,
		};

	// Panic function
	_LIT( KIHLExtJpgPanic, "IHLImageExtJpg" );
	void Panic( TInt aPanicCode ) { User::Panic( KIHLExtJpgPanic, aPanicCode ); }
	}

// ============================ MEMBER FUNCTIONS ===============================
// -----------------------------------------------------------------------------
//
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
CIHLFileImageExtJpg::CIHLFileImageExtJpg( TInt aImageIndex )
:CActive( CActive::EPriorityStandard ),
iImageIndex( aImageIndex )
    {
	CActiveScheduler::Add( this );
    }

// -----------------------------------------------------------------------------
//
// Two-phased constructor.
// -----------------------------------------------------------------------------

CIHLFileImageExtJpg* CIHLFileImageExtJpg::NewL( RFs& aFs, const TDesC8& aDataBuf,
                                    TInt aImageIndex, const TUint32 aOptions )
    {
    CIHLFileImageExtJpg* self = new (ELeave) CIHLFileImageExtJpg( aImageIndex );
	CleanupStack::PushL( self );
	self->ConstructL( aFs, aDataBuf, aOptions );
	CleanupStack::Pop(); // self
    return self;
    }

CIHLFileImageExtJpg* CIHLFileImageExtJpg::NewL( RFs& aFs, const TDesC& aFilename, 
									TInt aImageIndex, const TUint32 aOptions )
    {
    CIHLFileImageExtJpg* self = new (ELeave) CIHLFileImageExtJpg( aImageIndex );
	CleanupStack::PushL( self );
	self->ConstructL( aFs, aFilename, aOptions );
	CleanupStack::Pop(); // self
    return self;
    }

// -----------------------------------------------------------------------------
//
// Symbian constructor can leave.
// -----------------------------------------------------------------------------


void CIHLFileImageExtJpg::ConstructL( RFs& aFs, const TDesC& aFilename, const TUint32 /*aOptions*/ )
	{
	TRAPD( err,	iDecoder = CExtJpegDecoder::FileNewL( CExtJpegDecoder::EHwImplementation, aFs, aFilename ) );
	
	if ( err != KErrNone )
		{
		iDecoder = CExtJpegDecoder::FileNewL( CExtJpegDecoder::ESwImplementation, aFs, aFilename );
		}

	ConstructCommonL();
	}
	
void CIHLFileImageExtJpg::ConstructL( RFs& aFs, const TDesC8& aDataBuf, const TUint32 /*aOptions*/ )
    {
	TRAPD( err,	iDecoder = CExtJpegDecoder::DataNewL( CExtJpegDecoder::EHwImplementation, aFs, aDataBuf ) );
	
	if ( err != KErrNone )
		{
		iDecoder = CExtJpegDecoder::DataNewL( CExtJpegDecoder::ESwImplementation, aFs, aDataBuf );
		}
    
	ConstructCommonL();
    }
		
// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::ConstructCommonL
// -----------------------------------------------------------------------------	
void CIHLFileImageExtJpg::ConstructCommonL()
    {
	// Check frame count and image index
	iImageCount = iDecoder->FrameCount();
	__ASSERT_ALWAYS( iImageCount > 0, User::Leave( KErrCorrupt ) );

	// Get image types
	iDecoder->ImageType( iImageIndex, iImageType, iImageSubType );
	
	__ASSERT_ALWAYS( iImageIndex >= 0 && iImageIndex < iImageCount, User::Leave( KErrArgument ) );

	// cache frame info and set scale sizes
	iFrameInfo = iDecoder->FrameInfo( iImageIndex );
	if( iFrameInfo.iFlags & TFrameInfo::EFullyScaleable )
		{
		iFullyScaleable = ETrue;
		}
    }

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
CIHLFileImageExtJpg::~CIHLFileImageExtJpg()
    {
	Cancel();
	delete iDecoder;
    }

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::Type
// -----------------------------------------------------------------------------
TIHLInterfaceType CIHLFileImageExtJpg::Type() const
	{
	return TIHLInterfaceType( KIHLInterfaceIdFileImage,
							  KIHLImplementationIdFileImageExtJpg );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::ImageType
// -----------------------------------------------------------------------------
const TUid& CIHLFileImageExtJpg::ImageType() const
	{
	return iImageType;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::ImageSubType
// -----------------------------------------------------------------------------
const TUid& CIHLFileImageExtJpg::ImageSubType() const
	{
	return iImageSubType;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::ImageIndex
// -----------------------------------------------------------------------------
TInt CIHLFileImageExtJpg::ImageIndex() const
	{
	return iImageIndex;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::ImageCount
// -----------------------------------------------------------------------------
TInt CIHLFileImageExtJpg::ImageCount() const
	{
	return iImageCount;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::Size
// -----------------------------------------------------------------------------
TSize CIHLFileImageExtJpg::Size() const
	{
	return iFrameInfo.iOverallSizeInPixels;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::DisplayMode
// -----------------------------------------------------------------------------
TDisplayMode CIHLFileImageExtJpg::DisplayMode() const
	{
	TDisplayMode displayMode( iFrameInfo.iFrameDisplayMode );
	if( iFrameInfo.iFrameDisplayMode <= EGray256 )
		{
		displayMode = EGray256;
		}
	else if( iFrameInfo.iFrameDisplayMode <= EColor256 )
		{
		displayMode = EColor256;
		}

	return displayMode;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::MaskDisplayMode
// -----------------------------------------------------------------------------
TDisplayMode CIHLFileImageExtJpg::MaskDisplayMode() const
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
// CIHLFileImageExtJpg::BackgroundColor
// -----------------------------------------------------------------------------
TRgb CIHLFileImageExtJpg::BackgroundColor() const
	{
	return iFrameInfo.iBackgroundColor;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::CustomLoadSizeArray
// -----------------------------------------------------------------------------
const RArray<TSize>& CIHLFileImageExtJpg::CustomLoadSizeArray() const
	{
	return iLoadSizeArray;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::IsFullyScaleable
// -----------------------------------------------------------------------------
TBool CIHLFileImageExtJpg::IsFullyScaleable() const
	{
	return iFullyScaleable;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::IsAnimation
// -----------------------------------------------------------------------------
TBool CIHLFileImageExtJpg::IsAnimation() const
	{
	return iAnimation;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::AnimationFrameCount
// -----------------------------------------------------------------------------
TInt CIHLFileImageExtJpg::AnimationFrameCount() const
	{
	return iAnimationFrameCount;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::AnimationFrameDelay
// -----------------------------------------------------------------------------
TTimeIntervalMicroSeconds32 CIHLFileImageExtJpg::AnimationFrameDelay( TInt aAnimationFrameIndex ) const
	{
	//Animation is not supported
	( void ) aAnimationFrameIndex;
	Panic( KErrNotSupported );
	return 0; 
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::Load
// -----------------------------------------------------------------------------
TInt CIHLFileImageExtJpg::Load( TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex )
	{
	iImageIndex = aFrameIndex;
	return LoadRequest( aStatus, aDestination, iImageIndex );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::Load
// -----------------------------------------------------------------------------
TInt CIHLFileImageExtJpg::Load( TRect aSrcRect, TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex )
	{
	TRAPD( err, SetCroppingL( aSrcRect ) );
	if( err )
		{
		return err;
		}
	iImageIndex = aFrameIndex;
	return LoadRequest( aStatus, aDestination, iImageIndex );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::LoadAnimation
// -----------------------------------------------------------------------------
TInt CIHLFileImageExtJpg::LoadAnimation( TRequestStatus& aStatus, MIHLBitmap& aDestination,
							  TInt aAnimationFrameIndex )
	{
	//Animation is not supported
	( void ) aStatus;
	( void ) aDestination;
	( void ) aAnimationFrameIndex;
	Panic( KErrNotSupported );
	return 0;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::IsBusy
// -----------------------------------------------------------------------------
TBool CIHLFileImageExtJpg::IsBusy() const
	{
	return ( iImageState != EInactive );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::CancelLoad
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::CancelLoad()
	{
	Cancel();
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SetFilter
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::SetFilter( MIHLFilter* /*aFilter*/ )
	{
	// Not in use
	}


// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::Decoder
// -----------------------------------------------------------------------------
const CExtJpegDecoder& CIHLFileImageExtJpg::Decoder() const
	{
	return *iDecoder;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::DoCancel
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::DoCancel()
	{
	iDecoder->Cancel();

	// Delete all processed bitmaps
	ErrorCleanup();

	// Complete with cancel
	iImageState = EInactive;
	RequestComplete( KErrCancel );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::RunL
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::RunL()
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
// CIHLFileImageExtJpg::RunError
// -----------------------------------------------------------------------------
TInt CIHLFileImageExtJpg::RunError( TInt aError )
	{
	// Delete all processed bitmaps
	ErrorCleanup();

	// Complete with error
	iImageState = EInactive;
	RequestComplete( aError );
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::LoadRequest
// -----------------------------------------------------------------------------
TInt CIHLFileImageExtJpg::LoadRequest( TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex )
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
// CIHLFileImageExtJpg::StartLoadL
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::StartLoadL()
	{
	CFbsBitmap& dstBitmap = iDestination->BitmapModifyable();
	CFbsBitmap& dstMask = iDestination->MaskModifyable();

	if( MaskDisplayMode() && dstMask.Handle() )
		{
		iDecoder->Convert( &iStatus, dstBitmap, dstMask, iFrameIndex );
		}
	else
		{
		dstMask.Reset();
		iDecoder->Convert( &iStatus, dstBitmap, iFrameIndex );
		}
	iImageState = ECompleteLoad;
	SetActive();
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::CompleteLoadL
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::CompleteLoadL()
	{
	// Save source info destination
	iDestination->SetEditorPtr( this );
	iDestination->SetEditorValue( iFrameIndex );

	// Normal image ready
	iDestination = NULL;
	iImageState = EInactive;
	RequestComplete( KErrNone );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::ErrorCleanup
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::ErrorCleanup()
	{
	if( iDestination )
		{
		iDestination = NULL;
		}
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SelfComplete
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::SelfComplete()
	{
	SetActive();
	iStatus = KRequestPending;
	TRequestStatus* status = &iStatus;
	User::RequestComplete( status, KErrNone );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::RequestComplete
// -----------------------------------------------------------------------------
void CIHLFileImageExtJpg::RequestComplete( TInt aReason )
	{
	ASSERT( iImageStatus );
	User::RequestComplete( iImageStatus, aReason );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SetCroppingL
// -----------------------------------------------------------------------------

void CIHLFileImageExtJpg::SetCroppingL( TRect aCropRect )
	{
	iDecoder->SetCroppingL( aCropRect );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SetStreamingL
// -----------------------------------------------------------------------------

void CIHLFileImageExtJpg::SetStreamingL( TSize& aMacroBlockSize )
	{
	iDecoder->SetStreamingL( aMacroBlockSize );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SetRotationL
// -----------------------------------------------------------------------------

void CIHLFileImageExtJpg::SetRotationL( TInt aDegree )
	{
	iDecoder->SetRotationL( aDegree );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SetFlippingL
// -----------------------------------------------------------------------------

void CIHLFileImageExtJpg::SetFlippingL()
	{
	iDecoder->SetFlippingL();
	}
	
// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SetMirroringL
// -----------------------------------------------------------------------------

void CIHLFileImageExtJpg::SetMirroringL()
	{
	iDecoder->SetMirroringL();
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SetDctDecodingL
// -----------------------------------------------------------------------------

void CIHLFileImageExtJpg::SetDctDecodingL()
	{
	iDecoder->SetDctDecodingL();
	}
	
// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::ConvertL
// -----------------------------------------------------------------------------

void CIHLFileImageExtJpg::ConvertL( 
                        TRequestStatus* aRequestStatus, 
                        const CVisualFrame* aDestinationFrame, 
                        TInt& aNoOfDecodedMBlocks,
                        TInt aFrameNumber )
	{
	iDecoder->ConvertL( 
                        aRequestStatus, 
                        aDestinationFrame, 
                        aNoOfDecodedMBlocks,
                        aFrameNumber );
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::ContinueConvertL
// -----------------------------------------------------------------------------

void CIHLFileImageExtJpg::ContinueConvertL( 
                        TRequestStatus* aRequestStatus, 
                        const CVisualFrame* aDestinationFrame, 
                        TInt& aNoOfDecodedMBlocks,
                        TInt aFrameNumber )
	{
	iDecoder->ContinueConvertL( 
                        aRequestStatus, 
                        aDestinationFrame, 
                        aNoOfDecodedMBlocks,
                        aFrameNumber );	
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::SupportedFormatsL
// -----------------------------------------------------------------------------

TInt CIHLFileImageExtJpg::SupportedFormatsL()
	{
	return iDecoder->SupportedFormatsL();
	}

// -----------------------------------------------------------------------------
// CIHLFileImageExtJpg::CapabilitiesL
// -----------------------------------------------------------------------------
	
TInt CIHLFileImageExtJpg::CapabilitiesL()
	{
	return iDecoder->CapabilitiesL();
	}
	
//End of file
