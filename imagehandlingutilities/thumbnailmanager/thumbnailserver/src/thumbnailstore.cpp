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
#include <bautils.h>  
#include <IclExtJpegApi.h>

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
_LIT( KThumbnailDatabaseName, ":[102830AB]thumbnail_v3.db" );

_LIT( KDrv, ":");

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
#ifdef _DEBUG
    TPtrC errorMsg = iDatabase.LastErrorMessage();
    TN_DEBUG3( "RThumbnailTransaction::BeginL() lastError %S, ret = %d" , &errorMsg, err);
#endif
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
    TInt ret = iDatabase.Exec( KThumbnailCommitTransaction );
    
#ifdef _DEBUG
    TPtrC errorMsg = iDatabase.LastErrorMessage();
    TN_DEBUG3( "RThumbnailTransaction::CommitL() lastError %S, ret = %d" , &errorMsg, ret);
#endif  
    User::LeaveIfError( ret );
    
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
    
    if(iActivityManager)
        {
        delete iActivityManager;
        iActivityManager = NULL;
        }
    
    if(iDiskFullNotifier)
        {
        delete iDiskFullNotifier;
        iDiskFullNotifier = NULL;
        }

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

    CleanupStack::PopAndDestroy( databasePath );
    
    OpenDatabaseL();
             
    // to monitor device activity
    iActivityManager = CTMActivityManager::NewL( this, KStoreMaintenanceIdle);
    iActivityManager->Start();
    
    // once in every mount
    iDeleteThumbs = ETrue;
    iCheckFilesExist = ETrue;
    iLastCheckedRowID = -1;
    }

// ---------------------------------------------------------------------------
// OpenDatabaseL database file
// ---------------------------------------------------------------------------
TInt CThumbnailStore::OpenDatabaseFileL()
    {
    TN_DEBUG1( "CThumbnailStore::OpenDatabaseFile()" );
    HBufC* databasePath = HBufC::NewLC( KMaxFileName );
    TPtr pathPtr = databasePath->Des();
    TChar driveChar = 0;
    User::LeaveIfError( RFs::DriveToChar( iDrive, driveChar ));
    pathPtr.Append( driveChar );
    pathPtr.Append( KThumbnailDatabaseName );
    
    TInt ret = iDatabase.Open( pathPtr );
    CleanupStack::PopAndDestroy( databasePath );
    return ret;
    }

