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

#include "StdAfx.h"
#include "ProcessMemory.h"
#include "..\Common\StringLiteW.h"

//-----------------------------------------------------------

#define NKT_ACCESS_READ	 (PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_OPERATION)
#define NKT_ACCESS_WRITE	(NKT_ACCESS_READ|PROCESS_VM_WRITE)

#define XISALIGNED(x)  ((((SIZE_T)(x)) & (sizeof(SIZE_T)-1)) == 0)

//-----------------------------------------------------------

static SIZE_T TryMemCpy(__out void *lpDest, __in const void *lpSrc, __in SIZE_T nCount);

//-----------------------------------------------------------
// CNktProcessMemoryImpl

STDMETHODIMP CNktProcessMemoryImpl::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = { &IID_INktRemoteBridge };
  SIZE_T i;

  for (i=0; i<NKT_ARRAYLEN(arr); i++)
  {
    if (InlineIsEqualGUID(*arr[i],riid))
      return S_OK;
  }
  return S_FALSE;
}

HRESULT CNktProcessMemoryImpl::GetProtection(__in my_ssize_t remoteAddr, __out eNktProtection *pVal)
{
  MEMORY_BASIC_INFORMATION sMbi;
  HANDLE hTempProc;
  DWORD dwProt;
  int nNktProt;

  if (pVal != NULL)
    *pVal = (eNktProtection)0;
  if (pVal == NULL || remoteAddr == 0)
    return E_POINTER;
  //get process handle
  hTempProc = GetReadAccessHandle();
  if (hTempProc == NULL)
    return E_ACCESSDENIED;
  //query info
  if (::VirtualQueryEx(hTempProc, (LPVOID)(my_size_t)remoteAddr, &sMbi, sizeof(sMbi)) == FALSE)
    return E_ACCESSDENIED;
  //convert
  nNktProt = 0;
  dwProt = sMbi.Protect/*sMbi.AllocationProtect*/;
  if ((dwProt & PAGE_EXECUTE_READWRITE) != 0)
    nNktProt = protExecute | protRead | protWrite;
  else if ((dwProt & PAGE_EXECUTE_WRITECOPY) != 0)
    nNktProt = protExecute | protRead | protWrite | protIsWriteCopy;
  else if ((dwProt & PAGE_EXECUTE_READ) != 0)
    nNktProt = protExecute | protRead;
  else if ((dwProt & PAGE_READWRITE) != 0)
    nNktProt = protRead | protWrite;
  else if ((dwProt & PAGE_WRITECOPY) != 0)
    nNktProt = protRead | protWrite | protIsWriteCopy;
  if ((dwProt & PAGE_EXECUTE) != 0)
    nNktProt = protExecute;
  else if ((dwProt & PAGE_READONLY) != 0)
    nNktProt = protRead;
  // these are modifiers
  if ((dwProt & PAGE_GUARD) != 0)
    nNktProt |= protGuard;
  if ((dwProt & PAGE_NOCACHE) != 0)
    nNktProt |= protNoCache;
  if ((dwProt & PAGE_WRITECOMBINE) != 0)
    nNktProt |= protWriteCombine;
  //done
  *pVal = (eNktProtection)nNktProt;
  return S_OK;
}

HRESULT CNktProcessMemoryImpl::ReadMem(__in my_ssize_t localAddr, __in my_ssize_t remoteAddr,
                                       __in my_ssize_t bytes, __out my_ssize_t *pReaded)
{
  HRESULT hRes;
  SIZE_T nReaded;

  if (pReaded == NULL)
    return E_POINTER;
  *pReaded = 0;
  nReaded = 0;
  if (bytes == 0)
    return S_OK;
  if (localAddr == 0 || remoteAddr == 0)
    return E_POINTER;
  //read memory
  hRes = Read((LPVOID)localAddr, (LPVOID)remoteAddr, (SIZE_T)bytes, &nReaded);
  if (SUCCEEDED(hRes))
    *pReaded = (my_ssize_t)nReaded;
  return hRes;
}

HRESULT CNktProcessMemoryImpl::WriteMem(__in my_ssize_t remoteAddr, __in my_ssize_t localAddr,
                                        __in my_ssize_t bytes, __out my_ssize_t *pWritten)
{
  if (pWritten == NULL)
    return E_POINTER;
  *pWritten = 0;
  if (bytes == 0)
    return S_OK;
  if (localAddr == 0 || remoteAddr == 0)
    return E_POINTER;
  //write memory
  return WriteProtected((LPVOID)remoteAddr, (LPVOID)localAddr, (SIZE_T)bytes, (SIZE_T*)pWritten);
}

