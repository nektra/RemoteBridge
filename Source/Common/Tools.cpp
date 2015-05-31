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

#include "Tools.h"
#include <olectl.h>
#include <sddl.h>
#include <psapi.h>
#include <ShellAPI.h>
#include <process.h>
#include <intrin.h>
#include "AutoPtr.h"
#include "WaitableObjects.h"
#include "MallocMacros.h"

//-----------------------------------------------------------

#define NKT_NTSTATUS_UNSUCCESSFUL                0xC0000001L
#define NKT_NTSTATUS_INFO_LENGTH_MISMATCH        0xC0000004L
#define NKT_NTSTATUS_BUFFER_TOO_SMALL            0xC0000023L

#define NKT_ThreadBasicInformation                         0

//-----------------------------------------------------------

#pragma pack(8)
typedef struct {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} NKT_UNICODE_STRING;

typedef struct tagNKT_LIST_ENTRY {
  struct tagNKT_LIST_ENTRY* Flink;
  struct tagNKT_LIST_ENTRY* Blink;
} NKT_LIST_ENTRY;

typedef struct {
  NKT_LIST_ENTRY InLoadOrderLinks;
  NKT_LIST_ENTRY InMemoryOrderLinks;
  NKT_LIST_ENTRY InInitializationOrderLinks;
  PVOID DllBase;
  PVOID EntryPoint;
  PVOID SizeOfImage;
  NKT_UNICODE_STRING FullDllName;
  NKT_UNICODE_STRING BaseDllName;
  ULONG Flags;
  USHORT LoadCount;
  //following fields may be wrong although not used
  union {
    NKT_LIST_ENTRY HashLinks;
    struct {
      PVOID SectionPointer;
      ULONG CheckSum;
    };
  };
  union {
    struct {
      ULONG TimeDateStamp;
    };
    struct {
      PVOID LoadedImports;
    };
  };
  PVOID EntryPointActivationContext;
  PVOID PatchInformation;
  //structure continues but it is not needed
} NKT_LDR_DATA_TABLE_ENTRY;

typedef struct {
  SIZE_T UniqueProcess;
  SIZE_T UniqueThread;
} NKT_CLIENT_ID;

typedef struct {
  NTSTATUS ExitStatus;
  LPVOID TebBaseAddress;
  NKT_CLIENT_ID ClientId;
  ULONG_PTR AffinityMask;
  LONG Priority;
  LONG BasePriority;
} NKT_THREAD_BASIC_INFORMATION;
#pragma pack()

//-----------------------------------------------------------

typedef NTSTATUS (NTAPI *lpfnLdrLockLoaderLock)(__in ULONG Flags, __out_opt ULONG *Disposition,
                                                __out PULONG_PTR Cookie);
typedef NTSTATUS (NTAPI *lpfnLdrUnlockLoaderLock)(__in ULONG Flags, __inout ULONG_PTR Cookie);
typedef NTSTATUS (NTAPI *lpfnLdrFindEntryForAddress)(__in PVOID Address, __out PVOID *lplpEntry);
typedef NTSTATUS (NTAPI *lpfnRtlEnterCriticalSection)(__inout PVOID lpRtlCritSection);
typedef NTSTATUS (NTAPI *lpfnRtlLeaveCriticalSection)(__inout PVOID lpRtlCritSection);
typedef NTSTATUS (WINAPI *lpfnNtQueryInformationThread)(__in HANDLE ThreadHandle, __in int ThreadInformationClass,
                                                __out_bcount(ThreadInformationLength) PVOID ThreadInformation,
                                                __in ULONG ThreadInformationLength, __out_opt PULONG ReturnLength);
//NOTE: Wow64GetThreadContext is available on Vista x64 or later. for xp one can implement a code
//      similar to this: http://www.nynaeve.net/Code/GetThreadWow64Context.cpp
typedef BOOL (WINAPI *lpfnWow64GetThreadContext)(__in HANDLE hThread, __inout PWOW64_CONTEXT lpContext);
typedef BOOL (WINAPI *lpfnWow64SetThreadContext)(__in HANDLE hThread, __in CONST WOW64_CONTEXT *lpContext);
typedef BOOL (WINAPI *lpfnIsWow64Process)(__in HANDLE hProc, __in PBOOL lpbIsWow64);
typedef VOID (WINAPI *lpfnGetNativeSystemInfo)(__out LPSYSTEM_INFO lpSystemInfo);

//-----------------------------------------------------------

static HINSTANCE hNtDll = NULL;
static lpfnLdrLockLoaderLock fnLdrLockLoaderLock = NULL;
static lpfnLdrUnlockLoaderLock fnLdrUnlockLoaderLock = NULL;
static lpfnLdrFindEntryForAddress fnLdrFindEntryForAddress = NULL;
static lpfnRtlEnterCriticalSection fnRtlEnterCriticalSection = NULL;
static lpfnRtlLeaveCriticalSection fnRtlLeaveCriticalSection = NULL;
static lpfnNtQueryInformationThread fnNtQueryInformationThread = NULL;
static HINSTANCE hkernel32Dll = NULL;
static lpfnWow64GetThreadContext fnWow64GetThreadContext = NULL;
static lpfnWow64SetThreadContext fnWow64SetThreadContext = NULL;
static lpfnIsWow64Process fnIsWow64Process = NULL;
static lpfnGetNativeSystemInfo fnGetNativeSystemInfo = NULL;

static SYSTEM_INFO sSysInfo = { 0 };

//-----------------------------------------------------------

static HRESULT InitRoutines();
static __inline BOOL ReadProcMem(__in HANDLE hProc, __out LPVOID lpDest, __in LPVOID lpSrc, __in SIZE_T nDataSize);
static __inline BOOL IsBufferSmallError(__in HRESULT hRes);
static PRTL_CRITICAL_SECTION LdrGetLoaderLock();

//-----------------------------------------------------------

HRESULT CNktTools::SetProcessPrivilege(__in DWORD dwPid, __in_z LPCWSTR szPrivilegeW, __in BOOL bEnable)
{
  HRESULT hRes;
  HANDLE hProc;

  hProc = ::OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION, FALSE, dwPid);
  if (hProc == NULL)
    return NKT_HRESULT_FROM_LASTERROR();
  hRes = CNktTools::SetProcessPrivilege(hProc, szPrivilegeW, bEnable);
  ::CloseHandle(hProc);
  return hRes;
}

