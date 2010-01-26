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
* Description:  Static factory class for creating image instances.
*
*/


#ifndef IHLIMAGEFACTORY_H
#define IHLIMAGEFACTORY_H

// INCLUDES
#include <e32std.h>

// FORWARD DECLARATION
class RFs;
class RFile;
class MIHLFileImage;

// CLASS DECLARATION
/**
*  IHLImageFactory
*
*  Static factory class for creating file instances.
*  @lib IHL.lib
*  @since 3.0
*/
class IHLImageFactory
    {
    public:

        /**
        * Open file and create new image instance.
		* Default filemode is EFileShareReadersOnly and if opening fails, EFileShareAny is used.
		* Method leaves with appropriate error code if file could not be opened.
		* If file is multi-image, new image contains handle to the first image.
        *
		* Note! Method may leave with KErrBadHandle if RFs::ShareProtected() method is not called!
        *
        * @since 3.0
		* @param aFs Fileserver reference.
		* @param aFilename Full path of image file.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenFileImageL( RFs& aFs, const TDesC& aFilename );

        /**
        * Open file and create new image instance.
		* Default filemode is EFileShareReadersOnly and if opening fails, EFileShareAny is used.
		* Method leaves with appropriate error code if
		* file could not be opened or index is out of bounds.
        *
        * Note! Method may leave with KErrBadHandle if RFs::ShareProtected() method is not called!
        *
        * @since 3.0
		* @param aFs Fileserver reference.
		* @param aFilename Full path of image file.
		* @param aImageIndex Image index. This is used in multi-image
		*                    file to determine which image is opened.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenFileImageL( RFs& aFs, const TDesC& aFilename,
		                                               TInt aImageIndex );

        /**
        * Open file and create new image instance.
		* Default filemode is EFileShareReadersOnly and if opening fails, EFileShareAny is used.
		* Method leaves with appropriate error code if
		* file could not be opened or index is out of bounds.
        *
        * Note! Method may leave with KErrBadHandle if RFs::ShareProtected() method is not called!
        *
        * @since 3.0
		* @param aFs Fileserver reference.
		* @param aFilename Full path of image file.
		* @param aImageIndex Image index. This is used in multi-image
		*                    file to determine which image is opened.
		* @param aOptions Special opening options. Refer to MIHLFileImage header.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenFileImageL( RFs& aFs, const TDesC& aFilename,
		                                               TInt aImageIndex, const TUint32 aOptions );

        /**
        * Create new image instance.
		* Method leaves with appropriate error code if file type is unknown.
        * @since 3.0
		* @param aFile Open file handle.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenFileImageL( RFile& aFile );

        /**
        * Create new image instance.
		* Method leaves with appropriate error code if
		* file type is unknown or index is out of bounds.
        * @since 3.0
		* @param aFile Open file handle.
		* @param aImageIndex Image index. This is used in multi-image
		*                    file to determine which image is opened.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenFileImageL( RFile& aFile, TInt aImageIndex );

        /**
        * Create new image instance.
		* Method leaves with appropriate error code if
		* file type is unknown or index is out of bounds.
        * @since 3.0
		* @param aFile Open file handle.
		* @param aImageIndex Image index. This is used in multi-image
		*                    file to determine which image is opened.
		* @param aOptions Special opening options. Refer to MIHLFileImage header.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenFileImageL( RFile& aFile, TInt aImageIndex,
		                                               const TUint32 aOptions );

        /**
        * Create new image instance from buffered image data.
		* Method leaves with appropriate error code if buffer data could not be used.
		* If buffer contains multi-image, created image instance contains handle to
		* the first image.
        * @since 3.0
		* @param aFs Fileserver reference.
		* @param aDataBuf Buffer reference.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenBufferedFileImageL( RFs& aFs, const TDesC8& aDataBuf );

        /**
        * Create new image instance from buffered image data.
		* Method leaves with appropriate error code if buffer data could not be used
		* or image index is out of bounds. If buffer contains multi-image, created
		* image instance contains handle to the first image.
        * @since 3.0
		* @param aFs Fileserver reference.
		* @param aDataBuf Buffer reference.
		* @param aImageIndex Image index. This is used in multi-image
		*                    file to determine which image is opened.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenBufferedFileImageL( RFs& aFs, const TDesC8& aDataBuf,
		                                                       TInt aImageIndex );

        /**
        * Create new image instance from buffered image data.
		* Method leaves with appropriate error code if buffer data could not be used
		* or image index is out of bounds. If buffer contains multi-image, created
		* image instance contains handle to the first image.
        * @since 3.0
		* @param aFs Fileserver reference.
		* @param aDataBuf Buffer reference.
		* @param aImageIndex Image index. This is used in multi-image
		*                    file to determine which image is opened.
		* @param aOptions Special opening options. Refer to MIHLFileImage header.
		* @return New image instance.
		*/
		IMPORT_C static MIHLFileImage* OpenBufferedFileImageL( RFs& aFs, const TDesC8& aDataBuf,
		                                               TInt aImageIndex, const TUint32 aOptions );

	};

#endif   // IHLIMAGEFACTORY_H

// End of File
