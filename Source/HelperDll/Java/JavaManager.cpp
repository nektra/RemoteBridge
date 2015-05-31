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
#include "..\HookTrampoline.h"
#include "..\..\Common\MallocMacros.h"
#include "..\..\Common\AutoPtr.h"
#include "..\..\Common\ComPtr.h"
#include "..\..\Common\FnvHash.h"
#include "..\..\Common\SimplePipesIPC.h"
#include "..\AutoTls.h"
#include "JavaUtils.h"

#define X_BYTE_ENC(x, y) (x)
#if defined _M_IX86
  static
  #include "Asm\CreateJavaVM_x86.inl"
#elif defined _M_X64
  static
  #include "Asm\CreateJavaVM_x64.inl"
#else
  #error Platform not supported
#endif

//-----------------------------------------------------------

static JAVA::CMethodOrFieldDef* FindCompatibleMethodOrField(__in JNIEnv *env, __in JAVA::CClassDef *lpClassDef,
                                                            __in int nType, __in LPCWSTR szNameW,
                                                            __in JAVA::CUnmarshalledJValues *lpPeekedValues);

namespace JAVA {

#pragma warning(disable : 4355)
CMainManager::CMainManager(__in CCallbacks *_lpCallback, __in CNktHookLib *_lpHookMgr) : cJClassCacheMgr(this)
{
  _ASSERT(_lpCallback != NULL && _lpHookMgr != NULL);
  lpCallback  = _lpCallback;
  lpHookMgr = _lpHookMgr;
  NktInterlockedExchange(&nIsExiting, 0);
  nHookFlags = 0;
  NktInterlockedExchange(&(sJVM.nDllMutex), 0);
  sJVM.hDll = NULL;
  sJVM.lpVM = NULL;
  sJVM.wVersionMajor = sJVM.wVersionMinor = 0;
  sJVM.fnJNI_CreateJavaVM = NULL;
  sJVM.fnJNI_GetCreatedJavaVMs = NULL;
  memset(&(sJVM.sCreateJavaVM), 0, sizeof(sJVM.sCreateJavaVM));
#ifdef _DEBUG
  hHeap = ::HeapCreate(HEAP_CREATE_ENABLE_EXECUTE|HEAP_TAIL_CHECKING_ENABLED|HEAP_FREE_CHECKING_ENABLED, 0, 0);
#else //_DEBUG
  hHeap = ::HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
#endif //_DEBUG
  dwTlsIndex = ::TlsAlloc();
  return;
}
#pragma warning(default : 4355)

CMainManager::~CMainManager()
{
  ProcessDllUnload(sJVM.hDll);
  //----
  if (hHeap != NULL)
    ::HeapDestroy(hHeap);
  if (dwTlsIndex != TLS_OUT_OF_INDEXES)
    ::TlsFree(dwTlsIndex);
  return;
}

HRESULT CMainManager::Initialize()
{
  if (hHeap == NULL || dwTlsIndex == TLS_OUT_OF_INDEXES)
    return E_OUTOFMEMORY;
  //----
  return S_OK;
}

VOID CMainManager::OnExitProcess()
{
  NktInterlockedExchange(&nIsExiting, 1);
  return;
}

HRESULT CMainManager::ProcessDllLoad(__in HINSTANCE hDll)
{
  WCHAR szBufW[8192];
  SIZE_T i;
  HRESULT hRes;

  i = (SIZE_T)::GetModuleFileNameW(hDll, szBufW, NKT_ARRAYLEN(szBufW));
  while (i > 0 && szBufW[i-1] != L'/' && szBufW[i-1] != L'\\')
    i--;
  if (_wcsicmp(szBufW+i, L"jvm.dll") == 0)
  {
    CNktSimpleLockNonReentrant cDllLock(&(sJVM.nDllMutex));

    sJVM.hDll = hDll;
    sJVM.fnJNI_CreateJavaVM = (lpfnJNI_CreateJavaVM)::GetProcAddress(sJVM.hDll, "JNI_CreateJavaVM");
    sJVM.fnJNI_GetCreatedJavaVMs = (lpfnJNI_GetCreatedJavaVMs)::GetProcAddress(sJVM.hDll, "JNI_GetCreatedJavaVMs");
    sJVM.fnJVM_OnExit = (lpfnJVM_OnExit)::GetProcAddress(sJVM.hDll, "JVM_OnExit");
    if (sJVM.fnJVM_OnExit == NULL)
      sJVM.fnJVM_OnExit = (lpfnJVM_OnExit)::GetProcAddress(sJVM.hDll, "_JVM_OnExit@4");
    if (sJVM.fnJNI_CreateJavaVM != NULL && sJVM.fnJNI_GetCreatedJavaVMs != NULL &&
        sJVM.fnJVM_OnExit != NULL)
    {
      sJVM.sCreateJavaVM.lpProcToHook = sJVM.fnJNI_CreateJavaVM;
      sJVM.sCreateJavaVM.lpNewProcAddr = HOOKTRAMPOLINE2CREATE(hHeap, aCreateJavaVM, &CMainManager::CreateJavaVMStub,
                                                               FALSE);
      if (sJVM.sCreateJavaVM.lpNewProcAddr == NULL)
        return E_OUTOFMEMORY;
      hRes = NKT_HRESULT_FROM_WIN32(lpHookMgr->Hook(&(sJVM.sCreateJavaVM), 1));
      if (FAILED(hRes))
      {
        HookTrampoline2_Delete(hHeap, sJVM.sCreateJavaVM.lpNewProcAddr);
        memset(&(sJVM.sCreateJavaVM), 0, sizeof(sJVM.sCreateJavaVM));
        return hRes;
      }
      HookTrampoline2_SetCallOriginalProc(sJVM.sCreateJavaVM.lpNewProcAddr, sJVM.sCreateJavaVM.lpCallOriginal);
      //----
      hRes = DetectJVM();
      if (FAILED(hRes))
        return hRes;
    }
    else
    {
      sJVM.fnJNI_CreateJavaVM = NULL;
      sJVM.fnJNI_GetCreatedJavaVMs = NULL;
      sJVM.fnJVM_OnExit = NULL;
      sJVM.hDll = NULL;
    }
  }
  return S_OK;
}

HRESULT CMainManager::ProcessDllUnload(__in HINSTANCE hDll)
{
  if (sJVM.hDll != NULL && sJVM.hDll == hDll)
  {
    CNktSimpleLockNonReentrant cDllLock(&(sJVM.nDllMutex));

    if (sJVM.hDll != NULL && sJVM.hDll == hDll)
    {
      DetachJVM(NULL, FALSE);
      //----
      HookTrampoline2_Disable(sJVM.sCreateJavaVM.lpNewProcAddr, TRUE);
      lpHookMgr->Unhook(&(sJVM.sCreateJavaVM), 1);
      HookTrampoline2_Delete(hHeap, sJVM.sCreateJavaVM.lpNewProcAddr);
      memset(&sJVM.sCreateJavaVM, 0, sizeof(sJVM.sCreateJavaVM));
      //----
      sJVM.fnJNI_CreateJavaVM = NULL;
      sJVM.fnJNI_GetCreatedJavaVMs = NULL;
      sJVM.hDll = NULL;
    }
  }
  return S_OK;
}

HRESULT CMainManager::DefineJavaClass(__in TMSG_DEFINEJAVACLASS *lpMsg, __in SIZE_T nDataSize,
                                      __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  LPBYTE lpMsgData;
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_DEFINEJAVACLASS) ||
      nDataSize != sizeof(TMSG_DEFINEJAVACLASS)+(SIZE_T)(lpMsg->nByteCodeLen+lpMsg->nClassLen) ||
      lpMsg->nByteCodeLen < 1 || lpMsg->nByteCodeLen > (SIZE_T)0x7FFFFFFFUL ||
      lpMsg->nClassLen < 2 || (lpMsg->nClassLen & 1) != 0)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);
    TNktAutoFreePtr<CHAR> sClassNameA;
    TNktComPtr<CClassDef> cClassDef;
    TJavaAutoPtr<jclass> cCls;

    if (cEnv == NULL)
      return E_OUTOFMEMORY;
    lpMsgData = (LPBYTE)(lpMsg+1);
    sClassNameA.Attach(CNktStringW::Wide2Ansi((LPCWSTR)lpMsgData, (SIZE_T)(lpMsg->nClassLen/sizeof(WCHAR))));
    if (sClassNameA == NULL)
      return E_OUTOFMEMORY;
    NormalizeNameA(sClassNameA.Get());
    //define new class
    cCls.Attach(cEnv, cEnv->DefineClass(sClassNameA.Get(), NULL, (jbyte*)(lpMsgData+(SIZE_T)(lpMsg->nClassLen)),
                                        (jsize)(lpMsg->nByteCodeLen)));
    if (cCls != NULL)
    {
      hRes = cJClassCacheMgr.FindOrCreateClass(cEnv, cCls, &cClassDef);
    }
    else
    {
      //check for errors
      hRes = CheckJavaException(cEnv);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
    //register native calls
    if (SUCCEEDED(hRes))
      hRes = cClassDef->RegisterNatives(cEnv, hHeap);
    //done
    if (hRes == E_OUTOFMEMORY)
      return hRes;
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
  }
  return lpCallback->OnJavaBuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
}

