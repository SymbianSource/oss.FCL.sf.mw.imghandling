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
#include "thumbnailmanageruids.hrh"
#include "thumbnailstore.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailmanagerprivatecrkeys.h"

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
      iAudioGrid(EFalse), iAudioList(EFalse), iAudioFullscreen(EFalse)
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
    TInt raw_mode( KThumbnailDefaultDisplayMode );
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