// ---------------------------------------------------------------------------
// OpenDatabaseL database
// ---------------------------------------------------------------------------
TInt CThumbnailStore::OpenDatabaseL()
    {
    TN_DEBUG1( "CThumbnailStore::OpenDatabaseL()" );
        
    iDatabase.Close();
    
    TBool newDatabase(EFalse);
    TInt error = KErrNone;
    
    TInt err = OpenDatabaseFileL();
   
   if ( err == KErrNotFound )
       {
       // db not found, create new
       RecreateDatabaseL( EFalse);
       newDatabase = ETrue;
       err = KErrNone;
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
   
   TN_DEBUG3( "CThumbnailStore::ConstructL() -- error = %d, err = %d", error, err);
   
   // if wrong version, corrupted database or other error opening db
   if ( error == KErrNotSupported || (err != KErrNone && err != KErrNotFound) )
       {
       RecreateDatabaseL( ETrue);
       }
   else if(!newDatabase)
       {
       if(ResetThumbnailIDs() == KSqlErrCorrupt)
           {
           RecreateDatabaseL( ETrue);
           }
       
       //check ownership
       error = CheckImeiL();
       
       if(error != KErrNone)
           {
           if(error == KSqlErrCorrupt)
               {
               RecreateDatabaseL( ETrue);
               }
           //take ownership
           error = UpdateImeiL();
           
           if(error == KSqlErrCorrupt)
               {
               RecreateDatabaseL( ETrue);
               }
           
           //Touch blacklisted items
           TRAP(error, PrepareBlacklistedItemsForRetryL( ) );
           
           if(error == KSqlErrCorrupt)
               {
               RecreateDatabaseL( ETrue);
               }
           }
       
       //check is MMC known
       if(CheckMediaIDL() != KErrNone )
           {
           //Touch blacklisted items
           TRAP(error, PrepareBlacklistedItemsForRetryL() );
           
           if(error == KSqlErrCorrupt)
               {
               RecreateDatabaseL( ETrue);
               }
           }
       }
   
   PrepareDbL();
   return KErrNone;
    }

// ---------------------------------------------------------------------------
// PrepareDbL database tables
// ---------------------------------------------------------------------------
//
void CThumbnailStore::PrepareDbL()
    {
    TN_DEBUG1( "CThumbnailStore::PrepareDbL()" );
    TInt err(KErrNone);
    
    // add tables
    TRAPD(tableError, CreateTablesL() );
      
    if(!tableError)
      {
      TRAPD(err, AddVersionAndImeiL());
      if (err == KSqlErrCorrupt)
          {
          RecreateDatabaseL( ETrue);
          }
      User::LeaveIfError(err);
      }
          
      err = iDatabase.Exec( KThumbnailCreateTempInfoTable );
#ifdef _DEBUG
  if(err < 0)
      {
       TPtrC errorMsg = iDatabase.LastErrorMessage();
       TN_DEBUG2( "CThumbnailStore::ConstructL() KThumbnailCreateTempInfoTable %S" , &errorMsg);
      }
#endif
    User::LeaveIfError( err );

  err = iDatabase.Exec( KThumbnailCreateTempInfoDataTable );
#ifdef _DEBUG
  if(err < 0)
      {
       TPtrC errorMsg = iDatabase.LastErrorMessage();
       TN_DEBUG2( "CThumbnailStore::ConstructL() KThumbnailCreateTempInfoDataTable %S" , &errorMsg);
      }
#endif
       User::LeaveIfError( err );
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
    
    err = iDatabase.Exec(KThumbnailDeletedTable);
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailDeletedTable err=%d", err );
    
    err = iDatabase.Exec(KThumbnailVersionTable);
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailVersionTable err=%d", err );
    
    err = iDatabase.Exec( KThumbnailCreateInfoTableIndex1 );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateInfoTableIndex1 err=%d", err );

    err = iDatabase.Exec( KThumbnailCreateDeletedTableIndex );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateDeletedTableIndex err=%d", err );
    
    User::LeaveIfError( err );
    }

void CThumbnailStore::RecreateDatabaseL(const TBool aDelete)
    {
    TN_DEBUG1( "CThumbnailStore::RecreateDatabaseL()" );
    
    TVolumeInfo volumeinfo;
    iFs.Volume(volumeinfo, iDrive);
    TUint id = volumeinfo.iUniqueID;
    TBuf<50> mediaid;
    mediaid.Num(id);
    
    // delete db and create new
    iDatabase.Close();
    
    HBufC* databasePath = HBufC::NewLC( KMaxFileName );
    TPtr pathPtr = databasePath->Des();
    TChar driveChar = 0;
    User::LeaveIfError( RFs::DriveToChar( iDrive, driveChar ));
    pathPtr.Append( driveChar );
    pathPtr.Append( KThumbnailDatabaseName );
    
    TInt err(KErrNone);
    
    if(aDelete)
        {
        iDatabase.Delete(pathPtr);
        }
        
    const TDesC8& config = KThumbnailSqlConfig;

    RSqlSecurityPolicy securityPolicy;
    CleanupClosePushL( securityPolicy );
    securityPolicy.Create( KThumbnailDatabaseSecurityPolicy );

    TRAP(err, iDatabase.CreateL( pathPtr, securityPolicy, &config ));
    CleanupStack::PopAndDestroy( &securityPolicy );
    
        
#ifdef _DEBUG
    if(err < 0)
        {
        TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "CThumbnailStore::RecreateDatabaseL() KThumbnailInsertTempThumbnailInfoData %S" , &errorMsg);
        }
#endif
    TN_DEBUG2( "CThumbnailStore::RecreateDatabaseL() -- database created err = %d", err );
    User::LeaveIfError( err );
    CleanupStack::PopAndDestroy( databasePath );
    
    RFile64 file;
    file.Create(iFs, mediaid, EFileShareReadersOrWriters );
    file.Close();
    
    OpenDatabaseFileL();
    }

// ---------------------------------------------------------------------------
// CThumbnailStore::StoreThumbnailL()
// Stores thumbnail image.
// ---------------------------------------------------------------------------
//
void CThumbnailStore::StoreThumbnailL( const TDesC& aPath, const TDes8& aData,
    const TSize& aSize, const TSize& aOriginalSize, const TThumbnailFormat& aFormat, TInt aFlags, 
	const TThumbnailSize& aThumbnailSize, const TInt64 aModified, const TBool aThumbFromPath )
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
    
    TN_DEBUG2( "CThumbnailStore::StoreThumbnailL() timeStamp aModified %Ld", aModified );
        
    if( aModified )
        {
        timeStamp = aModified;
        }
    else
        {

        if (aPath.Length())
            {
            iFs.Modified(aPath, timeStamp);
            TN_DEBUG2( "CThumbnailStore::StoreThumbnailL() timeStamp       iFs %Ld", timeStamp.Int64() );
            }
        else
            {
            // otherwise current time
            timeStamp.UniversalTime();
            TN_DEBUG2( "CThumbnailStore::StoreThumbnailL() timeStamp   current %Ld", timeStamp.Int64() );
            }
        }
        
   TN_DEBUG2( "CThumbnailStore::StoreThumbnailL() timeStamp       set %Ld", timeStamp.Int64());
   
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
    if(err < 0)
        {
        TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "CThumbnailStore::StoreThumbnailL() KThumbnailInsertTempThumbnailInfoData %S" , &errorMsg);
        }
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
    const TInt64 aModified, TBool aThumbFromPath, TBool aBlackListed )
    {
    TSize thumbSize = aThumbnail->SizeInPixels();
    TN_DEBUG4( "CThumbnailStore::StoreThumbnailL( CFbsBitmap ) aThumbnailSize = %d, aThumbnailSize(%d,%d) IN", aThumbnailSize, thumbSize.iWidth, thumbSize.iHeight );

    __ASSERT_DEBUG(( aThumbnail ), ThumbnailPanic( EThumbnailNullPointer ));
    
    // don't store custom/unknown sizes or zero sizes
    if(aThumbnailSize == ECustomThumbnailSize || aThumbnailSize == EUnknownThumbnailSize 
            || thumbSize.iWidth <= 0 || thumbSize.iHeight <= 0 )
        {
        TN_DEBUG1( "CThumbnailStore::StoreThumbnailL() not stored");
        return;
        }
    
    HBufC* path = aPath.AllocLC();
    TPtr ptr(path->Des());
    StripDriveLetterL( ptr );

    // check for duplicates
    TBool exists = FindDuplicateL(*path, aThumbnailSize);
    
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
                  StoreThumbnailL( *path, ptr, aThumbnail->SizeInPixels(), aOriginalSize,
                          EThumbnailFormatJpeg, flags, aThumbnailSize, aModified, aThumbFromPath  );
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
            
                StoreThumbnailL( *path, buf->Ptr( 0 ), aThumbnail->SizeInPixels(),
                    aOriginalSize, EThumbnailFormatFbsBitmap, flags, aThumbnailSize, aModified);
  
                CleanupStack::PopAndDestroy( buf );
                }
            
            break;
            }
        }
    
    CleanupStack::PopAndDestroy( path );
    
    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( CFbsBitmap* ) out" );
    }

