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

#include "ComClassFactory.h"
#include "ComManager.h"
#include "..\HookTrampoline.h"
#include "..\..\Common\MallocMacros.h"
#include "..\..\Common\AutoPtr.h"
#include "ComHelpers.h"
#include "..\MainManager.h"

#define X_BYTE_ENC(x, y) (x)
#if defined _M_IX86
  static
  #include "Asm\IClassFactory_CreateInstance_x86.inl"
  static
  #include "Asm\IClassFactory2_CreateInstanceLic_x86.inl"
#elif defined _M_X64
  static
  #include "Asm\IClassFactory_CreateInstance_x64.inl"
  static
  #include "Asm\IClassFactory2_CreateInstanceLic_x64.inl"
#else
  #error Platform not supported
#endif

//-----------------------------------------------------------

typedef HRESULT (__stdcall *lpfnIClassFactory_CreateInstance)(__in IClassFactory *lpThis, __in IUnknown *pUnkOuter,
                                                              __in REFIID riid, __out void **ppvObject);
typedef HRESULT (__stdcall *lpfnIClassFactory2_CreateInstance)(__in IClassFactory2 *lpThis, __in IUnknown *pUnkOuter,
                                                               __in REFIID riid, __out void **ppvObject);
typedef HRESULT (__stdcall *lpfnIClassFactory2_CreateInstanceLic)(__in IClassFactory2 *lpThis, __in IUnknown *pUnkOuter,
                                                                  __in IUnknown *pUnkReserved, __in REFIID riid,
                                                                  __in BSTR bstrKey, __out PVOID *ppvObj);

//-----------------------------------------------------------

namespace COM {

CClassFactory::CClassFactory(__in CMainManager *_lpMgr, __in CDll *_lpDll, __in SIZE_T *_lpVTable) :
                     TNktLnkLstNode<CClassFactory>(), CNktFastMutex()
{
  lpMgr = _lpMgr;
  lpDll = _lpDll;
  lpVTable = _lpVTable;
  memset(sHkNfo_IClassFactoryS, 0, sizeof(sHkNfo_IClassFactoryS));
  return;
}

CClassFactory::~CClassFactory()
{
  //unhooking in destructor may lead to a deadlock because critical sections taken by heap manager
  _ASSERT(sHkNfo_IClassFactoryS[0].nHookId == 0 && sHkNfo_IClassFactoryS[1].nHookId == 0 &&
          sHkNfo_IClassFactoryS[2].nHookId == 0);
  return;
}

VOID CClassFactory::Finalize()
{
  SIZE_T i;

  lpMgr->GetHookMgr()->Unhook(sHkNfo_IClassFactoryS, 3);
  for (i=0; i<3; i++)
    HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_IClassFactoryS[i].lpNewProcAddr);
  memset(sHkNfo_IClassFactoryS, 0, sizeof(sHkNfo_IClassFactoryS));
  return;
}

HRESULT CClassFactory::AddSupportedClsId(__in REFCLSID rclsid)
{
  return cSupportedClsIds.Add(rclsid);
}

HRESULT CClassFactory::GetSupportedClsIdAt(__in SIZE_T nIndex, __out GUID &sGuid)
{
  return cSupportedClsIds.GetAt(nIndex, sGuid);
}

HRESULT CClassFactory::HookIClassFactory()
{
  if (sHkNfo_IClassFactoryS[0].nHookId == 0)
  {
    CNktAutoFastMutex cLock(this);
    HRESULT hRes;

    if (sHkNfo_IClassFactoryS[0].nHookId == 0)
    {
      sHkNfo_IClassFactoryS[0].lpProcToHook = (LPVOID)lpVTable[3];
      sHkNfo_IClassFactoryS[0].lpNewProcAddr = HOOKTRAMPOLINECREATE(lpMgr->GetHeap(), aIClassFactoryCreateInstance,
                                                                    &CClassFactory::IClassFactory_CreateInstanceStub);
      if (sHkNfo_IClassFactoryS[0].lpNewProcAddr == NULL)
        return E_OUTOFMEMORY;
      hRes = NKT_HRESULT_FROM_WIN32(lpMgr->GetHookMgr()->Hook(&sHkNfo_IClassFactoryS[0], 1));
      if (FAILED(hRes))
      {
        HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_IClassFactoryS[0].lpNewProcAddr);
        sHkNfo_IClassFactoryS[0].lpNewProcAddr = NULL;
        return hRes;
      }
    }
  }
  return S_OK;
}

