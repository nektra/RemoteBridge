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

;HRESULT __stdcall IUnknown_Generic...(__in IUnknown *lpThis, ...);
INIT:
_paramsBase$ = 8h + 28h
_rcx$ = 8h + 8h
_lpStackCopy$ = 20h
_hResult$ = 120h

    mov     QWORD PTR [rsp+20h], r9
    mov     QWORD PTR [rsp+18h], r8
    mov     QWORD PTR [rsp+10h], rdx
    mov     QWORD PTR [rsp+8], rcx
    push    rbp
    mov     rbp, rsp
    sub     rsp, 178h + 28h          ;locals + shadow space + return address. Size should be 0x####8h always to mantain 16-byte alignment

    mov     rcx, 224
@@: mov     rax, QWORD PTR _paramsBase$[rbp+rcx-8h]  ;copy 224 bytes from the stack (28 stack parameters should be enough)
    mov     QWORD PTR _lpStackCopy$[rsp+rcx-8h], rax
    sub     rcx, 8
    jne     @b
    ;call original
    mov     rcx, _rcx$[rbp]
@@: mov     rax, 0DEAD0010DEAD0010h  ;address of original proc (iz zero loop until replacement is written)
    test    rax, rax
    je      @b
    call    rax
    mov     QWORD PTR _hResult$[rsp], rax  ;save result

    mov     rcx, 0DEAD0001DEAD0001h  ;lpInterface
    mov     rdx, 0DEAD0002DEAD0002h  ;lpIntMethodData
    lea     r8, _rcx$[rbp]           ;lpParameters
    mov     r9, QWORD PTR _hResult$[rsp] ;nResult
    mov     rax, 0DEAD0003DEAD0003h  ;address of MyIUnknown_Generic
    call    rax

    mov     rax, QWORD PTR _hResult$[rsp]  ;restore result

    add     rsp, 178h + 28h
    pop     rbp
    ret

;---------------------------------------------------------------------------------

_TEXT ENDS

END
