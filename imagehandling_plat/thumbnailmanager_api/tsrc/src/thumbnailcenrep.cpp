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
* Description:  Class for handling central repositoty data
 *
*/


#include <centralrepository.h>
#include <thumbnailmanager.h>

#include "thumbnailcenrep.h"


#define THUMBNAIL_CENREP_UID 0x102830B0

const TUint32 KSizeImageGridWidth = 0x0;
const TUint32 KSizeImageGridHeight = 0x1;
const TUint32 KSizeImageListWidth = 0x2;
const TUint32 KSizeImageListHeight = 0x3;
const TUint32 KSizeImageFullscreenWidth = 0x4;
const TUint32 KSizeImageFullscreenHeight = 0x5;
const TUint32 KSizeVideoGridWidth = 0x6;
const TUint32 KSizeVideoGridHeight = 0x7;
const TUint32 KSizeVideoListWidth = 0x8;
const TUint32 KSizeVideoListHeight = 0x9;
const TUint32 KSizeVideoFullscreenWidth = 0x10;
const TUint32 KSizeVideoFullscreenHeight = 0x11;
const TUint32 KSizeAudioGridWidth = 0x12;
const TUint32 KSizeAudioGridHeight = 0x13;
const TUint32 KSizeAudioListWidth = 0x14;
const TUint32 KSizeAudioListHeight = 0x15;
const TUint32 KSizeAudioFullscreenWidth = 0x16;
const TUint32 KSizeAudioFullscreenHeight = 0x17;
const TUint32 KSizeContactListWidth = 0x18;
const TUint32 KSizeContactListHeight = 0x19;
const TUint32 KSizeContactGridWidth = 0x20;
const TUint32 KSizeContactGridHeight = 0x21;
const TUint32 KSizeContactFullscreenWidth = 0x22;
const TUint32 KSizeContactFullscreenHeight = 0x23;

const TUint32 KAutoCreateImageGrid = 0x100;
const TUint32 KAutoCreateImageList = 0x101;
const TUint32 KAutoCreateImageFullscreen = 0x102;
const TUint32 KAutoCreateVideoGrid = 0x103;
const TUint32 KAutoCreateVideoList = 0x104;
const TUint32 KAutoCreateVideoFullscreen = 0x105;
const TUint32 KAutoCreateAudioGrid = 0x106;
const TUint32 KAutoCreateAudioList = 0x107;
const TUint32 KAutoCreateAudioFullscreen = 0x108;
const TUint32 KAutoCreateContactGrid = 0x109;
const TUint32 KAutoCreateContactList = 0x110;
const TUint32 KAutoCreateContactFullscreen = 0x111;


// ---------------------------------------------------------------------------
// TThumbnailPersistentSize::TThumbnailPersistentSize
// ---------------------------------------------------------------------------
//
TThumbnailPersistentSize::TThumbnailPersistentSize( const TSize& aSize, TBool
    aCrop, TDisplayMode aMode, TInt aFormat ): iSize( aSize ), iCrop( aCrop ),
    iMode( aMode ), iFormat( aFormat )
    {
    iType = EUnknownThumbnailSize;
    iSourceType = EUnknownSourceType;
    iAutoCreate = ETrue;
    }

// ---------------------------------------------------------------------------
// TThumbnailPersistentSize::TThumbnailPersistentSize
// ---------------------------------------------------------------------------
//
TThumbnailPersistentSize::TThumbnailPersistentSize( TThumbnailSize aType,
    const TSize& aSize, TBool aCrop, TDisplayMode aMode, TInt aFormat,
    TBool aAutoCreate )
    : iType( aType ), iSize( aSize ), iCrop( aCrop ), iMode( aMode ),
      iFormat( aFormat ), iAutoCreate( aAutoCreate )
    {
    switch ( aType )
        {        
        case EImageGridThumbnailSize:
        case EImageListThumbnailSize:
        case EImageFullScreenThumbnailSize:
            iSourceType = EImage;
            break;
        case EVideoGridThumbnailSize:
        case EVideoListThumbnailSize:
        case EVideoFullScreenThumbnailSize:  
            iSourceType = EVideo;
            break;
        case EAudioGridThumbnailSize:
        case EAudioListThumbnailSize:
        case EAudioFullScreenThumbnailSize:
            iSourceType = EAudio;
            break;
        case EContactGridThumbnailSize:
        case EContactListThumbnailSize:
        case EContactFullScreenThumbnailSize:
            iSourceType = EContact;
            break;
        default:
            iSourceType = EUnknownSourceType;        
        }

    }

