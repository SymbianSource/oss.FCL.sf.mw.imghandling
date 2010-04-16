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
* Description:  Implementation of Scaling processor
*              : using Font and Bitmap Server bitmaps.
*
*/

// INCLUDE FILES
#include "CIHLScaler.h"
#include "IHLDebugPrint.h" // Debug print
#include <MIHLImageViewer.h>        // MIHLImageViewer
#include <fbs.h>
#include <hal.h>

// Private namespace for constants and functions
namespace
	{
	// Processed pixels per one activesheduler step
	const TInt KProcessPixelsPerStep = 16384;
	
	// These make the fast path scalings step out about every 5-10ms
	// no reason to cut more aggressively since it just slows down everything
	const TInt KBytesPerStepFastPath = 640*480;
	const TInt KBytesPerStepFastPathScaleOnly = 640*480*2;
	
	const TInt KStepPrecision = 16; // 16 bit = 65k subpixels
	const TInt KMaxProcessSize = KMaxTInt / 2;
	}

//functions and typedefs used inside ProcessBilinear scaling
namespace
	{
	// using our own RGB instead of TRGB as we need TInt32 for each value
	typedef struct
		{
		TInt32 r, g, b, a;
		} TColorRGB;
	
	// structure for handling data of 4 pixels
	typedef struct
		{
		TColorRGB colXY00, colXY01, colXY10, colXY11;
		TColorRGB col0, col1, col;
		} TPixelData;

	// functions to handle conversion between TColorRGB and colr value stored in TInt32
	inline void IntToRgb(TInt32 aIntRGB_val, TColorRGB &col)
		{
		col.r = (aIntRGB_val >> 16) & 0x00ff;
		col.g = (aIntRGB_val >> 8) & 0x00ff;
		col.b =  aIntRGB_val & 0x00ff;
		col.a = (aIntRGB_val >> 24) & 0x00ff;
		}
	inline TUint32 RgbToInt(TColorRGB &col)
		{
		return 0x00000000 | ( ( col.r & 0xff) << 16 ) | ( ( col.g & 0xff) << 8  ) 
		                  | (   col.b & 0xff)         | ( ( col.a & 0xff) << 24 );
		}

	// function which interpolates color using gathered data
	inline void ProcessColorData( TPixelData &dat, TPoint aSrcPos )
		{
		TInt32 pixel_width = 1 << KStepPrecision;
        
        //first pass (Y axis)
		dat.col0.r = (dat.colXY01.r - dat.colXY00.r)*(aSrcPos.iY % pixel_width) / pixel_width + dat.colXY00.r;
		dat.col0.g = (dat.colXY01.g - dat.colXY00.g)*(aSrcPos.iY % pixel_width) / pixel_width + dat.colXY00.g;
		dat.col0.b = (dat.colXY01.b - dat.colXY00.b)*(aSrcPos.iY % pixel_width) / pixel_width + dat.colXY00.b;
		dat.col0.a = (dat.colXY01.a - dat.colXY00.a)*(aSrcPos.iY % pixel_width) / pixel_width + dat.colXY00.a;

		dat.col1.r = (dat.colXY11.r - dat.colXY10.r)*(aSrcPos.iY % pixel_width) / pixel_width + dat.colXY10.r;
		dat.col1.g = (dat.colXY11.g - dat.colXY10.g)*(aSrcPos.iY % pixel_width) / pixel_width + dat.colXY10.g;
		dat.col1.b = (dat.colXY11.b - dat.colXY10.b)*(aSrcPos.iY % pixel_width) / pixel_width + dat.colXY10.b;
		dat.col1.a = (dat.colXY11.a - dat.colXY10.a)*(aSrcPos.iY % pixel_width) / pixel_width + dat.colXY10.a;

		//second pass (X axis)
		dat.col.r = (dat.col1.r - dat.col0.r)*(aSrcPos.iX % pixel_width) / pixel_width + dat.col0.r;
		dat.col.g = (dat.col1.g - dat.col0.g)*(aSrcPos.iX % pixel_width) / pixel_width + dat.col0.g;
		dat.col.b = (dat.col1.b - dat.col0.b)*(aSrcPos.iX % pixel_width) / pixel_width + dat.col0.b;
		dat.col.a = (dat.col1.a - dat.col0.a)*(aSrcPos.iX % pixel_width) / pixel_width + dat.col0.a;
		}
	}
	
// ======================== STATIC FACTORY FUNCTION ============================
// -----------------------------------------------------------------------------
// IHLScaler::CreateL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLScaler* IHLScaler::CreateL( const TUint32 aOptions )
	{
	return CIHLScaler::NewL( aOptions );
	}

// ============================ MEMBER FUNCTIONS ===============================
// -----------------------------------------------------------------------------
//
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
CIHLScaler::CIHLScaler( const TUint32 aOptions )
:CActive( CActive::EPriorityStandard ), iOptions( aOptions )
    {
	CActiveScheduler::Add( this );
    }

// -----------------------------------------------------------------------------
//
// Two-phased constructor.
// -----------------------------------------------------------------------------
CIHLScaler* CIHLScaler::NewL( const TUint32 aOptions )
    {
    CIHLScaler* self = new( ELeave ) CIHLScaler( aOptions );
    return self;
    }

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
CIHLScaler::~CIHLScaler()
    {
	Cancel();
    }

