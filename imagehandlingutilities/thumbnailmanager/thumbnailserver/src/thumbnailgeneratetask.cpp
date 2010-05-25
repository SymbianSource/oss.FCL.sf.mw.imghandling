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
* Description:  Task for generating new thumbnails
 *
*/


#include <e32base.h>
#include <fbs.h>

#include <thumbnailmanager.h>
#include "thumbnailgeneratetask.h"
#include "thumbnailscaletask.h"
#include "thumbnailprovider.h"
#include "thumbnailserver.h"
#include "thumbnailtaskprocessor.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"
#include "thumbnailpanic.h"
#include "thumbnailfetchedchecker.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailGenerateTask::CThumbnailGenerateTask()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailGenerateTask::CThumbnailGenerateTask( CThumbnailTaskProcessor&
    aProcessor, CThumbnailServer& aServer, RFile64* aFile, TDesC8* aBuffer,
    const TDataType* aMimeType, CThumbnailManager::TThumbnailFlags aFlags,
    const TSize& aSize, TDisplayMode aDisplayMode, TInt aPriority,
    RArray < TThumbnailPersistentSize >* aMissingSizes, const TDesC& aTargetUri,
    TThumbnailSize aThumbnailSize, const TInt64 aModified, 
    const CThumbnailManager::TThumbnailQualityPreference aQualityPreference ): 
    CThumbnailTask( aProcessor, aPriority ), iServer( aServer ), 
    iFlags( aFlags ), iSize( aSize ), iDisplayMode( aDisplayMode ),
    iMissingSizes( aMissingSizes ), iTargetUri( aTargetUri ),
    iThumbnailSize( aThumbnailSize ), iModified(aModified),
    iQualityPreference( aQualityPreference )
    {
    TN_DEBUG2( "CThumbnailGenerateTask(0x%08x)::CThumbnailGenerateTask()", this);
    
    if ( !aBuffer && aFile)
        {
        iFile = *aFile; 
        iFile.FullName( iFilename );
        }
    else if( aBuffer )
        {
        iBuffer = aBuffer;
        }
    if ( aMimeType )
        {
        iMimeType = *aMimeType;
        }
    else
        {
        iMimeType = TDataType(KNullDesC8);
        }
    
    // scaled bitmaps to pool by default
    iScaledBitmapToPool = ETrue;
    iBitmapHandle = 0;
    }


// ---------------------------------------------------------------------------
// CThumbnailGenerateTask::~CThumbnailGenerateTask()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailGenerateTask::~CThumbnailGenerateTask()
    {
    TN_DEBUG2("CThumbnailGenerateTask(0x%08x)::~CThumbnailGenerateTask()", this);
    
    if ( iProvider )
        {
        iProvider->CancelGetThumbnail();
        }
	
    if ( iMissingSizes )
        {
        iMissingSizes->Reset();
        delete iMissingSizes;
        }
		
    if ( iBitmapHandle )
        {
        iServer.DeleteBitmapFromPool( iBitmapHandle );
        iBitmapHandle = 0;
        }
		
    iProvider = NULL;
    delete iBuffer;
    iBuffer = NULL;
    
    iFile.Close();
    TN_DEBUG1("CThumbnailGenerateTask::~CThumbnailGenerateTask() - file handle closed");
    }


