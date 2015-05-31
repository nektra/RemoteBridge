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
#include "ComDll.h"
#include "ComManager.h"
#include "..\HookTrampoline.h"
#include "..\..\Common\MallocMacros.h"
#include "..\..\Common\AutoPtr.h"
#include "..\..\Common\FnvHash.h"
#include "ComHelpers.h"

//-----------------------------------------------------------

#define X_BYTE_ENC(x, y) (x)
#if defined _M_IX86
  static
  #include "Asm\DllGetClassObject_x86.inl"
#elif defined _M_X64
  static
  #include "Asm\DllGetClassObject_x64.inl"
#else
  #error Platform not supported
#endif

//-----------------------------------------------------------

static const IID IID_IdentityUnmarshal = {
  0x0000001B, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46
};

static const IID IID_IAggregator = {
  0xD8010000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46
};

static const IID IID_IContext = {
  0x000001C0, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46
};

static const IID IID_IObjContext = {
  0x000001C6, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46
};

static const IID IID_IRundown = {
  0x00000134, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46
};

static const IID IID_IRemUnknown = {
  0x00000131, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46
};

//-----------------------------------------------------------

typedef HRESULT (__stdcall *lpfnDllGetClassObject)(__in REFCLSID rclsid, __in REFIID riid, __out LPVOID *ppv);

//-----------------------------------------------------------

namespace COM {

CDll::CDll(__in CMainManager *_lpMgr, __in HINSTANCE _hDll) : TNktLnkLstNode<CDll>(), CNktFastMutex()
{
  MEMORY_BASIC_INFORMATION sMbi;
  LPBYTE lpBaseAddr, lpCurrAddress;

  lpMgr = _lpMgr;
  memset(&sHkNfo_DllGetClassObject, 0, sizeof(sHkNfo_DllGetClassObject));
  hDll = _hDll;
  nDllSize = 0;
  if (::VirtualQuery((LPVOID)hDll, &sMbi, sizeof(sMbi)) != 0 &&
      (sMbi.Type == MEM_MAPPED || sMbi.Type == MEM_IMAGE))
  {
    lpCurrAddress = lpBaseAddr = (LPBYTE)(sMbi.AllocationBase);
    do
    {
      nDllSize += sMbi.RegionSize;
      lpCurrAddress += sMbi.RegionSize;
      if (::VirtualQuery(lpCurrAddress, &sMbi, sizeof(sMbi)) == 0)
        break;
    }
    while (lpBaseAddr == (LPBYTE)(sMbi.AllocationBase));
  }
  return;
}

CDll::~CDll()
{
  //unhooking in destructor may lead to a deadlock because critical sections taken by heap manager
  _ASSERT(sHkNfo_DllGetClassObject.nHookId == 0);
  return;
}

HRESULT CDll::Initialize()
{
  HRESULT hRes;

  sHkNfo_DllGetClassObject.lpProcToHook = ::GetProcAddress(hDll, "DllGetClassObject");
  if (sHkNfo_DllGetClassObject.lpProcToHook != NULL)
  {
    sHkNfo_DllGetClassObject.lpNewProcAddr = HOOKTRAMPOLINECREATE(lpMgr->GetHeap(), aDllGetClassObject,
                                                                  &CDll::DllGetClassObjectStub);
    if (sHkNfo_DllGetClassObject.lpNewProcAddr == NULL)
      return E_OUTOFMEMORY;
    hRes = NKT_HRESULT_FROM_WIN32(lpMgr->GetHookMgr()->Hook(&sHkNfo_DllGetClassObject, 1));
    if (FAILED(hRes))
    {
      HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_DllGetClassObject.lpNewProcAddr);
      sHkNfo_DllGetClassObject.lpNewProcAddr = NULL;
      return hRes;
    }
  }
  return S_OK;
}

