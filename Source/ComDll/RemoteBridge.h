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

#include "resource.h"       // main symbols
#ifdef _WIN64
  #include "RemoteBridge_i64.h"
#else //_WIN64
  #include "RemoteBridge_i.h"
#endif //_WIN64
#include "CustomRegistryMap.h"
#include "RemoteBridgeEvents.h"
#include "Clients.h"

//-----------------------------------------------------------

extern HINSTANCE hDllInst;

//-----------------------------------------------------------

// CNktRemoteBridgeImpl
class ATL_NO_VTABLE CNktRemoteBridgeImpl : public CComObjectRootEx<CComMultiThreadModel>,
                              public CComCoClass<CNktRemoteBridgeImpl, &CLSID_NktRemoteBridge>,
                              public CNktRemoteBridgeEventsImpl<CNktRemoteBridgeImpl>,
                              public IConnectionPointContainerImpl<CNktRemoteBridgeImpl>,
                              public IProvideClassInfo2Impl<&CLSID_NktRemoteBridge, &__uuidof(DNktRemoteBridgeEvents),
                                                            &LIBID_RemoteBridge, 1, 0>,
                              public IObjectSafetyImpl<CNktRemoteBridgeImpl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
                              public IDispatchImpl<INktRemoteBridge, &IID_INktRemoteBridge, &LIBID_RemoteBridge, 1, 0>,
                              private CClientMgr
{
public:
  CNktRemoteBridgeImpl() : CComObjectRootEx<CComMultiThreadModel>(),
                                CComCoClass<CNktRemoteBridgeImpl, &CLSID_NktRemoteBridge>(),
                                CNktRemoteBridgeEventsImpl<CNktRemoteBridgeImpl>(),
                                IConnectionPointContainerImpl<CNktRemoteBridgeImpl>(),
                                IProvideClassInfo2Impl<&CLSID_NktRemoteBridge, &__uuidof(DNktRemoteBridgeEvents),
                                                       &LIBID_RemoteBridge, 1, 0>(),
                                IObjectSafetyImpl<CNktRemoteBridgeImpl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>(),
                                IDispatchImpl<INktRemoteBridge, &IID_INktRemoteBridge, &LIBID_RemoteBridge, 1, 0>(),
                                CClientMgr()
    {
    return;
    };

  ~CNktRemoteBridgeImpl()
    {
    UnhookAll();
    DisconnectAllNktJavaObjects();
    return;
    };

  DECLARE_REGISTRY_RESOURCEID_EX(IDR_INTERFACEREGISTRAR, L"RemoteBridge.NktRemoteBridge",
                                 L"1", L"NktRemoteBridge Class", CLSID_NktRemoteBridge,
                                 LIBID_RemoteBridge, L"Neutral")

  BEGIN_COM_MAP(CNktRemoteBridgeImpl)
    COM_INTERFACE_ENTRY(INktRemoteBridge)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, cUnkMarshaler.p)
  END_COM_MAP()

  BEGIN_CONNECTION_POINT_MAP(CNktRemoteBridgeImpl)
    CONNECTION_POINT_ENTRY(__uuidof(DNktRemoteBridgeEvents))
  END_CONNECTION_POINT_MAP()

  // ISupportsErrorInfo
  STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  DECLARE_GET_CONTROLLING_UNKNOWN()

  HRESULT FinalConstruct()
    {
    return ::CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &(cUnkMarshaler.p));
    };

  void FinalRelease()
    {
    cUnkMarshaler.Release();
    return;
    };

