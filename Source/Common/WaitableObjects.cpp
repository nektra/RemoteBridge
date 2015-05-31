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

#include "WaitableObjects.h"

//-----------------------------------------------------------

static LONG volatile nProcessorsCount = 0;

//-----------------------------------------------------------

BOOL NktIsMultiProcessor()
{
  if (nProcessorsCount == 0)
  {
    SYSTEM_INFO sInfo;
    LONG nTemp;

    ::GetSystemInfo(&sInfo);
    nTemp = (sInfo.dwNumberOfProcessors > 1) ? (LONG)sInfo.dwNumberOfProcessors : 1;
    NktInterlockedExchange(&nProcessorsCount, nTemp);
    return (nTemp > 1) ? TRUE : FALSE;
  }
  return (nProcessorsCount > 1) ? TRUE : FALSE;
}

VOID NktYieldProcessor()
{
  if (NktIsMultiProcessor() == FALSE)
    ::SwitchToThread();
  else
    ::Sleep(1);
  return;
}

//-----------------------------------------------------------

CNktEvent::CNktEvent()
{
  hEvent = NULL;
  return;
}

CNktEvent::~CNktEvent()
{
  Destroy();
  return;
}

BOOL CNktEvent::Create(__in BOOL bManualReset, __in BOOL bInitialState, __in_z_opt LPCWSTR szName,
                       __in_opt LPSECURITY_ATTRIBUTES lpSecAttr)
{
  Destroy();
  hEvent = ::CreateEventW(lpSecAttr, bManualReset, bInitialState, szName);
  if (hEvent == NULL)
    return FALSE;
  return TRUE;
}

BOOL CNktEvent::Open(__in_z_opt LPCWSTR szName)
{
  Destroy();
  hEvent = ::OpenEventW(EVENT_ALL_ACCESS, FALSE, szName);
  if (hEvent == NULL)
    return FALSE;
  return TRUE;
}

