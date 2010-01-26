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
* Description:  Implementation of scaling processor
*              : using MIHLBitmap type bitmaps.
*
*/


#ifndef CIHLBITMAPPROCESSOR_H
#define CIHLBITMAPPROCESSOR_H

// INCLUDES
#include <MIHLBitmapProcessor.h>
#include <e32base.h>

// FORWARD DECLARATIONS
class CIHLScaler;
class CIHLBitmap;

// CLASS DECLARATION
/**
*  CIHLBitmapProcessor
*
*  Implementation of scaling processor
*  using MIHLBitmap type bitmaps.
*  @lib IHL.lib
*  @since 3.0
*/
NONSHARABLE_CLASS( CIHLBitmapProcessor ) : public CActive,
										   public MIHLBitmapProcessor
    {
    public:  // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        static CIHLBitmapProcessor* NewL( const TUint32 aOptions = 0 );

        /*
        * Virtual destructor.
        */
        virtual ~CIHLBitmapProcessor();

    public: // From base class MIHLProcessor

        /*
        * From MIHLProcessor, see base class header.
        */
		TIHLInterfaceType Type() const;

    public: // From base class MIHLBitmapProcessor

        /*
        * From MIHLBitmapProcessor, see base class header.
        */
		TInt Process( TRequestStatus& aStatus,
					  const MIHLBitmap& aSrcBitmap,
					  const TRect& aSrcRect,
					  MIHLBitmap& aDstBitmap,
					  const TRect& aDstRect );

        /*
        * From MIHLBitmapProcessor, see base class header.
        */
		TBool IsBusy() const;

        /*
        * From MIHLBitmapProcessor, see base class header.
        */
		void CancelProcess();

        /*
        * From MIHLBitmapProcessor, see base class header.
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


	private: // Private methods

        /*
        * Process bitmap part of source image
        */
		void ProcessBitmapL();

        /*
        * Process mask part of source image
        */
		void ProcessMaskL();

        /*
        * Set this active object to completed state
        * -> one process step (RunL) will be executed.
        */
		void SelfComplete();

        /*
        * Set client request to completed state
        * -> process complete.
        * @param aReason Request complete reason
        */
		void RequestComplete( TInt aReason );

	private: // Private constructors

        CIHLBitmapProcessor();
        void ConstructL( const TUint32 aOptions );

	private: // Private data types

		enum TProcessorState
			{
			EInactive,
			EProcessBitmap,
			EProcessMask,
			EFinish,
			};

    private: // Data

		// Ref: Process status & state
		TRequestStatus* iProcessorStatus;
		TProcessorState iProcessorState;

		// Ref: Source bitmap & rect
		const MIHLBitmap* iSrcBitmap;
		TRect iSrcRect;

		// Ref: Destination bitmap & rect
		CIHLBitmap* iDstBitmap;
		TRect iDstRect;

		// Own: Scaler
		CIHLScaler* iScaler;

    };

#endif // CIHLBITMAPPROCESSOR_H

// End of File
