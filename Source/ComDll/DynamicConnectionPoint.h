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

#include "..\Common\LinkedList.h"

//-----------------------------------------------------------

template <class T, typename _uuid>
class ATL_NO_VTABLE IDynamicConnectionPoint : public _ICPLocator<&__uuidof(_uuid)>
{
  typedef CComEnum<IEnumConnections, &__uuidof(IEnumConnections), CONNECTDATA,
                   _Copy<CONNECTDATA>> CComEnumConnections;

public:
  IDynamicConnectionPoint() : _ICPLocator<&__uuidof(_uuid)>()
    {
    InterlockedExchange(&nCookie, 0);
    return;
    };

  ~IDynamicConnectionPoint()
    {
    CItem *lpItem;

    while ((lpItem=cList.PopTail()) != NULL)
      delete lpItem;
    return;
    };

  STDMETHOD(_LocCPQueryInterface)(__in REFIID riid, __deref_out void **ppvObject)
    {
    ATLASSERT(ppvObject != NULL);
    if (ppvObject == NULL)
      return E_POINTER;
    *ppvObject = NULL;
    if (InlineIsEqualGUID(riid, __uuidof(IConnectionPoint)) || InlineIsEqualUnknown(riid))
    {
      *ppvObject = this;
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
    };

  STDMETHOD(GetConnectionInterface)(__out IID* piid2)
    {
    if (piid2 == NULL)
      return E_POINTER;
    *piid2 = __uuidof(_uuid);
    return S_OK;
    };

  STDMETHOD(GetConnectionPointContainer)(__in IConnectionPointContainer** ppCPC)
    {
    T* pT = static_cast<T*>(this);

    // No need to check ppCPC for NULL since QI will do that for us
    return pT->QueryInterface(__uuidof(IConnectionPointContainer), (void**)ppCPC);
    };

  STDMETHOD(Advise)(__in IUnknown *pUnkSink, __out DWORD *pdwCookie)
    {
    T* pT = static_cast<T*>(this);
    TNktComPtr<IStream> cStream;
    TNktComPtr<IDispatch> cDisp;
    HRESULT hRes;

    if (pdwCookie != NULL)
      *pdwCookie = 0;
    if (pUnkSink == NULL || pdwCookie == NULL)
      return E_POINTER;
    hRes = pUnkSink->QueryInterface(IID_IDispatch, (void**)&cDisp);
    if (FAILED(hRes))
      return (hRes == E_NOINTERFACE) ? CONNECT_E_CANNOTCONNECT : hRes;
    //create marshaller
    hRes = ::CreateStreamOnHGlobal(NULL, TRUE, &cStream);
    if (SUCCEEDED(hRes))
      hRes = ::CoMarshalInterface(cStream, IID_IDispatch, cDisp, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLESTRONG);
    if (SUCCEEDED(hRes))
    {
      CNktAutoFastMutex cLock(&cMtx);
      TNktAutoDeletePtr<CItem> cNewItem;

      //find a free slot and add to list
      cNewItem.Attach(new CItem());
      if (cNewItem != NULL)
      {
        do
        {
          cNewItem->dwCookie = (DWORD)InterlockedIncrement(&nCookie);
        }
        while (cNewItem->dwCookie == 0);
        cNewItem->cObj.Attach(cDisp.Detach());
        cNewItem->cStream.Attach(cStream.Detach());
        *pdwCookie = cNewItem->dwCookie;
        cList.PushTail(cNewItem.Detach());
      }
      else
      {
        hRes = E_OUTOFMEMORY;
      }
    }
    if (FAILED(hRes))
      ::CoReleaseMarshalData(cStream);
    return hRes;
    };

  STDMETHOD(Unadvise)(__in DWORD dwCookie)
    {
    T* pT = static_cast<T*>(this);
    CItem *lpItem;

    {
      CNktAutoFastMutex cLock(&cMtx);
      TNktLnkLst<CItem>::Iterator it;

      for (lpItem=it.Begin(cList); lpItem!=NULL; lpItem=it.Next())
      {
        if (lpItem->dwCookie == dwCookie)
        {
          lpItem->RemoveNode();
          break;
        }
      }
    }
    if (lpItem == NULL)
    {
      _ASSERT(FALSE);
      return CONNECT_E_NOCONNECTION;
    }
    delete lpItem;
    return S_OK;
    };

  STDMETHOD(EnumConnections)(__deref_out IEnumConnections** ppEnum)
    {
    T* pT = static_cast<T*>(this);
    TNktAutoDeletePtr<CComObject<CComEnumConnections>> cEnum;
    TNktComPtr<IStream> cStream;
    TNktComPtr<IUnknown> cUnk;
    TNktComPtr<IDispatch> p;
    LARGE_INTEGER li;
    TNktAutoDeletePtr<CONNECTDATA> cCd;
    SIZE_T i, k, nItems, nCount;
    HRESULT hRes;

    if (ppEnum == NULL)
      return E_POINTER;
    *ppEnum = NULL;
    ATLTRY(cEnum.Attach(new CComObject<CComEnumConnections>));
    if (cEnum == NULL)
      return E_OUTOFMEMORY;
    nItems = 0;
    {
      CNktAutoFastMutex cLock(&cMtx);
      TNktLnkLst<CItem>::Iterator it;
      CItem *lpItem;

      nCount = 0;
      for (lpItem=it.Begin(cList); lpItem!=NULL; lpItem=it.Next())
        nCount++;
      cCd.Attach(new CONNECTDATA[(nCount > 0) ? nCount : 1]);
      if (cCd == NULL)
        return E_OUTOFMEMORY;
      //duplicate streams
      hRes = S_OK;
      for (lpItem=it.Begin(cList); lpItem!=NULL; lpItem=it.Next())
      {
        hRes = lpItem->cStream->Clone((IStream**)&(cCd[nItems].pUnk));
        if (FAILED(hRes))
          break;
        cCd[nItems].dwCookie = lpItem->dwCookie;
        nItems++;
      }
    }
    if (FAILED(hRes))
    {
      for (i=0; i<nItems; i++)
        ((IStream*)(cCd[nItems].pUnk))->Release();
      return hRes;
    }
    for (i=0; i<nItems; i++)
    {
      cStream.Release();
      cStream.Attach((IStream*)(cCd[i].pUnk));
      li.QuadPart = 0;
      hRes = cStream->Seek(li, SEEK_SET, NULL);
      if (SUCCEEDED(hRes))
      {
        p.Release();
        hRes = ::CoUnmarshalInterface(cStream, IID_IDispatch, (void**)&p);
      }
      if (SUCCEEDED(hRes))
        hRes = p->QueryInterface<IUnknown>(&(cCd[i].pUnk));
      if (FAILED(hRes))
      {
        for (k=0; k<nItems; k++)
        {
          if (k < i)
          {
            cCd[k].pUnk->Release(); //pUnk are real IUnknown interfaces
          }
          else if (k > i)
          {
            ((IStream*)(cCd[k].pUnk))->Release(); //pUnk are IStream interfaces
          }
        }
        return hRes;
      }
    }
    //don't copy the data, but transfer ownership to it
    {
      CONNECTDATA *lpCd = cCd.Detach();
      cEnum->Init(lpCd, lpCd+nItems, NULL, AtlFlagTakeOwnership);
    }
    hRes = cEnum->_InternalQueryInterface(__uuidof(IEnumConnections), (void**)ppEnum);
    return hRes;
    };

  HRESULT GetConnAt(__deref_out IDispatch **lplpDisp, __in int nIndex)
    {
    T* pT = static_cast<T*>(this);
    TNktComPtr<IStream> cStream;
    LARGE_INTEGER li;
    HRESULT hRes;

    _ASSERT(lplpDisp != NULL);
    *lplpDisp = NULL;
    {
      CNktAutoFastMutex cLock(&cMtx);
      TNktLnkLst<CItem>::Iterator it;
      CItem *lpItem;

      hRes = E_INVALIDARG;
      for (lpItem=it.Begin(cList); lpItem!=NULL; lpItem=it.Next())
      {
        if (nIndex == 0)
        {
          hRes = lpItem->cStream->Clone(&cStream);
          break;
        }
        nIndex--;
      }
    }
    if (SUCCEEDED(hRes))
    {
      li.QuadPart = 0;
      hRes = cStream->Seek(li, SEEK_SET, NULL);
    }
    if (SUCCEEDED(hRes))
      hRes = ::CoUnmarshalInterface(cStream, IID_IDispatch, (void**)lplpDisp);
    if (FAILED(hRes))
      return (hRes == E_INVALIDARG) ? E_INVALIDARG : E_FAIL;
    return S_OK;
    };

  //NOTE: May return DISP_E_MEMBERNOTFOUND
  HRESULT FireCommon(__inout IDispatch *lpConnDisp, __in DISPID nDispId, __in CComVariant aVarParams[],
                     __in int aVarParamsCount)
    {
    DISPPARAMS sParams = { aVarParams, NULL, aVarParamsCount, 0 };
    CComVariant cVarResult;
    HRESULT hRes;
    UINT nArgErr;

    try
    {
      hRes = lpConnDisp->Invoke(nDispId, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &sParams,
                                &cVarResult, NULL, &nArgErr);
    }
    catch (...)
    {
      hRes = E_FAIL;
    }
    return hRes;
    };

private:
  class CItem : public TNktLnkLstNode<CItem>
  {
  public:
    ~CItem()
      {
      if (cStream != NULL)
      {
        LARGE_INTEGER li;

        li.QuadPart = 0;
        cStream->Seek(li, SEEK_SET, NULL);
        ::CoReleaseMarshalData(cStream);
      }
      return;
      };

  public:
    DWORD dwCookie;
    TNktComPtr<IDispatch> cObj;
    TNktComPtr<IStream> cStream;
  };

  CNktFastMutex cMtx;
  TNktLnkLst<CItem> cList;
  LONG volatile nCookie;
};