VOID CNktEvent::Destroy()
{
  if (hEvent != NULL)
  {
    __try
    {
      ::CloseHandle(hEvent);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    { }
    hEvent = NULL;
  }
  return;
}

VOID CNktEvent::Attach(__in HANDLE _hEvent)
{
  Destroy();
  hEvent = (_hEvent != INVALID_HANDLE_VALUE) ? _hEvent : NULL;
  return;
}

BOOL CNktEvent::Wait(__in DWORD dwTimeout)
{
  DWORD dwRetCode;

  if (hEvent == NULL)
    return TRUE;
  dwRetCode = ::WaitForSingleObject(hEvent, dwTimeout);
  return (dwRetCode == WAIT_OBJECT_0) ? TRUE : FALSE;
}

BOOL CNktEvent::Reset()
{
  if (hEvent == NULL)
    return FALSE;
  if (::ResetEvent(hEvent) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL CNktEvent::Set()
{
  if (hEvent == NULL)
    return FALSE;
  if (::SetEvent(hEvent) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL CNktEvent::Pulse()
{
  if (hEvent == NULL)
    return FALSE;
  if (::PulseEvent(hEvent) == FALSE)
    return FALSE;
  return TRUE;
}

HANDLE CNktEvent::Detach()
{
  HANDLE hTempEvent = hEvent;
  hEvent = NULL;
  return hTempEvent;
}

//-----------------------------------------------------------

CNktMutex::CNktMutex()
{
  hMutex = NULL;
  nHeldCounter = 0;
  return;
}

CNktMutex::~CNktMutex()
{
  Destroy();
  return;
}

BOOL CNktMutex::Create(__in_z_opt LPCWSTR szName, __out_opt LPBOOL lpbAlreadyExists,
                       __in_opt LPSECURITY_ATTRIBUTES lpSecAttr)
{
  Destroy();
  if (lpbAlreadyExists != NULL)
    *lpbAlreadyExists = FALSE;
  ::SetLastError(NO_ERROR);
  hMutex = ::CreateMutexW(lpSecAttr, FALSE, szName);
  if (hMutex == NULL)
    return FALSE;
  if (lpbAlreadyExists != NULL && ::GetLastError() == ERROR_ALREADY_EXISTS)
    *lpbAlreadyExists = TRUE;
  return TRUE;
}

BOOL CNktMutex::Open(__in_z_opt LPCWSTR szName, __in BOOL bQueryOnly)
{
  Destroy();
  hMutex = ::OpenMutexW((bQueryOnly == FALSE) ? MUTEX_ALL_ACCESS : (STANDARD_RIGHTS_READ|SYNCHRONIZE), FALSE, szName);
  if (hMutex == NULL)
    return FALSE;
  return TRUE;
}

VOID CNktMutex::Destroy()
{
  if (hMutex != NULL)
  {
    while (nHeldCounter > 0)
      Unlock();
    __try
    {
      ::CloseHandle(hMutex);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    { }
    hMutex = NULL;
  }
  nHeldCounter = 0;
  return;
}

VOID CNktMutex::Attach(__in HANDLE _hMutex)
{
  Destroy();
  hMutex = (_hMutex != INVALID_HANDLE_VALUE) ? _hMutex : NULL;
  return;
}

BOOL CNktMutex::Lock(__in DWORD dwTimeout)
{
  DWORD dwRetCode;

  if (hMutex == NULL)
    return FALSE;
  if (NktInterlockedIncrement(&nHeldCounter) == 1)
  {
    dwRetCode = ::WaitForSingleObject(hMutex, dwTimeout);
    if (dwRetCode != WAIT_OBJECT_0 && dwRetCode != WAIT_ABANDONED_0)
    {
      NktInterlockedDecrement(&nHeldCounter);
      return FALSE;
    }
  }
  return TRUE;
}

BOOL CNktMutex::Unlock()
{
  if (hMutex == NULL)
    return FALSE;
  _ASSERT(nHeldCounter > 0);
  if (NktInterlockedDecrement(&nHeldCounter) == 0)
  {
    if (::ReleaseMutex(hMutex) == FALSE)
    {
      NktInterlockedIncrement(&nHeldCounter);
      return FALSE;
    }
  }
  return TRUE;
}

//-----------------------------------------------------------

VOID NktInitReaderWriterLock(LONG volatile *lpnValue)
{
  NktInterlockedExchange(lpnValue, 0);
  return;
}

BOOL NktTryAcquireShared(LONG volatile *lpnValue)
{
  LONG initVal;

  do
  {
    initVal = *lpnValue;
    if (initVal == 0x80000000L)
      return FALSE; //a writer is active
  }
  while (NktInterlockedCompareExchange(lpnValue, initVal+1, initVal) != initVal);
  return TRUE;
}

VOID NktAcquireShared(LONG volatile *lpnValue)
{
  while (NktTryAcquireShared(lpnValue) == FALSE)
    NktYieldProcessor();
  return;
}

VOID NktReleaseShared(LONG volatile *lpnValue)
{
  LONG initVal, newVal;

  do
  {
    initVal = *lpnValue;
    newVal = (initVal & 0x80000000L) | ((initVal & 0x7FFFFFFFL) - 1);
  }
  while (NktInterlockedCompareExchange(lpnValue, newVal, initVal) != initVal);
  return;
}

BOOL NktTryAcquireExclusive(LONG volatile *lpnValue)
{
  LONG initVal;

  do
  {
    initVal = *lpnValue;
    if ((initVal & 0x80000000L) != 0)
      return FALSE; //another writer is active or waiting
  }
  while (NktInterlockedCompareExchange(lpnValue, initVal | 0x80000000L, initVal) != initVal);
  //wait until no readers
  while ((*lpnValue & 0x7FFFFFFFL) != 0)
    NktYieldProcessor();
  return TRUE;
}

VOID NktAcquireExclusive(LONG volatile *lpnValue)
{
  while (NktTryAcquireExclusive(lpnValue) == FALSE)
    NktYieldProcessor();
  return;
}

VOID NktReleaseExclusive(LONG volatile *lpnValue)
{
  NktInterlockedExchange(lpnValue, 0);
  return;
}
