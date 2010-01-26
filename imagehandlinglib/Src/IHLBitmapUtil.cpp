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


// INCLUDE FILES
#include <IHLBitmapUtil.h>
#include <fbs.h>

// ============================ STATIC FUNCTIONS ===============================
// -----------------------------------------------------------------------------
// IHLBitmapUtil::DuplicateBitmapL
// -----------------------------------------------------------------------------
EXPORT_C CFbsBitmap* IHLBitmapUtil::DuplicateBitmapL( const CFbsBitmap& aBitmap )
	{
	CFbsBitmap* bitmap = DuplicateBitmapLC( aBitmap );
	CleanupStack::Pop(); // bitmap
	return bitmap;
	}

// -----------------------------------------------------------------------------
// IHLBitmapUtil::DuplicateBitmapLC
// -----------------------------------------------------------------------------
EXPORT_C CFbsBitmap* IHLBitmapUtil::DuplicateBitmapLC( const CFbsBitmap& aBitmap )
	{
	CFbsBitmap* bitmap = new(ELeave) CFbsBitmap;
	CleanupStack::PushL( bitmap );
	User::LeaveIfError( bitmap->Duplicate( aBitmap.Handle() ) );
	return bitmap;
	}

// -----------------------------------------------------------------------------
// IHLBitmapUtil::CopyBitmapL
// -----------------------------------------------------------------------------
EXPORT_C CFbsBitmap* IHLBitmapUtil::CopyBitmapL( const CFbsBitmap& aBitmap )
	{
	CFbsBitmap* bitmap = CopyBitmapLC( aBitmap );
	CleanupStack::Pop(); // bitmap
	return bitmap;
	}

// -----------------------------------------------------------------------------
// IHLBitmapUtil::CopyBitmapLC
// -----------------------------------------------------------------------------
EXPORT_C CFbsBitmap* IHLBitmapUtil::CopyBitmapLC( const CFbsBitmap& aBitmap )
	{
    CFbsBitmap* bitmap = new (ELeave) CFbsBitmap();
    CleanupStack::PushL( bitmap );
	if( !aBitmap.Handle() )
		{
		return bitmap;
		}

	TSize size( aBitmap.SizeInPixels() );
	TDisplayMode displayMode( aBitmap.DisplayMode() );

    User::LeaveIfError( bitmap->Create( size, displayMode ) );

    HBufC8* scanLine = HBufC8::NewLC(
        aBitmap.ScanLineLength( size.iWidth, displayMode ) );
    TPtr8 scanPtr( scanLine->Des() );

    TPoint pp;
    for ( TInt y( 0 ); y < size.iHeight; ++y )
        {
		pp.iY = y;
        aBitmap.GetScanLine( scanPtr, pp, size.iWidth, displayMode );
        bitmap->SetScanLine( scanPtr, y );
        }

    CleanupStack::PopAndDestroy(); // scanLine

	return bitmap;
	}

// -----------------------------------------------------------------------------
// IHLBitmapUtil::CopyBitmapL
// -----------------------------------------------------------------------------
EXPORT_C CFbsBitmap* IHLBitmapUtil::CopyBitmapL( const CFbsBitmap& aBitmap, const TRect& aRect )
	{
	CFbsBitmap* bitmap = CopyBitmapLC( aBitmap, aRect );
	CleanupStack::Pop(); // bitmap
	return bitmap;
	}

// -----------------------------------------------------------------------------
// IHLBitmapUtil::CopyBitmapLC
// -----------------------------------------------------------------------------
EXPORT_C CFbsBitmap* IHLBitmapUtil::CopyBitmapLC( const CFbsBitmap& aBitmap, const TRect& aRect )
	{
    CFbsBitmap* bitmap = new (ELeave) CFbsBitmap();
    CleanupStack::PushL( bitmap );
	if( !aBitmap.Handle() )
		{
		return bitmap;
		}

	TSize clipSize( aRect.Size() );
	TSize sourceSize( aBitmap.SizeInPixels() );
	if( aRect.iTl.iX == aRect.iBr.iX ||
		aRect.iTl.iY == aRect.iBr.iY ||
	    aRect.iTl.iX < 0 ||
	    aRect.iTl.iX > sourceSize.iWidth ||
	    aRect.iTl.iY < 0 ||
	    aRect.iTl.iY > sourceSize.iHeight )
		{
		User::Leave( KErrArgument );
		}
	
	TDisplayMode dispMode( aBitmap.DisplayMode() );
    User::LeaveIfError( bitmap->Create( clipSize, dispMode ) );

    HBufC8* scanLine = HBufC8::NewLC(
        aBitmap.ScanLineLength( clipSize.iWidth, dispMode ) );
    TPtr8 scanPtr( scanLine->Des() );

    TPoint pp( aRect.iTl );
    for ( TInt y( 0 ); y < clipSize.iHeight; ++y, ++pp.iY )
        {
        aBitmap.GetScanLine( scanPtr, pp, clipSize.iWidth, dispMode );
        bitmap->SetScanLine( scanPtr, y );
        }

    CleanupStack::PopAndDestroy(); // scanLine
	return bitmap;
	}

// -----------------------------------------------------------------------------
// IHLBitmapUtil::ScaleBitmap
// -----------------------------------------------------------------------------
EXPORT_C TInt IHLBitmapUtil::ScaleBitmap( const CFbsBitmap& /*aSrcBitmap*/,
										  CFbsBitmap& /*aDstBitmap*/,
										  TUint32 /*aOptions*/ )
	{
	return KErrNotSupported;
	}

// -----------------------------------------------------------------------------
// IHLBitmapUtil::ScaleBitmap
// -----------------------------------------------------------------------------
EXPORT_C TInt IHLBitmapUtil::ScaleBitmap( const CFbsBitmap& /*aSrcBitmap*/,
										  const TRect& /*aSrcRect*/,
										  CFbsBitmap& /*aDstBitmap*/,
										  const TRect& /*aDstRect*/,
										  TUint32 /*aOptions*/ )
	{
	return KErrNotSupported;
	}

//  End of File