// ---------------------------------------------------------------------------
// Finds possible existing duplicate thumbnail.
// ---------------------------------------------------------------------------
//
TBool CThumbnailStore::FindDuplicateL( const TDesC& aPath, const TThumbnailSize& aThumbnailSize )
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
    
    // check if duplicate in Deleted
    if (found)
        {
        stmt.Close();
        CleanupStack::PopAndDestroy( &stmt );
        CleanupClosePushL( stmt );     
            
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSqlFindDeleted ));
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindText( paramIndex, aPath ));
        
        rowStatus = stmt.Next();
        
        if(rowStatus == KSqlAtRow)
            {
            TN_DEBUG1( "CThumbnailStore::FindDuplicateL() - duplicate marked deleted" );
            
            DeleteThumbnailsL(aPath, ETrue);
            
            TN_DEBUG1( "CThumbnailStore::FindDuplicateL() - duplicate force-deleted" );
            
            found = EFalse;
            }
        }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    
    return found;
    }

// ---------------------------------------------------------------------------
// Get missing sizes by Path
// ---------------------------------------------------------------------------
//     
void CThumbnailStore::GetMissingSizesL( const TDesC& aPath, TInt aSourceType, RArray <
    TThumbnailPersistentSize > & aMissingSizes, TBool aCheckGridSizeOnly )
    {
    TN_DEBUG2( "CThumbnailStore::GetMissingSizesL() aSourceType == %d", aSourceType );
    
    HBufC* path = aPath.AllocLC();
    TPtr ptr(path->Des());
    StripDriveLetterL( ptr );
    
    // define sizes to be checked
    const TInt count = iPersistentSizes.Count();
    
    for ( TInt i = 0 ; i < count; i++ )
        {
        if ( iPersistentSizes[ i ].iSourceType == aSourceType && iPersistentSizes[ i ].iAutoCreate)
            {
            //if generating only grid size for image or video, other sizes are not missing
            if( aCheckGridSizeOnly )
			{
				if( (iPersistentSizes[i].iSourceType == TThumbnailPersistentSize::EImage || iPersistentSizes[i].iSourceType == TThumbnailPersistentSize::EVideo )&&
                     iPersistentSizes[i].iSizeType != TThumbnailPersistentSize::EGrid )
                    {
                    TN_DEBUG4( "CThumbnailStore::GetMissingSizesL() skip, aCheckGridSizeOnly = %d and  iPersistentSizes[%d].iSizeType == %d", 
                            aCheckGridSizeOnly, i, iPersistentSizes[i].iSizeType );
                    }
				else
				    {
                    aMissingSizes.Append( iPersistentSizes[ i ] );
				    }
			}
            else
                {
                aMissingSizes.Append( iPersistentSizes[ i ] );
                }
            }
        }
    
    TInt missingSizeCount = aMissingSizes.Count();
        
    TN_DEBUG2( "CThumbnailStore::GetMissingSizesL() missingSizeCount == %d", missingSizeCount );
    
    // check temp table first
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempSizeByPath ));
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, *path ));
       
    TInt rowStatus = stmt.Next();

    TInt round = 1;
    TInt size = 0;
    
    while (round <= 2)
        {
        while ( rowStatus == KSqlAtRow && missingSizeCount > 0 )
            {
            size = stmt.ColumnInt( 0 );
			
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
            CleanupStack::PopAndDestroy( path );
            return;
            }
        else if (round == 1)
            {
            // change to real table
            CleanupClosePushL( stmt );
            User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectSizeByPath ));
            paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmt.BindText( paramIndex, *path ));
            rowStatus = stmt.Next();    
            }
        
        round++;
        }
    
    CleanupStack::PopAndDestroy( path );
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
    
    HBufC* path = aPath.AllocLC();
    TPtr ptr(path->Des());
    StripDriveLetterL( ptr );
    
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
    TN_DEBUG2( "CThumbnailStore::FetchThumbnailL() KThumbnailSelectTempInfoByPath %S" , &errorMsg);
