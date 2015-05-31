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

#ifndef _NKT_TOOLS_H
#define _NKT_TOOLS_H

#include <windows.h>
#include <objbase.h>
#include <oaidl.h>
#include <ocidl.h>
#include <comip.h>
#include <ole2.h>
#include "StringLiteW.h"

//-----------------------------------------------------------

class CNktTools
{
public:
  typedef union {
    IMAGE_NT_HEADERS32 u32;
    IMAGE_NT_HEADERS64 u64;
  } NKT_IMAGE_NT_HEADER;

  static HRESULT SetProcessPrivilege(__in DWORD dwPid, __in_z LPCWSTR szPrivilegeW, __in BOOL bEnable);
  static HRESULT SetProcessPrivilege(__in HANDLE hProcess, __in_z LPCWSTR szPrivilegeW, __in BOOL bEnable);

  static HRESULT GetProcessPrivilege(__out LPBOOL lpbEnabled, __in DWORD dwPid, __in_z LPCWSTR szPrivilegeW);
  static HRESULT GetProcessPrivilege(__out LPBOOL lpbEnabled, __in HANDLE hProcess, __in_z LPCWSTR szPrivilegeW);

  static LPWSTR stristrW(__in_z LPCWSTR szStringW, __in_z LPCWSTR szToSearchW);

  static HRESULT wstr2ulong(__out ULONG *lpnValue, __in_z LPCWSTR szStringW);

  static LPSTR Wide2Ansi(__in_nz_opt LPCWSTR szSrcW, __in SIZE_T nSrcLen=(SIZE_T)-1);

  static VOID DoEvents(__in BOOL bKeepQuitMsg);

  static VOID DoMessageLoop();

  static DWORD GetProcessPlatformBits(__in HANDLE hProcess);

  static HRESULT CheckImageType(__out NKT_IMAGE_NT_HEADER *lpNtHdr, __in HANDLE hProcess, __in LPBYTE lpBaseAddr,
                                __out_opt LPBYTE *lplpNtHdrAddr=NULL);

  static HRESULT SuspendAfterCreateProcessW(__out LPHANDLE lphReadyExecutionEvent,
                                            __out LPHANDLE lphContinueExecutionEvent, __in HANDLE hParentProc,
                                            __in HANDLE hSuspendedProc, __in HANDLE hSuspendedMainThread);

  static HRESULT GetSystem32Path(__inout CNktStringW &cStrPathW);

  static HRESULT FixFolderStr(__inout CNktStringW &cStrPathW);

  static HRESULT GetModuleUsageCount(__in HINSTANCE hDll, __out SIZE_T &nModuleSize);
  static SIZE_T GetModuleSize(__in HINSTANCE hDll);

  static HRESULT _CreateProcess(__out DWORD &dwPid, __in_z_opt LPCWSTR szImagePathW, __in BOOL bSuspended,
                                __out_opt LPHANDLE lphProcess=NULL, __out_opt LPHANDLE lphContinueEvent=NULL);
  static HRESULT _CreateProcessWithLogon(__out DWORD &dwPid, __in_z LPCWSTR szImagePathW, __in_z LPCWSTR szUserNameW,
                               __in_z LPCWSTR szPasswordW, __in BOOL bSuspended, __out_opt LPHANDLE lphProcess=NULL,
                               __out_opt LPHANDLE lphContinueEvent=NULL);

  static DWORD GetThreadId(__in HANDLE hThread);
};

//-----------------------------------------------------------

#endif //_NKT_TOOLS_H
