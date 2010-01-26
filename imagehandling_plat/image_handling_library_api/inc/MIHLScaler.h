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
* Description:  Pure virtual interface to bitmap scaler
*              : using Font and Bitmap Server bitmaps.
*
*/


#ifndef MIHLSCALER_H
#define MIHLSCALER_H

// INCLUDES
#include <e32std.h>

// FORWARD DECLARATION
class MIHLScaler;
class MIHFilter;
class CFbsBitmap;

// FACTORY DECLARATION
class IHLScaler
    {
    public:
		
        /**
        * Create new MIHLScaler instance.
        * @since 3.0
		* @param aOptions Special options for bitmap scaling. Use MIHLScaler::TOptions values.
		* @return New scaler instance.
		*/
        IMPORT_C static MIHLScaler* CreateL( const TUint32 aOptions = 0 );
	};

// CLASS DECLARATION
/**
*  MIHLScaler
*
*  Pure virtual interface to bitmap scaler
*  using Font and Bitmap Server bitmaps.
*
*  If interface needs to be pushed into CleanupStack,
*  remember to use CleanupStackDeletePushL() function!
*  DO NOT USE CleanupStack::PushL()!!
*
*  @lib IHL.lib
*  @since 3.0
*/
class MIHLScaler
    {
    public:

        /**
        * Virtual destructor.
		*/
        virtual ~MIHLScaler() {}

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
        * Scale rectangle from source bitmap to a rectangle
		* in destination bitmap. Scaled area can be also
		* rotated and/or mirrored during process.
		*
		* Rotating is done by defining SOURCE rectangle
		* topleft and bottomright corners in following way:
		*
		* No rotate:  tl-----    90 degree:   -----tl
		*             |      |               |      |
		*             |      |               |      |
		*              -----br               br-----
		*
		* 180 degree: br-----    270 degree:  -----br
		*             |      |               |      |
		*             |      |               |      |
		*              -----tl               tl-----
		*
  		* Mirroring is done by defining DESTINATION rectangle
		* topleft and bottomright corners in following way:
		*
		* No mirroring:         tl-----
		*                       |      |
		*                       |      |
		*                        -----br
		*
		* X-axis mirroring:      -----tl
		*                       |      |
		*                       |      |
		*                       br-----
		*
		* Y-axis mirroring:      -----br
		*                       |      |
		*                       |      |
		*                       tl-----
        *
		* X and Y-axis          br-----
		* mirroring:            |      |
		*                       |      |
		*                        -----tl
		*
        * @since 3.0
        * @param aStatus Process request status reference.
        * @param aSrcBitmap Source bitmap reference.
        * @param aSrcRect Rectangle that is processed from source bitmap.
        * @param aDstBitmap Destination bitmap reference.
		*                   If destination bitmap has null handle,
		*                   processor will create new bitmap using size of
		*                   destination rectangle.
        * @param aDstRect Rectangle that is filled in destination bitmap.
		* @return Return system wide error codes:
		*         KerrArgument - rectangles are outside of bitmap area.
		*                      - source bitmap is empty (handle is null).
		*                      - destination bitmap displaymode differs
		*                        from source bitmap displaymode.
		*         KErrBusy     - processor has request pending already.
        */
		virtual TInt Scale( TRequestStatus& aStatus,
							const CFbsBitmap& aSrcBitmap,
							const TRect& aSrcRect,
							CFbsBitmap& aDstBitmap,
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

#endif // MIHLSCALER_H

// End of File
