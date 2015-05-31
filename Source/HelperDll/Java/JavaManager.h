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
#include <winternl.h>
#include "..\..\Common\WaitableObjects.h"
#include "..\..\Common\Threads.h"
#include "javainc.h"
#include "JavaObjectMgr.h"
#include "JavaClassCacheMgr.h"
#include "JavaVarConvert.h"

//-----------------------------------------------------------

namespace JAVA {

class CMainManager
{
public:
  class CCallbacks
  {
  public:
    virtual HRESULT OnJavaRaiseJavaNativeMethodCalled(__in TMSG_RAISEONJAVANATIVEMETHODCALLED *lpMsg,
                                   __in SIZE_T nMsgSize, __out TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY **lplpReply,
                                   __out SIZE_T *lpnReplySize) = 0;
    virtual LPBYTE OnJavaAllocIpcBuffer(__in SIZE_T nSize) = 0;
    virtual VOID OnJavaFreeIpcBuffer(__in LPVOID lpBuffer) = 0;
    virtual HRESULT OnJavaBuildSimpleReplyData(__in HRESULT hRes, __out LPVOID *lplpReplyData,
                                               __out SIZE_T *lpnReplyDataSize) = 0;
    virtual VOID OnJavaHookUsage(__in BOOL bEntering) = 0;
  };

public:
  CMainManager(__in CCallbacks *lpCallback, __in CNktHookLib *lpHookMgr);
  ~CMainManager();

  HRESULT Initialize();

  VOID OnExitProcess();

  HANDLE GetHeap()
    {
    return hHeap;
    };

  CNktHookLib* GetHookMgr()
    {
    return lpHookMgr;
    };

  DWORD GetTlsIndex()
    {
    return dwTlsIndex;
    };

  HRESULT ProcessDllLoad(__in HINSTANCE hDll);
  HRESULT ProcessDllUnload(__in HINSTANCE hDll);

  HRESULT DefineJavaClass(__in TMSG_DEFINEJAVACLASS *lpMsg, __in SIZE_T nDataSize,
                          __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

  HRESULT CreateJavaClass(__in TMSG_CREATEJAVAOBJECT *lpMsg, __in SIZE_T nDataSize,
                          __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

  HRESULT InvokeJavaMethod(__in TMSG_INVOKEJAVAMETHOD *lpMsg, __in SIZE_T nDataSize,
                           __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

  HRESULT SetJavaField(__in TMSG_PUTJAVAFIELD *lpMsg, __in SIZE_T nDataSize, __out LPVOID *lplpReplyData,
                       __out SIZE_T *lpnReplyDataSize);

  HRESULT GetJavaField(__in TMSG_GETJAVAFIELD *lpMsg, __in SIZE_T nDataSize, __out LPVOID *lplpReplyData,
                       __out SIZE_T *lpnReplyDataSize);

  HRESULT IsSameJavaObject(__in TMSG_ISSAMEJAVAOBJECT *lpMsg, __in SIZE_T nDataSize, __out LPVOID *lplpReplyData,
                           __out SIZE_T *lpnReplyDataSize);

  HRESULT RemoveJObjectRef(__in TMSG_REMOVEJOBJECTREF *lpMsg, __in SIZE_T nDataSize);

  HRESULT IsJVMAttached(__in TMSG_HEADER *lpMsgHdr, __in SIZE_T nDataSize, __out LPVOID *lplpReplyData,
                        __out SIZE_T *lpnReplyDataSize);

  HRESULT GetJavaObjectClass(__in TMSG_GETJAVAOBJECTCLASS *lpMsg, __in SIZE_T nDataSize, __out LPVOID *lplpReplyData,
                             __out SIZE_T *lpnReplyDataSize);

  HRESULT IsJavaObjectInstanceOf(__in TMSG_ISJAVAOBJECTINSTANCEOF *lpMsg, __in SIZE_T nDataSize,
                                 __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

private:
  HRESULT DetectJVM();
  HRESULT AttachJVM(__in JavaVM *vm, __in JNIEnv *env);
  VOID DetachJVM(__in JavaVM *vm, __in BOOL bInExitChain);
  VOID DetachJVM_2(__in JavaVM *vm, __in BOOL bInExitChain, __in JNIEnv *env);
  VOID OnCreateJavaVM(__in JavaVM *vm, __in JNIEnv *env);
  static VOID __stdcall CreateJavaVMStub(__in JAVA::CMainManager *lpCtx, __in JavaVM **pvm, __in JNIEnv **penv);

  friend class CMethodOrFieldDef;
  HRESULT OnNativeRaiseCallback(__in JNIEnv* env, __in_z LPCWSTR szClassNameW, __in_z LPCWSTR szMethodNameW,
                                __in jobject objOrClass, __in jvalue *lpParams,
                                __in CMethodOrFieldDef *lpMethodOrFieldDef, __out jvalue *lpRetVal);

private:
  CCallbacks *lpCallback;
  CNktHookLib *lpHookMgr;
  HANDLE hHeap;
  DWORD dwTlsIndex;
  //----
  struct {
    LONG volatile nDllMutex;
    CNktFastReadWriteLock cRwMtx;
    JavaVM *lpVM;
    WORD wVersionMajor, wVersionMinor;
    HINSTANCE hDll;
    lpfnJNI_CreateJavaVM fnJNI_CreateJavaVM;
    lpfnJNI_GetCreatedJavaVMs fnJNI_GetCreatedJavaVMs;
    lpfnJVM_OnExit fnJVM_OnExit;
    CNktHookLib::HOOK_INFO sCreateJavaVM;
  } sJVM;
  //----
  CJavaVariantInterchange cJavaVarInterchange;
  CJavaObjectManager cJObjectMgr;
  CJavaClassCacheManager cJClassCacheMgr;
  LONG volatile nIsExiting;

public:
  ULONG nHookFlags;
};

} //JAVA