HRESULT CNktTools::SetProcessPrivilege(__in HANDLE hProcess, __in_z LPCWSTR szPrivilegeW, __in BOOL bEnable)
{
  HANDLE hToken;
  LUID sLuid;
  HRESULT hRes;
  TOKEN_PRIVILEGES sTokPriv;

  if (::OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken) == FALSE)
  {
    hRes = NKT_HRESULT_FROM_LASTERROR();
    goto spp_end;
  }
  if (::LookupPrivilegeValueW(NULL, szPrivilegeW, &sLuid) == FALSE)
  {
    hRes = NKT_HRESULT_FROM_LASTERROR();
    goto spp_end;
  }
  //set privilege
  sTokPriv.PrivilegeCount = 1;
  sTokPriv.Privileges[0].Luid = sLuid;
  sTokPriv.Privileges[0].Attributes = (bEnable != FALSE) ? SE_PRIVILEGE_ENABLED : 0;
  if (::AdjustTokenPrivileges(hToken, FALSE, &sTokPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL) == FALSE)
  {
    hRes = NKT_HRESULT_FROM_LASTERROR();
    if (FAILED(hRes))
      goto spp_end;
  }
  else
  {
    hRes = S_OK;
  }
spp_end:
  if (hToken != NULL)
    ::CloseHandle(hToken);
  return hRes;
}

HRESULT CNktTools::GetProcessPrivilege(__out LPBOOL lpbEnabled, __in DWORD dwPid, __in_z LPCWSTR szPrivilegeW)
{
  HRESULT hRes;
  HANDLE hProc;

  if (lpbEnabled == NULL)
    return E_POINTER;
  *lpbEnabled = FALSE;
  hProc = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPid);
  if (hProc == NULL)
    return NKT_HRESULT_FROM_LASTERROR();
  hRes = CNktTools::GetProcessPrivilege(lpbEnabled, hProc, szPrivilegeW);
  ::CloseHandle(hProc);
  return hRes;
}

HRESULT CNktTools::GetProcessPrivilege(__out LPBOOL lpbEnabled, __in HANDLE hProcess, __in LPCWSTR szPrivilegeW)
{
  HANDLE hToken;
  LUID sLuid;
  TOKEN_PRIVILEGES *lpTokPriv;
  DWORD dwOsErr, dwRet, dwPass, dwIndex;

  if (lpbEnabled == NULL)
    return E_POINTER;
  *lpbEnabled = FALSE;
  //open process's token to get security information
  if (::OpenProcessToken(hProcess, TOKEN_READ|TOKEN_QUERY, &hToken) == FALSE)
  {
    dwOsErr = ::GetLastError();
    goto gpp_end;
  }
  if (::LookupPrivilegeValueW(NULL, szPrivilegeW, &sLuid) == FALSE)
  {
    dwOsErr = ::GetLastError();
    goto gpp_end;
  }
  //get user information from token
  dwRet = sizeof(TOKEN_PRIVILEGES);
  for (dwPass=1; dwPass<=2; dwPass++)
  {
    lpTokPriv = (TOKEN_PRIVILEGES*)NKT_MALLOC(dwRet);
    if (lpTokPriv == NULL)
    {
      dwOsErr = ERROR_OUTOFMEMORY;
      goto gpp_end;
    }
    if (::GetTokenInformation(hToken, TokenPrivileges, lpTokPriv, dwRet, &dwRet) != FALSE)
    {
      for (dwIndex=0; dwIndex<lpTokPriv->PrivilegeCount; dwIndex++)
      {
        if (memcmp(&(lpTokPriv->Privileges[dwIndex].Luid), &sLuid, sizeof(sLuid)) == 0)
        {
          if ((lpTokPriv->Privileges[dwIndex].Attributes & (SE_PRIVILEGE_ENABLED|SE_PRIVILEGE_ENABLED_BY_DEFAULT)) != 0)
            *lpbEnabled = TRUE;
          break;
        }
      }
      dwPass = 3; //force loop end
      break;
    }
    dwOsErr = ::GetLastError();
    if (dwOsErr != ERROR_INSUFFICIENT_BUFFER && dwOsErr != ERROR_BUFFER_OVERFLOW)
      goto gpp_end;
    NKT_FREE(lpTokPriv);
    lpTokPriv = NULL;
  }
gpp_end:
  if (lpTokPriv != NULL)
    NKT_FREE(lpTokPriv);
  if (hToken != NULL)
    ::CloseHandle(hToken);
  return NKT_HRESULT_FROM_WIN32(dwOsErr);
}

LPWSTR CNktTools::stristrW(__in_z LPCWSTR szStringW, __in_z LPCWSTR szToSearchW)
{
  LPWSTR cp, s1, s2;

  if (szStringW == NULL || szToSearchW == NULL)
    return NULL;
  if (*szToSearchW == 0)
    return (LPWSTR)szStringW;
  cp = (LPWSTR)szStringW;
  while (*cp != 0)
  {
    s1 = cp;
    s2 = (LPWSTR)szToSearchW;
    while (*s1 != 0 && *s2 != 0 &&
           ::CharLowerW((LPWSTR)(USHORT)(*s1)) == ::CharLowerW((LPWSTR)(USHORT)(*s2)))
    {
      s1++;
      s2++;
    }
    if (*s2 == 0)
      return cp;
    cp++;
  }
  return NULL;
}

HRESULT CNktTools::wstr2ulong(__out ULONG *lpnValue, __in_z LPCWSTR szStringW)
{
  ULONG k;

  _ASSERT(lpnValue != NULL);
  _ASSERT(szStringW != NULL);
  *lpnValue = 0;
  while (*szStringW != 0)
  {
    if (szStringW[0]<L'0' || szStringW[0]>L'9')
      return E_INVALIDARG;
    k = (*lpnValue) * 10;
    if (k < (*lpnValue))
      return NKT_HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    *lpnValue = k + (ULONG)(szStringW[0] - L'0');
    if ((*lpnValue) < k)
      return NKT_HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    szStringW++;
  }
  return S_OK;
}

LPSTR CNktTools::Wide2Ansi(__in_nz_opt LPCWSTR szSrcW, __in SIZE_T nSrcLen)
{
  SIZE_T nDestLen;
  LPSTR szDestA;

  if (szSrcW == NULL)
    szSrcW = L"";
  if (nSrcLen == (SIZE_T)-1)
    nSrcLen = wcslen(szSrcW);
  nDestLen = (SIZE_T)::WideCharToMultiByte(CP_ACP, 0, szSrcW, (int)nSrcLen, NULL, 0, NULL, NULL);
  szDestA = (LPSTR)NKT_MALLOC((nDestLen+1)*sizeof(CHAR));
  if (szDestA == NULL)
    return NULL;
  nDestLen = (SIZE_T)::WideCharToMultiByte(CP_ACP, 0, szSrcW, (int)nSrcLen, szDestA, (int)(nDestLen+1),
                                           NULL, NULL);
  szDestA[nDestLen] = 0;
  return szDestA;
}

VOID CNktTools::DoEvents(__in BOOL bKeepQuitMsg)
{
  MSG sMsg;

  while (::PeekMessage(&sMsg, NULL, 0, 0, PM_NOREMOVE) != FALSE)
  {
    if (bKeepQuitMsg != FALSE && sMsg.message == WM_QUIT)
      break;
    ::PeekMessage(&sMsg, NULL, 0, 0, PM_REMOVE);
    ::TranslateMessage(&sMsg);
    ::DispatchMessage(&sMsg);
  }
  return;
}