// ---------------------------------------------------------------------------
// CThumbnailGenerateTask::StartL()
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::StartL()
    {
    TN_DEBUG2( "CThumbnailGenerateTask(0x%08x)::StartL()", this );

    CThumbnailTask::StartL();
    
#ifdef _DEBUG
    aStart.UniversalTime();
#endif

      
    TParsePtrC parse(iFilename);
    TPtrC ext(parse.Ext());
    TBuf8< KMaxDataTypeLength > mimeType;                
    if  (ext.CompareF(KNonEmbeddedArtExt)== 0) 
        {       
        mimeType.Copy( KImageMime );  		
        }
    else
        {
        mimeType = iMimeType.Des8();
        }
    iProvider = iServer.ResolveProviderL(mimeType);
       	
    TN_DEBUG3( "CThumbnailGenerateTask(0x%08x) -- provider UID 0x%08x", this,
        iProvider->Uid());

    __ASSERT_DEBUG(( iProvider ), ThumbnailPanic( EThumbnailNullPointer ));

    iProvider->CancelGetThumbnail();
    iProvider->Reset();
    iProvider->SetObserver( *this );

    TSize effectiveTargetSize;
    TSize croppedTargetSize;

    CalculateTargetSizesL( effectiveTargetSize, croppedTargetSize );

    iProvider->SetTargetSize( effectiveTargetSize );
      
    if(effectiveTargetSize == TSize())
        {
        iProvider->SetTargetSize( croppedTargetSize );
        }
	
    TInt providerErr;
    
    if ( !iBuffer )
        {
        TRAP(providerErr, iProvider->GetThumbnailL( iServer.Fs(), iFile, iMimeType, iFlags,
            iDisplayMode, iQualityPreference ) );
        }
    else
        {
        TRAP( providerErr, iProvider->GetThumbnailL( iServer.Fs(), iBuffer, iMimeType, iFlags,
            iDisplayMode, iQualityPreference ));
        }
    
    DoBlacklisting( providerErr, TSize(0,0) );
    
    User::LeaveIfError( providerErr );
    
    TN_DEBUG2( "CThumbnailGenerateTask(0x%08x)::StartL() end", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailGenerateTask::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::RunL()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailGenerateTask::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::DoCancel()
    {
    TN_DEBUG2( "CThumbnailGenerateTask(0x%08x)::DoCancel()", this );
    if ( iProvider )
        {
        iProvider->CancelGetThumbnail();
        }
    }


// ---------------------------------------------------------------------------
// Calculate the largest non-cropped size and largest cropped size and
// let the provider plug-in know the values
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::CalculateTargetSizesL( TSize& aEffectiveTargetSize,
    TSize& aCroppedTargetSize )
    { 
    if ( iThumbnailSize == ECustomThumbnailSize )
        {
        if ( iFlags& CThumbnailManager::ECropToAspectRatio )
            {
            aCroppedTargetSize = iSize;
            }
        else
            {
            aEffectiveTargetSize = iSize;
            }       
        }
    
    if ( iThumbnailSize != ECustomThumbnailSize )
        {
        RArray < TThumbnailPersistentSize > sizes = iServer.PersistentSizesL();
        const TInt count = sizes.Count();
        for ( TInt i = 0; i < count; i++ )
            {
            TThumbnailPersistentSize size( sizes[i] );     

            iSize = size.iSize;
            if ( size.iCrop )
               {
               aCroppedTargetSize.iWidth = Max( aCroppedTargetSize.iWidth,
                   size.iSize.iWidth );
               aCroppedTargetSize.iHeight = Max( aCroppedTargetSize.iHeight,
                   size.iSize.iHeight );
               
               if(iBuffer)
                 {
                 iFlags = ( CThumbnailManager::TThumbnailFlags ) (iFlags | CThumbnailManager::ECropToAspectRatio);
                 aEffectiveTargetSize = aCroppedTargetSize;
                 }
               }
            else
               {
                aEffectiveTargetSize.iWidth = Max( aEffectiveTargetSize.iWidth,
                   size.iSize.iWidth );
                aEffectiveTargetSize.iHeight = Max( aEffectiveTargetSize.iHeight,
                   size.iSize.iHeight );
               }
            
            if(size.iType == iThumbnailSize)
              {
              break;
              }     
            }
        }    
    }

// ---------------------------------------------------------------------------
// Thumbnail provider observer callback to notify the server when
// thumbnail has been generated.
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::ThumbnailProviderReady( const TInt aError,
    CFbsBitmap* aBitmap, const TSize& aOriginalSize, const TBool aEXIF, const TBool aPortrait )
    {
    TN_DEBUG4( 
        "CThumbnailGenerateTask(0x%08x)::ThumbnailProviderReady(aError=%d, aBitmap=0x%08x)", this, aError, aBitmap );

    #ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailGenerateTask::ThumbnailProviderReady() generate took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
    #endif
    
    iPortrait = aPortrait;
    iEXIF = aEXIF;
    iOriginalSize = aOriginalSize;

    if ( aError )
        {
        delete aBitmap;
        aBitmap = NULL;

        DoBlacklisting( aError, aOriginalSize );
            
        Complete( aError );
        }
    else
        {
        // CreateScaleTasksL will take ownership of bitmap
        
#ifdef _DEBUG
        TN_DEBUG2( "CThumbnailGenerateTask::ThumbnailProviderReady() - displaymode is %d", aBitmap->DisplayMode());
#endif
        
        TRAPD( err, CreateScaleTasksL( aBitmap ));
        aBitmap = NULL;
        // If CreateScaleTasksL left, we are responsible for completing
        // the RMessage. If there was no leave, then this call will not
        // complete actual the message, just the mark the task as completed.
        Complete( err );
        }
    }


// ---------------------------------------------------------------------------
// Create tasks to scale the thumbnail to each persistent thumbnail size
// and also to the size the client requested
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::CreateScaleTasksL( CFbsBitmap* aBitmap )
    {
    __ASSERT_DEBUG(( aBitmap ), ThumbnailPanic( EThumbnailNullPointer ));

    CleanupStack::PushL( aBitmap );
    iServer.AddBitmapToPoolL( iRequestId.iSession, aBitmap, iRequestId );

    // Keep handle so we can delete bitmap from pool
    iBitmapHandle = aBitmap->Handle();
    CleanupStack::Pop( aBitmap );

    // compTask is the scale task which returns the bitmap to the client
    CThumbnailScaleTask* complTask = NULL;
	
    if ( iMissingSizes )
        {
        const TInt count = iMissingSizes->Count();
        
        for ( TInt i( 0 ); i < count; i++ )
            {
            TThumbnailSize size = (*iMissingSizes)[ i ].iType;
#ifdef _DEBUG
            TN_DEBUG3( "CThumbnailGenerateTask(0x%08x)::CreateScaleTasksL() *iMissingSizes)[ i ].iType == %d", this, size );
#endif
            
            if ( iPortrait )
                {
                if ( size == EFullScreenThumbnailSize ||
                     size == EVideoFullScreenThumbnailSize ||
                     size == EAudioFullScreenThumbnailSize ||
                     size == EImageFullScreenThumbnailSize )
                    {
                    TN_DEBUG2( "*iMissingSizes)[ i ].iWidth == %d", (*iMissingSizes)[ i ].iSize.iWidth );
                    TN_DEBUG2( "*iMissingSizes)[ i ].iHeight == %d", (*iMissingSizes)[ i ].iSize.iHeight );
                    TInt width = (*iMissingSizes)[ i ].iSize.iWidth; 
                    (*iMissingSizes)[ i ].iSize.iWidth = (*iMissingSizes)[ i ].iSize.iHeight;
                    (*iMissingSizes)[ i ].iSize.iHeight = width;
                    TN_DEBUG2( "*iMissingSizes)[ i ].iWidth == %d", (*iMissingSizes)[ i ].iSize.iWidth );
                    TN_DEBUG2( "*iMissingSizes)[ i ].iHeight == %d", (*iMissingSizes)[ i ].iSize.iHeight );
                    }
                }
            
            CThumbnailScaleTask* scaleTask = CThumbnailScaleTask::NewL( iProcessor, iServer, iFilename,
                aBitmap, iOriginalSize, (*iMissingSizes)[ i ].iSize, (*iMissingSizes)[ i ].iCrop, iDisplayMode,
                KMaxPriority, iTargetUri, (*iMissingSizes)[ i ].iType, iModified, iScaledBitmapToPool, iEXIF,
                iRequestId);
            CleanupStack::PushL( scaleTask );
            
            TInt err1 = KErrNone;
            TInt err2 = KErrNone;
            if(iFilename != KNullDesC)
                {
                TRAP(err1, iServer.StoreForPathL(iFilename));
                }
            if(iTargetUri != KNullDesC)
                {
                TRAP(err2, iServer.StoreForPathL(iTargetUri));
                }
            // if trying to access Z drive, don't try to store
            // don't want to store custom sizes
            if( err1 == KErrAccessDenied || err2 == KErrAccessDenied ||
                    (*iMissingSizes)[ i ].iType == ECustomThumbnailSize || 
                    (*iMissingSizes)[ i ].iType == EUnknownThumbnailSize )
                {
                scaleTask->SetDoStore( EFalse );
                TN_DEBUG2( "CThumbnailGenerateTask(0x%08x)::CreateScaleTasksL() - do not store", this );
                }
            else
                {
                scaleTask->SetDoStore( ETrue );
                }
            
            iProcessor.AddTaskL( scaleTask );
            CleanupStack::Pop( scaleTask );
            
            // completion to first task, because task processor works like stack
            if( i == 0 )
                {
                // compTask is now responsible for completing the RMessage
                scaleTask->SetMessageData( iRequestId, iMessage, iClientThread );
                ResetMessageData();
                }
            }
        }
    else
        {
        if( iPortrait )
            {
            if ( iThumbnailSize == EFullScreenThumbnailSize ||
                 iThumbnailSize == EVideoFullScreenThumbnailSize ||
                 iThumbnailSize == EAudioFullScreenThumbnailSize ||
                 iThumbnailSize == EImageFullScreenThumbnailSize )
                {
                TInt width = iSize.iWidth; 
                iSize.iWidth = iSize.iHeight;
                iSize.iHeight = width;
                }
            }
        
        complTask = CThumbnailScaleTask::NewL( iProcessor, iServer, iFilename,
            aBitmap, iOriginalSize, iSize, iFlags& CThumbnailManager
            ::ECropToAspectRatio, iDisplayMode, KMaxPriority, iTargetUri,
            iThumbnailSize, iModified, iScaledBitmapToPool, iEXIF, iRequestId );
        CleanupStack::PushL( complTask );
        
        TInt err1 = KErrNone;
        TInt err2 = KErrNone;
        if(iFilename != KNullDesC)
            {
            TRAP(err1, iServer.StoreForPathL(iFilename));
            }
        if(iTargetUri != KNullDesC)
            {
            TRAP(err2, iServer.StoreForPathL(iTargetUri));
            }
        // if trying to access Z drive, don't try to store
        // don't want to store custom sizes
        if( err1 == KErrAccessDenied || err2 == KErrAccessDenied ||
            iThumbnailSize == ECustomThumbnailSize || 
            iThumbnailSize == EUnknownThumbnailSize )
            {
            complTask->SetDoStore( EFalse );
            TN_DEBUG2( "CThumbnailGenerateTask(0x%08x)::CreateScaleTasksL() - do not store", this );
            }
        else
            {
            complTask->SetDoStore( ETrue );
            }
        
        iProcessor.AddTaskL( complTask );
        CleanupStack::Pop( complTask );
        
        // compTask is now responsible for completing the RMessage and
        // returning the bitmap to the client
        complTask->SetMessageData( iRequestId, iMessage, iClientThread );
        ResetMessageData();
        }

    // Scale tasks now reference the bitmap in the pool
    iServer.DeleteBitmapFromPool( iBitmapHandle );
    iBitmapHandle = 0;
    aBitmap = NULL;
    }

// ---------------------------------------------------------------------------
// Defines if scaled bitmaps need to be added to pool
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::ScaledBitmapToPool( TBool aBool )
    {
    iScaledBitmapToPool = aBool;
    }

// ---------------------------------------------------------------------------
// Stores a blacklisted thumbnail
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::CreateBlackListedL( const TSize& aOriginalSize )
    {
    TN_DEBUG1( "CThumbnailGenerateTask::CreateBlackListedL()");
    CFbsBitmap* tempBitmap = 0;
    tempBitmap = new (ELeave) CFbsBitmap();
    CleanupStack::PushL( tempBitmap );
    TSize tempSize( 1, 1 );
    User::LeaveIfError( tempBitmap->Create( tempSize, iDisplayMode ) );
    
    // Instead of creating a blacklisted entry of requested size (iThumbnailSize) in thumbnail db,
    // consider blacklisting all sizes (hence the changes are needed in thumbnail fetching logic too).
    // However, decoding of source to thumnail could succeed in other sizes, which makes blacklisting
    // of requested size only meaningful. 
    
    if(iFilename != KNullDesC)
        {
        iServer.StoreForPathL( iFilename )->StoreThumbnailL( 
            iFilename, tempBitmap, aOriginalSize, EFalse, iThumbnailSize, iModified, EFalse, ETrue );
		//remove result from fetched checker
        iServer.FetchedChecker().SetFetchResult( iFilename, KErrNone );
        }
    else if(iTargetUri != KNullDesC)
        {
        iServer.StoreForPathL( iTargetUri )->StoreThumbnailL( 
            iTargetUri, tempBitmap, aOriginalSize, EFalse, iThumbnailSize, iModified, EFalse, ETrue );
		//remove result from fetched checker
        iServer.FetchedChecker().SetFetchResult( iTargetUri, KErrNone );
        }

    CleanupStack::PopAndDestroy( tempBitmap );
    }

// ---------------------------------------------------------------------------
// Checks is blacklisting needed
// ---------------------------------------------------------------------------
//
void CThumbnailGenerateTask::DoBlacklisting( const TInt aError, const TSize& aOriginalSize )
    {
    TN_DEBUG1( "CThumbnailGenerateTask::DoBlacklisting()");
    // Create a temporary bitmap of size 1 for storing blacklisted thumbnail
    // Because no actual bitmap data is generated, there is no reason to 
    // add bitmap to server bitmap pool. Completion of client request with
    // error code just results in applications showing their default bitmap. 
    if( aError != KErrNone && (iFilename != KNullDesC || iTargetUri != KNullDesC ))
        {
        if (aError == KErrNotFound ||
            aError == KErrNotSupported ||
            aError == KErrCorrupt ||
            aError == KErrCompletion ||
            aError == KErrUnderflow ||
            aError == KErrNotReady || 
            aError == KErrGeneral )
            {
        
        if(iMissingSizes)
            {
            TN_DEBUG2( "CThumbnailGenerateTask::DoBlacklisting() - blacklist missing sizes count = %d", iMissingSizes->Count() );

            for ( TInt i( 0 ); i < iMissingSizes->Count(); i++ )
                {
                iThumbnailSize = (*iMissingSizes)[ i ].iType;
                TRAPD( err, CreateBlackListedL( aOriginalSize ) );
                if (err != KErrNone)
                   {
                   TN_DEBUG3( "CThumbnailGenerateTask::DoBlacklisting() - blacklisting missing size %d failed with code %d", iThumbnailSize, err );
                   }
                }
            return;
            }
        else
            {
            TN_DEBUG1( "CThumbnailGenerateTask::DoBlacklisting() - blacklist single size" );
            TRAPD( err, CreateBlackListedL( aOriginalSize ) );
            if (err != KErrNone)
                {
                TN_DEBUG2( "CThumbnailGenerateTask::DoBlacklisting() - blacklisting failed with code %d", err );
                }
            return;
            }
        }
    TN_DEBUG1( "CThumbnailGenerateTask::DoBlacklisting() - not blacklisted " );        
    }
}


