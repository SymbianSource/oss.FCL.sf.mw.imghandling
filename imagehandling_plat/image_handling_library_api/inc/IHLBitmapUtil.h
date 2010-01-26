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
* Description:  Static utility class for synchronous CFbsBitmap operations.
*
*/


#ifndef IHLBITMAPUTIL_H
#define IHLBITMAPUTIL_H

// INCLUDES
#include <e32std.h>

// FORWARD DECLARATION
class CFbsBitmap;

// CLASS DECLARATION
/**
*  IHLBitmapUtil
*
*  Static utility class for synchronous CFbsBitmap operations.
*  @lib IHL.lib
*  @since 3.0
*/
class IHLBitmapUtil
    {
    public:

        /**
        * Create new bitmap instance using
		* same server handle as in given bitmap.
		* Note that modifying bitmap content will also change
		* all other bitmap content that has same server handle.
        * @since 3.0
		* @param aBitmap Duplicated bitmap reference.
		* @return Duplicated CFbsBitmap instance.
		*/
		IMPORT_C static CFbsBitmap* DuplicateBitmapL( const CFbsBitmap& aBitmap );

        /**
        * Create new bitmap instance using
		* same server handle as in given bitmap.
		* Note that modifying bitmap content will also change
		* all other bitmap content that has same server handle.
		* New instance is pushed into CleanupStack.
        * @since 3.0
		* @param aBitmap Duplicated bitmap reference.
		* @return Duplicated CFbsBitmap instance.
		*/
		IMPORT_C static CFbsBitmap* DuplicateBitmapLC( const CFbsBitmap& aBitmap );

        /**
        * Copy bitmap into new same sized bitmap.
		* New bitmap has unique server handle and it can be
		* modified without risk of changes in given source bitmap.
        * @since 3.0
		* @param aBitmap Copied bitmap reference.
		* @return Copied CFbsBitmap instance.
		*/
		IMPORT_C static CFbsBitmap* CopyBitmapL( const CFbsBitmap& aBitmap );

        /**
        * Copy bitmap into new same sized bitmap.
		* New bitmap has unique server handle and it can be
		* modified without risk of changes in given source bitmap.
		* New instance is pushed into CleanupStack.
        * @since 3.0
		* @param aBitmap Copied bitmap reference.
		* @return Copied CFbsBitmap instance.
		*/
		IMPORT_C static CFbsBitmap* CopyBitmapLC( const CFbsBitmap& aBitmap );

        /**
        * Copy user defined area from source bitmap into new bitmap.
		* New bitmap has unique server handle and it can be
		* modified without risk of changes in given source bitmap.
        * @since 3.0
		* @param aBitmap Source bitmap reference.
		* @param aRect Copied rectangle from source bitmap.
		* @return Copied CFbsBitmap instance, size of given rect.
		*/		
		IMPORT_C static CFbsBitmap* CopyBitmapL( const CFbsBitmap& aBitmap, const TRect& aRect );

        /**
        * Copy user defined area from source bitmap into new bitmap.
		* New bitmap has unique server handle and it can be
		* modified without risk of changes in given source bitmap.
		* New instance is pushed into CleanupStack.
        * @since 3.0
		* @param aBitmap Source bitmap reference.
		* @param aRect Copied rectangle from source bitmap.
		* @return Copied CFbsBitmap instance, size of given rect.
		*/		
		IMPORT_C static CFbsBitmap* CopyBitmapLC( const CFbsBitmap& aBitmap, const TRect& aRect );

        /**
		* Synchronous bitmap scaler. Use only with small bitmaps!
		* Prefer asynchronous MIHLScaler instead.
		*
        * Scale source bitmap into destination bitmap.
        * @since 3.0
        * @param aSrcBitmap Source bitmap reference.
        * @param aDstBitmap Destination bitmap reference.
		*                   If destination bitmap has null handle,
		*                   processor will create new bitmap using size of
		*                   destination rectangle.
        * @param aOptions Reserved for future use.
		* @return Return system wide error codes:
		*         KerrArgument - source bitmap is empty (handle is null).
		*                      - destination bitmap displaymode differs
		*                        from source bitmap displaymode.
		*/
        IMPORT_C static TInt ScaleBitmap( const CFbsBitmap& aSrcBitmap, CFbsBitmap& aDstBitmap,
										  TUint32 aOptions = 0 );

        /**
		* Synchronous bitmap scaler. Use only with small bitmaps!
		* Prefer asynchronous MIHLScaler instead.
		*
        * Scale rectangle from source bitmap to a rectangle
		* in destination bitmap. Scaled area can be also
		* rotated and/or mirrored during process.
		* See full usage description from MIHLScaler header.
        * @since 3.0
        * @param aSrcBitmap Source bitmap reference.
        * @param aSrcRect Rectangle that is processed from source bitmap.
        * @param aDstBitmap Destination bitmap reference.
		*                   If destination bitmap has null handle,
		*                   processor will create new bitmap using size of
		*                   destination rectangle.
        * @param aDstRect Rectangle that is filled in destination bitmap.
        * @param aOptions Reserved for future use.
		* @return Return system wide error codes:
		*         KerrArgument - rectangles are outside of bitmap area.
		*                      - source bitmap is empty (handle is null).
		*                      - destination bitmap displaymode differs
		*                        from source bitmap displaymode.
		*/
        IMPORT_C static TInt ScaleBitmap( const CFbsBitmap& aSrcBitmap, const TRect& aSrcRect,
										  CFbsBitmap& aDstBitmap, const TRect& aDstRect,
										  TUint32 aOptions = 0 );
	};

#endif // IHLBITMAPUTIL_H

// End of File
