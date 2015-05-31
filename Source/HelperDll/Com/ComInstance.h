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
#include "ComBaseRefCounter.h"

//-----------------------------------------------------------

namespace COM {

class CDll;
class CClassFactory;
class CInterface;

//----------------

class CInstance : public TNktLnkLstNode<CInstance>, public CBaseRefCounter
{
public:
  CInstance(__in CInterface *lpInterface, __in REFIID sIid, __in IUnknown *lpUnkRef);
  ~CInstance();

  HRESULT Initialize(__in IUnknown *lpUnk);

  VOID Invalidate(__in BOOL bReleaseUnk, __in BOOL bReleaseMarshal);
  HRESULT Unmarshal();

  CInterface* GetInterface();
  ULONG GetInterfaceFlags();

  IUnknown* GetUnkRef();

  DWORD GetTid()
    {
    return dwTid;
    };

  BOOL CheckSupportedIid(__in REFIID sIid);

  HRESULT BuildResponsePacket(__in REFIID sIid, __in SIZE_T nPrefixBytes, __out LPVOID *lplpData,
                              __out SIZE_T *lpnDataSize);

private:
  friend class CMainManager;

  CInterface *lpInterface;
  CMainManager *lpMgr;
  IUnknown* volatile lpUnkRef;
  IID sIid;
  TNktComPtr<IUnknown> cUnk;
  TNktComPtr<IStream> cStream;
  DWORD dwTid;
  ULONG nRefAddedByCoMarshal;
};

} //COM
