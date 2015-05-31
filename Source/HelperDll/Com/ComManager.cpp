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
#include "ComManager.h"
#include "..\HookTrampoline.h"
#include "..\..\Common\MallocMacros.h"
#include "..\..\Common\AutoPtr.h"
#include "..\..\Common\FnvHash.h"
#include "..\..\Common\MemoryStream.h"
#include "..\..\Common\SimplePipesIPC.h"
#include "ComHelpers.h"
#include "..\CopyList.h"
#include <ExDisp.h>

#define X_BYTE_ENC(x, y) (x)
#if defined _M_IX86
  static
  #include "Asm\SHWeakReleaseInterface_x86.inl"
  static
  #include "Asm\CoUninitialize_x86.inl"
  static
  #include "Asm\CoInitializeSecurity_x86.inl"
#elif defined _M_X64
  static
  #include "Asm\SHWeakReleaseInterface_x64.inl"
  static
  #include "Asm\CoUninitialize_x64.inl"
  static
  #include "Asm\CoInitializeSecurity_x64.inl"
#else
  #error Platform not supported
#endif

//-----------------------------------------------------------

#define MSHLFLAGS_WEAK                                     8
#define MSHLFLAGS_KEEPALIVE                          0x20000

#define BACKGROUNDTHREAD_STOP_TIMEOUT                   5000

#ifndef STATUS_BAD_DLL_ENTRYPOINT
  #define STATUS_BAD_DLL_ENTRYPOINT  ((NTSTATUS)0xC0000251L)
#endif //!STATUS_BAD_DLL_ENTRYPOINT

//-----------------------------------------------------------

/*
MIDL_INTERFACE("00000008-0000-0000-C000-000000000046")
IProxyManager : public IUnknown
{
public:
  virtual HRESULT STDMETHODCALLTYPE CreateServer(REFCLSID rclsid, DWORD clsctx, void *pv);
  virtual BOOL STDMETHODCALLTYPE IsConnected(void);
  virtual HRESULT STDMETHODCALLTYPE LockConnection(BOOL fLock, BOOL fLastUnlockReleases);
  virtual void STDMETHODCALLTYPE Disconnect();
  virtual HRESULT STDMETHODCALLTYPE CreateServerWithHandler(REFCLSID rclsid, DWORD clsctx, void *pv,
                                                            REFCLSID rclsidHandler, IID iidSrv, void **ppv,
                                                            IID iidClnt, void *pClientSiteInterface);
};
*/

//-----------------------------------------------------------

