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
#include "RemoteBridge.h"
#include "JavaVarConvert.h"
#include "JavaObject.h"

//-----------------------------------------------------------

#define READBUFFER(dest, lpSrc, nSrcBytes, _type)          \
         if (nSrcBytes < sizeof(_type))                    \
         {                                                 \
           _ASSERT(FALSE);                                 \
           return NKT_E_InvalidData;                       \
         }                                                 \
         dest = *((_type NKT_UNALIGNED*)lpSrc);            \
         lpSrc += sizeof(_type);                           \
         nSrcBytes -= sizeof(_type)

#define STOREBUFFER(lpDest, val, _type)                    \
         if (lpDest != NULL)                               \
         {                                                 \
           *((_type NKT_UNALIGNED*)lpDest) = (_type)(val); \
           lpDest += sizeof(_type);                        \
         }

#include "JavaVarConvert_TransposeArrayOut.h"
#include "JavaVarConvert_TransposeArrayIn.h"

//-----------------------------------------------------------

namespace JAVA {

//Converts a variant or 1D array of variants into a data stream.
//* An element can be a multidimensional array of a supported type
//* If an element (or array) is an IDispatch, it must be an INktJavaObject in order to send its "jobject id"
HRESULT VarConvert_MarshalSafeArray(__in SAFEARRAY *pArray, __in DWORD dwPid, __in LPBYTE lpDest, __inout SIZE_T &nSize)
{
  TNktComSafeArrayAccess<VARIANT> cArrayData;
  CNktComVariant cVtTemp;
  VARTYPE vType;
  VARIANT *lpVar;
  ULONG nCount;
  HRESULT hRes;

  nSize = 0;
  if (pArray == NULL)
  {
msa_emptyparams:
    STOREBUFFER(lpDest, 0, ULONG);
    nSize += sizeof(ULONG);
    return S_OK;
  }
  hRes = ::SafeArrayGetVartype(pArray, &vType);
  if (FAILED(hRes))
    return hRes;
  if (vType != VT_VARIANT || pArray->cDims != 1)
    return E_INVALIDARG;
  if (pArray->rgsabound[0].cElements == 0)
    goto msa_emptyparams;
  hRes = cArrayData.Setup(pArray);
  if (FAILED(hRes))
    return hRes;
  //store items count
  STOREBUFFER(lpDest, pArray->rgsabound[0].cElements, ULONG);
  nSize += sizeof(ULONG);
  //store each item
  for (nCount=0; nCount<pArray->rgsabound[0].cElements; nCount++)
  {
    lpVar = cArrayData.Get() + nCount;
    if (V_ISBYREF(lpVar))
    {
      ::VariantClear(&(cVtTemp.sVt));
      hRes = ::VariantCopyInd(&(cVtTemp.sVt), (VARIANTARG*)lpVar);
      if (FAILED(hRes))
        return hRes;
      lpVar = &(cVtTemp.sVt);
    }
    if (!V_ISARRAY(lpVar))
    {
      switch (lpVar->vt)
      {
        case VT_EMPTY:
        case VT_NULL:
          STOREBUFFER(lpDest, VT_NULL, BYTE);
          nSize += sizeof(BYTE);
          break;

        case VT_UI1:
          STOREBUFFER(lpDest, VT_UI1, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->bVal, BYTE);
          nSize += sizeof(BYTE);
          break;

        case VT_I1:
          STOREBUFFER(lpDest, VT_UI1, BYTE); //VB compliant use VT_UI1 instead of VT_I1
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->cVal, CHAR);
          nSize += sizeof(CHAR);
          break;

        case VT_I2:
          STOREBUFFER(lpDest, VT_I2, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->iVal, SHORT);
          nSize += sizeof(SHORT);
          break;

        case VT_UI2:
          STOREBUFFER(lpDest, VT_I2, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->uiVal, SHORT);
          nSize += sizeof(SHORT);
          break;

        case VT_I4:
        case VT_INT:
          STOREBUFFER(lpDest, VT_I4, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->lVal, LONG);
          nSize += sizeof(LONG);
          break;

        case VT_UI4:
        case VT_UINT:
          STOREBUFFER(lpDest, VT_I4, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->ulVal, LONG);
          nSize += sizeof(LONG);
          break;

        case VT_I8:
          STOREBUFFER(lpDest, VT_I8, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->llVal, LONGLONG);
          nSize += sizeof(LONGLONG);
          break;

        case VT_UI8:
          STOREBUFFER(lpDest, VT_I8, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->ullVal, LONGLONG);
          nSize += sizeof(LONGLONG);
          break;

        case VT_R4:
          STOREBUFFER(lpDest, VT_R4, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->fltVal, float);
          nSize += sizeof(float);
          break;

        case VT_R8:
          STOREBUFFER(lpDest, VT_R8, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, lpVar->dblVal, double);
          nSize += sizeof(double);
          break;

        case VT_CY:
          STOREBUFFER(lpDest, VT_R8, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, (double)(lpVar->cyVal.int64) / 10000.0, double);
          nSize += sizeof(double);
          break;

        case VT_BSTR:
          {
          UINT nLen;

          if (lpVar->bstrVal == NULL)
            return E_POINTER;
          nLen = ::SysStringByteLen(lpVar->bstrVal);
          STOREBUFFER(lpDest, VT_BSTR, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, nLen, ULONG);
          nSize += sizeof(ULONG);
          if (lpDest != NULL)
          {
            memcpy(lpDest, lpVar->bstrVal, nLen);
            lpDest += nLen;
          }
          nSize += nLen;
          }
          break;

        case VT_BOOL:
          STOREBUFFER(lpDest, VT_BOOL, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, (lpVar->boolVal != VARIANT_FALSE) ? 1 : 0, BYTE);
          nSize += sizeof(BYTE);
          break;

        case VT_DECIMAL:
          STOREBUFFER(lpDest, VT_DECIMAL, BYTE);
          nSize += sizeof(BYTE);
          if (lpDest != NULL)
          {
            memcpy(lpDest, &(lpVar->decVal), sizeof(lpVar->decVal));
            lpDest += sizeof(lpVar->decVal);
          }
          nSize += sizeof(lpVar->decVal);
          break;

        case VT_DATE:
          {
          SYSTEMTIME sSysTime;

          if (::VariantTimeToSystemTime(lpVar->date, &sSysTime) == FALSE)
            return E_INVALIDARG;
          STOREBUFFER(lpDest, VT_DATE, BYTE);
          nSize += sizeof(BYTE);
          if (lpDest != NULL)
          {
            memcpy(lpDest, &sSysTime, sizeof(SYSTEMTIME));
            lpDest += sizeof(sSysTime);
          }
          nSize += sizeof(sSysTime);
          }
          break;

        case VT_UNKNOWN:
          {
          TNktComPtr<INktJavaObject> cJavaObj;
          CNktJavaObjectImpl *lpImpl;
          ULONGLONG obj;

          if (lpVar->punkVal != NULL)
          {
            hRes = lpVar->punkVal->QueryInterface(IID_INktJavaObject, (LPVOID*)&cJavaObj);
            if (FAILED(hRes))
              return hRes;
            lpImpl = static_cast<CNktJavaObjectImpl*>((INktJavaObject*)cJavaObj);
            if (lpImpl->GetPid() != dwPid) //check if the object belongs to the target process
              return E_INVALIDARG;
            obj = lpImpl->GetJObject();
          }
          else
          {
            obj = 0;
          }
          STOREBUFFER(lpDest, VT_DISPATCH, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, obj, ULONGLONG);
          nSize += sizeof(ULONGLONG);
          }
          break;

        case VT_DISPATCH:
          {
          TNktComPtr<INktJavaObject> cJavaObj;
          CNktJavaObjectImpl *lpImpl;
          ULONGLONG obj;

          if (lpVar->pdispVal != NULL)
          {
            hRes = lpVar->pdispVal->QueryInterface(IID_INktJavaObject, (LPVOID*)&cJavaObj);
            if (FAILED(hRes))
              return hRes;
            lpImpl = static_cast<CNktJavaObjectImpl*>((INktJavaObject*)cJavaObj);
            if (lpImpl->GetPid() != dwPid) //check if the object belongs to the target process
              return E_INVALIDARG;
            obj = lpImpl->GetJObject();
          }
          else
          {
            obj = 0;
          }
          STOREBUFFER(lpDest, VT_DISPATCH, BYTE);
          nSize += sizeof(BYTE);
          STOREBUFFER(lpDest, obj, ULONGLONG);
          nSize += sizeof(ULONGLONG);
          }
          break;

        default:
          return E_INVALIDARG;
      }
    }
    else
    {
      TNktComSafeArrayAccess<BYTE> cData;
      SHORT nDim;
      BYTE cType, *lpType;

      if (lpVar->parray == NULL)
        return E_POINTER;
      hRes = ::SafeArrayGetVartype(lpVar->parray, &vType);
      if (FAILED(hRes))
        return hRes;
      if (lpVar->parray->cDims < 1 || lpVar->parray->cDims > 255)
        return E_INVALIDARG;
      //store array type
      lpType = (lpDest != NULL) ? lpDest : &cType;
      STOREBUFFER(lpDest, vType|0x80, BYTE);
      nSize += sizeof(BYTE);
      //store array dimensions (bounds are defined right-to-left
      STOREBUFFER(lpDest, lpVar->parray->cDims, BYTE);
      nSize += sizeof(BYTE);
      for (nDim=lpVar->parray->cDims; nDim>0; nDim--)
      {
        if (lpVar->parray->cDims > 1 && lpVar->parray->rgsabound[nDim-1].cElements == 0)
          return NKT_E_Unsupported;
        STOREBUFFER(lpDest, lpVar->parray->rgsabound[nDim-1].cElements, ULONG);
        nSize += sizeof(ULONG);
      }
      //access data
      hRes = cData.Setup(lpVar->parray);
      if (FAILED(hRes))
        return hRes;
      switch (V_VT(lpVar) & VT_TYPEMASK)
      {
        case VT_EMPTY:
        case VT_NULL:
          *lpType = (BYTE)(VT_NULL|0x80);
          hRes = S_OK;
          break;

        case VT_UI1:
        case VT_I1:
          *lpType = (BYTE)(VT_UI1|0x80); //VB compliant use VT_UI1 instead of VT_I1
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(BYTE), lpVar->parray, 0, &TransOut_Byte, &nSize,
                                    &dwPid);
          break;

        case VT_I2:
        case VT_UI2:
          *lpType = (BYTE)(VT_I2|0x80);
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(USHORT), lpVar->parray, 0, &TransOut_Word, &nSize,
                                    &dwPid);
          break;

        case VT_I4:
        case VT_INT:
        case VT_UI4:
        case VT_UINT:
          *lpType = (BYTE)(VT_I4|0x80);
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(ULONG), lpVar->parray, 0, &TransOut_DWord, &nSize,
                                    &dwPid);
          break;

