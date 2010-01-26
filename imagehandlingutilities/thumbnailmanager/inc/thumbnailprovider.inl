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
* Description:  Thumbnail provider class inline functions
*
*/


#include <e32math.h>


inline TSize CThumbnailProvider::FitToSize( const TSize& aOrigSize,
                                         const TSize& aTargetSize,
                                         TBool aCrop )
    {
    TSize res;
    TReal32 srcAspect =
        static_cast<TReal32>( aOrigSize.iWidth ) / aOrigSize.iHeight;
    TReal32 reqAspect =
        static_cast<TReal32>( aTargetSize.iWidth ) / aTargetSize.iHeight;

    res = aTargetSize;

    // Check whether the width or the height is the limiting factor for
    // the size
    TBool widthLimited = srcAspect > reqAspect;
    if ( aCrop )
        {
        // When cropping, this works exactly the opposite way
        widthLimited = srcAspect < reqAspect;
        }

    if ( res.iWidth >= aOrigSize.iWidth &&
         res.iHeight >= aOrigSize.iHeight )
        {
        // Original is smaller than requested. No scaling needed.
        res = aOrigSize;
        }
    else if ( widthLimited )
        {
        // Width is the limiting factor, ie. the thumbnail is
        // wide compared to requested size.
        TReal trg;
        TReal src( aTargetSize.iWidth / srcAspect );
        Math::Round( trg, src, 1 );
        res.SetSize(
            aTargetSize.iWidth,
            trg );
        }
    else
        {
        // Height is the limiting factor, ie. the thumbnail is
        // taller compared to requested size.
        TReal trg;
        TReal src( aTargetSize.iHeight * srcAspect );
        Math::Round( trg, src, 1 );
        res.SetSize(
            trg,
            aTargetSize.iHeight );
        }
    return res;
    }


inline void CThumbnailProvider::ResolveSize()
    {
    // Get the actual target size for the thumbnail.
    // This is the maximum of the non-cropped and cropped
    // target sizes.

    TSize scaledSize = FitToSize( iOriginalSize, iTargetSize, EFalse );
    if ( iCroppedTargetSize.iHeight && iCroppedTargetSize.iWidth )
        {
        TSize croppedSize = FitToSize(
            iOriginalSize, iCroppedTargetSize, ETrue );
        scaledSize.iWidth = Max( scaledSize.iWidth, croppedSize.iWidth );
        scaledSize.iHeight = Max( scaledSize.iHeight, croppedSize.iHeight );
        }
    iTargetSize = scaledSize;
    TN_DEBUG3("CThumbnailProvider::ResolveSize() - iTargetSize=(%d, %d)",
                  iTargetSize.iWidth, iTargetSize.iHeight);
    }