public:
  STDMETHOD(Hook)(__in LONG procId, __in eNktHookFlags flags);

  STDMETHOD(Unhook)(__in LONG procId);

  STDMETHOD(UnhookAll)();

  STDMETHOD(WatchComInterface)(__in LONG procId, __in BSTR clsid, __in BSTR iid);

  STDMETHOD(UnwatchComInterface)(__in LONG procId, __in BSTR clsid, __in BSTR iid);

  STDMETHOD(UnwatchAllComInterfaces)(__in LONG procId);

  STDMETHOD(GetComInterfaceFromHwnd)(__in LONG procId, __in my_ssize_t hwnd, __in BSTR iid,
                                     __deref_out IUnknown **ppUnk);

  STDMETHOD(GetComInterfaceFromIndex)(__in LONG procId, __in BSTR iid, __in int index, __deref_out IUnknown **ppUnk);

  STDMETHOD(HookComInterfaceMethod)(__in LONG procId, __in BSTR iid, __in int methodIndex,
                                    __in VARIANT iidParamStringOrIndex, __in int returnParamIndex);

  STDMETHOD(FindWindow)(__in LONG procId, __in my_ssize_t hParentWnd, __in BSTR wndName, __in BSTR className,
                        __in VARIANT_BOOL recurseChilds, __out my_ssize_t *hWnd);

  STDMETHOD(put_WindowText)(__in LONG procId, __in my_ssize_t hWnd, __in BSTR text);

  STDMETHOD(get_WindowText)(__in LONG procId, __in my_ssize_t hWnd, __out BSTR *text);

  STDMETHOD(SendMessage)(__in LONG procId, __in my_ssize_t hWnd, __in LONG msg, __in my_ssize_t wParam,
                         __in my_ssize_t lParam, __out my_ssize_t *result);

  STDMETHOD(SendMessageTimeout)(__in LONG procId, __in my_ssize_t hWnd, __in LONG msg,
                                __in my_ssize_t wParam, __in my_ssize_t lParam, __in LONG timeout,
                                __out my_ssize_t *result);

  STDMETHOD(PostMessage)(__in LONG procId, __in my_ssize_t hWnd, __in LONG msg, __in my_ssize_t wParam,
                         __in my_ssize_t lParam, __out VARIANT_BOOL *result);

  STDMETHOD(GetChildWindow)(__in LONG procId, __in my_ssize_t hParentWnd, __in LONG index, __out my_ssize_t *hWnd);

  STDMETHOD(GetChildWindowFromId)(__in LONG procId, __in my_ssize_t hParentWnd, __in LONG ctrlId,
                                  __out my_ssize_t *hWnd);

  STDMETHOD(Memory)(__in LONG procId, __deref_out INktProcessMemory **ppProcMem);

  STDMETHOD(CreateProcess)(__in BSTR imagePath, __in VARIANT_BOOL suspended, __out_opt VARIANT *continueEvent,
                           __out LONG *procId);

  STDMETHOD(CreateProcessWithLogon)(__in BSTR imagePath, __in BSTR userName, __in BSTR password,
                                    __in VARIANT_BOOL suspended, __out_opt VARIANT *continueEvent, __out LONG *procId);

  STDMETHOD(ResumeProcess)(__in LONG procId, __in VARIANT continueEvent);

  STDMETHOD(TerminateProcess)(__in LONG procId, __in LONG exitCode);

  STDMETHOD(CloseHandle)(__in VARIANT h);

  STDMETHOD(InvokeComMethod)(__in IUnknown* unkObj, __in BSTR methodName, __in VARIANT parameters,
                             __out VARIANT *pResult);

  STDMETHOD(put_ComProperty)(__in IUnknown *lpUnk, __in BSTR propertyName, __in VARIANT value);

  STDMETHOD(get_ComProperty)(__in IUnknown *lpUnk, __in BSTR propertyName, __out VARIANT *pValue);

  STDMETHOD(put_ComPropertyWithParams)(__in IUnknown *lpUnk, __in BSTR propertyName, __in VARIANT parameters,
                                       __in VARIANT value);

  STDMETHOD(get_ComPropertyWithParams)(__in IUnknown *lpUnk, __in BSTR propertyName, __in VARIANT parameters,
                                       __out VARIANT *pValue);

  STDMETHOD(DefineJavaClass)(__in LONG procId, __in BSTR className, __in VARIANT byteCode);

  STDMETHOD(CreateJavaObject)(__in LONG procId, __in BSTR className, __in VARIANT constructorArgs,
                              __deref_out INktJavaObject **ppJavaObj);

  STDMETHOD(InvokeJavaStaticMethod)(__in LONG procId, __in BSTR className, __in BSTR methodName,
                                    __in VARIANT parameters, __out VARIANT *value);

  STDMETHOD(put_StaticJavaField)(__in LONG procId, __in BSTR className, __in BSTR fieldName, __in VARIANT value);

  STDMETHOD(get_StaticJavaField)(__in LONG procId, __in BSTR className, __in BSTR fieldName, __out VARIANT *value);

  STDMETHOD(IsJVMAttached)(__in  LONG procId, __out LONG *verMajor, __out LONG *verMinor, __out VARIANT_BOOL *pRet);

private:
  friend class CNktJavaObjectImpl;

  HRESULT RaiseOnComInterfaceCreatedEvent(__in TMSG_RAISEONCOMINTERFACECREATEDEVENT *lpMsg, __in SIZE_T nDataSize,
                             __in DWORD dwPid, __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

  HRESULT FindOrCreateNktJavaObject(__in DWORD dwPid, __in ULONGLONG javaobj, __deref_out INktJavaObject **ppJavaObj);
  VOID DisconnectNktJavaObject(__in CNktJavaObjectImpl *lpJavaObjImpl);
  VOID DisconnectAllNktJavaObjects();
  //----
  HRESULT InternalJavaInvokeMethod(__in DWORD dwPid, __in BSTR className, __in ULONGLONG javaobj,
                                   __in BSTR methodName, __in VARIANT parameters, __out VARIANT *value);
  HRESULT InternalJavaSetField(__in DWORD dwPid, __in BSTR className, __in ULONGLONG javaobj, __in BSTR fieldName,
                               __in VARIANT value);
  HRESULT InternalJavaGetField(__in DWORD dwPid, __in BSTR className, __in ULONGLONG javaobj, __in BSTR fieldName,
                               __out VARIANT *value);
  HRESULT InternalJavaIsSameObject(__in DWORD dwPid, __in ULONGLONG javaobj1, __in ULONGLONG javaobj2,
                                   __out VARIANT_BOOL *pRet);
  HRESULT InternalJavaGetClassName(__in DWORD dwPid, __in ULONGLONG javaobj, __out BSTR *className);
  HRESULT InternalJavaIsInstanceOf(__in DWORD dwPid, __in ULONGLONG javaobj, __in BSTR className,
                                   __out VARIANT_BOOL *pRet);

  HRESULT RaiseOnJavaNativeMethodCalled(__in TMSG_RAISEONJAVANATIVEMETHODCALLED *lpMsg, __in SIZE_T nDataSize,
                                        __in DWORD dwPid, __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

private:
  HRESULT SendMsg(__in CClient *lpClient, __in LPVOID lpData, __in SIZE_T nDataSize,
                  __inout_opt LPVOID *lplpReply=NULL, __inout_opt SIZE_T *lpnReplySize=NULL);

  HRESULT OnClientMessage(__in CClient *lpClient, __in LPBYTE lpData, __in SIZE_T nDataSize,
                          __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);
  VOID OnClientDisconnected(__in CClient *lpClient);

private:
  CComPtr<IUnknown> cUnkMarshaler;
  //----
  struct {
    CNktFastMutex cMtx;
    TNktLnkLst<CNktJavaObjectImpl> cList;
  } sJavaObjects;
};

//-----------------------------------------------------------

OBJECT_ENTRY_AUTO(__uuidof(NktRemoteBridge), CNktRemoteBridgeImpl)
