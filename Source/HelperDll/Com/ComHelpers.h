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
#include "..\AutoTls.h"
#include <Ole2.h>

//-----------------------------------------------------------

namespace COM {

class CAutoUnkown
{
public:
  CAutoUnkown(__in IUnknown *_lpUnk, __in DWORD _dwTlsIndex)
    {
    dwTlsIndex = _dwTlsIndex;
    {
      CAutoTls cAutoTls(_dwTlsIndex, (LPVOID)-1);

      if (FAILED(_lpUnk->QueryInterface(IID_IUnknown, (LPVOID*)&lpUnk)))
        lpUnk = NULL;
    }
    return;
    };

  ~CAutoUnkown()
    {
    Release();
    return;
    };

  IUnknown* GetUnk()
    {
    return lpUnk;
    };

  ULONG Release()
    {
    ULONG nRefCount = ULONG_MAX;
    {
      CAutoTls cAutoTls(dwTlsIndex, (LPVOID)-1);

      if (lpUnk != NULL)
      {
        nRefCount = lpUnk->Release();
        lpUnk = NULL;
      }
    }
    return nRefCount;
    };

private:
  DWORD dwTlsIndex;
  LPVOID lpOrigValue;
  IUnknown *lpUnk;
};

//-----------------------------------------------------------

SIZE_T* GetInterfaceVTable(__in IUnknown *lpUnk);

BOOL IsInterfaceDerivedFrom(__in IUnknown *lpUnk, __in REFIID sIid);

LPWSTR GetInterfaceName(__in REFGUID sGuid, __in_z LPCWSTR szClassW);

BOOL IsProxyDisconnected(__in HRESULT hRes);
ULONG GetIUnknownRefCount(__in IUnknown *lpUnk);

} //COM
