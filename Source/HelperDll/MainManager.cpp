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

#include "MainManager.h"
#include "..\Common\SimplePipesIPC.h"
#include "COM\ComManager.h"
#include "JAVA\JavaManager.h"
#include "..\Common\TransportMessages.h"
#include "..\Common\Tools.h"
#include "..\Common\AutoPtr.h"
#include "..\Common\MallocMacros.h"

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
#pragma pack()

typedef struct {
  TMSG_FINDWINDOW *lpMsgFindWindow;
  HRESULT hRes;
  HWND hWnd;
  LPWSTR lpMem;
  SIZE_T nMemSize;
} EWP_FindWindow_DATA;

typedef struct {
  TMSG_GETCHILDWINDOW_REPLY *lpReply;
  SIZE_T nIndex;
} EWP_GetChildWindow_DATA;

//-----------------------------------------------------------

typedef VOID (WINAPI *lpfnExitProcess)(__in UINT uExitCode);
typedef NTSTATUS (NTAPI *lpfnLdrLoadDll)(__in_opt LPWSTR PathToFile, __in ULONG Flags,
                                __in PUNICODE_STRING ModuleFileName, __in HINSTANCE *ModuleHandle);
typedef NTSTATUS (NTAPI *lpfnLdrUnloadDll)(__in HINSTANCE ModuleHandle);
typedef BOOL (WINAPI *lpfnCreateProcessInternalW)(__in HANDLE hToken, __in_z_opt LPCWSTR lpApplicationName,
                                __in_z_opt LPWSTR lpCommandLine, __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, __in BOOL bInheritHandles,
                                __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment,
                                __in_z_opt LPCWSTR lpCurrentDirectory, __in_opt LPSTARTUPINFOW lpStartupInfo,
                                __out LPPROCESS_INFORMATION lpProcessInformation, __out PHANDLE hNewToken);
typedef NTSTATUS (NTAPI *lpfnNtSuspendThread)(__in HANDLE ThreadHandle, __out_opt PULONG PreviousSuspendCount);
typedef NTSTATUS (NTAPI *lpfnNtResumeThread)(__in HANDLE ThreadHandle, __out_opt PULONG SuspendCount);
typedef NTSTATUS (NTAPI *lpfnNtAlertResumeThread)(__in HANDLE ThreadHandle, __out_opt PULONG SuspendCount);
typedef BOOL (WINAPI *lpfnEnumProcessModules)(__in HANDLE hProcess, __out HMODULE *lphModule,
                                              __in DWORD cb, __out LPDWORD lpcbNeeded);

//-----------------------------------------------------------

static DWORD WINAPI MainThreadProc(__in LPVOID lpParameter);
static VOID WINAPI OnExitProcess(__in UINT uExitCode);
static NTSTATUS NTAPI OnLdrLoadDll(__in_opt LPWSTR PathToFile, __in ULONG Flags,
                                   __in PUNICODE_STRING ModuleFileName, __in HINSTANCE *ModuleHandle);
static NTSTATUS NTAPI OnLdrUnloadDll(__in HINSTANCE ModuleHandle);
static BOOL WINAPI OnCreateProcessInternalW(__in HANDLE hToken, __in_z_opt LPCWSTR lpApplicationName,
                           __in_z_opt LPWSTR lpCommandLine, __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes,
                           __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, __in BOOL bInheritHandles,
                           __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment,
                           __in_z_opt LPCWSTR lpCurrentDirectory, __in_opt LPSTARTUPINFOW lpStartupInfo,
                           __out LPPROCESS_INFORMATION lpProcessInformation, __out PHANDLE hNewToken);
static NTSTATUS NTAPI OnNtSuspendThread(__in HANDLE ThreadHandle, __out_opt PULONG PreviousSuspendCount);
static NTSTATUS NTAPI OnNtResumeThread(__in HANDLE ThreadHandle, __out_opt PULONG SuspendCount);
static NTSTATUS NTAPI OnNtAlertResumeThread(__in HANDLE ThreadHandle, __out_opt PULONG SuspendCount);
static BOOL CALLBACK EWP_FindWindow(__in HWND hWnd, __in LPARAM lParam);
static BOOL CALLBACK ECWP_FindWindowChild(__in HWND hWnd, __in LPARAM lParam);
static BOOL CALLBACK ECWP_GetChildWnd(__in HWND hWnd, __in LPARAM lParam);
static BOOL WildcardMatch(__in LPCWSTR szW, __in SIZE_T nMaxLen, __in LPCWSTR szPatternW);

//-----------------------------------------------------------

namespace MainManager {

class CManager : public CNktFastMutex, private COM::CMainManager::CCallbacks, private JAVA::CMainManager::CCallbacks
{
public:
  CManager();
  ~CManager();

  LPVOID __cdecl operator new(__in size_t nSize, __in LPVOID lpInPlace)
    {
    return lpInPlace;
    };
#if _MSC_VER >= 1200
  VOID __cdecl operator delete(__in LPVOID p, __in LPVOID lpPlace)
    {
    return;
    };
#endif //_MSC_VER >= 1200

  HRESULT Initialize(__in INIT_DATA *lpInitData);

  BOOL Run();
  VOID OnExitProcess();

  VOID InitShutdownBecauseError();

  HRESULT SendMsg(__in LPVOID lpData, __in SIZE_T nDataSize, __inout_opt LPVOID *lplpReply=NULL,
                  __inout_opt SIZE_T *lpnReplySize=NULL);

  NTSTATUS OnLdrLoadDll(__in_opt LPWSTR PathToFile, __in ULONG Flags, __in PUNICODE_STRING ModuleFileName,
                        __out HINSTANCE *ModuleHandle);
  NTSTATUS OnLdrUnloadDll(__in HINSTANCE ModuleHandle);

  BOOL OnCreateProcessInternalW(__in HANDLE hToken, __in_z_opt LPCWSTR lpApplicationName,
                                __in_z_opt LPWSTR lpCommandLine, __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, __in BOOL bInheritHandles,
                                __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment,
                                __in_z_opt LPCWSTR lpCurrentDirectory, __in_opt LPSTARTUPINFOW lpStartupInfo,
                                __out LPPROCESS_INFORMATION lpProcessInformation, __out PHANDLE hNewToken);

  NTSTATUS OnNtSuspendThread(__in HANDLE ThreadHandle, __out_opt PULONG PreviousSuspendCount);
  NTSTATUS OnNtResumeThread(__in HANDLE ThreadHandle, __out_opt PULONG SuspendCount, __in BOOL bIsAlert);

