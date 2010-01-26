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


#ifndef THUMBNAILDISKUNMOUNTOBSERVER_H
#define THUMBNAILDISKUNMOUNTOBSERVER_H

#include "f32file.h"

class CThumbnailServer;

/**
 *  Class to monitor when volumes are unmounted
 *
 *  @since S60 v5.0
 */
class CThumbnailDiskUnmountObserver: public CActive
    {
public:

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @return New CThumbnailDiskUnmountObserver object.
     */
    static CThumbnailDiskUnmountObserver* NewL( RFs& aFs, TInt aDrive, CThumbnailServer* aServer );
    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailDiskUnmountObserver();
    
    void StartNotify();

private:
    void RunL();
    void DoCancel();


    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New CThumbnailDiskUnmountObserver object.
     */
    CThumbnailDiskUnmountObserver( RFs& aFs, TInt aDrive, CThumbnailServer* aServer );

    /**
     * Symbian 2nd phase constructor can leave.
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:
    // Data
    RFs& iFs;
    TInt iDrive;
    CThumbnailServer* iServer;
};

#endif // THUMBNAILDISKUNMOUNTOBSERVER_H
