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
* Description:  Class to monitor when volumes are unmounted
 *
*/


#ifndef THUMBNAILMEMORYCARDOBSERVER_H
#define THUMBNAILMEMORYCARDOBSERVER_H

#include "f32file.h"
#include "thumbnailserver.h" 

/**
 *  Class to monitor when memory card status changes
 *
 *  @since S60 v5.0
 */
class CThumbnailMemoryCardObserver: public CActive
    {
public:

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @return New CThumbnailMemoryCardObserver object.
     */
    static CThumbnailMemoryCardObserver* NewL( CThumbnailServer* aServer, RFs& aFs );
    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailMemoryCardObserver();

private:
    void RunL();
    void DoCancel();

    void StartNotify();

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New CThumbnailMemoryCardObserver object.
     */
    CThumbnailMemoryCardObserver( CThumbnailServer* aServer, RFs& aFs );    

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:
    // Data   
    CThumbnailServer* iServer;    
    RFs& iFs; 
};

#endif // THUMBNAILMEMORYCARDOBSERVER_H
