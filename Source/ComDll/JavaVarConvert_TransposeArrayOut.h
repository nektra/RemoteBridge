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

typedef HRESULT (*lpfnTransform_Out)(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize,
                                     __in LPVOID lpUserData);

//-----------------------------------------------------------

static HRESULT TransposeArray_Out(__inout LPBYTE &lpDest, __in LPBYTE lpSrc, __in SIZE_T nSrcSize,
                                  __in SAFEARRAY *parray, __in USHORT nCurrDim, __in lpfnTransform_Out fnTransform,
                                  __out SIZE_T *lpnSize, __in LPVOID lpUserData)
{
  ULONG i, nCount;
  SIZE_T nSrcMult, nDestSize;
  HRESULT hRes;

  nSrcMult = 1;
  for (i=0; i<nCurrDim; i++)
    nSrcMult *= (SIZE_T)(parray->rgsabound[parray->cDims-i-1].cElements);
  //----
  nCount = parray->rgsabound[parray->cDims-nCurrDim-1].cElements;
  if (nCurrDim < parray->cDims-1)
  {
    for (i=0; i<nCount; i++)
    {
      hRes = TransposeArray_Out(lpDest, lpSrc, nSrcSize, parray, nCurrDim+1, fnTransform, lpnSize, lpUserData);
      if (FAILED(hRes))
        return hRes;
      lpSrc += nSrcSize * nSrcMult;
    }
  }
  else
  {
    for (i=0; i<nCount; i++)
    {
      hRes = fnTransform(lpDest, lpSrc, &nDestSize, lpUserData);
      if (FAILED(hRes))
        return hRes;
      lpSrc += nSrcSize * nSrcMult;
      if (lpDest != NULL)
        lpDest += nDestSize;
      *lpnSize += nDestSize;
    }
  }
  return S_OK;
}

static HRESULT TransOut_Byte(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    *lpDest = *lpSrc;
  *lpnDestSize = sizeof(BYTE);
  return S_OK;
}

static HRESULT TransOut_Word(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    *((USHORT NKT_UNALIGNED*)lpDest) = *((USHORT NKT_UNALIGNED*)lpSrc);
  *lpnDestSize = sizeof(USHORT);
  return S_OK;
}

static HRESULT TransOut_DWord(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    *((ULONG NKT_UNALIGNED*)lpDest) = *((ULONG NKT_UNALIGNED*)lpSrc);
  *lpnDestSize = sizeof(ULONG);
  return S_OK;
}

static HRESULT TransOut_QWord(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    *((ULONGLONG NKT_UNALIGNED*)lpDest) = *((ULONGLONG NKT_UNALIGNED*)lpSrc);
  *lpnDestSize = sizeof(ULONGLONG);
  return S_OK;
}

static HRESULT TransOut_Float(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    *((float NKT_UNALIGNED*)lpDest) = *((float NKT_UNALIGNED*)lpSrc);
  *lpnDestSize = sizeof(float);
  return S_OK;
}

static HRESULT TransOut_Double(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    *((double NKT_UNALIGNED*)lpDest) = *((double NKT_UNALIGNED*)lpSrc);
  *lpnDestSize = sizeof(double);
  return S_OK;
}

static HRESULT TransOut_Cy(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    *((double NKT_UNALIGNED*)lpDest) = (double)(((CY NKT_UNALIGNED*)lpSrc)->int64) / 10000.0;
  *lpnDestSize = sizeof(double);
  return S_OK;
}

static HRESULT TransOut_BStr(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  BSTR bstrVal;
  UINT nLen;

  bstrVal = *((BSTR NKT_UNALIGNED*)lpSrc);
  if (bstrVal == NULL)
    return E_POINTER;
  nLen = ::SysStringByteLen(bstrVal);
  if (lpDest != NULL)
  {
    STOREBUFFER(lpDest, nLen, ULONG);
    memcpy(lpDest, bstrVal, nLen);
  }
  *lpnDestSize = sizeof(ULONG) + (SIZE_T)nLen;
  return S_OK;
}

static HRESULT TransOut_Bool(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    *lpDest = ((*((VARIANT_BOOL NKT_UNALIGNED*)lpSrc)) != VARIANT_FALSE) ? 1 : 0;
  *lpnDestSize = sizeof(BYTE);
  return S_OK;

}
static HRESULT TransOut_Decimal(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize,
                                __in LPVOID lpUserData)
{
  if (lpDest != NULL)
    memcpy(lpDest, lpSrc, sizeof(DECIMAL));
  *lpnDestSize = sizeof(DECIMAL);
  return S_OK;
}

static HRESULT TransOut_Date(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize, __in LPVOID lpUserData)
{
  SYSTEMTIME sSysTime;
  double nTmpDbl;

  nTmpDbl = *((DOUBLE NKT_UNALIGNED*)lpSrc);
  if (::VariantTimeToSystemTime(nTmpDbl, &sSysTime) == FALSE)
    return E_INVALIDARG;
  if (lpDest != NULL)
    memcpy(lpDest, &sSysTime, sizeof(SYSTEMTIME));
  *lpnDestSize = sizeof(SYSTEMTIME);
  return S_OK;
}

static HRESULT TransOut_IUnknown(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize,
                                 __in LPVOID lpUserData)
{
  TNktComPtr<INktJavaObject> cJavaObj;
  IUnknown *punkVal;
  CNktJavaObjectImpl *lpImpl;
  ULONGLONG obj;
  HRESULT hRes;

  punkVal = *((IUnknown* NKT_UNALIGNED*)lpSrc);
  if (punkVal != NULL)
  {
    hRes = punkVal->QueryInterface(IID_INktJavaObject, (LPVOID*)&cJavaObj);
    if (FAILED(hRes))
      return hRes;
    lpImpl = static_cast<CNktJavaObjectImpl*>((INktJavaObject*)cJavaObj);
    if (lpImpl->GetPid() != *((DWORD*)lpUserData)) //check if the object belongs to the target process
      return E_INVALIDARG;
    obj = lpImpl->GetJObject();
  }
  else
  {
    obj = 0;
  }
  if (lpDest != NULL)
    *((ULONGLONG NKT_UNALIGNED*)lpDest) = obj;
  *lpnDestSize = sizeof(ULONGLONG);
  return S_OK;
}

static HRESULT TransOut_IDispatch(__in LPBYTE lpDest, __in LPBYTE lpSrc, __out SIZE_T *lpnDestSize,
                                  __in LPVOID lpUserData)
{
  TNktComPtr<INktJavaObject> cJavaObj;
  IDispatch *pdispVal;
  CNktJavaObjectImpl *lpImpl;
  ULONGLONG obj;
  HRESULT hRes;

  pdispVal = *((IDispatch* NKT_UNALIGNED*)lpSrc);
  if (pdispVal != NULL)
  {
    hRes = pdispVal->QueryInterface(IID_INktJavaObject, (LPVOID*)&cJavaObj);
    if (FAILED(hRes))
      return hRes;
    lpImpl = static_cast<CNktJavaObjectImpl*>((INktJavaObject*)cJavaObj);
    if (lpImpl->GetPid() != *((DWORD*)lpUserData)) //check if the object belongs to the target process
      return E_INVALIDARG;
    obj = lpImpl->GetJObject();
  }
  else
  {
    obj = 0;
  }
  if (lpDest != NULL)
    *((ULONGLONG NKT_UNALIGNED*)lpDest) = obj;
  *lpnDestSize = sizeof(ULONGLONG);
  return S_OK;
}
