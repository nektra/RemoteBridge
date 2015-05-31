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
#include "ComInterface.h"
#include "ComManager.h"
#include "..\HookTrampoline.h"
#include "..\..\Common\MallocMacros.h"
#include "..\..\Common\AutoPtr.h"
#include "ComHelpers.h"
#include <ExDisp.h>

#define X_BYTE_ENC(x, y) (x)
#if defined _M_IX86
  static
  #include "Asm\IUnknown_QueryInterface_x86.inl"
  static
  #include "Asm\IUnknown_AddRef_x86.inl"
  static
  #include "Asm\IUnknown_Release_x86.inl"
  static
  #include "Asm\IUnknown_Generic_x86.inl"
#elif defined _M_X64
  static
  #include "Asm\IUnknown_QueryInterface_x64.inl"
  static
  #include "Asm\IUnknown_AddRef_x64.inl"
  static
  #include "Asm\IUnknown_Release_x64.inl"
  static
  #include "Asm\IUnknown_Generic_x64.inl"
#else
  #error Platform not supported
#endif

//-----------------------------------------------------------

typedef HRESULT (__stdcall *lpfnIUnknown_QueryInterface)(__in IUnknown *lpThis, __in REFIID riid,
                                                         __out void **ppvObject);
typedef ULONG (__stdcall *lpfnIUnknown_AddRef)(__in IUnknown *lpThis);
typedef ULONG (__stdcall *lpfnIUnknown_Release)(__in IUnknown *lpThis);

//-----------------------------------------------------------

namespace COM {

CInterface::CInterface(__in CMainManager *_lpMgr, __in CDll *_lpDll, __in SIZE_T *_lpVTable,
                       __in_opt CClassFactory *_lpClassFac) : TNktLnkLstNode<CInterface>(), CNktFastMutex()
{
  lpMgr = _lpMgr;
  lpDll = _lpDll;
  lpVTable = _lpVTable;
  lpClassFac = _lpClassFac;
  nFlags = 0;
  memset(sHkNfo_IUnk, 0, sizeof(sHkNfo_IUnk));
  return;
}

CInterface::~CInterface()
{
  //unhooking in destructor may lead to a deadlock because critical sections taken by heap manager
  _ASSERT(sHkNfo_IUnk[0].nHookId == 0 && sHkNfo_IUnk[1].nHookId == 0 && sHkNfo_IUnk[2].nHookId == 0);
  return;
}

HRESULT CInterface::Initialize(__in IUnknown *lpUnk)
{
  SIZE_T k;
  HRESULT hRes;

  _ASSERT(lpUnk != NULL);
  //----
  {
    CAutoTls cAutoTls(lpMgr->GetTlsIndex(), (LPVOID)-1); //disable extra release handling

    if (IsInterfaceDerivedFrom(lpUnk, IID_IDispatch) != FALSE)
      nFlags |= IID_IS_IDispatch;
    if (IsInterfaceDerivedFrom(lpUnk, IID_IOleWindow) != FALSE)
      nFlags |= IID_IS_IOleWindow;
    if (IsInterfaceDerivedFrom(lpUnk, IID_IWebBrowser2) != FALSE)
      nFlags |= IID_IS_IWebBrowser2;
  }
  //----
  for (k=0; k<3; k++)
    sHkNfo_IUnk[k].lpProcToHook = (LPVOID)(lpVTable[k]);
  sHkNfo_IUnk[0].lpNewProcAddr = HOOKTRAMPOLINECREATE(lpMgr->GetHeap(), aIUnknownQueryInterface,
                                                      &CInterface::IUnknown_QueryInterfaceStub);
  sHkNfo_IUnk[1].lpNewProcAddr = HOOKTRAMPOLINECREATE(lpMgr->GetHeap(), aIUnknownAddRef,
                                                      &CInterface::IUnknown_AddRefStub);
  sHkNfo_IUnk[2].lpNewProcAddr = HOOKTRAMPOLINECREATE(lpMgr->GetHeap(), aIUnknownRelease,
                                                      &CInterface::IUnknown_ReleaseStub);
  if (sHkNfo_IUnk[0].lpNewProcAddr == NULL || sHkNfo_IUnk[1].lpNewProcAddr == NULL ||
      sHkNfo_IUnk[2].lpNewProcAddr == NULL)
  {
    hRes = E_OUTOFMEMORY;
  }
  else
  {
    hRes = NKT_HRESULT_FROM_WIN32(lpMgr->GetHookMgr()->Hook(sHkNfo_IUnk, 1));
  }
  if (FAILED(hRes))
  {
    for (k=0; k<3; k++)
    {
      HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_IUnk[k].lpNewProcAddr);
      sHkNfo_IUnk[k].lpNewProcAddr = NULL;
    }
    return hRes;
  }
  {
    CNktAutoFastMutex cLock(&(lpMgr->cIFaceMethodsListMtx));
    TNktLnkLst<CInterfaceMethod>::Iterator it;
    CInterfaceMethod *lpInterfaceMethod;

    for (lpInterfaceMethod=it.Begin(lpMgr->cIFaceMethodsList); lpInterfaceMethod!=NULL && SUCCEEDED(hRes);
         lpInterfaceMethod=it.Next())
    {
      hRes = OnNewInspectInterfaceMethod(lpInterfaceMethod);
    }
  }
  return hRes;
}

