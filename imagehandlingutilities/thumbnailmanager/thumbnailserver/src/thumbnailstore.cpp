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
#include <ExifRead.h>
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


_LIT8( KThumbnailSqlConfig, "page_size=32768; cache_size=64;" );

const TInt KStreamBufferSize = 1024 * 8;
const TInt KMajor = 3;
const TInt KMinor = 2;

const TInt KStoreUnrecoverableErr = KErrCorrupt;

// Database path without drive letter
//Symbian^4 v5
_LIT( KThumbnailDatabaseName, ":[102830AB]thumbnail_v5.db" );


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
    if (iDatabase.InTransaction())
        {
        TN_DEBUG1( "RThumbnailTransaction::BeginL() - error: old transaction open!" );
        __ASSERT_DEBUG(( !iDatabase.InTransaction() ), ThumbnailPanic( EThumbnailSQLTransaction ));
        
        // old transaction already open, don't open another
        iState = EOldOpen;
        
        return;
        }
    
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
    if ( iState != EClosed && iState != EOldOpen )
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
    if ( iState != EOldOpen )
        {
        TInt ret = iDatabase.Exec( KThumbnailCommitTransaction );
    
#ifdef _DEBUG
    TPtrC errorMsg = iDatabase.LastErrorMessage();
    TN_DEBUG3( "RThumbnailTransaction::CommitL() lastError %S, ret = %d" , &errorMsg, ret);
#endif  
    User::LeaveIfError( ret );
        }
    
    iState = EClosed;
    }

// ---------------------------------------------------------------------------
// RThumbnailTransaction::Rollback()
// ---------------------------------------------------------------------------
//
TInt RThumbnailTransaction::Rollback()
    {
    if ( iState != EOldOpen )
        {
        // in some cases there could have been automatic rollback
        if (iDatabase.InTransaction())
            {
            const TInt err = iDatabase.Exec( KThumbnailRollbackTransaction );
            if ( err >= 0 )
                {
                iState = EClosed;
                }
            
            return err;
            }
        else
            {
            TN_DEBUG1( "RThumbnailTransaction::Rollback() - automatic rollback already done!" );
            }
        }
    
    iState = EClosed;
    
    return KErrNone;
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
    TN_DEBUG2( "CThumbnailStore::~CThumbnailStore() drive: %d", iDrive );
    
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

    if( iAutoFlushTimer )
        {
        iAutoFlushTimer->Cancel();
        delete iAutoFlushTimer;
        iAutoFlushTimer = NULL;
        }
    
    if( iMaintenanceTimer )
        {
        iMaintenanceTimer->Cancel();
        delete iMaintenanceTimer;
        iMaintenanceTimer = NULL;
        }
    
    CloseStatements();   
    iDatabase.Close();
    
    TN_DEBUG1( "CThumbnailStore::~CThumbnailStore() - database closed" );
    }

// ---------------------------------------------------------------------------
// CThumbnailStore::CThumbnailStore()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailStore::CThumbnailStore( RFs& aFs, TInt aDrive, TDesC& aImei, CThumbnailServer* aServer ): 
    iFs( aFs ), iDrive( aDrive ), iDriveChar( 0 ), iBatchItemCount(0), iImei(aImei), 
    iServer(aServer), iDiskFull(EFalse), iUnrecoverable(ETrue), iBatchFlushItemCount(KMInBatchItems)
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
    User::LeaveIfError( RFs::DriveToChar( iDrive, iDriveChar ));
    pathPtr.Append( iDriveChar );
    pathPtr.Append( KThumbnailDatabaseName );
    
	//start disk space monitor
    iDiskFullNotifier = CThumbnailStoreDiskSpaceNotifierAO::NewL( *this, 
                                            KDiskFullThreshold,
                                            pathPtr );

    CleanupStack::PopAndDestroy( databasePath );
    
    TN_DEBUG2( "CThumbnailStore::ConstructL() drive: %d", iDrive );
    
    OpenDatabaseL();
    
    // to monitor device activity
    iActivityManager = CTMActivityManager::NewL( this, KStoreMaintenanceIdle);
    iActivityManager->Start();
    
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
TInt CThumbnailStore::OpenDatabaseL( TBool aNewDatabase )
    {
    TN_DEBUG2( "CThumbnailStore::OpenDatabaseL() drive: %d", iDrive );
        
    CloseStatements();
    iDatabase.Close();
    iUnrecoverable = ETrue;
    
    TInt checkError = KErrNone;
    TInt blistError = KErrNone;
    TInt blistError2 = KErrNone;
    TInt imeiError = KErrNone;
    TInt err = KErrNone;
    
    if (aNewDatabase)
        {
        // delete existing and create new
        CleanupClosePushL(iDatabase);
        RecreateDatabaseL(ETrue);
        CleanupStack::Pop(&iDatabase);
        }
    else
        {
        // just open old
        err = OpenDatabaseFileL();
        
        TN_DEBUG2( "CThumbnailStore::OpenDatabaseL() -- err = %d", err);
               
        if ( err == KErrNone)
            {
            // db found, check version and rowids
            checkError = CheckVersion();
            if(checkError == KErrNone)
                {
                checkError = CheckRowIDs();
                }
            }

        // if db file not found, wrong version, corrupted database or other error opening db
        if ( err != KErrNone || checkError == KErrNotSupported )
            {
            CleanupClosePushL(iDatabase);
            RecreateDatabaseL(ETrue);
            CleanupStack::Pop(&iDatabase);
            
            aNewDatabase = ETrue;
            }          
        }    
   
    // opened existing database file
    if(!aNewDatabase)
        {       
        // add temp tables
        CreateTempTablesL();
    
        //check ownership
        imeiError = CheckImei();
       
        if(imeiError != KErrNone)
            {
            //take ownership
            TRAP(imeiError, UpdateImeiL() );
            
            //Touch blacklisted items
            TRAP(blistError, PrepareBlacklistedItemsForRetryL() );
            }
       
        //check if MMC is known
        if(CheckMediaIDL() != KErrNone)
            {
            //Touch blacklisted items
            TRAP(blistError2, PrepareBlacklistedItemsForRetryL() );
            }
        
        if(imeiError != KErrNone || blistError != KErrNone || blistError2 != KErrNone )
            {
            CleanupClosePushL(iDatabase);
            RecreateDatabaseL(ETrue);
            CleanupStack::Pop(&iDatabase);
            }
        }
   
    PrepareStatementsL();
    
    // database now usable
    iUnrecoverable = EFalse;
    
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// PrepareDbL database tables
// ---------------------------------------------------------------------------
//
void CThumbnailStore::PrepareDbL()
    {
    TN_DEBUG1( "CThumbnailStore::PrepareDbL()" );
    
    // add persistent tables
    CreateTablesL();
      
    AddVersionAndImeiL();
    
    // add temp tables
    CreateTempTablesL();
    }

// ---------------------------------------------------------------------------
// Create database tables
// ---------------------------------------------------------------------------
//
void CThumbnailStore::CreateTablesL()
    {
    TN_DEBUG1( "CThumbnailStore::CreateTablesL()" );
    
    TInt err = KErrNone;
    err = iDatabase.Exec( KThumbnailCreateInfoTable );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateInfoTable err=%d", err );
    User::LeaveIfError( err );
    
    err = iDatabase.Exec( KThumbnailCreateInfoDataTable );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateInfoDataTable err=%d", err );
    User::LeaveIfError( err );
    
    err = iDatabase.Exec(KThumbnailDeletedTable);
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailDeletedTable err=%d", err );
    User::LeaveIfError( err );
    
    err = iDatabase.Exec( KThumbnailCreateInfoTableIndex1 );
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailCreateInfoTableIndex1 err=%d", err );
    User::LeaveIfError( err );
    
    err = iDatabase.Exec(KThumbnailVersionTable);
    TN_DEBUG2( "CThumbnailStore::CreateTablesL() KThumbnailVersionTable err=%d", err );
    User::LeaveIfError( err );
    }

// ---------------------------------------------------------------------------
// Create temp tables
// ---------------------------------------------------------------------------
//
void CThumbnailStore::CreateTempTablesL()
    {
    TN_DEBUG1( "CThumbnailStore::CreateTempTablesL()" );

    TInt err = iDatabase.Exec( KThumbnailCreateTempInfoTable );

#ifdef _DEBUG
    if(err < 0)
        {
        TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "CThumbnailStore::PrepareDbL() KThumbnailCreateTempInfoTable %S" , &errorMsg);
        }
#endif
    
    User::LeaveIfError( err );

    err = iDatabase.Exec( KThumbnailCreateTempInfoDataTable );

#ifdef _DEBUG
    if(err < 0)
        {
        TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "CThumbnailStore::PrepareDbL() KThumbnailCreateTempInfoDataTable %S" , &errorMsg);
        }
