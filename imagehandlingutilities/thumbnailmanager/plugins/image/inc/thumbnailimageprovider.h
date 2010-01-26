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
* Description:  Image thumbnail provider plugin.
 *
*/


#ifndef THUMBNAILIMAGEPROVIDER_H
#define THUMBNAILIMAGEPROVIDER_H

#include "thumbnailprovider.h"

class CThumbnailImageDecoder;
class CThumbnailImageDecoderv2;

/**
 *  Image thumbnail provider plugin.
 *
 *  @since S60 v5.0
 */
class CThumbnailImageProvider: public CThumbnailProvider
    {

public:

    /**
     * Two-phased constructor.
     *
     * @since S60 v5.0
     * @return New image thumbnail provider instance.
     */
    static CThumbnailImageProvider* NewL();

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailImageProvider();

    /**
     * Function that will provide thumbnail from given parameters.
     *
     * @since S60 v5.0
     * @param aFs File server session
     * @param aFile Source image file.
     * @param aMimeType MIME-type of the image file.
     * @param aFlags Flags for thumbnail creation.
     * @param aDisplayMode Used display mode.
     */
    void GetThumbnailL( RFs& aFs, RFile64& aFile, const TDataType& aMimeType,
        const CThumbnailManager::TThumbnailFlags aFlags, const TDisplayMode
        aDisplayMode, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference  );
    
    /**
     * Function that will provide thumbnail from given parameters.
     *
     * @since S60 v5.0
     * @param aFs File server session
     * @param aBuffer Source image buffer.
     * @param aMimeType MIME-type of the image file.
     * @param aFlags Flags for thumbnail creation.
     * @param aDisplayMode Used display mode.
     */    
    void GetThumbnailL( RFs& aFs, TDesC8* aBuffer, const
        TDataType& aMimeType, const CThumbnailManager::TThumbnailFlags aFlags,
        const TDisplayMode aDisplayMode, const CThumbnailManager::TThumbnailQualityPreference aQualityPreference  );
    
    /**
     * Function that will provide thumbnail from given parameters.
     *
     * @since S60 v5.0
     * @param aFs File server session
     * @param aBuffer Source image buffer.
     */ 
    void GetThumbnailL( RFs& aFs, TDesC8& aBuffer);

    /**
     * Cancel a previous thumbnail request, if any.
     *
     * @since S60 5.0
     */
    void CancelGetThumbnail();

private:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @return New image thumbnail provider instance.
     */
    CThumbnailImageProvider();

private:

    /**
     * Thumbnail decoder.
     */
    CThumbnailImageDecoder* iImageDecoder;

    /**
     * Thumbnail decoderv2.
    */
    CThumbnailImageDecoderv2* iImageDecoderv2;
    
   /**
     * File server session.
     * Not own.
     */
    RFs iFs;

    /**
     * Mime-type of image
     */
    TDataType iMimeType;
};

#include "thumbnailprovider.inl"

#endif // THUMBNAILIMAGEPROVIDER_H
