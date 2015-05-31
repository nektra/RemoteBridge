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

#pragma once

#include "resource.h"       // main symbols
#ifdef _WIN64
  #include "RemoteBridge_i64.h"
#else //_WIN64
  #include "RemoteBridge_i.h"
#endif //_WIN64
#include "CustomRegistryMap.h"

//-----------------------------------------------------------
// CNktProcessMemoryImpl

class ATL_NO_VTABLE CNktProcessMemoryImpl : public CComObjectRootEx<CComMultiThreadModel>,
                                            public CComCoClass<CNktProcessMemoryImpl,
                                                               &CLSID_NktRemoteBridge>,
                                            public IObjectSafetyImpl<CNktProcessMemoryImpl,
                                                                     INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
                                            public IDispatchImpl<INktProcessMemory,
                                                                 &IID_INktRemoteBridge,
                                                                 &LIBID_RemoteBridge, 1, 0>
{
public:
  CNktProcessMemoryImpl() : CComObjectRootEx<CComMultiThreadModel>(),
                            CComCoClass<CNktProcessMemoryImpl, &CLSID_NktRemoteBridge>(),
                            IObjectSafetyImpl<CNktProcessMemoryImpl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>(),
                            IDispatchImpl<INktProcessMemory, &IID_INktRemoteBridge,
                                          &LIBID_RemoteBridge, 1, 0>()
    {
    dwPid = 0;
    bIsLocal = FALSE;
    hProc[0] = hProc[1] = NULL;
    bWriteAccessChecked = FALSE;
    return;
    };

  ~CNktProcessMemoryImpl()
    {
    if (hProc[1] != NULL)
      ::CloseHandle(hProc[1]);
    if (hProc[0] != NULL)
      ::CloseHandle(hProc[0]);
    return;
    };

  DECLARE_REGISTRY_RESOURCEID_EX(IDR_INTERFACEREGISTRAR, L"RemoteBridge.NktProcessMemory",
                                 L"1", L"NktProcessMemory Class", CLSID_NktProcessMemory,
                                 LIBID_RemoteBridge, L"Neutral")

  BEGIN_COM_MAP(CNktProcessMemoryImpl)
    COM_INTERFACE_ENTRY(INktProcessMemory)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, cUnkMarshaler.p)
  END_COM_MAP()

  // ISupportsErrorInfo
  STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  DECLARE_GET_CONTROLLING_UNKNOWN()

  HRESULT FinalConstruct()
    {
    return ::CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &(cUnkMarshaler.p));
    };

  void FinalRelease()
    {
    cUnkMarshaler.Release(); 
    return;
    };

