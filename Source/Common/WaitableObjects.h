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

#ifndef _NKT_WAITABLEOBJECTS_H
#define _NKT_WAITABLEOBJECTS_H

#include "Defines.h"
#include <intrin.h>
#include "Debug.h"

//-----------------------------------------------------------

#pragma intrinsic (_InterlockedExchange)
#pragma intrinsic (_InterlockedIncrement)
#pragma intrinsic (_InterlockedDecrement)
#pragma intrinsic (_InterlockedCompareExchange)

#define NktInterlockedExchange        _InterlockedExchange
#define NktInterlockedIncrement       _InterlockedIncrement
#define NktInterlockedDecrement       _InterlockedDecrement
#define NktInterlockedCompareExchange _InterlockedCompareExchange

_inline LONG NktInterlockedAnd(__inout LONG volatile *lpnDest, __in LONG nSet)
{
  LONG i, j;

  j = *lpnDest;
  do {
    i = j;
    j = NktInterlockedCompareExchange(lpnDest, i & nSet, i);
  }
  while (i != j);
  return j;
}

_inline LONG NktInterlockedOr(__inout LONG volatile *lpnDest, __in LONG nSet)
{
  LONG i, j;

  j = *lpnDest;
  do {
    i = j;
    j = NktInterlockedCompareExchange(lpnDest, i | nSet, i);
  }
  while (i != j);
  return j;
}

_inline LONG NktInterlockedXor(__inout LONG volatile *lpnDest, __in LONG nSet)
{
  LONG i, j;

  j = *lpnDest;
  do {
    i = j;
    j = NktInterlockedCompareExchange(lpnDest, i ^ nSet, i);
  }
  while (i != j);
  return j;
}

_inline VOID NktInterlockedBitSet(__inout LONG volatile *lpnDest, __in LONG nBit)
{
  LONG nValue = (LONG)(1UL << (nBit & 31));
  NktInterlockedOr(lpnDest+(nBit>>5), nValue);
  return;
}

_inline VOID NktInterlockedBitReset(__inout LONG volatile *lpnDest, __in LONG nBit)
{
  LONG nValue = (LONG)(1UL << (nBit & 31));
  NktInterlockedAnd(lpnDest+(nBit>>5), (~nValue));
  return;
}

_inline BOOL NktInterlockedBitTestAndSet(__inout LONG volatile *lpnDest, __in LONG nBit)
{
  LONG nValue = (LONG)(1UL << (nBit & 31));
  return ((NktInterlockedOr(lpnDest+(nBit>>5), nValue) & nValue) != 0) ? TRUE : FALSE;
}

_inline BOOL NktInterlockedBitTestAndReset(__inout LONG volatile *lpnDest, __in LONG nBit)
{
  LONG nValue = (LONG)(1UL << (nBit & 31));
  return ((NktInterlockedAnd(lpnDest+(nBit>>5), (~nValue)) & nValue) != 0) ? TRUE : FALSE;
}

_inline BOOL NktInterlockedBitTestAndComplement(__inout LONG volatile *lpnDest, __in LONG nBit)
{
  LONG nValue = (LONG)(1UL << (nBit & 31));
  return ((NktInterlockedXor(lpnDest+(nBit>>5), nValue) & nValue) != 0) ? TRUE : FALSE;
}

BOOL NktIsMultiProcessor();
VOID NktYieldProcessor();

//-----------------------------------------------------------

class CNktFastMutex
{
public:
  CNktFastMutex()
    {
    ::InitializeCriticalSection(&cs);
    return;
    };

  virtual ~CNktFastMutex()
    {
    ::DeleteCriticalSection(&cs);
    return;
    };

  _inline VOID Lock()
    {
    ::EnterCriticalSection(&cs);
    return;
    };

  _inline BOOL TryLock()
    {
    return ::TryEnterCriticalSection(&cs);
    };

  _inline VOID Unlock()
    {
    ::LeaveCriticalSection(&cs);
    return;
    };

private:
  CRITICAL_SECTION cs;
};

//-----------------------------------------------------------

