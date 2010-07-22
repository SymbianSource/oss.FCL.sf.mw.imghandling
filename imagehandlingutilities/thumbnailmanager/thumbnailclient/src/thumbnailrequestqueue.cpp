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


#include "thumbnailrequestqueue.h"
#include "thumbnailrequestactive.h"
#include "thumbnaillog.h"


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::NewL()
// Two-phased constructor.
// ---------------------------------------------------------------------------
//
CThumbnailRequestQueue* CThumbnailRequestQueue::NewL()
    {
    CThumbnailRequestQueue* self = new( ELeave )CThumbnailRequestQueue();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::CThumbnailRequestQueue()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailRequestQueue::CThumbnailRequestQueue()
    {
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestQueue::ConstructL()
    {
    iActiveRequests = 0;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::~CThumbnailRequestQueue()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailRequestQueue::~CThumbnailRequestQueue()
    {
    TN_DEBUG1( "CThumbnailRequestQueue::~CThumbnailRequestQueue()");
    
    iRequests.ResetAndDestroy();
    
    TN_DEBUG1( "CThumbnailRequestQueue::~CThumbnailRequestQueue() - All requests deleted");
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::Process()
// Activates next request if possible.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestQueue::Process()
    {
    TN_DEBUG1( "CThumbnailRequestQueue::Process()");
    
    while ( (iActiveRequests < KMaxClientRequests) &&
            (iRequests.Count() > iActiveRequests) )
        {             
        CThumbnailRequestActive* selectedRequest = NULL;
        TInt priority( KMinTInt );
        TInt reqPriority;
        CThumbnailRequestActive* request = NULL;
        
        for ( TInt i = 0; i < iRequests.Count(); i++ )
           {
           request = iRequests[i];

           // this task is not yet activated or processed
           if( request && !request->RequestCompleted() && !request->IsRequestActive()  )
               {
               TN_DEBUG4( "CThumbnailRequestQueue::Process() - candidate at %d, id = %d, (0x%08x)", i, 
                       request->RequestId(), 
                       request);
               reqPriority = request->Priority();
               if ( reqPriority > priority )
                   {
                   priority = reqPriority;
                   selectedRequest = request;
                   }
               }
           }
        
        // activate selected
        if ( selectedRequest )
           {
           TN_DEBUG1( "CThumbnailRequestQueue::Process() - starting next request");
                    
           iActiveRequests++;
           
           TRAPD(err, selectedRequest->StartL());
           if (err != KErrNone)
               {
               TN_DEBUG1( "CThumbnailRequestQueue::Process() - starting request failed");
               
               selectedRequest->StartError(err);
               }
           }
         else
            {
            break;
            }
        }
    
    TN_DEBUG3( "CThumbnailRequestQueue::Process() end - requests: %d, active requests: %d",
               iRequests.Count(), iActiveRequests );
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::AddRequestL()
// Adds new request to the queue.
// ---------------------------------------------------------------------------
//
void CThumbnailRequestQueue::AddRequestL( CThumbnailRequestActive* aRequest )
    {
    RemoveCompleted(NULL);
    iRequests.AppendL( aRequest );
    
    TN_DEBUG3( "CThumbnailRequestQueue::AddRequestL() end - requests: %d, active requests: %d",
               iRequests.Count(), iActiveRequests );
    }

void CThumbnailRequestQueue::RemoveCompleted( CThumbnailRequestActive* aRequestAO)
    {       
    //process completed queue and remove finished tasks
    for ( TInt i = iRequests.Count() -1; i >= 0 && iRequests.Count(); i-- )
         {
         CThumbnailRequestActive* request = iRequests[i];
         
         // remove completed task if it's not active anymore and not this
         if ( request->RequestCompleted() && !request->IsRequestActive() && aRequestAO != request)
             {
             // delete completed task
             TN_DEBUG3( "CThumbnailRequestQueue::RemoveCompleted() - deleted id = %d (0x%08x)", request->RequestId(), request);
             delete request;
             iRequests.Remove( i );
             }
         }
    
    if(!iRequests.Count())
        {
        iRequests.Compress();
        }
     
     TN_DEBUG3( "CThumbnailRequestQueue::RemoveCompleted() end - requests: %d, active requests: %d",
                    iRequests.Count(), iActiveRequests );
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::CancelRequest()
// Removes specific request from the queue.
// ---------------------------------------------------------------------------
//
TInt CThumbnailRequestQueue::CancelRequest( const TThumbnailRequestId aRequestId )
    {
    TN_DEBUG2( "CThumbnailRequestQueue::CancelRequest() - request ID: %d", aRequestId);
    
    TInt res = KErrNotFound;

    for ( TInt i = iRequests.Count(); --i >= 0; )
        {
        CThumbnailRequestActive* request = iRequests[i];
        if ( request->RequestId() == aRequestId )
            {
            if (iRequests[i]->IsActive()) 
                {
                // this doesn't yet actually cancel/complete the AO
                iRequests[i]->AsyncCancel();
                
                TN_DEBUG2( "CThumbnailRequestQueue::CancelRequest() - canceled request ID: %d", aRequestId);
                }
            else
                {
                delete request;
                iRequests.Remove( i );
                          
                TN_DEBUG2( "CThumbnailRequestQueue::CancelRequest() - removed request ID: %d", aRequestId);
                }

            res = KErrNone;
            break;
            }
        }
    
	RemoveCompleted(NULL);
	
    Process();
    
    return res;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::ChangeReqPriority()
// Changes priority of a request.
// ---------------------------------------------------------------------------
//
TInt CThumbnailRequestQueue::ChangePriority( const TThumbnailRequestId aRequestId,
                                             const TInt aNewPriority )
    {
    TN_DEBUG1( "CThumbnailRequestQueue::ChangePriority()");
    
    TInt err = KErrNotFound;
    const TInt count = iRequests.Count();
    
    for ( TInt i( 0 ); i < count; i++ )
        {
        if ( iRequests[i]->RequestId() == aRequestId )
            {
            iRequests[i]->ChangePriority( aNewPriority );
            
            err = KErrNone;
            break;
            }
        }
    
    return err;
    }


// ---------------------------------------------------------------------------
// CThumbnailRequestQueue::RequestComplete()
// Completes the request
// ---------------------------------------------------------------------------
//
void CThumbnailRequestQueue::RequestComplete(CThumbnailRequestActive* aRequestAO)
    {
    TN_DEBUG2( "CThumbnailRequestQueue::RequestComplete(0x%08x)", aRequestAO);
    
    iActiveRequests--;
    if(iActiveRequests <= -1)
        {
        iActiveRequests = 0;
        }
    
    RemoveCompleted( aRequestAO );
    
    Process();
    }


// End of file
