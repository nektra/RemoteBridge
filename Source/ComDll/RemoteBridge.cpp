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
#include "HelperInjector.h"
#include "..\Common\TransportMessages.h"
#include "..\Common\Tools.h"
#include "..\Common\Com\VariantMisc.h"
#include "ProcessMemory.h"

//-----------------------------------------------------------

STDMETHODIMP CNktRemoteBridgeImpl::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = { &IID_INktRemoteBridge };
  SIZE_T i;

  for (i=0; i<NKT_ARRAYLEN(arr); i++)
  {
    if (InlineIsEqualGUID(*arr[i],riid))
      return S_OK;
  }
  return S_FALSE;
}

STDMETHODIMP CNktRemoteBridgeImpl::Hook(__in LONG procId, __in eNktHookFlags flags)
{
  HRESULT hRes;

  if (procId == 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    CNktAutoHandle cThisProc, cTargetProc;
    struct {
      CNktAutoRemoteHandle cThisProc;
      CNktAutoRemoteHandle cShutdownEv;
      CNktAutoRemoteHandle cSlavePipe;
    } sRemote;
    TNktAutoFreePtr<INIT_DATA> cInitData;

    cInitData.Attach((INIT_DATA*)malloc(sizeof(INIT_DATA)));
    hRes = (cInitData != NULL) ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hRes))
    {
      cInitData->nStructSize = (ULONG)(sizeof(INIT_DATA));
      hRes = CClientMgr::AddClient((DWORD)procId, &cClient, sRemote.cSlavePipe);
    }
    if (SUCCEEDED(hRes))
    {
      cClient->nHookFlags = (ULONG)flags;
      cTargetProc.Attach(::OpenProcess(SYNCHRONIZE|PROCESS_DUP_HANDLE, FALSE, (DWORD)procId));
      if (cTargetProc == NULL)
        hRes = NKT_HRESULT_FROM_LASTERROR();
    }
    if (SUCCEEDED(hRes))
    {
      cThisProc.Attach(::OpenProcess(SYNCHRONIZE|PROCESS_DUP_HANDLE, FALSE, ::GetCurrentProcessId()));
      if (cThisProc == NULL)
        hRes = NKT_HRESULT_FROM_LASTERROR();
    }
    if (SUCCEEDED(hRes) &&
        cClient->cShutdownEv.Create(TRUE, FALSE) == FALSE)
      hRes = E_OUTOFMEMORY;
    if (SUCCEEDED(hRes))
      hRes = sRemote.cThisProc.Init(cTargetProc, cThisProc);
    if (SUCCEEDED(hRes))
      hRes = sRemote.cShutdownEv.Init(cTargetProc, cClient->cShutdownEv.GetEventHandle());
    if (SUCCEEDED(hRes))
    {
      cInitData->hControllerProc = NKT_PTR_2_ULONGLONG((HANDLE)sRemote.cThisProc);
      cInitData->hShutdownEv = NKT_PTR_2_ULONGLONG((HANDLE)sRemote.cShutdownEv);
      cInitData->hSlavePipe = NKT_PTR_2_ULONGLONG((HANDLE)sRemote.cSlavePipe);
      cInitData->nMasterPid = ::GetCurrentProcessId();
      cInitData->nFlags = (ULONG)flags;
      hRes = InjectHelper((DWORD)procId, cInitData);
    }
    if (SUCCEEDED(hRes))
    {
      sRemote.cShutdownEv.Detach();
      sRemote.cThisProc.Detach();
      sRemote.cSlavePipe.Detach();
    }
    else
    {
      CClientMgr::RemoveClient((DWORD)procId);
    }
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::Unhook(__in LONG procId)
{
  if (procId == 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);

    CClientMgr::RemoveClient((DWORD)procId);
  }
  return S_OK;
}

STDMETHODIMP CNktRemoteBridgeImpl::UnhookAll()
{
  ObjectLock cLock(this);

  CClientMgr::RemoveAllClients();
  return S_OK;
}

