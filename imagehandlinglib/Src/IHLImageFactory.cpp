/*
* Copyright (c) 2004-2007 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Static factory class for creating file instances.
*
*/


// INCLUDE FILES
#include <IHLImageFactory.h>

#include "CIHLFileImage.h"
#include "CIHLFileImageExtJpg.h"
#include <IclExtJpegApi.h>

// Private namespace for constants and functions
namespace
	{
	// First image index
	const TInt KIHLFirstImage = 0;
	const TInt KIHLNullOptions = 0;

	const TInt KMimeTypeStringLength = 255;
	_LIT( KJpgType, "image/jpeg" );


    // Helper functions
    // -----------------------------------------------------------------------------
    // OpenExtendedFileImageL
    // -----------------------------------------------------------------------------
    //
	CIHLFileImageExtJpg* OpenExtendedFileImageL( RFs& aFs, const TDesC& aFilename,
												TInt aImageIndex, const TUint32 aOptions )
		{
		CIHLFileImageExtJpg* fileImageExtJpg = NULL;
        TRAPD( err, fileImageExtJpg = CIHLFileImageExtJpg::NewL( aFs, aFilename,
                                                           aImageIndex, aOptions ) );
        if( !err )
            {
            // Hw or Sw extended decoder found; check for cropping capability
            CleanupStack::PushL( fileImageExtJpg );
            if( !( fileImageExtJpg->CapabilitiesL() & CExtJpegDecoder::ECapCropping ) )
                {
                //Decoder does not support cropping, use the old implementation
                CleanupStack::PopAndDestroy( fileImageExtJpg );
                fileImageExtJpg = NULL;
                }
            else
                {
                CleanupStack::Pop( fileImageExtJpg );
                }
            }

        return fileImageExtJpg;
		}

    // -----------------------------------------------------------------------------
    // MimeTypeIsJpgL
    // -----------------------------------------------------------------------------
    //
    TBool MimeTypeIsJpgL( RFile& aFile )
        {
        using namespace ContentAccess;

        TBuf<KMimeTypeStringLength> mime;
        CContent* content = CContent::NewL( aFile );
        CleanupStack::PushL( content );
        CData* data = content->OpenContentL( EPeek );
        CleanupStack::PushL( data );
        User::LeaveIfError( data->GetStringAttribute( EMimeType, mime ) );
        CleanupStack::PopAndDestroy( 2 ); //data, content
        if( mime.Compare( KJpgType ) == 0 ) return ETrue;
        return EFalse;
        }
	}

// ============================ STATIC FUNCTIONS ===============================
// -----------------------------------------------------------------------------
// IHLImageFactory::OpenFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenFileImageL( RFs& aFs, const TDesC& aFilename )
	{
	RFile fileHandle;
	TInt err( fileHandle.Open( aFs, aFilename, EFileShareReadersOnly ) );
	if( err )
		{
		User::LeaveIfError( fileHandle.Open( aFs, aFilename, EFileShareAny ) );
		}
	CleanupClosePushL( fileHandle );

	if ( MimeTypeIsJpgL( fileHandle ) )
		{
		CIHLFileImageExtJpg* fileImageExtJpg = OpenExtendedFileImageL( aFs,
																	aFilename,
																	KIHLFirstImage,
																	KIHLNullOptions );
		if ( fileImageExtJpg )
			{
			CleanupStack::PopAndDestroy();	// fileHandle.Close()
			return fileImageExtJpg;
			}
		}

	CIHLFileImage* fileImage = CIHLFileImage::NewL( fileHandle, KIHLFirstImage, KIHLNullOptions );

	CleanupStack::PopAndDestroy(); // fileHandle.Close()

	return fileImage;
	}

