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

#include "DynamicConnectionPoint.h"

//-----------------------------------------------------------

template <class T>
class CNktRemoteBridgeEventsImpl : public IDynamicConnectionPoint<T, DNktRemoteBridgeEvents>
{
public:
  HRESULT Fire_OnCreateProcessCall(__in DWORD dwPid, __in DWORD dwChildPid, __in DWORD dwMainThreadId,
                                   __in BOOL bIs64BitProcess)
    {
    TNktComPtr<IDispatch> cConnDisp;
    HRESULT hRes;
    int i;

    //this event is always called from a secondary thread
    for (i=0; ; i++)
    {
      cConnDisp.Release();
      hRes = GetConnAt(&cConnDisp, i);
      if (hRes == E_INVALIDARG)
        break;
      if (SUCCEEDED(hRes))
      {
        CComVariant aVarParams[4];

        aVarParams[3] = (LONG)dwPid;
        aVarParams[2] = (LONG)dwChildPid;
        aVarParams[1] = (LONG)dwMainThreadId;
        aVarParams[0].vt = VT_BOOL;
        aVarParams[0].boolVal = (bIs64BitProcess != FALSE) ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
        hRes = FireCommon(cConnDisp, dispidNktRemoteBridgeEventsOnCreateProcessCall, aVarParams, 4);
      }
    }
    return S_OK;
    };

  HRESULT Fire_OnProcessUnhooked(__in DWORD dwPid)
    {
    TNktComPtr<IDispatch> cConnDisp;
    HRESULT hRes;
    int i;

    //this event is always called from a secondary thread
    for (i=0; ; i++)
    {
      cConnDisp.Release();
      hRes = GetConnAt(&cConnDisp, i);
      if (hRes == E_INVALIDARG)
        break;
      if (SUCCEEDED(hRes))
      {
        CComVariant aVarParams[1];

        aVarParams[0] = (LONG)dwPid;
        hRes = FireCommon(cConnDisp, dispidNktRemoteBridgeEventsOnProcessUnhooked, aVarParams, 1);
      }
    }
    return S_OK;
    };

  HRESULT Fire_OnComInterfaceCreated(__in DWORD dwPid, __in DWORD dwTid, __in REFCLSID sClsId, __in REFIID sIid,
                                     __in IUnknown *lpUnk)
    {
    TNktComPtr<IDispatch> cConnDisp;
    HRESULT hRes;
    int i;

    //this event is always called from a secondary thread
    for (i=0; ; i++)
    {
      cConnDisp.Release();
      hRes = GetConnAt(&cConnDisp, i);
      if (hRes == E_INVALIDARG)
        break;
      if (SUCCEEDED(hRes))
      {
        CComVariant aVarParams[5];
        WCHAR szBufW[128];

        aVarParams[4] = (LONG)dwPid;
        aVarParams[3] = (LONG)dwTid;
        swprintf_s(szBufW, 128, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", sClsId.Data1, sClsId.Data2,
                   sClsId.Data3, sClsId.Data4[0], sClsId.Data4[1], sClsId.Data4[2], sClsId.Data4[3], sClsId.Data4[4],
                   sClsId.Data4[5], sClsId.Data4[6], sClsId.Data4[7]);
        aVarParams[2] = szBufW;
        if (aVarParams[2].vt != VT_BSTR)
          return E_OUTOFMEMORY;
        swprintf_s(szBufW, 128, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", sIid.Data1, sIid.Data2,
                   sIid.Data3, sIid.Data4[0], sIid.Data4[1], sIid.Data4[2], sIid.Data4[3], sIid.Data4[4],
                   sIid.Data4[5], sIid.Data4[6], sIid.Data4[7]);
        aVarParams[1] = szBufW;
        if (aVarParams[1].vt != VT_BSTR)
          return E_OUTOFMEMORY;
        aVarParams[0] = lpUnk;
        hRes = FireCommon(cConnDisp, dispidNktRemoteBridgeEventsOnComInterfaceCreated, aVarParams, 5);
      }
    }
    return S_OK;
    };

  HRESULT Fire_OnJavaNativeMethodCalled(__in DWORD dwPid, __in LPCWSTR szClassNameW, __in LPCWSTR szMethodNameW,
                                        __in IDispatch *lpJavaObjOrClass, __in SAFEARRAY *lpParams,
                                        __out VARIANT *lpResult)
    {
    TNktComPtr<IDispatch> cConnDisp;
    HRESULT hRes;
    int i;

    //this event is always called from a secondary thread
    for (i=0; ; i++)
    {
      cConnDisp.Release();
      hRes = GetConnAt(&cConnDisp, i);
      if (hRes == E_INVALIDARG)
        break;
      if (SUCCEEDED(hRes))
      {
        CComVariant aVarParams[6];

        aVarParams[5] = (LONG)dwPid;
        aVarParams[4] = szClassNameW;
        if (aVarParams[4].vt != VT_BSTR)
          return E_OUTOFMEMORY;
        aVarParams[3] = szMethodNameW;
        if (aVarParams[3].vt != VT_BSTR)
          return E_OUTOFMEMORY;
        aVarParams[2] = lpJavaObjOrClass;
        aVarParams[1].vt = VT_BYREF|VT_ARRAY|VT_VARIANT; //pass SAFEARRAY byref (VariantClear doesn't touch byref items)
        aVarParams[1].pparray = &lpParams;
        aVarParams[0].vt = VT_BYREF|VT_VARIANT;
        aVarParams[0].pvarVal = lpResult;
        hRes = FireCommon(cConnDisp, dispidNktRemoteBridgeEventsOnJavaCustomNativeCall, aVarParams, 6);
      }
    }
    return S_OK;
    };
};
