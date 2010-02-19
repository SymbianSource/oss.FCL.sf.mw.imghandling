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
* Description:  Store for thumbnails.
 *
*/


#include <s32mem.h>
#include <e32cmn.h>
#include <fbs.h>
#include <imageconversion.h>
#include <e32base.h>
#include <exifread.h>

#include <iclextjpegapi.h>
#include "thumbnailstore.h"
#include "thumbnailsql.h"
#include "thumbnaillog.h"
#include "thumbnailmanageruids.hrh"
#include "thumbnailcenrep.h"
#include "thumbnailpanic.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnailserver.h"



_LIT8( KThumbnailSqlConfig, "page_size=16384; cache_size=32;" );

const TInt KStreamBufferSize = 1024 * 8;
const TInt KMajor = 3;
const TInt KMinor = 2;

// Database path without drive letter
_LIT( KThumbnailDatabaseName, ":[102830AB]thumbnail_v2.db" );

// Allow access to database only for the server process
const TSecurityPolicy KThumbnailDatabaseSecurityPolicy( TSecureId(
    THUMBNAIL_MANAGER_SERVER_UID ));


// ---------------------------------------------------------------------------
// RThumbnailTransaction::RThumbnailTransaction::TThumbnailPersistentSize
// ---------------------------------------------------------------------------
//
RThumbnailTransaction::RThumbnailTransaction( RSqlDatabase& aDatabase ):
    iDatabase( aDatabase ), iState( EClosed )
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// RThumbnailTransaction::BeginL()
// ---------------------------------------------------------------------------
//
void RThumbnailTransaction::BeginL()
    {
    const TInt err = iDatabase.Exec( KThumbnailBeginTransaction );
    if ( err >= 0 )
        {
        iState = EOpen;
        }
    else
        {
        iState = EError;
        User::Leave( err );
        }
    }


// ---------------------------------------------------------------------------
// RThumbnailTransaction::Close()
// ---------------------------------------------------------------------------
//
void RThumbnailTransaction::Close()
    {
    if ( iState != EClosed )
        {
        Rollback();
        }
    }


// ---------------------------------------------------------------------------
// RThumbnailTransaction::CommitL()
// ---------------------------------------------------------------------------
//
void RThumbnailTransaction::CommitL()
    {
    User::LeaveIfError( iDatabase.Exec( KThumbnailCommitTransaction ));
    iState = EClosed;
    }


// ---------------------------------------------------------------------------
// RThumbnailTransaction::Rollback()
// ---------------------------------------------------------------------------
//
TInt RThumbnailTransaction::Rollback()
    {
    const TInt err = iDatabase.Exec( KThumbnailRollbackTransaction );
    if ( err >= 0 )
        {
        iState = EClosed;
        }
    return err;
    }

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailStore::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailStore* CThumbnailStore::NewL( RFs& aFs, TInt aDrive, TDesC& aImei, CThumbnailServer* aServer )
    {
    CThumbnailStore* self = new( ELeave )CThumbnailStore( aFs, aDrive, aImei, aServer );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::~CThumbnailStore()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailStore::~CThumbnailStore()
    {
    TN_DEBUG1( "CThumbnailStore::~CThumbnailStore()" );
    
    delete iDiskFullNotifier;
    iDiskFullNotifier = NULL; 

    if(!iServer->IsFormatting())
        {
 	    FlushCacheTable( ETrue );
        }
    if( iAutoFlushTimer )
        {
        iAutoFlushTimer->Cancel();
        delete iAutoFlushTimer;
        iAutoFlushTimer = NULL;
        }
    
    iDatabase.Close();
    TN_DEBUG1( "CThumbnailStore::~CThumbnailStore() - database closed" );
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::CThumbnailStore()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailStore::CThumbnailStore( RFs& aFs, TInt aDrive, TDesC& aImei,  CThumbnailServer* aServer ): 
    iFs( aFs ), iDrive( aDrive ), iBatchItemCount(0), iImei(aImei), iServer(aServer), iDiskFull(EFalse)
    {
    // no implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailStore::ConstructL()
    {
    TN_DEBUG1( "CThumbnailStore::ConstructL()" );

#ifdef _DEBUG
    iThumbCounter = 0;
#endif
    
    HBufC* databasePath = HBufC::NewLC( KMaxFileName );
    TPtr pathPtr = databasePath->Des();
    TChar driveChar = 0;
    User::LeaveIfError( RFs::DriveToChar( iDrive, driveChar ));
    pathPtr.Append( driveChar );
    pathPtr.Append( KThumbnailDatabaseName );
    
	//start disk space monitor
    iDiskFullNotifier = CThumbnailStoreDiskSpaceNotifierAO::NewL( *this, 
                                            KDiskFullThreshold,
                                            pathPtr );

    TVolumeInfo volumeinfo;
    iFs.Volume(volumeinfo, iDrive);
    TUint id = volumeinfo.iUniqueID;
    TBuf<50> mediaid;
    mediaid.Num(id);
    TBool newDatabase(EFalse);
    
    TInt error = KErrNone;
    
    TInt err = iDatabase.Open( pathPtr );
    if ( err == KErrNotFound )
        {
        // db not found, create new
        TN_DEBUG1( "CThumbnailStore::ConstructL() -- 1 creating database" );
        const TDesC8& config = KThumbnailSqlConfig;

        RSqlSecurityPolicy securityPolicy;
        CleanupClosePushL( securityPolicy );
        securityPolicy.Create( KThumbnailDatabaseSecurityPolicy );

        iDatabase.CreateL( pathPtr, securityPolicy, &config );
        CleanupStack::PopAndDestroy( &securityPolicy );

        TN_DEBUG1( "CThumbnailStore::ConstructL() -- 1 database created ok" );
                
        RFile64 file;
        file.Create(iFs, mediaid, EFileShareReadersOrWriters );
        file.Close();
        newDatabase = ETrue;
        }
    else if ( err == KErrNone)
        {
        // db found, check version and rowids
        error = CheckVersionL();
        if(error == KErrNone)
            {
            error = CheckRowIDsL();
            }  
        }
    
    // if wrong version, corrupted database or other error opening db
    if ( error == KErrNotSupported || (err != KErrNone && err != KErrNotFound) )
        {
        TN_DEBUG1( "CThumbnailStore::ConstructL() -- delete databases" );
        
        // delete db and create new
        iDatabase.Close();
        iDatabase.Delete(pathPtr);
        
        TN_DEBUG1( "CThumbnailStore::ConstructL() -- 2 creating database" );
        
        const TDesC8& config = KThumbnailSqlConfig;

        RSqlSecurityPolicy securityPolicy;
        CleanupClosePushL( securityPolicy );
        securityPolicy.Create( KThumbnailDatabaseSecurityPolicy );

        iDatabase.CreateL( pathPtr, securityPolicy, &config );
        CleanupStack::PopAndDestroy( &securityPolicy );

        TN_DEBUG1( "CThumbnailStore::ConstructL() -- 2 database created ok" );
        
        RFile64 file;
        file.Create(iFs, mediaid, EFileShareReadersOrWriters );
        file.Close();
        }
    else if(!newDatabase)
        {
        //check ownership
        if(CheckImeiL() != KErrNone)
            {
            ResetThumbnailIDs();
            
            //take ownership
            UpdateImeiL();
            
            //Touch blacklisted items
            TRAP_IGNORE( PrepareBlacklistedItemsForRetry( ) );
            }
        
        //check is MMC known
        if(CheckMediaIDL() != KErrNone )
            {
            ResetThumbnailIDs();
            
            //Touch blacklisted items
            TRAP_IGNORE( PrepareBlacklistedItemsForRetry() );
            }
        }
              
    CleanupStack::PopAndDestroy( databasePath );
    
    // add tables
    TRAPD(tableError, CreateTablesL() );
    
    if(!tableError)
        {
        AddVersionAndImeiL();
        }
    
    err = iDatabase.Exec( KThumbnailCreateTempInfoTable );
    TN_DEBUG2("CThumbnailStore::CreateTablesL() KThumbnailCreateTempInfoTable %d", err);
    User::LeaveIfError( err );
    err = iDatabase.Exec( KThumbnailCreateTempInfoDataTable );
    TN_DEBUG2("CThumbnailStore::CreateTablesL() KThumbnailCreateTempInfoDataTable %d", err);
    User::LeaveIfError( err );
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::StoreThumbnailL()
// Stores thumbnail image.
// ---------------------------------------------------------------------------
//
void CThumbnailStore::StoreThumbnailL( const TDesC& aPath, const TDes8& aData,
    const TSize& aSize, const TSize& aOriginalSize, const TThumbnailFormat& aFormat, TInt aFlags, 
	const TThumbnailSize& aThumbnailSize, TThumbnailId aThumbnailId, const TBool aThumbFromPath )
    {
    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( const TDes8& ) in" );

#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif

    //Encapsulate insert to Transaction
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );
    transaction.BeginL();
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    // Insert into ThumbnailInfo
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailInsertThumbnailInfoByPathAndId ));

    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamWidth );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aSize.iWidth ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamHeight );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aSize.iHeight ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamOriginalWidth );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aOriginalSize.iWidth ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamOriginalHeight );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aOriginalSize.iHeight ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamFormat );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aFormat ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamFlags );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aFlags ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSize );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailSize ));
    
    if( aThumbnailId > 0 )
        {
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
        }
    else
        {
        TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( ) aThumbnailId == 0" );
        }
    
    // orientation temporarily to 0
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamOrientation );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, 0 ));
    
    // thumb from associated path
    TInt fromPath = aThumbFromPath;
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamThumbFromPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, fromPath ));
    
    // try getting modification time from file
    TTime timeStamp;
    
    if (aPath.Length())
        {
        iFs.Modified(aPath, timeStamp);
        }
    else
        {
        // otherwise current time
        timeStamp.UniversalTime();
        }
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamModified );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt64( paramIndex, timeStamp.Int64() ));
    
    User::LeaveIfError( stmt.Exec());
    CleanupStack::PopAndDestroy( &stmt );
    
    RSqlStatement stmtData;
    CleanupClosePushL( stmtData );
    // Insert into ThumbnailInfoData
    TInt err = stmtData.Prepare( iDatabase, KThumbnailInsertTempThumbnailInfoData );
       
