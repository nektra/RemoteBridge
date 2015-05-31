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

#ifndef _NKT_DEFINITIONS_H
#define _NKT_DEFINITIONS_H

#include <windows.h>

//-----------------------------------------------------------

#ifndef NKT_SIZE_T_MAX
  #define NKT_SIZE_T_MAX  ((SIZE_T)(-1))
#endif //NKT_SIZE_T_MAX

#ifndef NKT_ARRAYLEN
  #define NKT_ARRAYLEN(x)           (sizeof(x)/sizeof(x[0]))
#endif //NKT_ARRAYLEN

#define NKT_ALIGN(x, _size)                                \
                        (((x)+((_size)-1)) & (~((_size)-1)))

#define NKT_ALIGN_ULONG(x)                                 \
                        NKT_ALIGN((ULONG)(x), sizeof(ULONG))

#define NKT_ALIGN_ULONGLONG(x)                             \
                NKT_ALIGN((ULONGLONG)(x), sizeof(ULONGLONG))

#if defined _M_IX86
  #define NKT_ALIGN_SIZE_T(x)             NKT_ALIGN_ULONG(x)
#elif defined _M_X64
  #define NKT_ALIGN_SIZE_T(x)         NKT_ALIGN_ULONGLONG(x)
#else
  #error Unsupported platform
#endif //_M_X64

#if defined(_M_X64) || defined(_M_IA64) || defined(_M_AMD64)
  #define NKT_UNALIGNED __unaligned
#else
  #define NKT_UNALIGNED
#endif

//-----------------------------------------------------------

#define NKT_DELETE(x)                     if ((x) != NULL) \
                                 { delete (x); (x) = NULL; }
#define NKT_RELEASE(x)                    if ((x) != NULL) \
                             { (x)->Release(); (x) = NULL; }

//-----------------------------------------------------------

#define _FAC_REMOTECOM                                 0xA34

#define MAKE_REMOTECOM_HRESULT(code)                       \
                         MAKE_HRESULT(1, _FAC_REMOTECOM, code)

/*
#define NKT_DVERR_CannotLoadDatabase         MAKE_REMOTECOM_HRESULT(1)
#define NKT_DVERR_OnlyOneInstance            MAKE_REMOTECOM_HRESULT(2)
#define NKT_DVERR_ProtocolVersionMismatch    MAKE_REMOTECOM_HRESULT(3)
#define NKT_DVERR_InvalidTransportData       MAKE_REMOTECOM_HRESULT(4)
*/
#define NKT_E_NotFound                   HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
#define NKT_E_ArithmeticOverflow         HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW)
#define NKT_E_AlreadyExists              HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)
#define NKT_E_AlreadyInitialized         HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED)
#define NKT_E_Cancelled                  HRESULT_FROM_WIN32(ERROR_CANCELLED)
#define NKT_E_ExceptionRaised            DISP_E_EXCEPTION
#define NKT_E_PartialCopy                HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY)
#define NKT_E_InvalidData                HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
#define NKT_E_IoPending                  HRESULT_FROM_WIN32(ERROR_IO_PENDING)
#define NKT_E_WriteFault                 HRESULT_FROM_WIN32(ERROR_WRITE_FAULT)
#define NKT_E_UnhandledException         HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION)
#define NKT_E_BrokenPipe                 HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE)
#define NKT_E_BufferOverflow             HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW)
#define NKT_E_ArithmeticOverflow         HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW)
#define NKT_E_Timeout                    HRESULT_FROM_WIN32(WAIT_TIMEOUT)
#define NKT_E_Unsupported                HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
#define NKT_E_LimitReached               HRESULT_FROM_WIN32(ERROR_IMPLEMENTATION_LIMIT)

#define NKT_DVERR_LicenseNotAccepted     MAKE_REMOTECOM_HRESULT(1)
/*
#define NKT_DVERR_HookIsInactive             MAKE_REMOTECOM_HRESULT(5)
*/

__inline HRESULT NKT_HRESULT_FROM_WIN32(DWORD dwOsErr)
{
  if (dwOsErr == ERROR_NOT_ENOUGH_MEMORY)
    dwOsErr = ERROR_OUTOFMEMORY;
  return HRESULT_FROM_WIN32(dwOsErr);
}

__inline HRESULT NKT_HRESULT_FROM_LASTERROR()
{
  return NKT_HRESULT_FROM_WIN32(::GetLastError());
}

//-----------------------------------------------------------

#define NKT_PTR_2_ULONGLONG(ptr)  ((ULONGLONG)(SIZE_T)(ptr))

//-----------------------------------------------------------

template <typename varType>
__inline int NKT_CMP(__in varType x, __in varType y)
{
  if (x < y)
    return -1;
  if (x > y)
    return 1;
  return 0;
}

//-----------------------------------------------------------

#endif //_NKT_DEFINITIONS_H
