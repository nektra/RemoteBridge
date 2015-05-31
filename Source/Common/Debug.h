/*
 * Copyright (C) 2010-2015 Nektra S.A., Buenos Aires, Argentina.
 * All rights reserved. Contact: http://www.nektra.com
 *
 *
 * This file is part of Remote Bridge
 *
 *
 * Commercial License Usage
 * ------------------------
 * Licensees holding valid commercial Remote Bridge licenses may use this
 * file in accordance with the commercial license agreement provided with
 * the Software or, alternatively, in accordance with the terms contained
 * in a written agreement between you and Nektra.  For licensing terms and
 * conditions see http://www.nektra.com/licensing/. Use the contact form
 * at http://www.nektra.com/contact/ for further information.
 *
 *
 * GNU General Public License Usage
 * --------------------------------
 * Alternatively, this file may be used under the terms of the GNU General
 * Public License version 3.0 as published by the Free Software Foundation
 * and appearing in the file LICENSE.GPL included in the packaging of this
 * file.  Please visit http://www.gnu.org/copyleft/gpl.html and review the
 * information to ensure the GNU General Public License version 3.0
 * requirements will be met.
 *
 **/

#ifndef _NKT_DEBUG_H
#define _NKT_DEBUG_H

#include <windows.h>
#include <time.h>
#include <WTypes.h>
#include <crtdbg.h>

//#define NKT_RELEASE_DEBUG_MACROS
//#define NKT_FUNCTION_TIMING_ENABLED
//#define NKT_LOG_DEBUG_TO_FILE

//-----------------------------------------------------------

//NOTE: Wide functions were removed because, internally, it transforms wide char to ansi
//      and some times crashes.

namespace Nektra
{

typedef enum {
  dlError = 0,
  dlWarning = 1,
  dlDebug = 2
} eDebugLevel;

#define DebugPrintErrorA(szFormat, ...) DebugPrintLnA(NKTError, szFormatA, __VA_ARGS__)

/**
Set new debug level
*/
VOID SetDebugLevel(__in eDebugLevel nNewDebugLevel);
eDebugLevel GetDebugLevel();

/**
Print formatted message.
*/
VOID DebugPrintA(__in eDebugLevel nDebugLevel, __in_z LPCSTR szFormatA, ...);
VOID DebugPrintV_A(__in eDebugLevel nDebugLevel, __in_z LPCSTR szFormatA, __in va_list ap);

/**
Print formatted message line.
*/
VOID DebugPrintLnA(__in eDebugLevel nDebugLevel, __in_z LPCSTR szFormatA, ...);
VOID DebugPrintLnV_A(__in eDebugLevel nDebugLevel, __in_z LPCSTR szFormatA, __in va_list ap);

VOID DebugPrintMultiStart();
VOID DebugPrintMultiEnd();

VOID DebugPrintSeparator();

/**
Break.
*/
__inline VOID DebugBreak()
{
  __debugbreak();
  return;
}

} //namespace

//-----------------------------------------------------------

#if (defined(_DEBUG) || defined(NKT_RELEASE_DEBUG_MACROS))
  #define NKT_INITDEBUGPRINTLEVEL() Nektra::SetDebugLevel(Nektra::dlDebug)
  #define NKT_DEBUGPRINTA(expr)     Nektra::DebugPrintLnA expr
#else //_DEBUG || NKT_RELEASE_DEBUG_MACROS
  #define NKT_INITDEBUGPRINTLEVEL() ((void)0)
  #define NKT_DEBUGPRINTA(expr)     ((void)0)
#endif //_DEBUG || NKT_RELEASE_DEBUG_MACROS

//-----------------------------------------------------------

#ifdef NKT_FUNCTION_TIMING_ENABLED
  #define NKT_FUNCTION_TIMING(fnc) CNktFunctionTimingDebug _cFuncTiming(fnc);

  class CNktFunctionTimingDebug
  {
  public:
    CNktFunctionTimingDebug(__in_z LPCSTR szNameA);
    ~CNktFunctionTimingDebug();

  private:
    LPCSTR szNameA;
    LARGE_INTEGER liStart, liEnd, liFreq;
    DWORD dwEnterTime;
  };

#else //NKT_FUNCTION_TIMING_ENABLED
  #define NKT_FUNCTION_TIMING(fnc)
#endif //NKT_FUNCTION_TIMING_ENABLED

//-----------------------------------------------------------

#endif //_NKT_DEBUG_H