#ifdef _DEBUG
    TPtrC errorMsg = iDatabase.LastErrorMessage();
    TN_DEBUG2( "CThumbnailStore::FetchThumbnailL() KThumbnailInsertTempThumbnailInfoData %S" , &errorMsg);
#endif    
    User::LeaveIfError( err );
    
    paramIndex = stmtData.ParameterIndex( KThumbnailSqlParamData );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmtData.BindBinary( paramIndex, aData ));

    User::LeaveIfError( stmtData.Exec());
    CleanupStack::PopAndDestroy( &stmtData );
	
    // Commit transaction
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );

    iBatchItemCount++;
    
    FlushCacheTable();

#ifdef _DEBUG
    iThumbCounter++;
    TN_DEBUG2( "CThumbnailStore::THUMBSTORE-COUNTER----------, Thumbs = %d", iThumbCounter );
    
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::StoreThumbnailL() insert to table %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif 
    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( const TDes8& ) out" );
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::StoreThumbnailL()
// Stores thumbnail image.
// ---------------------------------------------------------------------------
//
void CThumbnailStore::StoreThumbnailL( const TDesC& aPath, CFbsBitmap*
    aThumbnail, const TSize& aOriginalSize, TBool /*aCropped*/, const TThumbnailSize aThumbnailSize, 
    const TThumbnailId aThumbnailId, const TBool aThumbFromPath, TBool aBlackListed )
    {
    TSize thumbSize = aThumbnail->SizeInPixels();
    TN_DEBUG5( "CThumbnailStore::StoreThumbnailL( CFbsBitmap ) aThumbnailId = %d, aThumbnailSize = %d, aThumbnailSize(%d,%d) IN", aThumbnailId, aThumbnailSize, thumbSize.iWidth, thumbSize.iHeight );

    __ASSERT_DEBUG(( aThumbnail ), ThumbnailPanic( EThumbnailNullPointer ));
    
    // don't store custom/unknown sizes or zero sizes
    if(aThumbnailSize == ECustomThumbnailSize || aThumbnailSize == EUnknownThumbnailSize 
            || thumbSize.iWidth <= 0 || thumbSize.iHeight <= 0 )
        {
        TN_DEBUG1( "CThumbnailStore::StoreThumbnailL() not stored");
        return;
        }

    // check for duplicates
    TBool exists = FindDuplicateL(aPath, aThumbnailId, aThumbnailSize);
    
    for ( TInt i = iPersistentSizes.Count(); --i >= 0; )
        {
        TThumbnailPersistentSize & persistentSize = iPersistentSizes[i];
        
        // don't store duplicates or zero sizes
        if ( !exists )
            {
            TInt flags = 0;
            if ( persistentSize.iCrop )
                {
                flags |= KThumbnailDbFlagCropped;
                }
            
            if( aBlackListed )
                {
                flags |= KThumbnailDbFlagBlacklisted;
                }
            
            if( (aThumbnailSize == EImageFullScreenThumbnailSize || aThumbnailSize == EVideoFullScreenThumbnailSize ||
                 aThumbnailSize == EAudioFullScreenThumbnailSize) && !aBlackListed )
                {
                HBufC8* data = NULL;
                CImageEncoder* iEncoder = CImageEncoder::DataNewL( data,  KJpegMime(), CImageEncoder::EOptionAlwaysThread );
                TJpegImageData* imageData = new (ELeave) TJpegImageData();
             
                // Set some format specific data
                imageData->iSampleScheme = TJpegImageData::EColor444;
                imageData->iQualityFactor = 75; //?
             
                CFrameImageData* iFrameImageData = CFrameImageData::NewL();
             
                // frameData - ownership passed to iFrameImageData after AppendImageData
                User::LeaveIfError(iFrameImageData->AppendImageData(imageData));
                
#ifdef _DEBUG
        TN_DEBUG4( "CThumbnailStore::StoreThumbnailL() size %d x %d displaymode %d ", 
                aThumbnail->SizeInPixels().iWidth, 
                aThumbnail->SizeInPixels().iHeight, 
                aThumbnail->DisplayMode());
#endif
                
                TRequestStatus request;
                iEncoder->Convert( &request, *aThumbnail, iFrameImageData);
                User::WaitForRequest( request);
                  
                if(request== KErrNone)
                  {
                  TPtr8 ptr  = data->Des(); 
                  StoreThumbnailL( aPath, ptr, aThumbnail->SizeInPixels(), aOriginalSize,
                          EThumbnailFormatJpeg, flags, aThumbnailSize, aThumbnailId, aThumbFromPath  );
                  }
             
                delete iFrameImageData;
                iFrameImageData = NULL;          
                
                delete iEncoder;
                iEncoder = NULL;    
                delete data;
                data = NULL;                
                }
            else
                {
                CBufFlat* buf = CBufFlat::NewL( KStreamBufferSize );
                CleanupStack::PushL( buf );
                RBufWriteStream stream;
                stream.Open( *buf );
                aThumbnail->ExternalizeL( stream );
            
                StoreThumbnailL( aPath, buf->Ptr( 0 ), aThumbnail->SizeInPixels(),
                    aOriginalSize, EThumbnailFormatFbsBitmap, flags, aThumbnailSize, aThumbnailId );
  
                CleanupStack::PopAndDestroy( buf );
                }
            
            break;
            }
        }
    
    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( CFbsBitmap* ) out" );
    }


// ---------------------------------------------------------------------------
// Finds possible existing duplicate thumbnail.
// ---------------------------------------------------------------------------
//
TBool CThumbnailStore::FindDuplicateL( const TDesC& aPath, const TThumbnailId aThumbnailId,
                                       const TThumbnailSize& aThumbnailSize )
    {
    TN_DEBUG1( "CThumbnailStore::FindDuplicateL()" );
    
    TInt rowStatus = 0;
    TInt paramIndex = 0;
    TInt found = EFalse;
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );     
    
    User::LeaveIfError( stmt.Prepare( iDatabase, KTempFindDuplicate ));
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
        
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSize );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailSize ));
    
    rowStatus = stmt.Next();
    
    //if not found from temp table, look from real table
    if(rowStatus != KSqlAtRow)
        {
        stmt.Close();
        CleanupStack::PopAndDestroy( &stmt );
        CleanupClosePushL( stmt );
           
        User::LeaveIfError( stmt.Prepare( iDatabase, KFindDuplicate ));
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
            
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSize );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailSize ));
        
        rowStatus = stmt.Next();
        
        if(rowStatus == KSqlAtRow)
            {
            TN_DEBUG1( "CThumbnailStore::FindDuplicateL() - duplicate in main table" );
            
            found = ETrue;
            }
        else
            {
            TN_DEBUG1( "CThumbnailStore::FindDuplicateL() - no duplicate" );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbnailStore::FindDuplicateL() - duplicate in temp table" );
        
        found = ETrue;
        }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    
    return found;
    }


// ---------------------------------------------------------------------------
// Create database tables
// ---------------------------------------------------------------------------
//
void CThumbnailStore::CreateTablesL()
    {
    TN_DEBUG1( "CThumbnailStore::CreateTablesL()" );
    TInt err = 0;
    err = iDatabase.Exec( KThumbnailCreateInfoTable );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateInfoTable err=%d", err );
    err = iDatabase.Exec( KThumbnailCreateInfoDataTable );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateInfoDataTable err=%d", err );
    err = iDatabase.Exec( KThumbnailCreateInfoTableIndex1 );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateInfoTableIndex1 err=%d", err );
    err = iDatabase.Exec( KThumbnailCreateInfoTableIndex2 );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateInfoTableIndex2 err=%d", err );
    err = iDatabase.Exec(KThumbnailVersionTable);
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailVersionTable err=%d", err );
    User::LeaveIfError( err );
    }


