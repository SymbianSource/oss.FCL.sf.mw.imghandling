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


#ifndef THUMBNAILTASKPROCESSOR_H
#define THUMBNAILTASKPROCESSOR_H

#include "thumbnailmanager.h"    // TThumbnailFlags
#include "thumbnailmanagerconstants.h" // TThumbnailServerRequestId
#include "thumbnailprovider.h"
#include <e32property.h>


class CThumbnailTask;

/**
 *  Processor object for running queued tasks.
 *
 *  @since S60 v5.0
 */
class CThumbnailTaskProcessor: public CActive
    {
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return Instance of CThumbnailTaskProcessor.
     */
    static CThumbnailTaskProcessor* NewL();

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailTaskProcessor();

    /**
     * Adds new task to the task queue.
     *
     * @since S60 v5.0
     * @param aTask Task to be added to the queue.
     */
    void AddTaskL( CThumbnailTask* aTask );

    /**
     * Removes specific task from the queue.
     *
     * @since S60 v5.0
     * @param aRequestId ID of the task to be removed.
     */
    TInt RemoveTask( const TThumbnailServerRequestId& aRequestId );
    
    /**
     * Removes specific task from the queue.
     *
     * @since S60 v5.0
     * @param aSession Identifier for task to be removed.
     */
    void RemoveTasks( CThumbnailServerSession* aSession );
    
    /**
     * Removes all tasks from the queue.
     *
     * @since S60 v5.0
     */
    
    void RemoveAllTasks();

    /**
     * Changes priority of a task.
     *
     * @since S60 v5.0
     * @param aRequestId ID of the task.
     * @param aNewPriority New priority to be assigned to the task.
     */
    TInt ChangeTaskPriority( const TThumbnailServerRequestId& aRequestId, const
        TInt aNewPriority );

    /**
     * Marks task to completed.
     *
     * @since S60 v5.0
     * @param aTask Completed task.
     */
    void TaskComplete( CThumbnailTask* aTask );
    
    void SetDaemonAsProcess(TBool aValue);

protected:
    // from base class CActive

    /**
     * Handles an active object's request completion event.
     *
     * @since S60 v5.0
     */
    void RunL();

    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    void DoCancel();

    /**
     * Handles a leave occurring in the request completion event handler
     * RunL().
     *
     * @since S60 v5.0
     * @return Error code.
     */
    TInt RunError( TInt aError );

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return Instance of CThumbnailTaskProcessor.
     */
    CThumbnailTaskProcessor();

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();
    
    /**
     * Callback for harvesting complete timer
     *
     * @since S60 v5.0
     */
    static TInt PeriodicTimerCallBack(TAny* aAny);

private:
    // data

    /**
     * Task queue. Task objects are own by this class.
     * Own.
     */
    RPointerArray < CThumbnailTask > iTasks;

    /**
     * Current active task. Not own.
     */
    CThumbnailTask* iActiveTask;
    
    /**
     * PS used to notify Daemon when server is idle 
     */
    RProperty iProperty;
    /**
     * Holds current idle value
     */
    TBool iIdle;
    
    CPeriodic* iPeriodicTimer;
    
    TBool iTimerActive;
    
    TBool iIsDaemonAsProcess;
};

#endif // THUMBNAILTASKPROCESSOR_H