HRESULT CNktProcessMemoryImpl::Write(__in my_ssize_t remoteAddr, __in VARIANT var)
{
  LPVOID lpData;
  SIZE_T nItemsCount;
  VARIANT *lpVar;
  HRESULT hRes;
  my_ssize_t nWritten;

  if (remoteAddr == 0)
    return E_POINTER;
  //write
  lpVar = &var;
  while (lpVar != NULL && lpVar->vt == (VT_BYREF|VT_VARIANT))
    lpVar = lpVar->pvarVal;
  if (lpVar == NULL)
    return E_POINTER;
  switch (lpVar->vt)
  {
    case VT_I1:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->cVal), sizeof(char), &nWritten);
      break;
    case VT_UI1:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->bVal), sizeof(unsigned char), &nWritten);
      break;
    case VT_I2:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->iVal), sizeof(short), &nWritten);
      break;
    case VT_UI2:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->uiVal), sizeof(unsigned short), &nWritten);
      break;
    case VT_I4:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->lVal), sizeof(LONG), &nWritten);
      break;
    case VT_UI4:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->ulVal), sizeof(ULONG), &nWritten);
      break;
    case VT_INT:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->intVal), sizeof(LONG), &nWritten);
      break;
    case VT_UINT:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->uintVal), sizeof(ULONG), &nWritten);
      break;
    case VT_I8:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->llVal), sizeof(LONGLONG), &nWritten);
      break;
    case VT_UI8:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->ullVal), sizeof(ULONGLONG), &nWritten);
      break;
    case VT_R4:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->fltVal), sizeof(float), &nWritten);
      break;
    case VT_R8:
      hRes = WriteMem(remoteAddr, (my_ssize_t)&(lpVar->dblVal), sizeof(double), &nWritten);
      break;

    case VT_BYREF|VT_I1:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->pcVal), sizeof(char), &nWritten);
      break;
    case VT_BYREF|VT_UI1:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->pbVal), sizeof(unsigned char), &nWritten);
      break;
    case VT_BYREF|VT_I2:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->piVal), sizeof(short), &nWritten);
      break;
    case VT_BYREF|VT_UI2:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->puiVal), sizeof(unsigned short), &nWritten);
      break;
    case VT_BYREF|VT_I4:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->plVal), sizeof(LONG), &nWritten);
      break;
    case VT_BYREF|VT_UI4:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->pulVal), sizeof(ULONG), &nWritten);
      break;
    case VT_BYREF|VT_INT:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->pintVal), sizeof(LONG), &nWritten);
      break;
    case VT_BYREF|VT_UINT:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->puintVal), sizeof(ULONG), &nWritten);
      break;
    case VT_BYREF|VT_I8:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->pllVal), sizeof(LONGLONG), &nWritten);
      break;
    case VT_BYREF|VT_UI8:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->pullVal), sizeof(ULONGLONG), &nWritten);
      break;
    case VT_BYREF|VT_R4:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->pfltVal), sizeof(float), &nWritten);
      break;
    case VT_BYREF|VT_R8:
      hRes = WriteMem(remoteAddr, (my_ssize_t)(lpVar->pdblVal), sizeof(double), &nWritten);
      break;

    case VT_ARRAY|VT_I1:
    case VT_ARRAY|VT_UI1:
    case VT_ARRAY|VT_I2:
    case VT_ARRAY|VT_UI2:
    case VT_ARRAY|VT_I4:
    case VT_ARRAY|VT_UI4:
    case VT_ARRAY|VT_INT:
    case VT_ARRAY|VT_UINT:
    case VT_ARRAY|VT_I8:
    case VT_ARRAY|VT_UI8:
    case VT_ARRAY|VT_R4:
    case VT_ARRAY|VT_R8:
      if (lpVar->parray == NULL)
      {
        hRes = E_POINTER;
        break;
      }
      if (lpVar->parray->cDims != 1)
      {
        hRes = E_INVALIDARG;
        break;
      }
      nItemsCount = (SIZE_T)(lpVar->parray->rgsabound[0].cElements);
      //get array data
      hRes = ::SafeArrayAccessData(lpVar->parray, (LPVOID*)&lpData);
      if (FAILED(hRes))
        break;
      switch (lpVar->vt)
      {
        case VT_ARRAY|VT_I1:
        case VT_ARRAY|VT_UI1:
          nItemsCount *= sizeof(char);
          break;
        case VT_ARRAY|VT_I2:
        case VT_ARRAY|VT_UI2:
          nItemsCount *= sizeof(short);
          break;
        case VT_ARRAY|VT_I4:
        case VT_ARRAY|VT_INT:
        case VT_ARRAY|VT_UI4:
        case VT_ARRAY|VT_UINT:
          nItemsCount *= sizeof(LONG);
          break;
        case VT_ARRAY|VT_I8:
        case VT_ARRAY|VT_UI8:
          nItemsCount *= sizeof(LONGLONG);
          break;
        case VT_ARRAY|VT_R4:
          nItemsCount *= sizeof(float);
          break;
        case VT_ARRAY|VT_R8:
          nItemsCount *= sizeof(double);
          break;
      }
      hRes = WriteMem(remoteAddr, (my_ssize_t)lpData, nItemsCount, &nWritten);
      ::SafeArrayUnaccessData(lpVar->parray);
      break;

    default:
      hRes = E_INVALIDARG;
      break;
  }
  return hRes;
}

