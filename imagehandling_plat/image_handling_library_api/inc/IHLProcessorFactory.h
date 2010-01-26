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
* Description:  Static factory class for creating processors.
*
*/


#ifndef IHLPROCESSORFACTORY_H
#define IHLPROCESSORFACTORY_H

// INCLUDES
#include <e32std.h>

// FORWARD DECLARATION
class MIHLBitmapProcessor;

// CLASS DECLARATION
/**
*  IHLProcessorFactory
*
*  Static factory class for creating processors.
*  @lib IHL.lib
*  @since 3.0
*/
class IHLProcessorFactory
    {
    public:

        /**
        * Create new MIHLBitmapProcessor instance.
        * @since 3.0
		* @param aOptions Special options for bitmap processing. Refer to MIHLBitmapProcessor header.
		* @return New MIHLBitmapProcessor instance.
		*/
        IMPORT_C static MIHLBitmapProcessor* CreateBitmapProcessorL( const TUint32 aOptions = 0 );
	};

#endif // IHLPROCESSORFACTORY_H

// End of File
