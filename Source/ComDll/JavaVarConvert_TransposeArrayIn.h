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

typedef HRESULT (*lpfnTransform_In)(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                                    __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData);

//-----------------------------------------------------------

static HRESULT TransposeArray_In(__inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes, __in LPBYTE lpDest,
                                 __in SIZE_T nDestSize, __in SAFEARRAY *parray, __in USHORT nCurrDim,
                                 __in lpfnTransform_In fnTransform,
                                 __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  ULONG i, nCount;
  SIZE_T nDestMult;
  HRESULT hRes;

  nDestMult = 1;
  for (i=0; i<nCurrDim; i++)
    nDestMult *= (SIZE_T)(parray->rgsabound[parray->cDims-i-1].cElements);
  //----
  nCount = parray->rgsabound[parray->cDims-nCurrDim-1].cElements;
  if (nCurrDim < parray->cDims-1)
  {
    for (i=0; i<nCount; i++)
    {
      hRes = TransposeArray_In(lpSrc, nSrcBytes, lpDest, nDestSize, parray, nCurrDim+1, fnTransform, lpUnmarshalData);
      if (FAILED(hRes))
        return hRes;
      lpDest += nDestSize * nDestMult;
    }
  }
  else
  {
    for (i=0; i<nCount; i++)
    {
      hRes = fnTransform(lpDest, lpSrc, nSrcBytes, lpUnmarshalData);
      if (FAILED(hRes))
        return hRes;
      lpDest += nDestSize * nDestMult;
    }
  }
  return S_OK;
}

static HRESULT TransIn_Byte(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                            __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  if (nSrcBytes < sizeof(BYTE))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  *lpDest = *lpSrc;
  lpSrc++;
  nSrcBytes--;
  return S_OK;
}

static HRESULT TransIn_Word(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                            __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  if (nSrcBytes < sizeof(SHORT))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  *((SHORT NKT_UNALIGNED*)lpDest) = *((SHORT NKT_UNALIGNED*)lpSrc);
  lpSrc += sizeof(SHORT);
  nSrcBytes -= sizeof(SHORT);
  return S_OK;
}

static HRESULT TransIn_DWord(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                             __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  if (nSrcBytes < sizeof(LONG))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  *((LONG NKT_UNALIGNED*)lpDest) = *((LONG NKT_UNALIGNED*)lpSrc);
  lpSrc += sizeof(LONG);
  nSrcBytes -= sizeof(LONG);
  return S_OK;
}

static HRESULT TransIn_QWord(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                             __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  if (nSrcBytes < sizeof(LONGLONG))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  *((LONGLONG NKT_UNALIGNED*)lpDest) = *((LONGLONG NKT_UNALIGNED*)lpSrc);
  lpSrc += sizeof(LONGLONG);
  nSrcBytes -= sizeof(LONGLONG);
  return S_OK;
}

static HRESULT TransIn_Float(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                             __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  if (nSrcBytes < sizeof(float))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  *((float NKT_UNALIGNED*)lpDest) = *((float NKT_UNALIGNED*)lpSrc);
  lpSrc += sizeof(float);
  nSrcBytes -= sizeof(float);
  return S_OK;
}

static HRESULT TransIn_Double(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                              __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  if (nSrcBytes < sizeof(double))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  *((double NKT_UNALIGNED*)lpDest) = *((double NKT_UNALIGNED*)lpSrc);
  lpSrc += sizeof(double);
  nSrcBytes -= sizeof(double);
  return S_OK;
}


static HRESULT TransIn_BStr(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                            __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  BSTR bstrVal;
  ULONG nLen;

  READBUFFER(nLen, lpSrc, nSrcBytes, ULONG);
  if (nSrcBytes < (SIZE_T)nLen || (nLen & 1) != 0)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  bstrVal = ::SysAllocStringLen((OLECHAR*)lpSrc, (UINT)nLen/(UINT)sizeof(WCHAR));
  if (bstrVal == NULL)
    return E_OUTOFMEMORY;
  lpSrc += (SIZE_T)nLen;
  nSrcBytes -= (SIZE_T)nLen;
  *((BSTR NKT_UNALIGNED*)lpDest) = bstrVal;
  return S_OK;
}

static HRESULT TransIn_Bool(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                            __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  if (nSrcBytes < sizeof(BYTE))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  *((VARIANT_BOOL NKT_UNALIGNED*)lpDest) = (lpSrc[0] != 0) ? VARIANT_TRUE : VARIANT_FALSE;
  lpSrc++;
  nSrcBytes--;
  return S_OK;
}

static HRESULT TransIn_Decimal(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                               __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  if (nSrcBytes < sizeof(DECIMAL))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  memcpy(lpDest, lpSrc, sizeof(DECIMAL));
  lpSrc += sizeof(DECIMAL);
  nSrcBytes -= sizeof(DECIMAL);
  return S_OK;
}

static HRESULT TransIn_Date(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                            __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  SYSTEMTIME sSysTime;

  if (nSrcBytes < sizeof(SYSTEMTIME))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  memcpy(&sSysTime, lpSrc, sizeof(SYSTEMTIME));
  lpSrc += sizeof(SYSTEMTIME);
  nSrcBytes -= sizeof(SYSTEMTIME);
  if (::SystemTimeToVariantTime(&sSysTime, (DOUBLE*)lpDest) == FALSE)
    return E_INVALIDARG;
  return S_OK;
}

static HRESULT TransIn_IDispatch(__in LPBYTE lpDest, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                                 __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData)
{
  TNktComPtr<INktJavaObject> cJavaObj;
  ULONGLONG obj;
  IDispatch *lpDisp;
  HRESULT hRes;

  lpDisp = NULL;
  READBUFFER(obj, lpSrc, nSrcBytes, ULONGLONG);
  if (obj != 0)
  {
    hRes = (*(lpUnmarshalData->lpThis).*(lpUnmarshalData->lpfnFindOrCreateNktJavaObject))(
                         lpUnmarshalData->dwPid, obj, &cJavaObj);
    if (SUCCEEDED(hRes))
      hRes = cJavaObj.QueryInterface<IDispatch>(&lpDisp);
    if (FAILED(hRes))
      return hRes;
  }
  *((IDispatch NKT_UNALIGNED**)lpDest) = lpDisp;
  return S_OK;
}
