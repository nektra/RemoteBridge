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

#pragma once

#include "..\RemoteBridgeHelper.h"
#include "javainc.h"
#include "..\..\Common\StringLiteW.h"
#include <crtdbg.h>

//-----------------------------------------------------------

namespace JAVA {

template <class T>
class TJavaAutoPtr
{
public:
  TJavaAutoPtr()
    {
    env = NULL;
    obj = NULL;
    bLocal = TRUE;
    return;
    };

  ~TJavaAutoPtr()
    {
    Release();
    return;
    };

  operator T()
    {
    return obj;
    };

  T Get()
    {
    return obj;
    };

  VOID Attach(__in JNIEnv *_env, __in T _obj, __in BOOL _bLocal=TRUE)
    {
    if (_env != env || _obj != obj)
    {
      Release();
      env = _env;
      obj = _obj;
      bLocal = _bLocal;
    }
    return;
    };

  T Detach()
    {
    T objCopy = obj;
    env = NULL;
    obj = NULL;
    bLocal = TRUE;
    return objCopy;
    };

  VOID Release()
    {
    if (obj != NULL)
    {
      if (bLocal != FALSE)
        env->DeleteLocalRef(obj);
      else
        env->DeleteGlobalRef(obj);
      Detach();
    }
    return;
    };

  BOOL operator!() const
    {
    return (obj == NULL) ? TRUE : FALSE;
    };

  BOOL operator==(T obj2) const
    {
    return (obj == obj2) ? TRUE : FALSE;
    };

  BOOL IsEqualObject(__in T obj2)
    {
    if (obj==NULL && obj2==NULL)
      return TRUE;
    if (obj==NULL || obj2==NULL)
      return FALSE;
    return (env->IsSameObject(obj, obj2) != JNI_FALSE) ? TRUE : FALSE;
    };

  JNIEnv* GetEnv()
    {
    return env;
    };

protected:
  JNIEnv *env;
  T obj;
  BOOL bLocal;
};

//----------------

class CAutoThreadEnv
{
public:
  CAutoThreadEnv(__in JavaVM *_lpVM)
    {
    lpVM = NULL;
    env = NULL;
    Setup(_lpVM);
    return;
    };

  ~CAutoThreadEnv()
    {
    Release();
    return;
    };

  VOID Setup(__in JavaVM *_lpVM)
    {
    Release();
    lpVM = _lpVM;
    if (lpVM != NULL)
    {
      if (lpVM->AttachCurrentThread((void **)&env, NULL) < 0)
        env = NULL;
    }
    return;
    };

  VOID Release()
    {
    if (env != NULL)
    {
      lpVM->DetachCurrentThread();
      env = NULL;
    }
    lpVM = NULL;
    return;
    };

  operator JNIEnv*()
    {
    return env;
    };

  JNIEnv* operator->() const
    {
    _ASSERT(env != NULL);
    return env;
    };

  JNIEnv* Get()
    {
    return env;
    };

private:
  JavaVM *lpVM;
  JNIEnv *env;
};

//----------------

HRESULT CheckJavaException(__in JNIEnv *env);
BOOL ThrowJavaException(__in JNIEnv* env, __in HRESULT hRes);

HRESULT GetJObjectName(__inout CNktStringW &cStrNameW, __in JNIEnv *env, __in jobject obj);

jobject GetClassJObjectOfJObject(__in JNIEnv *env, __in jobject obj);
jobject GetSuperclassJObjectOfJObject(__in JNIEnv *env, __in jobject obj);

HRESULT GetJObjectClassName(__inout CNktStringW &cStrClassNameW, __in JNIEnv *env, __in jobject obj);
HRESULT GetJClassName(__inout CNktStringW &cStrClassNameW, __in JNIEnv *env, __in jclass cls);

VOID NormalizeNameA(__in LPSTR szNameA, __in BOOL bToDot=FALSE);
VOID NormalizeNameW(__in LPWSTR szNameW, __in BOOL bToDot=FALSE);

} //JAVA
