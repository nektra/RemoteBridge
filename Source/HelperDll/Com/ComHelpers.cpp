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

#include "ComHelpers.h"
#include <stdio.h>

//-----------------------------------------------------------

namespace COM {

SIZE_T* GetInterfaceVTable(__in IUnknown *lpUnk)
{
  try
  {
    if (lpUnk != NULL)
      return (SIZE_T*)(*((SIZE_T*)lpUnk));
  }
  catch (...)
  {
  }
  return NULL;
}

BOOL IsInterfaceDerivedFrom(__in IUnknown *lpUnk, __in REFIID sIid)
{
  IUnknown *lpUnkBase;

  if (SUCCEEDED(lpUnk->QueryInterface(sIid, (LPVOID*)&lpUnkBase)))
  {
    lpUnkBase->Release();
    return TRUE;
  }
  return FALSE;
}

LPWSTR GetInterfaceName(__in REFGUID sGuid, __in_z LPCWSTR szClassW)
{
  WCHAR szBufW[256], *sW;
  HKEY hKey;
  LONG k;

  swprintf_s(szBufW, 256, L"%s\\{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", szClassW, sGuid.Data1,
             sGuid.Data2, sGuid.Data3, sGuid.Data4[0], sGuid.Data4[1], sGuid.Data4[2],
             sGuid.Data4[3], sGuid.Data4[4], sGuid.Data4[5], sGuid.Data4[6], sGuid.Data4[7]);
  if (::RegOpenKeyExW(HKEY_CLASSES_ROOT, szBufW, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    return NULL;
  szBufW[0] = 0;
  k = 256;
  if (::RegQueryValue(hKey, NULL, szBufW, &k) != ERROR_SUCCESS)
  {
    ::RegCloseKey(hKey);
    return NULL;
  }
  ::RegCloseKey(hKey);
  k = (LONG)((wcslen(szBufW)+1)*sizeof(WCHAR));
  sW = (LPWSTR)::CoTaskMemAlloc((SIZE_T)(ULONG)k);
  if (sW == NULL)
    return NULL;
  memcpy(sW, szBufW, k);
  return sW;
}

BOOL IsProxyDisconnected(__in HRESULT hRes)
{
  return (hRes == CO_E_OBJNOTCONNECTED || hRes == CO_E_OBJNOTREG || hRes == RPC_E_SERVER_DIED_DNE ||
          hRes ==  RPC_E_DISCONNECTED || hRes == RPC_E_SERVER_DIED) ? TRUE : FALSE;
}

ULONG GetIUnknownRefCount(__in IUnknown *lpUnk)
{
  ULONG nRef;

  __try
  {
    nRef = lpUnk->AddRef();
    if (nRef > 1)
      nRef = lpUnk->Release();
    else
      nRef = ULONG_MAX;
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    nRef = ULONG_MAX;
  }
  return nRef;
}

} //COM
