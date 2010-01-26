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
* Description:  Pure virtual interface to scaling processor
*              : using MIHLBitmaps.
*
*/


#ifndef MIHLBITMAPPROCESSOR_H
#define MIHLBITMAPPROCESSOR_H

// INCLUDES
#include <MIHLProcessor.h>

// FORWARD DECLARATION
class MIHLBitmap;
class MIHFilter;

// CLASS DECLARATION
/**
*  MIHLBitmapProcessor
*
*  Pure virtual interface to scaling processor using MIHLBitmaps.
*
*  If interface needs to be pushed into CleanupStack,
*  remember to use CleanupStackDeletePushL() function!
*  DO NOT USE CleanupStack::PushL()!!
*
*  @lib IHL.lib
*  @since 3.0
*/
class MIHLBitmapProcessor : public MIHLProcessor
    {
	public:

        /**
        * Virtual destructor.
		*/
		virtual ~MIHLBitmapProcessor() {}

	public:

		/**
		* Flags to control scaler functionality.
		* These can be combined using an OR operation.
		* @since 3.0
		*/
		enum TOptions
			{
			EOptionUseBilinearInterpolation	= 0x01,
			};

    public:

        /**
        * Process rectangle from source bitmap to a rectangle
		* in destination bitmap. Scaled area can be also
		* rotated and/or mirrored during process.
		* See full description from MIHLScaler header.
        * @since 3.0
        * @param aStatus Process request status reference.
        * @param aSrcBitmap Source bitmap reference.
        * @param aSrcRect Rectangle that is processed from source bitmap.
        * @param aDstBitmap Destination bitmap reference.
        * @param aDstRect Rectangle that is filled in destination bitmap.
		* @return Return system wide error codes:
		*         KErrArgument - rectangles are outside of bitmap area.
		*                      - source bitmap is empty (handle is null).
		*                      - destination bitmap is empty (handle is null).
		*                      - destination bitmap displaymode differs
		*                        from source bitmap displaymode.
		*         KErrBusy     - processor has request pending already.
        */
		virtual TInt Process( TRequestStatus& aStatus,
							  const MIHLBitmap& aSrcBitmap,
							  const TRect& aSrcRect,
							  MIHLBitmap& aDstBitmap,
							  const TRect& aDstRect ) = 0;

        /**
        * Check if processor has request pending.
        * @since 3.0
		* @return ETrue if request is pending, EFalse if not.
        */
		virtual TBool IsBusy() const = 0;

        /**
        * Cancel pending request.
		* If not active, method does nothing.
        * @since 3.0
        */
		virtual void CancelProcess() = 0;

        /**
        * Set filter.
		* For future use, not used currently!
        * @since 3.0
        */
		virtual void SetFilter( MIHFilter* aFilter ) = 0;
    };

#endif // MIHLBITMAPPROCESSOR_H

// End of File
