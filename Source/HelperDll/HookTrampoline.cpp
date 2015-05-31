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

#include "HookTrampoline.h"
#include <crtdbg.h>
#include "..\Common\WaitableObjects.h"

//-----------------------------------------------------------

static VOID __stdcall MyNktAcquireShared(LONG volatile *lpnValue);
static VOID __stdcall MyNktReleaseShared(LONG volatile *lpnValue);
static VOID __stdcall MyNktAcquireExclusive(LONG volatile *lpnValue);
static VOID __stdcall MyNktReleaseExclusive(LONG volatile *lpnValue);

//-----------------------------------------------------------

LPBYTE HookTrampoline_Create(__in HANDLE hHeap, __in LPBYTE lpCode, __in SIZE_T nCodeSize, __in LPVOID lpCtx,
                             __in LPVOID lpFunc)
{
  LPBYTE lpTramp;
  SIZE_T i;

  _ASSERT(hHeap != NULL);
  _ASSERT(lpCode != NULL);
#if defined _M_IX86
  _ASSERT(nCodeSize >= 4);
#elif defined _M_X64
  _ASSERT(nCodeSize >= 8);
#endif
  _ASSERT(lpCtx != NULL);
  _ASSERT(lpFunc != NULL);
  lpTramp = (LPBYTE)::HeapAlloc(hHeap, 0, nCodeSize);
  if (lpTramp != NULL)
  {
    memcpy(lpTramp, lpCode, nCodeSize);
#if defined _M_IX86
    for (i=0; i<nCodeSize-4; i++)
    {
      switch (*((DWORD*)(lpTramp+i)))
      {
        case 0xDEAD0001:
          *((DWORD NKT_UNALIGNED *)(lpTramp+i)) = (DWORD)lpCtx;
          break;
        case 0xDEAD0002:
          *((DWORD NKT_UNALIGNED *)(lpTramp+i)) = (DWORD)lpFunc;
          break;
      }
    }
#elif defined _M_X64
    for (i=0; i<nCodeSize-8; i++)
    {
      switch (*((ULONGLONG __unaligned *)(lpTramp+i)))
      {
        case 0xDEAD0001DEAD0001:
          *((ULONGLONG NKT_UNALIGNED *)(lpTramp+i)) = (ULONGLONG)lpCtx;
          break;
        case 0xDEAD0002DEAD0002:
          *((ULONGLONG NKT_UNALIGNED *)(lpTramp+i)) = (ULONGLONG)lpFunc;
          break;
      }
    }
#endif
  }
  return lpTramp;
}

VOID HookTrampoline_Delete(__in HANDLE hHeap, __in LPVOID lpTrampoline)
{
  if (hHeap != NULL && lpTrampoline != NULL)
  {
    LPBYTE lpTramp = (LPBYTE)lpTrampoline;

    ::HeapFree(hHeap, 0, lpTramp);
  }
  return;
}

//-----------------------------------------------------------

LPBYTE HookTrampoline2_Create(__in HANDLE hHeap, __in LPBYTE lpCode, __in SIZE_T nCodeSize, __in LPVOID lpCtx,
                             __in LPVOID lpFunc, __in BOOL bUseShared)
{
  LPBYTE lpTramp;

  _ASSERT(hHeap != NULL);
  _ASSERT(lpCode != NULL);
#if defined _M_IX86
  _ASSERT(nCodeSize >= 4);
#elif defined _M_X64
  _ASSERT(nCodeSize >= 8);
#endif
  _ASSERT(lpCtx != NULL);
  _ASSERT(lpFunc != NULL);
  lpTramp = (LPBYTE)::HeapAlloc(hHeap, 0, nCodeSize);
  if (lpTramp != NULL)
  {
    memcpy(lpTramp, lpCode, nCodeSize);
    *((SIZE_T NKT_UNALIGNED *)(lpTramp+1*sizeof(SIZE_T))) = (SIZE_T)lpFunc;
    if (bUseShared != FALSE)
    {
      *((SIZE_T NKT_UNALIGNED *)(lpTramp+3*sizeof(SIZE_T))) = (SIZE_T)MyNktAcquireShared;
      *((SIZE_T NKT_UNALIGNED *)(lpTramp+4*sizeof(SIZE_T))) = (SIZE_T)MyNktReleaseShared;
    }
    else
    {
      *((SIZE_T NKT_UNALIGNED *)(lpTramp+3*sizeof(SIZE_T))) = (SIZE_T)MyNktAcquireExclusive;
      *((SIZE_T NKT_UNALIGNED *)(lpTramp+4*sizeof(SIZE_T))) = (SIZE_T)MyNktReleaseExclusive;
    }
    *((SIZE_T NKT_UNALIGNED *)(lpTramp+5*sizeof(SIZE_T))) = (SIZE_T)lpCtx;
    lpTramp += 6*sizeof(SIZE_T);
  }
  return lpTramp;
}

VOID HookTrampoline2_SetCallOriginalProc(__in LPVOID lpTrampoline, __in LPVOID lpCallOrigProc)
{
  LPBYTE lpTramp = (LPBYTE)lpTrampoline - 6*sizeof(SIZE_T);

  *((SIZE_T NKT_UNALIGNED *)(lpTramp+0*sizeof(SIZE_T))) = (SIZE_T)lpCallOrigProc;
  return;
}

VOID HookTrampoline2_Disable(__in LPVOID lpTrampoline, __in BOOL bLock)
{
  LPBYTE lpTramp = (LPBYTE)lpTrampoline - 6*sizeof(SIZE_T);
  LONG volatile *lpnMutex;
#if defined _M_IX86
  LONG volatile *lpnDest;
#elif defined _M_X64
  __int64 volatile *lpnDest;
#endif

  if (lpTrampoline != NULL)
  {
    lpnMutex = (LONG volatile*)(lpTramp+2*sizeof(SIZE_T));
    if (bLock != FALSE)
      NktAcquireExclusive(lpnMutex);
#if defined _M_IX86
    lpnDest = (LONG volatile*)(lpTramp+1*sizeof(SIZE_T));
    _InterlockedExchange(lpnDest, 0);
#elif defined _M_X64
    lpnDest = (__int64 volatile*)(lpTramp+1*sizeof(SIZE_T));
    _InterlockedExchange64(lpnDest, 0i64);
#endif
    if (bLock != FALSE)
      NktReleaseExclusive(lpnMutex);
  }
  return;
}

VOID HookTrampoline2_Delete(__in HANDLE hHeap, __in LPVOID lpTrampoline)
{
  if (hHeap != NULL && lpTrampoline != NULL)
  {
    LPBYTE lpTramp = (LPBYTE)lpTrampoline - 6*sizeof(SIZE_T);

    ::HeapFree(hHeap, 0, lpTramp);
  }
  return;
}

//-----------------------------------------------------------

static VOID __stdcall MyNktAcquireShared(LONG volatile *lpnValue)
{
  NktAcquireShared(lpnValue);
  return;
}

static VOID __stdcall MyNktReleaseShared(LONG volatile *lpnValue)
{
  NktReleaseShared(lpnValue);
  return;
}

static VOID __stdcall MyNktAcquireExclusive(LONG volatile *lpnValue)
{
  NktAcquireExclusive(lpnValue);
  return;
}

static VOID __stdcall MyNktReleaseExclusive(LONG volatile *lpnValue)
{
  NktReleaseExclusive(lpnValue);
  return;
}