public:
  STDMETHOD(GetProtection)(__in my_ssize_t remoteAddr, __out eNktProtection *pVal);

  STDMETHOD(ReadMem)(__in my_ssize_t localAddr, __in my_ssize_t remoteAddr, __in my_ssize_t bytes,
                     __out my_ssize_t *pReaded);

  STDMETHOD(WriteMem)(__in my_ssize_t remoteAddr, __in my_ssize_t localAddr, __in my_ssize_t bytes,
                      __out my_ssize_t *pWritten);

  STDMETHOD(Write)(__in my_ssize_t remoteAddr, __in VARIANT var);

  STDMETHOD(ReadString)(__in my_ssize_t remoteAddr, __in VARIANT_BOOL isAnsi, __out BSTR *pVal);

  STDMETHOD(ReadStringN)(__in my_ssize_t remoteAddr, __in VARIANT_BOOL isAnsi, __in LONG maxChars, __out BSTR *pVal);

  STDMETHOD(WriteString)(__in my_ssize_t remoteAddr, __in BSTR str, __in VARIANT_BOOL asAnsi,
                         __in VARIANT_BOOL writeNul);

  STDMETHOD(StringLength)(__in my_ssize_t remoteAddr, __in VARIANT_BOOL asAnsi, __out LONG *pVal);

  STDMETHOD(get_CharVal)(__in my_ssize_t remoteAddr, __out signed char *pVal);

  STDMETHOD(put_CharVal)(__in my_ssize_t remoteAddr, __in signed char newValue);

  STDMETHOD(get_ByteVal)(__in my_ssize_t remoteAddr, __out unsigned char *pVal);

  STDMETHOD(put_ByteVal)(__in my_ssize_t remoteAddr, __in unsigned char newValue);

  STDMETHOD(get_ShortVal)(__in my_ssize_t remoteAddr, __out short *pVal);

  STDMETHOD(put_ShortVal)(__in my_ssize_t remoteAddr, __in short newValue);

  STDMETHOD(get_UShortVal)(__in my_ssize_t remoteAddr, __out unsigned short *pVal);

  STDMETHOD(put_UShortVal)(__in my_ssize_t remoteAddr, __in unsigned short newValue);

  STDMETHOD(get_LongVal)(__in my_ssize_t remoteAddr, __out long *pVal);

  STDMETHOD(put_LongVal)(__in my_ssize_t remoteAddr, __in long newValue);

  STDMETHOD(get_ULongVal)(__in my_ssize_t remoteAddr, __out unsigned long *pVal);

  STDMETHOD(put_ULongVal)(__in my_ssize_t remoteAddr, __in unsigned long newValue);

  STDMETHOD(get_LongLongVal)(__in my_ssize_t remoteAddr, __out __int64 *pVal);

  STDMETHOD(put_LongLongVal)(__in my_ssize_t remoteAddr, __in __int64 newValue);

  STDMETHOD(get_ULongLongVal)(__in my_ssize_t remoteAddr, __out unsigned __int64 *pVal);

  STDMETHOD(put_ULongLongVal)(__in my_ssize_t remoteAddr, __in unsigned __int64 newValue);

  STDMETHOD(get_SSizeTVal)(__in my_ssize_t remoteAddr, __out my_ssize_t *pVal);

  STDMETHOD(put_SSizeTVal)(__in my_ssize_t remoteAddr, __in my_ssize_t newValue);

  STDMETHOD(get_SizeTVal)(__in my_ssize_t remoteAddr, __out my_size_t *pVal);

  STDMETHOD(put_SizeTVal)(__in my_ssize_t remoteAddr, __in my_size_t newValue);

  STDMETHOD(get_FloatVal)(__in my_ssize_t remoteAddr, __out float *pVal);

  STDMETHOD(put_FloatVal)(__in my_ssize_t remoteAddr, __in float newValue);

  STDMETHOD(get_DoubleVal)(__in my_ssize_t remoteAddr, __out double *pVal);

  STDMETHOD(put_DoubleVal)(__in my_ssize_t remoteAddr, __in double newValue);

  STDMETHOD(AllocMem)(__in my_ssize_t bytes, __in VARIANT_BOOL executeFlag, __out my_ssize_t *pVal);

  STDMETHOD(FreeMem)(__in my_ssize_t remoteAddr);

private:
  friend class CNktRemoteBridgeImpl;

  VOID Initialize(__in DWORD _dwPid)
    {
    dwPid = _dwPid;
    bIsLocal = (dwPid == ::GetCurrentProcessId()) ? TRUE : FALSE;
    return;
    };

  HANDLE GetReadAccessHandle();
  HANDLE GetWriteAccessHandle();

  HRESULT WriteProtected(__in LPVOID lpRemoteDest, __in LPCVOID lpLocalSrc, __in SIZE_T nSize,
                         __out_opt SIZE_T *lpnWritten);

  HRESULT Read(__out LPVOID lpLocalDest, __in LPCVOID lpRemoteSrc, __in SIZE_T nSize,
               __out_opt SIZE_T *lpnReaded);
  HRESULT Write(__in LPVOID lpRemoteDest, __in LPCVOID lpLocalSrc, __in SIZE_T nSize,
                __out_opt SIZE_T *lpnWritten);

  HRESULT ReadStringA(__out LPSTR* lpszDestA, __in LPCVOID lpRemoteSrc,
                      __in SIZE_T nMaxChars=NKT_SIZE_T_MAX);
  HRESULT ReadStringW(__out LPWSTR* lpszDestW, __in LPCVOID lpRemoteSrc,
                      __in SIZE_T nMaxChars=NKT_SIZE_T_MAX);

  HRESULT WriteStringA(__in LPVOID lpRemoteDest, __in_nz_opt LPCSTR szSrcA,
                       __in SIZE_T nSrcLen=NKT_SIZE_T_MAX, __in BOOL bWriteNulTerminator=TRUE);
  HRESULT WriteStringW(__in LPVOID lpRemoteDest, __in_nz_opt LPCWSTR szSrcW,
                       __in SIZE_T nSrcLen=NKT_SIZE_T_MAX, __in BOOL bWriteNulTerminator=TRUE);
  HRESULT GetStringLengthA(__in LPVOID lpRemoteDest, __out SIZE_T *lpnChars);
  HRESULT GetStringLengthW(__in LPVOID lpRemoteDest, __out SIZE_T *lpnChars);

private:
  DWORD dwPid;
  CComPtr<IUnknown> cUnkMarshaler;
  HANDLE volatile hProc[2];
  BOOL bWriteAccessChecked, bIsLocal;
};

OBJECT_ENTRY_NON_CREATEABLE_EX_AUTO(__uuidof(NktProcessMemory), CNktProcessMemoryImpl)
