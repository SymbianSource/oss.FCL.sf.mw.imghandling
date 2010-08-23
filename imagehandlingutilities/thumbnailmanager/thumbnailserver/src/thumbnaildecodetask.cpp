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
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnaildecodetaskTraces.h"
#endif



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
    OstTrace1( TRACE_NORMAL, CTHUMBNAILDECODETASK_CTHUMBNAILDECODETASK, "CThumbnailDecodeTask::CThumbnailDecodeTask;iDisplayMode=%u", iDisplayMode );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILDECODETASK_CTHUMBNAILDECODETASK, "CThumbnailDecodeTask::CThumbnailDecodeTask;this=%o", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailDecodeTask::~CThumbnailDecodeTask()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailDecodeTask::~CThumbnailDecodeTask()
    {
    TN_DEBUG2( "CThumbnailDecodeTask(0x%08x)::~CThumbnailDecodeTask()", this );
    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILDECODETASK_CTHUMBNAILDECODETASK, "CThumbnailDecodeTask::~CThumbnailDecodeTask" );
    
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
    OstTrace1( TRACE_NORMAL, CTHUMBNAILDECODETASK_STARTL, "CThumbnailDecodeTask::StartL;this=%o", this );

    CThumbnailTask::StartL();

    const TPtrC8 mimeType = KJpegMime();

    iProvider = iServer.ResolveProviderL( mimeType );
    TN_DEBUG3( "CThumbnailDecodeTask(0x%08x) -- provider UID 0x%08x", this,
        iProvider->Uid());
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILDECODETASK_STARTL, "CThumbnailDecodeTask::StartL;this=%o", this );

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
    OstTrace1( TRACE_NORMAL, CTHUMBNAILDECODETASK_RUNL, "CThumbnailDecodeTask::RunL;this=%o", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailDecodeTask::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailDecodeTask::DoCancel()
    {
    TN_DEBUG2( "CThumbnailDecodeTask(0x%08x)::DoCancel()", this );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILDECODETASK_DOCANCEL, "CThumbnailDecodeTask::DoCancel;this=%o", this );
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
    OstTrace1( TRACE_NORMAL, CTHUMBNAILDECODETASK_THUMBNAILPROVIDERREADY, "CThumbnailDecodeTask::ThumbnailProviderReady;this=%o", this );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILDECODETASK_THUMBNAILPROVIDERREADY, "CThumbnailDecodeTask::ThumbnailProviderReady;aError=%d", aError );
    OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILDECODETASK_THUMBNAILPROVIDERREADY, "CThumbnailDecodeTask::ThumbnailProviderReady;aBitmap=%o", aBitmap );

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
       // pass bitmap handle to client
       TThumbnailRequestParams& params = iParamsBuf();
       TInt ret = iMessage.Read( 0, iParamsBuf );
       
       if(ret == KErrNone )
           {
           params.iBitmapHandle = aBitmap->Handle();
           ret = iMessage.Write( 0, iParamsBuf );
           }
       
       // add bitmap to pool
       TRAPD(err, iServer.AddBitmapToPoolL( iRequestId.iSession, aBitmap, iRequestId ) );
       if (err != KErrNone)
           {
           Complete( err );
           delete aBitmap;
           aBitmap = NULL;
           }
       else
           {
           aBitmap = NULL; // Server owns the bitmap now
       
           // Complete message
           Complete( ret );
           ResetMessageData();
           }
       }
    else
        {
        delete aBitmap;
        aBitmap = NULL;
        }
    }