#endif
    
    User::LeaveIfError( err );
    }

void CThumbnailStore::RecreateDatabaseL(const TBool aDelete)
    {
    TN_DEBUG2( "CThumbnailStore::RecreateDatabaseL() drive: %d", iDrive );
    
    TVolumeInfo volumeinfo;
    User::LeaveIfError( iFs.Volume(volumeinfo, iDrive) );
    TUint id = volumeinfo.iUniqueID;
    TBuf<50> mediaid;
    mediaid.Num(id);
   
    CloseStatements();
    iDatabase.Close();
    iUnrecoverable = ETrue;
    
    TN_DEBUG1( "CThumbnailStore::RecreateDatabaseL() database closed" );
    
    HBufC* databasePath = HBufC::NewLC( KMaxFileName );
    TPtr pathPtr = databasePath->Des();
    User::LeaveIfError( RFs::DriveToChar( iDrive, iDriveChar ));
    pathPtr.Append( iDriveChar );
    pathPtr.Append( KThumbnailDatabaseName );
    
    TInt err(KErrNone);
    
    // delete old if necessary
    if(aDelete)
        {
        TN_DEBUG1( "CThumbnailStore::RecreateDatabaseL() delete database" );
        TInt del = iDatabase.Delete(pathPtr);     
        TN_DEBUG2( "CThumbnailStore::RecreateDatabaseL() deleted database, err: %d", del );       
        }
        
    const TDesC8& config = KThumbnailSqlConfig;

    RSqlSecurityPolicy securityPolicy;
    CleanupClosePushL( securityPolicy );
    securityPolicy.CreateL( KThumbnailDatabaseSecurityPolicy );

    // create new
    TN_DEBUG1( "CThumbnailStore::RecreateDatabaseL() create new" );
    TRAP(err, iDatabase.CreateL( pathPtr, securityPolicy, &config ));    
    TN_DEBUG2( "CThumbnailStore::RecreateDatabaseL() -- database created, err = %d", err );
    User::LeaveIfError(err);
    
    CleanupStack::PopAndDestroy( &securityPolicy );
    
    // add tables
    TRAPD(prepareErr, PrepareDbL() );
    
    TN_DEBUG2( "CThumbnailStore::RecreateDatabaseL() -- prepare tables, err = %d", prepareErr );
    
    TInt mediaidErr(KErrNone);
    
    // write media id file if doesn't exist
    if(!BaflUtils::FileExists( iFs, mediaid ))
        {
        RFile64 file;
        mediaidErr = file.Create(iFs, mediaid, EFileShareReadersOrWriters );
        file.Close();
        
        TN_DEBUG2( "CThumbnailStore::RecreateDatabaseL() -- mediaID file created, err = %d", mediaidErr );
        }
    
    // delete db if not fully complete
    if (prepareErr < 0 || mediaidErr < 0)
        {
        CloseStatements();
        iDatabase.Close();
        TN_DEBUG1( "CThumbnailStore::RecreateDatabaseL() delete database" );
        TInt del = iDatabase.Delete(pathPtr);     
        TN_DEBUG2( "CThumbnailStore::RecreateDatabaseL() deleted database, err: %d", del );
        }
    
    User::LeaveIfError( prepareErr );
    User::LeaveIfError( mediaidErr );
    
    CleanupStack::PopAndDestroy( databasePath );
    }

TInt CThumbnailStore::CheckRowIDs()
    {
    TN_DEBUG1( "CThumbnailStore::CheckRowIDs()");
    
    RSqlStatement stmt;
    TInt column = 0;   
    TInt rowStatus = 0;
    TInt64 inforows = -1;
    TInt64 infocount = -1;
    TInt64 datarows = -1;
    TInt64 datacount = -1;
    
    TInt ret = stmt.Prepare( iDatabase, KGetInfoRowID );
    if(ret < 0)
        {
        stmt.Close();
        TN_DEBUG1( "CThumbnailStore::CheckRowIDs() KGetInfoRowID failed %d");
        return KErrNotSupported;
        }
    rowStatus = stmt.Next();
                
    if ( rowStatus == KSqlAtRow)    
        {        
        inforows = stmt.ColumnInt64( column );  
        }
                
    stmt.Close();
    
    if(rowStatus < 0)
        {
#ifdef _DEBUG
        TPtrC errorMsg2 = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg2);
#endif
        return KErrNotSupported;
        }
    
    ret = stmt.Prepare( iDatabase, KGetInfoCount );
    if(ret < 0)
        {
        stmt.Close();
        TN_DEBUG1( "CThumbnailStore::CheckRowIDs() KGetInfoCount failed %d");
        return KErrNotSupported;
        }
    rowStatus = stmt.Next();
                
    if ( rowStatus == KSqlAtRow)    
        {        
        infocount = stmt.ColumnInt64( column );  
        }
                
    stmt.Close();
    
    if(rowStatus < 0)
        {
#ifdef _DEBUG
        TPtrC errorMsg2 = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg2);
#endif
        return KErrNotSupported;
        }
            
    ret = stmt.Prepare( iDatabase, KGetDataRowID );
    if(ret < 0)
        {
        stmt.Close();
        TN_DEBUG1( "CThumbnailStore::CheckRowIDs() KGetDataRowID failed");
        return KErrNotSupported;
        }
    rowStatus = stmt.Next();
                       
    if ( rowStatus == KSqlAtRow)    
        {        
        datarows = stmt.ColumnInt64( column );  
        }
            
    stmt.Close();
    
    if( rowStatus < 0)
        {
#ifdef _DEBUG
        TPtrC errorMsg2 = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg2);
#endif
        return KErrNotSupported;
        }
    
    ret = stmt.Prepare( iDatabase, KGetInfoDataCount );
    if(ret < 0)
        {
        stmt.Close();
        TN_DEBUG1( "CThumbnailStore::CheckRowIDs() KGetInfoDataCount failed %d");
        return KErrNotSupported;
        }
    rowStatus = stmt.Next();
                
    if ( rowStatus == KSqlAtRow)    
        {        
        datacount = stmt.ColumnInt64( column );  
        }
                
    stmt.Close();
    
    if(rowStatus < 0)
        {
#ifdef _DEBUG
        TPtrC errorMsg2 = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::ResetThumbnailIDs() lastError %S, ret = %d" , &errorMsg2);
#endif
        return KErrNotSupported;
        }
    
    TN_DEBUG2( "CThumbnailStore::CheckRowIDsL() - inforows %Ld", inforows );
    TN_DEBUG2( "CThumbnailStore::CheckRowIDsL() - infocount %Ld", infocount );
    TN_DEBUG2( "CThumbnailStore::CheckRowIDsL() - datarows %Ld", datarows );
    TN_DEBUG2( "CThumbnailStore::CheckRowIDsL() - datacount %Ld", datacount );
            
    if( inforows != datarows || datacount != infocount)
        {
        TN_DEBUG1( "CThumbnailStore::CheckRowIDsL() - tables out of sync" );
        return KErrNotSupported;
        }  
    else
        {
        return KErrNone;
        }
    }