#endif
    User::LeaveIfError( err );
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, *path ));
    
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
        User::LeaveIfError( stmt.BindText( paramIndex, *path ));
        
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
        if( flags & KThumbnailDbFlagBlacklisted && (*path).Length() )
            {
            TBool modified = EFalse;
            CheckModifiedByPathL( aPath, inTempTable, modified );
            if( modified )
                {
                // Close db to get deletion of thumbnails executed.
                stmt.Close();
                CleanupStack::PopAndDestroy( &stmt );
                DeleteThumbnailsL( *path );
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
    CleanupStack::PopAndDestroy( path );
    
    User::LeaveIfError( found );
    }

// -----------------------------------------------------------------------------
// Delete thumbnails for given object file by Path
// -----------------------------------------------------------------------------
//
void CThumbnailStore::DeleteThumbnailsL( const TDesC& aPath, TBool aForce, 
                                         TBool aTransaction )
    {
    TN_DEBUG2( "CThumbnailStore::DeleteThumbnailsL(%S)", &aPath );
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    TInt paramIndex = 0;
    TInt paramIndex1 = 0;
    TInt paramIndex2 = 0;
    TInt rowStatus = 0;
    TInt column = 0;
    TInt64 rowid = 0;
    TInt count = 0;

    HBufC* path = aPath.AllocLC();
    TPtr ptr(path->Des());
    StripDriveLetterL( ptr );
    
    RThumbnailTransaction transaction( iDatabase );
    if (aTransaction)
        {
        CleanupClosePushL( transaction );    
        transaction.BeginL();
        }
        
    RSqlStatement stmt;
    RSqlStatement stmt_info;
    RSqlStatement stmt_infodata;
    
    CleanupClosePushL( stmt );
    CleanupClosePushL( stmt_info );
    CleanupClosePushL( stmt_infodata );
        
    TN_DEBUG1( "CThumbnailStore::DeleteThumbnailsByPathL() -- TEMP TABLE lookup" );
    
    User::LeaveIfError( stmt.Prepare( iDatabase, KTempThumbnailSqlSelectRowIDInfoByPath) );
    User::LeaveIfError( stmt_info.Prepare( iDatabase, KTempThumbnailSqlDeleteInfoByPath) );
    User::LeaveIfError( stmt_infodata.Prepare( iDatabase, KTempThumbnailSqlDeleteInfoDataByPath) );
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, *path ));
    
    rowStatus = stmt.Next();
    
    while(rowStatus == KSqlAtRow)
        {  
        rowid = stmt.ColumnInt64( column ); 
        paramIndex1 = stmt_info.ParameterIndex( KThumbnailSqlParamRowID );
        User::LeaveIfError( paramIndex1 );
        User::LeaveIfError( stmt_info.BindInt64( paramIndex1, rowid ));
       
        TInt err = stmt_info.Exec();
        stmt_info.Reset();
        User::LeaveIfError( err );
             
        paramIndex2 = stmt_infodata.ParameterIndex( KThumbnailSqlParamRowID );
        User::LeaveIfError( paramIndex2 );
        User::LeaveIfError( stmt_infodata.BindInt64( paramIndex2, rowid ));
             
        err = stmt_infodata.Exec();
        stmt_infodata.Reset();
        User::LeaveIfError( err );
        
        TN_DEBUG1( "CThumbnailStore::DeleteThumbnailsByPathL() -- TEMP TABLE lookup - thumbnail deleted" );
       
        // fetch another row (temp table rowIDs are updated immediately)
        stmt.Reset();
       
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindText( paramIndex, *path ));
       
        rowStatus = stmt.Next();   
        }
    
    stmt_infodata.Close();
    stmt_info.Close();
    stmt.Close();
    
    CleanupStack::PopAndDestroy( &stmt_infodata );
    CleanupStack::PopAndDestroy( &stmt_info );
    CleanupStack::PopAndDestroy( &stmt );
    
    // if forcing instant delete
    if (aForce)
        {
        //look from real table 
        TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- MAIN TABLE lookup" );
        
        CleanupClosePushL( stmt );
        CleanupClosePushL( stmt_info );
        CleanupClosePushL( stmt_infodata );
        
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSqlSelectRowIDInfoByPath ));
        User::LeaveIfError( stmt_info.Prepare( iDatabase, KThumbnailSqlDeleteInfoByPath) );
        User::LeaveIfError( stmt_infodata.Prepare( iDatabase, KThumbnailSqlDeleteInfoDataByPath) );

        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindText( paramIndex, *path ));
             
        rowStatus = stmt.Next();   
           
        while(rowStatus == KSqlAtRow)
            { 
            rowid = stmt.ColumnInt64( column ); 
            paramIndex1 = stmt_info.ParameterIndex( KThumbnailSqlParamRowID );
            User::LeaveIfError( paramIndex1 );
            User::LeaveIfError( stmt_info.BindInt64( paramIndex1, rowid ));
               
            TInt err = stmt_info.Exec();
            stmt_info.Reset();
            User::LeaveIfError( err );
                    
            paramIndex2 = stmt_infodata.ParameterIndex( KThumbnailSqlParamRowID );  
            User::LeaveIfError( paramIndex2 );
            User::LeaveIfError( stmt_infodata.BindInt64( paramIndex2, rowid ));
                    
            err = stmt_infodata.Exec();
            stmt_infodata.Reset();
            User::LeaveIfError( err );
           
            TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- MAIN TABLE lookup - thumbnail deleted" );
            
            rowStatus = stmt.Next();
            }
        
        stmt_infodata.Close();
        stmt_info.Close();
        CleanupStack::PopAndDestroy( &stmt_infodata );
        CleanupStack::PopAndDestroy( &stmt_info );
        } 
    else
        {
        // only add path to deleted table
        CleanupClosePushL( stmt );  
        User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSqlInsertDeleted ) );
        
        paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt.BindText( paramIndex, *path ));
        
        count = stmt.Exec();
        }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );    
    
    if (aTransaction)
        {
        transaction.CommitL();
        CleanupStack::PopAndDestroy( &transaction );
        }
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::DeleteThumbnailByPathL() took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif
    
    // start maintenance if rows in main table were marked
    if (!aForce && count > 0)
        {
        TN_DEBUG2( "CThumbnailStore::DeleteThumbnailByPathL() -- MAIN TABLE lookup - %d rows marked deleted", count);
        
        iDeleteThumbs = ETrue;
        iActivityManager->Start();
        }
    
    CleanupStack::PopAndDestroy( path );
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
    
