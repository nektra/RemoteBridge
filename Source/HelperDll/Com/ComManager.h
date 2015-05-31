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
#include <Ole2.h>
#include <OCIdl.h>
#include <OleCtl.h>
#include "..\..\Common\Threads.h"
#include "..\..\Common\ComPtr.h"
#include "ComDll.h"
#include "ComClassFactory.h"
#include "ComInterface.h"
#include "ComInstance.h"
#include "ComClsIdIidList.h"

//-----------------------------------------------------------

#define IIDITEM_BIN_COUNT                                 47
#define CLASSFAC_BIN_COUNT                                17
#define INTERFACE_BIN_COUNT                               47

typedef BOOL (__stdcall *lpfnDllMain)(__in HINSTANCE hInstance, __in DWORD dwReason, LPVOID __in lpReserved);
typedef VOID (__stdcall *lpfnSHWeakReleaseInterface)(__in IUnknown *lpDest, __inout IUnknown **lppUnknown);
typedef VOID (__stdcall *lpfnCoUninitialize)(void);
typedef HRESULT (__stdcall *lpfnCoInitializeSecurity)(__in_opt PSECURITY_DESCRIPTOR pSecDesc, __in LONG cAuthSvc,
                                                      __in_ecount_opt(cAuthSvc) SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
                                                      __in_opt void *pReserved1, __in DWORD dwAuthnLevel,
                                                      __in DWORD dwImpLevel, __in_opt void *pAuthList,
                                                      __in DWORD dwCapabilities, __in_opt void *pReserved3);

//-----------------------------------------------------------

namespace COM {

class CMainManager : public CNktThread
{
public:
  class CCallbacks
  {
  public:
    virtual VOID OnComInterfaceCreated(__in REFCLSID sClsId, __in REFIID sIid, __in IUnknown *lpUnk) = 0;
    virtual LPBYTE OnComAllocIpcBuffer(__in SIZE_T nSize) = 0;
    virtual VOID OnComFreeIpcBuffer(__in LPVOID lpBuffer) = 0;
    virtual HRESULT OnComBuildSimpleReplyData(__in HRESULT hRes, __out LPVOID *lplpReplyData,
                                              __out SIZE_T *lpnReplyDataSize) = 0;
    virtual VOID OnComHookUsage(__in BOOL bEntering) = 0;
  };

public:
  class CThreadEventBase : public TNktLnkLstNode<CThreadEventBase>, public CBaseRefCounterNoLock
  {
  public:
    typedef enum {
      codeAddInstance=1, codeRemoveInstance, codeGetInterfaceFromHwnd, codeGetInterfaceFromIndex
    } eCode;

    CThreadEventBase(eCode _nCode, __in BOOL bPost=FALSE) : TNktLnkLstNode<CThreadEventBase>(), CBaseRefCounterNoLock()
      {
      if (bPost == FALSE)
        cCompletedEv.Create(TRUE, FALSE);
      InterlockedExchange(&nCode, (LONG)_nCode);
      InterlockedExchange(&hRes, E_FAIL);
      return;
      };

  public:
    LONG volatile nCode;
    CNktEvent cCompletedEv;
    LONG volatile hRes;
  };

  class CThreadEvent_AddInstance : public CThreadEventBase
  {
  public:
    CThreadEvent_AddInstance() : CThreadEventBase(codeAddInstance, TRUE)
      {
      return;
      };

  public:
    TNktComPtr<CInstance> cInstance;
  };

  class CThreadEvent_RemoveInstance : public CThreadEventBase
  {
  public:
    CThreadEvent_RemoveInstance(__in IUnknown *_lpUnk) : CThreadEventBase(codeRemoveInstance, FALSE)
      {
      lpUnk = _lpUnk;
      return;
      };

  public:
    IUnknown * lpUnk;
  };

  class CThreadEvent_GetInterfaceFromNNN : public CThreadEventBase
  {
  public:
    CThreadEvent_GetInterfaceFromNNN(__in HWND _hWnd) : CThreadEventBase(codeGetInterfaceFromHwnd, FALSE)
      {
      hWnd = _hWnd;
      lpData = NULL;
      nDataSize = 0;
      return;
      };
    CThreadEvent_GetInterfaceFromNNN(__in ULONG _nIndex) : CThreadEventBase(codeGetInterfaceFromIndex, FALSE)
      {
      nIndex = _nIndex;
      lpData = NULL;
      nDataSize = 0;
      return;
      };

  public:
    union {
      HWND hWnd;
      ULONG nIndex;
    };
    IID sIid;
    TMSG_SIMPLE_REPLY *lpData;
    SIZE_T nDataSize;
  };

  //----

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

  HRESULT FindOrCreateInterface(__in_opt CClassFactory *lpClassFac, __in IUnknown *lpUnk, __in REFIID riid,
                                __out CInterface **lplpItem);

