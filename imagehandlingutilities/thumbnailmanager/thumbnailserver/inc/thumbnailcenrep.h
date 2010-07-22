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
* Description:  Class for handling central repositoty data
 *
*/


#ifndef THUMBNAILCENREP_H
#define THUMBNAILCENREP_H

#include <gdi.h>

#include <thumbnailmanager.h> // TThumbnailSize

class CRepository;
class TThumbnailPersistentSize;
class TThumbnailAutoCreate;

/**
 *  Class for handling central repositoty data.
 *
 *  @since S60 v5.0
 */
class CThumbnailCenRep: public CBase
    {

public:

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailCenRep();

    /**
     * Two-phased constructor
     *
     * @since S60 v5.0
     * @return New CThumbnailCenRep instance.
     */
    static CThumbnailCenRep* NewL();

    /**
     * GetPersistentSizes
     *
     * @since S60 v5.0
     * @return Reference to array of central repository settings
     */
    RArray < TThumbnailPersistentSize > & GetPersistentSizes();
    
    /**
     * GetAutoCreateParams
     *
     * @since S60 v5.0
     * @return Reference to auto creation settings
     */
    TThumbnailAutoCreate & GetAutoCreateParams();
 
     /**
     * Get concrete persistent size associated to relative size
     *
     * @since S60 v5.0
     * @param aThumbnailSize Relative size
     * @return Persistent size object
     */   
    TThumbnailPersistentSize & PersistentSizeL( TThumbnailSize
            aThumbnailSize );

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New CThumbnailCenRep instance.
     */
    CThumbnailCenRep();

    /**
     * ConstructL
     *
     * @since S60 v5.0
     */
    void ConstructL();

private:
    // data

    /**
     * Central Repository
     * Own.
     */
    CRepository* iRepository;

    /**
     * Persistent sizes.
     */
    RArray < TThumbnailPersistentSize > iPersistentSizes;
    
    /**
     * Auto creation parameters.
     */
    TThumbnailAutoCreate * iAutoCreate;

};

/**
 *  Class for representing persistent thumbnail sizes and related
 *  parameters.
 *
 *  @since S60 v5.0
 */
class TThumbnailPersistentSize
    {
    /**
     * Format used for storing bitmaps.
     * @since S60 v5.0
     */
    enum TThumbnailImageFormat
        {
        /**
         * Use JPEG compression for stored bitmaps.
         */
        EJpeg, 

        /**
         * Use native Symbian bitmap format for stored bitmaps.
         */
        EBmp
    };
    
public:
    enum TThumbnailSourceType { EUnknownSourceType, EImage, EVideo, EAudio, EContact };
    enum TThumbnailSizeType { EUnknownSizeType, EGrid, EList, EFullscreen }; 
    
public:
    
    /**
     * C++ constructor.
     *
     * @since S60 v5.0
     * @param aSize Thumbnail size (in pixels) which will be stored.
     * @param aCrop If true, stored thumbnails are cropped to aspect ratio.
     * @param aMode Display mode used for stored bitmaps
     * @param aFormat Format used when storing thumbnails
     */
    TThumbnailPersistentSize( const TSize& aSize, TBool aCrop, TDisplayMode
                              aMode, TInt aFormat );
    
    TThumbnailPersistentSize( TThumbnailSize aType, const TSize& aSize, TBool aCrop, TDisplayMode
                              aMode, TInt aFormat, TBool aAutoCreate, TThumbnailSizeType aSizeType  );
    
public:
    
    // data
    TThumbnailSize iType;
    TThumbnailSourceType iSourceType;
    TSize iSize;
    TBool iCrop;
    TDisplayMode iMode;
    TInt iFormat;
    TBool iAutoCreate;
    TThumbnailSizeType iSizeType;
};

/**
 *  Class for representing thumbnail auto creation parameters.
 *
 *  @since S60 v5.0
 */
class TThumbnailAutoCreate
    {
    
public:
    
    /**
     * C++ constructor.
     *
     * @since S60 v5.0
     */
    TThumbnailAutoCreate();
    
public:
    
    // data
    TBool iImageGrid;
    TBool iImageList;
    TBool iImageFullscreen;
    TBool iVideoGrid;
    TBool iVideoList;
    TBool iVideoFullscreen;
    TBool iAudioGrid;
    TBool iAudioList;
    TBool iAudioFullscreen;
    TBool iContactGrid;
    TBool iContactList;
    TBool iContactFullscreen; 
};

#endif // THUMBNAILCENREP_H