#ifdef _DEBUG
    if(err_tempinfo < 0)
        {
        TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "CThumbnailStore::FlushCacheTable() lastError %S", &errorMsg);
        }
#endif
    
    if(err_tempinfo == KSqlErrCorrupt || err_tempinfo == KErrCorrupt )
        {
        TRAP_IGNORE(RecreateDatabaseL(ETrue));
        TRAP_IGNORE(OpenDatabaseL());
        return;
        }
    
    TInt err_tempdata = iDatabase.Exec( KThumbnailMoveFromTempDataToMainTable );
    
#ifdef _DEBUG
    if(err_tempdata < 0)
        {
        TPtrC errorMsg2 = iDatabase.LastErrorMessage();
        TN_DEBUG2( "CThumbnailStore::FlushCacheTable() KThumbnailMoveFromTempDataToMainTable %S", &errorMsg2);
        }
#endif
    if(err_tempdata == KSqlErrCorrupt || err_tempdata == KErrCorrupt )
        {
        TRAP_IGNORE(RecreateDatabaseL(ETrue));
        TRAP_IGNORE(OpenDatabaseL());
        return;
        }
    
    
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

       TN_DEBUG1( "CThumbnailStore::CheckImeiL() failed" );
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
    
    if(ret < 0 )
        {
#ifdef _DEBUG
   		 TPtrC errorMsg = iDatabase.LastErrorMessage();
    	TN_DEBUG2( "RThumbnailTransaction::CheckImeiL() lastError %S, ret = %d" , &errorMsg);
#endif
        return ret;
        }
    
    if( imei == iImei )
      {
      return KErrNone;  
      }
    else
      {
      TN_DEBUG1( "CThumbnailStore::CheckImeiL() mismatch" );
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
       TN_DEBUG1( "CThumbnailStore::CheckVersionL() unknown version" );
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
    
    if(ret < 0 )
        {
#ifdef _DEBUG
   		 TPtrC errorMsg = iDatabase.LastErrorMessage();
    	TN_DEBUG2( "RThumbnailTransaction::CheckVersionL() lastError %S, ret = %d" , &errorMsg);
#endif
        return ret;
        }
    
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
TInt CThumbnailStore::ResetThumbnailIDs()
    {
    TN_DEBUG1( "CThumbnailStore::ResetThumbnailIDs()" );

    TInt err = iDatabase.Exec( KTempThumbnailResetIDs );
    TN_DEBUG2( "CThumbnailStore::ResetThumbnailIDs() KThumbnailResetIDs - temp table, err=%d", err );

    if(err < 0)
        {
#ifdef _DEBUG
        TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg);
#endif
        return err;
    }

    err = iDatabase.Exec( KThumbnailResetIDs );
    TN_DEBUG2( "CThumbnailStore::ResetThumbnailIDs() KThumbnailResetIDs - main table, err=%d", err );
    
	if(err < 0)
        {
#ifdef _DEBUG
        TPtrC errorMsg2 = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg2);
#endif
        return err;
        }
	return KErrNone;
    }


// -----------------------------------------------------------------------------
// UpdateImeiL()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::UpdateImeiL()
    {
    TN_DEBUG1( "CThumbnailStore::UpdateImeiL()" );
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
         
      
    TInt ret = stmt.Prepare( iDatabase, KThumbnailUpdateIMEI );
    
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamImei );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, iImei ));
    
    TInt err =  stmt.Exec();
    
    if(err < 0)
        {
#ifdef _DEBUG
        TPtrC errorMsg2 = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg2);
