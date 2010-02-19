/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Helper class to get IMEI number. 
*
*/

#ifndef TMGETIMEI_H
#define TMGETIMEI_H

#include <e32base.h>
#include <etel3rdparty.h>

#include "thumbnailmanagerconstants.h"

class CTMGetImei: public CActive
    {
    private:
        CTelephony *iTelephony;
        CTelephony::TPhoneIdV1 iV1; 
        TBuf<KImeiBufferSize> iImei;
        CActiveSchedulerWait iAsw;
    public:
        virtual ~CTMGetImei();
        static CTMGetImei* NewL();
        static CTMGetImei* NewLC();
        TBuf<KImeiBufferSize> GetIMEI();

        void DoCancel();

        void RunL();

    private:
        CTMGetImei(): CActive(EPriorityStandard), iTelephony(NULL)
        {}
        void ConstructL();
    };

#endif //TNMGETIMEI_H