// -----------------------------------------------------------------------------
// CheckVersion()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::CheckVersion()
    {
    TN_DEBUG1( "CThumbnailStore::CheckVersion()" );
    RSqlStatement stmt;
         
    TInt rowStatus = 0;
    TInt column = 0;
    TInt minor = 0;
    TInt major = 0;

    TInt ret = stmt.Prepare( iDatabase, KThumbnailSelectFromVersion );
    if(ret < 0 )
       {  
       stmt.Close();
       TN_DEBUG1( "CThumbnailStore::CheckVersion() unknown version" );
       return KErrNotSupported;
       }
              
    rowStatus = stmt.Next();
    
    if ( rowStatus == KSqlAtRow)    
       {        
       major = stmt.ColumnInt( column++);
       minor = stmt.ColumnInt( column++);
       }
    
    stmt.Close();
    
    if( rowStatus < 0 )
        {
#ifdef _DEBUG
         TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::CheckVersion() lastError %S, ret = %d" , &errorMsg);
#endif
        return ret;
        }
    
    if(major == KMajor && minor == KMinor )
      {
      return KErrNone;  
      }
    else
      {
      TN_DEBUG1( "CThumbnailStore::CheckVersion() - wrong DB version" );
      return KErrNotSupported;  
      }
    }

// -----------------------------------------------------------------------------
// CheckImei()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::CheckImei()
    {
    TN_DEBUG1( "CThumbnailStore::CheckImei()" );
    RSqlStatement stmt;
         
    TInt rowStatus = 0;
    TInt column = 0;
    TBuf<KImeiBufferSize> imei;
      
    TInt ret = stmt.Prepare( iDatabase, KThumbnailSelectFromVersion );
    if(ret < 0 )
       {  
        stmt.Close();
       TN_DEBUG1( "CThumbnailStore::CheckImei() failed" );
       return KErrNotSupported;
       }
              
    rowStatus = stmt.Next();
    
    if ( rowStatus == KSqlAtRow)    
       {        
       column = 2; // imei column
       stmt.ColumnText( column, imei);  
       }
    
    stmt.Close(); 
    
    if( rowStatus < 0 )
        {
#ifdef _DEBUG
         TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::CheckImei() lastError %S, ret = %d" , &errorMsg);
#endif
        return ret;
        }
    
    if( imei == iImei )
      {
      return KErrNone;  
      }
    else
      {
      TN_DEBUG1( "CThumbnailStore::CheckImei() mismatch" );
      return KErrNotSupported;  
      }
    }

// -----------------------------------------------------------------------------
// CheckMediaID()
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::CheckMediaIDL()
    {
    TN_DEBUG1( "CThumbnailStore::CheckMediaIDL()" );
    
    TVolumeInfo volumeinfo;
    User::LeaveIfError( iFs.Volume(volumeinfo, iDrive) );
    TUint id = volumeinfo.iUniqueID;
    TBuf<50> mediaid;
    mediaid.Num(id);
    
    if(!BaflUtils::FileExists( iFs, mediaid ))
       {
       RFile64 file;
       TInt err = file.Create(iFs, mediaid, EFileShareReadersOrWriters );
       file.Close();
       TN_DEBUG2( "CThumbnailStore::CheckMediaIDL() -- mediaID file created, err = %d", err );
       
       return KErrNotSupported;
       } 

    return KErrNone;  
    }
     
// ----------------------------------------------------------------------------
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
    CleanupStack::PopAndDestroy( &stmt );  
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
            
    User::LeaveIfError( stmt.Prepare( iDatabase, KThumbnailUpdateIMEI ) );
    
    TInt paramIndex = stmt.ParameterIndex( KThumbnailSqlParamImei );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt.BindText( paramIndex, iImei ));
    
    TInt err = stmt.Exec();
    
    if(err < 0)
        {
#ifdef _DEBUG
        TPtrC errorMsg = iDatabase.LastErrorMessage();
        TN_DEBUG2( "RThumbnailTransaction::UpdateImeiL() lastError %S" , &errorMsg);
#endif
        User::Leave(err);
        }
    
    CleanupStack::PopAndDestroy( &stmt );
    }

