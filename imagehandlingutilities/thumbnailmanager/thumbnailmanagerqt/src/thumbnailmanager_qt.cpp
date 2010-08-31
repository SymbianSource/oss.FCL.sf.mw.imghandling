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

#include <QSize>
#include "thumbnailmanager_qt.h"
#include "thumbnailmanager_p_qt.h"
 
Q_DECL_EXPORT ThumbnailManager::ThumbnailManager( QObject* parentPtr ) :
QObject( parentPtr ),
d( new ThumbnailManagerPrivate() )
{
    d->q_ptr = this; 
}


Q_DECL_EXPORT ThumbnailManager::~ThumbnailManager()
{
    if( NULL != d ){
        delete d;
    }
}

Q_DECL_EXPORT ThumbnailManager::QualityPreference ThumbnailManager::qualityPreference() const
{
    return d->qualityPreference();
}

Q_DECL_EXPORT bool ThumbnailManager::setQualityPreference( QualityPreference
    qualityPreference )
{
    return d->setQualityPreference( qualityPreference );
}
 
Q_DECL_EXPORT QSize ThumbnailManager::thumbnailSize() const
{
    return d->thumbnailSize();
}

Q_DECL_EXPORT bool ThumbnailManager::setThumbnailSize( const QSize& thumbnailSize )
{
    return d->setThumbnailSize( thumbnailSize );
}

Q_DECL_EXPORT bool ThumbnailManager::setThumbnailSize( ThumbnailSize thumbnailSize )
{
    return d->setThumbnailSize( thumbnailSize );
}

Q_DECL_EXPORT ThumbnailManager::ThumbnailMode ThumbnailManager::mode() const
{
    return d->mode();
}

Q_DECL_EXPORT bool ThumbnailManager::setMode( ThumbnailMode mode )
{
    return d->setMode( mode );
}

Q_DECL_EXPORT int ThumbnailManager::getThumbnail( const QString& fileName, void * clientData, 
        int priority )
{
    return d->getThumbnail( fileName, clientData, priority );
}

Q_DECL_EXPORT int ThumbnailManager::getThumbnail( unsigned long int thumbnailId, void * clientData, 
        int priority )
{
    return d->getThumbnail( thumbnailId, clientData, priority );
}    

Q_DECL_EXPORT int ThumbnailManager::setThumbnail( const QPixmap& source, const QString& filename,
        void * clientData , int priority )
{
    return d->setThumbnail( source, filename, clientData, priority );
}

Q_DECL_EXPORT int ThumbnailManager::setThumbnail( const QImage& source, const QString& filename,
        void * clientData , int priority )
{
    return d->setThumbnail( source, filename, clientData, priority );
}

Q_DECL_EXPORT int ThumbnailManager::setThumbnail( const QString& sourceFileName, const QString& targetFileName,
        const QString& mimeType, void * clientData , int priority )
{
    return d->setThumbnail( sourceFileName, targetFileName, mimeType, clientData, priority );
}

Q_DECL_EXPORT void ThumbnailManager::deleteThumbnails( const QString& fileName )
{
    d->deleteThumbnails( fileName );
}

Q_DECL_EXPORT void ThumbnailManager::deleteThumbnails( unsigned long int thumbnailId )
{
    d->deleteThumbnails( thumbnailId );
}

Q_DECL_EXPORT bool ThumbnailManager::cancelRequest( int id )
{
    return d->cancelRequest( id );
}

Q_DECL_EXPORT bool ThumbnailManager::changePriority( int id, int newPriority )
{
    return d->changePriority( id, newPriority );
}

void ThumbnailManager::connectNotify(const char *signal)
{
    if (QLatin1String(signal) == SIGNAL(thumbnailReady(QPixmap,void*,int,int))) {
        d->connectionCounterPixmap++;
    } else if (QLatin1String(signal) == SIGNAL(thumbnailReady(QImage,void*,int,int))) {
        d->connectionCounterImage++;
    }
}

void ThumbnailManager::disconnectNotify(const char *signal)
{   
    if (QLatin1String(signal) == SIGNAL(thumbnailReady(QPixmap,void*,int,int))) {
        d->connectionCounterPixmap--;
    } else if (QLatin1String(signal) == SIGNAL(thumbnailReady(QImage,void*,int,int))) {
        d->connectionCounterImage--;
    }
}

