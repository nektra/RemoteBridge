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

;See: http://www.japheth.de/ and http://www.ntcore.com/files/vista_x64.htm

.code

_TEXT SEGMENT

;---------------------------------------------------------------------------------

INIT:
    jmp   @@RealInit

INCLUDE GetProcAddress_x64.inc.asm

;---------------------------------------------------------------------------------

@@RealInit:
lpData$ = 70h+28h + 8h
hNtDll$ = 32
fnLdrLoadDll$ = 40
fnLdrUnloadDll$ = 48
hMod$ = 56
nExitCode$ = 64
fnInitialize$ = 72
sDllNameUS$ = 80h ;sizeof(UNICODE_STRING64)=16

    mov  QWORD PTR [rsp+8h], rcx ;save 1st parameter for later use
    sub  rsp, 70h + 28h             ;locals + shadow space + return address. Size should be 0x####8h always to mantain 16-byte alignment

    ;find needed apis
    GetDataPtr @@aNtDllDll
    mov  rcx, rax
    call GetModuleBaseAddress
    mov  QWORD PTR hNtDll$[rsp], rax
    test rax, rax
    je   @@api_not_found
    GetDataPtr @@aLdrLoadDll
    mov  rdx, rax
    mov  rcx, QWORD PTR hNtDll$[rsp]
    call GetProcedureAddress
    mov  QWORD PTR fnLdrLoadDll$[rsp], rax
    test rax, rax
    je   @@api_not_found
    GetDataPtr @@aLdrUnloadDll
    mov  rdx, rax
    mov  rcx, QWORD PTR hNtDll$[rsp]
    call GetProcedureAddress
    mov  QWORD PTR fnLdrUnloadDll$[rsp], rax
    test rax, rax
    jne  @@check_and_load_dll

@@api_not_found:
    mov  DWORD PTR nExitCode$[ebp], 8007007Fh       ;ERROR_PROC_NOT_FOUND
    jmp  @@exit

@@check_and_load_dll:
    ;check if library is loaded
    mov  rcx, QWORD PTR lpData$[rsp]
    xor  rax, rax
    mov  eax, DWORD PTR [rcx]
    add  rcx, rax ;add size of struct
    call GetModuleBaseAddress
    test rax, rax
    je   @F
    mov  DWORD PTR nExitCode$[rsp], 800704DFh       ;ERROR_ALREADY_INITIALIZED
    jmp  @@exit

@@: ;build UNICODE_STRING
    mov  rcx, QWORD PTR lpData$[rsp]
    xor  rax, rax
    mov  eax, DWORD PTR [rcx]
    add  rcx, rax ;add size of struct
    lea  rax, QWORD PTR sDllNameUS$[rsp]
    mov  QWORD PTR [rax].UNICODE_STRING64.Buffer, rcx
    xor  rdx, rdx
@@: cmp  WORD PTR [rcx+rdx], 0
    je   @@got_lpszDllNameW_len
    add  rdx, 2
    jmp  @b
@@got_lpszDllNameW_len:
    mov  WORD PTR [rax].UNICODE_STRING64._Length, dx
    mov  WORD PTR [rax].UNICODE_STRING64.MaximumLength, dx

    ;load library
    xor  rcx, rcx ;PathToFile
    xor  rdx, rdx ;Flags
    lea  r8, QWORD PTR sDllNameUS$[rsp] ;ModuleFileName
    lea  r9, QWORD PTR hMod$[rsp] ;ModuleHandle
    call QWORD PTR fnLdrLoadDll$[rsp]
    test eax, 80000000h
    je   @F ;dll loaded
    mov  DWORD PTR nExitCode$[rsp], 80070002h       ;ERROR_FILE_NOT_FOUND
    jmp  @@exit

@@: ;find Initialize function address
    GetDataPtr @@aInitialize
    mov  rdx, rax
    mov  rcx, QWORD PTR hMod$[rsp]
    call GetProcedureAddress
    mov  QWORD PTR fnInitialize$[rsp], rax
    test rax, rax
    jne  @F
@@proc_not_found:
    mov  DWORD PTR nExitCode$[rsp], 8007007Fh       ;ERROR_PROC_NOT_FOUND
    jmp  @@exit_free_dll

@@: ;call initialize
    mov  rcx, QWORD PTR lpData$[rsp]
    call QWORD PTR fnInitialize$[rsp]     ;call dll's Initialize function (eax will have the return code)
    mov  DWORD PTR nExitCode$[rsp], eax
    test eax, eax
    je   @@exit

@@exit_free_dll:
    mov  rcx, QWORD PTR hMod$[rsp]
    call QWORD PTR fnLdrUnloadDll$[rsp]

@@exit:
    xor  rax, rax
    mov  eax, DWORD PTR nExitCode$[rsp]
    add  rsp, 70h + 28h
    ret

ALIGN 8
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