VOID CDll::Finalize()
{
  SIZE_T i;

  lpMgr->GetHookMgr()->Unhook(&sHkNfo_DllGetClassObject, 1);
  HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_DllGetClassObject.lpNewProcAddr);
  memset(&sHkNfo_DllGetClassObject, 0, sizeof(sHkNfo_DllGetClassObject));
  //----
  for (i=0; i<INTERFACE_BIN_COUNT; i++)
  {
    CInterface *lpInterface;

    do
    {
      {
        CNktAutoFastMutex cLock(&(sInterfaces[i].cMtx));

        lpInterface = sInterfaces[i].cList.PopTail();
      }
      if (lpInterface != NULL)
      {
        lpInterface->Finalize();
        delete lpInterface;
      }
    }
    while (lpInterface != NULL);
  }
  //----
  for (i=0; i<CLASSFAC_BIN_COUNT; i++)
  {
    CClassFactory *lpClassFactory;

    do
    {
      {
        CNktAutoFastMutex cLock(&(sClassFactories[i].cMtx));

        lpClassFactory = sClassFactories[i].cList.PopTail();
      }
      if (lpClassFactory != NULL)
      {
        lpClassFactory->Finalize();
        delete lpClassFactory;
      }
    }
    while (lpClassFactory != NULL);
  }
  return;
}

