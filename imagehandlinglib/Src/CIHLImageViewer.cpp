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
* Description:  Implementation of Image Viewer class.
*
*/


// INCLUDE FILES
#include "CIHLImageViewer.h"

#include "CIHLBitmap.h"
#include "CIHLBitmapProcessor.h"
#include "IHLImplementationIds.h"
#include "IHLDebugPrint.h" // Debug print
#include <IHLInterfaceIds.h>
#include <MIHLViewerObserver.h>
#include <MIHLFileImage.h>
#include <fbs.h>
#include <e32math.h>

// Private namespace for constants and functions
namespace
	{
	// Panic function
    _LIT( KIHLPanicString, "IHLImageViewer" );
    void Panic( TInt aPanicCode ) { User::Panic( KIHLPanicString, aPanicCode ); }
	}

// ============================ MEMBER FUNCTIONS ===============================
// -----------------------------------------------------------------------------
//
// C++ constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
CIHLImageViewer::CIHLImageViewer( const TSize& aViewerSize,
								  MIHLFileImage& aSource,
								  MIHLBitmap& aDestination,
								  MIHLViewerObserver& aObserver,
								  const TUint32 aOptions )
:CTimer( EPriorityNormal ), iObserver( aObserver ),
iSource( aSource ), iDestination( aDestination ),
iOptions( aOptions ), iViewerSize( aViewerSize ), iSrcBitmapScaleFactor(1)
    {
	CActiveScheduler::Add( this );
	iAnimDelay = 0;
	iAnimLastFrameDrawTime = 0;
	iAnimDrawStartFastTime = 0; 
    }

