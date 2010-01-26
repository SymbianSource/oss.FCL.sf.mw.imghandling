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
* Description:  Pure virtual interface to bitmap and mask container.
*
*/


#ifndef MIHLBITMAP_H
#define MIHLBITMAP_H

// INCLUDES
#include <gdi.h>

// FORWARD DECLARATION
class MIHLBitmap;
class CBitmapContext;
class CFbsBitmap;


// FACTORY DECLARATION
class IHLBitmap
    {
    public:

        /**
        * Create new MIHLBitmap instance.
        * @since 3.0
		* @return New default MIHLBitmap instance.
		*/
        IMPORT_C static MIHLBitmap* CreateL();

	};

// CLASS DECLARATION
/**
*  MIHLBitmap
*
*  Pure virtual interface to bitmap and mask container.
*
*  If interface needs to be pushed into CleanupStack,
*  remember to use CleanupStackDeletePushL() function!
*  DO NOT USE CleanupStack::PushL()!!
*
*  @lib IHL.lib
*  @since 3.0
*/
class MIHLBitmap
	{
	public:

        /**
        * Virtual destructor.
		*/
		virtual ~MIHLBitmap() {}

    public:

        /**
        * Create new bitmap.
        * @since 3.0
		* @param aSize Bitmap size.
		* @param aDisplayMode Bitmap displaymode.
		* @return KErrArgument if size is negative or displaymode is not supported.
		*         Otherwise system wide error codes.
        */
		virtual TInt Create( const TSize& aSize, TDisplayMode aDisplayMode ) = 0;

        /**
        * Create new bitmap and mask.
        * @since 3.0
		* @param aSize Bitmap and mask size.
		* @param aBitmapDisplayMode Bitmap displaymode.
		* @param aMaskDisplayMode Mask displaymode (EGray2 or EGray256)
		* @return KErrArgument if size is negative or displaymode is not supported.
		*         Otherwise system wide error codes.
        */
		virtual TInt Create( const TSize& aSize, TDisplayMode aBitmapDisplayMode,
												 TDisplayMode aMaskDisplayMode ) = 0;

        /**
        * Copy or duplicate bitmap.
		* Copied bitmap has unique server handle and it can be
		* modified without risk of changes in given source bitmap.
		* Duplicated bitmap has same server handle than source bitmap
		* and modifying duplicated bitmap will also modify source bitmap.
        * @since 3.0
		* @param aBitmap Source bitmap that is copied or duplicated.
		* @param aDuplicate ETrue to duplicate, EFalse to copy.
		* @return System wide error codes.
        */
		virtual TInt Copy( const CFbsBitmap& aBitmap, TBool aDuplicate ) = 0;

        /**
        * Copy or duplicate bitmap and mask.
		* Copied bitmaps has unique server handles and they can be
		* modified without risk of changes in given source bitmaps.
		* Duplicated bitmaps has same server handles than source bitmaps
		* and modifying duplicated bitmaps will also modify source bitmaps.
        * @since 3.0
		* @param aBitmap Source bitmap that is copied or duplicated.
		* @param aBitmap Source aMask that is copied or duplicated.
		* @param aDuplicate ETrue to duplicate, EFalse to copy.
		* @return System wide error codes.
        */
		virtual TInt Copy( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask, TBool aDuplicate ) = 0;

        /**
        * Copy or duplicate MIHLBitmap.
		* Copied bitmaps has unique server handles and they can be
		* modified without risk of changes in given source bitmaps.
		* Duplicated bitmaps has same server handles than source bitmaps
		* and modifying duplicated bitmaps will also modify source bitmaps.
        * @since 3.0
		* @param aBitmap Source MIHLBitmap that is copied or duplicated.
		* @param aDuplicate ETrue to duplicate, EFalse to copy.
		* @return System wide error codes.
        */
		virtual TInt Copy( const MIHLBitmap& aBitmap, TBool aDuplicate ) = 0;

        /**
        * Release bitmap and mask server handles.
		* Create() and Copy() methods will call this
		* automatically if any old bitmaps exist.
        * @since 3.0
        */
		virtual void Reset() = 0;

        /**
        * Check if bitmap is created or copied.
		* This checks that bitmap handle is valid and exist.
        * @since 3.0
		* @return ETrue if bitmap exist, EFalse if bitmap handle is zero.
        */
		virtual TBool IsCreated() const = 0;

        /**
        * Return bitmap reference.
		* Use MIHLBitmap::IsCreated() method
		* to check if bitmap actually exist.
        * @since 3.0
		* @return Reference to bitmap.
        */
		virtual const CFbsBitmap& Bitmap() const = 0;

        /**
        * Check if mask bitmap handle is valid and exist.
        * @since 3.0
		* @return ETrue if mask exist, EFalse if mask handle is zero.
        */
		virtual TBool HasMask() const = 0;

        /**
        * Return mask reference.
		* Use MIHLBitmap::HasMask() method
		* to check if mask actually exist.
        * @since 3.0
		* @return Reference to mask.
        */
		virtual const CFbsBitmap& Mask() const = 0;

       /**
        * Draw bitmap to bitmap context.
		* If bitmaps are not created, method does nothing.
		* @param aContext Bitmap context where bitmap is drawn.
		* @param aPoint Draw position.
        * @since 3.0
        */
		virtual void Draw( CBitmapContext& aContext, const TPoint& aPoint ) const = 0;

        /**
        * Draw bitmap to bitmap context.
		* If bitmaps are not created, method does nothing.
		* @param aContext Bitmap context where bitmap is drawn.
		* @param aPoint Draw position.
		* @param aSourceRect Source rectangle that is drawn.
        * @since 3.0
        */
		virtual void Draw( CBitmapContext& aContext, const TPoint& aPoint,
                           const TRect& aSourceRect ) const = 0;

	};

#endif   // MIHLBITMAP_H

// End of File