HRESULT CNktProcessMemoryImpl::ReadString(__in my_ssize_t remoteAddr, __in VARIANT_BOOL isAnsi,
                                          __out BSTR *pVal)
{
  union {
    LPVOID lpStr;
    LPSTR szDestA;
    LPWSTR szDestW;
  };
  CNktComBStr cComBStr;
  HRESULT hRes;

  if (pVal == NULL)
    return E_POINTER;
  lpStr = NULL;
  if (remoteAddr == 0)
    return E_POINTER;

  //read string
  if (isAnsi != VARIANT_FALSE)
    hRes = ReadStringA(&szDestA, (LPVOID)remoteAddr);
  else
    hRes = ReadStringW(&szDestW, (LPVOID)remoteAddr);
  if (SUCCEEDED(hRes))
  {
    if (isAnsi != VARIANT_FALSE)
      hRes = cComBStr.Set(szDestA);
    else
      hRes = cComBStr.Set(szDestW);
  }
  if (lpStr != NULL)
    NKT_FREE(lpStr);
  *pVal = cComBStr.Detach();
  return hRes;
}

HRESULT CNktProcessMemoryImpl::ReadStringN(__in my_ssize_t remoteAddr, __in VARIANT_BOOL isAnsi,
                                           __in LONG maxChars, __out BSTR *pVal)
{
  union {
    LPVOID lpStr;
    LPSTR szDestA;
    LPWSTR szDestW;
  };
  CNktComBStr cComBStr;
  HRESULT hRes;

  if (pVal == NULL)
    return E_POINTER;
  lpStr = NULL;
  if (remoteAddr == 0)
    return E_POINTER;
  if (maxChars < 0)
    return E_INVALIDARG;
  //read string
  if (isAnsi != VARIANT_FALSE)
    hRes = ReadStringA(&szDestA, (LPVOID)remoteAddr, (SIZE_T)maxChars);
  else
    hRes = ReadStringW(&szDestW, (LPVOID)remoteAddr, (SIZE_T)maxChars);
  if (SUCCEEDED(hRes))
  {
    if (isAnsi != VARIANT_FALSE)
      hRes = cComBStr.Set(szDestA);
    else
      hRes = cComBStr.Set(szDestW);
  }
  if (lpStr != NULL)
    NKT_FREE(lpStr);
  *pVal = cComBStr.Detach();
  return hRes;
}

HRESULT CNktProcessMemoryImpl::WriteString(__in my_ssize_t remoteAddr, __in BSTR str,
                                           __in VARIANT_BOOL asAnsi, __in VARIANT_BOOL writeNul)
{
  if (remoteAddr == NULL || str == NULL)
    return E_POINTER;
  if (asAnsi != VARIANT_FALSE)
  {
    TNktAutoFreePtr<CHAR> sA;

    sA.Attach(CNktStringW::Wide2Ansi(str, (SIZE_T)::SysStringLen(str)));
    if (sA == NULL)
      return E_OUTOFMEMORY;
    //write ansi string
    return WriteStringA((LPVOID)remoteAddr, sA, NKT_SIZE_T_MAX, (writeNul != VARIANT_FALSE) ? TRUE : FALSE);
  }
  //write wide string
  return WriteStringW((LPVOID)remoteAddr, str, NKT_SIZE_T_MAX, (writeNul != VARIANT_FALSE) ? TRUE : FALSE);
}

