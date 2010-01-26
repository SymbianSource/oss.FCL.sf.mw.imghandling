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
* Description:  Default implementation of bitmap class.
*
*/


#ifndef CIHLBITMAP_H
#define CIHLBITMAP_H

// INCLUDES
#include <MIHLBitmap.h>
#include <e32base.h>

// CLASS DECLARATION
/**
*  CIHLBitmap
*
*  Default implementation of bitmap class.
*  @lib IHL.lib
*  @since 3.0
*/
NONSHARABLE_CLASS( CIHLBitmap ) : public CBase, public MIHLBitmap
	{
    public:  // Constructors and destructor

        /**
        * Two-phased constructors
        */
        static CIHLBitmap* NewL();
		
        /**
        * Virtual destructor.
        */
        virtual ~CIHLBitmap();

	public: // From base class MIHLBitmap

        /**
        * From MIHLBitmap, see base class header.
        */
		TInt Create( const TSize& aSize, TDisplayMode aDisplayMode );

        /**
        * From MIHLBitmap, see base class header.
        */
		TInt Create( const TSize& aSize, TDisplayMode aBitmapDisplayMode,
										 TDisplayMode aMaskDisplayMode );

        /**
        * From MIHLBitmap, see base class header.
        */
		TInt Copy( const CFbsBitmap& aBitmap, TBool aDuplicate );

        /**
        * From MIHLBitmap, see base class header.
        */
		TInt Copy( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask, TBool aDuplicate );

        /**
        * From MIHLBitmap, see base class header.
        */
		TInt Copy( const MIHLBitmap& aBitmap, TBool aDuplicate );

        /**
        * From MIHLBitmap, see base class header.
        */
		void Reset();

        /**
        * From MIHLBitmap, see base class header.
        */
		TBool IsCreated() const;

        /**
        * From MIHLBitmap, see base class header.
        */
		const CFbsBitmap& Bitmap() const;

        /**
        * From MIHLBitmap, see base class header.
        */
		TBool HasMask() const;

        /**
        * From MIHLBitmap, see base class header.
        */
		const CFbsBitmap& Mask() const;

        /**
        * From MIHLBitmap, see base class header.
        */
		void Draw( CBitmapContext& aContext, const TPoint& aPoint ) const;

        /**
        * From MIHLBitmap, see base class header.
        */
		void Draw( CBitmapContext& aContext, const TPoint& aPoint, const TRect& aSourceRect ) const;

	public: // Internal interface

        /**
        * Non-const bitmap reference.
        * @since 3.0
		* @return Non-const bitmap reference.
		*/
		CFbsBitmap& BitmapModifyable();

        /**
        * Non-const bitmap reference.
        * @since 3.0
		* @return Non-const bitmap reference.
		*/
		CFbsBitmap& MaskModifyable();

        /**
        * Set editor pointer. This information is used
		* internally to detect what class has made changes to bitmap data.
        * @since 3.0
		* @param aEditorPtr Editor pointer
		*/
		void SetEditorPtr( const TAny* aEditorPtr );

        /**
        * Set editor value. Meaning of this value may vary depending
		* on type of editor class.
        * @since 3.0
		* @param aEditorValue Editor custom value 
		*/
		void SetEditorValue( TInt aEditorValue );

        /**
        * Return Editor pointer. NULL if just created or reseted.
		* @return Editor pointer.
		*/
		const TAny* EditorPtr() const;

        /**
        * Return editor value.
        * @since 3.0
		* @return Editor custom value.
		*/
		TInt EditorValue() const;

	public: // Private methods

		TInt CopyBitmap( const CFbsBitmap& aSource, CFbsBitmap& aDestination );

		TInt CopyBitmapData( const CFbsBitmap& aSource, CFbsBitmap& aDestination,
							 const TSize& aSize, const TDisplayMode& aDisplayMode );

	private: // Private constructors

        CIHLBitmap();
		void ConstructL();

    private: // Data

		// Own: Bitmap pointers
		CFbsBitmap* iBitmap;
		CFbsBitmap* iMask;

		// Ref: Editor ptr
		const TAny* iEditorPtr;

		// Editor value
		TInt iEditorValue;
		
	};

#endif // CIHLBITMAP_H

// End of File