VOID CNktTools::DoMessageLoop()
{
  MSG sMsg;

  while (::GetMessage( &sMsg, NULL, 0, 0 ) != FALSE)
  {
    ::TranslateMessage(&sMsg);
    ::DispatchMessage(&sMsg);
  }
  return;
}

DWORD CNktTools::GetProcessPlatformBits(__in HANDLE hProcess)
{
  BOOL bIsWow64;
  DWORD dwBits;

  if (FAILED(InitRoutines()))
    return 0;
  //----
#if defined _M_IX86
  dwBits = 32;
#elif defined _M_X64
  dwBits = 64;
#endif
  switch (sSysInfo.wProcessorArchitecture)
  {
    case PROCESSOR_ARCHITECTURE_AMD64:
    case PROCESSOR_ARCHITECTURE_IA64:
    case PROCESSOR_ARCHITECTURE_ALPHA64:
    //check on 64-bit platforms
    if (fnIsWow64Process != NULL && fnIsWow64Process(hProcess, &bIsWow64) != FALSE)
    {
#if defined _M_IX86
      if (bIsWow64 == FALSE)
        dwBits = 64;
#elif defined _M_X64
      if (bIsWow64 != FALSE)
        dwBits = 32;
#endif
    }
    break;
  }
  return dwBits;
}

HRESULT CNktTools::CheckImageType(__out NKT_IMAGE_NT_HEADER *lpNtHdr, __in HANDLE hProcess, __in LPBYTE lpBaseAddr,
                                  __out_opt LPBYTE *lplpNtHdrAddr)
{
  LPBYTE lpNtHdrSrc;
  WORD wTemp16;
  DWORD dwTemp32;

  if (lpNtHdr == NULL || hProcess == NULL || lpBaseAddr == NULL)
    return E_INVALIDARG;
  //----
  if (ReadProcMem(hProcess, &wTemp16, lpBaseAddr + FIELD_OFFSET(IMAGE_DOS_HEADER, e_magic), sizeof(wTemp16)) == FALSE)
    return E_FAIL;
  if (wTemp16 != IMAGE_DOS_SIGNATURE)
    return E_FAIL;
  //get header offset
  if (ReadProcMem(hProcess, &dwTemp32, lpBaseAddr + FIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew), sizeof(dwTemp32)) == FALSE)
    return E_FAIL;
  lpNtHdrSrc = lpBaseAddr + (SIZE_T)dwTemp32;
  if (lplpNtHdrAddr != NULL)
    *lplpNtHdrAddr = lpNtHdrSrc;
  //check image type
  if (ReadProcMem(hProcess, &wTemp16, lpNtHdrSrc + FIELD_OFFSET(IMAGE_NT_HEADERS32, FileHeader.Machine),
                  sizeof(wTemp16)) == FALSE)
    return E_FAIL;
  switch (wTemp16)
  {
    case IMAGE_FILE_MACHINE_I386:
      if (ReadProcMem(hProcess, lpNtHdr, lpNtHdrSrc, sizeof(lpNtHdr->u32)) == FALSE)
        return E_FAIL;
      //check signature
      if (lpNtHdr->u32.Signature != IMAGE_NT_SIGNATURE)
        return E_FAIL;
      return (HRESULT)32;

    case IMAGE_FILE_MACHINE_AMD64:
      if (ReadProcMem(hProcess, lpNtHdr, lpNtHdrSrc, sizeof(lpNtHdr->u64)) == FALSE)
        return E_FAIL;
      //check signature
      if (lpNtHdr->u64.Signature != IMAGE_NT_SIGNATURE)
        return E_FAIL;
      return (HRESULT)64;
  }
  return E_FAIL;
}

