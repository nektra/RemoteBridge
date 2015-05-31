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

#include "RemoteBridgeHelper.h"

//-----------------------------------------------------------

#if defined _M_IX86
  #define HOOKTRAMPOLINECREATE(hHeap, lpCode, lpFunc)      \
                   HookTrampoline_Create(hHeap, lpCode##X86, NKT_ARRAYLEN(lpCode##X86), this, lpFunc)
  #define HOOKTRAMPOLINE2CREATE(hHeap, lpCode, lpFunc, bUseShared)     \
                   HookTrampoline2_Create(hHeap, lpCode##X86, NKT_ARRAYLEN(lpCode##X86), this, lpFunc, bUseShared)
  #define HOOKTRAMPOLINE2CREATE_WITHOBJ(hHeap, lpCode, _this, lpFunc, bUseShared)     \
                   HookTrampoline2_Create(hHeap, lpCode##X86, NKT_ARRAYLEN(lpCode##X86), _this, lpFunc, bUseShared)
#elif defined _M_X64
  #define HOOKTRAMPOLINECREATE(hHeap, lpCode, lpFunc)      \
                   HookTrampoline_Create(hHeap, lpCode##X64, NKT_ARRAYLEN(lpCode##X64), this, lpFunc)
  #define HOOKTRAMPOLINE2CREATE(hHeap, lpCode, lpFunc, bUseShared)     \
                   HookTrampoline2_Create(hHeap, lpCode##X64, NKT_ARRAYLEN(lpCode##X64), this, lpFunc, bUseShared)
  #define HOOKTRAMPOLINE2CREATE_WITHOBJ(hHeap, lpCode, _this, lpFunc, bUseShared)     \
                   HookTrampoline2_Create(hHeap, lpCode##X64, NKT_ARRAYLEN(lpCode##X64), _this, lpFunc, bUseShared)
#endif

//-----------------------------------------------------------

LPBYTE HookTrampoline_Create(__in HANDLE hHeap, __in LPBYTE lpCode, __in SIZE_T nCodeSize, __in LPVOID lpCtx,
                             __in LPVOID lpFunc);
VOID HookTrampoline_Delete(__in HANDLE hHeap, __in LPVOID lpTrampoline);

LPBYTE HookTrampoline2_Create(__in HANDLE hHeap, __in LPBYTE lpCode, __in SIZE_T nCodeSize, __in LPVOID lpCtx,
                             __in LPVOID lpFunc, __in BOOL bUseShared);
VOID HookTrampoline2_SetCallOriginalProc(__in LPVOID lpTrampoline, __in LPVOID lpCallOrigProc);
VOID HookTrampoline2_Disable(__in LPVOID lpTrampoline, __in BOOL bLock);
VOID HookTrampoline2_Delete(__in HANDLE hHeap, __in LPVOID lpTrampoline);