// ---------------------------------------------------------------------------
// Get missing sizes by Path
// ---------------------------------------------------------------------------
//     
void CThumbnailStore::GetMissingSizesAndIDsL( const TDesC& aPath, TInt aSourceType, RArray <
    TThumbnailPersistentSize > & aMissingSizes, TBool& aMissingIDs )
    {
    TN_DEBUG2( "CThumbnailStore::GetMissingSizesAndIDsL() aSourceType == %d", aSourceType );
    // define sizes to be checked
    const TInt count = iPersistentSizes.Count();
    
    for ( TInt i = 0 ; i < count; i++ )
        {
        if ( iPersistentSizes[ i ].iSourceType == aSourceType && iPersistentSizes[ i ].iAutoCreate)
            {
            aMissingSizes.Append( iPersistentSizes[ i ] );
            }
        }
    
    TInt missingSizeCount = aMissingSizes.Count();
    aMissingIDs = EFalse;
        
    TN_DEBUG3( "CThumbnailStore::GetMissingSizesAndIDsL() missingSizeCount == %d, missingIDs == %d", missingSizeCount, aMissingIDs );
    
    // check temp table first
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempSizeByPath ));
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
       
    TInt rowStatus = stmt.Next();

    TInt round = 1;
    TInt size = 0;
    TInt id = 0;
    
    while (round <= 2)
        {
        while ( rowStatus == KSqlAtRow && missingSizeCount > 0 )
            {
            size = stmt.ColumnInt( 0 );
            id = stmt.ColumnInt( 1 );
			
			TN_DEBUG2( "CThumbnailStore::GetMissingSizesAndIDsL() id == %d", id );
            
			//if TNId is not valid mark that some are missing so that UpdateDb is run later
            if ( id <= 0)
                {
                TN_DEBUG1( "CThumbnailStore::GetMissingSizesAndIDsL() missing ID");
                aMissingIDs = ETrue;
                }
            
            missingSizeCount = aMissingSizes.Count();
            for ( TInt i = 0; i < missingSizeCount; i++ )
                {
                if ( aMissingSizes[ i ].iType == size )
                    {
                    TN_DEBUG1( "CThumbnailStore::GetMissingSizesL() -- thumbnail found" );
                    aMissingSizes.Remove( i );
                    missingSizeCount--;
                    break;
                    }
                }
                
            rowStatus = stmt.Next();
            }
        stmt.Close();
        CleanupStack::PopAndDestroy( &stmt );
        
        // all found
        if (missingSizeCount == 0)
            {
            return;
            }
        else if (round == 1)
            {
            // change to real table
            CleanupClosePushL( stmt );
            User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectSizeByPath ));
            paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
            rowStatus = stmt.Next();    
            }
        
        round++;
        }
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::FetchThumbnailL()
// Fetches thumbnail image.
// ---------------------------------------------------------------------------
//
TInt CThumbnailStore::FetchThumbnailL( TThumbnailId aThumbnailId, 
        CFbsBitmap*& aThumbnail, TDesC8* & aData, TThumbnailSize aThumbnailSize, TSize &aThumbnailRealSize )
    {
    TN_DEBUG3( "CThumbnailStore::FetchThumbnailL(%d) aThumbnailSize == %d", aThumbnailId, aThumbnailSize );
     delete aThumbnail;
     aThumbnail = NULL;

     RSqlStatement stmt;
     CleanupClosePushL( stmt );
     
     TInt paramIndex = 0;
     TInt found = KErrNotFound;
     TInt rowStatus = 0;
     TInt column = 0;
     TInt count = 0;
     TThumbnailSize thumbnailImage = EUnknownThumbnailSize;
     TThumbnailSize thumbnailVideo = EUnknownThumbnailSize;
     TThumbnailSize thumbnailAudio = EUnknownThumbnailSize;
     TBool inTempTable( ETrue );
     
     if(aThumbnailSize == EFullScreenThumbnailSize)
         {
         thumbnailImage = EImageFullScreenThumbnailSize;
         thumbnailVideo = EVideoFullScreenThumbnailSize;
         thumbnailAudio = EAudioFullScreenThumbnailSize;
         }
     else if(aThumbnailSize == EGridThumbnailSize)
         {
         thumbnailImage = EImageGridThumbnailSize;
         thumbnailVideo = EVideoGridThumbnailSize;
         thumbnailAudio = EAudioGridThumbnailSize;
         }
     else if(aThumbnailSize == EListThumbnailSize)
         {
         thumbnailImage = EImageListThumbnailSize;
         thumbnailVideo = EVideoListThumbnailSize;
         thumbnailAudio = EAudioListThumbnailSize;
         }
     
     if(aThumbnailSize == EFullScreenThumbnailSize ||
        aThumbnailSize == EGridThumbnailSize ||
        aThumbnailSize == EListThumbnailSize )
         {
         TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- No DataType -- TEMP TABLE lookup" );
         User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempInfoByIdv2 ));
         paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
         User::LeaveIfError( paramIndex );
         User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
             
         paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSizeImage );
         User::LeaveIfError( paramIndex );
         User::LeaveIfError( stmt.BindInt( paramIndex, thumbnailImage ));
         
         paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSizeVideo );
         User::LeaveIfError( paramIndex );
         User::LeaveIfError( stmt.BindInt( paramIndex, thumbnailVideo ));
         
         paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSizeAudio );
         User::LeaveIfError( paramIndex );
         User::LeaveIfError( stmt.BindInt( paramIndex, thumbnailAudio ));
             
         rowStatus = stmt.Next();
         //if not found from temp table, look from real table
         if(rowStatus != KSqlAtRow)
           {
           TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- No DataType -- MAIN TABLE lookup" );
           inTempTable = EFalse;
           stmt.Close();
           CleanupStack::PopAndDestroy( &stmt );
           CleanupClosePushL( stmt );
                
           User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectInfoByIdv2 ));
             
           paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
           User::LeaveIfError( paramIndex );
           User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
                 
           paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSizeImage );
           User::LeaveIfError( paramIndex );
           User::LeaveIfError( stmt.BindInt( paramIndex, thumbnailImage ));
                    
           paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSizeVideo );
           User::LeaveIfError( paramIndex );
           User::LeaveIfError( stmt.BindInt( paramIndex, thumbnailVideo ));
                    
           paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSizeAudio );
           User::LeaveIfError( paramIndex );
           User::LeaveIfError( stmt.BindInt( paramIndex, thumbnailAudio ));
             
           rowStatus = stmt.Next();
           }               
         }
     else
         {
         TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- TEMP TABLE lookup" );
         User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempInfoById ));
         paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
         User::LeaveIfError( paramIndex );
         User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
     
         paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSize );
         User::LeaveIfError( paramIndex );
         User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailSize ));
     
         rowStatus = stmt.Next();

         //if not found from temp table, look from real table
         if(rowStatus != KSqlAtRow)
            {
            TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- MAIN TABLE lookup" );
            inTempTable = EFalse;
            stmt.Close();
            CleanupStack::PopAndDestroy( &stmt );
            CleanupClosePushL( stmt );
        
            User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectInfoById ));
     
            paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
         
            paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSize );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailSize ));
     
            rowStatus = stmt.Next();
            }
         }
     if(rowStatus == KSqlAtRow)
        {
         TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- thumbnail found" );
         // Check whether blacklisted thumbnail entry modified. 
         // If thumbnail is marked as blacklisted and timestamp has 
         // changed, delete thumbnails from tables and leave with 
         // KErrNotFound to get thumbnail regenerated.
         column = 4;
         TInt flags = stmt.ColumnInt( column );
         if( flags & KThumbnailDbFlagBlacklisted )
             {
             TBool modified = EFalse;
             CheckModifiedByIdL( aThumbnailId, inTempTable, modified );
             if( modified )
                 {
                 // Close db to get deletion of thumbnails executed.
                 stmt.Close();
                 CleanupStack::PopAndDestroy( &stmt );
                 DeleteThumbnailsL( aThumbnailId );
                 User::Leave( KErrNotFound );
                 }
             else
                 {
                 User::Leave( KErrCompletion );
                 }
             }
         
         found = KErrNone;
         count = 0;
         count++;
         column = 0;
         TInt format = stmt.ColumnInt( column++ );  
         if(format == 1 /*TThumbnailFormat::EThumbnailFormatJpeg */ )
             {
             TPtrC8 ptr = stmt.ColumnBinaryL( column++ );
             HBufC8* data = ptr.AllocL() ;
             aThumbnail = NULL;
             aData = data;     
             
          } else {
             TPtrC8 ptr = stmt.ColumnBinaryL( column );
             RDesReadStream stream( ptr );
             aThumbnail = new( ELeave )CFbsBitmap();
             aThumbnail->InternalizeL( stream );
             aData = NULL;
          }
         
        //fetch real size of TN
        column = 2;
        aThumbnailRealSize.iWidth = stmt.ColumnInt( column++ );
        aThumbnailRealSize.iHeight = stmt.ColumnInt( column );
        }
     else
         {
         TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- thumbnail NOT found" );
         }
         
     stmt.Close();
     CleanupStack::PopAndDestroy( &stmt );

     User::LeaveIfError( found );
     return found;
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::FetchThumbnailL()
// Fetches thumbnail image.
// ---------------------------------------------------------------------------
//
void CThumbnailStore::FetchThumbnailL( const TDesC& aPath, CFbsBitmap* &
    aThumbnail, TDesC8* & aData, const TThumbnailSize aThumbnailSize, TSize &aThumbnailRealSize )
    {
    TN_DEBUG3( "CThumbnailStore::FetchThumbnailL(%S) aThumbnailSize==%d", &aPath, aThumbnailSize );
    delete aThumbnail;
    aThumbnail = NULL;

    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    
    TInt paramIndex = 0;
    TInt found = KErrNotFound;
    TInt rowStatus = 0;
    TInt column = 0;
    TBool inTempTable = ETrue;
    
    TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- TEMP TABLE lookup" );
    TInt err = stmt.Prepare( iDatabase, KThumbnailSelectTempInfoByPath );

#ifdef _DEBUG
    TPtrC errorMsg = iDatabase.LastErrorMessage();
    TN_DEBUG2( "CThumbnailStore::FetchThumbnailL() %S" , &errorMsg);
#endif
    User::LeaveIfError( err );
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSize );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailSize ));
    
    rowStatus = stmt.Next();

    //if not found from temp table, look from real table
    if(rowStatus != KSqlAtRow)
       {
       TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- MAIN TABLE lookup" );
       inTempTable = EFalse;
       stmt.Close();
       CleanupStack::PopAndDestroy( &stmt );
       CleanupClosePushL( stmt );
       
       User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectInfoByPath ));
    
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSize );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailSize ));
    
        rowStatus = stmt.Next();
       }

    if(rowStatus == KSqlAtRow)
       {
        TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- thumbnail found" );
        // Check whether blacklisted thumbnail entry modified. 
        // If thumbnail is marked as blacklisted and timestamp has 
        // changed, delete thumbnails from tables and leave with 
        // KErrNotFound to get thumbnail regenerated.
        column = 4;
        TInt flags = stmt.ColumnInt( column );
        if( flags & KThumbnailDbFlagBlacklisted && aPath.Length() )
            {
            TBool modified = EFalse;
            CheckModifiedByPathL( aPath, inTempTable, modified );
            if( modified )
                {
                // Close db to get deletion of thumbnails executed.
                stmt.Close();
                CleanupStack::PopAndDestroy( &stmt );
                DeleteThumbnailsL( aPath );
                User::Leave( KErrNotFound );
                }
            else
                {
                User::Leave( KErrCompletion );
                }
            }
        else if( !(flags & KThumbnailDbFlagBlacklisted) )
            {
            found = KErrNone;
            column = 0;
            TInt format = stmt.ColumnInt( column++ );  
            if(format == 1 /*TThumbnailFormat::EThumbnailFormatJpeg */ )
               {
               TPtrC8 ptr = stmt.ColumnBinaryL( column++ );
               HBufC8* data = ptr.AllocL() ;
               aThumbnail = NULL;
               aData = data;
               
            } else {
    
               TPtrC8 ptr = stmt.ColumnBinaryL( column++ );
               RDesReadStream stream( ptr );
               aThumbnail = new( ELeave )CFbsBitmap();
               aThumbnail->InternalizeL( stream );
               aData = NULL;
               }
            
            //fetch real size of TN
            column = 2;
            aThumbnailRealSize.iWidth = stmt.ColumnInt( column++ );
            aThumbnailRealSize.iHeight = stmt.ColumnInt( column );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- thumbnail NOT found" );
        }
        
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );

    User::LeaveIfError( found );
    }

