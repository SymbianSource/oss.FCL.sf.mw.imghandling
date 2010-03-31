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
* Description:  Task for decoding  a thumbnail
 *
*/


#ifndef THUMBNAILDECODETASK_H
#define THUMBNAILDECODETASK_H

#include <apmstd.h>

#include "thumbnailtask.h"
#include "thumbnailprovider.h"


class CThumbnailServer;

/**
 *  Task for decoding a thumbnail.
 *
 *  @since S60 v5.0
 */
class CThumbnailDecodeTask: public CThumbnailTask,
    public MThumbnailProviderObserver
    {
public:

    /**
     * C++ default constructor
     *
     * @since S60 v5.0
     * @param aProcessor Processor object for running tasks.
     * @param aServer Thumbnail server in use.
     * @param aBuffer Image in JPEG mode.
     * @param aPriority Priority of the request.
     * @return New CThumbnailDecodeTask object.
     */
    CThumbnailDecodeTask( CThumbnailTaskProcessor& aProcessor, CThumbnailServer& aServer, TDesC8* aBuffer,
            TInt aPriority, TDisplayMode aDisplayMode);

    /**
     * Destructor.
     *
     * @since S60 v5.0
     */
    virtual ~CThumbnailDecodeTask();

    /**
     * StartL
     *
     * @since S60 v5.0
     */
    void StartL();

    /**
     * Handles an active object's request completion event.
     *
     * @since S60 v5.0
     */
    void RunL();

    /**
     * Implements cancellation of an outstanding request.
     *
     * @since S60 v5.0
     */
    void DoCancel();

private:

   /**
     * For notifying when task has been finished.
     *
     * @since S60 v5.0
     * @param aError Error code.
     * @param aBitmap Thumbnail image.
     * @param aOriginalSize Original image size.
     */
    void ThumbnailProviderReady( const TInt aError, CFbsBitmap* aBitmap, const
        TSize& aOriginalSize, const TBool aEXIF, const TBool aPortait );


private:
    // Data

    /**
     * Server.
     */
    CThumbnailServer& iServer;
  
    /**
     * Image buffer
     */    
    TDesC8* iBuffer;

    /**
     * Size of the original image.
     */
    TSize iOriginalSize;

    /**
     * Thumbnail provider implementation.
     * Not own.
     */
    CThumbnailProvider* iProvider;
   
   /**
    * Temporary buffer for client/server parameters
    */
    TThumbnailRequestParamsPckgBuf iParamsBuf;
    
    /*
     * Displaymode of requested TN
     */
    TDisplayMode iDisplayMode;
    
};

#endif // THUMBNAILGENERATETASK_H
