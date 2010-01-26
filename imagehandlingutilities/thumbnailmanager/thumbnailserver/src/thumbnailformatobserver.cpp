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
* Description:  File System format monitor
*
*/

#include "thumbnailformatobserver.h"
#include "thumbnaillog.h"
 
#include <e32base.h>
#include <f32file.h>


// ======== MEMBER FUNCTIONS ========

CThumbnailFormatObserver::CThumbnailFormatObserver ( CThumbnailServer* aServer): 
     iServer( aServer )
    {
    TN_DEBUG1( "CThumbnailFormatObserver::CThumbnailFormatObserver()");
    
    }
    
    
// ---------------------------------------------------------------------------
// Second Phase Constructor
// ---------------------------------------------------------------------------
//
void CThumbnailFormatObserver::ConstructL()
    {
    TN_DEBUG1("CThumbnailFormatObserver::ConstructL in");

    iBackupSession = CBaBackupSessionWrapper::NewL();
    iBackupSession->RegisterBackupOperationObserverL( *this );


    TN_DEBUG1("CThumbnailFormatObserver::ConstructL out");
    }


// ---------------------------------------------------------------------------
// Two-Phased Constructor
// ---------------------------------------------------------------------------
//
CThumbnailFormatObserver* CThumbnailFormatObserver::NewL(CThumbnailServer* aServer )
    {
    CThumbnailFormatObserver* self = CThumbnailFormatObserver::NewLC( aServer );
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// Two-Phased Constructor
// ---------------------------------------------------------------------------
//
CThumbnailFormatObserver* CThumbnailFormatObserver::NewLC( CThumbnailServer* aServer )
    {
    CThumbnailFormatObserver* self = new( ELeave ) CThumbnailFormatObserver( aServer );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }


// ---------------------------------------------------------------------------
// destructor
// ---------------------------------------------------------------------------
//
CThumbnailFormatObserver::~CThumbnailFormatObserver()
    {

    if( iBackupSession )
        {
        iBackupSession->DeRegisterBackupOperationObserver( *this );
        }
    delete iBackupSession;
    }

// ---------------------------------------------------------------------------
// Checks the current status
// ---------------------------------------------------------------------------
//
void CThumbnailFormatObserver::PollStatus()
    {
    
    TN_DEBUG1("CThumbnailFormatObserver::PollStatus()");
    
    TBool aFormatting = iBackupSession->IsBackupOperationRunning();
    
    if( aFormatting )
        {        
        TRAP_IGNORE(iServer->CloseRemovableDrivesL());   
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailFormatObserver::HandleBackupOperationEventL
// Handles a format operation
// ---------------------------------------------------------------------------
//
void CThumbnailFormatObserver::HandleBackupOperationEventL(
                  const TBackupOperationAttributes& aBackupOperationAttributes)
    {
    TN_DEBUG1("CThumbnailFormatObserver::HandleBackupOperationEventL in");

    if( aBackupOperationAttributes.iOperation == EStart )
        {
        TRAP_IGNORE(iServer->CloseRemovableDrivesL());
        }
    else  // TOperationType::EEnd or TOperationType::EAbort
        {
        TRAP_IGNORE(iServer->OpenRemovableDrivesL());
        }

    TN_DEBUG1("CThumbnailFormatObserver::HandleBackupOperationEventL out");
    }
