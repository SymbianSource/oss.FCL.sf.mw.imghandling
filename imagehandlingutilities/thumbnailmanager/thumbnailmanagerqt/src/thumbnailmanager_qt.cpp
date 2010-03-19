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
* Description: 
*
*/

#include <qsize.h>
#include "thumbnailmanager_qt.h"
#include "thumbnailmanager_p_qt.h"
 
EXPORT_C ThumbnailManager::ThumbnailManager( QObject* parentPtr ) :
QObject( parentPtr ),
d( new ThumbnailManagerPrivate() )
{
    QObject::connect( d, SIGNAL( thumbnailReady( QPixmap , void * , int , int ) ),
            this, SIGNAL( thumbnailReady( QPixmap , void * , int , int ) ) );
}


EXPORT_C ThumbnailManager::~ThumbnailManager()
{
    QObject::disconnect( d, SIGNAL( thumbnailReady( QPixmap , void * , int , int ) ),
            this, SIGNAL( thumbnailReady( QPixmap , void * , int , int ) ) );
    if( NULL != d ){
        delete d;
    }
}

EXPORT_C ThumbnailManager::QualityPreference ThumbnailManager::qualityPreference() const
{
    return d->qualityPreference();
}

EXPORT_C bool ThumbnailManager::setQualityPreference( QualityPreference
    qualityPreference )
{
    return d->setQualityPreference( qualityPreference );
}
 
EXPORT_C QSize ThumbnailManager::thumbnailSize() const
{
    return d->thumbnailSize();
}

EXPORT_C bool ThumbnailManager::setThumbnailSize( const QSize& thumbnailSize )
{
    return d->setThumbnailSize( thumbnailSize );
}

EXPORT_C bool ThumbnailManager::setThumbnailSize( ThumbnailSize thumbnailSize )
{
    return d->setThumbnailSize( thumbnailSize );
}

EXPORT_C ThumbnailManager::ThumbnailMode ThumbnailManager::mode() const
{
    return d->mode();
}

EXPORT_C bool ThumbnailManager::setMode( ThumbnailMode mode )
{
    return d->setMode( mode );
}

EXPORT_C int ThumbnailManager::getThumbnail( const QString& fileName, void * clientData, 
        int priority )
{
    return d->getThumbnail( fileName, clientData, priority );
}

EXPORT_C int ThumbnailManager::getThumbnail( unsigned long int thumbnailId, void * clientData, 
        int priority )
{
    return d->getThumbnail( thumbnailId, clientData, priority );
}    

EXPORT_C int ThumbnailManager::setThumbnail( const QPixmap& source, const QString& filename,
        void * clientData , int priority )
{
    return d->setThumbnail( source, filename, clientData, priority );
}

EXPORT_C void ThumbnailManager::deleteThumbnails( const QString& fileName )
{
    d->deleteThumbnails( fileName );
}

EXPORT_C void ThumbnailManager::deleteThumbnails( unsigned long int thumbnailId )
{
    d->deleteThumbnails( thumbnailId );
}

EXPORT_C bool ThumbnailManager::cancelRequest( int id )
{
    return d->cancelRequest( id );
}

EXPORT_C bool ThumbnailManager::changePriority( int id, int newPriority )
{
    return d->changePriority( id, newPriority );
}

