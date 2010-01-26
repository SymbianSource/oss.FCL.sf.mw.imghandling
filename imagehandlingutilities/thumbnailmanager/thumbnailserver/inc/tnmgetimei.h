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

#ifndef _TNMGETIMEI_H_
#define _TNMGETIMEI_H_

#include <e32base.h>
#include <etel3rdparty.h>

#include "thumbnailmanagerconstants.h"

class CTnmgetimei: public CActive
    {
    private:
        CTelephony *iTelephony;
        CTelephony::TPhoneIdV1 iV1; 
        TBuf<KImeiBufferSize> iImei;
        CActiveSchedulerWait iAsw;
    public:
        virtual ~CTnmgetimei();
        static CTnmgetimei* NewL();
        static CTnmgetimei* NewLC();
        TBuf<KImeiBufferSize> GetIMEI();

        void DoCancel();

        void RunL();

    private:
        CTnmgetimei(): CActive(EPriorityStandard), iTelephony(NULL)
        {}
        void ConstructL();

    };

#endif //_TNMGETIMEI_H_

