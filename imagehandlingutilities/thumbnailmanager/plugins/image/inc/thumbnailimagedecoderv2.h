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
* Description:  Image thumbnail decoderv2.
 *
*/


#ifndef THUMBNAILIMAGEDECODERV2_H
#define THUMBNAILIMAGEDECODERV2_H

#include "thumbnailprovider.h"

class CImageDecoder;
class CFbsBitmap;

/**
 *  Image thumbnail decoder.
 *
 *  @since S60 v5.0
 */
class CThumbnailImageDecoderv2: public CActive
    {
public:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aFs File server session
     * @return New image thumbnail decoder instance.
     */
    CThumbnailImageDecoderv2( RFs& aFs);

    /**
     * Destructor
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailImageDecoderv2();

    /**
     * Creates a decoder from the given image buffer and gets image
     * information
     *
     * @since S60 v5.0
     * @param aBuffer Source image buffer.
     * @param aObserver Observer to relay notifications about completed
     *                  operations.
     */    
    void CreateL(TDesC8& aBuffer, MThumbnailProviderObserver& aObserver);    
    
    /**
     * Decode image.
     *
     * @since S60 v5.0
     * @param aDisplayMode Preferred display mode.
     */
    void DecodeL();

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
     * Creates image decoder to be used.
     *
     * @since S60 v5.0
     */
    void CreateDecoderL();
    
     /**
     * Leave if image is corrupt
     *
     * @since S60 v5.0
     */
    void LeaveIfCorruptL(const TInt aError );

private:
    // data

   /**
     * Image decoder.
     * Own.
     */
    CImageDecoder* iDecoder;

    /**
     * Original size of the image in pixels.
     */
    TSize iOriginalSize;
    
    /**
     * Decoded bitmap.
     * Own.
     */
    CFbsBitmap* iBitmap;

   /**
     * Used fileserver.
     */
    RFs& iFs;

    /**
     * Buffer from which thumbnail is to be created.
     */    
    TDesC8* iBuffer;

    /**
     * Observer.
     */
    MThumbnailProviderObserver* iObserver; // not own
};

#endif // THUMBNAILIMAGEDECODERV2_H
