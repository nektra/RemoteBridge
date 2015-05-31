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

#ifndef _NKT_AUTOHANDLE_H
#define _NKT_AUTOHANDLE_H

#include <windows.h>

//-----------------------------------------------------------

class CNktAutoHandle
{
public:
  CNktAutoHandle()
    {
    h = NULL;
    return;
    };

  ~CNktAutoHandle()
    {
    Reset();
    return;
    };

  VOID Reset()
    {
    CloseHandleSEH(h);
    h = NULL;
    return;
    };

  HANDLE* operator&()
    {
    _ASSERT(h == NULL || h == INVALID_HANDLE_VALUE);
    return &h;
    };

  operator HANDLE()
    {
    return Get();
    };

  HANDLE Get()
    {
    return (h != NULL && h != INVALID_HANDLE_VALUE) ? h : NULL;
    };

  BOOL operator!() const
    {
    return (h == NULL || h == INVALID_HANDLE_VALUE) ? TRUE : FALSE;
    };

  VOID Attach(__in HANDLE _h)
    {
    Reset();
    if (_h != NULL && _h != INVALID_HANDLE_VALUE)
      h = _h;
    return;
    };

  HANDLE Detach()
    {
    HANDLE hTemp;

    hTemp = (h != INVALID_HANDLE_VALUE) ? h : NULL;
    h = NULL;
    Reset();
    return h;
    };

  static VOID CloseHandleSEH(__in HANDLE h)
    {
    if (h != NULL && h != INVALID_HANDLE_VALUE)
    {
      __try
      {
        ::CloseHandle(h);
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      { }
    }
    return;
    };

protected:
  HANDLE h;
};

//-----------------------------------------------------------

class CNktAutoRemoteHandle
{
public:
  CNktAutoRemoteHandle()
    {
    hProc = h = NULL;
    return;
    };

  ~CNktAutoRemoteHandle()
    {
    Reset();
    return;
    };

  VOID Reset()
    {
    CloseRemoteHandleSEH(hProc, h);
    CNktAutoHandle::CloseHandleSEH(hProc);
    hProc = h = NULL;
    return;
    };

  HRESULT Init(__in DWORD dwPid, __in HANDLE _h)
    {
    DWORD dwOsErr;

    if (dwPid == 0 || _h == NULL || _h == INVALID_HANDLE_VALUE)
      return E_INVALIDARG;
    Reset();
    dwOsErr = NOERROR;
    hProc = ::OpenProcess(SYNCHRONIZE|PROCESS_DUP_HANDLE, FALSE, dwPid);
    if (hProc != NULL && hProc != INVALID_HANDLE_VALUE)
    {
      if (::DuplicateHandle(::GetCurrentProcess(), _h, hProc, &h, 0, FALSE, DUPLICATE_SAME_ACCESS) == FALSE)
        dwOsErr = ::GetLastError();
    }
    else
    {
      dwOsErr = ::GetLastError();
    }
    if (dwOsErr != NOERROR)
    {
      Reset();
      if (dwOsErr == ERROR_NOT_ENOUGH_MEMORY)
        dwOsErr = ERROR_OUTOFMEMORY;
      return HRESULT_FROM_WIN32(dwOsErr);
    }
    return S_OK;
    };

  HRESULT Init(__in HANDLE _hProc, __in HANDLE _h)
    {
    DWORD dwOsErr;

    if (_hProc == NULL || _hProc == INVALID_HANDLE_VALUE ||
        _h == NULL || _h == INVALID_HANDLE_VALUE)
      return E_INVALIDARG;
    Reset();
    dwOsErr = NOERROR;
    if (::DuplicateHandle(::GetCurrentProcess(), _hProc, ::GetCurrentProcess(), &hProc, 0, FALSE,
                          DUPLICATE_SAME_ACCESS) == FALSE ||
        ::DuplicateHandle(::GetCurrentProcess(), _h, _hProc, &h, 0, FALSE,
                          DUPLICATE_SAME_ACCESS) == FALSE)
    {
      dwOsErr = ::GetLastError();
      Reset();
      if (dwOsErr == ERROR_NOT_ENOUGH_MEMORY)
        dwOsErr = ERROR_OUTOFMEMORY;
      return HRESULT_FROM_WIN32(dwOsErr);
    }
    return S_OK;
    };

  operator HANDLE()
    {
    return Get();
    };

  HANDLE Get()
    {
    return h;
    };

  VOID Attach(__in HANDLE _hProc, __in HANDLE _h)
    {
    Reset();
    if (_hProc != NULL && _hProc != INVALID_HANDLE_VALUE)
      hProc = _hProc;
    if (_h != NULL && _h != INVALID_HANDLE_VALUE)
      h = _h;
    return;
    };

  HANDLE Detach()
    {
    HANDLE hTemp;

    hTemp = h;
    h = NULL;
    Reset();
    return hTemp;
    };

  static VOID CloseRemoteHandleSEH(__in HANDLE hProc, __in HANDLE h)
    {
    if (h != NULL && h != INVALID_HANDLE_VALUE &&
        hProc != NULL && hProc != INVALID_HANDLE_VALUE)
    {
      __try
      {
        ::DuplicateHandle(hProc, h, hProc, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE);
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      { }
    }
    return;
    };

protected:
  HANDLE hProc, h;
};

//-----------------------------------------------------------

#endif //_NKT_REMOTEHANDLE_H