namespace COM {

#pragma warning(disable : 4355)
CMainManager::CMainManager(__in CCallbacks *_lpCallback, __in CNktHookLib *_lpHookMgr) : CNktThread()
{
  _ASSERT(_lpCallback != NULL && _lpHookMgr != NULL);
  lpCallback  = _lpCallback;
  lpHookMgr = _lpHookMgr;
  nHookFlags = 0;
  memset(&sOLE32, 0, sizeof(sOLE32));
  memset(&sSHLWAPI, 0, sizeof(sSHLWAPI));
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
  CDll *lpDll;
  CInterfaceMethod *lpInterfaceMethod;
  CThreadEventBase *lpEventBase;

  CNktThread::Stop(BACKGROUNDTHREAD_STOP_TIMEOUT);
  RemoveAllInstances(TRUE);
  RemoveDeleteAsyncInstances(TRUE);
  //----
  while ((lpDll = cDllsList.PopTail()) != NULL)
  {
    lpDll->Finalize();
    delete lpDll;
  }
  //----
  while ((lpEventBase = sThreadEvents.cList[1].PopTail()) != NULL)
    lpEventBase->Release();
  while ((lpEventBase = sThreadEvents.cList[0].PopTail()) != NULL)
    lpEventBase->Release();
  //----
  while ((lpInterfaceMethod = cIFaceMethodsList.PopTail()) != NULL)
    delete lpInterfaceMethod;
  //----
  ProcessDllUnload(sOLE32.hDll);
  ProcessDllUnload(sSHLWAPI.hDll);
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
  if (sThreadEvents.cEventAvailableEv.Create(TRUE, FALSE) == FALSE ||
      CNktThread::Start() == FALSE)
    return E_OUTOFMEMORY;
  return S_OK;
}

VOID CMainManager::OnExitProcess()
{
  return;
}

HRESULT CMainManager::ProcessDllLoad(__in HINSTANCE hDll)
{
  WCHAR szBufW[8192];
  SIZE_T i;
  HRESULT hRes;

  if (hHeap == NULL || dwTlsIndex == TLS_OUT_OF_INDEXES)
    return E_OUTOFMEMORY;
  i = (SIZE_T)::GetModuleFileNameW(hDll, szBufW, NKT_ARRAYLEN(szBufW));
  while (i > 0 && szBufW[i-1] != L'/' && szBufW[i-1] != L'\\')
    i--;
  if (_wcsicmp(szBufW+i, L"ole32.dll") == 0)
  {
    CNktSimpleLockNonReentrant cDllLock(&(sOLE32.nDllMutex));
    PIMAGE_DOS_HEADER lpDosHdr;
    PIMAGE_NT_HEADERS lpNtHdr;

    sOLE32.hDll = hDll;
    lpDosHdr = (PIMAGE_DOS_HEADER)hDll;
    sOLE32.fnDllMain = NULL;
    if (lpDosHdr->e_magic == IMAGE_DOS_SIGNATURE)
    {
      lpNtHdr = (PIMAGE_NT_HEADERS)((LPBYTE)lpDosHdr + (SIZE_T)(ULONG)(lpDosHdr->e_lfanew));
      if (lpNtHdr->Signature == IMAGE_NT_SIGNATURE)
      {
        sOLE32.fnDllMain = (lpfnDllMain)((LPBYTE)(sOLE32.hDll) + (SIZE_T)(lpNtHdr->OptionalHeader.AddressOfEntryPoint));
        sOLE32.nDllSize = (SIZE_T)(lpNtHdr->OptionalHeader.SizeOfImage);
      }
    }
    if (sOLE32.fnDllMain == NULL)
      return HRESULT_FROM_NT(STATUS_BAD_DLL_ENTRYPOINT);
    sOLE32.fnCoUninitialize = (lpfnCoUninitialize)::GetProcAddress(sOLE32.hDll, "CoUninitialize");
    sOLE32.fnCoInitializeSecurity = (lpfnCoInitializeSecurity)::GetProcAddress(sOLE32.hDll, "CoInitializeSecurity");
    if (sOLE32.fnCoUninitialize != NULL)
    {
      sOLE32.sHooks[0].lpProcToHook = sOLE32.fnCoUninitialize;
      sOLE32.sHooks[0].lpNewProcAddr = HOOKTRAMPOLINECREATE(hHeap, aCoUninitialize, &CMainManager::CoUninitializeStub);
      if (sOLE32.sHooks[0].lpNewProcAddr == NULL)
        return E_OUTOFMEMORY;
    }
    else
    {
      sOLE32.sHooks[0].lpNewProcAddr = NULL;
    }
    if (sOLE32.fnCoInitializeSecurity != NULL)
    {
      sOLE32.sHooks[1].lpProcToHook = sOLE32.fnCoInitializeSecurity;
      sOLE32.sHooks[1].lpNewProcAddr = HOOKTRAMPOLINECREATE(hHeap, aCoInitializeSecurity,
                                                            &CMainManager::CoInitializeSecurityStub);
      if (sOLE32.sHooks[1].lpNewProcAddr == NULL)
      {
        HookTrampoline_Delete(hHeap, sOLE32.sHooks[0].lpNewProcAddr);
        memset(&(sOLE32.sHooks), 0, sizeof(sOLE32.sHooks));
        return E_OUTOFMEMORY;
      }
    }
    else
    {
      sOLE32.sHooks[1].lpNewProcAddr = NULL;
    }
    if (sOLE32.fnCoUninitialize != NULL || sOLE32.fnCoInitializeSecurity != NULL)
    {
      if (sOLE32.fnCoUninitialize != NULL && sOLE32.fnCoInitializeSecurity != NULL)
        hRes = NKT_HRESULT_FROM_WIN32(lpHookMgr->Hook(&(sOLE32.sHooks[0]), 2));
      else if (sOLE32.fnCoUninitialize != NULL)
        hRes = NKT_HRESULT_FROM_WIN32(lpHookMgr->Hook(&(sOLE32.sHooks[0]), 1));
      else
        hRes = NKT_HRESULT_FROM_WIN32(lpHookMgr->Hook(&(sOLE32.sHooks[1]), 1));
      if (FAILED(hRes))
      {
        HookTrampoline_Delete(hHeap, sOLE32.sHooks[1].lpNewProcAddr);
        HookTrampoline_Delete(hHeap, sOLE32.sHooks[0].lpNewProcAddr);
        memset(&(sOLE32.sHooks), 0, sizeof(sOLE32.sHooks));
        return hRes;
      }
    }
  }
  else if (_wcsicmp(szBufW+i, L"shlwapi.dll") == 0)
  {
    CNktSimpleLockNonReentrant cDllLock(&(sSHLWAPI.nDllMutex));

    sSHLWAPI.hDll = hDll;
    sSHLWAPI.fnSHWeakReleaseInterface = (lpfnSHWeakReleaseInterface)::GetProcAddress(sSHLWAPI.hDll,
                                                                                     "SHWeakReleaseInterface");
    if (sSHLWAPI.fnSHWeakReleaseInterface != NULL)
    {
      sSHLWAPI.sHooks[0].lpProcToHook = sSHLWAPI.fnSHWeakReleaseInterface;
      sSHLWAPI.sHooks[0].lpNewProcAddr = HOOKTRAMPOLINECREATE(hHeap, aSHWeakReleaseInterface,
                                                              &CMainManager::SHWeakReleaseInterfaceStub);
      if (sSHLWAPI.sHooks[0].lpNewProcAddr == NULL)
        return E_OUTOFMEMORY;
      hRes = NKT_HRESULT_FROM_WIN32(lpHookMgr->Hook(&(sSHLWAPI.sHooks[0]), 1));
      if (FAILED(hRes))
      {
        HookTrampoline_Delete(hHeap, sSHLWAPI.sHooks[0].lpNewProcAddr);
        memset(&(sSHLWAPI.sHooks), 0, sizeof(sSHLWAPI.sHooks));
        return hRes;
      }
    }
  }
  //----
  {
    CNktAutoFastMutex cLock(&cDllsListMtx);
    TNktLnkLst<CDll>::Iterator it;
    TNktAutoDeletePtr<CDll> cNewDllItem;
    CDll *lpDll;

    for (lpDll=it.Begin(cDllsList); lpDll!=NULL; lpDll=it.Next())
    {
      if (lpDll->GetHInstance() == hDll)
        break;
    }
    if (lpDll == NULL)
    {
      cNewDllItem.Attach(new CDll(this, hDll));
      if (cNewDllItem == NULL)
        return E_OUTOFMEMORY;
      hRes = cNewDllItem->Initialize();
      if (SUCCEEDED(hRes))
        cDllsList.PushTail(cNewDllItem.Detach());
      else
        cNewDllItem->Finalize();
      if (FAILED(hRes))
        return hRes;
    }
  }
  return S_OK;
}

HRESULT CMainManager::ProcessDllUnload(__in HINSTANCE hDll)
{
  TNktAutoDeletePtr<CDll> cDll;

  {
    CNktAutoFastMutex cLock(&cDllsListMtx);
    TNktLnkLst<CDll>::Iterator it;
    CDll *lpDll;

    for (lpDll=it.Begin(cDllsList); lpDll!=NULL; lpDll=it.Next())
    {
      if (lpDll->GetHInstance() == hDll)
      {
        lpDll->RemoveNode();
        cDll.Attach(lpDll);
        break;
      }
    }
  }
  if (cDll != NULL)
    cDll->Finalize();
  //----
  if (sOLE32.hDll != NULL && sOLE32.hDll == hDll)
  {
    CNktSimpleLockNonReentrant cDllLock(&(sOLE32.nDllMutex));

    if (sOLE32.hDll != NULL && sOLE32.hDll == hDll)
    {
      lpHookMgr->Unhook(sOLE32.sHooks, 2);
      HookTrampoline_Delete(hHeap, sOLE32.sHooks[1].lpNewProcAddr);
      HookTrampoline_Delete(hHeap, sOLE32.sHooks[0].lpNewProcAddr);
      memset(&sOLE32.sHooks, 0, sizeof(sOLE32.sHooks));
      //----
      sOLE32.fnCoUninitialize = NULL;
      sOLE32.fnCoInitializeSecurity = NULL;
      sOLE32.hDll = NULL;
    }
  }
  if (sSHLWAPI.hDll != NULL && sSHLWAPI.hDll == hDll)
  {
    CNktSimpleLockNonReentrant cDllLock(&(sSHLWAPI.nDllMutex));

    if (sSHLWAPI.hDll != NULL && sSHLWAPI.hDll == hDll)
    {
      lpHookMgr->Unhook(sSHLWAPI.sHooks, 1);
      HookTrampoline_Delete(hHeap, sSHLWAPI.sHooks[0].lpNewProcAddr);
      memset(&sSHLWAPI.sHooks, 0, sizeof(sSHLWAPI.sHooks));
      //----
      sSHLWAPI.fnSHWeakReleaseInterface = NULL;
      sSHLWAPI.hDll = NULL;
    }
  }
  return S_OK;
}

HRESULT CMainManager::FindOrCreateInterface(__in_opt CClassFactory *lpClassFac, __in IUnknown *lpUnk,
                                               __in REFIID riid, __out CInterface **lplpItem)
{
  CNktAutoFastMutex cLock(&cDllsListMtx);
  TNktLnkLst<CDll>::Iterator it;
  CDll *lpDll;
  SIZE_T *lpVTable;

  lpVTable = COM::GetInterfaceVTable(lpUnk);
  if (lpVTable == NULL)
    return E_POINTER;
  for (lpDll=it.Begin(cDllsList); lpDll!=NULL; lpDll=it.Next())
  {
    if ((SIZE_T)lpVTable >= (SIZE_T)(lpDll->GetHInstance()) &&
        (SIZE_T)lpVTable < (SIZE_T)(lpDll->GetHInstance())+lpDll->GetDllSize())
      break;
  }
  if (lpDll != NULL)
    return lpDll->FindOrCreateInterface(lpClassFac, lpUnk, riid, lplpItem);
  return E_FAIL;
}

HRESULT CMainManager::WatchInterface(__in TMSG_WATCHCOMINTERFACE *lpMsg, __in SIZE_T nDataSize,
                                     __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  HRESULT hRes;

  if (nDataSize != sizeof(TMSG_WATCHCOMINTERFACE))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  hRes = cWatchInterfaces.Add(lpMsg->sClsId, lpMsg->sIid);
  if (hRes == E_OUTOFMEMORY)
    return hRes;
  return lpCallback->OnComBuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
}

HRESULT CMainManager::UnwatchInterface(__in TMSG_UNWATCHCOMINTERFACE *lpMsg, __in SIZE_T nDataSize,
                                       __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  if (nDataSize != sizeof(TMSG_UNWATCHCOMINTERFACE))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  cWatchInterfaces.Remove(lpMsg->sClsId, lpMsg->sIid);
  /*
  if (cWatchInterfaces.Remove(sClsId, sIid) == S_OK)
    RemoveInterface(sIid);
  */
  return lpCallback->OnComBuildSimpleReplyData(S_OK, lplpReplyData, lpnReplyDataSize);
}

HRESULT CMainManager::UnwatchAllInterfaces(__out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  cWatchInterfaces.RemoveAll();
  /*
  RemoveAllInterfaces();
  */
  return lpCallback->OnComBuildSimpleReplyData(S_OK, lplpReplyData, lpnReplyDataSize);
}

HRESULT CMainManager::InspectInterfaceMethod(__in TMSG_INSPECTCOMINTERFACEMETHOD *lpMsg, __in SIZE_T nDataSize,
                                             __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  CNktAutoFastMutex cLock(&cIFaceMethodsListMtx);
  TNktLnkLst<CInterfaceMethod>::Iterator it;
  CInterfaceMethod *lpInterfaceMethod;
  HRESULT hRes;

  if (nDataSize != sizeof(TMSG_INSPECTCOMINTERFACEMETHOD))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  for (lpInterfaceMethod=it.Begin(cIFaceMethodsList); lpInterfaceMethod!=NULL; lpInterfaceMethod=it.Next())
  {
    if (memcmp(&(lpInterfaceMethod->sIid), &(lpMsg->sIid), sizeof(IID)) == 0 &&
        lpInterfaceMethod->nMethodIndex == lpMsg->nMethodIndex)
    {
      hRes = NKT_E_AlreadyExists;
      goto iim_done;
    }
  }
  lpInterfaceMethod = new CInterfaceMethod();
  if (lpInterfaceMethod == NULL)
  {
    hRes = E_OUTOFMEMORY;
    goto iim_done;
  }
  lpInterfaceMethod->sIid = lpMsg->sIid;
  lpInterfaceMethod->nMethodIndex = lpMsg->nMethodIndex;
  lpInterfaceMethod->nIidIndex = lpMsg->nIidIndex;
  lpInterfaceMethod->sQueryIid = lpMsg->sQueryIid;
  lpInterfaceMethod->nReturnParameterIndex = lpMsg->nReturnParameterIndex;
  cIFaceMethodsList.PushTail(lpInterfaceMethod);
  //hook already parsed interfaces
  hRes = S_OK;
  {
    CNktAutoFastMutex cLock(&cDllsListMtx);
    TNktLnkLst<CDll>::Iterator itDll;
    CDll *lpDll;

    for (lpDll=itDll.Begin(cDllsList); lpDll!=NULL && SUCCEEDED(hRes); lpDll=itDll.Next())
    {
      hRes = lpDll->OnNewInspectInterfaceMethod(lpInterfaceMethod);
    }
  }
iim_done:
  if (hRes == E_OUTOFMEMORY)
    return hRes;
  return lpCallback->OnComBuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
}

HRESULT CMainManager::GetInterfaceFromHWnd(__in TMSG_GETCOMINTERFACEFROMHWND *lpMsg, __in SIZE_T nDataSize,
                                           __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  TNktComPtr<CThreadEvent_GetInterfaceFromNNN> cEvent;
  HRESULT hRes;

  if (nDataSize != sizeof(TMSG_GETCOMINTERFACEFROMHWND))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  cEvent.Attach(new CThreadEvent_GetInterfaceFromNNN((HWND)(lpMsg->hWnd)));
  if (cEvent == NULL || cEvent->cCompletedEv.GetEventHandle() == NULL)
    return E_OUTOFMEMORY;
  cEvent->sIid = lpMsg->sIid;
  hRes = ExecuteThreadEvent(cEvent, FALSE);
  if (SUCCEEDED(hRes))
    hRes = cEvent->hRes;
  if (SUCCEEDED(hRes))
  {
    *lplpReplyData = cEvent->lpData;
    *lpnReplyDataSize = cEvent->nDataSize;
  }
  else if (hRes != E_OUTOFMEMORY)
  {
    hRes = lpCallback->OnComBuildSimpleReplyData((hRes == NKT_E_NotFound) ? S_OK : hRes, lplpReplyData,
                                                 lpnReplyDataSize);
  }
  return hRes;
}

HRESULT CMainManager::GetInterfaceFromHWnd_Worker(__in HWND hWnd, __in REFIID sIid, __out TMSG_SIMPLE_REPLY **lplpData,
                                                  __out SIZE_T *lpnDataSize)
{
  CAutoTls cAutoTls(dwTlsIndex, (LPVOID)-1); //disable extra release handling
  TNktLnkLst<CInstance>::Iterator it;
  TCopyList<CInstance> cInstItemsCopy;
  CInstance *lpInstItem;
  SIZE_T nIndex;
  HRESULT hRes;

  for (nIndex=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
  {
    CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));

    for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
    {
      if (lpInstItem->GetInterfaceFlags() != 0)
      {
        hRes = cInstItemsCopy.Add(lpInstItem, lpInstItem->GetInterfaceFlags());
        if (FAILED(hRes))
          return hRes;
      }
    }
  }
  //----
  for (nIndex=0; nIndex<cInstItemsCopy.GetCount(); nIndex++)
  {
    lpInstItem = cInstItemsCopy.GetItemAt(nIndex);
    if ((cInstItemsCopy.GetParamAt(nIndex) & IID_IS_IWebBrowser2) != 0)
    {
      TNktComPtr<IWebBrowser2> cWebBrowser2;
      SHANDLE_PTR hWinWnd;

      hRes = lpInstItem->cUnk->QueryInterface(IID_IWebBrowser2, (LPVOID*)&cWebBrowser2);
      if (SUCCEEDED(hRes))
      {
        hWinWnd = NULL;
        hRes = cWebBrowser2->get_HWND(&hWinWnd);
      }
      cWebBrowser2.Release();
      if (SUCCEEDED(hRes) && (HWND)hWinWnd == hWnd)
      {
        hRes = lpInstItem->BuildResponsePacket(sIid, sizeof(TMSG_SIMPLE_REPLY), (LPVOID*)lplpData, lpnDataSize);
        if (SUCCEEDED(hRes))
        {
          ((TMSG_SIMPLE_REPLY*)(*lplpData))->hRes = S_OK;
          break;
        }
      }
    }
    if ((cInstItemsCopy.GetParamAt(nIndex) & IID_IS_IOleWindow) != 0)
    {
      TNktComPtr<IOleWindow> cOleWin;
      HWND hWinWnd;

      hRes = lpInstItem->cUnk->QueryInterface(IID_IOleWindow, (LPVOID*)&cOleWin);
      if (SUCCEEDED(hRes))
      {
        hWinWnd = NULL;
        hRes = cOleWin->GetWindow(&hWinWnd);
      }
      cOleWin.Release();
      if (SUCCEEDED(hRes) && hWinWnd == hWnd)
      {
        hRes = lpInstItem->BuildResponsePacket(sIid, sizeof(TMSG_SIMPLE_REPLY), (LPVOID*)lplpData, lpnDataSize);
        if (SUCCEEDED(hRes))
        {
          ((TMSG_SIMPLE_REPLY*)(*lplpData))->hRes = S_OK;
          break;
        }
      }
    }
    if ((cInstItemsCopy.GetParamAt(nIndex) & IID_IS_IDispatch) != 0)
    {
      TNktComPtr<IDispatch> cDisp;
      DISPPARAMS sDp;
      VARIANT cRes;

      ::VariantInit(&cRes);
      hRes = lpInstItem->cUnk->QueryInterface(IID_IDispatch, (LPVOID*)&cDisp);
      if (SUCCEEDED(hRes))
      {
        memset(&sDp, 0, sizeof(sDp));
        try
        {
          hRes = cDisp->Invoke(DISPID_HWND, GUID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET,
                               &sDp, &cRes, NULL, NULL);
        }
        catch (...)
        {
          hRes = E_FAIL;
        }
      }
      cDisp.Release();
      if (SUCCEEDED(hRes))
      {
        hRes = S_FALSE;
        switch (cRes.vt)
        {
          case VT_I1:
            if ((HWND)cRes.cVal == hWnd)
              hRes = S_OK;
            break;
          case VT_I2:
            if ((HWND)cRes.iVal == hWnd)
              hRes = S_OK;
            break;
          case VT_I4:
            if ((HWND)cRes.lVal == hWnd)
              hRes = S_OK;
            break;
          case VT_UI1:
            if ((HWND)cRes.bVal == hWnd)
              hRes = S_OK;
            break;
          case VT_UI2:
            if ((HWND)cRes.uiVal == hWnd)
              hRes = S_OK;
            break;
          case VT_UI4:
            if ((HWND)cRes.ulVal == hWnd)
              hRes = S_OK;
            break;
        }
      }
      ::VariantClear(&cRes);
      if (hRes == S_OK)
      {
        hRes = lpInstItem->BuildResponsePacket(sIid, sizeof(TMSG_SIMPLE_REPLY), (LPVOID*)lplpData, lpnDataSize);
        if (SUCCEEDED(hRes))
        {
          ((TMSG_SIMPLE_REPLY*)(*lplpData))->hRes = S_OK;
          break;
        }
      }
    }
  }
  if (nIndex >= cInstItemsCopy.GetCount())
    hRes = NKT_E_NotFound;
  return hRes;
}

