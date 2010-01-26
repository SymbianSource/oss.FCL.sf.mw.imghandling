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
* Description:  Implementation of bitmap scaler class
*              : using Font and Bitmap Server bitmaps.
*
*/

#ifndef CIHLSCALER_H
#define CIHLSCALER_H

// INCLUDES
#include <MIHLScaler.h>
#include <e32base.h>
#include <gdi.h>

// Define this to show throughput measurements
//#define RD_MEASURE_THROUGHPUT

// FORWARD DECLARATIONS
class CFbsBitmap;

// CLASS DECLARATION
/**
*  CIHLScaler
*
*  Implementation of Scaling processor
*  using Font and Bitmap Server bitmaps.
*  @lib IHL.lib
*  @since 3.0
*/
NONSHARABLE_CLASS( CIHLScaler ) : public CActive, public MIHLScaler
    {
    public: // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        static CIHLScaler* NewL( const TUint32 aOptions = 0 );

        /*
        * Virtual destructor.
        */
        virtual ~CIHLScaler();

    public: // From base class MIHLScaler

        /*
        * From MIHLScaler, see base class header.
        */
		TInt Scale( TRequestStatus& aStatus,
					const CFbsBitmap& aSrcBitmap,
					const TRect& aSrcRect,
					CFbsBitmap& aDstBitmap,
					const TRect& aDstRect );

        /*
        * From MIHLScaler, see base class header.
        */
		TBool IsBusy() const;

        /*
        * From MIHLScaler, see base class header.
        */
		void CancelProcess();

        /*
        * From MIHLScaler, see base class header.
        */
		void SetFilter( MIHFilter* aFilter );

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
        * Check if rectangle is inside size.
        * @since 3.0
		* @return ETrue if rectangle is fully inside given area, EFalse if not.
		*/
		TBool IsValidRect( const TSize& aSize, const TRect& aRect );

	private: // Private methods

        /*
        * Initialize process parameters and stepper values.
        * @return ETrue if only doing scaling (and not rotating)
        */
		TBool InitStepperValues();
		
		/*
        * Select and initialize fastest possible code path
        */
        
		void InitCodePath(const TSize& aSrcSize, const TDisplayMode &aSrcMode, const TSize& aDstSize, const TDisplayMode &aDstMode, TBool aOnlyScaling);

        /*
        * Process step using nearest neighbour method.
        */
		TBool ProcessNearestNeighbour();
		TBool ProcessNearestNeighbour64K();           // For 16-bit non-compressed bitmaps
		TBool ProcessNearestNeighbour64KScaleOnly();  // For 16-bit non-compressed bitmaps (only scaling)
		TBool ProcessNearestNeighbour16MX();          // For 32-bit non-compressed bitmaps
		TBool ProcessNearestNeighbour16MXScaleOnly(); // For 32-bit non-compressed bitmaps (only scaling)
				
        /*
        * Process step using bilinear interpolation method.
        */
		TBool ProcessBilinear();

        /*
        * Complete dummy request. This causes RunL() method to be called.
        * Needed by state machine.
        */
		void SelfComplete();

        /*
        * Complete client process request.
        * This causes client RunL() method to be called.
        */
		void RequestComplete( TInt aReason );

	private: // Private constructors

        CIHLScaler( const TUint32 aOptions );

    private: // Data

		// Ref: Process status
		TRequestStatus* iScalerStatus;
        TBool iNeedProcess;

		// Process options
		const TUint32 iOptions;

        // Ref: Source bitmap
		const CFbsBitmap* iSrcBitmap;
		TRect iSrcRect;

		// Ref: Destination bitmaps
		CFbsBitmap* iDstBitmap;
		TRect iDstRect;

		// Process position and stepping
		TPoint iSrcIncrementInner;
		TPoint iSrcIncrementOuter;
		TPoint iSrcPos;

		TInt iDstIncrementInner;
		TInt iDstIncrementOuter;
		TPoint iDstPos;

		TInt iProcessInner;
		TInt iProcessOuter;
		TSize iProcessSize;
		
		// For fast code path
		TInt iSrcPitchInPixels;
		TInt iDstPitchInPixels;
		TUint32 iScanlinesLeft;
		TUint32 iScanlinesPerRound;
		TInt iDstStartOffset;
		TInt iDstResidualPixels;
		
		// Only for measuring scaler throughput
#ifdef RD_MEASURE_THROUGHPUT
		TUint32 iStartTime;
#endif
		
		/*
        * Forward process step event to selected processing code path
        * Called by RunL() method.
        */
		TBool (CIHLScaler::*ProcessingFunc)();
    };

#endif // CIHLSCALER_H

// End of File
