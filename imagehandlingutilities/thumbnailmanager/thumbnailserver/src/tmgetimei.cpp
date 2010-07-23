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


#include "tmgetimei.h"
#include "thumbnailmanagerconstants.h"

CTMGetImei* CTMGetImei::NewL()
    {
    CTMGetImei *self = CTMGetImei::NewLC();
    CleanupStack::Pop();
    return self;
    }

CTMGetImei* CTMGetImei::NewLC()
    {
    CTMGetImei *self = new (ELeave) CTMGetImei();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

void CTMGetImei::ConstructL()
   {
   iTelephony = CTelephony::NewL();
   CActiveScheduler::Add(this);
   }

CTMGetImei::~CTMGetImei()
    {
    Cancel();

    delete iTelephony;
    iTelephony = NULL;
    }

TBuf<KImeiBufferSize> CTMGetImei::GetIMEI()
    {    
    CTelephony::TPhoneIdV1Pckg phoneIdPckg( iV1 );  
    
    iTelephony->GetPhoneId( iStatus, phoneIdPckg );
    SetActive();
    iAsw.Start();
    Deque();
    return iImei;       
    }

void CTMGetImei::DoCancel()
    {
    iTelephony->CancelAsync(CTelephony::EGetPhoneIdCancel);
    }
   
void CTMGetImei::RunL()
    {
    if(iStatus == KErrNone)
        {
        iImei = iV1.iSerialNumber;
        }
    iAsw.AsyncStop();
    }
