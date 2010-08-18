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
#ifndef THUMBNAILFETCHEDCHECKER_H
#define THUMBNAILFETCHEDCHECKER_H

#include <e32base.h>
#include <thumbnailmanager.h>

NONSHARABLE_CLASS( CThumbnailFetchedChecker ): public CBase
    {
public:
    static CThumbnailFetchedChecker* NewL();
    virtual ~CThumbnailFetchedChecker();
public:
    TInt LastFetchResult( const TDesC& aUri, const TThumbnailSize aThumbnailSize );
    void SetFetchResult( const TDesC& aUri, const TThumbnailSize aThumbnailSize, TInt aError );
    void DeleteFetchResult( const TDesC& aUri );
    void RenameFetchResultL( const TDesC& aNewUri, const TDesC& aOldUri );
    void Reset();
private:
    CThumbnailFetchedChecker();
    
    NONSHARABLE_CLASS( CEntry ) : public CBase
        {
    public:
        static CEntry* NewL( const TDesC& aUri, const TThumbnailSize aThumbnailSize, TInt aError );
        static TInt FindCB( const CEntry& aEntry1, const CEntry& aEntry );
        static TInt FindCBUri( const TDesC* aUri, const CEntry& aEntry );
        static TInt InsertCB( const CEntry& aEntry1, const CEntry& aEntry2 );
        CEntry();
        virtual ~CEntry();
    public:
        HBufC* iUri;
        TInt32 iSize;
        TInt iError;
        };

private:
    RPointerArray< CEntry > iNotFetched;
};

#endif // THUMBNAILFETCHEDCHECKER_H
