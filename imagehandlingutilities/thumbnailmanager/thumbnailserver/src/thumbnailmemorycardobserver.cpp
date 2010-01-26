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
* Description:  Class to monitor when memory card status is changed
 *
*/


#include "thumbnailmemorycardobserver.h"
#include "thumbnaillog.h"

// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailMemoryCardObserver::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailMemoryCardObserver* CThumbnailMemoryCardObserver::NewL( CThumbnailServer* aServer, RFs& aFs )
    {
    CThumbnailMemoryCardObserver* self = new( ELeave )
        CThumbnailMemoryCardObserver( aServer, aFs );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailMemoryCardObserver::CThumbnailMemoryCardObserver()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailMemoryCardObserver::CThumbnailMemoryCardObserver( CThumbnailServer* aServer, RFs& aFs ): 
    CActive( CActive::EPriorityStandard ), iServer( aServer ), iFs( aFs ) 
    {
    TN_DEBUG1( "CThumbnailMemoryCardObserver::CThumbnailMemoryCardObserver()"
        );
    CActiveScheduler::Add( this );
    StartNotify();
    }


// ---------------------------------------------------------------------------
// CThumbnailMemoryCardObserver::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailMemoryCardObserver::ConstructL()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::~CThumbnailTaskProcessor()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailMemoryCardObserver::~CThumbnailMemoryCardObserver()
    {
    TN_DEBUG1( 
        "CThumbnailMemoryCardObserver::~CThumbnailMemoryCardObserver()" );
    Cancel();
    }


// ---------------------------------------------------------------------------
// CThumbnailMemoryCardObserver::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailMemoryCardObserver::RunL()
    {
    TN_DEBUG2( "CThumbnailMemoryCardObserver::RunL() iStatus = %d", iStatus.Int());
    if ( !iStatus.Int() )
        {
        // trap because nothing could be done in RunError
        TRAP_IGNORE( iServer->MemoryCardStatusChangedL() );
        StartNotify();
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailMemoryCardObserver::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailMemoryCardObserver::DoCancel()
    {
    iFs.NotifyChangeCancel();
    }

// ---------------------------------------------------------------------------
// CThumbnailMemoryCardObserver::StartNotify()
// ---------------------------------------------------------------------------
//
void CThumbnailMemoryCardObserver::StartNotify()
    {
    TN_DEBUG1( "CThumbnailMemoryCardObserver::StartNotify()" );
    
    if (IsActive()) 
        {
        Cancel();
        }
    
    iFs.NotifyChange(ENotifyDisk, iStatus);
    SetActive();
    }

// End of file
