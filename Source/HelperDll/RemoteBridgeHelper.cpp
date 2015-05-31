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

#include "RemoteBridgeHelper.h"
#include "MainManager.h"
#include "..\Common\StringLiteW.h"
#include "..\Common\Debug.h"

//-----------------------------------------------------------

#ifdef _DEBUG
  #define ATTACH_2_DEBUGGER_AT_STARTUP
#else //_DEBUG
  //#define ATTACH_2_DEBUGGER_AT_STARTUP
#endif //_DEBUG

#if defined _M_IX86
  #ifdef _DEBUG
    #pragma comment (lib, "..\\..\\Dependencies\\Deviare-InProc\\Libs\\2012\\NktHookLib_Debug.lib")
  #else //_DEBUG
    #pragma comment (lib, "..\\..\\Dependencies\\Deviare-InProc\\Libs\\2012\\NktHookLib.lib")
  #endif //_DEBUG
#elif defined _M_X64
  #ifdef _DEBUG
    #pragma comment (lib, "..\\..\\Dependencies\\Deviare-InProc\\Libs\\2012\\NktHookLib64_Debug.lib")
  #else //_DEBUG
    #pragma comment (lib, "..\\..\\Dependencies\\Deviare-InProc\\Libs\\2012\\NktHookLib64.lib")
  #endif //_DEBUG
#else
  #error Unsupported platform
#endif

//-----------------------------------------------------------

#ifdef ATTACH_2_DEBUGGER_AT_STARTUP
static BOOL AttachCurrentProcessToDebugger();
#endif //ATTACH_2_DEBUGGER_AT_STARTUP

//-----------------------------------------------------------

BOOL APIENTRY DllMain(__in HMODULE hModule, __in DWORD dwUlReasonForCall, __in LPVOID lpReserved)
{
  switch (dwUlReasonForCall)
  {
    case DLL_PROCESS_ATTACH:
      MainManager::hDllInst = hModule;
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      ::DisableThreadLibraryCalls(hModule);
      MainManager::OnProcessDetach(lpReserved);
      break;
  }
  return TRUE;
}

STDAPI Initialize(__in INIT_DATA *lpInitData)
{
#ifdef ATTACH_2_DEBUGGER_AT_STARTUP
  AttachCurrentProcessToDebugger();
#endif //ATTACH_2_DEBUGGER_AT_STARTUP
  return MainManager::Initialize(lpInitData);
}

#ifdef ATTACH_2_DEBUGGER_AT_STARTUP
static BOOL AttachCurrentProcessToDebugger()
{
  STARTUPINFO sSi;
  PROCESS_INFORMATION sPi;
  CNktStringW cStrTempW;
  SIZE_T i;
  BOOL b;
  DWORD dwExitCode;

  if (::IsDebuggerPresent() == FALSE)
  {
    memset(&sSi, 0, sizeof(sSi));
    memset(&sPi, 0, sizeof(sPi));
    if (cStrTempW.EnsureBuffer(406) == FALSE)
      return FALSE;
    ::GetSystemDirectoryW(cStrTempW, 4096);
    if (cStrTempW.Insert(L"\"", 0) == FALSE ||
        cStrTempW.Concat(L"VSJitDebugger.exe\" -p ") == FALSE ||
        cStrTempW.Concat(::GetCurrentProcessId()) == FALSE)
      return FALSE;
    b = ::CreateProcessW(NULL, (LPWSTR)cStrTempW, NULL, NULL, FALSE, 0, NULL, NULL, &sSi, &sPi);
    if (b != FALSE)
    {
      ::WaitForSingleObject(sPi.hProcess, INFINITE);
      ::GetExitCodeProcess(sPi.hProcess, &dwExitCode);
      if (dwExitCode != 0)
        b = FALSE;
    }
    if (sPi.hThread != NULL)
      ::CloseHandle(sPi.hThread);
    if (sPi.hProcess != NULL)
      ::CloseHandle(sPi.hProcess);
    if (b == FALSE)
      return FALSE;
    for (i=0; i<5*60; i++)
    {
      if (::IsDebuggerPresent() != FALSE)
        break;
      ::Sleep(200);
    }
  }
  if (::IsDebuggerPresent() != FALSE)
    Nektra::DebugBreak();
  return TRUE;
}
#endif //ATTACH_2_DEBUGGER_AT_STARTUP
