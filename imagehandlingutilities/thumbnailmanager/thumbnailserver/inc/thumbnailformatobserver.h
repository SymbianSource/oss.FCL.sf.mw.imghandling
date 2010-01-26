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
* Description:  Monitors File system for Format events
*
*/


#ifndef CTHUMBNAILFORMATOBSERVER_H
#define CTHUMBNAILFORMATOBSERVER_H

#include <babackup.h>
#include "f32file.h"
#include "thumbnailserver.h" 

/**
 *  File System monitor class to monitor for format events
 *
 *  @lib thumbnailsserver.exe
 *  @since S60 3.0
 */
NONSHARABLE_CLASS( CThumbnailFormatObserver ) : public CBase,
                                             public MBackupOperationObserver
    {

public:

    /**
    * Two-phase constructor
    * @param aObserver observer to the monitor
    */
    static CThumbnailFormatObserver* NewL( CThumbnailServer* aServer );

    /**
    * Two-phase constructor
    * @param aObserver observer to the monitor
    */
    static CThumbnailFormatObserver* NewLC( CThumbnailServer* aServer );

    /**
    * Destructor
    */
    virtual ~CThumbnailFormatObserver();

public: // New functions

    /**
    * Polls for the current monitor status
    * If an event is happening, it will callback the observer of the event
    */
    void PollStatus();

protected: // Base Class

    /*
    * From MBackupOperationObserver
    * Notifies this class of the backup event.
    * MMC App will signal a start / end.
    */
    void HandleBackupOperationEventL(const TBackupOperationAttributes& aBackupOperationAttributes);

private:

    /**
    *  C++ constructor
    *  aObserver observer to this event
    */
    CThumbnailFormatObserver ( CThumbnailServer* aServer );

    /*
    * Second phased constructor
    */
    void ConstructL();

private: // data

    CThumbnailServer* iServer;    
    
    CBaBackupSessionWrapper* iBackupSession;

    
    };

#endif // CTHUMBNAILFORMATOBSERVER_H
