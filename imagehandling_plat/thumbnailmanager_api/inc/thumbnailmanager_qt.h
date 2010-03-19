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
* Description: Qt interface class to Thumbnail Manager
*
*/

#ifndef THUMBNAILMANAGER_QT_H
#define THUMBNAILMANAGER_QT_H

#include <qobject>
#include <QPixmap.h>

class ThumbnailManagerPrivate;
class QString;
class QSize;

/** default priority value */
const int tnmWrapperPriorityIdle = -100;

class ThumbnailManager : public QObject
    {
    Q_OBJECT

public:


    /** Thumbnail size. */
    enum ThumbnailSize
    {
        /**
         * Small thumbnail
         */
        ThumbnailSmall = 0, 
        /**
         * Medium thumbnail
         */
        ThumbnailMedium, 
        /**
         * Large thumbnail
         */
        ThumbnailLarge
    };
    
    /** Mode of thumbnail creation. */
    enum ThumbnailMode
        {
        /**
         * Default mode. This means that:
         * - Thumbnail must be as large as requested (unless the actual object is smaller).
         * - Smaller thumbnails may be up scaled to desired resolution.
         * - Aspect ratio is maintained and thumbnails are not cropped. The
         *   resulting thumbnail may smaller in either width or height if
         *   the aspect ratio of the object does not match the aspect ratio
         *   of the requested size.
         */
        Default = 0, 

        /**
         * Allow thumbnails which are smaller than requested are. Thumbnail
         * bitmaps are never up scaled if this flag is set.
         */
        AllowAnySize = 1, 

        /**
         * New thumbnail images are not created if this flag is set. Only
         * existing thumbnails may be returned. If a requested thumbnail does
         * not exist null pixmap will be returned.
         */
        DoNotCreate = 2, 

        /**
         * Thumbnail images are cropped to match requested aspect ratio. If
         * this mode is set, the size of the resulting thumbnail always
         * matches the requested size.
         */
        CropToAspectRatio = 4
    };
    
    /**  Quality versus speed preference setting */
    enum QualityPreference
    {
        /**
         * Prefer thumbnails in the highest quality possible disregarding
         * any negative impact on performance.
         */
        OptimizeForQuality, 

        /**
         * Get thumbnails as fast as possible, even if
         * it means lower quality.
         */
        OptimizeForPerformance
    };

    
    /**
     * Constructor
     * 
     * @param parentPtr parent
     */    
    IMPORT_C ThumbnailManager( QObject* parentPtr = NULL );

    /**
     * Destructor
     */
    IMPORT_C ~ThumbnailManager();

    /**
     * Get quality versus performance preference.
     *
     * @return quality versus performance preference
     */
    IMPORT_C QualityPreference qualityPreference() const;

    /**
     * Set quality versus performance preference.
     *
     * @param qualityPreference New quality versus performance preference
     *                           value.
     * @return true on success
     */
    IMPORT_C bool setQualityPreference( QualityPreference qualityPreference );

    /**
     * Get the current desired size for thumbnail bitmaps.
     *
     * @return Current desired size for thumbnail bitmaps (in pixels).
     */
    IMPORT_C QSize thumbnailSize() const;

    /**
     * Set desired size for thumbnail bitmaps.
     *
     * @param thumbnailSize New desired thumbnail size.
     * @return true on success
     */
    IMPORT_C bool setThumbnailSize( const QSize& thumbnailSize );

    /**
     * Set desired size for thumbnail bitmaps.
     *
     * @param thumbnailSize New desired thumbnail size.
     * @return true on success
     */
    IMPORT_C bool setThumbnailSize( ThumbnailSize thumbnailSize );
    
    /**
     * Get current mode for thumbnail generation.
     *
     * @return Current mode.
     */
    IMPORT_C ThumbnailMode mode() const;

    /**
     * Set mode for thumbnail generation.
     *
     * @param mode New flags.
     * @return true on success 
     */
    IMPORT_C bool setMode( ThumbnailMode mode );

    /**
     * Get a thumbnail for an object file. If a thumbnail already exists, it
     * is loaded and if a thumbnail does not exist, it is created
     * transparently. If thumbnail loadinf fails thumbnailReady signal is emited 
     * with null pixmap and error code.
     *
     * @param fileName      Source object or file
     * @param clientData    Pointer to arbitrary client data.
     *                      This pointer is not used by the API for
     *                      anything other than returning it in the
     *                      ThumbnailReady signal.
     * @param priority      Priority for this operation
     * @return              Thumbnail request ID or -1 if request failed. This can be used to
     *                      cancel the request or change priority.
     *                      The ID is specific to this tnm
     *                      instance and may not be shared with other
     *                      instances.
     */
    IMPORT_C int getThumbnail( const QString& fileName, void * clientData = NULL, 
            int priority = tnmWrapperPriorityIdle );

    /**
     * Get a persistent thumbnail for an object file. If a thumbnail already
     * exists, it is loaded and if a thumbnail does not exist, it is created
     * transparently. If thumbnail loading fails thumbnailReady signal is emited 
     * with null pixmap and error code.
     *
     * @param thumbnailId   Thumbnail ID
     * @param clientData    Pointer to arbitrary client data.
     *                      This pointer is not used by the API for
     *                      anything other than returning it in the
     *                      ThumbnailReady signal.
     * @param priority      Priority for this operation
     * @return              Thumbnail request ID or -1 if request failed. This can be used to
     *                      cancel the request or change priority.
     *                      The ID is specific to this tnm
     *                      instance and may not be shared with other
     *                      instances.
     */    
    IMPORT_C int getThumbnail( unsigned long int thumbnailId, void * clientData = NULL, 
            int priority = tnmWrapperPriorityIdle );
    
    /**
     * Set a thumbnail for an object file generated from pixmap delivered.
     * thumbnailReady() signal will be emited when the operation is complete. 
     * 
     * @param source             Pixmap from which the thumbnail will be created
     * @param fileName           file name
     * @param clientData         Pointer to arbitrary client data.
     *                           This pointer is not used by the API for
     *                           anything other than returning it in the
     *                           ThumbnailReady callback.
     * @param priority           Priority for this operation
     * @return                   Thumbnail request ID or -1 if request failed. This can be used to
     *                           cancel the request or change priority. 
     *                           
     */    
    IMPORT_C int setThumbnail( const QPixmap& source, const QString& fileName,
            void * clientData = NULL, int priority = tnmWrapperPriorityIdle );
    
    /**
     * Delete all thumbnails for a given object. This is an asynchronous
     * operation, which always returns immediately.
     *
     * @param fileName      Source file
     */
    IMPORT_C void deleteThumbnails( const QString& fileName );

    /**
     * Delete all thumbnails for a given object. This is an asynchronous
     * operation, which always returns immediately.
     *
     * @param thumbnailId      thumbnail id
     */
    IMPORT_C void deleteThumbnails( unsigned long int thumbnailId );

    /**
     * Cancel a thumbnail operation.
     *
     * @param id      Request ID for the operation to be cancelled.
     * @return         true if cancelling was successful.
     */
    IMPORT_C bool cancelRequest( int id );

    /**
     * Change the priority of a queued thumbnail operation.
     *
     * @param id           Request ID for the request which to assign a new
     *                      priority.
     * @param newPriority  New priority value
     * @return              true if change was successful.
     */
    IMPORT_C bool changePriority( int id, int newPriority );
    
signals:  
    /**
     * Final thumbnail bitmap generation or loading is complete.
     *
     * @param pixmap     An object representing the resulting thumbnail.
     * @param clientData Client data
     * @param id         Request ID for the operation
     * @param errorCode  error code
     */
    void thumbnailReady( QPixmap , void * , int , int );    
    
private:
    ThumbnailManagerPrivate* d;
};

#endif // THUMBNAILMANAGER_QT
