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

#include "thumbagformatobserver.h"
#include "thumbnaillog.h"
 
#include <e32base.h>
#include <f32file.h>


// ======== MEMBER FUNCTIONS ========

CThumbAGFormatObserver::CThumbAGFormatObserver ( CThumbAGProcessor* aProcessor): 
     iProcessor( aProcessor )
    {
    TN_DEBUG1( "CThumbAGFormatObserver::CThumbAGFormatObserver()");
    
    }
    
    
// ---------------------------------------------------------------------------
// Second Phase Constructor
// ---------------------------------------------------------------------------
//
void CThumbAGFormatObserver::ConstructL()
    {
    TN_DEBUG1("CThumbAGFormatObserver::ConstructL in");

    iBackupSession = CBaBackupSessionWrapper::NewL();
    iBackupSession->RegisterBackupOperationObserverL( *this );


    TN_DEBUG1("CThumbAGFormatObserver::ConstructL out");
    }


// ---------------------------------------------------------------------------
// Two-Phased Constructor
// ---------------------------------------------------------------------------
//
CThumbAGFormatObserver* CThumbAGFormatObserver::NewL(CThumbAGProcessor* aProcessor )
    {
    CThumbAGFormatObserver* self = CThumbAGFormatObserver::NewLC( aProcessor );
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// Two-Phased Constructor
// ---------------------------------------------------------------------------
//
CThumbAGFormatObserver* CThumbAGFormatObserver::NewLC( CThumbAGProcessor* aProcessor )
    {
    CThumbAGFormatObserver* self = new( ELeave ) CThumbAGFormatObserver( aProcessor );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }


// ---------------------------------------------------------------------------
// destructor
// ---------------------------------------------------------------------------
//
CThumbAGFormatObserver::~CThumbAGFormatObserver()
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
void CThumbAGFormatObserver::PollStatus()
    {
    
    TN_DEBUG1("CThumbAGFormatObserver::PollStatus()");
    
    TBool formatting = iBackupSession->IsBackupOperationRunning();
    
    if( formatting )
        {     
        iProcessor->SetFormat(ETrue);   
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailFormatObserver::HandleBackupOperationEventL
// Handles a format operation
// ---------------------------------------------------------------------------
//
void CThumbAGFormatObserver::HandleBackupOperationEventL(
                  const TBackupOperationAttributes& aBackupOperationAttributes)
    {
    TN_DEBUG1("CThumbAGFormatObserver::HandleBackupOperationEventL in");

    if( aBackupOperationAttributes.iOperation == EStart )
        {
        iProcessor->SetFormat(ETrue);
        }
    else  // TOperationType::EEnd or TOperationType::EAbort
        {
        iProcessor->SetFormat(EFalse);
        }

    TN_DEBUG1("CThumbAGObserver::HandleBackupOperationEventL out");
    }


