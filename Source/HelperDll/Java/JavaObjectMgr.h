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
#include "JavaManager.h"
#include "..\..\Common\ArrayList.h"
#include "..\..\Common\WaitableObjects.h"
#include "JavaUtils.h"
namespace JAVA {
  class CMainManager;
}

//-----------------------------------------------------------

namespace JAVA {

class CJavaObjectManager
{
public:
  CJavaObjectManager();
  ~CJavaObjectManager();

  HRESULT Add(__in JNIEnv *env, __in jobject obj, __out_opt jobject *lpOrigObj);
  HRESULT Add(__in JNIEnv *env, __in jobject obj, __out TJavaAutoPtr<jobject> &cGlobalObj);
  VOID Remove(__in JNIEnv *env, __in jobject obj);
  VOID RemoveAll(__in JNIEnv *env);

  HRESULT GetLocalRef(__in JNIEnv *env, __in jobject obj, __out jobject *lpLocalObj);
  HRESULT GetLocalRef(__in JNIEnv *env, __in jobject obj, __out TJavaAutoPtr<jobject> &cLocalObj);

private:
  typedef struct {
    jobject globalObj;
    //DWORD dwMarkedForDeletionTick;
  } ITEM;

  static int JavaObject_Compare(__in void *, __in ITEM*, __in ITEM*);

private:
  LONG volatile nMutex;
  TNktArrayList4Structs<ITEM> aList;
};

} //JAVA
