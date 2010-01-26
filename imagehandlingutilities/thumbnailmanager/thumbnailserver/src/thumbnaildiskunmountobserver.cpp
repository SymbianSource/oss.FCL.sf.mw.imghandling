/*
* Copyright (c) 2007 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Class to monitor when volumes are unmounted
 *
*/


#include "thumbnaildiskunmountobserver.h"
#include "thumbnaillog.h"
#include "thumbnailserver.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailDiskUnmountObserver* CThumbnailDiskUnmountObserver::NewL( RFs& aFs,
    TInt aDrive, CThumbnailServer* aServer )
    {
    CThumbnailDiskUnmountObserver* self = new( ELeave )
        CThumbnailDiskUnmountObserver( aFs, aDrive, aServer );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::CThumbnailTaskProcessor()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailDiskUnmountObserver::CThumbnailDiskUnmountObserver( RFs& aFs,
    TInt aDrive, CThumbnailServer* aServer )
    : CActive( CActive::EPriorityStandard ), iFs( aFs ), iDrive( aDrive), iServer( aServer )
    {
    TN_DEBUG1( "CThumbnailDiskUnmountObserver::CThumbnailDiskUnmountObserver()"
        );
    CActiveScheduler::Add( this );
    StartNotify();
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailDiskUnmountObserver::ConstructL()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::~CThumbnailTaskProcessor()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailDiskUnmountObserver::~CThumbnailDiskUnmountObserver()
    {
    TN_DEBUG1( 
        "CThumbnailDiskUnmountObserver::~CThumbnailDiskUnmountObserver()" );
    Cancel();
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailDiskUnmountObserver::RunL()
    {
    TN_DEBUG2( "CThumbnailDiskUnmountObserver::RunL() iStatus = %d",
        iStatus.Int());
    
    if( !iStatus.Int() )
        {       
        // close store before allowing unmount
        // trap because nothing could be done in RunError anyway
        TRAP_IGNORE( iServer->CloseStoreForDriveL( iDrive ) );
        iFs.AllowDismount( iDrive );
        }
    if ( iStatus.Int() != KErrNotReady)
        {
        StartNotify();
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailDiskUnmountObserver::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailDiskUnmountObserver::DoCancel()
    {
    iFs.NotifyDismountCancel( iStatus );
    }

// ---------------------------------------------------------------------------
// CThumbnailDiskUnmountObserver::StartNotify()
// ---------------------------------------------------------------------------
//
void CThumbnailDiskUnmountObserver::StartNotify()
    {
    TN_DEBUG1( "CThumbnailDiskUnmountObserver::StartNotify()" );
    if(!IsActive())
        { 
        iFs.NotifyDismount( iDrive, iStatus, EFsDismountRegisterClient );
        SetActive();
        }
    }

// End of file
