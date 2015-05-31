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

static HRESULT UnmarshalJValuesArray_PeekTypes_In(__in JNIEnv *env, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                                                  __in BYTE nCurrDim, __in BYTE nTotalDims, __in ULONG *lpBounds,
                                                  __in JAVA::CJavaObjectManager *lpJObjectMgr,
                                                  __in BYTE nVarType, __out jvalue *lpVarValue)
{
  SIZE_T i, k;
  HRESULT hRes;

  if (nCurrDim < nTotalDims-1)
  {
    for (k=0; k<(SIZE_T)lpBounds[nCurrDim]; k++)
    {
      hRes = UnmarshalJValuesArray_PeekTypes_In(env, lpSrc, nSrcBytes, nCurrDim+1, nTotalDims, lpBounds, lpJObjectMgr,
                                                nVarType, lpVarValue);
      if (FAILED(hRes))
        return hRes;

    }
  }
  else
  {
    //read values
    switch (nVarType)
    {
      case VT_NULL:
        break;

      case VT_UI1:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(BYTE);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_I2:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(USHORT);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_I4:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(ULONG);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_I8:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(ULONGLONG);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_R4:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(float);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_R8:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(double);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_BSTR:
        for (i=0; i<(SIZE_T)lpBounds[nCurrDim]; i++)
        {
          ULONG nLen;

          READBUFFER(nLen, lpSrc, nSrcBytes, ULONG);
          if (nSrcBytes < nLen)
            return NKT_E_InvalidData;
          lpSrc += nLen;
          nSrcBytes -= nLen;
        }
        break;

      case VT_BOOL:
        i = (SIZE_T)lpBounds[nCurrDim];
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_DECIMAL:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(DECIMAL);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_DATE:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(SYSTEMTIME);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        lpSrc += i;
        nSrcBytes -= i;
        break;

      case VT_DISPATCH:
        i = (SIZE_T)lpBounds[nCurrDim] * sizeof(ULONGLONG);
        if (nSrcBytes < i)
          return NKT_E_InvalidData;
        if (lpVarValue->l == NULL && lpBounds[nCurrDim] > 0)
        {
          ULONGLONG obj;

          obj = *((ULONGLONG NKT_UNALIGNED*)lpSrc);
          if (obj != 0)
          {
            hRes = lpJObjectMgr->GetLocalRef(env, (jobject)obj, &(lpVarValue->l));
            if (FAILED(hRes))
              return hRes;
          }
        }
        lpSrc += i;
        nSrcBytes -= i;
        break;
    }
  }
  return S_OK;
}

//-----------------------------------------------------------

