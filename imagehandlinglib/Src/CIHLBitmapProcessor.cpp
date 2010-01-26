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



// INCLUDE FILES
#include "CIHLBitmapProcessor.h"

#include "CIHLScaler.h"
#include "CIHLBitmap.h"
#include "IHLImplementationIds.h"
#include "IHLDebugPrint.h" // Debug print

#include <IHLInterfaceIds.h>
#include <fbs.h> // CFbsBitmap
#include <bitdev.h> // CFbsBitmapDevice, CFbsBitGc

// Private namespace for constants and functions
namespace
	{
	// Panic function
    _LIT( KIHLPanicString, "IHLBitmapProcessor" );
    void Panic( TInt aPanicCode ) { User::Panic( KIHLPanicString, aPanicCode ); }
	}

// ============================ MEMBER FUNCTIONS ===============================
// -----------------------------------------------------------------------------
//
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
CIHLBitmapProcessor::CIHLBitmapProcessor()
:CActive( CActive::EPriorityStandard )
    {
	CActiveScheduler::Add( this );
    }

// -----------------------------------------------------------------------------
//
// Two-phased constructor.
// -----------------------------------------------------------------------------
CIHLBitmapProcessor* CIHLBitmapProcessor::NewL( const TUint32 aOptions )
    {
    CIHLBitmapProcessor* self = new( ELeave ) CIHLBitmapProcessor();
    CleanupStack::PushL( self );
    self->ConstructL( aOptions );
	CleanupStack::Pop(); // self
    return self;
    }

// -----------------------------------------------------------------------------
//
// Symbian constructor can leave.
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::ConstructL( const TUint32 aOptions )
	{
	iScaler = CIHLScaler::NewL( aOptions );
	}

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
CIHLBitmapProcessor::~CIHLBitmapProcessor()
    {
	Cancel();
	delete iScaler;
    }

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::Type
// -----------------------------------------------------------------------------
TIHLInterfaceType CIHLBitmapProcessor::Type() const
	{
	return TIHLInterfaceType( KIHLInterfaceIdBitmapProcessor, KIHLImplementationIdBitmapProcessor );
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::Process
// -----------------------------------------------------------------------------
TInt CIHLBitmapProcessor::Process( TRequestStatus& aStatus,
									const MIHLBitmap& aSrcBitmap,
									const TRect& aSrcRect,
									MIHLBitmap& aDstBitmap,
									const TRect& aDstRect )
	{
	if( IsBusy() )
		{
		return KErrNotReady;
		}

	// Check bitmaps (no checking of masks is needed)
	const CFbsBitmap& srcBitmap = aSrcBitmap.Bitmap();
	const CFbsBitmap& dstBitmap = aDstBitmap.Bitmap();
	if( !srcBitmap.Handle() ||
		!iScaler->IsValidRect( srcBitmap.SizeInPixels(), aSrcRect ) ||
		!dstBitmap.Handle() ||
		!iScaler->IsValidRect( dstBitmap.SizeInPixels(), aDstRect ) )
		{
		return KErrArgument;
		}

    IHL_DEBUG1( KIHLDebug, "IHL - CIHLBitmapProcessor - Start asynchronous bitmap processing" );

	// Set references to member data
	iSrcBitmap = &aSrcBitmap;
	iSrcRect = aSrcRect;
	iDstBitmap = static_cast<CIHLBitmap*>( &aDstBitmap ); //lint !e826
	iDstRect = aDstRect;

	// Start processing
	iProcessorStatus = &aStatus;
	*iProcessorStatus = KRequestPending;

	iProcessorState = EProcessBitmap;
	SelfComplete();
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::IsBusy
// -----------------------------------------------------------------------------
TBool CIHLBitmapProcessor::IsBusy() const
	{
	return ( iProcessorState != EInactive );
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::CancelProcess
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::CancelProcess()
	{
    IHL_DEBUG1( KIHLDebug, "IHL - CIHLBitmapProcessor - Process cancelled" );

	Cancel();
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::SetFilter
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::SetFilter( MIHFilter* /*aFilter*/ )
	{
	// Not in use
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::DoCancel
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::DoCancel()
	{
	iScaler->CancelProcess();
	iProcessorState = EInactive;
	RequestComplete( KErrCancel );
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::RunL
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::RunL()
	{
	User::LeaveIfError( iStatus.Int() );

	switch( iProcessorState )
		{
		case EProcessBitmap:
			{
			ProcessBitmapL();
			if( iDstBitmap->Mask().Handle() )
				{
				iProcessorState = EProcessMask;
				}
			else
				{
				// Destination has no mask -> finish process
				iProcessorState = EFinish;
				}
			break;
			}
		case EProcessMask:
			{
			ProcessMaskL();
			iProcessorState = EFinish;
			break;
			}
		case EFinish:
			{
            IHL_DEBUG1( KIHLDebug, "IHL - CIHLBitmapProcessor - Process complete!" );

			iProcessorState = EInactive;
			RequestComplete( KErrNone );
			break;
			}
		default:
			{
			Panic( KErrTotalLossOfPrecision );
			}
		}
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::RunError
// -----------------------------------------------------------------------------
TInt CIHLBitmapProcessor::RunError( TInt aError )
	{
    IHL_DEBUG2( KIHLDebug, "IHL - CIHLBitmapProcessor - Processing error: %d", aError );

	iProcessorState = EInactive;
	RequestComplete( aError );
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::ProcessBitmapL
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::ProcessBitmapL()
	{
	User::LeaveIfError( iScaler->Scale( iStatus,
										iSrcBitmap->Bitmap(),
										iSrcRect,
										iDstBitmap->BitmapModifyable(),
										iDstRect ) );
	SetActive();
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::ProcessMaskL
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::ProcessMaskL()
	{
	if( iSrcBitmap->Mask().Handle() )
		{
		User::LeaveIfError( iScaler->Scale( iStatus,
											iSrcBitmap->Mask(),
											iSrcRect,
											iDstBitmap->MaskModifyable(),
											iDstRect ) );
		SetActive();
		}
	else
		{
		// No source bitmap where mask can be copied
		// -> clear destination mask at given rectangle
		CFbsBitGc* bitGc;
		CFbsBitmapDevice* bitDev = CFbsBitmapDevice::NewL(
										&(iDstBitmap->BitmapModifyable()) );
		CleanupStack::PushL( bitDev );
		User::LeaveIfError( bitDev->CreateContext( bitGc ) );
		CleanupStack::PushL( bitGc );

		bitGc->SetPenColor( KRgbWhite ); // white is non-transparent
		bitGc->SetBrushColor( KRgbWhite );
		bitGc->SetBrushStyle( CGraphicsContext::ESolidBrush );
		bitGc->DrawRect( iDstRect );

		CleanupStack::PopAndDestroy( 2 ); // bitGc, bitDev
		}
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::SelfComplete
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::SelfComplete()
	{
	SetActive();
	iStatus = KRequestPending;
	TRequestStatus* status = &iStatus;
	User::RequestComplete( status, KErrNone );
	}

// -----------------------------------------------------------------------------
// CIHLBitmapProcessor::RequestComplete
// -----------------------------------------------------------------------------
void CIHLBitmapProcessor::RequestComplete( TInt aReason )
	{
	ASSERT( iProcessorStatus );
	User::RequestComplete( iProcessorStatus, aReason );
	}

//  End of File