VOID CInterface::Finalize()
{
  SIZE_T i;

  lpMgr->RemoveInstance(this);
  //----
  lpMgr->GetHookMgr()->Unhook(sHkNfo_IUnk, 3);
  //----
  for (i=0; i<3; i++)
    HookTrampoline_Delete(lpMgr->GetHeap(), sHkNfo_IUnk[i].lpNewProcAddr);
  memset(sHkNfo_IUnk, 0, sizeof(sHkNfo_IUnk));
  //----
  {
    CNktAutoFastMutex cLock(&cHookedMethodsListMtx);
    CHookedMethod *lpHookedMethod;

    while ((lpHookedMethod = cHookedMethodsList.PopTail()) != NULL)
    {
      lpMgr->GetHookMgr()->Unhook(&(lpHookedMethod->sHkNfo_IUnkMethod), 1);
      HookTrampoline_Delete(lpMgr->GetHeap(), lpHookedMethod->sHkNfo_IUnkMethod.lpNewProcAddr);
      delete lpHookedMethod;
    }
  }
  return;
}

HRESULT CInterface::AddSupportedIid(__in REFIID riid)
{
  return cSupportedIids.Add(riid);
}

HRESULT CInterface::GetSupportedIiAt(__in SIZE_T nIndex, __out GUID &sGuid)
{
  return cSupportedIids.GetAt(nIndex, sGuid);
}

HRESULT CInterface::OnIUnknown_QueryInterface(__in IUnknown *lpThis, __in REFIID riid,
                                                               __out void **ppvObject)
{
  CInterface *lpInterface;
  IUnknown *lpUnkTLS;
  HRESULT hRes, hRes2;

  hRes = ((lpfnIUnknown_QueryInterface)(sHkNfo_IUnk[0].lpCallOriginal))(lpThis, riid, ppvObject);
  if (SUCCEEDED(hRes) && ppvObject != NULL && (*ppvObject) != NULL &&
      memcmp(&riid, &IID_IUnknown, sizeof(IID)) != 0)
  {
    lpUnkTLS = (IUnknown*)::TlsGetValue(lpMgr->GetTlsIndex());
    if (lpUnkTLS == NULL || (lpUnkTLS != lpThis && lpUnkTLS != (IUnknown*)-1))
    {
      CAutoUnkown cAutoUnk(*((IUnknown**)ppvObject), lpMgr->GetTlsIndex());

      hRes2 = lpMgr->FindOrCreateInterface(lpClassFac, cAutoUnk.GetUnk(), riid, &lpInterface);
      if (SUCCEEDED(hRes2))
        hRes2 = lpMgr->AddInstance(lpInterface, riid, cAutoUnk.GetUnk());
    }
  }
  return hRes;
}

