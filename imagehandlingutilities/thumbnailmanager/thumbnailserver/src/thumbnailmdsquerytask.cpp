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


#include <e32base.h>

#include <mdeconstants.h>
#include <thumbnailmanager.h>

#include "thumbnailmdsquerytask.h"
#include "thumbnailmanagerconstants.h"
#include "thumbnaillog.h"
#include "thumbnailserver.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailmdsquerytaskTraces.h"
#endif


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::CThumbnailMDSQueryTask()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailMDSQueryTask::CThumbnailMDSQueryTask(
        CThumbnailTaskProcessor& aProcessor, TInt aPriority, CMdESession* aMdESession, CThumbnailServer& aServer): 
        CThumbnailTask( aProcessor, aPriority ), iMdESession( aMdESession ), iQuery(NULL), 
		iServer(aServer), iUpdateToDb(ETrue)
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::CThumbnailMDSQueryTask()", this );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMDSQUERYTASK_CTHUMBNAILMDSQUERYTASK, "CThumbnailMDSQueryTask::CThumbnailMDSQueryTask;this=%o", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::~CThumbnailMDSQueryTask()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailMDSQueryTask::~CThumbnailMDSQueryTask()
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::~CThumbnailMDSQueryTask()", this);
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILMDSQUERYTASK_CTHUMBNAILMDSQUERYTASK, "CThumbnailMDSQueryTask::~CThumbnailMDSQueryTask;this=%o", this );
           
    if (iQuery)
        {
        iQuery->Cancel();
        delete iQuery;
        iQuery = NULL;
        }
    }


// -----------------------------------------------------------------------------
// CThumbnailMDSQueryTask::HandleQueryNewResults()
// -----------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::HandleQueryNewResults( CMdEQuery& /*aQuery*/,
                                               const TInt /*aFirstNewItemIndex*/,
                                               const TInt /*aNewItemCount*/ )
    {
    // No implementation required
    }

// -----------------------------------------------------------------------------
// CThumbnailMDSQueryTask::HandleQueryCompleted()
// -----------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::HandleQueryCompleted( CMdEQuery& /*aQuery*/, const TInt aError )
    {
    TN_DEBUG3( "CThumbnailMDSQueryTask::HandleQueryCompleted(0x%08x), aError == %d", this, aError );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED, "CThumbnailMDSQueryTask::HandleQueryCompleted;this=%o", this );
    OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED, "CThumbnailMDSQueryTask::HandleQueryCompleted;aError=%d", aError );
    
    
    // if no errors in query
    if (aError == KErrNone && iQuery && iQuery->Count() > 0)
        {
        if( iQueryType == EURI )
            {
            const CMdEObject* object = &iQuery->Result(0);
            
            TN_DEBUG2( "CThumbnailMDSQueryTask::HandleQueryCompleted() - URI = %S", &object->Uri() );
            OstTraceExt1( TRACE_NORMAL, DUP2_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED, "CThumbnailMDSQueryTask::HandleQueryCompleted;object->Uri()=%S", object->Uri() );
                            
            // return path to client side       
            if( iDelete )
                {
                TN_DEBUG2( "CThumbnailMDSQueryTask::HandleQueryCompleted() delete %S", &iUri );
                OstTraceExt1( TRACE_NORMAL, DUP3_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED, "CThumbnailMDSQueryTask::HandleQueryCompleted - delete;iUri()=%S", iUri );
                TRAP_IGNORE( iServer.DeleteThumbnailsL( iUri ) );
                }
            else
                {
                ReturnPath(object->Uri());
                }
            }
        else
            {
            TN_DEBUG1( "CThumbnailMDSQueryTask::HandleQueryCompleted() - Don't ever come here!" );
            OstTrace0( TRACE_NORMAL, DUP4_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED, "CThumbnailMDSQueryTask::HandleQueryCompleted - Don't ever come here!" );
            if (ClientThreadAlive())
                {  
                Complete( KErrNotFound );
                ResetMessageData();
                }
            __ASSERT_DEBUG((EFalse), User::Panic(_L("CThumbnailMDSQueryTask::HandleQueryCompleted()"), KErrNotSupported));
            }
        }
    else
        {
        TN_DEBUG1( "CThumbnailMDSQueryTask::HandleQueryCompleted() - No results." );
        OstTrace0( TRACE_NORMAL, DUP5_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED, "CThumbnailMDSQueryTask::HandleQueryCompleted - No results." );
        if(!iDelete)
            {
            if (ClientThreadAlive())
                {  
                Complete( KErrNotFound );
                ResetMessageData();
                }
            }
        }
   }


// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::StartL()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::StartL()
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::StartL()", this );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMDSQUERYTASK_STARTL, "CThumbnailMDSQueryTask::StartL;this=%o", this );

    CThumbnailTask::StartL();
    
    if (iMessage.Handle())
        {
        // start query
        iQuery->FindL();
        }
    else
        {
        // no point to continue
        Complete( KErrCancel );
        ResetMessageData();
        }
    }


// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::RunL()
    {
    // No implementation required
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::RunL()", this );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMDSQUERYTASK_RUNL, "CThumbnailMDSQueryTask::RunL;this=%o", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::DoCancel()
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::DoCancel()", this );
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMDSQUERYTASK_DOCANCEL, "CThumbnailMDSQueryTask::DoCancel;this=%o", this );
    
    iQuery->Cancel();
    }

// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::QueryPathByIdL()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::QueryPathByIdL(TThumbnailId aId, TBool aDelete)
    {
    TN_DEBUG1( "CThumbnailMDSQueryTask()::QueryPathByIdL()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILMDSQUERYTASK_QUERYPATHBYIDL, "CThumbnailMDSQueryTask::QueryPathByIdL" );
    iQueryType = EURI;
    iDelete = aDelete;
    CMdENamespaceDef* defNamespace = &iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& objDef = defNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject );
    
    delete iQuery;
    iQuery = NULL;
    
    iQuery = iMdESession->NewObjectQueryL( *defNamespace, objDef, this );
	
	if(iQuery)
		{
	    iQuery->SetResultMode( EQueryResultModeItem );

    	CMdELogicCondition& rootCondition = iQuery->Conditions();
	    rootCondition.SetOperator( ELogicConditionOperatorOr );
    
	    // add ID condition
    	rootCondition.AddObjectConditionL( aId );
		}
    }

// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::ReturnPath()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::ReturnPath(const TDesC& aUri)
    {
    if ( ClientThreadAlive() )
        {
        // add path to message
        TInt ret = iMessage.Read( 0, iRequestParams );      
        if(ret == KErrNone)
            {
            TThumbnailRequestParams& params = iRequestParams();
            params.iFileName = aUri;
            ret = iMessage.Write( 0, iRequestParams );
            }
            
        // complete the message with a code from which client side
        // knows to make a new request using the path
        Complete( KThumbnailErrThumbnailNotFound );
        ResetMessageData();
        }
    }

// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::SetUpdateToDb()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::SetUpdateToDb(const TBool& aUpdateToDb )
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask()::SetCompleteTask() aUpdateToDb == %d", aUpdateToDb);
    OstTrace1( TRACE_NORMAL, CTHUMBNAILMDSQUERYTASK_SETUPDATETODB, "CThumbnailMDSQueryTask::SetUpdateToDb;aUpdateToDb=%u", aUpdateToDb );
    iUpdateToDb = aUpdateToDb;
    }