HRESULT CNktTools::SuspendAfterCreateProcessW(__out LPHANDLE lphReadyExecutionEvent,
                                              __out LPHANDLE lphContinueExecutionEvent, __in HANDLE hParentProc,
                                              __in HANDLE hSuspendedProc, __in HANDLE hSuspendedMainThread)
{
  BYTE aTempBuf[256], *lpRemoteCode;
  CNktEvent cContinueEvent, cReadyEvent;
  HANDLE hContinueEventCopy, hReadyEventCopy, hParentProcCopy;
  SIZE_T nOrigIP, nSetEventAddr, nWaitMultObjAddr, nCloseHandleAddr, nWritten;
  HMODULE hKernel32DLL;
  SIZE_T k, m, nNewEntryPointOfs, nNewStubOfs;
  DWORD dwTemp32, dwPlatformBits;
  CONTEXT sThreadCtx;
#if defined _M_X64
  WOW64_CONTEXT sWow64ThreadCtx;
#endif //_M_X64
  HRESULT hRes;

  if (lphReadyExecutionEvent == NULL || lphContinueExecutionEvent == NULL)
    return E_POINTER;
  if (lphReadyExecutionEvent != NULL)
    *lphReadyExecutionEvent = NULL;
  if (lphContinueExecutionEvent != NULL)
    *lphContinueExecutionEvent = NULL;
  hRes = InitRoutines();
  if (FAILED(hRes))
    return hRes;
  //get suspended thread execution context
  dwPlatformBits = CNktTools::GetProcessPlatformBits(hSuspendedProc);
  switch (dwPlatformBits)
  {
    case 32:
#if defined _M_IX86
      sThreadCtx.ContextFlags = CONTEXT_FULL;
      if (::GetThreadContext(hSuspendedMainThread, &sThreadCtx) == FALSE)
        return E_FAIL;
      nOrigIP = sThreadCtx.Eip;
#elif defined _M_X64
      sWow64ThreadCtx.ContextFlags = WOW64_CONTEXT_FULL;
      if (fnWow64GetThreadContext == NULL)
        return E_NOTIMPL;
      if (fnWow64GetThreadContext(hSuspendedMainThread, &sWow64ThreadCtx) == FALSE)
        return NKT_HRESULT_FROM_LASTERROR();
      nOrigIP = sWow64ThreadCtx.Eip;
#endif
      break;

#if defined _M_X64
    case 64:
      sThreadCtx.ContextFlags = CONTEXT_FULL;
      if (::GetThreadContext(hSuspendedMainThread, &sThreadCtx) == FALSE)
        return E_FAIL;
      nOrigIP = sThreadCtx.Rip;
      break;
#endif //_M_X64

    default:
      return E_FAIL;
  }
  //get some API addresses
  switch (dwPlatformBits)
  {
    case 32:
#if defined _M_IX86
      hKernel32DLL = ::GetModuleHandleW(L"kernel32.dll");
      if (hKernel32DLL == NULL)
        return E_FAIL;
      nSetEventAddr = (SIZE_T)::GetProcAddress(hKernel32DLL, "SetEvent");
      nWaitMultObjAddr = (SIZE_T)::GetProcAddress(hKernel32DLL, "WaitForMultipleObjects");
      nCloseHandleAddr = (SIZE_T)::GetProcAddress(hKernel32DLL, "CloseHandle");
#elif defined _M_X64
      //we are targetting a x86 process from a x64 one, different approach must be done
      //because the target process is suspended, the module entry list in the PEB is
      //not initialized, so we will scan the process memory regions until we find some
      //useful data
      LPBYTE lpCurrAddress, lpBaseAddr, lpExpDir, lpFuncAddr;
      NKT_IMAGE_NT_HEADER sNtLdr;
      MEMORY_BASIC_INFORMATION sMbi;
      BOOL bContinue;
      LPDWORD lpdwAddressOfFunctions, lpdwAddressOfNames;
      LPWORD lpwAddressOfNameOrdinals;
      SIZE_T i, nBlockSize, nExpDirSize;
      IMAGE_EXPORT_DIRECTORY sExpDir;
      DWORD dwTemp32;
      WORD wTemp16;
      CHAR szTempNameA[32];
      int nIndex;

      lpCurrAddress = NULL;
      for (bContinue=TRUE; bContinue!=FALSE;)
      {
        nSetEventAddr = nWaitMultObjAddr = nCloseHandleAddr = 0;
        //----
        if (::VirtualQueryEx(hSuspendedProc, lpCurrAddress, &sMbi, sizeof(sMbi)) == 0)
        {
          bContinue = FALSE;
          continue;
        }
        if (sMbi.Type == MEM_MAPPED || sMbi.Type == MEM_IMAGE)
        {
          lpBaseAddr = (LPBYTE)(sMbi.AllocationBase);
          nBlockSize = 0;
          do
          {
            nBlockSize += sMbi.RegionSize;
            lpCurrAddress += sMbi.RegionSize;
            if (::VirtualQueryEx(hSuspendedProc, lpCurrAddress, &sMbi, sizeof(sMbi)) == 0)
            {
              bContinue = FALSE;
              break;
            }
          }
          while (lpBaseAddr == (LPBYTE)(sMbi.AllocationBase));
          //check if there is a library here
          hRes = CNktTools::CheckImageType(&sNtLdr, hSuspendedProc, lpBaseAddr);
          if (hRes != (HRESULT)32)
            continue;
          //found a 32-bit dll, now try to find needed exports
          if (sNtLdr.u32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0 ||
              sNtLdr.u32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0)
            continue; //empty or no export table in module
          lpExpDir = lpBaseAddr + (SIZE_T)(sNtLdr.u32.OptionalHeader.DataDirectory[
                                              IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
          nExpDirSize = (SIZE_T)(sNtLdr.u32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
          //get export directory info
          if (ReadProcMem(hSuspendedProc, &sExpDir, lpExpDir, sizeof(sExpDir)) == FALSE)
            continue;
          lpdwAddressOfFunctions = (LPDWORD)(lpBaseAddr + (SIZE_T)(sExpDir.AddressOfFunctions));
          lpdwAddressOfNames = (LPDWORD)(lpBaseAddr + (SIZE_T)(sExpDir.AddressOfNames));
          lpwAddressOfNameOrdinals = (LPWORD)(lpBaseAddr + (SIZE_T)(sExpDir.AddressOfNameOrdinals));
          //process the exported names
          for (i=0; i<(SIZE_T)(sExpDir.NumberOfNames); i++)
          {
            //get the name address
            if (ReadProcMem(hSuspendedProc, &dwTemp32, lpdwAddressOfNames+i, sizeof(dwTemp32)) == FALSE ||
                dwTemp32 == NULL)
              continue;
            //get up to NKT_DV_ARRAYLEN(szTempNameA) chars of the name
            if (ReadProcMem(hSuspendedProc, szTempNameA, lpBaseAddr+(SIZE_T)dwTemp32, 32) == FALSE)
              continue;
            if (strcmp(szTempNameA, "NtSetEvent") == 0)
              nIndex = 0;
            else if (strcmp(szTempNameA, "NtWaitForMultipleObjects") == 0)
              nIndex = 1;
            else if (strcmp(szTempNameA, "NtClose") == 0)
              nIndex = 2;
            else
              continue;
            //get the ordinal
            if (ReadProcMem(hSuspendedProc, &wTemp16, lpwAddressOfNameOrdinals+i, sizeof(wTemp16)) == FALSE)
              continue;
            if ((DWORD)wTemp16 >= sExpDir.NumberOfFunctions)
              continue;
            //get address of function
            if (ReadProcMem(hSuspendedProc, &dwTemp32, lpdwAddressOfFunctions+(SIZE_T)wTemp16,
                            sizeof(DWORD)) == FALSE ||
                dwTemp32 == NULL)
              continue;
            //retrieve function address
            lpFuncAddr = lpBaseAddr + (SIZE_T)dwTemp32;
            if (lpFuncAddr >= lpExpDir && lpFuncAddr < lpExpDir+nExpDirSize)
            {
              //it is a forward declaration, ignore
              continue;
            }
            //got a function
            switch (nIndex)
            {
              case 0:
                nSetEventAddr = (SIZE_T)lpFuncAddr;
                break;
              case 1:
                nWaitMultObjAddr = (SIZE_T)lpFuncAddr;
                break;
              case 2:
                nCloseHandleAddr = (SIZE_T)lpFuncAddr;
                break;
            }
          }
          if (nSetEventAddr != 0 && nWaitMultObjAddr != 0 && nCloseHandleAddr != 0)
            break; //found needed ntdll.dll exported functions
        }
        else
        {
          //skip block
          lpCurrAddress += sMbi.RegionSize;
        }
      }
#endif
      break;

#if defined _M_X64
    case 64:
      hKernel32DLL = ::GetModuleHandleW(L"kernel32.dll");
      if (hKernel32DLL == NULL)
        return E_FAIL;
      nSetEventAddr = (SIZE_T)::GetProcAddress(hKernel32DLL, "SetEvent");
      nWaitMultObjAddr = (SIZE_T)::GetProcAddress(hKernel32DLL, "WaitForMultipleObjects");
      nCloseHandleAddr = (SIZE_T)::GetProcAddress(hKernel32DLL, "CloseHandle");
      break;
#endif //_M_X64
  }
  if (nSetEventAddr == 0 || nWaitMultObjAddr == 0 || nCloseHandleAddr == 0)
    return E_FAIL;
  //create "ready" and "continue" events
  if (cReadyEvent.Create(NULL, FALSE) == FALSE ||
      cContinueEvent.Create(NULL, FALSE) == FALSE)
    return E_OUTOFMEMORY;
  if (::DuplicateHandle(::GetCurrentProcess(), cReadyEvent.GetEventHandle(), hSuspendedProc, &hReadyEventCopy, 0,
                        FALSE, DUPLICATE_SAME_ACCESS) == FALSE ||
      ::DuplicateHandle(::GetCurrentProcess(), cContinueEvent.GetEventHandle(), hSuspendedProc, &hContinueEventCopy, 0,
                        FALSE, DUPLICATE_SAME_ACCESS) == FALSE ||
      ::DuplicateHandle(::GetCurrentProcess(), hParentProc, hSuspendedProc, &hParentProcCopy, 0, FALSE,
                        DUPLICATE_SAME_ACCESS) == FALSE)
    return NKT_HRESULT_FROM_LASTERROR();
  //create remote code
  lpRemoteCode = (LPBYTE)::VirtualAllocEx(hSuspendedProc, NULL, 1024, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if (lpRemoteCode == NULL)
    return E_OUTOFMEMORY;
  //write new stub
  m = 0;
  switch (dwPlatformBits)
  {
    case 32:
#if defined _M_IX86
      //data
      memset(aTempBuf+m, 0x00, 4);  m+=4; //will contain the entrypoint to jump to
      memcpy(aTempBuf+m, &hParentProcCopy, 4);  m+=4;
      memcpy(aTempBuf+m, &hContinueEventCopy, 4);  m+=4;
      //new entrypoint starts here
      nNewEntryPointOfs = m;
      //push READY EVENT HANDLE
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &hReadyEventCopy, 4);  m+=4;
      //mov eax, ADDRESS OF SetEvent
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nSetEventAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //push READY EVENT HANDLE
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &hReadyEventCopy, 4);  m+=4;
      //mov eax, ADDRESS OF CloseHandle
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //push INFINITE
      aTempBuf[m++] = 0x68;  memset(aTempBuf+m, 0xFF, 4);  m+=4;
      //push FALSE
      aTempBuf[m++] = 0x68;  memset(aTempBuf+m, 0x00, 4);  m+=4;
      //push address of array of ENGINE PROC HANDLE and CONTINUE EVENT HANDLE
      k = (SIZE_T)lpRemoteCode + 4;
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      //push 2
      k = 2;
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      //mov eax, ADDRESS OF WaitForMultipleObjects
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nWaitMultObjAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //push CONTINUE EVENT HANDLE
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &hContinueEventCopy, 4);  m+=4;
      //mov eax, ADDRESS OF CloseHandle
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //push ENGINE PROC HANDLE
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &hParentProcCopy, 4);  m+=4;
      //mov eax, ADDRESS OF CloseHandle
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //jmp dword ptr [lpRemoteCode] (absolute)
      dwTemp32 = (DWORD)lpRemoteCode;
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0x25;  memcpy(aTempBuf+m, &dwTemp32, 4);  m+=4;
      //NEW STUB
      //at this point eax has the original entry point, store it in lpRemoteCode using interlocked
      nNewStubOfs = m;
      //push ecx
      aTempBuf[m++] = 0x51;
      //mov ecx, lpRemoteCode
      k = (SIZE_T)lpRemoteCode;
      aTempBuf[m++] = 0xB9;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      //lock xchg eax, [ecx]
      aTempBuf[m++] = 0xF0;  aTempBuf[m++] = 0x87;  aTempBuf[m++] = 0x01;
      //mov eax, NEW ENTRY POINT
      k = (SIZE_T)lpRemoteCode + nNewEntryPointOfs;
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      //pop ecx
      aTempBuf[m++] = 0x59;
      //jmp to original eip
      k = (SIZE_T)lpRemoteCode + (m+6);
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0x25;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      memcpy(aTempBuf+m, &nOrigIP, 4);  m+=4;
#elif defined _M_X64
      //data
      memset(aTempBuf+m, 0x00, 4);  m+=4; //will contain the entrypoint to jump to
      memcpy(aTempBuf+m, &hParentProcCopy, 4);  m+=4;
      memcpy(aTempBuf+m, &hContinueEventCopy, 4);  m+=4;
      //new entrypoint starts here
      nNewEntryPointOfs = m;
      //push NULL
      aTempBuf[m++] = 0x68;  memset(aTempBuf+m, 0x00, 4);  m+=4;
      //push READY EVENT HANDLE
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &hReadyEventCopy, 4);  m+=4;
      //mov eax, ADDRESS OF NtSetEvent
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nSetEventAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //push READY EVENT HANDLE
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &hReadyEventCopy, 4);  m+=4;
      //mov eax, ADDRESS OF NtClose
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //push NULL
      aTempBuf[m++] = 0x68;  memset(aTempBuf+m, 0x00, 4);  m+=4;
      //push FALSE
      aTempBuf[m++] = 0x68;  memset(aTempBuf+m, 0x00, 4);  m+=4;
      //push WaitAnyObject
      dwTemp32 = 1;
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &dwTemp32, 4);  m+=4;
      //push address of array of ENGINE PROC HANDLE and CONTINUE EVENT HANDLE
      k = (SIZE_T)lpRemoteCode + 4;
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      //push 2
      dwTemp32 = 2;
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &dwTemp32, 4);  m+=4;
      //mov eax, ADDRESS OF NtWaitForMultipleObjects
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nWaitMultObjAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //push CONTINUE EVENT HANDLE
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &hContinueEventCopy, 4);  m+=4;
      //mov eax, ADDRESS OF NtClose
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //push ENGINE PROC HANDLE
      aTempBuf[m++] = 0x68;  memcpy(aTempBuf+m, &hParentProcCopy, 4);  m+=4;
      //mov eax, ADDRESS OF NtClose
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 4);  m+=4;
      //call eax
      aTempBuf[m++] = 0xFF;  aTempBuf[m++] = 0xD0;
      //jmp dword ptr [lpRemoteCode] (absolute)
      dwTemp32 = (DWORD)lpRemoteCode;
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0x25;  memcpy(aTempBuf+m, &dwTemp32, 4);  m+=4;
      //NEW STUB
      //at this point eax has the original entry point, store it in lpRemoteCode using interlocked
      nNewStubOfs = m;
      //push ecx
      aTempBuf[m++] = 0x51;
      //mov ecx, lpRemoteCode
      k = (SIZE_T)lpRemoteCode;
      aTempBuf[m++] = 0xB9;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      //lock xchg eax, [ecx]
      aTempBuf[m++] = 0xF0;  aTempBuf[m++] = 0x87;  aTempBuf[m++] = 0x01;
      //mov eax, NEW ENTRY POINT
      k = (SIZE_T)lpRemoteCode + nNewEntryPointOfs;
      aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      //pop ecx
      aTempBuf[m++] = 0x59;
      //jmp to original eip
      k = (SIZE_T)lpRemoteCode + (m+6);
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0x25;  memcpy(aTempBuf+m, &k, 4);  m+=4;
      memcpy(aTempBuf+m, &nOrigIP, 4);  m+=4;