// ---------------------------------------------------------------------------
// CThumbnailStore::PrepareStatementsL()
// ---------------------------------------------------------------------------
//
void CThumbnailStore::PrepareStatementsL()
    {
    TN_DEBUG1("CThumbnailStore::PrepareStatementsL()");
    
    TInt err = KErrNone;  
#ifdef _DEBUG
    TFileName msg;
#endif
    
    err = iStmt_KThumbnailSelectInfoByPath.Prepare( iDatabase, KThumbnailSelectInfoByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSelectInfoByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSelectTempInfoByPath.Prepare( iDatabase, KThumbnailSelectTempInfoByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSelectTempInfoByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );

    err = iStmt_KThumbnailInsertTempThumbnailInfo.Prepare( iDatabase, KThumbnailInsertTempThumbnailInfo );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailInsertTempThumbnailInfo %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailInsertTempThumbnailInfoData.Prepare( iDatabase, KThumbnailInsertTempThumbnailInfoData );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailInsertTempThumbnailInfoData %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSelectModifiedByPath.Prepare( iDatabase, KThumbnailSelectModifiedByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSelectModifiedByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSelectTempModifiedByPath.Prepare( iDatabase, KThumbnailSelectTempModifiedByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSelectTempModifiedByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailFindDuplicate.Prepare( iDatabase, KThumbnailFindDuplicate );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailFindDuplicate %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailTempFindDuplicate.Prepare( iDatabase, KThumbnailTempFindDuplicate );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailTempFindDuplicate %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSqlFindDeleted.Prepare( iDatabase, KThumbnailSqlFindDeleted );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSqlFindDeleted %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSelectSizeByPath.Prepare( iDatabase, KThumbnailSelectSizeByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSelectSizeByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSelectTempSizeByPath.Prepare( iDatabase, KThumbnailSelectTempSizeByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSelectTempSizeByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSqlSelectRowIDInfoByPath.Prepare( iDatabase, KThumbnailSqlSelectRowIDInfoByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSqlSelectRowIDInfoByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSqlDeleteInfoByPath.Prepare( iDatabase, KThumbnailSqlDeleteInfoByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSqlDeleteInfoByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );

    err = iStmt_KThumbnailSqlDeleteInfoDataByPath.Prepare( iDatabase, KThumbnailSqlDeleteInfoDataByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSqlDeleteInfoDataByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );

    err = iStmt_KTempThumbnailSqlSelectRowIDInfoByPath.Prepare( iDatabase, KTempThumbnailSqlSelectRowIDInfoByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KTempThumbnailSqlSelectRowIDInfoByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );

    err = iStmt_KTempThumbnailSqlDeleteInfoByPath.Prepare( iDatabase, KTempThumbnailSqlDeleteInfoByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KTempThumbnailSqlDeleteInfoByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );

    err = iStmt_KTempThumbnailSqlDeleteInfoDataByPath.Prepare( iDatabase, KTempThumbnailSqlDeleteInfoDataByPath );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KTempThumbnailSqlDeleteInfoDataByPath %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );

    err = iStmt_KThumbnailSqlInsertDeleted.Prepare( iDatabase, KThumbnailSqlInsertDeleted );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSqlInsertDeleted %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSqlSelectMarked.Prepare( iDatabase, KThumbnailSqlSelectMarked );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSqlSelectMarked %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );

    err = iStmt_KThumbnailSqlDeleteInfoByRowID.Prepare( iDatabase, KThumbnailSqlDeleteInfoByRowID );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSqlDeleteInfoByRowID %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );

    err = iStmt_KThumbnailSqlDeleteInfoDataByRowID.Prepare( iDatabase, KThumbnailSqlDeleteInfoDataByRowID );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSqlDeleteInfoDataByRowID %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailSelectAllPaths.Prepare( iDatabase, KThumbnailSelectAllPaths );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailSelectAllPaths %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailRename.Prepare( iDatabase, KThumbnailRename );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailRename %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    err = iStmt_KThumbnailTempRename.Prepare( iDatabase, KThumbnailTempRename );
#ifdef _DEBUG
    msg.Append( iDatabase.LastErrorMessage() );
    TN_DEBUG2( "CThumbnailStore::PrepareStatementsL() KThumbnailTempRename %S" , &msg );
    msg.Zero();
#endif
    User::LeaveIfError( err );
    
    TN_DEBUG1("CThumbnailStore::PrepareStatementsL() end");
    }

// ---------------------------------------------------------------------------
// CThumbnailStore::ResetStatement()
// ---------------------------------------------------------------------------
//
void CThumbnailStore::ResetStatement( TAny* aStmt )
    {
    // called by CleanupStack::PopAndDestroy()
    // just reset so that there's no need to prepare again
    ((RSqlStatement*)aStmt)->Reset();
    }

// ---------------------------------------------------------------------------
// CThumbnailStore::CloseStatements()
// ---------------------------------------------------------------------------
//
void CThumbnailStore::CloseStatements()
    {
    TN_DEBUG1("CThumbnailStore::CloseStatements()");
    
    iStmt_KThumbnailSelectInfoByPath.Close();
    iStmt_KThumbnailSelectTempInfoByPath.Close();   
    iStmt_KThumbnailInsertTempThumbnailInfo.Close();
    iStmt_KThumbnailInsertTempThumbnailInfoData.Close();  
    iStmt_KThumbnailSelectModifiedByPath.Close();
    iStmt_KThumbnailSelectTempModifiedByPath.Close();
    iStmt_KThumbnailFindDuplicate.Close();
    iStmt_KThumbnailTempFindDuplicate.Close();
    iStmt_KThumbnailSqlFindDeleted.Close();
    iStmt_KThumbnailSelectSizeByPath.Close();
    iStmt_KThumbnailSelectTempSizeByPath.Close();
    iStmt_KThumbnailSqlSelectRowIDInfoByPath.Close();
    iStmt_KThumbnailSqlDeleteInfoByPath.Close();
    iStmt_KThumbnailSqlDeleteInfoDataByPath.Close();
    iStmt_KTempThumbnailSqlSelectRowIDInfoByPath.Close();
    iStmt_KTempThumbnailSqlDeleteInfoByPath.Close();
    iStmt_KTempThumbnailSqlDeleteInfoDataByPath.Close();
    iStmt_KThumbnailSqlInsertDeleted.Close();
    iStmt_KThumbnailSqlSelectMarked.Close();
    iStmt_KThumbnailSqlDeleteInfoByRowID.Close();
    iStmt_KThumbnailSqlDeleteInfoDataByRowID.Close();
    iStmt_KThumbnailSelectAllPaths.Close();
    iStmt_KThumbnailRename.Close();
    iStmt_KThumbnailTempRename.Close();
    
    TN_DEBUG1("CThumbnailStore::CloseStatements() end");
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
    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( private ) in" );

#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif

    //Encapsulate insert to Transaction
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );
    transaction.BeginL();
    
    // Insert into TempThumbnailInfo
    RSqlStatement* stmt = NULL;
    stmt = &iStmt_KThumbnailInsertTempThumbnailInfo;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
    
    TInt paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, aPath ));

    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamWidth );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aSize.iWidth ));

    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamHeight );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aSize.iHeight ));

    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamOriginalWidth );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aOriginalSize.iWidth ));

    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamOriginalHeight );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aOriginalSize.iHeight ));

    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamFormat );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aFormat ));

    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamFlags );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aFlags ));

    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamSize );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aThumbnailSize ));
    
    // orientation temporarily to 0
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamOrientation );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, 0 ));
    
    // thumb from associated path
    TInt fromPath = aThumbFromPath;
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamThumbFromPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, fromPath ));
    
    // try getting modification time from file
    TTime timeStamp;
    
    TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( private ) timeStamp aModified %Ld", aModified );
        
    if( aModified )
        {
        timeStamp = aModified;
        }
    else
        {
        TInt timeErr = KErrNone;
    
        if (aPath.Length())
            {
            // need to add drive letter
            TFileName path;
            path.Append(iDriveChar);
            path.Append(KDrv);
            path.Append(aPath);
        
            timeErr = iFs.Modified(path, timeStamp);
            
            if (timeErr != KErrNone)
                {
                TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( private ) error getting timeStamp: %d", timeErr );
                }
            else
                {
                TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( private ) timeStamp       iFs %Ld", timeStamp.Int64() );
                }
            }
        
        if (!aPath.Length() || timeErr != KErrNone)
            {
            // otherwise current time
            timeStamp.UniversalTime();
            TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( private ) timeStamp   current %Ld", timeStamp.Int64() );
            }
        }
        
   TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( private ) timeStamp       set %Ld", timeStamp.Int64());
   
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamModified );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt64( paramIndex, timeStamp.Int64() ));
    
    User::LeaveIfError( stmt->Exec());
    CleanupStack::PopAndDestroy( stmt );
    
    // Insert into TempThumbnailInfoData
    RSqlStatement* stmtData = NULL;
    stmtData = &iStmt_KThumbnailInsertTempThumbnailInfoData;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmtData));
    
    paramIndex = stmtData->ParameterIndex( KThumbnailSqlParamData );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmtData->BindBinary( paramIndex, aData ));

    User::LeaveIfError( stmtData->Exec());
    CleanupStack::PopAndDestroy( stmtData );
	
    // Commit transaction
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );

    iBatchItemCount++;
    
    FlushCacheTable();