ULONG CInterface::OnIUnknown_AddRef(__in IUnknown *lpThis)
{
  ULONG nRef;

  nRef = ((lpfnIUnknown_AddRef)(sHkNfo_IUnk[1].lpCallOriginal))(lpThis);
  return nRef;
}

ULONG CInterface::OnIUnknown_Release(__in IUnknown *lpThis)
{
  ULONG nRef;

  nRef = ((lpfnIUnknown_Release)(sHkNfo_IUnk[2].lpCallOriginal))(lpThis);
  /*
  IUnknown *lpUnkTLS;
  ULONG nRef;

  lpUnkTLS = (IUnknown*)::TlsGetValue(lpMgr->GetTlsIndex());
  lpUnkTLS = (IUnknown*)-1;
  if (lpUnkTLS == NULL || (lpUnkTLS != lpThis && lpUnkTLS != (IUnknown*)-1))
  {
    /*
    nRef = ((lpfnIUnknown_AddRef)(sHkNfo_IUnk[1].lpCallOriginal))(lpThis);
    //The following "nRef == 3" comes from the following logic:
    //  Live COM object initally has a count of one
    //  Plus CoMarshalInterface = 2
    //  Plus above lpfnIUnknown_AddRef = 3
    //If the count is 3 and we are release, we are destroying the object
    nRef = ((lpfnIUnknown_Release)(sHkNfo_IUnk[2].lpCallOriginal))(lpThis);
    if (nRef <= 2)
    {
      CAutoUnkown cAutoUnk(lpThis, lpMgr->GetTlsIndex());

      lpMgr->RemoveInterface(cAutoUnk.GetUnk());
    }
    * /
    nRef = ((lpfnIUnknown_Release)(sHkNfo_IUnk[2].lpCallOriginal))(lpThis);
    if (nRef == 1)
    {
      CAutoUnkown cAutoUnk(lpThis, lpMgr->GetTlsIndex());
      ULONG nRef2;

      //if (lpMgr->RemoveInterface(cAutoUnk.GetUnk()) == S_OK)
      //  nRef = 0;
      lpMgr->RemoveInterface(cAutoUnk.GetUnk());
      nRef2 = cAutoUnk.Release();
      if (nRef2 != ULONG_MAX)
        nRef = nRef2;
    }
  }
  else
  {
    nRef = ((lpfnIUnknown_Release)(sHkNfo_IUnk[2].lpCallOriginal))(lpThis);
  }
  */
  return nRef;
}

