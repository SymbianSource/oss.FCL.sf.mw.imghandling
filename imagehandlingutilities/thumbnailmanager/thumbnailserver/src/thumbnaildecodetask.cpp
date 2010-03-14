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

#include "thumbnaildecodetask.h"
#include "thumbnailprovider.h"
#include "thumbnailserver.h"
#include "thumbnaillog.h"
#include "thumbnailpanic.h"


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailDecodeTask::CThumbnailDecodeTask()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailDecodeTask::CThumbnailDecodeTask( CThumbnailTaskProcessor& aProcessor, 
        CThumbnailServer& aServer, TDesC8* aBuffer, TInt aPriority, TDisplayMode aDisplayMode): CThumbnailTask( aProcessor,
    aPriority ), iServer( aServer ), iBuffer(aBuffer), iDisplayMode(aDisplayMode)
    {
    TN_DEBUG3( "CThumbnailDecodeTask(0x%08x)::CThumbnailDecodeTask() aDisplayMode = %d", this
        , iDisplayMode);
    }


// ---------------------------------------------------------------------------
// CThumbnailDecodeTask::~CThumbnailDecodeTask()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailDecodeTask::~CThumbnailDecodeTask()
    {
    if ( iProvider )
        {
        iProvider->CancelGetThumbnail();
        } 
    
    iProvider = NULL;
    delete iBuffer;
    iBuffer = NULL;
    }


// ---------------------------------------------------------------------------
// CThumbnailDecodeTask::StartL()
// ---------------------------------------------------------------------------
//
void CThumbnailDecodeTask::StartL()
    {
    TN_DEBUG2( "CThumbnailDecodeTask(0x%08x)::StartL()", this );

    CThumbnailTask::StartL();

    const TPtrC8 mimeType = KJpegMime();

    iProvider = iServer.ResolveProviderL( mimeType );
    TN_DEBUG3( "CThumbnailDecodeTask(0x%08x) -- provider UID 0x%08x", this,
        iProvider->Uid());

    __ASSERT_DEBUG(( iProvider ), ThumbnailPanic( EThumbnailNullPointer ));

    iProvider->CancelGetThumbnail();
    iProvider->Reset();
    iProvider->SetObserver( *this );

    iProvider->GetThumbnailL( iServer.Fs(), *iBuffer );
    }


// ---------------------------------------------------------------------------
// CThumbnailDecodeTask::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailDecodeTask::RunL()
    {
    // No implementation required
    TN_DEBUG2( "CThumbnailDecodeTask(0x%08x)::RunL()", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailDecodeTask::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailDecodeTask::DoCancel()
    {
    TN_DEBUG2( "CThumbnailDecodeTask(0x%08x)::DoCancel()", this );
    if ( iProvider )
        {
        iProvider->CancelGetThumbnail();
        }
    }


//
// ---------------------------------------------------------------------------
// Thumbnail provider observer callback to notify the server when
// thumbnail has been generated.
// ---------------------------------------------------------------------------
//
void CThumbnailDecodeTask::ThumbnailProviderReady( const TInt aError,
    CFbsBitmap* aBitmap, const TSize& aOriginalSize, const TBool /*aEXIF*/, const TBool /*aPortrait*/ )
    {
    TN_DEBUG4( "CThumbnailDecodeTask(0x%08x)::ThumbnailProviderReady(aError=%d, aBitmap=0x%08x)", 
               this, aError, aBitmap );

    iOriginalSize = aOriginalSize;

    if ( aError )
        {
        delete aBitmap;
        aBitmap = NULL;
        Complete( aError );
        return;
        }
    
    if( !aBitmap )
        {
        Complete( KErrGeneral );
        return;
        }
      
    if ( ClientThreadAlive() )
       {
       TRAP_IGNORE(iServer.AddBitmapToPoolL( iRequestId.iSession, aBitmap, iRequestId ));
       const TSize bitmapSize = aBitmap->SizeInPixels();
       iBitmapHandle = aBitmap->Handle();
       aBitmap = NULL;
       
       // Complete message and pass bitmap handle to client
       TThumbnailRequestParams& params = iParamsBuf();
       TInt ret = iMessage.Read( 0, iParamsBuf );
       
       if(ret == KErrNone )
           {
           params.iBitmapHandle = iBitmapHandle;
           ret = iMessage.Write( 0, iParamsBuf );
           }
       
       Complete( ret );
       ResetMessageData();

       // Successfully completed the message. The client will send
       // EReleaseBitmap message later to delete the bitmap from pool.
       // CThumbnailScaleTask is no longer responsible for that.
       iBitmapHandle = 0;
       }
    else
        {
        delete aBitmap;
        aBitmap = NULL;
        }
    }