#ifdef _DEBUG
    iThumbCounter++;
    TN_DEBUG2( "CThumbnailStore::THUMBSTORE-COUNTER----------, Thumbs = %d", iThumbCounter );
    
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( private ) insert to table %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif 
    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( private ) out" );
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
    TN_DEBUG4( "CThumbnailStore::StoreThumbnailL( public ) aThumbnailSize = %d, aThumbnailSize(%d,%d) IN", aThumbnailSize, thumbSize.iWidth, thumbSize.iHeight );

    __ASSERT_DEBUG(( aThumbnail ), ThumbnailPanic( EThumbnailNullPointer ));
    
    User::LeaveIfError( CheckDbState() );
    
    // don't store custom/unknown sizes or zero sizes
    if(aThumbnailSize == ECustomThumbnailSize || aThumbnailSize == EUnknownThumbnailSize 
            || thumbSize.iWidth <= 0 || thumbSize.iHeight <= 0 )
        {
        TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( public ) not stored");
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
                TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( public ) - encode jpg" );
            
                HBufC8* data = NULL;
                CleanupStack::PushL( data );
                
                CImageEncoder* encoder = NULL;
                TRAPD( decErr, encoder = CExtJpegEncoder::DataNewL( CExtJpegEncoder::EHwImplementation, data, CImageEncoder::EOptionAlwaysThread ) );
                if ( decErr != KErrNone )
                    {
                    TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( public ) - HW CExtJpegEncoder failed %d", decErr);
                
                    TRAPD( decErr, encoder = CExtJpegEncoder::DataNewL( CExtJpegEncoder::ESwImplementation, data, CImageEncoder::EOptionAlwaysThread ) );
                    if ( decErr != KErrNone )
                        {
                        TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( public ) - SW CExtJpegEncoder failed %d", decErr);
                    
                        TRAPD( decErr, encoder = CImageEncoder::DataNewL( data,  KJpegMime(), CImageEncoder::EOptionAlwaysThread ) );
                        if ( decErr != KErrNone )
                            {
                            TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( public ) - CImageEncoder failed %d", decErr);
                            
                            User::Leave(decErr);
                            }
                        else
                            {
                            TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( public ) - CImageEncoder created" );
                            }
                        }
                    else
                        {
                        TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( public ) - SW CExtJpegEncoder created" );
                        }
                    }
                else
                    {
                    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( public ) - HW CExtJpegEncoder created" );
                    }             
                
                CleanupStack::Pop( data );
                CleanupStack::PushL( encoder );
             
                CFrameImageData* frameImageData = CFrameImageData::NewL();
                CleanupStack::PushL( frameImageData );
                
                TJpegImageData* imageData = new (ELeave) TJpegImageData();
                CleanupStack::PushL( imageData );
                
                // Set some format specific data
                imageData->iSampleScheme = TJpegImageData::EColor444;
                imageData->iQualityFactor = 75;
                
                // imageData - ownership passed to frameImageData after AppendImageData
                User::LeaveIfError(frameImageData->AppendImageData(imageData));
                CleanupStack::Pop( imageData );
                
#ifdef _DEBUG
        TN_DEBUG4( "CThumbnailStore::StoreThumbnailL( public ) - size: %d x %d, displaymode: %d ", 
                aThumbnail->SizeInPixels().iWidth, 
                aThumbnail->SizeInPixels().iHeight, 
                aThumbnail->DisplayMode());
#endif
                
                TRequestStatus request;
                encoder->Convert( &request, *aThumbnail, frameImageData);
                User::WaitForRequest( request);
                
                CleanupStack::PopAndDestroy( frameImageData );
                CleanupStack::PopAndDestroy( encoder );
                
                if(request == KErrNone)
                    {           
                    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( public ) - encoding ok" );    
                
                    CleanupStack::PushL( data );
                    TPtr8 ptr = data->Des(); 
                    StoreThumbnailL( *path, ptr, aThumbnail->SizeInPixels(), 
                                     aOriginalSize, EThumbnailFormatJpeg, flags, 
                                     aThumbnailSize, aModified, aThumbFromPath  );
                    CleanupStack::Pop( data );
                    }
                else
                    {
                    TN_DEBUG2( "CThumbnailStore::StoreThumbnailL( public ) - encoding failed: %d", request.Int() );
                    }
                
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
                                 aOriginalSize, EThumbnailFormatFbsBitmap, flags, 
                                 aThumbnailSize, aModified, aThumbFromPath);
  
                CleanupStack::PopAndDestroy( buf );
                }
            
            break;
            }
        }
    
    CleanupStack::PopAndDestroy( path );
    
    TN_DEBUG1( "CThumbnailStore::StoreThumbnailL( public ) out" );
    }