// -----------------------------------------------------------------------------
// IHLImageFactory::OpenFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenFileImageL( RFs& aFs, const TDesC& aFilename,
														 TInt aImageIndex )
	{
	RFile fileHandle;
	TInt err( fileHandle.Open( aFs, aFilename, EFileShareReadersOnly ) );
	if( err )
		{
		User::LeaveIfError( fileHandle.Open( aFs, aFilename, EFileShareAny ) );
		}
	CleanupClosePushL( fileHandle );

	if ( MimeTypeIsJpgL( fileHandle ) )
		{
		CIHLFileImageExtJpg* fileImageExtJpg = OpenExtendedFileImageL( aFs,
																	aFilename,
																	aImageIndex,
																	KIHLNullOptions );
		if ( fileImageExtJpg )
			{
			CleanupStack::PopAndDestroy();	// fileHandle.Close()
			return fileImageExtJpg;
			}
		}

	CIHLFileImage* fileImage = CIHLFileImage::NewL( fileHandle, aImageIndex, KIHLNullOptions );

	CleanupStack::PopAndDestroy(); // fileHandle.Close()

	return fileImage;
	}

// -----------------------------------------------------------------------------
// IHLImageFactory::OpenFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenFileImageL( RFs& aFs, const TDesC& aFilename,
														 TInt aImageIndex, const TUint32 aOptions )
	{
	RFile fileHandle;
	TInt err( fileHandle.Open( aFs, aFilename, EFileShareReadersOnly ) );
	if( err )
		{
		User::LeaveIfError( fileHandle.Open( aFs, aFilename, EFileShareAny ) );
		}
	CleanupClosePushL( fileHandle );

	if ( MimeTypeIsJpgL( fileHandle ) )
		{
		CIHLFileImageExtJpg* fileImageExtJpg = OpenExtendedFileImageL( aFs,
																	aFilename,
																	aImageIndex,
																	aOptions );
		if ( fileImageExtJpg )
			{
			CleanupStack::PopAndDestroy();	// fileHandle.Close()
			return fileImageExtJpg;
			}
		}

	CIHLFileImage* fileImage = CIHLFileImage::NewL( fileHandle, aImageIndex, aOptions );

	CleanupStack::PopAndDestroy(); // fileHandle.Close()

	return fileImage;
	}

// -----------------------------------------------------------------------------
// IHLImageFactory::OpenFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenFileImageL( RFile& aFile )
	{
	return CIHLFileImage::NewL( aFile, KIHLFirstImage, KIHLNullOptions );
	}

// -----------------------------------------------------------------------------
// IHLImageFactory::OpenFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenFileImageL( RFile& aFile, TInt aImageIndex )
	{
	return CIHLFileImage::NewL( aFile, aImageIndex, KIHLNullOptions );
	}

// -----------------------------------------------------------------------------
// IHLImageFactory::OpenFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenFileImageL( RFile& aFile, TInt aImageIndex,
														 const TUint32 aOptions )
	{
	return CIHLFileImage::NewL( aFile, aImageIndex, aOptions );
	}

// -----------------------------------------------------------------------------
// IHLImageFactory::OpenBufferedFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenBufferedFileImageL( RFs& aFs, const TDesC8& aDataBuf )
	{
	return CIHLFileImage::NewL( aFs, aDataBuf, KIHLFirstImage, KIHLNullOptions );
	}

// -----------------------------------------------------------------------------
// IHLImageFactory::OpenBufferedFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenBufferedFileImageL( RFs& aFs,
                                        const TDesC8& aDataBuf, TInt aImageIndex )
	{
	return CIHLFileImage::NewL( aFs, aDataBuf, aImageIndex, KIHLNullOptions );
	}

// -----------------------------------------------------------------------------
// IHLImageFactory::OpenBufferedFileImageL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLFileImage* IHLImageFactory::OpenBufferedFileImageL( RFs& aFs,
            const TDesC8& aDataBuf, TInt aImageIndex, const TUint32 aOptions )
	{
	return CIHLFileImage::NewL( aFs, aDataBuf, aImageIndex, aOptions );
	}

//  End of File
