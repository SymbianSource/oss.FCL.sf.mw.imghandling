/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Thumbnail Manager API tests
*
*/

// [INCLUDE FILES] - do not remove
#include <e32svr.h>
#include <StifParser.h>
#include <Stiftestinterface.h>

#include <fbs.h>
#include <sqldb.h>
#include <ImageConversion.h>
#include <thumbnailmanager.h>

#include "ThumbnailManagerTest.h"


// ============================= LOCAL FUNCTIONS ===============================

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CThumbnailManagerTest::Delete
// Delete here all resources allocated and opened from test methods. 
// Called from destructor. 
// -----------------------------------------------------------------------------
//
void CThumbnailManagerTest::Delete() 
    {
    delete iPreviewThumbnail;
    iPreviewThumbnail = NULL;
    delete iThumbnail;
    iThumbnail = NULL;
    delete iEngine;
    iEngine = NULL;
    delete iObjectSource;
    iObjectSource = NULL;
    delete iCenrep;
    iCenrep = NULL;
    }

// -----------------------------------------------------------------------------
// CThumbnailManagerTest::RunMethodL
// Run specified method. Contains also table of test mothods and their names.
// -----------------------------------------------------------------------------
//
TInt CThumbnailManagerTest::RunMethodL( CStifItemParser& aItem ) 
    {
    static TStifFunctionInfo const KFunctions[] =
        {  
        ENTRY( "CreateInstanceL", CThumbnailManagerTest::CreateInstanceL ),
        ENTRY( "DeleteInstance", CThumbnailManagerTest::DeleteInstance ),
        ENTRY( "SetThumbnailSizeL", CThumbnailManagerTest::SetThumbnailSizeL ),
        ENTRY( "CheckThumbnailSizeL", CThumbnailManagerTest::CheckThumbnailSizeL ),
        ENTRY( "SetThumbnailEnumSizeL", CThumbnailManagerTest::SetThumbnailEnumSizeL ),
        ENTRY( "CreateSourceInstancePathL", CThumbnailManagerTest::CreateSourceInstancePathL ),
        ENTRY( "CreateSourceInstancePathTargetL", CThumbnailManagerTest::CreateSourceInstancePathTargetL ),
        ENTRY( "CreateSourceInstanceHandleL", CThumbnailManagerTest::CreateSourceInstanceHandleL ),
        ENTRY( "CreateSourceInstanceBufferL", CThumbnailManagerTest::CreateSourceInstanceBufferL ),
        ENTRY( "CreateSourceInstanceBitmapL", CThumbnailManagerTest::CreateSourceInstanceBitmapL ),
        ENTRY( "DeleteSourceInstance", CThumbnailManagerTest::DeleteSourceInstance ),
        ENTRY( "GetThumbnailL", CThumbnailManagerTest::GetThumbnailL ),
        ENTRY( "GetThumbnailByIdL", CThumbnailManagerTest::GetThumbnailByIdL ),
        ENTRY( "SetThumbnailL", CThumbnailManagerTest::SetThumbnailL ),
        ENTRY( "ImportThumbnailL", CThumbnailManagerTest::ImportThumbnailL ),
        ENTRY( "CheckPreviewThumbnailL", CThumbnailManagerTest::CheckPreviewThumbnailL ),
        ENTRY( "CheckThumbnailL", CThumbnailManagerTest::CheckThumbnailL ),
        ENTRY( "CheckThumbnailCenrepL", CThumbnailManagerTest::CheckThumbnailCenrepL ),
        ENTRY( "CancelPrevious", CThumbnailManagerTest::CancelPrevious ),
        ENTRY( "ChangePreviousPriority", CThumbnailManagerTest::ChangePreviousPriority ),
        ENTRY( "SetFlagsL", CThumbnailManagerTest::SetFlagsL ),
        ENTRY( "CheckFlags", CThumbnailManagerTest::CheckFlags ),
        ENTRY( "SetQualityPreferenceL", CThumbnailManagerTest::SetQualityPreferenceL ),
        ENTRY( "CheckQualityPreference", CThumbnailManagerTest::CheckQualityPreference ),
        ENTRY( "SetDisplayModeL", CThumbnailManagerTest::SetDisplayModeL ),
        ENTRY( "CheckDisplayModeL", CThumbnailManagerTest::CheckDisplayModeL ),
        ENTRY( "DeleteThumbnails", CThumbnailManagerTest::DeleteThumbnails ),
        ENTRY( "CreateThumbnailsByUrlL", CThumbnailManagerTest::CreateThumbnailsByUrlL ),
        ENTRY( "CreateThumbnails", CThumbnailManagerTest::CreateThumbnails ),
        ENTRY( "UpdatePathL", CThumbnailManagerTest::UpdatePathL ),
        ENTRY( "GetSupportedMimeTypesL", CThumbnailManagerTest::GetSupportedMimeTypesL )
        };

    const TInt count = sizeof( KFunctions ) / sizeof( TStifFunctionInfo );

    return RunInternalL( KFunctions, count, aItem );
    }

