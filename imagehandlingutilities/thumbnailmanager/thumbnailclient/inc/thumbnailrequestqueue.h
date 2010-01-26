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
* Description:  Processor object for running thumbnail requests
*
*/


#ifndef THUMBNAILREQUESTQUEUE_H
#define THUMBNAILREQUESTQUEUE_H

#include <e32base.h>

#include "thumbnailmanager.h"

class CThumbnailRequestActive;

/**
 *  Processor object for running queued tasks.
 *
 *  @since S60 v5.0
 */
class CThumbnailRequestQueue: public CBase
    {
public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return Instance of CThumbnailRequestQueue.
     */
    static CThumbnailRequestQueue* NewL();

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailRequestQueue();

    /**
     * Activates next request if possible.
     *
     * @since S60 v5.0
     */
    void Process();
    
    /**
     * Adds new request to the queue.
     *
     * @since S60 v5.0
     * @param aRequest Request to be added to the queue.
     */
    void AddRequestL( CThumbnailRequestActive* aRequest );

    /**
     * Cancels request in the queue.
     *
     * @since S60 v5.0
     * @param aId Request to be canceled.
     * @return Error code.
     */
    TInt CancelRequest( const TThumbnailRequestId aId );
    
    /**
     * Change priority of a request.
     *
     * @since S60 v5.0
     * @param aId Request to be canceled.
     * @param aNewPriority New priority.
     * @return Error code.
     */
    TInt ChangePriority( const TThumbnailRequestId aId, const TInt aNewPriority );
    
    /**
     * Marks request complete.
     *
     * @since S60 v5.0
     */
    void RequestComplete(CThumbnailRequestActive* aRequestAO = NULL);
    
    /**
     * Removes completed requests.
     *
     * @since S60 v5.0
     */
	void RemoveCompleted(CThumbnailRequestActive* aRequestAO);

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return Instance of CThumbnailRequestQueue.
     */
    CThumbnailRequestQueue();

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:

    /**
     * Array of active objects for each pending thumbnail request.
     */
    RPointerArray <CThumbnailRequestActive> iRequests;
    
    // number of currently active requests
    TInt iActiveRequests;

};

#endif // THUMBNAILREQUESTQUEUE_H
