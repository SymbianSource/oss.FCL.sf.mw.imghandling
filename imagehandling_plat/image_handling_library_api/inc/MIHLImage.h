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
* Description:  Pure virtual base class for all Image type interfaces.
*              : Class contains method to identify underlaying interface and
*              : implementation.
*
*/


#ifndef MIHLIMAGE_H
#define MIHLIMAGE_H

// INCLUDES
#include <TIHLInterfaceType.h>

// CLASS DECLARATION
/**
*  MIHLImage
*
*  This is pure virtual base class for all Image type interfaces.
*  Class contains method to identify underlaying interface and
*  implementation.
*
*  @lib IHL.lib
*  @since 3.0
*/
class MIHLImage
    {
    public:

        /**
        * Return identifier for underlaying interface
		* and implementation.
        * @since 3.0
		* @return Identifier for underlaying
		*         interface and implementation.
		*/
		virtual TIHLInterfaceType Type() const = 0;
		
    protected:
        // Derived class cannot be destructed through this interface
        ~MIHLImage() {}
	};

#endif   // MIHLIMAGE_H

// End of File
