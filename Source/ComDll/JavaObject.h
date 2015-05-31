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

//-----------------------------------------------------------
// CNktJavaObjectImpl

class ATL_NO_VTABLE CNktJavaObjectImpl : public CComObjectRootEx<CComMultiThreadModel>,
                                         public CComCoClass<CNktJavaObjectImpl, &CLSID_NktRemoteBridge>,
                                         public IObjectSafetyImpl<CNktJavaObjectImpl,
                                                                  INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
                                         public IDispatchImpl<INktJavaObject, &IID_INktRemoteBridge,
                                                              &LIBID_RemoteBridge, 1, 0>,
                                         public TNktLnkLstNode<CNktJavaObjectImpl>

{
public:
  CNktJavaObjectImpl() : CComObjectRootEx<CComMultiThreadModel>(),
                         CComCoClass<CNktJavaObjectImpl, &CLSID_NktRemoteBridge>(),
                         IObjectSafetyImpl<CNktJavaObjectImpl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>(),
                         IDispatchImpl<INktJavaObject, &IID_INktRemoteBridge, &LIBID_RemoteBridge, 1, 0>(),
                         TNktLnkLstNode<CNktJavaObjectImpl>()
    {
    dwPid = 0;
    javaobj = 0;
    lpBridgeImpl = NULL;
    return;
    };

  ~CNktJavaObjectImpl()
    {
    return;
    };

  DECLARE_REGISTRY_RESOURCEID_EX(IDR_INTERFACEREGISTRAR, L"RemoteBridge.NktJavaObject",
                                 L"1", L"NktJavaObject Class", CLSID_NktJavaObject,
                                 LIBID_RemoteBridge, L"Neutral")

  BEGIN_COM_MAP(CNktJavaObjectImpl)
    COM_INTERFACE_ENTRY(INktJavaObject)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, cUnkMarshaler.p)
  END_COM_MAP()

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
    Disconnect(FALSE);
    return;
    };

public:
  STDMETHOD(InvokeMethod)(__in BSTR methodName, __in VARIANT parameters, __out VARIANT *value);

  STDMETHOD(InvokeSuperclassMethod)(__in BSTR methodName, __in BSTR superClassName, __in VARIANT parameters,
                                    __out VARIANT *value);

  STDMETHOD(put_Field)(__in BSTR fieldName, __in VARIANT value);

  STDMETHOD(get_Field)(__in BSTR fieldName, __out VARIANT *value);

  STDMETHOD(put_SuperclassField)(__in BSTR fieldName, __in BSTR superClassName, __in VARIANT value);

  STDMETHOD(get_SuperclassField)(__in BSTR fieldName, __in BSTR superClassName, __out VARIANT *value);

  STDMETHOD(IsSameObject)(__in INktJavaObject *pJavaObj, __out VARIANT_BOOL *ret);

  STDMETHOD(GetClassName)(__out BSTR *className);

  STDMETHOD(IsInstanceOf)(__in BSTR className, __out VARIANT_BOOL *ret);

public:
  DWORD GetPid()
    {
    return dwPid;
    };
  ULONGLONG GetJObject()
    {
    return javaobj;
    };

private:
  friend class CNktRemoteBridgeImpl;

  VOID Disconnect(__in BOOL bCalledFromBridge);

private:
  ULONGLONG javaobj;
  DWORD dwPid;
  CComPtr<IUnknown> cUnkMarshaler;
  CNktRemoteBridgeImpl *lpBridgeImpl;
};

OBJECT_ENTRY_NON_CREATEABLE_EX_AUTO(__uuidof(NktJavaObject), CNktJavaObjectImpl)
