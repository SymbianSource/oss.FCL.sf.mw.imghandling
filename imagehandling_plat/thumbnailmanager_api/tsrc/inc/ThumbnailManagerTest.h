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
* Description:  Thumbnail Manager API tests
*
*/

#ifndef THUMBNAILMANAGERTEST_H
#define THUMBNAILMANAGERTEST_H

//  INCLUDES
#include <StifLogger.h>
#include <TestScripterInternal.h>
#include <StifTestModule.h>

#include <thumbnailmanager.h>
#include <thumbnailmanagerobserver.h>
#include <thumbnailobjectsource.h>
#include <thumbnaildata.h>

#include "thumbnailcenrep.h"

const TInt KPathPrefixLength( 2 );

// MACROS
#if ( defined (__WINS__) || defined (__WINSCW__) )
_LIT( KTMTestDataPath, "" );
#else 
_LIT( KTMTestDataPath, "\\testing" );
#endif

// Logging path
_LIT( KThumbnailManagerTestLogPath, "\\logs\\testframework\\ThumbnailManagerTest\\" ); 
// Log file
_LIT( KThumbnailManagerTestLogFile, "ThumbnailManagerTest.txt" ); 
_LIT( KThumbnailManagerTestLogFileWithTitle, "ThumbnailManagerTest_[%S].txt" );

// FORWARD DECLARATIONS
class CThumbnailManagerTest;

// CLASS DECLARATION

/**
*  CThumbnailManagerTest test class for STIF Test Framework TestScripter.
*  ?other_description_lines
*
*  @lib ?library
*  @since ?Series60_version
*/
NONSHARABLE_CLASS(CThumbnailManagerTest): public CScriptBase,
                                          public MThumbnailManagerObserver
    {
    public:  // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        static CThumbnailManagerTest* NewL( CTestModuleIf& aTestModuleIf );

        /**
        * Destructor.
        */
        virtual ~CThumbnailManagerTest();

    public: // Functions from base classes

        /**
        * From CScriptBase Runs a script line.
        * @since ?Series60_version
        * @param aItem Script line containing method name and parameters
        * @return Symbian OS error code
        */
        virtual TInt RunMethodL( CStifItemParser& aItem );

    protected:  // Functions from base classes

        /**
         * From MThumbnailManagerObserver
         */
        void ThumbnailPreviewReady( MThumbnailData& aThumbnail, TThumbnailRequestId aId );

        void ThumbnailReady( TInt aError, MThumbnailData& aThumbnail, TThumbnailRequestId aId );

    private:

        /**
        * C++ default constructor.
        */
        CThumbnailManagerTest( CTestModuleIf& aTestModuleIf );

        /**
        * By default Symbian 2nd phase constructor is private.
        */
        void ConstructL();

        /**
        * Frees all resources allocated from test methods.
        * @since ?Series60_version
        */
        void Delete();

        /**
        * Test methods are listed below. 
        */

        virtual TInt CreateInstanceL( CStifItemParser& aItem );
        virtual TInt DeleteInstance( CStifItemParser& aItem );
        
        virtual TInt CreateSourceInstancePathL( CStifItemParser& aItem );
        virtual TInt CreateSourceInstancePathTargetL( CStifItemParser& aItem );
        virtual TInt CreateSourceInstanceHandleL( CStifItemParser& aItem );
        virtual TInt CreateSourceInstanceBufferL( CStifItemParser& aItem );
        virtual TInt CreateSourceInstanceBitmapL( CStifItemParser& aItem );
        virtual TInt DeleteSourceInstance( CStifItemParser& aItem );

        virtual TInt GetThumbnailL( CStifItemParser& aItem );
        virtual TInt GetThumbnailByIdL( CStifItemParser& aItem );
        
        virtual TInt SetThumbnailL( CStifItemParser& aItem );
        virtual TInt ImportThumbnailL( CStifItemParser& aItem );
        
        virtual TInt CheckPreviewThumbnailL( CStifItemParser& aItem );
        virtual TInt CheckThumbnailL( CStifItemParser& aItem );
        virtual TInt CheckThumbnailCenrepL( CStifItemParser& aItem );

        virtual TInt CancelPrevious( CStifItemParser& aItem );
        virtual TInt ChangePreviousPriority( CStifItemParser& aItem );

        virtual TInt SetThumbnailSizeL( CStifItemParser& aItem );
        virtual TInt CheckThumbnailSizeL( CStifItemParser& aItem );
        virtual TInt SetThumbnailEnumSizeL( CStifItemParser& aItem );
        virtual TInt SetFlagsL( CStifItemParser& aItem );
        virtual TInt CheckFlags( CStifItemParser& aItem );
        virtual TInt SetQualityPreferenceL( CStifItemParser& aItem );
        virtual TInt CheckQualityPreference( CStifItemParser& aItem );
        virtual TInt SetDisplayModeL( CStifItemParser& aItem );
        virtual TInt CheckDisplayModeL( CStifItemParser& aItem );

        virtual TInt DeleteThumbnails( CStifItemParser& aItem );
        virtual TInt CreateThumbnailsByUrlL( CStifItemParser& aItem );
        
        virtual TInt CreateThumbnails( CStifItemParser& aItem );
        virtual TInt UpdatePathL( CStifItemParser& aItem );
        
        virtual TInt GetSupportedMimeTypesL( CStifItemParser& aItem );
        
        /**
         * Method used to log version of test class
         */
        void SendTestClassVersion();

    private:    // Data
        
        // Own. Thumbnail engine
        CThumbnailManager* iEngine;

        // Own. Thumbnail object source
        CThumbnailObjectSource* iObjectSource;

        // Own. File server session
        RFs iFs;

        // Own.
        RFile64 iFile;

        // Own. Bitmap from previous ThumbnailPreviewReady() callback
        CFbsBitmap* iPreviewThumbnail;

        // Own. Bitmap from previous ThumbnailReady() callback
        CFbsBitmap* iThumbnail;

        // Value for aId for previous ThumbnailPreviewReady() callback
        TThumbnailRequestId iPreviewThumbnailCbId;

        // Value for aId for previous ThumbnailReady() callback
        TThumbnailRequestId iThumbnailCbId;

        // Request ID of the previous request
        TThumbnailRequestId iPreviousRequestId;
        
        // Test data path
        TFileName iDataPath;
        
        // Temp buffer
        HBufC8* iBuffer;
        
        // Temp bitmap
        CFbsBitmap* iBitmap;
        
        // Own. Central repository data handler
        CThumbnailCenRep* iCenrep;
        
        // persistent sizes
        RArray < TThumbnailPersistentSize > iPersistentSizes;
        
        TThumbnailSize iCurrentThumbnailSize;
        
        TBool iDoNotCreate;
        TBool iCreateThumbnails;
    };

#endif      // THUMBNAILMANAGERTEST_H

// End of File
