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
#include "JavaObjectMgr.h"

//-----------------------------------------------------------

#define DELAYED_DELETION_THRESHOLD 10000

//-----------------------------------------------------------

namespace JAVA {

CJavaObjectManager::CJavaObjectManager()
{
  NktInterlockedExchange(&nMutex, 0);
  return;
}

CJavaObjectManager::~CJavaObjectManager()
{
  RemoveAll(NULL);
  return;
}

HRESULT CJavaObjectManager::Add(__in JNIEnv *env, __in jobject obj, __out_opt jobject *lpOrigObj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  //SIZE_T nIndex, nCount;
  ITEM sNewItem;

  _ASSERT(env != NULL);
  if (lpOrigObj != NULL)
    *lpOrigObj = NULL;
  if (obj == NULL)
    return E_POINTER;
  ////COMMENTED CODE NOTE: For simplicity we create a new global reference for any local one no matter
  ////                     if the object is the same. JNI uses global & local references in the same way.
  ////                     When an object has no references, it will become eligible for garbage collection.
  ////check if obj points to another instance of the same object
  //nCount = aList.GetCount();
  //for (nIndex=0; nIndex<nCount; nIndex++)
  //{
  //  if (env->IsSameObject(obj, aList[nIndex]) != JNI_FALSE)
  //  {
  //    if (lpOrigObj != NULL)
  //      *lpOrigObj = aList[nIndex];
  //    return S_FALSE; //found a match
  //  }
  //}
  //insert into list
  sNewItem.globalObj = env->NewGlobalRef(obj);
  if (sNewItem.globalObj == NULL)
    return E_OUTOFMEMORY;
  /*
  sNewItem.dwMarkedForDeletionTick = 0;
  */
  if (aList.SortedInsert(&sNewItem, &CJavaObjectManager::JavaObject_Compare, NULL) == FALSE)
  {
    env->DeleteGlobalRef(sNewItem.globalObj);
    return E_OUTOFMEMORY;
  }
  if (lpOrigObj != NULL)
    *lpOrigObj = sNewItem.globalObj;
  return S_OK;
}

HRESULT CJavaObjectManager::Add(__in JNIEnv *env, __in jobject obj, __out TJavaAutoPtr<jobject> &cGlobalObj)
{
  jobject globalObj;
  HRESULT hRes;

  hRes = Add(env, obj, &globalObj);
  if (SUCCEEDED(hRes))
    cGlobalObj.Attach(env, globalObj);
  return hRes;
}

VOID CJavaObjectManager::Remove(__in JNIEnv *env, __in jobject obj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  ITEM sSearchItem;
  /*DWORD dwCurrTick;*/
  SIZE_T nIndex/*, nCount*/;

  _ASSERT(env != NULL);
  sSearchItem.globalObj = obj;
  /*dwCurrTick = ::GetTickCount();*/
  nIndex = aList.BinarySearch(&sSearchItem, CJavaObjectManager::JavaObject_Compare, NULL);
  if (nIndex != NKT_SIZE_T_MAX)
  {
    /*
    aList[nIndex].dwMarkedForDeletionTick = dwCurrTick;
    if (aList[nIndex].dwMarkedForDeletionTick == 0)
      aList[nIndex].dwMarkedForDeletionTick = 1;
    */
    env->DeleteGlobalRef(aList[nIndex].globalObj);
    aList.RemoveElementAt(nIndex);
  }
  /*
  nCount = aList.GetCount();
  nIndex = 0;
  while (nIndex < nCount)
  {
    if (aList[nIndex].dwMarkedForDeletionTick != 0)
    {
      if (dwCurrTick - aList[nIndex].dwMarkedForDeletionTick >= DELAYED_DELETION_THRESHOLD)
      {
        env->DeleteGlobalRef(aList[nIndex].globalObj);
        nCount--;
        continue;
      }
    }
    nIndex++;
  }
  */
  return;
}

VOID CJavaObjectManager::RemoveAll(__in JNIEnv *env)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  SIZE_T nIndex, nCount;

  if (env != NULL)
  {
    nCount = aList.GetCount();
    for (nIndex=0; nIndex<nCount; nIndex++)
      env->DeleteGlobalRef(aList[nIndex].globalObj);
  }
  aList.RemoveAllElements();
  return;
}

HRESULT CJavaObjectManager::GetLocalRef(__in JNIEnv *env, __in jobject obj, __out jobject *lpLocalObj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  ITEM sSearchItem;
  SIZE_T nIndex;

  _ASSERT(env != NULL);
  if (lpLocalObj != NULL)
    *lpLocalObj = NULL;
  if (obj == NULL || lpLocalObj == NULL)
    return E_POINTER;
  sSearchItem.globalObj = obj;
  nIndex = aList.BinarySearch(&sSearchItem, &CJavaObjectManager::JavaObject_Compare, NULL);
  if (nIndex != NKT_SIZE_T_MAX)
  {
    *lpLocalObj = env->NewLocalRef(aList[nIndex].globalObj);
    return ((*lpLocalObj) != NULL) ? S_OK : E_OUTOFMEMORY;
  }
  *lpLocalObj = NULL;
  return NKT_E_NotFound;
}

HRESULT CJavaObjectManager::GetLocalRef(__in JNIEnv *env, __in jobject obj, __out TJavaAutoPtr<jobject> &cLocalObj)
{
  jobject localObj;
  HRESULT hRes;

  hRes = GetLocalRef(env, obj, &localObj);
  if (SUCCEEDED(hRes))
    cLocalObj.Attach(env, localObj);
  return hRes;
}

int CJavaObjectManager::JavaObject_Compare(__in void *, __in ITEM *lpElem1, __in ITEM *lpElem2)
{
  if ((SIZE_T)(lpElem1->globalObj) < (SIZE_T)(lpElem2->globalObj))
    return -1;
  if ((SIZE_T)(lpElem1->globalObj) > (SIZE_T)(lpElem2->globalObj))
    return 1;
  return 0;
}

} //JAVA

