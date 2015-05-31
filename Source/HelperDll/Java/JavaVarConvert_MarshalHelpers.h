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

#define MARSHALJVALUES_ChunkSize 16
static HRESULT VarConvert_MarshalJValue_ProcessArray(__in JNIEnv *env, __in jobject arr, __in ULONG *lpArraySizes,
                                                     __in ULONG nCurrDim, __in JAVA::CJValueDef *lpJValueDef,
                                                     __in JAVA::CJavaVariantInterchange *lpJavaVarInterchange,
                                                     __in JAVA::CJavaObjectManager *lpJObjectMgr,
                                                     __inout LPBYTE &lpDest, __inout SIZE_T &nSize)
{
  HRESULT hRes;

  //process
  if (nCurrDim < lpJValueDef->nArrayDims-1)
  {
    JAVA::TJavaAutoPtr<jobject> cObj;
    ULONG k;

    for (k=0; k<lpArraySizes[nCurrDim]; k++)
    {
      cObj.Release();
      cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, (jsize)k));
      if (cObj == NULL)
        return E_OUTOFMEMORY;
      if ((jsize)lpArraySizes[nCurrDim+1] != env->GetArrayLength((jarray)cObj.Get()))
        return NKT_E_Unsupported;
      hRes = VarConvert_MarshalJValue_ProcessArray(env, cObj.Get(), lpArraySizes, nCurrDim+1, lpJValueDef,
                                                   lpJavaVarInterchange, lpJObjectMgr, lpDest, nSize);
      if (FAILED(hRes))
        return hRes;
    }
  }
  else
  {
    union {
      jboolean myJValueChunk_z[MARSHALJVALUES_ChunkSize];
      jbyte    myJValueChunk_b[MARSHALJVALUES_ChunkSize];
      jchar    myJValueChunk_c[MARSHALJVALUES_ChunkSize];
      jshort   myJValueChunk_s[MARSHALJVALUES_ChunkSize];
      jint     myJValueChunk_i[MARSHALJVALUES_ChunkSize];
      jlong    myJValueChunk_j[MARSHALJVALUES_ChunkSize];
      jfloat   myJValueChunk_f[MARSHALJVALUES_ChunkSize];
      jdouble  myJValueChunk_d[MARSHALJVALUES_ChunkSize];
    };
    jsize k, m, nToCopy, nArrSize;

    nArrSize = (jsize)lpArraySizes[nCurrDim];
    switch (lpJValueDef->nType)
    {
      case JAVA::CJValueDef::typVoid:
        break;

      case JAVA::CJValueDef::typBoolean:
        if (lpDest != NULL)
        {
          for (k=0; k<nArrSize; k+=nToCopy)
          {
            nToCopy = nArrSize - k;
            if (nToCopy > MARSHALJVALUES_ChunkSize)
              nToCopy = MARSHALJVALUES_ChunkSize;
            env->GetBooleanArrayRegion((jbooleanArray)arr, k, nToCopy, myJValueChunk_z);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
              return hRes;
            for (m=0; m<nToCopy; m++)
              lpDest[m] = (myJValueChunk_z[m] != JNI_FALSE) ? 1 : 0;
            lpDest += (SIZE_T)nToCopy;
          }
        }
        nSize += (SIZE_T)nArrSize;
        break;

      case JAVA::CJValueDef::typByte:
        if (lpDest != NULL)
        {
          for (k=0; k<nArrSize; k+=nToCopy)
          {
            nToCopy = nArrSize - k;
            if (nToCopy > MARSHALJVALUES_ChunkSize)
              nToCopy = MARSHALJVALUES_ChunkSize;
            env->GetByteArrayRegion((jbyteArray)arr, k, nToCopy, myJValueChunk_b);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
              return hRes;
            memcpy(lpDest, myJValueChunk_b, (SIZE_T)nToCopy);
            lpDest += (SIZE_T)nToCopy;
          }
        }
        nSize += (SIZE_T)nArrSize;
        break;

      case JAVA::CJValueDef::typChar:
        if (lpDest != NULL)
        {
          for (k=0; k<nArrSize; k+=nToCopy)
          {
            nToCopy = nArrSize - k;
            if (nToCopy > MARSHALJVALUES_ChunkSize)
              nToCopy = MARSHALJVALUES_ChunkSize;
            env->GetCharArrayRegion((jcharArray)arr, k, nToCopy, myJValueChunk_c);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
              return hRes;
            memcpy(lpDest, myJValueChunk_c, (SIZE_T)nToCopy*sizeof(USHORT));
            lpDest += (SIZE_T)nToCopy*sizeof(USHORT);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(USHORT);
        break;

      case JAVA::CJValueDef::typShort:
        if (lpDest != NULL)
        {
          for (k=0; k<nArrSize; k+=nToCopy)
          {
            nToCopy = nArrSize - k;
            if (nToCopy > MARSHALJVALUES_ChunkSize)
              nToCopy = MARSHALJVALUES_ChunkSize;
            env->GetShortArrayRegion((jshortArray)arr, k, nToCopy, myJValueChunk_s);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
              return hRes;
            memcpy(lpDest, myJValueChunk_s, (SIZE_T)nToCopy*sizeof(USHORT));
            lpDest += (SIZE_T)nToCopy*sizeof(USHORT);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(USHORT);
        break;

      case JAVA::CJValueDef::typInt:
        if (lpDest != NULL)
        {
          for (k=0; k<nArrSize; k+=nToCopy)
          {
            nToCopy = nArrSize - k;
            if (nToCopy > MARSHALJVALUES_ChunkSize)
              nToCopy = MARSHALJVALUES_ChunkSize;
            env->GetIntArrayRegion((jintArray)arr, k, nToCopy, myJValueChunk_i);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
              return hRes;
            memcpy(lpDest, myJValueChunk_i, (SIZE_T)nToCopy*sizeof(ULONG));
            lpDest += (SIZE_T)nToCopy*sizeof(ULONG);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(ULONG);
        break;

      case JAVA::CJValueDef::typLong:
        if (lpDest != NULL)
        {
          for (k=0; k<nArrSize; k+=nToCopy)
          {
            nToCopy = nArrSize - k;
            if (nToCopy > MARSHALJVALUES_ChunkSize)
              nToCopy = MARSHALJVALUES_ChunkSize;
            env->GetLongArrayRegion((jlongArray)arr, k, nToCopy, myJValueChunk_j);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
              return hRes;
            memcpy(lpDest, myJValueChunk_j, (SIZE_T)nToCopy*sizeof(ULONGLONG));
            lpDest += (SIZE_T)nToCopy*sizeof(ULONGLONG);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(ULONGLONG);
        break;

      case JAVA::CJValueDef::typFloat:
        if (lpDest != NULL)
        {
          for (k=0; k<nArrSize; k+=nToCopy)
          {
            nToCopy = nArrSize - k;
            if (nToCopy > MARSHALJVALUES_ChunkSize)
              nToCopy = MARSHALJVALUES_ChunkSize;
            env->GetFloatArrayRegion((jfloatArray)arr, k, nToCopy, myJValueChunk_f);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
              return hRes;
            memcpy(lpDest, myJValueChunk_f, (SIZE_T)nToCopy*sizeof(float));
            lpDest += (SIZE_T)nToCopy*sizeof(float);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(float);
        break;

      case JAVA::CJValueDef::typDouble:
        if (lpDest != NULL)
        {
          for (k=0; k<nArrSize; k+=nToCopy)
          {
            nToCopy = nArrSize - k;
            if (nToCopy > MARSHALJVALUES_ChunkSize)
              nToCopy = MARSHALJVALUES_ChunkSize;
            env->GetDoubleArrayRegion((jdoubleArray)arr, k, nToCopy, myJValueChunk_d);
            hRes = JAVA::CheckJavaException(env);
            if (FAILED(hRes))
              return hRes;
            memcpy(lpDest, myJValueChunk_d, (SIZE_T)nToCopy*sizeof(double));
            lpDest += (SIZE_T)nToCopy*sizeof(double);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(double);
        break;

      case JAVA::CJValueDef::typString:
        {
        JAVA::TJavaAutoPtr<jstring> cStr;
        LPWSTR sW;
        jboolean isCopy;
        ULONG nLen;

        for (k=0; k<nArrSize; k++)
        {
          cStr.Attach(env, (jstring)(env->GetObjectArrayElement((jobjectArray)arr, k)));
          if (cStr == NULL)
            return E_OUTOFMEMORY;
          nLen = (ULONG)((SIZE_T)(env->GetStringLength(cStr)) * sizeof(WCHAR)); //before string critical
          sW = (LPWSTR)(env->GetStringCritical(cStr, &isCopy));
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
          env->ReleaseStringCritical(cStr, (jchar*)sW);
        }
        }
        break;

      case JAVA::CJValueDef::typBigDecimal:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj == NULL)
              return E_OUTOFMEMORY;
            hRes = lpJavaVarInterchange->JObjectToDecimal(env, cObj, (DECIMAL*)lpDest);
            if (FAILED(hRes))
              return hRes;
            lpDest += sizeof(DECIMAL);
          }
        }
        nSize += nArrSize * sizeof(DECIMAL);
        break;

      case JAVA::CJValueDef::typDate:
      case JAVA::CJValueDef::typCalendar:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj == NULL)
              return E_OUTOFMEMORY;
            hRes = lpJavaVarInterchange->JObjectToSystemTime(env, cObj, (SYSTEMTIME*)lpDest);
            if (FAILED(hRes))
              return hRes;
            lpDest += sizeof(SYSTEMTIME);
          }
        }
        nSize += nArrSize * sizeof(SYSTEMTIME);
        break;

      case JAVA::CJValueDef::typObject:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;
          ULONGLONG ullObj;
          jobject globalObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            ullObj = 0;
            if (cObj != NULL)
            {
              hRes = lpJObjectMgr->Add(env, cObj, &globalObj);
              if (FAILED(hRes))
                return hRes;
              ullObj = NKT_PTR_2_ULONGLONG(globalObj);
            }
            memcpy(lpDest, &ullObj, sizeof(ULONGLONG));
            lpDest += sizeof(ULONGLONG);
          }
        }
        nSize += nArrSize * sizeof(ULONGLONG);
        break;

      case JAVA::CJValueDef::typBooleanObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaLangBooleanToBoolean(env, cObj, &myJValueChunk_z[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_z[0] = JNI_FALSE;
            }
            lpDest[0] = (myJValueChunk_z[0] != JNI_FALSE) ? 1 : 0;
            lpDest++;
          }
        }
        nSize += nArrSize;
        break;

      case JAVA::CJValueDef::typByteObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaLangByteToByte(env, cObj, &myJValueChunk_b[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_b[0] = 0;
            }
            lpDest[0] = myJValueChunk_b[0];
            lpDest++;
          }
        }
        nSize += nArrSize;
        break;

      case JAVA::CJValueDef::typCharObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaLangCharacterToChar(env, cObj, &myJValueChunk_c[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_c[0] = 0;
            }
            *((USHORT NKT_UNALIGNED *)lpDest) = (USHORT)myJValueChunk_c[0];
            lpDest += sizeof(USHORT);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(USHORT);
        break;

      case JAVA::CJValueDef::typShortObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaLangShortToShort(env, cObj, &myJValueChunk_s[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_s[0] = 0;
            }
            *((USHORT NKT_UNALIGNED *)lpDest) = (USHORT)myJValueChunk_s[0];
            lpDest += sizeof(USHORT);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(USHORT);
        break;

      case JAVA::CJValueDef::typIntObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaLangIntegerToInteger(env, cObj, &myJValueChunk_i[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_i[0] = 0;
            }
            *((ULONG NKT_UNALIGNED *)lpDest) = (ULONG)myJValueChunk_i[0];
            lpDest += sizeof(ULONG);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(ULONG);
        break;

      case JAVA::CJValueDef::typLongObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaLangLongToLong(env, cObj, &myJValueChunk_j[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_j[0] = 0;
            }
            *((ULONGLONG NKT_UNALIGNED *)lpDest) = (ULONGLONG)myJValueChunk_j[0];
            lpDest += sizeof(ULONGLONG);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(ULONGLONG);
        break;

      case JAVA::CJValueDef::typFloatObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaLangFloatToFloat(env, cObj, &myJValueChunk_f[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_f[0] = 0.0f;
            }
            *((float NKT_UNALIGNED *)lpDest) = (float)myJValueChunk_f[0];
            lpDest += sizeof(float);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(float);
        break;

      case JAVA::CJValueDef::typDoubleObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaLangDoubleToDouble(env, cObj, &myJValueChunk_d[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_d[0] = 0.0;
            }
            *((double NKT_UNALIGNED *)lpDest) = (double)myJValueChunk_d[0];
            lpDest += sizeof(double);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(double);
        break;

      case JAVA::CJValueDef::typAtomicIntObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaUtilConcurrentAtomicIntegerToInteger(env, cObj, &myJValueChunk_i[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_i[0] = 0;
            }
            *((ULONG NKT_UNALIGNED *)lpDest) = (ULONG)myJValueChunk_i[0];
            lpDest += sizeof(ULONG);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(ULONG);
        break;

      case JAVA::CJValueDef::typAtomicLongObj:
        if (lpDest != NULL)
        {
          JAVA::TJavaAutoPtr<jobject> cObj;

          for (k=0; k<nArrSize; k++)
          {
            cObj.Release();
            cObj.Attach(env, env->GetObjectArrayElement((jobjectArray)arr, k));
            if (cObj != NULL)
            {
              hRes = lpJavaVarInterchange->JavaUtilConcurrentAtomicLongToLong(env, cObj, &myJValueChunk_j[0]);
              if (FAILED(hRes))
                return hRes;
            }
            else
            {
              myJValueChunk_j[0] = 0;
            }
            *((ULONGLONG NKT_UNALIGNED *)lpDest) = (ULONGLONG)myJValueChunk_j[0];
            lpDest += sizeof(ULONGLONG);
          }
        }
        nSize += (SIZE_T)nArrSize*sizeof(ULONGLONG);
        break;
    }
  }
  return S_OK;
}
#undef MARSHALJVALUES_ChunkSize
