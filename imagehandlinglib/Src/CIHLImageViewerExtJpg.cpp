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
* Description:  Implementation of Image Viewer class.
*
*/


// INCLUDE FILES
#include "CIHLImageViewerExtJpg.h"

#include "CIHLBitmap.h"
#include "CIHLBitmapProcessor.h"
#include "IHLImplementationIds.h"
#include <IHLInterfaceIds.h>
#include <IclExtJpegApi.h>
#include <imagecodecdata.h>
#include <MIHLViewerObserver.h>
#include "MIHLFileImageExtJpg.h"
#include <fbs.h>

// Private namespace for constants and functions
namespace
	{
	// Panic function
	_LIT( KIHLImageViewerExtJpgPanic, "IHLImageViewerExtJpg" );
	void Panic( TInt aPanicCode ) { User::Panic( KIHLImageViewerExtJpgPanic, aPanicCode ); }
	}

// ============================ MEMBER FUNCTIONS ===============================
// -----------------------------------------------------------------------------
//
// C++ constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
CIHLImageViewerExtJpg::CIHLImageViewerExtJpg( const TSize& aViewerSize,
								  MIHLFileImageExtJpg& aSource,
								  MIHLBitmap& aDestination,
								  MIHLViewerObserver& aObserver,
								  const TUint32 aOptions )
: CTimer( EPriorityNormal ), iObserver( aObserver ),
iSource( aSource ), iDestination( aDestination ),
iOptions( aOptions ), iViewerSize( aViewerSize ), iSrcBitmapScaleFactor(1)
    {

	CActiveScheduler::Add( this );
    }

// -----------------------------------------------------------------------------
//
// Two-phased constructor.
// -----------------------------------------------------------------------------
CIHLImageViewerExtJpg* CIHLImageViewerExtJpg::NewL( const TSize& aViewerSize,
										MIHLFileImageExtJpg& aSource,
										MIHLBitmap& aDestination,
										MIHLViewerObserver& aObserver,
										const TUint32 aOptions )
    {
    CIHLImageViewerExtJpg* self = new( ELeave ) CIHLImageViewerExtJpg(
		aViewerSize, aSource, aDestination, aObserver, aOptions );
    CleanupStack::PushL( self );
    self->ConstructL();
	CleanupStack::Pop(); // self
    return self;
    }

