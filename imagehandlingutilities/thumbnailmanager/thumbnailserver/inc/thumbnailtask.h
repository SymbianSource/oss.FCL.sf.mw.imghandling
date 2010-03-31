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


#ifndef THUMBNAILTASK_H
#define THUMBNAILTASK_H

#include "thumbnailmanager.h"    // TThumbnailRequestId
#include "thumbnailmanagerconstants.h" // TThumbnailServerRequestId


class CThumbnailTaskProcessor;
class CThumbnailServerSession;

/**
 *  Base class for thumbnail server tasks.
 *
 *  @since S60 v5.0
 */
class CThumbnailTask: public CActive
    {
public:
    /**
     * Task state.
     * @since S60 v5.0
     */
    enum TTaskState
        {
        /**
         * This task is waiting to be run.
         */
        EIdle, 

        /**
         * This task is currently running.
         */
        ERunning, 

        /**
         * This task has completed and can be destroyed.
         */
        EComplete
    };

public:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aProcessor Processor object for running tasks.
     * @param aPriority Priority of the task.
     * @return New CThumbnailTask instance.
     */
    CThumbnailTask( CThumbnailTaskProcessor& aProcessor, TInt aPriority );

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailTask();

public:

    /**
     * Returns priority of task.
     *
     * @since S60 v5.0
     * @return Priority of the task.
     */
    TInt Priority()const;

    /**
     * Returns state of task.
     *
     * @since S60 v5.0
     * @return State.
     */
    TTaskState State()const;

    /**
     * SetMessageData
     *
     * @since S60 v5.0
     * @param aRequestId Request ID.
     * @param aMessage Message.
     * @param aClientThread Client thread.
     */
    virtual void SetMessageData( const TThumbnailServerRequestId& aRequestId,
        const RMessage2& aMessage, const RThread& aClientThread );
    
    /**
     * SetMessageData if message is not needed to complete
     *
     * @since S60 v5.0
     * @param aRequestId Request ID.
     */
     virtual void SetMessageData( const TThumbnailServerRequestId& aRequestId );

    /**
     * Resets message data.
     *
     * @since S60 v5.0
     */
    virtual void ResetMessageData();

    /**
     * Cancels message.
     *
     * @since S60 v5.0
     */
    virtual void CancelMessage();

    /**
     * Returns ID of specific task.
     *
     * @since S60 v5.0
     * @return Request ID.
     */
    virtual TThumbnailServerRequestId RequestId()const;

    /**
     * Changes priority of the task.
     *
     * @since S60 v5.0
     * @param aNewPriority New priority.
     */
    virtual void ChangeTaskPriority( TInt aNewPriority );

    /**
     * StartL
     *
     * @since S60 v5.0
     */
    virtual void StartL();

    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    virtual void DoCancel();    
    
    /**
     * StartError
     *
     * @since S60 v5.0
     * @param aError Error code.
     */
    virtual void StartError( TInt aError );

    /**
     * Complete
     *
     * @since S60 v5.0
     * @param aReason Status code.
     */
    virtual void Complete( TInt aReason );
    
    /**
     * Continue without deleting task
     *
     * @since S60 v5.0
     */
    virtual void Continue();
    
    /**
     * Checks if client thread is still alive and RMessage2 handle valid.
     *
     * @since S60 v5.0
     * @param aGetThread Need to get thread first.
     */
    TBool ClientThreadAlive(const TBool aGetThread = EFalse);    
    
    inline RMessage2& GetMessageData()
        {
        return iMessage;
        }

protected:
    // data

    /**
     * Processor.
     */
    CThumbnailTaskProcessor& iProcessor;

    /**
     * Priority of the task.
     */
    TInt iPriority;

    /**
     * State of the task.
     */
    TTaskState iState;

    /**
     * Id of the request.
     */
    TThumbnailServerRequestId iRequestId;

    /**
     * Message.
     */
    RMessage2 iMessage;
    
    // client thread from RMessage2
    RThread iClientThread;
};

#endif // THUMBNAILTASK_H
