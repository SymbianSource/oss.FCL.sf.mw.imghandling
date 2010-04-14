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

const int KMaxStoredEntries = 50;

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
TInt CThumbnailFetchedChecker::LastFetchResult( const TDesC& aUri )
    {
    TInt i = iNotFetched.FindInOrder( aUri, CEntry::FindCB );
    if ( i >= 0 && i < iNotFetched.Count() )
        {
        return iNotFetched[ i ]->iError;
        }
    return KErrNone;
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::SetFetchResult()
// -----------------------------------------------------------------------------
//
void CThumbnailFetchedChecker::SetFetchResult( const TDesC& aUri, TInt aError )
    {
    if ( aError == KErrNone )
        {
        // Do not store successful results
        TInt i = iNotFetched.FindInOrder( aUri, CEntry::FindCB );
        if ( i >= 0 && i < iNotFetched.Count() )
            {
            delete iNotFetched[ i ];
            iNotFetched.Remove( i );
            }
        }
    else
        {
        // Add or update
        CEntry* entry = CEntry::New( aUri, aError );
        if ( entry )
            {
            TInt err = iNotFetched.Find( entry );
            if ( err != KErrNotFound )
                {
                TInt i = iNotFetched.FindInOrder( aUri, CEntry::FindCB );
                if ( i >= 0 && i < iNotFetched.Count() )
                    {
                    iNotFetched[ i ]->iError = aError;
                    }
                }
            else 
                {
                if( iNotFetched.Count() < KMaxStoredEntries )
                    {
                    TInt err = iNotFetched.InsertInOrder( entry, CEntry::InsertCB );
                    if ( err != KErrNone )
                        {
                        delete entry;
                        }
                    }
                }
            }
        }
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::Reset()
// -----------------------------------------------------------------------------
//
void CThumbnailFetchedChecker::Reset()
    {
    iNotFetched.ResetAndDestroy();
    }

// -----------------------------------------------------------------------------
// CThumbnailFetchedChecker::CEntry::New()
// -----------------------------------------------------------------------------
//
CThumbnailFetchedChecker::CEntry* CThumbnailFetchedChecker::CEntry::New(
        const TDesC& aUri, TInt aError )
    {
    CEntry* self = new (ELeave) CEntry();
    if ( self )
        {
        self->iUri = aUri.Alloc();
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
    const TDesC* aUri, const CThumbnailFetchedChecker::CEntry& aEntry )
    {
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
    return aEntry1.iUri->CompareF( *( aEntry2.iUri ) );
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
    }
