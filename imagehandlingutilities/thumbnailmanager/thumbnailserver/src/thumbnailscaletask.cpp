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
* Description:  Task for scaling thumbnails.
 *
*/


#include <e32base.h>
#include <fbs.h>
#include <e32math.h>
#include <bitdev.h>
#include <bitstd.h>

#include "thumbnailscaletask.h"
#include "thumbnailprovider.h"
#include "thumbnailserver.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"
#include "thumbnailpanic.h"


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailScaleTask::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailScaleTask* CThumbnailScaleTask::NewL( CThumbnailTaskProcessor&
    aProcessor, CThumbnailServer& aServer, const TDesC& aFilename, CFbsBitmap*
    aBitmap, const TSize& aOriginalSize, const TSize& aTargetSize, TBool aCrop,
    TDisplayMode aDisplayMode, TInt aPriority, const TDesC& aTargetUri,
    const TThumbnailSize aThumbnailSize, const TInt64 aModified,
    TBool aBitmapToPool, const TBool aEXIF, const TThumbnailServerRequestId aRequestId,
    const TBool aImportVirtual)
    {
    // We take ownership of aBitmap
    CleanupStack::PushL( aBitmap );
    CThumbnailScaleTask* self = new( ELeave )CThumbnailScaleTask( aProcessor,
        aServer, aFilename, aBitmap, aOriginalSize, aTargetSize, aCrop,
        aDisplayMode, aPriority, aTargetUri, aThumbnailSize, aModified,
        aBitmapToPool, aEXIF, aRequestId, aImportVirtual);
    CleanupStack::Pop( aBitmap );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailScaleTask::CThumbnailScaleTask()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailScaleTask::CThumbnailScaleTask( CThumbnailTaskProcessor& aProcessor,
    CThumbnailServer& aServer, const TDesC& aFilename, CFbsBitmap* aBitmap,
    const TSize& aOriginalSize, const TSize& aTargetSize, TBool aCrop,
    TDisplayMode aDisplayMode, TInt aPriority, const TDesC& aTargetUri,
    const TThumbnailSize aThumbnailSize, const TInt64 aModified,
    TBool aBitmapToPool, const TBool aEXIF, const TThumbnailServerRequestId aRequestId,
    const TBool aVirtualUri):
    CThumbnailTask( aProcessor, aPriority ), iServer( aServer ), iOwnBitmap( aBitmap ),
    iOriginalSize( aOriginalSize ), iTargetSize(aTargetSize), iTargetSizeTN( aTargetSize ), iCrop( aCrop ),
    iDisplayMode( aDisplayMode ), iFilename( aFilename ), iTargetUri( aTargetUri ),
    iThumbnailSize(aThumbnailSize), iModified(aModified),
    iBitmapToPool(aBitmapToPool), iEXIF(aEXIF), iVirtualUri( aVirtualUri )
    {
    TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::CThumbnailScaleTask()", this );
    
    iRequestId = aRequestId;
    }


// ---------------------------------------------------------------------------
// CThumbnailScaleTask::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::ConstructL()
    {
    iServer.AddBitmapToPoolL( iRequestId.iSession, iOwnBitmap, iRequestId );

    // Successfully added bitmap to pool, we are no longer responsible for
    // deleting it directly.
    iBitmap = iOwnBitmap;
    iOwnBitmap = NULL;
    iBitmapInPool = ETrue;
    
    iScaledBitmap = NULL;
    }


// ---------------------------------------------------------------------------
// CThumbnailScaleTask::~CThumbnailScaleTask()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailScaleTask::~CThumbnailScaleTask()
    {
    iServer.CancelScale();
    
    if ( iBitmapInPool && iBitmap )
        {
        TN_DEBUG1("CThumbnailScaleTask()::~CThumbnailScaleTask() delete original bitmap from pool");
        
        // Original bitmap is owned by server, decrease reference count
        iServer.DeleteBitmapFromPool( iBitmap->Handle());
        }

    // Scaled bitmap is owned by us, delete now
    delete iScaledBitmap;
    }


// ---------------------------------------------------------------------------
// CThumbnailScaleTask::StartL()
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::StartL()
    {
    TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::StartL()", this );

    CThumbnailTask::StartL();

    if ( !iCrop )
        {
        TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::StartL() - cropping OFF", this );
    
        // target size at max, keep aspect ratio
        CalculateTargetSize();
        }
    else
        {
        TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::StartL() - cropping ON", this );
    
        // exact target size, crop excess
        CalculateCropRectangle();
        }
    
    TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::StartL() - sizes calculated", this );
    
#ifdef _DEBUG
    aStart.UniversalTime();
#endif
    
    delete iScaledBitmap;
    iScaledBitmap = NULL;
    iScaledBitmap = new( ELeave )CFbsBitmap();
    
    TSize bitmapSize = iBitmap->SizeInPixels();
    
    if(bitmapSize.iHeight == iTargetSize.iHeight && bitmapSize.iWidth == iTargetSize.iWidth)
        {
        TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::StartL() - no need for scaling", this);
    
        // copy bitmap 1:1
        User::LeaveIfError( iScaledBitmap->Create( bitmapSize, iBitmap->DisplayMode() ));
        CFbsBitmapDevice* device = CFbsBitmapDevice::NewL(iScaledBitmap);
        CleanupStack::PushL(device);
        CFbsBitGc* gc = NULL;
        User::LeaveIfError(device->CreateContext(gc));
        CleanupStack::PushL(gc);
        gc->BitBlt(TPoint(0, 0), iBitmap);
        CleanupStack::PopAndDestroy(2, device); // gc
        
        TRAPD( err, StoreAndCompleteL());
        Complete( err );
        ResetMessageData();
        }
    else
        {
        TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::StartL() - scaling", this);
        
        User::LeaveIfError( iScaledBitmap->Create( iTargetSize, iBitmap->DisplayMode() ));
        iServer.ScaleBitmapL( iStatus, * iBitmap, * iScaledBitmap, iCropRectangle );
        SetActive();
        }  
    
    TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::StartL() end", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailScaleTask::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::RunL()
    {
    TInt err = iStatus.Int();

    TN_DEBUG3( "CThumbnailScaleTask(0x%08x)::RunL() err=%d)", this, err );

    #ifdef _DEBUG
    aStop.UniversalTime();
    TN_DEBUG2( "CThumbnailScaleTask::RunL() scale took %d ms", (TInt)aStop.MicroSecondsFrom(aStart).Int64()/1000);
    #endif

    if ( !err )
        {
        TRAP( err, StoreAndCompleteL());
        }
    
    Complete( err );
    ResetMessageData();
    }


// ---------------------------------------------------------------------------
// CThumbnailScaleTask::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::DoCancel()
    {
    TN_DEBUG2( "CThumbnailScaleTask(0x%08x)::DoCancel()", this );
    iServer.CancelScale();
    }


// ---------------------------------------------------------------------------
// Calculates target size to be used for the thumbnail
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::CalculateTargetSize()
    {
    __ASSERT_DEBUG( iOriginalSize.iHeight && iTargetSize.iHeight,
        ThumbnailPanic( EThumbnailBadSize ));
    
    if ( (iThumbnailSize == EFullScreenThumbnailSize ||
          iThumbnailSize == EImageFullScreenThumbnailSize ||
          iThumbnailSize == EVideoFullScreenThumbnailSize ||
          iThumbnailSize == EAudioFullScreenThumbnailSize) &&
          iOriginalSize.iHeight < iTargetSize.iHeight && 
          iOriginalSize.iWidth < iTargetSize.iWidth )
        {
        // do not upscale fullscreen thumbs
        iTargetSize = iOriginalSize;
        }
    else if ( iOriginalSize.iHeight && iTargetSize.iHeight )
        {
        const TReal32 srcAspect = static_cast < TReal32 > (
            iOriginalSize.iWidth ) / iOriginalSize.iHeight;

        // scale to maximum size within target size 
        if ( (iTargetSize.iHeight * srcAspect) <= iTargetSize.iWidth )
            {
            TReal trg = 0;
            TReal src( iTargetSize.iHeight * srcAspect );
            Math::Round( trg, src, 0 );
            iTargetSize.SetSize( trg, iTargetSize.iHeight );
            }
        else
            {
            TReal trg;
            TReal src( iTargetSize.iWidth / srcAspect );
            Math::Round( trg, src, 0 );
            iTargetSize.SetSize( iTargetSize.iWidth, trg );
            }
        }
    else
        {
        iTargetSize.SetSize( 0, 0 );
        }
    iCropRectangle.SetRect( TPoint(), iBitmap->SizeInPixels());
    }


// ---------------------------------------------------------------------------
// Calculates target size to be used for the thumbnail
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::CalculateCropRectangle()
    {
    const TSize srcSize = iBitmap->SizeInPixels();

    __ASSERT_DEBUG( srcSize.iHeight && iTargetSize.iHeight, ThumbnailPanic(
        EThumbnailBadSize ));

    if ( srcSize.iHeight && iTargetSize.iHeight )
        {
        const TReal32 srcAspect = static_cast < TReal32 > ( srcSize.iWidth ) /
            srcSize.iHeight;
        const TReal32 reqAspect = static_cast < TReal32 > ( iTargetSize.iWidth )
            / iTargetSize.iHeight;

        if ( (iTargetSize.iHeight * srcAspect) > iTargetSize.iWidth )
            {
            // Thumbnail is wider than requested and we crop
            // some of the right and left parts.
            TReal trg;
            TReal src( srcSize.iHeight* reqAspect );
            Math::Round( trg, src, 0 );
            const TSize cropSize( trg, srcSize.iHeight );
            iCropRectangle.iTl.SetXY(( srcSize.iWidth - cropSize.iWidth ) / 2, 0 );
            iCropRectangle.SetSize( cropSize );
            }
        else
            {
            // Thumbnail is taller than requested and we crop
            // some of the top and bottom parts.
            TReal trg;
            TReal src( srcSize.iWidth / reqAspect );
            Math::Round( trg, src, 0 );
            const TSize cropSize( srcSize.iWidth, trg );
            iCropRectangle.iTl.SetXY(0, ( srcSize.iHeight - cropSize.iHeight ) / 2 );
            iCropRectangle.SetSize( cropSize );
            }

        }
    else
        {
        iTargetSize.SetSize( 0, 0 );
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailScaleTask::StoreAndCompleteL()
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::StoreAndCompleteL()
    {
    TN_DEBUG6( "CThumbnailScaleTask(0x%08x)::StoreAndCompleteL() iFilename=%S, iThumbnailSize=%d, iBitmap=0x%08x, iScaledBitmap=0x%08x)", 
               this, &iFilename, iThumbnailSize, iBitmap, iScaledBitmap );
		 
    // do not store TN if quality is too low eg. orignal size of image is smaller than requested size
    // (do not store upscaled images)
    if ( (iTargetSizeTN.iWidth > iOriginalSize.iWidth || iTargetSizeTN.iHeight > iOriginalSize.iHeight) && iEXIF)
        {
        TN_DEBUG1("CThumbnailScaleTask()::StoreAndCompleteL() too low quality");
        iDoStore = EFalse;
        }
    
    TN_DEBUG3("CThumbnailScaleTask(0x%08x)::StoreAndCompleteL() iDoStore = %d", this, iDoStore);
    
    if ( iDoStore )
        {
        if (iTargetUri != KNullDesC)
            {
            if (iFilename != KNullDesC && iFilename.CompareF(iTargetUri) == 0)
                {
                // filename and target URI match, so thumb created from associated path
                iServer.StoreThumbnailL( iTargetUri, iScaledBitmap, iOriginalSize, iCrop, 
                                         iThumbnailSize, iModified, !iVirtualUri, !iVirtualUri );
                }
            else
                {
                // thumb not created from associated path
                iServer.StoreThumbnailL( iTargetUri, iScaledBitmap, iOriginalSize, iCrop, 
                                         iThumbnailSize, iModified, !iVirtualUri, EFalse );
                }  
            }
        else if (iFilename != KNullDesC)
            {
            iServer.StoreThumbnailL( iFilename, iScaledBitmap, iOriginalSize, iCrop, 
                                     iThumbnailSize, iModified, !iVirtualUri, !iVirtualUri );
            }
        }    
    
    if ( ClientThreadAlive() )
        {
        TThumbnailRequestParams& params = iParamsBuf();
        iMessage.ReadL( 0, iParamsBuf );
                    
        // if need to add scaled bitmap to pool
        if (iBitmapToPool)
            {
            TN_DEBUG1("CThumbnailScaleTask()::StoreAndCompleteL() scaled bitmap handle to params");
            
            params.iBitmapHandle = iScaledBitmap->Handle();
            }    
		
	    if( params.iQualityPreference == CThumbnailManager::EOptimizeForQualityWithPreview
	        && iEXIF && !iDoStore)
	        {
            TN_DEBUG1("CThumbnailScaleTask()::StoreAndCompleteL() EThumbnailPreviewThumbnail");
	    
		    // this is upscaled preview image
	        params.iControlFlags = EThumbnailPreviewThumbnail;
	        }
	    
        TN_DEBUG1("CThumbnailScaleTask()::StoreAndCompleteL() write params to message");
        
	    // pass bitmap handle to client
	    iMessage.WriteL( 0, iParamsBuf );
	    
        if (iBitmapToPool)
            {
            TN_DEBUG1("CThumbnailScaleTask()::StoreAndCompleteL() scaled bitmap to pool");
        
            iServer.AddBitmapToPoolL( iRequestId.iSession, iScaledBitmap, iRequestId );
            iScaledBitmap = NULL; // Server owns the bitmap now
            }
        }
    
    TN_DEBUG1("CThumbnailScaleTask()::StoreAndCompleteL() - end");
    }


// ---------------------------------------------------------------------------
// CThumbnailScaleTask::StoreAndCompleteL()
// Changes priority of the task.
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::ChangeTaskPriority( TInt /*aNewPriority*/ )
    {
    // The priority of scale tasks is fixed. Do nothing.
    }

// ---------------------------------------------------------------------------
// CThumbnailScaleTask::SetDoStore()
// Changes the store flag
// ---------------------------------------------------------------------------
//
void CThumbnailScaleTask::SetDoStore( TBool aDoStore )
    {
    iDoStore = aDoStore;
    }
