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
#include "HelperInjector.h"
#include "..\Common\Tools.h"
#include <Aclapi.h>
#include <winternl.h>

//-----------------------------------------------------------

#define X_BYTE_ENC(x, y) (x)
static
#include "Asm\Injector_x86.inl"
#if defined _M_X64
  static
  #include "Asm\Injector_x64.inl"
#endif

#define P_PROC_ACCESS (PROCESS_CREATE_THREAD|PROCESS_SET_INFORMATION|PROCESS_QUERY_INFORMATION|  \
                       PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ|PROCESS_DUP_HANDLE| \
                       STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE)

#ifdef _DEBUG
  #define INJECT_WAIT_TIME 60000
#else //_DEBUG
  #define INJECT_WAIT_TIME 10000
#endif //_DEBUG

//-----------------------------------------------------------

typedef struct {
  ULONG Size;
  ULONG Unknown1;
  ULONG Unknown2;
  PULONG Unknown3;
  ULONG Unknown4;
  ULONG Unknown5;
  ULONG Unknown6;
  PULONG Unknown7;
  ULONG Unknown8;
} NTCREATETHREADEXBUFFER;

typedef LONG (WINAPI *lpfnNtCreateThreadEx)(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess,
                                            LPVOID ObjectAttributes, HANDLE ProcessHandle,
                                            LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
                                            ULONG CreateFlags, SIZE_T ZeroBits, SIZE_T SizeOfStackCommit,
                                            SIZE_T SizeOfStackReserve, LPVOID lpBytesBuffer);

typedef DWORD (WINAPI *lpfnGetSecurityInfo)(__in HANDLE handle, __in  SE_OBJECT_TYPE ObjectType,
                            __in  SECURITY_INFORMATION SecurityInfo, __out_opt PSID *ppsidOwner,
                            __out_opt PSID *ppsidGroup, __out_opt PACL *ppDacl, __out_opt PACL *ppSacl,
                            __out_opt PSECURITY_DESCRIPTOR *ppSecurityDescriptor);

//-----------------------------------------------------------