TInt CThumbnailManagerTest::CreateInstanceL( CStifItemParser&  /*aItem*/ )
    {
    _LIT( KPanicTxt, "CreateInstanceL" );
    __ASSERT_ALWAYS( !iEngine, User::Panic( KPanicTxt, 0 ));
    
    iEngine = CThumbnailManager::NewL( *this );
    
    iCenrep = CThumbnailCenRep::NewL();
    iPersistentSizes = iCenrep->GetPersistentSizes();
    
    iCurrentThumbnailSize = EUnknownThumbnailSize;
    iDoNotCreate = EFalse;
    iCreateThumbnails =  EFalse;

    return KErrNone;
    }

TInt CThumbnailManagerTest::DeleteInstance( CStifItemParser&  /*aItem*/ )
    {
    // Bitmaps must be deleted before CThumbnailManager, since
    // CThumbnailManager will close the FBS session
    delete iPreviewThumbnail;
    iPreviewThumbnail = NULL;
    delete iThumbnail;
    iThumbnail = NULL;
    delete iEngine;
    iEngine = NULL;
    delete iObjectSource;
    iObjectSource = NULL;
    delete iCenrep;
    iCenrep = NULL;
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::SetThumbnailSizeL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "SetThumbnailSizeL" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt width = 0;
    TInt height = 0;
    User::LeaveIfError( aItem.GetNextInt( width ));
    User::LeaveIfError( aItem.GetNextInt( height ));
    iEngine->SetThumbnailSizeL( TSize( width, height ));
    return KErrNone;
    }

TInt CThumbnailManagerTest::CheckThumbnailSizeL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CheckThumbnailSize" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt width = 0;
    TInt height = 0;
    User::LeaveIfError( aItem.GetNextInt( width ));
    User::LeaveIfError( aItem.GetNextInt( height ));
    TInt ret = KErrArgument;
    TSize size = iEngine->ThumbnailSize();
    
    if ( size == TSize( width, height ))
        {
        ret = KErrNone;
        }
    
    return ret;
    }

