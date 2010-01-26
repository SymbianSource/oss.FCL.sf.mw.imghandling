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
* Description:  Pure virtual interface for Image Viewer class.
*              : Class can be used to view, process and play animation
*              : from source image.
*
*/


#ifndef MIHLIMAGEVIEWER_H
#define MIHLIMAGEVIEWER_H

// INCLUDES
#include <MIHLViewer.h>
#include <e32std.h>

// FORWARD DECLARATION
class MIHLFilter;

// CLASS DECLARATION
/**
*  MIHLImageViewer
*
*  Pure virtual interface for Image Viewer class.
*  Class can be used to view, process and play animation
*  from source image.
*
*  If interface needs to be pushed into CleanupStack,
*  remember to use CleanupStackDeletePushL() function!
*  DO NOT USE CleanupStack::PushL()!!
*
*  @lib IHL.lib
*  @since 3.0
*/
class MIHLImageViewer : public MIHLViewer
	{
	public:

        /**
        * Virtual destructor.
		*/
		virtual ~MIHLImageViewer() {}

	public:

		/**
		* Flags to control viewer functionality.
		* These can be combined using an OR operation.
		* @since 3.0
		*/
		enum TOptions
			{
			EOptionIgnoreAspectRatio	= 0x01,
			EOptionUseBilinearInterpolation = 0x02
			};

        /**
        * Rotation angle of source image.
        * @since 3.0
		*/
		enum TRotationAngle
			{
			ERotationAngle0		= 0,
			ERotationAngle90	= 90,
			ERotationAngle180	= 180,
			ERotationAngle270	= 270,
			};

	public:

        /**
        * Check if source image is animated.
        * @since 3.0
		* @return ETrue if source image is animated. EFalse if not.
		*/
		virtual TBool IsAnimation() const = 0;

        /**
        * Start animation playback.
		* If source image is not animated, method does nothing.
        * @since 3.0
		*/
		virtual void Play() = 0;

        /**
        * Stop animation playback.
		* If source image is not animated or animation is
		* not played currently, method does nothing.
        * @since 3.0
		*/
		virtual void Stop() = 0;

        /**
        * Check if animation playback is currently running.
        * @since 3.0
		* @return ETrue if animation is running. EFalse if not.
		*/
		virtual TBool IsPlaying() const = 0;

        /**
        * Count of frames in animated image.
		* If not animated, return zero.
        * @since 3.0
		* @return Frames count of animation.
		*/
		virtual TInt AnimationFrameCount() const = 0;

        /**
        * Current animation frame in display.
        * @since 3.0
		* @return Current animation frame index.
		*/
		virtual TInt AnimationFrame() const = 0;

        /**
        * Change current animation frame.
        * @since 3.0
		* @param aFrameIndex New animation frame.
		* @return System wide error code.
		*/
		virtual TInt SetAnimationFrame( TInt aFrameIndex ) = 0;

        /**
        * Change viewer size.
		* This is also the maximum size of destination bitmap.
        * @since 3.0
		* @param aViewerSize New viewer size.
		* @return System wide error code.
		*/
		virtual TInt SetViewerSize( const TSize& aViewerSize, TInt srcBitmapScaleFactor = 1 ) = 0;

        /**
        * Viewer size.
		* This is also the maximum size of destination bitmap.
        * @since 3.0
		* @return Viewer size.
		*/
		virtual TSize ViewerSize() const = 0;

        /**
        * Move source clip rectangle.
		* This is the area that is used from source image
		* and processed to destination bitmap.
		* Check valid move area with SourceRect()
		* and SourceSize() methods.
        * @since 3.0
		* @param aDx Movement in X-axis.
		* @param aDy Movement in Y-axis.
		* @return System wide error code.
		*/
		virtual TInt MoveSourceRect( TInt aDx, TInt aDy ) = 0;

        /**
        * Set source clip rectangle position.
		* This is the area that is used from source image
		* and processed to destination bitmap.
		* Check valid move area with SourceRect()
		* and SourceSize() methods.
        * @since 3.0
		* @param aPosition Topleft posion of rectangle.
		* @return System wide error code.
		*/
		virtual TInt SetSourceRectPosition( const TPoint& aPosition ) = 0;

        /**
		* This is the area that is used from source image
		* and processed to destination bitmap.
        * @since 3.0
		* @return Source clip rectangle.
		*/
		virtual TRect SourceRect() const = 0;

        /**
		* Source image size.
		* Note that this will change if image is rotated.
        * @since 3.0
		* @return Source image size.
		*/
		virtual TSize SourceSize() const = 0;

        /**
		* Set new zoom ratio.
		* Zoom ratio is multiplier for scaling,
		* 0,5 is 50%, 1.0 is 100%, 2,0 is 200% and so on.
		* Value cannot be zero or negative.
        * @since 3.0
		* @param aZoomRatio New zoom ratio.
		* @return System wide error code.
		*/
		virtual TInt SetZoomRatio( TReal aZoomRatio ) = 0;

        /**
		* Current zoom ratio.
        * @since 3.0
		* @return Current zoom ratio.
		*/
		virtual TReal ZoomRatio() const = 0;

        /**
		* Zoom to fit ratio.
		* Using this zoom ratio, image will fit
		* exactly to viewer.
        * @since 3.0
		* @return Zoom to fit ratio.
		*/
		virtual TReal ZoomToFitRatio() const = 0;

        /**
		* Rotate source image 90 degree clockwise.
		* Source rect is also moved to remain in same position
		* where it was according to source image data.
        * @since 3.0
		* @return System wide error code.
		*/
		virtual TInt RotateClockwise() = 0;

        /**
		* Rotate source image 90 degree counterclockwise.
		* Source rect is also moved to remain in same position
		* where it was according to source image data.
        * @since 3.0
		* @return System wide error code.
		*/
		virtual TInt RotateCounterClockwise() = 0;

        /**
		* Set rotation angle. Use values from TRotationAngle enumeration.
		* Source rect is also moved to remain in same position
		* where it was according to source image data.
        * @since 3.0
		* @param aRotationAngle New rotation angle.
		* @return System wide error code.
		*/
		virtual TInt SetRotationAngle( TInt aRotationAngle ) = 0;

        /**
		* Current rotation angle.
        * @since 3.0
		* @return Current rotation angle.
		*/
		virtual TInt RotationAngle() const = 0;

        /**
		* Set vertical mirroring for source image.
		* Source rect is also moved to remain in same position
		* where it was according to source image data.
        * @since 3.0
		* @param aValue Mirroring flag.
		* @return System wide error code.
		*/
		virtual TInt SetVerticalMirroring( TBool aValue ) = 0;

        /**
		* Check if source is vertically mirrored.
        * @since 3.0
		* @return ETrue if mirrored, EFalse if not.
		*/
		virtual TBool VerticalMirroring() const = 0;

        /**
		* Set horizontal mirroring for source image.
		* Source rect is also moved to remain in same position
		* where it was according to source image data.
        * @since 3.0
		* @param aValue Mirroring flag.
		* @return System wide error code.
		*/
		virtual TInt SetHorizontalMirroring( TBool aValue ) = 0;

        /**
		* Check if source is horizontally mirrored.
        * @since 3.0
		* @return ETrue if mirrored, EFalse if not.
		*/
		virtual TBool HorizontalMirroring() const = 0;

        /**
        * Set filter.
		* For future use, not used currently!
        * @since 3.0
        */
		virtual void SetFilter( MIHLFilter* aFilter ) = 0;
	};

#endif // MIHLIMAGEVIEWER_H

// End of File