#endif
      break;

#if defined _M_X64
    case 64:
      //data
      memset(aTempBuf+m, 0x00, 8);  m+=8; //will contain the entrypoint to jump to
      memcpy(aTempBuf+m, &hParentProcCopy, 8);  m+=8;
      memcpy(aTempBuf+m, &hContinueEventCopy, 8);  m+=8;
      //new entrypoint starts here
      nNewEntryPointOfs = m;
      //sub rsp, 48h
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0x83;  aTempBuf[m++] = 0xEC;  aTempBuf[m++] = 0x48;
      //mov rcx, READY EVENT HANDLE
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB9;  memcpy(aTempBuf+m, &hReadyEventCopy, 8);  m+=8;
      //mov rax, ADDRESS OF SetEvent
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nSetEventAddr, 8);  m+=8;
      //call rax
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0xD0;
      //mov rcx, READY EVENT HANDLE
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB9;  memcpy(aTempBuf+m, &hReadyEventCopy, 8);  m+=8;
      //mov rax, ADDRESS OF CloseHandle
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 8);  m+=8;
      //call rax
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0xD0;
      //mov rcx, 2
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB9;  aTempBuf[m++] = 0x02;
      memset(aTempBuf+m, 0x00, 7); m+=7;
      //mov rdx, address of array of ENGINE PROC HANDLE and CONTINUE EVENT HANDLE
      k = (SIZE_T)lpRemoteCode + 8;
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xBA;  memcpy(aTempBuf+m, &k, 8);  m+=8;
      //mov r8, FALSE
      aTempBuf[m++] = 0x49;  aTempBuf[m++] = 0xB8;  memset(aTempBuf+m, 0x00, 8);  m+=8;
      //mov r9, INFINITE
      aTempBuf[m++] = 0x49;  aTempBuf[m++] = 0xB9;  memset(aTempBuf+m, 0xFF, 4);  m+=4;
      memset(aTempBuf+m, 0x00, 4);  m+=4;
      //mov rax, ADDRESS OF WaitForMultipleObjects
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nWaitMultObjAddr, 8);  m+=8;
      //call rax
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0xD0;
      //mov rcx, CONTINUE EVENT HANDLE
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB9;  memcpy(aTempBuf+m, &hContinueEventCopy, 8);  m+=8;
      //mov rax, ADDRESS OF CloseHandle
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 8);  m+=8;
      //call rax
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0xD0;
      //mov rcx, ENGINE PROC HANDLE
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB9;  memcpy(aTempBuf+m, &hParentProcCopy, 8);  m+=8;
      //mov rax, ADDRESS OF CloseHandle
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &nCloseHandleAddr, 8);  m+=8;
      //call rax
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0xD0;
      //add rsp, 48h
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0x83;  aTempBuf[m++] = 0xC4;  aTempBuf[m++] = 0x48;
      //jmp qword ptr [lpRemoteCode]
      dwTemp32 = ~((DWORD)m+6) + 1;
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0x25;  memcpy(aTempBuf+m, &dwTemp32, 4);  m+=4;
      //NEW STUB
      //at this point rcx has the original entry point, store it in lpRemoteCode using interlocked
      nNewStubOfs = m;
      //push rax
      aTempBuf[m++] = 0x50;
      //mov rax, lpRemoteCode
      k = (SIZE_T)lpRemoteCode;
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB8;  memcpy(aTempBuf+m, &k, 8);  m+=8;
      //lock xchg rcx, [rax]
      aTempBuf[m++] = 0xF0;  aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0x87;  aTempBuf[m++] = 0x08;
      //mov rcx, NEW ENTRY POINT
      k = (SIZE_T)lpRemoteCode + nNewEntryPointOfs;
      aTempBuf[m++] = 0x48;  aTempBuf[m++] = 0xB9;  memcpy(aTempBuf+m, &k, 8);  m+=8;
      //pop rax
      aTempBuf[m++] = 0x58;
      //jmp to original rip
      aTempBuf[m++] = 0xFF; aTempBuf[m++] = 0x25;  memset(aTempBuf+m, 0x00, 4);  m+=4;
      memcpy(aTempBuf+m, &nOrigIP, 8);  m+=8;
      break;