  HRESULT OnServerMessage(__in HANDLE hConn, __in LPBYTE lpData, __in SIZE_T nDataSize,
                          __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

private:
  class CMySimpleIPC : public CNktSimplePipesIPC
  {
  public:
    CMySimpleIPC(__in CManager *_lpMgr, __in BOOL bComEnabled) :
                 CNktSimplePipesIPC((bComEnabled != FALSE) ? CNktSimplePipesIPC::atSTA : CNktSimplePipesIPC::atNone)
      {
      lpMgr = _lpMgr;
      return;
      };

  private:
    HRESULT OnMessage(__in HANDLE hConn, __in LPBYTE lpData, __in SIZE_T nDataSize,
                      __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
      {
      return lpMgr->OnServerMessage(hConn, lpData, nDataSize, lplpReplyData, lpnReplyDataSize);
      };

  private:
    CManager *lpMgr;
  };

  class CAvoidRecursion
  {
  public:
    CAvoidRecursion(__in DWORD dwTlsIndex)
      {
      SIZE_T nValue;

      nValue = (SIZE_T)::TlsGetValue(dwTlsIndex);
      if ((nValue & 1) != 0)
      {
        bIsRecursion = TRUE;
      }
      else
      {
        bIsRecursion = FALSE;
        nValue |= 1;
        ::TlsSetValue(dwTlsIndex, (LPVOID)nValue);
      }
      return;
      };

    ~CAvoidRecursion()
      {
      if (bIsRecursion != FALSE)
      {
        SIZE_T nValue = (SIZE_T)::TlsGetValue(dwTlsIndex);
        nValue &= ~1;
        ::TlsSetValue(dwTlsIndex, (LPVOID)nValue);
      }
      return;
      };

    BOOL IsRecursion()
      {
      return bIsRecursion;
      };

  private:
    DWORD dwTlsIndex;
    BOOL bIsRecursion;
  };

  class CCreatedChildProcess : public TNktLnkLstNode<CCreatedChildProcess>
  {
  public:
    CCreatedChildProcess(__in DWORD dwPid, __in DWORD dwTid, __in DWORD dwPlatformBits);
    ~CCreatedChildProcess();

    HRESULT Init(__in HANDLE hProcess, __in HANDLE hThread, __in HANDLE hReadyEvent, __in HANDLE hContinueEvent);

    public:
      DWORD dwPid, dwTid, dwPlatformBits;
      HANDLE hProcess, hThread;
      SIZE_T nSuspendCount;
      HANDLE hReadyEvent, hContinueEvent;
  };

  class CAutoUsageCount
  {
  public:
    CAutoUsageCount(__in LONG volatile *_lpnValue)
      {
      lpnValue = _lpnValue;
      NktInterlockedIncrement(lpnValue);
      return;
      };
    ~CAutoUsageCount()
      {
      NktInterlockedDecrement(lpnValue);
      return;
      };

  private:
    LONG volatile *lpnValue;
  };

private:
  HRESULT SendCreateProcessMsg(__in CCreatedChildProcess *lpChildProc);

  HRESULT BuildSimpleReplyData(__in HRESULT hRes, __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);

  VOID OnComInterfaceCreated(__in REFCLSID sClsId, __in REFIID sIid, __in IUnknown *lpUnk);
  LPBYTE OnComAllocIpcBuffer(__in SIZE_T nSize);
  VOID OnComFreeIpcBuffer(__in LPVOID lpBuffer);
  HRESULT OnComBuildSimpleReplyData(__in HRESULT hRes, __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);
  VOID OnComHookUsage(__in BOOL bEntering);

  HRESULT OnJavaRaiseJavaNativeMethodCalled(__in TMSG_RAISEONJAVANATIVEMETHODCALLED *lpMsg, __in SIZE_T nMsgSize,
                                            __out TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY **lplpReply,
                                            __out SIZE_T *lpnReplySize);
  LPBYTE OnJavaAllocIpcBuffer(__in SIZE_T nSize);
  VOID OnJavaFreeIpcBuffer(__in LPVOID lpBuffer);
  HRESULT OnJavaBuildSimpleReplyData(__in HRESULT hRes, __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize);
  VOID OnJavaHookUsage(__in BOOL bEntering);

private:
  //IMPORTANT: Order of variables is important for destructors to be executed in the correct order
  struct {
    CNktFastMutex cMtx;
    TNktLnkLst<CCreatedChildProcess> cList;
  } sCreatedChildProcesses;
  CMySimpleIPC *lpConn;
  HANDLE hConn;
  CNktHookLib cHookMgr; //above CComHookManager
  COM::CMainManager cComHookMgr;
  JAVA::CMainManager cJavaHookMgr;
  CNktEvent cShutdownEvent;
  CNktAutoHandle cMasterProc;
  LONG volatile nAppIsExiting;
  HINSTANCE hNtDll, hKernelBaseDll, hKernel32Dll;
  CNktHookLib::HOOK_INFO sBaseHooks[7];
  //CNktFastMutex cMyLdrLock;
  ULONG nHookFlags;
  LONG volatile nHookUsageCount;
};

//-----------------------------------------------------------

static CManager *lpMainMgr = NULL;
static BYTE aMainMgr[sizeof(CManager)] = {0};
HINSTANCE hDllInst = NULL;
static HANDLE hMainThread = NULL;

//-----------------------------------------------------------

HRESULT Initialize(__in INIT_DATA *lpInitData)
{
  DWORD dwTid;
  HRESULT hRes;

  _ASSERT(lpInitData != NULL);
  _ASSERT(lpInitData->nStructSize >= (ULONG)sizeof(INIT_DATA));
  lpMainMgr = new (aMainMgr) CManager;
  if (lpMainMgr == NULL)
    return E_OUTOFMEMORY;
  hRes = lpMainMgr->Initialize(lpInitData);
  //main thread
  if (SUCCEEDED(hRes))
  {
    hMainThread = ::CreateThread(NULL, 0, MainThreadProc, lpMainMgr, 0, &dwTid);
    if (hMainThread == NULL)
      hRes = E_OUTOFMEMORY;
  }
  if (FAILED(hRes))
  {
    lpMainMgr->~CManager();
    InterlockedExchangePointer((PVOID volatile*)&lpMainMgr, NULL);
  }
  return hRes;
}

//-----------------------------------------------------------

#pragma warning(disable : 4355)
CManager::CManager() : CNktFastMutex(), cComHookMgr(this, &cHookMgr), cJavaHookMgr(this, &cHookMgr)
{
  NktInterlockedExchange(&nAppIsExiting, 0);
  NktInterlockedExchange(&nHookUsageCount, 0);
  hNtDll = hKernel32Dll = hKernelBaseDll = NULL;
  memset(sBaseHooks, 0, sizeof(sBaseHooks));
  hConn = NULL;
  nHookFlags = 0;
  lpConn = NULL;
  return;
}
#pragma warning(default : 4355)

CManager::~CManager()
{
  CCreatedChildProcess *lpChildProc;
  SIZE_T i;

  cHookMgr.EnableHook(sBaseHooks, NKT_ARRAYLEN(sBaseHooks), FALSE);
  //continue execution of all suspended child processes
  while ((lpChildProc = sCreatedChildProcesses.cList.PopHead()) != NULL)
    delete lpChildProc;
  if (nAppIsExiting == 0 && lpConn != NULL)
    delete lpConn;
  //----
  for (i=30; i>0 && nHookUsageCount>0; i--)
    ::Sleep(100);
  return;
}

HRESULT CManager::Initialize(__in INIT_DATA *lpInitData)
{
  HINSTANCE hPsApiDll;
  lpfnEnumProcessModules fnEnumProcessModules;
  DWORD dw, dwSize, dwNeeded;
  HMODULE *lpModList;
  HRESULT hRes, hRes2;

  if (lpInitData == NULL || (SIZE_T)(lpInitData->nStructSize) != sizeof(INIT_DATA))
    return E_FAIL;
  lpConn = new CMySimpleIPC(this, ((lpInitData->nFlags & (ULONG)flgDisableComHooking) == 0) ? TRUE : FALSE);
  if (lpConn == NULL)
    return E_OUTOFMEMORY;
  //do a fake call to this method in order to initialize some routines before they are hooked
  CNktTools::GetProcessPlatformBits(::GetCurrentProcess());
  //parse init data
  cMasterProc.Attach((HANDLE)(lpInitData->hControllerProc));
  cShutdownEvent.Attach((HANDLE)(lpInitData->hShutdownEv));
  nHookFlags = lpInitData->nFlags;
#ifdef _DEBUG
  nHookFlags |= (ULONG)flgDebugPrintInterfaces;
#endif //_DEBUG
  cComHookMgr.nHookFlags = cJavaHookMgr.nHookFlags = nHookFlags;
  //init helpers
  hRes = S_OK;
  if (SUCCEEDED(hRes) && (nHookFlags & (ULONG)flgDisableComHooking) == 0)
    hRes = cComHookMgr.Initialize();
  if (SUCCEEDED(hRes) && (nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
    hRes = cJavaHookMgr.Initialize();
  if (SUCCEEDED(hRes))
    hRes = lpConn->SetupSlave(hConn, lpInitData->nMasterPid, (HANDLE)(lpInitData->hSlavePipe), cMasterProc);
  //init base hooks
  if (SUCCEEDED(hRes))
  {
    hNtDll = ::GetModuleHandleW(L"ntdll.dll");
    if (hNtDll != NULL)
    {
      sBaseHooks[1].lpProcToHook = ::GetProcAddress(hNtDll, "LdrLoadDll");
      sBaseHooks[1].lpNewProcAddr = ::OnLdrLoadDll;
      sBaseHooks[2].lpProcToHook = ::GetProcAddress(hNtDll, "LdrUnloadDll");
      sBaseHooks[2].lpNewProcAddr = ::OnLdrUnloadDll;
      sBaseHooks[4].lpProcToHook = ::GetProcAddress(hNtDll, "NtSuspendThread");
      sBaseHooks[4].lpNewProcAddr = ::OnNtSuspendThread;
      sBaseHooks[5].lpProcToHook = ::GetProcAddress(hNtDll, "NtResumeThread");
      sBaseHooks[5].lpNewProcAddr = ::OnNtResumeThread;
      sBaseHooks[6].lpProcToHook = ::GetProcAddress(hNtDll, "NtAlertResumeThread");
      sBaseHooks[6].lpNewProcAddr = ::OnNtAlertResumeThread;
      if (sBaseHooks[1].lpProcToHook == NULL || sBaseHooks[2].lpProcToHook == NULL ||
          sBaseHooks[4].lpProcToHook == NULL || sBaseHooks[5].lpProcToHook == NULL ||
          sBaseHooks[6].lpProcToHook == NULL)
        hRes = NKT_E_NotFound;
    }
    else
    {
      hRes = NKT_HRESULT_FROM_LASTERROR();
    }
  }
  if (SUCCEEDED(hRes))
  {
    sBaseHooks[3].lpNewProcAddr = ::OnCreateProcessInternalW;
    hKernelBaseDll = ::LoadLibraryW(L"kernelbase.dll");
    if (hKernelBaseDll != NULL)
      sBaseHooks[3].lpProcToHook = ::GetProcAddress(hKernelBaseDll, "CreateProcessInternalW");
    hKernel32Dll = ::LoadLibraryW(L"kernel32.dll");
    if (hKernel32Dll != NULL)
    {
      sBaseHooks[0].lpProcToHook = ::GetProcAddress(hKernel32Dll, "ExitProcess");
      sBaseHooks[0].lpNewProcAddr = ::OnExitProcess;
      if (sBaseHooks[3].lpProcToHook == NULL)
        sBaseHooks[3].lpProcToHook = ::GetProcAddress(hKernel32Dll, "CreateProcessInternalW");
      if (sBaseHooks[0].lpProcToHook == NULL || sBaseHooks[3].lpProcToHook == NULL)
        hRes = NKT_E_NotFound;
    }
    else
    {
      hRes = NKT_HRESULT_FROM_LASTERROR();
    }
  }
  if (SUCCEEDED(hRes))
    hRes = NKT_HRESULT_FROM_WIN32(cHookMgr.Hook(sBaseHooks, NKT_ARRAYLEN(sBaseHooks)));
  if (SUCCEEDED(hRes))
  {
    hPsApiDll = ::LoadLibraryW(L"psapi.dll");
    if (hPsApiDll != NULL)
    {
      fnEnumProcessModules = (lpfnEnumProcessModules)::GetProcAddress(hPsApiDll, "EnumProcessModules");
      if (fnEnumProcessModules != NULL)
      {
        dwSize = 256*sizeof(HMODULE);
        while (1)
        {
          lpModList = (HMODULE*)NKT_MALLOC(dwSize);
          if (lpModList == NULL)
          {
            hRes = E_OUTOFMEMORY;
            break;
          }
          if (fnEnumProcessModules(::GetCurrentProcess(), lpModList, dwSize, &dwNeeded) != FALSE)
          {
            dwSize = dwNeeded / sizeof(HMODULE);
            for (dw=0; dw<dwSize && SUCCEEDED(hRes); dw++)
            {
              if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
                hRes2 = cComHookMgr.ProcessDllLoad(lpModList[dw]);
              if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
                hRes2 = cJavaHookMgr.ProcessDllLoad(lpModList[dw]);
              //TODO: handle errors
            }
            NKT_FREE(lpModList);
            break;
          }
          NKT_FREE(lpModList);
          dwSize = dwNeeded;
        }
      }
      else
      {
        hRes = NKT_HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
      }
      ::FreeLibrary(hPsApiDll);
    }
    else
    {
      hRes = NKT_HRESULT_FROM_LASTERROR();
    }
  }
  return hRes;
}

BOOL CManager::Run()
{
  HANDLE hEvents[2];

  hEvents[0] = cShutdownEvent.GetEventHandle();
  hEvents[1] = cMasterProc.Get();
  ::WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
  return (nAppIsExiting != 0) ? TRUE : FALSE;
}

VOID CManager::OnExitProcess()
{
  NktInterlockedExchange(&nAppIsExiting, 1);
  cComHookMgr.OnExitProcess();;
  cJavaHookMgr.OnExitProcess();
  cShutdownEvent.Set();
  CNktThread::CoWaitAndDispatchMessages(INFINITE, 1, &hMainThread);
  return;
}

VOID CManager::InitShutdownBecauseError()
{
  cShutdownEvent.Set();
  return;
}

HRESULT CManager::SendMsg(__in LPVOID lpData, __in SIZE_T nDataSize, __inout_opt LPVOID *lplpReply,
                          __inout_opt SIZE_T *lpnReplySize)
{
  return lpConn->SendMsg(hConn, lpData, nDataSize, lplpReply, lpnReplySize);
}

NTSTATUS CManager::OnLdrLoadDll(__in_opt LPWSTR PathToFile, __in ULONG Flags,
                                __in PUNICODE_STRING ModuleFileName, __out HINSTANCE *ModuleHandle)
{
  //CNktAutoFastMutex cLock(&cMyLdrLock);
  CAutoUsageCount cAutoHookUsageCount(&nHookUsageCount);
  NTSTATUS nNtStatus;
  SIZE_T nUsageCount;
  HRESULT hRes;

  nNtStatus = lpfnLdrLoadDll(sBaseHooks[1].lpCallOriginal)(PathToFile, Flags, ModuleFileName,
                                                           ModuleHandle);
  if (NT_SUCCESS(nNtStatus) && ModuleHandle != NULL && (*ModuleHandle) != NULL)
  {
    if (SUCCEEDED(CNktTools::GetModuleUsageCount(*ModuleHandle, nUsageCount)) &&
        nUsageCount == 1)
    {
      if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
        hRes = cComHookMgr.ProcessDllLoad(*ModuleHandle);
      if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
        hRes = cJavaHookMgr.ProcessDllLoad(*ModuleHandle);
    //TODO: handle errors
    }
  }
  return nNtStatus;
}

NTSTATUS CManager::OnLdrUnloadDll(__in HINSTANCE ModuleHandle)
{
  //CNktAutoFastMutex cLock(&cMyLdrLock);
  CAutoUsageCount cAutoHookUsageCount(&nHookUsageCount);
  NTSTATUS nNtStatus;
  SIZE_T nUsageCount;
  HRESULT hRes;

  if (SUCCEEDED(CNktTools::GetModuleUsageCount(ModuleHandle, nUsageCount)) &&
      nUsageCount == 1)
  {
    if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
      hRes = cComHookMgr.ProcessDllUnload(ModuleHandle);
    if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
      hRes = cJavaHookMgr.ProcessDllUnload(ModuleHandle);
  }
  nNtStatus = lpfnLdrUnloadDll(sBaseHooks[2].lpCallOriginal)(ModuleHandle);
  return nNtStatus;
}

BOOL CManager::OnCreateProcessInternalW(__in HANDLE hToken, __in_z_opt LPCWSTR lpApplicationName,
                              __in_z_opt LPWSTR lpCommandLine, __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes,
                              __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, __in BOOL bInheritHandles,
                              __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment,
                              __in_z_opt LPCWSTR lpCurrentDirectory, __in_opt LPSTARTUPINFOW lpStartupInfo,
                              __out LPPROCESS_INFORMATION lpProcessInformation, __out PHANDLE hNewToken)
{
  CAutoUsageCount cAutoHookUsageCount(&nHookUsageCount);
  PROCESS_INFORMATION sPi;
  TNktAutoDeletePtr<CCreatedChildProcess> cChildProc;
  DWORD dwChildProcPlatformBits;
  HANDLE hReadyEvent, hContinueEvent;
  BOOL bWasSuspended;
  HRESULT hRes;

  if (lpProcessInformation == NULL || lpConn->IsConnected(hConn) != S_OK)
  {
    return lpfnCreateProcessInternalW(sBaseHooks[3].lpCallOriginal)(hToken, lpApplicationName, lpCommandLine,
                            lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
                            lpCurrentDirectory, lpStartupInfo, lpProcessInformation, hNewToken);
  }
  //suspend new process' main thread while informing the server
  bWasSuspended = ((dwCreationFlags & CREATE_SUSPENDED) != 0) ? TRUE : FALSE;
  dwCreationFlags |= CREATE_SUSPENDED;
  if (lpfnCreateProcessInternalW(sBaseHooks[3].lpCallOriginal)(hToken, lpApplicationName, lpCommandLine,
                       lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
                       lpCurrentDirectory, lpStartupInfo, lpProcessInformation, hNewToken) == FALSE)
    return FALSE;
  //on succeed
  try
  {
    memcpy(&sPi, lpProcessInformation, sizeof(sPi));
  }
  catch (...)
  {
    memset(&sPi, 0, sizeof(sPi));
  }
  if (sPi.hProcess == NULL)  //ignore call if memory is not readable
  {
err_cant_process:
    if (bWasSuspended != FALSE && sPi.hThread != NULL)
      ::ResumeThread(sPi.hThread);
    return TRUE;
  }
  //get child process platform bits
  dwChildProcPlatformBits = CNktTools::GetProcessPlatformBits(sPi.hProcess);
  switch (dwChildProcPlatformBits)
  {
    case 32:
      //try to complete the child process initialization by adding custom code at entry point
      hRes = CNktTools::SuspendAfterCreateProcessW(&hReadyEvent, &hContinueEvent, ::GetCurrentProcess(),
                                                   sPi.hProcess, sPi.hThread);
      if (FAILED(hRes))
        goto err_cant_process;
      break;

    case 64:
#if defined _M_IX86
      {
      TMSG_SUSPENDCHILDPROCESS sMsg;
      TMSG_SUSPENDCHILDPROCESS_REPLY *lpReply;
      SIZE_T nReplySize;
      HANDLE _h;

      //ask the server process to suspend the newly created process
      sMsg.sHeader.dwMsgCode = TMSG_CODE_SuspendChildProcess;
      if (::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentProcess(), cMasterProc, &_h, 0, FALSE,
                            DUPLICATE_SAME_ACCESS) == FALSE)
        goto err_cant_process;
      sMsg.hParentProc = (ULONGLONG)_h;
      if (::DuplicateHandle(::GetCurrentProcess(), sPi.hProcess, cMasterProc, &_h, 0, FALSE,
                            DUPLICATE_SAME_ACCESS) == FALSE)
      {
        CNktAutoRemoteHandle::CloseRemoteHandleSEH(cMasterProc, (HANDLE)sMsg.hParentProc);
        goto err_cant_process;
      }
      sMsg.hProcess = (ULONGLONG)_h;
      if (::DuplicateHandle(::GetCurrentProcess(), sPi.hThread, cMasterProc, &_h, 0, FALSE,
                            DUPLICATE_SAME_ACCESS) == FALSE)
      {
        CNktAutoRemoteHandle::CloseRemoteHandleSEH(cMasterProc, (HANDLE)sMsg.hParentProc);
        CNktAutoRemoteHandle::CloseRemoteHandleSEH(cMasterProc, (HANDLE)sMsg.hProcess);
        goto err_cant_process;
      }
      hRes = SendMsg(&sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
      if (FAILED(hRes))
      {
        InitShutdownBecauseError();
        goto err_cant_process;
      }
      if (nReplySize != sizeof(TMSG_SUSPENDCHILDPROCESS_REPLY))
      {
        lpConn->FreeBuffer(lpReply);
        InitShutdownBecauseError();
        goto err_cant_process;
      }
      if (FAILED(lpReply->hRes))
      {
        lpConn->FreeBuffer(lpReply);
        goto err_cant_process;
      }
      hContinueEvent = (HANDLE)(lpReply->hContinueEvent);
      hReadyEvent = (HANDLE)(lpReply->hReadyEvent);
      lpConn->FreeBuffer(lpReply);
      }
#elif defined _M_X64
      //try to complete the child process initialization by adding custom code at entry point
      hRes = CNktTools::SuspendAfterCreateProcessW(&hReadyEvent, &hContinueEvent, ::GetCurrentProcess(),
                                                   sPi.hProcess, sPi.hThread);
      if (FAILED(hRes))
        goto err_cant_process;
#endif
      break;

    default:
      goto err_cant_process;
  }
  //got the data, create the child process internal object
  cChildProc.Attach(new CCreatedChildProcess(sPi.dwProcessId, sPi.dwThreadId, dwChildProcPlatformBits));
  if (cChildProc == NULL)
  {
    CNktAutoHandle::CloseHandleSEH(hReadyEvent);
    CNktAutoHandle::CloseHandleSEH(hContinueEvent);
    goto err_cant_process;
  }
  if (FAILED(cChildProc->Init(sPi.hProcess, sPi.hThread, hReadyEvent, hContinueEvent)))
    goto err_cant_process;
  //we are done here
  if (bWasSuspended == FALSE)
  {
    HANDLE hEvents[2];

    ::ResumeThread(sPi.hThread);
    //wait until initialization completes (don't worry about return code)
    hEvents[0] = sPi.hThread;
    hEvents[1] = hReadyEvent;
    ::WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
    //if not suspended, send message to the server
    SendCreateProcessMsg(cChildProc);
  }
  else
  {
    //add the process to the "created processes" list if suspended
    CNktAutoFastMutex cListLock(&(sCreatedChildProcesses.cMtx));

    sCreatedChildProcesses.cList.PushTail(cChildProc.Detach());
  }
  return TRUE;
}

NTSTATUS CManager::OnNtSuspendThread(__in HANDLE ThreadHandle, __out_opt PULONG PreviousSuspendCount)
{
  CAutoUsageCount cAutoHookUsageCount(&nHookUsageCount);
  NTSTATUS nNtStatus;

  nNtStatus = lpfnNtSuspendThread(sBaseHooks[4].lpCallOriginal)(ThreadHandle, PreviousSuspendCount);
  if (NT_SUCCESS(nNtStatus))
  {
    TNktLnkLst<CCreatedChildProcess>::Iterator it;
    CCreatedChildProcess *lpChildProc;
    DWORD dwTid;

    dwTid = CNktTools::GetThreadId(ThreadHandle); //out of the lock to prevent deadlocks
    {
      CNktAutoFastMutex cListLock(&(sCreatedChildProcesses.cMtx));

      for (lpChildProc=it.Begin(sCreatedChildProcesses.cList); lpChildProc!=NULL; lpChildProc=it.Next())
      {
        if ((dwTid != 0) ? (dwTid == lpChildProc->dwTid) : (lpChildProc->hThread == ThreadHandle))
        {
          (lpChildProc->nSuspendCount)++;
          break;
        }
      }
    }
  }
  return nNtStatus;
}

NTSTATUS CManager::OnNtResumeThread(__in HANDLE ThreadHandle, __out_opt PULONG SuspendCount, __in BOOL bIsAlert)
{
  CAutoUsageCount cAutoHookUsageCount(&nHookUsageCount);
  TNktAutoDeletePtr<CCreatedChildProcess> cChildProc;
  NTSTATUS nNtStatus;

  if (bIsAlert == FALSE)
    nNtStatus = lpfnNtResumeThread(sBaseHooks[5].lpCallOriginal)(ThreadHandle, SuspendCount);
  else
    nNtStatus = lpfnNtAlertResumeThread(sBaseHooks[6].lpCallOriginal)(ThreadHandle, SuspendCount);
  if (NT_SUCCESS(nNtStatus))
  {
    TNktLnkLst<CCreatedChildProcess>::Iterator it;
    CCreatedChildProcess *lpChildProc;
    DWORD dwTid;

    dwTid = CNktTools::GetThreadId(ThreadHandle); //out of the lock to prevent deadlocks
    {
      CNktAutoFastMutex cListLock(&(sCreatedChildProcesses.cMtx));

      for (lpChildProc=it.Begin(sCreatedChildProcesses.cList); lpChildProc!=NULL; lpChildProc=it.Next())
      {
        if ((dwTid != 0) ? (dwTid == lpChildProc->dwTid) : (lpChildProc->hThread == ThreadHandle))
        {
          if ((--(lpChildProc->nSuspendCount)) == 0)
          {
            lpChildProc->RemoveNode();
            cChildProc.Attach(lpChildProc);
          }
          break;
        }
      }
    }
  }
  if (cChildProc != NULL)
  {
    HANDLE hEvents[2];
    HRESULT hRes;

    //wait until initialization completes (don't worry about return code)
    hEvents[0] = cChildProc->hThread;
    hEvents[1] = cChildProc->hReadyEvent;
    ::WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
    //send message to the server
    hRes = SendCreateProcessMsg(cChildProc);
  }
  return nNtStatus;
}

HRESULT CManager::OnServerMessage(__in HANDLE hConn, __in LPBYTE lpData, __in SIZE_T nDataSize,
                                  __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  union {
    TMSG_HEADER *lpMsgHdr;
    TMSG_WATCHCOMINTERFACE *lpMsgWatchComInt;
    TMSG_UNWATCHCOMINTERFACE *lpMsgUnwatchComInt;
    TMSG_INSPECTCOMINTERFACEMETHOD *lpMsgInspectComIntMethod;
    TMSG_GETCOMINTERFACEFROMINDEX *lpMsgGetComIntFromIndex;
    TMSG_GETCOMINTERFACEFROMHWND *lpMsgGetComIntFromHWnd;
    TMSG_FINDWINDOW *lpMsgFindWindow;
    TMSG_GETWINDOWTEXT *lpMsgGetWndText;
    TMSG_SETWINDOWTEXT *lpMsgSetWndText;
    TMSG_POSTSENDMESSAGE *lpMsgPostSendMsg;
    TMSG_GETCHILDWINDOW *lpMsgGetChildWnd;
    TMSG_DEFINEJAVACLASS *lpMsgDefineJavaClass;
    TMSG_CREATEJAVAOBJECT *lpMsgCreateJavaObject;
    TMSG_INVOKEJAVAMETHOD *lpMsgInvokeJavaMethod;
    TMSG_PUTJAVAFIELD *lpMsgPutJavaVariable;
    TMSG_GETJAVAFIELD *lpMsgGetJavaVariable;
    TMSG_ISSAMEJAVAOBJECT *lpMsgIsSameJavaObject;
    TMSG_REMOVEJOBJECTREF *lpMsgRemoveJObjectRef;
    TMSG_GETJAVAOBJECTCLASS *lpMsgGetJavaObjectClass;
    TMSG_ISJAVAOBJECTINSTANCEOF *lpMsgIsJavaObjectInstanceOf;
  };
  HRESULT hRes;

  if (nDataSize < sizeof(TMSG_HEADER))
  {
    _ASSERT(FALSE);
    hRes = NKT_E_InvalidData;
  }
  if (SUCCEEDED(hRes))
  {
    hRes = NKT_E_InvalidData;
    lpMsgHdr = (TMSG_HEADER*)lpData;
    switch (lpMsgHdr->dwMsgCode)
    {
      case TMSG_CODE_WatchComInterface:
        if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
          hRes = cComHookMgr.WatchInterface(lpMsgWatchComInt, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_UnwatchComInterface:
        if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
          hRes = cComHookMgr.UnwatchInterface(lpMsgUnwatchComInt, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_UnwatchAllComInterfaces:
        if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
          hRes = cComHookMgr.UnwatchAllInterfaces(lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_InspectComInterfaceMethod:
        if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
          hRes = cComHookMgr.InspectInterfaceMethod(lpMsgInspectComIntMethod, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_GetComInterfaceFromHWnd:
        if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
          hRes = cComHookMgr.GetInterfaceFromHWnd(lpMsgGetComIntFromHWnd, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_GetComInterfaceFromIndex:
        if ((nHookFlags & (ULONG)flgDisableComHooking) == 0)
          hRes = cComHookMgr.GetInterfaceFromIndex(lpMsgGetComIntFromIndex, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_FindWindow:
        {
        EWP_FindWindow_DATA sData;

        if (nDataSize < sizeof(TMSG_FINDWINDOW))
          break;
        if (nDataSize != sizeof(TMSG_FINDWINDOW)+(SIZE_T)(lpMsgFindWindow->nNameLen+lpMsgFindWindow->nClassLen))
          break;
        memset(&sData, 0, sizeof(sData));
        sData.lpMsgFindWindow = lpMsgFindWindow;
        if ((HWND)(lpMsgFindWindow->hParentWnd) == NULL)
          ::EnumWindows(EWP_FindWindow, (LPARAM)&sData);
        else
          ::EnumChildWindows((HWND)(lpMsgFindWindow->hParentWnd), ECWP_FindWindowChild, (LPARAM)&sData);
        if (sData.lpMem != NULL)
          NKT_FREE(sData.lpMem);
        if (FAILED(sData.hRes))
          break;
        *lplpReplyData = lpConn->AllocBuffer(sizeof(TMSG_FINDWINDOW_REPLY));
        if ((*lplpReplyData) == NULL)
        {
          hRes = E_OUTOFMEMORY;
          break;
        }
        ((TMSG_FINDWINDOW_REPLY*)(*lplpReplyData))->hWnd = (ULONGLONG)(sData.hWnd);
        *lpnReplyDataSize = sizeof(TMSG_FINDWINDOW_REPLY);
        hRes = S_OK;
        }
        break;

      case TMSG_CODE_GetWindowText:
        {
        TMSG_GETWINDOWTEXT_REPLY *lpReply;
        UINT nLen;

        if (nDataSize != sizeof(TMSG_GETWINDOWTEXT))
          break;
        hRes = S_OK;
        nLen = (UINT)::GetWindowTextLengthW((HWND)(lpMsgGetWndText->hWnd));
        *lplpReplyData = lpConn->AllocBuffer(sizeof(TMSG_GETWINDOWTEXT_REPLY)+nLen*sizeof(WCHAR));
        if ((*lplpReplyData) == NULL)
        {
          hRes = E_OUTOFMEMORY;
          break;
        }
        lpReply = (TMSG_GETWINDOWTEXT_REPLY*)(*lplpReplyData);
        lpReply->nTextLen = (ULONGLONG)(nLen*sizeof(WCHAR));
        ::GetWindowTextW((HWND)(lpMsgGetWndText->hWnd), lpReply->szTextW, nLen+1);
        lpReply->szTextW[nLen] = 0;
        *lpnReplyDataSize = sizeof(TMSG_GETWINDOWTEXT_REPLY)+nLen*sizeof(WCHAR);
        hRes = S_OK;
        }
        break;

      case TMSG_CODE_SetWindowText:
        {
        LPWSTR sW;
        SIZE_T nLen;

        if (nDataSize < sizeof(TMSG_SETWINDOWTEXT) ||
            ((nDataSize-sizeof(TMSG_SETWINDOWTEXT)) & 1) != 0)
          break;
        sW = (LPWSTR)(lpMsgSetWndText+1);
        nLen = (nDataSize - sizeof(TMSG_SETWINDOWTEXT)) / sizeof(WCHAR);
        if (nLen < 1 || sW[nLen-1] != 0)
          break;
        hRes = (::SetWindowTextW((HWND)(lpMsgSetWndText->hWnd), sW) != FALSE) ? S_OK : NKT_HRESULT_FROM_LASTERROR();
        if (hRes != E_OUTOFMEMORY)
          hRes = BuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
        }
        break;

      case TMSG_CODE_PostSendMessage:
        {
        DWORD_PTR dwRes;
        LRESULT lRes;

        if (nDataSize != sizeof(TMSG_POSTSENDMESSAGE))
          break;
        *lplpReplyData = lpConn->AllocBuffer(sizeof(TMSG_POSTSENDMESSAGE_REPLY));
        if ((*lplpReplyData) == NULL)
        {
          hRes = E_OUTOFMEMORY;
          break;
        }
        memset(*lplpReplyData, 0, sizeof(TMSG_POSTSENDMESSAGE_REPLY));
        if (lpMsgPostSendMsg->bPost == FALSE)
        {
          if (lpMsgPostSendMsg->nTimeout == ULONG_MAX)
          {
            lRes = ::SendMessageW((HWND)(lpMsgPostSendMsg->hWnd), (UINT)(lpMsgPostSendMsg->nMsg),
                                  (WPARAM)(lpMsgPostSendMsg->wParam), (LPARAM)(lpMsgPostSendMsg->lParam));
            ((TMSG_POSTSENDMESSAGE_REPLY*)(*lplpReplyData))->nResult = (ULONGLONG)(ULONG_PTR)lRes;
          }
          else
          {
            lRes = ::SendMessageTimeoutW((HWND)(lpMsgPostSendMsg->hWnd), (UINT)(lpMsgPostSendMsg->nMsg),
                                         (WPARAM)(lpMsgPostSendMsg->wParam), (LPARAM)(lpMsgPostSendMsg->lParam),
                                         SMTO_BLOCK, (UINT)(lpMsgPostSendMsg->nTimeout), &dwRes);
            if (lRes != 0)
            {
              ((TMSG_POSTSENDMESSAGE_REPLY*)(*lplpReplyData))->nResult = (ULONGLONG)dwRes;
            }
            else
            {
              ((TMSG_POSTSENDMESSAGE_REPLY*)(*lplpReplyData))->hRes = NKT_HRESULT_FROM_LASTERROR();
            }
          }
        }
        else
        {
          if (::PostMessageW((HWND)(lpMsgPostSendMsg->hWnd), (UINT)(lpMsgPostSendMsg->nMsg),
                             (WPARAM)(lpMsgPostSendMsg->wParam), (LPARAM)(lpMsgPostSendMsg->lParam)) == FALSE)
            ((TMSG_POSTSENDMESSAGE_REPLY*)(*lplpReplyData))->hRes = E_FAIL;
        }
        *lpnReplyDataSize = sizeof(TMSG_POSTSENDMESSAGE_REPLY);
        hRes = S_OK;
        }
        break;

      case TMSG_CODE_GetChildWindow:
        {
        EWP_GetChildWindow_DATA sData;
        DWORD dwPid;

        if (nDataSize != sizeof(TMSG_GETCHILDWINDOW))
          break;
        *lplpReplyData = lpConn->AllocBuffer(sizeof(TMSG_GETCHILDWINDOW_REPLY));
        if ((*lplpReplyData) == NULL)
        {
          hRes = E_OUTOFMEMORY;
          break;
        }
        memset(*lplpReplyData, 0, sizeof(TMSG_GETCHILDWINDOW_REPLY));
        ::GetWindowThreadProcessId((HWND)(lpMsgGetChildWnd->hParentWnd), &dwPid);
        if (dwPid == ::GetCurrentProcessId())
        {
          if (lpMsgGetChildWnd->bIsIndex == FALSE)
          {
            ((TMSG_GETCHILDWINDOW_REPLY*)(*lplpReplyData))->hWnd = (ULONGLONG)::GetDlgItem(
                            (HWND)(lpMsgGetChildWnd->hParentWnd), (int)(LONG)(lpMsgGetChildWnd->nIdOrIndex));
          }
          else
          {
            sData.lpReply = (TMSG_GETCHILDWINDOW_REPLY*)(*lplpReplyData);
            sData.nIndex = (SIZE_T)(lpMsgGetChildWnd->nIdOrIndex);
            ::EnumChildWindows((HWND)(lpMsgGetChildWnd->hParentWnd), ECWP_GetChildWnd, (LPARAM)&sData);
          }
        }
        else
        {
          ((TMSG_GETCHILDWINDOW_REPLY*)(*lplpReplyData))->hRes = E_INVALIDARG;
        }
        *lpnReplyDataSize = sizeof(TMSG_GETCHILDWINDOW_REPLY);
        hRes = S_OK;
        }
        break;

      case TMSG_CODE_DefineJavaClass:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.DefineJavaClass(lpMsgDefineJavaClass, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_CreateJavaObject:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.CreateJavaClass(lpMsgCreateJavaObject, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_InvokeJavaMethod:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.InvokeJavaMethod(lpMsgInvokeJavaMethod, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_PutJavaField:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.SetJavaField(lpMsgPutJavaVariable, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_GetJavaField:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.GetJavaField(lpMsgGetJavaVariable, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_IsSameJavaObject:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.IsSameJavaObject(lpMsgIsSameJavaObject, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_RemoveJObjectRef:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.RemoveJObjectRef(lpMsgRemoveJObjectRef, nDataSize);
        break;

      case TMSG_CODE_IsJVMAttached:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.IsJVMAttached(lpMsgHdr, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_GetJavaObjectClass:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
          hRes = cJavaHookMgr.GetJavaObjectClass(lpMsgGetJavaObjectClass, nDataSize, lplpReplyData, lpnReplyDataSize);
        break;

      case TMSG_CODE_IsJavaObjectInstanceOf:
        if ((nHookFlags & (ULONG)flgDisableJavaHooking) == 0)
        {
          hRes = cJavaHookMgr.IsJavaObjectInstanceOf(lpMsgIsJavaObjectInstanceOf, nDataSize, lplpReplyData,
                                                     lpnReplyDataSize);
        }
        break;

      default:
        _ASSERT(FALSE);
        break;
    }
  }
  if (FAILED(hRes))
    InitShutdownBecauseError();
  return hRes;
}

HRESULT CManager::SendCreateProcessMsg(__in CCreatedChildProcess *lpChildProc)
{
  TMSG_RAISEONCREATEPROCESSEVENT sMsg;
  TMSG_SIMPLE_REPLY *lpReply;
  SIZE_T nReplySize;
  HRESULT hRes;

  //ask the server process to suspend the newly created process
  sMsg.sHeader.dwMsgCode = TMSG_CODE_RaiseOnCreateProcessEvent;
  sMsg.dwPid = lpChildProc->dwPid;
  sMsg.dwTid = lpChildProc->dwTid;
  sMsg.dwPlatformBits = lpChildProc->dwPlatformBits;
  hRes = SendMsg(&sMsg, sizeof(sMsg), (LPVOID*)&lpReply, &nReplySize);
  if (SUCCEEDED(hRes))
  {
    if (nReplySize != sizeof(TMSG_SIMPLE_REPLY) || FAILED(lpReply->hRes))
    {
      _ASSERT(FALSE);
      hRes = NKT_E_InvalidData;
    }
    lpConn->FreeBuffer(lpReply);
  }
  return hRes;
}

HRESULT CManager::BuildSimpleReplyData(__in HRESULT hRes, __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
{
  *lplpReplyData = lpConn->AllocBuffer(sizeof(TMSG_SIMPLE_REPLY));
  if ((*lplpReplyData) == NULL)
    return E_OUTOFMEMORY;
  ((TMSG_SIMPLE_REPLY*)(*lplpReplyData))->hRes = hRes;
  *lpnReplyDataSize = sizeof(TMSG_SIMPLE_REPLY);
  return S_OK;
}

VOID CManager::OnComInterfaceCreated(__in REFCLSID sClsId, __in REFIID sIid, __in IUnknown *lpUnk)
{
  TMSG_RAISEONCOMINTERFACECREATEDEVENT *lpMsg;
  TMSG_SIMPLE_REPLY *lpReply;
  SIZE_T nReplySize, nDataSize;
  HRESULT hRes;

  hRes = cComHookMgr.BuildInterprocMarshalPacket(lpUnk, sIid, sizeof(TMSG_RAISEONCOMINTERFACECREATEDEVENT),
                                                 (LPVOID*)&lpMsg, &nDataSize);
  if (SUCCEEDED(hRes))
  {
    lpMsg->sHeader.dwMsgCode = TMSG_CODE_RaiseOnComInterfaceCreatedEvent;
    lpMsg->dwTid = ::GetCurrentThreadId();
    lpMsg->sClsId = sClsId;
    lpMsg->sIid = sIid;
    hRes = SendMsg(lpMsg, nDataSize, (LPVOID*)&lpReply, &nReplySize);
    NKT_FREE(lpMsg);
    if (SUCCEEDED(hRes))
    {
      if (nReplySize != sizeof(TMSG_SIMPLE_REPLY) || FAILED(lpReply->hRes))
      {
        _ASSERT(FALSE);
        hRes = NKT_E_InvalidData;
      }
      lpConn->FreeBuffer(lpReply);
    }
  }
  if (FAILED(hRes))
    InitShutdownBecauseError();
  return;
}

LPBYTE CManager::OnComAllocIpcBuffer(__in SIZE_T nSize)
{
  return lpConn->AllocBuffer(nSize);
}

VOID CManager::OnComFreeIpcBuffer(__in LPVOID lpBuffer)
{
  lpConn->FreeBuffer(lpBuffer);
  return;
}

HRESULT CManager::OnComBuildSimpleReplyData(__in HRESULT hRes, __out LPVOID *lplpReplyData,
                                            __out SIZE_T *lpnReplyDataSize)
{
  return BuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
}

VOID CManager::OnComHookUsage(__in BOOL bEntering)
{
  if (bEntering != FALSE)
    NktInterlockedIncrement(&nHookUsageCount);
  else
    NktInterlockedDecrement(&nHookUsageCount);
  return;
}

HRESULT CManager::OnJavaRaiseJavaNativeMethodCalled(__in TMSG_RAISEONJAVANATIVEMETHODCALLED *lpMsg,
                                 __in SIZE_T nMsgSize, __out TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY **lplpReply,
                                 __out SIZE_T *lpnReplySize)
{
  HRESULT hRes;

  hRes = SendMsg(lpMsg, nMsgSize, (LPVOID*)lplpReply, lpnReplySize);
  if (FAILED(hRes))
    InitShutdownBecauseError();
  return hRes;
}

LPBYTE CManager::OnJavaAllocIpcBuffer(__in SIZE_T nSize)
{
  return lpConn->AllocBuffer(nSize);
}

VOID CManager::OnJavaFreeIpcBuffer(__in LPVOID lpBuffer)
{
  lpConn->FreeBuffer(lpBuffer);
  return;
}

HRESULT CManager::OnJavaBuildSimpleReplyData(__in HRESULT hRes, __out LPVOID *lplpReplyData,
                                             __out SIZE_T *lpnReplyDataSize)
{
  return BuildSimpleReplyData(hRes, lplpReplyData, lpnReplyDataSize);
}

VOID CManager::OnJavaHookUsage(__in BOOL bEntering)
{
  if (bEntering != FALSE)
    NktInterlockedIncrement(&nHookUsageCount);
  else
    NktInterlockedDecrement(&nHookUsageCount);
  return;
}

//-----------------------------------------------------------

CManager::CCreatedChildProcess::CCreatedChildProcess(__in DWORD _dwPid, __in DWORD _dwTid, __in DWORD _dwPlatformBits) :
                                                     TNktLnkLstNode<CCreatedChildProcess>()
{
  dwPid = _dwPid;
  dwTid = _dwTid;
  dwPlatformBits = _dwPlatformBits;
  nSuspendCount = 1;
  hProcess = hThread = hReadyEvent = hContinueEvent = NULL;
  return;
}

CManager::CCreatedChildProcess::~CCreatedChildProcess()
{
  if (hThread != NULL)
    CNktAutoHandle::CloseHandleSEH(hThread);
  if (hProcess != NULL)
    CNktAutoHandle::CloseHandleSEH(hProcess);
  if (hReadyEvent != NULL)
    CNktAutoHandle::CloseHandleSEH(hReadyEvent);
  if (hContinueEvent != NULL)
  {
    ::SetEvent(hContinueEvent);
    CNktAutoHandle::CloseHandleSEH(hContinueEvent);
  }
  return;
}

HRESULT CManager::CCreatedChildProcess::Init(__in HANDLE _hProcess, __in HANDLE _hThread, __in HANDLE _hReadyEvent,
                                             __in HANDLE _hContinueEvent)
{
  hReadyEvent = _hReadyEvent;
  hContinueEvent = _hContinueEvent;
  if (::DuplicateHandle(::GetCurrentProcess(), _hProcess, ::GetCurrentProcess(), &hProcess, 0, FALSE,
                        DUPLICATE_SAME_ACCESS) == FALSE ||
      ::DuplicateHandle(::GetCurrentProcess(), _hThread, ::GetCurrentProcess(), &hThread, 0, FALSE,
                        DUPLICATE_SAME_ACCESS) == FALSE)
    return NKT_HRESULT_FROM_LASTERROR();
  return S_OK;
}

//-----------------------------------------------------------

VOID OnProcessDetach(__in LPVOID lpReserved)
{
  if (lpReserved != NULL)
  {
    //NULL=FreeLibrary was called / NON-NULL=process terminating
    if (hMainThread != NULL)
    {
      ::CloseHandle(hMainThread);
      hMainThread = NULL;
    }
  }
  return;
}

} //MainManager

//-----------------------------------------------------------

static DWORD WINAPI MainThreadProc(__in LPVOID lpParameter)
{
  MainManager::CManager *lpMgr = (MainManager::CManager*)lpParameter;

  if (lpMgr->Run() == FALSE)
  {
    if (MainManager::lpMainMgr != NULL)
    {
      MainManager::lpMainMgr->~CManager();
      InterlockedExchangePointer((PVOID volatile*)&(MainManager::lpMainMgr), NULL);
    }
    ::FreeLibraryAndExitThread(MainManager::hDllInst, 0);
  }
  return 0;
}

static VOID WINAPI OnExitProcess(__in UINT uExitCode)
{
  if (MainManager::lpMainMgr != NULL)
  {
    MainManager::lpMainMgr->OnExitProcess();
    MainManager::lpMainMgr->~CManager();
    InterlockedExchangePointer((PVOID volatile*)&(MainManager::lpMainMgr), NULL);
  }
  //at this point the hook should went out so call the original api again
  ::ExitProcess(uExitCode);
  //lpfnExitProcess(MainManager::sBaseHooks[0].lpCallOriginal)(uExitCode);
  return;
}

static NTSTATUS NTAPI OnLdrLoadDll(__in_opt LPWSTR PathToFile, __in ULONG Flags,
                                   __in PUNICODE_STRING ModuleFileName, __out HINSTANCE *ModuleHandle)
{
  return MainManager::lpMainMgr->OnLdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
}

static NTSTATUS NTAPI OnLdrUnloadDll(__in HINSTANCE ModuleHandle)
{
  return MainManager::lpMainMgr->OnLdrUnloadDll(ModuleHandle);
}

static NTSTATUS NTAPI OnNtSuspendThread(__in HANDLE ThreadHandle, __out_opt PULONG PreviousSuspendCount)
{
  return MainManager::lpMainMgr->OnNtSuspendThread(ThreadHandle, PreviousSuspendCount);
}

static NTSTATUS NTAPI OnNtResumeThread(__in HANDLE ThreadHandle, __out_opt PULONG SuspendCount)
{
  return MainManager::lpMainMgr->OnNtResumeThread(ThreadHandle, SuspendCount, FALSE);
}

static NTSTATUS NTAPI OnNtAlertResumeThread(__in HANDLE ThreadHandle, __out_opt PULONG SuspendCount)
{
  return MainManager::lpMainMgr->OnNtResumeThread(ThreadHandle, SuspendCount, TRUE);
}

static BOOL WINAPI OnCreateProcessInternalW(__in HANDLE hToken, __in_z_opt LPCWSTR lpApplicationName,
                           __in_z_opt LPWSTR lpCommandLine, __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes,
                           __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, __in BOOL bInheritHandles,
                           __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment,
                           __in_z_opt LPCWSTR lpCurrentDirectory, __in_opt LPSTARTUPINFOW lpStartupInfo,
                           __out LPPROCESS_INFORMATION lpProcessInformation, __out PHANDLE hNewToken)
{
  return MainManager::lpMainMgr->OnCreateProcessInternalW(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes,
                                                          lpThreadAttributes, bInheritHandles, dwCreationFlags,
                                                          lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                                          lpProcessInformation, hNewToken);
}

static BOOL EWP_FindWindow_EnsureMemory(__in EWP_FindWindow_DATA *lpData, __in SIZE_T nSize)
{
  if (nSize > lpData->nMemSize)
  {
    if (lpData->lpMem != NULL)
      NKT_FREE(lpData->lpMem);
    lpData->lpMem = (LPWSTR)NKT_MALLOC(nSize);
    if (lpData->lpMem == NULL)
    {
      lpData->hRes = E_OUTOFMEMORY;
      return FALSE;
    }
    lpData->nMemSize = nSize;
  }
  return TRUE;
}

static BOOL CALLBACK EWP_FindWindow(__in HWND hWnd, __in LPARAM lParam)
{
  EWP_FindWindow_DATA *lpData = (EWP_FindWindow_DATA*)lParam;
  LPWSTR sW;
  DWORD dwPid;
  int nLen;

  ::GetWindowThreadProcessId(hWnd, &dwPid);
  if (dwPid != ::GetCurrentProcessId())
    return TRUE;
  //match window text
  if (lpData->lpMsgFindWindow->nNameLen > 0)
  {
    nLen = ::GetWindowTextLengthW(hWnd);
    if (EWP_FindWindow_EnsureMemory(lpData, ((SIZE_T)(UINT)nLen+1)*sizeof(WCHAR)) == FALSE)
      return FALSE;
    sW = (LPWSTR)(lpData->lpMsgFindWindow+1);
    nLen = ::GetWindowTextW(hWnd, lpData->lpMem, nLen+1);
    lpData->lpMem[nLen] = 0;
    if (WildcardMatch(lpData->lpMem, (SIZE_T)(UINT)nLen, sW) == FALSE)
      goto ewp_fw_process_next;
  }
  //match class name
  if (lpData->lpMsgFindWindow->nClassLen > 0)
  {
    if (EWP_FindWindow_EnsureMemory(lpData, 512*sizeof(WCHAR)) == FALSE)
      return FALSE;
    sW = (LPWSTR)((LPBYTE)(lpData->lpMsgFindWindow+1) + (SIZE_T)(lpData->lpMsgFindWindow->nNameLen));
    nLen = ::GetClassNameW(hWnd, lpData->lpMem, 512);
    lpData->lpMem[nLen] = 0;
    if (WildcardMatch(lpData->lpMem, (SIZE_T)(UINT)nLen, sW) == FALSE)
      goto ewp_fw_process_next;
  }
  //got a match
  lpData->hWnd = hWnd;
  return FALSE;

ewp_fw_process_next:
  if (lpData->lpMsgFindWindow->bRecurseChilds != FALSE)
  {
    ::EnumChildWindows(hWnd, ECWP_FindWindowChild, lParam);
    if (lpData->hRes != S_OK || lpData->hWnd != NULL)
      return FALSE;
  }
  return TRUE;
}

static BOOL CALLBACK ECWP_FindWindowChild(__in HWND hWnd, __in LPARAM lParam)
{
  EWP_FindWindow_DATA *lpData = (EWP_FindWindow_DATA*)lParam;
  LPWSTR sW;
  int nLen;

  //match window text
  if (lpData->lpMsgFindWindow->nNameLen > 0)
  {
    nLen = ::GetWindowTextLengthW(hWnd);
    if (EWP_FindWindow_EnsureMemory(lpData, (nLen+1)*sizeof(WCHAR)) == FALSE)
      return FALSE;
    sW = (LPWSTR)(lpData->lpMsgFindWindow+1);
    nLen = ::GetWindowTextW(hWnd, lpData->lpMem, nLen+1);
    lpData->lpMem[nLen] = 0;
    if (WildcardMatch(lpData->lpMem, (SIZE_T)(UINT)nLen, sW) == FALSE)
      goto ewp_fw_process_next;
  }
  //match class name
  if (lpData->lpMsgFindWindow->nClassLen > 0)
  {
    if (EWP_FindWindow_EnsureMemory(lpData, 512*sizeof(WCHAR)) == FALSE)
      return FALSE;
    sW = (LPWSTR)((LPBYTE)(lpData->lpMsgFindWindow+1) + (SIZE_T)(lpData->lpMsgFindWindow->nNameLen));
    nLen = ::GetClassNameW(hWnd, lpData->lpMem, 512);
    lpData->lpMem[nLen] = 0;
    if (WildcardMatch(lpData->lpMem, (SIZE_T)(UINT)nLen, sW) == FALSE)
      goto ewp_fw_process_next;
  }
  //got a match
  lpData->hWnd = hWnd;
  return FALSE;

ewp_fw_process_next:
  if (lpData->lpMsgFindWindow->bRecurseChilds != FALSE)
  {
    ::EnumChildWindows(hWnd, ECWP_FindWindowChild, lParam);
    if (lpData->hRes != S_OK || lpData->hWnd != NULL)
      return FALSE;
  }
  return TRUE;
}

static BOOL CALLBACK ECWP_GetChildWnd(__in HWND hWnd, __in LPARAM lParam)
{
  EWP_GetChildWindow_DATA *lpData = (EWP_GetChildWindow_DATA*)lParam;

  if (lpData->nIndex == 0)
  {
    lpData->lpReply->hWnd = (ULONGLONG)hWnd;
    return FALSE;
  }
  (lpData->nIndex)--;
  return TRUE;
}

static BOOL WildcardMatch(__in LPCWSTR szW, __in SIZE_T nMaxLen, __in LPCWSTR szPatternW)
{
  SIZE_T nOffset;
  BOOL bStar;
  LPCWSTR p;

  bStar = FALSE;
loopStart:
  for (nOffset=0,p=szPatternW; nOffset<nMaxLen && szW[nOffset]!=0; nOffset++,p++)
  {
    switch (*p)
    {
      case L'?':
        break;

      case L'*':
        bStar = TRUE;
        szW += nOffset;
        nMaxLen -= nOffset;
        szPatternW = p;
        do
        {
          szPatternW++;
        }
        while ((*szPatternW) == L'*');
        if ((*szPatternW) == 0)
          return TRUE;
        goto loopStart;

      case L'\\':
        p++; //treat the next char as a literal
        //fall into default

      default:
        if (::CharUpperW((LPWSTR)(szW[nOffset])) != ::CharUpperW((LPWSTR)(*p)))
          goto starCheck;
        break;
    }
  }
  while ((*p) == L'*')
    p++;
  return ((*p) == 0) ? TRUE : FALSE;

starCheck:
  if (bStar == FALSE)
    return FALSE;
  szW++;
  nMaxLen--;
  goto loopStart;
}
