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

.486
.model flat, stdcall
.code

_TEXT SEGMENT

;---------------------------------------------------------------------------------

GetDataPtr MACRO
    DB      0E8h, 0h, 0h, 0h, 0h
    pop     eax
    sub     eax, $ - INIT - 1
ENDM

;jint __stdcall CreateJavaVM(JavaVM **pvm, void **penv, void *args);
INIT:

ORIGINAL_PARAM1                  equ 8h
ORIGINAL_PARAM2                  equ 0Ch
ORIGINAL_PARAM3                  equ 10h
OFFSET_CallOriginal              equ 0h
OFFSET_MyCreateJavaVM            equ 4h
OFFSET_VolatileLong              equ 8h
OFFSET_AddressOfNktAcquireShared equ 0Ch
OFFSET_AddressOfNktReleaseShared equ 10h
OFFSET_lpThis                    equ 14h

    DD      0h                        ;call-to-original
    DD      0h                        ;address of MyCreateJavaVM
    DD      0h                        ;volatile long
    DD      0h                        ;address of NktAcquireShared
    DD      0h                        ;address of NktReleaseShared
    DD      0h                        ;lpThis
    ;code begins here
    push    ebp
    mov     ebp, esp

    ;call original
    push    DWORD PTR [ebp+ORIGINAL_PARAM3]  ;args
    push    DWORD PTR [ebp+ORIGINAL_PARAM2]  ;penv
    push    DWORD PTR [ebp+ORIGINAL_PARAM1]  ;pvm
    GetDataPtr
    call    DWORD PTR [eax+OFFSET_CallOriginal]
    cmp     eax, 0                    ;succeeded?
    jl      @exit

    ;save non-volatile registers
    push    eax
    push    ebx
    push    ecx
    push    edx
    push    esi
    push    edi
    ;enter lock
    GetDataPtr
    lea     ebx, DWORD PTR [eax+OFFSET_VolatileLong]
    push    ebx
    call    DWORD PTR [eax+OFFSET_AddressOfNktAcquireShared]
    ;check if MyCreateJavaVM is not zero and call it
    GetDataPtr
    xor     ebx, ebx
    lock xadd DWORD PTR [eax+OFFSET_MyCreateJavaVM], ebx
    test    ebx, ebx
    je      @F
    GetDataPtr
    push    DWORD PTR [ebp+ORIGINAL_PARAM2]     ;penv
    push    DWORD PTR [ebp+ORIGINAL_PARAM1]     ;pvm
    push    DWORD PTR [eax+OFFSET_lpThis]       ;lpThis
    call    ebx
@@: ;leave lock
    GetDataPtr
    lea     ebx, DWORD PTR [eax+OFFSET_VolatileLong]
    push    ebx
    call    DWORD PTR [eax+OFFSET_AddressOfNktReleaseShared]
    ;restore non-volatile registers
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax

@exit:
    pop     ebp
    ret     0Ch

;---------------------------------------------------------------------------------

_TEXT ENDS

END
