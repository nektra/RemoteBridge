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

RETURN_N MACRO bytes
ALIGN 16
    mov     eax, DWORD PTR _hResult$[ebp]  ;restore return value
    add     esp, 20h
    pop     ebp
    ret     bytes 
ENDM

;HRESULT __stdcall IUnknown_Generic...(__in IUnknown *lpThis, ...);
INIT:
_paramsBase$ = 8h
_lpOrigStackPtr$ = -4h
_hResult$        = -8h
_nStackBytes$    = -0Ch

    push    ebp
    mov     ebp, esp
    sub     esp, 20h

    mov     DWORD PTR _lpOrigStackPtr$[ebp], esp  ;save stack pointer
    mov     ecx, 128
@@: mov     eax, DWORD PTR _paramsBase$[ebp+ecx-4h]  ;push 128 bytes from the stack (32 parameters should be enough) 
    push    eax
    sub     ecx, 4
    jne     @b
    ;call original
@@: mov     eax, 0DEAD0010h        ;address of original proc (iz zero loop until replacement is written)
    test    eax, eax
    je      @b
    call    eax
    mov     DWORD PTR _hResult$[ebp], eax  ;save result

    ;calculate used stack
    mov     eax, DWORD PTR _lpOrigStackPtr$[ebp]
    sub     eax, esp
    add     esp, eax                 ;restore stack pointer
    ;save the bytes count to add to the stack upon return
    mov     DWORD PTR _nStackBytes$[ebp], 80h
    sub     DWORD PTR _nStackBytes$[ebp], eax

    mov     eax, 0DEAD0001h          ;lpInterface
    push    eax
    mov     eax, 0DEAD0002h          ;lpIntMethodData
    push    eax
    lea     eax, DWORD PTR [ebp+8h]  ;lpParameters
    push    eax
    mov     eax, DWORD PTR _hResult$[ebp] ;nResult
    push    eax
    mov     eax, 0DEAD0003h          ;address of MyIUnknown_Generic
    call    eax

    call    @f
@@: pop     eax
    add     eax, OFFSET @first - $ + 1
    push    ebx
    mov     ebx, DWORD PTR _nStackBytes$[ebp]
    shl     ebx, 2                   ;this should do the align 16
    add     eax, ebx
    pop     ebx
    jmp     eax

ALIGN 16
@first:
    mov     eax, DWORD PTR _hResult$[ebp]  ;restore return value
    add     esp, 20h
    pop     ebp
    ret
    RETURN_N 4h
    RETURN_N 8h
    RETURN_N 0Ch
    RETURN_N 10h
    RETURN_N 14h
    RETURN_N 18h
    RETURN_N 1Ch
    RETURN_N 20h
    RETURN_N 24h
    RETURN_N 28h
    RETURN_N 2Ch
    RETURN_N 30h
    RETURN_N 34h
    RETURN_N 38h
    RETURN_N 3Ch
    RETURN_N 40h
    RETURN_N 44h
    RETURN_N 48h
    RETURN_N 4Ch
    RETURN_N 50h
    RETURN_N 54h
    RETURN_N 58h
    RETURN_N 5Ch
    RETURN_N 60h
    RETURN_N 64h
    RETURN_N 68h
    RETURN_N 6Ch
    RETURN_N 70h
    RETURN_N 74h
    RETURN_N 78h
    RETURN_N 7Ch
    RETURN_N 80h
    RETURN_N 84h
    RETURN_N 88h
    RETURN_N 8Ch

;---------------------------------------------------------------------------------

_TEXT ENDS

END