// -----------------------------------------------------------------------------
// Delete thumbnails for given object file by Path
// -----------------------------------------------------------------------------
//
void CThumbnailStore::DeleteThumbnailsL( const TDesC& aPath )
    {
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );
    transaction.BeginL();
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    
    TInt paramIndex = 0;
    TInt paramIndex1 = 0;
    TInt paramIndex2 = 0;
    TInt rowStatus = 0;
    TInt column = 0;
    TInt rowid = 0;
    TInt deleteCount = 0;
    
    TN_DEBUG1( "CThumbnailStore::DeleteThumbnailsByPathL() -- TEMP TABLE lookup" );
    TInt err = stmt.Prepare( iDatabase, KTempThumbnailSqlSelectRowIDInfoByPath);
    User::LeaveIfError( err );
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
    
    rowStatus = stmt.Next();
    RSqlStatement stmt_info;
    CleanupClosePushL( stmt_info );
    err = stmt_info.Prepare( iDatabase, KTempThumbnailSqlDeleteInfoByPath);
    RSqlStatement stmt_infodata;
    CleanupClosePushL( stmt_infodata );
    err = stmt_infodata.Prepare( iDatabase, KTempThumbnailSqlDeleteInfoDataByPath);
    
    
    while(rowStatus == KSqlAtRow)
       {  
       rowid = stmt.ColumnInt( column ); 
       paramIndex1 = stmt_info.ParameterIndex( KThumbnailSqlParamRowID );
       User::LeaveIfError( paramIndex1 );
       User::LeaveIfError( stmt_info.BindInt( paramIndex1, rowid ));
       
       deleteCount = stmt_info.Exec();
       stmt_info.Reset();
       User::LeaveIfError( deleteCount );
             
       paramIndex2 = stmt_infodata.ParameterIndex( KThumbnailSqlParamRowID );
       User::LeaveIfError( paramIndex2 );
       User::LeaveIfError( stmt_infodata.BindInt( paramIndex2, rowid ));
             
       deleteCount = stmt_infodata.Exec();
       stmt_infodata.Reset();
       User::LeaveIfError( deleteCount );
       
       TN_DEBUG1( "CThumbnailStore::DeleteThumbnailsByPathL() -- TEMP TABLE lookup - thumbnail deleted" );
       
       // fetch another row (temp table rowIDs are updated immediately)
       stmt.Reset();
       
       paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
       User::LeaveIfError( paramIndex );
       User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
       
       rowStatus = stmt.Next();   
       }
    stmt_infodata.Close();
    stmt_info.Close();
    CleanupStack::PopAndDestroy( &stmt_infodata );
    CleanupStack::PopAndDestroy( &stmt_info );
    

    //look from real table 
    TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- MAIN TABLE lookup" );
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    CleanupClosePushL( stmt );   
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSqlSelectRowIDInfoByPath ));
    
    User::LeaveIfError( err );
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
         
    rowStatus = stmt.Next();   
    CleanupClosePushL( stmt_info );
    err = stmt_info.Prepare( iDatabase, KThumbnailSqlDeleteInfoByPath);
    CleanupClosePushL( stmt_infodata );
    err = stmt_infodata.Prepare( iDatabase, KThumbnailSqlDeleteInfoDataByPath);
       
       
    while(rowStatus == KSqlAtRow)
       { 
       rowid = stmt.ColumnInt( column ); 
       paramIndex1 = stmt_info.ParameterIndex( KThumbnailSqlParamRowID );
       User::LeaveIfError( paramIndex1 );
       User::LeaveIfError( stmt_info.BindInt( paramIndex1, rowid ));
          
       deleteCount = stmt_info.Exec();
       stmt_info.Reset();
       User::LeaveIfError( deleteCount );
                
       paramIndex2 = stmt_infodata.ParameterIndex( KThumbnailSqlParamRowID );  
       User::LeaveIfError( paramIndex2 );
       User::LeaveIfError( stmt_infodata.BindInt( paramIndex2, rowid ));
                
       deleteCount = stmt_infodata.Exec();
       stmt_infodata.Reset();
       User::LeaveIfError( deleteCount );
       
       TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- MAIN TABLE lookup - thumbnail deleted" );
       
       rowStatus = stmt.Next();
       }
    
    stmt_infodata.Close();
    stmt_info.Close();
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt_infodata );
    CleanupStack::PopAndDestroy( &stmt_info );
    CleanupStack::PopAndDestroy( &stmt );    
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );
    }

