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

#include "tmformatobserver.h"
#include "thumbnaillog.h"
 
#include <e32base.h>
#include <f32file.h>
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "tmformatobserverTraces.h"
#endif


// ======== MEMBER FUNCTIONS ========

CTMFormatObserver::CTMFormatObserver ( MTMFormatObserver& aObserver ): 
    iObserver( aObserver )
    {
    TN_DEBUG1( "CTMFormatObserver::CTMFormatObserver()");
    OstTrace0( TRACE_NORMAL, CTMFORMATOBSERVER_CTMFORMATOBSERVER, "CTMFormatObserver::CTMFormatObserver" );
    }
    
    
// ---------------------------------------------------------------------------
// Second Phase Constructor
// ---------------------------------------------------------------------------
//
void CTMFormatObserver::ConstructL()
    {
    TN_DEBUG1("CTMFormatObserver::ConstructL");
    OstTrace0( TRACE_NORMAL, CTMFORMATOBSERVER_CONSTRUCTL, "CTMFormatObserver::ConstructL" );

    iBackupSession = CBaBackupSessionWrapper::NewL();
    iBackupSession->RegisterBackupOperationObserverL( *this );
    }


// ---------------------------------------------------------------------------
// Two-Phased Constructor
// ---------------------------------------------------------------------------
//
CTMFormatObserver* CTMFormatObserver::NewL( MTMFormatObserver& aObserver )
    {
    CTMFormatObserver* self = CTMFormatObserver::NewLC( aObserver );
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// Two-Phased Constructor
// ---------------------------------------------------------------------------
//
CTMFormatObserver* CTMFormatObserver::NewLC( MTMFormatObserver& aObserver )
    {
    CTMFormatObserver* self = new( ELeave ) CTMFormatObserver( aObserver );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }


// ---------------------------------------------------------------------------
// destructor
// ---------------------------------------------------------------------------
//
CTMFormatObserver::~CTMFormatObserver()
    {
    if( iBackupSession )
        {
        iBackupSession->DeRegisterBackupOperationObserver( *this );
        }
    
    delete iBackupSession;
    iBackupSession = NULL;
    }

// ---------------------------------------------------------------------------
// Checks the current status
// ---------------------------------------------------------------------------
//
void CTMFormatObserver::PollStatus()
    { 
    TN_DEBUG1("CTMFormatObserver::PollStatus()");
    OstTrace0( TRACE_NORMAL, CTMFORMATOBSERVER_POLLSTATUS, "CTMFormatObserver::PollStatus" );
    
    TBool formatting = iBackupSession->IsBackupOperationRunning();
    
    if( formatting )
        {     
        iObserver.FormatNotification(ETrue); 
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailFormatObserver::HandleBackupOperationEventL
// Handles a format operation
// ---------------------------------------------------------------------------
//
void CTMFormatObserver::HandleBackupOperationEventL(
                  const TBackupOperationAttributes& aBackupOperationAttributes)
    {
    TN_DEBUG1("CTMFormatObserver::HandleBackupOperationEventL");
    OstTrace0( TRACE_NORMAL, CTMFORMATOBSERVER_HANDLEBACKUPOPERATIONEVENTL, "CTMFormatObserver::HandleBackupOperationEventL" );

    if( aBackupOperationAttributes.iOperation == EStart )
        {
        iObserver.FormatNotification(ETrue);
        }
    else // TOperationType::EEnd or TOperationType::EAbort
        {
        iObserver.FormatNotification(EFalse);
        }
    }

