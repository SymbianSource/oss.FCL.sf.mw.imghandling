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


// INCLUDE FILES
#include "CIHLBitmap.h"
#include <fbs.h>

// ======================== STATIC FACTORY FUNCTIONS ===========================
// -----------------------------------------------------------------------------
// IHLBitmap::CreateL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLBitmap* IHLBitmap::CreateL()
	{
	return CIHLBitmap::NewL();
	}

// ============================ MEMBER FUNCTIONS ===============================
// -----------------------------------------------------------------------------
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
CIHLBitmap::CIHLBitmap()
    {
    }

// -----------------------------------------------------------------------------
//
// Second phase constructors. Can leave.
// -----------------------------------------------------------------------------
void CIHLBitmap::ConstructL()
	{
	iBitmap = new (ELeave) CFbsBitmap;
	iMask = new (ELeave) CFbsBitmap;
	}

// -----------------------------------------------------------------------------
//
// Two-phased constructors.
// -----------------------------------------------------------------------------
CIHLBitmap* CIHLBitmap::NewL()
	{
    CIHLBitmap* self = new( ELeave ) CIHLBitmap;
	CleanupStack::PushL( self );
    self->ConstructL();
	CleanupStack::Pop(); // self
    return self;
	}

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
CIHLBitmap::~CIHLBitmap()
    {
	delete iBitmap;
	delete iMask;
    }