#endif
        return ret;
        }
    
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    return KErrNone;
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
        iAutoFlushTimer->Start( KAutoFlushTimeout * 1000000, KAutoFlushTimeout * 1000000, 
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

// -----------------------------------------------------------------------------
// StartMaintenance()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::StartMaintenance()
    {
    TN_DEBUG1( "CThumbnailStore::StartMaintenance()");
    TInt err = KErrNone;
    
    if( iMaintenanceTimer && iMaintenanceTimer->IsActive() )
        {
        return;
        }
    else if (!iMaintenanceTimer)
        {
        TRAP(err, iMaintenanceTimer = CPeriodic::NewL(CActive::EPriorityIdle) );
        }
    
    if (err != KErrNone && !iMaintenanceTimer)
        {
        TN_DEBUG2( "CThumbnailStore::StartMaintenanceTimer() - Error creating timer (%d)", err );
        }
    else
        {
        iMaintenanceTimer->Start( KStoreMaintenancePeriodic, KStoreMaintenancePeriodic, 
                                  TCallBack(MaintenanceTimerCallBack, this));
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

// ---------------------------------------------------------------------------
// CThumbnailStore::MaintenanceTimerCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbnailStore::MaintenanceTimerCallBack(TAny* aAny)
    {
    CThumbnailStore* self = static_cast<CThumbnailStore*>( aAny );
 
    self->iMaintenanceTimer->Cancel();
    
    if (self->iIdle)
        {
        TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() - maintenance, store %d", self->iDrive);
    
        // thumbmnail deletion
        if (self->iDeleteThumbs)
            {
            TInt deleteCount = 0;
            
            // delete marked rows from database
            TRAPD( err, deleteCount = self->DeleteMarkedL() );
            if (err != KErrNone)
                {
                TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() - cleanup failed, err %d", err);
                return err;
                }
            
            // no more marked rows
            if (deleteCount < KStoreMaintenanceDeleteLimit || deleteCount == 0)
                {
                TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() - cleanup finished, store %d", self->iDrive);
                self->iDeleteThumbs = EFalse;
                }     
            }
        
        // file existance check
        else if (self->iCheckFilesExist)
            {
            TBool finished = EFalse;
        
            TRAPD( err, finished = self->FileExistenceCheckL() );
            if (err != KErrNone)
                {
                TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() - file existance check failed, err %d", err);
                return err;
                }
        
            // all files checked.
            if (finished)
                {
                TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() - file existance check finished, store %d", self->iDrive);
                self->iCheckFilesExist = EFalse;
                }
            }
        
        // next round
        if (self->iIdle && ( self->iDeleteThumbs || self->iCheckFilesExist) )
            {
            self->StartMaintenance();
            }  
        else
            {
            // no need to monitor activity anymore
            self->iActivityManager->Cancel();
            }
        }
    else
        {
        TN_DEBUG1( "CThumbnailStore::MaintenanceTimerCallBack() - device not idle");
        }

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
        TN_DEBUG1( "CThumbnailStore::CheckRowIDs() failed 1 %d");
        return KErrNotSupported;
        }
    rowStatus = stmt.Next();
                
    if ( rowStatus == KSqlAtRow)    
        {        
        inforows = stmt.ColumnInt64( column );  
        }
                
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    
    if(ret < 0)
        {
#ifdef _DEBUG
		TPtrC errorMsg2 = iDatabase.LastErrorMessage();
	    TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg2);
#endif
        return ret;
        }
            
    CleanupClosePushL( stmt );
    ret = stmt.Prepare( iDatabase, KGetDataRowID );
    if(ret < 0)
        {
        stmt.Close();
        CleanupStack::PopAndDestroy( &stmt );
        TN_DEBUG1( "CThumbnailStore::CheckRowIDs() failed 2");
        return KErrNotSupported;
        }
    rowStatus = stmt.Next();
                       
    if ( rowStatus == KSqlAtRow)    
        {        
        datarows = stmt.ColumnInt64( column );  
        }
            
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );
    
    if(ret < 0)
        {
#ifdef _DEBUG
		TPtrC errorMsg2 = iDatabase.LastErrorMessage();
	    TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg2);
#endif
        return ret;
        }
            
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

TBool CThumbnailStore::CheckModifiedByPathL( const TDesC& aPath, const TInt64 aModified, TBool& modifiedChanged )
    {
    TN_DEBUG2( "CThumbnailStore::CheckModifiedByPathL() %S", &aPath);
    
    HBufC* path = aPath.AllocLC();
    TPtr ptr(path->Des());
    StripDriveLetterL( ptr );
	
    TBool ret(EFalse);

    modifiedChanged = EFalse;

   TInt column = 0;
   
   RSqlStatement stmt;
   CleanupClosePushL( stmt );
      
   User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectTempModifiedByPath ));
   
   TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
   User::LeaveIfError( paramIndex );
   User::LeaveIfError( stmt.BindText( paramIndex, *path ));
    
   TInt rowStatus = stmt.Next();
   
   TBool checkMain = EFalse;
   
   TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- temp" );
   
   while(rowStatus == KSqlAtRow || !checkMain)
       {
       if(rowStatus == KSqlAtRow)
           {
           ret = ETrue;
           TInt64 oldModified = stmt.ColumnInt64( column );
           
           TN_DEBUG2( "CThumbnailStore::CheckModifiedL() -- timestamp old %Ld", oldModified);
           TN_DEBUG2( "CThumbnailStore::CheckModifiedL() -- timestamp mds %Ld", aModified);
          
           if (oldModified < aModified)
               {
               TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- timestamp is newer than original" );
               modifiedChanged = ETrue;
               break;
               }
           else if (oldModified > aModified)
               {
               TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- timestamp is older than original" );
               }
           else if (oldModified == aModified)
               {
               TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- timestamp is the same as original" );
               }
            }
           
       rowStatus = stmt.Next();
       
       //switch to main table if modified not found from temp
       if(rowStatus != KSqlAtRow && !checkMain && !modifiedChanged)
           {
           TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- main" );
           //come here only once
           checkMain = ETrue;
           
           stmt.Close();
           CleanupStack::PopAndDestroy( &stmt );
           CleanupClosePushL( stmt );
           
           User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectModifiedByPath ));
           
           paramIndex = stmt.ParameterIndex( KThumbnailSqlParamPath );
           User::LeaveIfError( paramIndex );
           User::LeaveIfError( stmt.BindText( paramIndex, *path ));
            
           rowStatus = stmt.Next();
           }
       }
    
   stmt.Close();
   CleanupStack::PopAndDestroy( &stmt ); 
   
   CleanupStack::PopAndDestroy( path );
   
   return ret;
}
	
