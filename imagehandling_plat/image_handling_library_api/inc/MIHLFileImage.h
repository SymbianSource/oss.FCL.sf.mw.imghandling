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
* Description:  Pure virtual interface for file based image class.
*              : This class acts as a window into single image or animation
*              : in one physical file. Note that file will be locked as long
*              : as instance of this class is present.
*
*/


#ifndef MIHLFILEIMAGE_H
#define MIHLFILEIMAGE_H

// INCLUDES
#include <MIHLImage.h>
#include <gdi.h>

// FORWARD DECLARATION
class MIHLBitmap;
class MIHLFilter;

// CLASS DECLARATION
/**
*  MIHLFileImage
*
*  Pure virtual interface for file based image class.
*  This class acts as a window into single image or animation
*  in one physical file. Note that file will be locked as long
*  as instance of this class is present.
*
*  If interface needs to be pushed into CleanupStack,
*  remember to use CleanupDeletePushL() function!
*  DO NOT USE CleanupStack::PushL()!!
*
*  @lib IHL.lib
*  @since 3.0
*/
class MIHLFileImage : public MIHLImage
    {
	public:

        /**
        * Virtual destructor.
		*/
		virtual ~MIHLFileImage() {}

	public:

		/**
		* Flags to control file image functionality.
		* These can be combined using an OR operation.
		* @since 3.0
		*/
		enum TOptions
			{
			EOptionNoDRMConsume	= 0x01,
			};

	public:

        /**
        * Return image type's unique identifier.
        * @since 3.0
		* @return Image type Uid.
		*/
		virtual const TUid& ImageType() const = 0;

        /**
        * Return image subtype's unique identifier.
        * @since 3.0
		* @return Image subtype Uid.
		*/
		virtual const TUid& ImageSubType() const = 0;

        /**
        * Return image index of this instance from source file.
		* Note that animation is counted as an single image.
        * @since 3.0
		* @return Index of this instance.
		*/
		virtual TInt ImageIndex() const = 0;

        /**
        * Return count of images in source file.
		* Note that animation is counted as an single image.
        * @since 3.0
		* @return Count of images in source file.
		*/
		virtual TInt ImageCount() const = 0;


        /**
        * Return image size.
        * @since 3.0
		* @return Image size in pixels.
		*/
		virtual TSize Size() const = 0;

        /**
        * Return preferred display mode for image.
		* Note that this display mode may differ from images original
		* display mode. Image processing may
		* need more colors than image has originally.
        * @since 3.0
		* @return Preferred image display mode.
		*/
		virtual TDisplayMode DisplayMode() const = 0;

        /**
        * Return display mode of mask bitmap.
		* If image doesn't support transparency, ENone is returned.
        * @since 3.0
		* @return ENone if image is not transparent.
		*         Mask display mode if image has transparency.
		*/
		virtual TDisplayMode MaskDisplayMode() const = 0;

        /**
        * Return image background color.
        * @since 3.0
		* @return Image background color.
		*/
		virtual TRgb BackgroundColor() const = 0;

        /**
        * Return array of fixed load sizes which can
		* be used in bitmap loading operation.
		* Array contain only sizes that differs from image original size.
		* If image can be loaded only to original size or it's fully
		* scaleable, array is empty.
		* Sizes starts from smallest and ends at largest.
        * @since 3.0
		* @return Array of supported image load sizes.
		*/
		virtual const RArray<TSize>& CustomLoadSizeArray() const = 0;

        /**
		* Check if image can be loaded directly into any given load size.
        * @since 3.0
		* @return Array of supported image load sizes.
		*/
		virtual TBool IsFullyScaleable() const = 0;

        /**
        * Check if image is animated.
        * @since 3.0
		* @return ETrue if image is animated, EFalse if not.
		*/
		virtual TBool IsAnimation() const = 0;

        /**
        * Return animation frame count.
		* If image is not animated, it has no animation frames either.
        * @since 3.0
		* @return Animation frame count.
		*/
		virtual TInt AnimationFrameCount() const = 0;

        /**
        * Return animation frame delay.
		* If image is not animated, it has no animation frames either.
        * @since 3.0
		* @param aAnimationFrameIndex Animation frame index.
		* @return Animation frame delay.
		*/
		virtual TTimeIntervalMicroSeconds32 AnimationFrameDelay( TInt aAnimationFrameIndex ) const = 0;

        /**
        * Load image into bitmap.
		* If using scale support, given bitmap must be created with wanted size
		* from CustomLoadSizeArray(). Displaymode can be get from Displaymode() method.
		* Mask bitmap is always reseted. It's recreated if image is transparent by using
		* size of given bitmap and displaymode from MaskDisplayMode() method.
		* If image is animated, first animation frame is loaded.
        * @since 3.0
        * @param aStatus Load request status reference.
		* @param aDestination Destination bitmap reference.
		* @param aFrameIndex for loading single frame from image
		* @return Return system wide error codes:
		*         KerrArgument if given bitmap is not created or it's size is incorrect.
		*         KErrBusy image has load request pending already.
		*/
		virtual TInt Load( TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex = 0 ) = 0;

        /**
        * Load animation frame into bitmap.
		* If using scale support, given bitmap must be created with wanted size
		* from CustomLoadSizeArray(). Displaymode can be get from Displaymode() method.
		* Because some animations are build on top of previous frames, passing already loaded previous
		* frame as a destination bitmap will increase load speed of next frames.
		* Method will panic if given animation frame index is out of bounds.
        * @since 3.0
        * @param aStatus Load request status reference.
		* @param aDestination Destination bitmap reference.
		* @param aAnimationFrameIndex Animation frame index.
		* @return Return system wide error codes:
		*         KErrArgument if given bitmap is not created or it's size is incorrect.
		*         KErrBusy image has load request pending already.
		*/
		virtual TInt LoadAnimation( TRequestStatus& aStatus, MIHLBitmap& aDestination,
                                    TInt aAnimationFrameIndex ) = 0;

        /**
        * Check if image has load request pending.
        * @since 3.0
		* @return ETrue if request is pending, EFalse if not.
        */
		virtual TBool IsBusy() const = 0;

        /**
        * Cancel pending load request.
		* If not active, method does nothing.
        * @since 3.0
        */
		virtual void CancelLoad() = 0;

        /**
        * Set filter.
		* For future use, not used currently!
        * @since 3.0
        */
		virtual void SetFilter( MIHLFilter* aFilter ) = 0;
	};

#endif   // MIHLFILEIMAGE_H

// End of File