TInt CThumbnailManagerTest::SetThumbnailEnumSizeL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "SetThumbnailEnumSizeL" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt enumSize = 0;
    User::LeaveIfError( aItem.GetNextInt( enumSize ));
    
    iEngine->SetThumbnailSizeL( (TThumbnailSize)enumSize );
    iCurrentThumbnailSize = (TThumbnailSize)enumSize;
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::SetFlagsL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "SetFlagsL" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt flags = 0;
    User::LeaveIfError( aItem.GetNextInt( flags ));

    iEngine->SetFlagsL(( CThumbnailManager::TThumbnailFlags )flags );
    
    if (flags == CThumbnailManager::EDoNotCreate)
        {
        iDoNotCreate = ETrue;
        }
    else 
        {
        iDoNotCreate = EFalse;
        }
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::CheckFlags( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CheckFlags" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt flags = 0;
    aItem.GetNextInt( flags );

    if ( iEngine->Flags() != flags )
        {
        return KErrArgument;
        }
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::SetQualityPreferenceL( CStifItemParser& aItem
    )
    {
    _LIT( KPanicTxt, "SetQualityPreferenceL" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt qp = 0;
    User::LeaveIfError( aItem.GetNextInt( qp ));

    iEngine->SetQualityPreferenceL( (CThumbnailManager::TThumbnailQualityPreference)qp );
    return KErrNone;
    }

TInt CThumbnailManagerTest::CheckQualityPreference( CStifItemParser&
    aItem )
    {
    _LIT( KPanicTxt, "CheckQualityPreference" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt qp = 0;
    aItem.GetNextInt( qp );

    if ( iEngine->QualityPreference() != qp )
        {
        return KErrArgument;
        }
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::SetDisplayModeL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "SetDisplayModeL" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt displaymode = 0;
    User::LeaveIfError( aItem.GetNextInt( displaymode ));

    iEngine->SetDisplayModeL( (TDisplayMode)displaymode );
    return KErrNone;
    }

TInt CThumbnailManagerTest::CheckDisplayModeL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CheckDisplayModeL" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt displaymode = 0;
    User::LeaveIfError( aItem.GetNextInt( displaymode ));

    if ( iEngine->DisplayMode() != displaymode )
        {
        return KErrArgument;
        }
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::CreateSourceInstancePathL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CreateSrcPath" );
    __ASSERT_ALWAYS( !iObjectSource, User::Panic( KPanicTxt, 0 ));

    TPtrC path;
    User::LeaveIfError( aItem.GetNextString( path ));

    TInt id = 0;
    aItem.GetNextInt( id );
    
    TFileName filePath( iDataPath );
    filePath.Append( path );
    filePath.ZeroTerminate();
    
    iLog->Log( _L( "CreateSourceInstancePathL - path = %S" ), &filePath );
    
    delete iObjectSource;
    iObjectSource = NULL;
    
    iObjectSource = CThumbnailObjectSource::NewL( filePath, id );
    return KErrNone;
    }

TInt CThumbnailManagerTest::CreateSourceInstancePathTargetL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CreateSrcPathTarget" );
    __ASSERT_ALWAYS( !iObjectSource, User::Panic( KPanicTxt, 0 ));

    TPtrC path;
    User::LeaveIfError( aItem.GetNextString( path ));

    TPtrC target;
    User::LeaveIfError( aItem.GetNextString( target ));
    
    TPtrC mimeType;
    User::LeaveIfError( aItem.GetNextString( mimeType ));
    
    TInt virtual1 = 0;
    aItem.GetNextInt( virtual1 );
    
    TInt virtual2 = 0;
    aItem.GetNextInt( virtual2 );
    
    TFileName filePath( iDataPath );
    filePath.Append( path );
    filePath.ZeroTerminate();
    
    if (virtual1)
        {
        filePath.Delete(2,1);
        }
    
    TFileName targetPath( iDataPath );
    targetPath.Append( target );
    targetPath.ZeroTerminate();
    
    if (virtual2)
        {
        targetPath.Delete(2,1);
        }
    
    iLog->Log( _L( "CreateSourceInstancePathTargetL - path = %S, target = %S" ), &filePath );
    
    delete iObjectSource;
    iObjectSource = NULL;
    
    iObjectSource = CThumbnailObjectSource::NewL( filePath, targetPath, mimeType );
    return KErrNone;
    }

TInt CThumbnailManagerTest::CreateSourceInstanceHandleL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CreateSrcHandle" );
    __ASSERT_ALWAYS( !iObjectSource, User::Panic( KPanicTxt, 0 ));

    TPtrC path;
    User::LeaveIfError( aItem.GetNextString( path ));

    TFileName filePath( iDataPath );
    filePath.Append( path );
    filePath.ZeroTerminate();    
    
    User::LeaveIfError( iFile.Open( iFs, filePath, EFileShareReadersOrWriters ));
    iLog->Log( _L( "CreateSourceInstanceHandleL - path = %S" ), &filePath );
    
    delete iObjectSource;
    iObjectSource = NULL;
    iObjectSource = CThumbnailObjectSource::NewL( iFile );
    return KErrNone;
    }