// -----------------------------------------------------------------------------
// PrepareBlacklistedItemsForRetryL()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::PrepareBlacklistedItemsForRetryL()
    {
    TN_DEBUG1( "CThumbnailStore::PrepareBlacklistedItemsForRetry()" );
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );

    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailTouchBlacklistedRows ));
    
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamFlag );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, KThumbnailDbFlagBlacklisted ));
    TInt err = stmt.Exec();
   
    TN_DEBUG2( "CThumbnailStore::PrepareBlacklistedItemsForRetryL() - main table, err=%d", err );
    
    CleanupStack::PopAndDestroy( &stmt );
    }

// -----------------------------------------------------------------------------
// DeleteMarkedL()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::DeleteMarkedL()
    {
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    TN_DEBUG1( "CThumbnailStore::DeleteMarkedL()" );
    
    TInt paramIndex = 0;
    TInt paramIndex1 = 0;
    TInt paramIndex2 = 0;
    TInt rowStatus = 0;
    TInt column = 0;
    TInt64 rowid = 0;
    TInt deleteCount = 0;
      
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );
    transaction.BeginL();
    
    RSqlStatement stmt;
    RSqlStatement stmt_info;
    RSqlStatement stmt_infodata;
    CleanupClosePushL( stmt );
    
    // select marked rows
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSqlSelectMarked ));
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamLimit );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, KStoreMaintenanceDeleteLimit ));
             
    rowStatus = stmt.Next();  
    
    CleanupClosePushL( stmt_info );
    User::LeaveIfError( stmt_info.Prepare( iDatabase, KThumbnailSqlDeleteInfoByRowID) );
    CleanupClosePushL( stmt_infodata );
    User::LeaveIfError( stmt_infodata.Prepare( iDatabase, KThumbnailSqlDeleteInfoDataByRowID) );    
           
    while(rowStatus == KSqlAtRow)
       { 
       rowid = stmt.ColumnInt64( column ); 
       paramIndex1 = stmt_info.ParameterIndex( KThumbnailSqlParamRowID );
       User::LeaveIfError( paramIndex1 );
       User::LeaveIfError( stmt_info.BindInt64( paramIndex1, rowid ));
              
       TInt err = stmt_info.Exec();
       stmt_info.Reset();
       User::LeaveIfError( err );
                    
       paramIndex2 = stmt_infodata.ParameterIndex( KThumbnailSqlParamRowID );  
       User::LeaveIfError( paramIndex2 );
       User::LeaveIfError( stmt_infodata.BindInt64( paramIndex2, rowid ));
                    
       err = stmt_infodata.Exec();
       stmt_infodata.Reset();
       User::LeaveIfError( err );
       deleteCount++;
       
       TN_DEBUG1( "CThumbnailStore::DeleteMarkedL() - thumbnail deleted" );
       
       rowStatus = stmt.Next();
       }
        
    stmt_infodata.Close();
    stmt_info.Close();
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt_infodata );
    CleanupStack::PopAndDestroy( &stmt_info );
    CleanupStack::PopAndDestroy( &stmt );    
    
    // remove successfully deleted paths from Deleted table
    if (deleteCount > 0)
        {
        CleanupClosePushL( stmt );  
        User::LeaveIfError( iDatabase.Exec( KThumbnailSqlDeleteFromDeleted ) ); 
        
        stmt.Close();
        CleanupStack::PopAndDestroy( &stmt );
        }
    
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::DeleteMarkedL() took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif
    
    return deleteCount;
    }

// -----------------------------------------------------------------------------
// FileExistenceCheckL()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::FileExistenceCheckL()
    {
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    TN_DEBUG1( "CThumbnailStore::FileExistenceCheckL()" );
    
    TInt paramIndex = 0;
    TInt rowStatus = 0; 
    TInt column = 0;
    TInt64 rowid = 0;
    TFileName path;
    TFileName prevPath;
    TFileName full;
    TInt count = 0;
    
    TBool finished = EFalse;
    
    TChar dChar = 0;
    User::LeaveIfError( iFs.DriveToChar( iDrive, dChar ));
    
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );    
    transaction.BeginL();
    
    RSqlStatement stmt;
    CleanupClosePushL( stmt );
    
    // get rows
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailSelectAllPaths ));
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamRowID );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt64( paramIndex, iLastCheckedRowID ));
    
    paramIndex = stmt.ParameterIndex( KThumbnailSqlParamLimit );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindInt( paramIndex, KStoreMaintenanceExistLimit ));
             
    rowStatus = stmt.Next();     
           
    while(rowStatus == KSqlAtRow)
        {
        column = 0;
        path.Zero();
        
        rowid = stmt.ColumnInt64( column++ );
        stmt.ColumnText( column, path );
    
        full.Zero();
        full.Append(dChar);
        full.Append(KDrv);
        full.Append(path);
        
        // if path matches previous one, skip
        if (path.CompareF(prevPath) != 0)
            {
            // file doesn't exist anymore, mark thumbs deleted
            if(!BaflUtils::FileExists( iFs, full ))
                {
                TN_DEBUG2( "CThumbnailStore::FileExistenceCheckL(%S) - not found", &full );
                DeleteThumbnailsL(path, EFalse, EFalse);                    
                }
            
            prevPath.Zero();
            prevPath.Append(path);
            }
        
        iLastCheckedRowID = rowid;
        count++;
       
        // get next
        rowStatus = stmt.Next();
        }
    
    if (count < KStoreMaintenanceExistLimit)
        {
        // all rows checked
        finished = ETrue;
        }
        
    stmt.Close();
    CleanupStack::PopAndDestroy( &stmt );    
    
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::FileExistenceCheckL() took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif
    
    return finished;
    }