HRESULT CClassFactory::HookIClassFactory2()
{
  if (sHkNfo_IClassFactoryS[1].nHookId == 0)
  {
    CNktAutoFastMutex cLock(this);
    HRESULT hRes;

    if (sHkNfo_IClassFactoryS[1].nHookId == 0)
    {
      _ASSERT(sHkNfo_IClassFactoryS[2].nHookId == 0);

      sHkNfo_IClassFactoryS[1].lpProcToHook = (LPVOID)lpVTable[3];
      sHkNfo_IClassFactoryS[1].lpNewProcAddr = HOOKTRAMPOLINECREATE(lpMgr->GetHeap(), aIClassFactoryCreateInstance,
                                                   &CClassFactory::IClassFactory2_CreateInstanceStub);
      if (sHkNfo_IClassFactoryS[1].lpNewProcAddr == NULL)
        return E_OUTOFMEMORY;
      sHkNfo_IClassFactoryS[2].lpProcToHook = (LPVOID)lpVTable[3];
      sHkNfo_IClassFactoryS[2].lpNewProcAddr = HOOKTRAMPOLINECREATE(lpMgr->GetHeap(), aIClassFactory2CreateInstanceLic,
                                                   &CClassFactory::IClassFactory2_CreateInstanceLicStub);
      if (sHkNfo_IClassFactoryS[2].lpNewProcAddr == NULL)
      {
        HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_IClassFactoryS[1].lpNewProcAddr);
        sHkNfo_IClassFactoryS[1].lpNewProcAddr = NULL;
        return E_OUTOFMEMORY;
      }
      hRes = lpMgr->GetHookMgr()->Hook(sHkNfo_IClassFactoryS+1, 2);
      if (FAILED(hRes))
      {
        HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_IClassFactoryS[2].lpNewProcAddr);
        sHkNfo_IClassFactoryS[2].lpNewProcAddr = NULL;
        HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_IClassFactoryS[1].lpNewProcAddr);
        sHkNfo_IClassFactoryS[1].lpNewProcAddr = NULL;
        return hRes;
      }
    }
  }
  return S_OK;
}

HRESULT CClassFactory::OnIClassFactory_CreateInstance(__in IClassFactory *lpThis, __in IUnknown *pUnkOuter,
                                                      __in REFIID riid, __out void **ppvObject)
{
  CInterface *lpInterface;
  IUnknown *lpUnkTLS;
  HRESULT hRes, hRes2;

  hRes = ((lpfnIClassFactory_CreateInstance)(sHkNfo_IClassFactoryS[0].lpCallOriginal))(lpThis, pUnkOuter, riid,
                                                                                       ppvObject);
  if (SUCCEEDED(hRes) && ppvObject != NULL && (*ppvObject) != NULL && lpVTable == GetInterfaceVTable(lpThis))
  {
    lpUnkTLS = (IUnknown*)::TlsGetValue(lpMgr->GetTlsIndex());
    if (lpUnkTLS == NULL || lpUnkTLS != (IUnknown*)-1)
    {
      CAutoUnkown cAutoUnk(*((IUnknown**)ppvObject), lpMgr->GetTlsIndex());

      hRes2 = lpMgr->FindOrCreateInterface(this, cAutoUnk.GetUnk(), riid, &lpInterface);
      if (SUCCEEDED(hRes2))
        hRes2 = lpMgr->AddInstance(lpInterface, riid, cAutoUnk.GetUnk());
      //inform manager if a desired interface was created
      if (SUCCEEDED(hRes2))
        lpMgr->OnInterfaceCreated(lpInterface, riid, *((IUnknown**)ppvObject));
    }
    else
    {
      if ((lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
      {
        CInterface::DebugOutput_Interface(riid, (SIZE_T)GetInterfaceVTable(*((IUnknown**)ppvObject)), NULL, this,
                                          "SKIPPED(1)");
      }
    }
  }
  return hRes;
}

HRESULT CClassFactory::OnIClassFactory2_CreateInstance(__in IClassFactory2 *lpThis, __in IUnknown *pUnkOuter,
                                                       __in REFIID riid, __deref_out void **ppvObject)
{
  CInterface *lpInterface;
  IUnknown *lpUnkTLS;
  HRESULT hRes, hRes2;

  hRes = ((lpfnIClassFactory2_CreateInstance)(sHkNfo_IClassFactoryS[1].lpCallOriginal))(lpThis,
                                                     pUnkOuter, riid, ppvObject);
  if (SUCCEEDED(hRes) && ppvObject != NULL && (*ppvObject) != NULL && lpVTable == GetInterfaceVTable(lpThis))
  {
    lpUnkTLS = (IUnknown*)::TlsGetValue(lpMgr->GetTlsIndex());
    if (lpUnkTLS == NULL || lpUnkTLS != (IUnknown*)-1)
    {
      CAutoUnkown cAutoUnk(*((IUnknown**)ppvObject), lpMgr->GetTlsIndex());

      hRes2 = lpMgr->FindOrCreateInterface(this, cAutoUnk.GetUnk(), riid, &lpInterface);
      if (SUCCEEDED(hRes2))
        hRes2 = lpMgr->AddInstance(lpInterface, riid, cAutoUnk.GetUnk());
      //inform manager if a desired interface was created
      if (SUCCEEDED(hRes2))
        lpMgr->OnInterfaceCreated(lpInterface, riid, *((IUnknown**)ppvObject));
    }
    else
    {
      if ((lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
      {
        CInterface::DebugOutput_Interface(riid, (SIZE_T)GetInterfaceVTable(*((IUnknown**)ppvObject)), NULL, this,
                                          "SKIPPED(2)");
      }
    }
  }
  return hRes;
}

HRESULT CClassFactory::OnIClassFactory2_CreateInstanceLic(__in IClassFactory2 *lpThis, __in IUnknown *pUnkOuter,
                                                          __in IUnknown *pUnkReserved, __in REFIID riid,
                                                          __in BSTR bstrKey, __deref_out PVOID *ppvObject)
{
  CInterface *lpInterface;
  IUnknown *lpUnkTLS;
  HRESULT hRes, hRes2;

  hRes = ((lpfnIClassFactory2_CreateInstanceLic)(sHkNfo_IClassFactoryS[2].lpCallOriginal))(lpThis,
                                                        pUnkOuter, pUnkReserved, riid, bstrKey, ppvObject);
  if (SUCCEEDED(hRes) && ppvObject != NULL && (*ppvObject) != NULL && lpVTable == GetInterfaceVTable(lpThis))
  {
    lpUnkTLS = (IUnknown*)::TlsGetValue(lpMgr->GetTlsIndex());
    if (lpUnkTLS == NULL || lpUnkTLS != (IUnknown*)-1)
    {
      CAutoUnkown cAutoUnk(*((IUnknown**)ppvObject), lpMgr->GetTlsIndex());

      hRes2 = lpMgr->FindOrCreateInterface(this, cAutoUnk.GetUnk(), riid, &lpInterface);
      if (SUCCEEDED(hRes2))
        hRes2 = lpMgr->AddInstance(lpInterface, riid, cAutoUnk.GetUnk());
      //inform manager if a desired interface was created
      if (SUCCEEDED(hRes2))
        lpMgr->OnInterfaceCreated(lpInterface, riid, *((IUnknown**)ppvObject));
    }
    else
    {
      if ((lpMgr->nHookFlags & MainManager::flgDebugPrintInterfaces) != 0)
      {
        CInterface::DebugOutput_Interface(riid, (SIZE_T)GetInterfaceVTable(*((IUnknown**)ppvObject)), NULL, this,
                                          "SKIPPED(3)");
      }
    }
  }
  return hRes;
}

VOID CClassFactory::DebugOutput_ClassFactory(__in REFCLSID rclsid, __in LPCSTR szSuffixA)
{
  LPWSTR sW;

  Nektra::DebugPrintMultiStart();
  //----
  sW = GetInterfaceName(rclsid, L"CLSID");
  NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM: CLSID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} [%S] %s.",
                   rclsid.Data1, rclsid.Data2, rclsid.Data3, rclsid.Data4[0], rclsid.Data4[1], rclsid.Data4[2],
                   rclsid.Data4[3], rclsid.Data4[4], rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7],
                   (sW != NULL) ? sW : L"", szSuffixA));
  if (sW != NULL)
    ::CoTaskMemFree(sW);
  //----
  Nektra::DebugPrintSeparator();
  Nektra::DebugPrintMultiEnd();
  return;
}

