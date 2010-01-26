/*
* Copyright (c) 2004-2007 Nokia Corporation and/or its subsidiary(-ies). 
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


// INCLUDE FILES
#include <IHLViewerFactory.h>

#include "CIHLImageViewerExtJpg.h"
#include "MIHLFileImageExtJpg.h"
#include "IHLImplementationIds.h"

#include "CIHLImageViewer.h"
#include <MIHLImage.h>
#include <MIHLFileImage.h>
#include <IHLInterfaceIds.h>


// ============================ STATIC FUNCTIONS ===============================
// -----------------------------------------------------------------------------
// IHLViewerFactory::CreateImageViewerL
// -----------------------------------------------------------------------------
//
EXPORT_C MIHLImageViewer* IHLViewerFactory::CreateImageViewerL(
											const TSize& aViewerSize,
											MIHLImage& aSource,
											MIHLBitmap& aDestination,
											MIHLViewerObserver& aObserver,
											const TUint32 aOptions )
	{
	MIHLImageViewer* viewer = NULL;
	if( aSource.Type().iInterfaceId == KIHLInterfaceIdFileImage )
		{
		if( aSource.Type().iImplementationId == KIHLImplementationIdFileImageExtJpg )
			{
			viewer = CIHLImageViewerExtJpg::NewL( aViewerSize, static_cast< MIHLFileImageExtJpg& >( aSource ), // CSI: 35 #
													 aDestination, aObserver, aOptions );
			}
		else
			{
			viewer = CIHLImageViewer::NewL( aViewerSize, static_cast< MIHLFileImage& >( aSource ),  // CSI: 35 #
													aDestination, aObserver, aOptions );
			}
		}
	else
		{
		User::Leave( KErrArgument );
		}
	return viewer;
	}

//  End of File