// -----------------------------------------------------------------------------
// StripDriveLetter
// -----------------------------------------------------------------------------
//
void CThumbnailStore::StripDriveLetterL( TDes& aPath )
    {
    TInt pos = aPath.Find(KDrv);
    
    // if URI contains drive letter
    if ( pos == 1 )
        {
        aPath.Delete(0,pos+1);
        }
    }

void CThumbnailStore::HandleDiskSpaceNotificationL( TBool aDiskFull )
    {
    TN_DEBUG2( "CThumbnailStore::HandleDiskSpaceNotificationL() aDiskFull = %d", aDiskFull );
    iDiskFull = aDiskFull;
    }

#ifdef _DEBUG
void CThumbnailStore::HandleDiskSpaceError(TInt aError )
#else
void CThumbnailStore::HandleDiskSpaceError(TInt /*aError*/ )
#endif
    {
    TN_DEBUG2( "CThumbnailStore::HandleDiskSpaceError() aError = %d", aError );
    }

TBool CThumbnailStore::IsDiskFull()
    {
    return iDiskFull;
    }

// -----------------------------------------------------------------------------
// ActivityDetected()
// -----------------------------------------------------------------------------
//
void CThumbnailStore::ActivityChanged(const TBool aActive)
    {
    TN_DEBUG2( "CThumbnailStore::ActivityChanged() aActive == %d", aActive);
    
    if( aActive )
        {
        iIdle = EFalse;
        }
    else
        {
        TInt MPXHarvesting(0);
        TInt DaemonProcessing(0);
        TInt ret = RProperty::Get(KTAGDPSNotification, KMPXHarvesting, MPXHarvesting);
        if(!ret)
            return;
        
        TN_DEBUG2( "CThumbnailStore::ActivityChanged() KMPXHarvesting == %d", KMPXHarvesting);
        
        ret = RProperty::Get(KTAGDPSNotification, KDaemonProcessing, DaemonProcessing);
        
        if(!ret)
            return;
        
        TN_DEBUG2( "CThumbnailStore::ActivityChanged() DaemonProcessing == %d", DaemonProcessing);
        
        if(!MPXHarvesting && !DaemonProcessing)
            {
            TN_DEBUG1( "CThumbnailStore::ActivityChanged() - starting maintenance");
            iIdle = ETrue;
            StartMaintenance();
            }
        }
    }

// CThumbnailStoreDiskSpaceNotifierAO class

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
        
        TInt ret(KErrNone);
        
        switch( status )
            {
            case KErrNone:
                ret = iFileServerSession.Volume( volumeInfo, iDrive );
                
                if(!ret)
                    {
                    
                    // Check if free space is less than threshold level
                    if( volumeInfo.iFree < iThreshold )
                        {
                        TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::RunL() FULL");
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
                        TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::RunL() NOT FULL");
                        iDiskFull = EFalse;
                        iObserver.HandleDiskSpaceNotificationL( iDiskFull );
                        }
                    }
                else
                    {
                    TN_DEBUG2( "CThumbnailStoreDiskSpaceNotifierAO::RunL() error %d NOT FULL", ret);
                    iDiskFull = EFalse;
                    iObserver.HandleDiskSpaceNotificationL( iDiskFull );
                    User::Leave( ret );
                    }
                
                StartNotifier();
                break;

            case KErrArgument:
                TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::RunL() KErrArgument");
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
                    TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::RunL() NOT FULL");
                    iDiskFull = EFalse;
                    }
                }
            }
        else
            {
            TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::RunL() NOT FULL");
            iDiskFull = EFalse;
            }
        iState = ENormal;
        iIterationCount = 0;
        StartNotifier();            
        }
    else
        {
        TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::RunL() KErrGeneral");
        User::Leave( KErrGeneral );
        }
    }

TInt CThumbnailStoreDiskSpaceNotifierAO::RunError(TInt aError)
    {
    TN_DEBUG2( "CThumbnailStoreDiskSpaceNotifierAO::RunError() %d", aError);
    
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
    TInt ret = iFileServerSession.Volume( volumeInfo, iDrive );
    
    if( !ret )
        {
        if ( volumeInfo.iFree < iThreshold )
            {
            TN_DEBUG1( "CThumbnailStoreDiskSpaceNotifierAO::ConstructL() FULL");
            iDiskFull = ETrue;
            }
        }
    else
        {
        TN_DEBUG2( "CThumbnailStoreDiskSpaceNotifierAO::ConstructL() error %d NOT FULL", ret);
        iDiskFull = EFalse;
        User::Leave( ret );
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
