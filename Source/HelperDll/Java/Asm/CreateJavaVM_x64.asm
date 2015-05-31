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

GetDataPtr MACRO
    DB 48h, 8Dh, 05h ; lea rax, [rip + OFFSET INIT - $ - 4]
    DD OFFSET INIT - $ - 4
ENDM

;jint __stdcall CreateJavaVM(JavaVM **pvm, void **penv, void *args);
INIT:

STACK_RESERVE equ 30h + 0h + 28h ;locals(30h) + fifth-parameter(0h) + shadow-space/return-address(28h). Size should be 0x####8h always to mantain 16-byte alignment
ORIGINAL_RCX                     equ STACK_RESERVE + 8h
ORIGINAL_RDX                     equ STACK_RESERVE + 10h
ORIGINAL_R8                      equ STACK_RESERVE + 18h
SAVE_RAX                         equ 0h + 20h
SAVE_RBX                         equ 0h + 28h
OFFSET_CallOriginal              equ 0h
OFFSET_MyCreateJavaVM            equ 8h
OFFSET_VolatileLong              equ 10h
OFFSET_AddressOfNktAcquireShared equ 18h
OFFSET_AddressOfNktReleaseShared equ 20h
OFFSET_lpThis                    equ 28h

    DQ      0h                        ;call-to-original
    DQ      0h                        ;address of MyCreateJavaVM
    DQ      0h                        ;volatile long
    DQ      0h                        ;address of NktAcquireShared
    DQ      0h                        ;address of NktReleaseShared
    DQ      0h                        ;lpThis
    ;code begins here
    mov     QWORD PTR [rsp+8h], rcx   ;save parameters for later use
    mov     QWORD PTR [rsp+10h], rdx
    mov     QWORD PTR [rsp+18h], r8
    sub     rsp, STACK_RESERVE

    ;call original
    GetDataPtr
    call    QWORD PTR [rax+OFFSET_CallOriginal]
    cmp     eax, 0                    ;succeeded?
    jl      @exit

    ;save registers
    mov     QWORD PTR [rsp+SAVE_RAX], rax
    mov     QWORD PTR [rsp+SAVE_RBX], rbx
    ;enter lock
    GetDataPtr
    lea     rcx, QWORD PTR [rax+OFFSET_VolatileLong]
    call    QWORD PTR [rax+OFFSET_AddressOfNktAcquireShared]
    ;check if MyCreateJavaVM is not zero and call it
    GetDataPtr
    xor     rcx, rcx
    lock xadd QWORD PTR [rax+OFFSET_MyCreateJavaVM], rbx
    test    rbx, rbx
    je      @F
    mov     rcx, QWORD PTR [rax+OFFSET_lpThis]  ;lpThis
    mov     rdx, QWORD PTR [rsp+ORIGINAL_RCX]   ;pvm
    mov     r8, QWORD PTR [rsp+ORIGINAL_RDX]   ;penv
    call    rbx
@@: ;leave lock
    GetDataPtr
    lea     rcx, QWORD PTR [rax+OFFSET_VolatileLong]
    call    QWORD PTR [rax+OFFSET_AddressOfNktReleaseShared]
    ;restore registers
    mov     rax, QWORD PTR [rsp+SAVE_RAX]
    mov     rbx, QWORD PTR [rsp+SAVE_RBX]

@exit:
    add     rsp, STACK_RESERVE
    ret

;---------------------------------------------------------------------------------

_TEXT ENDS

END
