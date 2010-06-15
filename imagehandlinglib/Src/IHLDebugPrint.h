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
* Description:  IHL debug macros
*
*/


#ifndef IHLDEBUGPRINT_H
#define IHLDEBUGPRINT_H

// Debug logging is enabled, you may enable debug printing in release builds also
#ifdef IHL_ENABLE_DEBUG_PRINT

    // warn if this is a release build!!
    #ifndef __WINSCW__
    #ifndef _DEBUG
        #if defined(__VC32__)
            #pragma message( "Warning: IHL debug printing is on in release build!" ) // CSI: 68 #
        #else // __GCC32__
            #warning "IHL debug printing is on in release build!"
        #endif // __VC32__
    #endif
    #endif // __WINSCW__

// no include files if no debug printing --> faster compile time
// INCLUDES
#include <e32std.h>
#include <e32svr.h>

    /**
     * Helper macro for defining debug strings with just the debug text.
     */
    #define IHL_DEBUG_STRING(name,s) _LIT(name,s)

    /**
     * Output to debugger output
     */
    #define IHL_DEBUG_PRINT RDebug::Print

#else   // IHL_ENABLE_DEBUG_PRINT

    /**
     * Dummy struct for checking that all IHL_DEBUG_PRINT's define string
     * literals using space-saving IHL_DEBUG_STRING.
     */
    struct TIHLEmptyDebugString { }; // CSI: 80 #

    /**
     * Macro for defining debug-only literal strings (empty release version)
     */
    #define IHL_DEBUG_STRING(name, s) const TIHLEmptyDebugString name = {}

    /**
     * Output to debugger output (empty)
     */
    #define IHL_DEBUG_PRINT IHL_EmptyPrint

    namespace {

    /// Empty debug print function for release builds.
    inline void IHL_EmptyPrint(TIHLEmptyDebugString)
        {
        }

    /// Empty debug print function for release builds.
    template<class T1>
    inline void IHL_EmptyPrint(TIHLEmptyDebugString,T1)
        {
        }

    /// Empty debug print function for release builds.
    template<class T1,class T2>
    inline void IHL_EmptyPrint(TIHLEmptyDebugString,T1,T2)
        {
        }

    /// Empty debug print function for release builds.
    template<class T1,class T2,class T3>
    inline void IHL_EmptyPrint(TIHLEmptyDebugString,T1,T2,T3)
        {
        }

    /// Empty debug print function for release builds.
    template<class T1,class T2,class T3,class T4>
    inline void IHL_EmptyPrint(TIHLEmptyDebugString,T1,T2,T3,T4)
        {
        }

    /// Empty debug print function for release builds.
    template<class T1,class T2,class T3,class T4,class T5>
    inline void IHL_EmptyPrint(TIHLEmptyDebugString,T1,T2,T3,T4,T5)
        {
        }

    /// Empty debug print function for release builds.
    template<class T1,class T2,class T3,class T4,class T5,class T6>
    inline void IHL_EmptyPrint(TIHLEmptyDebugString,T1,T2,T3,T4,T5,T6)
        {
        }

    /// Empty debug print function for release builds.
    template<class T1,class T2,class T3,class T4,class T5,class T6,class T7>
    inline void IHL_EmptyPrint(TIHLEmptyDebugString,T1,T2,T3,T4,T5,T6,T7)
        {
        }

    /// Empty debug print function for release builds.
    template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8>
    inline void IHL_EmptyPrint(TIHLEmptyDebugString,T1,T2,T3,T4,T5,T6,T7,T8)
        {
        }

    } // unnamed namespace


#endif  // IHL_ENABLE_DEBUG_PRINT

/**
 * Short variant for easier basic usage.
 */
#define IHL_DEBUG1( name, s ) IHL_DEBUG_STRING( name, s ); IHL_DEBUG_PRINT( name );

#ifdef IHL_ENABLE_DEBUG_PRINT
#define IHL_DEBUG( s ) RDebug::Print( _L( s ) )
#define IHL_DEBUG2( name, s, i ) IHL_DEBUG_STRING( name, s ); IHL_DEBUG_PRINT( name, i );
#define IHL_DEBUG3( name, s, i, j ) IHL_DEBUG_STRING( name, s ); IHL_DEBUG_PRINT( name, i, j );
#define IHL_DEBUG4( name, s, i, j, k ) IHL_DEBUG_STRING( name, s ); IHL_DEBUG_PRINT( name, i, j, k );
#define IHL_DEBUG5( name, s, i, j, k, l ) IHL_DEBUG_STRING( name, s ); IHL_DEBUG_PRINT( name, i, j, k, l );
#else
// If debug printing is not enabled, strip other parameters than the string
// away to prevent unwanted function calls in release build.
#define IHL_DEBUG( s )
#define IHL_DEBUG2( name, s, i ) IHL_DEBUG1( name, s )
#define IHL_DEBUG3( name, s, i, j ) IHL_DEBUG1( name, s )
#define IHL_DEBUG4( name, s, i, j, k ) IHL_DEBUG1( name, s )
#define IHL_DEBUG5( name, s, i, j, k, l ) IHL_DEBUG1( name, s )
#endif

#endif //  __IHLDEBUGPRINT_H__