// -----------------------------------------------------------------------------
//
// Symbian constructor can leave.
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::ConstructL()
	{
	CTimer::ConstructL();

	iCacheSource = CIHLBitmap::NewL();
	iCacheDestination = CIHLBitmap::NewL();

	// Check codec capabilities
	iCapabilities = iSource.CapabilitiesL();
	if( !( iCapabilities & ( CExtJpegDecoder::ECapRotation +
						CExtJpegDecoder::ECapMirroring +
						CExtJpegDecoder::ECapFlipping ) ) )
		{
		iInternalProcessingNeeded = ETrue;
		iProcessor = CIHLBitmapProcessor::NewL( iOptions );
		}

	// Initialize settings and start load (default is zoom to fit)
	iSourceSize = iSource.Size();
	CalculateZoomToFitRatio();
	User::LeaveIfError( SetZoomRatio( iZoomToFitRatio ) );
	}

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
CIHLImageViewerExtJpg::~CIHLImageViewerExtJpg()
    {
	Cancel();
	if( iProcessor )
		{
		delete iProcessor;
		}
	delete iCacheSource;
    delete iCacheDestination;
    }

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::Id
// -----------------------------------------------------------------------------
TIHLInterfaceType CIHLImageViewerExtJpg::Type() const
	{
	return TIHLInterfaceType( KIHLInterfaceIdImageViewer, KIHLImplementationIdImageViewerExtJpg );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::IsAnimation
// -----------------------------------------------------------------------------
TBool CIHLImageViewerExtJpg::IsAnimation() const
	{
	return EFalse;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::Play
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::Play()
	{

	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::Stop
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::Stop()
	{

	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::IsPlaying
// -----------------------------------------------------------------------------
TBool CIHLImageViewerExtJpg::IsPlaying() const
	{
	return EFalse;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::AnimationFrameCount
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::AnimationFrameCount() const
	{
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::AnimationFrame
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::AnimationFrame() const
	{
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SetAnimationFrame
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::SetAnimationFrame( TInt /*aFrameIndex*/ )
	{
    return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SetViewerSize
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::SetViewerSize( const TSize& aViewerSize, int aSrcBitmapScaleFactor )
	{
    // Public API, negative value check -> panic also on hardware
	__ASSERT_ALWAYS( aViewerSize.iWidth >= 0 &&
					 aViewerSize.iHeight >= 0, Panic( KErrArgument ) );

	iSrcBitmapScaleFactor = aSrcBitmapScaleFactor;

	// Save new viewer size
	iViewerSize = aViewerSize;

	// Recalculate source rect and destination size
	CalculateSourceRectAndDestinationSize();

	// Recalculate zoom to fit ratio
	CalculateZoomToFitRatio();

	// Start load
	return ApplySettings();
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::ViewerSize
// -----------------------------------------------------------------------------
TSize CIHLImageViewerExtJpg::ViewerSize() const
	{
	return iViewerSize;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::MoveSourceRect
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::MoveSourceRect( TInt aDx, TInt aDy )
	{
	TInt dx( aDx );
	TInt dy( aDy );
	switch( iRotationAngle )
		{
		case ERotationAngle0:
			{
			break;
			}
		case ERotationAngle90:
			{
			dx = aDy;
			dy = -aDx;
			break;
			}
		case ERotationAngle180:
			{
			dx = -aDx;
			dy = -aDy;
			break;
			}
		case ERotationAngle270:
			{
			dx = -aDy;
			dy = aDx;
			break;
			}
		default:
			{
            // Internal state error, debug panic only
            #ifdef _DEBUG
			    Panic( KErrGeneral );
            #endif
			}
		}
	if( iHorizontalMirroring )
		{
		dx = -dx;
		}
	if( iVerticalMirroring )
		{
		dy = -dy;
		}

	if( iSourceRect.iTl.iX + dx < 0 ||
		iSourceRect.iBr.iX + dx > iSourceSize.iWidth ||
		iSourceRect.iTl.iY + dy < 0 ||
		iSourceRect.iBr.iY + dy > iSourceSize.iHeight )
		{
		return KErrArgument;
		}

	iSourceRect.Move( dx, dy );

	// Start load
	return ApplySettings();
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SetSourceRectPosition
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::SetSourceRectPosition( const TPoint& aPosition )
	{
	TPoint newPosition( aPosition );
	TSize sourceRectSize( iSourceRect.Size() );
	switch( iRotationAngle )
		{
		case ERotationAngle0:
			{
			break;
			}
		case ERotationAngle90:
			{
			newPosition.iX = aPosition.iY;
			newPosition.iY = iSourceSize.iHeight - sourceRectSize.iHeight - aPosition.iX;
			break;
			}
		case ERotationAngle180:
			{
			newPosition.iX = iSourceSize.iWidth - sourceRectSize.iWidth - aPosition.iX;
			newPosition.iY = iSourceSize.iHeight - sourceRectSize.iHeight - aPosition.iY;
			break;
			}
		case ERotationAngle270:
			{
			newPosition.iX = iSourceSize.iWidth - sourceRectSize.iWidth - aPosition.iY;
			newPosition.iY = aPosition.iX;
			break;
			}
		default:
			{
            // Internal state error, debug panic only
            #ifdef _DEBUG
    			Panic( KErrGeneral );
            #endif
			}
		}
	if( iHorizontalMirroring )
		{
		newPosition.iX = iSourceSize.iWidth - sourceRectSize.iWidth - newPosition.iX;
		}
	if( iVerticalMirroring )
		{
		newPosition.iY = iSourceSize.iHeight - sourceRectSize.iHeight - newPosition.iY;
		}

	if( newPosition.iX < 0 ||
		newPosition.iX > ( iSourceSize.iWidth - sourceRectSize.iWidth ) ||
		newPosition.iY < 0 ||
		newPosition.iY > ( iSourceSize.iHeight - sourceRectSize.iHeight ) )
		{
		return KErrArgument;
		}

	iSourceRect = TRect( newPosition, sourceRectSize );

	// Start load
	return ApplySettings();
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SourceRect
// -----------------------------------------------------------------------------
TRect CIHLImageViewerExtJpg::SourceRect() const
	{
	TRect mirroredSourceRect( iSourceRect );
	if( iHorizontalMirroring )
		{
		mirroredSourceRect.iTl.iX  = iSourceSize.iWidth - iSourceRect.iBr.iX;
		mirroredSourceRect.iBr.iX  = iSourceSize.iWidth - iSourceRect.iTl.iX;
		}
	if( iVerticalMirroring )
		{
		mirroredSourceRect.iTl.iY  = iSourceSize.iHeight - iSourceRect.iBr.iY;
		mirroredSourceRect.iBr.iY  = iSourceSize.iHeight - iSourceRect.iTl.iY;
		}

	TRect rotatedSourceRect( mirroredSourceRect );
	switch( iRotationAngle )
		{
		case ERotationAngle0:
			{
			break;
			}
		case ERotationAngle90:
			{
			rotatedSourceRect.iTl.iX = iSourceSize.iHeight - mirroredSourceRect.iBr.iY;
			rotatedSourceRect.iTl.iY = mirroredSourceRect.iTl.iX;
			rotatedSourceRect.iBr.iX = iSourceSize.iHeight - mirroredSourceRect.iTl.iY;
			rotatedSourceRect.iBr.iY = mirroredSourceRect.iBr.iX;
			break;
			}
		case ERotationAngle180:
			{
			rotatedSourceRect.iTl.iX = iSourceSize.iWidth - mirroredSourceRect.iBr.iX;
			rotatedSourceRect.iTl.iY = iSourceSize.iHeight - mirroredSourceRect.iBr.iY;
			rotatedSourceRect.iBr.iX = iSourceSize.iWidth - mirroredSourceRect.iTl.iX;
			rotatedSourceRect.iBr.iY = iSourceSize.iHeight - mirroredSourceRect.iTl.iY;
			break;
			}
		case ERotationAngle270:
			{
			rotatedSourceRect.iTl.iX = mirroredSourceRect.iTl.iY;
			rotatedSourceRect.iTl.iY = iSourceSize.iWidth - mirroredSourceRect.iBr.iX;
			rotatedSourceRect.iBr.iX = mirroredSourceRect.iBr.iY;
			rotatedSourceRect.iBr.iY = iSourceSize.iWidth - mirroredSourceRect.iTl.iX;
			break;
			}
		default:
			{
            // Internal state error, debug panic only
            #ifdef _DEBUG
    			Panic( KErrGeneral );
            #endif
			}
		}

	return rotatedSourceRect;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SourceSize
// -----------------------------------------------------------------------------
TSize CIHLImageViewerExtJpg::SourceSize() const
	{
	if( iRotationAngle == ERotationAngle90 ||
		iRotationAngle == ERotationAngle270 )
		{
		return TSize( iSourceSize.iHeight, iSourceSize.iWidth );
		}
	else
		{
		return iSourceSize;
		}
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SetZoomRatio
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::SetZoomRatio( TReal aZoomRatio )
	{
	if( aZoomRatio <= 0 )
		{
		return KErrArgument;
		}

	iZoomRatio = aZoomRatio;

	// Recalculate source rect and destination size
	CalculateSourceRectAndDestinationSize();

	// Start load
	return ApplySettings();
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::ZoomRatio
// -----------------------------------------------------------------------------
TReal CIHLImageViewerExtJpg::ZoomRatio() const
	{
	return iZoomRatio;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::ZoomToFitRatio
// -----------------------------------------------------------------------------
TReal CIHLImageViewerExtJpg::ZoomToFitRatio() const
	{
	return iZoomToFitRatio;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::RotateClockwise
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::RotateClockwise()
	{
	TInt rotationAngle( iRotationAngle + ERotationAngle90 );
	if( rotationAngle > ERotationAngle270 )
		{
		rotationAngle = ERotationAngle0;
		}

	return SetRotationAngle( rotationAngle );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::RotateCounterClockwise
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::RotateCounterClockwise()
	{
	TInt rotationAngle( iRotationAngle - ERotationAngle90 );
	if( rotationAngle < ERotationAngle0 )
		{
		rotationAngle = ERotationAngle270;
		}

	return SetRotationAngle( rotationAngle );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SetRotationAngle
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::SetRotationAngle( TInt aRotationAngle )
	{
	if( aRotationAngle != ERotationAngle0 &&
		aRotationAngle != ERotationAngle90 &&
		aRotationAngle != ERotationAngle180 &&
		aRotationAngle != ERotationAngle270 )
		{
		return KErrArgument;
		}

	iRotationAngle = aRotationAngle;

	// Recalculate source rect and destination size
	CalculateSourceRectAndDestinationSize();

	// Recalculate zoom to fit ratio
	CalculateZoomToFitRatio();

	// Start load
	return ApplySettings();
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::RotationAngle
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::RotationAngle() const
	{
	return iRotationAngle;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SetVerticalMirroring
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::SetVerticalMirroring( TBool aValue )
	{
	if( iVerticalMirroring != aValue )
		{
		iVerticalMirroring = aValue;

		// Start load
		return ApplySettings();
		}
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::VerticalMirroring
// -----------------------------------------------------------------------------
TBool CIHLImageViewerExtJpg::VerticalMirroring() const
	{
	return iVerticalMirroring;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SetHorizontalMirroring
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::SetHorizontalMirroring( TBool aValue )
	{
	if( iHorizontalMirroring != aValue )
		{
		iHorizontalMirroring = aValue;
		return ApplySettings();
		}
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::HorizontalMirroring
// -----------------------------------------------------------------------------
TBool CIHLImageViewerExtJpg::HorizontalMirroring() const
	{
	return iHorizontalMirroring;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SetFilter
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::SetFilter( MIHLFilter* /*aFilter*/ )
	{
	// Not in use
	}



// Private methods
// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::DoCancel
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::DoCancel()
	{
	CTimer::DoCancel();
	// Set state to inactive
	switch( iViewerState )
		{
		case ELoad:
			{
			// Cancel asynchronous source loading
			iSource.CancelLoad();
			iCacheSource->Reset();
			break;
			}
		case EProcess:
			{
			// Cancel asynchronous processing
			iProcessor->CancelProcess();
			iCacheDestination->Reset();
			break;
			}
		default:
			{
			break;
			}
		}
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::RunL
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::RunL()
	{
	User::LeaveIfError( iStatus.Int() );

	// Save previous state and set current state immediately
	// to inactive to keep state up-to-date
	TViewerState prevState( iViewerState );
	iViewerState = EInactive;

	switch( prevState )
		{
		case ELoad:
			{
			//Use the internal processor if the codec is lacking some processing capability
			if( iInternalProcessingNeeded )
				{
				const CFbsBitmap& srcBitmap = iCacheSource->Bitmap();
				User::LeaveIfError( iCacheDestination->Create( iDestinationSize, srcBitmap.DisplayMode() ) );
				TRect processSourceRect( CalculateProcessSourceRect( iCacheSource->Bitmap().SizeInPixels() ) );
				TRect destinationRect( CalculateProcessDestinationRect() );

				User::LeaveIfError( iProcessor->Process( iStatus, *iCacheSource, processSourceRect,
									  *iCacheDestination, destinationRect ) );

				iViewerState = EProcess;
				SetActive();
				}
			else
				{
				User::LeaveIfError( Finish() );
				iObserver.ViewerBitmapChangedL();
				}
			break;
			}
		case EProcess:
			{
			User::LeaveIfError( Finish() );
			iObserver.ViewerBitmapChangedL();
			break;
			}
		default:
			{
            // Internal state error, debug panic only
			#ifdef _DEBUG
				Panic( KErrGeneral );
			#endif
			}
		}
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::RunError
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::RunError( TInt aError )
	{
	switch( iViewerState )
		{
		case ELoad:
			{
			// Cleanup cached source if load has been failed
			iCacheSource->Reset();
			break;
			}
		case EProcess:
			{
			// Cleanup cached destination if process has been failed
			iCacheDestination->Reset();
			break;
			}
		default:
			{
			break;
			}
		}

	iViewerState = EInactive;
	iObserver.ViewerError( aError );
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::ApplySettings
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::ApplySettings()
	{
	// Assert source rect and destination, debug build only
#ifdef _DEBUG
	AssertSourceRectAndDestinationSize();
#endif

	// Cancel current process if any
	Cancel();

	// Perform processing if supported by codec
	if( !iInternalProcessingNeeded )
		{
		TRAPD( err, iSource.SetRotationL( iRotationAngle ) );
		if( !err && iHorizontalMirroring )
			{
			TRAP( err, iSource.SetFlippingL() );
			}
		if( !err && iVerticalMirroring )
			{
			TRAP( err, iSource.SetMirroringL() );
			}
		if( err )
			{
			return err;
			}
		}

	// Start async load/process.
	TInt err( KErrNone );

	err = AsyncLoad();
	if( !err )
		{
		iViewerState = ELoad;
		SetActive();
		}

	return err;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::AsyncLoad
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::AsyncLoad()
	{
    // Internal state check, debug panic only
	__ASSERT_DEBUG( !iCacheDestination->Bitmap().Handle(), Panic( KErrGeneral ) );

	TInt err( KErrNone );

	// Load source bitmap

	iCacheSource->Reset();

	err = iCacheSource->Create( iDestinationSize, iSource.DisplayMode() );

	if( !err )
		{
		err = iSource.Load( iSourceRect, iStatus, *iCacheSource );
		}

	// Error cleanup if needed
	if( err )
		{
		iCacheSource->Reset();
		}
	return err;
	}



// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::Finish
// -----------------------------------------------------------------------------
TInt CIHLImageViewerExtJpg::Finish()
	{
    // Internal state check, debug panic only
	__ASSERT_DEBUG( iCacheSource->Bitmap().Handle(), Panic( KErrGeneral ) );

	TInt err( KErrNone );

	// Check if internal processor was used
	if( iInternalProcessingNeeded )
		{
		err = iDestination.Copy( *iCacheDestination, ETrue );
		iCacheDestination->Reset();
		}
	else
		{
		err = iDestination.Copy( *iCacheSource, ETrue );
		iCacheSource->Reset();
		}

	// Error cleanup if needed
	if( err )
		{
		iDestination.Reset();
		}

	return err;
	}


// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::SelfComplete
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::SelfComplete()
	{
	SetActive();
	iStatus = KRequestPending;
	TRequestStatus* status = &iStatus;
	User::RequestComplete( status, KErrNone );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::CalculateProcessSourceRect
// -----------------------------------------------------------------------------
TRect CIHLImageViewerExtJpg::CalculateProcessSourceRect( const TSize& aSourceCacheSize )
	{

	TRect loadRect( aSourceCacheSize );

	switch( iRotationAngle )
		{
		case ERotationAngle0:
			{
			break;
			}
		case ERotationAngle90:
			{
			TInt tmp( loadRect.iTl.iY );
			loadRect.iTl.iY = loadRect.iBr.iY;
			loadRect.iBr.iY = tmp;
			break;
			}
		case ERotationAngle180:
			{
			TPoint tmp( loadRect.iTl );
			loadRect.iTl = loadRect.iBr;
			loadRect.iBr = tmp;
			break;
			}
		case ERotationAngle270:
			{
			TInt tmp( loadRect.iTl.iX );
			loadRect.iTl.iX = loadRect.iBr.iX;
			loadRect.iBr.iX = tmp;
			break;
			}
		default:
			{
            // Internal state error, debug panic only
            #ifdef _DEBUG
    			Panic( KErrGeneral );
            #endif
			}
		}

	return loadRect;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::CalculateProcessDestinationRect
// -----------------------------------------------------------------------------
TRect CIHLImageViewerExtJpg::CalculateProcessDestinationRect()
	{
	TRect dstRect( iDestinationSize );
	switch( iRotationAngle )
		{
		case ERotationAngle0:
		case ERotationAngle180:
			{
			if( iHorizontalMirroring )
				{
				TInt tmp( dstRect.iTl.iX );
				dstRect.iTl.iX = dstRect.iBr.iX;
				dstRect.iBr.iX = tmp;
				}
			if( iVerticalMirroring )
				{
				TInt tmp( dstRect.iTl.iY );
				dstRect.iTl.iY = dstRect.iBr.iY;
				dstRect.iBr.iY = tmp;
				}
			break;
			}
		case ERotationAngle90:
		case ERotationAngle270:
			{
			if( iHorizontalMirroring )
				{
				TInt tmp( dstRect.iTl.iY );
				dstRect.iTl.iY = dstRect.iBr.iY;
				dstRect.iBr.iY = tmp;
				}
			if( iVerticalMirroring )
				{
				TInt tmp( dstRect.iTl.iX );
				dstRect.iTl.iX = dstRect.iBr.iX;
				dstRect.iBr.iX = tmp;
				}
			break;
			}
		default:
			{
            // Internal state error, debug panic only
            #ifdef _DEBUG
			    Panic( KErrGeneral );
            #endif
			}
		}
	return dstRect;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::CalculateZoomToFitRatio
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::CalculateZoomToFitRatio()
	{
	TSize sourceSize( SourceSize() );
	TReal widthRatio( TReal( iViewerSize.iWidth ) / TReal( sourceSize.iWidth ) );
	TReal heightRatio( TReal( iViewerSize.iHeight ) / TReal( sourceSize.iHeight ) );

	if( iOptions & MIHLImageViewer::EOptionIgnoreAspectRatio )
		{
		iZoomToFitRatio = ( widthRatio > heightRatio ) ? widthRatio : heightRatio;
		}
	else
		{
		iZoomToFitRatio = ( widthRatio < heightRatio ) ? widthRatio : heightRatio;
		}
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::CalculateSourceRectAndDestinationSize
// -----------------------------------------------------------------------------
void CIHLImageViewerExtJpg::CalculateSourceRectAndDestinationSize()
	{
	// Calculate new source rect
	TSize oldSourceRectSize( iSourceRect.Size() );

	TReal newSourceRectWidth;
	TReal newSourceRectHeight;
	TReal widthZoomRatio( iZoomRatio );
	TReal heightZoomRatio( iZoomRatio );

	if( iOptions & MIHLImageViewer::EOptionIgnoreAspectRatio )
		{
		TReal widthRatio( TReal( iViewerSize.iWidth ) / TReal( iSourceSize.iWidth ) );
		TReal heightRatio( TReal( iViewerSize.iHeight ) / TReal( iSourceSize.iHeight ) );
		if( widthRatio < heightRatio )
			{
			widthZoomRatio = widthZoomRatio * widthRatio / heightRatio;
			}
		else
			{
			heightZoomRatio = heightZoomRatio * heightRatio / widthRatio;
			}
		}

	if( iRotationAngle == ERotationAngle90 ||
		iRotationAngle == ERotationAngle270 )
		{
		newSourceRectWidth = iSrcBitmapScaleFactor * iViewerSize.iHeight / heightZoomRatio; //widthZoomRatio
		newSourceRectHeight = iSrcBitmapScaleFactor * iViewerSize.iWidth / widthZoomRatio; //heightZoomRatio
		}
	else
		{
		newSourceRectWidth = iSrcBitmapScaleFactor * iViewerSize.iWidth / widthZoomRatio;
		newSourceRectHeight = iSrcBitmapScaleFactor * iViewerSize.iHeight / heightZoomRatio;
		}

	// Check if source rect is not larger than source area
	if( newSourceRectWidth > iSourceSize.iWidth )
		{
		newSourceRectWidth = iSourceSize.iWidth;
		}
	if( newSourceRectHeight > iSourceSize.iHeight )
		{
		newSourceRectHeight = iSourceSize.iHeight;
		}
	iSourceRect.SetWidth( (TInt)newSourceRectWidth );
	iSourceRect.SetHeight( (TInt)newSourceRectHeight );

	// Calculate actual destination size (always same or smaller than iViewerSize !)
	if( iRotationAngle == ERotationAngle90 ||
		iRotationAngle == ERotationAngle270 )
		{
		iDestinationSize.iWidth = (TInt)( newSourceRectHeight * widthZoomRatio ); //heightZoomRatio
		iDestinationSize.iHeight = (TInt)( newSourceRectWidth * heightZoomRatio  ); //widthZoomRatio
		}
	else
		{
		iDestinationSize.iWidth = (TInt)( newSourceRectWidth * widthZoomRatio );
		iDestinationSize.iHeight = (TInt)( newSourceRectHeight * heightZoomRatio );
		}
	// Check that destination size is not rounded to zero
	if( iDestinationSize.iWidth == 0 )
		{
		iDestinationSize.iWidth = 1;
		}
	if( iDestinationSize.iHeight == 0 )
		{
		iDestinationSize.iHeight = 1;
		}

	// Move source rect keeping center point in same location
	iSourceRect.Move( ( oldSourceRectSize.iWidth - (TInt)newSourceRectWidth ) / 2,
					  ( oldSourceRectSize.iHeight - (TInt)newSourceRectHeight ) / 2 );

	// Move rect if partially out of source area
	TPoint moveOffset( 0, 0 );
	if( iSourceRect.iTl.iX < 0 )
		{
		moveOffset.iX = -iSourceRect.iTl.iX;
		}
	else if( iSourceRect.iBr.iX > iSourceSize.iWidth )
		{
		moveOffset.iX = iSourceSize.iWidth - iSourceRect.iBr.iX;
		}
	if( iSourceRect.iTl.iY < 0 ) //lint !e961
		{
		moveOffset.iY = -iSourceRect.iTl.iY;
		}
	else if( iSourceRect.iBr.iY > iSourceSize.iHeight )
		{
		moveOffset.iY = iSourceSize.iHeight - iSourceRect.iBr.iY;
		}
	iSourceRect.Move( moveOffset );  //lint !e961

	// Assert that rectangle is valid, debug build only
#ifdef _DEBUG
	AssertSourceRectAndDestinationSize();
#endif
	}

// -----------------------------------------------------------------------------
// CIHLImageViewerExtJpg::AssertSourceRectAndDestinationSize
// Used in debug build only
// -----------------------------------------------------------------------------
#ifdef _DEBUG
void CIHLImageViewerExtJpg::AssertSourceRectAndDestinationSize()
	{
	if( iSourceRect.iTl.iX < 0 ||
		iSourceRect.iBr.iX > iSourceSize.iWidth ||
		iSourceRect.iTl.iY < 0 ||
		iSourceRect.iBr.iY > iSourceSize.iHeight ||
		iDestinationSize.iWidth <= 0 || iDestinationSize.iHeight <= 0 )
		{
		Panic( KErrGeneral );
		}
	}
#endif

//  End of File