// -----------------------------------------------------------------------------
// Delete thumbnails for given object file by Id
// -----------------------------------------------------------------------------
//
void CThumbnailStore::DeleteThumbnailsL( const TThumbnailId& aTNId )
    {
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
        
    TInt paramIndex = 0;
    TInt paramIndex1 = 0;
    TInt paramIndex2 = 0;
    TInt rowStatus = 0;
    TInt column = 0;
    TInt rowid = 0;
    TInt deleteCount = 0;
      
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );
    transaction.BeginL();
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    
    TN_DEBUG1( "CThumbnailStore::DeleteThumbnailsByIdL() -- TEMP TABLE lookup" );
    TInt err = stmt.Prepare( iDatabase, KTempThumbnailSqlSelectRowIDInfoByID);
    User::LeaveIfError( err );
        
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aTNId ));
        
    rowStatus = stmt.Next();
    RSqlStatement stmt_info;
    CleanupClosePushL( stmt_info );
    err = stmt_info.Prepare( iDatabase, KTempThumbnailSqlDeleteInfoByID);
    RSqlStatement stmt_infodata;
    CleanupClosePushL( stmt_infodata );
    err = stmt_infodata.Prepare( iDatabase, KTempThumbnailSqlDeleteInfoDataByID);
        
        
    while(rowStatus == KSqlAtRow)
       {  
       rowid = stmt.ColumnInt( column ); 
       paramIndex1 = stmt_info.ParameterIndex( KThumbnailSqlParamRowID );
       User::LeaveIfError( paramIndex1 );
       User::LeaveIfError( stmt_info.BindInt( paramIndex1, rowid ));
           
       err = stmt_info.Exec();
       stmt_info.Reset();
       User::LeaveIfError( err );
                 
       paramIndex2 = stmt_infodata.ParameterIndex( KThumbnailSqlParamRowID );
       User::LeaveIfError( paramIndex2 );
       User::LeaveIfError( stmt_infodata.BindInt( paramIndex2, rowid ));
                 
       err = stmt_infodata.Exec();
       stmt_infodata.Reset();
       User::LeaveIfError( err );
       deleteCount++;
       
       TN_DEBUG1( "CThumbnailStore::DeleteThumbnailsByIdL() -- TEMP TABLE lookup - thumbnail deleted" );
       
       // fetch another row (temp table rowIDs are updated immediately)
       stmt.Reset();
       
       paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
       User::LeaveIfError( paramIndex );
       User::LeaveIfError( stmt.BindInt( paramIndex, aTNId ));
       
       rowStatus = stmt.Next();    
       }
    
    stmt_infodata.Close();
    stmt_info.Close();
    CleanupStack::PopAndDestroy( &stmt_infodata );
    CleanupStack::PopAndDestroy( &stmt_info );
        
    
    //look from real table       
    TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByIdL() -- MAIN TABLE lookup" );
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    CleanupClosePushL( stmt );   
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSqlSelectRowIDInfoByID ));
        
    User::LeaveIfError( err );
        
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aTNId ));
             
    rowStatus = stmt.Next();   
    CleanupClosePushL( stmt_info );
    err = stmt_info.Prepare( iDatabase, KThumbnailSqlDeleteInfoByID);
    CleanupClosePushL( stmt_infodata );
    err = stmt_infodata.Prepare( iDatabase, KThumbnailSqlDeleteInfoDataByID);
           
           
    while(rowStatus == KSqlAtRow)
       { 
       rowid = stmt.ColumnInt( column ); 
       paramIndex1 = stmt_info.ParameterIndex( KThumbnailSqlParamRowID );
       User::LeaveIfError( paramIndex1 );
       User::LeaveIfError( stmt_info.BindInt( paramIndex1, rowid ));
              
       err = stmt_info.Exec();
       stmt_info.Reset();
       User::LeaveIfError( err );
                    
       paramIndex2 = stmt_infodata.ParameterIndex( KThumbnailSqlParamRowID );  
       User::LeaveIfError( paramIndex2 );
       User::LeaveIfError( stmt_infodata.BindInt( paramIndex2, rowid ));
                    
       err = stmt_infodata.Exec();
       stmt_infodata.Reset();
       User::LeaveIfError( err );
       deleteCount++;
       
       TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByIdL() -- MAIN TABLE lookup - thumbnail deleted" );
       
       rowStatus = stmt.Next();
       }
        
    stmt_infodata.Close();
    stmt_info.Close();
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt_infodata );
    CleanupStack::PopAndDestroy( &stmt_info );
    CleanupStack::PopAndDestroy( &stmt );    
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::DeleteThumbnailsByIdL() took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif
    
    if(!deleteCount)
        {
        User::Leave(KErrNotFound);
        }
    }


// ---------------------------------------------------------------------------
// CThumbnailStore::PersistentSizes()
// ---------------------------------------------------------------------------
//
void CThumbnailStore::SetPersistentSizes(const RArray < TThumbnailPersistentSize > &aSizes)
    {
    iPersistentSizes = aSizes;
    }

// ---------------------------------------------------------------------------
// CThumbnailStore::FlushCacheTable()
// ---------------------------------------------------------------------------
//
void CThumbnailStore::FlushCacheTable( TBool aForce )
    {
    TN_DEBUG1("CThumbnailStore::FlushCacheTable() in");
    
    StopAutoFlush();
    
    if(iBatchItemCount <= 0)
        {
        // cache empty
        return;
        }
    
    if(iBatchItemCount < KMaxBatchItems && !aForce)
       {
       //some items in cache
       StartAutoFlush();
       return;
       }
    
    //cache full, flush now
    iBatchItemCount = 0;
    
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    // Move data from temp table to main....
    TInt err_begin = iDatabase.Exec( KThumbnailBeginTransaction );
    TN_DEBUG2("CThumbnailStore::FlushCacheTable() KThumbnailBeginTransaction %d", err_begin);
    
    TInt err_tempinfo = iDatabase.Exec( KThumbnailMoveFromTempInfoToMainTable );
    TN_DEBUG2("CThumbnailStore::FlushCacheTable() KThumbnailMoveFromTempInfoToMainTable %d", err_tempinfo);
    
    TInt err_tempdata = iDatabase.Exec( KThumbnailMoveFromTempDataToMainTable );
    TN_DEBUG2("CThumbnailStore::FlushCacheTable() KThumbnailMoveFromTempDataToMainTable %d", err_tempdata);
    
    TInt err_delinfo = iDatabase.Exec( KThumbnailDeleteFromTempInfoTable );
    TN_DEBUG2("CThumbnailStore::FlushCacheTable() KThumbnailDeleteFromTempInfoTable %d", err_delinfo);
    
    TInt err_deldata = iDatabase.Exec( KThumbnailDeleteFromTempDataTable );
    TN_DEBUG2("CThumbnailStore::FlushCacheTable() KThumbnailDeleteFromTempDataTable %d", err_deldata);
   
    
    if( err_tempinfo < 0 || err_tempdata < 0  || err_delinfo < 0  || err_deldata < 0 )
        {
        TInt err = iDatabase.Exec( KThumbnailRollbackTransaction );
        TN_DEBUG2("CThumbnailStore::FlushCacheTable() KThumbnailRollbackTransaction %d", err);
        }
    else
        {
        TInt err_commit = iDatabase.Exec( KThumbnailCommitTransaction );
        TN_DEBUG2("CThumbnailStore::FlushCacheTable() KThumbnailCommitTransaction %d", err_commit);
        }
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::FlushCacheTable() took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif

    TN_DEBUG1("CThumbnailStore::FlushCacheTable() out");
    }


// -----------------------------------------------------------------------------
// Find store for thumbnails by Id
// -----------------------------------------------------------------------------
//
void CThumbnailStore::FindStoreL(TThumbnailId aThumbnailId)
    {
    TN_DEBUG2( "CThumbnailStore::FindStore( %d )", aThumbnailId );
     RSqlStatement stmt;
     CleanupClosePushL( stmt );
         
     TInt paramIndex = 0;
     TInt found = KErrNotFound;
     TInt rowStatus = 0;
         
     TN_DEBUG1( "CThumbnailStore::FindStore() -- TEMP TABLE lookup" );
     User::LeaveIfError( stmt.Prepare( iDatabase, KTempThumbnailSqlSelectRowIDInfoByID ));
     paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
     User::LeaveIfError( paramIndex );
     User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
         
     rowStatus = stmt.Next();

    //if not found from temp table, look from real table
    if(rowStatus != KSqlAtRow)
        {
        TN_DEBUG1( "CThumbnailStore::FindStore() -- MAIN TABLE lookup" );
        stmt.Close();
        CleanupStack::PopAndDestroy( &stmt );
        CleanupClosePushL( stmt );
            
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSqlSelectRowIDInfoByID ));
         
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnailId ));
            
        rowStatus = stmt.Next();
        }

    if( rowStatus == KSqlAtRow )
        {
        TN_DEBUG1( "CThumbnailStore::FindStore() -- thumbnail found" );
        found = KErrNone;
        }
    else
        {
        TN_DEBUG1( "CThumbnailStore::FindStore() -- thumbnail NOT found" );
        }
             
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );

    User::LeaveIfError( found );
    }

// -----------------------------------------------------------------------------
// Updates path in current store by Id
// -----------------------------------------------------------------------------
//
TBool CThumbnailStore::UpdateStoreL( TThumbnailId aItemId, const TDesC& aNewPath )
    {
    TN_DEBUG3( "CThumbnailStore::UpdateStore( %d, %S) by ID", aItemId, &aNewPath);

    TBool doUpdate(EFalse);
    TPath oldPath;
    TInt column = 0;
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
       
    //check if path needs updating in temp table
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempPathByID ));
    
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aItemId ));
     
    TInt rowStatus = stmt.Next();

     //if not found from temp table, look from real table
     if(rowStatus != KSqlAtRow)
         {
         stmt.Close();
         CleanupStack::PopAndDestroy( &stmt );
         CleanupClosePushL( stmt );
         
         //check if path needs updating in main table
         User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectPathByID ));
         
         paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
         User::LeaveIfError( paramIndex );
         User::LeaveIfError( stmt.BindInt( paramIndex, aItemId ));
          
         rowStatus = stmt.Next();
         }
     
     if(rowStatus == KSqlAtRow)
         {
           TN_DEBUG1( "CThumbnailStore::UpdateStore() -- matching TN ID found" );
           oldPath = stmt.ColumnTextL(column);
            
            if(oldPath.CompareF(aNewPath) != 0)
                {
                TN_DEBUG1( "CThumbnailStore::UpdateStore() -- path no match" );
                doUpdate = ETrue;
                }
            else
                {
                TN_DEBUG1( "CThumbnailStore::UpdateStore() -- path match, skip..." );
                }
         }
     
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
        
    if(!doUpdate)
        {
        TN_DEBUG2( "CThumbnailStore::UpdateStore() -- no need to update old path=%S", &oldPath );
        return EFalse;
        }
    
    //Encapsulate update to Transaction
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );
    transaction.BeginL();
	
    CleanupClosePushL( stmt );
     
    TN_DEBUG1( "CThumbnailStore::UpdateStore() -- do temp path update" );

    User::LeaveIfError( stmt.Prepare( iDatabase, KTempThumbnailSqlUpdateById ));
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aNewPath ));
       
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aItemId ));
    User::LeaveIfError( stmt.Exec());
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    CleanupClosePushL( stmt );
    
    TN_DEBUG1( "CThumbnailStore::UpdateStore() -- do main table path update" );
             
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSqlUpdateById ));
                
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aNewPath ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aItemId ));  
            
    User::LeaveIfError( stmt.Exec());   
    CleanupStack::PopAndDestroy( &stmt );
    
    // Commit transaction
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );
    
    return ETrue;
    }

