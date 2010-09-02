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
* Description:
 *
*/

#include "thumbnailfetchedchecker.h"
#include "thumbnaillog.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "thumbnailfetchedcheckerTraces.h"
#endif


const int KMaxStoredEntries = 100;

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CThumbnailFetchedChecker()
// -----------------------------------------------------------------------------
//
CThumbnailFetchedChecker* CThumbnailFetchedChecker::NewL()
    {
    return new (ELeave) CThumbnailFetchedChecker();
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CThumbnailFetchedChecker()
// -----------------------------------------------------------------------------
//
CThumbnailFetchedChecker::CThumbnailFetchedChecker()
    {
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::~CThumbnailFetchedChecker()
// -----------------------------------------------------------------------------
//
CThumbnailFetchedChecker::~CThumbnailFetchedChecker()
    {
    iNotFetched.ResetAndDestroy();
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::LastFetchResult()
// -----------------------------------------------------------------------------
//
TInt CThumbnailFetchedChecker::LastFetchResult( const TDesC& aUri, const TThumbnailSize aThumbnailSize )
    {
    TN_DEBUG3( "CThumbnailFetchedChecker::LastFetchResult(aUri=%S aThumbnailSize=%d)", &aUri, aThumbnailSize);
    OstTraceExt2( TRACE_NORMAL, CTHUMBNAILFETCHEDCHECKER_LASTFETCHRESULT, "CThumbnailFetchedChecker::LastFetchResult;aUri=%S;aThumbnailSize=%hu", aUri, aThumbnailSize );

    CEntry* entry = NULL;
    TRAPD( err, entry = CEntry::NewL( aUri, aThumbnailSize, KErrNone ) );
    if ( !err && entry )
        {
        TInt ret = iNotFetched.FindInOrder( entry, CEntry::FindCB );
        if ( ret != KErrNotFound )
            {
            TN_DEBUG1( "CThumbnailFetchedChecker::LastFetchResult() -> error found");
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILFETCHEDCHECKER_LASTFETCHRESULT, "CThumbnailFetchedChecker::LastFetchResult  - error found" );
            delete entry;
            entry = NULL;
            return iNotFetched[ ret ]->iError;
            }
        }

    delete entry;        
    entry = NULL;
        
    return KErrNone;
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::SetFetchResult()
// -----------------------------------------------------------------------------
//
void CThumbnailFetchedChecker::SetFetchResult( const TDesC& aUri, const TThumbnailSize aThumbnailSize, TInt aError )
    {
    TN_DEBUG4( "CThumbnailFetchedChecker::SetFetchResult(aUri=%S aThumbnailSize=%d aError=%d)", &aUri, aThumbnailSize, aError);
    OstTraceExt3( TRACE_NORMAL, CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT, "CThumbnailFetchedChecker::SetFetchResult;aUri=%S;aThumbnailSize=%u;aError=%d", aUri, aThumbnailSize, aError );
    if ( aError == KErrNone )
        {
        // Remove successful results from store
        CEntry* entry = NULL;
        TRAPD( err, entry = CEntry::NewL( aUri, aThumbnailSize, aError ) );
        if ( !err && entry )
            {
            TInt i = iNotFetched.FindInOrder( entry, CEntry::FindCB );
            if ( i >= 0 )
                {
                TN_DEBUG2( "CThumbnailFetchedChecker::LastFetchResult() -> Remove successful results from store %d",  iNotFetched.Count() );
                OstTrace1( TRACE_NORMAL, DUP1_CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT, "CThumbnailFetchedChecker::SetFetchResult;iNotFetched.Count()=%d", iNotFetched.Count() );
                delete iNotFetched[ i ];
                iNotFetched[ i ] = NULL;
                iNotFetched.Remove( i );
                }
            }
        entry = NULL;
        delete entry;
        }
    else
        {

        // Add or update
        CEntry* entry = NULL;
        TRAPD( err, entry = CEntry::NewL( aUri, aThumbnailSize, aError ) );
        OstTraceExt3( TRACE_NORMAL, DUP4_CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT, "CThumbnailFetchedChecker::SetFetchResult;aUri=%S;aThumbnailSize=%u;aError=%d", aUri, aThumbnailSize, aError );
        if ( !err && entry )
            {
            err = iNotFetched.Find( entry );
            if ( err != KErrNotFound )
                {
                // update existing fetch result
                TInt i = iNotFetched.FindInOrder( entry, CEntry::FindCB );
                if ( i >= 0 )
                    {
                    TN_DEBUG1( "CThumbnailFetchedChecker::LastFetchResult() -> Update fetched tn error" );
                    OstTrace0( TRACE_NORMAL, DUP2_CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT, "CThumbnailFetchedChecker::SetFetchResult - Update fetched tn error" );
                    iNotFetched[ i ]->iError = aError;
                    }
                }
            else 
                {
                if( iNotFetched.Count() < KMaxStoredEntries )
                    {
                    // insert new fetch result
                    TInt err = iNotFetched.InsertInOrder( entry, CEntry::InsertCB );
                    if ( err == KErrNone )
                        {
                        TN_DEBUG2( "CThumbnailFetchedChecker::LastFetchResult() -> Inserted new fetched tn error %d", iNotFetched.Count());	
                        OstTrace1( TRACE_NORMAL, DUP3_CTHUMBNAILFETCHEDCHECKER_SETFETCHRESULT, "CThumbnailFetchedChecker::SetFetchResult;iNotFetched.Count()=%d", iNotFetched.Count() );
                        entry = NULL; // owned by array now
                        }
                    }
                }
            
            delete entry;
            entry = NULL;
            }
        }
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::DeleteFetchResult()
// -----------------------------------------------------------------------------
//
void CThumbnailFetchedChecker::DeleteFetchResult( const TDesC& aUri )
    {
    TN_DEBUG2( "CThumbnailFetchedChecker::DeleteFetchResult(%S)", &aUri);
    OstTraceExt1( TRACE_NORMAL, CTHUMBNAILFETCHEDCHECKER_DELETEFETCHRESULT, "CThumbnailFetchedChecker::DeleteFetchResult;aUri=%S", aUri );
    // delete all entries of passed uri
    TInt ret;
    do
        {
        ret = iNotFetched.FindInOrder( aUri, CEntry::FindCBUri );
        if ( ret >= 0 )
            {
            TN_DEBUG1( "CThumbnailFetchedChecker::DeleteFetchResult() -> Deteled URI from fetched list" );	
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILFETCHEDCHECKER_DELETEFETCHRESULT, "CThumbnailFetchedChecker::DeleteFetchResult - Deteled URI from fetched list" );
            delete iNotFetched[ ret ];
            iNotFetched[ ret ] = NULL;
            iNotFetched.Remove( ret );
            }
        }
    while(ret != KErrNotFound );
    
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::LastFetchResult()
// -----------------------------------------------------------------------------
//
void CThumbnailFetchedChecker::RenameFetchResultL( const TDesC& aNewUri, const TDesC& aOldUri )
    {
    TN_DEBUG3( "CThumbnailFetchedChecker::RenameFetchResult(aNewUri=%S aOldUri=%S)", &aNewUri, &aOldUri);
    OstTraceExt2( TRACE_NORMAL, CTHUMBNAILFETCHEDCHECKER_RENAMEFETCHRESULTL, "CThumbnailFetchedChecker::RenameFetchResultL;aNewUri=%S;aOldUri=%S", aNewUri, aOldUri );
    // change every occurence of passed uri
    TInt ret;
    do
        {
        ret = iNotFetched.FindInOrder( aOldUri, CEntry::FindCBUri );
        if ( ret >= 0 )
            {
            delete iNotFetched[ ret ]->iUri;
            iNotFetched[ ret ]->iUri = NULL;

            iNotFetched[ ret ]->iUri = aNewUri.AllocL();
            TN_DEBUG1( "CThumbnailFetchedChecker::RenameeFetchResult() -> Renamed URI in fetched list" );	
            OstTrace0( TRACE_NORMAL, DUP1_CTHUMBNAILFETCHEDCHECKER_RENAMEFETCHRESULTL, "CThumbnailFetchedChecker::RenameFetchResultL - Renamed URI in fetched list" );
            }
        }
    while(ret != KErrNotFound );
    
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::Reset()
// -----------------------------------------------------------------------------
//
void CThumbnailFetchedChecker::Reset()
    {
    TN_DEBUG1( "CThumbnailFetchedChecker::Reset()");
    OstTrace0( TRACE_NORMAL, CTHUMBNAILFETCHEDCHECKER_RESET, "CThumbnailFetchedChecker::Reset" );
    iNotFetched.ResetAndDestroy();
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CEntry::NewL()
// -----------------------------------------------------------------------------
//
CThumbnailFetchedChecker::CEntry* CThumbnailFetchedChecker::CEntry::NewL(
        const TDesC& aUri, const TThumbnailSize aThumbnailSize, TInt aError )
    {
    TN_DEBUG4( "CThumbnailFetchedChecker::CEntry::NewL(aUri=%S aThumbnailSize=%d aError=%d)", &aUri, aThumbnailSize, aError);
    OstTraceExt3( TRACE_NORMAL, CENTRY_NEWL, "CEntry::NewL;aUri=%S;aThumbnailSize=%u;aError=%d", aUri, aThumbnailSize, aError );
    CEntry* self  = new (ELeave) CEntry();
    if ( self )
        {
        self->iUri = aUri.AllocL();
        self->iSize = aThumbnailSize;
        self->iError = aError;
        if ( !self->iUri )
            {
            delete self;
            self = NULL;
            }
        }
    return self;
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CEntry::FindCB()
// -----------------------------------------------------------------------------
//
TInt CThumbnailFetchedChecker::CEntry::FindCB(
        const CThumbnailFetchedChecker::CEntry& aEntry, const CThumbnailFetchedChecker::CEntry& aEntry1 )
    {
    TN_DEBUG1( "CThumbnailFetchedChecker::CEntry::FindCB");
    OstTrace0( TRACE_NORMAL, CENTRY_FINDCB, "CEntry::FindCB" );
    if( aEntry1.iSize == aEntry.iSize)
        {
        // return index if size and uri matches
        return aEntry.iUri->CompareF( *( aEntry1.iUri ) );
        }
    else 
        {
        return KErrNotFound;
        }
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CEntry::FindCBUri()
// -----------------------------------------------------------------------------
//
TInt CThumbnailFetchedChecker::CEntry::FindCBUri(
        const TDesC* aUri, const CThumbnailFetchedChecker::CEntry& aEntry )
    {
    TN_DEBUG2( "CThumbnailFetchedChecker::CEntry::FindCBUri(aUri=%S", &aUri);
    OstTrace0( TRACE_NORMAL, CENTRY_FINDCBURI, "CEntry::FindCBUri" );
    return aUri->CompareF( *( aEntry.iUri ) );
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CEntry::InsertCB()
// -----------------------------------------------------------------------------
//
TInt CThumbnailFetchedChecker::CEntry::InsertCB(
        const CThumbnailFetchedChecker::CEntry& aEntry1,
        const CThumbnailFetchedChecker::CEntry& aEntry2 )
    {
    TN_DEBUG1( "CThumbnailFetchedChecker::CEntry::InsertCB");
    OstTrace0( TRACE_NORMAL, CENTRY_INSERTCB, "CEntry::InsertCB" );
    if( aEntry1.iSize == aEntry2.iSize)
        {
        // return index if size and uri matches
        return aEntry1.iUri->CompareF( *( aEntry2.iUri ) );
        }
    else 
        {
        return KErrNotFound;
        }
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CEntry::CEntry()
// -----------------------------------------------------------------------------
//
CThumbnailFetchedChecker::CEntry::CEntry()
    {
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CEntry::~CEntry()
// -----------------------------------------------------------------------------
//
CThumbnailFetchedChecker::CEntry::~CEntry()
    {
    delete iUri;
    iUri = NULL;
    }
