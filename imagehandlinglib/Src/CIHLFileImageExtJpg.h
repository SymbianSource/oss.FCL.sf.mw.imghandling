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
* Description:  Implementation of ImageFile class.
*
*/


#ifndef CIHLFILEIMAGEEXTJPG_H
#define CIHLFILEIMAGEEXTJPG_H

// INCLUDES
#include <e32base.h>
#include <imageconversion.h> // TFrameInfo
#include <gdi.h> // TDisplayMode
#include "MIHLFileImageExtJpg.h"

// FORWARD DECLARATIONS
class MIHLBitmap;
class CIHLBitmap;
class CExtJpegDecoder;
class CVisualFrame;

// CLASS DECLARATION
/**
*  CIHLFileImageExtJpg
*
*  Implementation of file based ext jpg image class.
*  @lib IHL.lib
*  @since 3.0
*/

NONSHARABLE_CLASS( CIHLFileImageExtJpg ) : public CActive, public MIHLFileImageExtJpg
    {
    public:  // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        static CIHLFileImageExtJpg* NewL( RFs& aFs, const TDesC8& aDataBuf,
                                    TInt aImageIndex, const TUint32 aOptions );
		static CIHLFileImageExtJpg* NewL( RFs& aFs, const TDesC& aFilename,
                                    TInt aImageIndex, const TUint32 aOptions );
        /*
        * Virtual destructor.
        */
        virtual ~CIHLFileImageExtJpg();

	public: // From base class MIHLImage

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
		TInt LoadAnimation( TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aAnimationFrameIndex );

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

	public: // From MIHLFileImageExtJpg
	
		/*
        * From MIHLFileImageExtJpg, see base class header.
        */
		TInt Load( TRect aSrcRect, TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex = 0 );
		
		/*
        * From MIHLFileImageExtJpg, see base class header.
        */
        void SetCroppingL( TRect aCropRect );
		
		/*
        * From MIHLFileImageExtJpg, see base class header.
        */
        void SetStreamingL( TSize& aMacroBlockSize );

        /*
        * From MIHLFileImageExtJpg, see base class header.
        */
        void SetRotationL( TInt aDegree );

        /*
        * From MIHLFileImageExtJpg, see base class header.
        */
        void SetFlippingL();

        /*
        * From MIHLFileImageExtJpg, see base class header.
        */
        void SetMirroringL();
        
        /*
        * From MIHLFileImageExtJpg, see base class header.
        */
        void SetDctDecodingL();
        
        /*
        * From MIHLFileImageExtJpg, see base class header.
        */
        void ConvertL( 
                        TRequestStatus* aRequestStatus, 
                        const CVisualFrame* aDestinationFrame, 
                        TInt& aNoOfDecodedMBlocks,
                        TInt aFrameNumber = 0 );

        /*
        * From MIHLFileImageExtJpg, see base class header.
        */
        void ContinueConvertL( 
                        TRequestStatus* aRequestStatus, 
                        const CVisualFrame* aDestinationFrame, 
                        TInt& aNoOfDecodedMBlocks,
                        TInt aFrameNumber = 0 );

        /*
        * From MIHLFileImageExtJpg, see base class header.
        */
        TInt SupportedFormatsL();

        /*
        * From MIHLFileImageExtJpg, see base class header.
        */
        TInt CapabilitiesL();

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
        * Return reference to used CExtJpegDecoder instance.
		* For internal use.
        * @since 3.0
		* @return Reference to used CExtJpegDecoder instance.
		*/
		const CExtJpegDecoder& Decoder() const;

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
        * Loading complete. Build possible animation and notify client.
        * Called by ActiveSheduler when state is ECompleteLoad.
        */
		void CompleteLoadL();

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

	private: // Private constructors

        CIHLFileImageExtJpg( TInt aImageIndex );
        
        void ConstructL( RFs& aFs, const TDesC& aFilename, const TUint32 aOptions );
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
		CExtJpegDecoder* iDecoder;

		// Image information
		TUid iImageType;
		TUid iImageSubType;
		TInt iImageIndex;
		TInt iImageCount;
	
		TBool iFullyScaleable;
		TFrameInfo iFrameInfo;
		TBool iAnimation;
		TInt iAnimationFrameCount;

		// Ref: Destination bitmap
		CIHLBitmap* iDestination;
		TInt iFrameIndex;
		
		RArray<TSize> iLoadSizeArray;
    };

#endif   // CIHLFILEIMAGEEXTJPG_H

// End of File
