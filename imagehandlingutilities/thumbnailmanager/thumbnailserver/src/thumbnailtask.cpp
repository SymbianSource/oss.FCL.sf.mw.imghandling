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
* Description:  Base class for thumbnail server tasks
 *
*/


#include <e32base.h>
#include <fbs.h>
#include "thumbnailtask.h"
#include "thumbnailtaskprocessor.h"
#include "thumbnailprovider.h"
#include "thumbnaillog.h"
#include "thumbnailpanic.h"
#include "thumbnailserversession.h"  // ConvertSqlErrToE32Err()
#include "thumbnailmanagerconstants.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailtaskTraces.h"
#endif


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailTask::CThumbnailTask()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailTask::CThumbnailTask( CThumbnailTaskProcessor& aProcessor, TInt
    aPriority ): CActive( EPriorityStandard ), iProcessor( aProcessor ),
    iPriority( aPriority ), iState( EIdle )
    {
    TN_DEBUG2( "CThumbnailTask(0x%08x)::CThumbnailTask()", this);
    CActiveScheduler::Add( this );
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::~CThumbnailTask()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailTask::~CThumbnailTask()
    {
    Cancel();
    CancelMessage();
    
    iClientThread.Close();
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::Priority()
// Returns priority of task.
// ---------------------------------------------------------------------------
//
TInt CThumbnailTask::Priority()const
    {
    return iPriority;
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::State()
// Returns state of task.
// ---------------------------------------------------------------------------
//
CThumbnailTask::TTaskState CThumbnailTask::State()const
    {
    return iState;
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::StartL()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::StartL()
    {
    TN_DEBUG3( "CThumbnailTask(0x%08x)::StartL() iState == %d ", this, iState );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILTASK_STARTL, "CThumbnailTask::StartL;this=%o", this );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILTASK_STARTL, "CThumbnailTask::StartL;iState=%u", iState );
    
    __ASSERT_DEBUG(( iState != ERunning ), ThumbnailPanic( EThumbnailAlreadyRunning ));
    iState = ERunning;
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::DoCancel()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::Complete()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::Complete( TInt aReason )
    {
    TN_DEBUG4( "CThumbnailTask(0x%08x)::Complete(aReason=%d) iState was %d",
        this, aReason, iState );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILTASK_COMPLETE, "CThumbnailTask::Complete;this=%o", this );
    OstTraceExt2( TRACE_NORMAL, DUP1_CTHUMBNAILTASK_COMPLETE, "CThumbnailTask::Complete;aReason=%d;iState=%u", aReason, iState );
    
    if ( iState != EComplete )
        {
        iState = EComplete;
        
        if ( ClientThreadAlive() )
            {
            if( iMessage.Identity() == KDaemonUid ) 
                {
                iProcessor.SetDaemonAsProcess(ETrue);
                }
            else
                {
                iProcessor.SetDaemonAsProcess(EFalse);
                }
            
            iMessage.Complete( CThumbnailServerSession::ConvertSqlErrToE32Err( aReason ));
            }
        
        ResetMessageData();
        
        iProcessor.TaskComplete( this );
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailTask::Continue()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::Continue()
    {
    if ( iState != EComplete )
        { 
        iState = EIdle;
        }
    
    iProcessor.TaskComplete( this );
    }

// ---------------------------------------------------------------------------
// CThumbnailTask::StartError()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::StartError( TInt aError )
    {
    // This is called if StartL() left. Complete this task with an error and
    // continue processing.
    TN_DEBUG3( "CThumbnailTask(0x%08x)::StartError(aError=%d)", this, aError );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILTASK_STARTERROR, "CThumbnailTask::StartError;this=%o", this );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILTASK_STARTERROR, "CThumbnailTask::StartError;aError=%d", aError );
    
    Complete( aError );
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::ChangeTaskPriority()
// Changes priority of the task.
// ---------------------------------------------------------------------------
//
void CThumbnailTask::ChangeTaskPriority( TInt aNewPriority )
    {
    iPriority = aNewPriority;
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::SetMessageData()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::SetMessageData( const TThumbnailServerRequestId&
    aRequestId, const RMessage2& aMessage, const RThread& aClientThread )
    {
    iMessage = aMessage;
    iRequestId = aRequestId;
    
    if ( iMessage.Handle())
        {
        // copy client thread handle
        iClientThread.Duplicate(aClientThread);
        }
    else
        {
        TN_DEBUG2( "CThumbnailTask(0x%08x)::ClientThreadAlive() - message null", this);
        OstTrace1( TRACE_NORMAL, CTHUMBNAILTASK_SETMESSAGEDATA, "CThumbnailTask::SetMessageData - message null;this=%o", this );
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailTask::SetMessageData()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::SetMessageData( const TThumbnailServerRequestId&
    aRequestId )
    {
    iMessage = RMessage2();
    iRequestId = aRequestId;
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::ResetMessageData()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::ResetMessageData()
    {
    iMessage = RMessage2();
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::ResetMessageData()
// Returns id of specific task.
// ---------------------------------------------------------------------------
//
TThumbnailServerRequestId CThumbnailTask::RequestId()const
    {
    return iRequestId;
    }


// ---------------------------------------------------------------------------
// CThumbnailTask::CancelMessage()
// ---------------------------------------------------------------------------
//
void CThumbnailTask::CancelMessage()
    {
    if ( ClientThreadAlive() )
        {
        iMessage.Complete( KErrCancel );
        }
    
    ResetMessageData();
    }

// ---------------------------------------------------------------------------
// CThumbnailTask::ClientThreadAlive()
// Checks if client thread is still alive and RMessage2 handle valid.
// ---------------------------------------------------------------------------
//
TBool CThumbnailTask::ClientThreadAlive(const TBool aGetThread)
    {
    TN_DEBUG1( "CThumbnailTask::ClientThreadAlive()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILTASK_CLIENTTHREADALIVE, "CThumbnailTask::ClientThreadAlive" );
    
    if ( iMessage.Handle())
        {
        if (aGetThread)
            {
            // get client thread
            TInt err = iMessage.Client( iClientThread );
            if (err != KErrNone)
                {
                TN_DEBUG2( "CThumbnailTask(0x%08x)::ClientThreadAlive() - client thread not found", this);
                OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILTASK_CLIENTTHREADALIVE, "CThumbnailTask::ClientThreadAlive - client thread not found;this=%o", this );
            
                ResetMessageData();
                
                return EFalse;
                }
            }
    
        // check if client thread alive
        TExitType exitType = iClientThread.ExitType();
        if( exitType != EExitPending )
            {
            TN_DEBUG2( "CThumbnailTask(0x%08x)::ClientThreadAlive() - client thread died", this);
            OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILTASK_CLIENTTHREADALIVE, "CThumbnailTask::ClientThreadAlive -  client thread died;this=%o", this );
        
            ResetMessageData();
            
            return EFalse;
            }
        else
            {
            // all OK
            return ETrue;
            }
        }
    else
        {
        return EFalse;
        }
    }

// End of file
