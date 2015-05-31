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

;HRESULT __stdcall IUnknown_QueryInterface(__in IUnknown *lpThis, __in REFIID riid, __out void **ppvObject);
INIT:
    sub     rsp, 20h + 28h             ;locals + shadow space + return address. Size should be 0x####8h always to mantain 16-byte alignment
    mov     r9, r8
    mov     r8, rdx
    mov     rdx, rcx
    mov     rcx, 0DEAD0001DEAD0001h    ;lpInterface
    mov     rax, 0DEAD0002DEAD0002h    ;address of MyIUnknown_QueryInterface 
    call    rax
    add     rsp, 20h + 28h
    ret

;---------------------------------------------------------------------------------

_TEXT ENDS

END