HRESULT CInterface::OnIUnknown_Generic(__in CInterfaceMethod *lpInterfaceMethod,
                                                        __in SIZE_T *lpParameters, __in SIZE_T nReturnValue)
{
  CInterface *lpInterface;
  IUnknown *lpUnk, *lpUnkTLS;
  IID sIid;
  HRESULT hRes;

  lpUnkTLS = (IUnknown*)::TlsGetValue(lpMgr->GetTlsIndex());
  if (lpUnkTLS == NULL || lpUnkTLS != (IUnknown*)-1)
  {
    //get the returned interface
    if (lpInterfaceMethod->nReturnParameterIndex == ULONG_MAX)
    {
      if (nReturnValue == 0)
        return S_OK; //no interface was returned (almost sure it is an error in original request)
      lpUnk = (IUnknown*)nReturnValue;
    }
    else
    {
      //check returned HRESULT
      if (((ULONG)nReturnValue & 0x80000000) != 0)
        return S_OK; //error
      if (lpParameters[lpInterfaceMethod->nReturnParameterIndex] == 0)
        return S_OK; //target LPVOID* was null
      try
      {
        lpUnk = *((IUnknown **)(lpParameters[lpInterfaceMethod->nReturnParameterIndex]));
      }
      catch (...)
      {
        return E_ACCESSDENIED;
      }
      if (lpUnk == NULL)
        return S_OK; //no interface was returned (almost sure it is an error in original request)
    }
    if (lpUnkTLS == NULL || (lpUnkTLS != lpUnk && lpUnkTLS != (IUnknown*)-1))
    {
      //get the IID
      if (lpInterfaceMethod->nIidIndex == ULONG_MAX)
      {
        sIid = lpInterfaceMethod->sIid;
      }
      else
      {
        try
        {
          sIid = *((IID*)(lpParameters[lpInterfaceMethod->nIidIndex]));
        }
        catch (...)
        {
          return E_ACCESSDENIED;
        }
      }
      {
        CAutoUnkown cAutoUnk(lpUnk, lpMgr->GetTlsIndex());

        hRes = lpMgr->FindOrCreateInterface(lpClassFac, cAutoUnk.GetUnk(), sIid, &lpInterface);
        if (SUCCEEDED(hRes))
          hRes = lpMgr->AddInstance(lpInterface, sIid, cAutoUnk.GetUnk());
        //inform manager if a desired interface was created
        if (SUCCEEDED(hRes))
          lpMgr->OnInterfaceCreated(lpInterface, sIid, lpUnk);
      }
    }
  }
  return S_OK;
}

HRESULT CInterface::OnNewInspectInterfaceMethod(__in CInterfaceMethod *lpInterfaceMethod)
{
  CNktAutoFastMutex cLock(&cHookedMethodsListMtx);
  TNktLnkLst<CHookedMethod>::Iterator it;
  TNktAutoDeletePtr<CHookedMethod> cNewHookedMethodItem;
  CHookedMethod *lpHookedMethod;
  LPBYTE lpCode, lpTramp, lpCallOrig;
  SIZE_T i, nCodeSize;
  HRESULT hRes;

  if (cSupportedIids.Check(lpInterfaceMethod->sIid) == FALSE)
    return S_OK;
  for (lpHookedMethod=it.Begin(cHookedMethodsList); lpHookedMethod!=NULL; lpHookedMethod=it.Next())
  {
    if (lpHookedMethod->lpInterfaceMethod == lpInterfaceMethod)
      return S_OK;
  }
  cNewHookedMethodItem.Attach(new CHookedMethod);
  if (cNewHookedMethodItem == NULL)
    return E_OUTOFMEMORY;
  cNewHookedMethodItem->lpInterfaceMethod = lpInterfaceMethod;
  cNewHookedMethodItem->sHkNfo_IUnkMethod.lpProcToHook = (LPVOID)(lpVTable[lpInterfaceMethod->nMethodIndex]);
#if defined _M_IX86
  lpCode = aIUnknownGenericX86;
  nCodeSize = sizeof(aIUnknownGenericX86);
#elif defined _M_X64
  lpCode = aIUnknownGenericX64;
  nCodeSize = sizeof(aIUnknownGenericX64);
#endif
  lpTramp = (LPBYTE)::HeapAlloc(lpMgr->GetHeap(), 0, nCodeSize);
  if (lpTramp == NULL)
    return E_OUTOFMEMORY;
  memcpy(lpTramp, lpCode, nCodeSize);
#if defined _M_IX86
  for (i=0; i<nCodeSize-4; i++)
  {
    switch (*((DWORD*)(lpTramp+i)))
    {
      case 0xDEAD0001:
        *((DWORD*)(lpTramp+i)) = (DWORD)this;
        break;
      case 0xDEAD0002:
        *((DWORD*)(lpTramp+i)) = (DWORD)lpInterfaceMethod;
        break;
      case 0xDEAD0003:
        *((DWORD*)(lpTramp+i)) = (DWORD)&CInterface::IUnknown_GenericStub;
        break;
      case 0xDEAD0010:
        lpCallOrig = lpTramp+i;
        *((DWORD*)(lpTramp+i)) = 0;
        break;
    }
  }
#elif defined _M_X64
  for (i=0; i<nCodeSize-8; i++)
  {
    switch (*((ULONGLONG __unaligned *)(lpTramp+i)))
    {
      case 0xDEAD0001DEAD0001:
        *((ULONGLONG __unaligned *)(lpTramp+i)) = (ULONGLONG)this;
        break;
      case 0xDEAD0002DEAD0002:
        *((ULONGLONG __unaligned *)(lpTramp+i)) = (ULONGLONG)lpInterfaceMethod;
        break;
      case 0xDEAD0003DEAD0003:
        *((ULONGLONG __unaligned *)(lpTramp+i)) = (ULONGLONG)&CInterface::IUnknown_GenericStub;
        break;
      case 0xDEAD0010DEAD0010:
        lpCallOrig = lpTramp+i;
        *((ULONGLONG __unaligned *)(lpTramp+i)) = 0;
        break;
    }
  }
#endif
  cNewHookedMethodItem->sHkNfo_IUnkMethod.lpNewProcAddr = lpTramp;
  hRes = NKT_HRESULT_FROM_WIN32(lpMgr->GetHookMgr()->Hook(&(cNewHookedMethodItem->sHkNfo_IUnkMethod), 1));
  if (SUCCEEDED(hRes))
  {
#if defined _M_IX86
    *((DWORD*)lpCallOrig) = (DWORD)(cNewHookedMethodItem->sHkNfo_IUnkMethod.lpCallOriginal);
#elif defined _M_X64
    *((ULONGLONG __unaligned *)lpCallOrig) = (ULONGLONG)(cNewHookedMethodItem->sHkNfo_IUnkMethod.lpCallOriginal);
#endif
    ::FlushInstructionCache(::GetCurrentProcess(), lpCallOrig-1, 1+sizeof(SIZE_T));
    cHookedMethodsList.PushTail(cNewHookedMethodItem.Detach());
  }
  else
  {
    HookTrampoline_Delete(lpMgr->GetHeap(), lpTramp);
  }
  return hRes;
}

