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
* Description:  Implementation of ImageFile class.
*
*/


#ifndef CIHLFILEIMAGE_H
#define CIHLFILEIMAGE_H

// INCLUDES
#include <MIHLFileImage.h>
#include <e32base.h>
#include <imageconversion.h> // TFrameInfo
#include <gdi.h> // TDisplayMode
#include <TIHLInterfaceType.h>

// FORWARD DECLARATIONS
class MIHLBitmap;
class CIHLBitmap;
class CImageDecoder;
class TRequestStatus;

// CLASS DECLARATION
/**
*  CIHLImage
*
*  Implementation of file based image class.
*  @lib IHL.lib
*  @since 3.0
*/
NONSHARABLE_CLASS( CIHLFileImage ) : public CActive, public MIHLFileImage
    {
    public:  // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        static CIHLFileImage* NewL( RFile& aFile, TInt aImageIndex, const TUint32 aOptions );
        static CIHLFileImage* NewL( RFs& aFs, const TDesC8& aDataBuf,
                                    TInt aImageIndex, const TUint32 aOptions );

        /*
        * Virtual destructor.
        */
        virtual ~CIHLFileImage();

	public: // From base class MIHLImage

        /*
        * From MIHLImage, see base class header.
        */
		TIHLInterfaceType Type() const;

	public: // From base class MIHLFileImage

        /*
        * From MIHLFileImage, see base class header.
        */
		const TUid& ImageType() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		const TUid& ImageSubType() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TInt ImageIndex() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TInt ImageCount() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TSize Size() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TDisplayMode DisplayMode() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TDisplayMode MaskDisplayMode() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TRgb BackgroundColor() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		const RArray<TSize>& CustomLoadSizeArray() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TBool IsFullyScaleable() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TBool IsAnimation() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TInt AnimationFrameCount() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TTimeIntervalMicroSeconds32 AnimationFrameDelay( TInt aAnimationFrameIndex ) const;

        /*
        * From MIHLFileImage, see base class header.
        */
		TInt Load( TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex = 0 ); 
		
        /*
        * From MIHLFileImage, see base class header.
        */
		TInt LoadAnimation( TRequestStatus& aStatus, MIHLBitmap& aDestination,
                            TInt aAnimationFrameIndex );

        /*
        * From MIHLFileImage, see base class header.
        */
		TBool IsBusy() const;

        /*
        * From MIHLFileImage, see base class header.
        */
		void CancelLoad();

        /*
        * From MIHLFileImage, see base class header.
        */
		void SetFilter( MIHLFilter* aFilter );

	private: // From base class CActive

        /*
        * From CActive, see base class header.
        */
		void DoCancel();

        /*
        * From CActive, see base class header.
        */
		void RunL();

        /*
        * From CActive, see base class header.
        */
		TInt RunError( TInt aError );

	public: // Internal interface

        /**
        * Return reference to used CImageDecoder instance.
		* For internal use.
        * @since 3.0
		* @return Reference to used CImageDecoder instance.
		*/
		const CImageDecoder& Decoder() const;

	private: // Private methods

        /*
        * Request load and start ActiveScheduler.
        * Called from Load() and LoadAnimation() methods.
        */
		TInt LoadRequest( TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex );

        /*
        * Start loading from file. Checks if image is still or animation.
        * Called by ActiveSheduler when state is EStartLoad.
        */
		void StartLoadL();

        /*
        * Start loading normal frame. Called by StartLoadL() method.
        */
		void StartLoadNormalFrame( TInt aFrameIndex );

        /*
        * Start loading sub frame. Called by StartLoadL() method.
        */
		void StartLoadSubFrameL( TInt aFrameIndex, TBool aAnimation );

        /*
        * Loading complete. Build possible animation and notify client.
        * Called by ActiveSheduler when state is ECompleteLoad.
        */
		void CompleteLoadL();

        /*
        * Build sub frame. Called by CompleteLoadL() method.
        */
		void BuildSubFrameL();

        /*
        * Fill whole bitmap using given color.
        * Used by BuildAnimationFrameL().
        */
		void FillL( CFbsBitmap& aBitmap, const TRgb& aColor );

        /*
        * Fill bitmap rect using given color.
        * Used by BuildAnimationFrameL().
        */
		void FillRectL( CFbsBitmap& aBitmap, const TRect& aRect, const TRgb& aColor );

        /*
        * Error cleanup. Called if error occurs during load.
        */
		void ErrorCleanup();

        /*
        * Complete dummy request. This causes RunL() method to be called.
        * Needed by state machine.
        */
		void SelfComplete();

        /*
        * Complete client load request.
        * This causes client RunL() method to be called.
        */
		void RequestComplete( TInt aReason );

        /*
        * Calculate scaled size.
        * Original size is divided by scale factor and result is rounded up
        * to next integer.
        */
		TSize ScaledLoadSize( const TSize& aOriginalSize, TInt aScaleFactor );

        /*
        * Calculate scaled frame position.
        * Original position is divided by scale factor and result is rounded up
        * to next integer.
        */
        TPoint ScaledFramePosition( const TPoint& aOriginalPos, TInt aScaleFactor );

	private: // Private constructors

        CIHLFileImage( TInt aImageIndex );
        void ConstructL( RFile& aFile, const TUint32 aOptions );
        void ConstructL( RFs& aFs, const TDesC8& aDataBuf, const TUint32 aOptions );
        void ConstructCommonL();

	private: // Private data types

		enum TImageState
			{
			EInactive,
			EStartLoad,
			ECompleteLoad,
			};

	private: // Data

		// Ref: Image status & state
		TRequestStatus* iImageStatus;
		TImageState iImageState;

		// Own: Image decoder
		CImageDecoder* iDecoder;

		// Image information
		TUid iImageType;
		TUid iImageSubType;
		TInt iImageIndex;
		TInt iImageCount;
		RArray<TSize> iLoadSizeArray;
		TBool iFullyScaleable;
		TFrameInfo iFrameInfo;
		TBool iGif;
		TBool iAnimation;
		TInt iAnimationFrameCount;

		// Ref: Destination bitmap
		CIHLBitmap* iDestination;
		TInt iFrameIndex;

		// Own: Temporary animation bitmap
		CIHLBitmap* iSubFrameBitmap;
		TInt iSubFrameIndex;
		CIHLBitmap* iPrevAnimationFrame;
        TInt iSubFrameScaleFactor;
    };

#endif   // CIHLFILEIMAGE_H

// End of File
