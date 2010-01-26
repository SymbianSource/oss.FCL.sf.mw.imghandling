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

// INCLUDE FILES
#include <Stiftestinterface.h>
#include <SettingServerClient.h>

#include <PathInfo.h>

#include "ThumbnailManagerTest.h"

// ============================= LOCAL FUNCTIONS ===============================

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CThumbnailManagerTest::CThumbnailManagerTest
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CThumbnailManagerTest::CThumbnailManagerTest( CTestModuleIf& aTestModuleIf ):
    CScriptBase( aTestModuleIf )
    {
    }

// -----------------------------------------------------------------------------
// CThumbnailManagerTest::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CThumbnailManagerTest::ConstructL()
    {
    iLog = CStifLogger::NewL( KThumbnailManagerTestLogPath, 
                              KThumbnailManagerTestLogFile,
                              CStifLogger::ETxt,
                              CStifLogger::EFile,
                              EFalse );
    
    User::LeaveIfError( iFs.Connect());
    User::LeaveIfError( iFs.ShareProtected());
    
    // set test data path according to env
#if ( defined (__WINS__) || defined (__WINSCW__) )
    iDataPath.Copy( PathInfo::PhoneMemoryRootPath().Left( KPathPrefixLength ));
#else
    iDataPath.Copy( PathInfo::MemoryCardRootPath ().Left( KPathPrefixLength ));
#endif
    iDataPath.Append( KTMTestDataPath );
    iDataPath.ZeroTerminate();
    }

// -----------------------------------------------------------------------------
// CThumbnailManagerTest::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CThumbnailManagerTest* CThumbnailManagerTest::NewL( CTestModuleIf& aTestModuleIf )
    {
    CThumbnailManagerTest* self = new (ELeave) CThumbnailManagerTest( aTestModuleIf );

    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop();

    return self;
    }

// Destructor
CThumbnailManagerTest::~CThumbnailManagerTest()
    { 
    // Delete resources allocated from test methods
    Delete();

    // Delete logger
    delete iLog; 
    
    iFs.Close();
    }

// ========================== OTHER EXPORTED FUNCTIONS =========================

// -----------------------------------------------------------------------------
// LibEntryL is a polymorphic Dll entry point.
// Returns: CScriptBase: New CScriptBase derived object
// -----------------------------------------------------------------------------
//
EXPORT_C CScriptBase* LibEntryL( CTestModuleIf& aTestModuleIf ) // Backpointer to STIF
    {
    return ( CScriptBase* ) CThumbnailManagerTest::NewL( aTestModuleIf );
    }


//  End of File
