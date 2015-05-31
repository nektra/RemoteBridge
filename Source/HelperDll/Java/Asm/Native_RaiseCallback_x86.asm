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

DATA STRUCT
  env            DWORD ?
  obj_or_class   DWORD ?
  lp3rdParameter DWORD ?
  __unnamed1     DWORD ?
  retValue       QWORD ?
DATA ENDS

EXTRAINFO STRUCT
  returnType  BYTE ?
  __unnamed1  BYTE ?
  retJumpSize WORD ?
EXTRAINFO ENDS

;---------------------------------------------------------------------------------

GetDataPtr MACRO
    DB      0E8h, 0h, 0h, 0h, 0h
    pop     eax
    sub     eax, $ - INIT - 1
ENDM

DoReturn MACRO retSize
    pop     ebx
    pop     ebp
    ret     retSize
ENDM

;ReturnType (*fnPtr)(JNIEnv *env, jclass[jobject] obj_or_class, ...); TURNS INTO
;void JNICALL Native_raiseCallback(__in CMethodOrFieldDef* ctx, __in DATA *parameters);

INIT:

ORIGINAL_PARAM1                  equ 8h
ORIGINAL_PARAM2                  equ 0Ch
ORIGINAL_PARAM3                  equ 10h
OFFSET_DATA                      equ -(SIZEOF DATA + 4h) ;4h=push ebx (not standard prolog but needed this way)
OFFSET_ExtraInfo                 equ 0h
OFFSET_MyNative_raiseCallback    equ 4h
OFFSET_VolatileLong              equ 8h
OFFSET_AddressOfNktAcquireShared equ 0Ch
OFFSET_AddressOfNktReleaseShared equ 10h
OFFSET_lpThis                    equ 14h

    DD      0h                        ;call-to-original (used as extrainfo)
    DD      0h                        ;address of MyNative_raiseCallback
    DD      0h                        ;volatile long
    DD      0h                        ;address of NktAcquireShared
    DD      0h                        ;address of NktReleaseShared
    DD      0h                        ;lpThis
    ;code begins here
    push    ebp
    mov     ebp, esp
    push    ebx
    sub     esp, 30h                  ;locals

    mov     eax, DWORD PTR [ebp+ORIGINAL_PARAM1]
    mov     DWORD PTR DATA.env[ebp+OFFSET_DATA], eax             ;set env
    mov     eax, DWORD PTR [ebp+ORIGINAL_PARAM2]
    mov     DWORD PTR DATA.obj_or_class[ebp+OFFSET_DATA], eax    ;set object or class
    lea     eax, DWORD PTR [ebp+ORIGINAL_PARAM3]
    mov     DWORD PTR DATA.lp3rdParameter[ebp+OFFSET_DATA], eax
    xor     eax, eax
    mov     DWORD PTR DATA.retValue[ebp+OFFSET_DATA], eax

    ;enter lock
    GetDataPtr
    lea     ebx, DWORD PTR [eax+OFFSET_VolatileLong]
    push    ebx
    call    DWORD PTR [eax+OFFSET_AddressOfNktAcquireShared]

    ;check if MyNative_raiseCallback is not zero and call it
    GetDataPtr
    xor     ebx, ebx
    lock xadd DWORD PTR [eax+OFFSET_MyNative_raiseCallback], ebx
    test    ebx, ebx
    je      @@nofunction
    lea     eax, DWORD PTR [ebp+OFFSET_DATA]
    push    eax
    GetDataPtr
    push    DWORD PTR [eax+OFFSET_lpThis]       ;lpThis
    call    ebx

@@nofunction:
    ;leave lock
    GetDataPtr
    lea     ebx, DWORD PTR [eax+OFFSET_VolatileLong]
    push    ebx
    call    DWORD PTR [eax+OFFSET_AddressOfNktReleaseShared]

    ;get parameters size
    GetDataPtr
    movzx   ebx, WORD PTR EXTRAINFO.retJumpSize[eax+OFFSET_ExtraInfo]
    add     ebx, retTableStart - INIT
    add     ebx, eax
    ;set return value depending on type
    GetDataPtr
    cmp     BYTE PTR EXTRAINFO.returnType[eax+OFFSET_ExtraInfo], 0
    jne     @F
    mov     eax, DWORD PTR DATA.retValue[ebp+OFFSET_DATA]
    jmp     @@done
@@: cmp     BYTE PTR EXTRAINFO.returnType[eax+OFFSET_ExtraInfo], 1
    jne     @F
    mov     eax, DWORD PTR DATA.retValue[ebp+OFFSET_DATA]
    mov     edx, DWORD PTR DATA.retValue[ebp+OFFSET_DATA+4]
    jmp     @@done
@@: fld     QWORD PTR DATA.retValue[ebp+OFFSET_DATA]

@@done:
    add     esp, 30h
    jmp     ebx
retTableStart:
    DoReturn 08h
    DoReturn 0Ch
    DoReturn 10h
    DoReturn 14h
    DoReturn 18h
    DoReturn 1Ch
    DoReturn 20h
    DoReturn 24h
    DoReturn 28h
    DoReturn 2Ch
    DoReturn 30h
    DoReturn 34h
    DoReturn 38h
    DoReturn 3Ch
    DoReturn 40h
    DoReturn 44h
    DoReturn 48h
    DoReturn 4Ch
    DoReturn 50h
    DoReturn 54h
    DoReturn 58h
    DoReturn 5Ch
    DoReturn 60h
    DoReturn 64h
    DoReturn 68h
    DoReturn 6Ch
    DoReturn 70h
    DoReturn 74h
    DoReturn 78h
    DoReturn 7Ch

;---------------------------------------------------------------------------------

_TEXT ENDS

END
