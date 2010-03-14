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


#ifndef CTMFORMATOBSERVER_H
#define CTMFORMATOBSERVER_H

#include <babackup.h>
#include "f32file.h"


/**
 * Observer interface
 */
class MTMFormatObserver
    {
public:

    virtual void FormatNotification( TBool aFormat ) = 0;
    };

/**
 *  File System monitor class to monitor for format events
 *
 *  @lib thumbnailsserver.exe
 *  @since S60 3.0
 */
class CTMFormatObserver: public CBase,
                         public MBackupOperationObserver
    {

public:

    /**
    * Two-phase constructor
    * @param aObserver observer to the monitor
    */
    static CTMFormatObserver* NewL( MTMFormatObserver& aObserver );

    /**
    * Two-phase constructor
    * @param aObserver observer to the monitor
    */
    static CTMFormatObserver* NewLC( MTMFormatObserver& aObserver );

    /**
    * Destructor
    */
    virtual ~CTMFormatObserver();

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
    CTMFormatObserver ( MTMFormatObserver& aObserver );

    /*
    * Second phased constructor
    */
    void ConstructL();

private: // data

    // not own
    MTMFormatObserver& iObserver; 
    
    CBaBackupSessionWrapper* iBackupSession;
    
    };

#endif // CTMFORMATOBSERVER_H