// -----------------------------------------------------------------------------
//
// Two-phased constructor.
// -----------------------------------------------------------------------------
CIHLImageViewer* CIHLImageViewer::NewL( const TSize& aViewerSize,
										MIHLFileImage& aSource,
										MIHLBitmap& aDestination,
										MIHLViewerObserver& aObserver,
										const TUint32 aOptions )
    {
    CIHLImageViewer* self = new( ELeave ) CIHLImageViewer(
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
void CIHLImageViewer::ConstructL()
	{
	CTimer::ConstructL();

	HAL::Get(HALData::EFastCounterFrequency, iTickPeriod);
	
	iCacheSource = CIHLBitmap::NewL();
	iCacheDestination = CIHLBitmap::NewL();
	iProcessor = CIHLBitmapProcessor::NewL( iOptions );

	// Initialize settings and start load (default is zoom to fit)
	iSourceSize = iSource.Size();
	CalculateZoomToFitRatio();
	User::LeaveIfError( SetZoomRatio( iZoomToFitRatio ) );
	}

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
CIHLImageViewer::~CIHLImageViewer()
    {
	Cancel();
	delete iProcessor;
	delete iCacheDestination;
	delete iCacheSource;
    }

// -----------------------------------------------------------------------------
// CIHLImageViewer::Id
// -----------------------------------------------------------------------------
TIHLInterfaceType CIHLImageViewer::Type() const
	{
	return TIHLInterfaceType( KIHLInterfaceIdImageViewer, KIHLImplementationIdImageViewer );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::IsAnimation
// -----------------------------------------------------------------------------
TBool CIHLImageViewer::IsAnimation() const
	{
	return iSource.IsAnimation();
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SheduleNextFrame
// -----------------------------------------------------------------------------
void CIHLImageViewer::SheduleNextFrame(TTimeIntervalMicroSeconds32 aDelay)
	{
	TInt delay=aDelay.Int();
	
	if (delay == 0) // 0 second deleay set in GIF file
		{
			iAnimDelay = TTimeIntervalMicroSeconds32(100000-iAnimLastFrameDrawTime);
		}
	else
		{
			iAnimDelay = delay - iAnimLastFrameDrawTime;
		}
	
	if (iAnimDelay.Int() <=0) //last frame drawn longer than animation frame interval. 
		{
			iAnimDelay = KMinimumInterval;
		}
	iAnimInitFastTime=User::FastCounter(); //store the start time
	HighRes(iAnimDelay);
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::Play
// -----------------------------------------------------------------------------
void CIHLImageViewer::Play()
	{
	if( iSource.IsAnimation() )
		{
		iIsPlaying = ETrue;
		if( iViewerState == EInactive)
			{
			iViewerState = EAnimation;
			SheduleNextFrame(	iSource.AnimationFrameDelay( iAnimationIndex )	);
			}
		}
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::Stop
// -----------------------------------------------------------------------------
void CIHLImageViewer::Stop()
	{
	iIsPlaying = EFalse;
	if( iViewerState == EAnimation )
		{
		Cancel();
		}
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::IsPlaying
// -----------------------------------------------------------------------------
TBool CIHLImageViewer::IsPlaying() const
	{
	return iIsPlaying;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::AnimationFrameCount
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::AnimationFrameCount() const
	{
	return iSource.AnimationFrameCount();
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::AnimationFrame
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::AnimationFrame() const
	{
	return iAnimationIndex;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SetAnimationFrame
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::SetAnimationFrame( TInt aFrameIndex )
	{
    // Public API, boundary check -> panic also on hardware
	__ASSERT_ALWAYS( aFrameIndex >= 0 &&
		aFrameIndex < iSource.AnimationFrameCount(), Panic( KErrArgument ) );

	// Save new index and clear old source cache to force load new frame
	iAnimationIndex = aFrameIndex;
	iCacheSource->Reset();

	// Start load
	return ApplySettings();
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SetViewerSize
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::SetViewerSize( const TSize& aViewerSize, int aSrcBitmapScaleFactor )
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
// CIHLImageViewer::ViewerSize
// -----------------------------------------------------------------------------
TSize CIHLImageViewer::ViewerSize() const
	{
	return iViewerSize;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::MoveSourceRect
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::MoveSourceRect( TInt aDx, TInt aDy )
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
// CIHLImageViewer::SetSourceRectPosition
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::SetSourceRectPosition( const TPoint& aPosition )
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
// CIHLImageViewer::SourceRect
// -----------------------------------------------------------------------------
TRect CIHLImageViewer::SourceRect() const
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
// CIHLImageViewer::SourceSize
// -----------------------------------------------------------------------------
TSize CIHLImageViewer::SourceSize() const
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
// CIHLImageViewer::SetZoomRatio
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::SetZoomRatio( TReal aZoomRatio )
	{
	if( aZoomRatio <= 0 || Math::IsInfinite( aZoomRatio ) )
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
// CIHLImageViewer::ZoomRatio
// -----------------------------------------------------------------------------
TReal CIHLImageViewer::ZoomRatio() const
	{
	return iZoomRatio;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::ZoomToFitRatio
// -----------------------------------------------------------------------------
TReal CIHLImageViewer::ZoomToFitRatio() const
	{
	return iZoomToFitRatio;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::RotateClockwise
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::RotateClockwise()
	{
	TInt rotationAngle( iRotationAngle + ERotationAngle90 );
	if( rotationAngle > ERotationAngle270 )
		{
		rotationAngle = ERotationAngle0;
		}

	return SetRotationAngle( rotationAngle );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::RotateCounterClockwise
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::RotateCounterClockwise()
	{
	TInt rotationAngle( iRotationAngle - ERotationAngle90 );
	if( rotationAngle < ERotationAngle0 )
		{
		rotationAngle = ERotationAngle270;
		}

	return SetRotationAngle( rotationAngle );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SetRotationAngle
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::SetRotationAngle( TInt aRotationAngle )
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
// CIHLImageViewer::RotationAngle
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::RotationAngle() const
	{
	return iRotationAngle;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SetVerticalMirroring
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::SetVerticalMirroring( TBool aValue )
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
// CIHLImageViewer::VerticalMirroring
// -----------------------------------------------------------------------------
TBool CIHLImageViewer::VerticalMirroring() const
	{
	return iVerticalMirroring;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SetHorizontalMirroring
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::SetHorizontalMirroring( TBool aValue )
	{
	if( iHorizontalMirroring != aValue )
		{
		iHorizontalMirroring = aValue;
		return ApplySettings();
		}
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::HorizontalMirroring
// -----------------------------------------------------------------------------
TBool CIHLImageViewer::HorizontalMirroring() const
	{
	return iHorizontalMirroring;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SetFilter
// -----------------------------------------------------------------------------
void CIHLImageViewer::SetFilter( MIHLFilter* /*aFilter*/ )
	{
	// Not in use
	}



// Private methods
// -----------------------------------------------------------------------------
// CIHLImageViewer::DoCancel
// -----------------------------------------------------------------------------
void CIHLImageViewer::DoCancel()
	{
	CTimer::DoCancel();
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

	// Set state to inactive
	iViewerState = EInactive;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::RunL
// -----------------------------------------------------------------------------
void CIHLImageViewer::RunL()
	{
	User::LeaveIfError( iStatus.Int() );

	
	TInt err( KErrNone );
	switch( iViewerState )
		{
		case ELoad:
			{
			// Async load successful; start async processing if needed
            if( NeedAsyncProcess() )
                {
                err = AsyncProcess();
                if( !err )
                    {
                    iViewerState = EProcess;
                    SetActive();
                    }
                break;
                }
            else
                {
                iViewerState=EInactive;
                // Flowtrough to EProcess if no error
                err = SyncProcess();
                if( err )
                    {
                    break;
                    }
                }
			}
		case EProcess:
			{
			// Process successful; finish and check if animation is needed
			err = Finish();
			if( !err )
				{
					TInt64 currentTime = User::FastCounter();
					if(iAnimDrawStartFastTime > 0) 
						{
						iAnimLastFrameDrawTime=(currentTime-iAnimDrawStartFastTime)*1000000/iTickPeriod;
						}

					if( iIsPlaying )
						{
						iViewerState = EAnimation;
						SheduleNextFrame(iSource.AnimationFrameDelay( iAnimationIndex ));
						}
					else 
						{
						iViewerState = EInactive;
						}
					// Notify client
					// NOTE! client may call any API method in this point!
					iObserver.ViewerBitmapChangedL();
				}
			break;
			}
		case EAnimation:
			{
			// Check if still playing..
			iAnimDrawStartFastTime = User::FastCounter();
			if( iIsPlaying )
				{
				// Change animation frame + 1
				UpdateAnimationIndex();
				err = AsyncLoad();
				if( !err )
					{
					iViewerState = ELoad;
					SetActive();
					}
				else 
					{
					iViewerState = EInactive;
					}
				}
			else 
				{
				iViewerState = EInactive;
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


	User::LeaveIfError( err );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::RunError
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::RunError( TInt aError )
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
// CIHLImageViewer::Set
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::ApplySettings()
	{
	// Assert source rect and destination, debug build only
#ifdef _DEBUG
	AssertSourceRectAndDestinationSize();
#endif

	// Cancel current process if any
	Cancel();

	// Check if cached source bitmap is already valid for processing..
	TBool validSourceCache( EFalse );
	const CFbsBitmap& cacheSource = iCacheSource->Bitmap();
	if( cacheSource.Handle() )
		{
		const TSize cacheSize( cacheSource.SizeInPixels() );
		const TSize minLoadSize( MinimumSourceLoadSize() );
		if( cacheSize.iWidth >= minLoadSize.iWidth &&
			cacheSize.iHeight >= minLoadSize.iHeight )
			{
			validSourceCache = ETrue;
			}
		}

	// Start async load/process..
	TInt err( KErrNone );
	if( validSourceCache )
		{
        if( NeedAsyncProcess() )
            {
            err = AsyncProcess();
            if( !err )
                {
                iViewerState = EProcess;
                SetActive();
                }
            }
        else
            {
            err = SyncProcess();
            if( !err )
                {
                iViewerState = EProcess;
                SelfComplete();
                }
            }
		}
	else
		{
		err = AsyncLoad();
		if( !err )
			{
			iViewerState = ELoad;
			SetActive();
			}
		}

	return err;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::AsyncLoad
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::AsyncLoad()
	{
    // Internal state check, debug panic only
	__ASSERT_DEBUG( !iCacheDestination->Bitmap().Handle(), Panic( KErrGeneral ) );

	TInt err( KErrNone );
	if( !iSource.IsAnimation() )
		{
		// Do not reset animation frame because loading
		// might be faster if previous frame can be utilized
		iCacheSource->Reset();
		}
	if( !iCacheSource->Bitmap().Handle() )
		{
		TDisplayMode transparency( iSource.MaskDisplayMode() );
		if( transparency )
			{
			err = iCacheSource->Create( MinimumSourceLoadSize(),
										iSource.DisplayMode(),
										transparency );
			}
		else
			{
			err = iCacheSource->Create( MinimumSourceLoadSize(),
										iSource.DisplayMode() );
			}
		}

	// Load source bitmap
	if( !err )
		{
		if( iSource.IsAnimation() )
			{
			err = iSource.LoadAnimation( iStatus, *iCacheSource, iAnimationIndex );
			}
		else
			{
			err = iSource.Load( iStatus, *iCacheSource, iSource.ImageIndex() );
			}

		}

	// Error cleanup if needed
	if( err )
		{
		iCacheSource->Reset();
		}
	return err;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::NeedAsyncProcess
// -----------------------------------------------------------------------------
TBool CIHLImageViewer::NeedAsyncProcess()
    {
    // Internal state check, debug panic only
    __ASSERT_DEBUG( iCacheSource->Bitmap().Handle(), Panic( KErrGeneral ) );
    __ASSERT_DEBUG( !iCacheDestination->Bitmap().Handle(), Panic( KErrGeneral ) );

    // First check if destination size differs from source size
    // (=scaling needed)
    TSize scaledSrcSize( iCacheSource->Bitmap().SizeInPixels() );
    if( scaledSrcSize.iWidth != iDestinationSize.iWidth ||
        scaledSrcSize.iHeight != iDestinationSize.iHeight )
        {
        return ETrue;
        }

    // Then check if source rect and source size differs
    // (=clipping needed)
    if( iSourceRect.iTl != TPoint( 0,0 ) ||
        iSourceRect.iBr != iSource.Size().AsPoint() )
        {
        return ETrue;
        }

    // Finally check rotation and mirroring
    if( iRotationAngle == ERotationAngle0 &&
        !iHorizontalMirroring &&
        !iVerticalMirroring )
        {
        return EFalse;
        }
    else if( iRotationAngle == ERotationAngle180 &&
        iHorizontalMirroring &&
        iVerticalMirroring )
        {
        return EFalse;
        }
    else
        {
        return ETrue;
        }
    }

// -----------------------------------------------------------------------------
// CIHLImageViewer::AsyncProcess
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::AsyncProcess()
	{
    IHL_DEBUG1( KIHLDebug, "IHL - CIHLImageViewer - AsyncProcess" );

    // Internal state check, debug panic only
	__ASSERT_DEBUG( iCacheSource->Bitmap().Handle(), Panic( KErrGeneral ) );
	__ASSERT_DEBUG( !iCacheDestination->Bitmap().Handle(), Panic( KErrGeneral ) );

	const CFbsBitmap& srcBitmap = iCacheSource->Bitmap();
	const CFbsBitmap& srcMask = iCacheSource->Mask();
	TInt err( KErrNone );

	// Create new cached destination
	if( srcMask.Handle() )
		{
		err = iCacheDestination->Create( iDestinationSize,
									srcBitmap.DisplayMode(), srcMask.DisplayMode() );
		}
	else
		{
		err = iCacheDestination->Create( iDestinationSize, srcBitmap.DisplayMode() );
		}

	// Scale and clip bitmap from cached source to new cached destination
	if( !err )
		{
		TRect scaledSourceRect(
            CalculateProcessSourceRect( srcBitmap.SizeInPixels() ) );

		TRect destinationRect( CalculateProcessDestinationRect() );

		err = iProcessor->Process( iStatus, *iCacheSource, scaledSourceRect,
									  *iCacheDestination, destinationRect );
		}

	// Error cleanup if needed
	if( err )
		{
		iCacheDestination->Reset();
		}

	return err;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SyncProcess
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::SyncProcess()
    {
    IHL_DEBUG1( KIHLDebug, "IHL - CIHLImageViewer - SyncProcess" );

    // Internal state check, debug panic only
    __ASSERT_DEBUG( iCacheSource->Bitmap().Handle(), Panic( KErrGeneral ) );
    __ASSERT_DEBUG( !iCacheDestination->Bitmap().Handle(), Panic( KErrGeneral ) );

    // Duplicate destination cache to destination bitmap
    TInt err( iCacheDestination->Copy( *iCacheSource, EFalse ) );
    return err;
    }

// -----------------------------------------------------------------------------
// CIHLImageViewer::Finish
// -----------------------------------------------------------------------------
TInt CIHLImageViewer::Finish()
	{
    // Internal state check, debug panic only
	__ASSERT_DEBUG( iCacheDestination->Bitmap().Handle(), Panic( KErrGeneral ) );

	// Duplicate destination cache to destination bitmap
	TInt err( iDestination.Copy( *iCacheDestination, ETrue ) );
	iCacheDestination->Reset();

	// Error cleanup if needed
	if( err )
		{
		iDestination.Reset();
		}
	return err;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::UpdateAnimationIndex
// -----------------------------------------------------------------------------
void CIHLImageViewer::UpdateAnimationIndex()
	{
    // Internal state check, debug panic only
	__ASSERT_DEBUG( iIsPlaying, Panic( KErrGeneral ) );

	// Check if animation is in last frame
	if( iAnimationIndex == iSource.AnimationFrameCount() - 1 )
		{
		iAnimationIndex = 0;
		}
	else
		{
		// Not in last frame; move to next frame
		iAnimationIndex++;
		}
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::SelfComplete
// -----------------------------------------------------------------------------
void CIHLImageViewer::SelfComplete()
	{
	SetActive();
	iStatus = KRequestPending;
	TRequestStatus* status = &iStatus;
	User::RequestComplete( status, KErrNone );
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::MinimumSourceLoadSize
// -----------------------------------------------------------------------------
TSize CIHLImageViewer::MinimumSourceLoadSize()
	{
	TSize minumumLoadSize( iSource.Size() );

	if( iSource.IsFullyScaleable() )
	    {
    	const TSize originalSize( iSource.Size() );
    	switch( iRotationAngle )
    		{
    		case ERotationAngle0:
    		case ERotationAngle180:
    			{
            	minumumLoadSize.iWidth = originalSize.iWidth * iDestinationSize.iWidth / Abs( iSourceRect.Width() );
            	minumumLoadSize.iHeight = originalSize.iHeight * iDestinationSize.iHeight / Abs( iSourceRect.Height() );
    			break;
    			}
    		case ERotationAngle90:
    		case ERotationAngle270:
    			{
            	minumumLoadSize.iWidth = originalSize.iWidth * iDestinationSize.iWidth / Abs( iSourceRect.Height() );
            	minumumLoadSize.iHeight = originalSize.iHeight * iDestinationSize.iHeight / Abs( iSourceRect.Width() );
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
    		
        // Limit fully scaleable loadsize to original size
    	if( minumumLoadSize.iWidth > originalSize.iWidth ||
    	    minumumLoadSize.iHeight > originalSize.iHeight )
    	    {
    	    minumumLoadSize = originalSize;
    	    }
	    }
	else
	    {
    	const RArray<TSize>& loadSizeArray = iSource.CustomLoadSizeArray();
    	const TInt count( loadSizeArray.Count() );

    	if( count )
    		{
    		TRect loadRect;
    		TSize loadRectSize;
    		TBool indexFound( EFalse );
    		for( TInt i( 0 ); ( i < count ) && !indexFound; ++i )
    			{
    			loadRect = CalculateProcessSourceRect( loadSizeArray[ i ] );
    			loadRectSize = loadRect.Size();
    			loadRectSize.iWidth = Abs( loadRectSize.iWidth );
    			loadRectSize.iHeight = Abs( loadRectSize.iHeight );

    			if( iDestinationSize.iWidth <= loadRectSize.iWidth &&
    				iDestinationSize.iHeight <= loadRectSize.iHeight )
    				{
    				minumumLoadSize = loadSizeArray[ i ];
    				indexFound = ETrue;
    				}
    			}
    		}
	    }

	return minumumLoadSize;
	}

// -----------------------------------------------------------------------------
// CIHLImageViewer::CalculateProcessSourceRect
// -----------------------------------------------------------------------------
TRect CIHLImageViewer::CalculateProcessSourceRect( const TSize& aLoadSize )
	{
	const TSize originalSize( iSource.Size() );
	TRect loadRect;
	if ( aLoadSize.iWidth > KErrNone && aLoadSize.iHeight > KErrNone )
		{
		loadRect.iTl.iX = iSourceRect.iTl.iX * aLoadSize.iWidth / originalSize.iWidth;
		loadRect.iTl.iY = iSourceRect.iTl.iY * aLoadSize.iHeight / originalSize.iHeight;
		loadRect.iBr.iX = iSourceRect.iBr.iX * aLoadSize.iWidth / originalSize.iWidth;
		loadRect.iBr.iY = iSourceRect.iBr.iY * aLoadSize.iHeight / originalSize.iHeight;
		}
	else
		{
		loadRect.SetRect(0,0,0,0);
		return loadRect;
		}

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
// CIHLImageViewer::CalculateProcessDestinationRect
// -----------------------------------------------------------------------------
TRect CIHLImageViewer::CalculateProcessDestinationRect()
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
// CIHLImageViewer::CalculateZoomToFitRatio
// -----------------------------------------------------------------------------
void CIHLImageViewer::CalculateZoomToFitRatio()
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
// CIHLImageViewer::CalculateSourceRectAndDestinationSize
// -----------------------------------------------------------------------------
void CIHLImageViewer::CalculateSourceRectAndDestinationSize()
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
		
	// Rounding the results
	TReal roundedWidth;
	Math::Round( roundedWidth, newSourceRectWidth, 0 );
	TReal roundedHeight;
	Math::Round( roundedHeight, newSourceRectHeight, 0 );
	
	iSourceRect.SetWidth( (TInt)roundedWidth );
	iSourceRect.SetHeight( (TInt)roundedHeight );

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
// CIHLImageViewer::AssertSourceRectAndDestinationSize
// Used in debug build only
// -----------------------------------------------------------------------------
#ifdef _DEBUG
void CIHLImageViewer::AssertSourceRectAndDestinationSize()
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