// -----------------------------------------------------------------------------
// Update IDs by Path
// -----------------------------------------------------------------------------
//
void CThumbnailStore::UpdateStoreL( const TDesC& aPath, TThumbnailId aNewId )
    {
    TN_DEBUG3( "CThumbnailStore::UpdateStore( %S, %d ) by Path", &aPath, aNewId);
    
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    //Encapsulate update to Transaction
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );
    transaction.BeginL();
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
       
    TN_DEBUG1( "CThumbnailStore::UpdateStoreL() -- do temp ID update" );
    
    TInt err = stmt.Prepare( iDatabase, KTempThumbnailUpdateIdByPath );
    
#ifdef _DEBUG
    TPtrC errorMsg = iDatabase.LastErrorMessage();
    TN_DEBUG2( "CThumbnailStore::UpdateStoreL() KTempThumbnailUpdateIdByPath %S" , &errorMsg);
#endif
    
    User::LeaveIfError( err );
    
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aNewId ));
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
    
    err =  stmt.Exec();
    
    TN_DEBUG2( "CThumbnailStore::UpdateStoreL() err==%d", err );
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    CleanupClosePushL( stmt );
    
    TN_DEBUG1( "CThumbnailStore::UpdateStoreL() -- do main table ID update" );
    
    err = stmt.Prepare( iDatabase, KThumbnailUpdateIdByPath );
    
#ifdef _DEBUG
    TPtrC errorMsg2 = iDatabase.LastErrorMessage();
    TN_DEBUG2( "CThumbnailStore::UpdateStoreL() KThumbnailUpdateIdByPath %S" , &errorMsg2);
#endif    
    
    User::LeaveIfError( err );
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aNewId ));
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
    
    err =  stmt.Exec();
    
    TN_DEBUG2( "CThumbnailStore::UpdateStoreL() err==%d", err );
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt ); 
    
    // Commit transaction
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::UpdateStoreL() took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif
    }

// -----------------------------------------------------------------------------
// Checks if given modification timestamp is newer than in DB
// -----------------------------------------------------------------------------
//
TBool CThumbnailStore::CheckModifiedL( const TThumbnailId aItemId, const TInt64 aModified )
    {
    TN_DEBUG2( "CThumbnailStore::CheckModifiedL( %d )", aItemId);

    TInt column = 0;
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
       
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempModifiedByID ));
    
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aItemId ));
     
    TInt rowStatus = stmt.Next();
    
    TBool modified = EFalse;
    TBool checkMain = EFalse;
    
    TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- temp" );
    
    while(rowStatus == KSqlAtRow || !checkMain)
        {
            if(rowStatus == KSqlAtRow)
                {
               TInt64 oldModified = stmt.ColumnInt64( column );
               
               if (oldModified < aModified)
                   {
                   TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- timestamp is newer than original" );
                   modified = ETrue;
                   break;
                   }
               else if (oldModified > aModified)
                   {
                   TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- timestamp is older than original" );
                   break;
                   }
               else if (oldModified == aModified)
                   {
                   TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- timestamp is the same as original" );
                   }
                }
            
        rowStatus = stmt.Next();
        
        //switch to main table if modified not found from temp
        if(rowStatus != KSqlAtRow && !checkMain && !modified)
            {
            TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- main" );
            //come here only once
            checkMain = ETrue;
            
            stmt.Close();
            CleanupStack::PopAndDestroy( &stmt );
            CleanupClosePushL( stmt );
            
            User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectModifiedByID ));
            
            paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmt.BindInt( paramIndex, aItemId ));
             
            rowStatus = stmt.Next();
            }
        }
     
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt ); 
    
    return modified;
    }

// -----------------------------------------------------------------------------
// Fetches thumbnails from store by Id
// -----------------------------------------------------------------------------
//
void CThumbnailStore::FetchThumbnailsL(TThumbnailId aItemId, RArray < TThumbnailDatabaseData* >& aThumbnails)
    {
    TN_DEBUG1( "CThumbnailStore::FetchThumbnails()" );
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    
    // first temp table
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempById ));

    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aItemId ));

    TInt rowStatus = stmt.Next();
    
    TPath path;
    TPath tnPath;
    while ( rowStatus == KSqlAtRow)
        {
        TN_DEBUG1( "CThumbnailStore::FetchThumbnails() -- thumbnail found from temp table" );
        
        TInt column = 0;
            
        TThumbnailDatabaseData* newRow = new(ELeave) TThumbnailDatabaseData;
            
        TInt err = stmt.ColumnText( column++, newRow->iPath );
        newRow->iTnId = stmt.ColumnInt( column++ );
        newRow->iSize = stmt.ColumnInt( column++ );
        newRow->iFormat = stmt.ColumnInt( column++ );
        err = stmt.ColumnText( column++, newRow->iTnPath);
        newRow->iWidth = stmt.ColumnInt( column++ );
        newRow->iHeight = stmt.ColumnInt( column++ );
        newRow->iOrigWidth = stmt.ColumnInt( column++ );
        newRow->iOrigHeight = stmt.ColumnInt( column++ );
        newRow->iFlags = stmt.ColumnInt( column++ );
        newRow->iVideoPosition = stmt.ColumnInt( column++ );
        newRow->iOrientation = stmt.ColumnInt( column++ );
        newRow->iThumbFromPath = stmt.ColumnInt( column++ );
        newRow->iModified = stmt.ColumnInt64( column++ );
        
        if(newRow->iFormat == 0)
            {
            TPtrC8 ptr = stmt.ColumnBinaryL( column++ );
            RDesReadStream stream( ptr );
            newRow->iBlob = new( ELeave )CFbsBitmap();
            newRow->iBlob->InternalizeL( stream );
            }
        else if(newRow->iFormat == 1)
            {
            TPtrC8 ptr = stmt.ColumnBinaryL( column++ );
            HBufC8* data = ptr.AllocL() ;
            newRow->iBlob = NULL;
            newRow->iData = data;
            }
        
        aThumbnails.Append( newRow );
               
        rowStatus = stmt.Next();
        }
    
    // then real table
    stmt.Close();
    CleanupStack::PopAndDestroy(&stmt); 
    CleanupClosePushL( stmt );
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectById ));

    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aItemId ));

    rowStatus = stmt.Next();
    while ( rowStatus == KSqlAtRow)
        {
        TN_DEBUG1( "CThumbnailStore::FetchThumbnails() -- thumbnail found from real table" );
        
        TInt column = 0;
        
        TThumbnailDatabaseData* newRow = new(ELeave) TThumbnailDatabaseData;
        
        TInt err = stmt.ColumnText( column++, newRow->iPath );
        newRow->iTnId = stmt.ColumnInt( column++ );
        newRow->iSize = stmt.ColumnInt( column++ );
        newRow->iFormat = stmt.ColumnInt( column++ );
        err = stmt.ColumnText( column++, newRow->iTnPath);
        newRow->iWidth = stmt.ColumnInt( column++ );
        newRow->iHeight = stmt.ColumnInt( column++ );
        newRow->iOrigWidth = stmt.ColumnInt( column++ );
        newRow->iOrigHeight = stmt.ColumnInt( column++ );
        newRow->iFlags = stmt.ColumnInt( column++ );
        newRow->iVideoPosition = stmt.ColumnInt( column++ );
        newRow->iOrientation = stmt.ColumnInt( column++ );
        newRow->iThumbFromPath = stmt.ColumnInt( column++ );
        newRow->iModified = stmt.ColumnInt64( column++ );
        
        if(newRow->iFormat == 0)
            {
            TPtrC8 ptr = stmt.ColumnBinaryL( column++ );
            RDesReadStream stream( ptr );
            newRow->iBlob = new( ELeave )CFbsBitmap();
            newRow->iBlob->InternalizeL( stream );
            }
        else if(newRow->iFormat == 1)
            {
            TPtrC8 ptr = stmt.ColumnBinaryL( column++ );
            HBufC8* data = ptr.AllocL() ;
            newRow->iBlob = NULL;
            newRow->iData = data;
            }
        
        aThumbnails.Append( newRow );
              
        rowStatus = stmt.Next();
        }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    }

// -----------------------------------------------------------------------------
// Stores thumbnails to store
// -----------------------------------------------------------------------------
//

