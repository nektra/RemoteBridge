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

#include "..\MainManager.h"
#include "JavaManager.h"
#include "JavaVarConvert.h"
#include "..\..\Common\AutoPtr.h"

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

#define CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, _type)        \
         if (nSrcBytes < sizeof(_type))                    \
         {                                                 \
           _ASSERT(FALSE);                                 \
           return NKT_E_InvalidData;                       \
         }                                                 \
         lpSrc += sizeof(_type);                           \
         nSrcBytes -= sizeof(_type)

#define STOREBUFFER(lpDest, val, _type)                    \
         if (lpDest != NULL)                               \
         {                                                 \
           *((_type NKT_UNALIGNED*)lpDest) = (_type)(val); \
           lpDest += sizeof(_type);                        \
         }

//-----------------------------------------------------------

#include "JavaVarConvert_UnmarshalHelpers.h"
#include "JavaVarConvert_MarshalHelpers.h"
#include "JavaVarConvert_UnmarshalledJValues.h"

//-----------------------------------------------------------

namespace JAVA {

//Convert a jvalue into a data stream
//* It can be a single value or an array of a defined type
//* If it is a jobject or an array of jobjects, we add the definition of that jobject (fields, methods, etc.)
HRESULT VarConvert_MarshalJValue(__in JNIEnv *env, __in jvalue val, __in CJValueDef *lpJValueDef,
                                 __in CJavaVariantInterchange *lpJavaVarInterchange,
                                 __in CJavaObjectManager *lpJObjectMgr, __in LPBYTE lpDest, __inout SIZE_T &nSize)
{
  HRESULT hRes;

  nSize = 0;
  //store type
  switch (lpJValueDef->nType)
  {
    case JAVA::CJValueDef::typVoid:
      STOREBUFFER(lpDest, (BYTE)VT_NULL, BYTE);
      break;

    case JAVA::CJValueDef::typBoolean:
    case JAVA::CJValueDef::typBooleanObj:
      STOREBUFFER(lpDest, (BYTE)VT_BOOL, BYTE);
      break;

    case JAVA::CJValueDef::typByte:
    case JAVA::CJValueDef::typByteObj:
      STOREBUFFER(lpDest, (BYTE)VT_UI1, BYTE); //VB compliant use VT_UI1 instead of VT_I1
      break;

    case JAVA::CJValueDef::typChar:
    case JAVA::CJValueDef::typCharObj:
    case JAVA::CJValueDef::typShort:
    case JAVA::CJValueDef::typShortObj:
      STOREBUFFER(lpDest, (BYTE)VT_I2, BYTE);
      break;

    case JAVA::CJValueDef::typInt:
    case JAVA::CJValueDef::typIntObj:
    case JAVA::CJValueDef::typAtomicIntObj:
      STOREBUFFER(lpDest, (BYTE)VT_I4, BYTE);
      break;

    case JAVA::CJValueDef::typLong:
    case JAVA::CJValueDef::typLongObj:
    case JAVA::CJValueDef::typAtomicLongObj:
      STOREBUFFER(lpDest, (BYTE)VT_I8, BYTE);
      break;

    case JAVA::CJValueDef::typFloat:
    case JAVA::CJValueDef::typFloatObj:
      STOREBUFFER(lpDest, (BYTE)VT_R4, BYTE);
      break;

    case JAVA::CJValueDef::typDouble:
    case JAVA::CJValueDef::typDoubleObj:
      STOREBUFFER(lpDest, (BYTE)VT_R8, BYTE);
      break;

    case JAVA::CJValueDef::typString:
      STOREBUFFER(lpDest, (BYTE)VT_BSTR, BYTE);
      break;

    case JAVA::CJValueDef::typBigDecimal:
      STOREBUFFER(lpDest, (BYTE)VT_DECIMAL, BYTE);
      break;

    case JAVA::CJValueDef::typDate:
    case JAVA::CJValueDef::typCalendar:
      STOREBUFFER(lpDest, (BYTE)VT_DATE, BYTE);
      break;

    case JAVA::CJValueDef::typObject:
      STOREBUFFER(lpDest, (BYTE)VT_DISPATCH, BYTE);
      break;

    default:
      _ASSERT(FALSE);
      hRes = E_NOTIMPL;
      break;
  }
  nSize += sizeof(BYTE);
  if (lpJValueDef->nType == JAVA::CJValueDef::typVoid)
    return S_OK; //if void we are done
  //store dimensions count
  if (lpJValueDef->nArrayDims > 255)
  {
    _ASSERT(FALSE);
    return E_NOTIMPL;
  }
  STOREBUFFER(lpDest, lpJValueDef->nArrayDims, BYTE);
  nSize += sizeof(BYTE);
  //if we deal with an array of something, process them. On multidimensional arrays, each subarray must be the same size
  if (lpJValueDef->nArrayDims > 0)
  {
    TNktAutoFreePtr<ULONG> aArraySizes;
    jsize _tempSiz;

    aArraySizes.Attach((ULONG*)NKT_MALLOC((SIZE_T)(lpJValueDef->nArrayDims) * sizeof(ULONG)));
    if (aArraySizes == NULL)
      return E_OUTOFMEMORY;
    //store array size
    _tempSiz = env->GetArrayLength((jarray)(val.l));
    if (_tempSiz == 0)
    {
      hRes = CheckJavaException(env);
      if (FAILED(hRes))
        return hRes;
      if (lpJValueDef->nArrayDims > 1)
        return NKT_E_Unsupported;
    }
    else if (_tempSiz > (jsize)0x7FFFFFFF)
    {
      _ASSERT(FALSE);
      return E_NOTIMPL;
    }
    (aArraySizes.Get())[0] = (ULONG)_tempSiz;
    //store deeper array sizes
    if (lpJValueDef->nArrayDims > 1)
    {
      JAVA::TJavaAutoPtr<jobject> cObj, cChildObj;
      ULONG k;

      for (k=0; k<lpJValueDef->nArrayDims-1; k++)
      {
        cChildObj.Release();
        cChildObj.Attach(env, env->GetObjectArrayElement((k == 0) ? (jobjectArray)val.l : (jobjectArray)cObj.Get(), 0));
        if (cChildObj == NULL)
          return E_OUTOFMEMORY;
        _tempSiz = env->GetArrayLength((jarray)cChildObj.Get());
        if (_tempSiz == 0)
        {
          hRes = JAVA::CheckJavaException(env);
          return (FAILED(hRes)) ? hRes : NKT_E_Unsupported;
        }
        else if (_tempSiz > (jsize)0x7FFFFFFF)
        {
          _ASSERT(FALSE);
          return E_NOTIMPL;
        }
        (aArraySizes.Get())[k+1] = (ULONG)_tempSiz;
        //---
        cObj.Release();
        cObj.Attach(env, cChildObj.Detach());
      }
    }
    if (lpDest != NULL)
    {
      memcpy(lpDest, aArraySizes.Get(), (SIZE_T)(lpJValueDef->nArrayDims) * sizeof(ULONG));
      lpDest += (SIZE_T)(lpJValueDef->nArrayDims) * sizeof(ULONG);
    }
    nSize += (SIZE_T)(lpJValueDef->nArrayDims) * sizeof(ULONG);
    //process array
    hRes = VarConvert_MarshalJValue_ProcessArray(env, val.l, aArraySizes.Get(), 0, lpJValueDef, lpJavaVarInterchange,
                                                 lpJObjectMgr, lpDest, nSize);
    if (FAILED(hRes))
      return hRes;
  }
  else
  {
    switch (lpJValueDef->nType)
    {
      case JAVA::CJValueDef::typVoid:
        break;

      case JAVA::CJValueDef::typBoolean:
        STOREBUFFER(lpDest, (val.z != JNI_FALSE) ? 1 : 0, BYTE);
        nSize++;
        break;

      case JAVA::CJValueDef::typByte:
        STOREBUFFER(lpDest, val.b, BYTE);
        nSize++;
        break;

      case JAVA::CJValueDef::typChar:
        STOREBUFFER(lpDest, val.c, USHORT);
        nSize += sizeof(USHORT);
        break;

      case JAVA::CJValueDef::typShort:
        STOREBUFFER(lpDest, val.s, USHORT);
        nSize += sizeof(USHORT);
        break;

      case JAVA::CJValueDef::typInt:
        STOREBUFFER(lpDest, val.i, ULONG);
        nSize += sizeof(ULONG);
        break;

      case JAVA::CJValueDef::typLong:
        STOREBUFFER(lpDest, val.j, ULONGLONG);
        nSize += sizeof(ULONGLONG);
        break;

      case JAVA::CJValueDef::typFloat:
        STOREBUFFER(lpDest, val.f, float);
        nSize += sizeof(float);
        break;

      case JAVA::CJValueDef::typDouble:
        STOREBUFFER(lpDest, val.d, double);
        nSize += sizeof(double);
        break;

      case JAVA::CJValueDef::typString:
        {
        LPWSTR sW;
        jboolean isCopy;
        ULONG nLen;

        if (val.l == NULL)
        {
          _ASSERT(FALSE);
          return E_NOTIMPL;
        }
        nLen = (ULONG)((SIZE_T)(env->GetStringLength((jstring)(val.l))) * sizeof(WCHAR)); //before string critical
        sW = (LPWSTR)(env->GetStringCritical((jstring)(val.l), &isCopy));
        if (sW == NULL)
          return E_OUTOFMEMORY;
        STOREBUFFER(lpDest, nLen, ULONG);
        nSize += sizeof(ULONG);
        if (lpDest != NULL)
        {
          memcpy(lpDest, sW, (SIZE_T)nLen);
          lpDest += (SIZE_T)nLen;
        }
        nSize += (SIZE_T)nLen;
        env->ReleaseStringCritical((jstring)(val.l), (jchar*)sW);
        }
        break;

      case JAVA::CJValueDef::typBigDecimal:
        if (val.l == NULL)
        {
          _ASSERT(FALSE);
          return E_NOTIMPL;
        }
        if (lpDest != NULL)
        {
          hRes = lpJavaVarInterchange->JObjectToDecimal(env, val.l, (DECIMAL*)lpDest);
          if (FAILED(hRes))
            return hRes;
          lpDest += sizeof(DECIMAL);
        }
        nSize += sizeof(DECIMAL);
        break;

      case JAVA::CJValueDef::typDate:
      case JAVA::CJValueDef::typCalendar:
        if (val.l == NULL)
        {
          _ASSERT(FALSE);
          return E_NOTIMPL;
        }
        if (lpDest != NULL)
        {
          hRes = lpJavaVarInterchange->JObjectToSystemTime(env, val.l, (SYSTEMTIME*)lpDest);
          if (FAILED(hRes))
            return hRes;
          lpDest += sizeof(SYSTEMTIME);
        }
        nSize += sizeof(SYSTEMTIME);
        break;

      case JAVA::CJValueDef::typObject:
        {
        ULONGLONG ullObj;
        jobject globalObj;

        ullObj = 0;
        if (val.l != NULL)
        {
          hRes = lpJObjectMgr->Add(env, val.l, &globalObj);
          if (FAILED(hRes))
            return hRes;
          ullObj = NKT_PTR_2_ULONGLONG(globalObj);
        }
        STOREBUFFER(lpDest, ullObj, ULONGLONG);
        nSize += sizeof(ULONGLONG);
        }
        break;

      case JAVA::CJValueDef::typBooleanObj:
        {
        jboolean z;

        hRes = lpJavaVarInterchange->JavaLangBooleanToBoolean(env, val.l, &z);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, (val.z != JNI_FALSE) ? 1 : 0, BYTE);
        nSize++;
        }
        break;

      case JAVA::CJValueDef::typByteObj:
        {
        jbyte b;

        hRes = lpJavaVarInterchange->JavaLangByteToByte(env, val.l, &b);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.b, BYTE);
        nSize++;
        }
        break;

      case JAVA::CJValueDef::typCharObj:
        {
        jchar c;

        hRes = lpJavaVarInterchange->JavaLangCharacterToChar(env, val.l, &c);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.c, USHORT);
        nSize += sizeof(USHORT);
        }
        break;