TInt CThumbnailManagerTest::CreateSourceInstanceBufferL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CreateSrcBuffer" );
    __ASSERT_ALWAYS( !iObjectSource, User::Panic( KPanicTxt, 0 ));

    TPtrC path;
    User::LeaveIfError( aItem.GetNextString( path ));

    TPtrC mimeType;
    User::LeaveIfError( aItem.GetNextString( mimeType ));
    
    TFileName filePath( iDataPath );
    filePath.Append( path );
    filePath.ZeroTerminate();    
    
    User::LeaveIfError( iFile.Open( iFs, filePath, EFileShareReadersOrWriters ));
    iLog->Log( _L( "CreateSourceInstanceBufferL - path = %S" ), &filePath );
    
    TInt64 size;
    User::LeaveIfError( iFile.Size( size ) );
    iBuffer = HBufC8::NewL( size );
    TPtr8 ptr = iBuffer->Des();
    iFile.Read( ptr );
    iFile.Close();
    
    delete iObjectSource;
    iObjectSource = NULL;
    iObjectSource = CThumbnailObjectSource::NewL( iBuffer, mimeType, filePath );
    
    iBuffer = NULL;
    return KErrNone;
    }

TInt CThumbnailManagerTest::CreateSourceInstanceBitmapL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CreateSrcBuffer" );
    __ASSERT_ALWAYS( !iObjectSource, User::Panic( KPanicTxt, 0 ));

    TPtrC path;
    User::LeaveIfError( aItem.GetNextString( path ));
    
    TFileName filePath( iDataPath );
    filePath.Append( path );
    filePath.ZeroTerminate();    
    
    TPtrC target;
    User::LeaveIfError( aItem.GetNextString( target ));
    
    TFileName targetPath( iDataPath );
    targetPath.Append( target );
    targetPath.ZeroTerminate(); 
    
    iLog->Log( _L( "CreateSourceInstanceBitmapL - bitmap = %S" ), &filePath );
    iLog->Log( _L( "CreateSourceInstanceBitmapL - target = %S" ), &targetPath );
    
    iBitmap = new (ELeave) CFbsBitmap();
    User::LeaveIfError(iBitmap->Load(filePath, 0));

    delete iObjectSource;
    iObjectSource = NULL;
    iObjectSource = CThumbnailObjectSource::NewL( iBitmap, targetPath );
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::DeleteSourceInstance( CStifItemParser& /*aItem*/ )
    {
    delete iObjectSource;
    iObjectSource = NULL;
    return KErrNone;
    }

TInt CThumbnailManagerTest::GetThumbnailL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "GetThumbnailL" );
    __ASSERT_ALWAYS( iObjectSource, User::Panic( KPanicTxt, 0 ));
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 1 ));

    TInt clientData = NULL;
    TInt priority = 0;

    TInt err = aItem.GetNextInt( clientData );
    if ( !err )
        {
        aItem.GetNextInt( priority );
        }

    iPreviousRequestId = iEngine->GetThumbnailL( *iObjectSource, ( TAny* )
        clientData, priority );
    iLog->Log( _L( "GetThumbnailL - request id %d" ), iPreviousRequestId );
    
    if (!iDoNotCreate)
        {
        iCreateThumbnails = EFalse;
        }
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::GetThumbnailByIdL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "GetThumbnailByIdL" );
    __ASSERT_ALWAYS( iObjectSource, User::Panic( KPanicTxt, 0 ));
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 1 ));

    TInt id = 0;
    TInt clientData = NULL;
    TInt priority = 0;

    User::LeaveIfError( aItem.GetNextInt( id ));
    TInt err = aItem.GetNextInt( clientData );
    if ( !err )
        {
        aItem.GetNextInt( priority );
        }

    iPreviousRequestId = iEngine->GetThumbnailL( id, (TAny*)clientData, priority );
    iLog->Log( _L( "GetThumbnailByIdL - request id %d" ), iPreviousRequestId );
    
    if (!iDoNotCreate)
        {
        iCreateThumbnails = EFalse;
        }
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::SetThumbnailL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "SetThumbnailL" );
    __ASSERT_ALWAYS( iObjectSource, User::Panic( KPanicTxt, 0 ));
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 1 ));

    TInt clientData = NULL;
    TInt priority = 0;

    TInt err = aItem.GetNextInt( clientData );
    if ( !err )
        {
        aItem.GetNextInt( priority );
        }

    iPreviousRequestId = iEngine->SetThumbnailL( *iObjectSource, ( TAny* )
        clientData, priority );
    iLog->Log( _L( "SetThumbnailL - request id %d" ), iPreviousRequestId );
    
    if (!iDoNotCreate)
        {
        iCreateThumbnails = EFalse;
        }
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::ImportThumbnailL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "ImportThumbnailL" );
    __ASSERT_ALWAYS( iObjectSource, User::Panic( KPanicTxt, 0 ));
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 1 ));

    TInt clientData = NULL;
    TInt priority = 0;
    TPtrC path;
    
    User::LeaveIfError( aItem.GetNextString( path ));

    TFileName filePath( iDataPath );
    filePath.Append( path );
    filePath.ZeroTerminate();
    
    TInt err = aItem.GetNextInt( clientData );
    if ( !err )
        {
        aItem.GetNextInt( priority );
        }

    iPreviousRequestId = iEngine->ImportThumbnailL( *iObjectSource, filePath,
                                                    (TAny*)clientData, priority );
    iLog->Log( _L( "ImportThumbnailL - request id %d" ), iPreviousRequestId );
    
    if (!iDoNotCreate)
        {
        iCreateThumbnails = EFalse;
        }
    
    return KErrNone;
    }

