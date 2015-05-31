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

//-----------------------------------------------------------

#define CLASSFAC_BIN_COUNT                                17
#define INTERFACE_BIN_COUNT                               47

//-----------------------------------------------------------

namespace COM {

class CMainManager;
class CClassFactory;
class CInterface;
class CInterfaceMethod;

//----------------

class CDll : public TNktLnkLstNode<CDll>, public CNktFastMutex
{
public:
  CDll(__in CMainManager *lpMgr, __in HINSTANCE hDll);
  ~CDll();

  HRESULT Initialize();
  VOID Finalize();

  HINSTANCE GetHInstance()
    {
    return hDll;
    };
  SIZE_T GetDllSize()
    {
    return nDllSize;
    };

  HRESULT OnDllGetClassObject(__in REFCLSID rclsid, __in REFIID riid, __out LPVOID *ppv);

  HRESULT FindOrCreateClassFactory(__in IUnknown *lpUnk, __in REFCLSID rclsid,
                                   __deref_out CClassFactory **lplpClassFac);
  HRESULT FindOrCreateInterface(__in_opt CClassFactory *lpClassFac, __in IUnknown *lpUnk, __in REFIID riid,
                                __deref_out CInterface **lplpInterface);

  HRESULT OnNewInspectInterfaceMethod(__in CInterfaceMethod *lpInterfaceMethod);

private:
  static HRESULT __stdcall DllGetClassObjectStub(__in CDll *lpCtx, __in REFCLSID rclsid, __in REFIID riid,
                                                 __out LPVOID *ppv);

  CMainManager *lpMgr;
  HINSTANCE hDll;
  SIZE_T nDllSize;
  CNktHookLib::HOOK_INFO sHkNfo_DllGetClassObject;
  struct {
    CNktFastMutex cMtx;
    TNktLnkLst<CClassFactory> cList;
  } sClassFactories[CLASSFAC_BIN_COUNT];
  struct {
    CNktFastMutex cMtx;
    TNktLnkLst<CInterface> cList;
  } sInterfaces[INTERFACE_BIN_COUNT];
};

} //COM