VOID CInterface::DebugOutput_Interface(__in REFIID riid, __in SIZE_T lpVTable, __in CInterface *lpInterface,
                                       __in CClassFactory *lpClassFac, __in_z LPCSTR szSuffixA)
{
  LPWSTR sW;
  GUID sClsId;
  SIZE_T nIndex;

  Nektra::DebugPrintMultiStart();
  //----
  sW = GetInterfaceName(riid, L"Interface");
  NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM: IID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} [%S] %s.",
                   riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2],
                   riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7],
                   (sW != NULL) ? sW : L"", szSuffixA));
  if (sW != NULL)
    ::CoTaskMemFree(sW);
  if (lpInterface != NULL)
  {
    NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM:   VTable: 0x%IX / Dll offset: 0x%IX.", lpVTable,
                     lpVTable - (SIZE_T)(lpInterface->GetDll()->GetHInstance())));
  }
  else
  {
    NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM:   VTable: 0x%IX.", lpVTable));
  }
  if (lpClassFac != NULL)
  {
    for (nIndex=0; ; nIndex++)
    {
      if (lpClassFac->GetSupportedClsIdAt(nIndex, sClsId) == FALSE)
        break;
      sW = GetInterfaceName(sClsId, L"CLSID");
      NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM:   %s {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} "
                                        "[%S]", (nIndex == 0) ? "ClassFactory" : "            ",
                       sClsId.Data1, sClsId.Data2, sClsId.Data3, sClsId.Data4[0], sClsId.Data4[1],
                       sClsId.Data4[2], sClsId.Data4[3], sClsId.Data4[4], sClsId.Data4[5], sClsId.Data4[6],
                       sClsId.Data4[7], (sW != NULL) ? sW : L""));
      if (sW != NULL)
        ::CoTaskMemFree(sW);
    }
  }
  //----
  Nektra::DebugPrintSeparator();
  Nektra::DebugPrintMultiEnd();
  return;
}

