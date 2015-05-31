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
#include <Ole2.h>
#include <OCIdl.h>
#include <OleCtl.h>

//-----------------------------------------------------------

namespace COM {

class CMainManager;
class CDll;
class CClassFactory;
class CInterface;

//----------------

class CClassFactory : public TNktLnkLstNode<CClassFactory>, public CNktFastMutex
{
public:
  CClassFactory(__in CMainManager *lpMgr, __in CDll *lpDll, __in SIZE_T *lpVTable);
  ~CClassFactory();

  VOID Finalize();

  SIZE_T* GetVTable()
    {
    return lpVTable;
    };

  HRESULT AddSupportedClsId(__in REFCLSID rclsid);
  HRESULT GetSupportedClsIdAt(__in SIZE_T nIndex, __out GUID &sGuid);

  HRESULT HookIClassFactory();
  HRESULT HookIClassFactory2();

  HRESULT OnIClassFactory_CreateInstance(__in IClassFactory *lpThis, __in IUnknown *pUnkOuter,
                                         __in REFIID riid, __deref_out void **ppvObject);
  HRESULT OnIClassFactory2_CreateInstance(__in IClassFactory2 *lpThis, __in IUnknown *pUnkOuter,
                                          __in REFIID riid, __deref_out void **ppvObject);
  HRESULT OnIClassFactory2_CreateInstanceLic(__in IClassFactory2 *lpThis, __in IUnknown *pUnkOuter,
                                             __in IUnknown *pUnkReserved, __in REFIID riid,
                                             __in BSTR bstrKey, __deref_out PVOID *ppvObject);

  static VOID DebugOutput_ClassFactory(__in REFCLSID rclsid, __in LPCSTR szSuffixA);

private:
  friend class CMainManager;

  static HRESULT __stdcall IClassFactory_CreateInstanceStub(__in CClassFactory *lpCtx, __in IClassFactory *lpThis,
                                                            __in IUnknown *pUnkOuter, __in REFIID riid,
                                                            __deref_out void **ppvObject);
  static HRESULT __stdcall IClassFactory2_CreateInstanceStub(__in CClassFactory *lpCtx, __in IClassFactory2 *lpThis,
                                                             __in IUnknown *pUnkOuter, __in REFIID riid,
                                                             __deref_out void **ppvObject);
  static HRESULT __stdcall IClassFactory2_CreateInstanceLicStub(__in CClassFactory *lpCtx, __in IClassFactory2 *lpThis,
                                                                __in IUnknown *pUnkOuter, __in IUnknown *pUnkReserved,
                                                                __in REFIID riid, __in BSTR bstrKey,
                                                                __deref_out PVOID *ppvObject);

  CMainManager *lpMgr;
  CDll *lpDll;
  SIZE_T *lpVTable;
  CGuidList cSupportedClsIds;
  CNktHookLib::HOOK_INFO sHkNfo_IClassFactoryS[3];
};

} //COM