#endif //_M_X64
  }
  //write memory
  if (::WriteProcessMemory(hSuspendedProc, lpRemoteCode, aTempBuf, m, &nWritten) == FALSE ||
      nWritten != m)
  {
    ::VirtualFreeEx(hSuspendedProc, lpRemoteCode, 0, MEM_RELEASE);
    return E_FAIL;
  }
  ::FlushInstructionCache(hSuspendedProc, lpRemoteCode, (m+0xFF) & (~0xFF));
  //set new entry point address
#if defined _M_IX86
  sThreadCtx.Eip = (DWORD)lpRemoteCode + nNewStubOfs;
  if (::SetThreadContext(hSuspendedMainThread, &sThreadCtx) == FALSE)
  {
    ::VirtualFreeEx(hSuspendedProc, lpRemoteCode, 0, MEM_RELEASE);
    return E_FAIL;
  }
#elif defined _M_X64
  switch (dwPlatformBits)
  {
    case 32:
      sWow64ThreadCtx.Eip = (DWORD)(lpRemoteCode + nNewStubOfs);
      if (fnWow64SetThreadContext(hSuspendedMainThread, &sWow64ThreadCtx) == FALSE)
      {
        hRes = NKT_HRESULT_FROM_LASTERROR();
        ::VirtualFreeEx(hSuspendedProc, lpRemoteCode, 0, MEM_RELEASE);
        return hRes;
      }
      break;

    case 64:
      sThreadCtx.Rip = (DWORD64)(lpRemoteCode + nNewStubOfs);
      if (::SetThreadContext(hSuspendedMainThread, &sThreadCtx) == FALSE)
      {
        hRes = NKT_HRESULT_FROM_LASTERROR();
        ::VirtualFreeEx(hSuspendedProc, lpRemoteCode, 0, MEM_RELEASE);
        return hRes;
      }
      break;
  }
#endif
  //done
  *lphReadyExecutionEvent = cReadyEvent.Detach();
  *lphContinueExecutionEvent = cContinueEvent.Detach();
  return S_OK;
}

HRESULT CNktTools::GetSystem32Path(__inout CNktStringW &cStrPathW)
{
  LPWSTR sW;
  SIZE_T nLen, nBufSize;

  for (nBufSize=4096; nBufSize<409600; nBufSize+=4096)
  {
    if (cStrPathW.EnsureBuffer(nBufSize) == FALSE)
      return E_OUTOFMEMORY;
    nLen = (SIZE_T)::GetSystemDirectory(cStrPathW, (UINT)nBufSize);
    if (nLen < nBufSize-4)
      break;
  }
  if (nBufSize >= 409600)
    return E_OUTOFMEMORY;
  sW = (LPWSTR)cStrPathW;
  if (nLen > 0 && sW[nLen-1] != L'\\' && sW[nLen-1] != L'/')
    sW[nLen++] = L'\\';
  sW[nLen] = 0;
  cStrPathW.Refresh();
  return FixFolderStr(cStrPathW);
}

HRESULT CNktTools::FixFolderStr(__inout CNktStringW &cStrPathW)
{
  SIZE_T i, nLen;
  LPWSTR sW;

  nLen = cStrPathW.GetLength();
  if (nLen > 0)
  {
    sW = (LPWSTR)cStrPathW;
    for (i=0; i<nLen; i++)
    {
      if (sW[i] == L'/')
        sW[i] = L'\\';
    }
    if (nLen > 0 && sW[nLen-1] != L'\\')
    {
      if (cStrPathW.Concat(L"\\") == FALSE)
        return E_OUTOFMEMORY;
    }
  }
  return S_OK;
}

