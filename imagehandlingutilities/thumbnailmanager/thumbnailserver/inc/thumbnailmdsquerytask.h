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
* Description:  Task for making MDS querys
*
*/


#ifndef THUMBNAILMDSQUERYTASK_H
#define THUMBNAILMDSQUERYTASK_H

#include <mdesession.h>
#include <mdccommon.h>
#include <mdeobjectquery.h>

#include "thumbnailtask.h"

class CThumbnailServer;

/**
 *  Task for making MDS querys
 *
 *  @since S60 v5.0
 */
class CThumbnailMDSQueryTask: public CThumbnailTask,
                              public MMdEQueryObserver
    {
public:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aProcessor Processor object for running tasks.
     * @param aPriority Priority of the request.
     * @return New CThumbnailMDSQueryTask object.
     */
    CThumbnailMDSQueryTask( CThumbnailTaskProcessor& aProcessor, TInt aPriority,
                            CMdESession* aMdESession, CThumbnailServer& aServer);

    /**
     * Destructor.
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailMDSQueryTask();    
    
    /**
     * StartL
     *
     * @since S60 v5.0
     */
    void StartL();

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
     * Query path by id
     *
     * @since S60 v5.0
     * @param aId ID
     */
    void QueryPathByIdL(TThumbnailId aId, TBool aDelete);
    
    /**
     * Sets are values updated to db automatically
     *
     * @since S60 v5.0
     * @param aComplete message, default is ETrue
     */
    void SetUpdateToDb(const TBool& aUpdateToDb );

public:
    
    // From MMdEQueryObserver
    void HandleQueryNewResults( CMdEQuery& aQuery, TInt aFirstNewItemIndex, TInt aNewItemCount );
    void HandleQueryCompleted( CMdEQuery& aQuery, TInt aError );    
    
protected:
    
    void ReturnPath(const TDesC& aUri);
    
private:
   
    // not own
    CMdESession* iMdESession;
    
    // own
    CMdEObjectQuery* iQuery;
    
    TThumbnailRequestParamsPckgBuf iRequestParams;
    
    /**
     * Server.
     */
    CThumbnailServer& iServer;
	
     /**
     * Query type used when filling missing information from MDS
     */
     TMDSQueryType iQueryType;
     
     /**
     * Does task update db
     */
     TBool iUpdateToDb;
     
     TFileName iUri;
     
     TBool iDelete;
};

#endif // THUMBNAILMDSQUERYTASK_H
