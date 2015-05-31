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
#include "JavaObject.h"
#include "RemoteBridge.h"

//-----------------------------------------------------------
// CNktJavaObjectImpl

STDMETHODIMP CNktJavaObjectImpl::InterfaceSupportsErrorInfo(REFIID riid)
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

STDMETHODIMP CNktJavaObjectImpl::InvokeMethod(__in BSTR methodName, __in VARIANT parameters, __out VARIANT *value)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  if (value != NULL)
  {
    ::VariantInit(value);
    V_VT(value) = VT_NULL;
  }
  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
    return E_FAIL;
  return cBridgeImpl->InternalJavaInvokeMethod(dwPid, NULL, javaobj, methodName, parameters, value);
}

STDMETHODIMP CNktJavaObjectImpl::InvokeSuperclassMethod(__in BSTR methodName, __in BSTR superClassName,
                                                        __in VARIANT parameters, __out VARIANT *value)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  if (value != NULL)
  {
    ::VariantInit(value);
    V_VT(value) = VT_NULL;
  }
  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
    return E_FAIL;
  if (superClassName == NULL)
    return E_POINTER;
  return cBridgeImpl->InternalJavaInvokeMethod(dwPid, superClassName, javaobj, methodName, parameters, value);
}

STDMETHODIMP CNktJavaObjectImpl::put_Field(__in BSTR fieldName, __in VARIANT value)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
    return E_FAIL;
  return cBridgeImpl->InternalJavaSetField(dwPid, NULL, javaobj, fieldName, value);
}

STDMETHODIMP CNktJavaObjectImpl::get_Field(__in BSTR fieldName, __out VARIANT *value)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  if (value != NULL)
  {
    ::VariantInit(value);
    V_VT(value) = VT_NULL;
  }
  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
    return E_FAIL;
  return cBridgeImpl->InternalJavaGetField(dwPid, NULL, javaobj, fieldName, value);
}

STDMETHODIMP CNktJavaObjectImpl::put_SuperclassField(__in BSTR fieldName, __in BSTR superClassName,
                                                     __in VARIANT value)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
    return E_FAIL;
  if (superClassName == NULL)
    return E_POINTER;
  return cBridgeImpl->InternalJavaSetField(dwPid, superClassName, javaobj, fieldName, value);
}

STDMETHODIMP CNktJavaObjectImpl::get_SuperclassField(__in BSTR fieldName, __in BSTR superClassName,
                                                        __out VARIANT *value)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  if (value != NULL)
  {
    ::VariantInit(value);
    V_VT(value) = VT_NULL;
  }
  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
    return E_FAIL;
  if (superClassName == NULL)
    return E_POINTER;
  return cBridgeImpl->InternalJavaGetField(dwPid, superClassName, javaobj, fieldName, value);
}

STDMETHODIMP CNktJavaObjectImpl::IsSameObject(__in INktJavaObject *pJavaObj, __out VARIANT_BOOL *ret)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;
  CNktJavaObjectImpl *lpJavaObjImpl;

  if (ret == NULL)
    return E_POINTER;
  *ret = VARIANT_FALSE;
  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
    return E_FAIL;
  if (pJavaObj == NULL)
    return E_POINTER;
  lpJavaObjImpl = static_cast<CNktJavaObjectImpl*>(pJavaObj);
  return cBridgeImpl->InternalJavaIsSameObject(dwPid, lpJavaObjImpl->javaobj, javaobj, ret);
}

STDMETHODIMP CNktJavaObjectImpl::GetClassName(__out BSTR *className)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  if (className == NULL)
    return E_POINTER;
  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
  {
    *className = ::SysAllocString(L"");
    return E_FAIL;
  }
  return cBridgeImpl->InternalJavaGetClassName(dwPid, javaobj, className);
}

STDMETHODIMP CNktJavaObjectImpl::IsInstanceOf(__in BSTR className, __out VARIANT_BOOL *ret)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  if (ret != NULL)
    *ret = VARIANT_FALSE;
  if (className == NULL || ret == NULL)
    return E_POINTER;
  if (className[0] == 0)
    return E_INVALIDARG;
  {
    ObjectLock cLock(this);

    cBridgeImpl = lpBridgeImpl;
  }
  if (cBridgeImpl == NULL)
    return E_FAIL;
  return cBridgeImpl->InternalJavaIsInstanceOf(dwPid, javaobj, className, ret);
}

VOID CNktJavaObjectImpl::Disconnect(__in BOOL bCalledFromBridge)
{
  TNktComPtr<CNktRemoteBridgeImpl> cBridgeImpl;

  {
    ObjectLock cLock(this);

    if (bCalledFromBridge == FALSE)
      cBridgeImpl = lpBridgeImpl;
    lpBridgeImpl = NULL;
  }
  if (cBridgeImpl != NULL)
     cBridgeImpl->DisconnectNktJavaObject(this);
  return;
}
