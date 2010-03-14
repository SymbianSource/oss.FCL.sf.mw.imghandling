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


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::CThumbnailMDSQueryTask()
// C++ default constructor can NOT contain any code, that might leave.
// ---------------------------------------------------------------------------
//
CThumbnailMDSQueryTask::CThumbnailMDSQueryTask(
        CThumbnailTaskProcessor& aProcessor, TInt aPriority, CMdESession* aMdESession, CThumbnailServer& aServer): 
        CThumbnailTask( aProcessor, aPriority ), iMdESession( aMdESession ), iServer(aServer), iUpdateToDb(ETrue)
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::CThumbnailMDSQueryTask()", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::~CThumbnailMDSQueryTask()
// Destructor.
// ---------------------------------------------------------------------------
//
CThumbnailMDSQueryTask::~CThumbnailMDSQueryTask()
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::~CThumbnailMDSQueryTask()", this);
           
    if (iQuery)
        {
        iQuery->Cancel();
        delete iQuery;
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
    
    // if no errors in query
    if (aError == KErrNone && iQuery && iQuery->Count() > 0)
        {
        if( iQueryType == EURI )
            {
            const CMdEObject* object = &iQuery->Result(0);
            
            TN_DEBUG2( "CThumbnailMDSQueryTask::HandleQueryCompleted() - URI = %S", &object->Uri() );
                            
            if( iUpdateToDb)
                {
                //update IDs to database by Path
                if (ClientThreadAlive(EFalse))
                    {               
                    TInt err = iMessage.Read( 0, iRequestParams );
                    if( err == KErrNone)
                        {
                        TThumbnailRequestParams& params = iRequestParams();
                        TRAP_IGNORE(iServer.UpdateIDL(object->Uri(), params.iThumbnailId));
                        }                   
                    }
                }
    
            // return path to client side            
            ReturnPath(object->Uri());
            }
        else if (iQueryType == EId )
            {
            const CMdEObject* object = &iQuery->Result(0);
            
            TN_DEBUG2( "CThumbnailMDSQueryTask::HandleQueryCompleted() - Id = %d",  object->Id());
            
            if( object->Id() != KNoId)
                {
                if( iUpdateToDb)
                    {
                    TRAP_IGNORE(iServer.UpdateIDL(object->Uri(), object->Id() ));
                    }
                else
                    {
                    // add Id to message
                    if (ClientThreadAlive(EFalse))
                        {
                        TInt ret = iMessage.Read( 0, iRequestParams );
                        if( ret == KErrNone)
                            {
                            TThumbnailRequestParams& params = iRequestParams();
                            params.iThumbnailId = object->Id();
                            ret = iMessage.Write( 0, iRequestParams );
                            }
                        }
                    }
                }
            
            // complete the message with a code from which client side
            // knows to make a new request using the path
            if (ClientThreadAlive(EFalse))
                {  
                Complete( KThumbnailErrThumbnailNotFound );
                ResetMessageData();
                }
            }
        else
            {
            TN_DEBUG1( "CThumbnailMDSQueryTask::HandleQueryCompleted() - Don't ever come here" );
            if (ClientThreadAlive(EFalse))
                {  
                Complete( KErrNotFound );
                ResetMessageData();
                }
            }
        }
    else
        {
        TN_DEBUG1( "CThumbnailMDSQueryTask::HandleQueryCompleted() - No results." );
        
        if( iQueryType == EId )
            {
            if( iUpdateToDb)
                {
                TN_DEBUG2( "CThumbnailMDSQueryTask::HandleQueryCompleted() delete %S", &iUri );
                TRAP_IGNORE( iServer.DeleteThumbnailsL( iUri ) );
                }
               
            if (ClientThreadAlive(EFalse))
                {     
                Complete( KThumbnailErrThumbnailNotFound );
                ResetMessageData();
                }
            }
        else 
            {
            if (ClientThreadAlive(EFalse))
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

    CThumbnailTask::StartL();

    // get client thread
    TInt err = iMessage.Client( iClientThread );
    if (err != KErrNone)
        {
        TN_DEBUG2( "CThumbnailTask(0x%08x)::ClientThreadAlive() - client thread not found", this);
    
        ResetMessageData();
        }
    
    // start query
    iQuery->FindL();
    }


// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::RunL()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::RunL()
    {
    // No implementation required
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::RunL()", this );
    }


// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::DoCancel()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::DoCancel()
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask(0x%08x)::DoCancel()", this );
    
    iQuery->Cancel();
    }

// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::QueryPathByIdL()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::QueryPathByIdL(TThumbnailId aId)
    {
    TN_DEBUG1( "CThumbnailMDSQueryTask()::QueryPathByIdL()");
    iQueryType = EURI;
    CMdENamespaceDef* defNamespace = &iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& objDef = defNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject );
    
    iQuery = iMdESession->NewObjectQueryL( *defNamespace, objDef, this );
    iQuery->SetResultMode( EQueryResultModeItem );

    CMdELogicCondition& rootCondition = iQuery->Conditions();
    rootCondition.SetOperator( ELogicConditionOperatorOr );
    
    // add ID condition
    rootCondition.AddObjectConditionL( aId );
    }

// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::ReturnPath()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::ReturnPath(const TDesC& aUri)
    {
    if ( ClientThreadAlive(EFalse) )
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
// CThumbnailMDSQueryTask::QueryIdByPathL()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::QueryIdByPathL(const TDesC& aUri)
    {
    TN_DEBUG1( "CThumbnailMDSQueryTask()::QueryIdByPathL()");
    iQueryType = EId;
    iUri = aUri;
    CMdENamespaceDef* defNamespace = &iMdESession->GetDefaultNamespaceDefL();
    CMdEObjectDef& objDef = defNamespace->GetObjectDefL( MdeConstants::Object::KBaseObject );
    
    iQuery = iMdESession->NewObjectQueryL( *defNamespace, objDef, this );
    iQuery->SetResultMode( EQueryResultModeItem );

    CMdELogicCondition& rootCondition = iQuery->Conditions();
    rootCondition.SetOperator( ELogicConditionOperatorOr );
    
    rootCondition.AddObjectConditionL(EObjectConditionCompareUri, aUri);
    }

// ---------------------------------------------------------------------------
// CThumbnailMDSQueryTask::SetUpdateToDb()
// ---------------------------------------------------------------------------
//
void CThumbnailMDSQueryTask::SetUpdateToDb(const TBool& aUpdateToDb )
    {
    TN_DEBUG2( "CThumbnailMDSQueryTask()::SetCompleteTask() aUpdateToDb == %d", aUpdateToDb);
    iUpdateToDb = aUpdateToDb;
    }
