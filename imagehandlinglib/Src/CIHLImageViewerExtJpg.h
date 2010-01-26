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
* Description:  Implementation of Image Viewer class.
*
*/


#ifndef CIHLIMAGEVIEWEREXTJPG_H
#define CIHLIMAGEVIEWEREXTJPG_H

// INCLUDES
#include "MIHLImageViewer.h"
#include <e32base.h>

//FORWARD DECLARATIONS
class MIHLFileImageExtJpg;
class MIHLBitmap;
class MIHLViewerObserver;
class CIHLBitmap;
class CIHLBitmapProcessor;

// CLASS DECLARATION
/**
*  CIHLImageViewer
*
*  Image Viewer implementation class.
*  @lib IHL.lib
*  @since 3.0
*/
NONSHARABLE_CLASS( CIHLImageViewerExtJpg ) : public CTimer, public MIHLImageViewer
    {
    public:  // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        static CIHLImageViewerExtJpg* NewL( const TSize& aViewerSize,
									  MIHLFileImageExtJpg& aSource,
									  MIHLBitmap& aDestination,
									  MIHLViewerObserver& aObserver,
									  const TUint32 aOptions );

        /*
        * Virtual destructor.
        */
        virtual ~CIHLImageViewerExtJpg();

	public: // From base class MIHLViewer
		
		  /*
        * From MIHLViewer, see base class header.
        */
		TIHLInterfaceType Type() const;
		
	public:
	
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

	public: 

      

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
        * Finish processing and notify client.
        */
		TInt Finish();

        /*
        * Complete dummy request. This causes RunL() method to be called.
        * Needed by state machine.
        */
		void SelfComplete();

        /*
        * Calculate source bitmap rectangle needed by processor.
        * Rectangle is affected by rotating and mirroring settings etc.
        */
		TRect CalculateProcessSourceRect( const TSize& aSourceCacheSize );

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
		void CalculateSourceRectAndDestinationSize();

        /*
        * Assert if source bitmap rectangle and destination bitmpa size are valid.
        * This method is called by CalculateSourceRectAndDestinationSize()
        */
#ifdef _DEBUG
		void AssertSourceRectAndDestinationSize();
#endif

	private: // Private constructors

        void ConstructL();
        CIHLImageViewerExtJpg( const TSize& aViewerSize,
						 MIHLFileImageExtJpg& aSource,
						 MIHLBitmap& aDestination,
						 MIHLViewerObserver& aObserver,
						 const TUint32 aOptions );

	private: // Private data types

		enum TViewerState
			{
			EInactive,
			ELoad,
			EProcess
			};

	private: // Data

		// Viewer state
		TViewerState iViewerState;

		// Ref: Observers
		MIHLViewerObserver& iObserver;

		// Ref: Viewer source image
		MIHLFileImageExtJpg& iSource;
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

		// Decoder capabilities
		TInt iCapabilities;
		
		// Internal processor flag
		TBool iInternalProcessingNeeded;
	
		// Own: Bitmap caches
		CIHLBitmap* iCacheSource;
		CIHLBitmap* iCacheDestination;
		// Own: Bitmap processor
		CIHLBitmapProcessor* iProcessor;
		// src bitmap scale factor
		TInt iSrcBitmapScaleFactor;

    };

#endif // CIHLIMAGEVIEWEREXTJPG_H

// End of File