// -----------------------------------------------------------------------------
// CIHLScaler::Scale
// -----------------------------------------------------------------------------
TInt CIHLScaler::Scale( TRequestStatus& aStatus,
					const CFbsBitmap& aSrcBitmap, const TRect& aSrcRect,
					CFbsBitmap& aDstBitmap, const TRect& aDstRect )
	{
	if( IsBusy() )
		{
		return KErrNotReady;
		}

    TSize srcRectSize( aSrcRect.Size() );
    TSize dstRectSize( aDstRect.Size() );
	if( Abs( srcRectSize.iWidth ) > KMaxProcessSize ||
	    Abs( srcRectSize.iHeight ) > KMaxProcessSize ||
	    Abs( dstRectSize.iWidth ) > KMaxProcessSize ||
	    Abs( dstRectSize.iHeight ) > KMaxProcessSize )
	    {
	    return KErrTooBig;
	    }

	if( !aSrcBitmap.Handle() ||
		!IsValidRect( aSrcBitmap.SizeInPixels(), aSrcRect ) )
		{
		return KErrArgument;
		}

	if( aDstBitmap.Handle() )
		{
		if( !IsValidRect( aDstBitmap.SizeInPixels(), aDstRect ) )
			{
			return KErrArgument;
			}
		}
	else
		{		
		// Create destination bitmap
		TSize dstSize( Abs( dstRectSize.iWidth ), Abs( dstRectSize.iHeight ) );
		TInt err = aDstBitmap.Create( dstSize, aSrcBitmap.DisplayMode() );
		if( err )
			{
			return err;
			}
		}

#ifdef RD_MEASURE_THROUGHPUT
	iStartTime = User::FastCounter(); // For measuring scaler throughput
#endif
    
    TSize srcSize( aSrcBitmap.SizeInPixels() );
    TSize dstSize( aDstBitmap.SizeInPixels() );
    
    if( srcSize.iWidth == 0 || srcSize.iHeight == 0 ||
        dstSize.iWidth == 0 || dstSize.iHeight == 0 ||
        aSrcRect.iTl.iX == aSrcRect.iBr.iX ||
        aSrcRect.iTl.iY == aSrcRect.iBr.iY ||
        aDstRect.iTl.iX == aDstRect.iBr.iX ||
        aDstRect.iTl.iY == aDstRect.iBr.iY )
        {
        // Bitmap or rect width or height is zero so there is nothing to do.
        // Complete process without error.
        iNeedProcess = EFalse;
        }
    else
        {
        iNeedProcess = ETrue;

    	// Set parameters to member data references
    	iSrcBitmap = &aSrcBitmap;
    	iSrcRect = aSrcRect;
    	iDstBitmap = &aDstBitmap;
    	iDstRect = aDstRect;
    	
    	TDisplayMode srcMode(iSrcBitmap->DisplayMode());
    	TDisplayMode dstMode(iDstBitmap->DisplayMode());

		IHL_DEBUG3( KIHLDebug1, "CIHLScaler: src bitmap size %dx%d", srcSize.iWidth, srcSize.iHeight);
		IHL_DEBUG3( KIHLDebug2, "CIHLScaler: dst bitmap size %dx%d", dstSize.iWidth, dstSize.iHeight);
		IHL_DEBUG5( KIHLDebug3, "CIHLScaler: src bitmap rect %d.%d -> %d.%d", iSrcRect.iTl.iX, iSrcRect.iTl.iY, iSrcRect.iBr.iX, iSrcRect.iBr.iY);
		IHL_DEBUG5( KIHLDebug4, "CIHLScaler: dst bitmap rect %d.%d -> %d.%d", iDstRect.iTl.iX, iDstRect.iTl.iY, iDstRect.iBr.iX, iDstRect.iBr.iY);
		IHL_DEBUG2( KIHLDebug5, "CIHLScaler: src bitmap mode %d", srcMode);
		IHL_DEBUG2( KIHLDebug6, "CIHLScaler: dst bitmap mode %d", dstMode);
		IHL_DEBUG2( KIHLDebug7, "CIHLScaler: src compressed %d", iSrcBitmap->IsCompressedInRAM());
		IHL_DEBUG2( KIHLDebug8, "CIHLScaler: dst compressed %d", iSrcBitmap->IsCompressedInRAM());
		
		// Init stepper values
    	TBool onlyScaling = InitStepperValues();
    	
    	// Select correct code path
    	InitCodePath(srcSize, srcMode, dstSize, dstMode, onlyScaling);
        }

    IHL_DEBUG1( KIHLDebug, "IHL - CIHLScaler - Start  bitmap scaling" );
	
	// Start processing
	iScalerStatus = &aStatus;
	*iScalerStatus = KRequestPending;

	SelfComplete();
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLScaler::IsBusy
// -----------------------------------------------------------------------------
TBool CIHLScaler::IsBusy() const
	{
	return IsActive();
	}

// -----------------------------------------------------------------------------
// CIHLScaler::CancelProcess
// -----------------------------------------------------------------------------
void CIHLScaler::CancelProcess()
	{
	Cancel();
	}

// -----------------------------------------------------------------------------
// CIHLScaler::SetFilter
// -----------------------------------------------------------------------------
void CIHLScaler::SetFilter( MIHFilter* /*aFilter*/ )
	{
	// Not in use
	}

// -----------------------------------------------------------------------------
// CIHLScaler::DoCancel
// -----------------------------------------------------------------------------
void CIHLScaler::DoCancel()
	{
	RequestComplete( KErrCancel );
	}

// -----------------------------------------------------------------------------
// CIHLScaler::RunL
// -----------------------------------------------------------------------------
void CIHLScaler::RunL()
	{
	User::LeaveIfError( iStatus.Int() );

	// Process bitmap (this if-else may look weird but it removes one extra triggering of the AS
	if( iNeedProcess )
		{
		// Call the selected code path directly via a function pointer
		iNeedProcess = (this->*ProcessingFunc)();
		}
	
	if(!iNeedProcess)
		{
#ifdef RD_MEASURE_THROUGHPUT
		// Calculate throughput
		TUint32 end = User::FastCounter();
		
		TInt tickPeriod;
		HAL::Get(HALData::EFastCounterFrequency, tickPeriod);
		TReal time = TReal(end-iStartTime) / TReal(tickPeriod);
		TReal bytes = iProcessSize.iWidth*iProcessSize.iHeight*2;
		
		IHL_DEBUG3("IHL - CIHLScaler - Scaling complete, %.3f ms, %.3f MB/s", time*1000.0, (bytes/time)/1024.0/1024.0);
#else
        IHL_DEBUG1( KIHLDebug, "IHL - CIHLScaler - Scaling complete!" );
#endif
		// Process complete
		RequestComplete( KErrNone );
		}
	else
		{
		// Another round
		SelfComplete();
		}
	}

// -----------------------------------------------------------------------------
// CIHLScaler::RunError
// -----------------------------------------------------------------------------
TInt CIHLScaler::RunError( TInt aError )
	{
    IHL_DEBUG2( KIHLDebug, "IHL - CIHLScaler - Scaling error: %d", aError );

	RequestComplete( aError );
	return KErrNone;
	}

// -----------------------------------------------------------------------------
// CIHLScaler::IsValidRect
// -----------------------------------------------------------------------------
TBool CIHLScaler::IsValidRect( const TSize& aSize, const TRect& aRect )
	{
	if( aRect.iTl.iX >= 0 &&
		aRect.iTl.iX <= aSize.iWidth &&
		aRect.iTl.iY >= 0 &&
		aRect.iTl.iY <= aSize.iHeight &&
		aRect.iBr.iX >= 0 &&
		aRect.iBr.iX <= aSize.iWidth &&
		aRect.iBr.iY >= 0 &&
		aRect.iBr.iY <= aSize.iHeight )
		{
		return ETrue;
		}
	return EFalse;
	}

// -----------------------------------------------------------------------------
// CIHLScaler::InitStepperValues
// -----------------------------------------------------------------------------
TBool CIHLScaler::InitStepperValues()
	{
	TBool onlyScaling = EFalse;
	
	TSize srcRectSize( Abs( iSrcRect.Width() ), Abs( iSrcRect.Height() ) );
	TSize dstRectSize( Abs( iDstRect.Width() ), Abs( iDstRect.Height() ) );

	TReal srcW = srcRectSize.iWidth << KStepPrecision;
	TReal srcH = srcRectSize.iHeight << KStepPrecision;
	
    // -----------------------------------------
	// Set source start point and increment
    // -----------------------------------------
	if( iSrcRect.iTl.iX > iSrcRect.iBr.iX &&
		iSrcRect.iTl.iY < iSrcRect.iBr.iY )
		{
        TReal incrementInner( srcH / TReal(dstRectSize.iWidth) );
        TReal incrementOuter( srcW / TReal(dstRectSize.iHeight) );

		iSrcIncrementInner.iX = 0;
		iSrcIncrementInner.iY = incrementInner;
		iSrcIncrementOuter.iX = -incrementOuter;
		iSrcIncrementOuter.iY = 0;

		iSrcPos.SetXY( (iSrcRect.iTl.iX << KStepPrecision) - (incrementOuter / 2.0),
		               (iSrcRect.iTl.iY << KStepPrecision) + (incrementInner / 2.0) );
		}
	else if	( iSrcRect.iTl.iX > iSrcRect.iBr.iX &&
			  iSrcRect.iTl.iY > iSrcRect.iBr.iY )
		{
        TReal incrementInner( srcW / TReal(dstRectSize.iWidth) );
        TReal incrementOuter( srcH / TReal(dstRectSize.iHeight) );

		iSrcIncrementInner.iX = -incrementInner;
		iSrcIncrementInner.iY = 0;
		iSrcIncrementOuter.iX = 0;
		iSrcIncrementOuter.iY = -incrementOuter;

		iSrcPos.SetXY( (iSrcRect.iTl.iX << KStepPrecision) - (incrementInner / 2.0),
		               (iSrcRect.iTl.iY << KStepPrecision) - (incrementOuter / 2.0) );
		}
	else if	( iSrcRect.iTl.iX < iSrcRect.iBr.iX &&
			  iSrcRect.iTl.iY > iSrcRect.iBr.iY )
		{
        TReal incrementInner( srcH / TReal(dstRectSize.iWidth) );
        TReal incrementOuter( srcW / TReal(dstRectSize.iHeight) );

		iSrcIncrementInner.iX = 0;
		iSrcIncrementInner.iY = -incrementInner;
		iSrcIncrementOuter.iX = incrementOuter;
		iSrcIncrementOuter.iY = 0;

		iSrcPos.SetXY( (iSrcRect.iTl.iX << KStepPrecision) + (incrementOuter / 2.0),
		               (iSrcRect.iTl.iY << KStepPrecision) - (incrementInner / 2.0) );
		}
	else
		{
        TReal incrementInner( srcW / TReal(dstRectSize.iWidth) );
        TReal incrementOuter( srcH / TReal(dstRectSize.iHeight) );

		IHL_DEBUG3(KIHLDebug, "incrementInner: %f, incrementOuter: %f", incrementInner, incrementOuter);
		
		iSrcIncrementInner.iX = incrementInner;
		iSrcIncrementInner.iY = 0;
		iSrcIncrementOuter.iX = 0;
		iSrcIncrementOuter.iY = incrementOuter;

		iSrcPos.SetXY( (iSrcRect.iTl.iX << KStepPrecision) + (incrementInner / 2.0),
		               (iSrcRect.iTl.iY << KStepPrecision) + (incrementOuter / 2.0) );
		               
		onlyScaling = ETrue;
		}

	// -----------------------------------------
	// Set destination start point and increment
    // -----------------------------------------
	
	iDstPos.iX = iDstRect.iTl.iX;
	iDstIncrementInner = 1;
	iDstPos.iY = iDstRect.iTl.iY;
	iDstIncrementOuter = 1;
	
	if( iDstRect.iTl.iX > iDstRect.iBr.iX )
		{
		// From right to left
		iDstPos.iX--;
		iDstIncrementInner = -1;
		onlyScaling = EFalse;
		}
	
	if( iDstRect.iTl.iY > iDstRect.iBr.iY )
		{
		// From bottom to up
		iDstPos.iY--;
		iDstIncrementOuter = -1;
		onlyScaling = EFalse;
		}

    // -----------------------------------------
	// Reset process counters
    // -----------------------------------------
	iProcessInner = 0;
	iProcessOuter = 0;
	iProcessSize = dstRectSize;
	
	// These are for fast code path
	
	// Init how many scanlines to process
	iScanlinesLeft = iProcessSize.iHeight; 
		
	return onlyScaling;
	}


// Optimize the fast code paths properly for ARM
#ifdef __ARMCC__
#pragma push
#pragma O3
#pragma Otime
#pragma arm
#endif

// -----------------------------------------------------------------------------
// CIHLScaler::ProcessNearestNeighbour64K
// Description: Scales & rotates 64K color bitmaps
// -----------------------------------------------------------------------------
TBool CIHLScaler::ProcessNearestNeighbour64K()
	{
	// Lock bitmap heaps
	iSrcBitmap->LockHeap();
	iDstBitmap->LockHeap();
	
	// Pointers to start of bitmap data
	TUint16 *srcBitmapPtr = (TUint16*)iSrcBitmap->DataAddress();
	TUint16 *dstBitmapPtr = (TUint16*)iDstBitmap->DataAddress();
	
	// Get pointer to correct destination pixel (offset is updated after every round)
	register TUint16* dst = dstBitmapPtr + iDstStartOffset;
	
	TUint32 width = iProcessSize.iWidth;
	
	// Check how many scanlines we can process this round
	register TUint32 scanlinesLeft = iScanlinesLeft;
	if(scanlinesLeft>iScanlinesPerRound)
		{
		scanlinesLeft = iScanlinesPerRound;
		}

	// How many scanlines left for the next round
	iScanlinesLeft-=scanlinesLeft;
	
	// Faster access to variables (it's slow to use member variables)
	register TInt srcPosX(iSrcPos.iX);
	register TInt srcPosY(iSrcPos.iY);
	register TInt srcPitch = iSrcPitchInPixels;
	
	TInt incOuterSrcX = (-iSrcIncrementInner.iX * width) + iSrcIncrementOuter.iX;
	TInt incOuterSrcY = (-iSrcIncrementInner.iY * width) + iSrcIncrementOuter.iY;
	
	while(scanlinesLeft!=0)
        {
		for(register TUint32 x = width; x!=0; x--)
			{
			register TUint16* src = srcBitmapPtr + ((srcPosY >> KStepPrecision) * srcPitch) + (srcPosX>>KStepPrecision);
			*dst = *src;

            // Add inner counters
			srcPosX+=iSrcIncrementInner.iX;
			srcPosY+=iSrcIncrementInner.iY;
			
			dst+=iDstIncrementInner;
			}
			
		// Reset inner counters and add outer counters
		srcPosX += incOuterSrcX;
		srcPosY += incOuterSrcY;

		// Move destination pointer to next pixel
		dst += iDstResidualPixels;
		
		// One scanline done, n to go
		scanlinesLeft--;
		}
	// Unlock bitmap heaps
	iDstBitmap->UnlockHeap();
	iSrcBitmap->UnlockHeap();
	
	if(iScanlinesLeft)
		{
		// Not all scanlines were processed yet
		
		// Save the necessary offsets for next round
		iSrcPos.iX = srcPosX;
		iSrcPos.iY = srcPosY;
		iDstStartOffset = dst - dstBitmapPtr;
		
		return ETrue;
		}
	
	// Processing done
	return EFalse;
	}

// -----------------------------------------------------------------------------
// CIHLScaler::ProcessNearestNeighbour64KScaleOnly
// -----------------------------------------------------------------------------
TBool CIHLScaler::ProcessNearestNeighbour64KScaleOnly()
	{
	// Lock bitmap heaps
	iSrcBitmap->LockHeap();
	iDstBitmap->LockHeap();
	
	// Get bitmap data start addresses
	TUint16 *srcBitmapPtr = (TUint16*)iSrcBitmap->DataAddress();
	TUint16 *dstBitmapPtr = (TUint16*)iDstBitmap->DataAddress();

	// Target height and width
	TInt width = iProcessSize.iWidth;
	
	// fixed point source coordinates
	TInt startX = iSrcPos.iX;
	TInt srcY = iSrcPos.iY;
	
	// How much to increase src position (in fixed point)
	TInt srcYIncrement = iSrcIncrementOuter.iY;
	register TInt srcXIncrement = iSrcIncrementInner.iX;
	
	// Set pointers to correct location (src = start of scanline, dst = start pixel)
	register TUint16* dst = dstBitmapPtr + iDstStartOffset;
	
	// Calculate how many scanlines we can process this round
	register TInt scanlinesLeft = iScanlinesLeft;
	if(scanlinesLeft>iScanlinesPerRound)
	{
		scanlinesLeft = iScanlinesPerRound;
	}
	
	iScanlinesLeft-=scanlinesLeft;
		
	while(scanlinesLeft--)
		{
		// Outer loop
		
		// Reset src X and scanline pointer
		register TInt srcX = startX;
		register TUint16* src = srcBitmapPtr + (srcY >> KStepPrecision) * iSrcPitchInPixels;
		
		// Init pixel counter
		register TUint32 pixels = width;
		
		// Unaligned pixels to the left of 8B-aligned section
		while((TUint32(dst)&0x7) && pixels)
		{
			*dst++ = (*(src + (srcX >> KStepPrecision)));

			srcX += srcXIncrement;
			
			pixels--;
		}
		
		// Aligned middle section		
		register TUint32 middle = pixels&0xFFFFFFFC;
		pixels &= 0x3;
		
		while(middle)
			{
			// Read four pixels
			register TUint16 p1 = (*(src + (srcX >> KStepPrecision)));
			srcX += srcXIncrement;
			register TUint16 p2 = (*(src + (srcX >> KStepPrecision)));
			srcX += srcXIncrement;
			register TUint16 p3 = (*(src + (srcX >> KStepPrecision)));
			srcX += srcXIncrement;
			register TUint16 p4 = (*(src + (srcX >> KStepPrecision)));
			srcX += srcXIncrement;
			
			// Write four pixels
			*(dst++) = p1;
			*(dst++) = p2;
			*(dst++) = p3;
			*(dst++) = p4;

			middle-=4;
			}
			
		// Unaligned residual pixels to the right of 8-aligned section
		while(pixels)
		{
			*dst++ = (*(src + (srcX >> KStepPrecision)));

			srcX += srcXIncrement;
			
			pixels--;
		}
		
		// Move to next correct src scanline
		srcY += srcYIncrement;
		
		// Advance dst to start of correct pixel in the next row
		dst += iDstResidualPixels;
		}
	
	iDstBitmap->UnlockHeap();
	iSrcBitmap->UnlockHeap();
	
	if(iScanlinesLeft)
		{
		// Not all scanlines were processed yet
		
		// Save the necessary offsets for next round
		iSrcPos.iY = srcY;
		iDstStartOffset = dst - dstBitmapPtr;

		return ETrue;
		}
	
	return EFalse;
	
	}

// -----------------------------------------------------------------------------
// CIHLScaler::ProcessNearestNeighbour16MX
// Description: Scales & rotates 16MU/MA color bitmaps
// -----------------------------------------------------------------------------
TBool CIHLScaler::ProcessNearestNeighbour16MX()
	{
	// Lock bitmap heaps
	iSrcBitmap->LockHeap();
	iDstBitmap->LockHeap();
	
	// Pointers to start of bitmap data
	TUint32 *srcBitmapPtr = (TUint32*)iSrcBitmap->DataAddress();
	TUint32 *dstBitmapPtr = (TUint32*)iDstBitmap->DataAddress();
	
	// Get pointer to correct destination pixel (offset is updated after every round)
	register TUint32* dst = dstBitmapPtr + iDstStartOffset;
	
	TUint32 width = iProcessSize.iWidth;
	
	// Check how many scanlines we can process this round
	register TUint32 scanlinesLeft = iScanlinesLeft;
	if(scanlinesLeft>iScanlinesPerRound)
		{
		scanlinesLeft = iScanlinesPerRound;
		}

	// How many scanlines left for the next round
	iScanlinesLeft-=scanlinesLeft;
	
	// Faster access to variables (it's slow to use member variables)
	register TInt srcPosX(iSrcPos.iX);
	register TInt srcPosY(iSrcPos.iY);
	register TInt srcPitch = iSrcPitchInPixels;
	
	TInt incOuterSrcX = (-iSrcIncrementInner.iX * width) + iSrcIncrementOuter.iX;
	TInt incOuterSrcY = (-iSrcIncrementInner.iY * width) + iSrcIncrementOuter.iY;
	
	while(scanlinesLeft!=0)
        {
		for(register TUint32 x = width; x!=0; x--)
			{
			register TUint32* src = srcBitmapPtr + ((srcPosY >> KStepPrecision) * srcPitch) + (srcPosX>>KStepPrecision);
			*dst = *src;

            // Add inner counters
			srcPosX+=iSrcIncrementInner.iX;
			srcPosY+=iSrcIncrementInner.iY;
			
			dst+=iDstIncrementInner;
			}
			
		// Reset inner counters and add outer counters
		srcPosX += incOuterSrcX;
		srcPosY += incOuterSrcY;

		// Move destination pointer to next pixel
		dst += iDstResidualPixels;
		
		// One scanline done, n to go
		scanlinesLeft--;
		}
	// Unlock bitmap heaps
	iDstBitmap->UnlockHeap();
	iSrcBitmap->UnlockHeap();
	
	if(iScanlinesLeft)
		{
		// Not all scanlines were processed yet
		
		// Save the necessary offsets for next round
		iSrcPos.iX = srcPosX;
		iSrcPos.iY = srcPosY;
		iDstStartOffset = dst - dstBitmapPtr;
		
		return ETrue;
		}
	
	// Processing done
	return EFalse;
	}
	
// -----------------------------------------------------------------------------
// CIHLScaler::ProcessNearestNeighbour64KScaleOnly
// -----------------------------------------------------------------------------
TBool CIHLScaler::ProcessNearestNeighbour16MXScaleOnly()
	{
	// Lock bitmap heaps
	iSrcBitmap->LockHeap();
	iDstBitmap->LockHeap();
	
	// Get bitmap data start addresses
	TUint32 *srcBitmapPtr = (TUint32*)iSrcBitmap->DataAddress();
	TUint32 *dstBitmapPtr = (TUint32*)iDstBitmap->DataAddress();

	// Target height and width
	TInt width = iProcessSize.iWidth;
	
	// fixed point source coordinates
	TInt startX = iSrcPos.iX;
	TInt srcY = iSrcPos.iY;
	
	// How much to increase src position (in fixed point)
	TInt srcYIncrement = iSrcIncrementOuter.iY;
	register TInt srcXIncrement = iSrcIncrementInner.iX;
	
	// Set pointers to correct location (src = start of scanline, dst = start pixel)
	register TUint32* dst = dstBitmapPtr + iDstStartOffset;
	
	// Calculate how many scanlines we can process this round
	register TInt scanlinesLeft = iScanlinesLeft;
	if(scanlinesLeft>iScanlinesPerRound)
	{
		scanlinesLeft = iScanlinesPerRound;
	}
	
	iScanlinesLeft-=scanlinesLeft;
		
	while(scanlinesLeft--)
		{
		// Outer loop
		
		// Reset src X and scanline pointer
		register TInt srcX = startX;
		register TUint32* src = srcBitmapPtr + (srcY >> KStepPrecision) * iSrcPitchInPixels;
		
		// Init pixel counter
		register TUint32 pixels = width;
		
		// Unaligned pixels to the left of 16B-aligned section
		while((TUint32(dst)&0xF) && pixels)
		{
			*dst++ = (*(src + (srcX >> KStepPrecision)));

			srcX += srcXIncrement;
			
			pixels--;
		}
		
		// Aligned middle section		
		register TUint32 middle = pixels&0xFFFFFFFC;
		pixels &= 0x3;
		
		while(middle)
			{
			// Read four pixels
			register TUint32 p1 = (*(src + (srcX >> KStepPrecision)));
			srcX += srcXIncrement;
			register TUint32 p2 = (*(src + (srcX >> KStepPrecision)));
			srcX += srcXIncrement;
			register TUint32 p3 = (*(src + (srcX >> KStepPrecision)));
			srcX += srcXIncrement;
			register TUint32 p4 = (*(src + (srcX >> KStepPrecision)));
			srcX += srcXIncrement;
			
			// Write four pixels
			*(dst+0) = p1;
			*(dst+1) = p2;
			*(dst+2) = p3;
			*(dst+3) = p4;

			middle-=4;
			dst+=4;		
			}
			
		// Unaligned residual pixels to the right of 8-aligned section
		while(pixels)
		{
			*dst++ = (*(src + (srcX >> KStepPrecision)));

			srcX += srcXIncrement;
			
			pixels--;
		}
		
		// Move to next correct src scanline
		srcY += srcYIncrement;
		
		// Advance dst to start of correct pixel in the next row
		dst += iDstResidualPixels;
		}
	
	iDstBitmap->UnlockHeap();
	iSrcBitmap->UnlockHeap();
	
	if(iScanlinesLeft)
		{
		// Not all scanlines were processed yet
		
		// Save the necessary offsets for next round
		iSrcPos.iY = srcY;
		iDstStartOffset = dst - dstBitmapPtr;

		return ETrue;
		}
	
	return EFalse;
	
	}
	
// Restore previous pragma state
#ifdef __ARMCC__
#pragma pop
#endif

// -----------------------------------------------------------------------------
// CIHLScaler::ProcessNearestNeighbour
// -----------------------------------------------------------------------------
TBool CIHLScaler::ProcessNearestNeighbour()
	{
	// Util needs non-const pointers even if it's only used for reading pixels
    TBitmapUtil srcBitmap( const_cast<CFbsBitmap*>( iSrcBitmap ) );
    TBitmapUtil dstBitmap( iDstBitmap );
    srcBitmap.Begin( TPoint( iSrcPos.iX >> KStepPrecision, iSrcPos.iY >> KStepPrecision ) );
    dstBitmap.Begin( iDstPos ); // heap already locked by srcBitmap
	TInt pixelCounter( KProcessPixelsPerStep );

    // Start outer and inner process loops
    while( iProcessOuter < iProcessSize.iHeight && pixelCounter )
        {
		while( iProcessInner < iProcessSize.iWidth && pixelCounter )
			{
			srcBitmap.SetPos( TPoint( iSrcPos.iX >> KStepPrecision, iSrcPos.iY >> KStepPrecision ) );
			dstBitmap.SetPos( iDstPos );
			dstBitmap.SetPixel( srcBitmap );

            // Add inner counters
			iSrcPos += iSrcIncrementInner;
			iDstPos.iX += iDstIncrementInner;
			++iProcessInner;
			--pixelCounter;
			}
        // Check if inner loop is finished (not just pixel counter)
		if( iProcessInner == iProcessSize.iWidth )
			{
			// Reset inner counters
			iSrcPos.iX -= iSrcIncrementInner.iX * iProcessInner;
			iSrcPos.iY -= iSrcIncrementInner.iY * iProcessInner;

			iDstPos.iX -= iDstIncrementInner * iProcessInner;
			iProcessInner = 0;

            // Add outer counters
			iSrcPos += iSrcIncrementOuter;
			iDstPos.iY += iDstIncrementOuter;
			++iProcessOuter;
			}
		}

    // Release heap lock
    dstBitmap.End();
    srcBitmap.End();

    // ETrue if more processing is still needed
	return ( iProcessOuter < iProcessSize.iHeight );
	}

// -----------------------------------------------------------------------------
// CIHLScaler::ProcessBilinear
// -----------------------------------------------------------------------------
TBool CIHLScaler::ProcessBilinear()
	{
	// Util needs non-const pointers
	//even if it's only used for reading pixels
	TBitmapUtil srcBitmap( const_cast<CFbsBitmap*>( iSrcBitmap ) );
	TBitmapUtil dstBitmap( iDstBitmap );

	srcBitmap.Begin( TPoint( iSrcPos.iX >> KStepPrecision, iSrcPos.iY >> KStepPrecision ) );
	dstBitmap.Begin( iDstPos ); // heap already locked by srcBitmap
	TInt pixelCounter( KProcessPixelsPerStep );

	TInt incSrcX = iSrcIncrementInner.iX + iSrcIncrementOuter.iX;
	TInt incSrcY = iSrcIncrementInner.iY + iSrcIncrementOuter.iY;

	TPixelData pixelData;
	TUint32 color;

	TInt32 pixel_width = 1 << KStepPrecision;

	//if there is no scaling then process through loop using NearestNeighbour alghoritm
	if ( incSrcX == pixel_width || -incSrcX == pixel_width ) 
		{
		while( iProcessOuter < iProcessSize.iHeight && pixelCounter )
			{
			while( iProcessInner < iProcessSize.iWidth && pixelCounter )
				{
				srcBitmap.SetPos( TPoint( iSrcPos.iX >> KStepPrecision, iSrcPos.iY >> KStepPrecision ) );
				dstBitmap.SetPos( iDstPos );
				dstBitmap.SetPixel( srcBitmap );

				// Add inner counters
				iSrcPos += iSrcIncrementInner;
				iDstPos.iX += iDstIncrementInner;
				++iProcessInner;
				--pixelCounter;
				}

			// Check if inner loop is finished (not just pixel counter)
			if( iProcessInner == iProcessSize.iWidth )
				{
				// Reset inner counters
				iSrcPos.iX -= iSrcIncrementInner.iX * iProcessInner;
				iSrcPos.iY -= iSrcIncrementInner.iY * iProcessInner;

				iDstPos.iX -= iDstIncrementInner * iProcessInner;
				iProcessInner = 0;

				// Add outer counters
				iSrcPos += iSrcIncrementOuter;
				iDstPos.iY += iDstIncrementOuter;
				++iProcessOuter;
				}
			}
		}
	// Process using bilinear method according to 
	// orientation between source and destination bitmap
	//
	// There are 4 possibilities of orientation and can be
	// determined by this if we're increasing or decreasing
	// position on source bitmap.
	else if ( ( incSrcY >= 0) && (incSrcX >= 0 ) )
		{
		while( iProcessOuter < iProcessSize.iHeight && pixelCounter )
			{
			while( iProcessInner < iProcessSize.iWidth && pixelCounter )
				{
				srcBitmap.SetPos( TPoint( iSrcPos.iX >> KStepPrecision, iSrcPos.iY >> KStepPrecision ) );
				dstBitmap.SetPos( iDstPos );

				// gather pixel data with step:
				// 00 > 10 
				//      V
				// 01 < 11
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY00);
				srcBitmap.IncXPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY10);
				srcBitmap.IncYPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY11);
				srcBitmap.DecXPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY01);
				//return to starting position
				srcBitmap.DecYPos();

				// interpolate color using gathered pixel-data
				ProcessColorData(pixelData, iSrcPos);

				// put color in destination package
				color = RgbToInt(pixelData.col);
				dstBitmap.SetPixel(color);

				// Add inner counters
				iSrcPos += iSrcIncrementInner;
				iDstPos.iX += iDstIncrementInner;
				++iProcessInner;
				--pixelCounter;
				}

			// Check if inner loop is finished (not just pixel counter)
			if( iProcessInner == iProcessSize.iWidth )
				{
				// Reset inner counters
				iSrcPos.iX -= iSrcIncrementInner.iX * iProcessInner;
				iSrcPos.iY -= iSrcIncrementInner.iY * iProcessInner;

				iDstPos.iX -= iDstIncrementInner * iProcessInner;
				iProcessInner = 0;

				// Add outer counters
				iSrcPos += iSrcIncrementOuter;
				iDstPos.iY += iDstIncrementOuter;
				++iProcessOuter;
				}
			}
		}
	else if ( ( incSrcY >= 0) && (incSrcX < 0 ) )
		{
		while( iProcessOuter < iProcessSize.iHeight && pixelCounter )
			{
			while( iProcessInner < iProcessSize.iWidth && pixelCounter )
				{
				srcBitmap.SetPos( TPoint( iSrcPos.iX >> KStepPrecision, iSrcPos.iY >> KStepPrecision ) );
				dstBitmap.SetPos( iDstPos );

				// gather pixel data with step:
				// 00   10 
				// /\   V
				// 01 < 11
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY10);
				srcBitmap.IncYPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY11);
				srcBitmap.DecXPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY01);
				srcBitmap.DecYPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY00);
				//return to starting position
				srcBitmap.IncXPos();

				// interpolate color using gathered pixel-data
				ProcessColorData(pixelData, iSrcPos);

				// put color in destination package
				color = RgbToInt(pixelData.col);
				dstBitmap.SetPixel(color);

				// Add inner counters
				iSrcPos += iSrcIncrementInner;
				iDstPos.iX += iDstIncrementInner;
				++iProcessInner;
				--pixelCounter;
				}

			// Check if inner loop is finished (not just pixel counter)
			if( iProcessInner == iProcessSize.iWidth )
				{
				// Reset inner counters
				iSrcPos.iX -= iSrcIncrementInner.iX * iProcessInner;
				iSrcPos.iY -= iSrcIncrementInner.iY * iProcessInner;

				iDstPos.iX -= iDstIncrementInner * iProcessInner;
				iProcessInner = 0;

				// Add outer counters
				iSrcPos += iSrcIncrementOuter;
				iDstPos.iY += iDstIncrementOuter;
				++iProcessOuter;
				}

			}
		}
	else if ( ( incSrcY < 0) && (incSrcX >= 0 ) )
		{
		while( iProcessOuter < iProcessSize.iHeight && pixelCounter )
			{
			while( iProcessInner < iProcessSize.iWidth && pixelCounter )
				{
				srcBitmap.SetPos( TPoint( iSrcPos.iX >> KStepPrecision, iSrcPos.iY >> KStepPrecision ) );
				dstBitmap.SetPos( iDstPos );

				// gather pixel data with step:
				// 00 > 10 
				// /\   V
				// 01   11
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY01);
				srcBitmap.DecYPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY00);
				srcBitmap.IncXPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY10);
				srcBitmap.IncYPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY11);
				//return to starting position
				srcBitmap.DecXPos();

				// interpolate color using gathered pixel-data
				ProcessColorData(pixelData, iSrcPos);

				// put color in destination package
				color = RgbToInt(pixelData.col);
				dstBitmap.SetPixel(color);

				// Add inner counters
				iSrcPos += iSrcIncrementInner;
				iDstPos.iX += iDstIncrementInner;
				++iProcessInner;
				--pixelCounter;
				}

			// Check if inner loop is finished (not just pixel counter)
			if( iProcessInner == iProcessSize.iWidth )
				{
				// Reset inner counters
				iSrcPos.iX -= iSrcIncrementInner.iX * iProcessInner;
				iSrcPos.iY -= iSrcIncrementInner.iY * iProcessInner;

				iDstPos.iX -= iDstIncrementInner * iProcessInner;
				iProcessInner = 0;

				// Add outer counters
				iSrcPos += iSrcIncrementOuter;
				iDstPos.iY += iDstIncrementOuter;
				++iProcessOuter;
				}
			}
		}
	else
		{
		while( iProcessOuter < iProcessSize.iHeight && pixelCounter )
			{
			while( iProcessInner < iProcessSize.iWidth && pixelCounter )
				{
				srcBitmap.SetPos( TPoint( iSrcPos.iX >> KStepPrecision, iSrcPos.iY >> KStepPrecision ) );
				dstBitmap.SetPos( iDstPos );

				// gather pixel data with step:
				// 00 > 10 
				// /\   
				// 01 < 11
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY11);
				srcBitmap.DecXPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY01);
				srcBitmap.DecYPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY00);
				srcBitmap.IncXPos();
				IntToRgb(srcBitmap.GetPixel(),pixelData.colXY10);
				//return to starting position
				srcBitmap.IncYPos();

				// interpolate color using gathered pixel-data
				ProcessColorData(pixelData, iSrcPos);

				// put color in destination package
				color = RgbToInt(pixelData.col);
				dstBitmap.SetPixel(color);

				// Add inner counters
				iSrcPos += iSrcIncrementInner;
				iDstPos.iX += iDstIncrementInner;
				++iProcessInner;
				--pixelCounter;
				}

			// Check if inner loop is finished (not just pixel counter)
			if( iProcessInner == iProcessSize.iWidth )
				{
				// Reset inner counters
				iSrcPos.iX -= iSrcIncrementInner.iX * iProcessInner;
				iSrcPos.iY -= iSrcIncrementInner.iY * iProcessInner;

				iDstPos.iX -= iDstIncrementInner * iProcessInner;
				iProcessInner = 0;

				// Add outer counters
				iSrcPos += iSrcIncrementOuter;
				iDstPos.iY += iDstIncrementOuter;
				++iProcessOuter;
				}

			}
		}

	// Release heap lock
	dstBitmap.End();
	srcBitmap.End();

	// ETrue if more processing is still needed
	return ( iProcessOuter < iProcessSize.iHeight );
	}