HRESULT CMainManager::CreateJavaClass(__in TMSG_CREATEJAVAOBJECT *lpMsg, __in SIZE_T nDataSize,
                                      __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  LPBYTE lpMsgData;
  jobject globalObj;
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_CREATEJAVAOBJECT))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (nDataSize < sizeof(TMSG_CREATEJAVAOBJECT)+(SIZE_T)(lpMsg->nClassLen+lpMsg->nParametersLen))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (lpMsg->nClassLen < 2 || (lpMsg->nClassLen & 1) != 0 ||
      lpMsg->nParametersLen < 0 || lpMsg->nParametersLen > (SIZE_T)0x7FFFFFFFUL)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);
    TNktAutoFreePtr<CHAR> sClassNameA;
    TJavaAutoPtr<jclass> cCls;
    TNktAutoDeletePtr<CUnmarshalledJValues> cValues;
    TNktComPtr<CClassDef> cClassDef;
    TJavaAutoPtr<jobject> cNewObj;
    CMethodOrFieldDef *lpMethodDef;

    globalObj = 0;
    if (cEnv == NULL)
      return E_OUTOFMEMORY;
    lpMsgData = (LPBYTE)(lpMsg+1);
    sClassNameA.Attach(CNktStringW::Wide2Ansi((LPCWSTR)lpMsgData, (SIZE_T)(lpMsg->nClassLen)/sizeof(WCHAR)));
    if (sClassNameA == NULL)
      return E_OUTOFMEMORY;
    NormalizeNameA(sClassNameA.Get());
    //find the class
    cCls.Attach(cEnv, cEnv->FindClass(sClassNameA));
    if (cCls != NULL)
    {
      hRes = S_OK;
    }
    else
    {
      //check for errors
      hRes = CheckJavaException(cEnv);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
    if (SUCCEEDED(hRes))
    {
      hRes = cJClassCacheMgr.FindOrCreateClass(cEnv, cCls, &cClassDef);
    }
    //peek parameter types
    if (SUCCEEDED(hRes))
    {
      hRes = VarConvert_UnmarshalJValues_PeekTypes(cEnv, lpMsgData+(SIZE_T)(lpMsg->nClassLen),
                                                   (SIZE_T)(lpMsg->nParametersLen), &cJObjectMgr, &cValues);
    }
    //find best matching method
    if (SUCCEEDED(hRes))
    {
      lpMethodDef = FindCompatibleMethodOrField(cEnv, cClassDef, 1, NULL, cValues);
      if (lpMethodDef == NULL)
        hRes = E_INVALIDARG;
    }
    //retrieve parameter matching the provided method
    if (SUCCEEDED(hRes))
    {
      cValues.Reset();
      hRes = VarConvert_UnmarshalJValues(cEnv, lpMsgData+(SIZE_T)(lpMsg->nClassLen), (SIZE_T)(lpMsg->nParametersLen),
                                         &cJavaVarInterchange, &cJObjectMgr, lpMethodDef, FALSE, &cValues);
    }
    //create new object
    if (SUCCEEDED(hRes))
    {
      cNewObj.Attach(cEnv, cEnv->NewObjectA(cCls, lpMethodDef->nMethodId, cValues->GetJValues()));
      if (cNewObj == NULL)
      {
        hRes = CheckJavaException(cEnv);
        if (SUCCEEDED(hRes))
          hRes = E_FAIL;
      }
    }
    //add to the global ref table
    if (SUCCEEDED(hRes))
      hRes = cJObjectMgr.Add(cEnv, cNewObj, &globalObj);
    //build response
    if (hRes == E_OUTOFMEMORY)
      return hRes;
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
  }
  *lplpReplyData = lpCallback->OnJavaAllocIpcBuffer(sizeof(TMSG_CREATEJAVAOBJECT_REPLY));
  if ((*lplpReplyData) == NULL)
    return E_OUTOFMEMORY;
  ((TMSG_CREATEJAVAOBJECT_REPLY*)(*lplpReplyData))->hRes = hRes;
  ((TMSG_CREATEJAVAOBJECT_REPLY*)(*lplpReplyData))->javaobj = NKT_PTR_2_ULONGLONG(globalObj);
  *lpnReplyDataSize = sizeof(TMSG_CREATEJAVAOBJECT_REPLY);
  return S_OK;
}