void CThumbnailManagerTest::ThumbnailPreviewReady( MThumbnailData&
    aThumbnail, TThumbnailRequestId aId )
    {
    iLog->Log( _L( "ThumbnailPreviewReady aId=%d" ), aId );
    delete iPreviewThumbnail;
    iPreviewThumbnail = aThumbnail.DetachBitmap();
    iPreviewThumbnailCbId = aId;
    Signal( KErrNone );
    }

void CThumbnailManagerTest::ThumbnailReady( TInt aError, MThumbnailData&
    aThumbnail, TThumbnailRequestId aId )
    {
    iLog->Log( _L( "ThumbnailReady aError=%d aId=%d" ), aError, aId );
    delete iThumbnail;
    iThumbnail = aThumbnail.DetachBitmap();
    iThumbnailCbId = aId;
    
    // ignore KErrNotFound, if auto create disabled (CreateThumbnails used)
    if (aError == KErrNotFound && iCreateThumbnails)
        {
        for ( TInt i = 0 ; i < iPersistentSizes.Count(); i++ )
            {
            if ( iPersistentSizes[i].iType == iCurrentThumbnailSize &&
                 !iPersistentSizes[i].iAutoCreate )
                {
                aError = KErrNone;
                break;
                }
            }
        }
    
    Signal( aError );
    }

TInt CThumbnailManagerTest::CheckPreviewThumbnailL( CStifItemParser& aItem )
    {
    if ( iPreviewThumbnail )
        {
        TInt width = 0;
        TInt height = 0;
        TInt displaymode = 0;
        User::LeaveIfError( aItem.GetNextInt( width ));
        User::LeaveIfError( aItem.GetNextInt( height ));
        User::LeaveIfError( aItem.GetNextInt( displaymode ));
        TSize size = iPreviewThumbnail->SizeInPixels();
        iLog->Log( _L( 
            "CheckPreviewThumbnailL - bitmap size %dx%d (expecting %dx%d)" ),
            size.iWidth, size.iHeight, width, height );
        iLog->Log( _L( "CheckPreviewThumbnailL - bitmap mode %d (expecting %d)"
            ), iPreviewThumbnail->DisplayMode(), displaymode );
        if ( size != TSize( width, height ))
            {
            iLog->Log( _L( "CheckPreviewThumbnailL - size mismatch" ));
            User::Leave( KErrArgument );
            }
        else if ( displaymode != iPreviewThumbnail->DisplayMode())
            {
            iLog->Log( _L( "CheckPreviewThumbnailL - displaymode mismatch" ));
            User::Leave( KErrArgument );
            }
        else
            {
            iLog->Log( _L( "CheckPreviewThumbnailL - ok" ));
            }
        }
    else
        {
        // Bitmap missing
        iLog->Log( _L( "CheckPreviewThumbnailL - preview bitmap NULL" ));
        User::Leave( KErrUnknown );
        }
    return KErrNone;
    }