  HRESULT WatchInterface(__in TMSG_WATCHCOMINTERFACE *lpMsg, __in SIZE_T nDataSize,
                         __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);
  HRESULT UnwatchInterface(__in TMSG_UNWATCHCOMINTERFACE *lpMsg, __in SIZE_T nDataSize,
                           __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);
  HRESULT UnwatchAllInterfaces(__out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

  HRESULT InspectInterfaceMethod(__in TMSG_INSPECTCOMINTERFACEMETHOD *lpMsg, __in SIZE_T nDataSize,
                                 __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

  HRESULT GetInterfaceFromHWnd(__in TMSG_GETCOMINTERFACEFROMHWND *lpMsg, __in SIZE_T nDataSize,
                               __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);
  HRESULT GetInterfaceFromIndex(__in TMSG_GETCOMINTERFACEFROMINDEX *lpMsg, __in SIZE_T nDataSize,
                                __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

  HRESULT BuildInterprocMarshalPacket(__in IUnknown *lpUnk, __in REFIID sIid, __in SIZE_T nPrefixBytes,
                                      __out LPVOID *lplpData, __out SIZE_T *lpnDataSize);

private:
  friend class CDll;
  friend class CClassFactory;
  friend class CInterface;
  friend class CInstance;

  HRESULT GetInterfaceFromHWnd_Worker(__in HWND hWnd, __in REFIID sIid, __out TMSG_SIMPLE_REPLY **lplpData,
                                      __out SIZE_T *lpnDataSize);
  HRESULT GetInterfaceFromIndex_Worker(__in REFIID sIid, __in ULONG nInterfaceIndex,
                                       __out TMSG_SIMPLE_REPLY **lplpData, __out SIZE_T *lpnDataSize);

  static VOID __stdcall SHWeakReleaseInterfaceStub(__in COM::CMainManager *lpCtx, __in IUnknown *lpDest,
                                                   __in IUnknown **lppUnknown);
  static VOID __stdcall CoUninitializeStub(__in COM::CMainManager *lpCtx);
  static HRESULT __stdcall CoInitializeSecurityStub(__in COM::CMainManager *lpCtx,
                                 __in_opt PSECURITY_DESCRIPTOR pSecDesc, __in LONG cAuthSvc,
                                 __in_ecount_opt(cAuthSvc) SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
                                 __in_opt void *pReserved1, __in DWORD dwAuthnLevel, __in DWORD dwImpLevel,
                                 __in_opt void *pAuthList, __in DWORD dwCapabilities, __in_opt void *pReserved3,
                                 __in SIZE_T nRetAddress);
  static VOID CALLBACK RemoveInvalidApcProc(__in ULONG_PTR);

  HRESULT AddInstance(__in CInterface *lpInterface, __in REFIID sIid, __in IUnknown *lpUnkRef);
  HRESULT RemoveInstance(__in IUnknown *lpUnk);
  VOID RemoveInstance(__in CInterface *lpInterface);
  //VOID RemoveInstance(__in IID &sIid);
  VOID RemoveAllInstances(__in BOOL bInDestructor);
  VOID RemoveInvalidInstances();
  VOID RemoveDeleteAsyncInstances(__in BOOL bInDestructor);

  CInstance* FindInstance(__in IUnknown *lpUnk);

  BOOL IsInterfaceWatched(__in CClassFactory *lpClassFac, __in REFIID sIid, __out_opt CLSID *lpClsId=NULL);

  VOID OnInterfaceCreated(__in CInterface *lpInterface, __in REFIID sIid, __in IUnknown *lpUnk);

  VOID OnSHWeakReleaseInterface(IUnknown *lpDest, IUnknown **lppUnknown);
  VOID OnCoUninitialize();
  HRESULT OnCoInitializeSecurity(__in_opt PSECURITY_DESCRIPTOR pSecDesc, __in LONG cAuthSvc,
                                 __in_ecount_opt(cAuthSvc) SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
                                 __in_opt void *pReserved1, __in DWORD dwAuthnLevel, __in DWORD dwImpLevel,
                                 __in_opt void *pAuthList, __in DWORD dwCapabilities, __in_opt void *pReserved3,
                                 __in SIZE_T nRetAddress);

  VOID OnRemoveInvalidInterfacesApcProc(__in DWORD dwTid);

  HRESULT ExecuteThreadEvent(__in CThreadEventBase *lpEvent, __in BOOL bIsPriority);
  VOID ThreadProc();

private:
  CCallbacks *lpCallback;
  CNktHookLib *lpHookMgr;
  HANDLE hHeap;
  DWORD dwTlsIndex;
  //----
  struct {
    LONG volatile nDllMutex;
    HINSTANCE hDll;
    SIZE_T nDllSize;
    lpfnDllMain fnDllMain;
    lpfnCoUninitialize fnCoUninitialize;
    lpfnCoInitializeSecurity fnCoInitializeSecurity;
    CNktHookLib::HOOK_INFO sHooks[2];
    LONG volatile nCoInitializeSecurityDefault;
    LONG volatile nCoInitializeSecurityError;
  } sOLE32;
  struct {
    LONG volatile nDllMutex;
    HINSTANCE hDll;
    lpfnSHWeakReleaseInterface fnSHWeakReleaseInterface;
    CNktHookLib::HOOK_INFO sHooks[1];
  } sSHLWAPI;
  CNktFastMutex cSHLWAPI_Mtx;
  //----
  CNktFastMutex cDllsListMtx;
  TNktLnkLst<CDll> cDllsList;
  //----
  struct {
    CNktFastMutex cMtx;
    CNktEvent cEventAvailableEv;
    TNktLnkLst<CThreadEventBase> cList[2];
  } sThreadEvents;
  struct {
    CNktFastMutex cMtx;
    TNktLnkLst<CInstance> cList;
  } sThreadAsyncDelete;
  //----
  struct {
    CNktFastMutex cMtx;
    TNktLnkLst<CInstance> cList;
  } sActiveObjects[IIDITEM_BIN_COUNT];
  //----
  CClsIdIidList cWatchInterfaces;
  //----
  CNktFastMutex cIFaceMethodsListMtx;
  TNktLnkLst<CInterfaceMethod> cIFaceMethodsList;

public:
  ULONG nHookFlags;
};

} //COM
