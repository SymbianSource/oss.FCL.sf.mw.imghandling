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


#include "tnmgetimei.h"
#include "thumbnailmanagerconstants.h"

CTnmgetimei* CTnmgetimei::NewL()
    {
    CTnmgetimei *self = CTnmgetimei::NewLC();
    CleanupStack::Pop();
    return self;
    }

CTnmgetimei* CTnmgetimei::NewLC()
    {
    CTnmgetimei *self = new (ELeave) CTnmgetimei();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

void CTnmgetimei::ConstructL()
   {
   iTelephony = CTelephony::NewL();
   CActiveScheduler::Add(this);
   }

CTnmgetimei::~CTnmgetimei()
    {
    Cancel();

    delete iTelephony;
    }

TBuf<KImeiBufferSize> CTnmgetimei::GetIMEI()
    {   
    
    CTelephony::TPhoneIdV1Pckg phoneIdPckg( iV1 );  
    
    iTelephony->GetPhoneId( iStatus, phoneIdPckg );
    SetActive();
    iAsw.Start();
    Deque();
    return iImei;
        
    }

void CTnmgetimei::DoCancel()
    {
    iTelephony->CancelAsync(CTelephony::EGetPhoneIdCancel);
    }
   
void CTnmgetimei::RunL()
    {
    if(iStatus == KErrNone)
        {
        iImei = iV1.iSerialNumber;
        }
    iAsw.AsyncStop();
    }
