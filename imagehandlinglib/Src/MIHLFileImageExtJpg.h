/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Pure virtual interface for file based external jpg image class.
*              : This class acts as a window into single image or animation
*              : in one physical file. Note that file will be locked as long
*              : as instance of this class is present.
*
*/


#ifndef MIHLFILEIMAGEEXTJPG_H
#define MIHLFILEIMAGEEXTJPG_H

// INCLUDES
#include <MIHLFileImage.h>
#include <gdi.h>

// FORWARD DECLARATION
class MIHLBitmap;
class MIHLFilter;
class CVisualFrame;

// CLASS DECLARATION
/**
*  MIHLFileImageExtJpg
*
*  Pure virtual interface for file based image class that uses IclExtJpegApi.
*  This class acts as a window into single image or animation
*  in one physical file. Note that file will be locked as long
*  as instance of this class is present.
*
*  If interface needs to be pushed into CleanupStack,
*  remember to use CleanupDeletePushL() function!
*  DO NOT USE CleanupStack::PushL()!!
*
*  @lib IHL.lib
*  @since 3.0
*/
class MIHLFileImageExtJpg : public MIHLFileImage
    {
	public:

        /**
        * Virtual destructor.
		*/
		virtual ~MIHLFileImageExtJpg() {}

	public:
		/**
        * Load part of the image into bitmap.	
        * @since 3.0
        * @param aSrcRect Part of the picture to be loaded
        * @param aStatus Load request status reference.
		* @param aDestination Destination bitmap reference.
		* @param aFrameIndex frame index for loading single frame from image
		* @return Return system wide error codes:
		*         KerrArgument if given bitmap is not created or it's size is incorrect.
		*         KErrBusy image has load request pending already.
		*/
		virtual TInt Load( TRect aSrcRect, TRequestStatus& aStatus, MIHLBitmap& aDestination, TInt aFrameIndex = 0 ) = 0;
	
	public:	//CExtJpegDecoder wrapper, see IclExtJpegApi.h
	
		virtual void SetCroppingL( TRect aCropRect ) = 0;

        virtual void SetStreamingL( TSize& aMacroBlockSize ) = 0;

        virtual void SetRotationL( TInt aDegree ) = 0;

        virtual void SetFlippingL() = 0;

        virtual void SetMirroringL() = 0;
        
        virtual void SetDctDecodingL() = 0;
        
        virtual void ConvertL( 
                        TRequestStatus* aRequestStatus, 
                        const CVisualFrame* aDestinationFrame, 
                        TInt& aNoOfDecodedMBlocks,
                        TInt aFrameNumber = 0 ) = 0;

        virtual void ContinueConvertL( 
                        TRequestStatus* aRequestStatus, 
                        const CVisualFrame* aDestinationFrame, 
                        TInt& aNoOfDecodedMBlocks,
                        TInt aFrameNumber = 0 ) = 0;

        virtual TInt SupportedFormatsL() = 0;

        virtual TInt CapabilitiesL() = 0;


	};
#endif