VOID CInterface::DebugOutput_InterfaceCreated(__in REFIID riid, __in CInterface *lpInterface, __in SIZE_T lpVTable)
{
  LPWSTR sW;
  GUID sClsId;
  SIZE_T nIndex;

  Nektra::DebugPrintMultiStart();
  //----
  sW = COM::GetInterfaceName(riid, L"Interface");
  NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM: IID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} [%S] created.",
                   riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
                   riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], (sW != NULL) ? sW : L""));
  if (sW != NULL)
    ::CoTaskMemFree(sW);
  if (lpInterface != NULL)
  {
    NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM:   VTable: 0x%IX / Dll offset: 0x%IX.", lpVTable,
                     lpVTable - (SIZE_T)(lpInterface->GetDll()->GetHInstance())));
  }
  else
  {
    NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM:   VTable: 0x%IX.", lpVTable));
  }
  if (lpInterface->GetClassFactory() != NULL)
  {
    for (nIndex=0; ; nIndex++)
    {
      if (lpInterface->GetClassFactory()->GetSupportedClsIdAt(nIndex, sClsId) == FALSE)
        break;
      sW = COM::GetInterfaceName(sClsId, L"CLSID");
      NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteCOM:   %s {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} "
                                        "[%S]", (nIndex == 0) ? "ClassFactory" : "            ",
                       sClsId.Data1, sClsId.Data2, sClsId.Data3, sClsId.Data4[0], sClsId.Data4[1],
                       sClsId.Data4[2], sClsId.Data4[3], sClsId.Data4[4], sClsId.Data4[5], sClsId.Data4[6],
                       sClsId.Data4[7], (sW != NULL) ? sW : L""));
      if (sW != NULL)
        ::CoTaskMemFree(sW);
    }
  }
  //----
  Nektra::DebugPrintSeparator();
  Nektra::DebugPrintMultiEnd();
  return;
}

HRESULT __stdcall CInterface::IUnknown_QueryInterfaceStub(__in CInterface *lpCtx, __in IUnknown *lpThis,
                                                     __in REFIID riid, __deref_out void **ppvObject)
{
  HRESULT hRes;

  lpCtx->lpMgr->lpCallback->OnComHookUsage(TRUE);
  hRes = lpCtx->OnIUnknown_QueryInterface(lpThis, riid, ppvObject);
  lpCtx->lpMgr->lpCallback->OnComHookUsage(FALSE);
  return hRes;
}

ULONG __stdcall CInterface::IUnknown_AddRefStub(__in CInterface *lpCtx, __in IUnknown *lpThis)
{
  ULONG nRes;

  lpCtx->lpMgr->lpCallback->OnComHookUsage(TRUE);
  nRes = lpCtx->OnIUnknown_AddRef(lpThis);
  lpCtx->lpMgr->lpCallback->OnComHookUsage(FALSE);
  return nRes;
}

ULONG __stdcall CInterface::IUnknown_ReleaseStub(__in CInterface *lpCtx, __in IUnknown *lpThis)
{
  ULONG nRes;

  lpCtx->lpMgr->lpCallback->OnComHookUsage(TRUE);
  nRes = lpCtx->OnIUnknown_Release(lpThis);
  lpCtx->lpMgr->lpCallback->OnComHookUsage(FALSE);
  return nRes;
}

HRESULT __stdcall CInterface::IUnknown_GenericStub(__in CInterface *lpCtx, __in CInterfaceMethod *lpInterfaceMethod,
                                                   __in SIZE_T *lpParameters, __in SIZE_T nReturnValue)
{
  HRESULT hRes;

  lpCtx->lpMgr->lpCallback->OnComHookUsage(TRUE);
  hRes = lpCtx->OnIUnknown_Generic(lpInterfaceMethod, lpParameters, nReturnValue);
  lpCtx->lpMgr->lpCallback->OnComHookUsage(FALSE);
  return hRes;
}

} //COM
