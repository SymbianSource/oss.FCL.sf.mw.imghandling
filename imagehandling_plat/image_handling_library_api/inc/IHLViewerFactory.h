/*
* Copyright (c) 2004 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Static factory class for creating viewers.
*
*/


#ifndef IHLVIEWERFACTORY_H
#define IHLVIEWERFACTORY_H

// INCLUDES
#include <e32std.h>

// FORWARD DECLARATION
class MIHLImageViewer;
class MIHLViewerObserver;
class MIHLImage;
class MIHLBitmap;

// CLASS DECLARATION
/**
*  IHLViewerFactory
*
*  Static factory class for creating viewers.
*  @lib IHL.lib
*  @since 3.0
*/
class IHLViewerFactory
	{
	public:

        /**
        * Create new MIHLImageViewer instance.
		* References to image, bitmap and observer must remain valid
		* through viewer lifespan. Note that viewer start processing
		* immediately after it's created.
        * @since 3.0
		* @param aViewerSize Size of viewer. This is also maximum
		*                    size of destination bitmap.
		* @param aSource Source image reference.
		* @param aDestination Destination bitmap reference.
		* @param aObserver Viewer observer reference.
		* @param aOptions Viewer options. Refer to MIHLImageViewer header.
		* @return New MIHLImageViewer instance.
		*/
		IMPORT_C static MIHLImageViewer* CreateImageViewerL( const TSize& aViewerSize,
															 MIHLImage& aSource,
															 MIHLBitmap& aDestination,
															 MIHLViewerObserver& aObserver,
															 const TUint32 aOptions = 0 );
	};

#endif // IHLVIEWERFACTORY_H

// End of File
