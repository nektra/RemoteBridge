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

#include <Windows.h>
#include <stdio.h>
#include "Debug.h"
#include "WaitableObjects.h"

//-----------------------------------------------------------

#ifndef NKT_ARRAYLEN
  #define NKT_ARRAYLEN(x)            (sizeof(x)/sizeof(x[0]))
#endif //NKT_ARRAYLEN

#define OUTPUT_BUFFER_SIZE                              4096

#define NKT_FORCE_OUTPUTDEBUG_ON_XP

//-----------------------------------------------------------

static LONG volatile nAccessMtx = 0;
static LONG volatile nMultiPrintMtx = 0;

//-----------------------------------------------------------

static VOID SendDebugOutput(__in LPCSTR szBufA);

//-----------------------------------------------------------

namespace Nektra
{

#if (defined(_DEBUG) || defined(NKT_RELEASE_DEBUG_MACROS) || defined(NKT_FUNCTION_TIMING_ENABLED))
  eDebugLevel nDbgLevel = dlDebug;
#else //_DEBUG || NKT_RELEASE_DEBUG_MACROS || NKT_FUNCTION_TIMING_ENABLED
  eDebugLevel nDbgLevel = dlError;
#endif //_DEBUG || NKT_RELEASE_DEBUG_MACROS || NKT_FUNCTION_TIMING_ENABLED

// nDbgLevel
VOID SetDebugLevel(__in eDebugLevel nNewDebugLevel)
{
  nDbgLevel = nNewDebugLevel;
  return;
}

eDebugLevel GetDebugLevel()
{
  return nDbgLevel;
}

// DebugPrint
VOID DebugPrintA(__in eDebugLevel nDebugLevel, __in_z LPCSTR szFormatA, ...)
{
  va_list ap;

  va_start(ap, szFormatA);
  DebugPrintV_A(nDebugLevel, szFormatA, ap);
  va_end(ap);
  return;
}

VOID DebugPrintV_A(__in eDebugLevel nDebugLevel, __in_z LPCSTR szFormatA, va_list ap)
{
  if ((int)nDbgLevel >= (int)nDebugLevel)
  {
    LPSTR szBufA = (LPSTR)::LocalAlloc(LMEM_FIXED, OUTPUT_BUFFER_SIZE);
    int i;
    if (szBufA != NULL)
    {
      i = vsnprintf_s(szBufA, OUTPUT_BUFFER_SIZE, _TRUNCATE, szFormatA, ap);
      if (i < 0)
        i = 0;
      else if (i > OUTPUT_BUFFER_SIZE-1)
        i = OUTPUT_BUFFER_SIZE-1;
      szBufA[i] = 0;
      SendDebugOutput(szBufA);
      ::LocalFree((HLOCAL)szBufA);
    }
  }
  return;
}

VOID DebugPrintLnA(__in eDebugLevel nDebugLevel, __in_z LPCSTR szFormatA, ...)
{
  va_list ap;

  va_start(ap, szFormatA);
  DebugPrintLnV_A(nDebugLevel, szFormatA, ap);
  va_end(ap);
  return;
}

VOID DebugPrintLnV_A(__in eDebugLevel nDebugLevel, __in_z LPCSTR szFormatA, va_list ap)
{
  if ((int)nDbgLevel >= (int)nDebugLevel)
  {
    LPSTR szBufA = (LPSTR)::LocalAlloc(LMEM_FIXED, OUTPUT_BUFFER_SIZE);
    int i;
    if (szBufA != NULL)
    {
      i = vsnprintf_s(szBufA, OUTPUT_BUFFER_SIZE, _TRUNCATE, szFormatA, ap);
      if (i < 0)
        i = 0;
      else if (i > OUTPUT_BUFFER_SIZE-2)
        i = OUTPUT_BUFFER_SIZE-2;
      szBufA[i++] = '\n';
      szBufA[i] = 0;
      SendDebugOutput(szBufA);
      ::LocalFree((HLOCAL)szBufA);
    }
  }
  return;
}

VOID DebugPrintMultiStart()
{
  while (NktInterlockedCompareExchange(&nMultiPrintMtx, 1, 0) != 0)
    NktYieldProcessor();
  return;
}

VOID DebugPrintMultiEnd()
{
  NktInterlockedExchange(&nMultiPrintMtx, 0);
  return;
}

VOID DebugPrintSeparator()
{
  NKT_DEBUGPRINTA((Nektra::dlDebug, "----------------------------------------------------------------"));
  return;
}

} //namespace

//-----------------------------------------------------------

#ifdef NKT_FUNCTION_TIMING_ENABLED

CNktFunctionTimingDebug::CNktFunctionTimingDebug(__in_z LPCSTR _szNameA)
{
  szNameA = _szNameA;
  if (szNameA == NULL)
    szNameA = "";
  Nektra::DebugPrintLnA(Nektra::dlDebug, "%s: Enter\n", szNameA);
  if (::QueryPerformanceFrequency(&liFreq) == FALSE ||
      ::QueryPerformanceCounter(&liStart) == FALSE)
  {
    liFreq.QuadPart = 0;
    dwEnterTime = ::GetTickCount();
  }
  return;
}

CNktFunctionTimingDebug::~CNktFunctionTimingDebug()
{
  double nDbl;
  DWORD dw;

  if (liFreq.QuadPart != 0)
  {
    ::QueryPerformanceCounter(&liEnd);
    if (liEnd.QuadPart >= liStart.QuadPart)
      liEnd.QuadPart -= liStart.QuadPart;
    else
      liEnd.QuadPart = (~liStart.QuadPart)+liEnd.QuadPart+1ui64;
    nDbl = double(liEnd.QuadPart) / (double(liFreq.QuadPart) / 1000.0);
  }
  else
  {
    dw = ::GetTickCount();
    if (dw >= dwEnterTime)
      dw -= dwEnterTime;
    else
      dw = (~dwEnterTime)+dw+1UL;
    nDbl = (double)dw;
  }
  Nektra::DebugPrintLnA(Nektra::dlDebug, "%s Leave [Elapsed %.3fms]\n", szNameA, (float)nDbl);
  return;
}
#endif //NKT_FUNCTION_TIMING_ENABLED

//-----------------------------------------------------------

static VOID SendDebugOutput(__in LPCSTR szBufA)
{
#ifdef NKT_LOG_DEBUG_TO_FILE
  CNktSimpleLockNonReentrant cAccessLock(&nAccessMtx);
  WCHAR szFileNameW[64];
  FILE *fp;

  swprintf_s(szFileNameW, 64, L"C:\\RemoteBridge_%lu.log", ::GetCurrentProcessId());
  if (_wfopen_s(&fp, szFileNameW, L"a+t") == 0)
  {
    fprintf_s(fp, "%s", szBufA);
    fclose(fp);
  }
#else //NKT_LOG_DEBUG_TO_FILE
#ifndef NKT_FORCE_OUTPUTDEBUG_ON_XP
  if (LOBYTE(LOWORD(::GetVersion())) >= 6)
  {
#endif //NKT_FORCE_OUTPUTDEBUG_ON_XP
    //because OutputDebugStringA problems with Windows 2003, we only enable it on Vista or later
    CNktSimpleLockNonReentrant cAccessLock(&nAccessMtx);

    ::OutputDebugStringA(szBufA);
#ifndef NKT_FORCE_OUTPUTDEBUG_ON_XP
  }
#endif //NKT_FORCE_OUTPUTDEBUG_ON_XP
#endif //NKT_LOG_DEBUG_TO_FILE
  return;
}
