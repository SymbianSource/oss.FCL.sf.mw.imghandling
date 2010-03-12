/*
* Copyright (c) 2006-2007 Nokia Corporation and/or its subsidiary(-ies). 
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
* Description:  Logging functions
 *
*/


#ifndef THUMBNAILLOG_H
#define THUMBNAILLOG_H

#include <e32std.h>
#include <e32svr.h>

//log to file instead of RDebug
//#define LOG_TO_FILE

//#define _DEBUG

#ifdef _DEBUG
#ifdef LOG_TO_FILE
//
// Log to file
//
#include <flogger.h>
_LIT( KThumbnailLogDir, "thumbnail" );
_LIT( KThumbnailLogFile, "thumbnail.txt" );

#define TN_DEBUG1( s ) \
RFileLogger::Write( \
KThumbnailLogDir, \
KThumbnailLogFile, \
EFileLoggingModeAppend, \
_L( s ) );

#define TN_DEBUG2( s, a ) \
RFileLogger::WriteFormat( \
KThumbnailLogDir, \
KThumbnailLogFile, \
EFileLoggingModeAppend, \
_L( s ), a );

#define TN_DEBUG3( s, a, b ) \
RFileLogger::WriteFormat( \
KThumbnailLogDir, \
KThumbnailLogFile, \
EFileLoggingModeAppend, \
_L( s ), a, b );

#define TN_DEBUG4( s, a, b, c ) \
RFileLogger::WriteFormat( \
KThumbnailLogDir, \
KThumbnailLogFile, \
EFileLoggingModeAppend, \
_L( s ), a, b, c );

#define TN_DEBUG5( s, a, b, c, d ) \
RFileLogger::WriteFormat( \
KThumbnailLogDir, \
KThumbnailLogFile, \
EFileLoggingModeAppend, \
_L( s ), a, b, c, d );

#define TN_DEBUG6( s, a, b, c, d, e ) \
RFileLogger::WriteFormat( \
KThumbnailLogDir, \
KThumbnailLogFile, \
EFileLoggingModeAppend, \
_L( s ), a, b, c, d, e );

#define TN_DEBUG7( s, a, b, c, d, e, f ) \
RFileLogger::WriteFormat( \
KThumbnailLogDir, \
KThumbnailLogFile, \
EFileLoggingModeAppend, \
_L( s ), a, b, c, d, e, f );

#else // LOG_TO_FILE
//
// Log to RDebug
//
#define TN_DEBUG1( s )                 RDebug::Print( _L( s ) )
#define TN_DEBUG2( s, a )              RDebug::Print( _L( s ), a )
#define TN_DEBUG3( s, a, b )           RDebug::Print( _L( s ), a, b )
#define TN_DEBUG4( s, a, b, c )        RDebug::Print( _L( s ), a, b, c )
#define TN_DEBUG5( s, a, b, c, d )     RDebug::Print( _L( s ), a, b, c, d )
#define TN_DEBUG6( s, a, b, c, d, e )  RDebug::Print( _L( s ), a, b, c, d, e )
#define TN_DEBUG7( s, a, b, c, d, e, f )  RDebug::Print( _L( s ), a, b, c, d, e, f )

#endif // LOG_TO_FILE

#else // _DEBUG
//
// No logging
//
#define TN_DEBUG1( s )
#define TN_DEBUG2( s, a )
#define TN_DEBUG3( s, a, b )
#define TN_DEBUG4( s, a, b, c )
#define TN_DEBUG5( s, a, b, c, d )
#define TN_DEBUG6( s, a, b, c, d, e )
#define TN_DEBUG7( s, a, b, c, d, e, f )
#endif // _DEBUG

#endif // THUMBNAILLOG_H
