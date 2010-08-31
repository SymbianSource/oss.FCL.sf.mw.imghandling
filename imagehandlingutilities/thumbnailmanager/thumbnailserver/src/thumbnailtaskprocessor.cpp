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
* Description:  Processor object for running queued tasks
 *
*/


#include <e32base.h>
#include <fbs.h>
#include "thumbnailtaskprocessor.h"
#include "thumbnailtask.h"
#include "thumbnailprovider.h"
#include "thumbnailmanageruids.hrh"
#include "thumbnailmanagerconstants.h"
#include "thumbnailgeneratetask.h"
#include "thumbnailpanic.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailtaskprocessorTraces.h"
#endif



// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailTaskProcessor* CThumbnailTaskProcessor::NewL()
    {
    CThumbnailTaskProcessor* self = new( ELeave )CThumbnailTaskProcessor();
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
CThumbnailTaskProcessor::CThumbnailTaskProcessor(): CActive( CActive
    ::EPriorityStandard )
    {
    CActiveScheduler::Add( this );
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailTaskProcessor::ConstructL()
    {
    // define PS property
    TInt ret = RProperty::Delete(KServerIdle, KIdle);
    TN_DEBUG2( "CThumbnailTaskProcessor::CThumbnailTaskProcessor() Delete KServerIdle %d", ret);
    OstTrace1( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_CONSTRUCTL, "CThumbnailTaskProcessor::ConstructL - Delete KServerIdle;ret=%d", ret );
    
    ret=RProperty::Define(KServerIdle, KIdle, RProperty::EInt);
    TN_DEBUG2( "CThumbnailTaskProcessor::ConstructL() Define KServerIdle %d", ret);
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILTASKPROCESSOR_CONSTRUCTL, "CThumbnailTaskProcessor::ConstructL - Define KServerIdle;ret=%d", ret );
    
    // attach to the property    
	ret = iProperty.Attach(KServerIdle, KIdle, EOwnerThread);
    TN_DEBUG2( "CThumbnailTaskProcessor::ConstructL()Attach %d", ret);
    OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILTASKPROCESSOR_CONSTRUCTL, "CThumbnailTaskProcessor::ConstructL - Attach;ret=%d", ret );
    
	// set initial PS value
    ret = iProperty.Set( ETrue );
    TN_DEBUG2( "CThumbnailTaskProcessor::ConstructL() Set IDLE ETrue %d", ret );
    OstTrace1( TRACE_NORMAL, DUP3_CTHUMBNAILTASKPROCESSOR_CONSTRUCTL, "CThumbnailTaskProcessor::ConstructL - Set IDLE ETrue;ret=%d", ret );
    iIdle = ETrue;
    
    iPeriodicTimer = CPeriodic::NewL(CActive::EPriorityIdle);
    
    iTimerActive = EFalse;
     
    iIsDaemonAsProcess = EFalse;
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::~CThumbnailTaskProcessor()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailTaskProcessor::~CThumbnailTaskProcessor()
    {
    Cancel();
    iTasks.ResetAndDestroy();
    iProperty.Close();
    TInt ret = RProperty::Delete(KServerIdle, KIdle);
    TN_DEBUG2( "CThumbnailTaskProcessor::CThumbnailTaskProcessor() Delete KServerIdle %d", ret);
    OstTrace1( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_CTHUMBNAILTASKPROCESSOR, "CThumbnailTaskProcessor::~CThumbnailTaskProcessor - Delete KServerIdle;ret=%d", ret );
    
    if(iPeriodicTimer)
       {
       iPeriodicTimer->Cancel();
       }
    delete iPeriodicTimer;
    iPeriodicTimer = NULL;
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::AddTaskL()
// Adds new task to the task queue.
// ---------------------------------------------------------------------------
//
void CThumbnailTaskProcessor::AddTaskL( CThumbnailTask* aTask )
    {
    TN_DEBUG1( "CThumbnailTaskProcessor::AddTaskL()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_ADDTASKL, "CThumbnailTaskProcessor::AddTaskL" );
    __ASSERT_DEBUG(( aTask ), ThumbnailPanic( EThumbnailNullPointer ));
    const TInt taskCount = iTasks.Count();
    iTasks.AppendL( aTask );
    
    if ( !IsActive() && !taskCount )
        {
        // this is the first task in queue, go active now
        iActiveTask = aTask;
        
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::RemoveTask()
// Removes specific tasks from the queue.
// ---------------------------------------------------------------------------
//
TInt CThumbnailTaskProcessor::RemoveTask( const TThumbnailServerRequestId&
    aRequestId )
    {
    TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTask() - request ID: %d", aRequestId.iRequestId);
    OstTrace1( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_REMOVETASK, "CThumbnailTaskProcessor::RemoveTask;aRequestId.iRequestId=%u", aRequestId.iRequestId );
    
    TBool cancel = EFalse;
    TInt res = KErrNotFound;
    
    for ( TInt i = iTasks.Count(); --i >= 0; )
        {
        CThumbnailTask* task = iTasks[i];
        if ( task->RequestId() == aRequestId )
            {
            if ( task->State() != CThumbnailTask::ERunning )
                {
                // Remove task from queue
                delete task;
                task = NULL;
                iTasks.Remove( i );
                
                TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTask() - removed request ID: %d", aRequestId.iRequestId);
                OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILTASKPROCESSOR_REMOVETASK, "CThumbnailTaskProcessor::RemoveTask - removed request ID:;aRequestId.iRequestId=%u", aRequestId.iRequestId );
                }
            else
                {
                // Task is already running, canceled first
                task->Cancel();
                delete task;
                task = NULL;
                iTasks.Remove( i );
                cancel = ETrue;
                
                TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTask() - canceled & removed request ID: %d", aRequestId.iRequestId);
                OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILTASKPROCESSOR_REMOVETASK, "CThumbnailTaskProcessor::RemoveTask - canceled & removed request ID:;aRequestId.iRequestId=%u", aRequestId.iRequestId );
                }
            
            res = KErrNone;
            
            // don't break, because there could be many tasks related to the same request ID
            }
        }
    
    // no tasks active anymore
    if ( cancel ) 
        {
        Cancel();
        TaskComplete(NULL);
        }
    
    if(!iTasks.Count())
        {
        iTasks.Compress();
        }
    
    TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTask() - remaining task count: %d", iTasks.Count());
    OstTrace1( TRACE_NORMAL, DUP3_CTHUMBNAILTASKPROCESSOR_REMOVETASK, "CThumbnailTaskProcessor::RemoveTask -  - remaining task count;iTasks.Count()=%d", iTasks.Count() );
    return res;
    }

// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::RemoveTasks()
// Removes specific task from the queue.
// ---------------------------------------------------------------------------
//
void CThumbnailTaskProcessor::RemoveTasks( CThumbnailServerSession* aSession )
    {
    TN_DEBUG1( "CThumbnailTaskProcessor::RemoveTasks()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_REMOVETASKS, "CThumbnailTaskProcessor::RemoveTasks" );
    
    TBool cancel = EFalse;
    
    for ( TInt i = iTasks.Count(); --i >= 0; )
        {
        CThumbnailTask* task = iTasks[i];
        
        if ( task->RequestId().iSession == aSession)
            {
            if ( task->State() != CThumbnailTask::ERunning )
                {
                TThumbnailRequestId id = task->RequestId().iRequestId;
                
                // Remove task from queue
                delete task;
                task = NULL;
                iTasks.Remove( i );
                
                TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTasks() - removed request ID: %d", id);
                OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILTASKPROCESSOR_REMOVETASKS, "CThumbnailTaskProcessor::RemoveTasks - removed request ID;id=%d", id );
                }
            else
                {
                TN_DEBUG1( "CThumbnailTaskProcessor::RemoveTasks() - task still running");
                OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILTASKPROCESSOR_REMOVETASKS, "CThumbnailTaskProcessor::RemoveTasks - task still running" );
                
                TThumbnailRequestId id = task->RequestId().iRequestId;
                
                // Task is already running, canceled first
                task->Cancel();
                delete task;
                task = NULL;
                iTasks.Remove( i );
                cancel = ETrue;
                
                TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTasks() - canceled & removed request ID: %d", id);
                OstTrace1( TRACE_NORMAL, DUP3_CTHUMBNAILTASKPROCESSOR_REMOVETASKS, "CThumbnailTaskProcessor::RemoveTasks - canceled & removed request ID;id=%d", id );
                }
            }
        }
    
    // no tasks active anymore
    if ( cancel ) 
        {
        Cancel();
        TaskComplete(NULL);
        }
    
    TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTasks() - remaining task count: %d", iTasks.Count());
    OstTrace1( TRACE_NORMAL, DUP4_CTHUMBNAILTASKPROCESSOR_REMOVETASKS, "CThumbnailTaskProcessor::RemoveTasks - remaining task count;iTasks.Count()=%d", iTasks.Count() );
    }

// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::RemoveAllTasks()
// Removes specific task from the queue.
// ---------------------------------------------------------------------------
//
void CThumbnailTaskProcessor::RemoveAllTasks()
    {
    TN_DEBUG1( "CThumbnailTaskProcessor::RemoveAllTasks()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_REMOVEALLTASKS, "CThumbnailTaskProcessor::RemoveAllTasks" );
    
    TBool cancel = EFalse;
    
    for ( TInt i = iTasks.Count(); --i >= 0; )
        {
        CThumbnailTask* task = iTasks[i];
        if ( task->State() != CThumbnailTask::ERunning )
            {
            TThumbnailRequestId id = task->RequestId().iRequestId;
            
            // Remove task from queue
            delete task;
            task = NULL;
            iTasks.Remove( i );
            
            TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTasks() - removed request ID: %d", id);
            OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILTASKPROCESSOR_REMOVEALLTASKS, "CThumbnailTaskProcessor::RemoveAllTasks -  removed request ID;id=%d", id );
            }
        else
            {
            TN_DEBUG1( "CThumbnailTaskProcessor::RemoveTasks() - task still running");
            OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILTASKPROCESSOR_REMOVEALLTASKS, "CThumbnailTaskProcessor::RemoveAllTasks - task still running" );
            
            TThumbnailRequestId id = task->RequestId().iRequestId;
            
            // Task is already running, canceled first
            task->Cancel();
            delete task;
            task = NULL;
            iTasks.Remove( i );
            cancel = ETrue;
            
            TN_DEBUG2( "CThumbnailTaskProcessor::RemoveTasks() - canceled & removed request ID: %d", id);
            OstTrace1( TRACE_NORMAL, DUP3_CTHUMBNAILTASKPROCESSOR_REMOVEALLTASKS, "CThumbnailTaskProcessor::RemoveAllTasks - canceled & removed request ID;id=%d", id );
            }
        }
    
    // no tasks active anymore
    if ( cancel ) 
        {
        Cancel();
        TaskComplete(NULL);
        }
    
    TN_DEBUG1( "CThumbnailTaskProcessor::RemoveAllTasks() - Task removed because of starting format" );
    OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILTASKPROCESSOR_REMOVEALLTASKS, "CThumbnailTaskProcessor::RemoveAllTasks - Task removed because of starting format" );
    }

// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::ChangeTaskPriority()
// Changes priority of a task.
// ---------------------------------------------------------------------------
//
TInt CThumbnailTaskProcessor::ChangeTaskPriority( const
    TThumbnailServerRequestId& aRequestId, const TInt aNewPriority )
    {
    TInt err = KErrNotFound;
    const TInt count = iTasks.Count();
    for ( TInt i( 0 ); i < count; i++ )
        {
        if ( iTasks[i]->RequestId() == aRequestId )
            {
            iTasks[i]->ChangeTaskPriority( aNewPriority );
            err = KErrNone;
            break;
            }
        }
    return err;
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailTaskProcessor::RunL()
    {
    iActiveTask = NULL;
    TInt priority( KMinTInt );
    TInt taskPriority;
    TBool processingDaemonTasksOnly(ETrue);
    
#ifdef _DEBUG
    TN_DEBUG2( "CThumbnailTaskProcessor::TASKPROCESSOR-COUNTER---------- in, Tasks = %d", iTasks.Count() );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_RUNL, "CThumbnailTaskProcessor::TASKPROCESSOR-COUNTER---------- in, Tasks;iTasks.Count()=%d", iTasks.Count() );
#endif
    
    // remove completed tasks and run priorised task
    for ( TInt i = iTasks.Count(); --i >= 0; )
        {
        CThumbnailTask* task = iTasks[i];
        if ( task->State() == CThumbnailTask::EComplete )
            {
            // Delete completed task
            delete task;
            task = NULL;
            iTasks.Remove( i );
            }
        else
            {
            if( task->State() == CThumbnailTask::EIdle )
                {
                taskPriority = task->Priority();
                if ( taskPriority > priority )
                    {
                    priority = taskPriority;
                    iActiveTask = task;
                    }
                }
            
            if ( processingDaemonTasksOnly && task->ClientThreadAlive() )
                {
                if(task->GetMessageData().Identity() != KDaemonUid )
                    {
                    TN_DEBUG1( "CThumbnailTaskProcessor::RunL() processingDaemonTasksOnly = EFalse" );
                    OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILTASKPROCESSOR_RUNL, "CThumbnailTaskProcessor::RunL processingDaemonTasksOnly = EFalse" );
                    processingDaemonTasksOnly = EFalse; 
                    }
                }
            }
        }

    if ( iActiveTask )
        {
        iActiveTask->StartL();
        }
    