      case JAVA::CJValueDef::typShortObj:
        {
        jshort s;

        hRes = lpJavaVarInterchange->JavaLangShortToShort(env, val.l, &s);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.s, USHORT);
        nSize += sizeof(USHORT);
        }
        break;

      case JAVA::CJValueDef::typIntObj:
        {
        jint i;

        hRes = lpJavaVarInterchange->JavaLangIntegerToInteger(env, val.l, &i);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.i, ULONG);
        nSize += sizeof(ULONG);
        }
        break;

      case JAVA::CJValueDef::typLongObj:
        {
        jlong j;

        hRes = lpJavaVarInterchange->JavaLangLongToLong(env, val.l, &j);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.j, ULONGLONG);
        nSize += sizeof(ULONGLONG);
        }
        break;

      case JAVA::CJValueDef::typFloatObj:
        {
        jfloat f;

        hRes = lpJavaVarInterchange->JavaLangFloatToFloat(env, val.l, &f);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.f, float);
        nSize += sizeof(float);
        }
        break;

      case JAVA::CJValueDef::typDoubleObj:
        {
        jdouble d;

        hRes = lpJavaVarInterchange->JavaLangDoubleToDouble(env, val.l, &d);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.d, double);
        nSize += sizeof(double);
        }
        break;

      case JAVA::CJValueDef::typAtomicIntObj:
        {
        jint i;

        hRes = lpJavaVarInterchange->JavaUtilConcurrentAtomicIntegerToInteger(env, val.l, &i);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.i, ULONG);
        nSize += sizeof(ULONG);
        }
        break;

      case JAVA::CJValueDef::typAtomicLongObj:
        {
        jlong j;

        hRes = lpJavaVarInterchange->JavaUtilConcurrentAtomicLongToLong(env, val.l, &j);
        if (FAILED(hRes))
          return hRes;
        STOREBUFFER(lpDest, val.j, ULONGLONG);
        nSize += sizeof(ULONGLONG);
        }
        break;
    }
  }
  return S_OK;
}