#define UNMARSHALJVALUES_ChunkSize 16
static HRESULT UnmarshalJValuesArray_In(__in JNIEnv *env, __inout LPBYTE &lpSrc, __inout SIZE_T &nSrcBytes,
                                        __out jobject *lpJObject, __in BYTE nCurrDim, __in BYTE nTotalDims,
                                        __in ULONG *lpBounds, __in BYTE nVarType, __in JAVA::CJValueDef *lpCurrJValue,
                                        __in JAVA::CJavaVariantInterchange *lpJavaVarInterchange,
                                        __in JAVA::CJavaObjectManager *lpJObjectMgr)
{
  JAVA::TJavaAutoPtr<jobject> cArray;
  JAVA::TJavaAutoPtr<jclass> cCls;
  jobject obj;
  SIZE_T i, k;
  HRESULT hRes;

  *lpJObject = NULL;
  if (nCurrDim < nTotalDims-1)
  {
    TNktAutoFreePtr<CHAR> sA;
    LPSTR szBaseClassNameA;
    SIZE_T nLen;

    //create base class depending on target
    switch (lpCurrJValue->nType)
    {
      case JAVA::CJValueDef::typBoolean:
        szBaseClassNameA = "Z";
        break;

      case JAVA::CJValueDef::typByte:
        szBaseClassNameA = "B";
        break;

      case JAVA::CJValueDef::typChar:
        szBaseClassNameA = "C";
        break;

      case JAVA::CJValueDef::typShort:
        szBaseClassNameA = "S";
        break;

      case JAVA::CJValueDef::typInt:
        szBaseClassNameA = "I";
        break;

      case JAVA::CJValueDef::typLong:
        szBaseClassNameA = "J";
        break;

      case JAVA::CJValueDef::typFloat:
        szBaseClassNameA = "F";
        break;

      case JAVA::CJValueDef::typDouble:
        szBaseClassNameA = "D";
        break;

      case JAVA::CJValueDef::typString:
        szBaseClassNameA = "Ljava/lang/String;";
        break;

      case JAVA::CJValueDef::typBigDecimal:
        szBaseClassNameA = "Ljava/math/BigDecimal;";
        break;

      case JAVA::CJValueDef::typDate:
        szBaseClassNameA = "Ljava/util/Date;";
        break;

      case JAVA::CJValueDef::typCalendar:
        szBaseClassNameA = "Ljava/util/Calendar;";
        break;

      case JAVA::CJValueDef::typObject:
        szBaseClassNameA = "Ljava/util/Object;";
        break;

      case JAVA::CJValueDef::typBooleanObj:
        szBaseClassNameA = "Ljava/lang/Boolean;";
        break;

      case JAVA::CJValueDef::typByteObj:
        szBaseClassNameA = "Ljava/lang/Byte;";
        break;

      case JAVA::CJValueDef::typCharObj:
        szBaseClassNameA = "Ljava/lang/Char;";
        break;

      case JAVA::CJValueDef::typShortObj:
        szBaseClassNameA = "Ljava/lang/Short;";
        break;

      case JAVA::CJValueDef::typIntObj:
        szBaseClassNameA = "Ljava/lang/Integer;";
        break;

      case JAVA::CJValueDef::typAtomicIntObj:
        szBaseClassNameA = "Ljava/util/concurrent/atomic/AtomicInteger;";
        break;

      case JAVA::CJValueDef::typLongObj:
        szBaseClassNameA = "Ljava/lang/Long;";
        break;

      case JAVA::CJValueDef::typAtomicLongObj:
        szBaseClassNameA = "Ljava/util/concurrent/atomic/AtomicLong;";
        break;

      case JAVA::CJValueDef::typFloatObj:
        szBaseClassNameA = "Ljava/lang/Float;";
        break;

      case JAVA::CJValueDef::typDoubleObj:
        szBaseClassNameA = "Ljava/lang/Double;";
        break;

      default:
        return NKT_E_InvalidData;
    }
    nLen = strlen(szBaseClassNameA) + 1;
    sA.Attach((LPSTR)NKT_MALLOC((SIZE_T)(nTotalDims-nCurrDim-1)+nLen));
    if (sA == NULL)
      return E_OUTOFMEMORY;
    for (i=0; i<(SIZE_T)(nTotalDims-nCurrDim-1); i++)
      sA.Get()[i] = '[';
    memcpy(sA.Get()+i, szBaseClassNameA, nLen);
    cCls.Attach(env, env->FindClass(sA.Get()));
    if (cCls == NULL)
      return E_INVALIDARG;
    cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
    if (cArray == NULL)
      return E_OUTOFMEMORY;
    for (k=0; k<(SIZE_T)lpBounds[nCurrDim]; k++)
    {
      hRes = UnmarshalJValuesArray_In(env, lpSrc, nSrcBytes, &obj, nCurrDim+1, nTotalDims, lpBounds, nVarType,
                                      lpCurrJValue, lpJavaVarInterchange, lpJObjectMgr);
      if (FAILED(hRes))
        return hRes;
      env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)k, obj);
      hRes = JAVA::CheckJavaException(env);
      if (FAILED(hRes))
        return hRes;
    }
  }
  else
  {
    jboolean myJValueChunk_z[UNMARSHALJVALUES_ChunkSize];
    jbyte    myJValueChunk_b[UNMARSHALJVALUES_ChunkSize];
    union {
      jchar    myJValueChunk_c[UNMARSHALJVALUES_ChunkSize];
      jshort   myJValueChunk_s[UNMARSHALJVALUES_ChunkSize];
    };
    jint     myJValueChunk_i[UNMARSHALJVALUES_ChunkSize];
    jlong    myJValueChunk_j[UNMARSHALJVALUES_ChunkSize];
    jfloat   myJValueChunk_f[UNMARSHALJVALUES_ChunkSize];
    jdouble  myJValueChunk_d[UNMARSHALJVALUES_ChunkSize];
    SIZE_T m, nToCopy;

    switch (lpCurrJValue->nType)
    {
      case JAVA::CJValueDef::typBoolean:
        cArray.Attach(env, env->NewBooleanArray((jsize)lpBounds[nCurrDim]));
        break;

      case JAVA::CJValueDef::typByte:
        cArray.Attach(env, env->NewByteArray((jsize)lpBounds[nCurrDim]));
        break;

      case JAVA::CJValueDef::typChar:
        cArray.Attach(env, env->NewCharArray((jsize)lpBounds[nCurrDim]));
        break;

      case JAVA::CJValueDef::typShort:
        cArray.Attach(env, env->NewShortArray((jsize)lpBounds[nCurrDim]));
        break;

      case JAVA::CJValueDef::typInt:
        cArray.Attach(env, env->NewIntArray((jsize)lpBounds[nCurrDim]));
        break;

      case JAVA::CJValueDef::typLong:
        cArray.Attach(env, env->NewLongArray((jsize)lpBounds[nCurrDim]));
        break;

      case JAVA::CJValueDef::typFloat:
        cArray.Attach(env, env->NewFloatArray((jsize)lpBounds[nCurrDim]));
        break;

      case JAVA::CJValueDef::typDouble:
        cArray.Attach(env, env->NewDoubleArray((jsize)lpBounds[nCurrDim]));
        break;

      case JAVA::CJValueDef::typString:
        cCls.Attach(env, env->FindClass("java/lang/String"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typBigDecimal:
        cCls.Attach(env, env->FindClass("java/math/BigDecimal"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typDate:
        cCls.Attach(env, env->FindClass("java/util/Date"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typCalendar:
        cCls.Attach(env, env->FindClass("java/util/Calendar"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typObject:
        cCls.Attach(env, env->FindClass("java/lang/Object"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typBooleanObj:
        cCls.Attach(env, env->FindClass("java/lang/Boolean"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typByteObj:
        cCls.Attach(env, env->FindClass("java/lang/Byte"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typCharObj:
        cCls.Attach(env, env->FindClass("java/lang/Char"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typShortObj:
        cCls.Attach(env, env->FindClass("java/lang/Short"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typIntObj:
        cCls.Attach(env, env->FindClass("java/lang/Integer"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typLongObj:
        cCls.Attach(env, env->FindClass("java/lang/Long"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typFloatObj:
        cCls.Attach(env, env->FindClass("java/lang/Float"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typDoubleObj:
        cCls.Attach(env, env->FindClass("java/lang/Double"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typAtomicIntObj:
        cCls.Attach(env, env->FindClass("java/util/concurrent/atomic/AtomicInteger"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      case JAVA::CJValueDef::typAtomicLongObj:
        cCls.Attach(env, env->FindClass("java/util/concurrent/atomic/AtomicLong"));
        if (cCls == NULL)
          return E_INVALIDARG;
        cArray.Attach(env, env->NewObjectArray((jsize)lpBounds[nCurrDim], cCls, NULL));
        break;

      default:
        return NKT_E_InvalidData;
    }
    if (cArray == NULL)
      return E_OUTOFMEMORY;
    //read values
    for (k=0; k<(SIZE_T)lpBounds[nCurrDim]; k+=nToCopy)
    {
      nToCopy = (SIZE_T)lpBounds[nCurrDim] - k;
      if (nToCopy > UNMARSHALJVALUES_ChunkSize)
        nToCopy = UNMARSHALJVALUES_ChunkSize;
      hRes = S_OK;
      switch (nVarType)
      {
        case VT_NULL:
          if (lpCurrJValue->nType != JAVA::CJValueDef::typObject)
          {
            _ASSERT(FALSE);
            return E_INVALIDARG;
          }
          for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
          {
            jobject obj;

            obj = 0;
            env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
            hRes = JAVA::CheckJavaException(env);
          }
          break;

        case VT_UI1:
          i = nToCopy*sizeof(BYTE);
          if (nSrcBytes < i)
            return NKT_E_InvalidData;
          memcpy(myJValueChunk_b, lpSrc, i);
          switch (lpCurrJValue->nType)
          {
            case JAVA::CJValueDef::typByte:
              env->SetByteArrayRegion((jbyteArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_b);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typShort:
              for (m=0; m<nToCopy; m++)
                myJValueChunk_s[m] = (jshort)(myJValueChunk_b[m]);
              env->SetShortArrayRegion((jshortArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_s);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typInt:
              for (m=0; m<nToCopy; m++)
                myJValueChunk_i[m] = (jint)(myJValueChunk_b[m]);
              env->SetIntArrayRegion((jintArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_i);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typLong:
              for (m=0; m<nToCopy; m++)
                myJValueChunk_j[m] = (jlong)(myJValueChunk_b[m]);
              env->SetLongArrayRegion((jlongArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_j);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typByteObj:
            case JAVA::CJValueDef::typShortObj:
            case JAVA::CJValueDef::typIntObj:
            case JAVA::CJValueDef::typAtomicIntObj:
            case JAVA::CJValueDef::typLongObj:
            case JAVA::CJValueDef::typAtomicLongObj:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                jobject obj;

                switch (lpCurrJValue->nType)
                {
                  case JAVA::CJValueDef::typByteObj:
                    hRes = lpJavaVarInterchange->ByteToJavaLangByte(env, myJValueChunk_b[m], &obj);
                    break;
                  case JAVA::CJValueDef::typShortObj:
                    hRes = lpJavaVarInterchange->ShortToJavaLangShort(env, (jshort)(myJValueChunk_b[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typIntObj:
                    hRes = lpJavaVarInterchange->IntegerToJavaLangInteger(env, (jint)(myJValueChunk_b[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typAtomicIntObj:
                    hRes = lpJavaVarInterchange->IntegerToJavaUtilConcurrentAtomicInteger(env,
                                                                  (jint)(myJValueChunk_b[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typLongObj:
                    hRes = lpJavaVarInterchange->LongToJavaLangLong(env, (jlong)(myJValueChunk_b[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typAtomicLongObj:
                    hRes = lpJavaVarInterchange->LongToJavaUtilConcurrentAtomicLong(env,
                                                               (jlong)(myJValueChunk_b[m]), &obj);
                    break;
                }
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          if (FAILED(hRes))
            return hRes;
          lpSrc += i;
          nSrcBytes -= i;
          break;

        case VT_I2:
          i = nToCopy*sizeof(USHORT);
          if (nSrcBytes < i)
            return NKT_E_InvalidData;
          memcpy(myJValueChunk_s, lpSrc, i);
          switch (lpCurrJValue->nType)
          {
            case JAVA::CJValueDef::typByte:
              for (m=0; m<nToCopy; m++)
              {
                if (myJValueChunk_s[m] < -128 || myJValueChunk_s[m] > 127)
                  return NKT_E_ArithmeticOverflow;
                myJValueChunk_b[m] = (jbyte)(myJValueChunk_s[m]);
              }
              env->SetByteArrayRegion((jbyteArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_b);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typShort:
              env->SetShortArrayRegion((jshortArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_s);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typChar:
              env->SetCharArrayRegion((jcharArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_c);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typInt:
              for (m=0; m<nToCopy; m++)
                myJValueChunk_i[m] = (jint)(myJValueChunk_s[m]);
              env->SetIntArrayRegion((jintArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_i);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typLong:
              for (m=0; m<nToCopy; m++)
                myJValueChunk_j[m] = (jlong)(myJValueChunk_s[m]);
              env->SetLongArrayRegion((jlongArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_j);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typByteObj:
            case JAVA::CJValueDef::typShortObj:
            case JAVA::CJValueDef::typCharObj:
            case JAVA::CJValueDef::typIntObj:
            case JAVA::CJValueDef::typAtomicIntObj:
            case JAVA::CJValueDef::typLongObj:
            case JAVA::CJValueDef::typAtomicLongObj:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                jobject obj;

                switch (lpCurrJValue->nType)
                {
                  case JAVA::CJValueDef::typByteObj:
                    if (myJValueChunk_s[m] < -128 || myJValueChunk_s[m] > 127)
                      hRes = NKT_E_ArithmeticOverflow;
                    else
                      hRes = lpJavaVarInterchange->ByteToJavaLangByte(env, (jbyte)(myJValueChunk_s[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typShortObj:
                    hRes = lpJavaVarInterchange->ShortToJavaLangShort(env, myJValueChunk_s[m], &obj);
                    break;
                  case JAVA::CJValueDef::typCharObj:
                    hRes = lpJavaVarInterchange->CharToJavaLangCharacter(env, (jchar)(myJValueChunk_c[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typIntObj:
                    hRes = lpJavaVarInterchange->IntegerToJavaLangInteger(env, (jint)(myJValueChunk_s[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typAtomicIntObj:
                    hRes = lpJavaVarInterchange->IntegerToJavaUtilConcurrentAtomicInteger(env,
                                                                  (jint)(myJValueChunk_s[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typLongObj:
                    hRes = lpJavaVarInterchange->LongToJavaLangLong(env, (jlong)(myJValueChunk_s[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typAtomicLongObj:
                    hRes = lpJavaVarInterchange->LongToJavaUtilConcurrentAtomicLong(env,
                                                               (jlong)(myJValueChunk_s[m]), &obj);
                    break;
                }
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          if (FAILED(hRes))
            return hRes;
          lpSrc += i;
          nSrcBytes -= i;
          break;

        case VT_I4:
          i = nToCopy*sizeof(ULONG);
          if (nSrcBytes < i)
            return NKT_E_InvalidData;
          memcpy(myJValueChunk_i, lpSrc, i);
          switch (lpCurrJValue->nType)
          {
            case JAVA::CJValueDef::typByte:
              for (m=0; m<nToCopy; m++)
              {
                if (myJValueChunk_i[m] < -128L || myJValueChunk_i[m] > 127L)
                  return NKT_E_ArithmeticOverflow;
                myJValueChunk_b[m] = (jbyte)(myJValueChunk_i[m]);
              }
              env->SetByteArrayRegion((jbyteArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_b);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typShort:
              for (m=0; m<nToCopy; m++)
              {
                if (myJValueChunk_i[m] < -32768L || myJValueChunk_i[m] > 32767L)
                  return NKT_E_ArithmeticOverflow;
                myJValueChunk_s[m] = (jshort)(myJValueChunk_i[m]);
              }
              env->SetShortArrayRegion((jshortArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_s);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typInt:
              env->SetIntArrayRegion((jintArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_i);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typLong:
              for (m=0; m<nToCopy; m++)
                myJValueChunk_j[m] = (jlong)(myJValueChunk_i[m]);
              env->SetLongArrayRegion((jlongArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_j);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typByteObj:
            case JAVA::CJValueDef::typShortObj:
            case JAVA::CJValueDef::typIntObj:
            case JAVA::CJValueDef::typAtomicIntObj:
            case JAVA::CJValueDef::typLongObj:
            case JAVA::CJValueDef::typAtomicLongObj:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                jobject obj;

                switch (lpCurrJValue->nType)
                {
                  case JAVA::CJValueDef::typByteObj:
                    if (myJValueChunk_i[m] < -128L || myJValueChunk_i[m] > 127L)
                      hRes = NKT_E_ArithmeticOverflow;
                    else
                      hRes = lpJavaVarInterchange->ByteToJavaLangByte(env, (jbyte)(myJValueChunk_i[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typShortObj:
                    if (myJValueChunk_i[m] < -32768L || myJValueChunk_i[m] > 32767L)
                      hRes = NKT_E_ArithmeticOverflow;
                    else
                      hRes = lpJavaVarInterchange->ShortToJavaLangShort(env, (jshort)(myJValueChunk_i[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typIntObj:
                    hRes = lpJavaVarInterchange->IntegerToJavaLangInteger(env, myJValueChunk_i[m], &obj);
                    break;
                  case JAVA::CJValueDef::typAtomicIntObj:
                    hRes = lpJavaVarInterchange->IntegerToJavaUtilConcurrentAtomicInteger(env, myJValueChunk_i[m],
                                                                                          &obj);
                    break;
                  case JAVA::CJValueDef::typLongObj:
                    hRes = lpJavaVarInterchange->LongToJavaLangLong(env, (jlong)(myJValueChunk_i[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typAtomicLongObj:
                    hRes = lpJavaVarInterchange->LongToJavaUtilConcurrentAtomicLong(env,
                                                               (jlong)(myJValueChunk_i[m]), &obj);
                    break;
                }
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          if (FAILED(hRes))
            return hRes;
          lpSrc += i;
          nSrcBytes -= i;
          break;

        case VT_I8:
          i = nToCopy*sizeof(ULONGLONG);
          if (nSrcBytes < i)
            return NKT_E_InvalidData;
          memcpy(myJValueChunk_j, lpSrc, i);
          switch (lpCurrJValue->nType)
          {
            case JAVA::CJValueDef::typByte:
              for (m=0; m<nToCopy; m++)
              {
                if (myJValueChunk_j[m] < -128i64 || myJValueChunk_j[m] > 127i64)
                  return NKT_E_ArithmeticOverflow;
                myJValueChunk_b[m] = (jbyte)(myJValueChunk_j[m]);
              }
              env->SetByteArrayRegion((jbyteArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_b);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typShort:
              for (m=0; m<nToCopy; m++)
              {
                if (myJValueChunk_j[m] < -32768i64 || myJValueChunk_j[m] > 32767i64)
                  return NKT_E_ArithmeticOverflow;
                myJValueChunk_s[m] = (jshort)(myJValueChunk_j[m]);
              }
              env->SetShortArrayRegion((jshortArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_s);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typInt:
              for (m=0; m<nToCopy; m++)
              {
                if (myJValueChunk_j[m] < -2147483648i64 || myJValueChunk_j[m] > 2147483647i64)
                  return NKT_E_ArithmeticOverflow;
                myJValueChunk_i[m] = (jint)(myJValueChunk_j[m]);
              }
              env->SetIntArrayRegion((jintArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_i);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typLong:
              env->SetLongArrayRegion((jlongArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_j);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typByteObj:
            case JAVA::CJValueDef::typShortObj:
            case JAVA::CJValueDef::typIntObj:
            case JAVA::CJValueDef::typAtomicIntObj:
            case JAVA::CJValueDef::typLongObj:
            case JAVA::CJValueDef::typAtomicLongObj:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                jobject obj;

                switch (lpCurrJValue->nType)
                {
                  case JAVA::CJValueDef::typByteObj:
                    if (myJValueChunk_j[m] < -128i64 || myJValueChunk_j[m] > 127i64)
                      hRes = NKT_E_ArithmeticOverflow;
                    else
                      hRes = lpJavaVarInterchange->ByteToJavaLangByte(env, (jbyte)(myJValueChunk_j[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typShortObj:
                    if (myJValueChunk_j[m] < -32768i64 || myJValueChunk_j[m] > 32767i64)
                      hRes = NKT_E_ArithmeticOverflow;
                    else
                      hRes = lpJavaVarInterchange->ShortToJavaLangShort(env, (jshort)(myJValueChunk_j[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typIntObj:
                    if (myJValueChunk_j[m] < -2147483648i64 || myJValueChunk_j[m] > 2147483647i64)
                      hRes = NKT_E_ArithmeticOverflow;
                    else
                      hRes = lpJavaVarInterchange->IntegerToJavaLangInteger(env, (jint)(myJValueChunk_j[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typAtomicIntObj:
                    if (myJValueChunk_j[m] < -2147483648i64 || myJValueChunk_j[m] > 2147483647i64)
                    {
                      hRes = NKT_E_ArithmeticOverflow;
                    }
                    else
                    {
                      hRes = lpJavaVarInterchange->IntegerToJavaUtilConcurrentAtomicInteger(env,
                                                                    (jint)(myJValueChunk_j[m]), &obj);
                    }
                    break;
                  case JAVA::CJValueDef::typLongObj:
                    hRes = lpJavaVarInterchange->LongToJavaLangLong(env, myJValueChunk_j[m], &obj);
                    break;
                  case JAVA::CJValueDef::typAtomicLongObj:
                    hRes = lpJavaVarInterchange->LongToJavaUtilConcurrentAtomicLong(env, myJValueChunk_j[m], &obj);
                    break;
                }
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          if (FAILED(hRes))
            return hRes;
          lpSrc += i;
          nSrcBytes -= i;
          break;

        case VT_R4:
          i = nToCopy*sizeof(float);
          if (nSrcBytes < i)
            return NKT_E_InvalidData;
          memcpy(myJValueChunk_f, lpSrc, i);
          switch (lpCurrJValue->nType)
          {
            case JAVA::CJValueDef::typFloat:
              env->SetFloatArrayRegion((jfloatArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_f);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typDouble:
              for (m=0; m<nToCopy; m++)
                myJValueChunk_d[m] = (jdouble)(myJValueChunk_f[m]);
              env->SetDoubleArrayRegion((jdoubleArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_d);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typBigDecimal:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                VARIANT vtTemp;
                jobject obj;

                vtTemp.vt = VT_R4;
                vtTemp.fltVal = myJValueChunk_f[m];
                hRes = ::VariantChangeType(&vtTemp, &vtTemp, 0, VT_DECIMAL);
                if (SUCCEEDED(hRes))
                  hRes = lpJavaVarInterchange->DecimalToJObject(env, &(vtTemp.decVal), &obj);
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            case JAVA::CJValueDef::typFloatObj:
            case JAVA::CJValueDef::typDoubleObj:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                jobject obj;

                switch (lpCurrJValue->nType)
                {
                  case JAVA::CJValueDef::typFloatObj:
                    hRes = lpJavaVarInterchange->FloatToJavaLangFloat(env, myJValueChunk_f[m], &obj);
                    break;
                  case JAVA::CJValueDef::typDoubleObj:
                    hRes = lpJavaVarInterchange->DoubleToJavaLangDouble(env, (jdouble)(myJValueChunk_f[m]), &obj);
                    break;
                }
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          if (FAILED(hRes))
            return (hRes == DISP_E_OVERFLOW) ? NKT_E_ArithmeticOverflow : hRes;
          lpSrc += i;
          nSrcBytes -= i;
          break;

        case VT_R8:
          i = nToCopy*sizeof(double);
          if (nSrcBytes < i)
            return NKT_E_InvalidData;
          memcpy(myJValueChunk_d, lpSrc, i);
          switch (lpCurrJValue->nType)
          {
            case JAVA::CJValueDef::typFloat:
              for (m=0; m<nToCopy; m++)
              {
                if (myJValueChunk_d[m] < -3.402823466e+38 || myJValueChunk_d[m] > 3.402823466e+38)
                  return NKT_E_ArithmeticOverflow;
                myJValueChunk_f[m] = (jfloat)(myJValueChunk_d[m]);
              }
              env->SetFloatArrayRegion((jfloatArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_f);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typDouble:
              env->SetDoubleArrayRegion((jdoubleArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_d);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typBigDecimal:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                VARIANT vtTemp;
                jobject obj;

                vtTemp.vt = VT_R8;
                vtTemp.dblVal = myJValueChunk_d[m];
                hRes = ::VariantChangeType(&vtTemp, &vtTemp, 0, VT_DECIMAL);
                if (SUCCEEDED(hRes))
                  hRes = lpJavaVarInterchange->DecimalToJObject(env, &(vtTemp.decVal), &obj);
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            case JAVA::CJValueDef::typFloatObj:
            case JAVA::CJValueDef::typDoubleObj:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                jobject obj;

                switch (lpCurrJValue->nType)
                {
                  case JAVA::CJValueDef::typFloatObj:
                    if (myJValueChunk_d[m] < -3.402823466e+38 || myJValueChunk_d[m] > 3.402823466e+38)
                      hRes = NKT_E_ArithmeticOverflow;
                    else
                      hRes = lpJavaVarInterchange->FloatToJavaLangFloat(env, (jfloat)(myJValueChunk_d[m]), &obj);
                    break;
                  case JAVA::CJValueDef::typDoubleObj:
                    hRes = lpJavaVarInterchange->DoubleToJavaLangDouble(env, myJValueChunk_d[m], &obj);
                    break;
                }
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          if (FAILED(hRes))
            return (hRes == DISP_E_OVERFLOW) ? NKT_E_ArithmeticOverflow : hRes;
          lpSrc += i;
          nSrcBytes -= i;
          break;

        case VT_BSTR:
          if (lpCurrJValue->nType != JAVA::CJValueDef::typString)
          {
            _ASSERT(FALSE);
            return E_INVALIDARG;
          }
          for (m=0; m<nToCopy; m++)
          {
            ULONG nLen;
            jobject obj;

            READBUFFER(nLen, lpSrc, nSrcBytes, ULONG);
            if (nSrcBytes < nLen)
              return NKT_E_InvalidData;
            obj = env->NewString((jchar*)lpSrc, (jsize)nLen / sizeof(WCHAR));
            if (obj == NULL)
              return E_OUTOFMEMORY;
            env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
            {
              env->DeleteLocalRef(obj);
              return hRes;
            }
            lpSrc += nLen;
            nSrcBytes -= nLen;
          }
          break;

        case VT_BOOL:
          if (nSrcBytes < nToCopy)
            return NKT_E_InvalidData;
          for (m=0; m<nToCopy; m++)
            myJValueChunk_z[m] = (lpSrc[m] != 0) ? JNI_TRUE : JNI_FALSE;
          switch (lpCurrJValue->nType)
          {
            case JAVA::CJValueDef::typBoolean:
              env->SetBooleanArrayRegion((jbooleanArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_z);
              hRes = JAVA::CheckJavaException(env);
              break;

            case JAVA::CJValueDef::typBooleanObj:
              for (m=0; m<nToCopy && SUCCEEDED(hRes); m++)
              {
                jobject obj;

                hRes = lpJavaVarInterchange->BooleanToJavaLangBoolean(env, myJValueChunk_z[m], &obj);
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          if (FAILED(hRes))
            return hRes;
          lpSrc += nToCopy;
          nSrcBytes -= nToCopy;
          break;

        case VT_DECIMAL:
          {
          VARIANT vtDst, vtSrc;
          jobject obj;

          if (nSrcBytes < nToCopy*sizeof(DECIMAL))
            return NKT_E_InvalidData;
          ::VariantInit(&vtSrc);
          ::VariantInit(&vtDst);
          vtSrc.vt = VT_BYREF|VT_DECIMAL;
          switch (lpCurrJValue->nType)
          {
            case JAVA::CJValueDef::typFloat:
              for (m=0; m<nToCopy; m++)
              {
                vtSrc.pdecVal = (DECIMAL*)lpSrc;
                hRes = ::VariantChangeType(&vtDst, &vtSrc, 0, VT_R4);
                if (FAILED(hRes))
                  return (hRes == DISP_E_OVERFLOW) ? NKT_E_ArithmeticOverflow : hRes;
                myJValueChunk_f[m] = (float)(vtDst.fltVal);
                lpSrc += sizeof(DECIMAL);
                nSrcBytes -= sizeof(DECIMAL);
              }
              env->SetFloatArrayRegion((jfloatArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_f);
              hRes = JAVA::CheckJavaException(env);
              if (FAILED(hRes))
                return hRes;
              break;

            case JAVA::CJValueDef::typDouble:
              for (m=0; m<nToCopy; m++)
              {
                vtSrc.pdecVal = (DECIMAL*)lpSrc;
                hRes = ::VariantChangeType(&vtDst, &vtSrc, 0, VT_R8);
                if (FAILED(hRes))
                  return (hRes == DISP_E_OVERFLOW) ? NKT_E_ArithmeticOverflow : hRes;
                myJValueChunk_d[m] = (double)(vtDst.dblVal);
                lpSrc += sizeof(DECIMAL);
                nSrcBytes -= sizeof(DECIMAL);
              }
              env->SetDoubleArrayRegion((jdoubleArray)(cArray.Get()), (jsize)k, (jsize)nToCopy, myJValueChunk_d);
              hRes = JAVA::CheckJavaException(env);
              if (FAILED(hRes))
                return hRes;
              break;

            case JAVA::CJValueDef::typBigDecimal:
              for (m=0; m<nToCopy; m++)
              {
                hRes = lpJavaVarInterchange->DecimalToJObject(env, (DECIMAL*)lpSrc, &obj);
                if (FAILED(hRes))
                  return hRes;
                env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                hRes = JAVA::CheckJavaException(env);
                if (FAILED(hRes))
                {
                  env->DeleteLocalRef(obj);
                  return hRes;
                }
                lpSrc += sizeof(DECIMAL);
                nSrcBytes -= sizeof(DECIMAL);
              }
              break;

            case JAVA::CJValueDef::typFloatObj:
              for (m=0; m<nToCopy; m++)
              {
                vtSrc.pdecVal = (DECIMAL*)lpSrc;
                hRes = ::VariantChangeType(&vtDst, &vtSrc, 0, VT_R4);
                if (SUCCEEDED(hRes))
                  hRes = lpJavaVarInterchange->FloatToJavaLangFloat(env, (jfloat)(vtDst.fltVal), &obj);
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
                if (FAILED(hRes))
                  return (hRes == DISP_E_OVERFLOW) ? NKT_E_ArithmeticOverflow : hRes;
              }
              break;

            case JAVA::CJValueDef::typDoubleObj:
              for (m=0; m<nToCopy; m++)
              {
                vtSrc.pdecVal = (DECIMAL*)lpSrc;
                hRes = ::VariantChangeType(&vtDst, &vtSrc, 0, VT_R8);
                if (SUCCEEDED(hRes))
                  hRes = lpJavaVarInterchange->DoubleToJavaLangDouble(env, (jdouble)(vtDst.dblVal), &obj);
                if (SUCCEEDED(hRes))
                {
                  env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
                  hRes = JAVA::CheckJavaException(env);
                  if (FAILED(hRes))
                    env->DeleteLocalRef(obj);
                }
                if (FAILED(hRes))
                  return (hRes == DISP_E_OVERFLOW) ? NKT_E_ArithmeticOverflow : hRes;
              }
              break;

            default:
              _ASSERT(FALSE);
              return E_INVALIDARG;
          }
          }
          break;

        case VT_DATE:
          if (lpCurrJValue->nType != JAVA::CJValueDef::typDate &&
              lpCurrJValue->nType != JAVA::CJValueDef::typCalendar)
          {
            _ASSERT(FALSE);
            return E_INVALIDARG;
          }
          for (m=0; m<nToCopy; m++)
          {
            jobject obj;

            if (nSrcBytes < sizeof(SYSTEMTIME))
              return NKT_E_InvalidData;
            hRes = lpJavaVarInterchange->SystemTimeToJObject(env, (SYSTEMTIME*)lpSrc,
                                               (lpCurrJValue->nType == JAVA::CJValueDef::typDate) ? TRUE : FALSE, &obj);
            if (FAILED(hRes))
              return hRes;
            env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), obj);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
            {
              env->DeleteLocalRef(obj);
              return hRes;
            }
            lpSrc += sizeof(SYSTEMTIME);
            nSrcBytes -= sizeof(SYSTEMTIME);
          }
          break;

        case VT_DISPATCH:
          if (lpCurrJValue->nType != JAVA::CJValueDef::typObject)
          {
            _ASSERT(FALSE);
            return E_INVALIDARG;
          }
          for (m=0; m<nToCopy; m++)
          {
            ULONGLONG obj;
            jobject localObj;

            READBUFFER(obj, lpSrc, nSrcBytes, ULONGLONG);
            if (obj != 0)
            {
              hRes = lpJObjectMgr->GetLocalRef(env, (jobject)obj, &localObj);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              localObj = 0;
            }
            env->SetObjectArrayElement((jobjectArray)(cArray.Get()), (jsize)(k+m), localObj);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
            {
              env->DeleteLocalRef(localObj);
              return hRes;
            }
          }
          break;
      }
    }
  }
  *lpJObject = cArray.Detach();
  return S_OK;
}
#undef UNMARSHALJVALUES_ChunkSize
