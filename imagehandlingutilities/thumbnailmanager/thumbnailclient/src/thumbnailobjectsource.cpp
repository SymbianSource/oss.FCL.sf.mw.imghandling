/*
* Copyright (c) 2006-2007 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Class for encapsulating the data source for objects.
 *
*/

#include <fbs.h>

#include "thumbnailobjectsource.h"


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailObjectSource::NewL()
// CThumbnailObjectSource::NewLC()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewL( const TDesC&
    aUri, const TDesC& aMimeType )
    {
    CThumbnailObjectSource* self = CThumbnailObjectSource::NewLC( aUri,
        aMimeType );
    CleanupStack::Pop( self );
    return self;
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewLC( const TDesC&
    aUri, const TDesC& aMimeType )
    {
    CThumbnailObjectSource* self = new( ELeave )CThumbnailObjectSource();
    CleanupStack::PushL( self );
    self->ConstructL( aUri, aMimeType );
    return self;
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewL( const RFile64&
    aFile, const TDesC& aMimeType )
    {
    CThumbnailObjectSource* self = CThumbnailObjectSource::NewLC( aFile,
        aMimeType );
    CleanupStack::Pop( self );
    return self;
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewLC( const RFile64&
    aFile, const TDesC& aMimeType )
    {
    CThumbnailObjectSource* self = new( ELeave )CThumbnailObjectSource();
    CleanupStack::PushL( self );
    self->ConstructL( aFile, aMimeType );
    return self;
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewL( CFbsBitmap*  aBitmap, 
    const TDesC& aUri )
    {
    CThumbnailObjectSource* self = CThumbnailObjectSource::NewLC( aBitmap, aUri );
    CleanupStack::Pop( self );
    return self;   
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewLC( CFbsBitmap*  aBitmap,
    const TDesC& aUri )
    {
    CThumbnailObjectSource* self = new( ELeave )CThumbnailObjectSource();
    CleanupStack::PushL( self );
    self->ConstructL( aBitmap, aUri );
    return self;
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewL( TDesC8* aBuffer,
    TDesC& aMimeType, const TDesC& aUri)
    {
    CThumbnailObjectSource* self = CThumbnailObjectSource::NewLC( aBuffer, 
        aMimeType, aUri );
    CleanupStack::Pop( self );
    return self;   
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewLC( TDesC8* aBuffer, 
    TDesC& aMimeType, const TDesC& aUri)
    {
    CThumbnailObjectSource* self = new( ELeave )CThumbnailObjectSource();
    CleanupStack::PushL( self );
    self->ConstructL( aBuffer, aMimeType, aUri );
    return self;
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewL( const TDesC& 
    aUri, const TThumbnailId aThumbnailId, const TDesC& aMimeType )
    {
    CThumbnailObjectSource* self = CThumbnailObjectSource::NewLC( aUri,
        aThumbnailId, aMimeType );
    CleanupStack::Pop( self );
    return self;
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewLC( const TDesC& aUri, 
    const TThumbnailId aThumbnailId, const TDesC& aMimeType )
    {
    CThumbnailObjectSource* self = new( ELeave )CThumbnailObjectSource();
    CleanupStack::PushL( self );
    self->ConstructL( aUri, aMimeType, aThumbnailId );
    return self;
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewL( const TDesC& aUri, 
    const TDesC& aTargetUri, const TDesC& aMimeType )
    {
    CThumbnailObjectSource* self = CThumbnailObjectSource::NewLC( aUri, aTargetUri,
        aMimeType );
    CleanupStack::Pop( self );
    return self;   
    }

EXPORT_C CThumbnailObjectSource* CThumbnailObjectSource::NewLC( const TDesC& aUri, 
    const TDesC& aTargetUri, const TDesC& aMimeType )
    {
    CThumbnailObjectSource* self = new( ELeave )CThumbnailObjectSource();
    CleanupStack::PushL( self );
    self->ConstructL( aUri, aTargetUri, aMimeType );
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailObjectSource::CThumbnailObjectSource()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailObjectSource::CThumbnailObjectSource()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailObjectSource::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailObjectSource::ConstructL( const TDesC& aUri, const TDesC&
    aMimeType)
    {
    iUri = aUri.AllocL();
    iMimeType = HBufC8::NewL( aMimeType.Length() );
    iMimeType->Des().Copy( aMimeType );
    iThumbnailId = 0;
    iBitmap = NULL;
    iTargetUri = NULL;
    }

void CThumbnailObjectSource::ConstructL( const RFile64& aFile, const TDesC&
    aMimeType )
    {
    iFile = aFile;
    iMimeType = HBufC8::NewL( aMimeType.Length() );
    iMimeType->Des().Copy( aMimeType );
    iThumbnailId = 0;
    iBitmap = NULL;
    iTargetUri = NULL;
    }

void CThumbnailObjectSource::ConstructL( CFbsBitmap* aBitmap, const TDesC& 
    aUri )
    {
    iBitmap = aBitmap;
    iUri = aUri.AllocL();
    iThumbnailId = 0;
    iTargetUri = NULL;
    }

void CThumbnailObjectSource::ConstructL( TDesC8* aBuffer, const TDesC&
    aMimeType, const TDesC& aUri )
    {
    iBuffer = aBuffer;
    iMimeType = HBufC8::NewL( aMimeType.Length() );
    iMimeType->Des().Copy( aMimeType );
    iUri = aUri.AllocL(); 
    iThumbnailId = 0;
    iBitmap = NULL;
    iTargetUri = NULL;
    }

void CThumbnailObjectSource::ConstructL( const TDesC& aUri, const TDesC&
    aMimeType, TThumbnailId aThumbnailId )
    {
    iUri = aUri.AllocL();
    iMimeType = HBufC8::NewL( aMimeType.Length() );
    iMimeType->Des().Copy( aMimeType );
    iThumbnailId = aThumbnailId;
    iTargetUri = NULL;
    }

void CThumbnailObjectSource::ConstructL( const TDesC& aUri, const TDesC& 
    aTargetUri, const TDesC& aMimeType )
    {
    iUri = aUri.AllocL();
    iTargetUri = aTargetUri.AllocL();
    iMimeType = HBufC8::NewL( aMimeType.Length() );
    iMimeType->Des().Copy( aMimeType );
    iThumbnailId = 0;
    iBitmap = NULL;
    }


// ---------------------------------------------------------------------------
// CThumbnailObjectSource::~CThumbnailObjectSource()()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailObjectSource::~CThumbnailObjectSource()
    {
    delete iUri;
    iUri = NULL;
    delete iMimeType;
    iMimeType = NULL;
    delete iBuffer;
    iBuffer = NULL;
    delete iBitmap;
    iBitmap = NULL;
    }


// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::FileHandle()
// ---------------------------------------------------------------------------
//
EXPORT_C RFile64& CThumbnailObjectSource::FileHandle()
    {
    return iFile;
    }

// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::Uri()
// ---------------------------------------------------------------------------
//
EXPORT_C const TDesC& CThumbnailObjectSource::Uri()
    {
    if ( iUri )
        {
        return * iUri;
        }
    return KNullDesC;
    }

// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::Buffer()
// ---------------------------------------------------------------------------
//
EXPORT_C TDesC8* CThumbnailObjectSource::Buffer()
    {
    return iBuffer;
    }

// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::GetBufferOwnership()
// ---------------------------------------------------------------------------
//
EXPORT_C TDesC8* CThumbnailObjectSource::GetBufferOwnership()
    {
    TDesC8* temp = iBuffer;
    iBuffer = NULL;
    return temp;
    }

// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::aMimeType()
// ---------------------------------------------------------------------------
//
EXPORT_C const TDesC8& CThumbnailObjectSource::MimeType()
    {
    if ( iMimeType )
        {
        return *iMimeType;
        }
    return KNullDesC8;
    }

// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::Id()
// ---------------------------------------------------------------------------
//
EXPORT_C TThumbnailId CThumbnailObjectSource::Id()
    {
    return iThumbnailId;
    }
	
// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::Bitmap()
// ---------------------------------------------------------------------------
//
EXPORT_C CFbsBitmap* CThumbnailObjectSource::Bitmap()
    {
    return iBitmap;
    }

// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::GetBitmapOwnership()
// ---------------------------------------------------------------------------
//
EXPORT_C CFbsBitmap* CThumbnailObjectSource::GetBitmapOwnership()
    {
    CFbsBitmap* temp = iBitmap;
    iBitmap = NULL;
    return temp;
    }

// ---------------------------------------------------------------------------
// CThumbnailObjectSourceImpl::TargetUri()
// ---------------------------------------------------------------------------
//
EXPORT_C const TDesC& CThumbnailObjectSource::TargetUri()
    {
    if ( iTargetUri )
        {
        return * iTargetUri;
        }
    return KNullDesC;
    }


// End of file