        case VT_I8:
        case VT_UI8:
          *lpType = (BYTE)(VT_I8|0x80);
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(ULONGLONG), lpVar->parray, 0, &TransOut_QWord, &nSize,
                                    &dwPid);
          break;

        case VT_R4:
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(FLOAT), lpVar->parray, 0, &TransOut_Float, &nSize,
                                    &dwPid);
          break;

        case VT_R8:
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(DOUBLE), lpVar->parray, 0, &TransOut_Double, &nSize,
                                    &dwPid);
          break;

        case VT_CY:
          *lpType = (BYTE)(VT_R8|0x80);
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(CY), lpVar->parray, 0, &TransOut_Cy, &nSize, &dwPid);
          break;

        case VT_BSTR:
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(BSTR), lpVar->parray, 0, &TransOut_BStr, &nSize,
                                    &dwPid);
          break;

        case VT_BOOL:
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(VARIANT_BOOL), lpVar->parray, 0, &TransOut_Bool,
                                    &nSize, &dwPid);
          break;

        case VT_DECIMAL:
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(DECIMAL), lpVar->parray, 0, &TransOut_Decimal, &nSize,
                                    &dwPid);
          break;

        case VT_DATE:
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(DATE), lpVar->parray, 0, &TransOut_Date, &nSize,
                                    &dwPid);
          break;

        case VT_UNKNOWN:
          *lpType = (BYTE)(VT_DISPATCH|0x80);
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(IUnknown*), lpVar->parray, 0, &TransOut_IUnknown,
                                    &nSize, &dwPid);
          break;

        case VT_DISPATCH:
          hRes = TransposeArray_Out(lpDest, cData.Get(), sizeof(IDispatch*), lpVar->parray, 0, &TransOut_IDispatch,
                                    &nSize, &dwPid);
          break;

        default:
          hRes = E_INVALIDARG;
          break;
      }
      if (FAILED(hRes))
        return hRes;
      cData.Reset();
    }
  }
  return S_OK;
}