HRESULT CMainManager::GetInterfaceFromIndex(__in TMSG_GETCOMINTERFACEFROMINDEX *lpMsg, __in SIZE_T nDataSize,
                                            __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  TNktComPtr<CThreadEvent_GetInterfaceFromNNN> cEvent;
  HRESULT hRes;

  if (nDataSize != sizeof(TMSG_GETCOMINTERFACEFROMINDEX))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  cEvent.Attach(new CThreadEvent_GetInterfaceFromNNN(lpMsg->nIndex));
  if (cEvent == NULL || cEvent->cCompletedEv.GetEventHandle() == NULL)
    return E_OUTOFMEMORY;
  cEvent->sIid = lpMsg->sIid;
  hRes = ExecuteThreadEvent(cEvent, FALSE);
  if (SUCCEEDED(hRes))
    hRes = cEvent->hRes;
  if (SUCCEEDED(hRes))
  {
    *lplpReplyData = cEvent->lpData;
    *lpnReplyDataSize = cEvent->nDataSize;
  }
  else if (hRes != E_OUTOFMEMORY)
  {
    hRes = lpCallback->OnComBuildSimpleReplyData((hRes == NKT_E_NotFound) ? S_OK : hRes, lplpReplyData,
                                                 lpnReplyDataSize);
  }
  return hRes;
}

