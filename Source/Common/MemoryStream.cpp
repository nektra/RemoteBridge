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

#include "MemoryStream.h"
#include <crtdbg.h>
#include <intrin.h>

//-----------------------------------------------------------

CStaticMemoryStream::CStaticMemoryStream(__in LPVOID _lpData, __in SIZE_T _nDataSize) : IStream()
{
  _ASSERT(_nDataSize == 0 || _lpData != NULL);
  nSize = _nDataSize;
  lpData = (LPBYTE)_lpData;
  nPos = 0;
  InterlockedExchange(&nRefCount, 1);
  return;
}

CStaticMemoryStream::~CStaticMemoryStream()
{
  return;
}

STDMETHODIMP CStaticMemoryStream::QueryInterface(__RPC__in REFIID riid, __RPC__deref_out void **ppvObject)
{
  if (ppvObject == NULL)
    return E_POINTER;
  if (riid == IID_IStream || riid == IID_ISequentialStream || riid == IID_IUnknown)
  {
    *ppvObject = this;
    AddRef();
    return S_OK;
  }
  *ppvObject = NULL;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CStaticMemoryStream::AddRef()
{
  return (ULONG)InterlockedIncrement(&nRefCount);
}

STDMETHODIMP_(ULONG) CStaticMemoryStream::Release()
{
  ULONG nRef = (ULONG)InterlockedDecrement(&nRefCount);
  if (nRef == 0)
    delete this;
  return nRef;
}

STDMETHODIMP CStaticMemoryStream::Read(__out_bcount_part(cb, *pcbRead) void *pv, __in ULONG cb,
                                       __out_opt ULONG *pcbRead)
{
  if (pv == NULL)
    return E_POINTER;

  if ((SIZE_T)cb > nSize-nPos)
    cb = (ULONG)(nSize-nPos);
  if (cb > 0 && pv == NULL)
  {
    if (pcbRead != NULL)
      *pcbRead = 0;
    return E_POINTER;
  }
  memcpy(pv, lpData+nPos, cb);
  nPos += (SIZE_T)cb;
  if (pcbRead != NULL)
    *pcbRead = cb;
  return S_OK;
}

STDMETHODIMP CStaticMemoryStream::Write(__in_bcount(cb)  const void *pv, __in ULONG cb,
                                        __out_opt ULONG *pcbWritten)
{
  if (pcbWritten != NULL)
    *pcbWritten = 0;
  return E_NOTIMPL;
}

STDMETHODIMP CStaticMemoryStream::Seek(__in LARGE_INTEGER dlibMove, __in DWORD dwOrigin,
                                       __out_opt ULARGE_INTEGER *plibNewPosition)
{
  ULONGLONG nTemp;

  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      if ((ULONGLONG)dlibMove.QuadPart > (ULONGLONG)nSize)
        return E_INVALIDARG;
      nPos = (SIZE_T)(ULONGLONG)(dlibMove.QuadPart);
      break;

    case STREAM_SEEK_CUR:
      if (dlibMove.QuadPart > 0)
      {
        nTemp = nSize-nPos;
        if ((ULONGLONG)dlibMove.QuadPart > nTemp)
          goto onerror;
        nPos += (SIZE_T)dlibMove.QuadPart;
      }
      else if (dlibMove.QuadPart < 0)
      {
        nTemp = (ULONGLONG)-dlibMove.QuadPart;
        if ((ULONGLONG)nPos < nTemp)
          goto onerror;
        nPos -= (SIZE_T)nTemp;
      }
      break;

    case STREAM_SEEK_END:
      if ((ULONGLONG)dlibMove.QuadPart > (ULONGLONG)nSize)
        return E_INVALIDARG;
      nPos = (SIZE_T)(ULONGLONG)(dlibMove.QuadPart);
      break;

    default:
onerror:
      if (plibNewPosition != NULL)
        plibNewPosition->QuadPart = (ULONGLONG)nPos;
      return E_INVALIDARG;
  }
  if (plibNewPosition != NULL)
    plibNewPosition->QuadPart = (ULONGLONG)nPos;
  return S_OK;
}

STDMETHODIMP CStaticMemoryStream::SetSize(__in ULARGE_INTEGER libNewSize)
{
  return E_NOTIMPL;
}

STDMETHODIMP CStaticMemoryStream::CopyTo(__RPC__in IStream *pstm, __in ULARGE_INTEGER cb,
                                         __out_opt ULARGE_INTEGER *pcbRead, __out_opt ULARGE_INTEGER *pcbWritten)
{
  BYTE aTemp[4096];
  ULONG cbToCopy, cbReaded, cbWritten;
  HRESULT hRes;

  if (pcbRead != NULL)
    pcbRead->QuadPart = 0;
  if (pcbWritten != NULL)
    pcbWritten->QuadPart = 0;
  if (pstm == NULL)
    return E_POINTER;
  hRes = S_OK;
  while (cb.QuadPart > 0)
  {
    cbToCopy = (cb.QuadPart > 4096) ? 4096 : (ULONG)(cb.QuadPart);
    hRes = Read(aTemp, cbToCopy, &cbReaded);
    if (FAILED(hRes) || cbReaded == 0)
      break;
    if (pcbRead != NULL)
      pcbRead->QuadPart += (ULONGLONG)cbReaded;
    //----
    hRes = pstm->Write(aTemp, cbReaded, &cbWritten);
    if (SUCCEEDED(hRes) && cbReaded != cbWritten)
      hRes = E_UNEXPECTED;
    if (FAILED(hRes))
      break;
    if (pcbWritten != NULL)
      pcbWritten->QuadPart += (ULONGLONG)cbWritten;
    //----
    cb.QuadPart -= (ULONGLONG)cbReaded;
  }
  return hRes;
}

STDMETHODIMP CStaticMemoryStream::Commit(__in DWORD grfCommitFlags)
{
  return E_NOTIMPL;
}

STDMETHODIMP CStaticMemoryStream::Revert()
{
  return E_NOTIMPL;
}

STDMETHODIMP CStaticMemoryStream::LockRegion(__in ULARGE_INTEGER libOffset, __in ULARGE_INTEGER cb,
                                             __in DWORD dwLockType)
{
  return E_NOTIMPL;
}

STDMETHODIMP CStaticMemoryStream::UnlockRegion(__in ULARGE_INTEGER libOffset, __in ULARGE_INTEGER cb,
                                               __in DWORD dwLockType)
{
  return E_NOTIMPL;
}

STDMETHODIMP CStaticMemoryStream::Stat(__RPC__out STATSTG *pstatstg, __in DWORD grfStatFlag)
{
  if (pstatstg == NULL)
    return E_POINTER;
  memset(pstatstg, 0, sizeof(STATSTG));
  return E_NOTIMPL;
}

STDMETHODIMP CStaticMemoryStream::Clone(__RPC__deref_out_opt IStream **ppstm)
{
  if (ppstm == NULL)
    return E_POINTER;
  *ppstm = NULL;
  return E_NOTIMPL;
}

VOID CStaticMemoryStream::Invalidate()
{
  nSize = nPos = 0;
  lpData = NULL;
  return;
}