//Converts a stream into an array of variants.
//* If there is a jobject in the stream, we parse the definition and buuild new INktJavaObject's (or
//  reuse existing one (see (*env)->IsSameObject(env, obj1, obj2))
HRESULT VarConvert_UnmarshalVariants(__in LPBYTE lpSrc, __in SIZE_T nSrcBytes,
                                     __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData, __inout VARIANT &vt)
{
  CNktComVariant cVt;
  TNktAutoFreePtr<SAFEARRAYBOUND> cArrayBounds;
  VARTYPE varType;
  BYTE nCurrDim, nTotalDims;
  HRESULT hRes;

  ::VariantInit(&vt);
  //get type
  if (nSrcBytes < 1)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  varType = (VARTYPE)lpSrc[0];
  lpSrc++;
  nSrcBytes--;
  switch (varType)
  {
    case VT_NULL:
      if (nSrcBytes != 0)
      {
        _ASSERT(FALSE);
        return NKT_E_InvalidData;
      }
      V_VT(&vt) = varType;
      return S_OK;

    case VT_BOOL:
    case VT_UI1:
    case VT_I2:
    case VT_I4:
    case VT_I8:
    case VT_R4:
    case VT_R8:
    case VT_BSTR:
    case VT_DECIMAL:
    case VT_DATE:
    case VT_DISPATCH:
      break;

    default:
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
  }
  //get dimensions count and create array
  READBUFFER(nTotalDims, lpSrc, nSrcBytes, BYTE);
  if (nTotalDims == 0)
  {
    switch (varType)
    {
      case VT_BOOL:
        READBUFFER(cVt.sVt.bVal, lpSrc, nSrcBytes, BYTE);
        cVt.sVt.boolVal = (cVt.sVt.bVal != 0) ? VARIANT_TRUE : VARIANT_FALSE;
        break;

      case VT_UI1:
        READBUFFER(cVt.sVt.bVal, lpSrc, nSrcBytes, BYTE);
        break;

      case VT_I2:
        READBUFFER(cVt.sVt.iVal, lpSrc, nSrcBytes, SHORT);
        break;

      case VT_I4:
        READBUFFER(cVt.sVt.lVal, lpSrc, nSrcBytes, LONG);
        break;

      case VT_I8:
        READBUFFER(cVt.sVt.llVal, lpSrc, nSrcBytes, LONGLONG);
        break;

      case VT_R4:
        READBUFFER(cVt.sVt.fltVal, lpSrc, nSrcBytes, float);
        break;

      case VT_R8:
        READBUFFER(cVt.sVt.dblVal, lpSrc, nSrcBytes, double);
        break;

      case VT_BSTR:
        {
        ULONG nLen;

        READBUFFER(nLen, lpSrc, nSrcBytes, ULONG);
        if (nSrcBytes < (SIZE_T)nLen || (nLen & 1) != 0)
        {
          _ASSERT(FALSE);
          return NKT_E_InvalidData;
        }
        cVt.sVt.bstrVal = ::SysAllocStringLen((OLECHAR*)lpSrc, (UINT)nLen/(UINT)sizeof(WCHAR));
        if (cVt.sVt.bstrVal == NULL)
          return E_OUTOFMEMORY;
        lpSrc += (SIZE_T)nLen;
        nSrcBytes -= (SIZE_T)nLen;
        }
        break;

      case VT_DECIMAL:
        if (nSrcBytes < sizeof(DECIMAL))
        {
          _ASSERT(FALSE);
          return NKT_E_InvalidData;
        }
        memcpy(&(cVt.sVt.decVal), lpSrc, sizeof(DECIMAL));
        lpSrc += sizeof(DECIMAL);
        nSrcBytes -= sizeof(DECIMAL);
        break;

      case VT_DATE:
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
        if (::SystemTimeToVariantTime(&sSysTime, &(cVt.sVt.date)) == FALSE)
          return E_INVALIDARG;
        }
        break;

      case VT_DISPATCH:
        {
        TNktComPtr<INktJavaObject> cJavaObj;
        ULONGLONG obj;

        READBUFFER(obj, lpSrc, nSrcBytes, ULONGLONG);
        if (obj != 0)
        {
          hRes = (*(lpUnmarshalData->lpThis).*(lpUnmarshalData->lpfnFindOrCreateNktJavaObject))(
                               lpUnmarshalData->dwPid, obj, &cJavaObj);
          if (SUCCEEDED(hRes))
            hRes = cJavaObj->QueryInterface(IID_IDispatch, (void**)&(cVt.sVt.pdispVal));
          if (FAILED(hRes))
            return hRes;
        }
        else
        {
          cVt.sVt.pdispVal = NULL;
        }
        }
        break;
    }
    cVt.sVt.vt = varType;
  }
  else
  {
    TNktComSafeArrayAccess<BYTE> cData;
    SAFEARRAYBOUND *lpBounds;

    cArrayBounds.Attach((SAFEARRAYBOUND*)NKT_MALLOC((SIZE_T)nTotalDims * sizeof(SAFEARRAYBOUND)));
    if (cArrayBounds == NULL)
      return E_OUTOFMEMORY;
    lpBounds = cArrayBounds.Get();
    for (nCurrDim=0; nCurrDim<nTotalDims; nCurrDim++,lpBounds++)
    {
      lpBounds->lLbound = 0;
      READBUFFER(lpBounds->cElements, lpSrc, nSrcBytes, ULONG);
      if (nTotalDims > 1 && lpBounds->cElements == 0)
      {
         //this situation should have been handled by java marshalling from helper code
        _ASSERT(FALSE);
        return NKT_E_InvalidData;
      }
    }
    cVt.sVt.parray = ::SafeArrayCreate(varType, (UINT)nTotalDims, cArrayBounds.Get());
    if (cVt.sVt.parray == NULL)
      return E_OUTOFMEMORY;
    cVt.sVt.vt = varType | VT_ARRAY;
    //read elements
    hRes = cData.Setup(cVt.sVt.parray);
    if (FAILED(hRes))
      return hRes;
    switch (varType)
    {
      case VT_UI1:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(BYTE), cVt.sVt.parray, 0, &TransIn_Byte,
                                 lpUnmarshalData);
        break;

      case VT_I2:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(SHORT), cVt.sVt.parray, 0, &TransIn_Word,
                                 lpUnmarshalData);
        break;

      case VT_I4:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(LONG), cVt.sVt.parray, 0, &TransIn_DWord,
                                 lpUnmarshalData);
        break;

      case VT_I8:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(LONGLONG), cVt.sVt.parray, 0, &TransIn_QWord,
                                 lpUnmarshalData);
        break;

      case VT_R4:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(FLOAT), cVt.sVt.parray, 0, &TransIn_Float,
                                 lpUnmarshalData);
        break;

      case VT_R8:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(DOUBLE), cVt.sVt.parray, 0, &TransIn_Double,
                                 lpUnmarshalData);
        break;

      case VT_BSTR:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(BSTR), cVt.sVt.parray, 0, &TransIn_BStr,
                                 lpUnmarshalData);
        break;

      case VT_BOOL:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(VARIANT_BOOL), cVt.sVt.parray, 0, &TransIn_Bool,
                                 lpUnmarshalData);
        break;

      case VT_DECIMAL:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(DECIMAL), cVt.sVt.parray, 0, &TransIn_Decimal,
                                 lpUnmarshalData);
        break;

      case VT_DATE:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(DOUBLE), cVt.sVt.parray, 0, &TransIn_Date,
                                 lpUnmarshalData);
        break;

      case VT_DISPATCH:
        hRes = TransposeArray_In(lpSrc, nSrcBytes, cData.Get(), sizeof(LPDISPATCH), cVt.sVt.parray, 0,
                                 &TransIn_IDispatch, lpUnmarshalData);
        break;
    }
    if (FAILED(hRes))
      return hRes;
    cData.Reset();
  }
  //done
  memcpy(&vt, &(cVt.sVt), sizeof(VARIANT));
  memset(&(cVt.sVt), 0, sizeof(VARIANT));
  return S_OK;
}

} //JAVA