// ---------------------------------------------------------------------------
// TThumbnailAutoCreate::TThumbnailAutoCreate
// ---------------------------------------------------------------------------
//
TThumbnailAutoCreate::TThumbnailAutoCreate()
    : iImageGrid(EFalse), iImageList(EFalse), iImageFullscreen(EFalse),
      iVideoGrid(EFalse), iVideoList(EFalse), iVideoFullscreen(EFalse),
      iAudioGrid(EFalse), iAudioList(EFalse), iAudioFullscreen(EFalse),
      iContactGrid(EFalse), iContactList(EFalse), iContactFullscreen(EFalse)
    {
    // No implementation required
    }

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailCenRep::CThumbnailCenRep()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailCenRep::CThumbnailCenRep()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailCenRep::~CThumbnailCenRep()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailCenRep::~CThumbnailCenRep()
    {
    iPersistentSizes.Close();
    delete iAutoCreate;
    delete iRepository;
    }

// ---------------------------------------------------------------------------
// CThumbnailCenRep::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailCenRep* CThumbnailCenRep::NewL()
    {
    CThumbnailCenRep* self = new( ELeave )CThumbnailCenRep();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// CThumbnailCenRep::ConstructL()
// Returns id of specific task.
// ---------------------------------------------------------------------------
//
void CThumbnailCenRep::ConstructL()
    {
    iRepository = CRepository::NewL( TUid::Uid( THUMBNAIL_CENREP_UID ));

    TInt xSize( 0 );
    TInt ySize( 0 );
    TBool flags( EFalse );
    const TBool KGridAndListThumbnailCropped = ETrue; 
    TInt raw_mode( EColor64K );
    TInt format( 0 );
    TBool autoCreate( EFalse );
    
    User::LeaveIfError( iRepository->Get( KSizeImageGridWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeImageGridHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateImageGrid, autoCreate ));

    iPersistentSizes.AppendL( TThumbnailPersistentSize( EImageGridThumbnailSize, TSize( xSize, ySize ),
            KGridAndListThumbnailCropped, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));

    User::LeaveIfError( iRepository->Get( KSizeImageListWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeImageListHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateImageList, autoCreate ));

    iPersistentSizes.AppendL( TThumbnailPersistentSize( EImageListThumbnailSize, TSize( xSize, ySize ),
            KGridAndListThumbnailCropped, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));
    
    User::LeaveIfError( iRepository->Get( KSizeImageFullscreenWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeImageFullscreenHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateImageFullscreen, autoCreate ));
    
    iPersistentSizes.AppendL( TThumbnailPersistentSize( EImageFullScreenThumbnailSize, TSize( xSize, ySize ),
                              flags, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));
    
    User::LeaveIfError( iRepository->Get( KSizeVideoGridWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeVideoGridHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateVideoGrid, autoCreate ));

    iPersistentSizes.AppendL( TThumbnailPersistentSize( EVideoGridThumbnailSize, TSize( xSize, ySize ),
            KGridAndListThumbnailCropped, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));

    User::LeaveIfError( iRepository->Get( KSizeVideoListWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeVideoListHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateVideoList, autoCreate ));

    iPersistentSizes.AppendL( TThumbnailPersistentSize( EVideoListThumbnailSize, TSize( xSize, ySize ),
            KGridAndListThumbnailCropped, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));
    
    User::LeaveIfError( iRepository->Get( KSizeVideoFullscreenWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeVideoFullscreenHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateVideoFullscreen, autoCreate ));
    
    iPersistentSizes.AppendL( TThumbnailPersistentSize( EVideoFullScreenThumbnailSize, TSize( xSize, ySize ),
                              flags, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));  
    
    User::LeaveIfError( iRepository->Get( KSizeAudioGridWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeAudioGridHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateAudioGrid, autoCreate ));

    iPersistentSizes.AppendL( TThumbnailPersistentSize( EAudioGridThumbnailSize, TSize( xSize, ySize ),
            KGridAndListThumbnailCropped, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));

    User::LeaveIfError( iRepository->Get( KSizeAudioListWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeAudioListHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateAudioList, autoCreate ));

    iPersistentSizes.AppendL( TThumbnailPersistentSize( EAudioListThumbnailSize, TSize( xSize, ySize ),
            KGridAndListThumbnailCropped, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));
    
    User::LeaveIfError( iRepository->Get( KSizeAudioFullscreenWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeAudioFullscreenHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateAudioFullscreen, autoCreate ));
    
    iPersistentSizes.AppendL( TThumbnailPersistentSize( EAudioFullScreenThumbnailSize, TSize( xSize, ySize ),
                              flags, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));     

    User::LeaveIfError( iRepository->Get( KSizeContactGridWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeContactGridHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateContactGrid, autoCreate ));

    iPersistentSizes.AppendL( TThumbnailPersistentSize( EContactGridThumbnailSize, TSize( xSize, ySize ),
            KGridAndListThumbnailCropped, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));
    
    User::LeaveIfError( iRepository->Get( KSizeContactListWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeContactListHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateContactList, autoCreate ));

    iPersistentSizes.AppendL( TThumbnailPersistentSize( EContactListThumbnailSize, TSize( xSize, ySize ),
            KGridAndListThumbnailCropped, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));
    
    User::LeaveIfError( iRepository->Get( KSizeContactFullscreenWidth, xSize ));
    User::LeaveIfError( iRepository->Get( KSizeContactFullscreenHeight, ySize ));
    User::LeaveIfError( iRepository->Get( KAutoCreateContactFullscreen, autoCreate ));
    
    iPersistentSizes.AppendL( TThumbnailPersistentSize( EContactFullScreenThumbnailSize, TSize( xSize, ySize ),
                              flags, static_cast <TDisplayMode> (raw_mode), format, autoCreate ));     
    
    iAutoCreate = new (ELeave) TThumbnailAutoCreate();
    
    User::LeaveIfError( iRepository->Get( KAutoCreateImageGrid, iAutoCreate->iImageGrid ));
    User::LeaveIfError( iRepository->Get( KAutoCreateImageList, iAutoCreate->iImageList ));
    User::LeaveIfError( iRepository->Get( KAutoCreateImageFullscreen, iAutoCreate->iImageFullscreen ));
    User::LeaveIfError( iRepository->Get( KAutoCreateVideoGrid, iAutoCreate->iVideoGrid ));
    User::LeaveIfError( iRepository->Get( KAutoCreateVideoList, iAutoCreate->iVideoList ));
    User::LeaveIfError( iRepository->Get( KAutoCreateVideoFullscreen, iAutoCreate->iVideoFullscreen ));
    User::LeaveIfError( iRepository->Get( KAutoCreateAudioGrid, iAutoCreate->iAudioGrid ));
    User::LeaveIfError( iRepository->Get( KAutoCreateAudioList, iAutoCreate->iAudioList ));
    User::LeaveIfError( iRepository->Get( KAutoCreateAudioFullscreen, iAutoCreate->iAudioFullscreen ));    
    User::LeaveIfError( iRepository->Get( KAutoCreateContactGrid, iAutoCreate->iContactGrid ));
    User::LeaveIfError( iRepository->Get( KAutoCreateContactList, iAutoCreate->iContactList ));
    User::LeaveIfError( iRepository->Get( KAutoCreateContactFullscreen, iAutoCreate->iContactFullscreen )); 
    }

// ---------------------------------------------------------------------------
// CThumbnailCenRep::GetPersistentSizes()
// ---------------------------------------------------------------------------
//
RArray < TThumbnailPersistentSize > & CThumbnailCenRep::GetPersistentSizes()
    {
    return iPersistentSizes;
    }

// ---------------------------------------------------------------------------
// CThumbnailCenRep::GetAutoCreateParams()
// ---------------------------------------------------------------------------
//
TThumbnailAutoCreate & CThumbnailCenRep::GetAutoCreateParams()
    {
    return *iAutoCreate;
    }

TThumbnailPersistentSize & CThumbnailCenRep::PersistentSizeL( TThumbnailSize
        aThumbnailSize )
    {
    TThumbnailPersistentSize* persistentSize = NULL;
    TInt i = iPersistentSizes.Count();
    for ( ; --i >= 0; )
        {
        persistentSize = &iPersistentSizes[i];
        if ( persistentSize->iType == aThumbnailSize )
            {
            break;
            }
        }
    if ( i < 0 )
        { // size not found
        User::Leave( KErrNotFound );
        }
    
    return *persistentSize;
    }

// End of file