HRESULT CNktProcessMemoryImpl::StringLength(__in my_ssize_t remoteAddr, __in VARIANT_BOOL asAnsi,
                                            __out LONG *pVal)
{
  SIZE_T nChars;
  HRESULT hRes;

  if (pVal == NULL)
    return E_POINTER;
  *pVal = 0;
  nChars = 0;
  if (remoteAddr == 0)
    return E_POINTER;
  //read string length
  if (asAnsi != VARIANT_FALSE)
    hRes = GetStringLengthA((LPVOID)remoteAddr, &nChars);
  else
    hRes = GetStringLengthW((LPVOID)remoteAddr, &nChars);
  if (nChars > LONG_MAX)
    nChars = LONG_MAX;
  if (SUCCEEDED(hRes))
    *pVal = (LONG)nChars;
  return hRes;
}

HRESULT CNktProcessMemoryImpl::get_CharVal(__in my_ssize_t remoteAddr, __out signed char *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(char), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_CharVal(__in my_ssize_t remoteAddr, __in signed char newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_ByteVal(__in my_ssize_t remoteAddr, __out unsigned char *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(char), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_ByteVal(__in my_ssize_t remoteAddr, __in unsigned char newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_ShortVal(__in my_ssize_t remoteAddr, __out short *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(short), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_ShortVal(__in my_ssize_t remoteAddr, __in short newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_UShortVal(__in my_ssize_t remoteAddr, __out unsigned short *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(short), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_UShortVal(__in my_ssize_t remoteAddr, __in unsigned short newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_LongVal(__in my_ssize_t remoteAddr, __out long *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(LONG), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_LongVal(__in my_ssize_t remoteAddr, __in long newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_ULongVal(__in my_ssize_t remoteAddr, __out unsigned long *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(LONG), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_ULongVal(__in my_ssize_t remoteAddr, __in unsigned long newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_LongLongVal(__in my_ssize_t remoteAddr, __out __int64 *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(LONGLONG), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_LongLongVal(__in my_ssize_t remoteAddr, __in __int64 newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_ULongLongVal(__in my_ssize_t remoteAddr, __out unsigned __int64 *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(LONGLONG), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_ULongLongVal(__in my_ssize_t remoteAddr, __in unsigned __int64 newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_SSizeTVal(__in my_ssize_t remoteAddr, __out my_ssize_t *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(my_ssize_t), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_SSizeTVal(__in my_ssize_t remoteAddr, __in my_ssize_t newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_SizeTVal(__in my_ssize_t remoteAddr, __out my_size_t *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(my_size_t), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_SizeTVal(__in my_ssize_t remoteAddr, __in my_size_t newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_FloatVal(__in my_ssize_t remoteAddr, __out float *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(float), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_FloatVal(__in my_ssize_t remoteAddr, __in float newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::get_DoubleVal(__in my_ssize_t remoteAddr, __out double *pVal)
{
  my_ssize_t nReaded;

  return ReadMem((my_ssize_t)pVal, remoteAddr, sizeof(double), &nReaded);
}

HRESULT CNktProcessMemoryImpl::put_DoubleVal(__in my_ssize_t remoteAddr, __in double newValue)
{
  my_ssize_t nWritten;

  return WriteMem(remoteAddr, (my_ssize_t)&newValue, sizeof(newValue), &nWritten);
}

HRESULT CNktProcessMemoryImpl::AllocMem(__in my_ssize_t bytes, __in VARIANT_BOOL executeFlag,
                                        __out my_ssize_t *pVal)
{
  LPVOID lpRemoteDest;
  HANDLE hTempProc;
  DWORD dwProtFlags;

  if (bytes == 0)
    bytes = 1; //act like malloc
  if (bytes < 0)
    return E_INVALIDARG;
  //get process handle
  hTempProc = GetWriteAccessHandle();
  if (hTempProc == NULL)
    return E_ACCESSDENIED;
  //convert protection flags
  dwProtFlags = (executeFlag != VARIANT_FALSE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
  //do allocation
  lpRemoteDest = ::VirtualAllocEx(hTempProc, NULL, (SIZE_T)bytes, MEM_COMMIT, dwProtFlags);
  if (lpRemoteDest == NULL)
    return E_OUTOFMEMORY;
  *pVal = (my_ssize_t)lpRemoteDest;
  return S_OK;
}

HRESULT CNktProcessMemoryImpl::FreeMem(__in my_ssize_t remoteAddr)
{
  HANDLE hTempProc;

  if (remoteAddr != 0)
  {
    //get process handle
    hTempProc = GetWriteAccessHandle();
    if (hTempProc == NULL)
      return E_ACCESSDENIED;
    //free memory
    if (::VirtualFreeEx(hTempProc, (LPVOID)(SIZE_T)remoteAddr, 0, MEM_RELEASE) == FALSE)
      return E_FAIL;
  }
  return S_OK;
}

HANDLE CNktProcessMemoryImpl::GetReadAccessHandle()
{
  if (hProc[0] == NULL)
  {
    ObjectLock cLock(this);

    if (hProc[0] == NULL)
      hProc[0] = ::OpenProcess(NKT_ACCESS_READ, FALSE, dwPid);
  }
  return hProc[0];
}

HANDLE CNktProcessMemoryImpl::GetWriteAccessHandle()
{
  if (hProc[1] == NULL)
  {
    ObjectLock cLock(this);

    if (hProc[1] == NULL && bWriteAccessChecked == FALSE)
    {
      bWriteAccessChecked = TRUE;
      hProc[1] = ::OpenProcess(NKT_ACCESS_WRITE, FALSE, dwPid);
    }
  }
  return hProc[1];
}

HRESULT CNktProcessMemoryImpl::WriteProtected(__in LPVOID lpRemoteDest, __in LPCVOID lpLocalSrc,
                                              __in SIZE_T nSize, __out_opt SIZE_T *lpnWritten)
{
  LPBYTE s, d;
  SIZE_T nToCopy, nWritten, nTotalWritten;
  DWORD dwOrigProt, dwNewProt, dwDummy;
  MEMORY_BASIC_INFORMATION sMbi;
  HANDLE hTempProc;
  HRESULT hRes;

  if (lpnWritten != NULL)
    *lpnWritten = NULL;
  //check parameters
  if (nSize == 0)
    return S_OK;
  if (lpRemoteDest == NULL || lpLocalSrc == NULL)
    return E_POINTER;
  //get process handle
  hTempProc = GetWriteAccessHandle();
  if (hTempProc == NULL)
    return E_ACCESSDENIED;
  //do write
  hRes = S_OK;
  nTotalWritten = 0;
  {
    ObjectLock cLock(this);

    d = (LPBYTE)lpRemoteDest;
    s = (LPBYTE)lpLocalSrc;
    while (nSize > 0)
    {
      //check current protection
      if (::VirtualQueryEx(hTempProc, d, &sMbi, sizeof(sMbi)) == FALSE)
      {
        hRes = E_FAIL;
        break;
      }
      //calculate available writable bytes in region
      nToCopy = 0;
      if (d >= (LPBYTE)(sMbi.BaseAddress))
      {
        nToCopy = d - (LPBYTE)(sMbi.BaseAddress);
        nToCopy = (nToCopy < sMbi.RegionSize) ? (sMbi.RegionSize - nToCopy) : 0;
      }
      _ASSERT(nToCopy > 0);
      if (nToCopy == 0)
      {
        hRes = E_FAIL; //should not happen
        break;
      }
      //calculate new desired access based on the current one
      if ((sMbi.Protect & PAGE_GUARD) != 0)
      {
        hRes = E_FAIL; //if a page is guarded, do not touch
        break;
      }
      dwNewProt = sMbi.Protect & 0xFF;
      switch (dwNewProt)
      {
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
          break;
        case PAGE_EXECUTE_READ:
          dwNewProt = PAGE_EXECUTE_READWRITE;
          break;
        default:
          dwNewProt = PAGE_READWRITE;
          break;
      }
      dwNewProt |= (sMbi.Protect & 0xFFFFFF00); //add the rest of the flags
      //if access protection needs a change, do it
      if (dwNewProt != sMbi.Protect)
      {
        if (::VirtualProtectEx(hTempProc, sMbi.BaseAddress, sMbi.RegionSize, dwNewProt,
                               &dwOrigProt) == FALSE)
        {
          hRes = E_ACCESSDENIED;
          break;
        }
      }
      //do partial copy
      if (nToCopy > nSize)
        nToCopy = nSize;
      nWritten = 0;
      if (bIsLocal == FALSE)
      {
        hRes = (::WriteProcessMemory(hTempProc, d, s, nToCopy, &nWritten) != FALSE) ?
                                     S_OK : NKT_HRESULT_FROM_LASTERROR();
      }
      else
      {
        nWritten = TryMemCpy(d, s, nToCopy);
        hRes = (nToCopy == nWritten) ? S_OK : E_ACCESSDENIED;
      }
      nTotalWritten += nWritten;
      //restore original access
      if (dwNewProt != sMbi.Protect)
        ::VirtualProtectEx(hTempProc, sMbi.BaseAddress, sMbi.RegionSize, dwOrigProt, &dwDummy);
      //check copy operation
      if (FAILED(hRes) || nToCopy != nWritten)
        break;
      //advance buffer
      s += nWritten;
      d += nWritten;
      nSize -= nWritten;
    }
  }
  if (nTotalWritten > 0 && nSize > 0)
    hRes = NKT_E_PartialCopy;
  if (FAILED(hRes) && hRes != E_ACCESSDENIED && hRes != NKT_E_PartialCopy)
    hRes = E_FAIL;
  if (lpnWritten != NULL)
    *lpnWritten = nTotalWritten;
  return hRes;
}


HRESULT CNktProcessMemoryImpl::Read(__out LPVOID lpLocalDest, __in LPCVOID lpRemoteSrc, __in SIZE_T nSize,
                                    __out_opt SIZE_T *lpnReaded)
{
  HRESULT hRes;
  SIZE_T nReaded;
  HANDLE hTempProc;

  if (lpnReaded != NULL)
    *lpnReaded = 0;
  //check parameters
  if (nSize == 0)
    return S_OK;
  if (lpLocalDest == NULL || lpRemoteSrc == NULL)
    return E_POINTER;
  //do read
  nReaded = 0;
  if (bIsLocal == FALSE)
  {
    //get process handle
    hTempProc = GetReadAccessHandle();
    if (hTempProc != NULL)
    {
      hRes = (::ReadProcessMemory(hTempProc, lpRemoteSrc, lpLocalDest, nSize, &nReaded) != FALSE) ?
                                  S_OK : NKT_HRESULT_FROM_LASTERROR();
    }
    else
    {
      hRes = E_ACCESSDENIED;
    }
  }
  else
  {
    nReaded = TryMemCpy(lpLocalDest, lpRemoteSrc, nSize);
    hRes = (nSize == nReaded) ? S_OK : E_ACCESSDENIED;
  }
  if (nReaded > 0 && nSize != nReaded)
    hRes = NKT_E_PartialCopy;
  if (FAILED(hRes) && hRes != E_ACCESSDENIED && hRes != NKT_E_PartialCopy)
    hRes = E_FAIL;
  if (lpnReaded != NULL)
    *lpnReaded = nReaded;
  return hRes;
}

HRESULT CNktProcessMemoryImpl::Write(__in LPVOID lpRemoteDest, __in LPCVOID lpLocalSrc, __in SIZE_T nSize,
                                     __out_opt SIZE_T *lpnWritten)
{
  SIZE_T nWritten;
  HANDLE hTempProc;
  HRESULT hRes;

  if (lpnWritten != NULL)
    *lpnWritten = NULL;
  //check parameters
  if (nSize == 0)
    return S_OK;
  if (lpRemoteDest == NULL || lpLocalSrc == NULL)
    return E_POINTER;
  //do write
  if (bIsLocal == FALSE)
  {
    nWritten = 0;
    //get process handle
    hTempProc = GetWriteAccessHandle();
    if (hTempProc == NULL)
      return E_ACCESSDENIED;
    hRes = (::WriteProcessMemory(hTempProc, lpRemoteDest, lpLocalSrc, nSize, &nWritten) != FALSE) ?
                                 S_OK : NKT_HRESULT_FROM_LASTERROR();
  }
  else
  {
    nWritten = TryMemCpy(lpRemoteDest, lpLocalSrc, nSize);
    hRes = (nSize == nWritten) ? S_OK : E_ACCESSDENIED;
  }
  if (nWritten > 0 && nSize != nWritten)
    hRes = NKT_E_PartialCopy;
  if (FAILED(hRes) && hRes != E_ACCESSDENIED && hRes != NKT_E_PartialCopy)
    hRes = E_FAIL;
  if (lpnWritten != NULL)
    *lpnWritten = nWritten;
  return hRes;
}

HRESULT CNktProcessMemoryImpl::ReadStringA(__out LPSTR* lpszDestA, __in LPCVOID lpRemoteSrc,
                                           __in SIZE_T nMaxChars)
{
  ObjectLock cLock(this);
  HRESULT hRes;
  LPSTR sA;
  SIZE_T nCurrLen, nCurrSize;

  if (lpszDestA == NULL)
    return E_POINTER;
  *lpszDestA = NULL;
  if (lpRemoteSrc == NULL)
    return E_POINTER;
  //initial alloc
  nCurrSize = 256;
  *lpszDestA = (LPSTR)NKT_MALLOC(nCurrSize*sizeof(CHAR));
  if (*lpszDestA == NULL)
    return E_OUTOFMEMORY;
  nCurrLen = 0;
  for (hRes=S_OK;;)
  {
    //check buffer size
    if (nCurrLen >= nCurrSize)
    {
      if ((nCurrSize<<1) < nCurrSize)
      {
        hRes = E_OUTOFMEMORY;
        break;
      }
      nCurrSize <<= 1;
      sA = (LPSTR)NKT_REALLOC(*lpszDestA, nCurrSize*sizeof(CHAR));
      if (sA == NULL)
      {
        hRes = E_OUTOFMEMORY;
        break;
      }
      *lpszDestA = sA;
    }
    //check max chars
    if (nMaxChars == 0)
    {
      (*lpszDestA)[nCurrLen] = 0;
      break;
    }
    else if (nMaxChars != NKT_SIZE_T_MAX)
      nMaxChars--;
    //read a char
    hRes = Read((*lpszDestA)+nCurrLen, (LPSTR)lpRemoteSrc+nCurrLen, sizeof(CHAR), NULL);
    if (FAILED(hRes))
    {
      (*lpszDestA)[nCurrLen] = 0;
      break;
    }
    if ((*lpszDestA)[nCurrLen] == 0)
      break;
    nCurrLen++;
  }
  if (hRes == E_OUTOFMEMORY)
  {
    NKT_FREE(*lpszDestA);
    *lpszDestA = NULL;
  }
  return hRes;
}

HRESULT CNktProcessMemoryImpl::ReadStringW(__out LPWSTR* lpszDestW, __in LPCVOID lpRemoteSrc,
                                           __in SIZE_T nMaxChars)
{
  ObjectLock cLock(this);
  HRESULT hRes;
  LPWSTR sW;
  SIZE_T nCurrLen, nCurrSize;

  if (lpszDestW == NULL)
    return E_POINTER;
  *lpszDestW = NULL;
  if (lpRemoteSrc == NULL)
    return E_POINTER;
  //initial alloc
  nCurrSize = 256;
  *lpszDestW = (LPWSTR)NKT_MALLOC(nCurrSize*sizeof(WCHAR));
  if (*lpszDestW == NULL)
    return E_OUTOFMEMORY;
  nCurrLen = 0;
  for (hRes=S_OK;;)
  {
    //check buffer size
    if (nCurrLen >= nCurrSize)
    {
      if ((nCurrSize<<1) < nCurrSize)
      {
        hRes = E_OUTOFMEMORY;
        break;
      }
      nCurrSize <<= 1;
      sW = (LPWSTR)NKT_REALLOC(*lpszDestW, nCurrSize*sizeof(WCHAR));
      if (sW == NULL)
      {
        hRes = E_OUTOFMEMORY;
        break;
      }
      *lpszDestW = sW;
    }
    //check max chars
    if (nMaxChars == 0)
    {
      (*lpszDestW)[nCurrLen] = 0;
      break;
    }
    else if (nMaxChars != NKT_SIZE_T_MAX)
      nMaxChars--;
    //read a char
    hRes = Read((*lpszDestW)+nCurrLen, (LPWSTR)lpRemoteSrc+nCurrLen, sizeof(WCHAR), NULL);
    if (FAILED(hRes))
    {
      (*lpszDestW)[nCurrLen] = 0;
      break;
    }
    if ((*lpszDestW)[nCurrLen] == 0)
      break;
    nCurrLen++;
  }
  if (hRes == E_OUTOFMEMORY)
  {
    NKT_FREE(*lpszDestW);
    *lpszDestW = NULL;
  }
  return hRes;
}

HRESULT CNktProcessMemoryImpl::WriteStringA(__in LPVOID lpRemoteDest, __in_nz_opt LPCSTR szSrcA,
                                            __in SIZE_T nSrcLen, __in BOOL bWriteNulTerminator)
{
  ObjectLock cLock(this);
  HRESULT hRes;
  CHAR chA;

  if (lpRemoteDest == NULL || szSrcA == NULL)
    return E_POINTER;
  if (nSrcLen == NKT_SIZE_T_MAX)
    nSrcLen = strlen(szSrcA);
  if (nSrcLen > 0)
  {
    hRes = WriteProtected((LPSTR)lpRemoteDest, szSrcA, nSrcLen*sizeof(CHAR), NULL);
    if (FAILED(hRes))
      return hRes;
  }
  if (bWriteNulTerminator != FALSE)
  {
    chA = 0;
    hRes = WriteProtected((LPSTR)lpRemoteDest+nSrcLen,  &chA, sizeof(CHAR), NULL);
    if (FAILED(hRes))
      return hRes;
  }
  return S_OK;
}

HRESULT CNktProcessMemoryImpl::WriteStringW(__in LPVOID lpRemoteDest, __in_nz_opt LPCWSTR szSrcW,
                                            __in SIZE_T nSrcLen, __in BOOL bWriteNulTerminator)
{
  ObjectLock cLock(this);
  HRESULT hRes;
  WCHAR chW;

  if (lpRemoteDest == NULL || szSrcW == NULL)
    return E_POINTER;
  if (nSrcLen == NKT_SIZE_T_MAX)
    nSrcLen = wcslen(szSrcW);
  if (nSrcLen > 0)
  {
    hRes = WriteProtected((LPWSTR)lpRemoteDest, szSrcW, nSrcLen*sizeof(WCHAR), NULL);
    if (FAILED(hRes))
      return hRes;
  }
  if (bWriteNulTerminator != FALSE)
  {
    chW = 0;
    hRes = WriteProtected((LPWSTR)lpRemoteDest+nSrcLen,  &chW, sizeof(WCHAR), NULL);
    if (FAILED(hRes))
      return hRes;
  }
  return S_OK;
}

HRESULT CNktProcessMemoryImpl::GetStringLengthA(__in LPVOID lpRemoteDest, __out SIZE_T *lpnChars)
{
  ObjectLock cLock(this);
  HRESULT hRes;
  CHAR chA;

  if (lpnChars == NULL)
    return E_POINTER;
  *lpnChars = 0;
  if (lpRemoteDest == NULL)
    return E_POINTER;
  for (;;)
  {
    hRes = Read(&chA, (LPSTR)lpRemoteDest+(*lpnChars), sizeof(CHAR), NULL);
    if (FAILED(hRes))
      return hRes;
    if (chA == 0)
      break;
    (*lpnChars)++;
  }
  return S_OK;
}

HRESULT CNktProcessMemoryImpl::GetStringLengthW(__in LPVOID lpRemoteDest, __out SIZE_T *lpnChars)
{
  ObjectLock cLock(this);
  HRESULT hRes;
  WCHAR chW;

  if (lpnChars == NULL)
    return E_POINTER;
  *lpnChars = 0;
  if (lpRemoteDest == NULL)
    return E_POINTER;
  for (;;)
  {
    hRes = Read(&chW, (LPWSTR)lpRemoteDest+(*lpnChars), sizeof(WCHAR), NULL);
    if (FAILED(hRes))
      return hRes;
    if (chW == 0)
      break;
    (*lpnChars)++;
  }
  return S_OK;
}

//-----------------------------------------------------------

static SIZE_T TryMemCpy(__out void *lpDest, __in const void *lpSrc, __in SIZE_T nCount)
{
  LPBYTE s, d;
  SIZE_T nOrigCount;

  s = (LPBYTE)lpSrc;
  d = (LPBYTE)lpDest;
  nOrigCount = nCount;
  try
  {
    if (XISALIGNED(s) && XISALIGNED(d))
    {
      while (nCount >= sizeof(SIZE_T))
      {
        *((SIZE_T*)d) = *((SIZE_T*)s);
        s += sizeof(SIZE_T);
        d += sizeof(SIZE_T);
        nCount -= sizeof(SIZE_T);
      }
    }
    while (nCount > 0)
    {
      *d++ = *s++;
      nCount--;
    }
  }
  catch (...)
  { }
  return nOrigCount-nCount;
}
