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
#include "..\Common\TransportMessages.h"
#include "..\Common\Tools.h"
#include "..\Common\Com\VariantMisc.h"

//-----------------------------------------------------------

static HRESULT ValidateGuid(__out GUID &sGuid, __in BSTR guid);
static HRESULT StreamFromData(__in LPVOID lpData, __in SIZE_T nDataSize, __deref_out IStream **lplpStream);
static HRESULT IDispatchInvokeHelper(__in IUnknown *lpUnk, __in WORD wType, __in BSTR name,
                                     __in VARIANT *pParameters, __in VARIANT *pValue, __out VARIANT *pResult);

//-----------------------------------------------------------

STDMETHODIMP CNktRemoteBridgeImpl::WatchComInterface(__in LONG procId, __in BSTR clsid, __in BSTR iid)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TMSG_WATCHCOMINTERFACE sMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  HRESULT hRes;

  if (procId == 0)
    return E_INVALIDARG;
  if ((clsid == NULL || clsid[0] == 0) && (iid == NULL || iid[0] == 0))
    return E_INVALIDARG;
  memset(&(sMsg.sClsId), 0, sizeof(sMsg.sClsId));
  memset(&(sMsg.sIid), 0, sizeof(sMsg.sIid));
  if (clsid != NULL && clsid[0] != 0)
  {
    hRes = ValidateGuid(sMsg.sClsId, clsid);
    if (FAILED(hRes))
      return hRes;
  }
  if (iid != NULL && iid[0] != 0)
  {
    hRes = ValidateGuid(sMsg.sIid, iid);
    if (FAILED(hRes))
      return hRes;
  }
  //----
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableComHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.sHeader.dwMsgCode = TMSG_CODE_WatchComInterface;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
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
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::UnwatchComInterface(__in LONG procId, __in BSTR clsid, __in BSTR iid)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TMSG_UNWATCHCOMINTERFACE sMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  HRESULT hRes;

  if (procId == 0)
    return E_INVALIDARG;
  if ((clsid == NULL || clsid[0] == 0) && (iid == NULL || iid[0] == 0))
    return E_INVALIDARG;
  memset(&(sMsg.sClsId), 0, sizeof(sMsg.sClsId));
  memset(&(sMsg.sIid), 0, sizeof(sMsg.sIid));
  if (clsid != NULL && clsid[0] != 0)
  {
    hRes = ValidateGuid(sMsg.sClsId, clsid);
    if (FAILED(hRes))
      return hRes;
  }
  if (iid != NULL && iid[0] != 0)
  {
    hRes = ValidateGuid(sMsg.sIid, iid);
    if (FAILED(hRes))
      return hRes;
  }
  //----
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableComHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.sHeader.dwMsgCode = TMSG_CODE_UnwatchComInterface;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
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
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::UnwatchAllComInterfaces(__in LONG procId)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TMSG_HEADER sMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  HRESULT hRes;

  if (procId == 0)
    return E_INVALIDARG;
  //----
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableComHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.dwMsgCode = TMSG_CODE_UnwatchAllComInterfaces;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
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
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::GetComInterfaceFromHwnd(__in LONG procId, __in my_ssize_t hwnd, __in BSTR iid,
                                                           __deref_out IUnknown **ppUnk)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TMSG_GETCOMINTERFACEFROMHWND sMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  TNktComPtr<IStream> cStream;
  HRESULT hRes;

  if (ppUnk == NULL)
    return E_POINTER;
  *ppUnk = NULL;
  if (procId == 0 || hwnd == 0)
    return E_INVALIDARG;
  hRes = ValidateGuid(sMsg.sIid, iid);
  if (FAILED(hRes))
    return hRes;
  //----
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableComHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.sHeader.dwMsgCode = TMSG_CODE_GetComInterfaceFromHWnd;
    sMsg.hWnd = NKT_PTR_2_ULONGLONG((my_size_t)hwnd);
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize >= sizeof(TMSG_SIMPLE_REPLY))
    {
      hRes = lpReply->hRes;
      if (SUCCEEDED(hRes) && nReplySize > sizeof(TMSG_SIMPLE_REPLY))
      {
        hRes = StreamFromData((LPBYTE)(lpReply+1), nReplySize-sizeof(TMSG_SIMPLE_REPLY), &cStream);
        if (SUCCEEDED(hRes))
          hRes = ::CoUnmarshalInterface(cStream, IID_NULL, (LPVOID*)ppUnk);
        if (FAILED(hRes))
          *ppUnk = NULL;
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::GetComInterfaceFromIndex(__in LONG procId, __in BSTR iid, __in int index,
                                                            __deref_out IUnknown **ppUnk)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  TMSG_GETCOMINTERFACEFROMINDEX sMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  TNktComPtr<IStream> cStream;
  HRESULT hRes;

  if (ppUnk == NULL)
    return E_POINTER;
  *ppUnk = NULL;
  if (procId == 0 || index < 0)
    return E_INVALIDARG;
  hRes = ValidateGuid(sMsg.sIid, iid);
  if (FAILED(hRes))
    return hRes;
  //----
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableComHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.sHeader.dwMsgCode = TMSG_CODE_GetComInterfaceFromIndex;
    sMsg.nIndex = (ULONG)index;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
  }
  if (SUCCEEDED(hRes))
  {
    if (nReplySize >= sizeof(TMSG_SIMPLE_REPLY))
    {
      hRes = lpReply->hRes;
      if (SUCCEEDED(hRes) && nReplySize > sizeof(TMSG_SIMPLE_REPLY))
      {
        hRes = StreamFromData((LPBYTE)(lpReply+1), nReplySize-sizeof(TMSG_SIMPLE_REPLY), &cStream);
        if (SUCCEEDED(hRes))
          hRes = ::CoUnmarshalInterface(cStream, IID_NULL, (LPVOID*)ppUnk);
        if (FAILED(hRes))
          *ppUnk = NULL;
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
  }
  if (lpReply != NULL)
    FreeIpcBuffer(lpReply);
  return hRes;
}

STDMETHODIMP CNktRemoteBridgeImpl::HookComInterfaceMethod(__in LONG procId, __in BSTR iid, __in int methodIndex,
                                                          __in VARIANT iidParamStringOrIndex, __in int returnParamIndex)
{
  TNktComPtr<CClientMgr::CClient> cClient;
  CNktComVariant cVt;
  TMSG_INSPECTCOMINTERFACEMETHOD sMsg;
  TMSG_SIMPLE_REPLY *lpReply = NULL;
  SIZE_T nReplySize;
  HRESULT hRes;

  if (procId == 0 || methodIndex < 0 || returnParamIndex < -1)
    return E_INVALIDARG;
  hRes = ValidateGuid(sMsg.sIid, iid);
  if (SUCCEEDED(hRes))
  {
    hRes = ::VariantCopyInd(&(cVt.sVt), (VARIANTARG*)&iidParamStringOrIndex);
    if (FAILED(hRes))
      return hRes;
    switch (V_VT(&(cVt.sVt)))
    {
      case VT_I1:
        if (cVt.sVt.cVal < 0)
          hRes = E_INVALIDARG;
        else
          sMsg.nIidIndex = (ULONG)(BYTE)(cVt.sVt.cVal);
        break;
      case VT_I2:
        if (cVt.sVt.iVal < 0)
          hRes = E_INVALIDARG;
        else
          sMsg.nIidIndex = (ULONG)(USHORT)(cVt.sVt.iVal);
        break;
      case VT_I4:
        if (cVt.sVt.lVal < 0)
          hRes = E_INVALIDARG;
        else
          sMsg.nIidIndex = (ULONG)(cVt.sVt.lVal);
        break;
      case VT_UI1:
        sMsg.nIidIndex = (ULONG)(cVt.sVt.bVal);
        break;
      case VT_UI2:
        sMsg.nIidIndex = (ULONG)(cVt.sVt.uiVal);
        break;
      case VT_UI4:
        if (cVt.sVt.ulVal == ULONG_MAX)
          hRes = E_INVALIDARG;
        else
          sMsg.nIidIndex = cVt.sVt.ulVal;
        break;
      case  VT_BSTR:
        sMsg.nIidIndex = ULONG_MAX;
        hRes = ValidateGuid(sMsg.sQueryIid, cVt.sVt.bstrVal);
        break;
      default:
        hRes = E_INVALIDARG;
        break;
    }
  }
  if (FAILED(hRes))
    return hRes;
  //----
  hRes = CClientMgr::GetClientByPid((DWORD)procId, &cClient);
  if (SUCCEEDED(hRes) && (cClient->nHookFlags & (ULONG)flgDisableComHooking) != 0)
    hRes = E_ACCESSDENIED;
  if (SUCCEEDED(hRes))
  {
    sMsg.sHeader.dwMsgCode = TMSG_CODE_InspectComInterfaceMethod;
    sMsg.nMethodIndex = (ULONG)methodIndex;
    sMsg.nReturnParameterIndex = (returnParamIndex < 0) ? ULONG_MAX : (ULONG)returnParamIndex;
    hRes = SendMsg(cClient, &sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
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
  return hRes;
}

HRESULT CNktRemoteBridgeImpl::InvokeComMethod(__in IUnknown* lpUnk, __in BSTR methodName, __in VARIANT parameters,
                                              __out VARIANT *pResult)
{
  return IDispatchInvokeHelper(lpUnk, DISPATCH_METHOD, methodName, &parameters, NULL, pResult);
}

HRESULT CNktRemoteBridgeImpl::put_ComProperty(__in IUnknown *lpUnk, __in BSTR propertyName, __in VARIANT value)
{
  return IDispatchInvokeHelper(lpUnk, DISPATCH_PROPERTYPUT, propertyName, NULL, &value, NULL);
}

HRESULT CNktRemoteBridgeImpl::get_ComProperty(__in IUnknown *lpUnk, __in BSTR propertyName, __out VARIANT *pValue)
{
  return IDispatchInvokeHelper(lpUnk, DISPATCH_PROPERTYGET, propertyName, NULL, NULL, pValue);
}

HRESULT CNktRemoteBridgeImpl::put_ComPropertyWithParams(__in IUnknown *lpUnk, __in BSTR propertyName,
                                                        __in VARIANT parameters, __in VARIANT value)
{
  return IDispatchInvokeHelper(lpUnk, DISPATCH_PROPERTYPUT, propertyName, &parameters, &value, NULL);
}

HRESULT CNktRemoteBridgeImpl::get_ComPropertyWithParams(__in IUnknown *lpUnk, __in BSTR propertyName,
                                                        __in VARIANT parameters, __out VARIANT *pValue)
{
  return IDispatchInvokeHelper(lpUnk, DISPATCH_PROPERTYGET, propertyName, &parameters, NULL, pValue);
}

HRESULT CNktRemoteBridgeImpl::RaiseOnComInterfaceCreatedEvent(__in TMSG_RAISEONCOMINTERFACECREATEDEVENT *lpMsg,
                                                 __in SIZE_T nDataSize, __in DWORD dwPid, __out LPVOID *lplpReplyData,
                                                 __out SIZE_T *lpnReplyDataSize)
{
  TNktComPtr<IUnknown> cUnk;
  TNktComPtr<IStream> cStream;
  HRESULT hRes;

  if (nDataSize <= sizeof(TMSG_RAISEONCOMINTERFACECREATEDEVENT))
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  hRes = StreamFromData((LPBYTE)(lpMsg+1), nDataSize-sizeof(TMSG_RAISEONCOMINTERFACECREATEDEVENT), &cStream);
  NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteBridge: StreamFromData %08X", hRes));
  if (SUCCEEDED(hRes))
  {
    hRes = ::CoUnmarshalInterface(cStream, IID_NULL, (LPVOID*)&cUnk);
    NKT_DEBUGPRINTA((Nektra::dlDebug, "RemoteBridge: CoUnmarshalInterface %08X", hRes));
  }
  if (SUCCEEDED(hRes))
  {
    Fire_OnComInterfaceCreated(dwPid, lpMsg->dwTid, lpMsg->sClsId, lpMsg->sIid, cUnk);
  }
  *lplpReplyData = AllocIpcBuffer(sizeof(TMSG_SIMPLE_REPLY));
  if ((*lplpReplyData) == NULL)
    return E_OUTOFMEMORY;
  ((TMSG_SIMPLE_REPLY*)(*lplpReplyData))->hRes = S_OK;
  *lpnReplyDataSize = sizeof(TMSG_SIMPLE_REPLY);
  return S_OK;
}

//-----------------------------------------------------------

static HRESULT ValidateGuid(__out GUID &sGuid, __in BSTR guid)
{
  SIZE_T i, k, nHasBraces;
  BYTE aBuf[16], nVal;

  if (guid == NULL)
    return E_POINTER;
  nHasBraces = (guid[0] == L'{') ? 1 : 0;
  for (i=k=0; i<36; i++)
  {
    if (i == 8 || i == 13 || i == 18 || i == 23)
    {
      if (guid[i+nHasBraces] != L'-')
        return E_INVALIDARG;
    }
    else
    {
      if (guid[i+nHasBraces] >= L'0' && guid[i+nHasBraces] <= L'9')
        nVal = (BYTE)(guid[i+nHasBraces] - L'0');
      else if (guid[i+nHasBraces] >= L'A' && guid[i+nHasBraces] <= L'F')
        nVal = (BYTE)(guid[i+nHasBraces] - L'A' + 10);
      else if (guid[i+nHasBraces] >= L'a' && guid[i+nHasBraces] <= L'f')
        nVal = (BYTE)(guid[i+nHasBraces] - L'a' + 10);
      else
        return E_INVALIDARG;
      if ((k & 1) == 0)
        aBuf[k>>1] = nVal << 4;
      else
        aBuf[k>>1] |= nVal;
      k++;
    }
  }
  if (nHasBraces == 1 && guid[37] != L'}')
    return E_INVALIDARG;
  if (guid[36+nHasBraces*2] != 0)
    return E_INVALIDARG;
  sGuid.Data1 = ((DWORD)aBuf[0] << 24) | ((DWORD)aBuf[1] << 16) | ((DWORD)aBuf[2] << 8) | (DWORD)aBuf[3];
  sGuid.Data2 = ((DWORD)aBuf[4] << 8) | (DWORD)aBuf[5];
  sGuid.Data3 = ((DWORD)aBuf[6] << 8) | (DWORD)aBuf[7];
  memcpy(sGuid.Data4, aBuf+8, 8);
  return S_OK;
}

static HRESULT StreamFromData(__in LPVOID lpData, __in SIZE_T nDataSize, __deref_out IStream **lplpStream)
{
  TNktComPtr<IStream> cStream;
  HGLOBAL hMem;
  LPVOID lpMem;
  HRESULT hRes;

  _ASSERT(lplpStream != NULL);
  if (lpData == NULL)
    return E_POINTER;
  if (nDataSize == 0)
    return E_INVALIDARG;
  hMem = ::GlobalAlloc(GMEM_MOVEABLE, nDataSize);
  if (hMem == NULL)
    return E_OUTOFMEMORY;
  lpMem = ::GlobalLock(hMem);
  if (lpMem == NULL)
  {
    ::GlobalFree(hMem);
    return E_OUTOFMEMORY;
  }
  memcpy(lpMem, lpData, nDataSize);
  ::GlobalUnlock(hMem);
  hRes = ::CreateStreamOnHGlobal(hMem, TRUE, &cStream);
  if (FAILED(hRes))
  {
    ::GlobalFree(hMem);
    return hRes;
  }
  *lplpStream = cStream.Detach();
  return S_OK;
}

static HRESULT IDispatchInvokeHelper(__in IUnknown *lpUnk, __in WORD wType, __in BSTR name,
                                     __in VARIANT *pParameters, __in VARIANT *pValue, __out VARIANT *pResult)
{
  TNktComPtr<IDispatch> cDisp;
  CNktComVariant cVtParams, vtTempResult;
  SAFEARRAY *lpParamsArray;
  VARIANTARG aArgs[32];
  DISPID nDispID, nDispID_PropPut;
  DISPPARAMS sDispParams;
  UINT i;
  ULONG k;
  HRESULT hRes;

  if (pResult != NULL)
    ::VariantInit(pResult);
  if (lpUnk == NULL || name == NULL)
    return E_POINTER;
  hRes = lpUnk->QueryInterface(IID_IDispatch, (LPVOID*)&cDisp);
  if (FAILED(hRes))
    return hRes;
  lpParamsArray = NULL;
  if (pParameters != NULL)
  {
    hRes = CheckInputVariantAndConvertToSafeArray(&(cVtParams.sVt), pParameters, FALSE);
    if (FAILED(hRes))
      return hRes;
    if (cVtParams.sVt.vt != VT_NULL && cVtParams.sVt.vt != VT_EMPTY)
    {
      if (cVtParams.sVt.parray->rgsabound[0].cElements > 0)
        lpParamsArray = cVtParams.sVt.parray;
    }
  }
  switch (wType)
  {
    case DISPATCH_PROPERTYPUT:
      if (pValue == NULL)
        return E_POINTER;
      break;
    case DISPATCH_PROPERTYGET:
      if (pResult == NULL)
        return E_POINTER;
      break;
  }
  //get dispid
  hRes = cDisp->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &nDispID);
  //fill data (put propertyput value first)
  if (SUCCEEDED(hRes))
  {
    memset(&sDispParams, 0, sizeof(sDispParams));
    if (wType == DISPATCH_PROPERTYPUT)
    {
      nDispID_PropPut = DISPID_PROPERTYPUT;
      sDispParams.cNamedArgs = 1;
      sDispParams.rgdispidNamedArgs = &nDispID_PropPut;
      hRes = ::VariantCopy(&aArgs[0], pValue);
      if (SUCCEEDED(hRes))
        sDispParams.cArgs = 1;
    }
  }
  if (SUCCEEDED(hRes) && lpParamsArray != NULL)
  {
    TNktComSafeArrayAccess<VARIANTARG> cParamData;
    ULONG nElemCount;

    nElemCount = lpParamsArray->rgsabound[0].cElements;
    hRes = cParamData.Setup(lpParamsArray);
    if (SUCCEEDED(hRes))
    {
      for (k=0; k<nElemCount; k++)
      {
        hRes = ::VariantCopy(&aArgs[sDispParams.cArgs], cParamData.Get()+k);
        if (FAILED(hRes))
          break;
        (sDispParams.cArgs)++;
      }
    }
  }
  if (SUCCEEDED(hRes))
  {
    if (sDispParams.cArgs > 0)
      sDispParams.rgvarg = aArgs;
    hRes = cDisp->Invoke(nDispID, IID_NULL, LOCALE_USER_DEFAULT, wType, &sDispParams,
                         (pResult != NULL) ? pResult : &(vtTempResult.sVt), NULL, NULL);
  }
  for (i=0; i<sDispParams.cArgs; i++)
    ::VariantClear(&aArgs[i]);
  return hRes;
}