HRESULT __stdcall CClassFactory::IClassFactory_CreateInstanceStub(__in CClassFactory *lpCtx,
                                               __in IClassFactory *lpThis, __in IUnknown *pUnkOuter, __in REFIID riid,
                                               __deref_out void **ppvObject)
{
  HRESULT hRes;

  lpCtx->lpMgr->lpCallback->OnComHookUsage(TRUE);
  hRes = lpCtx->OnIClassFactory_CreateInstance(lpThis, pUnkOuter, riid, ppvObject);
  lpCtx->lpMgr->lpCallback->OnComHookUsage(FALSE);
  return hRes;
}

HRESULT __stdcall CClassFactory::IClassFactory2_CreateInstanceStub(__in CClassFactory *lpCtx,
                                                __in IClassFactory2 *lpThis, __in IUnknown *pUnkOuter,
                                                __in REFIID riid, __deref_out void **ppvObject)
{
  HRESULT hRes;

  lpCtx->lpMgr->lpCallback->OnComHookUsage(TRUE);
  hRes = lpCtx->OnIClassFactory2_CreateInstance(lpThis, pUnkOuter, riid, ppvObject);
  lpCtx->lpMgr->lpCallback->OnComHookUsage(FALSE);
  return hRes;
}

HRESULT __stdcall CClassFactory::IClassFactory2_CreateInstanceLicStub(__in CClassFactory *lpCtx,
                                                __in IClassFactory2 *lpThis, __in IUnknown *pUnkOuter,
                                                __in IUnknown *pUnkReserved, __in REFIID riid, __in BSTR bstrKey,
                                                __deref_out PVOID *ppvObject)
{
  HRESULT hRes;

  lpCtx->lpMgr->lpCallback->OnComHookUsage(TRUE);
  hRes = lpCtx->OnIClassFactory2_CreateInstanceLic(lpThis, pUnkOuter, pUnkReserved, riid, bstrKey, ppvObject);
  lpCtx->lpMgr->lpCallback->OnComHookUsage(FALSE);
  return hRes;
}

} //COM
