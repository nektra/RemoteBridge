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
#include "..\Common\TransportMessages.h"
#include "..\Common\Tools.h"
#include "..\Common\Com\VariantMisc.h"
#include "JavaObject.h"
#include "JavaVarConvert.h"

//-----------------------------------------------------------

HRESULT CNktRemoteBridgeImpl::DefineJavaClass(__in LONG procId, __in BSTR className, __in VARIANT byteCode)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TNktAutoDeletePtr<TMSG_DEFINEJAVACLASS> cMsg;
  TNktComSafeArrayAccess<BYTE> cByteCode;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nLen[2], nReplySize;
  LPBYTE lpDest;
  HRESULT hRes;

  if (className == NULL)
    return E_POINTER;
  if (procId == 0 || className[0] == 0)
    return E_INVALIDARG;
  if (byteCode.vt != (VT_ARRAY|VT_I1) && byteCode.vt != (VT_ARRAY|VT_UI1))
    return E_INVALIDARG;
  if (byteCode.parray == NULL)
    return E_POINTER;
  if (byteCode.parray->cDims != 1 || byteCode.parray->rgsabound[0].cElements == 0)
    return E_INVALIDARG;
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
    hRes = cByteCode.Setup(byteCode.parray);
  if (SUCCEEDED(hRes))
  {
    nLen[0] = ((className != NULL) ? (::wcslen(className)+1) : 0) * sizeof(WCHAR);
    nLen[1] = (SIZE_T)(byteCode.parray->rgsabound[0].cElements);
    cMsg.Attach((TMSG_DEFINEJAVACLASS*)NKT_MALLOC(sizeof(TMSG_DEFINEJAVACLASS)+nLen[0]+nLen[1]));
    if (cMsg == NULL)
      hRes = E_OUTOFMEMORY;
  }
  if (SUCCEEDED(hRes))
  {
    cMsg->sHeader.dwMsgCode = TMSG_CODE_DefineJavaClass;
    cMsg->nClassLen = (ULONGLONG)nLen[0];
    cMsg->nByteCodeLen = (ULONGLONG)nLen[1];
    lpDest = (LPBYTE)(((TMSG_DEFINEJAVACLASS*)cMsg)+1);
    memcpy(lpDest, className, nLen[0]);
    memcpy(lpDest+nLen[0], cByteCode.Get(), nLen[1]);
    hRes = SendMsg(cClient, (TMSG_DEFINEJAVACLASS*)cMsg, sizeof(TMSG_DEFINEJAVACLASS)+nLen[0]+nLen[1],
                   (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize == sizeof(TMSG_SIMPLE_REPLY))
    {
      hRes = lpReply->hRes;
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::CreateJavaObject(__in LONG procId, __in BSTR className, __in VARIANT constructorArgs,
                                               __deref_out INktJavaObject **ppJavaObj)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TNktAutoDeletePtr<TMSG_CREATEJAVAOBJECT> cMsg;
  TMSG_CREATEJAVAOBJECT_REPLY *lpReply = NULL;
  SIZE_T nLen[2], nReplySize;
  CNktComVariant cVt, cVtConstructorArgs;
  SAFEARRAY *lpConstructorArgsArray;
  LPBYTE lpDest;
  HRESULT hRes;

  if (ppJavaObj != NULL)
    *ppJavaObj = NULL;
  if (className == NULL || ppJavaObj == NULL)
    return E_POINTER;
  if (procId == 0 || className[0] == 0)
    return E_INVALIDARG;
  //----
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    nLen[0] = ((className != NULL) ? (::wcslen(className)+1) : 0) * sizeof(WCHAR);
    //----
    hRes = CheckInputVariantAndConvertToSafeArray(&(cVtConstructorArgs.sVt), &constructorArgs, FALSE);
  }
  if (SUCCEEDED(hRes))
  {
    lpConstructorArgsArray = (cVtConstructorArgs.sVt.vt != VT_NULL &&
                              cVtConstructorArgs.sVt.vt != VT_EMPTY) ? cVtConstructorArgs.sVt.parray : NULL;
    hRes = JAVA::VarConvert_MarshalSafeArray(lpConstructorArgsArray, (DWORD)procId, NULL, nLen[1]);
  }
  if (SUCCEEDED(hRes))
  {
    cMsg.Attach((TMSG_CREATEJAVAOBJECT*)NKT_MALLOC(sizeof(TMSG_CREATEJAVAOBJECT)+nLen[0]+nLen[1]));
    if (cMsg == NULL)
      hRes = E_OUTOFMEMORY;
  }
  if (SUCCEEDED(hRes))
  {
    cMsg->sHeader.dwMsgCode = TMSG_CODE_CreateJavaObject;
    cMsg->nClassLen = (ULONGLONG)nLen[0];
    cMsg->nParametersLen = (ULONGLONG)nLen[1];
    lpDest = (LPBYTE)(((TMSG_CREATEJAVAOBJECT*)cMsg)+1);
    memcpy(lpDest, className, nLen[0]);
    hRes = JAVA::VarConvert_MarshalSafeArray(lpConstructorArgsArray, (DWORD)procId, lpDest+nLen[0], nLen[1]);
  }
  if (SUCCEEDED(hRes))
  {
    hRes = SendMsg(cClient, (TMSG_CREATEJAVAOBJECT*)cMsg, sizeof(TMSG_CREATEJAVAOBJECT)+nLen[0]+nLen[1],
                   (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize == sizeof(TMSG_CREATEJAVAOBJECT_REPLY))
    {
      hRes = lpReply->hRes;
      if (SUCCEEDED(hRes))
      {
        if (lpReply->javaobj != 0)
        {
          hRes = FindOrCreateNktJavaObject((DWORD)procId, lpReply->javaobj, ppJavaObj);
        }
        else
        {
          _ASSERT(FALSE);
          hRes = NKT_E_InvalidData;
        }
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::InvokeJavaStaticMethod(__in LONG procId, __in BSTR className, __in BSTR methodName,
                                                     __in VARIANT parameters, __out VARIANT *value)
{
  return InternalJavaInvokeMethod((DWORD)procId, className, 0, methodName, parameters, value);
}

HRESULT CNktRemoteBridgeImpl::put_StaticJavaField(__in LONG procId, __in BSTR className, __in BSTR fieldName,
                                                  __in VARIANT value)
{
  return InternalJavaSetField((DWORD)procId, className, 0, fieldName, value);
}

HRESULT CNktRemoteBridgeImpl::get_StaticJavaField(__in LONG procId, __in BSTR className, __in BSTR fieldName,
                                                  __out VARIANT *value)
{
  return InternalJavaGetField((DWORD)procId, className, 0, fieldName, value);
}

HRESULT CNktRemoteBridgeImpl::IsJVMAttached(__in  LONG procId, __out LONG *verMajor, __out LONG *verMinor,
                                            __out VARIANT_BOOL *pRet)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TMSG_ISJVMATTACHED_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  TMSG_HEADER sMsg;
  HRESULT hRes;

  if (pRet != NULL)
    *pRet = VARIANT_FALSE;
  if (verMajor != NULL)
    *verMajor = 0;
  if (verMinor != NULL)
    *verMinor = 0;
  if (pRet == NULL)
    return E_POINTER;
  if ((verMajor == NULL && verMinor != NULL) ||
      (verMajor != NULL && verMinor == NULL))
    return E_POINTER;
  if (procId == 0)
    return E_INVALIDARG;
  //----
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.dwMsgCode = TMSG_CODE_IsJVMAttached;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize == sizeof(TMSG_ISJVMATTACHED_REPLY))
    {
      if (lpReply->hRes == S_OK)
      {
        *pRet = VARIANT_TRUE;
        if (verMajor != NULL)
          *verMajor = (LONG)(ULONG)(lpReply->wVersionMajor);
        if (verMinor != NULL)
          *verMinor = (LONG)(ULONG)(lpReply->wVersionMinor);
      }
      else if (lpReply->hRes != S_FALSE)
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
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::FindOrCreateNktJavaObject(__in DWORD dwPid, __in ULONGLONG javaobj,
                                                        __deref_out INktJavaObject **ppJavaObj)
{
  CNktAutoFastMutex cLock(&(sJavaObjects.cMtx));
  CComObject<CNktJavaObjectImpl>* pJavaObj;
  TNktLnkLst<CNktJavaObjectImpl>::Iterator it;
  CNktJavaObjectImpl *lpJavaObjImpl;
  HRESULT hRes;

  if (ppJavaObj == NULL)
    return E_POINTER;
  *ppJavaObj = NULL;
  if (dwPid == 0 || javaobj == 0)
    return E_INVALIDARG;
  for (lpJavaObjImpl=it.Begin(sJavaObjects.cList); lpJavaObjImpl!=NULL; lpJavaObjImpl=it.Next())
  {
    if (lpJavaObjImpl->dwPid == dwPid && lpJavaObjImpl->javaobj == javaobj)
      return lpJavaObjImpl->QueryInterface(IID_INktJavaObject, (LPVOID*)ppJavaObj);
  }
  //not found, create a new object
  hRes = CComObject<CNktJavaObjectImpl>::CreateInstance(&pJavaObj);
  if (SUCCEEDED(hRes))
  {
    hRes = pJavaObj->QueryInterface(IID_INktJavaObject, (LPVOID*)ppJavaObj);
    if (SUCCEEDED(hRes))
    {
      pJavaObj->dwPid = dwPid;
      pJavaObj->javaobj = javaobj;
      pJavaObj->lpBridgeImpl = this;
      sJavaObjects.cList.PushHead(pJavaObj);
    }
    else
    {
      delete pJavaObj;
    }
  }
  return hRes;
}

VOID CNktRemoteBridgeImpl::DisconnectNktJavaObject(__in CNktJavaObjectImpl *lpJavaObjImpl)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  DWORD dwPid;
  TMSG_REMOVEJOBJECTREF sMsg;
  HRESULT hRes;

  {
    CNktAutoFastMutex cLock(&(sJavaObjects.cMtx));

    lpJavaObjImpl->RemoveNode();
    dwPid = lpJavaObjImpl->GetPid();
    sMsg.javaobj = lpJavaObjImpl->GetJObject();
  }
  //----
  hRes = CClientMgr::GetClientByPid(dwPid, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.sHeader.dwMsgCode = TMSG_CODE_RemoveJObjectRef;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg));
  }
  return;
}

VOID CNktRemoteBridgeImpl::DisconnectAllNktJavaObjects()
{
  CNktAutoFastMutex cLock(&(sJavaObjects.cMtx));
  CNktJavaObjectImpl *lpJavaObjImpl;

  while ((lpJavaObjImpl=sJavaObjects.cList.PopHead()) != NULL)
    lpJavaObjImpl->Disconnect(TRUE);
  return;
}

HRESULT CNktRemoteBridgeImpl::InternalJavaInvokeMethod(__in DWORD dwPid, __in BSTR className, __in ULONGLONG javaobj,
                                                       __in BSTR methodName, __in VARIANT parameters,
                                                       __out VARIANT *value)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TNktAutoDeletePtr<TMSG_INVOKEJAVAMETHOD> cMsg;
  VARCONVERT_UNMARSHALVARIANTS_DATA sUnmarshalData;
  TMSG_INVOKEJAVAMETHOD_REPLY *lpReply = NULL;
  CNktComVariant cVtTemp, cVt, cVtParamsArgs;
  SAFEARRAY *lpParamsArgsArray;
  SIZE_T nLen[3], nReplySize;
  LPBYTE lpDest;
  HRESULT hRes;

  if (value != NULL)
  {
    ::VariantInit(value);
    V_VT(value) = VT_EMPTY;
  }
  else
  {
    value = &(cVtTemp.sVt);
  }
  if (methodName == NULL)
    return E_POINTER;
  if (dwPid == 0 || methodName[0] == 0)
    return E_INVALIDARG;
  if (javaobj == 0)
  {
    //if no JObject is given, className is the name of a static class
    if (className == NULL)
      return E_POINTER;
    if (className[0] == 0)
      return E_INVALIDARG;
  }
  else
  {
    //if JObject is given and className too, className is the name of the SuperClass to use
    //if className is NULL, then call a std method
    if (className != NULL && className[0] == 0)
      return E_INVALIDARG;
  }
  //----
  hRes = CClientMgr::GetClientByPid(dwPid, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    nLen[0] = ((className != NULL) ? (::wcslen(className)+1) : 0) * sizeof(WCHAR);
    nLen[1] = ((methodName != NULL) ? (::wcslen(methodName)+1) : 0) * sizeof(WCHAR);
    //----
    hRes = CheckInputVariantAndConvertToSafeArray(&(cVtParamsArgs.sVt), &parameters, FALSE);
  }
  if (SUCCEEDED(hRes))
  {
    lpParamsArgsArray = (cVtParamsArgs.sVt.vt != VT_NULL &&
                         cVtParamsArgs.sVt.vt != VT_EMPTY) ? cVtParamsArgs.sVt.parray : NULL;
    hRes = JAVA::VarConvert_MarshalSafeArray(lpParamsArgsArray, dwPid, NULL, nLen[2]);
  }
  if (SUCCEEDED(hRes))
  {
    cMsg.Attach((TMSG_INVOKEJAVAMETHOD*)NKT_MALLOC(sizeof(TMSG_INVOKEJAVAMETHOD)+nLen[0]+nLen[1]+nLen[2]));
    if (cMsg == NULL)
      hRes = E_OUTOFMEMORY;
  }
  if (SUCCEEDED(hRes))
  {
    cMsg->sHeader.dwMsgCode = TMSG_CODE_InvokeJavaMethod;
    cMsg->javaobj = javaobj;
    cMsg->nClassLen = (ULONGLONG)nLen[0];
    cMsg->nMethodLen = (ULONGLONG)nLen[1];
    cMsg->nParametersLen = (ULONGLONG)nLen[2];
    lpDest = (LPBYTE)(((TMSG_INVOKEJAVAMETHOD*)cMsg)+1);
    memcpy(lpDest, className, nLen[0]);
    memcpy(lpDest+nLen[0], methodName, nLen[1]);
    hRes = JAVA::VarConvert_MarshalSafeArray(lpParamsArgsArray, dwPid, lpDest+nLen[0]+nLen[1], nLen[2]);
  }
  if (SUCCEEDED(hRes))
  {
    hRes = SendMsg(cClient, (TMSG_INVOKEJAVAMETHOD*)cMsg, sizeof(TMSG_INVOKEJAVAMETHOD)+nLen[0]+nLen[1]+nLen[2],
                   (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize >= sizeof(TMSG_INVOKEJAVAMETHOD_REPLY) &&
        nReplySize == sizeof(TMSG_INVOKEJAVAMETHOD_REPLY)+(SIZE_T)(lpReply->nResultLen))
    {
      hRes = lpReply->hRes;
      if (SUCCEEDED(hRes))
      {
        sUnmarshalData.lpThis = this;
        sUnmarshalData.lpfnFindOrCreateNktJavaObject = &CNktRemoteBridgeImpl::FindOrCreateNktJavaObject;
        sUnmarshalData.dwPid = dwPid;
        hRes = JAVA::VarConvert_UnmarshalVariants((LPBYTE)(lpReply+1), (SIZE_T)(lpReply->nResultLen),
                                                  &sUnmarshalData, *value);
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::InternalJavaSetField(__in DWORD dwPid, __in BSTR className, __in ULONGLONG javaobj,
                                                   __in BSTR fieldName, __in VARIANT value)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TNktAutoDeletePtr<TMSG_PUTJAVAFIELD> cMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SAFEARRAY *lpValueArray = NULL;
  SIZE_T nLen[3], nReplySize;
  LONG indices[1];
  LPBYTE lpDest;
  HRESULT hRes;

  if (fieldName == NULL)
    return E_POINTER;
  if (dwPid == 0 || fieldName[0] == 0)
    return E_INVALIDARG;
  if (javaobj == 0)
  {
    if (className == NULL)
      return E_POINTER;
    if (className[0] == 0)
      return E_INVALIDARG;
  }
  else
  {
    //if JObject is given and className too, className is the name of the SuperClass to use
    //if className is NULL, then call a std method
    if (className != NULL && className[0] == 0)
      return E_INVALIDARG;
  }
  //----
  hRes = CClientMgr::GetClientByPid(dwPid, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    nLen[0] = ((className != NULL) ? (::wcslen(className)+1) : 0) * sizeof(WCHAR);
    nLen[1] = ((fieldName != NULL) ? (::wcslen(fieldName)+1) : 0) * sizeof(WCHAR);
    lpValueArray = ::SafeArrayCreateVector(VT_VARIANT, 0, 1);
    if (lpValueArray == NULL)
      hRes = E_OUTOFMEMORY;
  }
  if (SUCCEEDED(hRes))
  {
    indices[0] = 0;
    hRes = ::SafeArrayPutElement(lpValueArray, indices, &value);
  }
  if (SUCCEEDED(hRes))
  {
    hRes = JAVA::VarConvert_MarshalSafeArray(lpValueArray, dwPid, NULL, nLen[2]);
  }
  if (SUCCEEDED(hRes))
  {
    cMsg.Attach((TMSG_PUTJAVAFIELD*)NKT_MALLOC(sizeof(TMSG_PUTJAVAFIELD)+nLen[0]+nLen[1]+nLen[2]));
    if (cMsg == NULL)
      hRes = E_OUTOFMEMORY;
  }
  if (SUCCEEDED(hRes))
  {
    cMsg->sHeader.dwMsgCode = TMSG_CODE_PutJavaField;
    cMsg->javaobj = javaobj;
    cMsg->nClassLen = (ULONGLONG)nLen[0];
    cMsg->nFieldLen = (ULONGLONG)nLen[1];
    cMsg->nValueLen = (ULONGLONG)nLen[2];
    lpDest = (LPBYTE)(((TMSG_PUTJAVAFIELD*)cMsg)+1);
    memcpy(lpDest, className, nLen[0]);
    memcpy(lpDest+nLen[0], fieldName, nLen[1]);
    hRes = JAVA::VarConvert_MarshalSafeArray(lpValueArray, dwPid, lpDest+nLen[0]+nLen[1], nLen[2]);
  }
  if (SUCCEEDED(hRes))
  {
    hRes = SendMsg(cClient, (TMSG_PUTJAVAFIELD*)cMsg, sizeof(TMSG_PUTJAVAFIELD)+nLen[0]+nLen[1]+nLen[2],
                   (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize == sizeof(TMSG_SIMPLE_REPLY))
    {
      hRes = lpReply->hRes;
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpValueArray != NULL)
    ::SafeArrayDestroy(lpValueArray);
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::InternalJavaGetField(__in DWORD dwPid, __in BSTR className, __in ULONGLONG javaobj,
                                                   __in BSTR fieldName, __out VARIANT *value)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TNktAutoDeletePtr<TMSG_GETJAVAFIELD> cMsg;
  VARCONVERT_UNMARSHALVARIANTS_DATA sUnmarshalData;
  TMSG_GETJAVAFIELD_REPLY *lpReply = NULL;
  SIZE_T nLen[2], nReplySize;
  LPBYTE lpDest;
  HRESULT hRes;

  if (value != NULL)
  {
    ::VariantInit(value);
    V_VT(value) = VT_NULL;
  }
  if (fieldName == NULL || value == NULL)
    return E_POINTER;
  if (dwPid == 0 || fieldName[0] == 0)
    return E_INVALIDARG;
  if (javaobj == 0)
  {
    if (className == NULL)
      return E_POINTER;
    if (className[0] == 0)
      return E_INVALIDARG;
  }
  else
  {
    //if JObject is given and className too, className is the name of the SuperClass to use
    //if className is NULL, then call a std method
    if (className != NULL && className[0] == 0)
      return E_INVALIDARG;
  }
  //----
  hRes = CClientMgr::GetClientByPid(dwPid, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    nLen[0] = ((className != NULL) ? (::wcslen(className)+1) : 0) * sizeof(WCHAR);
    nLen[1] = ((fieldName != NULL) ? (::wcslen(fieldName)+1) : 0) * sizeof(WCHAR);
    cMsg.Attach((TMSG_GETJAVAFIELD*)NKT_MALLOC(sizeof(TMSG_GETJAVAFIELD)+nLen[0]+nLen[1]));
    if (cMsg == NULL)
      hRes = E_OUTOFMEMORY;
  }
  if (SUCCEEDED(hRes))
  {
    cMsg->sHeader.dwMsgCode = TMSG_CODE_GetJavaField;
    cMsg->javaobj = javaobj;
    cMsg->nClassLen = (ULONGLONG)nLen[0];
    cMsg->nFieldLen = (ULONGLONG)nLen[1];
    lpDest = (LPBYTE)(((TMSG_GETJAVAFIELD*)cMsg)+1);
    memcpy(lpDest, className, nLen[0]);
    memcpy(lpDest+nLen[0], fieldName, nLen[1]);
    hRes = SendMsg(cClient, (TMSG_GETJAVAFIELD*)cMsg, sizeof(TMSG_GETJAVAFIELD)+nLen[0]+nLen[1],
                   (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize >= sizeof(TMSG_GETJAVAFIELD_REPLY) &&
        nReplySize == sizeof(TMSG_GETJAVAFIELD_REPLY)+(SIZE_T)(lpReply->nResultLen))
    {
      hRes = lpReply->hRes;
      if (SUCCEEDED(hRes))
      {
        sUnmarshalData.lpThis = this;
        sUnmarshalData.lpfnFindOrCreateNktJavaObject = &CNktRemoteBridgeImpl::FindOrCreateNktJavaObject;
        sUnmarshalData.dwPid = dwPid;
        hRes = JAVA::VarConvert_UnmarshalVariants((LPBYTE)(lpReply+1), (SIZE_T)(lpReply->nResultLen),
                                                  &sUnmarshalData, *value);
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::InternalJavaIsSameObject(__in DWORD dwPid, __in ULONGLONG javaobj1,
                                                       __in ULONGLONG javaobj2, __out VARIANT_BOOL *pRet)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TMSG_ISSAMEJAVAOBJECT sMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  HRESULT hRes;

  _ASSERT(pRet != NULL);
  *pRet = VARIANT_FALSE;
  if (javaobj1 == 0 || javaobj2 == 0)
    return E_POINTER;
  //----
  hRes = CClientMgr::GetClientByPid(dwPid, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.sHeader.dwMsgCode = TMSG_CODE_IsSameJavaObject;
    sMsg.javaobj1 = javaobj1;
    sMsg.javaobj2 = javaobj2;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize == sizeof(TMSG_SIMPLE_REPLY))
    {
      if (lpReply->hRes == S_OK)
        *pRet = VARIANT_TRUE;
      else if (lpReply->hRes != S_FALSE)
        hRes = lpReply->hRes;
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::InternalJavaGetClassName(__in DWORD dwPid, __in ULONGLONG javaobj, __out BSTR *className)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TMSG_GETJAVAOBJECTCLASS sMsg;
  TMSG_GETJAVAOBJECTCLASS_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  HRESULT hRes;

  _ASSERT(className != NULL);
  if (javaobj == 0)
  {
    *className = ::SysAllocString(L"");
    return E_POINTER;
  }
  //----
  hRes = CClientMgr::GetClientByPid(dwPid, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.sHeader.dwMsgCode = TMSG_CODE_GetJavaObjectClass;
    sMsg.javaobj = javaobj;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize >= sizeof(TMSG_GETJAVAOBJECTCLASS_REPLY) &&
        nReplySize == sizeof(TMSG_GETJAVAOBJECTCLASS_REPLY)+(SIZE_T)(lpReply->nClassNameLen) &&
        (lpReply->nClassNameLen & 1) == 0)
    {
      hRes = lpReply->hRes;
      if (SUCCEEDED(hRes))
      {
        *className = ::SysAllocStringLen((LPWSTR)(lpReply+1), (UINT)(lpReply->nClassNameLen) / (UINT)sizeof(WCHAR));
        hRes = ((*className) != NULL) ? S_OK : E_OUTOFMEMORY;
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  if (FAILED(hRes))
    *className = ::SysAllocString(L"");
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::InternalJavaIsInstanceOf(__in DWORD dwPid, __in ULONGLONG javaobj, __in BSTR className,
                                                       __out VARIANT_BOOL *pRet)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TNktAutoDeletePtr<TMSG_ISJAVAOBJECTINSTANCEOF> cMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nLen, nReplySize;
  LPBYTE lpDest;
  HRESULT hRes;

  _ASSERT(pRet != NULL);
  *pRet = VARIANT_FALSE;
  if (javaobj == 0 || className == NULL)
    return E_POINTER;
  if (className[0] == 0)
    return E_INVALIDARG;
  //----
  hRes = CClientMgr::GetClientByPid(dwPid, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableJavaHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    nLen = (::wcslen(className)+1) * sizeof(WCHAR);
    cMsg.Attach((TMSG_ISJAVAOBJECTINSTANCEOF*)NKT_MALLOC(sizeof(TMSG_ISJAVAOBJECTINSTANCEOF)+nLen));
    if (cMsg == NULL)
      hRes = E_OUTOFMEMORY;
  }
  if (SUCCEEDED(hRes))
  {
    cMsg->sHeader.dwMsgCode = TMSG_CODE_IsJavaObjectInstanceOf;
    cMsg->nClassLen = (ULONGLONG)nLen;
    cMsg->javaobj = javaobj;
    lpDest = (LPBYTE)(((TMSG_ISJAVAOBJECTINSTANCEOF*)cMsg)+1);
    memcpy(lpDest, className, nLen);
    hRes = SendMsg(cClient, (TMSG_ISJAVAOBJECTINSTANCEOF*)cMsg, sizeof(TMSG_ISJAVAOBJECTINSTANCEOF)+nLen,
                   (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize == sizeof(TMSG_SIMPLE_REPLY))
    {
      if (lpReply->hRes == S_OK)
        *pRet = VARIANT_TRUE;
      else if (lpReply->hRes != S_FALSE)
        hRes = lpReply->hRes;
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

#define _reply ((TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY*)(*lplpReplyData))
HRESULT CNktRemoteBridgeImpl::RaiseOnJavaNativeMethodCalled(__in TMSG_RAISEONJAVANATIVEMETHODCALLED *lpMsg,
                                                            __in SIZE_T nDataSize, __in DWORD dwPid,
                                                            __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktStringW cStrClassNameW, cStrMethodNameW;
  LPBYTE lpMsgData;
  ULONGLONG *lpParamSizes;
  CNktComVariant cVtParams, cVtRes, cVtRes2;
  TNktComPtr<INktJavaObject> cJavaObjOrClass;
  SAFEARRAY *lpResultArray;
  SIZE_T i, k;
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED) ||
      lpMsg->nClassNameLen == 0 || (lpMsg->nClassNameLen & 1) != 0 ||
      lpMsg->nMethodNameLen == 0 || (lpMsg->nMethodNameLen & 1) != 0)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  i = (SIZE_T)(lpMsg->nClassNameLen + lpMsg->nMethodNameLen) + (SIZE_T)(lpMsg->nParamsCount)*sizeof(ULONGLONG);
  if (nDataSize < sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED)+i)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  lpMsgData = (LPBYTE)(lpMsg+1);
  lpParamSizes = (ULONGLONG*)(lpMsgData + (SIZE_T)(lpMsg->nClassNameLen + lpMsg->nMethodNameLen));
  //final size check
  for (k=0; k<(SIZE_T)(lpMsg->nParamsCount); k++)
    i += (SIZE_T)lpParamSizes[k];
  if (nDataSize != sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED)+i)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  //get class name
  hRes = S_OK;
  if (cStrClassNameW.CopyN((LPWSTR)lpMsgData, (SIZE_T)(lpMsg->nClassNameLen)/sizeof(WCHAR)) != FALSE)
    lpMsgData += (SIZE_T)(lpMsg->nClassNameLen);
  else
    hRes = E_OUTOFMEMORY;
  //get method name
  if (SUCCEEDED(hRes))
  {
    if (cStrMethodNameW.CopyN((LPWSTR)lpMsgData, (SIZE_T)(lpMsg->nMethodNameLen)/sizeof(WCHAR)) != FALSE)
      lpMsgData += (SIZE_T)(lpMsg->nMethodNameLen) + (SIZE_T)(lpMsg->nParamsCount)*sizeof(ULONGLONG);
    else
      hRes = E_OUTOFMEMORY;
  }
  //get java object/class
  if (SUCCEEDED(hRes))
    hRes = FindOrCreateNktJavaObject(dwPid, lpMsg->javaobj_or_class, &cJavaObjOrClass);
  //create array
  if (SUCCEEDED(hRes))
  {
    cVtParams.sVt.vt = VT_ARRAY|VT_VARIANT;
    cVtParams.sVt.parray = ::SafeArrayCreateVector(VT_VARIANT, 0, (ULONG)(lpMsg->nParamsCount));
    if (cVtParams.sVt.parray == NULL)
      hRes = E_OUTOFMEMORY;
  }
  //get marshalled parameters
  if (SUCCEEDED(hRes) && lpMsg->nParamsCount > 0)
  {
    VARCONVERT_UNMARSHALVARIANTS_DATA sUnmarshalData;
    TNktComSafeArrayAccess<VARIANT> cArrayAccess;

    hRes = cArrayAccess.Setup(cVtParams.sVt.parray);
    if (SUCCEEDED(hRes))
    {
      sUnmarshalData.lpThis = this;
      sUnmarshalData.lpfnFindOrCreateNktJavaObject = &CNktRemoteBridgeImpl::FindOrCreateNktJavaObject;
      sUnmarshalData.dwPid = dwPid;
      for (k=0; SUCCEEDED(hRes) && k<(SIZE_T)(lpMsg->nParamsCount); k++)
      {
        hRes = JAVA::VarConvert_UnmarshalVariants(lpMsgData, (SIZE_T)lpParamSizes[k], &sUnmarshalData,
                                                  cArrayAccess.Get()[k]);
        lpMsgData += (SIZE_T)lpParamSizes[k];
      }
    }
  }
  //fire event
  if (SUCCEEDED(hRes))
  {
    TNktComPtr<IDispatch> cDisp;

    hRes = cJavaObjOrClass.QueryInterface<IDispatch>(&cDisp);
    if (SUCCEEDED(hRes))
    {
      hRes = Fire_OnJavaNativeMethodCalled(dwPid, cStrClassNameW, cStrMethodNameW, cDisp, cVtParams.sVt.parray,
                                           &(cVtRes.sVt));
    }
  }
  if (hRes == E_OUTOFMEMORY)
    return hRes;
  //build response
  if (SUCCEEDED(hRes))
    hRes = CheckInputVariantAndConvertToSafeArray(&(cVtRes2.sVt), &(cVtRes.sVt), TRUE);
  if (SUCCEEDED(hRes))
  {
    lpResultArray = (cVtRes2.sVt.vt != VT_NULL && cVtRes2.sVt.vt != VT_EMPTY) ? cVtRes2.sVt.parray : NULL;
    hRes = JAVA::VarConvert_MarshalSafeArray(lpResultArray, dwPid, NULL, k);
  }
  else
  {
    k = 0;
  }
  *lplpReplyData = AllocIpcBuffer(sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY)+k);
  if ((*lplpReplyData) == NULL)
    return E_OUTOFMEMORY;
  if (SUCCEEDED(hRes))
  {
    hRes = JAVA::VarConvert_MarshalSafeArray(lpResultArray, dwPid, (LPBYTE)(_reply+1), k);
    if (FAILED(hRes))
    {
      if (hRes == E_OUTOFMEMORY)
        return E_OUTOFMEMORY;
      k = 0;
    }
  }
  _reply->hRes = hRes;
  _reply->nResultLen = (ULONGLONG)k;
  *lpnReplyDataSize = sizeof(TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY) + k;
  return S_OK;
}
#undef _reply
