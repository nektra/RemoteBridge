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

DATA STRUCT
  env            QWORD ?
  obj_or_class   QWORD ?
  _R8            QWORD ?
  _R9            QWORD ?
  _XMM2          QWORD ?
  _XMM3          QWORD ?
  lp5thParameter QWORD ?
  retValue       QWORD ?
DATA ENDS

EXTRAINFO STRUCT
  returnType     BYTE ?
EXTRAINFO ENDS

;---------------------------------------------------------------------------------

GetDataPtr MACRO
    DB 48h, 8Dh, 05h ; lea rax, [rip + OFFSET INIT - $ - 4]
    DD OFFSET INIT - $ - 4
ENDM

;ReturnType (*fnPtr)(JNIEnv *env, jclass[jobject] obj_or_class, ...); TURNS INTO
;void JNICALL Native_raiseCallback(__in CMethodOrFieldDef* ctx, __in DATA *parameters);

INIT:

STACK_RESERVE equ 80h + 28h ;locals(80h) + shadow-space/return-address(28h). Size should be 0x####8h always to mantain 16-byte alignment
OFFSET_DATA                      equ 0h + 40h
OFFSET_ExtraInfo                 equ 0h
OFFSET_MyNative_raiseCallback    equ 8h
OFFSET_VolatileLong              equ 10h
OFFSET_AddressOfNktAcquireShared equ 18h
OFFSET_AddressOfNktReleaseShared equ 20h
OFFSET_lpThis                    equ 28h

    DQ      0h                        ;call-to-original (used as extrainfo)
    DQ      0h                        ;address of MyNative_raiseCallback
    DQ      0h                        ;volatile long
    DQ      0h                        ;address of NktAcquireShared
    DQ      0h                        ;address of NktReleaseShared
    DQ      0h                        ;lpThis

    ;code begins here
    sub     rsp, STACK_RESERVE

    mov     QWORD PTR DATA.env[rsp+OFFSET_DATA], rcx             ;set env
    mov     QWORD PTR DATA.obj_or_class[rsp+OFFSET_DATA], rdx    ;set object or class
    mov     QWORD PTR DATA._R8[rsp+OFFSET_DATA], r8
    mov     QWORD PTR DATA._R9[rsp+OFFSET_DATA], r9
    movsd   QWORD PTR DATA._XMM2[rsp+OFFSET_DATA], xmm2
    movsd   QWORD PTR DATA._XMM3[rsp+OFFSET_DATA], xmm3
    lea     rax, QWORD PTR [rsp + STACK_RESERVE + 28h]
    mov     QWORD PTR DATA.lp5thParameter[rsp+OFFSET_DATA], rax
    xor     rax, rax
    mov     QWORD PTR DATA.retValue[rsp], rax

    ;enter lock
    GetDataPtr
    lea     rcx, QWORD PTR [rax+OFFSET_VolatileLong]
    call    QWORD PTR [rax+OFFSET_AddressOfNktAcquireShared]
    ;check if MyNative_raiseCallback is not zero and call it
    GetDataPtr
    xor     r9, r9
    lock xadd QWORD PTR [rax+OFFSET_MyNative_raiseCallback], r9
    test    r9, r9
    je      @@nofunction
    GetDataPtr
    mov     rcx, QWORD PTR [rax+OFFSET_lpThis]  ;lpThis
    lea     rdx, QWORD PTR OFFSET_DATA[rsp]
    call    r9

@@nofunction:
    ;leave lock
    GetDataPtr
    lea     rcx, QWORD PTR [rax+OFFSET_VolatileLong]
    call    QWORD PTR [rax+OFFSET_AddressOfNktReleaseShared]

    ;set return value depending on type
    GetDataPtr
    cmp     BYTE PTR EXTRAINFO.returnType[rax+OFFSET_ExtraInfo], 0
    jne     @F
    mov     rax, QWORD PTR DATA.retValue[rsp+OFFSET_DATA]
    jmp     @@done
@@: movsd   xmm0, QWORD PTR DATA.retValue[rsp+OFFSET_DATA]

@@done:
    add     rsp, STACK_RESERVE
    ret

;---------------------------------------------------------------------------------

_TEXT ENDS

END
