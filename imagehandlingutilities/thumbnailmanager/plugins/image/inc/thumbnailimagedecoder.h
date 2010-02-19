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
* Description:  Image thumbnail decoder.
 *
*/


#ifndef THUMBNAILIMAGEDECODER_H
#define THUMBNAILIMAGEDECODER_H

#include "thumbnailprovider.h"

class CImageDecoder;
class CFbsBitmap;

// Size of buffer to load from file start to get EXIF thumbnail
const TInt KJpegLoadBufferSize = 64 * 1024;


/**
 *  Image thumbnail decoder.
 *
 *  @since S60 v5.0
 */
class CThumbnailImageDecoder: public CActive
    {
public:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aFs File server session
     * @return New image thumbnail decoder instance.
     */
    CThumbnailImageDecoder( RFs& aFs );

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailImageDecoder();

    /**
     * Creates a decoder from the given image file and gets image
     * information
     *
     * @since S60 v5.0
     * @param aFile Source image file.
     * @param aObserver Observer to relay notifications about completed
     *                  operations.
     * @param aFlags Decoding option flags.
     * @param aMimeType Mime-type of image to be decoded.
     * @param aSize desired size of thumbnail.
     * @param aRotateIfNeeded If ETrue rectangle is rotated to maximize the decoded size (full screen images only)
     */
    void CreateL( RFile64& aFile, MThumbnailProviderObserver& aObserver, const
        CThumbnailManager::TThumbnailQualityPreference aFlags, const TDataType& aMimeType,
        const TSize& aSize );

    /**
     * Creates a decoder from the given image buffer and gets image
     * information
     *
     * @since S60 v5.0
     * @param aBuffer Source image buffer.
     * @param aObserver Observer to relay notifications about completed
     *                  operations.
     * @param aFlags Decoding option flags.
     * @param aMimeType Mime-type of image to be decoded.
     * @param aSize desired size of thumbnail.
     * @param aRotateIfNeeded If ETrue rectangle is rotated to maximize the decoded size (full screen images only)
     */    
    void CreateL( const TDesC8* aBuffer, MThumbnailProviderObserver&
        aObserver, const CThumbnailManager::TThumbnailQualityPreference aFlags, const
        TDataType& aMimeType, const TSize& aSize);    
    
    /**
     * Decode image.
     *
     * @since S60 v5.0
     * @param aDisplayMode Preferred display mode.
     */
    void DecodeL( const TDisplayMode aDisplayMode, const CThumbnailManager::TThumbnailFlags aFlags );

    /**
     * Returns the size of original image.
     *
     * @since S60 v5.0
     * @return Size of original image.
     */
    const TSize& OriginalSize()const;
	
     /**
     * Leave if image is corrupt
     *
     * @since S60 v5.0
     */
    void LeaveIfCorruptL(const TInt aError );

private:
    /**
     * Used to release reserved resources.
     *
     * @since S60 v5.0
     */
    void Release();

    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    void DoCancel();

    /**
     * Handles an active object's request completion event.
     *
     * @since S60 v5.0
     */
    void RunL();

    /**
     * Used to check if source is jpg/jpeg image.
     *
     * @since S60 v5.0
     * @return ETrue if image was jpg/jpeg format, otherwise EFalse
     */
    TBool IsJpeg();

    /**
     * Used to check if source is svg image.
     *
     * @since S60 v5.0
     * @return ETrue if image was in svg format, otherwise EFalse
     */
    TBool IsSvg();

    /**
     * Creates image decoder to be used.
     *
     * @since S60 v5.0
     */
    void CreateDecoderL( CThumbnailManager::TThumbnailQualityPreference aFlags );

    /**
     * Creates ExifDecoder.
     *
     * @since S60 v5.0
     */
    void CreateExifDecoderL( CThumbnailManager::TThumbnailQualityPreference aFlags );

private:
    // data

    /**
     * Requested size.
     */
    TSize iSize;

    /**
     * Image decoder.
     * Own.
     */
    CImageDecoder* iDecoder;

    /**
     * Decoded bitmap.
     * Own.
     */
    CFbsBitmap* iBitmap;

    /**
     * Image info flags (from TFrameInfo)
     */
    TUint32 iFrameInfoFlags;

    /**
     * Original size of the image in pixels.
     */
    TSize iOriginalSize;

    /**
     * Buffer for reading the beginning of a JPEG file to get the
     * EXIF thumbnail data.
     */
    HBufC8* iJpegReadBuffer;

    /**
     * EXIF thumbnail image compressed as JPEG
     */
    HBufC8* iExifThumbImage;

    /**
     * Used fileserver.
     */
    RFs& iFs;

    /**
     * File from which thumbnail is to be created.
     */
    RFile64 iFile;

    /**
     * Buffer from which thumbnail is to be created.
     */    
    const TDesC8* iBuffer;

    /**
     * Observer.
     */
    MThumbnailProviderObserver* iObserver; // not own

    /**
     * Mime-type.
     */
    TDataType iMimeType;

     /**
     * Is origin EXIF
     */
    TBool iEXIF;
    
    /**
     * Portrait image
     */    
    TBool iPortrait;
};

#endif // THUMBNAILIMAGEDECODER_H
