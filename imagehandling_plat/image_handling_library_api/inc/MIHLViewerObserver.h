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
* Description:  Pure virtual base class for viewer observer.
*
*/


#ifndef IHLVIEWEROBSERVER_H
#define IHLVIEWEROBSERVER_H

// INCLUDES
#include <e32std.h>

// CLASS DECLARATION
/**
*  MIHLViewerObserver
*
*  Pure virtual base class for viewer observer.
*  @lib IHL.lib
*  @since 3.0
*/
class MIHLViewerObserver
	{
	public:

        /**
        * This callback is currently not supported!
        *
        * Notifies client of viewer events.
		* Event codes are defined in viewer header.
		* Leave situation is handled in ViewerError() callback.
        * @since 3.0
        * @param aEvent Type of event.
        * @param aEventCode1 Event specific code 1.
        * @param aEventCode2 Event specific code 2.
		*/
		virtual void ViewerEventL( TInt /*aEvent*/, TInt /*aEventCode1*/, TInt /*aEventCode2*/ ) {}

		/**
        * Notifies client when viewer bitmap content is changed.
		* Leave situation is handled in ViewerError() callback.
        * @since 3.0
		*/
		virtual void ViewerBitmapChangedL() = 0;

        /**
        * Notifies client if error occurs in viewer.
        * @since 3.0
        * @param aError System wide error code.
		*/
		virtual void ViewerError( TInt aError ) = 0;

	};

#endif // IHLIMAGEVIEWEROBSERVER_H

// End of File
