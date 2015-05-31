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
#include "JavaUtils.h"
#include "..\..\Common\WaitableObjects.h"
#include <crtdbg.h>

//-----------------------------------------------------------

namespace JAVA {

HRESULT CheckJavaException(__in JNIEnv *env)
{
  static struct {
    LPCSTR szNameA;
    HRESULT hRes;
  } aExceptionsList[] = {
    { "java/lang/OutOfMemoryError", E_OUTOFMEMORY },
    { "java/lang/IllegalArgumentException", E_INVALIDARG },
    { "java/lang/NullPointerException", E_POINTER },
    { NULL, E_FAIL }
  };
  TJavaAutoPtr<jthrowable> cExc;
  TJavaAutoPtr<jclass> cCls;
  ULONG i;

  _ASSERT(env != NULL);
  cExc.Attach(env, env->ExceptionOccurred());
  if (cExc == NULL)
    return S_OK;
#ifdef _DEBUG
  env->ExceptionDescribe();
#endif //_DEBUG
  env->ExceptionClear(); //clear exception in order to let next calls to work as expected (jni rulez)
  for (i=0; aExceptionsList[i].szNameA!=NULL; i++)
  {
    cCls.Attach(env, env->FindClass(aExceptionsList[i].szNameA));
    if (cCls != NULL && env->IsInstanceOf(cExc, cCls) != JNI_FALSE)
      return aExceptionsList[i].hRes;
    cCls.Release();
  }
  return E_FAIL;
}

BOOL ThrowJavaException(__in JNIEnv* env, __in HRESULT hRes)
{
  TJavaAutoPtr<jclass> cCls;
  CHAR szBufA[256], *sA;

  switch (hRes)
  {
    case E_OUTOFMEMORY:
      sA = "java/lang/OutOfMemoryError";
      break;
    case E_INVALIDARG:
      sA = "java/lang/IllegalArgumentException";
      break;
    case E_POINTER:
      sA = "java/lang/NullPointerException";
      break;
    default:
      sA = "java/lang/InternalError";
      break;
  }
  cCls.Attach(env, env->FindClass(sA));
  if (cCls == NULL)
    return FALSE;
  sprintf_s(szBufA, NKT_ARRAYLEN(szBufA), "RemoteBridge Error Code: %ld", hRes);
  env->ThrowNew(cCls, szBufA);
  return TRUE;
}

HRESULT GetJObjectName(__inout CNktStringW &cStrNameW, __in JNIEnv *env, __in jobject obj)
{
  TJavaAutoPtr<jobject> cClsObj;
  TJavaAutoPtr<jclass> cClsDescr;
  TJavaAutoPtr<jstring> cStrName;
  jmethodID getName_MethodId;
  jboolean isCopy;
  const jchar *sW;
  SIZE_T nLen;

  _ASSERT(env != NULL);
  cStrNameW.Empty();
  cClsDescr.Attach(env, env->GetObjectClass(obj));
  if (cClsDescr == NULL)
    return E_FAIL;
  getName_MethodId = env->GetMethodID(cClsDescr, "getName", "()Ljava/lang/String;");
  if (getName_MethodId == NULL)
    return E_FAIL;
  cStrName.Attach(env, (jstring)(env->CallObjectMethod(obj, getName_MethodId)));
  if (cStrName == NULL)
    return E_OUTOFMEMORY;
  nLen = (SIZE_T)(env->GetStringLength(cStrName)); //before string critical
  if (nLen > 0)
  {
    sW = env->GetStringCritical(cStrName, &isCopy);
    if (sW == NULL)
      return E_OUTOFMEMORY;
    if (cStrNameW.CopyN((LPCWSTR)sW, nLen) == FALSE)
    {
      env->ReleaseStringCritical(cStrName, sW);
      return E_OUTOFMEMORY;
    }
    env->ReleaseStringCritical(cStrName, sW);
    NormalizeNameW((LPWSTR)cStrNameW);
  }
  return S_OK;
}

jobject GetClassJObjectOfJObject(__in JNIEnv *env, __in jobject obj)
{
  TJavaAutoPtr<jclass> cCls;
  jmethodID getClass_MethodId;

  _ASSERT(env != NULL);
  if (obj == NULL)
    return NULL;
  cCls.Attach(env, env->GetObjectClass(obj));
  if (cCls == NULL)
    return NULL;
  getClass_MethodId = env->GetMethodID(cCls, "getClass", "()Ljava/lang/Class;");
  return env->CallObjectMethod(obj, getClass_MethodId);
}

jobject GetSuperclassJObjectOfJObject(__in JNIEnv *env, __in jobject obj)
{
  TJavaAutoPtr<jclass> cCls;
  jmethodID getSuperclass_MethodId;

  _ASSERT(env != NULL);
  if (obj == NULL)
    return NULL;
  cCls.Attach(env, env->GetObjectClass(obj));
  if (cCls == NULL)
    return NULL;
  getSuperclass_MethodId = env->GetMethodID(cCls, "getSuperclass", "()Ljava/lang/Class;");
  return env->CallObjectMethod(obj, getSuperclass_MethodId);
}

HRESULT GetJObjectClassName(__inout CNktStringW &cStrClassNameW, __in JNIEnv *env, __in jobject obj)
{
  TJavaAutoPtr<jobject> cClsObj;
  TJavaAutoPtr<jclass> cClsDescr;
  TJavaAutoPtr<jstring> cStrName;
  jmethodID getName_MethodId;
  jboolean isCopy;
  const jchar *sW;
  SIZE_T nLen;

  _ASSERT(env != NULL);
  cStrClassNameW.Empty();
  cClsObj.Attach(env, GetClassJObjectOfJObject(env, obj));
  if (cClsObj == NULL)
    return E_FAIL;
  cClsDescr.Attach(env, env->GetObjectClass(cClsObj));
  if (cClsDescr == NULL)
    return E_FAIL;
  getName_MethodId = env->GetMethodID(cClsDescr, "getName", "()Ljava/lang/String;");
  if (getName_MethodId == NULL)
    return E_FAIL;
  cStrName.Attach(env, (jstring)(env->CallObjectMethod(cClsObj, getName_MethodId)));
  if (cStrName == NULL)
    return E_OUTOFMEMORY;
  nLen = (SIZE_T)(env->GetStringLength(cStrName)); //before string critical
  if (nLen > 0)
  {
    sW = env->GetStringCritical(cStrName, &isCopy);
    if (sW == NULL)
      return E_OUTOFMEMORY;
    if (cStrClassNameW.CopyN((LPCWSTR)sW, nLen) == FALSE)
    {
      env->ReleaseStringCritical(cStrName, sW);
      return E_OUTOFMEMORY;
    }
    env->ReleaseStringCritical(cStrName, sW);
    NormalizeNameW((LPWSTR)cStrClassNameW);
  }
  return S_OK;
}

HRESULT GetJClassName(__inout CNktStringW &cStrClassNameW, __in JNIEnv *env, __in jclass cls)
{
  TJavaAutoPtr<jclass> cClsDescr;
  TJavaAutoPtr<jstring> cStrName;
  jmethodID getName_MethodId;
  jboolean isCopy;
  const jchar *sW;
  SIZE_T nLen;

  _ASSERT(env != NULL);
  cStrClassNameW.Empty();
  cClsDescr.Attach(env, env->GetObjectClass(cls));
  if (cClsDescr == NULL)
    return E_FAIL;
  getName_MethodId = env->GetMethodID(cClsDescr, "getName", "()Ljava/lang/String;");
  if (getName_MethodId == NULL)
    return E_FAIL;
  cStrName.Attach(env, (jstring)(env->CallObjectMethod(cls, getName_MethodId)));
  if (cStrName == NULL)
    return E_OUTOFMEMORY;
  nLen = (SIZE_T)(env->GetStringLength(cStrName)); //before string critical
  if (nLen > 0)
  {
    sW = env->GetStringCritical(cStrName, &isCopy);
    if (sW == NULL)
      return E_OUTOFMEMORY;
    if (cStrClassNameW.CopyN((LPCWSTR)sW, nLen) == FALSE)
    {
      env->ReleaseStringCritical(cStrName, sW);
      return E_OUTOFMEMORY;
    }
    env->ReleaseStringCritical(cStrName, sW);
    NormalizeNameW((LPWSTR)cStrClassNameW);
  }
  return S_OK;
}

VOID NormalizeNameA(__in LPSTR szNameA, __in BOOL bToDot)
{
  if (bToDot == FALSE)
  {
    while (szNameA[0] != 0)
    {
      if (szNameA[0] == '.')
        szNameA[0] = '/';
      szNameA++;
    }
  }
  else
  {
    while (szNameA[0] != 0)
    {
      if (szNameA[0] == '/')
        szNameA[0] = '.';
      szNameA++;
    }
  }
  return;
}

VOID NormalizeNameW(__in LPWSTR szNameW, __in BOOL bToDot)
{
  if (bToDot == FALSE)
  {
    while (szNameW[0] != 0)
    {
      if (szNameW[0] == L'.')
        szNameW[0] = L'/';
      szNameW++;
    }
  }
  else
  {
    while (szNameW[0] != 0)
    {
      if (szNameW[0] == L'/')
        szNameW[0] = L'.';
      szNameW++;
    }
  }
  return;
}

} //JAVA