HRESULT CMainManager::GetInterfaceFromIndex_Worker(__in REFIID sIid, __in ULONG nInterfaceIndex,
                                                   __out TMSG_SIMPLE_REPLY **lplpData, __out SIZE_T *lpnDataSize)
{
  CAutoTls cAutoTls(dwTlsIndex, (LPVOID)-1); //disable extra release handling
  TNktLnkLst<CInstance>::Iterator it;
  TCopyList<CInstance> cInstItemsCopy;
  TNktComPtr<IUnknown> cUnk;
  CInstance *lpInstItem;
  SIZE_T nIndex;
  HRESULT hRes;

  for (nIndex=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
  {
    CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));

    for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
    {
      if (lpInstItem->CheckSupportedIid(sIid) != FALSE)
      {
        hRes = cInstItemsCopy.Add(lpInstItem, lpInstItem->GetInterfaceFlags());
        if (FAILED(hRes))
          return hRes;
      }
    }
  }
  //----
  for (nIndex=0; nIndex<cInstItemsCopy.GetCount(); nIndex++)
  {
    lpInstItem = cInstItemsCopy.GetItemAt(nIndex);
    cUnk.Release();
    if (SUCCEEDED(lpInstItem->cUnk->QueryInterface(sIid, (LPVOID*)&cUnk)))
    {
      if (nInterfaceIndex == 0)
      {
        hRes = lpInstItem->BuildResponsePacket(sIid, sizeof(TMSG_SIMPLE_REPLY), (LPVOID*)lplpData, lpnDataSize);
        if (SUCCEEDED(hRes))
        {
          ((TMSG_SIMPLE_REPLY*)(*lplpData))->hRes = S_OK;
          break;
        }
        break;
      }
      nInterfaceIndex--;
    }
  }
  //----
  if (nIndex >= cInstItemsCopy.GetCount())
    hRes = NKT_E_NotFound;
  return hRes;
}