#ifdef _DEBUG
    TN_DEBUG2( "CThumbnailTaskProcessor::TASKPROCESSOR-COUNTER---------- out, Tasks = %d", iTasks.Count() );
    OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILTASKPROCESSOR_RUNL, "CThumbnailTaskProcessor::TASKPROCESSOR-COUNTER---------- out, Tasks;iTasks.Count()=%d", iTasks.Count() );
#endif

	//update PS value for Daemon
    if( iTasks.Count() > 0 && iIdle && !processingDaemonTasksOnly)
        {
        //set not idle
        if(iTimerActive)
           {
           iPeriodicTimer->Cancel();
           iTimerActive = EFalse;
           }
        TInt ret = iProperty.Set( EFalse );
        TN_DEBUG2( "CThumbnailTaskProcessor::RunL() iProperty Set EFalse %d", ret );
        OstTrace1( TRACE_NORMAL, DUP3_CTHUMBNAILTASKPROCESSOR_RUNL, "CThumbnailTaskProcessor::RunL - iProperty Set EFalse;ret=%d", ret );
        iIdle = EFalse;
        }
    else
        {
        if(!iIdle)
            {
            //set idle    
            if(iIsDaemonAsProcess)
               {
               if(iTimerActive)
                 {
                 iPeriodicTimer->Cancel();
                 iTimerActive = EFalse;
                 } 
               TN_DEBUG1( "CThumbnailTaskProcessor::RunL() - daemon is the process");
               OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILTASKPROCESSOR_RUNL, "CThumbnailTaskProcessor::RunL - daemon is the process" );
               TInt ret = iProperty.Set( ETrue );
               TN_DEBUG2( "CThumbnailTaskProcessor::RunL() iProperty Set ETrue %d", ret );
               iIdle = ETrue;
               }
            else
               {
               TN_DEBUG1( "CThumbnailTaskProcessor::RunL() - daemon is not the process - start timer");
               OstTrace0( TRACE_NORMAL, DUP5_CTHUMBNAILTASKPROCESSOR_RUNL, "CThumbnailTaskProcessor::RunL - daemon is not the process - start timer" );
               if(iTimerActive)
                  {
                  iPeriodicTimer->Cancel();
                  iTimerActive = EFalse;
                  }  
               iPeriodicTimer->Start( KHarvestingCompleteTimeout, KHarvestingCompleteTimeout, 
                                   TCallBack(PeriodicTimerCallBack, this));
               iTimerActive = ETrue;
               }
            }
        }
    }