void CThumbnailStore::StoreThumbnailsL(const TDesC& aNewPath, RArray < TThumbnailDatabaseData* >& aThumbnails)
    {
    TN_DEBUG1( "CThumbnailStore::StoreThumbnails()" );
    
    TInt ThumbnailCount = aThumbnails.Count();
    RSqlStatement stmt;
    for ( TInt i = 0; i < ThumbnailCount; i++ )
        {
        RThumbnailTransaction transaction( iDatabase );
        CleanupClosePushL( transaction );
        transaction.BeginL();
        
        CleanupClosePushL( stmt );
          
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailInsertThumbnailInfoByPathAndId ));
        
        TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindText( paramIndex, aNewPath ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamWidth );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iWidth ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamHeight );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iHeight ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamOriginalWidth );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iOrigWidth ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamOriginalHeight );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iOrigHeight ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamFormat );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iFormat ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamFlags );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iFlags ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamSize );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iSize ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iTnId ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamOrientation );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iOrientation ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamThumbFromPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt( paramIndex, aThumbnails[ i ]->iThumbFromPath ));
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamModified );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindInt64( paramIndex, aThumbnails[ i ]->iModified ));
        
        User::LeaveIfError( stmt.Exec());    
        CleanupStack::PopAndDestroy( &stmt );
        
        RSqlStatement stmtData;
        CleanupClosePushL( stmtData );
        TInt err = stmtData.Prepare( iDatabase, KThumbnailInsertTempThumbnailInfoData );
        
        if(aThumbnails[ i ]->iFormat == 0)
            {
            CBufFlat* buf = CBufFlat::NewL( KStreamBufferSize );
            CleanupStack::PushL( buf );
            RBufWriteStream stream;
            stream.Open( *buf );
            aThumbnails[ i ]->iBlob->ExternalizeL( stream );
            paramIndex = stmtData.ParameterIndex( KThumbnailSqlParamData );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmtData.BindBinary( paramIndex, buf->Ptr( 0 ) ));
            CleanupStack::PopAndDestroy( buf );
            delete aThumbnails[i]->iBlob;
            aThumbnails[i]->iBlob = NULL;
            }
        else if(aThumbnails[ i ]->iFormat == 1)
            {
            paramIndex = stmtData.ParameterIndex( KThumbnailSqlParamData );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmtData.BindBinary( paramIndex, *aThumbnails[ i ]->iData ));
            delete aThumbnails[i]->iData;
            aThumbnails[i]->iData = NULL;
            }
        
        User::LeaveIfError( stmtData.Exec());
        CleanupStack::PopAndDestroy( &stmtData );
        
        // Commit transaction
        transaction.CommitL();
        CleanupStack::PopAndDestroy( &transaction );
        
        delete aThumbnails[i];
        aThumbnails[i] = NULL;
        iBatchItemCount++;                          
        }
    
    FlushCacheTable();
    }

// -----------------------------------------------------------------------------
// CheckVersionAndImeiL()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::CheckImeiL()
    {
    TN_DEBUG1( "CThumbnailStore::CheckImeiL()" );
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
         
    TInt rowStatus = 0;
    TInt column = 0;
    TBuf<KImeiBufferSize> imei;
      
    TInt ret = stmt.Prepare( iDatabase, KThumbnailSelectFromVersion );
    if(ret < 0 )
       {  
       stmt.Close();
       CleanupStack::PopAndDestroy( &stmt ); 
       return KErrNotSupported;
       }
              
    rowStatus = stmt.Next();
    
    if ( rowStatus == KSqlAtRow)    
       {        
       column=2;
       stmt.ColumnText( column++, imei);  
       }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt ); 
    
    if( imei == iImei )
      {
      return KErrNone;  
      }
    else
      {
      return KErrNotSupported;  
      }
    }

// -----------------------------------------------------------------------------
// CheckVersionAndImeiL()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::CheckVersionL()
    {
    TN_DEBUG1( "CThumbnailStore::CheckVersionL()" );
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
         
    TInt rowStatus = 0;
    TInt column = 0;
    TInt minor = 0;
    TInt major = 0;

      
    TInt ret = stmt.Prepare( iDatabase, KThumbnailSelectFromVersion );
    if(ret < 0 )
       {  
       stmt.Close();
       CleanupStack::PopAndDestroy( &stmt ); 
       return KErrNotSupported;
       }
              
    rowStatus = stmt.Next();
    
    if ( rowStatus == KSqlAtRow)    
       {        
       major = stmt.ColumnInt( column++);
       minor = stmt.ColumnInt( column++);
       }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt ); 
    
    if(major == KMajor && minor == KMinor )
      {
      return KErrNone;  
      }
    else
      {
      TN_DEBUG1( "CThumbnailStore::CheckVersionL() - wrong DB version" );
      return KErrNotSupported;  
      }
    }


// -----------------------------------------------------------------------------
// CheckVersionAndImeiL()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::CheckMediaIDL()
    {
    
    TN_DEBUG1( "CThumbnailStore::CheckMediaIDL()" );
    TInt err = 0;
    
    TVolumeInfo volumeinfo;
    err = iFs.Volume(volumeinfo, iDrive);
    TUint id = volumeinfo.iUniqueID;
    TBuf<50> mediaid;
    mediaid.Num(id);
    
    RFile64 file;
    err = file.Open(iFs, mediaid, EFileShareReadersOrWriters);
    if(err)
       {
       file.Create(iFs, mediaid, EFileShareReadersOrWriters );
       file.Close();
       return KErrNotSupported;
       } 
    file.Close();
    return KErrNone;  
    }
     
// -----------------------------------------------------------------------------
// AddVersionAndImeiL()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::AddVersionAndImeiL()
    {
    
    TN_DEBUG1( "CThumbnailStore::AddVersionAndImei()" );
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
            
    TInt paramIndex = 0;
            
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailInsertToVersion ));
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamImei );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, iImei ));  
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamMinor );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, KMinor )); 
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamMajor );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, KMajor )); 
    
    User::LeaveIfError( stmt.Exec());
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    
    }

// -----------------------------------------------------------------------------
// ResetThumbnailIDs()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::ResetThumbnailIDs()
    {
    TN_DEBUG1( "CThumbnailStore::ResetThumbnailIDs()" );

    TInt err = iDatabase.Exec( KTempThumbnailResetIDs );
    TN_DEBUG2( "CThumbnailStore::ResetThumbnailIDs() KThumbnailResetIDs - temp table, err=%d", err );
    
    err = iDatabase.Exec( KThumbnailResetIDs );
    TN_DEBUG2( "CThumbnailStore::ResetThumbnailIDs() KThumbnailResetIDs - main table, err=%d", err );
    }


// -----------------------------------------------------------------------------
// UpdateImeiL()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::UpdateImeiL()
    {
    TN_DEBUG1( "CThumbnailStore::UpdateImeiL()" );
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
         
      
    TInt ret = stmt.Prepare( iDatabase, KThumbnailUpdateIMEI );
    
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamImei );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, iImei ));
    
    TInt err =  stmt.Exec();
    
    TN_DEBUG2( "CThumbnailStore::UpdateImeiL() err==%d", err );
    
    User::LeaveIfError( err );
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt ); 
    }

// -----------------------------------------------------------------------------
// StartAutoFlush()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::StartAutoFlush()
    {
    TN_DEBUG1( "CThumbnailStore::StartAutoFlush()" );
    
    TInt err = KErrNone;
    
    if( iAutoFlushTimer )
        {
        iAutoFlushTimer->Cancel();
        }
    else
        {
        TRAP(err, iAutoFlushTimer = CPeriodic::NewL(CActive::EPriorityIdle) );
        }
    
    if (err != KErrNone)
        {
        TN_DEBUG2( "CThumbnailStore::StartAutoFlush() - Error creating timer (%d)", err );
        }
    else
        {
        iAutoFlushTimer->Start( KAutoFlushTimeout, KAutoFlushTimeout, 
                                TCallBack(AutoFlushTimerCallBack, this));
        }
    }

// -----------------------------------------------------------------------------
// StopAutoFlush()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::StopAutoFlush()
    {
    TN_DEBUG1( "CThumbnailStore::StopAutoFlush()" );
    if( iAutoFlushTimer )
        {
        iAutoFlushTimer->Cancel();
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailStore::AutoFlushTimerCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbnailStore::AutoFlushTimerCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbnailStore::AutoFlushTimerCallBack()");
    CThumbnailStore* self = static_cast<CThumbnailStore*>( aAny );
    
    self->FlushCacheTable(ETrue);

    return KErrNone; // Return value ignored by CPeriodic
    }

TInt CThumbnailStore::CheckRowIDsL()
    {
    TN_DEBUG1( "CThumbnailStore::CheckRowIDs()");
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    TInt column = 0;   
    TInt rowStatus = 0;
    TInt64 inforows = 0;
    TInt64 datarows = 0;
    
    TInt ret = stmt.Prepare( iDatabase, KGetInfoRowID );
    if(ret < 0)
        {
        stmt.Close();
        CleanupStack::PopAndDestroy( &stmt );
        return KErrNotSupported;
        }
    rowStatus = stmt.Next();
                
    if ( rowStatus == KSqlAtRow)    
        {        
        inforows = stmt.ColumnInt64( column );  
        }
            
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
            
    CleanupClosePushL( stmt );
    ret = stmt.Prepare( iDatabase, KGetDataRowID );
    if(ret < 0)
        {
        stmt.Close();
        CleanupStack::PopAndDestroy( &stmt );
        return KErrNotSupported;
        }
    rowStatus = stmt.Next();
                       
    if ( rowStatus == KSqlAtRow)    
        {        
        datarows = stmt.ColumnInt64( column );  
        }
            
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
            
    if( inforows != datarows)
        {
        TN_DEBUG1( "CThumbnailStore::CheckRowIDsL() - tables out of sync" );
        return KErrNotSupported;
        }  
    else
        {
        return KErrNone;
        }
    }

void CThumbnailStore::CheckModifiedByIdL( TUint32 aId, TBool aTempTable,
    TBool& aModified )
    {
    TN_DEBUG1( "CThumbnailStore::CheckModifiedByIdL()");
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    TInt column( 0 );
    
    if( aTempTable )
        {
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempPathModifiedByID ) );
        }
    else
        {
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectPathModifiedByID ) );
        }
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamId );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, aId ));
     
    TInt rowStatus = stmt.Next();
    
    if(rowStatus == KSqlAtRow)
        {
        TPath path = stmt.ColumnTextL(column++);
           
        if (path.Length())
            {
            TInt64 modified = stmt.ColumnInt64( column );
            TTime timeStamp;
            iFs.Modified( path, timeStamp );
            
            if( modified != timeStamp.Int64() )
                {
                aModified = ETrue;
                }
            }
        }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    }