HRESULT CMainManager::AddInstance(__in CInterface *lpInterface, __in REFIID sIid, __in IUnknown *lpUnkRef)
{
  CAutoTls cAutoTls(dwTlsIndex, lpUnkRef); //disable extra release hand.
  TNktLnkLst<CInstance>::Iterator it;
  TNktComPtr<CInstance> cInstItem;
  CInstance *lpInstItem;
  SIZE_T nIndex;
  HRESULT hRes;

  //return (IsInterfaceWatched(lpInterface->lpClassFac, sIid) != FALSE) ? S_OK : E_FAIL;

  _ASSERT(lpInterface != NULL);
  if (lpUnkRef == NULL || lpInterface->cSupportedIids.Check(sIid) == FALSE)
    return E_FAIL;
  if (IsInterfaceWatched(lpInterface->lpClassFac, sIid) == FALSE)
    return E_FAIL;

  //----
  nIndex = (SIZE_T)fnv_32a_buf(&lpUnkRef, sizeof(IUnknown*), FNV1A_32_INIT) % IIDITEM_BIN_COUNT;
  {
    CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));

    for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
    {
      if (lpInstItem->GetUnkRef() == lpUnkRef)
        return S_OK;
    }
    cInstItem.Attach(new CInstance(lpInterface, sIid, lpUnkRef));
    if (cInstItem == NULL)
      return E_OUTOFMEMORY;
    sActiveObjects[nIndex].cList.PushTail(cInstItem);
  }
  hRes = cInstItem->Initialize(lpUnkRef);
  if (SUCCEEDED(hRes))
  {
    cInstItem.Detach();
  }
  else
  {
    CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));

    cInstItem->RemoveNode();
  }
  return hRes;
}

HRESULT CMainManager::RemoveInstance(__in IUnknown *lpUnk)
{
  CInstance *lpInstItem;
  SIZE_T nIndex;
  HRESULT hRes;

  if (lpUnk == NULL)
    return S_FALSE;
  hRes = S_FALSE;
  nIndex = (SIZE_T)fnv_32a_buf(&lpUnk, sizeof(IUnknown*), FNV1A_32_INIT) % IIDITEM_BIN_COUNT;
  {
    CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));
    TNktLnkLst<CInstance>::Iterator it;

    for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
    {
      if (lpInstItem->GetUnkRef() == lpUnk)
      {
        lpInstItem->RemoveNode();
        hRes = S_OK;
        break;
      }
    }
  }
  if (lpInstItem != NULL)
  {
    lpInstItem->Invalidate(TRUE, TRUE);
    lpInstItem->Release();
  }
  return hRes;
}

VOID CMainManager::RemoveInstance(__in CInterface *lpInterface)
{
  CInstance *lpInstItem;
  SIZE_T nIndex;

  //an interface is removed when a dll is being unloaded so at this point there
  //should not be any remaining instances but if some issue occurs, detach data from them
  for (nIndex=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
  {
    do
    {
      {
        CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));
        TNktLnkLst<CInstance>::Iterator it;

        for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
        {
          if (lpInstItem->GetInterface() == lpInterface)
          {
            lpInstItem->RemoveNode();
            break;
          }
        }
      }
      if (lpInstItem != NULL)
      {
        lpInstItem->Invalidate(FALSE, FALSE);
      }
    }
    while (lpInstItem != NULL);
  }
  return;
}

/*
VOID CMainManager::RemoveInstance(__in IID &sIid)
{
  CInstance *lpInstItem;
  SIZE_T nIndex;

  for (nIndex=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
  {
    do
    {
      {
        CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));
        TNktLnkLst<CInstance>::Iterator it;

        for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
        {
          if (lpInstItem->CheckSupportedIid(sIid) != FALSE)
          {
            lpInstItem->RemoveNode();
            break;
          }
        }
      }
      if (lpInstItem != NULL)
      {
        CNktAutoFastMutex cLock(&(sThreadAsyncDelete.cMtx));

        sThreadAsyncDelete.cList.PushTail(lpInstItem);
      }
    }
    while (lpInstItem != NULL);
  }
  return;
}
*/

VOID CMainManager::RemoveAllInstances(__in BOOL bInDestructor)
{
  CInstance *lpInstItem;
  SIZE_T nIndex;

  for (nIndex=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
  {
    do
    {
      if (bInDestructor != FALSE)
      {
        CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));

        lpInstItem = sActiveObjects[nIndex].cList.PopTail();
      }
      else
      {
        lpInstItem = sActiveObjects[nIndex].cList.PopTail();
      }
      if (lpInstItem != NULL)
      {
        if (bInDestructor == FALSE)
          lpInstItem->Invalidate(TRUE, TRUE);
        else
          lpInstItem->Invalidate(FALSE, FALSE);
        lpInstItem->Release();
      }
    }
    while (lpInstItem != NULL);
  }
  return;
}

CInstance* CMainManager::FindInstance(__in IUnknown *lpUnk)
{
  CInstance *lpInstItem;
  SIZE_T nIndex;

  nIndex = (SIZE_T)fnv_32a_buf(&lpUnk, sizeof(IUnknown*), FNV1A_32_INIT) % IIDITEM_BIN_COUNT;
  {
    CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));
    TNktLnkLst<CInstance>::Iterator it;

    for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
    {
      if (lpInstItem->GetUnkRef() == lpUnk)
      {
        lpInstItem->AddRef();
        break;
      }
    }
  }
  return lpInstItem;
}

VOID CMainManager::RemoveInvalidInstances()
{
  CInstance *lpInstItem;
  DWORD aTidsList[256];
  SIZE_T nIndex, i, nTidsCount;
  HANDLE hThread;

  for (nIndex=nTidsCount=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
  {
    CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));
    TNktLnkLst<CInstance>::Iterator it;

    for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL && nTidsCount<NKT_ARRAYLEN(aTidsList);
         lpInstItem=it.Next())
    {
      for (i=0; i<nTidsCount; i++)
      {
        if (aTidsList[i] == lpInstItem->GetTid())
          break;
      }
      if (i >= nTidsCount)
        aTidsList[nTidsCount++] = lpInstItem->GetTid();
    }
  }
  //----
  for (nIndex=0; nIndex<nTidsCount && CheckForAbort(0)==FALSE; nIndex++)
  {
    hThread = ::OpenThread(THREAD_SET_CONTEXT, FALSE, aTidsList[nIndex]);
    if (hThread != NULL)
    {
      ::QueueUserAPC(&CMainManager::RemoveInvalidApcProc, hThread, (ULONG_PTR)this);
      ::CloseHandle(hThread);
    }
    else
    {
      hThread = ::OpenThread(SYNCHRONIZE, FALSE, aTidsList[nIndex]);
      if (hThread != NULL)
      {
        ::CloseHandle(hThread);
      }
      else
      {
        //remove all instances of thread aTidsList[nIndex]
        OnRemoveInvalidInterfacesApcProc(aTidsList[nIndex]);
      }
    }
  }
  return;
}