HRESULT CMainManager::InvokeJavaMethod(__in TMSG_INVOKEJAVAMETHOD *lpMsg, __in SIZE_T nDataSize,
                                       __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  LPBYTE lpMsgData;
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_INVOKEJAVAMETHOD))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (nDataSize != sizeof(TMSG_INVOKEJAVAMETHOD)+(SIZE_T)(lpMsg->nClassLen+lpMsg->nMethodLen+lpMsg->nParametersLen))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (lpMsg->javaobj == 0)
  {
    if (lpMsg->nClassLen == 0 || (lpMsg->nClassLen & 1) != 0)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
  }
  else
  {
    if (lpMsg->nClassLen > 0 && (lpMsg->nClassLen & 1) != 0)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
  }
  if (lpMsg->nMethodLen < 2 || (lpMsg->nMethodLen & 1) != 0 ||
      lpMsg->nParametersLen < sizeof(ULONG) || lpMsg->nParametersLen > 0x7FFFFFFFui64)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);
    TNktAutoFreePtr<CHAR> sClassNameA;
    CNktStringW cStrMethodW;
    TJavaAutoPtr<jclass> cCls;
    TJavaAutoPtr<jobject> cObj, cRetObj;
    TNktAutoDeletePtr<CUnmarshalledJValues> cValues;
    TNktComPtr<CClassDef> cClassDef;
    CMethodOrFieldDef *lpMethodDef;
    jvalue varRet;

    if (cEnv == NULL)
      return E_OUTOFMEMORY;
    lpMsgData = (LPBYTE)(lpMsg+1);
    if (lpMsg->nClassLen > 0)
    {
      sClassNameA.Attach(CNktStringW::Wide2Ansi((LPCWSTR)lpMsgData, (SIZE_T)(lpMsg->nClassLen)/sizeof(WCHAR)));
      if (sClassNameA == NULL)
        return E_OUTOFMEMORY;
      NormalizeNameA(sClassNameA.Get());
    }
    if (cStrMethodW.CopyN((LPCWSTR)(lpMsgData+(SIZE_T)(lpMsg->nClassLen)),
                          (SIZE_T)(lpMsg->nMethodLen)/sizeof(WCHAR)) == FALSE)
      return E_OUTOFMEMORY;
    //get the object
    hRes = (lpMsg->javaobj > 0) ? cJObjectMgr.GetLocalRef(cEnv, (jobject)(lpMsg->javaobj), cObj) : S_OK;
    //find the class or superclass
    if (SUCCEEDED(hRes))
    {
      if (lpMsg->nClassLen > 0)
        cCls.Attach(cEnv, cEnv->FindClass(sClassNameA));
      else
        cCls.Attach(cEnv, cEnv->GetObjectClass(cObj));
      if (cCls != NULL)
      {
        hRes = cJClassCacheMgr.FindOrCreateClass(cEnv, cCls, &cClassDef);
      }
      else
      {
        //check for errors
        hRes = CheckJavaException(cEnv);
        if (SUCCEEDED(hRes))
          hRes = E_FAIL;
      }
    }
    //peek parameter types
    if (SUCCEEDED(hRes))
    {
      hRes = VarConvert_UnmarshalJValues_PeekTypes(cEnv, lpMsgData+(SIZE_T)(lpMsg->nClassLen+lpMsg->nMethodLen),
                                                   (SIZE_T)(lpMsg->nParametersLen), &cJObjectMgr, &cValues);
    }
    //find best matching method
    if (SUCCEEDED(hRes))
    {
      lpMethodDef = FindCompatibleMethodOrField(cEnv, cClassDef, 2, (LPCWSTR)cStrMethodW, cValues);
      if (lpMethodDef == NULL)
        hRes = E_INVALIDARG;
    }
    //retrieve parameter matching the provided method
    if (SUCCEEDED(hRes))
    {
      cValues.Reset();
      hRes = VarConvert_UnmarshalJValues(cEnv, lpMsgData+(SIZE_T)(lpMsg->nClassLen+lpMsg->nMethodLen),
                                         (SIZE_T)(lpMsg->nParametersLen), &cJavaVarInterchange, &cJObjectMgr,
                                         lpMethodDef, FALSE, &cValues);
    }
    //do the call
    if (SUCCEEDED(hRes))
    {
      if (lpMethodDef->nType == CMethodOrFieldDef::typStaticMethod)
      {
        if (cCls != NULL)
        {
          if (lpMethodDef->cJValueDefReturn.nArrayDims > 0)
          {
            cRetObj.Attach(cEnv, varRet.l = cEnv->CallStaticObjectMethodA(cCls, lpMethodDef->nMethodId,
                                                                          cValues->GetJValues()));
          }
          else
          {
            switch (lpMethodDef->cJValueDefReturn.nType)
            {
              case CJValueDef::typVoid:
                cEnv->CallStaticVoidMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typBoolean:
                varRet.z = cEnv->CallStaticBooleanMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typByte:
                varRet.b = cEnv->CallStaticByteMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typChar:
                varRet.c = cEnv->CallStaticCharMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typShort:
                varRet.s = cEnv->CallStaticShortMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typInt:
                varRet.i = cEnv->CallStaticIntMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typLong:
                varRet.j = cEnv->CallStaticLongMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typFloat:
                varRet.f = cEnv->CallStaticFloatMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typDouble:
                varRet.d = cEnv->CallStaticDoubleMethodA(cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typString:
              case CJValueDef::typBigDecimal:
              case CJValueDef::typDate:
              case CJValueDef::typCalendar:
              case CJValueDef::typObject:
              case CJValueDef::typBooleanObj:
              case CJValueDef::typByteObj:
              case CJValueDef::typCharObj:
              case CJValueDef::typShortObj:
              case CJValueDef::typIntObj:
              case CJValueDef::typLongObj:
              case CJValueDef::typFloatObj:
              case CJValueDef::typDoubleObj:
              case CJValueDef::typAtomicIntObj:
              case CJValueDef::typAtomicLongObj:
                cRetObj.Attach(cEnv, varRet.l = cEnv->CallStaticObjectMethodA(cCls, lpMethodDef->nMethodId,
                                                                              cValues->GetJValues()));
                break;
              default:
                _ASSERT(FALSE);
                hRes = E_NOTIMPL;
                break;
            }
          }
          //check for errors
          if (SUCCEEDED(hRes))
            hRes = CheckJavaException(cEnv);
        }
        else
        {
          hRes = E_INVALIDARG;
        }
      }
      else if (lpMsg->nClassLen > 0)
      {
        if (cObj != NULL)
        {
          if (lpMethodDef->cJValueDefReturn.nArrayDims > 0)
          {
            cRetObj.Attach(cEnv, varRet.l = cEnv->CallNonvirtualObjectMethodA(cObj, cCls, lpMethodDef->nMethodId,
                                                                              cValues->GetJValues()));
          }
          else
          {
            switch (lpMethodDef->cJValueDefReturn.nType)
            {
              case CJValueDef::typVoid:
                cEnv->CallNonvirtualVoidMethodA(cObj, cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                varRet.l = NULL;
                break;
              case CJValueDef::typBoolean:
                varRet.z = cEnv->CallNonvirtualBooleanMethodA(cObj, cCls, lpMethodDef->nMethodId,
                                                              cValues->GetJValues());
                break;
              case CJValueDef::typByte:
                varRet.b = cEnv->CallNonvirtualByteMethodA(cObj, cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typChar:
                varRet.c = cEnv->CallNonvirtualCharMethodA(cObj, cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typShort:
                varRet.s = cEnv->CallNonvirtualShortMethodA(cObj, cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typInt:
                varRet.i = cEnv->CallNonvirtualIntMethodA(cObj, cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typLong:
                varRet.j = cEnv->CallNonvirtualLongMethodA(cObj, cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typFloat:
                varRet.f = cEnv->CallNonvirtualFloatMethodA(cObj, cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typDouble:
                varRet.d = cEnv->CallNonvirtualDoubleMethodA(cObj, cCls, lpMethodDef->nMethodId, cValues->GetJValues());
                break;
              case CJValueDef::typString:
              case CJValueDef::typBigDecimal:
              case CJValueDef::typDate:
              case CJValueDef::typCalendar:
              case CJValueDef::typObject:
              case CJValueDef::typBooleanObj:
              case CJValueDef::typByteObj:
              case CJValueDef::typCharObj:
              case CJValueDef::typShortObj:
              case CJValueDef::typIntObj:
              case CJValueDef::typLongObj:
              case CJValueDef::typFloatObj:
              case CJValueDef::typDoubleObj:
              case CJValueDef::typAtomicIntObj:
              case CJValueDef::typAtomicLongObj:
                cRetObj.Attach(cEnv, varRet.l = cEnv->CallNonvirtualObjectMethodA(cObj, cCls, lpMethodDef->nMethodId,
                                                                                  cValues->GetJValues()));
                break;
              default:
                _ASSERT(FALSE);
                hRes = E_NOTIMPL;
                break;
            }
          }
          //check for errors
          if (SUCCEEDED(hRes))
            hRes = CheckJavaException(cEnv);
        }
        else
        {
          hRes = E_INVALIDARG;
        }
      }
      else if (cObj != NULL)
      {
        if (lpMethodDef->cJValueDefReturn.nArrayDims > 0)
        {
          cRetObj.Attach(cEnv, varRet.l = cEnv->CallObjectMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues()));
        }
        else
        {
          switch (lpMethodDef->cJValueDefReturn.nType)
          {
            case CJValueDef::typVoid:
              cEnv->CallVoidMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              varRet.l = NULL;
              break;
            case CJValueDef::typBoolean:
              varRet.z = cEnv->CallBooleanMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              break;
            case CJValueDef::typByte:
              varRet.b = cEnv->CallByteMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              break;
            case CJValueDef::typChar:
              varRet.c = cEnv->CallCharMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              break;
            case CJValueDef::typShort:
              varRet.s = cEnv->CallShortMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              break;
            case CJValueDef::typInt:
              varRet.i = cEnv->CallIntMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              break;
            case CJValueDef::typLong:
              varRet.j = cEnv->CallLongMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              break;
            case CJValueDef::typFloat:
              varRet.f = cEnv->CallFloatMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              break;
            case CJValueDef::typDouble:
              varRet.d = cEnv->CallDoubleMethodA(cObj, lpMethodDef->nMethodId, cValues->GetJValues());
              break;
            case CJValueDef::typString:
            case CJValueDef::typBigDecimal:
            case CJValueDef::typDate:
            case CJValueDef::typCalendar:
            case CJValueDef::typObject:
            case CJValueDef::typBooleanObj:
            case CJValueDef::typByteObj:
            case CJValueDef::typCharObj:
            case CJValueDef::typShortObj:
            case CJValueDef::typIntObj:
            case CJValueDef::typLongObj:
            case CJValueDef::typFloatObj:
            case CJValueDef::typDoubleObj:
            case CJValueDef::typAtomicIntObj:
            case CJValueDef::typAtomicLongObj:
              cRetObj.Attach(cEnv, varRet.l = cEnv->CallObjectMethodA(cObj, lpMethodDef->nMethodId,
                                                                      cValues->GetJValues()));
              break;
            default:
              _ASSERT(FALSE);
              hRes = E_NOTIMPL;
              break;
          }
        }
        //check for errors
        if (SUCCEEDED(hRes))
          hRes = CheckJavaException(cEnv);
      }
      else
      {
        hRes = E_INVALIDARG;
      }
    }
    //build result based on result
    if (SUCCEEDED(hRes))
    {
      TMSG_INVOKEJAVAMETHOD_REPLY *lpReply;
      SIZE_T nSize;

      hRes = VarConvert_MarshalJValue(cEnv, varRet, &(lpMethodDef->cJValueDefReturn), &cJavaVarInterchange,
                                      &cJObjectMgr, NULL, nSize);
      if (SUCCEEDED(hRes))
      {
        lpReply = (TMSG_INVOKEJAVAMETHOD_REPLY*)lpCallback->OnJavaAllocIpcBuffer(sizeof(TMSG_INVOKEJAVAMETHOD_REPLY)+
                                                                                nSize);
        if (lpReply == NULL)
          return E_OUTOFMEMORY;
        hRes = VarConvert_MarshalJValue(cEnv, varRet, &(lpMethodDef->cJValueDefReturn), &cJavaVarInterchange,
                                        &cJObjectMgr, (LPBYTE)(lpReply+1), nSize);
        if (SUCCEEDED(hRes))
        {
          lpReply->hRes = S_OK;
          lpReply->nResultLen = (ULONGLONG)nSize;
          *lplpReplyData = lpReply;
          *lpnReplyDataSize = sizeof(TMSG_INVOKEJAVAMETHOD_REPLY) + nSize;
          return S_OK;
        }
        //if we reach here, it means we hit an error
        lpCallback->OnJavaFreeIpcBuffer(lpReply);
      }
    }
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
  }
  if (hRes == E_OUTOFMEMORY)
    return hRes;
  *lplpReplyData = lpCallback->OnJavaAllocIpcBuffer(sizeof(TMSG_INVOKEJAVAMETHOD_REPLY));
  if ((*lplpReplyData) == NULL)
    return E_OUTOFMEMORY;
  ((TMSG_INVOKEJAVAMETHOD_REPLY*)(*lplpReplyData))->hRes = hRes;
  ((TMSG_INVOKEJAVAMETHOD_REPLY*)(*lplpReplyData))->nResultLen = 0;
  *lpnReplyDataSize = sizeof(TMSG_INVOKEJAVAMETHOD_REPLY);
  return S_OK;
}

HRESULT CMainManager::SetJavaField(__in TMSG_PUTJAVAFIELD *lpMsg, __in SIZE_T nDataSize,
                                   __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  LPBYTE lpMsgData;
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_PUTJAVAFIELD))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (nDataSize != sizeof(TMSG_PUTJAVAFIELD)+(SIZE_T)(lpMsg->nClassLen+lpMsg->nFieldLen+lpMsg->nValueLen))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (lpMsg->javaobj == 0)
  {
    if (lpMsg->nClassLen == 0 || (lpMsg->nClassLen & 1) != 0)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
  }
  else
  {
    if (lpMsg->nClassLen > 0 && (lpMsg->nClassLen & 1) != 0)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
  }
  if (lpMsg->nFieldLen == 0 || (lpMsg->nFieldLen & 1) != 0 ||
      lpMsg->nValueLen < sizeof(ULONG) || lpMsg->nValueLen > 0x7FFFFFFFui64)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);
    TNktAutoFreePtr<CHAR> sClassNameA;
    CNktStringW cStrFieldW;
    TJavaAutoPtr<jclass> cCls;
    TJavaAutoPtr<jobject> cObj;
    TNktAutoDeletePtr<CUnmarshalledJValues> cValues;
    TNktComPtr<CClassDef> cClassDef;
    CMethodOrFieldDef *lpFieldDef;

    if (cEnv == NULL)
      return E_OUTOFMEMORY;
    lpMsgData = (LPBYTE)(lpMsg+1);
    if (lpMsg->nClassLen > 0)
    {
      sClassNameA.Attach(CNktStringW::Wide2Ansi((LPCWSTR)lpMsgData, (SIZE_T)(lpMsg->nClassLen)/sizeof(WCHAR)));
      if (sClassNameA == NULL)
        return E_OUTOFMEMORY;
      NormalizeNameA(sClassNameA.Get());
    }
    if (cStrFieldW.CopyN((LPCWSTR)(lpMsgData+(SIZE_T)(lpMsg->nClassLen)),
                         (SIZE_T)(lpMsg->nFieldLen)/sizeof(WCHAR)) == FALSE)
      return E_OUTOFMEMORY;
    //get the object
    hRes = (lpMsg->javaobj > 0) ? cJObjectMgr.GetLocalRef(cEnv, (jobject)(lpMsg->javaobj), cObj) : S_OK;
    //find the class or superclass
    if (SUCCEEDED(hRes))
    {
      if (lpMsg->nClassLen > 0)
        cCls.Attach(cEnv, cEnv->FindClass(sClassNameA));
      else
        cCls.Attach(cEnv, cEnv->GetObjectClass(cObj));
      if (cCls != NULL)
      {
        hRes = cJClassCacheMgr.FindOrCreateClass(cEnv, cCls, &cClassDef);
      }
      else
      {
        //check for errors
        hRes = CheckJavaException(cEnv);
        if (SUCCEEDED(hRes))
          hRes = E_FAIL;
      }
    }
    //peek parameter types
    if (SUCCEEDED(hRes))
    {
      hRes = VarConvert_UnmarshalJValues_PeekTypes(cEnv, lpMsgData+(SIZE_T)(lpMsg->nClassLen+lpMsg->nFieldLen),
                                                   (SIZE_T)(lpMsg->nValueLen), &cJObjectMgr, &cValues);
      if (SUCCEEDED(hRes) && cValues->GetJValuesCount() != 1)
        hRes = E_INVALIDARG;
    }
    //find best matching field
    if (SUCCEEDED(hRes))
    {
      lpFieldDef = FindCompatibleMethodOrField(cEnv, cClassDef, 3, (LPCWSTR)cStrFieldW, cValues);
      if (lpFieldDef == NULL)
        hRes = E_INVALIDARG;
    }
    //retrieve parameter matching the provided method
    if (SUCCEEDED(hRes))
    {
      cValues.Reset();
      hRes = VarConvert_UnmarshalJValues(cEnv, lpMsgData+(SIZE_T)(lpMsg->nClassLen+lpMsg->nFieldLen),
                                         (SIZE_T)(lpMsg->nValueLen), &cJavaVarInterchange, &cJObjectMgr,
                                         lpFieldDef, FALSE, &cValues);
    }
    //do the set-field
    if (SUCCEEDED(hRes))
    {
      jvalue *lpVar;

      lpVar = cValues->GetJValues();
      _ASSERT(lpVar != NULL);
      switch (lpFieldDef->nType)
      {
        case CMethodOrFieldDef::typField:
          if (cObj != NULL)
          {
            //if setting a non-static field we need an object
            if (lpFieldDef->cJValueDefReturn.nArrayDims > 0)
            {
              cEnv->SetObjectField(cObj, lpFieldDef->nFieldId, lpVar->l);
            }
            else
            {
              switch (lpFieldDef->cJValueDefReturn.nType)
              {
                case CJValueDef::typVoid:
                  hRes = E_INVALIDARG;
                  break;
                case CJValueDef::typBoolean:
                  cEnv->SetBooleanField(cObj, lpFieldDef->nFieldId, lpVar->z);
                  break;
                case CJValueDef::typByte:
                  cEnv->SetByteField(cObj, lpFieldDef->nFieldId, lpVar->b);
                  break;
                case CJValueDef::typChar:
                  cEnv->SetCharField(cObj, lpFieldDef->nFieldId, lpVar->c);
                  break;
                case CJValueDef::typShort:
                  cEnv->SetShortField(cObj, lpFieldDef->nFieldId, lpVar->s);
                  break;
                case CJValueDef::typInt:
                  cEnv->SetIntField(cObj, lpFieldDef->nFieldId, lpVar->i);
                  break;
                case CJValueDef::typLong:
                  cEnv->SetLongField(cObj, lpFieldDef->nFieldId, lpVar->j);
                  break;
                case CJValueDef::typFloat:
                  cEnv->SetFloatField(cObj, lpFieldDef->nFieldId, lpVar->f);
                  break;
                case CJValueDef::typDouble:
                  cEnv->SetDoubleField(cObj, lpFieldDef->nFieldId, lpVar->d);
                  break;
                case CJValueDef::typString:
                case CJValueDef::typBigDecimal:
                case CJValueDef::typDate:
                case CJValueDef::typCalendar:
                case CJValueDef::typObject:
                case CJValueDef::typBooleanObj:
                case CJValueDef::typByteObj:
                case CJValueDef::typCharObj:
                case CJValueDef::typShortObj:
                case CJValueDef::typIntObj:
                case CJValueDef::typLongObj:
                case CJValueDef::typFloatObj:
                case CJValueDef::typDoubleObj:
                case CJValueDef::typAtomicIntObj:
                case CJValueDef::typAtomicLongObj:
                  cEnv->SetObjectField(cObj, lpFieldDef->nFieldId, lpVar->l);
                  break;
                default:
                  _ASSERT(FALSE);
                  hRes = E_NOTIMPL;
                  break;
              }
            }
            //check for errors
            if (SUCCEEDED(hRes))
              hRes = CheckJavaException(cEnv);
          }
          else
          {
            hRes = E_INVALIDARG;
          }
          break;

        case CMethodOrFieldDef::typStaticField:
          if (cCls != NULL)
          {
            //for setting a static field we need a class
            if (lpFieldDef->cJValueDefReturn.nArrayDims > 0)
            {
              cEnv->SetStaticObjectField(cCls, lpFieldDef->nFieldId, lpVar->l);
            }
            else
            {
              switch (lpFieldDef->cJValueDefReturn.nType)
              {
                case CJValueDef::typVoid:
                  hRes = E_INVALIDARG;
                  break;
                case CJValueDef::typBoolean:
                  cEnv->SetStaticBooleanField(cCls, lpFieldDef->nFieldId, lpVar->z);
                  break;
                case CJValueDef::typByte:
                  cEnv->SetStaticByteField(cCls, lpFieldDef->nFieldId, lpVar->b);
                  break;
                case CJValueDef::typChar:
                  cEnv->SetStaticCharField(cCls, lpFieldDef->nFieldId, lpVar->c);
                  break;
                case CJValueDef::typShort:
                  cEnv->SetStaticShortField(cCls, lpFieldDef->nFieldId, lpVar->s);
                  break;
                case CJValueDef::typInt:
                  cEnv->SetStaticIntField(cCls, lpFieldDef->nFieldId, lpVar->i);
                  break;
                case CJValueDef::typLong:
                  cEnv->SetStaticLongField(cCls, lpFieldDef->nFieldId, lpVar->j);
                  break;
                case CJValueDef::typFloat:
                  cEnv->SetStaticFloatField(cCls, lpFieldDef->nFieldId, lpVar->f);
                  break;
                case CJValueDef::typDouble:
                  cEnv->SetStaticDoubleField(cCls, lpFieldDef->nFieldId, lpVar->d);
                  break;
                case CJValueDef::typString:
                case CJValueDef::typBigDecimal:
                case CJValueDef::typDate:
                case CJValueDef::typCalendar:
                case CJValueDef::typObject:
                case CJValueDef::typBooleanObj:
                case CJValueDef::typByteObj:
                case CJValueDef::typCharObj:
                case CJValueDef::typShortObj:
                case CJValueDef::typIntObj:
                case CJValueDef::typLongObj:
                case CJValueDef::typFloatObj:
                case CJValueDef::typDoubleObj:
                case CJValueDef::typAtomicIntObj:
                case CJValueDef::typAtomicLongObj:
                  cEnv->SetStaticObjectField(cCls, lpFieldDef->nFieldId, lpVar->l);
                  break;
                default:
                  _ASSERT(FALSE);
                  hRes = E_NOTIMPL;
                  break;
              }
            }
            //check for errors
            if (SUCCEEDED(hRes))
              hRes = CheckJavaException(cEnv);
          }
          else
          {
            hRes = E_INVALIDARG;
          }
          break;

        default:
          _ASSERT(FALSE);
          hRes = E_FAIL;
          break;
      }
    }
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
  }
  if (hRes == E_OUTOFMEMORY)
    return hRes;
  return lpCallback->OnJavaBuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
}

HRESULT CMainManager::GetJavaField(__in TMSG_GETJAVAFIELD *lpMsg, __in SIZE_T nDataSize,
                                   __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  LPBYTE lpMsgData;
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_GETJAVAFIELD))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (nDataSize != sizeof(TMSG_GETJAVAFIELD)+(SIZE_T)(lpMsg->nClassLen+lpMsg->nFieldLen))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (lpMsg->javaobj == 0)
  {
    if (lpMsg->nClassLen == 0 || (lpMsg->nClassLen & 1) != 0)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
  }
  else
  {
    if (lpMsg->nClassLen > 0 && (lpMsg->nClassLen & 1) != 0)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
  }
  if (lpMsg->nFieldLen == 0 || (lpMsg->nFieldLen & 1) != 0)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);
    TNktAutoFreePtr<CHAR> sClassNameA;
    CNktStringW cStrFieldW;
    TJavaAutoPtr<jclass> cCls;
    TJavaAutoPtr<jobject> cObj, cRetObj;
    TNktComPtr<CClassDef> cClassDef;
    CMethodOrFieldDef *lpFieldDef;
    jvalue varRet;

    if (cEnv == NULL)
      return E_OUTOFMEMORY;
    lpMsgData = (LPBYTE)(lpMsg+1);
    if (lpMsg->nClassLen > 0)
    {
      sClassNameA.Attach(CNktStringW::Wide2Ansi((LPCWSTR)lpMsgData, (SIZE_T)(lpMsg->nClassLen)/sizeof(WCHAR)));
      if (sClassNameA == NULL)
        return E_OUTOFMEMORY;
      NormalizeNameA(sClassNameA.Get());
    }
    if (cStrFieldW.CopyN((LPCWSTR)(lpMsgData+(SIZE_T)(lpMsg->nClassLen)),
                         (SIZE_T)(lpMsg->nFieldLen)/sizeof(WCHAR)) == FALSE)
      return E_OUTOFMEMORY;
    //get the object
    hRes = (lpMsg->javaobj > 0) ? cJObjectMgr.GetLocalRef(cEnv, (jobject)(lpMsg->javaobj), cObj) : S_OK;
    //find the class or superclass
    if (SUCCEEDED(hRes))
    {
      if (lpMsg->nClassLen > 0)
        cCls.Attach(cEnv, cEnv->FindClass(sClassNameA));
      else
        cCls.Attach(cEnv, cEnv->GetObjectClass(cObj));
      if (cCls != NULL)
      {
        hRes = cJClassCacheMgr.FindOrCreateClass(cEnv, cCls, &cClassDef);
      }
      else
      {
        //check for errors
        hRes = CheckJavaException(cEnv);
        if (SUCCEEDED(hRes))
          hRes = E_FAIL;
      }
    }
    //find best matching field
    if (SUCCEEDED(hRes))
    {
      lpFieldDef = cClassDef->FindField((LPCWSTR)cStrFieldW);
      if (lpFieldDef == NULL)
        hRes = E_INVALIDARG;
    }
    //do the get-field
    if (SUCCEEDED(hRes))
    {
      switch (lpFieldDef->nType)
      {
        case CMethodOrFieldDef::typField:
          if (cObj != NULL)
          {
            //if getting a non-static field we need an object
            if (lpFieldDef->cJValueDefReturn.nArrayDims > 0)
            {
              cRetObj.Attach(cEnv, varRet.l = cEnv->GetObjectField(cObj, lpFieldDef->nFieldId));
            }
            else
            {
              switch (lpFieldDef->cJValueDefReturn.nType)
              {
                case CJValueDef::typVoid:
                  hRes = E_INVALIDARG;
                  break;
                case CJValueDef::typBoolean:
                  varRet.z = cEnv->GetBooleanField(cObj, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typByte:
                  varRet.b = cEnv->GetByteField(cObj, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typChar:
                  varRet.c = cEnv->GetCharField(cObj, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typShort:
                  varRet.s = cEnv->GetShortField(cObj, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typInt:
                  varRet.i = cEnv->GetIntField(cObj, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typLong:
                  varRet.j = cEnv->GetLongField(cObj, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typFloat:
                  varRet.f = cEnv->GetFloatField(cObj, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typDouble:
                  varRet.d = cEnv->GetDoubleField(cObj, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typString:
                case CJValueDef::typBigDecimal:
                case CJValueDef::typDate:
                case CJValueDef::typCalendar:
                case CJValueDef::typObject:
                case CJValueDef::typBooleanObj:
                case CJValueDef::typByteObj:
                case CJValueDef::typCharObj:
                case CJValueDef::typShortObj:
                case CJValueDef::typIntObj:
                case CJValueDef::typLongObj:
                case CJValueDef::typFloatObj:
                case CJValueDef::typDoubleObj:
                case CJValueDef::typAtomicIntObj:
                case CJValueDef::typAtomicLongObj:
                  cRetObj.Attach(cEnv, varRet.l = cEnv->GetObjectField(cObj, lpFieldDef->nFieldId));
                  break;
                default:
                  _ASSERT(FALSE);
                  hRes = E_NOTIMPL;
                  break;
              }
            }
            //check for errors
            if (SUCCEEDED(hRes))
              hRes = CheckJavaException(cEnv);
          }
          else
          {
            hRes = E_INVALIDARG;
          }
          break;

        case CMethodOrFieldDef::typStaticField:
          if (cCls != NULL)
          {
            //for getting a static field we need a class
            if (lpFieldDef->cJValueDefReturn.nArrayDims > 0)
            {
              cRetObj.Attach(cEnv, varRet.l = cEnv->GetStaticObjectField(cCls, lpFieldDef->nFieldId));
            }
            else
            {
              switch (lpFieldDef->cJValueDefReturn.nType)
              {
                case CJValueDef::typVoid:
                  hRes = E_INVALIDARG;
                  break;
                case CJValueDef::typBoolean:
                  varRet.z = cEnv->GetStaticBooleanField(cCls, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typByte:
                  varRet.b = cEnv->GetStaticByteField(cCls, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typChar:
                  varRet.c = cEnv->GetStaticCharField(cCls, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typShort:
                  varRet.s = cEnv->GetStaticShortField(cCls, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typInt:
                  varRet.i = cEnv->GetStaticIntField(cCls, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typLong:
                  varRet.j = cEnv->GetStaticLongField(cCls, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typFloat:
                  varRet.f = cEnv->GetStaticFloatField(cCls, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typDouble:
                  varRet.d = cEnv->GetStaticDoubleField(cCls, lpFieldDef->nFieldId);
                  break;
                case CJValueDef::typString:
                case CJValueDef::typBigDecimal:
                case CJValueDef::typDate:
                case CJValueDef::typCalendar:
                case CJValueDef::typObject:
                case CJValueDef::typBooleanObj:
                case CJValueDef::typByteObj:
                case CJValueDef::typCharObj:
                case CJValueDef::typShortObj:
                case CJValueDef::typIntObj:
                case CJValueDef::typLongObj:
                case CJValueDef::typFloatObj:
                case CJValueDef::typDoubleObj:
                case CJValueDef::typAtomicIntObj:
                case CJValueDef::typAtomicLongObj:
                  cRetObj.Attach(cEnv, varRet.l = cEnv->GetStaticObjectField(cCls, lpFieldDef->nFieldId));
                  break;
                default:
                  _ASSERT(FALSE);
                  hRes = E_NOTIMPL;
                  break;
              }
            }
            //check for errors
            if (SUCCEEDED(hRes))
              hRes = CheckJavaException(cEnv);
          }
          else
          {
            hRes = E_INVALIDARG;
          }
          break;

        default:
          _ASSERT(FALSE);
          hRes = E_FAIL;
          break;
      }
    }
    //build result based on result
    if (SUCCEEDED(hRes))
    {
      TMSG_GETJAVAFIELD_REPLY *lpReply;
      SIZE_T nSize;

      hRes = VarConvert_MarshalJValue(cEnv, varRet, &(lpFieldDef->cJValueDefReturn), &cJavaVarInterchange,
                                      &cJObjectMgr, NULL, nSize);
      if (SUCCEEDED(hRes))
      {
        lpReply = (TMSG_GETJAVAFIELD_REPLY*)lpCallback->OnJavaAllocIpcBuffer(sizeof(TMSG_GETJAVAFIELD_REPLY)+
                                                                               nSize);
        if (lpReply == NULL)
          return E_OUTOFMEMORY;
        hRes = VarConvert_MarshalJValue(cEnv, varRet, &(lpFieldDef->cJValueDefReturn), &cJavaVarInterchange,
                                        &cJObjectMgr, (LPBYTE)(lpReply+1), nSize);
        if (SUCCEEDED(hRes))
        {
          lpReply->hRes = S_OK;
          lpReply->nResultLen = (ULONGLONG)nSize;
          *lplpReplyData = lpReply;
          *lpnReplyDataSize = sizeof(TMSG_GETJAVAFIELD_REPLY) + nSize;
          return S_OK;
        }
        //if we reach here, it means we hit an error
        lpCallback->OnJavaFreeIpcBuffer(lpReply);
      }
    }
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
  }
  if (hRes == E_OUTOFMEMORY)
    return hRes;
  *lplpReplyData = lpCallback->OnJavaAllocIpcBuffer(sizeof(TMSG_GETJAVAFIELD_REPLY));
  if ((*lplpReplyData) == NULL)
    return E_OUTOFMEMORY;
  ((TMSG_GETJAVAFIELD_REPLY*)(*lplpReplyData))->hRes = hRes;
  ((TMSG_GETJAVAFIELD_REPLY*)(*lplpReplyData))->nResultLen = 0;
  *lpnReplyDataSize = sizeof(TMSG_GETJAVAFIELD_REPLY);
  return S_OK;
}

HRESULT CMainManager::IsSameJavaObject(__in TMSG_ISSAMEJAVAOBJECT *lpMsg, __in SIZE_T nDataSize,
                                       __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  HRESULT hRes;

  if (nDataSize != sizeof(TMSG_ISSAMEJAVAOBJECT))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (lpMsg->javaobj1 == 0 || lpMsg->javaobj2 == 0)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);
    TJavaAutoPtr<jobject> cObj1, cObj2;

    if (cEnv == NULL)
      return E_OUTOFMEMORY;
    hRes =  cJObjectMgr.GetLocalRef(cEnv, (jobject)(lpMsg->javaobj1), cObj1);
    if (SUCCEEDED(hRes))
      hRes =  cJObjectMgr.GetLocalRef(cEnv, (jobject)(lpMsg->javaobj2), cObj2);
    if (SUCCEEDED(hRes))
      hRes = (cEnv->IsSameObject(cObj1, cObj2) != JNI_FALSE) ? S_OK : S_FALSE;
    else if (hRes = NKT_E_NotFound)
      hRes = S_FALSE;
    if (hRes == E_OUTOFMEMORY)
      return hRes;
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
  }
  return lpCallback->OnJavaBuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
}

HRESULT CMainManager::RemoveJObjectRef(__in TMSG_REMOVEJOBJECTREF *lpMsg, __in SIZE_T nDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);

  if (nDataSize != sizeof(TMSG_REMOVEJOBJECTREF))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);

    if (cEnv != NULL)
      cJObjectMgr.Remove(cEnv, (jobject)(lpMsg->javaobj));
  }
  return S_OK;
}

HRESULT CMainManager::IsJVMAttached(__in TMSG_HEADER *lpMsgHdr, __in SIZE_T nDataSize,
                                    __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  TMSG_ISJVMATTACHED_REPLY *lpReply;

  lpReply = (TMSG_ISJVMATTACHED_REPLY*)lpCallback->OnJavaAllocIpcBuffer(sizeof(TMSG_ISJVMATTACHED_REPLY));
  if (lpReply == NULL)
    return E_OUTOFMEMORY;
  memset(lpReply, 0, sizeof(TMSG_ISJVMATTACHED_REPLY));
  if (sJVM.lpVM != NULL)
  {
    lpReply->hRes = S_OK;
    lpReply->wVersionMajor = sJVM.wVersionMajor;
    lpReply->wVersionMinor = sJVM.wVersionMinor;
  }
  else
  {
    lpReply->hRes = S_FALSE;
  }
  *lplpReplyData = lpReply;
  *lpnReplyDataSize = sizeof(TMSG_ISJVMATTACHED_REPLY);
  return S_OK;
}

HRESULT CMainManager::GetJavaObjectClass(__in TMSG_GETJAVAOBJECTCLASS *lpMsg, __in SIZE_T nDataSize,
                                         __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  HRESULT hRes;

  if (nDataSize != sizeof(TMSG_GETJAVAOBJECTCLASS))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (lpMsg->javaobj == 0)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);
    TJavaAutoPtr<jclass> cCls;
    TJavaAutoPtr<jobject> cObj;
    TNktComPtr<CClassDef> cClassDef;

    if (cEnv == NULL)
      return E_OUTOFMEMORY;
    //get the object
    hRes = cJObjectMgr.GetLocalRef(cEnv, (jobject)(lpMsg->javaobj), cObj);
    //find the class or superclass
    if (SUCCEEDED(hRes))
    {
      cCls.Attach(cEnv, cEnv->GetObjectClass(cObj));
      if (cCls != NULL)
      {
        hRes = cJClassCacheMgr.FindOrCreateClass(cEnv, cCls, &cClassDef);
      }
      else
      {
        //check for errors
        hRes = CheckJavaException(cEnv);
        if (SUCCEEDED(hRes))
          hRes = E_FAIL;
      }
    }
    //build result
    if (SUCCEEDED(hRes))
    {
      TMSG_GETJAVAOBJECTCLASS_REPLY *lpReply;
      SIZE_T nSize;

      nSize = cClassDef->cStrNameW.GetLength() * sizeof(WCHAR);
      lpReply = (TMSG_GETJAVAOBJECTCLASS_REPLY*)lpCallback->OnJavaAllocIpcBuffer(sizeof(TMSG_GETJAVAOBJECTCLASS_REPLY)+
                                                                                nSize);
      if (lpReply == NULL)
        return E_OUTOFMEMORY;
      lpReply->hRes = S_OK;
      lpReply->nClassNameLen = (ULONGLONG)nSize;
      memcpy((LPBYTE)(lpReply+1), (LPWSTR)(cClassDef->cStrNameW), nSize); //already normalized to slashes
      *lplpReplyData = lpReply;
      *lpnReplyDataSize = sizeof(TMSG_GETJAVAFIELD_REPLY) + nSize;
      return S_OK;
    }
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
  }
  if (hRes == E_OUTOFMEMORY)
    return hRes;
  *lplpReplyData = lpCallback->OnJavaAllocIpcBuffer(sizeof(TMSG_GETJAVAOBJECTCLASS_REPLY));
  if ((*lplpReplyData) == NULL)
    return E_OUTOFMEMORY;
  ((TMSG_GETJAVAOBJECTCLASS_REPLY*)(*lplpReplyData))->hRes = hRes;
  ((TMSG_GETJAVAOBJECTCLASS_REPLY*)(*lplpReplyData))->nClassNameLen = 0;
  *lpnReplyDataSize = sizeof(TMSG_GETJAVAOBJECTCLASS_REPLY);
  return S_OK;
}

HRESULT CMainManager::IsJavaObjectInstanceOf(__in TMSG_ISJAVAOBJECTINSTANCEOF *lpMsg, __in SIZE_T nDataSize,
                                             __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), TRUE);
  LPBYTE lpMsgData;
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_ISJAVAOBJECTINSTANCEOF))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (nDataSize != sizeof(TMSG_ISJAVAOBJECTINSTANCEOF)+(SIZE_T)(lpMsg->nClassLen) ||
      lpMsg->javaobj == 0 ||
      lpMsg->nClassLen < 2 || (lpMsg->nClassLen & 1) != 0)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  if (sJVM.lpVM != NULL)
  {
    CAutoThreadEnv cEnv(sJVM.lpVM);
    TNktAutoFreePtr<CHAR> sClassNameA;
    TJavaAutoPtr<jclass> cCls;
    TJavaAutoPtr<jobject> cObj;
    jboolean bIsInstance;

    if (cEnv == NULL)
      return E_OUTOFMEMORY;
    lpMsgData = (LPBYTE)(lpMsg+1);
    //get class to check
    sClassNameA.Attach(CNktStringW::Wide2Ansi((LPCWSTR)lpMsgData, (SIZE_T)(lpMsg->nClassLen)/sizeof(WCHAR)));
    if (sClassNameA == NULL)
      return E_OUTOFMEMORY;
    NormalizeNameA(sClassNameA.Get());
    //find the class or superclass
    cCls.Attach(cEnv, cEnv->FindClass(sClassNameA));
    if (cCls != NULL)
    {
      hRes = S_OK;
    }
    else
    {
      //check for errors
      hRes = CheckJavaException(cEnv);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
    //get the object
    if (SUCCEEDED(hRes))
      hRes = cJObjectMgr.GetLocalRef(cEnv, (jobject)(lpMsg->javaobj), cObj);
    //do check
    if (SUCCEEDED(hRes))
    {
      bIsInstance = cEnv->IsInstanceOf(cObj, cCls);
      //check for errors
      hRes = CheckJavaException(cEnv);
      if (SUCCEEDED(hRes))
        hRes = (bIsInstance != JNI_FALSE) ? S_OK : S_FALSE;
    }
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
  }
  if (hRes == E_OUTOFMEMORY)
    return hRes;
  return lpCallback->OnJavaBuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
}

HRESULT CMainManager::DetectJVM()
{
  JavaVM *vmBuf;
  jsize nVMs;
  jint res;
  HRESULT hRes;

  _ASSERT(sJVM.fnJNI_GetCreatedJavaVMs != NULL);
  try
  {
    res = sJVM.fnJNI_GetCreatedJavaVMs(&vmBuf, 1, &nVMs);
  }
  catch (...)
  {
    res = JNI_ERR;
  }
  if (res != JNI_OK || nVMs < 1 || vmBuf == NULL)
    return S_OK;
  {
    CAutoThreadEnv cEnv(vmBuf);

    hRes = AttachJVM(vmBuf, cEnv);
  }
  return hRes;
}

HRESULT CMainManager::AttachJVM(__in JavaVM *vm, __in JNIEnv *env)
{
  CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), FALSE);

  if (sJVM.lpVM == NULL)
  {
    jint i;

    if (env == NULL)
      return E_FAIL;
    sJVM.lpVM = vm;
    i = env->GetVersion();
    sJVM.wVersionMajor = (WORD)((i & 0x7FFF0000L) >> 16);
    sJVM.wVersionMinor = (WORD) (i & 0x0000FFFFL);
    /*
    if (FAILED(hRes))
    {
      DetachJVM_2(vm, FALSE, env);
      return hRes;
    }
    */
  }
  return S_OK;
}

VOID CMainManager::DetachJVM(__in JavaVM *vm, __in BOOL bInExitChain)
{
  if (nIsExiting == 0)
  {
    CNktAutoFastReadWriteLock cRwLock(&(sJVM.cRwMtx), FALSE);
    CAutoThreadEnv cEnv(sJVM.lpVM);

    DetachJVM_2(vm, bInExitChain, cEnv);
  }
  return;
}

VOID CMainManager::DetachJVM_2(__in JavaVM *vm, __in BOOL bInExitChain, __in JNIEnv *env)
{
  if ((vm != NULL && sJVM.lpVM == vm) ||
      (vm == NULL && sJVM.lpVM != NULL))
  {
    _ASSERT(env != NULL);
    cJavaVarInterchange.Reset(env);
    cJObjectMgr.RemoveAll(env);
    cJClassCacheMgr.RemoveAll(env, bInExitChain);
    sJVM.wVersionMajor = sJVM.wVersionMinor = 0;
    sJVM.lpVM = NULL;
  }
  return;
}

VOID CMainManager::OnCreateJavaVM(__in JavaVM *vm, __in JNIEnv *env)
{
  AttachJVM(vm, env);
  return;
}

VOID __stdcall CMainManager::CreateJavaVMStub(__in CMainManager *lpCtx, __in JavaVM **pvm, __in JNIEnv **penv)
{
  lpCtx->lpCallback->OnJavaHookUsage(TRUE);
  if (pvm != NULL && (*pvm) != NULL && penv != NULL && (*penv) != NULL)
    lpCtx->OnCreateJavaVM(*pvm, *penv);
  lpCtx->lpCallback->OnJavaHookUsage(FALSE);
  return;
}

HRESULT CMainManager::OnNativeRaiseCallback(__in JNIEnv* env, __in_z LPCWSTR szClassNameW, __in_z LPCWSTR szMethodNameW,
                                            __in jobject objOrClass, __in jvalue *lpParams,
                                            __in CMethodOrFieldDef *lpMethodOrFieldDef, __out jvalue *lpRetVal)
{
  TNktAutoDeletePtr<TMSG_RAISEONJAVANATIVEMETHODCALLED> cMsg;
  TNktLnkLst<CJValueDef>::Iterator it;
  CJValueDef *lpJValueDef;
  SIZE_T i, k, nParamsCount, nParamsSize, nLen[2], nMsgExtraSize, nReplySize;
  TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY *lpReply;
  LPBYTE lpDest;
  ULONGLONG *lpParamSizes;
  jobject globalObj;
  HRESULT hRes;

  _ASSERT(lpMethodOrFieldDef != NULL);
  _ASSERT(szClassNameW != NULL && szMethodNameW != NULL);
  //calculate parameters sizes
  for (nParamsSize=nParamsCount=0,lpJValueDef=it.Begin(lpMethodOrFieldDef->cJValueDefParams); lpJValueDef!=NULL;
       lpJValueDef=it.Next(),nParamsCount++)
  {
    hRes = VarConvert_MarshalJValue(env, lpParams[nParamsCount], lpJValueDef, &cJavaVarInterchange, &cJObjectMgr,
                                    NULL, k);
    if (FAILED(hRes))
      return hRes;
    nParamsSize += k;
  }
  //get name len
  nLen[0] = wcslen(szClassNameW) * sizeof(WCHAR);
  nLen[1] = wcslen(szMethodNameW) * sizeof(WCHAR);
  nMsgExtraSize = nLen[0] + nLen[1] + nParamsCount*sizeof(ULONGLONG) + nParamsSize;
  //build the message to send to the server
  cMsg.Attach((TMSG_RAISEONJAVANATIVEMETHODCALLED*)NKT_MALLOC(sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED)+
                                                              nMsgExtraSize));
  if (cMsg == NULL)
    return E_OUTOFMEMORY;
  cMsg->sHeader.dwMsgCode = TMSG_CODE_RaiseOnJavaNativeMethodCalled;
  cMsg->nClassNameLen = (ULONG)nLen[0];
  cMsg->nMethodNameLen = (ULONG)nLen[1];
  hRes = cJObjectMgr.Add(env, objOrClass, &globalObj);
  if (FAILED(hRes))
    return hRes;
  cMsg->javaobj_or_class = NKT_PTR_2_ULONGLONG(globalObj);
  cMsg->nParamsCount = (ULONGLONG)nParamsCount;
  lpDest = (LPBYTE)(((TMSG_RAISEONJAVANATIVEMETHODCALLED*)cMsg+1));
  memcpy(lpDest, szClassNameW, nLen[0]);
  lpDest += nLen[0];
  memcpy(lpDest, szMethodNameW, nLen[1]);
  lpDest += nLen[1];
  lpParamSizes = (ULONGLONG*)lpDest;
  lpDest += nParamsCount*sizeof(ULONGLONG);
  for (i=0,lpJValueDef=it.Begin(lpMethodOrFieldDef->cJValueDefParams); lpJValueDef!=NULL; lpJValueDef=it.Next(),i++)
  {
    hRes = VarConvert_MarshalJValue(env, lpParams[i], lpJValueDef, &cJavaVarInterchange, &cJObjectMgr, lpDest, k);
    if (FAILED(hRes))
      return hRes;
    lpDest += k;
    lpParamSizes[i] = (ULONGLONG)k;
  }
  //send message
  lpReply = NULL;
  nReplySize = 0;
  hRes = lpCallback->OnJavaRaiseJavaNativeMethodCalled((TMSG_RAISEONJAVANATIVEMETHODCALLED*)cMsg,
                                                       sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED)+nMsgExtraSize,
                                                       &lpReply, &nReplySize);
  //parse response
  if (SUCCEEDED(hRes))
  {
    if (nReplySize >= sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY) &&
        nReplySize == sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY)+(SIZE_T)(lpReply->nResultLen))
    {
      if (SUCCEEDED(lpReply->hRes))
      {
        TNktAutoDeletePtr<CUnmarshalledJValues> cValues;

        hRes = VarConvert_UnmarshalJValues(env, (LPBYTE)(lpReply+1), (SIZE_T)(lpReply->nResultLen),
                                           &cJavaVarInterchange, &cJObjectMgr, lpMethodOrFieldDef, TRUE, &cValues);
        if (SUCCEEDED(hRes))
        {
          _ASSERT(cValues->GetJValuesCount() == 1);
          jvalue *lpJVal = cValues->GetJValues();
          *lpRetVal = lpJVal[0];
          memset(lpJVal, 0, sizeof(jvalue)); //zero it to avoid releasing on CUnmarshalledJValues' destructor
        }
      }
      else
      {
        hRes = lpReply->hRes;
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  //done
  if (lpReply != NULL)
    lpCallback->OnJavaFreeIpcBuffer(lpReply);
  //if (hRes == E_OUTOFMEMORY || hRes == NKT_E_InvalidData)
  return hRes;
}

} //JAVA

