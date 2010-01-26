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
* Description:  This struct contain interface and
*              : implementation ids for IHL classes.
*
*/


#ifndef TIHLINTERFACETYPE_H
#define TIHLINTERFACETYPE_H

// INCLUDES
#include <e32std.h>

// CLASS DECLARATION
/**
*  TIHLInterfaceType
*
*  This struct contain interface and
*  implementation ids for IHL classes.
*
*  Used id values are defined in IHLInterfaceIds.h header.
*
*  @lib IHL.lib
*  @since 3.0
*/
class TIHLInterfaceType
	{
	public: // Constructor and destructor

		inline TIHLInterfaceType( TInt aInterfaceId, TInt aImplementationId );
		virtual ~TIHLInterfaceType() {};

	public: // Public data

		/**
		* Class interface id.
		* Used to identify interface type currently in use.
		*/
		TInt iInterfaceId;

		/**
		* Class implementation id.
		* Used to identify implementation class behind interface.
		* For internal use.
		*/
		TInt iImplementationId;

	};

inline TIHLInterfaceType::TIHLInterfaceType( TInt aInterfaceId, TInt aImplementationId )
:iInterfaceId( aInterfaceId ), iImplementationId( aImplementationId ) {}

#endif // TIHLINTERFACETYPE_H

// End of File
