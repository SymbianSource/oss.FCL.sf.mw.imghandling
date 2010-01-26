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


#ifndef CIHLIMAGEVIEWER_H
#define CIHLIMAGEVIEWER_H

// INCLUDES
#include <MIHLImageViewer.h>
#include <e32base.h>
#include <hal.h>

//FORWARD DECLARATIONS
class MIHLFileImage;
class MIHLBitmap;
class MIHLViewerObserver;
class CIHLBitmap;
class CIHLBitmapProcessor;

#define KMinimumInterval 10000

// CLASS DECLARATION
/**
*  CIHLImageViewer
*
*  Image Viewer implementation class.
*  @lib IHL.lib
*  @since 3.0
*/
NONSHARABLE_CLASS( CIHLImageViewer ) : public CTimer, public MIHLImageViewer
    {
    public:  // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        static CIHLImageViewer* NewL( const TSize& aViewerSize,
									  MIHLFileImage& aSource,
									  MIHLBitmap& aDestination,
									  MIHLViewerObserver& aObserver,
									  const TUint32 aOptions );

        /*
        * Virtual destructor.
        */
        virtual ~CIHLImageViewer();

	public: // From base class MIHLViewer

        /*
        * From MIHLViewer, see base class header.
        */
		TIHLInterfaceType Type() const;

	public: // From base class MIHLImageViewer

        /*
        * From MIHLImageViewer, see base class header.
        */
		TBool IsAnimation() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		void Play();

        /*
        * From MIHLImageViewer, see base class header.
        */
		void Stop();

        /*
        * From MIHLImageViewer, see base class header.
        */
		TBool IsPlaying() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt AnimationFrameCount() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt AnimationFrame() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt SetAnimationFrame( TInt aFrameIndex );

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt SetViewerSize( const TSize& aViewerSize, TInt srcBitmapScaleFactor = 1 );

        /*
        * From MIHLImageViewer, see base class header.
        */
		TSize ViewerSize() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt MoveSourceRect( TInt aDx, TInt aDy );

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt SetSourceRectPosition( const TPoint& aPosition );

        /*
        * From MIHLImageViewer, see base class header.
        */
		TRect SourceRect() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TSize SourceSize() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt SetZoomRatio( TReal aZoomRatio );

        /*
        * From MIHLImageViewer, see base class header.
        */
		TReal ZoomRatio() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TReal ZoomToFitRatio() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt RotateClockwise();

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt RotateCounterClockwise();

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt SetRotationAngle( TInt aRotationAngle );

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt RotationAngle() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt SetVerticalMirroring( TBool aValue );

        /*
        * From MIHLImageViewer, see base class header.
        */
		TBool VerticalMirroring() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		TInt SetHorizontalMirroring( TBool aValue );

        /*
        * From MIHLImageViewer, see base class header.
        */
		TBool HorizontalMirroring() const;

        /*
        * From MIHLImageViewer, see base class header.
        */
		void SetFilter( MIHLFilter* aFilter );

	public: // From base class CTimer

        /*
        * From CTimer, see base class header.
        */
		void DoCancel();

        /*
        * From CTimer, see base class header.
        */
		void RunL();

        /*
        * From CTimer, see base class header.
        */
		TInt RunError( TInt aError );

	private: // Private methods

        /*
        * Start asynchronous processing with new settings.
        * This is needed always when settings are changed.
        */
		TInt ApplySettings();

        /*
        * Start load process source bitmap.
        */
		TInt AsyncLoad();

        /*
        * Check if process is needed.
        */
        TBool NeedAsyncProcess();

        /*
        * Start processing with new settings.
        */
		TInt AsyncProcess();

        /*
        * No external processing.
        * Just duplicates source to destination.
        */
        TInt SyncProcess();

        /*
        * Finish processing and notify client.
        */
		TInt Finish();

        /*
        * Update frame index when playing animation.
        */
		void UpdateAnimationIndex();

        /*
        * Complete dummy request. This causes RunL() method to be called.
        * Needed by state machine.
        */
		void SelfComplete();

        /*
        * Calculate minimum source bitmap size (using fixed loadsizes)
        * needed by processing and destination bitmap.
        */
		TSize MinimumSourceLoadSize();

        /*
        * Calculate source bitmap rectangle needed by processor.
        * Rectangle is affected by rotating and mirroring settings etc.
        */
		TRect CalculateProcessSourceRect( const TSize& aLoadSize );

        /*
        * Calculate destination bitmap rectangle needed by processor.
        * Rectangle is affected by rotating and mirroring settings etc.
        */
		TRect CalculateProcessDestinationRect();

        /*
        * Calculate needed zoomratio for "fit-to-screen" zooming.
        */
		void CalculateZoomToFitRatio();

        /*
        * Calculate source bitmap rectangle and destination bitmap size.
        * These values needs to be recalculated when rectangle setting is changed.
        */
		void CalculateSourceRectAndDestinationSize( );
		
		/*
		* Shedules next frame of animation in using aDelay CTimer::Hires().
		* If this is not the first frame the shedule time is decreased by 
		* previous frame draw time.
		*/
		void SheduleNextFrame(TTimeIntervalMicroSeconds32 aDelay);

        /*
        * Assert if source bitmap rectangle and destination bitmpa size are valid.
        * This method is called by CalculateSourceRectAndDestinationSize()
        */
#ifdef _DEBUG
		void AssertSourceRectAndDestinationSize();
#endif

	private: // Private constructors

        void ConstructL();
        CIHLImageViewer( const TSize& aViewerSize,
						 MIHLFileImage& aSource,
						 MIHLBitmap& aDestination,
						 MIHLViewerObserver& aObserver,
						 const TUint32 aOptions );

	private: // Private data types

		enum TViewerState
			{
			EInactive,
			ELoad,
			EProcess,
			EAnimation,
			};

	private: // Data

		// Viewer state
		TViewerState iViewerState;

		// Ref: Observers
		MIHLViewerObserver& iObserver;

		// Ref: Viewer source image
		MIHLFileImage& iSource;
		TSize iSourceSize;
		TRect iSourceRect;

		// Ref: Viewer destination bitmap
		MIHLBitmap& iDestination;
		TSize iDestinationSize;

		// Viewing settings
		const TUint32 iOptions;
		TSize iViewerSize;
		TReal iZoomRatio;
		TReal iZoomToFitRatio;
		TInt iRotationAngle;
		TBool iVerticalMirroring;
		TBool iHorizontalMirroring;

		// Own: Bitmap caches
		CIHLBitmap* iCacheSource;
		CIHLBitmap* iCacheDestination;

		// Own: Bitmap processor
		CIHLBitmapProcessor* iProcessor;

		// Animation info
		TBool iIsPlaying;
		TInt iAnimationIndex;

		// src bitmap scale factor
		TInt iSrcBitmapScaleFactor;
		
		// Animation time tweaking
		TInt64 iAnimInitFastTime;			//clock ticks
		TInt64 iAnimDrawStartFastTime;		//clock tics
		TInt iAnimLastFrameDrawTime; 		//micro seconds
		TInt iTickPeriod;					//clock tick period
		TTimeIntervalMicroSeconds32 iAnimDelay;
		
		

    };

#endif // CIHLIMAGEVIEWER_H

// End of File