// -----------------------------------------------------------------------------
// CIHLScaler::SelfComplete
// -----------------------------------------------------------------------------
void CIHLScaler::SelfComplete()
	{
	SetActive();
	iStatus = KRequestPending;
	TRequestStatus* status = &iStatus;
	User::RequestComplete( status, KErrNone );
	}

// -----------------------------------------------------------------------------
// CIHLScaler::RequestComplete
// -----------------------------------------------------------------------------
void CIHLScaler::RequestComplete( TInt aReason )
	{
	ASSERT( iScalerStatus );
	User::RequestComplete( iScalerStatus, aReason );
	}

// -----------------------------------------------------------------------------
// CIHLScaler::InitCodePath
// -----------------------------------------------------------------------------
void CIHLScaler::InitCodePath(const TSize& aSrcSize, const TDisplayMode &aSrcMode, const TSize& aDstSize, const TDisplayMode &aDstMode, TBool aOnlyScaling)
	{
	// Choose the correct processing code path	
	ProcessingFunc = &CIHLScaler::ProcessNearestNeighbour;
	    	
	if( iOptions & MIHLImageViewer::EOptionUseBilinearInterpolation )
		{
		// TODO: optimize bilinear scaling
		IHL_DEBUG("CIHLScaler::InitCodePath: slow bilinear");
		ProcessingFunc = &CIHLScaler::ProcessBilinear;
		iScanlinesPerRound = KProcessPixelsPerStep / iProcessSize.iWidth;
		}
	else if(aSrcMode == EColor64K && aSrcMode==aDstMode && !iSrcBitmap->IsCompressedInRAM() && !iDstBitmap->IsCompressedInRAM())
		{
		// 16 bit non-compressed bitmaps
		
		// Calculate how many pixels the scanline is actually wide in the memory
		iSrcPitchInPixels = CFbsBitmap::ScanLineLength( aSrcSize.iWidth, aSrcMode ) >> 1;
		iDstPitchInPixels = CFbsBitmap::ScanLineLength( aDstSize.iWidth, aDstMode ) >> 1;
		
		// How many pixels to move destination pointer at the end of each scanline to get to next pixel
		iDstResidualPixels = iDstIncrementOuter*iDstPitchInPixels - iDstIncrementInner*iProcessSize.iWidth;

		// Buffer offset to the first destination pixel
		iDstStartOffset = iDstPos.iY * iDstPitchInPixels + iDstPos.iX;

		if(!aOnlyScaling)
			{
			IHL_DEBUG("CIHLScaler::InitCodePath: fast 64K scale&rotate");
			ProcessingFunc = &CIHLScaler::ProcessNearestNeighbour64K;
			
			// Calculate how often the process should allow AS to run
			iScanlinesPerRound = KBytesPerStepFastPath / (iProcessSize.iWidth<<1);
			}
		else
			{
			IHL_DEBUG("CIHLScaler::InitCodePath: fast 64K scale only");
			ProcessingFunc = &CIHLScaler::ProcessNearestNeighbour64KScaleOnly;
			
			// Calculate how often the process should allow AS to run
			iScanlinesPerRound = KBytesPerStepFastPathScaleOnly / (iProcessSize.iWidth<<1);
			}
		}
		
	else if((aSrcMode == EColor16MU || aSrcMode == EColor16MA) && aSrcMode==aDstMode && !iSrcBitmap->IsCompressedInRAM() && !iDstBitmap->IsCompressedInRAM())
		{
		// 32 bit non-compressed bitmaps
		
		// Calculate how many pixels the scanline is actually wide in the memory
		iSrcPitchInPixels = CFbsBitmap::ScanLineLength( aSrcSize.iWidth, aSrcMode ) >> 2;
		iDstPitchInPixels = CFbsBitmap::ScanLineLength( aDstSize.iWidth, aDstMode ) >> 2;
		
		// How many pixels to move destination pointer at the end of each scanline to get to next pixel
		iDstResidualPixels = iDstIncrementOuter*iDstPitchInPixels - iDstIncrementInner*iProcessSize.iWidth;

		// Buffer offset to the first destination pixel
		iDstStartOffset = iDstPos.iY * iDstPitchInPixels + iDstPos.iX;

		if(!aOnlyScaling)
			{
			IHL_DEBUG("CIHLScaler::InitCodePath: fast 16MX scale&rotate");
			ProcessingFunc = &CIHLScaler::ProcessNearestNeighbour16MX;
			
			// Calculate how often the process should allow AS to run
			iScanlinesPerRound = KBytesPerStepFastPath / (iProcessSize.iWidth<<2);
			}
		else
			{
			IHL_DEBUG("CIHLScaler::InitCodePath: fast 16MX scale only");
			ProcessingFunc = &CIHLScaler::ProcessNearestNeighbour16MXScaleOnly;
			
			// Calculate how often the process should allow AS to run
			iScanlinesPerRound = KBytesPerStepFastPathScaleOnly / (iProcessSize.iWidth<<2);
			}
		}
	else
		{
		IHL_DEBUG("CIHLScaler::InitCodePath: slow nearest neighbor");
		iScanlinesPerRound = KProcessPixelsPerStep / iProcessSize.iWidth;
		}
		
	IHL_DEBUG2(KIHLDebug, "CIHLScaler::InitCodePath: scanlines per round = %d", iScanlinesPerRound);
	}

//  End of File