class CNktAutoFastMutex
{
public:
  CNktAutoFastMutex(__inout CNktFastMutex *lpFastMutex, __in BOOL bTry=FALSE)
    {
    lpFM = lpFastMutex;
    _ASSERT(lpFM != NULL);
    if (bTry == FALSE)
    {
      lpFM->Lock();
      bLockHeld = TRUE;
    }
    else
    {
      bLockHeld = lpFM->TryLock();
    }
    return;
    };

  virtual ~CNktAutoFastMutex()
    {
    if (bLockHeld != FALSE)
      lpFM->Unlock();
    return;
    };

  BOOL IsLockHeld() const
    {
    return bLockHeld;
    };

private:
  CNktFastMutex *lpFM;
  BOOL bLockHeld;
};

class CNktAutoFastMutexReverse
{
public:
  CNktAutoFastMutexReverse(__inout CNktFastMutex *lpFastMutex)
  {
    lpFM = lpFastMutex;
    _ASSERT(lpFM != NULL);
    lpFM->Unlock();
    return;
  };

  virtual ~CNktAutoFastMutexReverse()
  {
    lpFM->Lock();
    return;
  };

private:
  CNktFastMutex *lpFM;
};

//-----------------------------------------------------------

class CNktEvent
{
public:
  CNktEvent();
  ~CNktEvent();

  BOOL Create(__in BOOL bManualReset, __in BOOL bInitialState, __in_z_opt LPCWSTR szName=NULL,
              __in_opt LPSECURITY_ATTRIBUTES lpSecAttr=NULL);
  BOOL Open(__in_z_opt LPCWSTR szName=NULL);
  VOID Destroy();

  VOID Attach(__in HANDLE hEvent);

  HANDLE GetEventHandle()
    {
    return hEvent;
    };

  BOOL Wait(__in DWORD dwTimeout);

  BOOL Reset();
  BOOL Set();
  BOOL Pulse();

  HANDLE Detach();

private:
  HANDLE hEvent;
};

//-----------------------------------------------------------

class CNktMutex
{
public:
  CNktMutex();
  ~CNktMutex();

  BOOL Create(__in_z_opt LPCWSTR szName=NULL, __out_opt LPBOOL lpbAlreadyExists=NULL,
              __in_opt LPSECURITY_ATTRIBUTES lpSecAttr=NULL);
  BOOL Open(__in_z_opt LPCWSTR szName=NULL, __in BOOL bQueryOnly=FALSE);
  VOID Destroy();

  VOID Attach(__in HANDLE hMutex);

  HANDLE GetMutexHandle()
    {
    return hMutex;
    };

  BOOL Lock(__in DWORD dwTimeout=INFINITE);
  BOOL Unlock();

private:
  HANDLE hMutex;
  LONG volatile nHeldCounter;
};

//-----------------------------------------------------------

class CNktAutoMutex
{
public:
  CNktAutoMutex(__in CNktMutex *_lpMtx, __in DWORD dwTimeOut=INFINITE)
    {
    lpMtx = _lpMtx;
    _ASSERT(lpMtx != NULL);
    bLockHeld = lpMtx->Lock(dwTimeOut);
    return;
    };

  ~CNktAutoMutex()
    {
    if (bLockHeld != FALSE)
      lpMtx->Unlock();
    return;
    };

  BOOL IsLockHeld() const
    {
    return bLockHeld;
    };

private:
  CNktMutex *lpMtx;
  BOOL bLockHeld;
};

//-----------------------------------------------------------

class CNktSimpleLockNonReentrant
{
public:
  CNktSimpleLockNonReentrant(__inout LONG volatile *_lpnLock)
    {
    lpnLock = _lpnLock;
    while (NktInterlockedCompareExchange(lpnLock, 1, 0) != 0)
      NktYieldProcessor();
    return;
    };

  ~CNktSimpleLockNonReentrant()
    {
    NktInterlockedExchange(lpnLock, 0);
    return;
    };

private:
  LONG volatile *lpnLock;
};

//-----------------------------------------------------------

class CNktFastReadWriteLock
{
public:
  CNktFastReadWriteLock()
    {
    NktInterlockedExchange(&nMutex, 0);
    NktInterlockedExchange(&nReaders, 0);
    NktInterlockedExchange(&nWriters, 0);
    dwWriterThreadId = 0;
    hWriteLockEvent = ::CreateEventW(NULL, TRUE, TRUE, NULL);
    return;
    };