TInt CThumbnailManagerTest::CheckThumbnailL( CStifItemParser& aItem )
    {
    if ( iThumbnail )
        {
        TInt width = 0;
        TInt height = 0;
        TInt displaymode = 0;
        User::LeaveIfError( aItem.GetNextInt( width ));
        User::LeaveIfError( aItem.GetNextInt( height ));
        User::LeaveIfError( aItem.GetNextInt( displaymode ));
        TSize size = iThumbnail->SizeInPixels();
        iLog->Log( _L( "CheckThumbnailL - bitmap size %dx%d (expecting %dx%d)" )
            , size.iWidth, size.iHeight, width, height );
        iLog->Log( _L( "CheckThumbnailL - bitmap mode %d (expecting %d)" ),
            iThumbnail->DisplayMode(), displaymode );
        
        // one needs to match, aspect ratio scaling makes this difficult...
        if ( size.iWidth != width && size.iHeight != height )
            {
            iLog->Log( _L( "CheckThumbnailL - size mismatch" ));
            User::Leave( KErrArgument );
            }
        else if ( displaymode != iThumbnail->DisplayMode())
            {
            iLog->Log( _L( "CheckThumbnailL - displaymode mismatch" ));
            }
        else
            {
            iLog->Log( _L( "CheckThumbnailL - ok" ));
            }
        }
    else
        {
        // Bitmap missing
        iLog->Log( _L( "CheckThumbnailL - bitmap NULL" ));
        User::Leave( KErrNotFound );
        }
    return KErrNone;
    }

TInt CThumbnailManagerTest::CheckThumbnailCenrepL( CStifItemParser& aItem )
    {
    TInt err = KErrNone;
    TBool full = EFalse;
    
    TInt sizeType;
    TInt displaymode = 0;
    User::LeaveIfError( aItem.GetNextInt( sizeType ));
    User::LeaveIfError( aItem.GetNextInt( displaymode ));
    
    TThumbnailSize size = (TThumbnailSize)sizeType;
    if (size == EImageFullScreenThumbnailSize ||
        size == EVideoFullScreenThumbnailSize ||
        size == EAudioFullScreenThumbnailSize)
        {
        full = ETrue;
        }
    
    if ( iThumbnail )
        {
        TInt width = 0;
        TInt height = 0;
        
        for ( TInt i = 0 ; i < iPersistentSizes.Count(); i++ )
            {
            if ( iPersistentSizes[i].iType == size )
                {
                width = iPersistentSizes[i].iSize.iWidth;
                height = iPersistentSizes[i].iSize.iHeight;
                break;
                }
            }
        
        TSize thumbSize = iThumbnail->SizeInPixels();
        
        iLog->Log( _L( "CheckThumbnailCenrepL - bitmap size %dx%d (expecting %dx%d)" )
            , thumbSize.iWidth, thumbSize.iHeight, width, height );
        iLog->Log( _L( "CheckThumbnailCenrepL - bitmap mode %d (expecting %d)" ),
            iThumbnail->DisplayMode(), displaymode );
        
        // one needs to match, and both need to be smaller than cenrep value
        if ( (thumbSize.iWidth == width || thumbSize.iHeight == height) &&
             thumbSize.iWidth <= width && thumbSize.iHeight <= height)
            {
            iLog->Log( _L( "CheckThumbnailCenrepL - ok" ));
            }
        else if (full && (thumbSize.iWidth <= width && thumbSize.iHeight <= height))
            {
            iLog->Log( _L( "CheckThumbnailCenrepL - fullscreen ok, not upscaled" ));
            }
        else
            {
            iLog->Log( _L( "CheckThumbnailCenrepL - size mismatch" ));
            err = KErrArgument;
            }
        
        // check display mode
        if ( displaymode != iThumbnail->DisplayMode())
            {
            iLog->Log( _L( "CheckThumbnailCenrepL - displaymode mismatch" ));
            err = KErrArgument;
            }
        }
    else
        {
        for ( TInt i = 0 ; i < iPersistentSizes.Count(); i++ )
            {
            if ( iPersistentSizes[i].iType == size && iPersistentSizes[i].iAutoCreate)
                {
                // Bitmap missing
                iLog->Log( _L( "CheckThumbnailCenrepL - bitmap NULL" ));
                err = KErrNotFound;
                break;
                }
            }
        }
    
    return err;
    }

