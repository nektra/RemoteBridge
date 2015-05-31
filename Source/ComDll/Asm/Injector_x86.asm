;
; Copyright (C) 2010-2015 Nektra S.A., Buenos Aires, Argentina.
; All rights reserved. Contact: http://www.nektra.com
;
;
; This file is part of Remote Bridge
;
;
; Commercial License Usage
; ------------------------
; Licensees holding valid commercial Remote Bridge licenses may use this
; file in accordance with the commercial license agreement provided with
; the Software or, alternatively, in accordance with the terms contained
; in a written agreement between you and Nektra.  For licensing terms and
; conditions see http://www.nektra.com/licensing/. Use the contact form
; at http://www.nektra.com/contact/ for further information.
;
;
; GNU General Public License Usage
; --------------------------------
; Alternatively, this file may be used under the terms of the GNU General
; Public License version 3.0 as published by the Free Software Foundation
; and appearing in the file LICENSE.GPL included in the packaging of this
; file.  Please visit http://www.gnu.org/copyleft/gpl.html and review the
; information to ensure the GNU General Public License version 3.0
; requirements will be met.
;
;

.386
.model flat, stdcall
.code

_TEXT SEGMENT

;---------------------------------------------------------------------------------

INIT:
    jmp   @@RealInit

INCLUDE GetProcAddress_x86.inc.asm

;---------------------------------------------------------------------------------

@@RealInit:
lpData$ = 8h
hNtDll$ = -4h
fnLdrLoadDll$ = -8h
fnLdrUnloadDll$ = -0Ch
hMod$ = -10h
nExitCode$ = -14h
fnInitialize$ = -18h
sDllNameUS$ = -20h ;sizeof(UNICODE_STRING32)=8

    push ebp
    mov  ebp, esp
    sub  esp, 40h ;locals

    ;find needed apis
    GetDataPtr eax, @@aNtDllDll
    push eax
    call GetModuleBaseAddress
    mov  DWORD PTR hNtDll$[ebp], eax
    test eax, eax
    je   @@api_not_found
    GetDataPtr eax, @@aLdrLoadDll
    push eax
    push DWORD PTR hNtDll$[ebp]
    call GetProcedureAddress
    mov  DWORD PTR fnLdrLoadDll$[ebp], eax
    test eax, eax
    je   @@api_not_found
    GetDataPtr eax, @@aLdrUnloadDll
    push eax
    push DWORD PTR hNtDll$[ebp]
    call GetProcedureAddress
    mov  DWORD PTR fnLdrUnloadDll$[ebp], eax
    test eax, eax
    jne  @@check_and_load_dll
@@api_not_found:
    mov  DWORD PTR nExitCode$[ebp], 8007007Fh       ;ERROR_PROC_NOT_FOUND
    jmp  @@exit

@@check_and_load_dll:
    ;check if library is loaded
    mov  eax, DWORD PTR lpData$[ebp]
    add  eax, DWORD PTR [eax] ;add size of struct
    push eax
    call GetModuleBaseAddress
    test eax, eax
    je   @F
    mov  DWORD PTR nExitCode$[ebp], 800704DFh       ;ERROR_ALREADY_INITIALIZED
    jmp  @@exit

@@: ;build UNICODE_STRING
    push ebx
    push edi
    mov  edi, DWORD PTR lpData$[ebp]
    add  edi, DWORD PTR [edi] ;add size of struct
    lea  eax, DWORD PTR sDllNameUS$[ebp]
    mov  DWORD PTR [eax].UNICODE_STRING32.Buffer, edi
    xor  ebx, ebx
@@: cmp  WORD PTR [edi+ebx], 0
    je   @@got_lpszDllNameW_len
    add  ebx, 2
    jmp  @b
@@got_lpszDllNameW_len:
    mov  WORD PTR [eax].UNICODE_STRING32._Length, bx
    mov  WORD PTR [eax].UNICODE_STRING32.MaximumLength, bx
    pop  edi
    pop  ebx

    ;load library
    lea  eax, DWORD PTR hMod$[ebp]
    push eax ;ModuleHandle
    lea  eax, DWORD PTR sDllNameUS$[ebp]
    push eax ;ModuleFileName
    xor  eax, eax
    push eax ;Flags
    push eax ;PathToFile
    call DWORD PTR fnLdrLoadDll$[ebp]
    test eax, 80000000h
    je   @F ;dll loaded
    mov  DWORD PTR nExitCode$[ebp], 80070002h       ;ERROR_FILE_NOT_FOUND
    jmp  @@exit

@@: ;find Initialize function address
    GetDataPtr eax, @@aInitialize
    push eax
    push DWORD PTR hMod$[ebp]
    call GetProcedureAddress
    mov  DWORD PTR fnInitialize$[ebp], eax
    test eax, eax
    jne  @F
@@proc_not_found:
    mov  DWORD PTR nExitCode$[ebp], 8007007Fh       ;ERROR_PROC_NOT_FOUND
    jmp  @@exit_free_dll

@@: ;call initialize
    mov  eax, DWORD PTR lpData$[ebp]
    push eax
    call DWORD PTR fnInitialize$[ebp]     ;call dll's Initialize function (eax will have the return code)
    mov  DWORD PTR nExitCode$[ebp], eax
    test eax, eax
    je   @@exit

@@exit_free_dll:
    push DWORD PTR hMod$[ebp]
    call DWORD PTR fnLdrUnloadDll$[ebp]

@@exit:
    mov  eax, DWORD PTR nExitCode$[ebp]
    add  esp, 40h
    pop  ebp
    ret  4h

ALIGN 4
@@aNtDllDll:
    DB 'n', 0, 't', 0, 'd', 0, 'l', 0, 'l', 0, '.', 0, 'd', 0, 'l', 0, 'l', 0, 0, 0
@@aLdrLoadDll:
    DB "LdrLoadDll", 0
@@aLdrUnloadDll:
    DB "LdrUnloadDll", 0
@@aInitialize:
    DB "Initialize", 0

;---------------------------------------------------------------------------------

_TEXT ENDS

END