HRESULT CNktTools::GetModuleUsageCount(__in HINSTANCE hDll, __out SIZE_T &nModuleSize)
{
  ULONG_PTR nCookie;
  NKT_LDR_DATA_TABLE_ENTRY *lpEntry;
  PRTL_CRITICAL_SECTION lpCS;
  HRESULT hRes, hResLock;

  nModuleSize = (SIZE_T)-1;
  if (hDll == NULL)
    return E_INVALIDARG;
  hRes = InitRoutines();
  if (FAILED(hRes))
    return hRes;
  if (fnLdrLockLoaderLock != NULL)
  {
    hResLock = HRESULT_FROM_NT(fnLdrLockLoaderLock(0, NULL, &nCookie));
  }
  else if (fnRtlEnterCriticalSection != NULL)
  {
    lpCS = LdrGetLoaderLock();
    _ASSERT(lpCS != NULL);
    hResLock = HRESULT_FROM_NT(fnRtlEnterCriticalSection(lpCS));
  }
  else
  {
    hResLock = E_NOTIMPL;
  }
  try
  {
    hRes = HRESULT_FROM_NT(fnLdrFindEntryForAddress((PVOID)hDll, (PVOID*)&lpEntry));
    if (SUCCEEDED(hRes) && lpEntry->LoadCount != 0xFFFF)
      nModuleSize = (SIZE_T)(lpEntry->LoadCount);
  }
  catch (...)
  {
    nModuleSize = (SIZE_T)-1; //assume fixed dll
    hRes = E_FAIL;
  }
  if (SUCCEEDED(hResLock))
  {
    if (fnLdrLockLoaderLock != NULL)
      fnLdrUnlockLoaderLock(0, nCookie);
    else if (fnRtlEnterCriticalSection != NULL)
      fnRtlLeaveCriticalSection(lpCS);
  }
  return hRes;
}

SIZE_T CNktTools::GetModuleSize(__in HINSTANCE hDll)
{
  IMAGE_DATA_DIRECTORY sDataDirEntry;
#if defined _M_IX86
  IMAGE_NT_HEADERS32 *lpNtHdr;
#elif defined _M_X64
  IMAGE_NT_HEADERS64 *lpNtHdr;
#else
  #error Unsupported platform
#endif
  SIZE_T i, j, nImageSize;
  DWORD dwTemp32;
  WORD wTemp16;
  LPBYTE lpBaseAddr;

  lpBaseAddr = (LPBYTE)hDll;
  if (lpBaseAddr == NULL)
    return 0;
  //check dos signature
  memcpy(&wTemp16, lpBaseAddr + FIELD_OFFSET(IMAGE_DOS_HEADER, e_magic), sizeof(WORD));
  if (wTemp16 != IMAGE_DOS_SIGNATURE)
    return 0;
  //get header offset
  memcpy(&dwTemp32, lpBaseAddr + FIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew), sizeof(DWORD));
#if defined _M_IX86
  lpNtHdr = (IMAGE_NT_HEADERS32*)(lpBaseAddr + (SIZE_T)dwTemp32);
#elif defined _M_X64
  lpNtHdr = (IMAGE_NT_HEADERS64*)(lpBaseAddr + (SIZE_T)dwTemp32);
#endif
  nImageSize = (SIZE_T)(lpNtHdr->OptionalHeader.SizeOfImage);
  for (i=0; i<IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++)
  {
    memcpy(&sDataDirEntry, &(lpNtHdr->OptionalHeader.DataDirectory[i]), sizeof(sDataDirEntry));
    if (sDataDirEntry.VirtualAddress != NULL && sDataDirEntry.Size != 0)
    {
      j = (SIZE_T)sDataDirEntry.VirtualAddress + (SIZE_T)sDataDirEntry.Size;
      if (j > nImageSize)
        nImageSize = j;
    }
  }
  return nImageSize;
}

HRESULT CNktTools::_CreateProcess(__out DWORD &dwPid, __in_z_opt LPCWSTR szImagePathW, __in BOOL bSuspended,
                                  __out_opt LPHANDLE lphProcess, __out_opt LPHANDLE lphContinueEvent)
{
  return _CreateProcessWithLogon(dwPid, szImagePathW, NULL, NULL, bSuspended, lphProcess, lphContinueEvent);
}

HRESULT CNktTools::_CreateProcessWithLogon(__out DWORD &dwPid, __in_z LPCWSTR szImagePathW, __in_z LPCWSTR szUserNameW,
                                 __in_z LPCWSTR szPasswordW, __in BOOL bSuspended, __out_opt LPHANDLE lphProcess,
                                 __out_opt LPHANDLE lphContinueEvent)
{
  CNktStringW cStrTempW;
  STARTUPINFOW sSiW;
  PROCESS_INFORMATION sPi;
  HRESULT hRes;
  BOOL bCallSuccess;
  HANDLE hEvents[2], hReadyEvent, hContinueEvent;

  dwPid = 0;
  if (lphProcess != NULL)
    *lphProcess = NULL;
  if (lphContinueEvent != NULL)
    *lphContinueEvent = NULL;
  if (szImagePathW == NULL)
    return E_POINTER;
  if (szImagePathW[0] == 0)
    return E_INVALIDARG;
  if (bSuspended != FALSE && lphContinueEvent == NULL)
    return E_POINTER;
  memset(&sSiW, 0, sizeof(sSiW));
  sSiW.cb = sizeof(sSiW);
  //::GetStartupInfoW(&sSiW);
  memset(&sPi, 0, sizeof(sPi));
  //CreateProcessWithLogonW() requires a read/write parameter (see MSDN)
  if (cStrTempW.Copy(szImagePathW) == FALSE)
    return E_OUTOFMEMORY;
  if (szUserNameW == NULL || szUserNameW[0] == 0)
  {
    bCallSuccess = ::CreateProcessW(NULL, (LPWSTR)cStrTempW, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL,
                                    &sSiW, &sPi);
  }
  else
  {
    if (szPasswordW == NULL)
      szPasswordW = L"";
    bCallSuccess = ::CreateProcessWithLogonW(szUserNameW, NULL, szPasswordW, LOGON_WITH_PROFILE, NULL,
                                             (LPWSTR)cStrTempW, CREATE_SUSPENDED, NULL, NULL, &sSiW, &sPi);
  }
  if (bCallSuccess != FALSE)
  {
    hRes = CNktTools::SuspendAfterCreateProcessW(&hReadyEvent, &hContinueEvent, ::GetCurrentProcess(), sPi.hProcess,
                                                 sPi.hThread);
    if (SUCCEEDED(hRes))
    {
      ::ResumeThread(sPi.hThread);
      //wait until initialization completes (don't worry about return code)
      hEvents[0] = sPi.hThread;
      hEvents[1] = hReadyEvent;
      ::WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
      ::CloseHandle(hReadyEvent); //not used so close
    }
  }
  else
  {
    hRes = NKT_HRESULT_FROM_LASTERROR();
  }
  if (SUCCEEDED(hRes))
  {
    if (bSuspended != FALSE)
    {
      *lphContinueEvent = hContinueEvent;
    }
    else
    {
      ::SetEvent(hContinueEvent);
      ::CloseHandle(hContinueEvent);
    }
    dwPid = sPi.dwProcessId;
  }
  else if (bSuspended != FALSE)
  {
    ::TerminateProcess(sPi.hProcess, 0);
  }
  if (sPi.hThread != NULL)
    ::CloseHandle(sPi.hThread);
  if (sPi.hProcess != NULL)
  {
    if (SUCCEEDED(hRes) && lphProcess != NULL)
      *lphProcess = sPi.hProcess;
    else
      ::CloseHandle(sPi.hProcess);
  }
  return hRes;
}