STDMETHODIMP CNktRemoteBridgeImpl::FindWindow(__in LONG procId, __in my_ssize_t hParentWnd, __in BSTR wndName,
                                              __in BSTR className, __in VARIANT_BOOL recurseChilds,
                                              __out my_ssize_t *hWnd)
{
  TNktAutoDeletePtr<TMSG_FINDWINDOW> cMsg;
  HRESULT hRes;

  if (hWnd == NULL)
    return E_POINTER;
  *hWnd = 0;
  if (wndName == NULL && className == NULL)
    return E_POINTER;
  if (procId == 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    TMSG_FINDWINDOW_REPLY *lpReply = NULL;
    SIZE_T nLen[2], nReplySize;
    LPBYTE lpDest;

    hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
    if (SUCCEEDED(hRes))
    {
      nLen[0] = ((wndName != NULL) ? (::wcslen(wndName)+1) : 0) * sizeof(WCHAR);
      nLen[1] = ((className != NULL) ? (::wcslen(className)+1) : 0) * sizeof(WCHAR);
      cMsg.Attach((TMSG_FINDWINDOW*)NKT_MALLOC(sizeof(TMSG_FINDWINDOW)+nLen[0]+nLen[1]));
      if (cMsg == NULL)
        hRes = E_OUTOFMEMORY;
    }
    if (SUCCEEDED(hRes))
    {
      cMsg->sHeader.dwMsgCode = TMSG_CODE_FindWindow;
      cMsg->hParentWnd = NKT_PTR_2_ULONGLONG((my_size_t)hParentWnd);
      cMsg->bRecurseChilds = (recurseChilds != VARIANT_FALSE) ? TRUE : FALSE;
      cMsg->nNameLen = (ULONGLONG)nLen[0];
      cMsg->nClassLen = (ULONGLONG)nLen[1];
      lpDest = (LPBYTE)(((TMSG_FINDWINDOW*)cMsg)+1);
      if (wndName != NULL)
        memcpy(lpDest, wndName, nLen[0]);
      if (className != NULL)
        memcpy(lpDest+nLen[0], className, nLen[1]);
      hRes = SendMsg(cClient, (TMSG_FINDWINDOW*)cMsg, sizeof(TMSG_FINDWINDOW)+nLen[0]+nLen[1], (LPVOID*)&lpReply,
                     &nReplySize);
    }
    if (SUCCEEDED(hRes))
    {
      if (nReplySize == sizeof(TMSG_FINDWINDOW_REPLY))
      {
        *hWnd = (my_ssize_t)(my_size_t)(lpReply->hWnd);
      }
      else
      {
        _ASSERT(FALSE);
        hRes = NKT_E_InvalidData;
      }
    }
    if (lpReply != NULL)
      FreeIpcBuffer(lpReply);
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::put_WindowText(__in LONG procId, __in my_ssize_t hWnd, __in BSTR text)
{
  TNktAutoDeletePtr<TMSG_SETWINDOWTEXT> cMsg;
  HRESULT hRes;

  if (text == NULL)
    return E_POINTER;
  if (procId == 0 || hWnd == 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    TMSG_SIMPLE_REPLY *lpReply = NULL;
    SIZE_T nLen, nReplySize;
    LPBYTE lpDest;

    hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
    if (SUCCEEDED(hRes))
    {
      nLen = (::wcslen(text)+1) * sizeof(WCHAR);
      cMsg.Attach((TMSG_SETWINDOWTEXT*)NKT_MALLOC(sizeof(TMSG_SETWINDOWTEXT)+nLen));
      if (cMsg == NULL)
        hRes = E_OUTOFMEMORY;
    }
    if (SUCCEEDED(hRes))
    {
      cMsg->sHeader.dwMsgCode = TMSG_CODE_SetWindowText;
      cMsg->hWnd = NKT_PTR_2_ULONGLONG((my_size_t)hWnd);
      lpDest = (LPBYTE)(((TMSG_SETWINDOWTEXT*)cMsg)+1);
      memcpy(lpDest, text, nLen);
      hRes = SendMsg(cClient, (TMSG_SETWINDOWTEXT*)cMsg, sizeof(TMSG_SETWINDOWTEXT)+nLen, (LPVOID*)&lpReply,
                     &nReplySize);
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
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::get_WindowText(__in LONG procId, __in my_ssize_t hWnd, __out BSTR *text)
{
  TMSG_GETWINDOWTEXT sMsg;
  HRESULT hRes;

  if (text == NULL)
    return E_POINTER;
  *text = NULL;
  if (procId == 0 || hWnd == 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    TMSG_GETWINDOWTEXT_REPLY *lpReply = NULL;
    SIZE_T nReplySize;

    hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
    if (SUCCEEDED(hRes))
    {
      sMsg.sHeader.dwMsgCode = TMSG_CODE_GetWindowText;
      sMsg.hWnd = NKT_PTR_2_ULONGLONG((my_size_t)hWnd);
      hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
    }
    if (SUCCEEDED(hRes))
    {
      if (nReplySize >= sizeof(TMSG_GETWINDOWTEXT_REPLY) &&
          nReplySize == sizeof(TMSG_GETWINDOWTEXT_REPLY)+(SIZE_T)(lpReply->nTextLen))
      {
        *text = ::SysAllocStringLen(lpReply->szTextW, (UINT)(lpReply->nTextLen) / (UINT)sizeof(WCHAR));
        if ((*text) == NULL)
          hRes = E_OUTOFMEMORY;
      }
      else
      {
        _ASSERT(FALSE);
        hRes = NKT_E_InvalidData;
      }
    }
    if (lpReply != NULL)
      FreeIpcBuffer(lpReply);
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::SendMessage(__in LONG procId, __in my_ssize_t hWnd, __in LONG msg,
                                               __in my_ssize_t wParam, __in my_ssize_t lParam,
                                               __out my_ssize_t *result)
{
  TMSG_POSTSENDMESSAGE sMsg;
  HRESULT hRes;

  if (result == NULL)
    return E_POINTER;
  *result = 0;
  if (procId == 0 || hWnd == 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    TMSG_POSTSENDMESSAGE_REPLY *lpReply = NULL;
    SIZE_T nReplySize;

    hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
    if (SUCCEEDED(hRes))
    {
      sMsg.sHeader.dwMsgCode = TMSG_CODE_PostSendMessage;
      sMsg.hWnd = NKT_PTR_2_ULONGLONG((my_size_t)hWnd);
      sMsg.nMsg = (ULONG)msg;
      sMsg.wParam = NKT_PTR_2_ULONGLONG((my_size_t)wParam);
      sMsg.lParam = NKT_PTR_2_ULONGLONG((my_size_t)lParam);
      sMsg.nTimeout = ULONG_MAX;
      sMsg.bPost = FALSE;
      hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
    }
    if (SUCCEEDED(hRes))
    {
      if (nReplySize == sizeof(TMSG_POSTSENDMESSAGE_REPLY))
      {
        hRes = lpReply->hRes;
        if (SUCCEEDED(hRes))
          *result = (my_ssize_t)(my_size_t)(lpReply->nResult);
      }
      else
      {
        _ASSERT(FALSE);
        hRes = NKT_E_InvalidData;
      }
    }
    if (lpReply != NULL)
      FreeIpcBuffer(lpReply);
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::SendMessageTimeout(__in LONG procId, __in my_ssize_t hWnd, __in LONG msg,
                                                      __in my_ssize_t wParam, __in my_ssize_t lParam, __in LONG timeout,
                                                      __out my_ssize_t *result)
{
  TMSG_POSTSENDMESSAGE sMsg;
  HRESULT hRes;

  if (timeout == ULONG_MAX)
    return SendMessage(procId, hWnd, msg, wParam, lParam, result);
  if (result == NULL)
    return E_POINTER;
  *result = 0;
  if (procId == 0 || hWnd == 0 || timeout < 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    TMSG_POSTSENDMESSAGE_REPLY *lpReply = NULL;
    SIZE_T nReplySize;

    hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
    if (SUCCEEDED(hRes))
    {
      sMsg.sHeader.dwMsgCode = TMSG_CODE_PostSendMessage;
      sMsg.hWnd = NKT_PTR_2_ULONGLONG((my_size_t)hWnd);
      sMsg.nMsg = (ULONG)msg;
      sMsg.wParam = NKT_PTR_2_ULONGLONG((my_size_t)wParam);
      sMsg.lParam = NKT_PTR_2_ULONGLONG((my_size_t)lParam);
      sMsg.nTimeout = (ULONG)timeout;
      sMsg.bPost = FALSE;
      hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
    }
    if (SUCCEEDED(hRes))
    {
      if (nReplySize == sizeof(TMSG_POSTSENDMESSAGE_REPLY))
      {
        hRes = lpReply->hRes;
        if (SUCCEEDED(hRes))
          *result = (my_ssize_t)(my_size_t)(lpReply->nResult);
      }
      else
      {
        _ASSERT(FALSE);
        hRes = NKT_E_InvalidData;
      }
    }
    if (lpReply != NULL)
      FreeIpcBuffer(lpReply);
  }
  return S_OK;
}

STDMETHODIMP CNktRemoteBridgeImpl::PostMessage(__in LONG procId, __in my_ssize_t hWnd, __in LONG msg,
                                                  __in my_ssize_t wParam, __in my_ssize_t lParam,
                                                  __out VARIANT_BOOL *result)
{
  TMSG_POSTSENDMESSAGE sMsg;
  HRESULT hRes;

  if (result == NULL)
    return E_POINTER;
  *result = VARIANT_FALSE;
  if (procId == 0 || hWnd == 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    TMSG_POSTSENDMESSAGE_REPLY *lpReply = NULL;
    SIZE_T nReplySize;

    hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
    if (SUCCEEDED(hRes))
    {
      sMsg.sHeader.dwMsgCode = TMSG_CODE_PostSendMessage;
      sMsg.hWnd = NKT_PTR_2_ULONGLONG((my_size_t)hWnd);
      sMsg.nMsg = (ULONG)msg;
      sMsg.wParam = NKT_PTR_2_ULONGLONG((my_size_t)wParam);
      sMsg.lParam = NKT_PTR_2_ULONGLONG((my_size_t)lParam);
      sMsg.nTimeout = ULONG_MAX;
      sMsg.bPost = TRUE;
      hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
    }
    if (SUCCEEDED(hRes))
    {
      if (nReplySize == sizeof(TMSG_POSTSENDMESSAGE_REPLY))
      {
        hRes = lpReply->hRes;
        *result = VARIANT_TRUE;
      }
      else
      {
        _ASSERT(FALSE);
        hRes = NKT_E_InvalidData;
      }
    }
    if (lpReply != NULL)
      FreeIpcBuffer(lpReply);
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::GetChildWindow(__in LONG procId, __in my_ssize_t hParentWnd, __in LONG index,
                                                  __out my_ssize_t *hWnd)
{
  TMSG_GETCHILDWINDOW sMsg;
  HRESULT hRes;

  if (hWnd == NULL)
    return E_POINTER;
  *hWnd = 0;
  if (procId == 0 || hParentWnd == 0 || index < 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    TMSG_GETCHILDWINDOW_REPLY *lpReply = NULL;
    SIZE_T nReplySize;

    hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
    if (SUCCEEDED(hRes))
    {
      sMsg.sHeader.dwMsgCode = TMSG_CODE_GetChildWindow;
      sMsg.hParentWnd = NKT_PTR_2_ULONGLONG((my_size_t)hParentWnd);
      sMsg.bIsIndex = TRUE;
      sMsg.nIdOrIndex = (ULONG)index;
      hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
    }
    if (SUCCEEDED(hRes))
    {
      if (nReplySize == sizeof(TMSG_GETCHILDWINDOW_REPLY))
      {
        hRes = lpReply->hRes;
        if (SUCCEEDED(hRes))
          *hWnd = (my_ssize_t)(my_size_t)(lpReply->hWnd);
      }
      else
      {
        _ASSERT(FALSE);
        hRes = NKT_E_InvalidData;
      }
    }
    if (lpReply != NULL)
      FreeIpcBuffer(lpReply);
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::GetChildWindowFromId(__in LONG procId, __in my_ssize_t hParentWnd, __in LONG ctrlId,
                                                        __out my_ssize_t *hWnd)
{
  TMSG_GETCHILDWINDOW sMsg;
  HRESULT hRes;

  if (hWnd == NULL)
    return E_POINTER;
  *hWnd = 0;
  if (procId == 0 || hParentWnd == 0)
    return E_INVALIDARG;
  //----
  {
    ObjectLock cLock(this);
    TNktComPtr<CClientMgr::CClient> cClient;
    TMSG_GETCHILDWINDOW_REPLY *lpReply = NULL;
    SIZE_T nReplySize;

    hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
    if (SUCCEEDED(hRes))
    {
      sMsg.sHeader.dwMsgCode = TMSG_CODE_GetChildWindow;
      sMsg.hParentWnd = NKT_PTR_2_ULONGLONG((my_size_t)hParentWnd);
      sMsg.bIsIndex = FALSE;
      sMsg.nIdOrIndex = (ULONG)ctrlId;
      hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
    }
    if (SUCCEEDED(hRes))
    {
      if (nReplySize == sizeof(TMSG_GETCHILDWINDOW_REPLY))
      {
        hRes = lpReply->hRes;
        if (SUCCEEDED(hRes))
          *hWnd = (my_ssize_t)(my_size_t)(lpReply->hWnd);
      }
      else
      {
        _ASSERT(FALSE);
        hRes = NKT_E_InvalidData;
      }
    }
    if (lpReply != NULL)
      FreeIpcBuffer(lpReply);
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::Memory(__in LONG procId, __deref_out INktProcessMemory **ppProcMem)
{
  CComObject<CNktProcessMemoryImpl>* pProcMem;
  HRESULT hRes;

  if (ppProcMem == NULL)
    return E_POINTER;
  *ppProcMem = NULL;
  if (procId == 0)
    return E_INVALIDARG;
  //----
  hRes = CComObject<CNktProcessMemoryImpl>::CreateInstance(&pProcMem);
  if (SUCCEEDED(hRes))
  {
    pProcMem->Initialize((DWORD)procId);
    hRes = pProcMem->QueryInterface(IID_INktProcessMemory, (LPVOID*)ppProcMem);
    if (FAILED(hRes))
      delete pProcMem;
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::CreateProcess(__in BSTR imagePath, __in VARIANT_BOOL suspended,
                                                 __out_opt VARIANT *continueEvent, __out LONG *procId)
{
  return CreateProcessWithLogon(imagePath, NULL, NULL, suspended, continueEvent, procId);
}

STDMETHODIMP CNktRemoteBridgeImpl::CreateProcessWithLogon(__in BSTR imagePath, __in BSTR userName, __in BSTR password,
                                                          __in VARIANT_BOOL suspended, __out_opt VARIANT *continueEvent,
                                                          __out LONG *procId)
{
  HANDLE hContinueEvent;
  DWORD dwPid;
  HRESULT hRes;

  if (continueEvent != NULL)
  {
    ::VariantInit(continueEvent);
    continueEvent->vt = VT_EMPTY;
  }
  if (procId == NULL)
    return E_POINTER;
  *procId = NULL;
  if (imagePath == NULL ||
      (suspended != VARIANT_FALSE && continueEvent == NULL))
    return E_POINTER;
  //create new process
  if (userName == NULL)
  {
    hRes = CNktTools::_CreateProcess(dwPid, imagePath, (suspended != VARIANT_FALSE) ? TRUE : FALSE,
                                     NULL, &hContinueEvent);
  }
  else
  {
    hRes = CNktTools::_CreateProcessWithLogon(dwPid, imagePath, userName, password,
                                              (suspended != VARIANT_FALSE) ? TRUE : FALSE, NULL, &hContinueEvent);
  }
  //done
  if (SUCCEEDED(hRes) && continueEvent != NULL && hContinueEvent != NULL)
  {
#ifdef _WIN64
    continueEvent->vt = VT_I8;
    continueEvent->llVal = (LONGLONG)NKT_PTR_2_ULONGLONG(hContinueEvent);
#else _WIN64
    continueEvent->vt = VT_I4;
    continueEvent->lVal = (LONG)(ULONG)hContinueEvent;
#endif //_WIN64
  }
  *procId = (LONG)dwPid;
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::ResumeProcess(__in LONG procId, __in VARIANT continueEvent)
{
  CNktComVariant cVt;
  ULONGLONG nValue;
  HRESULT hRes;

  if (procId == 0)
    return E_INVALIDARG;
  //----
  hRes = ::VariantCopyInd(&(cVt.sVt), (VARIANTARG*)&continueEvent);
  if (FAILED(hRes))
    return hRes;
  if (NumericFromVariant(&(cVt.sVt), &nValue) == FALSE || nValue == 0)
    return E_INVALIDARG;
  ::SetEvent((HANDLE)nValue);
  CNktAutoHandle::CloseHandleSEH((HANDLE)nValue);
  return S_OK;
}

STDMETHODIMP CNktRemoteBridgeImpl::TerminateProcess(__in LONG procId, __in LONG exitCode)
{
  HANDLE hProc;
  HRESULT hRes;

  if (procId == 0)
    return E_FAIL;
  //----
  hProc = ::OpenProcess(PROCESS_TERMINATE|SYNCHRONIZE, FALSE, (DWORD)procId);
  if (hProc != NULL)
  {
    if (::TerminateProcess(hProc, (DWORD)exitCode) != FALSE)
      hRes = S_OK;
    else
      hRes = NKT_HRESULT_FROM_LASTERROR();
    ::CloseHandle(hProc);
  }
  else
  {
    hRes = E_ACCESSDENIED;
  }
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::CloseHandle(__in VARIANT h)
{
  CNktComVariant cVt;
  ULONGLONG nValue;
  HRESULT hRes;

  hRes = ::VariantCopyInd(&(cVt.sVt), (VARIANTARG*)&h);
  if (FAILED(hRes))
    return hRes;
  if (NumericFromVariant(&(cVt.sVt), &nValue) == FALSE || nValue == 0)
    return E_INVALIDARG;
  CNktAutoHandle::CloseHandleSEH((HANDLE)nValue);
  return S_OK;
}

HRESULT CNktRemoteBridgeImpl::SendMsg(__in CClient *lpClient, __in LPVOID lpData, __in SIZE_T nDataSize,
                                      __inout_opt LPVOID *lplpReply, __inout_opt SIZE_T *lpnReplySize)
{
  return CClientMgr::SendMsg(lpClient, lpData, nDataSize, lplpReply, lpnReplySize);
}

HRESULT CNktRemoteBridgeImpl::OnClientMessage(__in CClient *lpClient, __in LPBYTE lpData, __in SIZE_T nDataSize,
                                              __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  //ObjectLock cLock(this);
  union {
    TMSG_HEADER *lpMsgHdr;
    TMSG_SUSPENDCHILDPROCESS *lpMsgSuspendProc;
    TMSG_RAISEONCREATEPROCESSEVENT *lpMsgRaiseOnCreateProcEv;
    TMSG_RAISEONCOMINTERFACECREATEDEVENT *lpMsgRaiseOnComIntCreatedEv;
    TMSG_RAISEONJAVANATIVEMETHODCALLED *lpMsgRaiseJavaNativeMethodCalled;
  };
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_HEADER))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  lpMsgHdr = (TMSG_HEADER*)lpData;
  switch (lpMsgHdr->dwMsgCode)
  {
    case TMSG_CODE_SuspendChildProcess:
      {
      HANDLE hReadyExecutionEvent, hContinueExecutionEvent;
      TMSG_SUSPENDCHILDPROCESS_REPLY *lpReply;

      if (nDataSize != sizeof(TMSG_SUSPENDCHILDPROCESS))
      {
        _ASSERT(FALSE);
        return NKT_E_InvalidData;
      }
      hRes = CNktTools::SuspendAfterCreateProcessW(&hReadyExecutionEvent, &hContinueExecutionEvent,
                                    (HANDLE)(lpMsgSuspendProc->hParentProc), (HANDLE)(lpMsgSuspendProc->hProcess),
                                    (HANDLE)(lpMsgSuspendProc->hThread));
      *lplpReplyData = AllocIpcBuffer(sizeof(TMSG_SUSPENDCHILDPROCESS_REPLY));
      if ((*lplpReplyData) == NULL)
        return E_OUTOFMEMORY;
      lpReply = (TMSG_SUSPENDCHILDPROCESS_REPLY*)(*lplpReplyData);
      lpReply->hRes = hRes;
      lpReply->hReadyEvent = NKT_PTR_2_ULONGLONG(hReadyExecutionEvent);
      lpReply->hContinueEvent = NKT_PTR_2_ULONGLONG(hContinueExecutionEvent);
      *lpnReplyDataSize = sizeof(TMSG_SUSPENDCHILDPROCESS_REPLY);
      }
      break;

    case TMSG_CODE_RaiseOnCreateProcessEvent:
      if (nDataSize != sizeof(TMSG_RAISEONCREATEPROCESSEVENT))
      {
        _ASSERT(FALSE);
        return NKT_E_InvalidData;
      }
      Fire_OnCreateProcessCall(lpClient->GetPid(), lpMsgRaiseOnCreateProcEv->dwPid,
                               lpMsgRaiseOnCreateProcEv->dwTid,
                               (lpMsgRaiseOnCreateProcEv->dwPlatformBits == 64) ? TRUE : FALSE);
      *lplpReplyData = AllocIpcBuffer(sizeof(TMSG_SIMPLE_REPLY));
      if ((*lplpReplyData) == NULL)
        return E_OUTOFMEMORY;
      ((TMSG_SIMPLE_REPLY*)(*lplpReplyData))->hRes = S_OK;
      *lpnReplyDataSize = sizeof(TMSG_SIMPLE_REPLY);
      break;

    case TMSG_CODE_RaiseOnComInterfaceCreatedEvent:
      hRes = RaiseOnComInterfaceCreatedEvent(lpMsgRaiseOnComIntCreatedEv, nDataSize, lpClient->GetPid(),
                                             lplpReplyData, lpnReplyDataSize);
      if (FAILED(hRes))
        return hRes;
      break;

    case TMSG_CODE_RaiseOnJavaNativeMethodCalled:
      hRes = RaiseOnJavaNativeMethodCalled(lpMsgRaiseJavaNativeMethodCalled, nDataSize, lpClient->GetPid(),
                                           lplpReplyData, lpnReplyDataSize);
      if (FAILED(hRes))
        return hRes;
      break;

    default:
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
  }
  return S_OK;
}

VOID CNktRemoteBridgeImpl::OnClientDisconnected(__in CClient *lpClient)
{
  Fire_OnProcessUnhooked(lpClient->GetPid());
  return;
}