TInt CThumbnailManagerTest::CancelPrevious( CStifItemParser&  /*aItem*/ )
    {
    _LIT( KPanicTxt, "CancelPrevious" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt err = iEngine->CancelRequest( iPreviousRequestId );
    iLog->Log( _L( "CancelPrevious() returned %d" ), err );
    return err;
    }

TInt CThumbnailManagerTest::ChangePreviousPriority( CStifItemParser&
    aItem )
    {
    _LIT( KPanicTxt, "ChangePreviousPriority" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt priority = 0;
    aItem.GetNextInt( priority );

    TInt err = iEngine->ChangePriority( iPreviousRequestId, priority );
    iLog->Log( _L( "ChangePriority() returned %d" ), err );
    return err;
    }


TInt CThumbnailManagerTest::DeleteThumbnails( CStifItemParser&  /*aItem*/ )
    {
    iEngine->DeleteThumbnails( *iObjectSource );
    return KErrNone;
    }

TInt CThumbnailManagerTest::CreateThumbnailsByUrlL( CStifItemParser&  aItem )
    {
    _LIT( KPanicTxt, "CreateSrcPath" );
    __ASSERT_ALWAYS( !iObjectSource, User::Panic( KPanicTxt, 0 ));

    TPtrC path;
    User::LeaveIfError( aItem.GetNextString( path ));

    TInt id = 0;
    aItem.GetNextInt( id );
    
    TFileName filePath( iDataPath );
    filePath.Append( path );
    filePath.ZeroTerminate();
    
    iLog->Log( _L( "CreateSourceInstancePathL - path = %S" ), &filePath );
    
    delete iObjectSource;
    iObjectSource = NULL;
    
    iObjectSource = CThumbnailObjectSource::NewL( filePath, id );
    
    id = iEngine->CreateThumbnails( *iObjectSource );
    if (id < 0)
        {
        iLog->Log( _L( "CreateThumbnails - error %d" ), id );
        return id;  
        }
    else
        {
        iPreviousRequestId = id;
        iLog->Log( _L( "CreateThumbnails - request id %d" ), iPreviousRequestId );
        }
    
    iCreateThumbnails = ETrue;
    
    return KErrNone;
    }
    
    
TInt CThumbnailManagerTest::CreateThumbnails( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "CreateThumbnails" );
    __ASSERT_ALWAYS( iObjectSource, User::Panic( KPanicTxt, 0 ));
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 1 ));

    TInt priority = 0;

    aItem.GetNextInt( priority );

    TInt id;
    id = iEngine->CreateThumbnails( *iObjectSource, priority );
    if (id < 0)
        {
        iLog->Log( _L( "CreateThumbnails - error %d" ), id );
        return id;  
        }
    else
        {
        iPreviousRequestId = id;
        iLog->Log( _L( "CreateThumbnails - request id %d" ), iPreviousRequestId );
        }
    
    iCreateThumbnails = ETrue;
    
    return KErrNone;
    }

TInt CThumbnailManagerTest::UpdatePathL( CStifItemParser& aItem )
    {
    _LIT( KPanicTxt, "UpdatePath" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    TInt id = 0;
    TPtrC path;
    User::LeaveIfError( aItem.GetNextInt( id ));
    User::LeaveIfError( aItem.GetNextString( path ));
    
    TFileName filePath( iDataPath );
    filePath.Append( path );
    filePath.ZeroTerminate();
    
    iLog->Log( _L( "UpdatePath - path = %S" ), &filePath );

    iEngine->UpdateThumbnailsL(id, filePath, 0, 1000, CActive::EPriorityIdle);

    return KErrNone;
    }

TInt CThumbnailManagerTest::GetSupportedMimeTypesL( CStifItemParser& /*aItem*/ )
    {
    _LIT( KPanicTxt, "GetSupportedMimeTypesL" );
    __ASSERT_ALWAYS( iEngine, User::Panic( KPanicTxt, 0 ));

    iEngine->GetSupportedMimeTypesL();

    return KErrNone;
    }


// ========================== OTHER EXPORTED FUNCTIONS =========================
// None

//  [End of File] - Do not remove