DWORD CNktTools::GetThreadId(__in HANDLE hThread)
{
  NKT_THREAD_BASIC_INFORMATION sTbi;

  if (hThread != NULL)
  {
    if (SUCCEEDED(InitRoutines()) && fnNtQueryInformationThread != NULL)
      if (fnNtQueryInformationThread(hThread, NKT_ThreadBasicInformation, &sTbi, sizeof(sTbi), NULL) >= 0)
        return (DWORD)(sTbi.ClientId.UniqueThread);
  }
  return 0;
}

//-----------------------------------------------------------

static HRESULT InitRoutines()
{
  static LONG volatile nMtx = 0;

  if (hNtDll == NULL)
  {
    CNktSimpleLockNonReentrant cLock(&nMtx);
    HINSTANCE _hNtDll, _hkernel32Dll;

    if (hNtDll == NULL)
    {
      _hNtDll = ::GetModuleHandleW(L"ntdll.dll");
      if (_hNtDll != NULL)
      {
        fnLdrLockLoaderLock = (lpfnLdrLockLoaderLock)::GetProcAddress(_hNtDll, "LdrLockLoaderLock");
        fnLdrUnlockLoaderLock = (lpfnLdrUnlockLoaderLock)::GetProcAddress(_hNtDll, "LdrUnlockLoaderLock");
        fnLdrFindEntryForAddress = (lpfnLdrFindEntryForAddress)::GetProcAddress(_hNtDll, "LdrFindEntryForAddress");
        fnRtlEnterCriticalSection = (lpfnRtlEnterCriticalSection)::GetProcAddress(_hNtDll, "RtlEnterCriticalSection");
        fnRtlLeaveCriticalSection = (lpfnRtlLeaveCriticalSection)::GetProcAddress(_hNtDll, "RtlLeaveCriticalSection");
        fnNtQueryInformationThread = (lpfnNtQueryInformationThread)::GetProcAddress(_hNtDll,
                                                                                    "NtQueryInformationThread");
        if (fnLdrLockLoaderLock == NULL || fnLdrUnlockLoaderLock == NULL)
        {
          fnLdrLockLoaderLock = NULL;
          fnLdrUnlockLoaderLock = NULL;
        }
        if (fnRtlEnterCriticalSection == NULL || fnRtlLeaveCriticalSection == NULL)
        {
          fnRtlEnterCriticalSection = NULL;
          fnRtlLeaveCriticalSection = NULL;
        }
        if (fnLdrFindEntryForAddress == NULL ||
            (fnLdrLockLoaderLock == NULL && fnRtlEnterCriticalSection == NULL))
          return NKT_E_NotFound;
      }
      //----
      _hkernel32Dll = ::LoadLibraryW(L"kernel32.dll");
      if (_hkernel32Dll != NULL)
      {
        fnWow64GetThreadContext = (lpfnWow64GetThreadContext)::GetProcAddress(_hkernel32Dll, "Wow64GetThreadContext");
        fnWow64SetThreadContext = (lpfnWow64SetThreadContext)::GetProcAddress(_hkernel32Dll, "Wow64SetThreadContext");
        if (fnWow64GetThreadContext == NULL || fnWow64SetThreadContext == NULL)
        {
          fnWow64GetThreadContext = NULL;
          fnWow64SetThreadContext = NULL;
        }
        fnIsWow64Process = (lpfnIsWow64Process)::GetProcAddress(_hkernel32Dll, "IsWow64Process");
        fnGetNativeSystemInfo = (lpfnGetNativeSystemInfo)::GetProcAddress(_hkernel32Dll, "GetNativeSystemInfo");
      }
      //----
      if (sSysInfo.wProcessorArchitecture == 0)
      {
        if (fnGetNativeSystemInfo != NULL)
          fnGetNativeSystemInfo(&sSysInfo);
        else
          ::GetSystemInfo(&sSysInfo);
      }
      //----
      InterlockedExchangePointer((PVOID volatile *)&hkernel32Dll, (PVOID)_hkernel32Dll);
      InterlockedExchangePointer((PVOID volatile *)&hNtDll, (PVOID)_hNtDll);
    }
  }
  return S_OK;
}

static __inline BOOL ReadProcMem(__in HANDLE hProc, __out LPVOID lpDest, __in LPVOID lpSrc, __in SIZE_T nDataSize)
{
  SIZE_T nReaded;

  return (::ReadProcessMemory(hProc, lpSrc, lpDest, nDataSize, &nReaded) != FALSE &&
          nReaded == nDataSize) ? TRUE : FALSE;
}

static __inline BOOL IsBufferSmallError(__in HRESULT hRes)
{
  return (hRes == HRESULT_FROM_NT(NKT_NTSTATUS_INFO_LENGTH_MISMATCH) ||
          hRes == HRESULT_FROM_NT(NKT_NTSTATUS_BUFFER_TOO_SMALL) ||
          hRes == NKT_HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) ||
          hRes == NKT_HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW)) ? TRUE : FALSE;
}

static PRTL_CRITICAL_SECTION LdrGetLoaderLock()
{
#if defined _M_IX86
  static LONG volatile nLoaderLock = 0;
#elif defined _M_X64
  static __int64 volatile nLoaderLock = 0;
#else
#error Unsupported platform
#endif

  if (nLoaderLock == 0)
  {
    LPBYTE lpPtr;

    //loader lock will not vary its address when process is running
#if defined _M_IX86
    lpPtr = (LPBYTE)__readfsdword(0x30); //get PEB from the TIB
    lpPtr = *((LPBYTE*)(lpPtr+0xA0));    //get loader lock pointer
    _InterlockedExchange(&nLoaderLock, (LONG)lpPtr);
#elif defined _M_X64
    lpPtr = (LPBYTE)__readgsqword(0x30); //get TEB
    lpPtr = *((LPBYTE*)(lpPtr+0x60));
    lpPtr = *((LPBYTE*)(lpPtr+0x110));
    _InterlockedExchange64(&nLoaderLock, (__int64)lpPtr);
#else
#error Unsupported platform
#endif
  }
  return (PRTL_CRITICAL_SECTION)nLoaderLock;
}