HRESULT VarConvert_UnmarshalJValues_PeekTypes(__in JNIEnv *env, __in LPBYTE lpSrc, __in SIZE_T nSrcBytes,
                                              __in CJavaObjectManager *lpJObjectMgr,
                                              __out CUnmarshalledJValues **lpValues)
{
  TNktAutoDeletePtr<CUnmarshalledJValues> cUnmJV;
  ULONG nJValuesCount;
  BYTE nVarType;
  jvalue varValue;
  HRESULT hRes;

  _ASSERT(env != NULL);
  _ASSERT(lpSrc != NULL);
  _ASSERT(lpValues != NULL);
  *lpValues = NULL;
  cUnmJV.Attach(new CUnmarshalledJValues(env));
  if (cUnmJV == NULL)
    return E_OUTOFMEMORY;
  //read items count
  READBUFFER(nJValuesCount, lpSrc, nSrcBytes, ULONG);
  //read items
  while (nJValuesCount > 0)
  {
    memset(&varValue, 0, sizeof(varValue));
    //get type
    READBUFFER(nVarType, lpSrc, nSrcBytes, BYTE);
    if ((nVarType & 0x80) == 0)
    {
      switch (nVarType)
      {
        case VT_NULL:
          hRes = cUnmJV->Add(varValue, CJValueDef::typObject, 0);
          break;

        case VT_UI1: //VB compliant use VT_UI1 instead of VT_I1
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, jbyte);
          hRes = cUnmJV->Add(varValue, CJValueDef::typByte, 0);
          break;

        case VT_I2:
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, jshort);
          hRes = cUnmJV->Add(varValue, CJValueDef::typShort, 0);
          break;

        case VT_I4:
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, jint);
          hRes = cUnmJV->Add(varValue, CJValueDef::typInt, 0);
          break;

        case VT_I8:
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, jlong);
          hRes = cUnmJV->Add(varValue, CJValueDef::typLong, 0);
          break;

        case VT_R4:
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, jfloat);
          hRes = cUnmJV->Add(varValue, CJValueDef::typFloat, 0);
          break;

        case VT_R8:
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, jdouble);
          hRes = cUnmJV->Add(varValue, CJValueDef::typDouble, 0);
          break;

        case VT_BSTR:
          {
          ULONG nLen;

          READBUFFER(nLen, lpSrc, nSrcBytes, ULONG);
          if (nSrcBytes < nLen)
            return E_INVALIDARG;
          hRes = cUnmJV->Add(varValue, CJValueDef::typString, 0);
          if (SUCCEEDED(hRes))
          {
            lpSrc += nLen;
            nSrcBytes -= nLen;
          }
          }
          break;

        case VT_BOOL:
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, BYTE);
          hRes = cUnmJV->Add(varValue, CJValueDef::typBoolean, 0);
          break;

        case VT_DECIMAL:
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, DECIMAL);
          hRes = cUnmJV->Add(varValue, CJValueDef::typBigDecimal, 0);
          break;

        case VT_DATE:
          CHECKANDSKIPBUFFER(lpSrc, nSrcBytes, SYSTEMTIME);
          hRes = cUnmJV->Add(varValue, CJValueDef::typDate, 0);
          break;

        case VT_DISPATCH:
          {
          ULONGLONG obj;

          READBUFFER(obj, lpSrc, nSrcBytes, ULONGLONG);
          if (obj != 0)
          {
            hRes = lpJObjectMgr->GetLocalRef(env, (jobject)obj, &(varValue.l));
          }
          else
          {
            hRes = S_OK;
          }
          if (SUCCEEDED(hRes))
            hRes = cUnmJV->Add(varValue, CJValueDef::typObject, 0);
          if (FAILED(hRes) && varValue.l != NULL)
            env->DeleteLocalRef(varValue.l);
          }
          break;

        default:
          hRes = E_INVALIDARG;
          break;
      }
    }
    else
    {
      TNktAutoDeletePtr<ULONG> cBounds;
      BYTE nTotalDims;
      CJValueDef::eType nType;
      SIZE_T nDim;

      nVarType &= ~0x80;
      //read dimensions count
      READBUFFER(nTotalDims, lpSrc, nSrcBytes, BYTE);
      if (nTotalDims == 0)
        return E_INVALIDARG;
      //read bounds
      cBounds.Attach((ULONG*)NKT_MALLOC((SIZE_T)nTotalDims*sizeof(ULONG)));
      if (cBounds == NULL)
        return E_OUTOFMEMORY;
      for (nDim=0; nDim<(SIZE_T)nTotalDims; nDim++)
      {
        READBUFFER(cBounds[nDim], lpSrc, nSrcBytes, ULONG);
        if (nTotalDims > 1 && cBounds[nDim] == 0)
        {
          //this situation should have been handled by variant marshalling from com code
          _ASSERT(FALSE);
          return NKT_E_InvalidData;
        }
      }
      hRes = UnmarshalJValuesArray_PeekTypes_In(env, lpSrc, nSrcBytes, 0, nTotalDims, cBounds, lpJObjectMgr,
                                                nVarType, &varValue);
      if (SUCCEEDED(hRes))
      {
        switch (nVarType)
        {
          case VT_NULL:
          case VT_DISPATCH:
            nType = CJValueDef::typObject;
            break;

          case VT_UI1: //VB compliant use VT_UI1 instead of VT_I1
            nType = CJValueDef::typByte;
            break;

          case VT_I2:
            nType = CJValueDef::typShort;
            break;

          case VT_I4:
            nType = CJValueDef::typInt;
            break;

          case VT_I8:
            nType = CJValueDef::typLong;
            break;

          case VT_R4:
            nType = CJValueDef::typFloat;
            break;

          case VT_R8:
            nType = CJValueDef::typDouble;
            break;

          case VT_BSTR:
            nType = CJValueDef::typString;
            break;

          case VT_BOOL:
            nType = CJValueDef::typBoolean;
            break;

          case VT_DECIMAL:
            nType = CJValueDef::typBigDecimal;
            break;

          case VT_DATE:
            nType = CJValueDef::typDate;
            break;

          default:
            return E_INVALIDARG;
        }
        hRes = cUnmJV->Add(varValue, nType, nTotalDims);
      }
      if (FAILED(hRes) && nVarType == VT_DISPATCH && varValue.l != NULL)
        env->DeleteLocalRef(varValue.l);
    }
    if (FAILED(hRes))
      return hRes;
    nJValuesCount--;
  }
  //done
  *lpValues = cUnmJV.Detach();
  return S_OK;
}