// -----------------------------------------------------------------------------
// CIHLBitmap::Create
// -----------------------------------------------------------------------------
TInt CIHLBitmap::Create( const TSize& aSize, TDisplayMode aDisplayMode )
	{
	Reset();
	return iBitmap->Create( aSize, aDisplayMode );
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Create
// -----------------------------------------------------------------------------
TInt CIHLBitmap::Create( const TSize& aSize, TDisplayMode aBitmapDisplayMode,
											 TDisplayMode aMaskDisplayMode )
	{
    Reset();
	TInt err( KErrArgument );
    if( aMaskDisplayMode == EGray2 || aMaskDisplayMode == EGray256 )
        {
    	err = iBitmap->Create( aSize, aBitmapDisplayMode );
    	if( err )
    		{
    		return err;
    		}
    	err = iMask->Create( aSize, aMaskDisplayMode );
    	if( err )
    		{
    		iBitmap->Reset();
    		}
        }
	return err;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Copy
// -----------------------------------------------------------------------------
TInt CIHLBitmap::Copy( const CFbsBitmap& aBitmap, TBool aDuplicate )
	{
	Reset();
	TInt err( KErrNone );
	TInt bitmapHandle( aBitmap.Handle() );
	if( bitmapHandle )
		{
		if( aDuplicate )
			{
			err = iBitmap->Duplicate( bitmapHandle );
			}
		else
			{
			err = CopyBitmap( aBitmap, *iBitmap );
			}
		}
	return err;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Copy
// -----------------------------------------------------------------------------
TInt CIHLBitmap::Copy( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask, TBool aDuplicate )
	{
	Reset();
	TInt bitmapHandle( aBitmap.Handle() );
	TInt maskHandle( aMask.Handle() );
	if( bitmapHandle && maskHandle &&
		aBitmap.SizeInPixels() != aMask.SizeInPixels() )
		{
		return KErrArgument;
		}

	if( maskHandle )
		{
		switch( aMask.DisplayMode() )
		    {
		    case EGray2:
		    case EGray256:
		        {
		        break;
		        }
		    default:
		        {
		        return KErrArgument;
		        }
		    }
		}

	TInt err( KErrNone );
	if( bitmapHandle )
		{
		if( aDuplicate )
			{
			err = iBitmap->Duplicate( bitmapHandle );
			}
		else
			{
			err = CopyBitmap( aBitmap, *iBitmap );
			}
		}

	if( !err && maskHandle )
		{
		if( aDuplicate )
			{
			err = iMask->Duplicate( maskHandle );
			}
		else
			{
			err = CopyBitmap( aMask, *iMask );
			}
		if( err )
			{
			iBitmap->Reset();
			}
		}
	return err;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Copy
// -----------------------------------------------------------------------------
TInt CIHLBitmap::Copy( const MIHLBitmap& aBitmap, TBool aDuplicate )
	{
	return Copy( aBitmap.Bitmap(), aBitmap.Mask(), aDuplicate );
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Reset
// -----------------------------------------------------------------------------
void CIHLBitmap::Reset()
	{
	iBitmap->Reset();
	iMask->Reset();
	iEditorPtr = NULL;
	iEditorValue = 0;
	}


// -----------------------------------------------------------------------------
// CIHLBitmap::IsCreated
// -----------------------------------------------------------------------------
TBool CIHLBitmap::IsCreated() const
	{
	return ( iBitmap->Handle() != 0 );
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Bitmap
// -----------------------------------------------------------------------------
const CFbsBitmap& CIHLBitmap::Bitmap() const
	{
	return *iBitmap;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::HasMask
// -----------------------------------------------------------------------------
TBool CIHLBitmap::HasMask() const
	{
	return ( iMask->Handle() != 0 );
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Mask
// -----------------------------------------------------------------------------
const CFbsBitmap& CIHLBitmap::Mask() const
	{
	return *iMask;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Draw
// -----------------------------------------------------------------------------
void CIHLBitmap::Draw( CBitmapContext& aContext, const TPoint& aPoint ) const
	{
	if( iBitmap->Handle() )
		{
		if( iMask->Handle() )
			{
			aContext.BitBltMasked( aPoint, iBitmap, iBitmap->SizeInPixels(), iMask, EFalse );
			}
		else
			{
			aContext.BitBlt( aPoint, iBitmap, iBitmap->SizeInPixels() );
			}
		}
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::Draw
// -----------------------------------------------------------------------------
void CIHLBitmap::Draw( CBitmapContext& aContext, const TPoint& aPoint,
                       const TRect& aSourceRect ) const
	{
	if( iBitmap->Handle() )
		{
		if( iMask->Handle() )
			{
			aContext.BitBltMasked( aPoint, iBitmap, aSourceRect, iMask, EFalse );
			}
		else
			{
			aContext.BitBlt( aPoint, iBitmap, aSourceRect );
			}
		}
	}

// Internal interface
// -----------------------------------------------------------------------------
// CIHLBitmap::SetFilename
// -----------------------------------------------------------------------------
CFbsBitmap& CIHLBitmap::BitmapModifyable()
	{
	return *iBitmap;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::SetFilename
// -----------------------------------------------------------------------------
CFbsBitmap& CIHLBitmap::MaskModifyable()
	{
	return *iMask;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::SetEditorPtr
// -----------------------------------------------------------------------------
void CIHLBitmap::SetEditorPtr( const TAny* aEditorPtr )
	{
	iEditorPtr = aEditorPtr;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::SetEditorValue
// -----------------------------------------------------------------------------
void CIHLBitmap::SetEditorValue( TInt aEditorValue )
	{
	iEditorValue = aEditorValue;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::EditorPtr
// -----------------------------------------------------------------------------
const TAny* CIHLBitmap::EditorPtr() const
	{
	return iEditorPtr;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::FrameIndex
// -----------------------------------------------------------------------------
TInt CIHLBitmap::EditorValue() const
	{
	return iEditorValue;
	}

// Private methods
// -----------------------------------------------------------------------------
// CIHLBitmap::CopyBitmap
// -----------------------------------------------------------------------------
TInt CIHLBitmap::CopyBitmap( const CFbsBitmap& aSource, CFbsBitmap& aDestination )
	{
	TSize size( aSource.SizeInPixels() );
	TDisplayMode displayMode( aSource.DisplayMode() );
	TInt err( aDestination.Create( size, displayMode ) );
	if( !err )
		{
		err = CopyBitmapData( aSource, aDestination, size, displayMode );
		if( err )
			{
			aDestination.Reset();
			}
		}
	return err;
	}

// -----------------------------------------------------------------------------
// CIHLBitmap::CopyBitmapData
// -----------------------------------------------------------------------------
TInt CIHLBitmap::CopyBitmapData( const CFbsBitmap& aSource, CFbsBitmap& aDestination,
								 const TSize& aSize, const TDisplayMode& aDisplayMode )
	{
	HBufC8* scanLine = HBufC8::New( aSource.ScanLineLength( aSize.iWidth, aDisplayMode ) );
	if( scanLine )
		{
		TPtr8 scanPtr( scanLine->Des() );
		TPoint pp;
		for( pp.iY = 0; pp.iY < aSize.iHeight; ++pp.iY )
			{
			aSource.GetScanLine( scanPtr, pp, aSize.iWidth, aDisplayMode );
			aDestination.SetScanLine( scanPtr, pp.iY );
			}

		delete scanLine;
		return KErrNone;
		}
	return KErrNoMemory; // scanLine alloc failed
	}

//  End of File