// ---------------------------------------------------------------------------
// Handle any error from RunL
// ---------------------------------------------------------------------------
//
TInt CThumbnailTaskProcessor::RunError( TInt aError )
    {
    TN_DEBUG1( "CThumbnailTaskProcessor::RunError()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_RUNERROR, "CThumbnailTaskProcessor::RunError" );
    if ( iActiveTask )
        {
        // This will complete the task and continue processing from the
        // next task
        iActiveTask->StartError( aError );
        }
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailTaskProcessor::DoCancel()
    {
    // No implementation required
    }


// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::TaskComplete()
// Marks task to completed
// ---------------------------------------------------------------------------
//
void CThumbnailTaskProcessor::TaskComplete( CThumbnailTask*  /*aTask*/ )
    {
    TN_DEBUG1( "CThumbnailTaskProcessor::TaskComplete()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_TASKCOMPLETE, "CThumbnailTaskProcessor::TaskComplete" );
 
    if ( iTasks.Count() && !IsActive())
        {
        // More tasks yet to be run
        SetActive();
        TRequestStatus* statusPtr = &iStatus;
        User::RequestComplete( statusPtr, KErrNone );
        }
    else if( iTasks.Count() == 0 && !iIdle)
        {
        if(iIsDaemonAsProcess)
            {
            if(iTimerActive)
              {
              iPeriodicTimer->Cancel();
              iTimerActive = EFalse;
              } 
            TN_DEBUG1( "CThumbnailTaskProcessor::TaskComplete() - daemon is the process");
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILTASKPROCESSOR_TASKCOMPLETE, "CThumbnailTaskProcessor::TaskComplete - daemon is the process" );
            TInt ret = iProperty.Set( ETrue );
            TN_DEBUG2( "CThumbnailTaskProcessor::TaskComplete() iProperty Set ETrue %d", ret );
            OstTrace1( TRACE_NORMAL, DUP2_CTHUMBNAILTASKPROCESSOR_TASKCOMPLETE, "CThumbnailTaskProcessor::TaskComplete - iProperty Set ETrue;ret=%d", ret );
            iIdle = ETrue;
            }
        else
            {
            if(iTimerActive)
              {
              iPeriodicTimer->Cancel();
              iTimerActive = EFalse;
              }  
            iPeriodicTimer->Start( KPSKeyTimeout, KPSKeyTimeout, 
                             TCallBack(PeriodicTimerCallBack, this));
            iTimerActive = ETrue;
            }
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailTaskProcessor::PeriodicTimerCallBack()
// ---------------------------------------------------------------------------
//
TInt CThumbnailTaskProcessor::PeriodicTimerCallBack(TAny* aAny)
    {
    TN_DEBUG1( "CThumbnailTaskProcessor::PeriodicTimerCallBack()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_PERIODICTIMERCALLBACK, "CThumbnailTaskProcessor::PeriodicTimerCallBack" );
    
    CThumbnailTaskProcessor* self = static_cast<CThumbnailTaskProcessor*>( aAny );
    
    self->iTimerActive = EFalse;
    self->iPeriodicTimer->Cancel();
    TInt ret = self->iProperty.Set( ETrue );
    TN_DEBUG2( "CThumbnailTaskProcessor::PeriodicTimerCallBack() iProperty Set ETrue %d", ret );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILTASKPROCESSOR_PERIODICTIMERCALLBACK, "CThumbnailTaskProcessor::PeriodicTimerCallBack - iProperty Set ETrue;ret=%d", ret );
    self->iIdle = ETrue;

    return KErrNone; // Return value ignored by CPeriodic
    }

void CThumbnailTaskProcessor::SetDaemonAsProcess(TBool aValue)
    {
    TN_DEBUG2( "CThumbnailTaskProcessor::SetDaemonAsProcess() Daemon set %d", aValue );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILTASKPROCESSOR_SETDAEMONASPROCESS, "CThumbnailTaskProcessor::SetDaemonAsProcess - Daemon set;aValue=%d", aValue );
    iIsDaemonAsProcess = aValue;
    }


// End of file