//Converts a stream into an array of jvalues.
//* An array is basically a jobject and we only need the root to access or delete it
//* Also we return an array of bytes indicating the type of each jvalue
HRESULT VarConvert_UnmarshalJValues(__in JNIEnv *env, __in LPBYTE lpSrc, __in SIZE_T nSrcBytes,
                                    __in CJavaVariantInterchange *lpJavaVarInterchange,
                                    __in CJavaObjectManager *lpJObjectMgr, __in CMethodOrFieldDef *lpMethodDef,
                                    __in BOOL bParseResult, __out CUnmarshalledJValues **lpValues)
{
  TNktAutoDeletePtr<CUnmarshalledJValues> cUnmJV;
  TNktLnkLst<CJValueDef>::Iterator itJVal;
  CJValueDef *lpCurrJValue;
  ULONG nJValuesCount;
  BYTE nVarType;
  jvalue varValue;
  HRESULT hRes;

  _ASSERT(env != NULL);
  _ASSERT(lpSrc != NULL);
  _ASSERT(lpValues != NULL);
  *lpValues = NULL;
  cUnmJV.Attach(new CUnmarshalledJValues(env));
  if (cUnmJV == NULL)
    return E_OUTOFMEMORY;
  //read items count
  READBUFFER(nJValuesCount, lpSrc, nSrcBytes, ULONG);
  //read items
  lpCurrJValue = (bParseResult == FALSE) ? (itJVal.Begin(lpMethodDef->cJValueDefParams)) :
                                           &(lpMethodDef->cJValueDefReturn);
  while (lpCurrJValue != NULL)
  {
    if ((nJValuesCount--) == 0)
    {
      _ASSERT(FALSE);
      return E_INVALIDARG;
    }
    //get type
    READBUFFER(nVarType, lpSrc, nSrcBytes, BYTE);
    if ((nVarType & 0x80) == 0)
    {
      if (lpCurrJValue->nArrayDims != 0)
      {
        _ASSERT(FALSE);
        return E_INVALIDARG;
      }
      switch (nVarType)
      {
        case VT_NULL:
          if (lpCurrJValue->nType != CJValueDef::typObject)
          {
            _ASSERT(FALSE);
            return E_INVALIDARG;
          }
          varValue.l = NULL;
          hRes = cUnmJV->Add(varValue, CJValueDef::typObject, 0);
          break;

        case VT_UI1: //VB compliant use VT_UI1 instead of VT_I1
          READBUFFER(varValue.b, lpSrc, nSrcBytes, jbyte);
          switch (lpCurrJValue->nType)
          {
            case CJValueDef::typByte:
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typShort:
              varValue.s = (jshort)(varValue.b);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typInt:
              varValue.i = (jint)(varValue.b);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typLong:
              varValue.j = (jlong)(varValue.b);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typByteObj:
            case CJValueDef::typShortObj:
            case CJValueDef::typIntObj:
            case CJValueDef::typAtomicIntObj:
            case CJValueDef::typLongObj:
            case CJValueDef::typAtomicLongObj:
              {
              jvalue tempVal;

              switch (lpCurrJValue->nType)
              {
                case CJValueDef::typByteObj:
                  hRes = lpJavaVarInterchange->ByteToJavaLangByte(env, (varValue.b), &(tempVal.l));
                  break;
                case CJValueDef::typShortObj:
                  hRes = lpJavaVarInterchange->ShortToJavaLangShort(env, (jshort)(varValue.b), &(tempVal.l));
                  break;
                case CJValueDef::typIntObj:
                  hRes = lpJavaVarInterchange->IntegerToJavaLangInteger(env, (jint)(varValue.b), &(tempVal.l));
                  break;
                case CJValueDef::typAtomicIntObj:
                  hRes = lpJavaVarInterchange->IntegerToJavaUtilConcurrentAtomicInteger(env, (jint)(varValue.b),
                                                                                        &(tempVal.l));
                  break;
                case CJValueDef::typLongObj:
                  hRes = lpJavaVarInterchange->LongToJavaLangLong(env, (jlong)(varValue.b), &(tempVal.l));
                  break;
                case CJValueDef::typAtomicLongObj:
                  hRes = lpJavaVarInterchange->LongToJavaUtilConcurrentAtomicLong(env, (jlong)(varValue.b), &(tempVal.l));
                  break;
              }
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(tempVal, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(tempVal.l);
              }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          break;

        case VT_I2:
          READBUFFER(varValue.s, lpSrc, nSrcBytes, jshort);
          switch (lpCurrJValue->nType)
          {
            case CJValueDef::typByte:
              if (varValue.s < -128 || varValue.s > 127)
                return NKT_E_ArithmeticOverflow;
              varValue.b = (jbyte)(varValue.s);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typShort:
            case CJValueDef::typChar:
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typInt:
              varValue.i = (jint)(varValue.s);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typLong:
              varValue.j = (jlong)(varValue.s);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typByteObj:
            case CJValueDef::typShortObj:
            case CJValueDef::typCharObj:
            case CJValueDef::typIntObj:
            case CJValueDef::typAtomicIntObj:
            case CJValueDef::typLongObj:
            case CJValueDef::typAtomicLongObj:
              {
              jvalue tempVal;

              switch (lpCurrJValue->nType)
              {
                case CJValueDef::typByteObj:
                  if (varValue.s < -128 || varValue.s > 127)
                    hRes = NKT_E_ArithmeticOverflow;
                  else
                    hRes = lpJavaVarInterchange->ByteToJavaLangByte(env, (jbyte)(varValue.s), &(tempVal.l));
                  break;
                case CJValueDef::typShortObj:
                  hRes = lpJavaVarInterchange->ShortToJavaLangShort(env, varValue.s, &(tempVal.l));
                  break;
                case CJValueDef::typCharObj:
                  hRes = lpJavaVarInterchange->CharToJavaLangCharacter(env, (jchar)(varValue.s), &(tempVal.l));
                  break;
                case CJValueDef::typIntObj:
                  hRes = lpJavaVarInterchange->IntegerToJavaLangInteger(env, (jint)(varValue.s), &(tempVal.l));
                  break;
                case CJValueDef::typAtomicIntObj:
                  hRes = lpJavaVarInterchange->IntegerToJavaUtilConcurrentAtomicInteger(env, (jint)(varValue.s),
                                                                                        &(tempVal.l));
                  break;
                case CJValueDef::typLongObj:
                  hRes = lpJavaVarInterchange->LongToJavaLangLong(env, (jlong)(varValue.s), &(tempVal.l));
                  break;
                case CJValueDef::typAtomicLongObj:
                  hRes = lpJavaVarInterchange->LongToJavaUtilConcurrentAtomicLong(env, (jlong)(varValue.s),
                                                                                  &(tempVal.l));
                  break;
              }
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(tempVal, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(tempVal.l);
              }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          break;

        case VT_I4:
          READBUFFER(varValue.i, lpSrc, nSrcBytes, jint);
          switch (lpCurrJValue->nType)
          {
            case CJValueDef::typByte:
              if (varValue.i < -128L || varValue.i > 127L)
                return NKT_E_ArithmeticOverflow;
              varValue.b = (jbyte)(varValue.i);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typShort:
              if (varValue.i < -32768L || varValue.i > 32767L)
                return NKT_E_ArithmeticOverflow;
              varValue.s = (jshort)(varValue.i);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typInt:
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typLong:
              varValue.j = (jlong)(varValue.i);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typByteObj:
            case CJValueDef::typShortObj:
            case CJValueDef::typIntObj:
            case CJValueDef::typAtomicIntObj:
            case CJValueDef::typLongObj:
            case CJValueDef::typAtomicLongObj:
              {
              jvalue tempVal;

              switch (lpCurrJValue->nType)
              {
                case CJValueDef::typByteObj:
                  if (varValue.i < -128L || varValue.i > 127L)
                    hRes = NKT_E_ArithmeticOverflow;
                  else
                    hRes = lpJavaVarInterchange->ByteToJavaLangByte(env, (jbyte)(varValue.i), &(tempVal.l));
                  break;
                case CJValueDef::typShortObj:
                  if (varValue.i < -32768L || varValue.i > 32767L)
                    hRes = NKT_E_ArithmeticOverflow;
                  else
                    hRes = lpJavaVarInterchange->ShortToJavaLangShort(env, (jshort)(varValue.i), &(tempVal.l));
                  break;
                case CJValueDef::typIntObj:
                  hRes = lpJavaVarInterchange->IntegerToJavaLangInteger(env, varValue.i, &(tempVal.l));
                  break;
                case CJValueDef::typAtomicIntObj:
                  hRes = lpJavaVarInterchange->IntegerToJavaUtilConcurrentAtomicInteger(env, varValue.i, &(tempVal.l));
                  break;
                case CJValueDef::typLongObj:
                  hRes = lpJavaVarInterchange->LongToJavaLangLong(env, (jlong)(varValue.i), &(tempVal.l));
                  break;
                case CJValueDef::typAtomicLongObj:
                  hRes = lpJavaVarInterchange->LongToJavaUtilConcurrentAtomicLong(env, (jlong)(varValue.i),
                                                                                  &(tempVal.l));
                  break;
              }
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(tempVal, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(tempVal.l);
              }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          break;

        case VT_I8:
          READBUFFER(varValue.j, lpSrc, nSrcBytes, jlong);
          switch (lpCurrJValue->nType)
          {
            case CJValueDef::typByte:
              if (varValue.j < -128i64 || varValue.j > 127i64)
                return NKT_E_ArithmeticOverflow;
              varValue.b = (jbyte)(varValue.j);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typShort:
              if (varValue.j < -32768i64 || varValue.j > 32767i64)
                return NKT_E_ArithmeticOverflow;
              varValue.s = (jshort)(varValue.j);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typInt:
              if (varValue.j < -2147483648i64 || varValue.j > 2147483647i64)
                return NKT_E_ArithmeticOverflow;
              varValue.i = (jint)(varValue.j);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typLong:
              varValue.j = (jlong)(varValue.i);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typByteObj:
            case CJValueDef::typShortObj:
            case CJValueDef::typIntObj:
            case CJValueDef::typAtomicIntObj:
            case CJValueDef::typLongObj:
            case CJValueDef::typAtomicLongObj:
              {
              jvalue tempVal;

              switch (lpCurrJValue->nType)
              {
                case CJValueDef::typByteObj:
                  if (varValue.j < -128i64 || varValue.j > 127i64)
                    hRes = NKT_E_ArithmeticOverflow;
                  else
                    hRes = lpJavaVarInterchange->ByteToJavaLangByte(env, (jbyte)(varValue.j), &(tempVal.l));
                  break;
                case CJValueDef::typShortObj:
                  if (varValue.j < -32768i64 || varValue.j > 32767i64)
                    hRes = NKT_E_ArithmeticOverflow;
                  else
                    hRes = lpJavaVarInterchange->ShortToJavaLangShort(env, (jshort)(varValue.j), &(tempVal.l));
                  break;
                case CJValueDef::typIntObj:
                  if (varValue.j < -2147483648i64 || varValue.j > 2147483647i64)
                    hRes = NKT_E_ArithmeticOverflow;
                  else
                    hRes = lpJavaVarInterchange->IntegerToJavaLangInteger(env, (jint)(varValue.j), &(tempVal.l));
                  break;
                case CJValueDef::typAtomicIntObj:
                  if (varValue.j < -2147483648i64 || varValue.j > 2147483647i64)
                  {
                    hRes = NKT_E_ArithmeticOverflow;
                  }
                  else
                  {
                    hRes = lpJavaVarInterchange->IntegerToJavaUtilConcurrentAtomicInteger(env, (jint)(varValue.j),
                                                                                          &(tempVal.l));
                  }
                  break;
                case CJValueDef::typLongObj:
                  hRes = lpJavaVarInterchange->LongToJavaLangLong(env, varValue.j, &(tempVal.l));
                  break;
                case CJValueDef::typAtomicLongObj:
                  hRes = lpJavaVarInterchange->LongToJavaUtilConcurrentAtomicLong(env, varValue.j, &(tempVal.l));
                  break;
              }
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(tempVal, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(tempVal.l);
              }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          break;

        case VT_R4:
          READBUFFER(varValue.f, lpSrc, nSrcBytes, jfloat);
          switch (lpCurrJValue->nType)
          {
            case CJValueDef::typFloat:
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typDouble:
              varValue.d = (jdouble)(varValue.f);
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typBigDecimal:
              {
              VARIANT vtTemp;

              vtTemp.vt = VT_R4;
              vtTemp.fltVal = varValue.f;
              hRes = ::VariantChangeType(&vtTemp, &vtTemp, 0, VT_DECIMAL);
              if (SUCCEEDED(hRes))
                hRes = lpJavaVarInterchange->DecimalToJObject(env, &(vtTemp.decVal), &(varValue.l));
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(varValue.l);
              }
              }
              break;

            case CJValueDef::typFloatObj:
            case CJValueDef::typDoubleObj:
              {
              jvalue tempVal;

              switch (lpCurrJValue->nType)
              {
                case CJValueDef::typFloatObj:
                  hRes = lpJavaVarInterchange->FloatToJavaLangFloat(env, varValue.f, &(tempVal.l));
                  break;
                case CJValueDef::typDoubleObj:
                  hRes = lpJavaVarInterchange->DoubleToJavaLangDouble(env, (jdouble)(varValue.f), &(tempVal.l));
                  break;
              }
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(tempVal, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(tempVal.l);
              }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          break;

        case VT_R8:
          READBUFFER(varValue.d, lpSrc, nSrcBytes, jdouble);
          switch (lpCurrJValue->nType)
          {
            case CJValueDef::typFloat:
              if (varValue.d < -3.402823466e+38 || varValue.d > 3.402823466e+38)
                return NKT_E_ArithmeticOverflow;
              varValue.f = (jfloat)varValue.d;
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typDouble:
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              break;

            case CJValueDef::typBigDecimal:
              {
              VARIANT vtTemp;

              vtTemp.vt = VT_R8;
              vtTemp.dblVal = varValue.d;
              hRes = ::VariantChangeType(&vtTemp, &vtTemp, 0, VT_DECIMAL);
              if (SUCCEEDED(hRes))
                hRes = lpJavaVarInterchange->DecimalToJObject(env, &(vtTemp.decVal), &(varValue.l));
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(varValue.l);
              }
              }
              break;

            case CJValueDef::typFloatObj:
            case CJValueDef::typDoubleObj:
              {
              jvalue tempVal;

              switch (lpCurrJValue->nType)
              {
                case CJValueDef::typFloatObj:
                  if (varValue.d < -3.402823466e+38 || varValue.d > 3.402823466e+38)
                    hRes = NKT_E_ArithmeticOverflow;
                  else
                    hRes = lpJavaVarInterchange->FloatToJavaLangFloat(env, (jfloat)(varValue.d), &(tempVal.l));
                  break;
                case CJValueDef::typDoubleObj:
                  hRes = lpJavaVarInterchange->DoubleToJavaLangDouble(env, varValue.d, &(tempVal.l));
                  break;
              }
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(tempVal, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(tempVal.l);
              }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          break;

        case VT_BSTR:
          {
          ULONG nLen;

          if (lpCurrJValue->nType != CJValueDef::typString)
          {
            _ASSERT(FALSE);
            return E_INVALIDARG;
          }
          READBUFFER(nLen, lpSrc, nSrcBytes, ULONG);
          if (nSrcBytes < (SIZE_T)nLen || (nLen & 1) != 0)
          {
            _ASSERT(FALSE);
            return NKT_E_InvalidData;
          }
          varValue.l = env->NewString((jchar*)lpSrc, (jsize)nLen / sizeof(WCHAR));
          if (varValue.l == NULL)
            return E_OUTOFMEMORY;
          hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
          if (SUCCEEDED(hRes))
          {
            lpSrc += (SIZE_T)nLen;
            nSrcBytes -= (SIZE_T)nLen;
          }
          else
          {
            env->DeleteLocalRef(varValue.l);
          }
          }
          break;

        case VT_BOOL:
          switch (lpCurrJValue->nType)
          {
            case CJValueDef::typBoolean:
              {
              BYTE nVal;

              READBUFFER(nVal, lpSrc, nSrcBytes, BYTE);
              varValue.z = (nVal != 0) ? JNI_TRUE : JNI_FALSE;
              hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
              }
              break;

            case CJValueDef::typBooleanObj:
              {
              jvalue tempVal;

              hRes = lpJavaVarInterchange->BooleanToJavaLangBoolean(env, varValue.z, &(tempVal.l));
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(tempVal, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(tempVal.l);
              }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          break;

        case VT_DECIMAL:
          if (nSrcBytes < sizeof(DECIMAL))
            return E_INVALIDARG;
          switch (lpCurrJValue->nType)
          {
            case CJValueDef::typFloat:
            case CJValueDef::typFloatObj:
              {
              VARIANT vtDst, vtSrc;

              ::VariantInit(&vtSrc);
              ::VariantInit(&vtDst);
              vtSrc.vt = VT_BYREF|VT_DECIMAL;
              vtSrc.pdecVal = (DECIMAL*)lpSrc;
              hRes = ::VariantChangeType(&vtDst, &vtSrc, 0, VT_R4);
              if (SUCCEEDED(hRes))
              {
                if (lpCurrJValue->nType == CJValueDef::typFloat)
                {
                  varValue.f = (float)(vtDst.fltVal);
                  hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
                }
                else
                {
                  hRes = lpJavaVarInterchange->FloatToJavaLangFloat(env, (jfloat)(vtDst.fltVal), &(varValue.l));
                  if (SUCCEEDED(hRes))
                  {
                    hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
                    if (FAILED(hRes))
                      env->DeleteLocalRef(varValue.l);
                  }
                }
              }
              if (hRes == DISP_E_OVERFLOW)
                hRes = NKT_E_ArithmeticOverflow;
              }
              break;

            case CJValueDef::typDouble:
            case CJValueDef::typDoubleObj:
              {
              VARIANT vtDst, vtSrc;

              ::VariantInit(&vtSrc);
              ::VariantInit(&vtDst);
              vtSrc.vt = VT_BYREF|VT_DECIMAL;
              vtSrc.pdecVal = (DECIMAL*)lpSrc;
              hRes = ::VariantChangeType(&vtDst, &vtSrc, 0, VT_R8);
              if (SUCCEEDED(hRes))
              {
                if (lpCurrJValue->nType == CJValueDef::typDouble)
                {
                  varValue.d = (double)(vtDst.dblVal);
                  hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
                }
                else
                {
                  hRes = lpJavaVarInterchange->DoubleToJavaLangDouble(env, (jdouble)(vtDst.dblVal), &(varValue.l));
                  if (SUCCEEDED(hRes))
                  {
                    hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
                    if (FAILED(hRes))
                      env->DeleteLocalRef(varValue.l);
                  }
                }
              }
              if (hRes == DISP_E_OVERFLOW)
                hRes = NKT_E_ArithmeticOverflow;
              }
              break;

            case CJValueDef::typBigDecimal:
              hRes = lpJavaVarInterchange->DecimalToJObject(env, (DECIMAL*)lpSrc, &(varValue.l));
              if (SUCCEEDED(hRes))
              {
                hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
                if (FAILED(hRes))
                  env->DeleteLocalRef(varValue.l);
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          if (SUCCEEDED(hRes))
          {
            lpSrc += sizeof(DECIMAL);
            nSrcBytes -= sizeof(DECIMAL);
          }
          break;

        case VT_DATE:
          if (lpCurrJValue->nType != CJValueDef::typDate &&
              lpCurrJValue->nType != CJValueDef::typCalendar)
          {
            _ASSERT(FALSE);
            return E_INVALIDARG;
          }
          if (nSrcBytes < sizeof(SYSTEMTIME))
            return E_INVALIDARG;
          hRes = lpJavaVarInterchange->SystemTimeToJObject(env, (SYSTEMTIME*)lpSrc,
                                                           (lpCurrJValue->nType == CJValueDef::typDate) ? TRUE : FALSE,
                                                           &(varValue.l));
          if (SUCCEEDED(hRes))
            hRes = cUnmJV->Add(varValue, lpCurrJValue->nType, 0);
          if (SUCCEEDED(hRes))
          {
            lpSrc += sizeof(SYSTEMTIME);
            nSrcBytes -= sizeof(SYSTEMTIME);
          }
          else
          {
            if (varValue.l != NULL)
              env->DeleteLocalRef(varValue.l);
          }
          break;

        case VT_DISPATCH:
          {
          ULONGLONG obj;

          if (lpCurrJValue->nType != CJValueDef::typObject)
          {
            _ASSERT(FALSE);
            return E_INVALIDARG;
          }
          READBUFFER(obj, lpSrc, nSrcBytes, ULONGLONG);
          if (obj != 0)
          {
            hRes = lpJObjectMgr->GetLocalRef(env, (jobject)obj, &(varValue.l));
          }
          else
          {
            varValue.l = 0;
            hRes = S_OK;
          }
          if (SUCCEEDED(hRes))
            hRes = cUnmJV->Add(varValue, CJValueDef::typObject, 0);
          if (FAILED(hRes) && varValue.l != NULL)
            env->DeleteLocalRef(varValue.l);
          }
          break;

        default:
          hRes = E_INVALIDARG;
          break;
      }
    }
    else
    {
      TNktAutoDeletePtr<ULONG> cBounds;
      BYTE nTotalDims;
      CJValueDef::eType nType;
      SIZE_T nDim;

      nVarType &= ~0x80;
      //read dimensions count
      READBUFFER(nTotalDims, lpSrc, nSrcBytes, BYTE);
      if (nTotalDims == 0)
        return E_INVALIDARG;
      if (lpCurrJValue->nArrayDims != (ULONG)nTotalDims)
      {
        _ASSERT(FALSE);
        return E_INVALIDARG;
      }
      //read bounds
      cBounds.Attach((ULONG*)NKT_MALLOC((SIZE_T)nTotalDims*sizeof(ULONG)));
      if (cBounds == NULL)
        return E_OUTOFMEMORY;
      for (nDim=0; nDim<(SIZE_T)nTotalDims; nDim++)
      {
        READBUFFER(cBounds[nDim], lpSrc, nSrcBytes, ULONG);
        if (nTotalDims > 1 && cBounds[nDim] == 0)
        {
          //this situation should have been handled by variant marshalling from com code
          _ASSERT(FALSE);
          return NKT_E_InvalidData;
        }
      }
      hRes = UnmarshalJValuesArray_In(env, lpSrc, nSrcBytes, &(varValue.l), 0, nTotalDims, cBounds, nVarType,
                                      lpCurrJValue, lpJavaVarInterchange, lpJObjectMgr);
      if (SUCCEEDED(hRes))
      {
        switch (nVarType)
        {
          case VT_NULL:
          case VT_DISPATCH:
            nType = CJValueDef::typObject;
            break;

          case VT_UI1: //VB compliant use VT_UI1 instead of VT_I1
          case VT_I4:
          case VT_I8:
            switch (lpCurrJValue->nType)
            {
              case CJValueDef::typByte:
              case CJValueDef::typShort:
              case CJValueDef::typInt:
              case CJValueDef::typLong:
              case CJValueDef::typByteObj:
              case CJValueDef::typShortObj:
              case CJValueDef::typIntObj:
              case CJValueDef::typAtomicIntObj:
              case CJValueDef::typLongObj:
              case CJValueDef::typAtomicLongObj:
                nType = lpCurrJValue->nType;
                break;

              default:
                _ASSERT(FALSE);
                return E_INVALIDARG;
            }
            break;

          case VT_I2:
            switch (lpCurrJValue->nType)
            {
              case CJValueDef::typByte:
              case CJValueDef::typShort:
              case CJValueDef::typChar:
              case CJValueDef::typInt:
              case CJValueDef::typLong:
              case CJValueDef::typByteObj:
              case CJValueDef::typShortObj:
              case CJValueDef::typCharObj:
              case CJValueDef::typIntObj:
              case CJValueDef::typAtomicIntObj:
              case CJValueDef::typLongObj:
              case CJValueDef::typAtomicLongObj:
                nType = lpCurrJValue->nType;
                break;

              default:
                _ASSERT(FALSE);
                return E_INVALIDARG;
            }
            break;

          case VT_R4:
          case VT_R8:
            switch (lpCurrJValue->nType)
            {
              case CJValueDef::typFloat:
              case CJValueDef::typDouble:
              case CJValueDef::typBigDecimal:
              case CJValueDef::typFloatObj:
              case CJValueDef::typDoubleObj:
                nType = lpCurrJValue->nType;
                break;

              default:
                _ASSERT(FALSE);
                return E_INVALIDARG;
            }
            break;

          case VT_BSTR:
            switch (lpCurrJValue->nType)
            {
              case CJValueDef::typString:
                nType = lpCurrJValue->nType;
                break;

              default:
                _ASSERT(FALSE);
                return E_INVALIDARG;
            }
            break;

          case VT_BOOL:
            switch (lpCurrJValue->nType)
            {
              case CJValueDef::typBoolean:
                nType = lpCurrJValue->nType;
                break;

              default:
                _ASSERT(FALSE);
                return E_INVALIDARG;
            }
            break;

          case VT_DECIMAL:
            switch (lpCurrJValue->nType)
            {
              case CJValueDef::typFloat:
              case CJValueDef::typDouble:
              case CJValueDef::typBigDecimal:
              case CJValueDef::typFloatObj:
              case CJValueDef::typDoubleObj:
                nType = lpCurrJValue->nType;
                break;

              default:
                _ASSERT(FALSE);
                return E_INVALIDARG;
            }
            break;

          case VT_DATE:
            switch (lpCurrJValue->nType)
            {
              case CJValueDef::typDate:
              case CJValueDef::typCalendar:
                nType = lpCurrJValue->nType;
                break;

              default:
                _ASSERT(FALSE);
                return E_INVALIDARG;
            }
            break;

          default:
            _ASSERT(FALSE);
            return E_INVALIDARG;
        }
        hRes = cUnmJV->Add(varValue, nType, nTotalDims);
      }
      if (FAILED(hRes))
        env->DeleteLocalRef(varValue.l);
    }
    if (FAILED(hRes))
      return hRes;
    //next
    lpCurrJValue = (bParseResult == FALSE) ? itJVal.Next() : NULL;
  }
  if (nJValuesCount != 0)
  {
    _ASSERT(FALSE);
    return E_INVALIDARG;
  }
  //done
  *lpValues = cUnmJV.Detach();
  return S_OK;
}

} //JAVA