// ---------------------------------------------------------------------------
// Finds possible existing duplicate thumbnail.
// ---------------------------------------------------------------------------
//
TBool CThumbnailStore::FindDuplicateL( const TDesC& aPath, const TThumbnailSize& aThumbnailSize )
    {
    TN_DEBUG1( "CThumbnailStore::FindDuplicateL()" );
    
    User::LeaveIfError( CheckDbState() );
    
    TInt rowStatus = 0;
    TInt paramIndex = 0;
    TInt found = EFalse;
    
    RSqlStatement* stmt = NULL;
    stmt = &iStmt_KThumbnailTempFindDuplicate;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));  
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, aPath ));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamSize );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aThumbnailSize ));
    
    rowStatus = stmt->Next();
    
    //if not found from temp table, look from real table
    if(rowStatus != KSqlAtRow)
        {
        CleanupStack::PopAndDestroy( stmt );
        stmt = &iStmt_KThumbnailFindDuplicate;
        CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
           
        paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt->BindText( paramIndex, aPath ));
            
        paramIndex = stmt->ParameterIndex( KThumbnailSqlParamSize );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt->BindInt( paramIndex, aThumbnailSize ));
        
        rowStatus = stmt->Next();
        
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
    
    CleanupStack::PopAndDestroy( stmt );
    
    // check if duplicate in Deleted
    if (found)
        {
        stmt = &iStmt_KThumbnailSqlFindDeleted;
        CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));   
            
        paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt->BindText( paramIndex, aPath ));
        
        rowStatus = stmt->Next();
        
        CleanupStack::PopAndDestroy( stmt );
        
        if(rowStatus == KSqlAtRow)
            {
            TN_DEBUG1( "CThumbnailStore::FindDuplicateL() - duplicate marked deleted" );
            
            DeleteThumbnailsL(aPath, ETrue);
            
            TN_DEBUG1( "CThumbnailStore::FindDuplicateL() - duplicate force-deleted" );
            
            found = EFalse;
            }
        }
    
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
    
    User::LeaveIfError( CheckDbState() );
    
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
                    aMissingSizes.AppendL( iPersistentSizes[ i ] );
				    }
                }
            else
                {
                aMissingSizes.AppendL( iPersistentSizes[ i ] );
                }
            }
        }
    
    TInt missingSizeCount = aMissingSizes.Count();
        
    TN_DEBUG2( "CThumbnailStore::GetMissingSizesL() missingSizeCount == %d", missingSizeCount );
    
    // check temp table first
    RSqlStatement* stmt = NULL;
    stmt = &iStmt_KThumbnailSelectTempSizeByPath;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));

    TInt paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, *path ));
       
    TInt rowStatus = stmt->Next();

    TInt round = 1;
    TInt size = 0;
    
    while (round <= 2)
        {
        while ( rowStatus == KSqlAtRow && missingSizeCount > 0 )
            {
            size = stmt->ColumnInt( 0 );
			
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
                
            rowStatus = stmt->Next();
            }

        CleanupStack::PopAndDestroy( stmt );
        
        // all found
        if (missingSizeCount == 0)
            {
            CleanupStack::PopAndDestroy( path );
            return;
            }
        else if (round == 1)
            {
            // change to real table
            stmt = &iStmt_KThumbnailSelectSizeByPath;
            CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
            
            paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmt->BindText( paramIndex, *path ));
            rowStatus = stmt->Next();    
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
    
    User::LeaveIfError( CheckDbState() );
    
    HBufC* path = aPath.AllocLC();
    TPtr ptr(path->Des());
    StripDriveLetterL( ptr );
    
    TInt paramIndex = 0;
    TInt found = KErrNotFound;
    TInt rowStatus = 0;
    TInt column = 0;
    
    TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- TEMP TABLE lookup" );

    RSqlStatement* stmt = NULL;
    stmt = &iStmt_KThumbnailSelectTempInfoByPath;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, *path ));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamSize );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, aThumbnailSize ));
    
    rowStatus = stmt->Next();

    //if not found from temp table, look from real table
    if(rowStatus != KSqlAtRow)
       {
       TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- MAIN TABLE lookup" );
      
       CleanupStack::PopAndDestroy( stmt );
       stmt = &iStmt_KThumbnailSelectInfoByPath;
       CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
    
        paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt->BindText( paramIndex, *path ));
        
        paramIndex = stmt->ParameterIndex( KThumbnailSqlParamSize );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt->BindInt( paramIndex, aThumbnailSize ));
    
        rowStatus = stmt->Next();
       }

    if(rowStatus == KSqlAtRow)
       {
        TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- thumbnail found" );
        
        // Check whether blacklisted thumbnail entry modified. 
        // If thumbnail is marked as blacklisted and timestamp has 
        // changed, delete thumbnails from tables and leave with 
        // KErrNotFound to get thumbnail regenerated.
        column = 4;
        TInt flags = stmt->ColumnInt( column );
        if( flags & KThumbnailDbFlagDeleted )
            {
            CleanupStack::PopAndDestroy( stmt );
            
            // delete existing blacklisted thumbs
            DeleteThumbnailsL(*path, ETrue);
            
            CleanupStack::PopAndDestroy( path );
        
            User::Leave( KErrNotFound );
            }
        else if( flags & KThumbnailDbFlagBlacklisted )
            {
            CleanupStack::PopAndDestroy( stmt );
            CleanupStack::PopAndDestroy( path );
        
            User::Leave( KErrCompletion );
            }
        else if( !(flags & KThumbnailDbFlagBlacklisted) )
            {
            found = KErrNone;
            column = 0;
            TInt format = stmt->ColumnInt( column++ );  
            if(format == 1 /*TThumbnailFormat::EThumbnailFormatJpeg */ )
                {
                TPtrC8 ptr = stmt->ColumnBinaryL( column++ );
                HBufC8* data = ptr.AllocL() ;
                aThumbnail = NULL;
                aData = data;               
                } 
            else 
                {
                TPtrC8 ptr = stmt->ColumnBinaryL( column++ );
                RDesReadStream stream( ptr );
                aThumbnail = new( ELeave )CFbsBitmap();
                CleanupStack::PushL( aThumbnail );
                aThumbnail->InternalizeL( stream );
                CleanupStack::Pop( aThumbnail );
                aData = NULL;
                }
            
            //fetch real size of TN
            column = 2;
            aThumbnailRealSize.iWidth = stmt->ColumnInt( column++ );
            aThumbnailRealSize.iHeight = stmt->ColumnInt( column );
            }
        }
    else
        {
        TN_DEBUG1( "CThumbnailStore::FetchThumbnailL() -- thumbnail NOT found" );
        }
        
    CleanupStack::PopAndDestroy( stmt );
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
    
    User::LeaveIfError( CheckDbState() );
    
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
        
    TN_DEBUG1( "CThumbnailStore::DeleteThumbnailsByPathL() -- TEMP TABLE lookup" );
    
    RSqlStatement* stmt = NULL;
    RSqlStatement* stmt_info = NULL;
    RSqlStatement* stmt_infodata = NULL;
    
    stmt = &iStmt_KTempThumbnailSqlSelectRowIDInfoByPath;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
    stmt_info = &iStmt_KTempThumbnailSqlDeleteInfoByPath;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt_info));    
    stmt_infodata = &iStmt_KTempThumbnailSqlDeleteInfoDataByPath;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt_infodata));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, *path ));
    
    rowStatus = stmt->Next();
    
    while(rowStatus == KSqlAtRow)
        {  
        rowid = stmt->ColumnInt64( column ); 
        paramIndex1 = stmt_info->ParameterIndex( KThumbnailSqlParamRowID );
        User::LeaveIfError( paramIndex1 );
        User::LeaveIfError( stmt_info->BindInt64( paramIndex1, rowid ));
       
        TInt err = stmt_info->Exec();
        stmt_info->Reset();
        User::LeaveIfError( err );
             
        paramIndex2 = stmt_infodata->ParameterIndex( KThumbnailSqlParamRowID );
        User::LeaveIfError( paramIndex2 );
        User::LeaveIfError( stmt_infodata->BindInt64( paramIndex2, rowid ));
             
        err = stmt_infodata->Exec();
        stmt_infodata->Reset();
        User::LeaveIfError( err );
        
        TN_DEBUG1( "CThumbnailStore::DeleteThumbnailsByPathL() -- TEMP TABLE lookup - thumbnail deleted" );
       
        // fetch another row (temp table rowIDs are updated immediately)
        stmt->Reset();
       
        paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt->BindText( paramIndex, *path ));
       
        rowStatus = stmt->Next();   
        }
    
    CleanupStack::PopAndDestroy( stmt_infodata );
    CleanupStack::PopAndDestroy( stmt_info );
    CleanupStack::PopAndDestroy( stmt );
        
    // if forcing instant delete
    if (aForce)
        {
        //look from real table 
        TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- MAIN TABLE lookup" );
        
        stmt = &iStmt_KThumbnailSqlSelectRowIDInfoByPath;
        CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
        stmt_info = &iStmt_KThumbnailSqlDeleteInfoByPath;
        CleanupStack::PushL(TCleanupItem(ResetStatement, stmt_info));    
        stmt_infodata = &iStmt_KThumbnailSqlDeleteInfoDataByPath;
        CleanupStack::PushL(TCleanupItem(ResetStatement, stmt_infodata));

        paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt->BindText( paramIndex, *path ));
             
        rowStatus = stmt->Next();   
           
        while(rowStatus == KSqlAtRow)
            { 
            rowid = stmt->ColumnInt64( column ); 
            paramIndex1 = stmt_info->ParameterIndex( KThumbnailSqlParamRowID );
            User::LeaveIfError( paramIndex1 );
            User::LeaveIfError( stmt_info->BindInt64( paramIndex1, rowid ));
               
            TInt err = stmt_info->Exec();
            stmt_info->Reset();
            User::LeaveIfError( err );
                    
            paramIndex2 = stmt_infodata->ParameterIndex( KThumbnailSqlParamRowID );  
            User::LeaveIfError( paramIndex2 );
            User::LeaveIfError( stmt_infodata->BindInt64( paramIndex2, rowid ));
                    
            err = stmt_infodata->Exec();
            stmt_infodata->Reset();
            User::LeaveIfError( err );
           
            TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- MAIN TABLE lookup - thumbnail deleted" );
            
            rowStatus = stmt->Next();
            }
        
        CleanupStack::PopAndDestroy( stmt_infodata );
        CleanupStack::PopAndDestroy( stmt_info );
        CleanupStack::PopAndDestroy( stmt );
        
		//remove delete mark
        User::LeaveIfError( iDatabase.Exec( KThumbnailSqlDeleteFromDeleted ) );
        } 
    else
        {
        TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- MAIN TABLE lookup" );        
    
        stmt = &iStmt_KThumbnailSqlSelectRowIDInfoByPath;
        CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));

        paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
        User::LeaveIfError( paramIndex );
        User::LeaveIfError( stmt->BindText( paramIndex, *path ));
             
        rowStatus = stmt->Next();   
           
        CleanupStack::PopAndDestroy( stmt );
        
        // there were matching rows in main table
        if (rowStatus == KSqlAtRow)
            {        
            TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- add to Deleted" );
        
            // only add path to deleted table
            stmt = &iStmt_KThumbnailSqlInsertDeleted;
            CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
            
            paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmt->BindText( paramIndex, *path ));
            
            count = stmt->Exec();
            
            CleanupStack::PopAndDestroy( stmt );
            }
        else
            {
            TN_DEBUG1( "CThumbnailStore::DeleteThumbnailByPathL() -- no thumbs in MAIN" );
            }
        }    
    
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

// -----------------------------------------------------------------------------
// Rename thumbnails
// -----------------------------------------------------------------------------
//
void CThumbnailStore::RenameThumbnailsL( const TDesC& aCurrentPath, const TDesC& aNewPath )
    {
    TN_DEBUG2( "CThumbnailStore::RenameThumbnailsL(%S)", &aCurrentPath );
    
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    User::LeaveIfError( CheckDbState() );
    
    TInt paramIndex = 0;

    HBufC* path = aCurrentPath.AllocLC();
    TPtr ptr(path->Des());
    StripDriveLetterL( ptr );
    
    HBufC* newPath = aNewPath.AllocLC();
    TPtr ptr2(newPath->Des());
    StripDriveLetterL( ptr2 );
    
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );    
    transaction.BeginL();
        
    TN_DEBUG1( "CThumbnailStore::RenameThumbnailsL() -- TEMP TABLE" );
    
    RSqlStatement* stmt = NULL;
    stmt = &iStmt_KThumbnailTempRename;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, *path ));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamNewPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, *newPath ));
    
    User::LeaveIfError( stmt->Exec() );
    
    TN_DEBUG1( "CThumbnailStore::RenameThumbnailsL() -- MAIN TABLE" );
    
    CleanupStack::PopAndDestroy( stmt );
    stmt = &iStmt_KThumbnailRename;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, *path ));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamNewPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, *newPath ));
    
    User::LeaveIfError( stmt->Exec() );
    
    CleanupStack::PopAndDestroy( stmt );
    
    // if thumb was for some reason already marked deleted, clean from deleted
    User::LeaveIfError( iDatabase.Exec( KThumbnailSqlDeleteFromDeleted ) );
    
    transaction.CommitL();
    CleanupStack::PopAndDestroy( &transaction );
    
    CleanupStack::PopAndDestroy( newPath );
    CleanupStack::PopAndDestroy( path );
    
#ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailStore::RenameThumbnailsL() took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
#endif      
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
    
    if(iBatchItemCount <= 0 || CheckDbState() != KErrNone)
        {
        // cache empty or db unusable
        TN_DEBUG1( "CThumbnailStore::FlushCacheTable() error ");
        return;
        }
    
    // longer flush allowed if MTP sync on
    TInt MPXHarvesting(0);
    TInt ret = RProperty::Get(KTAGDPSNotification, KMPXHarvesting, MPXHarvesting);
    if(ret != KErrNone)
       {
       TN_DEBUG2( "CThumbnailStore::FlushCacheTable() error checking MTP sync: %d", ret);
       }
    
    //set init max flush delay
    TReal32 aMaxFlushDelay(KMaxFlushDelay);
    TReal32 aPreviousFlushDelay(iPreviousFlushDelay);
    TReal32 aBatchFlushItemCount(iBatchFlushItemCount);
    
    if(MPXHarvesting)
        {
        //MTP or MPX harvesting active, allow longer flush -> bigger batch size
        TN_DEBUG1("CThumbnailStore::FlushCacheTable() MTP sync, longer flush..");
        aMaxFlushDelay = KMaxMTPFlushDelay;
        }
    
    //1st item in batch    
    if( iBatchItemCount == 1)
        {
        TN_DEBUG2("CThumbnailStore::FlushCacheTable() calculate new batch size iPreviousFlushDelay = %d", iPreviousFlushDelay);
        //adjust batch size dynamically between min and max based on previous flush speed
        if( iPreviousFlushDelay > 0 )
            {
            TReal32 aNewBatchFlushItemCount = aMaxFlushDelay / aPreviousFlushDelay * aBatchFlushItemCount;
            iBatchFlushItemCount = (TInt)aNewBatchFlushItemCount;

            TN_DEBUG2("CThumbnailStore::FlushCacheTable() aMaxFlushDelay %e", aMaxFlushDelay);      
            TN_DEBUG2("CThumbnailStore::FlushCacheTable() aPreviousFlushDelay %e", aPreviousFlushDelay);      
			TN_DEBUG2("CThumbnailStore::FlushCacheTable() aBatchFlushItemCount %e", aBatchFlushItemCount);      
            TN_DEBUG2("CThumbnailStore::FlushCacheTable() aNewBatchFlushItemCount %e", aNewBatchFlushItemCount);
            TN_DEBUG2("CThumbnailStore::FlushCacheTable() iBatchFlushItemCount %d", iBatchFlushItemCount);
            
            if( iBatchFlushItemCount < KMInBatchItems )
                {
                iBatchFlushItemCount = KMInBatchItems;
                }
            else if( iBatchFlushItemCount > KMaxBatchItems )
                {
                iBatchFlushItemCount = KMaxBatchItems;
                }
            }
        else
            {
            //cannot calculate, init values set to min
            iBatchFlushItemCount = KMInBatchItems;
            }
        }
    
    TN_DEBUG3("CThumbnailStore::FlushCacheTable() iBatchFlushItemCount = %d, iBatchItemCount = %d", iBatchFlushItemCount, iBatchItemCount);
    
    if( iBatchItemCount < iBatchFlushItemCount && !aForce)
       {
       //some items in cache
       StartAutoFlush();
       return;
       }    
    
    iStartFlush.UniversalTime();
    
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
    
    TInt err_tempdata = iDatabase.Exec( KThumbnailMoveFromTempDataToMainTable );
    
#ifdef _DEBUG
    if(err_tempdata < 0)
        {
        TPtrC errorMsg2 = iDatabase.LastErrorMessage();
        TN_DEBUG2( "CThumbnailStore::FlushCacheTable() KThumbnailMoveFromTempDataToMainTable %S", &errorMsg2);
        }