  ~CNktFastReadWriteLock()
    {
    return;
    };

  BOOL TryAcquireExclusive()
    {
    CNktSimpleLockNonReentrant cLock(&nMutex);
    DWORD dwTid;
    BOOL b;

    b = FALSE;
    if (nReaders == 0)
    {
      dwTid = ::GetCurrentThreadId();
      if (dwWriterThreadId == 0 || dwWriterThreadId == dwTid)
      {
        NktInterlockedIncrement(&nWriters);
        dwWriterThreadId = dwTid;
        b = TRUE;
      }
    }
    return b;
    };

  VOID AcquireExclusive()
    {
    while (TryAcquireExclusive() == FALSE)
    {
      if (hWriteLockEvent != NULL)
        ::WaitForSingleObject(hWriteLockEvent, INFINITE);
      else
        NktYieldProcessor();
    }
    return;
    };

  BOOL TryAcquireShared()
    {
    CNktSimpleLockNonReentrant cLock(&nMutex);
    BOOL b;

    if (nWriters == 0)
    {
      if (NktInterlockedIncrement(&nReaders) == 1)
      {
        if (hWriteLockEvent != NULL)
          ::ResetEvent(hWriteLockEvent);
      }
      b = TRUE;
    }
    else
    {
      b = FALSE;
    }
    return b;
    };

  VOID AcquireShared()
    {
    while (TryAcquireShared() == FALSE)
      NktYieldProcessor();
    return;
    };

  VOID Release()
    {
    CNktSimpleLockNonReentrant cLock(&nMutex);

    _ASSERT(nWriters > 0 || nReaders > 0);
    if (nWriters > 0)
    {
      if (NktInterlockedDecrement(&nWriters) == 0)
        dwWriterThreadId = 0;
    }
    else if (nReaders > 0)
    {
      if (NktInterlockedDecrement(&nReaders) == 0)
      {
        if (hWriteLockEvent != NULL)
          ::SetEvent(hWriteLockEvent);
      }
    }
    else
    {
      _ASSERT(FALSE);
    }
    return;
    };

private:
  LONG volatile nMutex;
  LONG volatile nReaders;
  LONG volatile nWriters;
  DWORD volatile dwWriterThreadId;
  HANDLE hWriteLockEvent;
};

//-----------------------------------------------------------

class CNktAutoFastReadWriteLock
{
public:
  CNktAutoFastReadWriteLock(__in CNktFastReadWriteLock *lpRwLock, __in BOOL bShared, __in BOOL bTry=FALSE)
    {
    lpLock = lpRwLock;
    _ASSERT(lpLock != NULL);
    if (bShared != FALSE)
    {
      if (bTry == FALSE)
      {
        lpLock->AcquireShared();
        bLockHeld = TRUE;
      }
      else
        bLockHeld = lpLock->TryAcquireShared();
    }
    else
    {
      if (bTry == FALSE)
      {
        lpLock->AcquireExclusive();
        bLockHeld = TRUE;
      }
      else
      {
        bLockHeld = lpLock->TryAcquireExclusive();
      }
    }
    return;
    };

  virtual ~CNktAutoFastReadWriteLock()
    {
    if (bLockHeld != FALSE)
      lpLock->Release();
    return;
    };

  BOOL IsLockHeld() const
    {
    return bLockHeld;
    };

private:
  CNktFastReadWriteLock *lpLock;
  BOOL bLockHeld;
};

//-----------------------------------------------------------

VOID NktInitReaderWriterLock(LONG volatile *lpnValue);

BOOL NktTryAcquireShared(LONG volatile *lpnValue);
VOID NktAcquireShared(LONG volatile *lpnValue);
VOID NktReleaseShared(LONG volatile *lpnValue);

BOOL NktTryAcquireExclusive(LONG volatile *lpnValue);
VOID NktAcquireExclusive(LONG volatile *lpnValue);
VOID NktReleaseExclusive(LONG volatile *lpnValue);

//-----------------------------------------------------------

#endif //_NKT_WAITABLEOBJECTS_H