//-----------------------------------------------------------

static JAVA::CMethodOrFieldDef* FindCompatibleMethodOrField(__in JNIEnv *env, __in JAVA::CClassDef *lpClassDef,
                                                            __in int nType, __in LPCWSTR szNameW,
                                                            __in JAVA::CUnmarshalledJValues *lpPeekedValues)
{
  TNktLnkLst<JAVA::CMethodOrFieldDef> *lpList;
  TNktLnkLst<JAVA::CMethodOrFieldDef>::Iterator it;
  TNktLnkLst<JAVA::CJValueDef>::Iterator itJVal;
  JAVA::CJValueDef *lpCurrJValue;
  JAVA::CMethodOrFieldDef *lpCurr, *lpBest;
  JAVA::CUnmarshalledJValues::JVALUETYPE *lpValTypes;
  jvalue *lpValValues;
  SIZE_T k, nCurrDist, nBestDist;

  switch (nType)
  {
    case 1: //constructors
      _ASSERT(szNameW == NULL);
      lpList = &(lpClassDef->cConstructors);
      break;
    case 2: //methods
      _ASSERT(szNameW != NULL);
      lpList = &(lpClassDef->cMethods);
      break;
    case 3: //fields
      _ASSERT(szNameW != NULL);
      lpList = &(lpClassDef->cFields);
      break;
    default:
      _ASSERT(FALSE);
      return NULL;
  }
  lpBest = NULL;
  nBestDist = (SIZE_T)-1;
  for (lpCurr=it.Begin(*lpList); lpCurr!=NULL; lpCurr=it.Next())
  {
    if (szNameW != NULL)
    {
      if (_wcsicmp((LPWSTR)(lpCurr->cStrNameW), szNameW) != 0)
        continue; //distinct name => ignore
    }
    //get parameters count
    for (k=0,lpCurrJValue=itJVal.Begin(lpCurr->cJValueDefParams); lpCurrJValue!=NULL; lpCurrJValue=itJVal.Next(),k++);
    if (k != lpPeekedValues->GetJValuesCount())
      continue; //mismatch => ignore
    //calculate current method distance
    lpValTypes = lpPeekedValues->GetJValueTypes();
    lpValValues = lpPeekedValues->GetJValues();
    nCurrDist = 0;
    for (k=0,lpCurrJValue=itJVal.Begin(lpCurr->cJValueDefParams); lpCurrJValue!=NULL && nCurrDist!=(SIZE_T)-1;
         lpCurrJValue=itJVal.Next(),k++)
    {
      //check if array or single value
      if (lpCurrJValue->nArrayDims != (ULONG)(lpValTypes[k].nDimCount))
      {
        nCurrDist = (SIZE_T)-1; //dimension mismatch => ignore
        break;
      }
      //check type
      switch (lpCurrJValue->nType)
      {
        case JAVA::CJValueDef::typVoid:
          nCurrDist = (SIZE_T)-1; //void params (?)
          break;

        case JAVA::CJValueDef::typBoolean:
        case JAVA::CJValueDef::typBooleanObj:
          if (lpValTypes[k].nType != JAVA::CJValueDef::typBoolean)
            nCurrDist = (SIZE_T)-1; //booleans only match VT_BOOL
          break;

        case JAVA::CJValueDef::typByte:
        case JAVA::CJValueDef::typByteObj:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typByte:
              break; //ok
            case JAVA::CJValueDef::typShort:
              nCurrDist++;
              break; //may downgrade to byte
            case JAVA::CJValueDef::typInt:
              nCurrDist += 2;
              break; //may downgrade to byte
            case JAVA::CJValueDef::typLong:
              nCurrDist += 3;
              break; //may downgrade to byte
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typChar:
        case JAVA::CJValueDef::typCharObj:
        case JAVA::CJValueDef::typShort:
        case JAVA::CJValueDef::typShortObj:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typByte:
              break; //can convert to short/char without problems
            case JAVA::CJValueDef::typChar:
            case JAVA::CJValueDef::typShort:
              break; //ok
            case JAVA::CJValueDef::typInt:
              nCurrDist++;
              break; //may downgrade to short
            case JAVA::CJValueDef::typLong:
              nCurrDist += 2;
              break; //may downgrade to short
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typInt:
        case JAVA::CJValueDef::typIntObj:
        case JAVA::CJValueDef::typAtomicIntObj:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typByte:
            case JAVA::CJValueDef::typShort:
              break; //can convert to int without problems
            case JAVA::CJValueDef::typInt:
              break; //ok
            case JAVA::CJValueDef::typLong:
              nCurrDist++;
              break; //may downgrade to int
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typLong:
        case JAVA::CJValueDef::typLongObj:
        case JAVA::CJValueDef::typAtomicLongObj:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typByte:
            case JAVA::CJValueDef::typShort:
            case JAVA::CJValueDef::typInt:
              break; //can convert to long without problems
            case JAVA::CJValueDef::typLong:
              break; //ok
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typFloat:
        case JAVA::CJValueDef::typFloatObj:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typFloat:
              break; //ok
            case JAVA::CJValueDef::typDouble:
              nCurrDist++;
              break; //may downgrade to float
            case JAVA::CJValueDef::typBigDecimal:
              nCurrDist += 2;
              break; //may downgrade to double
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typDouble:
        case JAVA::CJValueDef::typDoubleObj:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typFloat:
              break; //can convert to double without problems
            case JAVA::CJValueDef::typDouble:
              break; //ok
            case JAVA::CJValueDef::typBigDecimal:
              nCurrDist++;
              break; //may downgrade to double
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typString:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typString:
              break; //ok
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typBigDecimal:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typFloat:
            case JAVA::CJValueDef::typDouble:
              break; //can convert to bigdecimal without problems
            case JAVA::CJValueDef::typBigDecimal:
              break; //ok
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typDate:
        case JAVA::CJValueDef::typCalendar:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typDate:
            case JAVA::CJValueDef::typCalendar:
              break; //ok
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;

        case JAVA::CJValueDef::typObject:
          switch (lpValTypes[k].nType)
          {
            case JAVA::CJValueDef::typObject:
              if (lpValValues[k].l != NULL && lpCurrJValue->lpClassDef->globalJClass != NULL)
              {
                if (env->IsInstanceOf(lpValValues[k].l, lpCurrJValue->lpClassDef->globalJClass) == JNI_FALSE)
                  nCurrDist = (SIZE_T)-1; //no conversion possible
              }
              break; //ok
            default:
              nCurrDist = (SIZE_T)-1; //no conversion possible
              break;
          }
          break;
      }
    }
    //check
    if (nCurrDist < nBestDist)
    {
      lpBest = lpCurr;
      nBestDist = nCurrDist;
      if (nBestDist == 0)
        break; //full match => no need to continue searching
    }
  }
  return lpBest;
}