#endif
    
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
    
    // db got corrupted
    if(err_tempinfo == KSqlErrCorrupt || err_tempinfo == KErrCorrupt ||
       err_tempdata == KSqlErrCorrupt || err_tempdata == KErrCorrupt)
        {
        TN_DEBUG1("CThumbnailStore::FlushCacheTable() db corrupted");
    
        // open new
        TRAP_IGNORE(OpenDatabaseL(ETrue));
        }
   
    iStopFlush.UniversalTime();
    iPreviousFlushDelay = (TInt)iStopFlush.MicroSecondsFrom(iStartFlush).Int64()/1000;
    
    TN_DEBUG2( "CThumbnailStore::FlushCacheTable() took %d ms", iPreviousFlushDelay);
        
    //cache flushed
    iBatchItemCount = 0;

    TN_DEBUG1("CThumbnailStore::FlushCacheTable() out");
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
    
    TInt MPXHarvesting(0);
    TInt DaemonProcessing(0);
    TInt ret = RProperty::Get(KTAGDPSNotification, KMPXHarvesting, MPXHarvesting);
    if(ret != KErrNone || MPXHarvesting)
        {
        TN_DEBUG3( "CThumbnailStore::MaintenanceTimerCallBack() KMPXHarvesting err == %d, MPXHarvesting == %d", ret, MPXHarvesting);
        self->iIdle = EFalse;
        }
    TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() KMPXHarvesting == %d", MPXHarvesting);

    ret = RProperty::Get(KTAGDPSNotification, KDaemonProcessing, DaemonProcessing);
    if(ret != KErrNone || DaemonProcessing)
        {
        TN_DEBUG3( "CThumbnailStore::MaintenanceTimerCallBack() KDaemonProcessing err == %d, DaemonProcessing == %d", ret, DaemonProcessing);
        self->iIdle = EFalse;
        }
    TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() DaemonProcessing == %d", DaemonProcessing);
    
    if (self->iIdle)
        {
        TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() - maintenance, store %d", self->iDrive);
    
        // thumbmnail deletion
        if (self->iDeleteThumbs)
            {
            TN_DEBUG1( "CThumbnailStore::MaintenanceTimerCallBack() - cleanup");
        
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
        
        // file existence check
        else if (self->iCheckFilesExist)
            {
            TN_DEBUG1( "CThumbnailStore::MaintenanceTimerCallBack() - file existence check");
        
            TBool finished = EFalse;
        
            TRAPD( err, finished = self->FileExistenceCheckL() );
            if (err != KErrNone)
                {
                TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() - file existence check failed, err %d", err);
                return err;
                }
        
            // all files checked.
            if (finished)
                {
                TN_DEBUG2( "CThumbnailStore::MaintenanceTimerCallBack() - file existence check finished, store %d", self->iDrive);
                self->iCheckFilesExist = EFalse;
                }
            }
        
        // next round
        if (self->iIdle && ( self->iDeleteThumbs || self->iCheckFilesExist) )
            {
            TN_DEBUG1( "CThumbnailStore::MaintenanceTimerCallBack() - continue maintenance");
            self->StartMaintenance();
            }  
        else if (!self->iDeleteThumbs && !self->iCheckFilesExist)
            {
            TN_DEBUG1( "CThumbnailStore::MaintenanceTimerCallBack() - no more maintenance");
        
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

TBool CThumbnailStore::CheckModifiedByPathL( const TDesC& aPath, const TInt64 aModified, TBool& modifiedChanged )
    {
    TN_DEBUG2( "CThumbnailStore::CheckModifiedByPathL() %S", &aPath);
    
    User::LeaveIfError( CheckDbState() );
    
    HBufC* path = aPath.AllocLC();
    TPtr ptr(path->Des());
    StripDriveLetterL( ptr );
	
    TBool ret(EFalse);

    modifiedChanged = EFalse;

    TInt column = 0;
   
    RSqlStatement* stmt = NULL;
    stmt = &iStmt_KThumbnailSelectTempModifiedByPath;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
   
    TInt paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindText( paramIndex, *path ));
    
    TInt rowStatus = stmt->Next();
   
    TBool checkMain = EFalse;
   
    TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- temp" );
   
    while(rowStatus == KSqlAtRow || !checkMain)
        {
        if(rowStatus == KSqlAtRow)
            {
            ret = ETrue;
            TInt64 oldModified = stmt->ColumnInt64( column );
           
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
            
            rowStatus = stmt->Next();
            }
       
        //switch to main table if modified not found from temp
        if(rowStatus != KSqlAtRow && !checkMain && !modifiedChanged)
            {
            TN_DEBUG1( "CThumbnailStore::CheckModifiedL() -- main" );
            //come here only once
            checkMain = ETrue;
           
            CleanupStack::PopAndDestroy( stmt );
            stmt = &iStmt_KThumbnailSelectModifiedByPath;
            CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
           
            paramIndex = stmt->ParameterIndex( KThumbnailSqlParamPath );
            User::LeaveIfError( paramIndex );
            User::LeaveIfError( stmt->BindText( paramIndex, *path ));
            
            rowStatus = stmt->Next();
            }
        }
    
    CleanupStack::PopAndDestroy( stmt );   
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
    TN_DEBUG1( "CThumbnailStore::DeleteMarkedL()" );
   
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    User::LeaveIfError( CheckDbState() );
    
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
    
    RSqlStatement* stmt = NULL;
    RSqlStatement* stmt_info = NULL;
    RSqlStatement* stmt_infodata = NULL;
    
    stmt = &iStmt_KThumbnailSqlSelectMarked;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
    stmt_info = &iStmt_KThumbnailSqlDeleteInfoByRowID;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt_info));    
    stmt_infodata = &iStmt_KThumbnailSqlDeleteInfoDataByRowID;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt_infodata));
    
    // select marked rows
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamLimit );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, KStoreMaintenanceDeleteLimit ));
             
    rowStatus = stmt->Next();  
           
    while(rowStatus == KSqlAtRow)
       { 
       rowid = stmt->ColumnInt64( column ); 
       paramIndex1 = stmt_info->ParameterIndex( KThumbnailSqlParamRowID );
       User::LeaveIfError( paramIndex1 );
       User::LeaveIfError( stmt_info->BindInt64( paramIndex1, rowid ));
              
       TInt err = stmt_info->Exec();
       stmt_info->Reset();
       User::LeaveIfError( err );
                    
       paramIndex2 = stmt_infodata->ParameterIndex( KThumbnailSqlParamRowID );  
       User::LeaveIfError( paramIndex2 );
       User::LeaveIfError( stmt_infodata->BindInt64( paramIndex2, rowid ));
                    
       err = stmt_infodata->Exec();
       stmt_infodata->Reset();
       User::LeaveIfError( err );
       deleteCount++;
       
       TN_DEBUG1( "CThumbnailStore::DeleteMarkedL() - thumbnail deleted" );
       
       rowStatus = stmt->Next();
       }
        
    CleanupStack::PopAndDestroy( stmt_infodata );
    CleanupStack::PopAndDestroy( stmt_info );
    CleanupStack::PopAndDestroy( stmt );    
    
    // remove successfully deleted paths from Deleted table
    if (deleteCount > 0)
        {
        User::LeaveIfError( iDatabase.Exec( KThumbnailSqlDeleteFromDeleted ) ); 
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
    TN_DEBUG1( "CThumbnailStore::FileExistenceCheckL()" );
    
#ifdef _DEBUG
    TTime aStart, aStop;
    aStart.UniversalTime();
#endif
    
    User::LeaveIfError( CheckDbState() );
    
    TInt paramIndex = 0;
    TInt rowStatus = 0; 
    TInt column = 0;
    TInt64 rowid = 0;
    TFileName path;
    TFileName prevPath;
    TFileName full;
    TInt count = 0;
    
    TBool finished = EFalse;
    
    RThumbnailTransaction transaction( iDatabase );
    CleanupClosePushL( transaction );    
    transaction.BeginL();
    
    // get rows    
    RSqlStatement* stmt = NULL;
    stmt = &iStmt_KThumbnailSelectAllPaths;
    CleanupStack::PushL(TCleanupItem(ResetStatement, stmt));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamRowID );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt64( paramIndex, iLastCheckedRowID ));
    
    paramIndex = stmt->ParameterIndex( KThumbnailSqlParamLimit );
    User::LeaveIfError( paramIndex );
    User::LeaveIfError( stmt->BindInt( paramIndex, KStoreMaintenanceExistLimit ));
             
    rowStatus = stmt->Next();     
           
    while(rowStatus == KSqlAtRow)
        {
        column = 0;
        path.Zero();
        
        rowid = stmt->ColumnInt64( column++ );
        stmt->ColumnText( column, path );
    
        full.Zero();
        full.Append(iDriveChar);
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
        rowStatus = stmt->Next();
        }
    
    if (count < KStoreMaintenanceExistLimit)
        {
        // all rows checked
        finished = ETrue;
        }
        
    CleanupStack::PopAndDestroy( stmt );    
    
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
    TInt pos2 = aPath.Find(KBackSlash);
    
    // if URI contains drive letter
    if ( pos == 1 )
        {
        // normal URI
        if ( pos2 == 2 )
            {
            aPath.Delete(0,pos+1);
            }
        // virtual URI
        else
            {
            aPath.Replace(0,2,KBackSlash);
            }
        }
    }

// -----------------------------------------------------------------------------
// CheckDbState
// -----------------------------------------------------------------------------
//
TInt CThumbnailStore::CheckDbState()
    {
    if (iUnrecoverable)
        {
        TN_DEBUG1( "CThumbnailStore::CheckDbState() - database in unrecoverable state" );
        __ASSERT_DEBUG( !iUnrecoverable, ThumbnailPanic( EThumbnailDatabaseUnrecoverable ));
        
        return KStoreUnrecoverableErr;
        }
    else
        {
        return KErrNone;
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
        if(ret != KErrNone || MPXHarvesting)
            {
            TN_DEBUG3( "CThumbnailStore::ActivityChanged() KMPXHarvesting err == %d, MPXHarvesting == %d", ret, MPXHarvesting);
            iIdle = EFalse;
            return;
            }
        
        ret = RProperty::Get(KTAGDPSNotification, KDaemonProcessing, DaemonProcessing);
        if(ret != KErrNone || DaemonProcessing)
            {
            TN_DEBUG3( "CThumbnailStore::ActivityChanged() KDaemonProcessing err == %d DaemonProcessing == %d", ret, DaemonProcessing );
            iIdle = EFalse;
            return;
            }
        
        TN_DEBUG1( "CThumbnailStore::ActivityChanged() - starting maintenance");
        iIdle = ETrue;
        StartMaintenance();
        }
    }


// -----------------------------------------------------------------------------
// CThumbnailStoreDiskSpaceNotifierAO class
// -----------------------------------------------------------------------------
//
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
        
        User::LeaveIfError( iFileServerSession.Volume( volumeInfo, iDrive ) );
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
                User::LeaveIfError( iFileServerSession.Volume( volumeInfo, iDrive ) );
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