HRESULT InjectHelper(__in DWORD dwPid, __in INIT_DATA* lpInitData)
{
  LPBYTE lpCodeAddr, lpRemoteCode;
  SIZE_T k, nCodeSize, nDllNameLen, nBytesWritten;
  HANDLE hRemThread, hProc;
  HMODULE hNtDll, hAdvApi32Dll;
  lpfnNtCreateThreadEx fnNtCreateThreadEx;
  lpfnGetSecurityInfo fnGetSecurityInfo;
  WCHAR szDllNameW[4096];
  SECURITY_ATTRIBUTES sSecAttrib;
  SECURITY_DESCRIPTOR sSecDesc, *lpSecDesc;
  OBJECT_ATTRIBUTES sObjAttr;
  DWORD dwTid, dwRes, dwPlatformBits;
  HRESULT hRes;

  _ASSERT(dwPid != 0);
  _ASSERT(lpInitData != NULL);
  _ASSERT(lpInitData->nStructSize >= (ULONG)sizeof(INIT_DATA));
  //open target process
  hProc = ::OpenProcess(P_PROC_ACCESS, FALSE, dwPid);
  if (hProc == NULL || hProc == INVALID_HANDLE_VALUE)
    return NKT_HRESULT_FROM_LASTERROR();
  dwPlatformBits = CNktTools::GetProcessPlatformBits(hProc);
  //get trampoline code
  switch (dwPlatformBits)
  {
    case 32:
      lpCodeAddr = (LPBYTE)aInjectorX86;
      nCodeSize = NKT_ARRAYLEN(aInjectorX86);
      break;
#if defined _M_X64
    case 64:
      lpCodeAddr = (LPBYTE)aInjectorX64;
      nCodeSize = NKT_ARRAYLEN(aInjectorX64);
      break;
#endif //_M_X64
    default:
      ::CloseHandle(hProc);
      _ASSERT(FALSE);
      return E_FAIL;
  }
  //get dll full name
  nDllNameLen = (SIZE_T)::GetModuleFileNameW(hDllInst, szDllNameW, NKT_ARRAYLEN(szDllNameW)-64);
  szDllNameW[nDllNameLen] = 0;
  nDllNameLen = wcslen(szDllNameW);
  while (nDllNameLen > 0 && szDllNameW[nDllNameLen-1] != L'\\' && szDllNameW[nDllNameLen-1] != L'/')
    nDllNameLen--;
  switch (dwPlatformBits)
  {
    case 32:
      memcpy(szDllNameW+nDllNameLen, L"RemoteBridgeHelper.dll", 23*sizeof(WCHAR));
      nDllNameLen = (nDllNameLen+23) * sizeof(WCHAR);
      break;
#if defined _M_X64
    case 64:
      memcpy(szDllNameW+nDllNameLen, L"RemoteBridgeHelper64.dll", 25*sizeof(WCHAR));
      nDllNameLen = (nDllNameLen+25) * sizeof(WCHAR);
      break;
#endif //_M_X64
  }
  //copy trampoline to target process
  k = nCodeSize + (SIZE_T)(lpInitData->nStructSize) + nDllNameLen;
  lpRemoteCode = (LPBYTE)::VirtualAllocEx(hProc, NULL, k, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if (lpRemoteCode == NULL)
  {
    hRes = E_OUTOFMEMORY;
hwp_onerror:
    if (lpRemoteCode != NULL)
      ::VirtualFreeEx(hProc, lpRemoteCode, 0, MEM_RELEASE);
    ::CloseHandle(hProc);
    return hRes;
  }
  if (::WriteProcessMemory(hProc, lpRemoteCode, lpCodeAddr, nCodeSize, &nBytesWritten) == FALSE ||
      nBytesWritten != nCodeSize)
  {
    hRes = E_ACCESSDENIED;
    goto hwp_onerror;
  }
  if (::WriteProcessMemory(hProc, lpRemoteCode+nCodeSize, lpInitData, (SIZE_T)(lpInitData->nStructSize),
                           &nBytesWritten) == FALSE ||
      nBytesWritten != (SIZE_T)(lpInitData->nStructSize))
  {
    hRes = E_ACCESSDENIED;
    goto hwp_onerror;
  }
  if (::WriteProcessMemory(hProc, lpRemoteCode+nCodeSize+(SIZE_T)(lpInitData->nStructSize), szDllNameW,
                           nDllNameLen, &nBytesWritten) == FALSE ||
      nBytesWritten != nDllNameLen)
  {
    hRes = E_ACCESSDENIED;
    goto hwp_onerror;
  }
  //try to build security access dacl
  lpSecDesc = NULL;
  hAdvApi32Dll = ::LoadLibraryW(L"advapi32.dll");
  if (hAdvApi32Dll != NULL)
  {
    fnGetSecurityInfo = (lpfnGetSecurityInfo)::GetProcAddress(hAdvApi32Dll, "GetSecurityInfo");
    if (fnGetSecurityInfo != NULL)
    {
      if (fnGetSecurityInfo(hProc, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL,
                            (PSECURITY_DESCRIPTOR*)&lpSecDesc) != ERROR_SUCCESS)
        lpSecDesc = NULL;
    }
    ::FreeLibrary(hAdvApi32Dll);
  }
  if (lpSecDesc == NULL)
  {
    ::InitializeSecurityDescriptor(&sSecDesc, SECURITY_DESCRIPTOR_REVISION);
    ::SetSecurityDescriptorDacl(&sSecDesc, TRUE, NULL, FALSE);
    lpSecDesc = &sSecDesc;
  }
  sSecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
  sSecAttrib.bInheritHandle = FALSE;
  sSecAttrib.lpSecurityDescriptor = lpSecDesc;
  //execute remote code
  fnNtCreateThreadEx = NULL;
  hNtDll = ::GetModuleHandleW(L"ntdll.dll");
  if (hNtDll != NULL)
    fnNtCreateThreadEx = (lpfnNtCreateThreadEx)::GetProcAddress(hNtDll, "NtCreateThreadEx");
  if (fnNtCreateThreadEx == NULL)
  {
    hRemThread = ::CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)lpRemoteCode,
                                      lpRemoteCode+nCodeSize, 0, &dwTid);
    hRes = (hRemThread != NULL) ? S_OK : NKT_HRESULT_FROM_LASTERROR();
  }
  else
  {
    InitializeObjectAttributes(&sObjAttr, NULL, (sSecAttrib.bInheritHandle != FALSE) ? OBJ_INHERIT : 0,
                               NULL, lpSecDesc);
    /*
    nktMemSet(&sNtCreateThreadExBuf, 0, sizeof(sNtCreateThreadExBuf));
    nktMemSet(dwNtCreateThreadExBufData, 0, sizeof(dwNtCreateThreadExBufData));
    sNtCreateThreadExBuf.Size = sizeof(sNtCreateThreadExBuf);
    sNtCreateThreadExBuf.Unknown1 = 0x10003;
    sNtCreateThreadExBuf.Unknown2 = 0x8;
    sNtCreateThreadExBuf.Unknown3 = &dwNtCreateThreadExBufData[0];
    sNtCreateThreadExBuf.Unknown5 = 0x10004;
    sNtCreateThreadExBuf.Unknown6 = 4;
    sNtCreateThreadExBuf.Unknown7 = &dwNtCreateThreadExBufData[1];
    */
    hRes = HRESULT_FROM_NT(fnNtCreateThreadEx(&hRemThread, 0x001FFFFFUL, &sObjAttr, hProc,
                                              (LPTHREAD_START_ROUTINE)lpRemoteCode,
                                              lpRemoteCode+nCodeSize, 0, 0, NULL, NULL, NULL));
    if (FAILED(hRes))
      hRemThread = NULL;
  }
  if (lpSecDesc != &sSecDesc)
    ::LocalFree(lpSecDesc);
  //execute remote code
  if (FAILED(hRes))
    goto hwp_onerror;
  ::SetThreadPriority(hRemThread, THREAD_PRIORITY_HIGHEST);
  //wait... a race condition may arise: user may be logged on before hook is done
  //should be infinite? what happens if the thread is abnormally terminated?
  dwRes = ::WaitForSingleObject(hRemThread, INJECT_WAIT_TIME);
  if (dwRes != WAIT_OBJECT_0)
  {
    //don't free memory if wait failed to avoid possible hang in target process
    lpRemoteCode = NULL;
    ::CloseHandle(hRemThread);
    hRes = E_FAIL;
    goto hwp_onerror;
  }
  ::GetExitCodeThread(hRemThread, &dwRes);
  hRes = (HRESULT)dwRes;
  if (FAILED(hRes))
    goto hwp_onerror;
  ::VirtualFreeEx(hProc, lpRemoteCode, 0, MEM_RELEASE);
  ::CloseHandle(hRemThread);
  ::CloseHandle(hProc);
  return S_OK;
}