void CThumbnailStore::CheckModifiedByPathL( const TDesC& aPath, TBool aTempTable,
    TBool& aModified )
    {
    TN_DEBUG1( "CThumbnailStore::CheckModifiedByPathL()");
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    TInt column( 0 );
    
    if( aTempTable )
        {
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempModifiedByPath ) );
        }
    else
        {
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectModifiedByPath ) );
        }
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
     
    TInt rowStatus = stmt.Next();
    
    if(rowStatus == KSqlAtRow)
        {
        TInt64 modified = stmt.ColumnInt64( column );
        TTime timeStamp;
        iFs.Modified( aPath, timeStamp );
        
        if( modified != timeStamp.Int64() )
            {
            aModified = ETrue;
            }
        }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    }

// -----------------------------------------------------------------------------
// PrepareBlacklistedItemsForRetry()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::PrepareBlacklistedItemsForRetry()
    {
    TN_DEBUG1( "CThumbnailStore::PrepareBlacklistedItemsForRetry()" );
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );

    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailTouchBlacklistedRows ));
    
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamFlag );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, KThumbnailDbFlagBlacklisted ));
    TInt err = stmt.Exec();
   
    TN_DEBUG2( "CThumbnailStore::PrepareBlacklistedItemsForRetry() - main table, err=%d", err );
    
    CleanupStack::PopAndDestroy( &stmt );
    }

void CThumbnailStore::HandleDiskSpaceNotificationL( TBool aDiskFull )
    {
    TN_DEBUG2( "CThumbnailStore::HandleDiskSpaceNotificationL() aDiskFull = %d", aDiskFull );
    iDiskFull = aDiskFull;
    }


void CThumbnailStore::HandleDiskSpaceError(TInt aError )
    {
    if (aError != KErrNone)
        {
        TN_DEBUG2( "CThumbnailStore::HandleDiskSpaceError() aError = %d", aError );
        }
    }

TBool CThumbnailStore::IsDiskFull()
    {
    return iDiskFull;
    }

CThumbnailStoreDiskSpaceNotifierAO* CThumbnailStoreDiskSpaceNotifierAO::NewL(
        MThumbnailStoreDiskSpaceNotifierObserver& aObserver, TInt64 aThreshold, const TDesC& aFilename)
    {
    CThumbnailStoreDiskSpaceNotifierAO* self = 
        CThumbnailStoreDiskSpaceNotifierAO::NewLC( aObserver, aThreshold, aFilename);
    CleanupStack::Pop( self );
    return self;
    }

CThumbnailStoreDiskSpaceNotifierAO* CThumbnailStoreDiskSpaceNotifierAO::NewLC(
        MThumbnailStoreDiskSpaceNotifierObserver& aObserver, TInt64 aThreshold, const TDesC& aFilename)
    {
    TDriveNumber driveNumber = GetDriveNumberL( aFilename );
    
    CThumbnailStoreDiskSpaceNotifierAO* self = 
        new ( ELeave ) CThumbnailStoreDiskSpaceNotifierAO( aObserver, aThreshold, driveNumber );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

TDriveNumber CThumbnailStoreDiskSpaceNotifierAO::GetDriveNumberL( const TDesC& aFilename )
    {
    TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::GetDriveNumberL()");
    TLex driveParser( aFilename );
    
    TChar driveChar = driveParser.Get();

    if( 0 == driveChar || TChar( ':' ) != driveParser.Peek() )
        {
        TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::GetDriveNumberL() KErrArgument");
        User::Leave( KErrArgument );
        }
        
    TInt driveNumber;
    
    RFs::CharToDrive( driveChar, driveNumber );
    
    return (TDriveNumber)driveNumber;
    }


CThumbnailStoreDiskSpaceNotifierAO::~CThumbnailStoreDiskSpaceNotifierAO()
    {
    TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::~CThumbnailStoreDiskSpaceNotifierAO()");
    Cancel();

    iFileServerSession.Close();
    }

void CThumbnailStoreDiskSpaceNotifierAO::RunL()
    {   
    TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::RunL()");
    TVolumeInfo volumeInfo;

    if ( iState == CThumbnailStoreDiskSpaceNotifierAO::ENormal )
        {
        TInt status = iStatus.Int();
        
        switch( status )
            {
            case KErrNone:
                iFileServerSession.Volume( volumeInfo, iDrive );
                
                // Check if free space is less than threshold level
                if( volumeInfo.iFree < iThreshold )
                    {
                    iDiskFull = ETrue;
                    iObserver.HandleDiskSpaceNotificationL( iDiskFull );
                    iState = EIterate;
                    iIterationCount = 0;
                    SetActive();
                    TRequestStatus* status = &iStatus;
                    User::RequestComplete( status, KErrNone );
                    return;
                    }
                else
                    {
                    iDiskFull = EFalse;
                    iObserver.HandleDiskSpaceNotificationL( iDiskFull );
                    }
                StartNotifier();
                break;

            case KErrArgument:
                TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::GetDriveNumberL() KErrArgument");
                User::Leave( status );
                break;
            default:
                break;
            }
        }
    else if ( iState == CThumbnailStoreDiskSpaceNotifierAO::EIterate )
        {
        const TInt KMaxIterations = 10;
        
        iFileServerSession.Volume( volumeInfo, iDrive );
        if ( volumeInfo.iFree < iThreshold )
            {
            iObserver.HandleDiskSpaceNotificationL( iDiskFull );
            ++iIterationCount;
            if ( iIterationCount < KMaxIterations )
                {
                SetActive();
                TRequestStatus* status = &iStatus;
                User::RequestComplete( status, KErrNone );
                return;
                }
            else
                {
                iFileServerSession.Volume( volumeInfo, iDrive );
                if ( volumeInfo.iFree >= iThreshold )
                    {
                    iDiskFull = EFalse;
                    }
                }
            }
        else
            {
            iDiskFull = EFalse;
            }
        iState = ENormal;
        iIterationCount = 0;
        StartNotifier();            
        }
    else
        {
        User::Leave( KErrGeneral );
        }
    }

TInt CThumbnailStoreDiskSpaceNotifierAO::RunError(TInt aError)
    {
    TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::RunError()");
    iObserver.HandleDiskSpaceError( aError );
    
    return KErrNone;
    }

void CThumbnailStoreDiskSpaceNotifierAO::DoCancel()
    {
    TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::DoCancel()");
    if( IsActive() )
        {   
        iFileServerSession.NotifyDiskSpaceCancel();
        }
    }

CThumbnailStoreDiskSpaceNotifierAO::CThumbnailStoreDiskSpaceNotifierAO(
    MThumbnailStoreDiskSpaceNotifierObserver& aObserver, TInt64 aThreshold, const TDriveNumber aDrive)
    : CActive( CActive::EPriorityStandard ), 
    iObserver( aObserver ), iThreshold( aThreshold ), iDrive( aDrive ), iState( CThumbnailStoreDiskSpaceNotifierAO::ENormal ), iDiskFull( EFalse )
    {
    TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::CThumbnailStoreDiskSpaceNotifierAO()");
    CActiveScheduler::Add( this );
    }

void CThumbnailStoreDiskSpaceNotifierAO::ConstructL()
    {   
    TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::ConstructL()");
    TInt KMessageSlotCount = 2; // slots for NotifyDiskSpace and NotifyDiskSpaceCancel

    User::LeaveIfError( iFileServerSession.Connect( KMessageSlotCount ) );
    
    TVolumeInfo volumeInfo;
    iFileServerSession.Volume( volumeInfo, iDrive );    
    if ( volumeInfo.iFree < iThreshold )
        {
        iDiskFull = ETrue;
        }

    iObserver.HandleDiskSpaceNotificationL( iDiskFull );
    
    StartNotifier();
    }

void CThumbnailStoreDiskSpaceNotifierAO::StartNotifier()
    {   
    TN_DEBUG2( "CThumbnailStoreDiskSpaceNotifierAO::StartNotifier() iDrive == %d", iDrive);
    iFileServerSession.NotifyDiskSpace( iThreshold, iDrive, iStatus );
    
    SetActive();
    }

TBool CThumbnailStoreDiskSpaceNotifierAO::DiskFull() const
    {
    return iDiskFull;
    }

// End of file
