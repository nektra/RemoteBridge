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

#include "VariantMisc.h"
#include <crtdbg.h>
#include "..\ComPtr.h"

//-----------------------------------------------------------

BOOL NumericFromVariant(__in VARIANT *lpVar, __out PULONGLONG lpnValue, __out_opt LPBOOL lpbIsSigned)
{
  _ASSERT(lpnValue != NULL);
  if (lpVar == NULL)
    return FALSE;
  switch (lpVar->vt)
  {
    case VT_I1:
      *lpnValue = (ULONGLONG)(LONGLONG)(lpVar->cVal);
      break;
    case VT_I2:
      *lpnValue = (ULONGLONG)(LONGLONG)(lpVar->iVal);
      break;
    case VT_I4:
      *lpnValue = (ULONGLONG)(LONGLONG)(lpVar->lVal);
      break;
    case VT_I8:
      *lpnValue = (ULONGLONG)(lpVar->llVal);
      break;
    case VT_INT:
      *lpnValue = (ULONGLONG)(LONGLONG)(lpVar->intVal);
      break;
    case VT_UI1:
      *lpnValue = (ULONGLONG)(lpVar->bVal);
      break;
    case VT_UI2:
      *lpnValue = (ULONGLONG)(lpVar->uiVal);
      break;
    case VT_UI4:
      *lpnValue = (ULONGLONG)(lpVar->ulVal);
      break;
    case VT_UI8:
      *lpnValue = (lpVar->ullVal);
      break;
    case VT_UINT:
      *lpnValue = (ULONGLONG)(lpVar->uintVal);
      break;
    default:
      return FALSE;
  }
  //check if signed
  if (lpbIsSigned != NULL)
  {
    switch (lpVar->vt)
    {
      case VT_I1:
      case VT_I2:
      case VT_I4:
      case VT_I8:
      case VT_INT:
        *lpbIsSigned = TRUE;
        break;
      case VT_UI1:
      case VT_UI2:
      case VT_UI4:
      case VT_UI8:
      case VT_UINT:
        *lpbIsSigned = FALSE;
        break;
    }
  }
  return TRUE;
}

HRESULT CheckInputVariantAndConvertToSafeArray(__out VARIANT *lpVarOut, __in VARIANT *lpVarIn, __in BOOL bConvertNull)
{
  CNktComVariant cVt, cVt2;
  HRESULT hRes;

  ::VariantInit(lpVarOut);
  //do de-referencing if needed
  hRes = ::VariantCopyInd(&(cVt.sVt), (VARIANTARG*)lpVarIn);
  if (FAILED(hRes))
    return hRes;
  //process special cases
  if (cVt.sVt.vt == VT_NULL || cVt.sVt.vt == VT_EMPTY)
  {
    if (bConvertNull == FALSE)
    {
      cVt.Detach(lpVarOut);
      return S_OK;
    }
    //force null
    cVt.sVt.vt = VT_NULL;
  }
  //uncommonly used vt_vector are not supported
  if (V_ISVECTOR(&(cVt.sVt)))
    return E_NOTIMPL;
  //if empty/null w
  //check if already an array
  if (V_ISARRAY(&(cVt.sVt)))
  {
    TNktComSafeArrayAccess<BYTE> cSrcData;
    ULONG i, nElemsCount;
    VARIANT vtTemp;
    LPBYTE lpSrc, lpDest;
    SIZE_T nElemSize;

    //null array?
    if (cVt.sVt.parray == NULL)
      return E_NOTIMPL;
    //dimensions count must be 1
    if (cVt.sVt.parray->cDims != 1)
      return E_INVALIDARG;
    //get array type
    memset(&vtTemp, 0, sizeof(vtTemp));
    vtTemp.vt = V_VT(&(cVt.sVt)) & VT_TYPEMASK;
    //already an array of variants?
    if (vtTemp.vt == VT_VARIANT)
    {
      cVt.Detach(lpVarOut);
      return S_OK;
    }
    //we have to convert an array of ? to an array of variants
    nElemSize = (SIZE_T)::SafeArrayGetElemsize(cVt.sVt.parray);
    if (nElemSize == 0)
      return E_NOTIMPL;
    //create destination array
    nElemsCount = cVt.sVt.parray->rgsabound[0].cElements;
    cVt2.sVt.parray = ::SafeArrayCreateVector(VT_VARIANT, 0, nElemsCount);
    if (cVt2.sVt.parray == NULL)
      return E_OUTOFMEMORY;
    cVt2.sVt.vt = VT_VARIANT|VT_ARRAY;
    //do conversion
    hRes = cSrcData.Setup(cVt.sVt.parray);
    if (FAILED(hRes))
      return hRes;
    lpDest = (vtTemp.vt != VT_DECIMAL) ? (LPBYTE)&(vtTemp.lVal) : (LPBYTE)&(vtTemp.decVal);
    //process each element
    lpSrc = cSrcData.Get();
    for (i=0; i<nElemsCount; i++, lpSrc+=nElemSize)
    {
      memcpy(lpDest, lpSrc, nElemSize);
      hRes = ::SafeArrayPutElement(cVt2.sVt.parray, (LONG*)&i, &vtTemp);
      if (FAILED(hRes))
        return hRes;
    }
  }
  else
  {
    LONG aIndices;

    //put the single element inside an array
    cVt2.sVt.parray = ::SafeArrayCreateVector(VT_VARIANT, 0, 1);
    if (cVt2.sVt.parray == NULL)
      return E_OUTOFMEMORY;
    cVt2.sVt.vt = VT_VARIANT|VT_ARRAY;
    aIndices = 0;
    hRes = ::SafeArrayPutElement(cVt2.sVt.parray, &aIndices, &(cVt.sVt));
    if (FAILED(hRes))
      return hRes;
  }
  //done
  cVt2.Detach(lpVarOut);
  return S_OK;
}
