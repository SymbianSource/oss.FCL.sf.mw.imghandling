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

#ifndef THUMBNAILMANAGER_P_QT_H
#define THUMBNAILMANAGER_P_QT_H

#include <QObject>
#include <QPixmap>

#include <thumbnailmanager.h>
#include <thumbnailmanager_qt.h>
#include <thumbnailmanagerobserver.h>


class QString;
class QSize;
class CThumbnailManager;
class MThumbnailData;
class CFbsBitmap;
class QByteArray;

class TestThumbnailManager;

class ThumbnailManagerPrivate : 
public QObject,
public MThumbnailManagerObserver
    {
    Q_OBJECT
public:

    /**
     * Constructor
     */    
        
    ThumbnailManagerPrivate();

    /**
     * Destructor
     */
    virtual ~ThumbnailManagerPrivate();

    /**
     * Get quality versus performance preference.
     */
    ThumbnailManager::QualityPreference qualityPreference() const;
    
    /**
     * Set quality versus performance preference.
     */
    bool setQualityPreference( ThumbnailManager::QualityPreference qualityPreference );
    
    /**
     * Get the current desired size for thumbnail bitmaps.
     */
    QSize thumbnailSize() const;

    /**
     * Set desired size for thumbnail bitmaps.
     */
    bool setThumbnailSize( const QSize& aThumbnailSize );
    
    /**
     * Set desired size for thumbnail bitmaps.
     */
    bool setThumbnailSize( ThumbnailManager::ThumbnailSize thumbnailSize );

    /**
     * Get current mode for thumbnail generation.
     */
    ThumbnailManager::ThumbnailMode mode() const;

    /**
     * Set mode for thumbnail generation. 
     */
    bool setMode( ThumbnailManager::ThumbnailMode mode );

    /**
     * Get a thumbnail for an object file. If a thumbnail already exists, it
     * is loaded and if a thumbnail does not exist, it is created
     * transparently. 
     */
    int getThumbnail( const QString& fileName, void * clientData, 
            int priority );

    /**
     * Get a persistent thumbnail for an object file. If a thumbnail already
     *  exists, it is loaded and if a thumbnail does not exist, it is created
     * transparently. 
     */    
    int getThumbnail( unsigned long int aThumbnailId, void * clientData, 
            int priority );
    
    /**
     * Set a thumbnail for an object file generated from pixmap delivered.
     * thumbnailReady() signal will be emited when the operation is complete.                           
     */    
    int setThumbnail( const QPixmap& source, const QString& fileName,
            void * clientData, int priority );
    
    /**
     * Set a thumbnail for an object file generated from pixmap delivered.
     * thumbnailReady() signal will be emited when the operation is complete.                           
     */    
    int setThumbnail( const QImage& source, const QString& fileName,
            void * clientData, int priority );
    
    /**
     * Set a thumbnail for an object file generated from source file.
     * thumbnailReady() signal will be emited when the operation is complete.                           
     */    
    int setThumbnail( const QString& sourceFileName, const QString& targetFileName,
            const QString& mimeType, void * clientData, int priority );   
    
    /**
     * Delete all thumbnails for a given object. This is an asynchronous
     * operation, which always returns immediately.
     */
    void deleteThumbnails( const QString& fileName );

    /**
     * Delete all thumbnails for a given object. This is an asynchronous
     * operation, which always returns immediately.
     */
    void deleteThumbnails( unsigned long int thumbnailId );
    
    /**
     * Cancel a thumbnail operation.
     */
    bool cancelRequest( int id );

    /**
     * Change the priority of a queued thumbnail operation.
     */
    bool changePriority( int id, int newPriority );
    
    //FROM  MThumbnailManagerObserver
    /**
     * Preview thumbnail generation or loading is complete.
     *
     * This callback is not called unless EOptimizeForQualityWithPreview flag
     * was specified.
     *
     * This callback is not called if any of the following is true:
     * - an error occurred while processing the preview thumbnail
     * - a suitable existing thumbnail was not found
     * - a high quality thumbnail is already available
     *
     * The MThumbnailData instance is only valid for the duration of
     * this call. It may not be stored for later use. If the client should
     * want to use the bitmap data later, it can claim ownership of the
     * CFbsBitmap object by calling DetachBitmap().
     *
     * @since S60 v5.0
     * @param aThumbnail     An object representing the resulting thumbnail.
     * @param aId            Request ID for the operation
     */
    void ThumbnailPreviewReady( MThumbnailData& aThumbnail,
        TThumbnailRequestId aId );

    /**
     * Final thumbnail bitmap generation or loading is complete.
     *
     * The MThumbnailData instance is only valid for the duration of
     * this call. It may not be stored for later use. If the client should
     * want to use the bitmap data later, it can claim ownership of the
     * CFbsBitmap object by calling DetachBitmap().
     *
     * @since S60 v5.0
     * @param aError         Error code.
     * @param aThumbnail     An object representing the resulting thumbnail.
     * @param aId            Request ID for the operation.
     */
    void ThumbnailReady( TInt aError, MThumbnailData& aThumbnail,
        TThumbnailRequestId aId );
    

signals:

    /**
     * Final thumbnail bitmap generation or loading is complete.
     */
    void thumbnailReady( QPixmap , void * , int , int );

    void thumbnailReady( QImage , void * , int , int );

private:
    /**
     * Limit priority to presumed range.
     */
    int convertPriority(int basePriority);


public:
    
    int connectionCounterImage;

    int connectionCounterPixmap;
    
private:
    
    ThumbnailManager *q_ptr;

    CThumbnailManager* iThumbnailManager;
    
    QByteArray* byteArray;

    friend class ThumbnailManager;
    friend class TestThumbnailManager;
    
};

#endif // ThumbnailManager_H
