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
#include "..\..\Common\LinkedList.h"
#include "..\..\Common\ComPtr.h"
#include "..\..\Common\WaitableObjects.h"
#include "ComGuidList.h"

//-----------------------------------------------------------

#define IID_IS_IDispatch                              0x0001
#define IID_IS_IOleWindow                             0x0002
#define IID_IS_IWebBrowser2                           0x0004

//-----------------------------------------------------------

namespace COM {

class CMainManager;
class CDll;
class CClassFactory;

//----------------

class CInterfaceMethod : public TNktLnkLstNode<CInterfaceMethod>, public CNktFastMutex
{
public:
  IID sIid;
  ULONG nMethodIndex;
  ULONG nIidIndex;
  IID sQueryIid;
  ULONG nReturnParameterIndex;
};

//----------------

class CInterface : public TNktLnkLstNode<CInterface>, public CNktFastMutex
{
public:
  CInterface(__in CMainManager *lpMgr, __in CDll *lpDll, __in SIZE_T *lpVTable, __in_opt CClassFactory *lpClassFac);
  ~CInterface();

  HRESULT Initialize(__in IUnknown *lpUnk);
  VOID Finalize();

  ULONG GetFlags()
    {
    return nFlags;
    };

  CDll* GetDll()
    {
    return lpDll;
    };

  SIZE_T* GetVTable()
    {
    return lpVTable;
    };

  CClassFactory* GetClassFactory()
    {
    return lpClassFac;
    };

  HRESULT AddSupportedIid(__in REFIID riid);
  HRESULT GetSupportedIiAt(__in SIZE_T nIndex, __out GUID &sGuid);

  HRESULT OnIUnknown_QueryInterface(__in IUnknown *lpThis, __in REFIID riid, __out void **ppvObject);
  ULONG OnIUnknown_AddRef(__in IUnknown *lpThis);
  ULONG OnIUnknown_Release(__in IUnknown *lpThis);
  HRESULT OnIUnknown_Generic(__in CInterfaceMethod *lpIntMethodData, __in SIZE_T *lpParameters,
                             __in SIZE_T nReturnValue);

  HRESULT OnNewInspectInterfaceMethod(__in CInterfaceMethod *lpInterfaceMethod);

  static VOID DebugOutput_Interface(__in REFIID riid, __in SIZE_T lpVTable, __in CInterface *lpInterface,
                                    __in CClassFactory *lpClassFac, __in_z LPCSTR szSuffixA);
  static VOID DebugOutput_InterfaceCreated(__in REFIID riid, __in CInterface *lpInterface, __in SIZE_T lpVTable);

public:
  class CHookedMethod : public TNktLnkLstNode<CHookedMethod>
  {
  public:
    CInterfaceMethod* lpInterfaceMethod;
    CNktHookLib::HOOK_INFO sHkNfo_IUnkMethod;
  };

private:
  friend class CMainManager;
  friend class CInstance;

  static HRESULT __stdcall IUnknown_QueryInterfaceStub(__in CInterface *lpCtx, __in IUnknown *lpThis,
                                                       __in REFIID riid, __deref_out void **ppvObject);
  static ULONG __stdcall IUnknown_AddRefStub(__in CInterface *lpCtx, __in IUnknown *lpThis);
  static ULONG __stdcall IUnknown_ReleaseStub(__in CInterface *lpCtx, __in IUnknown *lpThis);
  static HRESULT __stdcall IUnknown_GenericStub(__in CInterface *lpCtx, __in CInterfaceMethod *lpInterfaceMethod,
                                                __in SIZE_T *lpParameters, __in SIZE_T nReturnValue);

  CMainManager *lpMgr;
  CDll *lpDll;
  SIZE_T *lpVTable;
  ULONG nFlags;
  CClassFactory *lpClassFac;
  CGuidList cSupportedIids;
  CNktHookLib::HOOK_INFO sHkNfo_IUnk[3];
  CNktFastMutex cHookedMethodsListMtx;
  TNktLnkLst<CHookedMethod> cHookedMethodsList;
  CNktFastMutex cRefCounterMtx;
};

} //COM