HRESULT CDll::OnDllGetClassObject(__in REFCLSID rclsid, __in REFIID riid, __out LPVOID *ppv)
{
  CClassFactory *lpClassFactory;
  CInterface *lpInterface;
  IUnknown *lpUnkTLS;
  HRESULT hRes, hRes2;

  hRes = ((lpfnDllGetClassObject)(sHkNfo_DllGetClassObject.lpCallOriginal))(rclsid, riid, ppv);
  if (SUCCEEDED(hRes) && ppv != NULL && (*ppv) != NULL)
  {
    TNktComPtr<IClassFactory2> cCf2;
    TNktComPtr<IClassFactory> cCf;

    hRes2 = (*((IUnknown**)ppv))->QueryInterface(IID_IClassFactory2, (LPVOID*)&cCf2);
    if (SUCCEEDED(hRes2))
    {
      hRes2 = FindOrCreateClassFactory(cCf2, rclsid, &lpClassFactory);
      if (SUCCEEDED(hRes2))
        hRes2 = lpClassFactory->HookIClassFactory2();
    }
    else
    {
      hRes2 = (*((IUnknown**)ppv))->QueryInterface(IID_IClassFactory, (LPVOID*)&cCf);
      if (SUCCEEDED(hRes2))
      {
        hRes2 = FindOrCreateClassFactory(cCf, rclsid, &lpClassFactory);
        if (SUCCEEDED(hRes2))
          hRes2 = lpClassFactory->HookIClassFactory();
      }
      else
      {
        /*
        CAutoTls cAutoTls(lpMgr->GetTlsIndex(), (LPVOID)-1); //disable extra release handling
        TNktComPtr<IID_IClassFactory2> cCf2;
        TNktComPtr<IID_IClassFactory> cCf;

        hRes2 = (*ppv)->QueryInterface(IID_IClassFactory2, (LPVOID)&cCf2);
        if (SUCCEEDED(hRes2))
        {
          hRes2 = FindOrCreateClassFactory((IClassFactory2*)cCf2, &lpClassFactory);
          if (SUCCEEDED(hRes2))
            hRes2 = lpClassFactory->HookIClassFactory2();
          cCf2.Release();
        }
        else
        {
          hRes2 = (*ppv)->QueryInterface(IID_IClassFactory, (LPVOID)&cCf);
          if (SUCCEEDED(hRes2))
          {
            hRes2 = FindOrCreateClassFactory((IClassFactory*)cCf, &lpClassFactory);
            if (SUCCEEDED(hRes2))
              hRes2 = lpClassFactory->HookIClassFactory();
            cCf.Release();
          }
          else
          {
            xxx
          }
        }
        */
        lpUnkTLS = (IUnknown*)::TlsGetValue(lpMgr->GetTlsIndex());
        if (lpUnkTLS == NULL || lpUnkTLS != (IUnknown*)-1)
        {
          CAutoUnkown cAutoUnk(*((IUnknown**)ppv), lpMgr->GetTlsIndex());

          hRes2 = lpMgr->FindOrCreateInterface(NULL, cAutoUnk.GetUnk(), riid, &lpInterface);
          if (SUCCEEDED(hRes2))
            hRes2 = lpMgr->AddInstance(lpInterface, riid, cAutoUnk.GetUnk());
          //inform manager if a desired interface was created
          if (SUCCEEDED(hRes2))
            lpMgr->OnInterfaceCreated(lpInterface, riid, *((IUnknown**)ppv));
        }
        else
        {
          if ((lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
          {
            CInterface::DebugOutput_Interface(riid, (SIZE_T)GetInterfaceVTable(*((IUnknown**)ppv)), NULL, NULL,
                                              "SKIPPED(3)");
          }
        }
      }
    }
  }
  return hRes;
}

HRESULT CDll::FindOrCreateClassFactory(__in IUnknown *lpUnk, __in REFCLSID rclsid,
                                       __deref_out CClassFactory **lplpClassFac)
{
  TNktLnkLst<CClassFactory>::Iterator it;
  TNktAutoDeletePtr<CClassFactory> cNewClassFactory;
  SIZE_T nIndex, *lpVTable;
  HRESULT hRes;

  _ASSERT(lplpClassFac != NULL);
  *lplpClassFac = NULL;
  //----
  lpVTable = GetInterfaceVTable(lpUnk);
  if (lpVTable == NULL)
    return E_ACCESSDENIED;
  nIndex = (SIZE_T)fnv_32a_buf(&lpVTable, sizeof(IUnknown*), FNV1A_32_INIT) % CLASSFAC_BIN_COUNT;
  {
    CNktAutoFastMutex cLock(&(sClassFactories[nIndex].cMtx));
    CClassFactory *lpClassFactory;

    for (lpClassFactory=it.Begin(sClassFactories[nIndex].cList); lpClassFactory!=NULL; lpClassFactory=it.Next())
    {
      if (lpClassFactory->GetVTable() == lpVTable)
      {
        hRes = lpClassFactory->AddSupportedClsId(rclsid);
        if (FAILED(hRes))
          return hRes;
        if (hRes == S_OK && (lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
          CClassFactory::DebugOutput_ClassFactory(rclsid, "added");
        *lplpClassFac = lpClassFactory;
        return S_OK;
      }
    }
    cNewClassFactory.Attach(new CClassFactory(lpMgr, this, lpVTable));
    if (cNewClassFactory == NULL)
      return E_OUTOFMEMORY;
    hRes = cNewClassFactory->AddSupportedClsId(rclsid);
    if (SUCCEEDED(hRes))
    {
      if ((lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
        CClassFactory::DebugOutput_ClassFactory(rclsid, "added");
      sClassFactories[nIndex].cList.PushTail((CClassFactory*)cNewClassFactory);
      *lplpClassFac = cNewClassFactory.Detach();
    }
  }
  return hRes;
}

HRESULT CDll::FindOrCreateInterface(__in_opt CClassFactory *lpClassFac, __in IUnknown *lpUnk,
                                                     __in REFIID riid, __deref_out CInterface **lplpInterface)
{
  TNktAutoDeletePtr<CInterface> cInterface;
  SIZE_T nIndex, *lpVTable;
  HRESULT hRes;

  _ASSERT(lpUnk != NULL);
  _ASSERT(lplpInterface != NULL);
  *lplpInterface = NULL;
  lpVTable = GetInterfaceVTable(lpUnk);
  if (lpVTable == NULL)
    return E_ACCESSDENIED;
  //----
  {
    CAutoTls cAutoTls(lpMgr->GetTlsIndex(), (LPVOID)-1); //disable extra release handling

    if (IsInterfaceDerivedFrom(lpUnk, IID_IGlobalInterfaceTable) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRunningObjectTable) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IAggregator) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IMarshal) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IStdMarshalInfo) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IdentityUnmarshal) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRemUnknown) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRundown) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IPSFactory) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IPSFactoryBuffer) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IMultiQI) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IProxy) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcChannel) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcHelper) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcOptions) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcStub) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcStubBuffer) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcProxy) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcProxyBuffer) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IStubManager) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IProxyManager) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcChannelBuffer) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IAsyncRpcChannelBuffer) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IRpcSyntaxNegotiate) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IInternalMoniker) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IChannelHook) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IClientSecurity) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IServerSecurity) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_ICallFactory) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IReleaseMarshalBuffers) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IComThreadingInfo) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IContext) != FALSE ||
        IsInterfaceDerivedFrom(lpUnk, IID_IObjContext) != FALSE)
    {
      if ((lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
        CInterface::DebugOutput_Interface(riid, (SIZE_T)lpVTable, NULL, lpClassFac, "NOT added (1)");
      return E_NOTIMPL;
    }
  }
  //----
  nIndex = (SIZE_T)fnv_32a_buf(&lpVTable, sizeof(IUnknown*), FNV1A_32_INIT) % INTERFACE_BIN_COUNT;
  {
    CNktAutoFastMutex cLock(&(sInterfaces[nIndex].cMtx));
    TNktLnkLst<CInterface>::Iterator it;
    CInterface *lpInterface;

    for (lpInterface=it.Begin(sInterfaces[nIndex].cList); lpInterface!=NULL; lpInterface=it.Next())
    {
      if (lpInterface->GetVTable() == lpVTable)
      {
        hRes = lpInterface->AddSupportedIid(riid);
        if (FAILED(hRes))
          return hRes;
        if (hRes == S_OK && (lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
        {
          CInterface::DebugOutput_Interface(riid, (SIZE_T)lpVTable, lpInterface, lpInterface->GetClassFactory(),
                                            "added");
        }
        *lplpInterface = lpInterface;
        return S_OK;
      }
    }
    cInterface.Attach(new CInterface(lpMgr, this, lpVTable, lpClassFac));
    if (cInterface == NULL)
      return E_OUTOFMEMORY;
    hRes = cInterface->Initialize(lpUnk);
    if (SUCCEEDED(hRes))
      hRes = cInterface->AddSupportedIid(riid);
    if (SUCCEEDED(hRes))
    {
      if ((lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
        CInterface::DebugOutput_Interface(riid, (SIZE_T)lpVTable, cInterface, cInterface->GetClassFactory(), "added");
      sInterfaces[nIndex].cList.PushTail((CInterface*)cInterface);
      *lplpInterface = cInterface.Detach();
    }
  }
  return hRes;
}

HRESULT CDll::OnNewInspectInterfaceMethod(__in CInterfaceMethod *lpInterfaceMethod)
{
  SIZE_T nIndex;
  HRESULT hRes;

  _ASSERT(lpInterfaceMethod != NULL);
  //----
  for (nIndex=0; nIndex<INTERFACE_BIN_COUNT; nIndex++)
  {
    CNktAutoFastMutex cLock(&(sInterfaces[nIndex].cMtx));
    TNktLnkLst<CInterface>::Iterator it;
    CInterface *lpInterface;

    for (lpInterface=it.Begin(sInterfaces[nIndex].cList); lpInterface!=NULL; lpInterface=it.Next())
    {
      hRes = lpInterface->OnNewInspectInterfaceMethod(lpInterfaceMethod);
    }
  }
  return S_OK;
}

HRESULT __stdcall CDll::DllGetClassObjectStub(__in CDll *lpCtx, __in REFCLSID rclsid, __in REFIID riid,
                                              __out LPVOID *ppv)
{
  HRESULT hRes;

  lpCtx->lpMgr->lpCallback->OnComHookUsage(TRUE);
  hRes = lpCtx->OnDllGetClassObject(rclsid, riid, ppv);
  lpCtx->lpMgr->lpCallback->OnComHookUsage(FALSE);
  return hRes;
}

} //COM