VOID CMainManager::RemoveDeleteAsyncInstances(__in BOOL bInDestructor)
{
  CInstance *lpInstItem;

  if (bInDestructor == FALSE)
  {
    do
    {
      {
        CNktAutoFastMutex cLock(&(sThreadAsyncDelete.cMtx));

        lpInstItem = sThreadAsyncDelete.cList.PopTail();
      }
      if (lpInstItem != NULL)
      {
        lpInstItem->Invalidate(TRUE, TRUE);
        lpInstItem->Release();
      }
    }
    while (lpInstItem != NULL);
  }
  else
  {
    while ((lpInstItem = sThreadAsyncDelete.cList.PopTail()) != NULL)
    {
      lpInstItem->Invalidate(FALSE, FALSE);
      lpInstItem->Release();
    }
  }
  return;
}

HRESULT CMainManager::BuildInterprocMarshalPacket(__in IUnknown *lpUnk, __in REFIID sIid, __in SIZE_T nPrefixBytes,
                                                  __out LPVOID *lplpData, __out SIZE_T *lpnDataSize)
{
  TNktComPtr<IStream> cIProcStream;
  LARGE_INTEGER liZero;
  ULARGE_INTEGER liCurr;
  LPVOID lpMem;
  HGLOBAL hMem;
  HRESULT hRes;

  _ASSERT(lpUnk != NULL);
  _ASSERT(lplpData != NULL && lpnDataSize != NULL);
  *lplpData = NULL;
  *lpnDataSize = 0;
  hRes = ::CreateStreamOnHGlobal(NULL, TRUE, &cIProcStream);
  if (SUCCEEDED(hRes))
    hRes = ::CoMarshalInterface(cIProcStream, sIid, lpUnk, MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
  if (FAILED(hRes))
    return hRes;
  liZero.QuadPart = liCurr.QuadPart = 0;
  hRes = cIProcStream->Seek(liZero, STREAM_SEEK_CUR, &liCurr);
  if (SUCCEEDED(hRes))
  {
    if (liCurr.QuadPart <= 0x7FFFFFF0)
    {
      *lpnDataSize = (SIZE_T)(liCurr.QuadPart) + nPrefixBytes;
      *lplpData = lpCallback->OnComAllocIpcBuffer(*lpnDataSize);
      if ((*lplpData) == NULL)
        hRes = E_OUTOFMEMORY;
    }
    else
    {
      hRes = NKT_E_BufferOverflow;
    }
  }
  if (SUCCEEDED(hRes))
    hRes = ::GetHGlobalFromStream(cIProcStream, &hMem);
  if (SUCCEEDED(hRes))
  {
    lpMem = ::GlobalLock(hMem);
    if (lpMem != NULL)
    {
      memcpy((LPBYTE)(*lplpData)+nPrefixBytes, lpMem, (*lpnDataSize)-nPrefixBytes);
      ::GlobalUnlock(hMem);
    }
    else
    {
      hRes = E_OUTOFMEMORY;
    }
  }
  if (FAILED(hRes))
  {
    liZero.QuadPart = liCurr.QuadPart = 0;
    cIProcStream->Seek(liZero, STREAM_SEEK_SET, &liCurr);
    ::CoReleaseMarshalData(cIProcStream);
    if ((*lplpData) != NULL)
      *lplpData = NULL;
  }
  return hRes;
}

BOOL CMainManager::IsInterfaceWatched(__in CClassFactory *lpClassFac, __in REFIID sIid, __out_opt CLSID *lpClsId)
{
  SIZE_T nIndex;
  CLSID sClsId;
  BOOL bWatch;

  if (lpClsId != NULL)
    *lpClsId = GUID_NULL;
  bWatch = cWatchInterfaces.Check(GUID_NULL, sIid);
  nIndex = 0;
  while (bWatch == FALSE)
  {
    if (lpClassFac != NULL)
    {
      if (lpClassFac->cSupportedClsIds.GetAt(nIndex, sClsId) == FALSE)
        break;
    }
    else
    {
      if (nIndex != 0)
        break;
      memset(&sClsId, 0, sizeof(sClsId));
    }
    bWatch = cWatchInterfaces.Check(sClsId, sIid);
    if (bWatch == FALSE)
      bWatch = cWatchInterfaces.Check(sClsId, GUID_NULL);
    if (bWatch != FALSE && lpClsId != NULL)
      *lpClsId = sClsId;
    nIndex++;
  }
  return bWatch;
}

VOID CMainManager::OnInterfaceCreated(__in CInterface *lpInterface, __in REFIID sIid, __in IUnknown *lpUnk)
{
  /*/
  CInstance *lpInstance;
  */
  CLSID sClsId;

  _ASSERT(lpInterface != NULL);
  if ((nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
    CInterface::DebugOutput_InterfaceCreated(sIid, lpInterface, (SIZE_T)COM::GetInterfaceVTable(lpUnk));
  if (IsInterfaceWatched(lpInterface->lpClassFac, sIid, &sClsId) != FALSE)
  {
    /*
    lpInstance = FindInstance(lpUnk);
    if ((nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
    {
      Nektra::DebugPrintMultiStart();
      NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM: %sending OnInterfaceCreated message",
                       (lpInstance != NULL) ? "S" : "NOT s" ));
      Nektra::DebugPrintSeparator();
      Nektra::DebugPrintMultiEnd();
    }
    if (lpInstance != NULL)
    {
    */
      lpCallback->OnComInterfaceCreated(sClsId, sIid, lpUnk);
      /*
      lpInstance->Release();
    }
    */
  }
  return;
}

VOID CMainManager::OnSHWeakReleaseInterface(IUnknown *lpDest, IUnknown **lppUnknown)
{
  BOOL bRaiseException;
  DWORD dwExceptionCode;

 
  //serialize calls to the unsafe "SHWeakReleaseInterface" api
  bRaiseException = FALSE;
  cSHLWAPI_Mtx.Lock();
  __try
  {
    lpfnSHWeakReleaseInterface(sSHLWAPI.sHooks[0].lpCallOriginal)(lpDest, lppUnknown);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    dwExceptionCode = ::GetExceptionCode();
    bRaiseException = TRUE;
  }
  cSHLWAPI_Mtx.Unlock();
  if (bRaiseException != FALSE)
  {
#ifdef _DEBUG
    Nektra::DebugBreak();
#endif //_DEBUG
    ::RaiseException(dwExceptionCode, 0, 0, NULL);
  }
  return;
}

VOID CMainManager::OnCoUninitialize()
{
  CAutoTls cAutoTls(dwTlsIndex, (LPVOID)-1); //disable extra release handling
  TNktLnkLst<CInstance> cTempList;
  CInstance *lpInstItem;
  DWORD dwTid, dwInitCount;
  LPBYTE lpPtr;
  SIZE_T nIndex;

  dwTid = ::GetCurrentThreadId();
  if (CNktThread::GetThreadId() != dwTid)
  {
    //get Ole initialization count
    dwInitCount = 0xFFFFFFFFUL;
#if defined _M_IX86
    lpPtr = (LPBYTE)__readfsdword(0x18);      //get TEB
    lpPtr = *((LPBYTE*)(lpPtr+0x0F80));       //get ReservedForOle pointer
    if (lpPtr != NULL)
      dwInitCount = *((DWORD*)(lpPtr+0x18));  //get oleinit count
#elif defined _M_X64
    lpPtr = (LPBYTE)__readgsqword(0x30);      //get TEB
    lpPtr = *((LPBYTE*)(lpPtr+0x1758));       //get ReservedForOle pointer
    if (lpPtr != NULL)
      dwInitCount = *((DWORD*)(lpPtr+0x28));  //get oleinit count
#else
  #error Unsupported platform
#endif
    if (dwInitCount == 1)
    {
      //last initialization
      for (nIndex=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
      {
        CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));
        TNktLnkLst<CInstance>::Iterator it;

        for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
        {
          if (lpInstItem->GetTid() == dwTid)
          {
            lpInstItem->RemoveNode();
            cTempList.PushTail(lpInstItem);
          }
        }
      }
      {
        CNktAutoFastMutex cLock(&(sThreadAsyncDelete.cMtx));

        while ((lpInstItem = cTempList.PopHead()) != NULL)
          sThreadAsyncDelete.cList.PushTail(lpInstItem);
      }
    }
  }
  //----
  lpfnCoUninitialize(sOLE32.sHooks[0].lpCallOriginal)();
  return;
}

static BOOL Helper_OnCoInitializeSecurity(__in LONG volatile *lpnMutex, __in LONG volatile *lpnCoInitSecError)
{
  CNktSimpleLockNonReentrant cDllLock(lpnMutex);

  if ((*lpnCoInitSecError) == 0)
  {
    NktInterlockedExchange(lpnCoInitSecError, 1);
    return TRUE;
  }
  return FALSE;
}

HRESULT CMainManager::OnCoInitializeSecurity(__in_opt PSECURITY_DESCRIPTOR pSecDesc, __in LONG cAuthSvc,
                                             __in_ecount_opt(cAuthSvc) SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
                                             __in_opt void *pReserved1, __in DWORD dwAuthnLevel, __in DWORD dwImpLevel,
                                             __in_opt void *pAuthList, __in DWORD dwCapabilities,
                                             __in_opt void *pReserved3, __in SIZE_T nRetAddress)
{
  return lpfnCoInitializeSecurity(sOLE32.sHooks[1].lpCallOriginal)(pSecDesc, cAuthSvc, asAuthSvc, pReserved1,
                                                                   dwAuthnLevel, dwImpLevel, pAuthList, dwCapabilities,
                                                                   pReserved3);
  /*
  HRESULT hRes;
  BOOL bFromOle32;

  bFromOle32 = (nRetAddress < (SIZE_T)sOLE32.hDll ||
                nRetAddress >= (SIZE_T)sOLE32.hDll + sOLE32.nDllSize) ? FALSE : TRUE;
  hRes = lpfnCoInitializeSecurity(sOLE32.sHooks[1].lpCallOriginal)(pSecDesc, cAuthSvc, asAuthSvc, pReserved1,
                                                                   dwAuthnLevel, dwImpLevel, pAuthList, dwCapabilities,
                                                                   pReserved3);
  if (SUCCEEDED(hRes) && bFromOle32 != FALSE)
  {
    NktInterlockedExchange(&(sOLE32.nCoInitializeSecurityDefault), 1);
  }
  if (hRes == RPC_E_TOO_LATE && sOLE32.nCoInitializeSecurityDefault == 1 && bFromOle32 == FALSE)
  {
    if (Helper_OnCoInitializeSecurity(&(sOLE32.nDllMutex), &(sOLE32.nCoInitializeSecurityError)) != FALSE)
    {
      hRes = S_OK;
      ::CoUninitialize();
      __try
      {
        sOLE32.fnDllMain(sOLE32.hDll, DLL_PROCESS_DETACH, NULL);
        if (sOLE32.fnDllMain(sOLE32.hDll, DLL_PROCESS_ATTACH, NULL) == FALSE)
          hRes = HRESULT_FROM_NT(STATUS_DLL_INIT_FAILED);
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
        hRes = HRESULT_FROM_NT(GetExceptionCode());
      }
      if (SUCCEEDED(hRes))
        hRes = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
      if (SUCCEEDED(hRes))
      {
        hRes = lpfnCoInitializeSecurity(sOLE32.sHooks[1].lpCallOriginal)(pSecDesc, cAuthSvc, asAuthSvc, pReserved1,
                                        dwAuthnLevel, dwImpLevel, pAuthList, dwCapabilities, pReserved3);
      }
    }
  }
  return hRes;
  */
}

VOID CMainManager::OnRemoveInvalidInterfacesApcProc(__in DWORD dwTid)
{
  TCopyList<CInstance> cInstItemsCopy;
  SIZE_T nIndex;
  CInstance *lpInstItem;

  if (dwTid == 0)
  {
    dwTid = ::GetCurrentThreadId();
    for (nIndex=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
    {
      CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));
      TNktLnkLst<CInstance>::Iterator it;

      for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
      {
        if (lpInstItem->GetTid() == dwTid)
          cInstItemsCopy.Add(lpInstItem, 0);
      }
    }
    for (nIndex=0; nIndex<cInstItemsCopy.GetCount(); nIndex++)
    {
      IUnknown *lpUnk;
      BOOL bRemove;
      ULONG nCount;

      bRemove = FALSE;
      lpInstItem = cInstItemsCopy.GetItemAt(nIndex);
      {
        CNktAutoFastMutex cItemLock(lpInstItem);

        lpUnk = lpInstItem->GetUnkRef();
        if (lpUnk != NULL)
        {
          nCount = GetIUnknownRefCount(lpUnk);
          if (nCount == ULONG_MAX || nCount <= lpInstItem->nRefAddedByCoMarshal)
            bRemove = TRUE;
        }
        else
        {
          bRemove = TRUE;
        }
      }
      if (bRemove != FALSE)
      {
        TNktComPtr<CThreadEvent_RemoveInstance> cEvent;

        cEvent.Attach(new CThreadEvent_RemoveInstance(lpInstItem->GetUnkRef()));
        if (cEvent != NULL && cEvent->cCompletedEv.GetEventHandle() != NULL)
          ExecuteThreadEvent(cEvent, TRUE);
      }
    }
  }
  else
  {
    for (nIndex=0; nIndex<IIDITEM_BIN_COUNT; nIndex++)
    {
      CNktAutoFastMutex cLock(&(sActiveObjects[nIndex].cMtx));
      TNktLnkLst<CInstance>::Iterator it;

      for (lpInstItem=it.Begin(sActiveObjects[nIndex].cList); lpInstItem!=NULL; lpInstItem=it.Next())
      {
        if (lpInstItem->GetTid() == dwTid)
        {
          if (SUCCEEDED(cInstItemsCopy.Add(lpInstItem, 0)))
            lpInstItem->RemoveNode();
        }
      }
    }
    for (nIndex=0; nIndex<cInstItemsCopy.GetCount(); nIndex++)
    {
      lpInstItem = cInstItemsCopy.GetItemAt(nIndex);
      lpInstItem->Invalidate(TRUE, TRUE);
      lpInstItem->Release();
    }
  }
  return;
}

HRESULT CMainManager::ExecuteThreadEvent(__in CThreadEventBase *lpEvent, __in BOOL bIsPriority)
{
  HANDLE hDoneEvent;
  DWORD dwHitEvent;

  _ASSERT(lpEvent != NULL);
  {
    CNktAutoFastMutex cLock(&(sThreadEvents.cMtx));

    sThreadEvents.cList[(bIsPriority != FALSE) ? 0 : 1].PushTail(lpEvent);
    lpEvent->AddRef();
    hDoneEvent = lpEvent->cCompletedEv.GetEventHandle();
  }
  if (sThreadEvents.cEventAvailableEv.Set() == FALSE)
  {
    {
      CNktAutoFastMutex cLock(&(sThreadEvents.cMtx));

      lpEvent->RemoveNode();
      lpEvent->Release();
    }
    return E_FAIL;
  }
  if (hDoneEvent != NULL)
  {
    CNktThread::CoWait(INFINITE, 1, &hDoneEvent, &dwHitEvent);
    if (dwHitEvent != 1)
      return E_FAIL;
  }
  return S_OK;
}

VOID CMainManager::ThreadProc()
{
  HRESULT hRes, hResCoInit;
  HANDLE hAvailEvent;
  BOOL bCall_CoDisableCallCancellation;
  DWORD dwHitEvent;

  SetThreadName("RemoteCOMHelper/ComHookMgr");
  hResCoInit = ::OleInitialize(NULL);
  bCall_CoDisableCallCancellation = FALSE;
  if (SUCCEEDED(hResCoInit) &&
      SUCCEEDED(::CoEnableCallCancellation(NULL)))
    bCall_CoDisableCallCancellation = TRUE;
  hAvailEvent = sThreadEvents.cEventAvailableEv.GetEventHandle();
  //----
  while (CheckForAbort(1000, 1, &hAvailEvent, &dwHitEvent) == FALSE)
  {
    if (dwHitEvent == 1)
    {
      CThreadEventBase *lpEvent;

      sThreadEvents.cEventAvailableEv.Reset();
      do
      {
        {
          CNktAutoFastMutex cLock(&(sThreadEvents.cMtx));

          lpEvent = sThreadEvents.cList[0].PopHead();
          if (lpEvent == NULL)
            lpEvent = sThreadEvents.cList[1].PopHead();
        }
        if (lpEvent != NULL)
        {
          switch (lpEvent->nCode)
          {
            case CThreadEventBase::codeAddInstance:
              {
              CThreadEvent_AddInstance *lpEvData = (CThreadEvent_AddInstance*)lpEvent;
              HRESULT hRes2;

              hRes = lpEvData->cInstance->Unmarshal();
              if (FAILED(hRes))
              {
                hRes2 = RemoveInstance(lpEvData->cInstance->cUnk);
              }
              }
              break;

            case CThreadEventBase::codeRemoveInstance:
              {
              CThreadEvent_RemoveInstance *lpEvData = (CThreadEvent_RemoveInstance*)lpEvent;

              hRes = RemoveInstance(lpEvData->lpUnk);
              }
              break;

            case CThreadEventBase::codeGetInterfaceFromHwnd:
              {
              CThreadEvent_GetInterfaceFromNNN *lpEvData = (CThreadEvent_GetInterfaceFromNNN*)lpEvent;

              hRes = GetInterfaceFromHWnd_Worker(lpEvData->hWnd, lpEvData->sIid, &(lpEvData->lpData),
                                                 &(lpEvData->nDataSize));
              }
              break;

            case CThreadEventBase::codeGetInterfaceFromIndex:
              {
              CThreadEvent_GetInterfaceFromNNN *lpEvData = (CThreadEvent_GetInterfaceFromNNN*)lpEvent;

              hRes = GetInterfaceFromIndex_Worker(lpEvData->sIid, lpEvData->nIndex, &(lpEvData->lpData),
                                                  &(lpEvData->nDataSize));
              }
              break;

            default:
              hRes = E_FAIL;
              break;
          }
          InterlockedExchange((LONG volatile *)&(lpEvent->hRes), (LONG)hRes);
          if (lpEvent->cCompletedEv.GetEventHandle() != NULL)
            lpEvent->cCompletedEv.Set();
          lpEvent->Release();
        }
      }
      while (lpEvent != NULL && CheckForAbort(0) == FALSE);
    }
    else
    {
      RemoveInvalidInstances();
      RemoveDeleteAsyncInstances(FALSE);
    }
  }
  //----
  RemoveAllInstances(FALSE);
  //----
  if (bCall_CoDisableCallCancellation != FALSE)
    ::CoDisableCallCancellation(NULL);
  if (SUCCEEDED(hResCoInit))
    ::OleUninitialize();
  return;
}

VOID __stdcall CMainManager::SHWeakReleaseInterfaceStub(__in COM::CMainManager *lpCtx, __in IUnknown *lpDest,
                                                        __in IUnknown **lppUnknown)
{
  lpCtx->lpCallback->OnComHookUsage(TRUE);
  lpCtx->OnSHWeakReleaseInterface(lpDest, lppUnknown);
  lpCtx->lpCallback->OnComHookUsage(FALSE);
  return;
}

VOID __stdcall CMainManager::CoUninitializeStub(__in COM::CMainManager *lpCtx)
{
  lpCtx->lpCallback->OnComHookUsage(TRUE);
  lpCtx->OnCoUninitialize();
  lpCtx->lpCallback->OnComHookUsage(FALSE);
  return;
}

HRESULT __stdcall CMainManager::CoInitializeSecurityStub(__in COM::CMainManager *lpCtx,
                                      __in_opt PSECURITY_DESCRIPTOR pSecDesc, __in LONG cAuthSvc,
                                      __in_ecount_opt(cAuthSvc) SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
                                      __in_opt void *pReserved1, __in DWORD dwAuthnLevel, __in DWORD dwImpLevel,
                                      __in_opt void *pAuthList, __in DWORD dwCapabilities, __in_opt void *pReserved3,
                                      __in SIZE_T nRetAddress)
{
  HRESULT hRes;

  lpCtx->lpCallback->OnComHookUsage(TRUE);
  hRes = lpCtx->OnCoInitializeSecurity(pSecDesc, cAuthSvc, asAuthSvc, pReserved1, dwAuthnLevel, dwImpLevel, pAuthList,
                                       dwCapabilities, pReserved3, nRetAddress);
  lpCtx->lpCallback->OnComHookUsage(FALSE);
  return hRes;
}

VOID CALLBACK CMainManager::RemoveInvalidApcProc(__in ULONG_PTR dwParam)
{
  ((CMainManager*)dwParam)->OnRemoveInvalidInterfacesApcProc(0);
  return;
}

} //COM
